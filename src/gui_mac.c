/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *				GUI/Motif support by Robert Webb
 *				Macintosh port by Dany St-Amant
 *					      and Axel Kielhorn
 *				Port to MPW by Bernhard Prümmer
 *				Initial Carbon port by Ammon Skidmore
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * NOTE: Comment mentionning FAQ refer to the book:
 *       "Macworld Mac Programming FAQs" from "IDG Books"
 */

/*
 * WARNING: Vim must be able to compile without Carbon
 *	    As the desired minimum requirement are circa System 7
 *	    (I want to run it on my Mac Classic) (Dany)
 */

/*
 * TODO: Change still to merge from the macvim's iDisk
 *
 * error_ga, mch_errmsg, Navigation's changes in gui_mch_browse
 * uses of MenuItemIndex, changes in gui_mch_set_shellsize,
 * ScrapManager error handling.
 * Comments about function remaining to Carbonize.
 *
 */

/* TODO: find the best place for this (Dany) */
#if 0
#  if ! TARGET_API_MAC_CARBON
/* Enable the new API functions even when not compiling for Carbon */
/* Apple recomends Universal Interface 3.3.2 or later */
#  define OPAQUE_TOOLBOX_STRUCTS		1
#  define ACCESSOR_CALLS_ARE_FUNCTIONS	1
/* Help Menu not supported by Carbon */
#  define USE_HELPMENU
# endif
#endif

#include <Devices.h> /* included first to avoid CR problems */
#include "vim.h"

/* Enable Contextual Menu Support */
#if UNIVERSAL_INTERFACES_VERSION >= 0x0320
# define USE_CTRLCLICKMENU
#endif

/* Put Vim Help in MacOS Help */
#define USE_HELPMENU

/* Enable AEVENT */
#define USE_AEVENT

/* Compile as CodeWarior External Editor */
#if defined(FEAT_CW_EDITOR) && !defined(USE_AEVENT)
# define USE_AEVENT /* Need Apple Event Support */
#endif

/* The VIM creator is CodeWarior specific */
#if !(defined(__MRC__) || defined(__SC__) || defined(__APPLE_CC__))
# define USE_VIM_CREATOR_ID
#else
# if 0 /* Was this usefull for some compiler? (Dany) */
static    OSType	_fcreator = 'VIM!';
static    OSType	_ftype = 'TEXT';
# endif
#endif

/* Vim's Scrap flavor. */
#define VIMSCRAPFLAVOR 'VIM!'

/* CARBON version only tested with Project Builder under MacOS X */
#undef USE_CARBONIZED
#if (defined(__APPLE_CC__) || defined(__MRC__)) && defined(TARGET_API_MAC_CARBON)
# if TARGET_API_MAC_CARBON
#  define USE_CARBONIZED
# endif
#endif

#undef USE_MOUSEWHEEL
#if defined(MACOS_X) && defined(USE_CARBONIZED)
# define USE_MOUSEWHEEL
static EventHandlerUPP mouseWheelHandlerUPP = NULL;
#endif

/* Debugging feature: start Vim window OFFSETed */
#undef USE_OFFSETED_WINDOW

/* Debugging feature: use CodeWarior SIOUX */
#undef USE_SIOUX


/* Include some file. TODO: move into os_mac.h */
#include <Menus.h>
#include <Resources.h>
#if !TARGET_API_MAC_CARBON
#include <StandardFile.h>
#include <Traps.h>
#endif
#include <Balloons.h>
#include <Processes.h>
#ifdef USE_AEVENT
# include <AppleEvents.h>
# include <AERegistry.h>
#endif
#ifdef USE_CTRLCLICKMENU
# include <Gestalt.h>
#endif
#ifdef USE_SIOUX
# include <stdio.h>
# include <sioux.h>
# include <console.h>
#endif
#if UNIVERSAL_INTERFACES_VERSION >= 0x0330
# include <ControlDefinitions.h>
# include <Navigation.h>  /* Navigation only part of ?? */
#endif

#if TARGET_API_MAC_CARBON && 0
/* New Help Interface for Mac, not implemented yet.*/
# include <MacHelp.h>
#endif

/*
 * Translate new name to old ones
 * New function only available in MacOS 8.5,
 * So use old one to be compatible back to System 7
 */
#ifndef USE_CARBONIZED
# undef  EnableMenuItem
# define EnableMenuItem EnableItem
# undef  DisableMenuItem
# define DisableMenuItem DisableItem
#endif

/* Carbon does not support the Get/SetControll functions,
 * use Get/SetControl32Bit instead and rename for non-carbon
 * systems.
 */

#ifndef USE_CARBONIZED
# undef    SetControl32BitMaximum
# define   SetControl32BitMaximum SetControlMaximum
# undef    SetControl32BitMinimum
# define   SetControl32BitMinimum SetControlMinimum
# undef    SetControl32BitValue
# define   SetControl32BitValue SetControlValue
# undef    GetControl32BitValue
# define   GetControl32BitValue GetControlValue
#endif

/*
 * ???
 */

#define kNothing 0
#define kCreateEmpty 2 /*1*/
#define kCreateRect 2
#define kDestroy 3

/*
 * Dany: Don't like those...
 */

#define topLeft(r)	(((Point*)&(r))[0])
#define botRight(r)	(((Point*)&(r))[1])


/* Time of last mouse click, to detect double-click */
static long lastMouseTick = 0;

/* ??? */
static RgnHandle cursorRgn;
static RgnHandle dragRgn;
static Rect dragRect;
static short dragRectEnbl;
static short dragRectControl;

/* This variable is set when waiting for an event, which is the only moment
 * scrollbar dragging can be done directly.  It's not allowed while commands
 * are executed, because it may move the cursor and that may cause unexpected
 * problems (e.g., while ":s" is working).
 */
static int allow_scrollbar = FALSE;

/* Last mouse click caused contextual menu, (to provide proper release) */
#ifdef USE_CTRLCLICKMENU
static short clickIsPopup;
#endif

/* Feedback Action for Scrollbar */
ControlActionUPP gScrollAction;
ControlActionUPP gScrollDrag;

/* Keeping track of which scrollbar is being dragged */
static ControlHandle dragged_sb = NULL;

#if defined(USE_CARBONIZED) && defined(MACOS_X)
static struct
{
    FMFontFamily family;
    FMFontSize size;
    FMFontStyle style;
    Boolean isPanelVisible;
} gFontPanelInfo = { 0, 0, 0, false };
#endif

/*
 * The Quickdraw global is predefined in CodeWarior
 * but is not in Apple MPW
 */
#if (defined(__MRC__) || defined(__SC__))
# if !(defined(TARGET_API_MAC_CARBON) && TARGET_API_MAC_CARBON)
QDGlobals qd;
# endif
#endif

/* Colors Macros */
#define RGB(r,g,b)	((r) << 16) + ((g) << 8) + (b)
#define Red(c)		((c & 0x00FF0000) >> 16)
#define Green(c)	((c & 0x0000FF00) >>  8)
#define Blue(c)		((c & 0x000000FF) >>  0)

/* Key mapping */

#define vk_Esc		0x35	/* -> 1B */

#define vk_F1		0x7A	/* -> 10 */
#define vk_F2		0x78  /*0x63*/
#define vk_F3		0x63  /*0x76*/
#define vk_F4		0x76  /*0x60*/
#define vk_F5		0x60  /*0x61*/
#define vk_F6		0x61  /*0x62*/
#define vk_F7		0x62  /*0x63*/  /*?*/
#define vk_F8		0x64
#define vk_F9		0x65
#define vk_F10		0x6D
#define vk_F11		0x67
#define vk_F12		0x6F
#define vk_F13		0x69
#define vk_F14		0x6B
#define vk_F15		0x71

#define vk_Clr		0x47	/* -> 1B (ESC) */
#define vk_Enter	0x4C	/* -> 03 */

#define vk_Space	0x31	/* -> 20 */
#define vk_Tab		0x30	/* -> 09 */
#define vk_Return	0x24	/* -> 0D */
/* This is wrong for OSX, what is it for? */
#define vk_Delete	0X08	/* -> 08 BackSpace */

#define vk_Help		0x72	/* -> 05 */
#define vk_Home		0x73	/* -> 01 */
#define	vk_PageUp	0x74	/* -> 0D */
#define vk_FwdDelete	0x75	/* -> 7F */
#define	vk_End		0x77	/* -> 04 */
#define vk_PageDown	0x79	/* -> 0C */

#define vk_Up		0x7E	/* -> 1E */
#define vk_Down		0x7D	/* -> 1F */
#define	vk_Left		0x7B	/* -> 1C */
#define vk_Right	0x7C	/* -> 1D */

#define vk_Undo		vk_F1
#define vk_Cut		vk_F2
#define	vk_Copy		vk_F3
#define	vk_Paste	vk_F4
#define vk_PrintScreen	vk_F13
#define vk_SCrollLock	vk_F14
#define	vk_Pause	vk_F15
#define	vk_NumLock	vk_Clr
#define vk_Insert	vk_Help

#define KeySym	char

static struct
{
    KeySym  key_sym;
    char_u  vim_code0;
    char_u  vim_code1;
} special_keys[] =
{
    {vk_Up,		'k', 'u'},
    {vk_Down,		'k', 'd'},
    {vk_Left,		'k', 'l'},
    {vk_Right,		'k', 'r'},

    {vk_F1,		'k', '1'},
    {vk_F2,		'k', '2'},
    {vk_F3,		'k', '3'},
    {vk_F4,		'k', '4'},
    {vk_F5,		'k', '5'},
    {vk_F6,		'k', '6'},
    {vk_F7,		'k', '7'},
    {vk_F8,		'k', '8'},
    {vk_F9,		'k', '9'},
    {vk_F10,		'k', ';'},

    {vk_F11,		'F', '1'},
    {vk_F12,		'F', '2'},
    {vk_F13,		'F', '3'},
    {vk_F14,		'F', '4'},
    {vk_F15,		'F', '5'},

/*  {XK_Help,		'%', '1'}, */
/*  {XK_Undo,		'&', '8'}, */
/*  {XK_BackSpace,	'k', 'b'}, */
#ifndef MACOS_X
    {vk_Delete,		'k', 'b'},
#endif
    {vk_Insert,		'k', 'I'},
    {vk_FwdDelete,	'k', 'D'},
    {vk_Home,		'k', 'h'},
    {vk_End,		'@', '7'},
/*  {XK_Prior,		'k', 'P'}, */
/*  {XK_Next,		'k', 'N'}, */
/*  {XK_Print,		'%', '9'}, */

    {vk_PageUp,		'k', 'P'},
    {vk_PageDown,	'k', 'N'},

    /* End of list marker: */
    {(KeySym)0,		0, 0}
};

/*
 * ------------------------------------------------------------
 * Forward declaration (for those needed)
 * ------------------------------------------------------------
 */

#ifdef USE_AEVENT
OSErr HandleUnusedParms(const AppleEvent *theAEvent);
#endif

/*
 * ------------------------------------------------------------
 * Conversion Utility
 * ------------------------------------------------------------
 */

/*
 * C2Pascal_save
 *
 * Allocate memory and convert the C-String passed in
 * into a pascal string
 *
 */

char_u *C2Pascal_save(char_u *Cstring)
{
    char_u  *PascalString;
    int	    len;

    if (Cstring == NULL)
	return NULL;

    len = STRLEN(Cstring);

    if (len > 255) /* Truncate if necessary */
	len = 255;

    PascalString = alloc(len + 1);
    if (PascalString != NULL)
    {
	mch_memmove(PascalString + 1, Cstring, len);
	PascalString[0] = len;
    }

    return PascalString;
}

/*
 * C2Pascal_save_and_remove_backslash
 *
 * Allocate memory and convert the C-String passed in
 * into a pascal string. Also remove the backslash at the same time
 *
 */

char_u *C2Pascal_save_and_remove_backslash(char_u *Cstring)
{
    char_u  *PascalString;
    int	    len;
    char_u  *p, *c;

    len = STRLEN(Cstring);

    if (len > 255) /* Truncate if necessary */
	len = 255;

    PascalString = alloc(len + 1);
    if (PascalString != NULL)
    {
	for (c = Cstring, p = PascalString+1, len = 0; (*c != 0) && (len < 255); c++)
	{
	    if ((*c == '\\') && (c[1] != 0))
	    {
		c++;
	    }
	    *p = *c;
	    p++;
	    len++;
	}
	PascalString[0] = len;
    }

    return PascalString;
}

/*
 * Convert the modifiers of an Event into vim's modifiers (mouse)
 */

    int_u
EventModifiers2VimMouseModifiers(EventModifiers macModifiers)
{
    int_u vimModifiers = 0x00;

    if (macModifiers & (shiftKey | rightShiftKey))
	vimModifiers |= MOUSE_SHIFT;
    if (macModifiers & (controlKey | rightControlKey))
	vimModifiers |= MOUSE_CTRL;
    if (macModifiers & (optionKey | rightOptionKey))
	vimModifiers |= MOUSE_ALT;
#if 0
    /* Not yet supported */
    if (macModifiers & (cmdKey)) /* There's no rightCmdKey */
	vimModifiers |= MOUSE_CMD;
#endif
    return (vimModifiers);
}

/*
 * Convert the modifiers of an Event into vim's modifiers (keys)
 */

    static int_u
EventModifiers2VimModifiers(EventModifiers macModifiers)
{
    int_u vimModifiers = 0x00;

    if (macModifiers & (shiftKey | rightShiftKey))
	vimModifiers |= MOD_MASK_SHIFT;
    if (macModifiers & (controlKey | rightControlKey))
	vimModifiers |= MOD_MASK_CTRL;
    if (macModifiers & (optionKey | rightOptionKey))
	vimModifiers |= MOD_MASK_ALT;
#ifdef USE_CMD_KEY
    if (macModifiers & (cmdKey)) /* There's no rightCmdKey */
	vimModifiers |= MOD_MASK_CMD;
#endif
    return (vimModifiers);
}

/* Convert a string representing a point size into pixels. The string should
 * be a positive decimal number, with an optional decimal point (eg, "12", or
 * "10.5"). The pixel value is returned, and a pointer to the next unconverted
 * character is stored in *end. The flag "vertical" says whether this
 * calculation is for a vertical (height) size or a horizontal (width) one.
 *
 * From gui_w48.c
 */
    static int
points_to_pixels(char_u *str, char_u **end, int vertical)
{
    int		pixels;
    int		points = 0;
    int		divisor = 0;

    while (*str)
    {
	if (*str == '.' && divisor == 0)
	{
	    /* Start keeping a divisor, for later */
	    divisor = 1;
	    continue;
	}

	if (!isdigit(*str))
	    break;

	points *= 10;
	points += *str - '0';
	divisor *= 10;

	++str;
    }

    if (divisor == 0)
	divisor = 1;

    pixels = points/divisor;
    *end = str;
    return pixels;
}

/*
 * Convert a list of FSSpec aliases into a list of fullpathname
 * character strings.
 */

char_u **new_fnames_from_AEDesc(AEDesc *theList, long *numFiles, OSErr *error)
{
    char_u	**fnames = NULL;
    OSErr	newError;
    long	fileCount;
    FSSpec	fileToOpen;
    long	actualSize;
    AEKeyword	dummyKeyword;
    DescType	dummyType;

    /* Get number of files in list */
    *error = AECountItems(theList, numFiles);
    if (*error)
    {
#ifdef USE_SIOUX
	printf("fname_from_AEDesc: AECountItems error: %d\n", error);
#endif
	return(fnames);
    }

    /* Allocate the pointer list */
    fnames = (char_u **) alloc(*numFiles * sizeof(char_u *));

    /* Empty out the list */
    for (fileCount = 0; fileCount < *numFiles; fileCount++)
	fnames[fileCount] = NULL;

    /* Scan the list of FSSpec */
    for (fileCount = 1; fileCount <= *numFiles; fileCount++)
    {
	/* Get the alias for the nth file, convert to an FSSpec */
	newError = AEGetNthPtr(theList, fileCount, typeFSS,
				&dummyKeyword, &dummyType,
				(Ptr) &fileToOpen, sizeof(FSSpec), &actualSize);
	if (newError)
	{
	    /* Caller is able to clean up */
	    /* TODO: Should be clean up or not? For safety. */
#ifdef USE_SIOUX
	    printf("aevt_odoc: AEGetNthPtr error: %d\n", newError);
#endif
	    return(fnames);
	}

	/* Convert the FSSpec to a pathname */
	fnames[fileCount - 1] = FullPathFromFSSpec_save(fileToOpen);
    }

    return (fnames);
}

/*
 * ------------------------------------------------------------
 * CodeWarrior External Editor Support
 * ------------------------------------------------------------
 */
#ifdef FEAT_CW_EDITOR

/*
 * Handle the Window Search event from CodeWarrior
 *
 * Description
 * -----------
 *
 * The IDE sends the Window Search AppleEvent to the editor when it
 * needs to know whether a particular file is open in the editor.
 *
 * Event Reply
 * -----------
 *
 * None. Put data in the location specified in the structure received.
 *
 * Remarks
 * -------
 *
 * When the editor receives this event, determine whether the specified
 * file is open. If it is, return the modification date/time for that file
 * in the appropriate location specified in the structure. If the file is
 * not opened, put the value fnfErr(file not found) in that location.
 *
 */

#if defined(__MWERKS__)  /* only in Codewarrior */
# pragma options align=mac68k
#endif
typedef struct WindowSearch WindowSearch;
struct WindowSearch /* for handling class 'KAHL', event 'SRCH', keyDirectObject typeChar*/
{
    FSSpec theFile; // identifies the file
    long *theDate; // where to put the modification date/time
};
#if defined(__MWERKS__)  /* only in Codewarrior */
# pragma options align=reset
#endif

    pascal OSErr
Handle_KAHL_SRCH_AE(const AppleEvent *theAEvent, AppleEvent *theReply, long refCon)
{
    OSErr	error = noErr;
    buf_T	*buf;
    int		foundFile = false;
    DescType	typeCode;
    WindowSearch SearchData;
    Size	actualSize;

    error = AEGetParamPtr(theAEvent, keyDirectObject, typeChar, &typeCode, (Ptr) &SearchData, sizeof(WindowSearch), &actualSize);
    if (error)
    {
#ifdef USE_SIOUX
	printf("KAHL_SRCH: AEGetParamPtr error: %d\n", error);
#endif
	return(error);
    }

    error = HandleUnusedParms(theAEvent);
    if (error)
    {
#ifdef USE_SIOUX
	printf("KAHL_SRCH: HandleUnusedParms error: %d\n", error);
#endif
	return(error);
    }

    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	if (buf->b_ml.ml_mfp != NULL
		&& SearchData.theFile.parID == buf->b_FSSpec.parID
		&& SearchData.theFile.name[0] == buf->b_FSSpec.name[0]
		&& STRNCMP(SearchData.theFile.name, buf->b_FSSpec.name, buf->b_FSSpec.name[0] + 1) == 0)
	    {
		foundFile = true;
		break;
	    }

    if (foundFile == false)
	*SearchData.theDate = fnfErr;
    else
	*SearchData.theDate = buf->b_mtime;

#ifdef USE_SIOUX
    printf("KAHL_SRCH: file \"%#s\" {%d}", SearchData.theFile.name,SearchData.theFile.parID);
    if (foundFile == false)
	printf(" NOT");
    printf(" found. [date %lx, %lx]\n", *SearchData.theDate, buf->b_mtime_read);
#endif

    return error;
};

/*
 * Handle the Modified (from IDE to Editor) event from CodeWarrior
 *
 * Description
 * -----------
 *
 * The IDE sends this event to the external editor when it wants to
 * know which files that are open in the editor have been modified.
 *
 * Parameters   None.
 * ----------
 *
 * Event Reply
 * -----------
 * The reply for this event is:
 *
 * keyDirectObject typeAEList required
 *  each element in the list is a structure of typeChar
 *
 * Remarks
 * -------
 *
 * When building the reply event, include one element in the list for
 * each open file that has been modified.
 *
 */

#if defined(__MWERKS__)  /* only in Codewarrior */
# pragma options align=mac68k
#endif
typedef struct ModificationInfo ModificationInfo;
struct ModificationInfo /* for replying to class 'KAHL', event 'MOD ', keyDirectObject typeAEList*/
{
    FSSpec theFile; // identifies the file
    long theDate; // the date/time the file was last modified
    short saved; // set this to zero when replying, unused
};
#if defined(__MWERKS__)  /* only in Codewarrior */
# pragma options align=reset
#endif

    pascal OSErr
Handle_KAHL_MOD_AE(const AppleEvent *theAEvent, AppleEvent *theReply, long refCon)
{
    OSErr	error = noErr;
    AEDescList	replyList;
    long	numFiles;
    ModificationInfo theFile;
    buf_T	*buf;

    theFile.saved = 0;

    error = HandleUnusedParms(theAEvent);
    if (error)
    {
#ifdef USE_SIOUX
	printf("KAHL_MOD: HandleUnusedParms error: %d\n", error);
#endif
	return(error);
    }

    /* Send the reply */
/*  replyObject.descriptorType = typeNull;
    replyObject.dataHandle     = nil;*/

/* AECreateDesc(typeChar, (Ptr)&title[1], title[0], &data) */
    error = AECreateList(nil, 0, false, &replyList);
    if (error)
    {
#ifdef USE_SIOUX
	printf("KAHL_MOD: AECreateList error: %d\n", error);
#endif
	return(error);
    }

#if 0
    error = AECountItems(&replyList, &numFiles);
#ifdef USE_SIOUX
    printf("KAHL_MOD ReplyList: %x %x\n", replyList.descriptorType, replyList.dataHandle);
    printf("KAHL_MOD ItemInList: %d\n", numFiles);
#endif

    /* AEPutKeyDesc(&replyList, keyAEPnject, &aDesc)
     * AEPutKeyPtr(&replyList, keyAEPosition, typeChar, (Ptr)&theType,
     * sizeof(DescType))
     */

    /* AEPutDesc */
#endif

    numFiles = 0;
    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	if (buf->b_ml.ml_mfp != NULL)
	{
	    /* Add this file to the list */
	    theFile.theFile = buf->b_FSSpec;
	    theFile.theDate = buf->b_mtime;
/*	    theFile.theDate = time(NULL) & (time_t) 0xFFFFFFF0; */
	    error = AEPutPtr(&replyList, numFiles, typeChar, (Ptr) &theFile, sizeof(theFile));
#ifdef USE_SIOUX
	    if (numFiles == 0)
		printf("KAHL_MOD: ");
	    else
		printf(", ");
	    printf("\"%#s\" {%d} [date %lx, %lx]", theFile.theFile.name, theFile.theFile.parID, theFile.theDate, buf->b_mtime_read);
	    if (error)
		printf(" (%d)", error);
	    numFiles++;
#endif
	};

#ifdef USE_SIOUX
    printf("\n");
#endif

#if 0
    error = AECountItems(&replyList, &numFiles);
#ifdef USE_SIOUX
    printf("KAHL_MOD ItemInList: %d\n", numFiles);
#endif
#endif

    /* We can add data only if something to reply */
    error = AEPutParamDesc(theReply, keyDirectObject, &replyList);

#ifdef USE_SIOUX
    if (error)
	printf("KAHL_MOD: AEPutParamDesc error: %d\n", error);
#endif

    if (replyList.dataHandle)
	AEDisposeDesc(&replyList);

    return error;
};

/*
 * Handle the Get Text event from CodeWarrior
 *
 * Description
 * -----------
 *
 * The IDE sends the Get Text AppleEvent to the editor when it needs
 * the source code from a file. For example, when the user issues a
 * Check Syntax or Compile command, the compiler needs access to
 * the source code contained in the file.
 *
 * Event Reply
 * -----------
 *
 * None. Put data in locations specified in the structure received.
 *
 * Remarks
 * -------
 *
 * When the editor receives this event, it must set the size of the handle
 * in theText to fit the data in the file. It must then copy the entire
 * contents of the specified file into the memory location specified in
 * theText.
 *
 */

#if defined(__MWERKS__)  /* only in Codewarrior */
# pragma options align=mac68k
#endif
typedef struct CW_GetText CW_GetText;
struct CW_GetText /* for handling class 'KAHL', event 'GTTX', keyDirectObject typeChar*/
{
    FSSpec theFile; /* identifies the file */
    Handle theText; /* the location where you return the text (must be resized properly) */
    long *unused;   /* 0 (not used) */
    long *theDate;  /* where to put the modification date/time */
};
#if defined(__MWERKS__)  /* only in Codewarrior */
# pragma options align=reset
#endif

    pascal OSErr
Handle_KAHL_GTTX_AE(const AppleEvent *theAEvent, AppleEvent *theReply, long refCon)
{
    OSErr	error = noErr;
    buf_T	*buf;
    int		foundFile = false;
    DescType	typeCode;
    CW_GetText	GetTextData;
    Size	actualSize;
    char_u	*line;
    char_u	*fullbuffer = NULL;
    long	linesize;
    long	lineStart;
    long	BufferSize;
    long	lineno;

    error = AEGetParamPtr(theAEvent, keyDirectObject, typeChar, &typeCode, (Ptr) &GetTextData, sizeof(GetTextData), &actualSize);

    if (error)
    {
#ifdef USE_SIOUX
	printf("KAHL_GTTX: AEGetParamPtr error: %d\n", error);
#endif
	return(error);
    }

    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	if (buf->b_ml.ml_mfp != NULL)
	    if (GetTextData.theFile.parID == buf->b_FSSpec.parID)
	    {
		foundFile = true;
		break;
	    }

    if (foundFile)
    {
	BufferSize = 0; /* GetHandleSize(GetTextData.theText); */
	for (lineno = 0; lineno <= buf->b_ml.ml_line_count; lineno++)
	{
	    /* Must use the right buffer */
	    line = ml_get_buf(buf, (linenr_T) lineno, FALSE);
	    linesize = STRLEN(line) + 1;
	    lineStart = BufferSize;
	    BufferSize += linesize;
	    /* Resize handle to linesize+1 to include the linefeed */
	    SetHandleSize(GetTextData.theText, BufferSize);
	    if (GetHandleSize(GetTextData.theText) != BufferSize)
	    {
	#ifdef USE_SIOUX
		printf("KAHL_GTTX: SetHandleSize increase: %d, size %d\n",
			linesize, BufferSize);
	#endif
		break; /* Simple handling for now */
	    }
	    else
	    {
		HLock(GetTextData.theText);
		fullbuffer = (char_u *) *GetTextData.theText;
		STRCPY((char_u *)(fullbuffer + lineStart), line);
		fullbuffer[BufferSize-1] = '\r';
		HUnlock(GetTextData.theText);
	    }
	}
	if (fullbuffer != NULL)
	{
	    HLock(GetTextData.theText);
	    fullbuffer[BufferSize-1] = 0;
	    HUnlock(GetTextData.theText);
	}
	if (foundFile == false)
	    *GetTextData.theDate = fnfErr;
	else
/*	    *GetTextData.theDate = time(NULL) & (time_t) 0xFFFFFFF0;*/
	    *GetTextData.theDate = buf->b_mtime;
    }
#ifdef USE_SIOUX
    printf("KAHL_GTTX: file \"%#s\" {%d} [date %lx, %lx]", GetTextData.theFile.name, GetTextData.theFile.parID, *GetTextData.theDate, buf->b_mtime_read);
    if (foundFile == false)
	printf(" NOT");
    printf(" found. (BufferSize = %d)\n", BufferSize);
#endif

    error = HandleUnusedParms(theAEvent);
    if (error)
    {
#ifdef USE_SIOUX
	printf("KAHL_GTTX: HandleUnusedParms error: %d\n", error);
#endif
	return(error);
    }

    return(error);
}

/*
 *
 */

/* Taken from MoreAppleEvents:ProcessHelpers*/
pascal	OSErr	FindProcessBySignature(const OSType targetType,
					const OSType targetCreator,
					      ProcessSerialNumberPtr psnPtr)
{
    OSErr	anErr = noErr;
    Boolean	lookingForProcess = true;

    ProcessInfoRec  infoRec;

    infoRec.processInfoLength = sizeof(ProcessInfoRec);
    infoRec.processName = nil;
    infoRec.processAppSpec = nil;

    psnPtr->lowLongOfPSN = kNoProcess;
    psnPtr->highLongOfPSN = kNoProcess;

    while (lookingForProcess)
    {
	anErr = GetNextProcess(psnPtr);
	if (anErr != noErr)
	    lookingForProcess = false;
	else
	{
	    anErr = GetProcessInformation(psnPtr, &infoRec);
	    if ((anErr == noErr)
		    && (infoRec.processType == targetType)
		    && (infoRec.processSignature == targetCreator))
		lookingForProcess = false;
	}
    }

    return anErr;
}//end FindProcessBySignature

    void
Send_KAHL_MOD_AE(buf_T *buf)
{
    OSErr	anErr = noErr;
    AEDesc	targetAppDesc = { typeNull, nil };
    ProcessSerialNumber	    psn = { kNoProcess, kNoProcess };
    AppleEvent	theReply = { typeNull, nil };
    AESendMode	sendMode;
    AppleEvent  theEvent = {typeNull, nil };
    AEIdleUPP   idleProcUPP = nil;
    ModificationInfo ModData;


    anErr = FindProcessBySignature('APPL', 'CWIE', &psn);
#ifdef USE_SIOUX
    printf("CodeWarrior is");
    if (anErr != noErr)
	printf(" NOT");
    printf(" running\n");
#endif
    if (anErr == noErr)
    {
	anErr = AECreateDesc(typeProcessSerialNumber, &psn,
			      sizeof(ProcessSerialNumber), &targetAppDesc);

	if (anErr == noErr)
	{
	    anErr = AECreateAppleEvent( 'KAHL', 'MOD ', &targetAppDesc,
					kAutoGenerateReturnID, kAnyTransactionID, &theEvent);
	}

	AEDisposeDesc(&targetAppDesc);

	/* Add the parms */
	ModData.theFile = buf->b_FSSpec;
	ModData.theDate = buf->b_mtime;

	if (anErr == noErr)
	    anErr = AEPutParamPtr(&theEvent, keyDirectObject, typeChar, &ModData, sizeof(ModData));

	if (idleProcUPP == nil)
	    sendMode = kAENoReply;
	else
	    sendMode = kAEWaitReply;

	if (anErr == noErr)
	    anErr = AESend(&theEvent, &theReply, sendMode, kAENormalPriority, kNoTimeOut, idleProcUPP, nil);
	if (anErr == noErr  &&  sendMode == kAEWaitReply)
	{
#ifdef USE_SIOUX
	    printf("KAHL_MOD: Send error: %d\n", anErr);
#endif
/*	    anErr =  AEHGetHandlerError(&theReply);*/
	}
	(void) AEDisposeDesc(&theReply);
    }
}
#endif /* FEAT_CW_EDITOR */

/*
 * ------------------------------------------------------------
 * Apple Event Handling procedure
 * ------------------------------------------------------------
 */
#ifdef USE_AEVENT

/*
 * Handle the Unused parms of an AppleEvent
 */

    OSErr
HandleUnusedParms(const AppleEvent *theAEvent)
{
    OSErr	error;
    long	actualSize;
    DescType	dummyType;
    AEKeyword	missedKeyword;

    /* Get the "missed keyword" attribute from the AppleEvent. */
    error = AEGetAttributePtr(theAEvent, keyMissedKeywordAttr,
			      typeKeyword, &dummyType,
			      (Ptr)&missedKeyword, sizeof(missedKeyword),
			      &actualSize);

    /* If the descriptor isn't found, then we got the required parameters. */
    if (error == errAEDescNotFound)
    {
	error = noErr;
    }
    else
    {
#if 0
	/* Why is this removed? */
	error = errAEEventNotHandled;
#endif
    }

    return error;
}


/*
 * Handle the ODoc AppleEvent
 *
 * Deals with all files dragged to the application icon.
 *
 */

#if defined(__MWERKS__)  /* only in Codewarrior */
# pragma options align=mac68k
#endif
typedef struct SelectionRange SelectionRange;
struct SelectionRange /* for handling kCoreClassEvent:kOpenDocuments:keyAEPosition typeChar */
{
    short unused1; // 0 (not used)
    short lineNum; // line to select (<0 to specify range)
    long startRange; // start of selection range (if line < 0)
    long endRange; // end of selection range (if line < 0)
    long unused2; // 0 (not used)
    long theDate; // modification date/time
};
#if defined(__MWERKS__)  /* only in Codewarrior */
# pragma options align=reset
#endif

/* The IDE uses the optional keyAEPosition parameter to tell the ed-
   itor the selection range. If lineNum is zero or greater, scroll the text
   to the specified line. If lineNum is less than zero, use the values in
   startRange and endRange to select the specified characters. Scroll
   the text to display the selection. If lineNum, startRange, and
   endRange are all negative, there is no selection range specified.
 */

    pascal OSErr
HandleODocAE(const AppleEvent *theAEvent, AppleEvent *theReply, long refCon)
{
    /*
     * TODO: Clean up the code with convert the AppleEvent into
     *       a ":args"
     */
    OSErr	error = noErr;
//    OSErr	firstError = noErr;
//    short	numErrors = 0;
    AEDesc	theList;
    DescType	typeCode;
    long	numFiles;
 //   long	 fileCount;
    char_u	**fnames;
//    char_u	fname[256];
    Size	actualSize;
    SelectionRange thePosition;
    short	gotPosition = false;
    long	lnum;

#ifdef USE_SIOUX
    printf("aevt_odoc:\n");
#endif

    /* the direct object parameter is the list of aliases to files (one or more) */
    error = AEGetParamDesc(theAEvent, keyDirectObject, typeAEList, &theList);
    if (error)
    {
#ifdef USE_SIOUX
	printf("aevt_odoc: AEGetParamDesc error: %d\n", error);
#endif
	return(error);
    }


    error = AEGetParamPtr(theAEvent, keyAEPosition, typeChar, &typeCode, (Ptr) &thePosition, sizeof(SelectionRange), &actualSize);
    if (error == noErr)
	gotPosition = true;
    if (error == errAEDescNotFound)
	error = noErr;
    if (error)
    {
#ifdef USE_SIOUX
	printf("aevt_odoc: AEGetParamPtr error: %d\n", error);
#endif
	return(error);
    }

#ifdef USE_SIOUX
    printf("aevt_odoc: lineNum: %d, startRange %d, endRange %d, [date %lx]\n",
	    thePosition.lineNum, thePosition.startRange, thePosition.endRange,
	    thePosition.theDate);
#endif
/*
    error = AEGetParamDesc(theAEvent, keyAEPosition, typeChar, &thePosition);

    if (^error) then
    {
	if (thePosition.lineNum >= 0)
	{
	  // Goto this line
	}
	else
	{
	  // Set the range char wise
	}
    }
 */


#ifdef FEAT_VISUAL
    reset_VIsual();
#endif

    fnames = new_fnames_from_AEDesc(&theList, &numFiles, &error);

    if (error)
    {
      /* TODO: empty fnames[] first */
      vim_free(fnames);
      return (error);
    }

    if (starting > 0)
    {
	int i;
	char_u *p;

	/* these are the initial files dropped on the Vim icon */
	for (i = 0 ; i < numFiles; i++)
	{
	    if (ga_grow(&global_alist.al_ga, 1) == FAIL
				      || (p = vim_strsave(fnames[i])) == NULL)
		mch_exit(2);
	    else
		alist_add(&global_alist, p, 2);
	}
	goto finished;
    }

    /* Handle the drop, :edit to get to the file */
    handle_drop(numFiles, fnames, FALSE);

    /* TODO: Handle the goto/select line more cleanly */
    if ((numFiles == 1) & (gotPosition))
    {
	if (thePosition.lineNum >= 0)
	{
	    lnum = thePosition.lineNum;
	/*  oap->motion_type = MLINE;
	    setpcmark();*/
	    if (lnum < 1L)
		lnum = 1L;
	    else if (lnum > curbuf->b_ml.ml_line_count)
		lnum = curbuf->b_ml.ml_line_count;
	    curwin->w_cursor.lnum = lnum;
	/*  beginline(BL_SOL | BL_FIX);*/
	}
	else
	    goto_byte(thePosition.startRange + 1);
    }

    /* Update the screen display */
    update_screen(NOT_VALID);
    setcursor();
    out_flush();

  finished:
    AEDisposeDesc(&theList); /* dispose what we allocated */

    error = HandleUnusedParms(theAEvent);
    if (error)
    {
#ifdef USE_SIOUX
	printf("aevt_odoc: HandleUnusedParms error: %d\n", error);
#endif
	return(error);
    }
    return(error);
}

/*
 *
 */

    pascal OSErr
Handle_aevt_oapp_AE(const AppleEvent *theAEvent, AppleEvent *theReply, long refCon)
{
    OSErr	error = noErr;

#ifdef USE_SIOUX
    printf("aevt_oapp:\n");
#endif

    error = HandleUnusedParms(theAEvent);
    if (error)
    {
	return(error);
    }

    return(error);
}

/*
 *
 */

    pascal OSErr
Handle_aevt_quit_AE(const AppleEvent *theAEvent, AppleEvent *theReply, long refCon)
{
    OSErr	error = noErr;

#ifdef USE_SIOUX
    printf("aevt_quit\n");
#endif

    error = HandleUnusedParms(theAEvent);
    if (error)
    {
	return(error);
    }

    /* Need to fake a :confirm qa */
    do_cmdline_cmd((char_u *)"confirm qa");

    return(error);
}

/*
 *
 */

    pascal OSErr
Handle_aevt_pdoc_AE(const AppleEvent *theAEvent, AppleEvent *theReply, long refCon)
{
    OSErr	error = noErr;

#ifdef USE_SIOUX
    printf("aevt_pdoc:\n");
#endif

    error = HandleUnusedParms(theAEvent);
    if (error)
    {
	return(error);
    }

    return(error);
}

/*
 * Handling of unknown AppleEvent
 *
 * (Just get rid of all the parms)
 */
    pascal OSErr
Handle_unknown_AE(const AppleEvent *theAEvent, AppleEvent *theReply, long refCon)
{
    OSErr	error = noErr;

#ifdef USE_SIOUX
    printf("Unknown Event: %x\n", theAEvent->descriptorType);
#endif

    error = HandleUnusedParms(theAEvent);
    if (error)
    {
	return(error);
    }

    return(error);
}



#if TARGET_API_MAC_CARBON
# define NewAEEventHandlerProc(x) NewAEEventHandlerUPP(x)
#endif

/*
 * Install the various AppleEvent Handlers
 */
    OSErr
InstallAEHandlers(void)
{
    OSErr   error;

    /* install open application handler */
    error = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication,
		    NewAEEventHandlerProc(Handle_aevt_oapp_AE), 0, false);
    if (error)
    {
	return error;
    }

    /* install quit application handler */
    error = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
		    NewAEEventHandlerProc(Handle_aevt_quit_AE), 0, false);
    if (error)
    {
	return error;
    }

    /* install open document handler */
    error = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
		    NewAEEventHandlerProc(HandleODocAE), 0, false);
    if (error)
    {
	return error;
    }

    /* install print document handler */
    error = AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments,
		    NewAEEventHandlerProc(Handle_aevt_pdoc_AE), 0, false);

/* Install Core Suite */
/*  error = AEInstallEventHandler(kAECoreSuite, kAEClone,
		    NewAEEventHandlerProc(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEClose,
		    NewAEEventHandlerProc(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAECountElements,
		    NewAEEventHandlerProc(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAECreateElement,
		    NewAEEventHandlerProc(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEDelete,
		    NewAEEventHandlerProc(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEDoObjectsExist,
		    NewAEEventHandlerProc(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEGetData,
		    NewAEEventHandlerProc(Handle_unknown_AE), kAEGetData, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEGetDataSize,
		    NewAEEventHandlerProc(Handle_unknown_AE), kAEGetDataSize, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEGetClassInfo,
		    NewAEEventHandlerProc(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEGetEventInfo,
		    NewAEEventHandlerProc(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEMove,
		    NewAEEventHandlerProc(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAESave,
		    NewAEEventHandlerProc(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAESetData,
		    NewAEEventHandlerProc(Handle_unknown_AE), nil, false);
*/

#ifdef FEAT_CW_EDITOR
    /*
     * Bind codewarrior support handlers
     */
    error = AEInstallEventHandler('KAHL', 'GTTX',
		    NewAEEventHandlerProc(Handle_KAHL_GTTX_AE), 0, false);
    if (error)
    {
	return error;
    }
    error = AEInstallEventHandler('KAHL', 'SRCH',
		    NewAEEventHandlerProc(Handle_KAHL_SRCH_AE), 0, false);
    if (error)
    {
	return error;
    }
    error = AEInstallEventHandler('KAHL', 'MOD ',
		    NewAEEventHandlerProc(Handle_KAHL_MOD_AE), 0, false);
    if (error)
    {
	return error;
    }
#endif

    return error;

}
#endif /* USE_AEVENT */


#if defined(USE_CARBONIZED) && defined(MACOS_X)
/*
 * Callback function, installed by InstallFontPanelHandler(), below,
 * to handle Font Panel events.
 */
    static OSStatus
FontPanelHandler(EventHandlerCallRef inHandlerCallRef, EventRef inEvent,
    void *inUserData)
{
    if (GetEventKind(inEvent) == kEventFontPanelClosed)
    {
	gFontPanelInfo.isPanelVisible = false;
	return noErr;
    }

    if (GetEventKind(inEvent) == kEventFontSelection)
    {
	OSStatus status;
	FMFontFamily newFamily;
	FMFontSize newSize;
	FMFontStyle newStyle;

	/* Retrieve the font family ID number. */
	status = GetEventParameter(inEvent, kEventParamFMFontFamily,
		/*inDesiredType=*/typeFMFontFamily, /*outActualType=*/NULL,
		/*inBufferSize=*/sizeof(FMFontFamily), /*outActualSize=*/NULL,
		&newFamily);
	if (status == noErr)
	    gFontPanelInfo.family = newFamily;

	/* Retrieve the font size. */
	status = GetEventParameter(inEvent, kEventParamFMFontSize,
		typeFMFontSize, NULL, sizeof(FMFontSize), NULL, &newSize);
	if (status == noErr)
	    gFontPanelInfo.size = newSize;

	/* Retrieve the font style (bold, etc.).  Currently unused. */
	status = GetEventParameter(inEvent, kEventParamFMFontStyle,
		typeFMFontStyle, NULL, sizeof(FMFontStyle), NULL, &newStyle);
	if (status == noErr)
	    gFontPanelInfo.style = newStyle;
    }
    return noErr;
}


    static void
InstallFontPanelHandler()
{
    EventTypeSpec eventTypes[2];
    EventHandlerUPP handlerUPP;
    /* EventHandlerRef handlerRef; */

    eventTypes[0].eventClass = kEventClassFont;
    eventTypes[0].eventKind  = kEventFontSelection;
    eventTypes[1].eventClass = kEventClassFont;
    eventTypes[1].eventKind  = kEventFontPanelClosed;

    handlerUPP = NewEventHandlerUPP(FontPanelHandler);

    InstallApplicationEventHandler(handlerUPP, /*numTypes=*/2, eventTypes,
	    /*userData=*/NULL, /*handlerRef=*/NULL);
}


/*
 * Fill the buffer pointed to by outName with the name and size
 * of the font currently selected in the Font Panel.
 */
    static void
GetFontPanelSelection(char_u* outName)
{
    Str255 buf;
    Boolean isBold = false, isItalic = false;

    if (!outName)
	return;

    (void)FMGetFontFamilyName(gFontPanelInfo.family, buf);
    p2cstrcpy(outName, buf);

#if 0 /* TODO: enable when styles are supported in gui_mac_find_font() */
    isBold = (gFontPanelInfo.style & bold);
    isItalic = (gFontPanelInfo.style & italic);
#endif

    sprintf(&outName[buf[0]], ":h%d%s%s", gFontPanelInfo.size,
	    (isBold ? ":b" : ""), (isItalic ? ":i" : ""));
}
#endif


/*
 * ------------------------------------------------------------
 * Unfiled yet
 * ------------------------------------------------------------
 */

/*
 *  gui_mac_get_menu_item_index
 *
 *  Returns the index inside the menu wher
 */
    short /* Shoulde we return MenuItemIndex? */
gui_mac_get_menu_item_index(pMenu)
    vimmenu_T *pMenu;
{
    short	index;
    short	itemIndex = -1;
    vimmenu_T	*pBrother;

    /* Only menu without parent are the:
     * -menu in the menubar
     * -popup menu
     * -toolbar (guess)
     *
     * Which are not items anyway.
     */
    if (pMenu->parent)
    {
	/* Start from the Oldest Brother */
	pBrother = pMenu->parent->children;
	index = 1;
	while ((pBrother) && (itemIndex == -1))
	{
	    if (pBrother == pMenu)
		itemIndex = index;
	    index++;
	    pBrother = pBrother->next;
	}
#ifdef USE_HELPMENU
	/* Adjust index in help menu (for predefined ones) */
	if (itemIndex != -1)
	    if (pMenu->parent->submenu_id == kHMHelpMenuID)
		itemIndex += gui.MacOSHelpItems;
#endif
    }
    return itemIndex;
}

    static vimmenu_T *
gui_mac_get_vim_menu(menuID, itemIndex, pMenu)
    short	menuID;
    short	itemIndex;
    vimmenu_T	*pMenu;
{
    short	index;
    vimmenu_T	*pChildMenu;
    vimmenu_T	*pElder = pMenu->parent;


    /* Only menu without parent are the:
     * -menu in the menubar
     * -popup menu
     * -toolbar (guess)
     *
     * Which are not items anyway.
     */

    if ((pElder) && (pElder->submenu_id == menuID))
    {
#ifdef USE_HELPMENU
	if (menuID == kHMHelpMenuID)
	    itemIndex -= gui.MacOSHelpItems;
#endif

	for (index = 1; (index != itemIndex) && (pMenu != NULL); index++)
	    pMenu = pMenu->next;
    }
    else
    {
	for (; pMenu != NULL; pMenu = pMenu->next)
	{
	    if (pMenu->children != NULL)
	    {
		pChildMenu = gui_mac_get_vim_menu
			   (menuID, itemIndex, pMenu->children);
		if (pChildMenu)
		{
		    pMenu = pChildMenu;
		    break;
		}
	    }
	}
    }
    return pMenu;
}

/*
 * ------------------------------------------------------------
 * MacOS Feedback procedures
 * ------------------------------------------------------------
 */
    pascal
    void
gui_mac_drag_thumb(ControlHandle theControl, short partCode)
{
    scrollbar_T		*sb;
    int			value, dragging;
    ControlHandle	theControlToUse;
    int			dont_scroll_save = dont_scroll;

    theControlToUse = dragged_sb;

    sb = gui_find_scrollbar((long) GetControlReference(theControlToUse));

    if (sb == NULL)
	return;

    /* Need to find value by diff between Old Poss New Pos */
    value = GetControl32BitValue(theControlToUse);
    dragging = (partCode != 0);

    /* When "allow_scrollbar" is FALSE still need to remember the new
     * position, but don't actually scroll by setting "dont_scroll". */
    dont_scroll = !allow_scrollbar;
    gui_drag_scrollbar(sb, value, dragging);
    dont_scroll = dont_scroll_save;
}

    pascal
    void
gui_mac_scroll_action(ControlHandle theControl, short partCode)
{
    /* TODO: have live support */
    scrollbar_T *sb, *sb_info;
    long	data;
    long	value;
    int		page;
    int		dragging = FALSE;
    int		dont_scroll_save = dont_scroll;

    sb = gui_find_scrollbar((long)GetControlReference(theControl));

    if (sb == NULL)
	return;

    if (sb->wp != NULL)		/* Left or right scrollbar */
    {
	/*
	 * Careful: need to get scrollbar info out of first (left) scrollbar
	 * for window, but keep real scrollbar too because we must pass it to
	 * gui_drag_scrollbar().
	 */
	sb_info = &sb->wp->w_scrollbars[0];

	if (sb_info->size > 5)
	    page = sb_info->size - 2;	/* use two lines of context */
	else
	    page = sb_info->size;
    }
    else			/* Bottom scrollbar */
    {
	sb_info = sb;
	page = W_WIDTH(curwin) - 5;
    }

    switch (partCode)
    {
	case  kControlUpButtonPart:   data = -1;    break;
	case  kControlDownButtonPart: data = 1;     break;
	case  kControlPageDownPart:   data = page;  break;
	case  kControlPageUpPart:     data = -page; break;
		    default: data = 0; break;
    }

    value = sb_info->value + data;
/*  if (value > sb_info->max)
	value = sb_info->max;
    else if (value < 0)
	value = 0;*/

    /* When "allow_scrollbar" is FALSE still need to remember the new
     * position, but don't actually scroll by setting "dont_scroll". */
    dont_scroll = !allow_scrollbar;
    gui_drag_scrollbar(sb, value, dragging);
    dont_scroll = dont_scroll_save;

    out_flush();
    gui_mch_set_scrollbar_thumb(sb, value, sb_info->size, sb_info->max);

/*  if (sb_info->wp != NULL)
    {
	win_T	*wp;
	int	sb_num;

	sb_num = 0;
	for (wp = firstwin; wp != sb->wp && wp != NULL; wp = W_NEXT(wp))
	sb_num++;

	if (wp != NULL)
	{
	    current_scrollbar = sb_num;
	    scrollbar_value = value;
	    gui_do_scroll();
	    gui_mch_set_scrollbar_thumb(sb, value, sb_info->size, sb_info->max);
	}
    }*/
}

/*
 * ------------------------------------------------------------
 * MacOS Click Handling procedures
 * ------------------------------------------------------------
 */


/*
 * Handle a click inside the window, it may happens in the
 * scrollbar or the contents.
 *
 * TODO: Add support for potential TOOLBAR
 */
    void
gui_mac_doInContentClick(theEvent, whichWindow)
    EventRecord *theEvent;
    WindowPtr	 whichWindow;
{
    Point		thePoint;
    int_u		vimModifiers;
    short		thePortion;
    ControlHandle	theControl;
    int			vimMouseButton;
    short		dblClick;

    thePoint = theEvent->where;
    GlobalToLocal(&thePoint);
    SelectWindow(whichWindow);

    thePortion = FindControl(thePoint, whichWindow, &theControl);

    if (theControl != NUL)
    {
	/* We hit a scollbar */

	if (thePortion != kControlIndicatorPart)
	{
	    dragged_sb = theControl;
	    TrackControl(theControl, thePoint, gScrollAction);
	    dragged_sb = NULL;
	}
	else
	{
	    dragged_sb = theControl;
#if 1
	    TrackControl(theControl, thePoint, gScrollDrag);
#else
	    TrackControl(theControl, thePoint, NULL);
#endif
	    /* pass 0 as the part to tell gui_mac_drag_thumb, that the mouse
	     * button has been released */
	    gui_mac_drag_thumb(theControl, 0); /* Should it be thePortion ? (Dany) */
	    dragged_sb = NULL;
	}
    }
    else
    {
	/* We are inside the contents */

	/* Convert the CTRL, OPTION, SHIFT and CMD key */
	vimModifiers = EventModifiers2VimMouseModifiers(theEvent->modifiers);

	/* Defaults to MOUSE_LEFT as there's only one mouse button */
	vimMouseButton = MOUSE_LEFT;

#ifdef USE_CTRLCLICKMENU
	/* Convert the CTRL_MOUSE_LEFT to MOUSE_RIGHT */
	clickIsPopup = FALSE;

	if ((gui.MacOSHaveCntxMenu) && (mouse_model_popup()))
	    if (IsShowContextualMenuClick(theEvent))
	    {
		vimMouseButton = MOUSE_RIGHT;
		vimModifiers &= ~MOUSE_CTRL;
		clickIsPopup = TRUE;
	    }
#endif

	/* Is it a double click ? */
	dblClick = ((theEvent->when - lastMouseTick) < GetDblTime());

	/* Send the mouse clicj to Vim */
	gui_send_mouse_event(vimMouseButton, thePoint.h,
					  thePoint.v, dblClick, vimModifiers);

	/* Create the rectangle around the cursor to detect
	 * the mouse dragging
	 */
#ifdef USE_CTRLCLICKMENU
#if 0
	/* TODO: Do we need to this even for the contextual menu?
	 * It may be require for popup_setpos, but for popup?
	 */
	if (vimMouseButton == MOUSE_LEFT)
#endif
#endif
	{
	    SetRect(&dragRect, FILL_X(X_2_COL(thePoint.h)),
				FILL_Y(Y_2_ROW(thePoint.v)),
				FILL_X(X_2_COL(thePoint.h)+1),
				FILL_Y(Y_2_ROW(thePoint.v)+1));

	    dragRectEnbl = TRUE;
	    dragRectControl = kCreateRect;
	}
    }
}

/*
 * Handle the click in the titlebar (to move the window)
 */
    void
gui_mac_doInDragClick(where, whichWindow)
    Point	where;
    WindowPtr	whichWindow;
{
    Rect	movingLimits;
    Rect	*movingLimitsPtr = &movingLimits;

    /* TODO: may try to prevent move outside screen? */
#ifdef USE_CARBONIZED
    movingLimitsPtr = GetRegionBounds(GetGrayRgn(), &movingLimits);
#else
    movingLimitsPtr = &(*GetGrayRgn())->rgnBBox;
#endif
    DragWindow(whichWindow, where, movingLimitsPtr);
}

/*
 * Handle the click in the grow box
 */
    void
gui_mac_doInGrowClick(where, whichWindow)
    Point	where;
    WindowPtr	whichWindow;
{

    long	    newSize;
    unsigned short  newWidth;
    unsigned short  newHeight;
    Rect	    resizeLimits;
    Rect	    *resizeLimitsPtr = &resizeLimits;
#ifdef USE_CARBONIZED
    Rect	    NewContentRect;

    resizeLimitsPtr = GetRegionBounds(GetGrayRgn(), &resizeLimits);
#else
    resizeLimits = qd.screenBits.bounds;
#endif

    /* Set the minimun size */
    /* TODO: Should this come from Vim? */
    resizeLimits.top = 100;
    resizeLimits.left = 100;

#ifdef USE_CARBONIZED
    newSize = ResizeWindow(whichWindow, where, &resizeLimits, &NewContentRect);
    newWidth  = NewContentRect.right - NewContentRect.left;
    newHeight = NewContentRect.bottom - NewContentRect.top;
    gui_resize_shell(newWidth, newHeight);
    gui_mch_set_bg_color(gui.back_pixel);
    gui_set_shellsize(TRUE, FALSE);
#else
    newSize = GrowWindow(whichWindow, where, &resizeLimits);
    if (newSize != 0)
    {
	newWidth  = newSize & 0x0000FFFF;
	newHeight = (newSize >> 16) & 0x0000FFFF;

	gui_mch_set_bg_color(gui.back_pixel);

	gui_resize_shell(newWidth, newHeight);

	/*
	 * We need to call gui_set_shellsize as the size
	 * used by Vim may be smaller than the size selected
	 * by the user. This cause some overhead
	 * TODO: add a check inside gui_resize_shell?
	 */
	gui_set_shellsize(TRUE, FALSE);

	/*
	 * Origin of the code below is unknown.
	 * Functionality is unknown.
	 * Time of commented out is unknown.
	 */
/*	SetPort(wp);
	InvalRect(&wp->portRect);
	if (isUserWindow(wp)) {
	    DrawingWindowPeek	aWindow = (DrawingWindowPeek)wp;

	    if (aWindow->toolRoutines.toolWindowResizedProc)
		CallToolWindowResizedProc(aWindow->toolRoutines.toolWindowResizedProc, wp);
	}*/
    };
#endif

}

/*
 * Handle the click in the zoom box
 */
#ifdef USE_CARBONIZED
    static void
gui_mac_doInZoomClick(theEvent, whichWindow)
    EventRecord	*theEvent;
    WindowPtr	whichWindow;
{
    Rect	r;
    Point	p;
    short	thePart;

    /* ideal width is current */
    p.h = Columns * gui.char_width + 2 * gui.border_offset;
    if (gui.which_scrollbars[SBAR_LEFT])
	p.h += gui.scrollbar_width;
    if (gui.which_scrollbars[SBAR_RIGHT])
	p.h += gui.scrollbar_width;
    /* ideal height is as heigh as we can get */
    p.v = 15 * 1024;

    thePart = IsWindowInStandardState(whichWindow, &p, &r)
						       ? inZoomIn : inZoomOut;

    if (!TrackBox(whichWindow, theEvent->where, thePart))
	return;

    /* use returned width */
    p.h = r.right - r.left;
    /* adjust returned height */
    p.v = r.bottom - r.top - 2 * gui.border_offset;
    if (gui.which_scrollbars[SBAR_BOTTOM])
	p.v -= gui.scrollbar_height;
    p.v -= p.v % gui.char_height;
    p.v += 2 * gui.border_width;
    if (gui.which_scrollbars[SBAR_BOTTOM]);
	p.v += gui.scrollbar_height;

    ZoomWindowIdeal(whichWindow, thePart, &p);

    GetWindowBounds(whichWindow, kWindowContentRgn, &r);
    gui_resize_shell(r.right - r.left, r.bottom - r.top);
    gui_mch_set_bg_color(gui.back_pixel);
    gui_set_shellsize(TRUE, FALSE);
}
#endif /* defined(USE_CARBONIZED) */

/*
 * ------------------------------------------------------------
 * MacOS Event Handling procedure
 * ------------------------------------------------------------
 */

/*
 * Handle the Update Event
 */

    void
gui_mac_doUpdateEvent(event)
    EventRecord	*event;
{
    WindowPtr	whichWindow;
    GrafPtr	savePort;
    RgnHandle	updateRgn;
#ifdef USE_CARBONIZED
    Rect	updateRect;
#endif
    Rect	*updateRectPtr;
    Rect	rc;
    Rect	growRect;
    RgnHandle	saveRgn;


#ifdef USE_CARBONIZED
    updateRgn = NewRgn();
    if (updateRgn == NULL)
	return;
#endif

    /* This could be done by the caller as we
     * don't require anything else out of the event
     */
    whichWindow = (WindowPtr) event->message;

    /* Save Current Port */
    GetPort(&savePort);

    /* Select the Window's Port */
#ifdef USE_CARBONIZED
    SetPortWindowPort(whichWindow);
#else
    SetPort(whichWindow);
#endif

    /* Let's update the window */
      BeginUpdate(whichWindow);
	/* Redraw the biggest rectangle covering the area
	 * to be updated.
	 */
#ifdef USE_CARBONIZED
	GetPortVisibleRegion(GetWindowPort(whichWindow), updateRgn);
# if 0
	/* Would be more appropriate to use the follwing but doesn't
	 * seem to work under MacOS X (Dany)
	 */
	GetWindowRegion(whichWindow, kWindowUpdateRgn, updateRgn);
# endif
#else
	updateRgn = whichWindow->visRgn;
#endif
	/* Use the HLock useless in Carbon? Is it harmful?*/
	HLock((Handle) updateRgn);
#ifdef USE_CARBONIZED
	  updateRectPtr = GetRegionBounds(updateRgn, &updateRect);
# if 0
	  /* Code from original Carbon Port (using GetWindowRegion.
	   * I believe the UpdateRgn is already in local (Dany)
	   */
	  GlobalToLocal(&topLeft(updateRect)); /* preCarbon? */
	  GlobalToLocal(&botRight(updateRect));
# endif
#else
	  updateRectPtr = &(*updateRgn)->rgnBBox;
#endif
	  /* Update the content (i.e. the text) */
	  gui_redraw(updateRectPtr->left, updateRectPtr->top,
		      updateRectPtr->right - updateRectPtr->left,
		      updateRectPtr->bottom   - updateRectPtr->top);
	  /* Clear the border areas if needed */
	  gui_mch_set_bg_color(gui.back_pixel);
	  if (updateRectPtr->left < FILL_X(0))
	  {
	    SetRect(&rc, 0, 0, FILL_X(0), FILL_Y(Rows));
	    EraseRect(&rc);
	  }
	  if (updateRectPtr->top < FILL_Y(0))
	  {
	    SetRect(&rc, 0, 0, FILL_X(Columns), FILL_Y(0));
	    EraseRect(&rc);
	  }
	  if (updateRectPtr->right > FILL_X(Columns))
	  {
	    SetRect(&rc, FILL_X(Columns), 0,
			   FILL_X(Columns) + gui.border_offset, FILL_Y(Rows));
	    EraseRect(&rc);
	  }
	  if (updateRectPtr->bottom > FILL_Y(Rows))
	  {
	    SetRect(&rc, 0, FILL_Y(Rows), FILL_X(Columns) + gui.border_offset,
					    FILL_Y(Rows) + gui.border_offset);
	    EraseRect(&rc);
	  }
	HUnlock((Handle) updateRgn);
#ifdef USE_CARBONIZED
	DisposeRgn(updateRgn);
#endif

	/* Update scrollbars */
	DrawControls(whichWindow);

	/* Update the GrowBox */
	/* Taken from FAQ 33-27 */
	saveRgn = NewRgn();
#ifdef USE_CARBONIZED
	GetWindowBounds(whichWindow, kWindowGrowRgn, &growRect);
#else
	growRect = whichWindow->portRect;
	growRect.top  = growRect.bottom - 15;
	growRect.left = growRect.right  - 15;
#endif
	GetClip(saveRgn);
	ClipRect(&growRect);
	DrawGrowIcon(whichWindow);
	SetClip(saveRgn);
	DisposeRgn(saveRgn);
      EndUpdate(whichWindow);

    /* Restore original Port */
    SetPort(savePort);
}

/*
 * Handle the activate/deactivate event
 * (apply to a window)
 */
    void
gui_mac_doActivateEvent(event)
    EventRecord	*event;
{
    WindowPtr	whichWindow;

    whichWindow = (WindowPtr) event->message;
    if ((event->modifiers) & activeFlag)
	/* Activate */
	gui_focus_change(TRUE);
    else
    {
	/* Deactivate */
	gui_focus_change(FALSE);
/*	DON'T KNOW what the code below was doing
	found in the deactivate clause, but the
	clause writting TRUE into in_focus (BUG)
 */

#if 0	/* Removed by Dany as per above June 2001 */
	a_bool = false;
	SetPreserveGlyph(a_bool);
	SetOutlinePreferred(a_bool);
#endif
    }
}


/*
 * Handle the suspend/resume event
 * (apply to the application)
 */
    void
gui_mac_doSuspendEvent(event)
    EventRecord	*event;
{
    /* The frontmost application just changed */

    /* NOTE: the suspend may happen before the deactivate
     *       seen on MacOS X
     */

    /* May not need to change focus as the window will
     * get an activate/desactivate event
     */
    if (event->message & 1)
	/* Resume */
	gui_focus_change(TRUE);
    else
	/* Suspend */
	gui_focus_change(FALSE);
}

/*
 * Handle the key
 */

    void
gui_mac_doKeyEvent(EventRecord *theEvent)
{
    /* TODO: add support for COMMAND KEY */
    long		menu;
    unsigned char	string[20];
    short		num, i;
    short		len = 0;
    KeySym		key_sym;
    int			key_char;
    int			modifiers;
    int			simplify = FALSE;

    /* Mask the mouse (as per user setting) */
    if (p_mh)
	ObscureCursor();

    /* Get the key code and it's ASCII representation */
    key_sym = ((theEvent->message & keyCodeMask) >> 8);
    key_char = theEvent->message & charCodeMask;
    num = 1;

    /* Intercept CTRL-C */
    if (theEvent->modifiers & controlKey)
    {
	if (key_char == Ctrl_C && ctrl_c_interrupts)
	    got_int = TRUE;
	else if ((theEvent->modifiers & ~(controlKey|shiftKey)) == 0
		&& (key_char == '2' || key_char == '6'))
	{
	    /* CTRL-^ and CTRL-@ don't work in the normal way. */
	    if (key_char == '2')
		key_char = Ctrl_AT;
	    else
		key_char = Ctrl_HAT;
	    theEvent->modifiers = 0;
	}
    }

    /* Intercept CMD-. */
    if (theEvent->modifiers & cmdKey)
	if (key_char == '.')
	    got_int = TRUE;

    /* Handle command key as per menu */
    /* TODO: should override be allowed? Require YAO or could use 'winaltkey' */
    if (theEvent->modifiers & cmdKey)
	/* Only accept CMD alone or with CAPLOCKS and the mouse button.
	 * Why the mouse button? */
	if ((theEvent->modifiers & (~(cmdKey | btnState | alphaLock))) == 0)
	{
	    menu = MenuKey(key_char);
	    if (HiWord(menu))
	    {
		gui_mac_handle_menu(menu);
		return;
	    }
	}

    /* Convert the modifiers */
    modifiers = EventModifiers2VimModifiers(theEvent->modifiers);


    /* Handle special keys. */
#if 0
    /* Why has this been removed? */
    if	(!(theEvent->modifiers & (cmdKey | controlKey | rightControlKey)))
#endif
    {
	/* Find the special key (for non-printable keyt_char) */
	if  ((key_char < 0x20) || (key_char == 0x7f))
	    for (i = 0; special_keys[i].key_sym != (KeySym)0; i++)
		if (special_keys[i].key_sym == key_sym)
		{
# if 0
		    /* We currently don't have not so special key */
		    if (special_keys[i].vim_code1 == NUL)
			key_char = special_keys[i].vim_code0;
		    else
# endif
			key_char = TO_SPECIAL(special_keys[i].vim_code0,
						special_keys[i].vim_code1);
		    simplify = TRUE;
		    break;
		}
    }

    /* For some keys the modifier is included in the char itself. */
    if (simplify || key_char == TAB || key_char == ' ')
	key_char = simplify_key(key_char, &modifiers);

    /* Add the modifier to the input bu if needed */
    /* Do not want SHIFT-A or CTRL-A with modifier */
    if (!IS_SPECIAL(key_char)
	    && key_sym != vk_Space
	    && key_sym != vk_Tab
	    && key_sym != vk_Return
	    && key_sym != vk_Enter
	    && key_sym != vk_Esc)
    {
#if 1
    /* Clear modifiers when only one modifier is set */
	if ((modifiers == MOD_MASK_SHIFT)
		|| (modifiers == MOD_MASK_CTRL)
		|| (modifiers == MOD_MASK_ALT))
	    modifiers = 0;
#else
	if (modifiers & MOD_MASK_CTRL)
	    modifiers = modifiers & ~MOD_MASK_CTRL;
	if (modifiers & MOD_MASK_ALT)
	    modifiers = modifiers & ~MOD_MASK_ALT;
	if (modifiers & MOD_MASK_SHIFT)
	    modifiers = modifiers & ~MOD_MASK_SHIFT;
#endif
    }
	if (modifiers)
	{
	    string[len++] = CSI;
	    string[len++] = KS_MODIFIER;
	    string[len++] = modifiers;
	}

	if (IS_SPECIAL(key_char))
	{
	    string[len++] = CSI;
	    string[len++] = K_SECOND(key_char);
	    string[len++] = K_THIRD(key_char);
	}
	else
	{
#ifdef FEAT_MBYTE
	    /* Convert characters when needed (e.g., from MacRoman to latin1).
	     * This doesn't work for the NUL byte. */
	    if (input_conv.vc_type != CONV_NONE && key_char > 0)
	    {
		char_u	from[2], *to;
		int	l;

		from[0] = key_char;
		from[1] = NUL;
		l = 1;
		to = string_convert(&input_conv, from, &l);
		if (to != NULL)
		{
		    for (i = 0; i < l && len < 19; i++)
		    {
			if (to[i] == CSI)
			{
			    string[len++] = KS_EXTRA;
			    string[len++] = KE_CSI;
			}
			else
			    string[len++] = to[i];
		    }
		    vim_free(to);
		}
		else
		    string[len++] = key_char;
	    }
	    else
#endif
		string[len++] = key_char;
	}

	if (len == 1 && string[0] == CSI)
	{
	    /* Turn CSI into K_CSI. */
	    string[ len++ ] = KS_EXTRA;
	    string[ len++ ] = KE_CSI;
	}

    add_to_input_buf(string, len);
}

/*
 * Handle MouseClick
 */
    void
gui_mac_doMouseDownEvent(theEvent)
    EventRecord *theEvent;
{
    short		thePart;
    WindowPtr		whichWindow;

    thePart = FindWindow(theEvent->where, &whichWindow);

    switch (thePart)
    {
	case (inDesk):
	    /* TODO: what to do? */
	    break;

	case (inMenuBar):
	    gui_mac_handle_menu(MenuSelect(theEvent->where));
	    break;

	case (inContent):
	    gui_mac_doInContentClick(theEvent, whichWindow);
	    break;

	case (inDrag):
	    gui_mac_doInDragClick(theEvent->where, whichWindow);
	    break;

	case (inGrow):
	    gui_mac_doInGrowClick(theEvent->where, whichWindow);
	    break;

	case (inGoAway):
	    if (TrackGoAway(whichWindow, theEvent->where))
		gui_shell_closed();
	    break;

	case (inZoomIn):
	case (inZoomOut):
#ifdef USE_CARBONIZED
	    gui_mac_doInZoomClick(theEvent, whichWindow);
#endif
	    break;
    }
}

/*
 * Handle MouseMoved
 * [this event is a moving in and out of a region]
 */
    void
gui_mac_doMouseMovedEvent(event)
    EventRecord *event;
{
    Point   thePoint;
    int_u   vimModifiers;

    thePoint = event->where;
    GlobalToLocal(&thePoint);
    vimModifiers = EventModifiers2VimMouseModifiers(event->modifiers);

    if (!Button())
	gui_mouse_moved(thePoint.h, thePoint.v);
    else
#ifdef USE_CTRLCLICKMENU
	if (!clickIsPopup)
#endif
	    gui_send_mouse_event(MOUSE_DRAG, thePoint.h,
					     thePoint.v, FALSE, vimModifiers);

    /* Reset the region from which we move in and out */
    SetRect(&dragRect, FILL_X(X_2_COL(thePoint.h)),
			FILL_Y(Y_2_ROW(thePoint.v)),
			FILL_X(X_2_COL(thePoint.h)+1),
			FILL_Y(Y_2_ROW(thePoint.v)+1));

    if (dragRectEnbl)
	dragRectControl = kCreateRect;

}

/*
 * Handle the mouse release
 */
    void
gui_mac_doMouseUpEvent(theEvent)
    EventRecord *theEvent;
{
    Point   thePoint;
    int_u   vimModifiers;

    /* TODO: Properly convert the Contextual menu mouse-up */
    /*       Potential source of the double menu */
    lastMouseTick = theEvent->when;
    dragRectEnbl = FALSE;
    dragRectControl = kCreateEmpty;
    thePoint = theEvent->where;
    GlobalToLocal(&thePoint);

    vimModifiers = EventModifiers2VimMouseModifiers(theEvent->modifiers);
#ifdef USE_CTRLCLICKMENU
    if (clickIsPopup)
    {
	vimModifiers &= ~MOUSE_CTRL;
	clickIsPopup = FALSE;
    }
#endif
    gui_send_mouse_event(MOUSE_RELEASE, thePoint.h, thePoint.v, FALSE, vimModifiers);
}

#ifdef USE_MOUSEWHEEL
    static pascal OSStatus
gui_mac_mouse_wheel(EventHandlerCallRef nextHandler, EventRef theEvent,
								   void *data)
{
    EventRef	bogusEvent;
    Point	point;
    Rect	bounds;
    UInt32	mod;
    SInt32	delta;
    int_u	vim_mod;

    if (noErr != GetEventParameter(theEvent, kEventParamMouseWheelDelta,
			      typeSInt32, NULL, sizeof(SInt32), NULL, &delta))
	goto bail;
    if (noErr != GetEventParameter(theEvent, kEventParamMouseLocation,
			      typeQDPoint, NULL, sizeof(Point), NULL, &point))
	goto bail;
    if (noErr != GetEventParameter(theEvent, kEventParamKeyModifiers,
				typeUInt32, NULL, sizeof(UInt32), NULL, &mod))
	goto bail;

    vim_mod = 0;
    if (mod & shiftKey)
	vim_mod |= MOUSE_SHIFT;
    if (mod & controlKey)
	vim_mod |= MOUSE_CTRL;
    if (mod & optionKey)
	vim_mod |= MOUSE_ALT;

    /* post a bogus event to wake up WaitNextEvent */
    if (noErr != CreateEvent(NULL, kEventClassMouse, kEventMouseMoved, 0,
					    kEventAttributeNone, &bogusEvent))
	goto bail;
    if (noErr != PostEventToQueue(GetMainEventQueue(), bogusEvent,
							   kEventPriorityLow))
	goto bail;

    if (noErr == GetWindowBounds(gui.VimWindow, kWindowContentRgn, &bounds))
    {
	point.h -= bounds.left;
	point.v -= bounds.top;
    }

    gui_send_mouse_event((delta > 0) ? MOUSE_4 : MOUSE_5,
					    point.h, point.v, FALSE, vim_mod);

    return noErr;

  bail:
    /*
     * when we fail give any additional callback handler a chance to perform
     * it's actions
     */
    return CallNextEventHandler(nextHandler, theEvent);
}
#endif /* defined(USE_MOUSEWHEEL) */

#if 0

/*
 * This would be the normal way of invoking the contextual menu
 * but the Vim API doesn't seem to a support a request to get
 * the menu that we should display
 */
    void
gui_mac_handle_contextual_menu(event)
    EventRecord *event;
{
/*
 *  Clone PopUp to use menu
 *  Create a object descriptor for the current selection
 *  Call the procedure
 */

//  Call to Handle Popup
    OSStatus status = ContextualMenuSelect(CntxMenu, event->where, false, kCMHelpItemNoHelp, "", NULL, &CntxType, &CntxMenuID, &CntxMenuItem);

    if (status != noErr)
	return;

    if (CntxType == kCMMenuItemSelected)
    {
	/* Handle the menu CntxMenuID, CntxMenuItem */
	/* The submenu can be handle directly by gui_mac_handle_menu */
	/* But what about the current menu, is the meny changed by ContextualMenuSelect */
	gui_mac_handle_menu((CntxMenuID << 16) + CntxMenuItem);
    }
    else if (CntxMenuID == kCMShowHelpSelected)
    {
	/* Should come up with the help */
    }

}
#endif

/*
 * Handle menubar selection
 */
    void
gui_mac_handle_menu(menuChoice)
    long menuChoice;
{
    short	menu = HiWord(menuChoice);
    short	item = LoWord(menuChoice);
    vimmenu_T	*theVimMenu = root_menu;
#ifndef USE_CARBONIZED
    MenuHandle	appleMenu;
    Str255	itemName;
#endif

    if (menu == 256)  /* TODO: use constant or gui.xyz */
    {
	if (item == 1)
	    gui_mch_beep(); /* TODO: Popup dialog or do :intro */
	else
	{
#ifndef USE_CARBONIZED
	    /* Desk Accessory doesn't exist in Carbon */
	    appleMenu = GetMenuHandle(menu);
	    GetMenuItemText(appleMenu, item, itemName);
	    (void) OpenDeskAcc(itemName);
#endif
	}
    }
    else if (item != 0)
    {
	theVimMenu = gui_mac_get_vim_menu(menu, item, root_menu);

	if (theVimMenu)
	    gui_menu_cb(theVimMenu);
    }
    HiliteMenu(0);
}

/*
 * Dispatch the event to proper handler
 */

    void
gui_mac_handle_event(event)
    EventRecord *event;
{
    OSErr	error;

    /* Handle contextual menu right now (if needed) */
#ifdef USE_CTRLCLICKMENU
    if (gui.MacOSHaveCntxMenu)
	if (IsShowContextualMenuClick(event))
	{
# if 0
	    gui_mac_handle_contextual_menu(event);
# else
	    gui_mac_doMouseDownEvent(event);
# endif
	    return;
	}
#endif

    /* Handle normal event */
    switch (event->what)
    {
	case (keyDown):
	case (autoKey):
	    gui_mac_doKeyEvent(event);
	    break;

	case (keyUp):
	    /* We don't care about when the key get release */
	    break;

	case (mouseDown):
	    gui_mac_doMouseDownEvent(event);
	    break;

	case (mouseUp):
	    gui_mac_doMouseUpEvent(event);
	    break;

	case (updateEvt):
	    gui_mac_doUpdateEvent(event);
	    break;

	case (diskEvt):
	    /* We don't need special handling for disk insertion */
	    break;

	case (activateEvt):
	    gui_mac_doActivateEvent(event);
	    break;

	case (osEvt):
	    switch ((event->message >> 24) & 0xFF)
	    {
		case (0xFA): /* mouseMovedMessage */
		    gui_mac_doMouseMovedEvent(event);
		    break;
		case (0x01): /* suspendResumeMessage */
		    gui_mac_doSuspendEvent(event);
		    break;
	    }
	    break;

#ifdef USE_AEVENT
	case (kHighLevelEvent):
	    /* Someone's talking to us, through AppleEvents */
	    error = AEProcessAppleEvent(event); /* TODO: Error Handling */
	    break;
#endif
    }
}

/*
 * ------------------------------------------------------------
 * Unknown Stuff
 * ------------------------------------------------------------
 */


    GuiFont
gui_mac_find_font(font_name)
    char_u *font_name;
{
    char_u	c;
    char_u	*p;
    char_u	pFontName[256];
    Str255	systemFontname;
    short	font_id;
    short	size=9;
    GuiFont	font;
#if 0
    char_u      *fontNamePtr;
#endif

    for (p = font_name; ((*p != 0) && (*p != ':')); p++)
	;

    c = *p;
    *p = 0;

#if 1
    STRCPY(&pFontName[1], font_name);
    pFontName[0] = STRLEN(font_name);
    *p = c;

    GetFNum(pFontName, &font_id);

    if (font_id == 0)
    {
	/*
	 * Try again, this time replacing underscores in the font name
	 * with spaces (:set guifont allows the two to be used
	 * interchangeably; the Font Manager doesn't).
	 */
	int i, changed = FALSE;

	for (i = pFontName[0]; i > 0; --i)
	{
	    if (pFontName[i] == '_')
	    {
		pFontName[i] = ' ';
		changed = TRUE;
	    }
	}
	if (changed)
	    GetFNum(pFontName, &font_id);
    }

#else
    /* name = C2Pascal_save(menu->dname); */
    fontNamePtr = C2Pascal_save_and_remove_backslash(font_name);

    GetFNum(fontNamePtr, &font_id);
#endif


    if (font_id == 0)
    {
	/* Oups, the system font was it the one the user want */

	GetFontName(0, systemFontname);
	if (!EqualString(pFontName, systemFontname, false, false))
	    return NOFONT;
    }
    if (*p == ':')
    {
	p++;
	/* Set the values found after ':' */
	while (*p)
	{
	    switch (*p++)
	    {
		case 'h':
		    size = points_to_pixels(p, &p, TRUE);
		    break;
		    /*
		     * TODO: Maybe accept width and styles
		     */
	    }
	    while (*p == ':')
		p++;
	}
    }

    if (size < 1)
	size = 1;   /* Avoid having a size of 0 with system font */

    font = (size << 16) + ((long) font_id & 0xFFFF);

    return font;
}

/*
 * ------------------------------------------------------------
 * GUI_MCH functionnality
 * ------------------------------------------------------------
 */

/*
 * Parse the GUI related command-line arguments.  Any arguments used are
 * deleted from argv, and *argc is decremented accordingly.  This is called
 * when vim is started, whether or not the GUI has been started.
 */
    void
gui_mch_prepare(argc, argv)
    int		*argc;
    char	**argv;
{
    /* TODO: Move most of this stuff toward gui_mch_init */
#ifdef USE_EXE_NAME
    FSSpec	applDir;
# ifndef USE_FIND_BUNDLE_PATH
    short	applVRefNum;
    long	applDirID;
    Str255	volName;
# else
    ProcessSerialNumber psn;
    FSRef	applFSRef;
# endif
#endif

#ifndef USE_CARBONIZED
    MaxApplZone();	    /* What could replace thos */
    /* In Carbon, all shared library are automatically load in
     * there's no need to init them
     */
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(nil);
#else
    /* Why did I put that in? (Dany) */
    MoreMasterPointers (0x40 * 3); /* we love handles */
#endif

#if 0
    InitCursor();

#ifdef USE_CARBONIZED
    RegisterAppearanceClient();
#endif

#ifdef USE_AEVENT
    (void) InstallAEHandlers();
#endif

#ifdef USE_CTRLCLICKMENU
    if (Gestalt(gestaltContextualMenuAttr, &gestalt_rc) == noErr)
	gui.MacOSHaveCntxMenu = BitTst(&gestalt_rc, 31-gestaltContextualMenuTrapAvailable);
    else
	gui.MacOSHaveCntxMenu = false;

    if (gui.MacOSHaveCntxMenu)
	gui.MacOSHaveCntxMenu = (InitContextualMenus()==noErr);
#endif

#ifdef USE_SIOUX
    SIOUXSettings.standalone = false;
    SIOUXSettings.initializeTB = false;
    SIOUXSettings.setupmenus = false;
    SIOUXSettings.asktosaveonclose = false;
    SIOUXSettings.showstatusline = true;
    SIOUXSettings.toppixel = 300;
    SIOUXSettings.leftpixel = 10;
    InstallConsole(1); /* fileno(stdout) = 1, on page 430 of MSL C */
    printf("Debugging console enabled\n");
    /*	SIOUXSetTitle((char_u *) "Vim Stdout"); */
#endif

    pomme = NewMenu(256, "\p\024"); /* 0x14= = Apple Menu */

    AppendMenu(pomme, "\pAbout VIM");
#ifndef USE_CARBONIZED
    AppendMenu(pomme, "\p-");
    AppendResMenu(pomme, 'DRVR');
#endif

    InsertMenu(pomme, 0);

    DrawMenuBar();


#ifndef USE_OFFSETED_WINDOW
    SetRect(&windRect, 10, 48, 10+80*7 + 16, 48+24*11);
#else
    SetRect(&windRect, 300, 40, 300+80*7 + 16, 40+24*11);
#endif


#ifdef USE_CARBONIZED
    CreateNewWindow(kDocumentWindowClass,
		kWindowResizableAttribute | kWindowCollapseBoxAttribute,
		&windRect, &gui.VimWindow);
    SetPortWindowPort(gui.VimWindow);
#else
    gui.VimWindow = NewCWindow(nil, &windRect, "\pgVim on Macintosh", true, documentProc,
			(WindowPtr) -1L, false, 0);
    SetPort(gui.VimWindow);
#endif

    gui.char_width = 7;
    gui.char_height = 11;
    gui.char_ascent = 6;
    gui.num_rows = 24;
    gui.num_cols = 80;
    gui.in_focus = TRUE; /* For the moment -> syn. of front application */

#if TARGET_API_MAC_CARBON
    gScrollAction = NewControlActionUPP(gui_mac_scroll_action);
    gScrollDrag   = NewControlActionUPP(gui_mac_drag_thumb);
#else
    gScrollAction = NewControlActionProc(gui_mac_scroll_action);
    gScrollDrag   = NewControlActionProc(gui_mac_drag_thumb);
#endif

    /* Getting a handle to the Help menu */
#ifdef USE_HELPMENU
# ifdef USE_CARBONIZED
    HMGetHelpMenu(&gui.MacOSHelpMenu, NULL);
# else
    (void) HMGetHelpMenuHandle(&gui.MacOSHelpMenu);
# endif

    if (gui.MacOSHelpMenu != nil)
	gui.MacOSHelpItems = CountMenuItems(gui.MacOSHelpMenu);
    else
	gui.MacOSHelpItems = 0;
#endif

    dragRectEnbl = FALSE;
    dragRgn = NULL;
    dragRectControl = kCreateEmpty;
    cursorRgn = NewRgn();
#endif
#ifdef USE_EXE_NAME
# ifndef USE_FIND_BUNDLE_PATH
    HGetVol(volName, &applVRefNum, &applDirID);
    /* TN2015: mention a possible bad VRefNum */
    FSMakeFSSpec(applVRefNum, applDirID, "\p", &applDir);
# else
    /* OSErr GetApplicationBundleFSSpec(FSSpecPtr theFSSpecPtr)
     * of TN2015
     * This technic remove the ../Contents/MacOS/etc part
     */
    (void)GetCurrentProcess(&psn);
    /* if (err != noErr) return err; */

    (void)GetProcessBundleLocation(&psn, &applFSRef);
    /* if (err != noErr) return err; */

    (void)FSGetCatalogInfo(&applFSRef, kFSCatInfoNone, NULL, NULL, &applDir, NULL);

    /* This technic return NIL when we disallow_gui */
# endif
    exe_name = FullPathFromFSSpec_save(applDir);
#endif

#ifdef USE_VIM_CREATOR_ID
    _fcreator = 'VIM!';
    _ftype = 'TEXT';
#endif
}

#ifndef ALWAYS_USE_GUI
/*
 * Check if the GUI can be started.  Called before gvimrc is sourced.
 * Return OK or FAIL.
 */
    int
gui_mch_init_check(void)
{
    /* TODO: For MacOS X find a way to return FAIL, if the user logged in
     * using the >console
     */
    if (disallow_gui) /* see main.c for reason to disallow */
	return FAIL;
    return OK;
}
#endif

    static OSErr
receiveHandler(WindowRef theWindow, void* handlerRefCon, DragRef theDrag)
{
    int		x, y;
    int_u	modifiers;
    char_u	**fnames = NULL;
    int		count;
    int		i, j;

    /* Get drop position, modifiers and count of items */
    {
	Point	point;
	SInt16	mouseUpModifiers;
	UInt16	countItem;

	GetDragMouse(theDrag, &point, NULL);
	GlobalToLocal(&point);
	x = point.h;
	y = point.v;
	GetDragModifiers(theDrag, NULL, NULL, &mouseUpModifiers);
	modifiers = EventModifiers2VimMouseModifiers(mouseUpModifiers);
	CountDragItems(theDrag, &countItem);
	count = countItem;
    }

    fnames = (char_u **)alloc(count * sizeof(char_u *));
    if (fnames == NULL)
	return dragNotAcceptedErr;

    /* Get file names dropped */
    for (i = j = 0; i < count; ++i)
    {
	DragItemRef	item;
	OSErr		err;
	Size		size;
	FlavorType	type = flavorTypeHFS;
	HFSFlavor	hfsFlavor;

	fnames[i] = NULL;
	GetDragItemReferenceNumber(theDrag, i + 1, &item);
	err = GetFlavorDataSize(theDrag, item, type, &size);
	if (err != noErr || size > sizeof(hfsFlavor))
	    continue;
	err = GetFlavorData(theDrag, item, type, &hfsFlavor, &size, 0);
	if (err != noErr)
	    continue;
	fnames[j++] = FullPathFromFSSpec_save(hfsFlavor.fileSpec);
    }
    count = j;

    gui_handle_drop(x, y, modifiers, fnames, count);
    return noErr;
}

/*
 * Initialise the GUI.  Create all the windows, set up all the call-backs
 * etc.
 */
    int
gui_mch_init()
{
    /* TODO: Move most of this stuff toward gui_mch_init */
    Rect	windRect;
    MenuHandle	pomme;
#ifdef USE_CTRLCLICKMENU
    long	gestalt_rc;
#endif
#ifdef USE_MOUSEWHEEL
    EventTypeSpec   eventTypeSpec;
    EventHandlerRef mouseWheelHandlerRef;
#endif
#if 1
    InitCursor();

#ifdef USE_CARBONIZED
    RegisterAppearanceClient();
#endif

#ifdef USE_AEVENT
    (void) InstallAEHandlers();
#endif

#ifdef USE_CTRLCLICKMENU
    if (Gestalt(gestaltContextualMenuAttr, &gestalt_rc) == noErr)
	gui.MacOSHaveCntxMenu = BitTst(&gestalt_rc, 31-gestaltContextualMenuTrapAvailable);
    else
	gui.MacOSHaveCntxMenu = false;

    if (gui.MacOSHaveCntxMenu)
	gui.MacOSHaveCntxMenu = (InitContextualMenus()==noErr);
#endif

#ifdef USE_SIOUX
    SIOUXSettings.standalone = false;
    SIOUXSettings.initializeTB = false;
    SIOUXSettings.setupmenus = false;
    SIOUXSettings.asktosaveonclose = false;
    SIOUXSettings.showstatusline = true;
    SIOUXSettings.toppixel = 300;
    SIOUXSettings.leftpixel = 10;
    InstallConsole(1); /* fileno(stdout) = 1, on page 430 of MSL C */
    printf("Debugging console enabled\n");
    /*	SIOUXSetTitle((char_u *) "Vim Stdout"); */
#endif

    pomme = NewMenu(256, "\p\024"); /* 0x14= = Apple Menu */

    AppendMenu(pomme, "\pAbout VIM");
#ifndef USE_CARBONIZED
    AppendMenu(pomme, "\p-");
    AppendResMenu(pomme, 'DRVR');
#endif

    InsertMenu(pomme, 0);

    DrawMenuBar();


#ifndef USE_OFFSETED_WINDOW
    SetRect(&windRect, 10, 48, 10+80*7 + 16, 48+24*11);
#else
    SetRect(&windRect, 300, 40, 300+80*7 + 16, 40+24*11);
#endif

    gui.VimWindow = NewCWindow(nil, &windRect, "\pgVim on Macintosh", true,
#ifdef USE_CARBONIZED
			zoomDocProc,
#else
			documentProc,
#endif
			(WindowPtr)-1L, true, 0);
    InstallReceiveHandler((DragReceiveHandlerUPP)receiveHandler,
	    gui.VimWindow, NULL);
#ifdef USE_CARBONIZED
    SetPortWindowPort(gui.VimWindow);
#else
    SetPort(gui.VimWindow);
#endif

    gui.char_width = 7;
    gui.char_height = 11;
    gui.char_ascent = 6;
    gui.num_rows = 24;
    gui.num_cols = 80;
    gui.in_focus = TRUE; /* For the moment -> syn. of front application */

#if TARGET_API_MAC_CARBON
    gScrollAction = NewControlActionUPP(gui_mac_scroll_action);
    gScrollDrag   = NewControlActionUPP(gui_mac_drag_thumb);
#else
    gScrollAction = NewControlActionProc(gui_mac_scroll_action);
    gScrollDrag   = NewControlActionProc(gui_mac_drag_thumb);
#endif

#if defined(USE_CARBONIZED) && defined(MACOS_X)
    /* Install Carbon event callbacks. */
    (void)InstallFontPanelHandler();
#endif

    /* Getting a handle to the Help menu */
#ifdef USE_HELPMENU
# ifdef USE_CARBONIZED
    HMGetHelpMenu(&gui.MacOSHelpMenu, NULL);
# else
    (void) HMGetHelpMenuHandle(&gui.MacOSHelpMenu);
# endif

    if (gui.MacOSHelpMenu != nil)
	gui.MacOSHelpItems = CountMenuItems(gui.MacOSHelpMenu);
    else
	gui.MacOSHelpItems = 0;
#endif

    dragRectEnbl = FALSE;
    dragRgn = NULL;
    dragRectControl = kCreateEmpty;
    cursorRgn = NewRgn();
#endif
    /* Display any pending error messages */
    display_errors();

    /* Get background/foreground colors from system */
    /* TODO: do the approriate call to get real defaults */
    gui.norm_pixel = 0x00000000;
    gui.back_pixel = 0x00FFFFFF;

    /* Get the colors from the "Normal" group (set in syntax.c or in a vimrc
     * file). */
    set_normal_colors();

    /*
     * Check that none of the colors are the same as the background color.
     * Then store the current values as the defaults.
     */
    gui_check_colors();
    gui.def_norm_pixel = gui.norm_pixel;
    gui.def_back_pixel = gui.back_pixel;

    /* Get the colors for the highlight groups (gui_check_colors() might have
     * changed them) */
    highlight_gui_started();

    /*
     * Setting the gui constants
     */
#ifdef FEAT_MENU
    gui.menu_height = 0;
#endif
    gui.scrollbar_height = gui.scrollbar_width = 15; /* cheat 1 overlap */
    gui.border_offset = gui.border_width = 2;

#if defined(FEAT_GUI) && defined(MACOS_X)
    /* If Quartz-style text antialiasing is available (see
       gui_mch_draw_string() below), enable it for all font sizes. */
    vim_setenv((char_u *)"QDTEXT_MINSIZE", (char_u *)"1");
#endif

#ifdef USE_MOUSEWHEEL
    eventTypeSpec.eventClass = kEventClassMouse;
    eventTypeSpec.eventKind = kEventMouseWheelMoved;
    mouseWheelHandlerUPP = NewEventHandlerUPP(gui_mac_mouse_wheel);
    if (noErr != InstallApplicationEventHandler(mouseWheelHandlerUPP, 1,
				 &eventTypeSpec, NULL, &mouseWheelHandlerRef))
    {
	mouseWheelHandlerRef = NULL;
	DisposeEventHandlerUPP(mouseWheelHandlerUPP);
	mouseWheelHandlerUPP = NULL;
    }
#endif

#ifdef FEAT_MBYTE
    set_option_value((char_u *)"termencoding", 0L, (char_u *)"macroman", 0);
#endif

    /* TODO: Load bitmap if using TOOLBAR */
    return OK;
}

/*
 * Called when the foreground or background color has been changed.
 */
    void
gui_mch_new_colors()
{
    /* TODO:
     * This proc is called when Normal is set to a value
     * so what msut be done? I don't know
     */
}

/*
 * Open the GUI window which was created by a call to gui_mch_init().
 */
    int
gui_mch_open()
{
    ShowWindow(gui.VimWindow);

    if (gui_win_x != -1 && gui_win_y != -1)
	gui_mch_set_winpos(gui_win_x, gui_win_y);

#ifdef USE_CARBONIZED
    /*
     * Make the GUI the foreground process (in case it was launched
     * from the Terminal or via :gui).
     */
    {
	ProcessSerialNumber psn;
	if (GetCurrentProcess(&psn) == noErr)
	    SetFrontProcess(&psn);
    }
#endif

    return OK;
}

    void
gui_mch_exit(int rc)
{
    /* TODO: find out all what is missing here? */
    DisposeRgn(cursorRgn);

#ifdef USE_MOUSEWHEEL
    if (mouseWheelHandlerUPP != NULL)
	DisposeEventHandlerUPP(mouseWheelHandlerUPP);
#endif

    /* Exit to shell? */
    exit(rc);
}

/*
 * Get the position of the top left corner of the window.
 */
    int
gui_mch_get_winpos(int *x, int *y)
{
    /* TODO */
#ifdef USE_CARBONIZED
    Rect	bounds;
    OSStatus	status;

    /* Carbon >= 1.0.2, MacOS >= 8.5 */
    status = GetWindowBounds(gui.VimWindow, kWindowStructureRgn, &bounds);

    if (status != noErr)
	return FAIL;
    *x = bounds.left;
    *y = bounds.top;
    return OK;
#endif
    return FAIL;
}

/*
 * Set the position of the top left corner of the window to the given
 * coordinates.
 */
    void
gui_mch_set_winpos(int x, int y)
{
    /* TODO:  Should make sure the window is move within range
     *	      e.g.: y > ~16 [Menu bar], x > 0, x < screen width
     */
    MoveWindow(gui.VimWindow, x, y, TRUE);
}

    void
gui_mch_set_shellsize(
    int		width,
    int		height,
    int		min_width,
    int		min_height,
    int		base_width,
    int		base_height)
{
#ifdef USE_CARBONIZED
    CGrafPtr	VimPort;
    Rect	VimBound;
#endif

    if (gui.which_scrollbars[SBAR_LEFT])
    {
#ifdef USE_CARBONIZED
	VimPort = GetWindowPort(gui.VimWindow);
	GetPortBounds(VimPort, &VimBound);
	VimBound.left = -gui.scrollbar_width; /* + 1;*/
	SetPortBounds(VimPort, &VimBound);
    /*	GetWindowBounds(gui.VimWindow, kWindowGlobalPortRgn, &winPortRect); ??*/
#else
	gui.VimWindow->portRect.left = -gui.scrollbar_width; /* + 1;*/
    /*	SetWindowBounds(gui.VimWindow, kWindowGlobalPortRgn, &winPortRect); ??*/
#endif
    }
    else
    {
#ifdef USE_CARBONIZED
	VimPort = GetWindowPort(gui.VimWindow);
	GetPortBounds(VimPort, &VimBound);
	VimBound.left = 0;
	SetPortBounds(VimPort, &VimBound);
#else
	gui.VimWindow->portRect.left = 0;
#endif;
    }

    SizeWindow(gui.VimWindow, width, height, TRUE);

    gui_resize_shell(width, height);
}

/*
 * Get the screen dimensions.
 * Allow 10 pixels for horizontal borders, 40 for vertical borders.
 * Is there no way to find out how wide the borders really are?
 * TODO: Add live udate of those value on suspend/resume.
 */
    void
gui_mch_get_screen_dimensions(screen_w, screen_h)
    int		*screen_w;
    int		*screen_h;
{
    GDHandle	dominantDevice = GetMainDevice();
    Rect	screenRect = (**dominantDevice).gdRect;

    *screen_w = screenRect.right - 10;
    *screen_h = screenRect.bottom - 40;
}


#if defined(USE_CARBONIZED) && defined(MACOS_X)
/*
 * Open the Font Panel and wait for the user to select a font and
 * close the panel.  Then fill the buffer pointed to by font_name with
 * the name and size of the selected font and return the font's handle,
 * or NOFONT in case of an error.
 */
    static GuiFont
gui_mac_select_font(char_u *font_name)
{
    GuiFont		    selected_font = NOFONT;
    OSStatus		    status;
    FontSelectionQDStyle    curr_font;

    /* Initialize the Font Panel with the current font. */
    curr_font.instance.fontFamily = gui.norm_font & 0xFFFF;
    curr_font.size = (gui.norm_font >> 16);
    /* TODO: set fontStyle once styles are supported in gui_mac_find_font() */
    curr_font.instance.fontStyle = 0;
    curr_font.hasColor = false;
    curr_font.version = 0; /* version number of the style structure */
    status = SetFontInfoForSelection(kFontSelectionQDType,
	    /*numStyles=*/1, &curr_font, /*eventTarget=*/NULL);

    gFontPanelInfo.family = curr_font.instance.fontFamily;
    gFontPanelInfo.style = curr_font.instance.fontStyle;
    gFontPanelInfo.size = curr_font.size;

    /* Pop up the Font Panel. */
    status = FPShowHideFontPanel();
    if (status == noErr)
    {
	/*
	 * The Font Panel is modeless.  We really need it to be modal,
	 * so we spin in an event loop until the panel is closed.
	 */
	gFontPanelInfo.isPanelVisible = true;
	while (gFontPanelInfo.isPanelVisible)
	{
	    EventRecord e;
	    WaitNextEvent(everyEvent, &e, /*sleep=*/20, /*mouseRgn=*/NULL);
	}

	GetFontPanelSelection(font_name);
	selected_font = gui_mac_find_font(font_name);
    }
    return selected_font;
}
#endif


/*
 * Initialise vim to use the font with the given name.	Return FAIL if the font
 * could not be loaded, OK otherwise.
 */
    int
gui_mch_init_font(font_name, fontset)
    char_u	*font_name;
    int		fontset;	    /* not used */
{
    /* TODO: Add support for bold italic underline proportional etc... */
    Str255	suggestedFont = "\pMonaco";
    int		suggestedSize = 9;
    FontInfo	font_info;
    short	font_id;
    GuiFont	font;

    if (font_name == NULL)
    {
	/* First try to get the suggested font */
	GetFNum(suggestedFont, &font_id);

	if (font_id == 0)
	{
	    /* Then pickup the standard application font */
	    font_id = GetAppFont();
	}
	font = (suggestedSize << 16) + ((long) font_id & 0xFFFF);
    }
#if defined(USE_CARBONIZED) && defined(MACOS_X)
    else if (STRCMP(font_name, "*") == 0)
    {
	char_u *new_p_guifont, font_name[512];

	font = gui_mac_select_font(font_name);
	if (font == NOFONT)
	    return FAIL;

	/* Set guifont to the name of the selected font. */
	new_p_guifont = alloc(STRLEN(font_name) + 1);
	if (new_p_guifont != NULL)
	{
	    STRCPY(new_p_guifont, font_name);
	    vim_free(p_guifont);
	    p_guifont = new_p_guifont;
	    /* Replace spaces in the font name with underscores. */
	    for ( ; *new_p_guifont; ++new_p_guifont)
	    {
		if (*new_p_guifont == ' ')
		    *new_p_guifont = '_';
	    }
	}
    }
#endif
    else
    {
	font = gui_mac_find_font(font_name);

	if (font == NOFONT)
	    return FAIL;
    }
    gui.norm_font = font;

    TextSize(font >> 16);
    TextFont(font & 0xFFFF);

    GetFontInfo(&font_info);

    gui.char_ascent = font_info.ascent;
    gui.char_width  = CharWidth('_');
    gui.char_height = font_info.ascent + font_info.descent + p_linespace;

    return OK;

}

    int
gui_mch_adjust_charsize()
{
    FontInfo    font_info;

    GetFontInfo(&font_info);
    gui.char_height = font_info.ascent + font_info.descent + p_linespace;
    gui.char_ascent = font_info.ascent + p_linespace / 2;
    return OK;
}

/*
 * Get a font structure for highlighting.
 */
    GuiFont
gui_mch_get_font(name, giveErrorIfMissing)
    char_u	*name;
    int		giveErrorIfMissing;
{
    GuiFont font;

    font = gui_mac_find_font(name);

    if (font == NOFONT)
    {
	if (giveErrorIfMissing)
	    EMSG2(_(e_font), name);
	return NOFONT;
    }
    /*
     * TODO : Accept only monospace
     */

    return font;
}

/*
 * Set the current text font.
 */
    void
gui_mch_set_font(font)
    GuiFont	font;
{
    /*
     * TODO: maybe avoid set again the current font.
     */
    TextSize(font >> 16);
    TextFont(font & 0xFFFF);
}

#if 0 /* not used */
/*
 * Return TRUE if the two fonts given are equivalent.
 */
    int
gui_mch_same_font(f1, f2)
    GuiFont	f1;
    GuiFont	f2;
{
    return f1 == f2;
}
#endif

/*
 * If a font is not going to be used, free its structure.
 */
    void
gui_mch_free_font(font)
    GuiFont	font;
{
    /*
     * Free font when "font" is not 0.
     * Nothing to do in the current implementation, since
     * nothing is allocated for each font used.
     */
}

    static int
hex_digit(c)
    int		c;
{
    if (isdigit(c))
	return c - '0';
    c = TOLOWER_ASC(c);
    if (c >= 'a' && c <= 'f')
	return c - 'a' + 10;
    return -1000;
}

/*
 * Return the Pixel value (color) for the given color name.  This routine was
 * pretty much taken from example code in the Silicon Graphics OSF/Motif
 * Programmer's Guide.
 * Return INVALCOLOR when failed.
 */
    guicolor_T
gui_mch_get_color(name)
    char_u *name;
{
    /* TODO: Add support for the new named color of MacOS 8
     */
    RGBColor	MacColor;
//    guicolor_T	color = 0;

    typedef struct guicolor_tTable
    {
	char	    *name;
	guicolor_T  color;
    } guicolor_tTable;

    /*
     * The comment at the end of each line is the source
     * (Mac, Window, Unix) and the number is the unix rgb.txt value
     */
    static guicolor_tTable table[] =
    {
	{"Black",	RGB(0x00, 0x00, 0x00)},
	{"darkgray",	RGB(0x80, 0x80, 0x80)}, /*W*/
	{"darkgrey",	RGB(0x80, 0x80, 0x80)}, /*W*/
	{"Gray",	RGB(0xC0, 0xC0, 0xC0)}, /*W*/
	{"Grey",	RGB(0xC0, 0xC0, 0xC0)}, /*W*/
	{"lightgray",	RGB(0xE0, 0xE0, 0xE0)}, /*W*/
	{"lightgrey",	RGB(0xE0, 0xE0, 0xE0)}, /*W*/
	{"white",	RGB(0xFF, 0xFF, 0xFF)},
	{"darkred",	RGB(0x80, 0x00, 0x00)}, /*W*/
	{"red",		RGB(0xDD, 0x08, 0x06)}, /*M*/
	{"lightred",	RGB(0xFF, 0xA0, 0xA0)}, /*W*/
	{"DarkBlue",	RGB(0x00, 0x00, 0x80)}, /*W*/
	{"Blue",	RGB(0x00, 0x00, 0xD4)}, /*M*/
	{"lightblue",	RGB(0xA0, 0xA0, 0xFF)}, /*W*/
	{"DarkGreen",	RGB(0x00, 0x80, 0x00)}, /*W*/
	{"Green",	RGB(0x00, 0x64, 0x11)}, /*M*/
	{"lightgreen",	RGB(0xA0, 0xFF, 0xA0)}, /*W*/
	{"DarkCyan",	RGB(0x00, 0x80, 0x80)}, /*W ?0x307D7E */
	{"cyan",	RGB(0x02, 0xAB, 0xEA)}, /*M*/
	{"lightcyan",	RGB(0xA0, 0xFF, 0xFF)}, /*W*/
	{"darkmagenta",	RGB(0x80, 0x00, 0x80)}, /*W*/
	{"magenta",	RGB(0xF2, 0x08, 0x84)}, /*M*/
	{"lightmagenta",RGB(0xF0, 0xA0, 0xF0)}, /*W*/
	{"brown",	RGB(0x80, 0x40, 0x40)}, /*W*/
	{"yellow",	RGB(0xFC, 0xF3, 0x05)}, /*M*/
	{"lightyellow",	RGB(0xFF, 0xFF, 0xA0)}, /*M*/
	{"SeaGreen",	RGB(0x2E, 0x8B, 0x57)}, /*W 0x4E8975 */
	{"orange",	RGB(0xFC, 0x80, 0x00)}, /*W 0xF87A17 */
	{"Purple",	RGB(0xA0, 0x20, 0xF0)}, /*W 0x8e35e5 */
	{"SlateBlue",	RGB(0x6A, 0x5A, 0xCD)}, /*W 0x737CA1 */
	{"Violet",	RGB(0x8D, 0x38, 0xC9)}, /*U*/
    };

    int		r, g, b;
    int		i;

    if (name[0] == '#' && strlen((char *) name) == 7)
    {
	/* Name is in "#rrggbb" format */
	r = hex_digit(name[1]) * 16 + hex_digit(name[2]);
	g = hex_digit(name[3]) * 16 + hex_digit(name[4]);
	b = hex_digit(name[5]) * 16 + hex_digit(name[6]);
	if (r < 0 || g < 0 || b < 0)
	    return INVALCOLOR;
	return RGB(r, g, b);
    }
    else
    {
	if (STRICMP(name, "hilite") == 0)
	{
	    LMGetHiliteRGB(&MacColor);
	    return (RGB(MacColor.red >> 8, MacColor.green >> 8, MacColor.blue >> 8));
	}
	/* Check if the name is one of the colors we know */
	for (i = 0; i < sizeof(table) / sizeof(table[0]); i++)
	    if (STRICMP(name, table[i].name) == 0)
		return table[i].color;
    }


    /*
     * Last attempt. Look in the file "$VIM/rgb.txt".
     */
    {
#define LINE_LEN 100
	FILE	*fd;
	char	line[LINE_LEN];
	char_u	*fname;

#ifdef COLON_AS_PATHSEP
	fname = expand_env_save((char_u *)"$VIMRUNTIME:rgb.txt");
#else
	fname = expand_env_save((char_u *)"$VIMRUNTIME/rgb.txt");
#endif
	if (fname == NULL)
	    return INVALCOLOR;

	fd = fopen((char *)fname, "rt");
	vim_free(fname);
	if (fd == NULL)
	    return INVALCOLOR;

	while (!feof(fd))
	{
	    int		len;
	    int		pos;
	    char	*color;

	    fgets(line, LINE_LEN, fd);
	    len = strlen(line);

	    if (len <= 1 || line[len-1] != '\n')
		continue;

	    line[len-1] = '\0';

	    i = sscanf(line, "%d %d %d %n", &r, &g, &b, &pos);
	    if (i != 3)
		continue;

	    color = line + pos;

	    if (STRICMP(color, name) == 0)
	    {
		fclose(fd);
		return (guicolor_T) RGB(r, g, b);
	    }
	}
	fclose(fd);
    }

    return INVALCOLOR;
}

/*
 * Set the current text foreground color.
 */
    void
gui_mch_set_fg_color(color)
    guicolor_T	color;
{
    RGBColor TheColor;

    TheColor.red = Red(color) * 0x0101;
    TheColor.green = Green(color) * 0x0101;
    TheColor.blue = Blue(color) * 0x0101;

    RGBForeColor(&TheColor);
}

/*
 * Set the current text background color.
 */
    void
gui_mch_set_bg_color(color)
    guicolor_T	color;
{
    RGBColor TheColor;

    TheColor.red = Red(color) * 0x0101;
    TheColor.green = Green(color) * 0x0101;
    TheColor.blue = Blue(color) * 0x0101;

    RGBBackColor(&TheColor);
}

    void
gui_mch_draw_string(row, col, s, len, flags)
    int		row;
    int		col;
    char_u	*s;
    int		len;
    int		flags;
{
#if defined(FEAT_GUI) && defined(MACOS_X)
    SInt32	sys_version;
#endif
#ifdef FEAT_MBYTE
    char_u	*tofree = NULL;

    if (output_conv.vc_type != CONV_NONE)
    {
	tofree = string_convert(&output_conv, s, &len);
	if (tofree != NULL)
	    s = tofree;
    }
#endif

#if defined(FEAT_GUI) && defined(MACOS_X)
    /*
     * On OS X, try using Quartz-style text antialiasing.
     */
    sys_version = 0;

    Gestalt(gestaltSystemVersion, &sys_version);
    if (sys_version >= 0x1020)
    {
	/* Quartz antialiasing is available only in OS 10.2 and later. */
	UInt32 qd_flags = (p_antialias ?
			     kQDUseCGTextRendering | kQDUseCGTextMetrics : 0);
	(void)SwapQDTextFlags(qd_flags);
    }

    /*
     * When antialiasing we're using srcOr mode, we have to clear the block
     * before drawing the text.
     * Also needed when 'linespace' is non-zero to remove the cursor and
     * underlining.
     * But not when drawing transparently.
     * The following is like calling gui_mch_clear_block(row, col, row, col +
     * len - 1), but without setting the bg color to gui.back_pixel.
     */
    if (((sys_version >= 0x1020 && p_antialias) || p_linespace != 0)
	    && !(flags & DRAW_TRANSP))
    {
	Rect rc;

	rc.left = FILL_X(col);
	rc.top = FILL_Y(row);
	rc.right = FILL_X(col + len) + (col + len == Columns);
	rc.bottom = FILL_Y(row + 1);
	EraseRect(&rc);
    }

    if (sys_version >= 0x1020 && p_antialias)
    {
	StyleParameter face;

	face = normal;
	if (flags & DRAW_BOLD)
	    face |= bold;
	if (flags & DRAW_UNDERL)
	    face |= underline;
	TextFace(face);

	/* Quartz antialiasing works only in srcOr transfer mode. */
	TextMode(srcOr);

	MoveTo(TEXT_X(col), TEXT_Y(row));
	DrawText((char*)s, 0, len);
    }
    else
#endif
    {
	/* Use old-style, non-antialiased QuickDraw text rendering. */
	TextMode(srcCopy);
	TextFace(normal);

    /*  SelectFont(hdc, gui.currFont); */

	if (flags & DRAW_TRANSP)
	{
	    TextMode(srcOr);
	}

	MoveTo(TEXT_X(col), TEXT_Y(row));
	DrawText((char *)s, 0, len);


	if (flags & DRAW_BOLD)
	{
	    TextMode(srcOr);
	    MoveTo(TEXT_X(col) + 1, TEXT_Y(row));
	    DrawText((char *)s, 0, len);
	}

	if (flags & DRAW_UNDERL)
	{
	    MoveTo(FILL_X(col), FILL_Y(row + 1) - 1);
	    LineTo(FILL_X(col + len) - 1, FILL_Y(row + 1) - 1);
	}
    }

#ifdef FEAT_MBYTE
    vim_free(tofree);
#endif
}

/*
 * Return OK if the key with the termcap name "name" is supported.
 */
    int
gui_mch_haskey(name)
    char_u  *name;
{
    int i;

    for (i = 0; special_keys[i].key_sym != (KeySym)0; i++)
	if (name[0] == special_keys[i].vim_code0 &&
					 name[1] == special_keys[i].vim_code1)
	    return OK;
    return FAIL;
}

    void
gui_mch_beep()
{
    SysBeep(1); /* Should this be 0? (????) */
}

    void
gui_mch_flash(msec)
    int	    msec;
{
    /* Do a visual beep by reversing the foreground and background colors */
    Rect    rc;

    /*
     * Note: InvertRect() excludes right and bottom of rectangle.
     */
    rc.left = 0;
    rc.top = 0;
    rc.right = gui.num_cols * gui.char_width;
    rc.bottom = gui.num_rows * gui.char_height;
    InvertRect(&rc);

    ui_delay((long)msec, TRUE);		/* wait for some msec */

    InvertRect(&rc);
}

/*
 * Invert a rectangle from row r, column c, for nr rows and nc columns.
 */
    void
gui_mch_invert_rectangle(r, c, nr, nc)
    int		r;
    int		c;
    int		nr;
    int		nc;
{
    Rect	rc;

    /*
     * Note: InvertRect() excludes right and bottom of rectangle.
     */
    rc.left = FILL_X(c);
    rc.top = FILL_Y(r);
    rc.right = rc.left + nc * gui.char_width;
    rc.bottom = rc.top + nr * gui.char_height;
    InvertRect(&rc);

}

/*
 * Iconify the GUI window.
 */
    void
gui_mch_iconify()
{
    /* TODO: find out what could replace iconify
     *	     -window shade?
     *	     -hide application?
     */
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Bring the Vim window to the foreground.
 */
    void
gui_mch_set_foreground()
{
    /* TODO */
}
#endif

/*
 * Draw a cursor without focus.
 */
    void
gui_mch_draw_hollow_cursor(color)
    guicolor_T	color;
{
    Rect rc;

    gui_mch_set_fg_color(color);

    /*
     * Note: FrameRect() excludes right and bottom of rectangle.
     */
    rc.left = FILL_X(gui.col);
    rc.top = FILL_Y(gui.row);
    rc.right = rc.left + gui.char_width;
    rc.bottom = rc.top + gui.char_height;

    gui_mch_set_fg_color(color);

    FrameRect(&rc);
}

/*
 * Draw part of a cursor, only w pixels wide, and h pixels high.
 */
    void
gui_mch_draw_part_cursor(w, h, color)
    int		w;
    int		h;
    guicolor_T	color;
{
    Rect rc;

#ifdef FEAT_RIGHTLEFT
    /* vertical line should be on the right of current point */
    if (CURSOR_BAR_RIGHT)
	rc.left = FILL_X(gui.col + 1) - w;
    else
#endif
	rc.left = FILL_X(gui.col);
    rc.top = FILL_Y(gui.row) + gui.char_height - h;
    rc.right = rc.left + w;
    rc.bottom = rc.top + h;

    gui_mch_set_fg_color(color);

    PaintRect(&rc);
}



/*
 * Catch up with any queued X events.  This may put keyboard input into the
 * input buffer, call resize call-backs, trigger timers etc.  If there is
 * nothing in the X event queue (& no timers pending), then we return
 * immediately.
 */
    void
gui_mch_update()
{
    /* TODO: find what to do
     *	     maybe call gui_mch_wait_for_chars (0)
     *	     more like look at EventQueue then
     *	     call heart of gui_mch_wait_for_chars;
     *
     *	if (eventther)
     *	    gui_mac_handle_event(&event);
     */
    EventRecord theEvent;

    if (EventAvail(everyEvent, &theEvent))
	if (theEvent.what != nullEvent)
	    gui_mch_wait_for_chars(0);
}

/*
 * Simple wrapper to neglect more easily the time
 * spent inside WaitNextEvent while profiling.
 */

#if defined(__MWERKS__)  /* only in Codewarrior */
# pragma profile reset
#endif
    pascal
    Boolean
WaitNextEventWrp(EventMask eventMask, EventRecord *theEvent, UInt32 sleep, RgnHandle mouseRgn)
{
    if (((long) sleep) < -1)
	sleep = 32767;
    return WaitNextEvent(eventMask, theEvent, sleep, mouseRgn);
}

/*
 * GUI input routine called by gui_wait_for_chars().  Waits for a character
 * from the keyboard.
 *  wtime == -1	    Wait forever.
 *  wtime == 0	    This should never happen.
 *  wtime > 0	    Wait wtime milliseconds for a character.
 * Returns OK if a character was found to be available within the given time,
 * or FAIL otherwise.
 */
#if defined(__MWERKS__)  /* only in Codewarrior */
# pragma profile reset
#endif
    int
gui_mch_wait_for_chars(wtime)
    int	    wtime;
{
    EventMask	mask  = (everyEvent);
    EventRecord event;
    long	entryTick;
    long	currentTick;
    long	sleeppyTick;

    /* If we are providing life feedback with the scrollbar,
     * we don't want to try to wait for an event, or else
     * there won't be any life feedback.
     */
    if (dragged_sb != NULL)
	return FAIL;
	/* TODO: Check if FAIL is the proper return code */

    entryTick = TickCount();

    allow_scrollbar = TRUE;

    do
    {
/*	if (dragRectControl == kCreateEmpty)
	{
	    dragRgn = NULL;
	    dragRectControl = kNothing;
	}
	else*/ if (dragRectControl == kCreateRect)
	{
	    dragRgn = cursorRgn;
	    RectRgn(dragRgn, &dragRect);
	    dragRectControl = kNothing;
	}
	/*
	 * Don't use gui_mch_update() because then we will spin-lock until a
	 * char arrives, instead we use WaitNextEventWrp() to hang until an
	 * event arrives.  No need to check for input_buf_full because we are
	 * returning as soon as it contains a single char.
	 */
	/* TODO: reduce wtime accordinly???  */
	if (wtime > -1)
	    sleeppyTick = 60*wtime/1000;
	else
	    sleeppyTick = 32767;
	if (WaitNextEventWrp(mask, &event, sleeppyTick, dragRgn))
	{
#ifdef USE_SIOUX
	    if (!SIOUXHandleOneEvent(&event))
#endif
		gui_mac_handle_event(&event);
	    if (input_available())
	    {
		allow_scrollbar = FALSE;
		return OK;
	    }
	}
	currentTick = TickCount();
    }
    while ((wtime == -1) || ((currentTick - entryTick) < 60*wtime/1000));

    allow_scrollbar = FALSE;
    return FAIL;
}

#if defined(__MWERKS__)  /* only in Codewarrior */
# pragma profile reset
#endif

/*
 * Output routines.
 */

/* Flush any output to the screen */
    void
gui_mch_flush()
{
    /* TODO: Is anything needed here? */
}

/*
 * Clear a rectangular region of the screen from text pos (row1, col1) to
 * (row2, col2) inclusive.
 */
    void
gui_mch_clear_block(row1, col1, row2, col2)
    int		row1;
    int		col1;
    int		row2;
    int		col2;
{
    Rect rc;

    /*
     * Clear one extra pixel at the far right, for when bold characters have
     * spilled over to the next column.
     */
    rc.left = FILL_X(col1);
    rc.top = FILL_Y(row1);
    rc.right = FILL_X(col2 + 1) + (col2 == Columns - 1);
    rc.bottom = FILL_Y(row2 + 1);

    gui_mch_set_bg_color(gui.back_pixel);
    EraseRect(&rc);
}

/*
 * Clear the whole text window.
 */
    void
gui_mch_clear_all()
{
    Rect	rc;

    rc.left = 0;
    rc.top = 0;
    rc.right = Columns * gui.char_width + 2 * gui.border_width;
    rc.bottom = Rows * gui.char_height + 2 * gui.border_width;

    gui_mch_set_bg_color(gui.back_pixel);
    EraseRect(&rc);
/*  gui_mch_set_fg_color(gui.norm_pixel);
    FrameRect(&rc);
*/
}

/*
 * Delete the given number of lines from the given row, scrolling up any
 * text further down within the scroll region.
 */
    void
gui_mch_delete_lines(row, num_lines)
    int		row;
    int		num_lines;
{
    Rect	rc;

    /* changed without checking! */
    rc.left = FILL_X(gui.scroll_region_left);
    rc.right = FILL_X(gui.scroll_region_right + 1);
    rc.top = FILL_Y(row);
    rc.bottom = FILL_Y(gui.scroll_region_bot + 1);

    gui_mch_set_bg_color(gui.back_pixel);
    ScrollRect(&rc, 0, -num_lines * gui.char_height, (RgnHandle) nil);

    gui_clear_block(gui.scroll_region_bot - num_lines + 1,
						       gui.scroll_region_left,
	gui.scroll_region_bot, gui.scroll_region_right);
}

/*
 * Insert the given number of lines before the given row, scrolling down any
 * following text within the scroll region.
 */
    void
gui_mch_insert_lines(row, num_lines)
    int		row;
    int		num_lines;
{
    Rect rc;

    rc.left = FILL_X(gui.scroll_region_left);
    rc.right = FILL_X(gui.scroll_region_right + 1);
    rc.top = FILL_Y(row);
    rc.bottom = FILL_Y(gui.scroll_region_bot + 1);

    gui_mch_set_bg_color(gui.back_pixel);

    ScrollRect(&rc, 0, gui.char_height * num_lines, (RgnHandle) nil);

    /* Update gui.cursor_row if the cursor scrolled or copied over */
    if (gui.cursor_row >= gui.row
	    && gui.cursor_col >= gui.scroll_region_left
	    && gui.cursor_col <= gui.scroll_region_right)
    {
	if (gui.cursor_row <= gui.scroll_region_bot - num_lines)
	    gui.cursor_row += num_lines;
	else if (gui.cursor_row <= gui.scroll_region_bot)
	    gui.cursor_is_valid = FALSE;
    }

    gui_clear_block(row, gui.scroll_region_left,
				row + num_lines - 1, gui.scroll_region_right);
}

    /*
     * TODO: add a vim format to the clipboard which remember
     *	     LINEWISE, CHARWISE, BLOCKWISE
     */

    void
clip_mch_request_selection(cbd)
    VimClipboard *cbd;
{

    Handle	textOfClip;
    int		flavor = 0;
#ifdef USE_CARBONIZED
    Size	scrapSize;
    ScrapFlavorFlags	scrapFlags;
    ScrapRef    scrap = nil;
    OSStatus	error;
#else
    long	scrapOffset;
    long	scrapSize;
#endif
    int		type;
    char	*searchCR;
    char_u	*tempclip;


#ifdef USE_CARBONIZED
    error = GetCurrentScrap(&scrap);
    if (error != noErr)
	return;

    error = GetScrapFlavorFlags(scrap, VIMSCRAPFLAVOR, &scrapFlags);
    if (error == noErr)
    {
	error = GetScrapFlavorSize(scrap, VIMSCRAPFLAVOR, &scrapSize);
	if (error == noErr && scrapSize > 1)
	    flavor = 1;
    }

    if (flavor == 0)
    {
	error = GetScrapFlavorFlags(scrap, kScrapFlavorTypeText, &scrapFlags);
	if (error != noErr)
	    return;

	error = GetScrapFlavorSize(scrap, kScrapFlavorTypeText, &scrapSize);
	if (error != noErr)
	    return;
    }

    ReserveMem(scrapSize);
#else
    /* Call to LoadScrap seem to avoid problem with crash on first paste */
    scrapSize = LoadScrap();
    scrapSize = GetScrap(nil, 'TEXT', &scrapOffset);

    if (scrapSize > 0)
#endif
    {
#ifdef USE_CARBONIZED
	/* In CARBON we don't need a Handle, a pointer is good */
	textOfClip = NewHandle(scrapSize);
	/* tempclip = lalloc(scrapSize+1, TRUE); */
#else
	textOfClip = NewHandle(0);
#endif
	HLock(textOfClip);
#ifdef USE_CARBONIZED
	error = GetScrapFlavorData(scrap,
		flavor ? VIMSCRAPFLAVOR : kScrapFlavorTypeText,
		&scrapSize, *textOfClip);
#else
	scrapSize = GetScrap(textOfClip, 'TEXT', &scrapOffset);
#endif
	scrapSize -= flavor;

	if (flavor)
	    type = **textOfClip;
	else
	    type = (strchr(*textOfClip, '\r') != NULL) ? MLINE : MCHAR;

	tempclip = lalloc(scrapSize + 1, TRUE);
	STRNCPY(tempclip, *textOfClip + flavor, scrapSize);
	tempclip[scrapSize] = 0;

	searchCR = (char *)tempclip;
	while (searchCR != NULL)
	{
	    searchCR = strchr(searchCR, '\r');

	    if (searchCR != NULL)
		searchCR[0] = '\n';

	}

#ifdef FEAT_MBYTE
	if (input_conv.vc_type != CONV_NONE)
	{
	    char_u	*to;
	    int		l = scrapSize;

	    to = string_convert(&input_conv, tempclip, &l);
	    if (to != NULL)
	    {
		vim_free(tempclip);
		tempclip = to;
		scrapSize = l;
	    }
	}
#endif
	clip_yank_selection(type, tempclip, scrapSize, cbd);

	vim_free(tempclip);
	HUnlock(textOfClip);

	DisposeHandle(textOfClip);
    }
}

    void
clip_mch_lose_selection(cbd)
    VimClipboard *cbd;
{
    /*
     * TODO: Really nothing to do?
     */
}

    int
clip_mch_own_selection(cbd)
    VimClipboard *cbd;
{
    return OK;
}

/*
 * Send the current selection to the clipboard.
 */
    void
clip_mch_set_selection(cbd)
    VimClipboard *cbd;
{
    Handle	textOfClip;
    long	scrapSize;
    int		type;
#ifdef USE_CARBONIZED
    ScrapRef    scrap;
#endif

    char_u	*str = NULL;

    if (!cbd->owned)
	return;

    clip_get_selection(cbd);

    /*
     * Once we set the clipboard, lose ownership.  If another application sets
     * the clipboard, we don't want to think that we still own it.
     *
     */

    cbd->owned = FALSE;

    type = clip_convert_selection(&str, (long_u *) &scrapSize, cbd);

#ifdef FEAT_MBYTE
    if (str != NULL && output_conv.vc_type != CONV_NONE)
    {
	char_u	*to;
	int	l = scrapSize;

	to = string_convert(&output_conv, str, &l);
	if (to != NULL)
	{
	    vim_free(str);
	    str = to;
	    scrapSize = l;
	}
    }
#endif

    if (type >= 0)
    {
#ifdef USE_CARBONIZED
	ClearCurrentScrap();
#else
	ZeroScrap();
#endif

#ifdef USE_CARBONIZED
	textOfClip = NewHandle(scrapSize + 1);
#else
	textOfClip = NewHandle(scrapSize);
#endif
	HLock(textOfClip);

#ifdef USE_CARBONIZED
	**textOfClip = type;
	STRNCPY(*textOfClip + 1, str, scrapSize);
	GetCurrentScrap(&scrap);
	PutScrapFlavor(scrap, kScrapFlavorTypeText, kScrapFlavorMaskNone,
		scrapSize, *textOfClip + 1);
	PutScrapFlavor(scrap, VIMSCRAPFLAVOR, kScrapFlavorMaskNone,
		scrapSize + 1, *textOfClip);
#else
	STRNCPY(*textOfClip, str, scrapSize);
	PutScrap(scrapSize, 'TEXT', *textOfClip);
#endif
	HUnlock(textOfClip);
	DisposeHandle(textOfClip);
    }

    vim_free(str);
}

    void
gui_mch_set_text_area_pos(x, y, w, h)
    int		x;
    int		y;
    int		w;
    int		h;
{
    Rect	VimBound;

/*  HideWindow(gui.VimWindow); */
#ifdef USE_CARBONIZED
    GetWindowBounds(gui.VimWindow, kWindowGlobalPortRgn, &VimBound);
#else
    VimBound = gui.VimWindow->portRect;
#endif

    if (gui.which_scrollbars[SBAR_LEFT])
    {
	VimBound.left = -gui.scrollbar_width + 1;
    }
    else
    {
	VimBound.left = 0;
    }

#ifdef USE_CARBONIZED
    SetWindowBounds(gui.VimWindow, kWindowGlobalPortRgn, &VimBound);
#endif

    ShowWindow(gui.VimWindow);
}

/*
 * Menu stuff.
 */

    void
gui_mch_enable_menu(flag)
    int		flag;
{
    /*
     * Menu is always active in itself
     * (maybe we should only disable a vim menu
     *	and keep standard menu)
     *
     */
}

    void
gui_mch_set_menu_pos(x, y, w, h)
    int		x;
    int		y;
    int		w;
    int		h;
{
    /*
     * The menu is always at the top of the screen
     * Maybe a futur version will permit a menu in the window
     *
     */
}

/*
 * Add a sub menu to the menu bar.
 */
    void
gui_mch_add_menu(menu, idx)
    vimmenu_T	*menu;
    int		idx;
{
    /*
     * TODO: Try to use only menu_id instead of both menu_id and menu_handle.
     * TODO: use menu->mnemonic and menu->actext
     * TODO: Try to reuse menu id
     *       Carbon Help suggest to use only id between 1 and 235
     */
    static long	 next_avail_id = 128;
    long	 menu_after_me = 0; /* Default to the end */
    char_u	*name;
    short	 index;
    vimmenu_T	*parent = menu->parent;
    vimmenu_T	*brother = menu->next;

    /* Cannot add a menu if ... */
    if ((parent != NULL && parent->submenu_id == 0))
	return;

    /* menu ID greater than 1024 are reserved for ??? */
    if (next_avail_id == 1024)
	return;

    /* My brother could be the PopUp, find my real brother */
    while ((brother != NULL) && (!menu_is_menubar(brother->name)))
	brother = brother->next;

    /*  Find where to insert the menu (for MenuBar) */
    if ((parent == NULL) && (brother != NULL))
	menu_after_me = brother->submenu_id;

    /* If the menu is not part of the menubar (and its submenus), add it 'nowhere' */
    if (!menu_is_menubar(menu->name))
	menu_after_me = hierMenu;

    /* Convert the name */
    name = C2Pascal_save(menu->dname);
    if (name == NULL)
	return;

    /* Create the menu unless it's the help menu */
#ifdef USE_HELPMENU
    if (STRNCMP(name, "\4Help", 5) == 0)
    {
	menu->submenu_id = kHMHelpMenuID;
	menu->submenu_handle = gui.MacOSHelpMenu;
    }
    else
#endif
    {
	/* Carbon suggest use of
	 * OSStatus CreateNewMenu(MenuID, MenuAttributes, MenuRef *);
	 * OSStatus SetMenuTitle(MenuRef, ConstStr255Param title);
	 */
	menu->submenu_id = next_avail_id;
	menu->submenu_handle = NewMenu(menu->submenu_id, name);
	next_avail_id++;
    }

    if (parent == NULL)
    {
	/* Adding a menu to the menubar, or in the no mans land (for PopUp) */

	/* TODO: Verify if we could only Insert Menu if really part of the
	 * menubar The Inserted menu are scanned or the Command-key combos
	 */

	/* Insert the menu unless it's the Help menu */
#ifdef USE_HELPMENU
	if (menu->submenu_id != kHMHelpMenuID)
#endif
	    InsertMenu(menu->submenu_handle, menu_after_me); /* insert before */
#if 1
	/* Vim should normally update it. TODO: verify */
	DrawMenuBar();
#endif
    }
    else
    {
	/* Adding as a submenu */

	index = gui_mac_get_menu_item_index(menu);

	/* Call InsertMenuItem followed by SetMenuItemText
	 * to avoid special character recognition by InsertMenuItem
	 */
	InsertMenuItem(parent->submenu_handle, "\p ", idx); /* afterItem */
	SetMenuItemText(parent->submenu_handle, idx+1, name);
	SetItemCmd(parent->submenu_handle, idx+1, 0x1B);
	SetItemMark(parent->submenu_handle, idx+1, menu->submenu_id);
	InsertMenu(menu->submenu_handle, hierMenu);
    }

    vim_free(name);

#if 0
    /* Done by Vim later on */
    DrawMenuBar();
#endif
}

/*
 * Add a menu item to a menu
 */
    void
gui_mch_add_menu_item(menu, idx)
    vimmenu_T	*menu;
    int		idx;
{
    char_u	*name;
    vimmenu_T	*parent = menu->parent;
    int		menu_inserted;

    /* Cannot add item, if the menu have not been created */
    if (parent->submenu_id == 0)
	return;

    /* Could call SetMenuRefCon [CARBON] to associate with the Menu,
       for older OS call GetMenuItemData (menu, item, isCommandID?, data) */

    /* Convert the name */
    name = C2Pascal_save(menu->dname);

    /* Where are just a menu item, so no handle, no id */
    menu->submenu_id = 0;
    menu->submenu_handle = NULL;

#ifdef USE_HELPMENU
    /* The index in the help menu are offseted */
    if (parent->submenu_id == kHMHelpMenuID)
	idx += gui.MacOSHelpItems;
#endif

    menu_inserted = 0;
    if (menu->actext)
    {
	/* If the accelerator text for the menu item looks like it describes
	 * a command key (e.g., "<D-S-t>" or "<C-7>"), display it as the
	 * item's command equivalent.
	 */
	int	    key = 0;
	int	    modifiers = 0;
	char_u	    *p_actext;

	p_actext = menu->actext;
	key = find_special_key(&p_actext, &modifiers, /*keycode=*/0);
	if (*p_actext != 0)
	    key = 0; /* error: trailing text */
	/* find_special_key() returns a keycode with as many of the
	 * specified modifiers as appropriate already applied (e.g., for
	 * "<D-C-x>" it returns Ctrl-X as the keycode and MOD_MASK_CMD
	 * as the only modifier).  Since we want to display all of the
	 * modifiers, we need to convert the keycode back to a printable
	 * character plus modifiers.
	 * TODO: Write an alternative find_special_key() that doesn't
	 * apply modifiers.
	 */
	if (key > 0 && key < 32)
	{
	    /* Convert a control key to an uppercase letter.  Note that
	     * by this point it is no longer possible to distinguish
	     * between, e.g., Ctrl-S and Ctrl-Shift-S.
	     */
	    modifiers |= MOD_MASK_CTRL;
	    key += '@';
	}
	/* If the keycode is an uppercase letter, set the Shift modifier.
	 * If it is a lowercase letter, don't set the modifier, but convert
	 * the letter to uppercase for display in the menu.
	 */
	else if (key >= 'A' && key <= 'Z')
	    modifiers |= MOD_MASK_SHIFT;
	else if (key >= 'a' && key <= 'z')
	    key += 'A' - 'a';
	/* Note: keycodes below 0x22 are reserved by Apple. */
	if (key >= 0x22 && vim_isprintc_strict(key))
	{
	    int		valid = 1;
	    char_u      mac_mods = kMenuNoModifiers;
	    /* Convert Vim modifier codes to Menu Manager equivalents. */
	    if (modifiers & MOD_MASK_SHIFT)
		mac_mods |= kMenuShiftModifier;
	    if (modifiers & MOD_MASK_CTRL)
		mac_mods |= kMenuControlModifier;
	    if (!(modifiers & MOD_MASK_CMD))
		mac_mods |= kMenuNoCommandModifier;
	    if (modifiers & MOD_MASK_ALT || modifiers & MOD_MASK_MULTI_CLICK)
		valid = 0; /* TODO: will Alt someday map to Option? */
	    if (valid)
	    {
		char_u	    item_txt[10];
		/* Insert the menu item after idx, with its command key. */
		item_txt[0] = 3; item_txt[1] = ' '; item_txt[2] = '/';
		item_txt[3] = key;
		InsertMenuItem(parent->submenu_handle, item_txt, idx);
		/* Set the modifier keys. */
		SetMenuItemModifiers(parent->submenu_handle, idx+1, mac_mods);
		menu_inserted = 1;
	    }
	}
    }
    /* Call InsertMenuItem followed by SetMenuItemText
     * to avoid special character recognition by InsertMenuItem
     */
    if (!menu_inserted)
	InsertMenuItem(parent->submenu_handle, "\p ", idx); /* afterItem */
    /* Set the menu item name. */
    SetMenuItemText(parent->submenu_handle, idx+1, name);

#if 0
    /* Called by Vim */
    DrawMenuBar();
#endif

    /* TODO: Can name be freed? */
    vim_free(name);
}

    void
gui_mch_toggle_tearoffs(enable)
    int	    enable;
{
    /* no tearoff menus */
}

/*
 * Destroy the machine specific menu widget.
 */
    void
gui_mch_destroy_menu(menu)
    vimmenu_T	*menu;
{
    short	index = gui_mac_get_menu_item_index(menu);

    if (index > 0)
    {
      if (menu->parent)
      {
#ifdef USE_HELPMENU
	if (menu->parent->submenu_handle != nil) /*gui.MacOSHelpMenu)*/
#endif
	{
	    /* For now just don't delete help menu items. (Huh? Dany) */
	    DeleteMenuItem(menu->parent->submenu_handle, index);

	    /* Delete the Menu if it was a hierarchical Menu */
	    if (menu->submenu_id != 0)
	    {
		DeleteMenu(menu->submenu_id);
		DisposeMenu(menu->submenu_handle);
	    }
	}
#ifdef USE_HELPMENU
# ifdef DEBUG_MAC_MENU
	else
	{
	    printf("gmdm 1\n");
	}
# endif
#endif
      }
#ifdef DEBUG_MAC_MENU
      else
      {
	printf("gmdm 2\n");
      }
#endif
    }
    else
    {
	/* Do not delete the Help Menu */
#ifdef USE_HELPMENU
	if (menu->submenu_id != kHMHelpMenuID)
#endif
	{
	    DeleteMenu(menu->submenu_id);
	    DisposeMenu(menu->submenu_handle);
	}
    }
    /* Shouldn't this be already done by Vim. TODO: Check */
    DrawMenuBar();
}

/*
 * Make a menu either grey or not grey.
 */
    void
gui_mch_menu_grey(menu, grey)
    vimmenu_T	*menu;
    int		grey;
{
    /* TODO: Check if menu really exists */
    short index = gui_mac_get_menu_item_index(menu);
/*
    index = menu->index;
*/
    if (grey)
    {
	if (menu->children)
	    DisableMenuItem(menu->submenu_handle, index);
	if (menu->parent)
	  if (menu->parent->submenu_handle)
	    DisableMenuItem(menu->parent->submenu_handle, index);
    }
    else
    {
	if (menu->children)
	    EnableMenuItem(menu->submenu_handle, index);
	if (menu->parent)
	  if (menu->parent->submenu_handle)
	    EnableMenuItem(menu->parent->submenu_handle, index);
    }
}

/*
 * Make menu item hidden or not hidden
 */
    void
gui_mch_menu_hidden(menu, hidden)
    vimmenu_T	*menu;
    int		hidden;
{
    /* There's no hidden mode on MacOS */
    gui_mch_menu_grey(menu, hidden);
}


/*
 * This is called after setting all the menus to grey/hidden or not.
 */
    void
gui_mch_draw_menubar()
{
    DrawMenuBar();
}


/*
 * Scrollbar stuff.
 */

    void
gui_mch_enable_scrollbar(sb, flag)
    scrollbar_T	*sb;
    int		flag;
{
    if (flag)
	ShowControl(sb->id);
    else
	HideControl(sb->id);

#ifdef DEBUG_MAC_SB
    printf("enb_sb (%x) %x\n",sb->id, flag);
#endif
}

    void
gui_mch_set_scrollbar_thumb(sb, val, size, max)
    scrollbar_T *sb;
    long	val;
    long	size;
    long	max;
{
    SetControl32BitMaximum (sb->id, max);
    SetControl32BitMinimum (sb->id, 0);
    SetControl32BitValue   (sb->id, val);
#ifdef DEBUG_MAC_SB
    printf("thumb_sb (%x) %x, %x,%x\n",sb->id, val, size, max);
#endif
}

    void
gui_mch_set_scrollbar_pos(sb, x, y, w, h)
    scrollbar_T *sb;
    int		x;
    int		y;
    int		w;
    int		h;
{
    gui_mch_set_bg_color(gui.back_pixel);
/*  if (gui.which_scrollbars[SBAR_LEFT])
    {
	MoveControl(sb->id, x-16, y);
	SizeControl(sb->id, w + 1, h);
    }
    else
    {
	MoveControl(sb->id, x, y);
	SizeControl(sb->id, w + 1, h);
    }*/
    if (sb == &gui.bottom_sbar)
	h += 1;
    else
	w += 1;

    if (gui.which_scrollbars[SBAR_LEFT])
	x -= 15;

    MoveControl(sb->id, x, y);
    SizeControl(sb->id, w, h);
#ifdef DEBUG_MAC_SB
    printf("size_sb (%x) %x, %x, %x, %x\n",sb->id, x, y, w, h);
#endif
}

    void
gui_mch_create_scrollbar(sb, orient)
    scrollbar_T *sb;
    int		orient;	/* SBAR_VERT or SBAR_HORIZ */
{
    Rect bounds;

    bounds.top = -16;
    bounds.bottom = -10;
    bounds.right = -10;
    bounds.left = -16;

    sb->id = NewControl(gui.VimWindow,
			 &bounds,
			 "\pScrollBar",
			 TRUE,
			 0, /* current*/
			 0, /* top */
			 0, /* bottom */
#ifdef USE_CARBONIZED
			 kControlScrollBarLiveProc,
#else
			 scrollBarProc,
#endif
			 (long) sb->ident);
#ifdef DEBUG_MAC_SB
    printf("create_sb (%x) %x\n",sb->id, orient);
#endif
}

    void
gui_mch_destroy_scrollbar(sb)
    scrollbar_T *sb;
{
    gui_mch_set_bg_color(gui.back_pixel);
    DisposeControl(sb->id);
#ifdef DEBUG_MAC_SB
    printf("dest_sb (%x) \n",sb->id);
#endif
}


/*
 * Cursor blink functions.
 *
 * This is a simple state machine:
 * BLINK_NONE	not blinking at all
 * BLINK_OFF	blinking, cursor is not shown
 * BLINK_ON blinking, cursor is shown
 */
    void
gui_mch_set_blinking(long wait, long on, long off)
{
    /* TODO: TODO: TODO: TODO: */
/*    blink_waittime = wait;
    blink_ontime = on;
    blink_offtime = off;*/
}

/*
 * Stop the cursor blinking.  Show the cursor if it wasn't shown.
 */
    void
gui_mch_stop_blink()
{
    gui_update_cursor(TRUE, FALSE);
    /* TODO: TODO: TODO: TODO: */
/*    gui_w32_rm_blink_timer();
    if (blink_state == BLINK_OFF)
    gui_update_cursor(TRUE, FALSE);
    blink_state = BLINK_NONE;*/
}

/*
 * Start the cursor blinking.  If it was already blinking, this restarts the
 * waiting time and shows the cursor.
 */
    void
gui_mch_start_blink()
{
    gui_update_cursor(TRUE, FALSE);
    /* TODO: TODO: TODO: TODO: */
/*    gui_w32_rm_blink_timer(); */

    /* Only switch blinking on if none of the times is zero */
/*    if (blink_waittime && blink_ontime && blink_offtime)
    {
    blink_timer = SetTimer(NULL, 0, (UINT)blink_waittime,
			    (TIMERPROC)_OnBlinkTimer);
    blink_state = BLINK_ON;
    gui_update_cursor(TRUE, FALSE);
    }*/
}

/*
 * Return the RGB value of a pixel as long.
 */
    long_u
gui_mch_get_rgb(guicolor_T pixel)
{
    return (Red(pixel) << 16) + (Green(pixel) << 8) + Blue(pixel);
}



#ifdef FEAT_BROWSE
/*
 * Pop open a file browser and return the file selected, in allocated memory,
 * or NULL if Cancel is hit.
 *  saving  - TRUE if the file will be saved to, FALSE if it will be opened.
 *  title   - Title message for the file browser dialog.
 *  dflt    - Default name of file.
 *  ext     - Default extension to be added to files without extensions.
 *  initdir - directory in which to open the browser (NULL = current dir)
 *  filter  - Filter for matched files to choose from.
 *  Has a format like this:
 *  "C Files (*.c)\0*.c\0"
 *  "All Files\0*.*\0\0"
 *  If these two strings were concatenated, then a choice of two file
 *  filters will be selectable to the user.  Then only matching files will
 *  be shown in the browser.  If NULL, the default allows all files.
 *
 *  *NOTE* - the filter string must be terminated with TWO nulls.
 */
    char_u *
gui_mch_browse(
    int saving,
    char_u *title,
    char_u *dflt,
    char_u *ext,
    char_u *initdir,
    char_u *filter)
{
#if defined (USE_NAVIGATION_SERVICE) || defined (USE_CARBONIZED)
    /* TODO: Add Ammon's safety checl (Dany) */
    NavReplyRecord	reply;
    char_u		*fname = NULL;
    char_u		**fnames = NULL;
    long		numFiles;
    NavDialogOptions	navOptions;
    OSErr		error;

    /* Get Navigation Service Defaults value */
    NavGetDefaultDialogOptions(&navOptions);


    /* TODO: If we get a :browse args, set the Multiple bit. */
    navOptions.dialogOptionFlags =  kNavAllowInvisibleFiles
				 |  kNavDontAutoTranslate
				 |  kNavDontAddTranslateItems
			    /*	 |  kNavAllowMultipleFiles */
				 |  kNavAllowStationery;

    (void) C2PascalString(title,   &navOptions.message);
    (void) C2PascalString(dflt,    &navOptions.savedFileName);
    /* Could set clientName?
     *		 windowTitle? (there's no title bar?)
     */

    if (saving)
    {
	/* Change first parm AEDesc (typeFSS) *defaultLocation to match dflt */
	NavPutFile(NULL, &reply, &navOptions, NULL, 'TEXT', 'VIM!', NULL);
	if (!reply.validRecord)
	    return NULL;
    }
    else
    {
	/* Change first parm AEDesc (typeFSS) *defaultLocation to match dflt */
	NavGetFile(NULL, &reply, &navOptions, NULL, NULL, NULL, NULL, NULL);
	if (!reply.validRecord)
	    return NULL;
    }

    fnames = new_fnames_from_AEDesc(&reply.selection, &numFiles, &error);

    NavDisposeReply(&reply);

    if (fnames)
    {
	fname = fnames[0];
	vim_free(fnames);
    }

    /* TODO: Shorten the file name if possible */
    return fname;
#else
    SFTypeList		fileTypes;
    StandardFileReply	reply;
    Str255		Prompt;
    Str255		DefaultName;
    Str255		Directory;

    /* TODO: split dflt in path and filename */

    (void) C2PascalString(title,   &Prompt);
    (void) C2PascalString(dflt,    &DefaultName);
    (void) C2PascalString(initdir, &Directory);

    if (saving)
    {
	/* Use a custon filter instead of nil FAQ 9-4 */
	StandardPutFile(Prompt, DefaultName,  &reply);
	if (!reply.sfGood)
	    return NULL;
    }
    else
    {
	StandardGetFile(nil, -1, fileTypes, &reply);
	if (!reply.sfGood)
	    return NULL;
    }

    /* Work fine but append a : for new file */
    return (FullPathFromFSSpec_save(reply.sfFile));

    /* Shorten the file name if possible */
/*    mch_dirname(IObuff, IOSIZE);
    p = shorten_fname(fileBuf, IObuff);
    if (p == NULL)
    p = fileBuf;
    return vim_strsave(p);
*/
#endif
}
#endif /* FEAT_BROWSE */

#ifdef FEAT_GUI_DIALOG
/*
 * Stuff for dialogues
 */

/*
 * Create a dialogue dynamically from the parameter strings.
 * type       = type of dialogue (question, alert, etc.)
 * title      = dialogue title. may be NULL for default title.
 * message    = text to display. Dialogue sizes to accommodate it.
 * buttons    = '\n' separated list of button captions, default first.
 * dfltbutton = number of default button.
 *
 * This routine returns 1 if the first button is pressed,
 *	    2 for the second, etc.
 *
 *	    0 indicates Esc was pressed.
 *	    -1 for unexpected error
 *
 * If stubbing out this fn, return 1.
 */

typedef struct
{
    short   idx;
    short   width;	/* Size of the text in pixel */
    Rect    box;
} vgmDlgItm; /* Vim Gui_Mac.c Dialog Item */

#define MoveRectTo(r,x,y) OffsetRect(r,x-r->left,y-r->top)

    static void
macMoveDialogItem(
    DialogRef	theDialog,
    short	itemNumber,
    short	X,
    short	Y,
    Rect	*inBox)
{
#if 0 /* USE_CARBONIZED */
    /* Untested */
    MoveDialogItem(theDialog, itemNumber, X, Y);
    if (inBox != nil)
	GetDialogItem(theDialog, itemNumber, &itemType, &itemHandle, inBox);
#else
    short	itemType;
    Handle	itemHandle;
    Rect	localBox;
    Rect	*itemBox = &localBox;

    if (inBox != nil)
	itemBox = inBox;

    GetDialogItem(theDialog, itemNumber, &itemType, &itemHandle, itemBox);
    OffsetRect(itemBox, -itemBox->left, -itemBox->top);
    OffsetRect(itemBox, X, Y);
    /* To move a control (like a button) we need to call both
     * MoveControl and SetDialogItem. FAQ 6-18 */
    if (1) /*(itemType & kControlDialogItem) */
	MoveControl((ControlRef) itemHandle, X, Y);
    SetDialogItem(theDialog, itemNumber, itemType, itemHandle, itemBox);
#endif
}

    static void
macSizeDialogItem(
    DialogRef	theDialog,
    short	itemNumber,
    short	width,
    short	height)
{
    short	itemType;
    Handle	itemHandle;
    Rect	itemBox;

    GetDialogItem(theDialog, itemNumber, &itemType, &itemHandle, &itemBox);

    /* When width or height is zero do not change it */
    if (width  == 0)
	width  = itemBox.right  - itemBox.left;
    if (height == 0)
	height = itemBox.bottom - itemBox.top;

#if 0 /* USE_CARBONIZED */
    SizeDialogItem(theDialog, itemNumber, width, height); /* Untested */
#else
    /* Resize the bounding box */
    itemBox.right  = itemBox.left + width;
    itemBox.bottom = itemBox.top  + height;

    /* To resize a control (like a button) we need to call both
     * SizeControl and SetDialogItem. (deducted from FAQ 6-18) */
    if (itemType & kControlDialogItem)
	SizeControl((ControlRef) itemHandle, width, height);

    /* Configure back the item */
    SetDialogItem(theDialog, itemNumber, itemType, itemHandle, &itemBox);
#endif
}

    static void
macSetDialogItemText(
    DialogRef	theDialog,
    short	itemNumber,
    Str255	itemName)
{
    short	itemType;
    Handle	itemHandle;
    Rect	itemBox;

    GetDialogItem(theDialog, itemNumber, &itemType, &itemHandle, &itemBox);

    if (itemType & kControlDialogItem)
	SetControlTitle((ControlRef) itemHandle, itemName);
    else
	SetDialogItemText(itemHandle, itemName);
}

    int
gui_mch_dialog(
    int		type,
    char_u	*title,
    char_u	*message,
    char_u	*buttons,
    int		dfltbutton,
    char_u	*textfield)
{
    Handle	buttonDITL;
    Handle	iconDITL;
    Handle	inputDITL;
    Handle	messageDITL;
    Handle	itemHandle;
    Handle	iconHandle;
    DialogPtr	theDialog;
    char_u	len;
    char_u	PascalTitle[256];	/* place holder for the title */
    char_u	name[256];
    GrafPtr	oldPort;
    short	itemHit;
    char_u	*buttonChar;
    Rect	box;
    short	button;
    short	lastButton;
    short	itemType;
    short	useIcon;
    short	width;
    short	totalButtonWidth = 0;   /* the width of all button together incuding spacing */
    short	widestButton = 0;
    short	dfltButtonEdge     = 20;  /* gut feeling */
    short	dfltElementSpacing = 13;  /* from IM:V.2-29 */
    short       dfltIconSideSpace  = 23;  /* from IM:V.2-29 */
    short	maximumWidth       = 400; /* gut feeling */
    short	maxButtonWidth	   = 175; /* gut feeling */

    short	vertical;
    short	dialogHeight;
    short	messageLines = 3;
    FontInfo	textFontInfo;

    vgmDlgItm   iconItm;
    vgmDlgItm   messageItm;
    vgmDlgItm   inputItm;
    vgmDlgItm   buttonItm;

    WindowRef	theWindow;

    /* Check 'v' flag in 'guioptions': vertical button placement. */
    vertical = (vim_strchr(p_go, GO_VERTICAL) != NULL);

    /* Create a new Dialog Box from template. */
    theDialog = GetNewDialog(129, nil, (WindowRef) -1);

    /* Get the WindowRef */
    theWindow = GetDialogWindow(theDialog);

    /* Hide the window.
     * 1. to avoid seeing slow drawing
     * 2. to prevent a problem seen while moving dialog item
     *    within a visible window. (non-Carbon MacOS 9)
     * Could be avoided by changing the resource.
     */
    HideWindow(theWindow);

    /* Change the graphical port to the dialog,
     * so we can measure the text with the proper font */
    GetPort(&oldPort);
#ifdef USE_CARBONIZED
    SetPortDialogPort(theDialog);
#else
    SetPort(theDialog);
#endif

    /* Get the info about the default text,
     * used to calculate the height of the message
     * and of the  text field */
    GetFontInfo(&textFontInfo);

    /*	Set the dialog title */
    if (title != NULL)
    {
	(void) C2PascalString(title, &PascalTitle);
	SetWTitle(theWindow, PascalTitle);
    }

    /* Creates the buttons and add them to the Dialog Box. */
    buttonDITL = GetResource('DITL', 130);
    buttonChar = buttons;
    button = 0;

    for (;*buttonChar != 0;)
    {
	/* Get the name of the button */
	button++;
	len = 0;
	for (;((*buttonChar != DLG_BUTTON_SEP) && (*buttonChar != 0) && (len < 255)); buttonChar++)
	{
	    if (*buttonChar != DLG_HOTKEY_CHAR)
		name[++len] = *buttonChar;
	}
	if (*buttonChar != 0)
	  buttonChar++;
	name[0] = len;

	/* Add the button */
	AppendDITL(theDialog, buttonDITL, overlayDITL); /* appendDITLRight); */

	/* Change the button's name */
	macSetDialogItemText(theDialog, button, name);

	/* Resize the button to fit its name */
	width = StringWidth(name) + 2 * dfltButtonEdge;
	/* Limite the size of any button to an acceptable value. */
	/* TODO: Should be based on the message width */
	if (width > maxButtonWidth)
	    width = maxButtonWidth;
	macSizeDialogItem(theDialog, button, width, 0);

	totalButtonWidth += width;

	if (width > widestButton)
	    widestButton = width;
    }
    ReleaseResource(buttonDITL);
    lastButton = button;

    /* Add the icon to the Dialog Box. */
    iconItm.idx = lastButton + 1;
    iconDITL = GetResource('DITL', 131);
    switch (type)
    {
	case VIM_GENERIC:  useIcon = kNoteIcon;
	case VIM_ERROR:    useIcon = kStopIcon;
	case VIM_WARNING:  useIcon = kCautionIcon;
	case VIM_INFO:     useIcon = kNoteIcon;
	case VIM_QUESTION: useIcon = kNoteIcon;
	default:      useIcon = kStopIcon;
    };
    AppendDITL(theDialog, iconDITL, overlayDITL);
    ReleaseResource(iconDITL);
    GetDialogItem(theDialog, iconItm.idx, &itemType, &itemHandle, &box);
    /* TODO: Should the item be freed? */
    iconHandle = GetIcon(useIcon);
    SetDialogItem(theDialog, iconItm.idx, itemType, iconHandle, &box);

    /* Add the message to the Dialog box. */
    messageItm.idx = lastButton + 2;
    messageDITL = GetResource('DITL', 132);
    AppendDITL(theDialog, messageDITL, overlayDITL);
    ReleaseResource(messageDITL);
    GetDialogItem(theDialog, messageItm.idx, &itemType, &itemHandle, &box);
    (void) C2PascalString(message, &name);
    SetDialogItemText(itemHandle, name);
    messageItm.width = StringWidth(name);

    /* Add the input box if needed */
    if (textfield != NULL)
    {
	/* Cheat for now reuse the message and convet to text edit */
	inputItm.idx = lastButton + 3;
	inputDITL = GetResource('DITL', 132);
	AppendDITL(theDialog, inputDITL, overlayDITL);
	ReleaseResource(inputDITL);
	GetDialogItem(theDialog, inputItm.idx, &itemType, &itemHandle, &box);
/*	  SetDialogItem(theDialog, inputItm.idx, kEditTextDialogItem, itemHandle, &box);*/
	(void) C2PascalString(textfield, &name);
	SetDialogItemText(itemHandle, name);
	inputItm.width = StringWidth(name);
    }

    /* Set the <ENTER> and <ESC> button. */
    SetDialogDefaultItem(theDialog, dfltbutton);
    SetDialogCancelItem(theDialog, 0);

    /* Reposition element */

    /* Check if we need to force vertical */
    if (totalButtonWidth > maximumWidth)
	vertical = TRUE;

    /* Place icon */
    macMoveDialogItem(theDialog, iconItm.idx, dfltIconSideSpace, dfltElementSpacing, &box);
    iconItm.box.right = box.right;
    iconItm.box.bottom = box.bottom;

    /* Place Message */
    messageItm.box.left = iconItm.box.right + dfltIconSideSpace;
    macSizeDialogItem(theDialog, messageItm.idx, 0,  messageLines * (textFontInfo.ascent + textFontInfo.descent));
    macMoveDialogItem(theDialog, messageItm.idx, messageItm.box.left, dfltElementSpacing, &messageItm.box);

    /* Place Input */
    if (textfield != NULL)
    {
	inputItm.box.left = messageItm.box.left;
	inputItm.box.top  = messageItm.box.bottom + dfltElementSpacing;
	macSizeDialogItem(theDialog, inputItm.idx, 0, textFontInfo.ascent + textFontInfo.descent);
	macMoveDialogItem(theDialog, inputItm.idx, inputItm.box.left, inputItm.box.top, &inputItm.box);
	/* Convert the static text into a text edit.
	 * For some reason this change need to be done last (Dany) */
	GetDialogItem(theDialog, inputItm.idx, &itemType, &itemHandle, &inputItm.box);
	SetDialogItem(theDialog, inputItm.idx, kEditTextDialogItem, itemHandle, &inputItm.box);
	SelectDialogItemText(theDialog, inputItm.idx, 0, 32767);
    }

    /* Place Button */
    if (textfield != NULL)
    {
	buttonItm.box.left = inputItm.box.left;
	buttonItm.box.top  = inputItm.box.bottom + dfltElementSpacing;
    }
    else
    {
	buttonItm.box.left = messageItm.box.left;
	buttonItm.box.top  = messageItm.box.bottom + dfltElementSpacing;
    }

    for (button=1; button <= lastButton; button++)
    {

	macMoveDialogItem(theDialog, button, buttonItm.box.left, buttonItm.box.top, &box);
	/* With vertical, it's better to have all button the same lenght */
	if (vertical)
	{
	    macSizeDialogItem(theDialog, button, widestButton, 0);
	    GetDialogItem(theDialog, button, &itemType, &itemHandle, &box);
	}
	/* Calculate position of next button */
	if (vertical)
	    buttonItm.box.top  = box.bottom + dfltElementSpacing;
	else
	    buttonItm.box.left  = box.right + dfltElementSpacing;
    }

    /* Resize the dialog box */
    dialogHeight = box.bottom + dfltElementSpacing;
    SizeWindow(theWindow, maximumWidth, dialogHeight, TRUE);

#ifdef USE_CARBONIZED
    /* Magic resize */
    AutoSizeDialog(theDialog);
    /* Need a horizontal resize anyway so not that useful */
#endif

    /* Display it */
    ShowWindow(theWindow);
/*  BringToFront(theWindow); */
    SelectWindow(theWindow);

/*  DrawDialog(theDialog); */
#if 0
    GetPort(&oldPort);
#ifdef USE_CARBONIZED
    SetPortDialogPort(theDialog);
#else
    SetPort(theDialog);
#endif
#endif

    /* Hang until one of the button is hit */
    do
    {
	ModalDialog(nil, &itemHit);
    } while ((itemHit < 1) || (itemHit > lastButton));

    /* Copy back the text entered by the user into the param */
    if (textfield != NULL)
    {
	GetDialogItem(theDialog, inputItm.idx, &itemType, &itemHandle, &box);
	GetDialogItemText(itemHandle, (char_u *) &name);
#if IOSIZE < 256
	/* Truncate the name to IOSIZE if needed */
	if (name[0] > IOSIZE)
	    name[0] = IOSIZE - 1;
#endif
	STRNCPY(textfield, &name[1], name[0]);
	textfield[name[0]] = NUL;
    }

    /* Restore the original graphical port */
    SetPort(oldPort);

    /* Get ride of th edialog (free memory) */
    DisposeDialog(theDialog);

    return itemHit;
/*
 * Usefull thing which could be used
 * SetDialogTimeout(): Auto click a button after timeout
 * SetDialogTracksCursor() : Get the I-beam cursor over input box
 * MoveDialogItem():	    Probably better than SetDialogItem
 * SizeDialogItem():		(but is it Carbon Only?)
 * AutoSizeDialog():	    Magic resize of dialog based on text lenght
 */
}
#endif /* FEAT_DIALOG_GUI */

/*
 * Display the saved error message(s).
 */
#ifdef USE_MCH_ERRMSG
    void
display_errors()
{
    char	*p;
    char_u	pError[256];

    if (error_ga.ga_data != NULL)
    {
	/* avoid putting up a message box with blanks only */
	for (p = (char *)error_ga.ga_data; *p; ++p)
	    if (!isspace(*p))
	    {
		if (STRLEN(p) > 255)
		    pError[0] = 255;
		else
		    pError[0] = STRLEN(p);

		STRNCPY(&pError[1], p, pError[0]);
		ParamText(pError, nil, nil, nil);
		Alert(128, nil);
		break;
		/* TODO: handled message longer than 256 chars
		 *	 use auto-sizeable alert
		 *	 or dialog with scrollbars (TextEdit zone)
		 */
	    }
	ga_clear(&error_ga);
    }
}
#endif

/*
 * Get current y mouse coordinate in text window.
 * Return -1 when unknown.
 */
    int
gui_mch_get_mouse_x()
{
    Point where;

    GetMouse(&where);

    return (where.h);
}

    int
gui_mch_get_mouse_y()
{
    Point where;

    GetMouse(&where);

    return (where.v);
}

    void
gui_mch_setmouse(x, y)
    int		x;
    int		y;
{
    /* TODO */
#if 0
    /* From FAQ 3-11 */

    CursorDevicePtr myMouse;
    Point	    where;

    if (   NGetTrapAddress(_CursorDeviceDispatch, ToolTrap)
	!= NGetTrapAddress(_Unimplemented,   ToolTrap))
    {
	/* New way */

	/*
	 * Get first devoice with one button.
	 * This will probably be the standad mouse
	 * startat head of cursor dev list
	 *
	 */

	myMouse = nil;

	do
	{
	    /* Get the next cursor device */
	    CursorDeviceNextDevice(&myMouse);
	}
	while ((myMouse != nil) && (myMouse->cntButtons != 1));

	CursorDeviceMoveTo(myMouse, x, y);
    }
    else
    {
	/* Old way */
	where.h = x;
	where.v = y;

	*(Point *)RawMouse = where;
	*(Point *)MTemp    = where;
	*(Ptr)    CrsrNew  = 0xFFFF;
    }
#endif
}

    void
gui_mch_show_popupmenu(menu)
    vimmenu_T *menu;
{
#ifdef USE_CTRLCLICKMENU
/*
 *  Clone PopUp to use menu
 *  Create a object descriptor for the current selection
 *  Call the procedure
 */

    MenuHandle	CntxMenu;
    Point	where;
    OSStatus	status;
    UInt32	CntxType;
    SInt16	CntxMenuID;
    UInt16	CntxMenuItem;
    Str255	HelpName = "";
    GrafPtr	savePort;

    /* Save Current Port: On MacOS X we seem to lose the port */
    GetPort(&savePort); /*OSX*/

    GetMouse(&where);
    LocalToGlobal(&where); /*OSX*/
    CntxMenu = menu->submenu_handle;

    /* TODO: Get the text selection from Vim */

    /* Call to Handle Popup */
    status = ContextualMenuSelect(CntxMenu, where, false, kCMHelpItemNoHelp, HelpName, NULL, &CntxType, &CntxMenuID, &CntxMenuItem);

    if (status == noErr)
    {
	if (CntxType == kCMMenuItemSelected)
	{
	    /* Handle the menu CntxMenuID, CntxMenuItem */
	    /* The submenu can be handle directly by gui_mac_handle_menu */
	    /* But what about the current menu, is the menu changed by ContextualMenuSelect */
	    gui_mac_handle_menu((CntxMenuID << 16) + CntxMenuItem);
	}
	else if (CntxMenuID == kCMShowHelpSelected)
	{
	    /* Should come up with the help */
	}
    }

    /* Restore original Port */
    SetPort(savePort); /*OSX*/
#endif
}

#if defined(FEAT_CW_EDITOR) || defined(PROTO)
/* TODO: Is it need for MACOS_X? (Dany) */
    void
mch_post_buffer_write(buf_T *buf)
{
# ifdef USE_SIOUX
    printf("Writing Buf...\n");
# endif
    GetFSSpecFromPath(buf->b_ffname, &buf->b_FSSpec);
    Send_KAHL_MOD_AE(buf);
}
#endif

#ifdef FEAT_TITLE
/*
 * Set the window title and icon.
 * (The icon is not taken care of).
 */
    void
gui_mch_settitle(title, icon)
    char_u *title;
    char_u *icon;
{
    /* TODO: Get vim to make sure maxlen (from p_titlelen) is smaller
     *       that 256. Even better get it to fit nicely in the titlebar.
     */
    char_u   *pascalTitle;

    if (title == NULL)		/* nothing to do */
	return;

    pascalTitle = C2Pascal_save(title);
    if (pascalTitle != NULL)
    {
	SetWTitle(gui.VimWindow, pascalTitle);
	vim_free(pascalTitle);
    }
}
#endif

/*
 * Transfered from os_mac.c for MacOS X using os_unix.c prep work
 */

    int
C2PascalString(CString, PascalString)
    char_u  *CString;
    Str255  *PascalString;
{
    char_u *PascalPtr = (char_u *) PascalString;
    int    len;
    int    i;

    PascalPtr[0] = 0;
    if (CString == NULL)
	return 0;

    len = STRLEN(CString);
    if (len > 255)
	len = 255;

    for (i = 0; i < len; i++)
	PascalPtr[i+1] = CString[i];

    PascalPtr[0] = len;

    return 0;
}

    int
GetFSSpecFromPath(file, fileFSSpec)
    char_u *file;
    FSSpec *fileFSSpec;
{
    /* From FAQ 8-12 */
    Str255      filePascal;
    CInfoPBRec	myCPB;
    OSErr	err;

    (void) C2PascalString(file, &filePascal);

    myCPB.dirInfo.ioNamePtr   = filePascal;
    myCPB.dirInfo.ioVRefNum   = 0;
    myCPB.dirInfo.ioFDirIndex = 0;
    myCPB.dirInfo.ioDrDirID   = 0;

    err= PBGetCatInfo(&myCPB, false);

    /*    vRefNum, dirID, name */
    FSMakeFSSpec(0, 0, filePascal, fileFSSpec);

    /* TODO: Use an error code mechanism */
    return 0;
}

/*
 * Convert a FSSpec to a fuill path
 */

char_u *FullPathFromFSSpec_save(FSSpec file)
{
    /*
     * TODO: Add protection for 256 char max.
     */

    CInfoPBRec	theCPB;
    char_u	fname[256];
    char_u	*filenamePtr = fname;
    OSErr	error;
    int		folder = 1;
#ifdef USE_UNIXFILENAME
    SInt16	dfltVol_vRefNum;
    SInt32	dfltVol_dirID;
    FSRef	refFile;
    OSStatus	status;
    UInt32	pathSize = 256;
    char_u	pathname[256];
    char_u	*path = pathname;
#else
    Str255	directoryName;
    char_u	temporary[255];
    char_u	*temporaryPtr = temporary;
#endif

#ifdef USE_UNIXFILENAME
    /* Get the default volume */
    /* TODO: Remove as this only work if Vim is on the Boot Volume*/
    error=HGetVol(NULL, &dfltVol_vRefNum, &dfltVol_dirID);

    if (error)
      return NULL;
#endif

    /* Start filling fname with file.name  */
    STRNCPY(filenamePtr, &file.name[1], file.name[0]);
    filenamePtr[file.name[0]] = 0; /* NULL terminate the string */

    /* Get the info about the file specified in FSSpec */
    theCPB.dirInfo.ioFDirIndex = 0;
    theCPB.dirInfo.ioNamePtr   = file.name;
    theCPB.dirInfo.ioVRefNum   = file.vRefNum;
  /*theCPB.hFileInfo.ioDirID   = 0;*/
    theCPB.dirInfo.ioDrDirID   = file.parID;

    /* As ioFDirIndex = 0, get the info of ioNamePtr,
       which is relative to ioVrefNum, ioDirID */
    error = PBGetCatInfo(&theCPB, false);

    /* If we are called for a new file we expect fnfErr */
    if ((error) && (error != fnfErr))
      return NULL;

    /* Check if it's a file or folder       */
    /* default to file if file don't exist  */
    if (((theCPB.hFileInfo.ioFlAttrib & ioDirMask) == 0) || (error))
      folder = 0; /* It's not a folder */
    else
      folder = 1;

#ifdef USE_UNIXFILENAME
    /*
     * The function used here are available in Carbon, but
     * do nothing une MacOS 8 and 9
     */
    if (error == fnfErr)
    {
	/* If the file to be saved does not already exist, it isn't possible
	   to convert its FSSpec into an FSRef.  But we can construct an
	   FSSpec for the file's parent folder (since we have its volume and
	   directory IDs), and since that folder does exist, we can convert
	   that FSSpec into an FSRef, convert the FSRef in turn into a path,
	   and, finally, append the filename. */
	FSSpec dirSpec;
	FSRef dirRef;
	Str255 emptyFilename = "\p";
	error = FSMakeFSSpec(theCPB.dirInfo.ioVRefNum,
	    theCPB.dirInfo.ioDrDirID, emptyFilename, &dirSpec);
	if (error)
	    return NULL;

	error = FSpMakeFSRef(&dirSpec, &dirRef);
	if (error)
	    return NULL;

	status = FSRefMakePath(&dirRef, (UInt8*)path, pathSize);
	if (status)
	    return NULL;

	STRCAT(path, "/");
	STRCAT(path, filenamePtr);
    }
    else
    {
	/* If the file to be saved already exists, we can get its full path
	   by converting its FSSpec into an FSRef. */
	error=FSpMakeFSRef(&file, &refFile);
	if (error)
	    return NULL;

	status=FSRefMakePath(&refFile, (UInt8 *) path, pathSize);
	if (status)
	    return NULL;
    }

    /* Add a slash at the end if needed */
    if (folder)
	STRCAT(path, "/");

    return (vim_strsave(path));
#else
    /* TODO: Get rid of all USE_UNIXFILENAME below */
    /* Set ioNamePtr, it's the same area which is always reused. */
    theCPB.dirInfo.ioNamePtr = directoryName;

    /* Trick for first entry, set ioDrParID to the first value
     * we want for ioDrDirID*/
    theCPB.dirInfo.ioDrParID = file.parID;
    theCPB.dirInfo.ioDrDirID = file.parID;

    if ((TRUE) && (file.parID != fsRtDirID /*fsRtParID*/))
    do
    {
	theCPB.dirInfo.ioFDirIndex = -1;
     /* theCPB.dirInfo.ioNamePtr   = directoryName; Already done above. */
	theCPB.dirInfo.ioVRefNum   = file.vRefNum;
     /* theCPB.dirInfo.ioDirID     = irrevelant when ioFDirIndex = -1 */
	theCPB.dirInfo.ioDrDirID   = theCPB.dirInfo.ioDrParID;

	/* As ioFDirIndex = -1, get the info of ioDrDirID, */
	/*  *ioNamePtr[0 TO 31] will be updated		   */
	error = PBGetCatInfo(&theCPB,false);

	if (error)
	  return NULL;

	/* Put the new directoryName in front of the current fname */
	STRCPY(temporaryPtr, filenamePtr);
	STRNCPY(filenamePtr, &directoryName[1], directoryName[0]);
	filenamePtr[directoryName[0]] = 0; /* NULL terminate the string */
	STRCAT(filenamePtr, ":");
	STRCAT(filenamePtr, temporaryPtr);
    }
#if 1 /* def USE_UNIXFILENAME */
    while ((theCPB.dirInfo.ioDrParID != fsRtDirID) /* && */
	 /*  (theCPB.dirInfo.ioDrDirID != fsRtDirID)*/);
#else
    while (theCPB.dirInfo.ioDrDirID != fsRtDirID);
#endif

    /* Get the information about the volume on which the file reside */
    theCPB.dirInfo.ioFDirIndex = -1;
 /* theCPB.dirInfo.ioNamePtr   = directoryName; Already done above. */
    theCPB.dirInfo.ioVRefNum   = file.vRefNum;
 /* theCPB.dirInfo.ioDirID     = irrevelant when ioFDirIndex = -1 */
    theCPB.dirInfo.ioDrDirID   = theCPB.dirInfo.ioDrParID;

    /* As ioFDirIndex = -1, get the info of ioDrDirID, */
    /*	*ioNamePtr[0 TO 31] will be updated	       */
    error = PBGetCatInfo(&theCPB,false);

    if (error)
      return NULL;

    /* For MacOS Classic always add the volume name	     */
    /* For MacOS X add the volume name preceded by "Volumes" */
    /*	when we are not refering to the boot volume	     */
#ifdef USE_UNIXFILENAME
    if (file.vRefNum != dfltVol_vRefNum)
#endif
    {
	/* Add the volume name */
	STRCPY(temporaryPtr, filenamePtr);
	STRNCPY(filenamePtr, &directoryName[1], directoryName[0]);
	filenamePtr[directoryName[0]] = 0; /* NULL terminate the string */
	STRCAT(filenamePtr, ":");
	STRCAT(filenamePtr, temporaryPtr);

#ifdef USE_UNIXFILENAME
	STRCPY(temporaryPtr, filenamePtr);
	filenamePtr[0] = 0; /* NULL terminate the string */
	STRCAT(filenamePtr, "Volumes:");
	STRCAT(filenamePtr, temporaryPtr);
#endif
    }

    /* Append final path separator if it's a folder */
    if (folder)
	STRCAT(fname, ":");

    /* As we use Unix File Name for MacOS X convert it */
#ifdef USE_UNIXFILENAME
    /* Need to insert leading / */
    /* TODO: get the above code to use directly the / */
    STRCPY(&temporaryPtr[1], filenamePtr);
    temporaryPtr[0] = '/';
    STRCPY(filenamePtr, temporaryPtr);
    {
    char	*p;
    for (p = fname; *p; p++)
	if (*p == ':')
	    *p = '/';
    }
#endif

    return (vim_strsave(fname));
#endif
}

#if defined(USE_IM_CONTROL) || defined(PROTO)
/*
 * Input Method Control functions.
 */

/*
 * Notify cursor position to IM.
 */
    void
im_set_position(int row, int col)
{
    /* TODO: Implement me! */
}

/*
 * Set IM status on ("active" is TRUE) or off ("active" is FALSE).
 */
    void
im_set_active(int active)
{
    KeyScript(active ? smKeySysScript : smKeyRoman);
}

/*
 * Get IM status.  When IM is on, return not 0.  Else return 0.
 */
    int
im_get_status()
{
    SInt32 script = GetScriptManagerVariable(smKeyScript);
    return (script != smRoman
	    && script == GetScriptManagerVariable(smSysScript)) ? 1 : 0;
}
#endif /* defined(USE_IM_CONTROL) || defined(PROTO) */
