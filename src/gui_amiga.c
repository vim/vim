/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *				Amiga GUI support by Michael Nielsen
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include <stdlib.h>
#include <string.h>
#include <exec/types.h>
#include <intuition/intuition.h>
#include <utility/tagitem.h>
#include <graphics/text.h>
#include <graphics/rastport.h>
#include <graphics/layers.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/layers.h>
#include <devices/timer.h>
#include <assert.h>
#include "vim.h"
#include "gui_amiga.h"
#include <math.h>
#include <limits.h>

#ifdef __AROS__
#include <aros/debug.h>
#endif

#include "version.h"

#if defined(FEAT_GUI_AMIGA) || defined(PROTO)

#define KEYUP		76
#define KEYDOWN		77
#define KEYRIGHT	78
#define KEYLEFT		79
#define KEYBACKSPACE	0x41
#define KEYDELETE	0x46
#define KEYINSERT	0x47
#define KEYHOME		0x70
#define KEYEND		0x71
#define KEYWHEELUP	0x7A
#define KEYWHEELDOWN	0x7B

/* When generating prototypes on Unix, these need to be defined */
#ifdef PROTO
# define STRPTR char *
# define BOOL int
# define UBYTE int
#endif

static struct PropInfo Gadget2SInfo = { AUTOKNOB+PROPBORDERLESS+FREEVERT+PROPNEWLOOK, 0, 0, MAXBODY, MAXBODY, };
//static struct Image Image1 = { 0, 0, 10, 397,	0, NULL, 0x0000, 0x0000, NULL };
static struct Gadget propGadget = { NULL, -12, 15, 10, -28,
	GFLG_RELRIGHT+GFLG_RELHEIGHT,
	GACT_RELVERIFY+GACT_RIGHTBORDER+GACT_IMMEDIATE,
	GTYP_PROPGADGET+GTYP_GZZGADGET,
	NULL, NULL,
	NULL, NULL, (APTR)&Gadget2SInfo, NULL, NULL };

static struct timerequest *TimerIO;
static struct MsgPort	  *TimerMP;
static BOOL		   TimerSent;

struct GFXBase		*gfxBase;
struct ExecBase		*execBase;
struct LayersBase	*layersBase;

struct MyColor
{
    WORD pen;
    BOOL alloced;
};

struct MyColor MyColorTable[256];

struct TagItem tags[] =
{
    {WA_Left, 0},
    {WA_Top, 0},
    {WA_Width, 400},
    {WA_Height, 400},
    {WA_Title, (ULONG)VIM_VERSION_SHORT},
    {WA_ScreenTitle, (ULONG)VIM_VERSION_LONG},
    {WA_DragBar, TRUE},			/* enable dragging of the window */
    {WA_DepthGadget, TRUE},		/* enable the depth gadget */
    {WA_CloseGadget, TRUE},		/* enable the close gadget*/
    {WA_SizeGadget, TRUE},		/* enable the size gadget */
    {WA_SizeBBottom, TRUE},		/* sizegadget contained in bottom border */
    {WA_SmartRefresh, TRUE},		/* choose smart refresh, saves us doing a lot of work */
    {WA_ReportMouse, TRUE},		/* Report the position of the mouse */
    {WA_GimmeZeroZero, TRUE},
    {WA_Activate, TRUE},		/* Activate window on startup */
    {WA_Activate, TRUE},		/* Activate window on startup */
    {WA_NoCareRefresh, TRUE},		/* Refresh screen, don't tell us */
    {WA_NewLookMenus, TRUE},		/* use the new options for the menu */
    {WA_AutoAdjust, TRUE},		/* If window is too big for screen adjust size*/
    {WA_NoCareRefresh, TRUE},		/* If window is too big for screen adjust size*/
    {WA_MouseQueue, 1},			/* Limit number of pending mouse movement*/
    {WA_RptQueue, 10},			/* Limit number of pending keystrokes*/
    {WA_IDCMP,				/* IDCMP, what events interest us  */
	IDCMP_NEWSIZE			/* Notify us about size change of window*/
	    |IDCMP_REFRESHWINDOW	/* Notify us when the window needs refreshing */
	    |IDCMP_MOUSEBUTTONS		/* Notify us when the mouse buttons have been used */
	    |IDCMP_MOUSEMOVE		/* Notify us when the mouse is moving */
	    |IDCMP_GADGETDOWN		/* Notify us when a gadget has been selected */
	    |IDCMP_GADGETUP		/* Notify us when a gadget has been released */
	    |IDCMP_MENUPICK		/* Notify us when a menu has been picked */
	    |IDCMP_CLOSEWINDOW		/* Notify us when the user tries to close the window */
	    |IDCMP_VANILLAKEY		/* Notify us about keystrokes */
	    |IDCMP_RAWKEY		/* Notify us when raw key events have been used, ie cursor*/
	    |IDCMP_INTUITICKS		/* Simpler timer for the blink option */
	    |IDCMP_MENUHELP		/* Allow the help key to be used during menu events */
	    |IDCMP_GADGETHELP		/* Allow the help key to be used during gadget events */
	    |IDCMP_INACTIVEWINDOW	/* notify of inactive window */
	    |IDCMP_ACTIVEWINDOW		/* notify of inactive window */
    },
    {TAG_DONE, NULL}
};

#if defined(D)
#undef D
#endif

/*#define D(_msg) fprintf(stderr, "%s\n", _msg)*/

#define D(_A)
#define kprintf(s, ...)

static void AmigaError(const char *string);

void HandleEvent(unsigned long * object);
static UBYTE getrealcolor(guicolor_T i);

static struct NewWindow vimNewWindow =
{
    0, 0,		/* window XY origin relative to TopLeft of screen */
    0, 0,		/* window width and height */
    0, 1,		/* detail and block pens */
    NULL,		/* IDCMP flags */
    NULL,		/* other window flags */
    &propGadget,	/* first gadget in gadget list */
    NULL,		/* custom CHECKMARK imagery */
    "Amiga Vim gui",	/* window title */
    NULL,		/* custom screen pointer */
    NULL,		/* custom bitmap */
    50, 50,		/* minimum width and height */
    (unsigned short)-1, (unsigned short)-1,	/* maximum width and height */
    WBENCHSCREEN	/* destination screen type */
};

static struct
{
    unsigned int  key_sym;
    char_u  vim_code0;
    char_u  vim_code1;
} special_keys[] =
{
    {0,		    0, 0}
};

#if 0
    /* not used? */
    static int
hex_digit(int c)
{
    if (isdigit(c))
	return c - '0';
    c = TOLOWER_ASC(c);
    if (c >= 'a' && c <= 'f')
	return c - 'a' + 10;
    return -1000;
}
#endif

static int characterWidth = -1;
static int characterHeight = -1;
static struct
{
    BOOL	active;
    enum
    {
	CursorOff,
	CursorOn,
	CursorWait
    }		state;
    int		onTime;
    int		offTime;
    int		waitTime;
    int		current;
} cursor =
{
    TRUE,
    CursorWait,
    10,
    10,
    7,
    0
};

enum DrawBoxMode
{
    DB_Filled,
    DB_NotFilled
};

    static void
TextDimensions(void)
{
    struct TextExtent textExt;

    TextExtent(gui.window->RPort, "s", 1, &textExt);

    characterWidth = textExt.te_Width;
    characterHeight = textExt.te_Height;
}

    static int
posWidthCharToPoint(int width)
{
    return (width)*characterWidth;
}

    static int
posHeightCharToPoint(int height)
{
    return (int)(height)*characterHeight;
}

    static int
posWidthPointToChar(int width)
{
    //return (int)floor((float)width/(float)characterWidth)-1;
    return width /characterWidth;
}

    static int
posHeightPointToChar(int height)
{
    //return (int)floor((float)height/(float)characterHeight)-2;
    return height / characterHeight;
}

    static int
widthCharToPoint(int width)
{
    return (width)*(characterWidth);
}

    static int
heightCharToPoint(int height)
{
    return (height)*characterHeight;
}

    static int
widthPointToChar(int width)
{
    return (width)/characterWidth;
}

    static int
heightPointToChar(int height)
{
    return (height)/characterHeight;
}

    static void
refreshBorder(void)
{
    /*WaitBOVP(gui.window->);*/
    RefreshWindowFrame(gui.window);
}

    static void
drawBox(enum DrawBoxMode mode, unsigned short col, unsigned short row, int w, int h, guicolor_T color)
{
    LONG apen = GetAPen(gui.window->RPort);
    LONG x1, y1, x2, y2;

kprintf(" drawbox %d,%d color %d\n", col, row, color);

    SetAPen(gui.window->RPort, getrealcolor(color));

    x1 = posWidthCharToPoint(col);
    y1 = posHeightCharToPoint(row + 1) - h;
    x2 = x1 + w - 1;
    y2 = posHeightCharToPoint(row + 1) - 1;

    switch(mode)
    {
	case DB_Filled:
	    RectFill(gui.window->RPort, x1, y1, x2, y2);
	    break;

	case DB_NotFilled:
	    Move(gui.window->RPort, x1, y1);
	    Draw(gui.window->RPort, x2, y1);
	    Draw(gui.window->RPort, x2, y2);
	    Draw(gui.window->RPort, x1, y2);
	    Draw(gui.window->RPort, x1, y1);
	    break;
    }

    SetAPen(gui.window->RPort, apen);

}

    static enum event
EventHandler(void)
{
    struct IntuiMessage *msg;
    enum event		returnEvent = ev_Ignore;
    int			class, code;
    static int		dragging = 0;
    static int		mouseX, mouseY;
    char_u		string[40];
    BOOL		quit_request = FALSE;

    msg = (struct IntuiMessage *)GetMsg(gui.window->UserPort);

    if (!msg)
    {
	returnEvent = ev_NullEvent;
    }
    else
    {

	class = msg->Class;
	code = msg->Code;

	switch(class)
	{
	    case IDCMP_INTUITICKS:
		/*
		   if (cursor.active)
		   {
		   cursor.current ++;
		   if (cursor.state == CursorOff)
		   {
		   printf("cursor turned on\n");
		   if (cursor.offTime < cursor.current)
		   {
		   gui_undraw_cursor();
		   cursor.state = CursorOn;
		   cursor.current = 0;
		   }
		   }
		   else if (cursor.state == CursorOn)
		   {
		   printf("cursor turned off\n");
		   if (cursor.onTime < cursor.current)
		   {
		   cursor.state = CursorOff;
		   gui_update_cursor(FALSE);
		   cursor.current = 0;
		   }
		   }
		   else if (cursor.state == CursorWait)
		   {
		   printf("cursor turned Wait\n");
		   if (cursor.waitTime < cursor.current)
		   {
		   cursor.state = CursorOn;
		   cursor.current = 0;
		   }
	}
	}
		   else
		   {
		   }
		   returnEvent = ev_IntuiTicks;
		   */
		   break;

	    case IDCMP_MOUSEBUTTONS:
		   {
		       int vim_modifiers=0;
		       D("Mouse button event detected");
		       switch (msg->Qualifier )
		       {
			   case IEQUALIFIER_LALT:
			   case IEQUALIFIER_RALT:
			       D("detected a Alt key");
			       vim_modifiers|=MOUSE_ALT;
			       break;

			   case IEQUALIFIER_LSHIFT:
			   case IEQUALIFIER_RSHIFT:
			       D("detected a Shift key");
			       vim_modifiers|=MOUSE_SHIFT;
			       break;
			   case IEQUALIFIER_CONTROL:
			       D("detected a Control key");
			       vim_modifiers |= MOUSE_CTRL;
			       break;
		       }
		       if (code == SELECTDOWN)
		       {
			   D("Select Down detected\n");
			   dragging = 1;
			   gui_send_mouse_event(MOUSE_LEFT,
				   mouseX = msg->MouseX - gui.window->BorderLeft,
				   mouseY = msg->MouseY - gui.window->BorderTop,
				   FALSE,
				   vim_modifiers);
			   /*gui_start_highlight(HL_ALL);*/
		       }
		       else if (code == SELECTUP)
		       {
			   D("Select UP detected\n");
			   dragging = 0;
			   gui_send_mouse_event(MOUSE_RELEASE,
				   msg->MouseX - gui.window->BorderLeft,
				   msg->MouseY - gui.window->BorderTop,
				   FALSE, vim_modifiers);
			   /*gui_stop_highlight(mask);*/
		       }
		       returnEvent = ev_MouseButtons;
		       break;
		   }
	    case IDCMP_MOUSEMOVE:
		   if ((abs(mouseX-(msg->MouseX - gui.window->BorderLeft)) > characterWidth) ||
		       (abs(mouseY-(msg->MouseY - gui.window->BorderTop))>characterHeight))
		   {
		       int vim_modifiers=0;

		       switch (msg->Qualifier )
		       {
			   case IEQUALIFIER_LALT:
			   case IEQUALIFIER_RALT:
			       D("detected a Alt key");
			       vim_modifiers|=MOUSE_ALT;
			       break;

			   case IEQUALIFIER_LSHIFT:
			   case IEQUALIFIER_RSHIFT:
			       D("detected a Shift key");
			       vim_modifiers|=MOUSE_SHIFT;
			       break;
			   case IEQUALIFIER_CONTROL:
			       D("detected a Control key");
			       vim_modifiers |= MOUSE_CTRL;
			       break;
		       }

		       mouseX = msg->MouseX - gui.window->BorderLeft;
		       mouseY = msg->MouseY - gui.window->BorderTop;
		       if (!dragging)
		       {
			   gui_send_mouse_event(MOUSE_SETPOS, mouseX, mouseY, FALSE, vim_modifiers);
			   break;
		       }
		       else
		       {
			   D("dragging\n");
			   gui_send_mouse_event(MOUSE_DRAG, mouseX, mouseY, FALSE, vim_modifiers);
		       }
		   }
		   returnEvent = ev_MouseMove;
		   break;
	    case IDCMP_VANILLAKEY:
kprintf("===vanillakey %d\n", code);
		   {
		       string[0] = (char_u)code;
		       if (code == CSI)
		       {
			   /* Insert CSI as K_CSI.  Untested! */
			   string[1] = KS_EXTRA;
			   string[2] = (int)KE_CSI;
			   add_to_input_buf(string, 3);
		       }
		       else if (code == 8)
		       {
			    string[0] = CSI;
			    string[1] = 'k';
			    string[2] = 'b';
			    add_to_input_buf(string, 3);
		       }
		       else if (code == 127)
		       {
			    string[0] = CSI;
			    string[1] = 'k';
			    string[2] = 'D';
			    add_to_input_buf(string, 3);
		       }
		       else
		       {
			   int	len = 1;

			   if (input_conv.vc_type != CONV_NONE)
			       len = convert_input(string, 1, sizeof(string));
			   add_to_input_buf(string, len);
		       }
		       returnEvent = ev_KeyStroke;
		       break;

		case IDCMP_RAWKEY:
		       if (msg->Qualifier & IEQUALIFIER_LSHIFT)
		       {
		       }
		       else if (msg->Qualifier & IEQUALIFIER_RSHIFT)
		       {
		       }
		       else if (msg->Qualifier & IEQUALIFIER_CONTROL)
		       {
			   if (code == 33)
			   {
			       trash_input_buf();
			   }
		       }
		       else if (msg->Code == KEYUP)
		       {
			   string[0] = CSI;
			   string[1] = 'k';
			   string[2] = 'u';
			   add_to_input_buf(string, 3);
		       }
		       else if (msg->Code == KEYLEFT)
		       {
			   string[0] = CSI;
			   string[1] = 'k';
			   string[2] = 'l';
			   add_to_input_buf(string, 3);
		       }
		       else if (msg->Code == KEYRIGHT)
		       {
kprintf("## keyright");
			   string[0] = CSI;
			   string[1] = 'k';
			   string[2] = 'r';
			   add_to_input_buf(string, 3);
		       }
		       else if (msg->Code == KEYDOWN)
		       {
			   string[0] = CSI;
			   string[1] = 'k';
			   string[2] = 'd';
			   add_to_input_buf(string, 3);
		       }
		       else if (msg->Code == KEYBACKSPACE)
		       {
			   string[0] = CSI;
			   string[1] = 'k';
			   string[2] = 'b';
			   add_to_input_buf(string, 3);
		       }
		       else if (msg->Code == KEYDELETE)
		       {
			   string[0] = CSI;
			   string[1] = 'k';
			   string[2] = 'D';
			   add_to_input_buf(string, 3);
		       }
		       else if (msg->Code == KEYINSERT)
		       {
			   string[0] = CSI;
			   string[1] = 'k';
			   string[2] = 'I';
			   add_to_input_buf(string, 3);
		       }
		       else if (msg->Code == KEYHOME)
		       {
			   string[0] = CSI;
			   string[1] = 'k';
			   string[2] = 'h';
			   add_to_input_buf(string, 3);
		       }
		       else if (msg->Code == KEYEND)
		       {
			   string[0] = CSI;
			   string[1] = '@';
			   string[2] = '7';
			   add_to_input_buf(string, 3);
		       }
		       else if (msg->Code == KEYWHEELUP)
		       {
			   int vim_modifiers=0;

			   switch (msg->Qualifier )
			   {
			       case IEQUALIFIER_LALT:
			       case IEQUALIFIER_RALT:
				   D("detected a Alt key");
				   vim_modifiers|=MOUSE_ALT;
				   break;

			       case IEQUALIFIER_LSHIFT:
			       case IEQUALIFIER_RSHIFT:
				   D("detected a Shift key");
				   vim_modifiers|=MOUSE_SHIFT;
				   break;
			       case IEQUALIFIER_CONTROL:
				   D("detected a Control key");
				   vim_modifiers |= MOUSE_CTRL;
				   break;
			   }
			   gui_send_mouse_event(MOUSE_4, 0, 1, FALSE, vim_modifiers);

		       }
		       else if (msg->Code == KEYWHEELDOWN)
		       {
			   int vim_modifiers=0;

			   switch (msg->Qualifier )
			   {
			       case IEQUALIFIER_LALT:
			       case IEQUALIFIER_RALT:
				   D("detected a Alt key");
				   vim_modifiers|=MOUSE_ALT;
				   break;

			       case IEQUALIFIER_LSHIFT:
			       case IEQUALIFIER_RSHIFT:
				   D("detected a Shift key");
				   vim_modifiers|=MOUSE_SHIFT;
				   break;
			       case IEQUALIFIER_CONTROL:
				   D("detected a Control key");
				   vim_modifiers |= MOUSE_CTRL;
				   break;
			   }
			   gui_send_mouse_event(MOUSE_5, 0, 1, FALSE, vim_modifiers);
		       }

		       returnEvent = ev_KeyStroke;
		       break;
		   }
	    case IDCMP_MENUVERIFY:
		   returnEvent = ev_MenuVerify;
		   /* Menu verification requested */
		   switch (code)
		   {
		       case MENUWAITING:
			   /*
			    ** It's not for us, the user is accessing another
			    ** programs menu, this is a good time to do some
			    ** cleanup etc
			    */
			   break;
		       case MENUHOT:
			   /*
			    ** It is our menu that is going hot, we have kontrol
			    ** Menu action can be cancelled by
			    ** msg->Code = MENUCANCEL;
			    */
			   break;
		       default:
			   break;
		   }
		   break;
	    case IDCMP_MENUPICK:
		   returnEvent = ev_MenuPick;
		   {
		       /*
			** one of our menu's have been selected, let's find out which
			*/
		       union myMenuItemUnion *item;
		       int menuNumber;

		       menuNumber = code;

		       item = (union myMenuItemUnion *) ItemAddress(gui.menu, menuNumber);


		       if (item)
		       {
			   gui_menu_cb(item->myMenuItem.guiMenu);
		       }
		   }
		   break;
	    case IDCMP_CLOSEWINDOW:
		    quit_request = TRUE;
		    break;

	    case IDCMP_NEWSIZE:
		   {
		       int cx, cy;
		       //cx = widthPointToChar(gui.window->GZZWidth);
		       //cy = heightPointToChar(gui.window->GZZHeight);

		       cx = gui.window->GZZWidth;
		       cy = gui.window->GZZHeight - characterHeight;

		       gui_resize_shell(cx, cy);

		       returnEvent = ev_NewSize;
		       break;
		   }
	    case IDCMP_REFRESHWINDOW:
		   refreshBorder();
		   returnEvent = ev_RefreshWindow;
		   break;
	    case IDCMP_GADGETDOWN:
		   returnEvent = ev_GadgetDown;
		   break;
	    case IDCMP_GADGETUP:
		   returnEvent = ev_GadgetUp;
		   break;
	    case IDCMP_MENUHELP:
		   returnEvent = ev_MenuHelp;
		   break;
	    case IDCMP_GADGETHELP:
		   returnEvent = ev_GadgetHelp;
		   break;
	    case IDCMP_INACTIVEWINDOW:
		   gui.in_focus = FALSE;
		   gui_update_cursor(TRUE, FALSE);
		   break;

	    case IDCMP_ACTIVEWINDOW:
		   gui.in_focus = TRUE;
		   gui_update_cursor(TRUE, FALSE);
		   break;
	    default:
		   break;
	}
	ReplyMsg((struct Message*)msg);
    }

    if (quit_request)
    {
	getout(0); // gui_mch_exit(1);
    }

    return returnEvent;
    /* mouse positin gui.window->MoseY, gui.window->MouseX) */
}

    static int
checkEventHandler(void)
{
    enum event happened;

    do
    {
	happened = EventHandler() ;
    }
    while  (happened != ev_NullEvent);

    return OK;
}

    static int
charEventHandler(int wtime)
{
    enum event happened;
    int rc;

    do
    {
	Wait(1<<gui.window->UserPort->mp_SigBit);

	happened = EventHandler() ;
    }
    while ((happened != ev_IntuiTicks) && (happened != ev_KeyStroke) && (happened != ev_MenuPick) && (happened != ev_MouseMove) &&(happened != ev_MouseButtons) );

    if (happened == ev_KeyStroke || happened == ev_MenuPick)
	rc = OK;
    else
	rc = FAIL;

    return rc;
}


/*
 * add primary menu
 */
    void
gui_mch_add_menu_item(vimmenu_T *menu, int idx)
{
    union myMenuItemUnion *menuItemUnion = NULL;
    struct IntuiText *menutext = NULL;
    vimmenu_T *parent;

    assert(menu != NULL);
    assert(menu->parent != NULL);
    parent = menu->parent;

    /* Don't add menu separator */
    if (menu_is_separator(menu->name))
	return;

    if (parent->menuItemPtr == NULL)
	return;

    /* TODO: use menu->mnemonic and menu->actext */
    menutext = (struct IntuiText *) malloc(sizeof(struct IntuiText));

    SetAttrib(menutext, FrontPen, 3);
    SetAttrib(menutext, BackPen, 1);
    SetAttrib(menutext, DrawMode, COMPLEMENT);
    SetAttrib(menutext, LeftEdge, 0);
    SetAttrib(menutext, TopEdge, 0);
    SetAttrib(menutext, ITextFont, NULL);
    SetAttrib(menutext, NextText, NULL);

    menuItemUnion = malloc(sizeof(*menuItemUnion));

    SetAttrib(&menuItemUnion->menuItem, NextItem, parent->menuItemPtr);
    SetAttrib(&menuItemUnion->menuItem, LeftEdge, 0);
    SetAttrib(&menuItemUnion->menuItem, Width, characterWidth*strlen(menu->dname));
    SetAttrib(&menuItemUnion->menuItem, Height, characterHeight+2);
    SetAttrib(&menuItemUnion->menuItem, Flags, ITEMTEXT+ITEMENABLED+HIGHCOMP);
    SetAttrib(&menuItemUnion->menuItem, MutualExclude, 0);
    SetAttrib(&menuItemUnion->menuItem, ItemFill, (APTR)menutext);
    SetAttrib(&menuItemUnion->menuItem, SelectFill, NULL);
    SetAttrib(&menuItemUnion->menuItem, Command, NULL);
    SetAttrib(&menuItemUnion->menuItem, SubItem, NULL);
    SetAttrib(&menuItemUnion->menuItem, NextSelect, MENUNULL);

    menutext->IText = malloc(strlen(menu->dname) + 1);

    strcpy(menutext->IText, menu->dname);

    menuItemUnion->menuItem.NextItem = NULL;


    if (parent)
    {
	if (!parent->menuItemPtr)
	{
	    D("Adding first subElement");
	    SetAttrib(&menuItemUnion->menuItem, TopEdge, 0);
	    parent->menuPtr->FirstItem = &menuItemUnion->menuItem;
	    parent->menuItemPtr = &menuItemUnion->menuItem;
	}
	else
	{
	    struct MenuItem *tmpMenuItem;
	    tmpMenuItem = parent->menuItemPtr;
	    while (tmpMenuItem->NextItem)
	    {
		tmpMenuItem = tmpMenuItem->NextItem;
	    }
	    tmpMenuItem->NextItem = &menuItemUnion->menuItem;
	    SetAttrib(&menuItemUnion->menuItem, TopEdge, tmpMenuItem->TopEdge+tmpMenuItem->Height);
	}
    }
    menu->menuPtr= NULL;
    menu->menuItemPtr = &menuItemUnion->menuItem;
    menuItemUnion->myMenuItem.guiMenu = menu;
}


    static struct Menu *
getMenu(struct RastPort *rast, int left, STRPTR name)
{
    struct Menu *menu;
    struct TextExtent textExt;

    menu = malloc(sizeof(*menu));
    menu->NextMenu = NULL;
    menu->LeftEdge = left;

    TextExtent(rast, name, strlen(name), &textExt);

    menu->TopEdge = 0;
    menu->Width = textExt.te_Width;
    menu->Height = textExt.te_Height;
    menu->Flags = ITEMTEXT+HIGHCOMP+MENUENABLED;
    menu->MenuName = name;
    menu->FirstItem = NULL;

    return menu;
}

/*
 * add  1st level submenu item
 */
    void
gui_mch_add_menu(vimmenu_T *menu, int idx)
{
    struct Menu	*newMenu;
    int		pos = 0;

    if (!menu_is_menubar(menu->name))
	return;

    menu->menuPtr = newMenu = getMenu(gui.window->RPort, 0, menu->dname);
    menu->menuItemPtr = NULL;
    newMenu->NextMenu = NULL;

    if (!gui.menu)
    {
	D("Adding head menu");
	gui.menu = newMenu ;
    }
    else
    {
	struct Menu *tmpMenu;

	tmpMenu = gui.menu;
	while (tmpMenu->NextMenu)
	    tmpMenu = tmpMenu->NextMenu;
	tmpMenu->NextMenu = newMenu;
	pos = tmpMenu->LeftEdge +
	    TextLength(gui.window->RPort, tmpMenu->MenuName,
		    strlen(tmpMenu->MenuName));
	newMenu->LeftEdge = pos;
    }
}

    void
gui_mch_toggle_tearoffs(enable)
    int		enable;
{
    /* no tearoff menus */
}

    int
gui_mch_set_blinking(long wait, long on, long off)
{
    cursor.waitTime = wait/100;
    cursor.onTime = on/100;
    cursor.offTime = off/100;
    return OK;
}

    void
gui_mch_prepare(int *argc, char **argv)
{
    D("gui_mch_prepare");

    execBase = (struct ExecBase *)OpenLibrary("exec.library", NULL);
    gfxBase = (struct GFXBase *)OpenLibrary("graphics.library", NULL);
    layersBase = (struct LayersBase *)OpenLibrary("layers.library", NULL);

    if (!execBase)
    {
	D("Cannot open exec.library, aborting");
    }
    if (!gfxBase)
    {
	D("Cannot open graphics.library, aborting");
    }
    if (!layersBase)
    {
	D("Cannot open graphics.library, aborting");
    }
    D("gui_mch_prepare done ");
}

    void
atexitDoThis(void)
{
kprintf("atexitdothis###\n");
    gui_mch_exit(-1);
}

/*
 * Check if the GUI can be started.  Called before gvimrc is sourced.
 * Return OK or FAIL.
 */
    int
gui_mch_init_check(void)
{
    if (execBase && gfxBase && layersBase)
	return OK;
    return FAIL;
}

    int
gui_mch_init(void)
{
    int returnCode = FAIL; /* assume failure*/

    TimerMP = CreateMsgPort();
    if (!TimerMP) return FAIL;

    TimerIO = (struct timerequest *)CreateIORequest(TimerMP, sizeof(*TimerIO));
    if (!TimerIO) return FAIL;

    if (OpenDevice("timer.device", UNIT_VBLANK, &TimerIO->tr_node, 0)) return FAIL;

    gui.window = OpenWindowTagList(&vimNewWindow, tags);
    if (gui.window)
    {
	gui.in_use = TRUE;
	gui.in_focus=TRUE;
	gui.norm_pixel = gui.def_norm_pixel = 1;
	gui.back_pixel = gui.def_back_pixel = 0;

	set_normal_colors();
	gui_check_colors();

	SetDrMd(gui.window->RPort, JAM2);
	gui_mch_set_colors(gui.norm_pixel, gui.back_pixel);

	atexit(atexitDoThis);

	TextDimensions();
	returnCode = OK; /* we've had success */
	if (gui_win_x != -1 && gui_win_y != -1)
	    gui_mch_set_winpos(gui_win_x, gui_win_y);

	gui_mch_clear_all();

    }
    gui.menu = NULL;

    return returnCode;
}

    void
gui_mch_new_colors(void)
{
kprintf("### gui_mch_new_colors\n");
    SetAPen(gui.window->RPort, getrealcolor(gui.norm_pixel));
    SetBPen(gui.window->RPort, getrealcolor(gui.back_pixel));

    D("gui_mch_new_colors");
}

    int
gui_mch_open(void)
{
    D("gui_mch_open");

    highlight_gui_started();
    return OK;
}

    void
gui_mch_exit(int returnCode)
{
kprintf("###gui_mch_exit\n");
    D("****gui_mch_exit");

    if (TimerSent)
    {
	if (!CheckIO(&TimerIO->tr_node)) AbortIO(&TimerIO->tr_node);
	WaitIO(&TimerIO->tr_node);
	TimerSent = FALSE;
    }

    if (TimerIO)
    {
	CloseDevice(&TimerIO->tr_node);
	DeleteIORequest(&TimerIO->tr_node);
	TimerIO = NULL;
    }

    if (TimerMP)
    {
	DeleteMsgPort(TimerMP);
	TimerMP = NULL;
    }

    if (gui.window)
    {
	int i;

	for(i = 0; i < sizeof(MyColorTable) / sizeof(MyColorTable[0]); i++)
	{
	    if (MyColorTable[i].alloced)
	    {
		ReleasePen(gui.window->WScreen->ViewPort.ColorMap, MyColorTable[i].pen);
		MyColorTable[i].alloced = FALSE;
	    }
	}

	D("Closeing window ");
	CloseWindow(gui.window);
	CloseLibrary((struct Library*)execBase);
	CloseLibrary((struct Library*)gfxBase);
	gui.window = NULL;
	gui.in_use = FALSE;
	//getout(1);
    }
}

/*
 * Get the position of the top left corner of the window.
 */
    int
gui_mch_get_winpos(int *x, int *y)
{
    if (gui.window)
    {
	*x = gui.window->LeftEdge;
	*y = gui.window->TopEdge;
    }
    else
    {
	return FAIL;
    }

    return OK;
}

/*
 * Set the position of the top left corner of the window to the given
 * coordinates.
 */
    void
gui_mch_set_winpos(int x, int y)
{
    if (gui.window)
    {
	ChangeWindowBox(gui.window, x, y, gui.window->Width, gui.window->Height);
    }
}

    void
gui_mch_set_shellsize(int width, int height,
	int min_width, int min_height, int base_width, int base_height)
{
    D("gui_mch_set_shellsize");

    ChangeWindowBox(gui.window, gui.window->LeftEdge,
	    gui.window->TopEdge, widthCharToPoint(width) + gui.window->BorderLeft + gui.window->BorderRight,
	    heightCharToPoint(height) + gui.window->BorderTop + gui.window->BorderBottom);
    checkEventHandler();
}

    void
gui_mch_get_screen_dimensions(int *screen_w, int *screen_h)
{
//    *screen_w = widthPointToChar(gui.window->GZZWidth);
//    *screen_h = heightPointToChar(gui.window->GZZHeight);
    *screen_w = gui.window->GZZWidth;
    *screen_h = gui.window->GZZHeight - characterHeight;


kprintf("=== get_screen_dimensions: screen %d,%d character %d,%d  console %d,%d\n",
gui.window->GZZWidth,
gui.window->GZZHeight,
characterWidth,
characterHeight,
*screen_w,
*screen_h);

}

    void
gui_mch_set_text_area_pos(int x, int y, int w, int h)
{
    D("gui_mch_set_text_area_pos");
}

    void
gui_mch_enable_scrollbar(scrollbar_T *sb, int flag)
{
    /* done by default */
    /* TODO: disable scrollbar when it's too small */
}

    void
gui_mch_set_scrollbar_thumb(scrollbar_T *sb, long val, long size, long max)
{
    ULONG total = max;
    ULONG visible = size;
    ULONG top = val;
    ULONG hidden;
    ULONG overlap = 0;
    UWORD body, pot;

kprintf("__set_scrollbar_thumb val %d  size %d  max %d\n", val, size, max);

    if (total > visible)
	hidden = total - visible;
    else
	hidden = 0;

    if (top > hidden)
	top = hidden;

    body = (hidden > 0) ?
	    (UWORD)(((ULONG)(visible - overlap) * MAXBODY) / (total - overlap)) :
	    MAXBODY;

    pot  = (hidden > 0) ? (UWORD)(((ULONG) top * MAXPOT) / hidden) : 0;

kprintf("__pot %x  body %x\n", pot, body);

    NewModifyProp(&propGadget, gui.window, NULL,
		  Gadget2SInfo.Flags,
		  MAXPOT, pot,
		  MAXBODY, body,
		  1);
    return;

}

    void
gui_mch_set_scrollbar_pos(scrollbar_T *sb, int x, int y, int w, int h)
{
    D("gui_mch_set_scrollbar_pos");
    /*NewModifyProp(&propGadget, gui.window, NULL, MAXPOT, MAXPOT/sb->max*y, MAXPOT, MAXBODY/sb->max/sb->size, 1);*/
}

    void
gui_mch_create_scrollbar(scrollbar_T *sb, int orient)
{
    /* this is done by default */
}

#if defined(FEAT_WINDOWS) || defined(PROTO)
    void
gui_mch_destroy_scrollbar(scrollbar_T *sb)
{
    /* this is done by default */
}
#endif

int gui_mch_init_font(char_u *font_name, int fontset)
{
    /*D("gui_mch_init_font");*/

    gui.char_width = characterWidth;
    gui.char_height = characterHeight;
    gui.char_ascent = gui.window->RPort->TxBaseline;

    return OK;
}

    int
gui_mch_adjust_charsize()
{
    return FAIL;
}

    GuiFont
gui_mch_get_font( char_u *name, int giveErrorIfMissing)
{
    /*D("gui_mch_get_font");*/
    return NULL;
}

    void
gui_mch_set_font(GuiFont font)
{
    /*D("gui_mch_set_font");*/
}

#if 0 /* not used */
    int
gui_mch_same_font(GuiFont f1, GuiFont f2)
{
    D("gui_mch_same_font");
}
#endif

    void
gui_mch_free_font(GuiFont font)
{
    if (font)
	D("gui_mch_free_font");
}

#define RGB(a, b, c) ((a && 0xff) * 0x10000 + (b * 0xff) * 0x100 + (c & 0xff))

/*
 * Get color handle for color "name".
 * Return INVALCOLOR when not possible.
 */

    typedef struct guicolor_tTable
    {
	char	    *name;
	unsigned long    color;
	UBYTE		red;
	UBYTE		green;
	UBYTE		blue;
    } guicolor_tTable;

    static guicolor_tTable table[] =
    {
	{"Grey",	0, 190,190,190},
	{"Black",	1, 0, 0, 0},
	{"DarkBlue",	2, 0, 0, 139},
	{"DarkGreen",	3, 0, 100, 0},
	{"DarkCyan",	4, 0, 139, 139},
	{"DarkRed",	5, 139, 0, 0},
	{"DarkMagenta",	6, 139, 0, 139},
	{"Brown",	7, 165, 42, 42},
	{"Gray",	8, 190, 190, 190},
	{"Grey",	9, 190, 190, 190},
	{"LightGray",	10, 211, 211, 211},
	{"LightGrey",	11, 211, 211, 211},
	{"DarkGray",	12, 169, 169, 169},
	{"DarkGrey",	13, 169, 169, 169},
	{"Blue",	14, 0, 0, 255},
	{"LightBlue",	15, 173, 216, 230},
	{"Green",	16, 0, 255, 0},
	{"LightGreen",	17, 144, 238, 144},
	{"Cyan",	18, 0, 255, 255},
	{"LightCyan",	19, 224, 255, 255},
	{"Red",		20, 255, 0, 0},
	{"LightRed",	21, 255, 0, 0}, /*?*/
	{"Magenta",	22, 255, 0, 255},
	{"LightMagenta",23, 255, 0, 255}, /*?*/
	{"Yellow",	24, 255, 255, 0},
	{"LightYellow",	25, 255, 255, 224},	/* TODO: add DarkYellow */
	{"White",	26, 255, 255, 255},
	{"SeaGreen",	27, 46, 139, 87},
	{"Orange",	28, 255, 165, 0},
	{"Purple",	30, 160, 32, 240},
	{"SlateBlue",	31, 106, 90, 205},
	{"grey90",	32, 229, 229, 229},
	{"grey95",	33, 242, 242, 242},
	{"grey80",	34, 204, 204, 204},
	{NULL, NULL},
    };

    guicolor_T
gui_mch_get_color(char_u *name)
{

    guicolor_T color = INVALCOLOR;

    int i;

    for (i = 0; table[i].name != NULL;i++)
    {
	if (stricmp(name, table[i].name) == 0)
	{
	    //color = table[i].color;
	    color = i;
	}
    }

#if 0
    if (color == INVALCOLOR)
    {
	char *looky = NULL;

	color = strtol((char*)name, &looky, 10);
	if (*looky != NUL)
	    color = INVALCOLOR;
    }
#endif

    kprintf("gui_mch_get_color[%s] = %s\n", name, table[color].name);

    return color;
}

static UBYTE getrealcolor(guicolor_T i)
{
    if (!MyColorTable[i].alloced)
    {
	MyColorTable[i].pen = ObtainBestPen(gui.window->WScreen->ViewPort.ColorMap,
					    table[i].red * 0x01010101,
					    table[i].green * 0x01010101,
					    table[i].blue * 0x01010101,
					    OBP_FailIfBad, FALSE,
					    OBP_Precision, PRECISION_GUI,
					    TAG_DONE);
	if (MyColorTable[i].pen != -1)
	{
	    MyColorTable[i].alloced = TRUE;
	}
    }

    return MyColorTable[i].pen;
}


    void
gui_mch_set_colors(guicolor_T fg, guicolor_T bg)
{
#if 0
    if (fg == 0)
    {
	fg = 1;
    }
#endif
    SetABPenDrMd(gui.window->RPort, getrealcolor(fg), getrealcolor(bg), JAM2);

kprintf("gui_mch_set_colors %s,%s\n", table[fg].name, table[bg].name);
}

    void
gui_mch_set_fg_color(guicolor_T color)
{
#if 0
    if (color == 0)
    {
	color = 1; /* vim sends 0 as default color which is ALWAYS the
		      background on the amiga scrolling with colours as the
		      background is a very bad idea on slow machines*/
    }
#endif
    SetAPen(gui.window->RPort, getrealcolor(color));
    SetDrMd(gui.window->RPort, JAM2);

kprintf("gui_mch_set_fg_color %s\n", table[color].name);

}

    void
gui_mch_set_bg_color(guicolor_T color)
{
    SetBPen(gui.window->RPort, getrealcolor(color));
kprintf("gui_mch_set_bg_color %s\n", table[color].name);

}

    void
gui_mch_draw_string(int row, int col, char_u *s, int len, int flags)
{
#if 1
    char tempstring[300];

    memcpy(tempstring, s, len);
    tempstring[len] = '\0';

    kprintf("gui_mch_draw_string(%s) flags %x\n", tempstring, flags);
#endif

    if (flags & DRAW_TRANSP)
    {
	SetDrMd(gui.window->RPort, JAM1);
	Move(gui.window->RPort, posWidthCharToPoint(col), posHeightCharToPoint(row) + gui.window->RPort->TxBaseline);
	Text(gui.window->RPort, s, len);
    }
    else
    {
	SetDrMd(gui.window->RPort, JAM2);
	Move(gui.window->RPort, posWidthCharToPoint(col), posHeightCharToPoint(row) + gui.window->RPort->TxBaseline);
	Text(gui.window->RPort, s, len);
    }

    if (flags & DRAW_BOLD)
    {
	SetDrMd(gui.window->RPort, JAM1);
	Move(gui.window->RPort, posWidthCharToPoint(col)+1, posHeightCharToPoint(row) + gui.window->RPort->TxBaseline);
	Text(gui.window->RPort, s, len);
    }

    if (flags & DRAW_UNDERL)
    {
	Move(gui.window->RPort, posWidthCharToPoint(col), posHeightCharToPoint(row + 1) - 1);
	Draw(gui.window->RPort, posWidthCharToPoint(col+len) - 1, posHeightCharToPoint(row + 1) - 1);
    }

    SetDrMd(gui.window->RPort, JAM2);
}

    int
gui_mch_haskey(char_u *name)
{
    int i;

    D("gui_mch_haskey");

    for (i = 0; special_keys[i].vim_code1 != NUL; i++)
	if (name[0] == special_keys[i].vim_code0 &&
		name[1] == special_keys[i].vim_code1)
	    return OK;
    return FAIL;
}

    void
gui_mch_beep(void)
{
    D("gui_mch_beep");
}

    void
gui_mch_flash(int msec)
{
    D("gui_mch_flash");

    SetDrMd(gui.window->RPort, COMPLEMENT);
    RectFill(gui.window->RPort, 0, 0, gui.window->GZZWidth - 1, gui.window->GZZHeight - 1);
    Delay(msec * 50 / 1000);
    RectFill(gui.window->RPort, 0, 0, gui.window->GZZWidth - 1, gui.window->GZZHeight - 1);
    SetDrMd(gui.window->RPort, JAM2);
}

    void
gui_mch_invert_rectangle( int r, int c, int nr, int nc)
{
    printf("gui_mch_invert_rectangle %d %d %d %d\n", r, c, nr, nc);
}

    void
gui_mch_iconify(void)
{
    D("gui_mch_iconify");
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Bring the Vim window to the foreground.
 */
    void
gui_mch_set_foreground()
{
    WindowToFront(gui.window);
    D("gui_mch_set_foreground");
}
#endif

    void
gui_mch_settitle(char_u  *title, char_u  *icon)
{
    SetWindowTitles(gui.window, title, (STRPTR)~0);
    D("gui_mch_settitle");
}

    void
gui_mch_stop_blink(void)
{
    gui_undraw_cursor();
    D("gui_mch_stop_blink");
}

    void
gui_mch_start_blink(void)
{
    gui_update_cursor(FALSE, FALSE);
    D("gui_mch_start_blink");
}

    void
gui_mch_draw_hollow_cursor(guicolor_T color)
{
    drawBox(DB_NotFilled, gui.col, gui.row, characterWidth, characterHeight, color);
}

    void
gui_mch_draw_part_cursor( int w, int h, guicolor_T color)
{
    D("gui_mch_part_cursor");
    drawBox(DB_Filled, gui.col, gui.row, w, h, color);
}

    void
gui_mch_update(void)
{
    checkEventHandler();
    return ;
}

    int
gui_mch_wait_for_chars(int wtime)
{
    ULONG timermask = 1L << TimerMP->mp_SigBit;
    ULONG winmask = 1L << gui.window->UserPort->mp_SigBit;
    int retval = FAIL;

    kprintf("========== gui_mch_wait_for_chars %d\n", wtime);

    if (wtime == -1) wtime = 1000000000;
    if (wtime < 20) wtime = 20;

    SetSignal(0, timermask);
    TimerIO->tr_node.io_Command = TR_ADDREQUEST;
    TimerIO->tr_time.tv_secs = wtime / 1000;
    TimerIO->tr_time.tv_micro = (wtime % 1000) * 1000;
    SendIO(&TimerIO->tr_node);
    TimerSent = TRUE;

    for(;;)
    {
	ULONG sigs = Wait(winmask | timermask);

	if (sigs & winmask)
	{
	    checkEventHandler();
	    if (!vim_is_input_buf_empty())
	    {
		retval = OK;
		if (!CheckIO(&TimerIO->tr_node)) AbortIO(&TimerIO->tr_node);
		WaitIO(&TimerIO->tr_node);
		TimerSent = FALSE;
		break;
	    }
	}

	if (sigs & timermask)
	{
	    struct Message *msg;

	    if ((msg = GetMsg(TimerMP)))
	    {
		ReplyMsg(msg);
		TimerSent = FALSE;
		retval = FAIL;
		break;
	    }
	}
    }

    return retval;

//    assert(wtime != 0);
//    return charEventHandler(wtime);
}

    void
gui_mch_flush(void)
{
}

    void
gui_mch_clear_block(int row1, int col1, int row2, int col2)
{
    UBYTE apen = GetAPen(gui.window->RPort);

    SetAPen(gui.window->RPort, getrealcolor(gui.back_pixel));
    RectFill(gui.window->RPort,
	     posWidthCharToPoint(col1),
	     posHeightCharToPoint(row1),
	     posWidthCharToPoint(col2 + 1) - 1,
	     posHeightCharToPoint(row2 + 1) - 1);
    SetAPen(gui.window->RPort, apen);

}

    void
gui_mch_clear_all(void)
{
    SetRast(gui.window->RPort, GetBPen(gui.window->RPort));
    refreshBorder();
    D("gui_mch_clear_all");
}

    void
gui_mch_delete_lines(int row, int num_lines)
{
    ScrollWindowRaster(gui.window,
	    0,
	    characterHeight * num_lines,
	    posWidthCharToPoint(gui.scroll_region_left),
	    posHeightCharToPoint(row),
	    posWidthCharToPoint(gui.scroll_region_right + 1) - 1,
	    posHeightCharToPoint(gui.scroll_region_bot + 1) - 1);

    gui_clear_block(gui.scroll_region_bot - num_lines + 1,
		    gui.scroll_region_left,
		    gui.scroll_region_bot,
		    gui.scroll_region_right);

}

    void
gui_mch_insert_lines(int row, int num_lines)
{
     ScrollWindowRaster(gui.window,
	    0,
	    -characterHeight*num_lines,
	    posWidthCharToPoint(gui.scroll_region_left),
	    posHeightCharToPoint(row),
	    posWidthCharToPoint(gui.scroll_region_right + 1) - 1,
	    posHeightCharToPoint(gui.scroll_region_bot +1 ) - 1);

    gui_clear_block(row, gui.scroll_region_left,
		    row + num_lines - 1, gui.scroll_region_right);

}

    void
gui_mch_enable_menu(int flag)
{
    D("gui_mch_enable_menu");
}

    void
gui_mch_set_menu_pos(int x, int y, int w, int h)
{
    D("gui_mch_set_menu_pos");
}

    void
gui_mch_destroy_menu(vimmenu_T *menu)
{
    D("gui_mch_destroy_menu");
    ClearMenuStrip(gui.window);
}

    void
gui_mch_menu_grey(vimmenu_T *menu, int grey)
{
    D("gui_mch_menu_grey");
}

    void
gui_mch_menu_hidden(vimmenu_T *menu, int hidden)
{
    D("gui_mch_menu_hidden");
    ClearMenuStrip(gui.window);
}

    void
gui_mch_draw_menubar(void)
{
    D("gui_mch_draw_menubar");
    SetMenuStrip(gui.window, gui.menu);
}

    static void
AmigaError(const char *string)
{
    static struct IntuiText pos = { 3, 0, JAM2, 17, 5, NULL, "Cancel", NULL} ;
    static struct IntuiText neg = { 3, 0, JAM2, 17, 5, NULL, "Cancel", NULL} ;
    static struct IntuiText message = { 3, 0, JAM2, 17, 5, NULL, NULL, NULL} ;
    static char *strptr = 0;

    if (strptr)
	free(strptr);
    strptr = malloc(strlen(string)+1);

    message.IText = strptr;
    strcpy(strptr, string);

    AutoRequest(NULL, &message, &pos, &neg, 0, 0, 300, 300);
}

    int
clip_mch_own_selection(VimClipboard *cbd)
{
    D("clib_mch_own_selection");
    return OK;
}

    void
mch_setmouse(int  on)
{
}

/*
 * Get current y mouse coordinate in text window.
 * Return -1 when unknown.
 */
    int
gui_mch_get_mouse_x()
{
    return gui.window->GZZMouseX;
}

    int
gui_mch_get_mouse_y()
{
    return gui.window->GZZMouseY;
}

    void
gui_mch_setmouse(x, y)
    int		x;
    int		y;
{
    /* TODO */
}

    void
gui_mch_show_popupmenu(vimmenu_T *menu)
{
    /* TODO */
}

    void
clip_mch_lose_selection(VimClipboard *cbd)
{
    D("clip_mch_lose_selecction");
}

    void
clip_mch_request_selection(VimClipboard *cbd)
{
    D("clip_mch_requst_selection");
}

    void
clip_mch_set_selection(VimClipboard *cbd)
{
}

    long_u
gui_mch_get_rgb(guicolor_T pixel)
{
    ULONG coltable[3], color;

    GetRGB32(gui.window->WScreen->ViewPort.ColorMap,
	     getrealcolor(pixel),
	     1,
	     coltable);

    color = ((coltable[0] & 0xFF000000) >> 8) |
	    ((coltable[1] & 0xFF000000) >> 16) |
	    ((coltable[2] & 0xFF000000) >> 24);

    return color;
}

#endif /* USE_AMIGA_GUI*/
