/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *				GUI/Motif support by Robert Webb
 *				Macintosh port by Dany St-Amant
 *					      and Axel Kielhorn
 *				Port to MPW by Bernhard Pruemmer
 *				Initial Carbon port by Ammon Skidmore
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * NOTES: - Vim 7+ does not support classic MacOS. Please use Vim 6.x
 *	  - Comments mentioning FAQ refer to the book:
 *	    "Macworld Mac Programming FAQs" from "IDG Books"
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

/* TODO (Jussi)
 *   * Clipboard does not work (at least some cases)
 *   * ATSU font rendering has some problems
 *   * Investigate and remove dead code (there is still lots of that)
 */

#include <Devices.h> /* included first to avoid CR problems */
#include "vim.h"

#define USE_CARBONIZED
#define USE_AEVENT		/* Enable AEVENT */
#undef USE_OFFSETED_WINDOW	/* Debugging feature: start Vim window OFFSETed */

/* Compile as CodeWarrior External Editor */
#if defined(FEAT_CW_EDITOR) && !defined(USE_AEVENT)
# define USE_AEVENT /* Need Apple Event Support */
#endif

/* Vim's Scrap flavor. */
#define VIMSCRAPFLAVOR 'VIM!'
#define SCRAPTEXTFLAVOR kScrapFlavorTypeUnicode

static EventHandlerUPP mouseWheelHandlerUPP = NULL;
SInt32 gMacSystemVersion;

#ifdef MACOS_CONVERT
# define USE_CARBONKEYHANDLER

static int im_is_active = FALSE;
# if 0
    /* TODO: Implement me! */
static int im_start_row = 0;
static int im_start_col = 0;
# endif

# define NR_ELEMS(x)	(sizeof(x) / sizeof(x[0]))

static TSMDocumentID gTSMDocument;

static void im_on_window_switch(int active);
static EventHandlerUPP keyEventHandlerUPP = NULL;
static EventHandlerUPP winEventHandlerUPP = NULL;

static pascal OSStatus gui_mac_handle_window_activate(
	EventHandlerCallRef nextHandler, EventRef theEvent, void *data);

static pascal OSStatus gui_mac_handle_text_input(
	EventHandlerCallRef nextHandler, EventRef theEvent, void *data);

static pascal OSStatus gui_mac_update_input_area(
	EventHandlerCallRef nextHandler, EventRef theEvent);

static pascal OSStatus gui_mac_unicode_key_event(
	EventHandlerCallRef nextHandler, EventRef theEvent);

#endif


/* Include some file. TODO: move into os_mac.h */
#include <Menus.h>
#include <Resources.h>
#include <Processes.h>
#ifdef USE_AEVENT
# include <AppleEvents.h>
# include <AERegistry.h>
#endif
# include <Gestalt.h>
#if UNIVERSAL_INTERFACES_VERSION >= 0x0330
# include <ControlDefinitions.h>
# include <Navigation.h>  /* Navigation only part of ?? */
#endif

/* Help Manager (balloon.h, HM prefixed functions) are not supported
 * under Carbon (Jussi) */
#  if 0
/* New Help Interface for Mac, not implemented yet.*/
#    include <MacHelp.h>
#  endif

/*
 * These seem to be rectangle options. Why are they not found in
 * headers? (Jussi)
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
static short clickIsPopup;

/* Feedback Action for Scrollbar */
ControlActionUPP gScrollAction;
ControlActionUPP gScrollDrag;

/* Keeping track of which scrollbar is being dragged */
static ControlHandle dragged_sb = NULL;

/* Vector of char_u --> control index for hotkeys in dialogs */
static short *gDialogHotKeys;

static struct
{
    FMFontFamily family;
    FMFontSize size;
    FMFontStyle style;
    Boolean isPanelVisible;
} gFontPanelInfo = { 0, 0, 0, false };

#ifdef MACOS_CONVERT
# define USE_ATSUI_DRAWING
int	    p_macatsui_last;
ATSUStyle   gFontStyle;
ATSUStyle   gWideFontStyle;
Boolean	    gIsFontFallbackSet;
UInt32      useAntialias_cached = 0x0;
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
/*  {vk_Delete,		'k', 'b'}, */
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

#ifdef FEAT_GUI_TABLINE
static void initialise_tabline(void);
static WindowRef drawer = NULL; // TODO: put into gui.h
#endif

#ifdef USE_ATSUI_DRAWING
static void gui_mac_set_font_attributes(GuiFont font);
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

    char_u *
C2Pascal_save(char_u *Cstring)
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

    char_u *
C2Pascal_save_and_remove_backslash(char_u *Cstring)
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
		c++;
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

#ifdef MACOS_CONVERT
/*
 * Deletes all traces of any Windows-style mnemonic text (including any
 * parentheses) from a menu item and returns the cleaned menu item title.
 * The caller is responsible for releasing the returned string.
 */
    static CFStringRef
menu_title_removing_mnemonic(vimmenu_T *menu)
{
    CFStringRef		name;
    size_t		menuTitleLen;
    CFIndex		displayLen;
    CFRange		mnemonicStart;
    CFRange		mnemonicEnd;
    CFMutableStringRef	cleanedName;

    menuTitleLen = STRLEN(menu->dname);
    name = (CFStringRef) mac_enc_to_cfstring(menu->dname, menuTitleLen);

    if (name)
    {
	/* Simple mnemonic-removal algorithm, assumes single parenthesized
	 * mnemonic character towards the end of the menu text */
	mnemonicStart = CFStringFind(name, CFSTR("("), kCFCompareBackwards);
	displayLen = CFStringGetLength(name);

	if (mnemonicStart.location != kCFNotFound
		&& (mnemonicStart.location + 2) < displayLen
		&& CFStringGetCharacterAtIndex(name,
		       mnemonicStart.location + 1) == (UniChar)menu->mnemonic)
	{
	    if (CFStringFindWithOptions(name, CFSTR(")"),
			CFRangeMake(mnemonicStart.location + 1,
			    displayLen - mnemonicStart.location - 1),
			kCFCompareBackwards, &mnemonicEnd) &&
		    (mnemonicStart.location + 2) == mnemonicEnd.location)
	    {
		cleanedName = CFStringCreateMutableCopy(NULL, 0, name);
		if (cleanedName)
		{
		    CFStringDelete(cleanedName,
			    CFRangeMake(mnemonicStart.location,
				mnemonicEnd.location + 1 -
				mnemonicStart.location));

		    CFRelease(name);
		    name = cleanedName;
		}
	    }
	}
    }

    return name;
}
#endif

/*
 * Convert a list of FSSpec aliases into a list of fullpathname
 * character strings.
 */

    char_u **
new_fnames_from_AEDesc(AEDesc *theList, long *numFiles, OSErr *error)
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
	return fnames;

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
	    return fnames;
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

typedef struct WindowSearch WindowSearch;
struct WindowSearch /* for handling class 'KAHL', event 'SRCH', keyDirectObject typeChar*/
{
    FSSpec theFile; // identifies the file
    long *theDate; // where to put the modification date/time
};

    pascal OSErr
Handle_KAHL_SRCH_AE(
	const AppleEvent    *theAEvent,
	AppleEvent	    *theReply,
	long		    refCon)
{
    OSErr	error = noErr;
    buf_T	*buf;
    int		foundFile = false;
    DescType	typeCode;
    WindowSearch SearchData;
    Size	actualSize;

    error = AEGetParamPtr(theAEvent, keyDirectObject, typeChar, &typeCode, (Ptr) &SearchData, sizeof(WindowSearch), &actualSize);
    if (error)
	return error;

    error = HandleUnusedParms(theAEvent);
    if (error)
	return error;

    FOR_ALL_BUFFERS(buf)
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

typedef struct ModificationInfo ModificationInfo;
struct ModificationInfo /* for replying to class 'KAHL', event 'MOD ', keyDirectObject typeAEList*/
{
    FSSpec theFile; // identifies the file
    long theDate; // the date/time the file was last modified
    short saved; // set this to zero when replying, unused
};

    pascal OSErr
Handle_KAHL_MOD_AE(
	const AppleEvent    *theAEvent,
	AppleEvent	    *theReply,
	long		    refCon)
{
    OSErr	error = noErr;
    AEDescList	replyList;
    long	numFiles;
    ModificationInfo theFile;
    buf_T	*buf;

    theFile.saved = 0;

    error = HandleUnusedParms(theAEvent);
    if (error)
	return error;

    /* Send the reply */
/*  replyObject.descriptorType = typeNull;
    replyObject.dataHandle     = nil;*/

/* AECreateDesc(typeChar, (Ptr)&title[1], title[0], &data) */
    error = AECreateList(nil, 0, false, &replyList);
    if (error)
	return error;

#if 0
    error = AECountItems(&replyList, &numFiles);

    /* AEPutKeyDesc(&replyList, keyAEPnject, &aDesc)
     * AEPutKeyPtr(&replyList, keyAEPosition, typeChar, (Ptr)&theType,
     * sizeof(DescType))
     */

    /* AEPutDesc */
#endif

    numFiles = 0;
    FOR_ALL_BUFFERS(buf)
	if (buf->b_ml.ml_mfp != NULL)
	{
	    /* Add this file to the list */
	    theFile.theFile = buf->b_FSSpec;
	    theFile.theDate = buf->b_mtime;
/*	    theFile.theDate = time(NULL) & (time_t) 0xFFFFFFF0; */
	    error = AEPutPtr(&replyList, numFiles, typeChar, (Ptr) &theFile, sizeof(theFile));
	};

#if 0
    error = AECountItems(&replyList, &numFiles);
#endif

    /* We can add data only if something to reply */
    error = AEPutParamDesc(theReply, keyDirectObject, &replyList);

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

typedef struct CW_GetText CW_GetText;
struct CW_GetText /* for handling class 'KAHL', event 'GTTX', keyDirectObject typeChar*/
{
    FSSpec theFile; /* identifies the file */
    Handle theText; /* the location where you return the text (must be resized properly) */
    long *unused;   /* 0 (not used) */
    long *theDate;  /* where to put the modification date/time */
};

    pascal OSErr
Handle_KAHL_GTTX_AE(
	const AppleEvent    *theAEvent,
	AppleEvent	    *theReply,
	long		    refCon)
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
	return error;

    FOR_ALL_BUFFERS(buf)
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

    error = HandleUnusedParms(theAEvent);

    return error;
}

/*
 *
 */

/* Taken from MoreAppleEvents:ProcessHelpers*/
    pascal	OSErr
FindProcessBySignature(
	const OSType		targetType,
	const OSType		targetCreator,
	ProcessSerialNumberPtr	psnPtr)
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

static long drop_numFiles;
static short drop_gotPosition;
static SelectionRange drop_thePosition;

    static void
drop_callback(void *cookie UNUSED)
{
    /* TODO: Handle the goto/select line more cleanly */
    if ((drop_numFiles == 1) & (drop_gotPosition))
    {
	if (drop_thePosition.lineNum >= 0)
	{
	    lnum = drop_thePosition.lineNum + 1;
	/*  oap->motion_type = MLINE;
	    setpcmark();*/
	    if (lnum < 1L)
		lnum = 1L;
	    else if (lnum > curbuf->b_ml.ml_line_count)
		lnum = curbuf->b_ml.ml_line_count;
	    curwin->w_cursor.lnum = lnum;
	    curwin->w_cursor.col = 0;
	/*  beginline(BL_SOL | BL_FIX);*/
	}
	else
	    goto_byte(drop_thePosition.startRange + 1);
    }

    /* Update the screen display */
    update_screen(NOT_VALID);

    /* Select the text if possible */
    if (drop_gotPosition)
    {
	VIsual_active = TRUE;
	VIsual_select = FALSE;
	VIsual = curwin->w_cursor;
	if (drop_thePosition.lineNum < 0)
	{
	    VIsual_mode = 'v';
	    goto_byte(drop_thePosition.endRange);
	}
	else
	{
	    VIsual_mode = 'V';
	    VIsual.col = 0;
	}
    }
}

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

    /* the direct object parameter is the list of aliases to files (one or more) */
    error = AEGetParamDesc(theAEvent, keyDirectObject, typeAEList, &theList);
    if (error)
	return error;


    error = AEGetParamPtr(theAEvent, keyAEPosition, typeChar, &typeCode, (Ptr) &thePosition, sizeof(SelectionRange), &actualSize);
    if (error == noErr)
	gotPosition = true;
    if (error == errAEDescNotFound)
	error = noErr;
    if (error)
	return error;

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

    reset_VIsual();
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
	int fnum = -1;

	/* these are the initial files dropped on the Vim icon */
	for (i = 0 ; i < numFiles; i++)
	{
	    if (ga_grow(&global_alist.al_ga, 1) == FAIL
				      || (p = vim_strsave(fnames[i])) == NULL)
		mch_exit(2);
	    else
		alist_add(&global_alist, p, 2);
	    if (fnum == -1)
		fnum = GARGLIST[GARGCOUNT - 1].ae_fnum;
	}

	/* If the file name was already in the buffer list we need to switch
	 * to it. */
	if (curbuf->b_fnum != fnum)
	{
	    char_u cmd[30];

	    vim_snprintf((char *)cmd, 30, "silent %dbuffer", fnum);
	    do_cmdline_cmd(cmd);
	}

	/* Change directory to the location of the first file. */
	if (GARGCOUNT > 0
		      && vim_chdirfile(alist_name(&GARGLIST[0]), "drop") == OK)
	    shorten_fnames(TRUE);

	goto finished;
    }

    /* Handle the drop, :edit to get to the file */
    drop_numFiles = numFiles;
    drop_gotPosition = gotPosition;
    drop_thePosition = thePosition;
    handle_drop(numFiles, fnames, FALSE, drop_callback, NULL);

    setcursor();
    out_flush();

    /* Fake mouse event to wake from stall */
    PostEvent(mouseUp, 0);

finished:
    AEDisposeDesc(&theList); /* dispose what we allocated */

    error = HandleUnusedParms(theAEvent);
    return error;
}

/*
 *
 */

    pascal OSErr
Handle_aevt_oapp_AE(
	const AppleEvent    *theAEvent,
	AppleEvent	    *theReply,
	long		    refCon)
{
    OSErr	error = noErr;

    error = HandleUnusedParms(theAEvent);
    return error;
}

/*
 *
 */

    pascal OSErr
Handle_aevt_quit_AE(
	const AppleEvent    *theAEvent,
	AppleEvent	    *theReply,
	long		    refCon)
{
    OSErr	error = noErr;

    error = HandleUnusedParms(theAEvent);
    if (error)
	return error;

    /* Need to fake a :confirm qa */
    do_cmdline_cmd((char_u *)"confirm qa");

    return error;
}

/*
 *
 */

    pascal OSErr
Handle_aevt_pdoc_AE(
	const AppleEvent    *theAEvent,
	AppleEvent	    *theReply,
	long		    refCon)
{
    OSErr	error = noErr;

    error = HandleUnusedParms(theAEvent);

    return error;
}

/*
 * Handling of unknown AppleEvent
 *
 * (Just get rid of all the parms)
 */
    pascal OSErr
Handle_unknown_AE(
	const AppleEvent    *theAEvent,
	AppleEvent	    *theReply,
	long		    refCon)
{
    OSErr	error = noErr;

    error = HandleUnusedParms(theAEvent);

    return error;
}


/*
 * Install the various AppleEvent Handlers
 */
    OSErr
InstallAEHandlers(void)
{
    OSErr   error;

    /* install open application handler */
    error = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication,
		    NewAEEventHandlerUPP(Handle_aevt_oapp_AE), 0, false);
    if (error)
	return error;

    /* install quit application handler */
    error = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication,
		    NewAEEventHandlerUPP(Handle_aevt_quit_AE), 0, false);
    if (error)
	return error;

    /* install open document handler */
    error = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments,
		    NewAEEventHandlerUPP(HandleODocAE), 0, false);
    if (error)
	return error;

    /* install print document handler */
    error = AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments,
		    NewAEEventHandlerUPP(Handle_aevt_pdoc_AE), 0, false);

/* Install Core Suite */
/*  error = AEInstallEventHandler(kAECoreSuite, kAEClone,
		    NewAEEventHandlerUPP(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEClose,
		    NewAEEventHandlerUPP(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAECountElements,
		    NewAEEventHandlerUPP(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAECreateElement,
		    NewAEEventHandlerUPP(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEDelete,
		    NewAEEventHandlerUPP(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEDoObjectsExist,
		    NewAEEventHandlerUPP(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEGetData,
		    NewAEEventHandlerUPP(Handle_unknown_AE), kAEGetData, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEGetDataSize,
		    NewAEEventHandlerUPP(Handle_unknown_AE), kAEGetDataSize, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEGetClassInfo,
		    NewAEEventHandlerUPP(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEGetEventInfo,
		    NewAEEventHandlerUPP(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAEMove,
		    NewAEEventHandlerUPP(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAESave,
		    NewAEEventHandlerUPP(Handle_unknown_AE), nil, false);

    error = AEInstallEventHandler(kAECoreSuite, kAESetData,
		    NewAEEventHandlerUPP(Handle_unknown_AE), nil, false);
*/

#ifdef FEAT_CW_EDITOR
    /*
     * Bind codewarrior support handlers
     */
    error = AEInstallEventHandler('KAHL', 'GTTX',
		    NewAEEventHandlerUPP(Handle_KAHL_GTTX_AE), 0, false);
    if (error)
	return error;
    error = AEInstallEventHandler('KAHL', 'SRCH',
		    NewAEEventHandlerUPP(Handle_KAHL_SRCH_AE), 0, false);
    if (error)
	return error;
    error = AEInstallEventHandler('KAHL', 'MOD ',
		    NewAEEventHandlerUPP(Handle_KAHL_MOD_AE), 0, false);
#endif

    return error;

}
#endif /* USE_AEVENT */


/*
 * Callback function, installed by InstallFontPanelHandler(), below,
 * to handle Font Panel events.
 */
    static OSStatus
FontPanelHandler(
	EventHandlerCallRef inHandlerCallRef,
	EventRef inEvent,
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
InstallFontPanelHandler(void)
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
#define FONT_STYLE_BUFFER_SIZE 32
    static void
GetFontPanelSelection(char_u *outName)
{
    Str255	    buf;
    ByteCount	    fontNameLen = 0;
    ATSUFontID	    fid;
    char_u	    styleString[FONT_STYLE_BUFFER_SIZE];

    if (!outName)
	return;

    if (FMGetFontFamilyName(gFontPanelInfo.family, buf) == noErr)
    {
	/* Canonicalize localized font names */
	if (FMGetFontFromFontFamilyInstance(gFontPanelInfo.family,
		    gFontPanelInfo.style, &fid, NULL) != noErr)
	    return;

	/* Request font name with Mac encoding (otherwise we could
	 * get an unwanted utf-16 name) */
	if (ATSUFindFontName(fid, kFontFullName, kFontMacintoshPlatform,
		    kFontNoScriptCode, kFontNoLanguageCode,
		    255, (char *)outName, &fontNameLen, NULL) != noErr)
	    return;

	/* Only encode font size, because style (bold, italic, etc) is
	 * already part of the font full name */
	vim_snprintf((char *)styleString, FONT_STYLE_BUFFER_SIZE, ":h%d",
		gFontPanelInfo.size/*,
		((gFontPanelInfo.style & bold)!=0 ? ":b" : ""),
		((gFontPanelInfo.style & italic)!=0 ? ":i" : ""),
		((gFontPanelInfo.style & underline)!=0 ? ":u" : "")*/);

	if ((fontNameLen + STRLEN(styleString)) < 255)
	    STRCPY(outName + fontNameLen, styleString);
    }
    else
    {
	*outName = NUL;
    }
}


/*
 * ------------------------------------------------------------
 * Unfiled yet
 * ------------------------------------------------------------
 */

/*
 *  gui_mac_get_menu_item_index
 *
 *  Returns the index inside the menu where
 */
    short /* Should we return MenuItemIndex? */
gui_mac_get_menu_item_index(vimmenu_T *pMenu)
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
    }
    return itemIndex;
}

    static vimmenu_T *
gui_mac_get_vim_menu(short menuID, short itemIndex, vimmenu_T *pMenu)
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
	page = curwin->w_width - 5;
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
gui_mac_doInContentClick(EventRecord *theEvent, WindowPtr whichWindow)
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
	/* We hit a scrollbar */

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

	/* Convert the CTRL_MOUSE_LEFT to MOUSE_RIGHT */
	/* TODO: NEEDED? */
	clickIsPopup = FALSE;

	if (mouse_model_popup() && IsShowContextualMenuClick(theEvent))
	{
	    vimMouseButton = MOUSE_RIGHT;
	    vimModifiers &= ~MOUSE_CTRL;
	    clickIsPopup = TRUE;
	}

	/* Is it a double click ? */
	dblClick = ((theEvent->when - lastMouseTick) < GetDblTime());

	/* Send the mouse click to Vim */
	gui_send_mouse_event(vimMouseButton, thePoint.h,
					  thePoint.v, dblClick, vimModifiers);

	/* Create the rectangle around the cursor to detect
	 * the mouse dragging
	 */
#if 0
	/* TODO: Do we need to this even for the contextual menu?
	 * It may be require for popup_setpos, but for popup?
	 */
	if (vimMouseButton == MOUSE_LEFT)
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
gui_mac_doInDragClick(Point where, WindowPtr whichWindow)
{
    Rect	movingLimits;
    Rect	*movingLimitsPtr = &movingLimits;

    /* TODO: may try to prevent move outside screen? */
    movingLimitsPtr = GetRegionBounds(GetGrayRgn(), &movingLimits);
    DragWindow(whichWindow, where, movingLimitsPtr);
}

/*
 * Handle the click in the grow box
 */
    void
gui_mac_doInGrowClick(Point where, WindowPtr whichWindow)
{

    long	    newSize;
    unsigned short  newWidth;
    unsigned short  newHeight;
    Rect	    resizeLimits;
    Rect	    *resizeLimitsPtr = &resizeLimits;
    Rect	    NewContentRect;

    resizeLimitsPtr = GetRegionBounds(GetGrayRgn(), &resizeLimits);

    /* Set the minimum size */
    /* TODO: Should this come from Vim? */
    resizeLimits.top = 100;
    resizeLimits.left = 100;

    newSize = ResizeWindow(whichWindow, where, &resizeLimits, &NewContentRect);
    newWidth  = NewContentRect.right - NewContentRect.left;
    newHeight = NewContentRect.bottom - NewContentRect.top;
    gui_resize_shell(newWidth, newHeight);
    gui_mch_set_bg_color(gui.back_pixel);
    gui_set_shellsize(TRUE, FALSE, RESIZE_BOTH);
}

/*
 * Handle the click in the zoom box
 */
    static void
gui_mac_doInZoomClick(EventRecord *theEvent, WindowPtr whichWindow)
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
    /* ideal height is as high as we can get */
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
    if (gui.which_scrollbars[SBAR_BOTTOM])
	p.v += gui.scrollbar_height;

    ZoomWindowIdeal(whichWindow, thePart, &p);

    GetWindowBounds(whichWindow, kWindowContentRgn, &r);
    gui_resize_shell(r.right - r.left, r.bottom - r.top);
    gui_mch_set_bg_color(gui.back_pixel);
    gui_set_shellsize(TRUE, FALSE, RESIZE_BOTH);
}

/*
 * ------------------------------------------------------------
 * MacOS Event Handling procedure
 * ------------------------------------------------------------
 */

/*
 * Handle the Update Event
 */

    void
gui_mac_doUpdateEvent(EventRecord *event)
{
    WindowPtr	whichWindow;
    GrafPtr	savePort;
    RgnHandle	updateRgn;
    Rect	updateRect;
    Rect	*updateRectPtr;
    Rect	rc;
    Rect	growRect;
    RgnHandle	saveRgn;


    updateRgn = NewRgn();
    if (updateRgn == NULL)
	return;

    /* This could be done by the caller as we
     * don't require anything else out of the event
     */
    whichWindow = (WindowPtr) event->message;

    /* Save Current Port */
    GetPort(&savePort);

    /* Select the Window's Port */
    SetPortWindowPort(whichWindow);

    /* Let's update the window */
      BeginUpdate(whichWindow);
	/* Redraw the biggest rectangle covering the area
	 * to be updated.
	 */
	GetPortVisibleRegion(GetWindowPort(whichWindow), updateRgn);
# if 0
	/* Would be more appropriate to use the following but doesn't
	 * seem to work under MacOS X (Dany)
	 */
	GetWindowRegion(whichWindow, kWindowUpdateRgn, updateRgn);
# endif

	/* Use the HLock useless in Carbon? Is it harmful?*/
	HLock((Handle) updateRgn);

	  updateRectPtr = GetRegionBounds(updateRgn, &updateRect);
# if 0
	  /* Code from original Carbon Port (using GetWindowRegion.
	   * I believe the UpdateRgn is already in local (Dany)
	   */
	  GlobalToLocal(&topLeft(updateRect)); /* preCarbon? */
	  GlobalToLocal(&botRight(updateRect));
# endif
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
	DisposeRgn(updateRgn);

	/* Update scrollbars */
	DrawControls(whichWindow);

	/* Update the GrowBox */
	/* Taken from FAQ 33-27 */
	saveRgn = NewRgn();
	GetWindowBounds(whichWindow, kWindowGrowRgn, &growRect);
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
gui_mac_doActivateEvent(EventRecord *event)
{
    WindowPtr	whichWindow;

    whichWindow = (WindowPtr) event->message;
    /* Dim scrollbars */
    if (whichWindow == gui.VimWindow)
    {
	ControlRef rootControl;
	GetRootControl(gui.VimWindow, &rootControl);
	if ((event->modifiers) & activeFlag)
	    ActivateControl(rootControl);
	else
	    DeactivateControl(rootControl);
    }

    /* Activate */
    gui_focus_change((event->modifiers) & activeFlag);
}


/*
 * Handle the suspend/resume event
 * (apply to the application)
 */
    void
gui_mac_doSuspendEvent(EventRecord *event)
{
    /* The frontmost application just changed */

    /* NOTE: the suspend may happen before the deactivate
     *       seen on MacOS X
     */

    /* May not need to change focus as the window will
     * get an activate/deactivate event
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
#ifdef USE_CARBONKEYHANDLER
    static pascal OSStatus
gui_mac_handle_window_activate(
	EventHandlerCallRef nextHandler,
	EventRef	    theEvent,
	void		    *data)
{
    UInt32 eventClass = GetEventClass(theEvent);
    UInt32 eventKind  = GetEventKind(theEvent);

    if (eventClass == kEventClassWindow)
    {
	switch (eventKind)
	{
	    case kEventWindowActivated:
		im_on_window_switch(TRUE);
		return noErr;

	    case kEventWindowDeactivated:
		im_on_window_switch(FALSE);
		return noErr;
	}
    }

    return eventNotHandledErr;
}

    static pascal OSStatus
gui_mac_handle_text_input(
	EventHandlerCallRef nextHandler,
	EventRef	    theEvent,
	void		    *data)
{
    UInt32 eventClass = GetEventClass(theEvent);
    UInt32 eventKind  = GetEventKind(theEvent);

    if (eventClass != kEventClassTextInput)
	return eventNotHandledErr;

    if ((kEventTextInputUpdateActiveInputArea != eventKind) &&
	(kEventTextInputUnicodeForKeyEvent    != eventKind) &&
	(kEventTextInputOffsetToPos	      != eventKind) &&
	(kEventTextInputPosToOffset	      != eventKind) &&
	(kEventTextInputGetSelectedText       != eventKind))
	      return eventNotHandledErr;

    switch (eventKind)
    {
    case kEventTextInputUpdateActiveInputArea:
	return gui_mac_update_input_area(nextHandler, theEvent);
    case kEventTextInputUnicodeForKeyEvent:
	return gui_mac_unicode_key_event(nextHandler, theEvent);

    case kEventTextInputOffsetToPos:
    case kEventTextInputPosToOffset:
    case kEventTextInputGetSelectedText:
	break;
    }

    return eventNotHandledErr;
}

    static pascal
OSStatus gui_mac_update_input_area(
	EventHandlerCallRef nextHandler,
	EventRef	    theEvent)
{
    return eventNotHandledErr;
}

static int dialog_busy = FALSE;	    /* TRUE when gui_mch_dialog() wants the
				       keys */

# define INLINE_KEY_BUFFER_SIZE 80
    static pascal OSStatus
gui_mac_unicode_key_event(
	EventHandlerCallRef nextHandler,
	EventRef	    theEvent)
{
    /* Multibyte-friendly key event handler */
    OSStatus	err = -1;
    UInt32	actualSize;
    UniChar	*text;
    char_u	result[INLINE_KEY_BUFFER_SIZE];
    short	len = 0;
    UInt32	key_sym;
    char	charcode;
    int		key_char;
    UInt32	modifiers, vimModifiers;
    size_t	encLen;
    char_u	*to = NULL;
    Boolean	isSpecial = FALSE;
    int		i;
    EventRef	keyEvent;

    /* Mask the mouse (as per user setting) */
    if (p_mh)
	ObscureCursor();

    /* Don't use the keys when the dialog wants them. */
    if (dialog_busy)
	return eventNotHandledErr;

    if (noErr != GetEventParameter(theEvent, kEventParamTextInputSendText,
		typeUnicodeText, NULL, 0, &actualSize, NULL))
	return eventNotHandledErr;

    text = (UniChar *)alloc(actualSize);
    if (!text)
	return eventNotHandledErr;

    err = GetEventParameter(theEvent, kEventParamTextInputSendText,
	    typeUnicodeText, NULL, actualSize, NULL, text);
    require_noerr(err, done);

    err = GetEventParameter(theEvent, kEventParamTextInputSendKeyboardEvent,
	    typeEventRef, NULL, sizeof(EventRef), NULL, &keyEvent);
    require_noerr(err, done);

    err = GetEventParameter(keyEvent, kEventParamKeyModifiers,
	    typeUInt32, NULL, sizeof(UInt32), NULL, &modifiers);
    require_noerr(err, done);

    err = GetEventParameter(keyEvent, kEventParamKeyCode,
	    typeUInt32, NULL, sizeof(UInt32), NULL, &key_sym);
    require_noerr(err, done);

    err = GetEventParameter(keyEvent, kEventParamKeyMacCharCodes,
	    typeChar, NULL, sizeof(char), NULL, &charcode);
    require_noerr(err, done);

#ifndef USE_CMD_KEY
    if (modifiers & cmdKey)
	goto done;  /* Let system handle Cmd+... */
#endif

    key_char = charcode;
    vimModifiers = EventModifiers2VimModifiers(modifiers);

    /* Find the special key (eg., for cursor keys) */
    if (actualSize <= sizeof(UniChar) &&
	    ((text[0] < 0x20) || (text[0] == 0x7f)))
    {
	for (i = 0; special_keys[i].key_sym != (KeySym)0; ++i)
	    if (special_keys[i].key_sym == key_sym)
	    {
		key_char = TO_SPECIAL(special_keys[i].vim_code0,
			special_keys[i].vim_code1);
		key_char = simplify_key(key_char,
			(int *)&vimModifiers);
		isSpecial = TRUE;
		break;
	    }
    }

    /* Intercept CMD-. and CTRL-c */
    if (((modifiers & controlKey) && key_char == 'c') ||
	    ((modifiers & cmdKey) && key_char == '.'))
	got_int = TRUE;

    if (!isSpecial)
    {
	/* remove SHIFT for keys that are already shifted, e.g.,
	 * '(' and '*' */
	if (key_char < 0x100 && !isalpha(key_char) && isprint(key_char))
	    vimModifiers &= ~MOD_MASK_SHIFT;

	/* remove CTRL from keys that already have it */
	if (key_char < 0x20)
	    vimModifiers &= ~MOD_MASK_CTRL;

	/* don't process unicode characters here */
	if (!IS_SPECIAL(key_char))
	{
	    /* Following code to simplify and consolidate vimModifiers
	     * taken liberally from gui_w48.c */
	    key_char = simplify_key(key_char, (int *)&vimModifiers);

	    /* Interpret META, include SHIFT, etc. */
	    key_char = extract_modifiers(key_char, (int *)&vimModifiers);
	    if (key_char == CSI)
		key_char = K_CSI;

	    if (IS_SPECIAL(key_char))
		isSpecial = TRUE;
	}
    }

    if (vimModifiers)
    {
	result[len++] = CSI;
	result[len++] = KS_MODIFIER;
	result[len++] = vimModifiers;
    }

    if (isSpecial && IS_SPECIAL(key_char))
    {
	result[len++] = CSI;
	result[len++] = K_SECOND(key_char);
	result[len++] = K_THIRD(key_char);
    }
    else
    {
	encLen = actualSize;
	to = mac_utf16_to_enc(text, actualSize, &encLen);
	if (to)
	{
	    /* This is basically add_to_input_buf_csi() */
	    for (i = 0; i < encLen && len < (INLINE_KEY_BUFFER_SIZE-1); ++i)
	    {
		result[len++] = to[i];
		if (to[i] == CSI)
		{
		    result[len++] = KS_EXTRA;
		    result[len++] = (int)KE_CSI;
		}
	    }
	    vim_free(to);
	}
    }

    add_to_input_buf(result, len);
    err = noErr;

done:
    vim_free(text);
    if (err == noErr)
    {
	/* Fake event to wake up WNE (required to get
	 * key repeat working */
	PostEvent(keyUp, 0);
	return noErr;
    }

    return eventNotHandledErr;
}
#else
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

    /* Get the key code and its ASCII representation */
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
#endif

/*
 * Handle MouseClick
 */
    void
gui_mac_doMouseDownEvent(EventRecord *theEvent)
{
    short		thePart;
    WindowPtr		whichWindow;

    thePart = FindWindow(theEvent->where, &whichWindow);

#ifdef FEAT_GUI_TABLINE
    /* prevent that the vim window size changes if it's activated by a
       click into the tab pane */
    if (whichWindow == drawer)
	return;
#endif

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
	    gui_mac_doInZoomClick(theEvent, whichWindow);
	    break;
    }
}

/*
 * Handle MouseMoved
 * [this event is a moving in and out of a region]
 */
    void
gui_mac_doMouseMovedEvent(EventRecord *event)
{
    Point   thePoint;
    int_u   vimModifiers;

    thePoint = event->where;
    GlobalToLocal(&thePoint);
    vimModifiers = EventModifiers2VimMouseModifiers(event->modifiers);

    if (!Button())
	gui_mouse_moved(thePoint.h, thePoint.v);
    else
	if (!clickIsPopup)
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
gui_mac_doMouseUpEvent(EventRecord *theEvent)
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
    if (clickIsPopup)
    {
	vimModifiers &= ~MOUSE_CTRL;
	clickIsPopup = FALSE;
    }
    gui_send_mouse_event(MOUSE_RELEASE, thePoint.h, thePoint.v, FALSE, vimModifiers);
}

    static pascal OSStatus
gui_mac_mouse_wheel(EventHandlerCallRef nextHandler, EventRef theEvent,
								   void *data)
{
    Point	point;
    Rect	bounds;
    UInt32	mod;
    SInt32	delta;
    int_u	vim_mod;
    EventMouseWheelAxis axis;

    if (noErr == GetEventParameter(theEvent, kEventParamMouseWheelAxis,
			  typeMouseWheelAxis, NULL, sizeof(axis), NULL, &axis)
	    && axis != kEventMouseWheelAxisY)
	goto bail; /* Vim only does up-down scrolling */

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

    if (noErr == GetWindowBounds(gui.VimWindow, kWindowContentRgn, &bounds))
    {
	point.h -= bounds.left;
	point.v -= bounds.top;
    }

    gui_send_mouse_event((delta > 0) ? MOUSE_4 : MOUSE_5,
					    point.h, point.v, FALSE, vim_mod);

    /* post a bogus event to wake up WaitNextEvent */
    PostEvent(keyUp, 0);

    return noErr;

bail:
    /*
     * when we fail give any additional callback handler a chance to perform
     * its actions
     */
    return CallNextEventHandler(nextHandler, theEvent);
}

     void
gui_mch_mousehide(int hide)
{
    /* TODO */
}

#if 0

/*
 * This would be the normal way of invoking the contextual menu
 * but the Vim API doesn't seem to a support a request to get
 * the menu that we should display
 */
    void
gui_mac_handle_contextual_menu(EventRecord *event)
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
	/* But what about the current menu, is the many changed by ContextualMenuSelect */
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
gui_mac_handle_menu(long menuChoice)
{
    short	menu = HiWord(menuChoice);
    short	item = LoWord(menuChoice);
    vimmenu_T	*theVimMenu = root_menu;

    if (menu == 256)  /* TODO: use constant or gui.xyz */
    {
	if (item == 1)
	    gui_mch_beep(); /* TODO: Popup dialog or do :intro */
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
gui_mac_handle_event(EventRecord *event)
{
    OSErr	error;

    /* Handle contextual menu right now (if needed) */
    if (IsShowContextualMenuClick(event))
    {
# if 0
	gui_mac_handle_contextual_menu(event);
# else
	gui_mac_doMouseDownEvent(event);
# endif
	return;
    }

    /* Handle normal event */
    switch (event->what)
    {
#ifndef USE_CARBONKEYHANDLER
	case (keyDown):
	case (autoKey):
	    gui_mac_doKeyEvent(event);
	    break;
#endif
	case (keyUp):
	    /* We don't care about when the key is released */
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
gui_mac_find_font(char_u *font_name)
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

    /* Get the font name, minus the style suffix (:h, etc) */
    char_u fontName[256];
    char_u *styleStart = vim_strchr(font_name, ':');
    size_t fontNameLen = styleStart ? styleStart - font_name : STRLEN(fontName);
    vim_strncpy(fontName, font_name, fontNameLen);

    ATSUFontID fontRef;
    FMFontStyle fontStyle;
    font_id = 0;

    if (ATSUFindFontFromName(&pFontName[1], pFontName[0], kFontFullName,
		kFontMacintoshPlatform, kFontNoScriptCode, kFontNoLanguageCode,
		&fontRef) == noErr)
    {
	if (FMGetFontFamilyInstanceFromFont(fontRef, &font_id, &fontStyle) != noErr)
	    font_id = 0;
    }

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
	    if (ATSUFindFontFromName(&pFontName[1], pFontName[0],
			kFontFullName, kFontNoPlatformCode, kFontNoScriptCode,
			kFontNoLanguageCode, &fontRef) == noErr)
	    {
		if (FMGetFontFamilyInstanceFromFont(fontRef, &font_id, &fontStyle) != noErr)
		    font_id = 0;
	    }
    }

#else
    /* name = C2Pascal_save(menu->dname); */
    fontNamePtr = C2Pascal_save_and_remove_backslash(font_name);

    GetFNum(fontNamePtr, &font_id);
#endif


    if (font_id == 0)
    {
	/* Oups, the system font was it the one the user want */

	if (FMGetFontFamilyName(systemFont, systemFontname) != noErr)
	    return NOFONT;
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
 * GUI_MCH functionality
 * ------------------------------------------------------------
 */

/*
 * Parse the GUI related command-line arguments.  Any arguments used are
 * deleted from argv, and *argc is decremented accordingly.  This is called
 * when vim is started, whether or not the GUI has been started.
 */
    void
gui_mch_prepare(int *argc, char **argv)
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

#if 0
    InitCursor();

    RegisterAppearanceClient();

#ifdef USE_AEVENT
    (void) InstallAEHandlers();
#endif

    pomme = NewMenu(256, "\p\024"); /* 0x14= = Apple Menu */

    AppendMenu(pomme, "\pAbout VIM");

    InsertMenu(pomme, 0);

    DrawMenuBar();


#ifndef USE_OFFSETED_WINDOW
    SetRect(&windRect, 10, 48, 10+80*7 + 16, 48+24*11);
#else
    SetRect(&windRect, 300, 40, 300+80*7 + 16, 40+24*11);
#endif


    CreateNewWindow(kDocumentWindowClass,
		kWindowResizableAttribute | kWindowCollapseBoxAttribute,
		&windRect, &gui.VimWindow);
    SetPortWindowPort(gui.VimWindow);

    gui.char_width = 7;
    gui.char_height = 11;
    gui.char_ascent = 6;
    gui.num_rows = 24;
    gui.num_cols = 80;
    gui.in_focus = TRUE; /* For the moment -> syn. of front application */

    gScrollAction = NewControlActionUPP(gui_mac_scroll_action);
    gScrollDrag   = NewControlActionUPP(gui_mac_drag_thumb);

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
receiveHandler(WindowRef theWindow, void *handlerRefCon, DragRef theDrag)
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

    /* Fake mouse event to wake from stall */
    PostEvent(mouseUp, 0);

    return noErr;
}

/*
 * Initialise the GUI.  Create all the windows, set up all the call-backs
 * etc.
 */
    int
gui_mch_init(void)
{
    /* TODO: Move most of this stuff toward gui_mch_init */
    Rect	    windRect;
    MenuHandle	    pomme;
    EventHandlerRef mouseWheelHandlerRef;
    EventTypeSpec   eventTypeSpec;
    ControlRef	    rootControl;

    if (Gestalt(gestaltSystemVersion, &gMacSystemVersion) != noErr)
	gMacSystemVersion = 0x1000; /* TODO: Default to minimum sensible value */

#if 1
    InitCursor();

    RegisterAppearanceClient();

#ifdef USE_AEVENT
    (void) InstallAEHandlers();
#endif

    pomme = NewMenu(256, "\p\024"); /* 0x14= = Apple Menu */

    AppendMenu(pomme, "\pAbout VIM");

    InsertMenu(pomme, 0);

    DrawMenuBar();


#ifndef USE_OFFSETED_WINDOW
    SetRect(&windRect, 10, 48, 10+80*7 + 16, 48+24*11);
#else
    SetRect(&windRect, 300, 40, 300+80*7 + 16, 40+24*11);
#endif

    gui.VimWindow = NewCWindow(nil, &windRect, "\pgVim on Macintosh", true,
			zoomDocProc,
			(WindowPtr)-1L, true, 0);
    CreateRootControl(gui.VimWindow, &rootControl);
    InstallReceiveHandler((DragReceiveHandlerUPP)receiveHandler,
	    gui.VimWindow, NULL);
    SetPortWindowPort(gui.VimWindow);

    gui.char_width = 7;
    gui.char_height = 11;
    gui.char_ascent = 6;
    gui.num_rows = 24;
    gui.num_cols = 80;
    gui.in_focus = TRUE; /* For the moment -> syn. of front application */

    gScrollAction = NewControlActionUPP(gui_mac_scroll_action);
    gScrollDrag   = NewControlActionUPP(gui_mac_drag_thumb);

    /* Install Carbon event callbacks. */
    (void)InstallFontPanelHandler();

    dragRectEnbl = FALSE;
    dragRgn = NULL;
    dragRectControl = kCreateEmpty;
    cursorRgn = NewRgn();
#endif
    /* Display any pending error messages */
    display_errors();

    /* Get background/foreground colors from system */
    /* TODO: do the appropriate call to get real defaults */
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

    /* If Quartz-style text anti aliasing is available (see
       gui_mch_draw_string() below), enable it for all font sizes. */
    vim_setenv((char_u *)"QDTEXT_MINSIZE", (char_u *)"1");

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

#ifdef USE_CARBONKEYHANDLER
    InterfaceTypeList supportedServices = { kUnicodeDocument };
    NewTSMDocument(1, supportedServices, &gTSMDocument, 0);

    /* We don't support inline input yet, use input window by default */
    UseInputWindow(gTSMDocument, TRUE);

    /* Should we activate the document by default? */
    // ActivateTSMDocument(gTSMDocument);

    EventTypeSpec textEventTypes[] = {
	{ kEventClassTextInput, kEventTextInputUpdateActiveInputArea },
	{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent },
	{ kEventClassTextInput, kEventTextInputPosToOffset },
	{ kEventClassTextInput, kEventTextInputOffsetToPos },
    };

    keyEventHandlerUPP = NewEventHandlerUPP(gui_mac_handle_text_input);
    if (noErr != InstallApplicationEventHandler(keyEventHandlerUPP,
						NR_ELEMS(textEventTypes),
						textEventTypes, NULL, NULL))
    {
	DisposeEventHandlerUPP(keyEventHandlerUPP);
	keyEventHandlerUPP = NULL;
    }

    EventTypeSpec windowEventTypes[] = {
	{ kEventClassWindow, kEventWindowActivated },
	{ kEventClassWindow, kEventWindowDeactivated },
    };

    /* Install window event handler to support TSMDocument activate and
     * deactivate */
    winEventHandlerUPP = NewEventHandlerUPP(gui_mac_handle_window_activate);
    if (noErr != InstallWindowEventHandler(gui.VimWindow,
					   winEventHandlerUPP,
					   NR_ELEMS(windowEventTypes),
					   windowEventTypes, NULL, NULL))
    {
	DisposeEventHandlerUPP(winEventHandlerUPP);
	winEventHandlerUPP = NULL;
    }
#endif

#ifdef FEAT_GUI_TABLINE
    /*
     * Create the tabline
     */
    initialise_tabline();
#endif

    /* TODO: Load bitmap if using TOOLBAR */
    return OK;
}

/*
 * Called when the foreground or background color has been changed.
 */
    void
gui_mch_new_colors(void)
{
    /* TODO:
     * This proc is called when Normal is set to a value
     * so what must be done? I don't know
     */
}

/*
 * Open the GUI window which was created by a call to gui_mch_init().
 */
    int
gui_mch_open(void)
{
    ShowWindow(gui.VimWindow);

    if (gui_win_x != -1 && gui_win_y != -1)
	gui_mch_set_winpos(gui_win_x, gui_win_y);

    /*
     * Make the GUI the foreground process (in case it was launched
     * from the Terminal or via :gui).
     */
    {
	ProcessSerialNumber psn;
	if (GetCurrentProcess(&psn) == noErr)
	    SetFrontProcess(&psn);
    }

    return OK;
}

#ifdef USE_ATSUI_DRAWING
    static void
gui_mac_dispose_atsui_style(void)
{
    if (p_macatsui && gFontStyle)
	ATSUDisposeStyle(gFontStyle);
    if (p_macatsui && gWideFontStyle)
	ATSUDisposeStyle(gWideFontStyle);
}
#endif

    void
gui_mch_exit(int rc)
{
    /* TODO: find out all what is missing here? */
    DisposeRgn(cursorRgn);

#ifdef USE_CARBONKEYHANDLER
    if (keyEventHandlerUPP)
	DisposeEventHandlerUPP(keyEventHandlerUPP);
#endif

    if (mouseWheelHandlerUPP != NULL)
	DisposeEventHandlerUPP(mouseWheelHandlerUPP);

#ifdef USE_ATSUI_DRAWING
    gui_mac_dispose_atsui_style();
#endif

#ifdef USE_CARBONKEYHANDLER
    FixTSMDocument(gTSMDocument);
    DeactivateTSMDocument(gTSMDocument);
    DeleteTSMDocument(gTSMDocument);
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
    Rect	bounds;
    OSStatus	status;

    /* Carbon >= 1.0.2, MacOS >= 8.5 */
    status = GetWindowBounds(gui.VimWindow, kWindowStructureRgn, &bounds);

    if (status != noErr)
	return FAIL;
    *x = bounds.left;
    *y = bounds.top;
    return OK;
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
    MoveWindowStructure(gui.VimWindow, x, y);
}

    void
gui_mch_set_shellsize(
    int		width,
    int		height,
    int		min_width,
    int		min_height,
    int		base_width,
    int		base_height,
    int		direction)
{
    CGrafPtr	VimPort;
    Rect	VimBound;

    if (gui.which_scrollbars[SBAR_LEFT])
    {
	VimPort = GetWindowPort(gui.VimWindow);
	GetPortBounds(VimPort, &VimBound);
	VimBound.left = -gui.scrollbar_width; /* + 1;*/
	SetPortBounds(VimPort, &VimBound);
    /*	GetWindowBounds(gui.VimWindow, kWindowGlobalPortRgn, &winPortRect); ??*/
    }
    else
    {
	VimPort = GetWindowPort(gui.VimWindow);
	GetPortBounds(VimPort, &VimBound);
	VimBound.left = 0;
	SetPortBounds(VimPort, &VimBound);
    }

    SizeWindow(gui.VimWindow, width, height, TRUE);

    gui_resize_shell(width, height);
}

/*
 * Get the screen dimensions.
 * Allow 10 pixels for horizontal borders, 40 for vertical borders.
 * Is there no way to find out how wide the borders really are?
 * TODO: Add live update of those value on suspend/resume.
 */
    void
gui_mch_get_screen_dimensions(int *screen_w, int *screen_h)
{
    GDHandle	dominantDevice = GetMainDevice();
    Rect	screenRect = (**dominantDevice).gdRect;

    *screen_w = screenRect.right - 10;
    *screen_h = screenRect.bottom - 40;
}


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

#ifdef USE_ATSUI_DRAWING
    static void
gui_mac_create_atsui_style(void)
{
    if (p_macatsui && gFontStyle == NULL)
    {
	if (ATSUCreateStyle(&gFontStyle) != noErr)
	    gFontStyle = NULL;
    }
    if (p_macatsui && gWideFontStyle == NULL)
    {
	if (ATSUCreateStyle(&gWideFontStyle) != noErr)
	    gWideFontStyle = NULL;
    }

    p_macatsui_last = p_macatsui;
}
#endif

/*
 * Initialise vim to use the font with the given name.	Return FAIL if the font
 * could not be loaded, OK otherwise.
 */
    int
gui_mch_init_font(char_u *font_name, int fontset)
{
    /* TODO: Add support for bold italic underline proportional etc... */
    Str255	suggestedFont = "\pMonaco";
    int		suggestedSize = 10;
    FontInfo	font_info;
    short	font_id;
    GuiFont	font;
    char_u	used_font_name[512];

#ifdef USE_ATSUI_DRAWING
    gui_mac_create_atsui_style();
#endif

    if (font_name == NULL)
    {
	/* First try to get the suggested font */
	GetFNum(suggestedFont, &font_id);

	if (font_id == 0)
	{
	    /* Then pickup the standard application font */
	    font_id = GetAppFont();
	    STRCPY(used_font_name, "default");
	}
	else
	    STRCPY(used_font_name, "Monaco");
	font = (suggestedSize << 16) + ((long) font_id & 0xFFFF);
    }
    else if (STRCMP(font_name, "*") == 0)
    {
	char_u *new_p_guifont;

	font = gui_mac_select_font(used_font_name);
	if (font == NOFONT)
	    return FAIL;

	/* Set guifont to the name of the selected font. */
	new_p_guifont = alloc(STRLEN(used_font_name) + 1);
	if (new_p_guifont != NULL)
	{
	    STRCPY(new_p_guifont, used_font_name);
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
    else
    {
	font = gui_mac_find_font(font_name);
	vim_strncpy(used_font_name, font_name, sizeof(used_font_name) - 1);

	if (font == NOFONT)
	    return FAIL;
    }

    gui.norm_font = font;

    hl_set_font_name(used_font_name);

    TextSize(font >> 16);
    TextFont(font & 0xFFFF);

    GetFontInfo(&font_info);

    gui.char_ascent = font_info.ascent;
    gui.char_width  = CharWidth('_');
    gui.char_height = font_info.ascent + font_info.descent + p_linespace;

#ifdef USE_ATSUI_DRAWING
    if (p_macatsui && gFontStyle)
	gui_mac_set_font_attributes(font);
#endif

    return OK;
}

/*
 * Adjust gui.char_height (after 'linespace' was changed).
 */
    int
gui_mch_adjust_charheight(void)
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
gui_mch_get_font(char_u *name, int giveErrorIfMissing)
{
    GuiFont font;

    font = gui_mac_find_font(name);

    if (font == NOFONT)
    {
	if (giveErrorIfMissing)
	    semsg(_(e_font), name);
	return NOFONT;
    }
    /*
     * TODO : Accept only monospace
     */

    return font;
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Return the name of font "font" in allocated memory.
 * Don't know how to get the actual name, thus use the provided name.
 */
    char_u *
gui_mch_get_fontname(GuiFont font, char_u *name)
{
    if (name == NULL)
	return NULL;
    return vim_strsave(name);
}
#endif

#ifdef USE_ATSUI_DRAWING
    static void
gui_mac_set_font_attributes(GuiFont font)
{
    ATSUFontID	fontID;
    Fixed	fontSize;
    Fixed	fontWidth;

    fontID    = font & 0xFFFF;
    fontSize  = Long2Fix(font >> 16);
    fontWidth = Long2Fix(gui.char_width);

    ATSUAttributeTag attribTags[] =
    {
	kATSUFontTag, kATSUSizeTag, kATSUImposeWidthTag,
	kATSUMaxATSUITagValue + 1
    };

    ByteCount attribSizes[] =
    {
	sizeof(ATSUFontID), sizeof(Fixed), sizeof(fontWidth),
	sizeof(font)
    };

    ATSUAttributeValuePtr attribValues[] =
    {
	&fontID, &fontSize, &fontWidth, &font
    };

    if (FMGetFontFromFontFamilyInstance(fontID, 0, &fontID, NULL) == noErr)
    {
	if (ATSUSetAttributes(gFontStyle,
		    (sizeof attribTags) / sizeof(ATSUAttributeTag),
		    attribTags, attribSizes, attribValues) != noErr)
	{
# ifndef NDEBUG
	    fprintf(stderr, "couldn't set font style\n");
# endif
	    ATSUDisposeStyle(gFontStyle);
	    gFontStyle = NULL;
	}

	if (has_mbyte)
	{
	    /* FIXME: we should use a more mbyte sensitive way to support
	     * wide font drawing */
	    fontWidth = Long2Fix(gui.char_width * 2);

	    if (ATSUSetAttributes(gWideFontStyle,
			(sizeof attribTags) / sizeof(ATSUAttributeTag),
			attribTags, attribSizes, attribValues) != noErr)
	    {
		ATSUDisposeStyle(gWideFontStyle);
		gWideFontStyle = NULL;
	    }
	}
    }
}
#endif

/*
 * Set the current text font.
 */
    void
gui_mch_set_font(GuiFont font)
{
#ifdef USE_ATSUI_DRAWING
    GuiFont			currFont;
    ByteCount			actualFontByteCount;

    if (p_macatsui && gFontStyle)
    {
	/* Avoid setting same font again */
	if (ATSUGetAttribute(gFontStyle, kATSUMaxATSUITagValue + 1,
		    sizeof(font), &currFont, &actualFontByteCount) == noErr
		&& actualFontByteCount == (sizeof font))
	{
	    if (currFont == font)
		return;
	}

	gui_mac_set_font_attributes(font);
    }

    if (p_macatsui && !gIsFontFallbackSet)
    {
	/* Setup automatic font substitution. The user's guifontwide
	 * is tried first, then the system tries other fonts. */
/*
	ATSUAttributeTag fallbackTags[] = { kATSULineFontFallbacksTag };
	ByteCount fallbackSizes[] = { sizeof(ATSUFontFallbacks) };
	ATSUCreateFontFallbacks(&gFontFallbacks);
	ATSUSetObjFontFallbacks(gFontFallbacks, );
*/
	if (gui.wide_font)
	{
	    ATSUFontID fallbackFonts;
	    gIsFontFallbackSet = TRUE;

	    if (FMGetFontFromFontFamilyInstance(
			(gui.wide_font & 0xFFFF),
			0,
			&fallbackFonts,
			NULL) == noErr)
	    {
		ATSUSetFontFallbacks((sizeof fallbackFonts)/sizeof(ATSUFontID),
				     &fallbackFonts,
				     kATSUSequentialFallbacksPreferred);
	    }
/*
	ATSUAttributeValuePtr fallbackValues[] = { };
*/
	}
    }
#endif
    TextSize(font >> 16);
    TextFont(font & 0xFFFF);
}

/*
 * If a font is not going to be used, free its structure.
 */
    void
gui_mch_free_font(GuiFont font)
{
    /*
     * Free font when "font" is not 0.
     * Nothing to do in the current implementation, since
     * nothing is allocated for each font used.
     */
}

/*
 * Return the Pixel value (color) for the given color name.  This routine was
 * pretty much taken from example code in the Silicon Graphics OSF/Motif
 * Programmer's Guide.
 * Return INVALCOLOR when failed.
 */
    guicolor_T
gui_mch_get_color(char_u *name)
{
    /* TODO: Add support for the new named color of MacOS 8
     */
    RGBColor	MacColor;

    if (STRICMP(name, "hilite") == 0)
    {
	LMGetHiliteRGB(&MacColor);
	return (RGB(MacColor.red >> 8, MacColor.green >> 8, MacColor.blue >> 8));
    }
    return gui_get_color_cmn(name);
}

    guicolor_T
gui_mch_get_rgb_color(int r, int g, int b)
{
    return gui_get_rgb_color_cmn(r, g, b);
}

/*
 * Set the current text foreground color.
 */
    void
gui_mch_set_fg_color(guicolor_T color)
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
gui_mch_set_bg_color(guicolor_T color)
{
    RGBColor TheColor;

    TheColor.red = Red(color) * 0x0101;
    TheColor.green = Green(color) * 0x0101;
    TheColor.blue = Blue(color) * 0x0101;

    RGBBackColor(&TheColor);
}

RGBColor specialColor;

/*
 * Set the current text special color.
 */
    void
gui_mch_set_sp_color(guicolor_T color)
{
    specialColor.red = Red(color) * 0x0101;
    specialColor.green = Green(color) * 0x0101;
    specialColor.blue = Blue(color) * 0x0101;
}

/*
 * Draw undercurl at the bottom of the character cell.
 */
    static void
draw_undercurl(int flags, int row, int col, int cells)
{
    int			x;
    int			offset;
    const static int	val[8] = {1, 0, 0, 0, 1, 2, 2, 2 };
    int			y = FILL_Y(row + 1) - 1;

    RGBForeColor(&specialColor);

    offset = val[FILL_X(col) % 8];
    MoveTo(FILL_X(col), y - offset);

    for (x = FILL_X(col); x < FILL_X(col + cells); ++x)
    {
	offset = val[x % 8];
	LineTo(x, y - offset);
    }
}


    static void
draw_string_QD(int row, int col, char_u *s, int len, int flags)
{
    char_u	*tofree = NULL;

    if (output_conv.vc_type != CONV_NONE)
    {
	tofree = string_convert(&output_conv, s, &len);
	if (tofree != NULL)
	    s = tofree;
    }

    /*
     * On OS X, try using Quartz-style text antialiasing.
     */
    if (gMacSystemVersion >= 0x1020)
    {
	/* Quartz antialiasing is available only in OS 10.2 and later. */
	UInt32 qd_flags = (p_antialias ?
			     kQDUseCGTextRendering | kQDUseCGTextMetrics : 0);
	QDSwapTextFlags(qd_flags);
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
    if (((gMacSystemVersion >= 0x1020 && p_antialias) || p_linespace != 0)
	    && !(flags & DRAW_TRANSP))
    {
	Rect rc;

	rc.left = FILL_X(col);
	rc.top = FILL_Y(row);
	/* Multibyte computation taken from gui_w32.c */
	if (has_mbyte)
	{
	    /* Compute the length in display cells. */
	    rc.right = FILL_X(col + mb_string2cells(s, len));
	}
	else
	    rc.right = FILL_X(col + len) + (col + len == Columns);
	rc.bottom = FILL_Y(row + 1);
	EraseRect(&rc);
    }

    if (gMacSystemVersion >= 0x1020 && p_antialias)
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
    {
	/* Use old-style, non-antialiased QuickDraw text rendering. */
	TextMode(srcCopy);
	TextFace(normal);

    /*  SelectFont(hdc, gui.currFont); */

	if (flags & DRAW_TRANSP)
	    TextMode(srcOr);

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
	if (flags & DRAW_STRIKE)
	{
	    MoveTo(FILL_X(col), FILL_Y(row + 1) - gui.char_height/2);
	    LineTo(FILL_X(col + len) - 1, FILL_Y(row + 1) - gui.char_height/2);
	}
    }

    if (flags & DRAW_UNDERC)
	draw_undercurl(flags, row, col, len);

    vim_free(tofree);
}

#ifdef USE_ATSUI_DRAWING

    static void
draw_string_ATSUI(int row, int col, char_u *s, int len, int flags)
{
    /* ATSUI requires utf-16 strings */
    UniCharCount utf16_len;
    UniChar *tofree = mac_enc_to_utf16(s, len, (size_t *)&utf16_len);
    utf16_len /= sizeof(UniChar);

    /* - ATSUI automatically antialiases text (Someone)
     * - for some reason it does not work... (Jussi) */
#ifdef MAC_ATSUI_DEBUG
    fprintf(stderr, "row = %d, col = %d, len = %d: '%c'\n",
	    row, col, len, len == 1 ? s[0] : ' ');
#endif
    /*
     * When antialiasing we're using srcOr mode, we have to clear the block
     * before drawing the text.
     * Also needed when 'linespace' is non-zero to remove the cursor and
     * underlining.
     * But not when drawing transparently.
     * The following is like calling gui_mch_clear_block(row, col, row, col +
     * len - 1), but without setting the bg color to gui.back_pixel.
     */
    if ((flags & DRAW_TRANSP) == 0)
    {
	Rect rc;

	rc.left = FILL_X(col);
	rc.top = FILL_Y(row);
	/* Multibyte computation taken from gui_w32.c */
	if (has_mbyte)
	{
	    /* Compute the length in display cells. */
	    rc.right = FILL_X(col + mb_string2cells(s, len));
	}
	else
	    rc.right = FILL_X(col + len) + (col + len == Columns);

	rc.bottom = FILL_Y(row + 1);
	EraseRect(&rc);
    }

    {
	TextMode(srcCopy);
	TextFace(normal);

	/*  SelectFont(hdc, gui.currFont); */
	if (flags & DRAW_TRANSP)
	    TextMode(srcOr);

	MoveTo(TEXT_X(col), TEXT_Y(row));

	if (gFontStyle && flags & DRAW_BOLD)
	{
	    Boolean attValue = true;
	    ATSUAttributeTag attribTags[] = { kATSUQDBoldfaceTag };
	    ByteCount attribSizes[] = { sizeof(Boolean) };
	    ATSUAttributeValuePtr attribValues[] = { &attValue };

	    ATSUSetAttributes(gFontStyle, 1, attribTags, attribSizes, attribValues);
	}

	UInt32 useAntialias = p_antialias ? kATSStyleApplyAntiAliasing
					  : kATSStyleNoAntiAliasing;
	if (useAntialias != useAntialias_cached)
	{
	    ATSUAttributeTag attribTags[] = { kATSUStyleRenderingOptionsTag };
	    ByteCount attribSizes[] = { sizeof(UInt32) };
	    ATSUAttributeValuePtr attribValues[] = { &useAntialias };

	    if (gFontStyle)
		ATSUSetAttributes(gFontStyle, 1, attribTags,
						   attribSizes, attribValues);
	    if (gWideFontStyle)
		ATSUSetAttributes(gWideFontStyle, 1, attribTags,
						   attribSizes, attribValues);

	    useAntialias_cached = useAntialias;
	}

	if (has_mbyte)
	{
	    int n, width_in_cell, last_width_in_cell;
	    UniCharArrayOffset offset = 0;
	    UniCharCount yet_to_draw = 0;
	    ATSUTextLayout textLayout;
	    ATSUStyle      textStyle;

	    last_width_in_cell = 1;
	    ATSUCreateTextLayout(&textLayout);
	    ATSUSetTextPointerLocation(textLayout, tofree,
				       kATSUFromTextBeginning,
				       kATSUToTextEnd, utf16_len);
	    /*
	       ATSUSetRunStyle(textLayout, gFontStyle,
	       kATSUFromTextBeginning, kATSUToTextEnd); */

	    /* Compute the length in display cells. */
	    for (n = 0; n < len; n += MB_BYTE2LEN(s[n]))
	    {
		width_in_cell = (*mb_ptr2cells)(s + n);

		/* probably we are switching from single byte character
		 * to multibyte characters (which requires more than one
		 * cell to draw) */
		if (width_in_cell != last_width_in_cell)
		{
#ifdef MAC_ATSUI_DEBUG
		    fprintf(stderr, "\tn = %2d, (%d-%d), offset = %d, yet_to_draw = %d\n",
			    n, last_width_in_cell, width_in_cell, offset, yet_to_draw);
#endif
		    textStyle = last_width_in_cell > 1 ? gWideFontStyle
								 : gFontStyle;

		    ATSUSetRunStyle(textLayout, textStyle, offset, yet_to_draw);
		    offset += yet_to_draw;
		    yet_to_draw = 0;
		    last_width_in_cell = width_in_cell;
		}

		yet_to_draw++;
	    }

	    if (yet_to_draw)
	    {
#ifdef MAC_ATSUI_DEBUG
		fprintf(stderr, "\tn = %2d, (%d-%d), offset = %d, yet_to_draw = %d\n",
			n, last_width_in_cell, width_in_cell, offset, yet_to_draw);
#endif
		/* finish the rest style */
		textStyle = width_in_cell > 1 ? gWideFontStyle : gFontStyle;
		ATSUSetRunStyle(textLayout, textStyle, offset, kATSUToTextEnd);
	    }

	    ATSUSetTransientFontMatching(textLayout, TRUE);
	    ATSUDrawText(textLayout,
			 kATSUFromTextBeginning, kATSUToTextEnd,
			 kATSUUseGrafPortPenLoc, kATSUUseGrafPortPenLoc);
	    ATSUDisposeTextLayout(textLayout);
	}
	else
	{
	    ATSUTextLayout textLayout;

	    if (ATSUCreateTextLayoutWithTextPtr(tofree,
			kATSUFromTextBeginning, kATSUToTextEnd,
			utf16_len,
			(gFontStyle ? 1 : 0), &utf16_len,
			(gFontStyle ? &gFontStyle : NULL),
			&textLayout) == noErr)
	    {
		ATSUSetTransientFontMatching(textLayout, TRUE);

		ATSUDrawText(textLayout,
			kATSUFromTextBeginning, kATSUToTextEnd,
			kATSUUseGrafPortPenLoc, kATSUUseGrafPortPenLoc);

		ATSUDisposeTextLayout(textLayout);
	    }
	}

	/* drawing is done, now reset bold to normal */
	if (gFontStyle && flags & DRAW_BOLD)
	{
	    Boolean attValue = false;

	    ATSUAttributeTag attribTags[] = { kATSUQDBoldfaceTag };
	    ByteCount attribSizes[] = { sizeof(Boolean) };
	    ATSUAttributeValuePtr attribValues[] = { &attValue };

	    ATSUSetAttributes(gFontStyle, 1, attribTags, attribSizes,
								attribValues);
	}
    }

    if (flags & DRAW_UNDERC)
	draw_undercurl(flags, row, col, len);

    vim_free(tofree);
}
#endif

    void
gui_mch_draw_string(int row, int col, char_u *s, int len, int flags)
{
#if defined(USE_ATSUI_DRAWING)
    if (p_macatsui == 0 && p_macatsui_last != 0)
	/* switch from macatsui to nomacatsui */
	gui_mac_dispose_atsui_style();
    else if (p_macatsui != 0 && p_macatsui_last == 0)
	/* switch from nomacatsui to macatsui */
	gui_mac_create_atsui_style();

    if (p_macatsui)
	draw_string_ATSUI(row, col, s, len, flags);
    else
#endif
	draw_string_QD(row, col, s, len, flags);
}

/*
 * Return OK if the key with the termcap name "name" is supported.
 */
    int
gui_mch_haskey(char_u *name)
{
    int i;

    for (i = 0; special_keys[i].key_sym != (KeySym)0; i++)
	if (name[0] == special_keys[i].vim_code0 &&
					 name[1] == special_keys[i].vim_code1)
	    return OK;
    return FAIL;
}

    void
gui_mch_beep(void)
{
    SysBeep(1); /* Should this be 0? (????) */
}

    void
gui_mch_flash(int msec)
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
gui_mch_invert_rectangle(int r, int c, int nr, int nc)
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
gui_mch_iconify(void)
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
gui_mch_set_foreground(void)
{
    /* TODO */
}
#endif

/*
 * Draw a cursor without focus.
 */
    void
gui_mch_draw_hollow_cursor(guicolor_T color)
{
    Rect rc;

    /*
     * Note: FrameRect() excludes right and bottom of rectangle.
     */
    rc.left = FILL_X(gui.col);
    rc.top = FILL_Y(gui.row);
    rc.right = rc.left + gui.char_width;
    if (mb_lefthalve(gui.row, gui.col))
	rc.right += gui.char_width;
    rc.bottom = rc.top + gui.char_height;

    gui_mch_set_fg_color(color);

    FrameRect(&rc);
}

/*
 * Draw part of a cursor, only w pixels wide, and h pixels high.
 */
    void
gui_mch_draw_part_cursor(int w, int h, guicolor_T color)
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

    FrameRect(&rc);
//    PaintRect(&rc);
}



/*
 * Catch up with any queued X events.  This may put keyboard input into the
 * input buffer, call resize call-backs, trigger timers etc.  If there is
 * nothing in the X event queue (& no timers pending), then we return
 * immediately.
 */
    void
gui_mch_update(void)
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
    int
gui_mch_wait_for_chars(int wtime)
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
	/* TODO: reduce wtime accordingly???  */
	if (wtime > -1)
	    sleeppyTick = 60 * wtime / 1000;
	else
	    sleeppyTick = 32767;

	if (WaitNextEventWrp(mask, &event, sleeppyTick, dragRgn))
	{
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

/*
 * Output routines.
 */

/* Flush any output to the screen */
    void
gui_mch_flush(void)
{
    /* TODO: Is anything needed here? */
}

/*
 * Clear a rectangular region of the screen from text pos (row1, col1) to
 * (row2, col2) inclusive.
 */
    void
gui_mch_clear_block(int row1, int col1, int row2, int col2)
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
gui_mch_clear_all(void)
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
gui_mch_delete_lines(int row, int num_lines)
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
gui_mch_insert_lines(int row, int num_lines)
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
clip_mch_request_selection(VimClipboard *cbd)
{

    Handle	textOfClip;
    int		flavor = 0;
    Size	scrapSize;
    ScrapFlavorFlags	scrapFlags;
    ScrapRef    scrap = nil;
    OSStatus	error;
    int		type;
    char	*searchCR;
    char_u	*tempclip;


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
	error = GetScrapFlavorFlags(scrap, SCRAPTEXTFLAVOR, &scrapFlags);
	if (error != noErr)
	    return;

	error = GetScrapFlavorSize(scrap, SCRAPTEXTFLAVOR, &scrapSize);
	if (error != noErr)
	    return;
    }

    ReserveMem(scrapSize);

    /* In CARBON we don't need a Handle, a pointer is good */
    textOfClip = NewHandle(scrapSize);

    /* tempclip = alloc(scrapSize+1); */
    HLock(textOfClip);
    error = GetScrapFlavorData(scrap,
	    flavor ? VIMSCRAPFLAVOR : SCRAPTEXTFLAVOR,
	    &scrapSize, *textOfClip);
    scrapSize -= flavor;

    if (flavor)
	type = **textOfClip;
    else
	type = MAUTO;

    tempclip = alloc(scrapSize + 1);
    mch_memmove(tempclip, *textOfClip + flavor, scrapSize);
    tempclip[scrapSize] = 0;

#ifdef MACOS_CONVERT
    {
	/* Convert from utf-16 (clipboard) */
	size_t encLen = 0;
	char_u *to = mac_utf16_to_enc((UniChar *)tempclip, scrapSize, &encLen);

	if (to != NULL)
	{
	    scrapSize = encLen;
	    vim_free(tempclip);
	    tempclip = to;
	}
    }
#endif

    searchCR = (char *)tempclip;
    while (searchCR != NULL)
    {
	searchCR = strchr(searchCR, '\r');
	if (searchCR != NULL)
	    *searchCR = '\n';
    }

    clip_yank_selection(type, tempclip, scrapSize, cbd);

    vim_free(tempclip);
    HUnlock(textOfClip);

    DisposeHandle(textOfClip);
}

    void
clip_mch_lose_selection(VimClipboard *cbd)
{
    /*
     * TODO: Really nothing to do?
     */
}

    int
clip_mch_own_selection(VimClipboard *cbd)
{
    return OK;
}

/*
 * Send the current selection to the clipboard.
 */
    void
clip_mch_set_selection(VimClipboard *cbd)
{
    Handle	textOfClip;
    long	scrapSize;
    int		type;
    ScrapRef    scrap;

    char_u	*str = NULL;

    if (!cbd->owned)
	return;

    clip_get_selection(cbd);

    /*
     * Once we set the clipboard, lose ownership.  If another application sets
     * the clipboard, we don't want to think that we still own it.
     */
    cbd->owned = FALSE;

    type = clip_convert_selection(&str, (long_u *)&scrapSize, cbd);

#ifdef MACOS_CONVERT
    size_t utf16_len = 0;
    UniChar *to = mac_enc_to_utf16(str, scrapSize, &utf16_len);
    if (to)
    {
	scrapSize = utf16_len;
	vim_free(str);
	str = (char_u *)to;
    }
#endif

    if (type >= 0)
    {
	ClearCurrentScrap();

	textOfClip = NewHandle(scrapSize + 1);
	HLock(textOfClip);

	**textOfClip = type;
	mch_memmove(*textOfClip + 1, str, scrapSize);
	GetCurrentScrap(&scrap);
	PutScrapFlavor(scrap, SCRAPTEXTFLAVOR, kScrapFlavorMaskNone,
		scrapSize, *textOfClip + 1);
	PutScrapFlavor(scrap, VIMSCRAPFLAVOR, kScrapFlavorMaskNone,
		scrapSize + 1, *textOfClip);
	HUnlock(textOfClip);
	DisposeHandle(textOfClip);
    }

    vim_free(str);
}

    void
gui_mch_set_text_area_pos(int x, int y, int w, int h)
{
    Rect	VimBound;

/*  HideWindow(gui.VimWindow); */
    GetWindowBounds(gui.VimWindow, kWindowGlobalPortRgn, &VimBound);

    if (gui.which_scrollbars[SBAR_LEFT])
	VimBound.left = -gui.scrollbar_width + 1;
    else
	VimBound.left = 0;

    SetWindowBounds(gui.VimWindow, kWindowGlobalPortRgn, &VimBound);

    ShowWindow(gui.VimWindow);
}

/*
 * Menu stuff.
 */

    void
gui_mch_enable_menu(int flag)
{
    /*
     * Menu is always active.
     */
}

    void
gui_mch_set_menu_pos(int x, int y, int w, int h)
{
    /*
     * The menu is always at the top of the screen.
     */
}

/*
 * Add a sub menu to the menu bar.
 */
    void
gui_mch_add_menu(vimmenu_T *menu, int idx)
{
    /*
     * TODO: Try to use only menu_id instead of both menu_id and menu_handle.
     * TODO: use menu->mnemonic and menu->actext
     * TODO: Try to reuse menu id
     *       Carbon Help suggest to use only id between 1 and 235
     */
    static long	 next_avail_id = 128;
    long	 menu_after_me = 0; /* Default to the end */
    CFStringRef name;
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
#ifdef MACOS_CONVERT
    name = menu_title_removing_mnemonic(menu);
#else
    name = C2Pascal_save(menu->dname);
#endif
    if (name == NULL)
	return;

    /* Create the menu unless it's the help menu */
    {
	/* Carbon suggest use of
	 * OSStatus CreateNewMenu(MenuID, MenuAttributes, MenuRef *);
	 * OSStatus SetMenuTitle(MenuRef, ConstStr255Param title);
	 */
	menu->submenu_id = next_avail_id;
	if (CreateNewMenu(menu->submenu_id, 0, (MenuRef *)&menu->submenu_handle) == noErr)
	    SetMenuTitleWithCFString((MenuRef)menu->submenu_handle, name);
	next_avail_id++;
    }

    if (parent == NULL)
    {
	/* Adding a menu to the menubar, or in the no mans land (for PopUp) */

	/* TODO: Verify if we could only Insert Menu if really part of the
	 * menubar The Inserted menu are scanned or the Command-key combos
	 */

	/* Insert the menu */
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
	SetMenuItemTextWithCFString(parent->submenu_handle, idx+1, name);
	SetItemCmd(parent->submenu_handle, idx+1, 0x1B);
	SetItemMark(parent->submenu_handle, idx+1, menu->submenu_id);
	InsertMenu(menu->submenu_handle, hierMenu);
    }

    CFRelease(name);

#if 0
    /* Done by Vim later on */
    DrawMenuBar();
#endif
}

/*
 * Add a menu item to a menu
 */
    void
gui_mch_add_menu_item(vimmenu_T *menu, int idx)
{
    CFStringRef name;
    vimmenu_T	*parent = menu->parent;
    int		menu_inserted;

    /* Cannot add item, if the menu have not been created */
    if (parent->submenu_id == 0)
	return;

    /* Could call SetMenuRefCon [CARBON] to associate with the Menu,
       for older OS call GetMenuItemData (menu, item, isCommandID?, data) */

    /* Convert the name */
#ifdef MACOS_CONVERT
    name = menu_title_removing_mnemonic(menu);
#else
    name = C2Pascal_save(menu->dname);
#endif

    /* Where are just a menu item, so no handle, no id */
    menu->submenu_id = 0;
    menu->submenu_handle = NULL;

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
	key = find_special_key(&p_actext, &modifiers, FALSE, FALSE, FALSE);
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
    SetMenuItemTextWithCFString(parent->submenu_handle, idx+1, name);

#if 0
    /* Called by Vim */
    DrawMenuBar();
#endif

    CFRelease(name);
}

    void
gui_mch_toggle_tearoffs(int enable)
{
    /* no tearoff menus */
}

/*
 * Destroy the machine specific menu widget.
 */
    void
gui_mch_destroy_menu(vimmenu_T *menu)
{
    short	index = gui_mac_get_menu_item_index(menu);

    if (index > 0)
    {
      if (menu->parent)
      {
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
gui_mch_menu_grey(vimmenu_T *menu, int grey)
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
gui_mch_menu_hidden(vimmenu_T *menu, int hidden)
{
    /* There's no hidden mode on MacOS */
    gui_mch_menu_grey(menu, hidden);
}


/*
 * This is called after setting all the menus to grey/hidden or not.
 */
    void
gui_mch_draw_menubar(void)
{
    DrawMenuBar();
}


/*
 * Scrollbar stuff.
 */

    void
gui_mch_enable_scrollbar(
	scrollbar_T	*sb,
	int		flag)
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
gui_mch_set_scrollbar_thumb(
	scrollbar_T *sb,
	long val,
	long size,
	long max)
{
    SetControl32BitMaximum (sb->id, max);
    SetControl32BitMinimum (sb->id, 0);
    SetControl32BitValue   (sb->id, val);
    SetControlViewSize     (sb->id, size);
#ifdef DEBUG_MAC_SB
    printf("thumb_sb (%x) %lx, %lx,%lx\n",sb->id, val, size, max);
#endif
}

    void
gui_mch_set_scrollbar_pos(
	scrollbar_T *sb,
	int x,
	int y,
	int w,
	int h)
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
gui_mch_create_scrollbar(
	scrollbar_T *sb,
	int orient)	/* SBAR_VERT or SBAR_HORIZ */
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
			 kControlScrollBarLiveProc,
			 (long) sb->ident);
#ifdef DEBUG_MAC_SB
    printf("create_sb (%x) %x\n",sb->id, orient);
#endif
}

    void
gui_mch_destroy_scrollbar(scrollbar_T *sb)
{
    gui_mch_set_bg_color(gui.back_pixel);
    DisposeControl(sb->id);
#ifdef DEBUG_MAC_SB
    printf("dest_sb (%x) \n",sb->id);
#endif
}

    int
gui_mch_is_blinking(void)
{
    return FALSE;
}

    int
gui_mch_is_blink_off(void)
{
    return FALSE;
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
gui_mch_stop_blink(int may_call_gui_update_cursor)
{
    if (may_call_gui_update_cursor)
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
gui_mch_start_blink(void)
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
    guicolor_T
gui_mch_get_rgb(guicolor_T pixel)
{
    return (guicolor_T)((Red(pixel) << 16) + (Green(pixel) << 8) + Blue(pixel));
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
    /* TODO: Add Ammon's safety check (Dany) */
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


/* ModalDialog() handler for message dialogs that have hotkey accelerators.
 * Expects a mapping of hotkey char to control index in gDialogHotKeys;
 * setting gDialogHotKeys to NULL disables any hotkey handling.
 */
    static pascal Boolean
DialogHotkeyFilterProc (
    DialogRef	    theDialog,
    EventRecord	    *event,
    DialogItemIndex *itemHit)
{
    char_u keyHit;

    if (event->what == keyDown || event->what == autoKey)
    {
	keyHit = (event->message & charCodeMask);

	if (gDialogHotKeys && gDialogHotKeys[keyHit])
	{
#ifdef DEBUG_MAC_DIALOG_HOTKEYS
	    printf("user pressed hotkey '%c' --> item %d\n", keyHit, gDialogHotKeys[keyHit]);
#endif
	    *itemHit = gDialogHotKeys[keyHit];

	    /* When handing off to StdFilterProc, pretend that the user
	     * clicked the control manually. Note that this is also supposed
	     * to cause the button to hilite briefly (to give some user
	     * feedback), but this seems not to actually work (or it's too
	     * fast to be seen).
	     */
	    event->what = kEventControlSimulateHit;

	    return true; /* we took care of it */
	}

	/* Defer to the OS's standard behavior for this event.
	 * This ensures that Enter will still activate the default button. */
	return StdFilterProc(theDialog, event, itemHit);
    }
    return false;      /* Let ModalDialog deal with it */
}


/* TODO: There have been some crashes with dialogs, check your inbox
 * (Jussi)
 */
    int
gui_mch_dialog(
    int		type,
    char_u	*title,
    char_u	*message,
    char_u	*buttons,
    int		dfltbutton,
    char_u	*textfield,
    int		ex_cmd)
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
    short	hotKeys[256];		/* map of hotkey -> control ID */
    char_u	aHotKey;
    Rect	box;
    short	button;
    short	lastButton;
    short	itemType;
    short	useIcon;
    short	width;
    short	totalButtonWidth = 0;   /* the width of all buttons together
					   including spacing */
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

    ModalFilterUPP dialogUPP;

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
    SetPortDialogPort(theDialog);

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

    /* initialize the hotkey mapping */
    vim_memset(hotKeys, 0, sizeof(hotKeys));

    for (;*buttonChar != 0;)
    {
	/* Get the name of the button */
	button++;
	len = 0;
	for (;((*buttonChar != DLG_BUTTON_SEP) && (*buttonChar != 0) && (len < 255)); buttonChar++)
	{
	    if (*buttonChar != DLG_HOTKEY_CHAR)
		name[++len] = *buttonChar;
	    else
	    {
		aHotKey = (char_u)*(buttonChar+1);
		if (aHotKey >= 'A' && aHotKey <= 'Z')
		    aHotKey = (char_u)((int)aHotKey + (int)'a' - (int)'A');
		hotKeys[aHotKey] = button;
#ifdef DEBUG_MAC_DIALOG_HOTKEYS
		printf("### hotKey for button %d is '%c'\n", button, aHotKey);
#endif
	    }
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
	/* Limit the size of any button to an acceptable value. */
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
	case VIM_GENERIC:
	case VIM_INFO:
	case VIM_QUESTION: useIcon = kNoteIcon; break;
	case VIM_WARNING:  useIcon = kCautionIcon; break;
	case VIM_ERROR:    useIcon = kStopIcon; break;
	default:	   useIcon = kStopIcon;
    }
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
	/* Cheat for now reuse the message and convert to text edit */
	inputItm.idx = lastButton + 3;
	inputDITL = GetResource('DITL', 132);
	AppendDITL(theDialog, inputDITL, overlayDITL);
	ReleaseResource(inputDITL);
	GetDialogItem(theDialog, inputItm.idx, &itemType, &itemHandle, &box);
/*	  SetDialogItem(theDialog, inputItm.idx, kEditTextDialogItem, itemHandle, &box);*/
	(void) C2PascalString(textfield, &name);
	SetDialogItemText(itemHandle, name);
	inputItm.width = StringWidth(name);

	/* Hotkeys don't make sense if there's a text field */
	gDialogHotKeys = NULL;
    }
    else
	/* Install hotkey table */
	gDialogHotKeys = (short *)&hotKeys;

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
	/* With vertical, it's better to have all buttons the same length */
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

    /* Magic resize */
    AutoSizeDialog(theDialog);
    /* Need a horizontal resize anyway so not that useful */

    /* Display it */
    ShowWindow(theWindow);
/*  BringToFront(theWindow); */
    SelectWindow(theWindow);

/*  DrawDialog(theDialog); */
#if 0
    GetPort(&oldPort);
    SetPortDialogPort(theDialog);
#endif

#ifdef USE_CARBONKEYHANDLER
    /* Avoid that we use key events for the main window. */
    dialog_busy = TRUE;
#endif

    /* Prepare the shortcut-handling filterProc for handing to the dialog */
    dialogUPP = NewModalFilterUPP(DialogHotkeyFilterProc);

    /* Hang until one of the button is hit */
    do
	ModalDialog(dialogUPP, &itemHit);
    while ((itemHit < 1) || (itemHit > lastButton));

#ifdef USE_CARBONKEYHANDLER
    dialog_busy = FALSE;
#endif

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
	vim_strncpy(textfield, &name[1], name[0]);
    }

    /* Restore the original graphical port */
    SetPort(oldPort);

    /* Free the modal filterProc */
    DisposeRoutineDescriptor(dialogUPP);

    /* Get ride of the dialog (free memory) */
    DisposeDialog(theDialog);

    return itemHit;
/*
 * Useful thing which could be used
 * SetDialogTimeout(): Auto click a button after timeout
 * SetDialogTracksCursor() : Get the I-beam cursor over input box
 * MoveDialogItem():	    Probably better than SetDialogItem
 * SizeDialogItem():		(but is it Carbon Only?)
 * AutoSizeDialog():	    Magic resize of dialog based on text length
 */
}
#endif /* FEAT_DIALOG_GUI */

/*
 * Display the saved error message(s).
 */
#ifdef USE_MCH_ERRMSG
    void
display_errors(void)
{
    char	*p;
    char_u	pError[256];

    if (error_ga.ga_data == NULL)
	return;

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
#endif

/*
 * Get current mouse coordinates in text window.
 */
    void
gui_mch_getmouse(int *x, int *y)
{
    Point where;

    GetMouse(&where);

    *x = where.h;
    *y = where.v;
}

    void
gui_mch_setmouse(int x, int y)
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
	 * Get first device with one button.
	 * This will probably be the standard mouse
	 * start at head of cursor dev list
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
gui_mch_show_popupmenu(vimmenu_T *menu)
{
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
    status = ContextualMenuSelect(CntxMenu, where, false, kCMHelpItemRemoveHelp,
		       HelpName, NULL, &CntxType, &CntxMenuID, &CntxMenuItem);

    if (status == noErr)
    {
	if (CntxType == kCMMenuItemSelected)
	{
	    /* Handle the menu CntxMenuID, CntxMenuItem */
	    /* The submenu can be handle directly by gui_mac_handle_menu */
	    /* But what about the current menu, is the menu changed by
	     * ContextualMenuSelect */
	    gui_mac_handle_menu((CntxMenuID << 16) + CntxMenuItem);
	}
	else if (CntxMenuID == kCMShowHelpSelected)
	{
	    /* Should come up with the help */
	}
    }

    /* Restore original Port */
    SetPort(savePort); /*OSX*/
}

#if defined(FEAT_CW_EDITOR) || defined(PROTO)
/* TODO: Is it need for MACOS_X? (Dany) */
    void
mch_post_buffer_write(buf_T *buf)
{
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
gui_mch_settitle(char_u *title, char_u *icon)
{
    /* TODO: Get vim to make sure maxlen (from p_titlelen) is smaller
     *       that 256. Even better get it to fit nicely in the titlebar.
     */
#ifdef MACOS_CONVERT
    CFStringRef windowTitle;
    size_t	windowTitleLen;
#else
    char_u   *pascalTitle;
#endif

    if (title == NULL)		/* nothing to do */
	return;

#ifdef MACOS_CONVERT
    windowTitleLen = STRLEN(title);
    windowTitle  = (CFStringRef)mac_enc_to_cfstring(title, windowTitleLen);

    if (windowTitle)
    {
	SetWindowTitleWithCFString(gui.VimWindow, windowTitle);
	CFRelease(windowTitle);
    }
#else
    pascalTitle = C2Pascal_save(title);
    if (pascalTitle != NULL)
    {
	SetWTitle(gui.VimWindow, pascalTitle);
	vim_free(pascalTitle);
    }
#endif
}
#endif

/*
 * Transferred from os_mac.c for MacOS X using os_unix.c prep work
 */

    int
C2PascalString(char_u *CString, Str255 *PascalString)
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
GetFSSpecFromPath(char_u *file, FSSpec *fileFSSpec)
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
 * Convert a FSSpec to a full path
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
    vim_strncpy(filenamePtr, &file.name[1], file.name[0]);

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
     * The functions used here are available in Carbon, but do nothing on
     * MacOS 8 and 9.
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
     /* theCPB.dirInfo.ioDirID     = irrelevant when ioFDirIndex = -1 */
	theCPB.dirInfo.ioDrDirID   = theCPB.dirInfo.ioDrParID;

	/* As ioFDirIndex = -1, get the info of ioDrDirID, */
	/*  *ioNamePtr[0 TO 31] will be updated		   */
	error = PBGetCatInfo(&theCPB,false);

	if (error)
	  return NULL;

	/* Put the new directoryName in front of the current fname */
	STRCPY(temporaryPtr, filenamePtr);
	vim_strncpy(filenamePtr, &directoryName[1], directoryName[0]);
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
 /* theCPB.dirInfo.ioDirID     = irrelevant when ioFDirIndex = -1 */
    theCPB.dirInfo.ioDrDirID   = theCPB.dirInfo.ioDrParID;

    /* As ioFDirIndex = -1, get the info of ioDrDirID, */
    /*	*ioNamePtr[0 TO 31] will be updated	       */
    error = PBGetCatInfo(&theCPB,false);

    if (error)
      return NULL;

    /* For MacOS Classic always add the volume name	     */
    /* For MacOS X add the volume name preceded by "Volumes" */
    /*	when we are not referring to the boot volume	     */
#ifdef USE_UNIXFILENAME
    if (file.vRefNum != dfltVol_vRefNum)
#endif
    {
	/* Add the volume name */
	STRCPY(temporaryPtr, filenamePtr);
	vim_strncpy(filenamePtr, &directoryName[1], directoryName[0]);
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

#if defined(USE_CARBONKEYHANDLER) || defined(PROTO)
/*
 * Input Method Control functions.
 */

/*
 * Notify cursor position to IM.
 */
    void
im_set_position(int row, int col)
{
# if 0
    /* TODO: Implement me! */
    im_start_row = row;
    im_start_col = col;
# endif
}

static ScriptLanguageRecord gTSLWindow;
static ScriptLanguageRecord gTSLInsert;
static ScriptLanguageRecord gTSLDefault = { 0, 0 };

static Component	     gTSCWindow;
static Component	     gTSCInsert;
static Component	     gTSCDefault;

static int		     im_initialized = 0;

    static void
im_on_window_switch(int active)
{
    ScriptLanguageRecord *slptr = NULL;
    OSStatus err;

    if (! gui.in_use)
	return;

    if (im_initialized == 0)
    {
	im_initialized = 1;

	/* save default TSM component (should be U.S.) to default */
	GetDefaultInputMethodOfClass(&gTSCDefault, &gTSLDefault,
				     kKeyboardInputMethodClass);
    }

    if (active == TRUE)
    {
	im_is_active = TRUE;
	ActivateTSMDocument(gTSMDocument);
	slptr = &gTSLWindow;

	if (slptr)
	{
	    err = SetDefaultInputMethodOfClass(gTSCWindow, slptr,
					       kKeyboardInputMethodClass);
	    if (err == noErr)
		err = SetTextServiceLanguage(slptr);

	    if (err == noErr)
		KeyScript(slptr->fScript | smKeyForceKeyScriptMask);
	}
    }
    else
    {
	err = GetTextServiceLanguage(&gTSLWindow);
	if (err == noErr)
	    slptr = &gTSLWindow;

	if (slptr)
	    GetDefaultInputMethodOfClass(&gTSCWindow, slptr,
					 kKeyboardInputMethodClass);

	im_is_active = FALSE;
	DeactivateTSMDocument(gTSMDocument);
    }
}

/*
 * Set IM status on ("active" is TRUE) or off ("active" is FALSE).
 */
    void
im_set_active(int active)
{
    ScriptLanguageRecord *slptr = NULL;
    OSStatus err;

    if (!gui.in_use)
	return;

    if (im_initialized == 0)
    {
	im_initialized = 1;

	/* save default TSM component (should be U.S.) to default */
	GetDefaultInputMethodOfClass(&gTSCDefault, &gTSLDefault,
				     kKeyboardInputMethodClass);
    }

    if (active == TRUE)
    {
	im_is_active = TRUE;
	ActivateTSMDocument(gTSMDocument);
	slptr = &gTSLInsert;

	if (slptr)
	{
	    err = SetDefaultInputMethodOfClass(gTSCInsert, slptr,
					       kKeyboardInputMethodClass);
	    if (err == noErr)
		err = SetTextServiceLanguage(slptr);

	    if (err == noErr)
		KeyScript(slptr->fScript | smKeyForceKeyScriptMask);
	}
    }
    else
    {
	err = GetTextServiceLanguage(&gTSLInsert);
	if (err == noErr)
	    slptr = &gTSLInsert;

	if (slptr)
	    GetDefaultInputMethodOfClass(&gTSCInsert, slptr,
					 kKeyboardInputMethodClass);

	/* restore to default when switch to normal mode, so than we could
	 * enter commands easier */
	SetDefaultInputMethodOfClass(gTSCDefault, &gTSLDefault,
				     kKeyboardInputMethodClass);
	SetTextServiceLanguage(&gTSLDefault);

	im_is_active = FALSE;
	DeactivateTSMDocument(gTSMDocument);
    }
}

/*
 * Get IM status.  When IM is on, return not 0.  Else return 0.
 */
    int
im_get_status(void)
{
    if (! gui.in_use)
	return 0;

    return im_is_active;
}

#endif



#if defined(FEAT_GUI_TABLINE) || defined(PROTO)
// drawer implementation
static MenuRef contextMenu = NULL;
enum
{
    kTabContextMenuId = 42
};

// the caller has to CFRelease() the returned string
    static CFStringRef
getTabLabel(tabpage_T *page)
{
    get_tabline_label(page, FALSE);
#ifdef MACOS_CONVERT
    return (CFStringRef)mac_enc_to_cfstring(NameBuff, STRLEN(NameBuff));
#else
    // TODO: check internal encoding?
    return CFStringCreateWithCString(kCFAllocatorDefault, (char *)NameBuff,
						   kCFStringEncodingMacRoman);
#endif
}


#define DRAWER_SIZE 150
#define DRAWER_INSET 16

static ControlRef dataBrowser = NULL;

// when the tabline is hidden, vim doesn't call update_tabline(). When
// the tabline is shown again, show_tabline() is called before update_tabline(),
// and because of this, the tab labels and vim's internal tabs are out of sync
// for a very short time. to prevent inconsistent state, we store the labels
// of the tabs, not pointers to the tabs (which are invalid for a short time).
static CFStringRef *tabLabels = NULL;
static int tabLabelsSize = 0;

enum
{
    kTabsColumn = 'Tabs'
};

    static int
getTabCount(void)
{
    tabpage_T	*tp;
    int		numTabs = 0;

    FOR_ALL_TABPAGES(tp)
	++numTabs;
    return numTabs;
}

// data browser item display callback
    static OSStatus
dbItemDataCallback(ControlRef browser,
	DataBrowserItemID itemID,
	DataBrowserPropertyID property /* column id */,
	DataBrowserItemDataRef itemData,
	Boolean changeValue)
{
    OSStatus status = noErr;

    // assert(property == kTabsColumn); // why is this violated??

    // changeValue is true if we have a modifiable list and data was changed.
    // In our case, it's always false.
    // (that is: if (changeValue) updateInternalData(); else return
    // internalData();
    if (!changeValue)
    {
	CFStringRef str;

	assert(itemID - 1 >= 0 && itemID - 1 < tabLabelsSize);
	str = tabLabels[itemID - 1];
	status = SetDataBrowserItemDataText(itemData, str);
    }
    else
	status = errDataBrowserPropertyNotSupported;

    return status;
}

// data browser action callback
    static void
dbItemNotificationCallback(ControlRef browser,
	DataBrowserItemID item,
	DataBrowserItemNotification message)
{
    switch (message)
    {
	case kDataBrowserItemSelected:
	    send_tabline_event(item);
	    break;
    }
}

// callbacks needed for contextual menu:
    static void
dbGetContextualMenuCallback(ControlRef browser,
	MenuRef *menu,
	UInt32 *helpType,
	CFStringRef *helpItemString,
	AEDesc *selection)
{
    // on mac os 9: kCMHelpItemNoHelp, but it's not the same
    *helpType = kCMHelpItemRemoveHelp; // OS X only ;-)
    *helpItemString = NULL;

    *menu = contextMenu;
}

    static void
dbSelectContextualMenuCallback(ControlRef browser,
	MenuRef menu,
	UInt32 selectionType,
	SInt16 menuID,
	MenuItemIndex menuItem)
{
    if (selectionType == kCMMenuItemSelected)
    {
	MenuCommand command;
	GetMenuItemCommandID(menu, menuItem, &command);

	// get tab that was selected when the context menu appeared
	// (there is always one tab selected). TODO: check if the context menu
	// isn't opened on an item but on empty space (has to be possible some
	// way, the finder does it too ;-) )
	Handle items = NewHandle(0);
	if (items != NULL)
	{
	    int numItems;

	    GetDataBrowserItems(browser, kDataBrowserNoItem, false,
					   kDataBrowserItemIsSelected, items);
	    numItems = GetHandleSize(items) / sizeof(DataBrowserItemID);
	    if (numItems > 0)
	    {
		int idx;
		DataBrowserItemID *itemsPtr;

		HLock(items);
		itemsPtr = (DataBrowserItemID *)*items;
		idx = itemsPtr[0];
		HUnlock(items);
		send_tabline_menu_event(idx, command);
	    }
	    DisposeHandle(items);
	}
    }
}

// focus callback of the data browser to always leave focus in vim
    static OSStatus
dbFocusCallback(EventHandlerCallRef handler, EventRef event, void *data)
{
    assert(GetEventClass(event) == kEventClassControl
	    && GetEventKind(event) == kEventControlSetFocusPart);

    return paramErr;
}


// drawer callback to resize data browser to drawer size
    static OSStatus
drawerCallback(EventHandlerCallRef handler, EventRef event, void *data)
{
    switch (GetEventKind(event))
    {
	case kEventWindowBoundsChanged: // move or resize
	    {
		UInt32 attribs;
		GetEventParameter(event, kEventParamAttributes, typeUInt32,
				       NULL, sizeof(attribs), NULL, &attribs);
		if (attribs & kWindowBoundsChangeSizeChanged) // resize
		{
		    Rect r;
		    GetWindowBounds(drawer, kWindowContentRgn, &r);
		    SetRect(&r, 0, 0, r.right - r.left, r.bottom - r.top);
		    SetControlBounds(dataBrowser, &r);
		    SetDataBrowserTableViewNamedColumnWidth(dataBrowser,
							kTabsColumn, r.right);
		}
	    }
	    break;
    }

    return eventNotHandledErr;
}

// Load DataBrowserChangeAttributes() dynamically on tiger (and better).
// This way the code works on 10.2 and 10.3 as well (it doesn't have the
// blue highlights in the list view on these systems, though. Oh well.)


#import <mach-o/dyld.h>

enum { kMyDataBrowserAttributeListViewAlternatingRowColors = (1 << 1) };

    static OSStatus
myDataBrowserChangeAttributes(ControlRef inDataBrowser,
	OptionBits inAttributesToSet,
	OptionBits inAttributesToClear)
{
    long osVersion;
    char *symbolName;
    NSSymbol symbol = NULL;
    OSStatus (*dataBrowserChangeAttributes)(ControlRef inDataBrowser,
	      OptionBits   inAttributesToSet, OptionBits inAttributesToClear);

    Gestalt(gestaltSystemVersion, &osVersion);
    if (osVersion < 0x1040) // only supported for 10.4 (and up)
	return noErr;

    // C name mangling...
    symbolName = "_DataBrowserChangeAttributes";
    if (!NSIsSymbolNameDefined(symbolName)
	    || (symbol = NSLookupAndBindSymbol(symbolName)) == NULL)
	return noErr;

    dataBrowserChangeAttributes = NSAddressOfSymbol(symbol);
    if (dataBrowserChangeAttributes == NULL)
	return noErr; // well...
    return dataBrowserChangeAttributes(inDataBrowser,
				      inAttributesToSet, inAttributesToClear);
}

    static void
initialise_tabline(void)
{
    Rect drawerRect = { 0, 0, 0, DRAWER_SIZE };
    DataBrowserCallbacks dbCallbacks;
    EventTypeSpec focusEvent = {kEventClassControl, kEventControlSetFocusPart};
    EventTypeSpec resizeEvent = {kEventClassWindow, kEventWindowBoundsChanged};
    DataBrowserListViewColumnDesc colDesc;

    // drawers have to have compositing enabled
    CreateNewWindow(kDrawerWindowClass,
	    kWindowStandardHandlerAttribute
		    | kWindowCompositingAttribute
		    | kWindowResizableAttribute
		    | kWindowLiveResizeAttribute,
	    &drawerRect, &drawer);

    SetThemeWindowBackground(drawer, kThemeBrushDrawerBackground, true);
    SetDrawerParent(drawer, gui.VimWindow);
    SetDrawerOffsets(drawer, kWindowOffsetUnchanged, DRAWER_INSET);


    // create list view embedded in drawer
    CreateDataBrowserControl(drawer, &drawerRect, kDataBrowserListView,
								&dataBrowser);

    dbCallbacks.version = kDataBrowserLatestCallbacks;
    InitDataBrowserCallbacks(&dbCallbacks);
    dbCallbacks.u.v1.itemDataCallback =
				NewDataBrowserItemDataUPP(dbItemDataCallback);
    dbCallbacks.u.v1.itemNotificationCallback =
		NewDataBrowserItemNotificationUPP(dbItemNotificationCallback);
    dbCallbacks.u.v1.getContextualMenuCallback =
	      NewDataBrowserGetContextualMenuUPP(dbGetContextualMenuCallback);
    dbCallbacks.u.v1.selectContextualMenuCallback =
	NewDataBrowserSelectContextualMenuUPP(dbSelectContextualMenuCallback);

    SetDataBrowserCallbacks(dataBrowser, &dbCallbacks);

    SetDataBrowserListViewHeaderBtnHeight(dataBrowser, 0); // no header
    SetDataBrowserHasScrollBars(dataBrowser, false, true); // only vertical
    SetDataBrowserSelectionFlags(dataBrowser,
	      kDataBrowserSelectOnlyOne | kDataBrowserNeverEmptySelectionSet);
    SetDataBrowserTableViewHiliteStyle(dataBrowser,
					     kDataBrowserTableViewFillHilite);
    Boolean b = false;
    SetControlData(dataBrowser, kControlEntireControl,
		  kControlDataBrowserIncludesFrameAndFocusTag, sizeof(b), &b);

    // enable blue background in data browser (this is only in 10.4 and vim
    // has to support older osx versions as well, so we have to load this
    // function dynamically)
    myDataBrowserChangeAttributes(dataBrowser,
		      kMyDataBrowserAttributeListViewAlternatingRowColors, 0);

    // install callback that keeps focus in vim and away from the data browser
    InstallControlEventHandler(dataBrowser, dbFocusCallback, 1, &focusEvent,
								  NULL, NULL);

    // install callback that keeps data browser at the size of the drawer
    InstallWindowEventHandler(drawer, drawerCallback, 1, &resizeEvent,
								  NULL, NULL);

    // add "tabs" column to data browser
    colDesc.propertyDesc.propertyID = kTabsColumn;
    colDesc.propertyDesc.propertyType = kDataBrowserTextType;

    // add if items can be selected (?): kDataBrowserListViewSelectionColumn
    colDesc.propertyDesc.propertyFlags = kDataBrowserDefaultPropertyFlags;

    colDesc.headerBtnDesc.version = kDataBrowserListViewLatestHeaderDesc;
    colDesc.headerBtnDesc.minimumWidth = 100;
    colDesc.headerBtnDesc.maximumWidth = 150;
    colDesc.headerBtnDesc.titleOffset = 0;
    colDesc.headerBtnDesc.titleString = CFSTR("Tabs");
    colDesc.headerBtnDesc.initialOrder = kDataBrowserOrderIncreasing;
    colDesc.headerBtnDesc.btnFontStyle.flags = 0; // use default font
    colDesc.headerBtnDesc.btnContentInfo.contentType = kControlContentTextOnly;

    AddDataBrowserListViewColumn(dataBrowser, &colDesc, 0);

    // create tabline popup menu required by vim docs (see :he tabline-menu)
    CreateNewMenu(kTabContextMenuId, 0, &contextMenu);
    AppendMenuItemTextWithCFString(contextMenu, CFSTR("Close Tab"), 0,
						    TABLINE_MENU_CLOSE, NULL);
    AppendMenuItemTextWithCFString(contextMenu, CFSTR("New Tab"), 0,
						      TABLINE_MENU_NEW, NULL);
    AppendMenuItemTextWithCFString(contextMenu, CFSTR("Open Tab..."), 0,
						     TABLINE_MENU_OPEN, NULL);
}


/*
 * Show or hide the tabline.
 */
    void
gui_mch_show_tabline(int showit)
{
    if (showit == 0)
	CloseDrawer(drawer, true);
    else
	OpenDrawer(drawer, kWindowEdgeRight, true);
}

/*
 * Return TRUE when tabline is displayed.
 */
    int
gui_mch_showing_tabline(void)
{
    WindowDrawerState state = GetDrawerState(drawer);

    return state == kWindowDrawerOpen || state == kWindowDrawerOpening;
}

/*
 * Update the labels of the tabline.
 */
    void
gui_mch_update_tabline(void)
{
    tabpage_T	*tp;
    int		numTabs = getTabCount();
    int		nr = 1;
    int		curtabidx = 1;

    // adjust data browser
    if (tabLabels != NULL)
    {
	int i;

	for (i = 0; i < tabLabelsSize; ++i)
	    CFRelease(tabLabels[i]);
	free(tabLabels);
    }
    tabLabels = (CFStringRef *)malloc(numTabs * sizeof(CFStringRef));
    tabLabelsSize = numTabs;

    for (tp = first_tabpage; tp != NULL; tp = tp->tp_next, ++nr)
    {
	if (tp == curtab)
	    curtabidx = nr;
	tabLabels[nr-1] = getTabLabel(tp);
    }

    RemoveDataBrowserItems(dataBrowser, kDataBrowserNoItem, 0, NULL,
						  kDataBrowserItemNoProperty);
    // data browser uses ids 1, 2, 3, ... numTabs per default, so we
    // can pass NULL for the id array
    AddDataBrowserItems(dataBrowser, kDataBrowserNoItem, numTabs, NULL,
						  kDataBrowserItemNoProperty);

    DataBrowserItemID item = curtabidx;
    SetDataBrowserSelectedItems(dataBrowser, 1, &item, kDataBrowserItemsAssign);
}

/*
 * Set the current tab to "nr".  First tab is 1.
 */
    void
gui_mch_set_curtab(int nr)
{
    DataBrowserItemID item = nr;
    SetDataBrowserSelectedItems(dataBrowser, 1, &item, kDataBrowserItemsAssign);

    // TODO: call something like this?: (or restore scroll position, or...)
    RevealDataBrowserItem(dataBrowser, item, kTabsColumn,
						      kDataBrowserRevealOnly);
}

#endif // FEAT_GUI_TABLINE
