/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved			by Bram Moolenaar
 *			    BeBox GUI support Copyright 1998 by Olaf Seibert.
 *			    All Rights Reserved.
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 *
 * BeOS GUI.
 *
 * GUI support for the Buzzword Enhanced Operating System.
 *
 * Ported to R4 by Richard Offer <richard@whitequeen.com> Jul 99
 *
 */

/*
 * Structure of the BeOS GUI code:
 *
 * There are 3 threads.
 * 1. The initial thread. In gui_mch_prepare() this gets to run the
 *    BApplication message loop. But before it starts doing that,
 *    it creates thread 2:
 * 2. The main() thread. This thread is created in gui_mch_prepare()
 *    and its purpose in life is to call main(argc, argv) again.
 *    This thread is doing the bulk of the work.
 * 3. Sooner or later, a window is opened by the main() thread. This
 *    causes a second message loop to be created: the window thread.
 *
 * == alternatively ===
 *
 * #if RUN_BAPPLICATION_IN_NEW_THREAD...
 *
 * 1. The initial thread. In gui_mch_prepare() this gets to spawn
 *    thread 2. After doing that, it returns to main() to do the
 *    bulk of the work, being the main() thread.
 * 2. Runs the BApplication.
 * 3. The window thread, just like in the first case.
 *
 * This second alternative is cleaner from Vim's viewpoint. However,
 * the BeBook seems to assume everywhere that the BApplication *must*
 * run in the initial thread. So perhaps doing otherwise is very wrong.
 *
 * However, from a B_SINGLE_LAUNCH viewpoint, the first is better.
 * If Vim is marked "Single Launch" in its application resources,
 * and a file is dropped on the Vim icon, and another Vim is already
 * running, the file is passed on to the earlier Vim. This happens
 * in BApplication::Run(). So we want Vim to terminate if
 * BApplication::Run() terminates. (See the BeBook, on BApplication.
 * However, it seems that the second copy of Vim isn't even started
 * in this case... which is for the better since I wouldn't know how
 * to detect this case.)
 *
 * Communication between these threads occurs mostly by translating
 * BMessages that come in and posting an appropriate translation on
 * the VDCMP (Vim Direct Communication Message Port). Therefore the
 * actions required for keypresses and window resizes, etc, are mostly
 * performed in the main() thread.
 *
 * A notable exception to this is the Draw() event. The redrawing of
 * the window contents is performed asynchronously from the window
 * thread. To make this work correctly, a locking protocol is used when
 * any thread is accessing the essential variables that are used by
 * the window thread.
 *
 * This locking protocol consists of locking Vim's window. This is both
 * convenient and necessary.
 */
extern "C" {

#define new		xxx_new_xxx

#include <float.h>
#include <assert.h>
#include "vim.h"
#include "globals.h"
#include "proto.h"
#include "option.h"

#undef new

}	/* extern "C" */

/* ---------------- start of header part ---------------- */

#include <be/app/MessageQueue.h>
#include <be/app/Clipboard.h>
#include <be/kernel/OS.h>
#include <be/support/Beep.h>
#include <be/interface/View.h>
#include <be/interface/Window.h>
#include <be/interface/MenuBar.h>
#include <be/interface/MenuItem.h>
#include <be/interface/ScrollBar.h>
#include <be/interface/Region.h>
#include <be/interface/Screen.h>
#include <be/storage/Path.h>
#include <be/storage/Directory.h>
#include <be/storage/Entry.h>
#include <be/app/Application.h>
#include <be/support/Debug.h>

/*
 * The macro B_PIXEL_ALIGNMENT shows us which version
 * of the header files we're using.
 */
#if defined(B_PIXEL_ALIGNMENT)
#define HAVE_R3_OR_LATER    1
#else
#define HAVE_R3_OR_LATER    0
#endif

class VimApp;
class VimFormView;
class VimTextAreaView;
class VimWindow;

extern key_map *keyMap;
extern char *keyMapChars;

extern int main(int argc, char **argv);

#ifndef B_MAX_PORT_COUNT
#define B_MAX_PORT_COUNT    100
#endif

/*
 * VimApp seems comparable to the X "vimShell"
 */
class VimApp: public BApplication
{
    typedef BApplication Inherited;
public:
    VimApp(const char *appsig);
    ~VimApp();

    // callbacks:
#if 0
    virtual void DispatchMessage(BMessage *m, BHandler *h)
    {
	m->PrintToStream();
	Inherited::DispatchMessage(m, h);
    }
#endif
    virtual void ReadyToRun();
    virtual void ArgvReceived(int32 argc, char **argv);
    virtual void RefsReceived(BMessage *m);
    virtual bool QuitRequested();

    static void SendRefs(BMessage *m, bool changedir);
private:
};

class VimWindow: public BWindow
{
    typedef BWindow Inherited;
public:
    VimWindow();
    ~VimWindow();

    virtual void DispatchMessage(BMessage *m, BHandler *h);
    virtual void WindowActivated(bool active);
    virtual bool QuitRequested();

    VimFormView		*formView;

private:
    void init();

};

class VimFormView: public BView
{
    typedef BView Inherited;
public:
    VimFormView(BRect frame);
    ~VimFormView();

    // callbacks:
    virtual void AllAttached();
    virtual void FrameResized(float new_width, float new_height);

#define MENUBAR_MARGIN	1
    float MenuHeight() const
	{ return menuBar ? menuBar->Frame().Height() + MENUBAR_MARGIN: 0; }
    BMenuBar *MenuBar() const
	{ return menuBar; }

private:
    void init(BRect);

    BMenuBar		*menuBar;
    VimTextAreaView	*textArea;
};

class VimTextAreaView: public BView
{
    typedef BView Inherited;
public:
    VimTextAreaView(BRect frame);
    ~VimTextAreaView();

    // callbacks:
    virtual void Draw(BRect updateRect);
    virtual void KeyDown(const char *bytes, int32 numBytes);
    virtual void MouseDown(BPoint point);
    virtual void MouseUp(BPoint point);
    virtual void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
    virtual void MessageReceived(BMessage *m);

    // own functions:
    int mchInitFont(char_u *name);
    void mchDrawString(int row, int col, char_u *s, int len, int flags);
    void mchClearBlock(int row1, int col1, int row2, int col2);
    void mchClearAll();
    void mchDeleteLines(int row, int num_lines);
    void mchInsertLines(int row, int num_lines);

    static void guiSendMouseEvent(int button, int x, int y, int repeated_click, int_u modifiers);
    static void guiBlankMouse(bool should_hide);
    static int_u mouseModifiersToVim(int32 beModifiers);

    int32 mouseDragEventCount;

private:
    void init(BRect);

    int_u	    vimMouseButton;
    int_u	    vimMouseModifiers;
};

class VimScrollBar: public BScrollBar
{
    typedef BScrollBar Inherited;
public:
    VimScrollBar(scrollbar_T *gsb, orientation posture);
    ~VimScrollBar();

    virtual void ValueChanged(float newValue);
    virtual void MouseUp(BPoint where);
    void SetValue(float newval);
    scrollbar_T *getGsb()
	{ return gsb; }

    int32	    scrollEventCount;

private:
    scrollbar_T *gsb;
    float	ignoreValue;
};


/*
 * For caching the fonts that are used;
 * Vim seems rather sloppy in this regard.
 */
class VimFont: public BFont
{
    typedef BFont Inherited;
public:
    VimFont();
    VimFont(const VimFont *rhs);
    VimFont(const BFont *rhs);
    VimFont(const VimFont &rhs);
    ~VimFont();

    VimFont *next;
    int refcount;
    char_u *name;

private:
    void init();
};

/* ---------------- end of GUI classes ---------------- */

struct MainArgs {
    int		 argc;
    char	**argv;
};

/*
 * These messages are copied through the VDCMP.
 * Therefore they ought not to have anything fancy.
 * They must be of POD type (Plain Old Data)
 * as the C++ standard calls them.
 */

#define	KEY_MSG_BUFSIZ	7
#if KEY_MSG_BUFSIZ < MAX_KEY_CODE_LEN
#error Increase KEY_MSG_BUFSIZ!
#endif

struct VimKeyMsg {
    char_u	length;
    char_u	chars[KEY_MSG_BUFSIZ];	/* contains Vim encoding */
};

struct VimResizeMsg {
    int		width;
    int		height;
};

struct VimScrollBarMsg {
    VimScrollBar *sb;
    long	value;
    int		stillDragging;
};

struct VimMenuMsg {
    vimmenu_T	*guiMenu;
};

struct VimMouseMsg {
    int		button;
    int		x;
    int		y;
    int		repeated_click;
    int_u	modifiers;
};

struct VimFocusMsg {
    bool	active;
};

struct VimRefsMsg {
    BMessage   *message;
    bool	changedir;
};

struct VimMsg {
    enum VimMsgType {
	Key, Resize, ScrollBar, Menu, Mouse, Focus, Refs
    };

    union {
	struct VimKeyMsg	Key;
	struct VimResizeMsg	NewSize;
	struct VimScrollBarMsg	Scroll;
	struct VimMenuMsg	Menu;
	struct VimMouseMsg	Mouse;
	struct VimFocusMsg	Focus;
	struct VimRefsMsg	Refs;
    } u;
};

#define RGB(r, g, b)	((char_u)(r) << 16 | (char_u)(g) << 8 | (char_u)(b) << 0)
#define GUI_TO_RGB(g)	{ (g) >> 16, (g) >> 8, (g) >> 0, 255 }

/* ---------------- end of header part ---------------- */

static struct specialkey
{
    uint16  BeKeys;
#define KEY(a,b)	((a)<<8|(b))
#define K(a)		KEY(0,a)		    // for ASCII codes
#define F(b)		KEY(1,b)		    // for scancodes
    char_u  vim_code0;
    char_u  vim_code1;
} special_keys[] =
{
    {K(B_UP_ARROW),	    'k', 'u'},
    {K(B_DOWN_ARROW),	    'k', 'd'},
    {K(B_LEFT_ARROW),	    'k', 'l'},
    {K(B_RIGHT_ARROW),	    'k', 'r'},
    {K(B_BACKSPACE),	    'k', 'b'},
    {K(B_INSERT),	    'k', 'I'},
    {K(B_DELETE),	    'k', 'D'},
    {K(B_HOME),		    'k', 'h'},
    {K(B_END),		    '@', '7'},
    {K(B_PAGE_UP),	    'k', 'P'},	    /* XK_Prior */
    {K(B_PAGE_DOWN),	    'k', 'N'},	    /* XK_Next, */

#define FIRST_FUNCTION_KEY  11
    {F(B_F1_KEY),	    'k', '1'},
    {F(B_F2_KEY),	    'k', '2'},
    {F(B_F3_KEY),	    'k', '3'},
    {F(B_F4_KEY),	    'k', '4'},
    {F(B_F5_KEY),	    'k', '5'},
    {F(B_F6_KEY),	    'k', '6'},
    {F(B_F7_KEY),	    'k', '7'},
    {F(B_F8_KEY),	    'k', '8'},
    {F(B_F9_KEY),	    'k', '9'},
    {F(B_F10_KEY),	    'k', ';'},

    {F(B_F11_KEY),	    'F', '1'},
    {F(B_F12_KEY),	    'F', '2'},
//  {XK_F13,		    'F', '3'},		/* would be print screen/ */
						/* sysreq */
    {F(0x0F),		    'F', '4'},		/* scroll lock */
    {F(0x10),		    'F', '5'},		/* pause/break */
//  {XK_F16,	    'F', '6'},
//  {XK_F17,	    'F', '7'},
//  {XK_F18,	    'F', '8'},
//  {XK_F19,	    'F', '9'},
//  {XK_F20,	    'F', 'A'},
//
//  {XK_F21,	    'F', 'B'},
//  {XK_F22,	    'F', 'C'},
//  {XK_F23,	    'F', 'D'},
//  {XK_F24,	    'F', 'E'},
//  {XK_F25,	    'F', 'F'},
//  {XK_F26,	    'F', 'G'},
//  {XK_F27,	    'F', 'H'},
//  {XK_F28,	    'F', 'I'},
//  {XK_F29,	    'F', 'J'},
//  {XK_F30,	    'F', 'K'},
//
//  {XK_F31,	    'F', 'L'},
//  {XK_F32,	    'F', 'M'},
//  {XK_F33,	    'F', 'N'},
//  {XK_F34,	    'F', 'O'},
//  {XK_F35,	    'F', 'P'},	    /* keysymdef.h defines up to F35 */

//  {XK_Help,	    '%', '1'},	    /* XK_Help */
    {F(B_PRINT_KEY),	    '%', '9'},

#if 0
    /* Keypad keys: */
    {F(0x48),	    'k', 'l'},	    /* XK_KP_Left */
    {F(0x4A),	    'k', 'r'},	    /* XK_KP_Right */
    {F(0x38),	    'k', 'u'},	    /* XK_KP_Up */
    {F(0x59),	    'k', 'd'},	    /* XK_KP_Down */
    {F(0x64),	    'k', 'I'},	    /* XK_KP_Insert */
    {F(0x65),	    'k', 'D'},	    /* XK_KP_Delete */
    {F(0x37),	    'k', 'h'},	    /* XK_KP_Home */
    {F(0x58),	    '@', '7'},	    /* XK_KP_End */
    {F(0x39),	    'k', 'P'},	    /* XK_KP_Prior */
    {F(0x60),	    'k', 'N'},	    /* XK_KP_Next */
    {F(0x49),	    '&', '8'},	    /* XK_Undo, keypad 5 */
#endif

    /* End of list marker: */
    {0,			    0, 0}
};

#define NUM_SPECIAL_KEYS    (sizeof(special_keys)/sizeof(special_keys[0]))

/* ---------------- VimApp ---------------- */

    static void
docd(BPath &path)
{
    mch_chdir(path.Path());
    /* Do this to get the side effects of a :cd command */
    do_cmdline_cmd((char_u *)"cd .");
}

/*
 * Really handle dropped files and folders.
 */
    static void
RefsReceived(BMessage *m, bool changedir)
{
    uint32 type;
    int32 count;

    //m->PrintToStream();
    switch (m->what) {
    case B_REFS_RECEIVED:
    case B_SIMPLE_DATA:
	m->GetInfo("refs", &type, &count);
	if (type != B_REF_TYPE)
	    goto bad;
	break;
    case B_ARGV_RECEIVED:
	m->GetInfo("argv", &type, &count);
	if (type != B_STRING_TYPE)
	    goto bad;
	if (changedir) {
	    char *dirname;
	    if (m->FindString("cwd", (const char **) &dirname) == B_OK) {
		chdir(dirname);
		do_cmdline_cmd((char_u *)"cd .");
	    }
	}
	break;
    default:
    bad:
	//fprintf(stderr, "bad!\n");
	delete m;
	return;
    }

#ifdef FEAT_VISUAL
    reset_VIsual();
#endif

    char_u  **fnames;
    fnames = (char_u **) alloc(count * sizeof(char_u *));
    int fname_index = 0;

    switch (m->what) {
    case B_REFS_RECEIVED:
    case B_SIMPLE_DATA:
	//fprintf(stderr, "case B_REFS_RECEIVED\n");
	for (int i = 0; i < count; ++i)
	{
	    entry_ref ref;
	    if (m->FindRef("refs", i, &ref) == B_OK) {
		BEntry entry(&ref, false);
		BPath path;
		entry.GetPath(&path);

		/* Change to parent directory? */
		if (changedir) {
		    BPath parentpath;
		    path.GetParent(&parentpath);
		    docd(parentpath);
		}

		/* Is it a directory? If so, cd into it. */
		BDirectory bdir(&ref);
		if (bdir.InitCheck() == B_OK) {
		    /* don't cd if we already did it */
		    if (!changedir)
			docd(path);
		} else {
		    mch_dirname(IObuff, IOSIZE);
		    char_u *fname = shorten_fname((char_u *)path.Path(), IObuff);
		    if (fname == NULL)
			fname = (char_u *)path.Path();
		    fnames[fname_index++] = vim_strsave(fname);
		    //fprintf(stderr, "%s\n", fname);
		}

		/* Only do it for the first file/dir */
		changedir = false;
	    }
	}
	break;
    case B_ARGV_RECEIVED:
	//fprintf(stderr, "case B_ARGV_RECEIVED\n");
	for (int i = 1; i < count; ++i)
	{
	    char *fname;

	    if (m->FindString("argv", i, (const char **) &fname) == B_OK) {
		fnames[fname_index++] = vim_strsave((char_u *)fname);
	    }
	}
	break;
    default:
	//fprintf(stderr, "case default\n");
	break;
    }

    delete m;

    /* Handle the drop, :edit to get to the file */
    if (fname_index > 0) {
	handle_drop(fname_index, fnames, FALSE);

	/* Update the screen display */
	update_screen(NOT_VALID);
	setcursor();
	out_flush();
    } else {
	vim_free(fnames);
    }
}

VimApp::VimApp(const char *appsig):
    BApplication(appsig)
{
}

VimApp::~VimApp()
{
}

    void
VimApp::ReadyToRun()
{
    /*
     * Apparently signals are inherited by the created thread -
     * disable the most annoying ones.
     */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
}

    void
VimApp::ArgvReceived(int32 arg_argc, char **arg_argv)
{
    if (!IsLaunching()) {
	/*
	 * This can happen if we are set to Single or Exclusive
	 * Launch. Be nice and open the file(s).
	 */
	if (gui.vimWindow)
	    gui.vimWindow->Minimize(false);
	BMessage *m = CurrentMessage();
	DetachCurrentMessage();
	SendRefs(m, true);
    }
}

    void
VimApp::RefsReceived(BMessage *m)
{
    /* Horrible hack!!! XXX XXX XXX
     * The real problem is that b_start_ffc is set too late for
     * the initial empty buffer. As a result the window will be
     * split instead of abandoned.
     */
    int limit = 15;
    while (--limit >= 0 && (curbuf == NULL || curbuf->b_start_ffc == 0))
	snooze(100000);    // 0.1 s
    if (gui.vimWindow)
	gui.vimWindow->Minimize(false);
    DetachCurrentMessage();
    SendRefs(m, true);
}

/*
 * Pass a BMessage on to the main() thread.
 * Caller must have detached the message.
 */
    void
VimApp::SendRefs(BMessage *m, bool changedir)
{
    VimRefsMsg rm;
    rm.message = m;
    rm.changedir = changedir;

    write_port(gui.vdcmp, VimMsg::Refs, &rm, sizeof(rm));
    // calls ::RefsReceived
}

    bool
VimApp::QuitRequested()
{
    (void)Inherited::QuitRequested();
    return false;
}

/* ---------------- VimWindow ---------------- */

VimWindow::VimWindow():
    BWindow(BRect(40, 40, 150, 150),
	    "Vim",
	    B_TITLED_WINDOW,
	    0,
	    B_CURRENT_WORKSPACE)

{
    init();
}

VimWindow::~VimWindow()
{
    if (formView) {
	RemoveChild(formView);
	delete formView;
    }
    gui.vimWindow = NULL;
}

    void
VimWindow::init()
{
    /* Attach the VimFormView */
    formView = new VimFormView(Bounds());
    if (formView != NULL) {
	AddChild(formView);
    }
}

    void
VimWindow::DispatchMessage(BMessage *m, BHandler *h)
{
    /*
     * Route B_MOUSE_UP messages to MouseUp(), in
     * a manner that should be compatible with the
     * intended future system behaviour.
     */
    switch (m->what) {
    case B_MOUSE_UP:
	// if (!h) h = PreferredHandler();
// gcc isn't happy without this extra set of braces, complains about
// jump to case label crosses init of 'class BView * v'
// richard@whitequeen.com jul 99
	{
	BView *v = dynamic_cast<BView *>(h);
	if (v) {
	    //m->PrintToStream();
	    BPoint where;
	    m->FindPoint("where", &where);
	    v->MouseUp(where);
	} else {
	    Inherited::DispatchMessage(m, h);
	}
	}
	break;
    default:
	Inherited::DispatchMessage(m, h);
    }
}

    void
VimWindow::WindowActivated(bool active)
{
    Inherited::WindowActivated(active);
    /* the textArea gets the keyboard action */
    if (active && gui.vimTextArea)
	gui.vimTextArea->MakeFocus(true);

    struct VimFocusMsg fm;
    fm.active = active;

    write_port(gui.vdcmp, VimMsg::Focus, &fm, sizeof(fm));
}

    bool
VimWindow::QuitRequested()
{
    struct VimKeyMsg km;
    km.length = 5;
    memcpy((char *)km.chars, "\033:qa\r", km.length);

    write_port(gui.vdcmp, VimMsg::Key, &km, sizeof(km));

    return false;
}

/* ---------------- VimFormView ---------------- */

VimFormView::VimFormView(BRect frame):
    BView(frame, "VimFormView", B_FOLLOW_ALL_SIDES,
	    B_WILL_DRAW | B_FRAME_EVENTS),
    menuBar(NULL),
    textArea(NULL)
{
    init(frame);
}

VimFormView::~VimFormView()
{
    if (menuBar) {
	RemoveChild(menuBar);
#ifdef never
	// deleting the menuBar leads to SEGV on exit
	// richard@whitequeen.com Jul 99
	delete menuBar;
#endif
    }
    if (textArea) {
	RemoveChild(textArea);
	delete textArea;
    }
    gui.vimForm = NULL;
}

    void
VimFormView::init(BRect frame)
{
    menuBar = new BMenuBar(BRect(0,0,-MENUBAR_MARGIN,-MENUBAR_MARGIN),
	    "VimMenuBar");

    AddChild(menuBar);

    BRect remaining = frame;
    textArea = new VimTextAreaView(remaining);
    AddChild(textArea);
    /* The textArea will be resized later when menus are added */

    gui.vimForm = this;
}

    void
VimFormView::AllAttached()
{
    /*
     * Apparently signals are inherited by the created thread -
     * disable the most annoying ones.
     */
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    if (menuBar && textArea) {
	/*
	 * Resize the textArea to fill the space left over by the menu.
	 * This is somewhat futile since it will be done again once
	 * menus are added to the menu bar.
	 */
	BRect remaining = Bounds();
	remaining.top = MenuHeight();
	textArea->ResizeTo(remaining.Width(), remaining.Height());
	textArea->MoveTo(remaining.left, remaining.top);

#ifdef FEAT_MENU
	menuBar->ResizeTo(remaining.right, remaining.top);
	gui.menu_height = (int) remaining.top;
#endif
    }
    Inherited::AllAttached();
}

    void
VimFormView::FrameResized(float new_width, float new_height)
{
    BWindow *w = Window();
#if 1
    /*
     * Look if there are more resize messages in the queue.
     * If so, ignore this one. The later one will be handled
     * eventually.
     */
    BMessageQueue *q = w->MessageQueue();
    if (q->FindMessage(B_VIEW_RESIZED, 0) != NULL) {
	return;
    }
#endif
    new_width += 1;	    // adjust from width to number of pixels occupied
    new_height += 1;

#if !HAVE_R3_OR_LATER
    int adjust_h, adjust_w;

    adjust_w = ((int)new_width - gui_get_base_width()) % gui.char_width;
    adjust_h = ((int)new_height - gui_get_base_height()) % gui.char_height;

    if (adjust_w > 0 || adjust_h > 0) {
	/*
	 * This will generate a new FrameResized() message.
	 * If we're running R3 or later, SetWindowAlignment() should make
	 * sure that this does not happen.
	 */
	w->ResizeBy(-adjust_w, -adjust_h);

	return;
    }
#endif

    struct VimResizeMsg sm;
    sm.width = (int) new_width;
    sm.height = (int) new_height;

    write_port(gui.vdcmp, VimMsg::Resize, &sm, sizeof(sm));
    // calls gui_resize_shell(new_width, new_height);

    return;

    /*
     * The area below the vertical scrollbar is erased to the colour
     * set with SetViewColor() automatically, because we had set
     * B_WILL_DRAW. Resizing the window tight around the vertical
     * scroll bar also helps to avoid debris.
     */
}

/* ---------------- VimTextAreaView ---------------- */

VimTextAreaView::VimTextAreaView(BRect frame):
    BView(frame, "VimTextAreaView", B_FOLLOW_ALL_SIDES,
	    B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
    mouseDragEventCount(0)
{
    init(frame);
}

VimTextAreaView::~VimTextAreaView()
{
    gui.vimTextArea = NULL;
}

    void
VimTextAreaView::init(BRect frame)
{
    /* set up global var for fast access */
    gui.vimTextArea = this;

    /*
     * Tell the app server not to erase the view: we will
     * fill it in completely by ourselves.
     * (Does this really work? Even if not, it won't harm either.)
     */
    SetViewColor(B_TRANSPARENT_32_BIT);
#define PEN_WIDTH   1
    SetPenSize(PEN_WIDTH);
}

    void
VimTextAreaView::Draw(BRect updateRect)
{
    /*
     * XXX Other ports call here:
     * out_flush();	     * make sure all output has been processed *
     * but we can't do that, since it involves too much information
     * that is owned by other threads...
     */

    /*
     *  No need to use gui.vimWindow->Lock(): we are locked already.
     *  However, it would not hurt.
     */
    gui_redraw((int) updateRect.left, (int) updateRect.top,
	    (int) (updateRect.Width() + PEN_WIDTH), (int) (updateRect.Height() + PEN_WIDTH));

    /* Clear the border areas if needed */
    rgb_color rgb = GUI_TO_RGB(gui.back_pixel);
    SetLowColor(rgb);

    if (updateRect.left < FILL_X(0))	// left border
	FillRect(BRect(updateRect.left, updateRect.top,
		       FILL_X(0)-PEN_WIDTH, updateRect.bottom), B_SOLID_LOW);
    if (updateRect.top < FILL_Y(0))	// top border
	FillRect(BRect(updateRect.left, updateRect.top,
		       updateRect.right, FILL_Y(0)-PEN_WIDTH), B_SOLID_LOW);
    if (updateRect.right >= FILL_X(Columns)) // right border
	FillRect(BRect(FILL_X((int)Columns), updateRect.top,
		       updateRect.right, updateRect.bottom), B_SOLID_LOW);
    if (updateRect.bottom >= FILL_Y(Rows))   // bottom border
	FillRect(BRect(updateRect.left, FILL_Y((int)Rows),
		       updateRect.right, updateRect.bottom), B_SOLID_LOW);
}

    void
VimTextAreaView::KeyDown(const char *bytes, int32 numBytes)
{
    struct VimKeyMsg km;
    char_u *dest = km.chars;

    BMessage *msg = Window()->CurrentMessage();
    assert(msg);
    //msg->PrintToStream();

    /*
     * Convert special keys to Vim codes.
     * I think it is better to do it in the window thread
     * so we use at least a little bit of the potential
     * of our 2 CPUs. Besides, due to the fantastic mapping
     * of special keys to UTF-8, we have quite some work to
     * do...
     * TODO: I'm not quite happy with detection of special
     * keys. Perhaps I should use scan codes after all...
     */
    if (numBytes > 1) {
	/* This cannot be a special key */
	if (numBytes > KEY_MSG_BUFSIZ)
	    numBytes = KEY_MSG_BUFSIZ;	    // should never happen... ???
	km.length = numBytes;
	memcpy((char *)dest, bytes, numBytes);
    } else {
	int32 scancode = 0;
	msg->FindInt32("key", &scancode);

	int32 beModifiers = 0;
	msg->FindInt32("modifiers", &beModifiers);

	char_u string[3];
	int len = 0;
	km.length = 0;

	bool canHaveVimModifiers = false;

	/*
	 * For normal, printable ASCII characters, don't look them up
	 * to check if they might be a special key. They aren't.
	 */
	assert(B_BACKSPACE <= 0x20);
	assert(B_DELETE == 0x7F);
	if (((char_u)bytes[0] <= 0x20 || (char_u)bytes[0] == 0x7F) &&
		numBytes == 1) {
	    /*
	     * Due to the great nature of Be's mapping of special keys,
	     * viz. into the range of the control characters,
	     * we can only be sure it is *really* a special key if
	     * if it is special without using ctrl. So, only if ctrl is
	     * used, we need to check it unmodified.
	     */
	    if (beModifiers & B_CONTROL_KEY) {
		int index = keyMap->normal_map[scancode];
		int newNumBytes = keyMapChars[index];
		char_u *newBytes = (char_u *)&keyMapChars[index + 1];

		/*
		 * Check if still special without the control key.
		 * This is needed for BACKSPACE: that key does produce
		 * different values with modifiers (DEL).
		 * Otherwise we could simply have checked for equality.
		 */
		if (newNumBytes != 1 || (*newBytes > 0x20 &&
					 *newBytes != 0x7F )) {
		    goto notspecial;
		}
		bytes = (char *)newBytes;
	    }
	    canHaveVimModifiers = true;

	    uint16 beoskey;
	    int first, last;

	    /*
	     * If numBytes == 0 that probably always indicates a special key.
	     * (does not happen yet)
	     */
	    if (numBytes == 0 || bytes[0] == B_FUNCTION_KEY) {
		beoskey = F(scancode);
		first = FIRST_FUNCTION_KEY;
		last = NUM_SPECIAL_KEYS;
	    } else if (*bytes == '\n' && scancode == 0x47) {
		 /* remap the (non-keypad) ENTER key from \n to \r. */
		string[0] = '\r';
		len = 1;
		first = last = 0;
	    } else {
		beoskey = K(bytes[0]);
		first = 0;
		last = FIRST_FUNCTION_KEY;
	    }

	    for (int i = first; i < last; i++) {
		if (special_keys[i].BeKeys == beoskey) {
		    string[0] = CSI;
		    string[1] = special_keys[i].vim_code0;
		    string[2] = special_keys[i].vim_code1;
		    len = 3;
		}
	    }
	}
    notspecial:
	if (len == 0) {
	    string[0] = bytes[0];
	    len = 1;
	}

	/* Special keys (and a few others) may have modifiers */
#if 0
	if (len == 3 ||
		bytes[0] == B_SPACE || bytes[0] == B_TAB ||
		bytes[0] == B_RETURN || bytes[0] == '\r' ||
		bytes[0] == B_ESCAPE)
#else
	if (canHaveVimModifiers)
#endif
	{
	    int modifiers;
	    modifiers = 0;
	    if (beModifiers & B_SHIFT_KEY)
		modifiers |= MOD_MASK_SHIFT;
	    if (beModifiers & B_CONTROL_KEY)
		modifiers |= MOD_MASK_CTRL;
	    if (beModifiers & B_OPTION_KEY)
		modifiers |= MOD_MASK_ALT;

	    /*
	     * For some keys a shift modifier is translated into another key
	     * code.  Do we need to handle the case where len != 1 and
	     * string[0] != CSI? (Not for BeOS, since len == 3 implies
	     * string[0] == CSI...)
	     */
	    int key;
	    if (string[0] == CSI && len == 3)
		key = TO_SPECIAL(string[1], string[2]);
	    else
		key = string[0];
	    key = simplify_key(key, &modifiers);
	    if (IS_SPECIAL(key))
	    {
		string[0] = CSI;
		string[1] = K_SECOND(key);
		string[2] = K_THIRD(key);
		len = 3;
	    }
	    else
	    {
		string[0] = key;
		len = 1;
	    }

	    if (modifiers)
	    {
		*dest++ = CSI;
		*dest++ = KS_MODIFIER;
		*dest++ = modifiers;
		km.length = 3;
	    }
	}
	memcpy((char *)dest, string, len);
	km.length += len;
    }

    write_port(gui.vdcmp, VimMsg::Key, &km, sizeof(km));

    /*
     * blank out the pointer if necessary
     */
    if (p_mh && !gui.pointer_hidden)
    {
	guiBlankMouse(true);
	gui.pointer_hidden = TRUE;
    }
}
    void
VimTextAreaView::guiSendMouseEvent(
    int	    button,
    int	    x,
    int	    y,
    int	    repeated_click,
    int_u   modifiers)
{
    VimMouseMsg mm;

    mm.button = button;
    mm.x = x;
    mm.y = y;
    mm.repeated_click = repeated_click;
    mm.modifiers = modifiers;

    write_port(gui.vdcmp, VimMsg::Mouse, &mm, sizeof(mm));
    // calls gui_send_mouse_event()

    /*
     * if our pointer is currently hidden, then we should show it.
     */
    if (gui.pointer_hidden)
    {
	guiBlankMouse(false);
	gui.pointer_hidden = FALSE;
    }
}

    void
VimTextAreaView::guiBlankMouse(bool should_hide)
{
    if (should_hide) {
	//gui.vimApp->HideCursor();
	gui.vimApp->ObscureCursor();
	/*
	 * ObscureCursor() would even be easier, but then
	 * Vim's idea of mouse visibility does not necessarily
	 * correspond to reality.
	 */
    } else {
	//gui.vimApp->ShowCursor();
    }
}

    int_u
VimTextAreaView::mouseModifiersToVim(int32 beModifiers)
{
    int_u vim_modifiers = 0x0;

    if (beModifiers & B_SHIFT_KEY)
	vim_modifiers |= MOUSE_SHIFT;
    if (beModifiers & B_CONTROL_KEY)
	vim_modifiers |= MOUSE_CTRL;
    if (beModifiers & B_OPTION_KEY)	    /* Alt or Meta key */
	vim_modifiers |= MOUSE_ALT;

    return vim_modifiers;
}

    void
VimTextAreaView::MouseDown(BPoint point)
{
    BMessage *m = Window()->CurrentMessage();
    assert(m);

    int32 buttons = 0;
    m->FindInt32("buttons", &buttons);

    int vimButton;

    if (buttons & B_PRIMARY_MOUSE_BUTTON)
	vimButton = MOUSE_LEFT;
    else if (buttons & B_SECONDARY_MOUSE_BUTTON)
	vimButton = MOUSE_RIGHT;
    else if (buttons & B_TERTIARY_MOUSE_BUTTON)
	vimButton = MOUSE_MIDDLE;
    else
	return;			/* Unknown button */

    vimMouseButton = 1;		/* don't care which one */

    /* Handle multiple clicks */
    int32 clicks = 0;
    m->FindInt32("clicks", &clicks);

    int32 modifiers = 0;
    m->FindInt32("modifiers", &modifiers);

    vimMouseModifiers = mouseModifiersToVim(modifiers);

    guiSendMouseEvent(vimButton, point.x, point.y,
	    clicks > 1 /* = repeated_click*/, vimMouseModifiers);
}

    void
VimTextAreaView::MouseUp(BPoint point)
{
    vimMouseButton = 0;

    BMessage *m = Window()->CurrentMessage();
    assert(m);
    //m->PrintToStream();

    int32 modifiers = 0;
    m->FindInt32("modifiers", &modifiers);

    vimMouseModifiers = mouseModifiersToVim(modifiers);

    guiSendMouseEvent(MOUSE_RELEASE, point.x, point.y,
	    0 /* = repeated_click*/, vimMouseModifiers);

    Inherited::MouseUp(point);
}

    void
VimTextAreaView::MouseMoved(BPoint point, uint32 transit, const BMessage *message)
{
    /*
     * if our pointer is currently hidden, then we should show it.
     */
    if (gui.pointer_hidden)
    {
	guiBlankMouse(false);
	gui.pointer_hidden = FALSE;
    }

    if (!vimMouseButton)    /* could also check m->"buttons" */
	return;

    atomic_add(&mouseDragEventCount, 1);

    /* Don't care much about "transit" */
    guiSendMouseEvent(MOUSE_DRAG, point.x, point.y, 0, vimMouseModifiers);
}

    void
VimTextAreaView::MessageReceived(BMessage *m)
{
    switch (m->what) {
    case 'menu':
	{
	    VimMenuMsg mm;
	    mm.guiMenu = NULL;	/* in case no pointer in msg */
	    m->FindPointer("VimMenu", (void **)&mm.guiMenu);

	    write_port(gui.vdcmp, VimMsg::Menu, &mm, sizeof(mm));
	}
	break;
    default:
	if (m->WasDropped()) {
	    BWindow *w = Window();
	    w->DetachCurrentMessage();
	    w->Minimize(false);
	    VimApp::SendRefs(m, (modifiers() & B_SHIFT_KEY) != 0);
	} else {
	    Inherited::MessageReceived(m);
	}
	break;
    }
}

    int
VimTextAreaView::mchInitFont(char_u *name)
{
    VimFont *newFont = (VimFont *)gui_mch_get_font(name, 0);

    gui.norm_font = (GuiFont)newFont;
    gui_mch_set_font((GuiFont)newFont);
    if (name)
	hl_set_font_name(name);

    SetDrawingMode(B_OP_COPY);

    /*
     * Try to load other fonts for bold, italic, and bold-italic.
     * We should also try to work out what font to use for these when they are
     * not specified by X resources, but we don't yet.
     */

    return OK;
}

    void
VimTextAreaView::mchDrawString(int row, int col, char_u *s, int len, int flags)
{
    /*
     * First we must erase the area, because DrawString won't do
     * that for us. XXX Most of the time this is a waste of effort
     * since the bachground has been erased already... DRAW_TRANSP
     * should be set when appropriate!!!
     * (Rectangles include the bottom and right edge)
     */
    if (!(flags & DRAW_TRANSP)) {
	BRect r(FILL_X(col), FILL_Y(row),
		FILL_X(col + len) - PEN_WIDTH, FILL_Y(row + 1) - PEN_WIDTH);
	FillRect(r, B_SOLID_LOW);
    }
    BPoint where(TEXT_X(col), TEXT_Y(row));
    DrawString((char *)s, len, where);

    if (flags & DRAW_BOLD) {
	where.x += 1.0;
	SetDrawingMode(B_OP_BLEND);
	DrawString((char *)s, len, where);
	SetDrawingMode(B_OP_COPY);
    }
    if (flags & DRAW_UNDERL) {
	BPoint start(FILL_X(col), FILL_Y(row + 1) - PEN_WIDTH);
	BPoint end(FILL_X(col + len) - PEN_WIDTH, start.y);

	StrokeLine(start, end);
    }
}

    void
VimTextAreaView::mchClearBlock(
    int		row1,
    int		col1,
    int		row2,
    int		col2)
{
    BRect r(FILL_X(col1), FILL_Y(row1),
	    FILL_X(col2 + 1) - PEN_WIDTH, FILL_Y(row2 + 1) - PEN_WIDTH);
    gui_mch_set_bg_color(gui.back_pixel);
    FillRect(r, B_SOLID_LOW);
}

    void
VimTextAreaView::mchClearAll()
{
    gui_mch_set_bg_color(gui.back_pixel);
    FillRect(Bounds(), B_SOLID_LOW);
}

/*
 * mchDeleteLines() Lock()s the window by itself.
 */
    void
VimTextAreaView::mchDeleteLines(int row, int num_lines)
{
    if (row + num_lines > gui.scroll_region_bot)
    {
	/* Scrolled out of region, just blank the lines out */
	gui_clear_block(row, gui.scroll_region_left,
		gui.scroll_region_bot, gui.scroll_region_right);
    }
    else
    {
	/* copy one extra pixel, for when bold has spilled over */
	int width = gui.char_width * (gui.scroll_region_right
				- gui.scroll_region_left + 1) + 1 - PEN_WIDTH;
	int height = gui.char_height *
		     (gui.scroll_region_bot - row - num_lines + 1) - PEN_WIDTH;

	BRect source, dest;

	source.left = FILL_X(gui.scroll_region_left);
	source.top = FILL_Y(row + num_lines);
	source.right = source.left + width;
	source.bottom = source.top + height;

	dest.left = FILL_X(gui.scroll_region_left);
	dest.top = FILL_Y(row);
	dest.right = dest.left + width;
	dest.bottom = dest.top + height;

	/* XXX Attempt at a hack: */
	gui.vimWindow->UpdateIfNeeded();
#if 0
	/* XXX Attempt at a hack: */
	if (gui.vimWindow->NeedsUpdate()) {
	    fprintf(stderr, "mchDeleteLines: NeedsUpdate!\n");
	    gui.vimWindow->UpdateIfNeeded();
	    while (gui.vimWindow->NeedsUpdate()) {
		if (false && gui.vimWindow->Lock()) {
		    Sync();
		    gui.vimWindow->Unlock();
		}
		snooze(2);
	    }
	}
#endif

	if (gui.vimWindow->Lock()) {
	    Sync();
	    CopyBits(source, dest);
	    //Sync();

	    /* Update gui.cursor_row if the cursor scrolled or copied over */
	    if (gui.cursor_row >= row
		&& gui.cursor_col >= gui.scroll_region_left
		&& gui.cursor_col <= gui.scroll_region_right)
	    {
		if (gui.cursor_row < row + num_lines)
		    gui.cursor_is_valid = FALSE;
		else if (gui.cursor_row <= gui.scroll_region_bot)
		    gui.cursor_row -= num_lines;
	    }

	    /* Clear one column more for when bold has spilled over */
	    gui_clear_block(gui.scroll_region_bot - num_lines + 1,
						       gui.scroll_region_left,
		gui.scroll_region_bot, gui.scroll_region_right);

	    gui.vimWindow->Unlock();
	    /*
	     * The Draw() callback will be called now if some of the source
	     * bits were not in the visible region.
	     */

	    //gui_x11_check_copy_area();
	}
    }
}

/*
 * mchInsertLines() Lock()s the window by itself.
 */
    void
VimTextAreaView::mchInsertLines(int row, int num_lines)
{
    if (row + num_lines > gui.scroll_region_bot)
    {
	/* Scrolled out of region, just blank the lines out */
	gui_clear_block(row, gui.scroll_region_left,
		gui.scroll_region_bot, gui.scroll_region_right);
    }
    else
    {
	/* copy one extra pixel, for when bold has spilled over */
	int width = gui.char_width * (gui.scroll_region_right
				- gui.scroll_region_left + 1) + 1 - PEN_WIDTH;
	int height = gui.char_height *
		     (gui.scroll_region_bot - row - num_lines + 1) - PEN_WIDTH;

	BRect source, dest;

	source.left = FILL_X(gui.scroll_region_left);
	source.top = FILL_Y(row);
	source.right = source.left + width;
	source.bottom = source.top + height;

	dest.left = FILL_X(gui.scroll_region_left);
	dest.top = FILL_Y(row + num_lines);
	dest.right = dest.left + width;
	dest.bottom = dest.top + height;

	/* XXX Attempt at a hack: */
	gui.vimWindow->UpdateIfNeeded();
#if 0
	/* XXX Attempt at a hack: */
	if (gui.vimWindow->NeedsUpdate())
	    fprintf(stderr, "mchInsertLines: NeedsUpdate!\n");
	gui.vimWindow->UpdateIfNeeded();
	while (gui.vimWindow->NeedsUpdate())
	    snooze(2);
#endif

	if (gui.vimWindow->Lock()) {
	    Sync();
	    CopyBits(source, dest);
	    //Sync();

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
	    /* Clear one column more for when bold has spilled over */
	    gui_clear_block(row, gui.scroll_region_left,
		    row + num_lines - 1, gui.scroll_region_right);

	    gui.vimWindow->Unlock();
	    /*
	     * The Draw() callback will be called now if some of the source
	     * bits were not in the visible region.
	     * However, if we scroll too fast it can't keep up and the
	     * update region gets messed up. This seems to be because copying
	     * un-Draw()n bits does not generate Draw() calls for the copy...
	     * I moved the hack to before the CopyBits() to reduce the
	     * amount of additional waiting needed.
	     */

	    //gui_x11_check_copy_area();
	}
    }

}

/* ---------------- VimScrollBar ---------------- */

/* BUG: XXX
 * It seems that BScrollBar determine their direction not from
 * "posture" but from if they are "tall" or "wide" in shape...
 *
 * Also, place them out of sight, because Vim enables them before
 * they are positioned.
 */
VimScrollBar::VimScrollBar(scrollbar_T *g, orientation posture):
    BScrollBar(posture == B_HORIZONTAL ?  BRect(-100,-100,-10,-90) :
					  BRect(-100,-100,-90,-10),
		"vim scrollbar", (BView *)NULL,
	    0.0, 10.0, posture),
    ignoreValue(-1),
    scrollEventCount(0)
{
    gsb = g;
    SetResizingMode(B_FOLLOW_NONE);
}

VimScrollBar::~VimScrollBar()
{
}

    void
VimScrollBar::ValueChanged(float newValue)
{
    if (ignoreValue >= 0.0 && newValue == ignoreValue) {
	ignoreValue = -1;
	return;
    }
    ignoreValue = -1;
    /*
     * We want to throttle the amount of scroll messages generated.
     * Normally I presume you won't get a new message before we've
     * handled the previous one, but because we're passing them on this
     * happens very quickly. So instead we keep a counter of how many
     * scroll events there are (or will be) in the VDCMP, and the
     * throttling happens at the receiving end.
     */
    atomic_add(&scrollEventCount, 1);

    struct VimScrollBarMsg sm;

    sm.sb = this;
    sm.value = (long) newValue;
    sm.stillDragging = TRUE;

    write_port(gui.vdcmp, VimMsg::ScrollBar, &sm, sizeof(sm));

    // calls gui_drag_scrollbar(sb, newValue, TRUE);
}

/*
 * When the mouse goes up, report that scrolling has stopped.
 * MouseUp() is NOT called when the mouse-up occurs outside
 * the window, even though the thumb does move while the mouse
 * is outside... This has some funny effects... XXX
 * So we do special processing when the window de/activates.
 */
    void
VimScrollBar::MouseUp(BPoint where)
{
    //BMessage *m = Window()->CurrentMessage();
    //m->PrintToStream();

    atomic_add(&scrollEventCount, 1);

    struct VimScrollBarMsg sm;

    sm.sb = this;
    sm.value = (long) Value();
    sm.stillDragging = FALSE;

    write_port(gui.vdcmp, VimMsg::ScrollBar, &sm, sizeof(sm));

    // calls gui_drag_scrollbar(sb, newValue, FALSE);

    Inherited::MouseUp(where);
}

    void
VimScrollBar::SetValue(float newValue)
{
    if (newValue == Value())
	return;

    ignoreValue = newValue;
    Inherited::SetValue(newValue);
}

/* ---------------- VimFont ---------------- */

VimFont::VimFont(): BFont()
{
    init();
}

VimFont::VimFont(const VimFont *rhs): BFont(rhs)
{
    init();
}

VimFont::VimFont(const BFont *rhs): BFont(rhs)
{
    init();
}

VimFont::VimFont(const VimFont &rhs): BFont(rhs)
{
    init();
}

VimFont::~VimFont()
{
}

    void
VimFont::init()
{
    next = NULL;
    refcount = 1;
    name = NULL;
}

/* ---------------- ---------------- */

// some global variables
static char appsig[] = "application/x-vnd.Rhialto-Vim-5";
key_map *keyMap;
char *keyMapChars;
int main_exitcode = 127;

    status_t
gui_beos_process_event(bigtime_t timeout)
{
    struct VimMsg vm;
    long what;
    ssize_t size;

    size = read_port_etc(gui.vdcmp, &what, &vm, sizeof(vm),
	    B_TIMEOUT, timeout);

    if (size >= 0) {
	switch (what) {
	case VimMsg::Key:
	    {
		char_u *string = vm.u.Key.chars;
		int len = vm.u.Key.length;
		if (len == 1 && string[0] == Ctrl_chr('C')) {
		    trash_input_buf();
		    got_int = TRUE;
		}
		add_to_input_buf(string, len);
	    }
	    break;
	case VimMsg::Resize:
	    gui_resize_shell(vm.u.NewSize.width, vm.u.NewSize.height);
	    break;
	case VimMsg::ScrollBar:
	    {
		/*
		 * If loads of scroll messages queue up, use only the last
		 * one. Always report when the scrollbar stops dragging.
		 * This is not perfect yet anyway: these events are queued
		 * yet again, this time in the keyboard input buffer.
		 */
		int32 oldCount =
		    atomic_add(&vm.u.Scroll.sb->scrollEventCount, -1);
		if (oldCount <= 1 || !vm.u.Scroll.stillDragging)
		    gui_drag_scrollbar(vm.u.Scroll.sb->getGsb(),
			    vm.u.Scroll.value, vm.u.Scroll.stillDragging);
	    }
	    break;
	case VimMsg::Menu:
	    gui_menu_cb(vm.u.Menu.guiMenu);
	    break;
	case VimMsg::Mouse:
	    {
		int32 oldCount;
		if (vm.u.Mouse.button == MOUSE_DRAG)
		    oldCount =
			atomic_add(&gui.vimTextArea->mouseDragEventCount, -1);
		else
		    oldCount = 0;
		if (oldCount <= 1)
		    gui_send_mouse_event(vm.u.Mouse.button, vm.u.Mouse.x,
			    vm.u.Mouse.y, vm.u.Mouse.repeated_click,
			    vm.u.Mouse.modifiers);
	    }
	    break;
	case VimMsg::Focus:
	    gui.in_focus = vm.u.Focus.active;
	    /* XXX Signal that scrollbar dragging has stopped?
	     * This is needed because we don't get a MouseUp if
	     * that happens while outside the window... :-(
	     */
	    if (gui.dragged_sb) {
		gui.dragged_sb = SBAR_NONE;
	    }
	    gui_update_cursor(TRUE, FALSE);
	    break;
	case VimMsg::Refs:
	    ::RefsReceived(vm.u.Refs.message, vm.u.Refs.changedir);
	    break;
	default:
	    // unrecognised message, ignore it
	    break;
	}
    }

    /*
     * If size < B_OK, it is an error code.
     */
    return size;
}

/*
 * Here are some functions to protect access to ScreenLines[] and
 * LineOffset[]. These are used from the window thread to respond
 * to a Draw() callback. When that occurs, the window is already
 * locked by the system.
 *
 * Other code that needs to lock is any code that changes these
 * variables. Other read-only access, or access merely to the
 * contents of the screen buffer, need not be locked.
 *
 * If there is no window, don't call Lock() but do succeed.
 */

    int
vim_lock_screen()
{
    return !gui.vimWindow || gui.vimWindow->Lock();
}

    void
vim_unlock_screen()
{
    if (gui.vimWindow)
	gui.vimWindow->Unlock();
}

#define RUN_BAPPLICATION_IN_NEW_THREAD	0

#if RUN_BAPPLICATION_IN_NEW_THREAD

    int32
run_vimapp(void *args)
{
    VimApp app(appsig);

    gui.vimApp = &app;
    app.Run();			    /* Run until Quit() called */

    return 0;
}

#else

    int32
call_main(void *args)
{
    struct MainArgs *ma = (MainArgs *)args;

    return main(ma->argc, ma->argv);
}
#endif

extern "C" {

/*
 * Parse the GUI related command-line arguments.  Any arguments used are
 * deleted from argv, and *argc is decremented accordingly.  This is called
 * when vim is started, whether or not the GUI has been started.
 */
    void
gui_mch_prepare(
    int		*argc,
    char	**argv)
{
    /*
     * We don't have any command line arguments for the BeOS GUI yet,
     * but this is an excellent place to create our Application object.
     */
    if (!gui.vimApp) {
	thread_info tinfo;
	get_thread_info(find_thread(NULL), &tinfo);

	/* May need the port very early on to process RefsReceived() */
	gui.vdcmp = create_port(B_MAX_PORT_COUNT, "vim VDCMP");

#if RUN_BAPPLICATION_IN_NEW_THREAD
	thread_id tid = spawn_thread(run_vimapp, "vim VimApp",
						tinfo.priority, NULL);
	if (tid >= B_OK) {
	    resume_thread(tid);
	} else {
	    getout(1);
	}
#else
	MainArgs ma = { *argc, argv };
	thread_id tid = spawn_thread(call_main, "vim main()",
						tinfo.priority, &ma);
	if (tid >= B_OK) {
	    VimApp app(appsig);

	    gui.vimApp = &app;
	    resume_thread(tid);
	    /*
	     * This is rather horrible.
	     * call_main will call main() again...
	     * There will be no infinite recursion since
	     * gui.vimApp is set now.
	     */
	    app.Run();			    /* Run until Quit() called */
	    //fprintf(stderr, "app.Run() returned...\n");
	    status_t dummy_exitcode;
	    (void)wait_for_thread(tid, &dummy_exitcode);

	    /*
	     * This path should be the normal one taken to exit Vim.
	     * The main() thread calls mch_exit() which calls
	     * gui_mch_exit() which terminates its thread.
	     */
	    exit(main_exitcode);
	}
#endif
    }
    /* Don't fork() when starting the GUI. Spawned threads are not
     * duplicated with a fork(). The result is a mess.
     */
    gui.dofork = FALSE;
    /*
     * XXX Try to determine whether we were started from
     * the Tracker or the terminal.
     * It would be nice to have this work, because the Tracker
     * follows symlinks, so even if you double-click on gvim,
     * when it is a link to vim it will still pass a command name
     * of vim...
     * We try here to see if stdin comes from /dev/null. If so,
     * (or if there is an error, which should never happen) start the GUI.
     * This does the wrong thing for vim - </dev/null, and we're
     * too early to see the command line parsing. Tough.
     * On the other hand, it starts the gui for vim file & which is nice.
     */
    if (!isatty(0)) {
	struct stat stat_stdin, stat_dev_null;

	if (fstat(0, &stat_stdin) == -1 ||
	    stat("/dev/null", &stat_dev_null) == -1 ||
	    (stat_stdin.st_dev == stat_dev_null.st_dev &&
	     stat_stdin.st_ino == stat_dev_null.st_ino))
	    gui.starting = TRUE;
    }
}

/*
 * Check if the GUI can be started.  Called before gvimrc is sourced.
 * Return OK or FAIL.
 */
    int
gui_mch_init_check(void)
{
    return OK;		/* TODO: GUI can always be started? */
}

/*
 * Initialise the GUI.  Create all the windows, set up all the call-backs
 * etc.
 */
    int
gui_mch_init()
{
    gui.def_norm_pixel = RGB(0x00, 0x00, 0x00);	// black
    gui.def_back_pixel = RGB(0xFF, 0xFF, 0xFF);	// white
    gui.norm_pixel = gui.def_norm_pixel;
    gui.back_pixel = gui.def_back_pixel;

    gui.scrollbar_width = (int) B_V_SCROLL_BAR_WIDTH;
    gui.scrollbar_height = (int) B_H_SCROLL_BAR_HEIGHT;
#ifdef FEAT_MENU
    gui.menu_height = 19;	// initial guess -
				// correct for my default settings
#endif
    gui.border_offset = 3;	// coordinates are inside window borders

    if (gui.vdcmp < B_OK)
	return FAIL;
    get_key_map(&keyMap, &keyMapChars);

    gui.vimWindow = new VimWindow();	/* hidden and locked */
    if (!gui.vimWindow)
	return FAIL;

    gui.vimWindow->Run();		/* Run() unlocks but does not show */

    /* Get the colors from the "Normal" group (set in syntax.c or in a vimrc
     * file) */
    set_normal_colors();

    /*
     * Check that none of the colors are the same as the background color
     */
    gui_check_colors();

    /* Get the colors for the highlight groups (gui_check_colors() might have
     * changed them) */
    highlight_gui_started();		/* re-init colors and fonts */

    gui_mch_new_colors();		/* window must exist for this */

    return OK;
}

/*
 * Called when the foreground or background color has been changed.
 */
    void
gui_mch_new_colors()
{
    rgb_color rgb = GUI_TO_RGB(gui.back_pixel);

    if (gui.vimWindow->Lock()) {
	gui.vimForm->SetViewColor(rgb);
	// Does this not have too much effect for those small rectangles?
	gui.vimForm->Invalidate();
	gui.vimWindow->Unlock();
    }
}

/*
 * Open the GUI window which was created by a call to gui_mch_init().
 */
    int
gui_mch_open()
{
    if (gui_win_x != -1 && gui_win_y != -1)
	gui_mch_set_winpos(gui_win_x, gui_win_y);

    /* Actually open the window */
    if (gui.vimWindow->Lock()) {
	gui.vimWindow->Show();
	gui.vimWindow->Unlock();

#if USE_THREAD_FOR_INPUT_WITH_TIMEOUT
	/* Kill the thread that may have been created for the Terminal */
	beos_cleanup_read_thread();
#endif

	return OK;
    }

    return FAIL;
}

    void
gui_mch_exit(int vim_exitcode)
{
    if (gui.vimWindow) {
	thread_id tid = gui.vimWindow->Thread();
	gui.vimWindow->Lock();
	gui.vimWindow->Quit();
	/* Wait until it is truely gone */
	int32 exitcode;
	wait_for_thread(tid, &exitcode);
    }
    delete_port(gui.vdcmp);
#if !RUN_BAPPLICATION_IN_NEW_THREAD
    /*
     * We are in the main() thread - quit the App thread and
     * quit ourselves (passing on the exitcode). Use a global since the
     * value from exit_thread() is only used if wait_for_thread() is
     * called in time (race condition).
     */
#endif
    if (gui.vimApp) {
	VimTextAreaView::guiBlankMouse(false);

	main_exitcode = vim_exitcode;
#if RUN_BAPPLICATION_IN_NEW_THREAD
	thread_id tid = gui.vimApp->Thread();
	int32 exitcode;
	gui.vimApp->Lock();
	gui.vimApp->Quit();
	gui.vimApp->Unlock();
	wait_for_thread(tid, &exitcode);
#else
	gui.vimApp->Lock();
	gui.vimApp->Quit();
	gui.vimApp->Unlock();
	/* suicide */
	exit_thread(vim_exitcode);
#endif
    }
    /* If we are somehow still here, let mch_exit() handle things. */
}

/*
 * Get the position of the top left corner of the window.
 */
    int
gui_mch_get_winpos(int *x, int *y)
{
    /* TODO */
    return FAIL;
}

/*
 * Set the position of the top left corner of the window to the given
 * coordinates.
 */
    void
gui_mch_set_winpos(int x, int y)
{
    /* TODO */
}

/*
 * Set the size of the window to the given width and height in pixels.
 */
    void
gui_mch_set_shellsize(
    int		width,
    int		height,
    int		min_width,
    int		min_height,
    int		base_width,
    int		base_height)
{
    /*
     * We are basically given the size of the VimForm, if I understand
     * correctly. Since it fills the window completely, this will also
     * be the size of the window.
     */
    if (gui.vimWindow->Lock()) {
	gui.vimWindow->ResizeTo(width - PEN_WIDTH, height - PEN_WIDTH);

	/* set size limits */
	float minWidth, maxWidth, minHeight, maxHeight;

	gui.vimWindow->GetSizeLimits(&minWidth, &maxWidth,
				     &minHeight, &maxHeight);
	gui.vimWindow->SetSizeLimits(min_width, maxWidth,
				     min_height, maxHeight);

#if HAVE_R3_OR_LATER
	/*
	 * Set the resizing alignment depending on font size.
	 * XXX This is untested, since I don't have R3 yet.
	 */
	SetWindowAlignment(
	    B_PIXEL_ALIGNMENT,		// window_alignment mode,
	    1,				// int32 h,
	    0,				// int32 hOffset = 0,
	    gui.char_width,		// int32 width = 0,
	    base_width,			// int32 widthOffset = 0,
	    1,				// int32 v = 0,
	    0,				// int32 vOffset = 0,
	    gui.char_height,		// int32 height = 0,
	    base_height			// int32 heightOffset = 0
	);
#else
	/* don't know what to do with base_{width,height}. */
#endif

	gui.vimWindow->Unlock();
    }
}

    void
gui_mch_get_screen_dimensions(
    int		*screen_w,
    int		*screen_h)
{
    BRect frame;

    {
	BScreen screen(gui.vimWindow);

	if (screen.IsValid()) {
	    frame = screen.Frame();
	} else {
	    frame.right = 640;
	    frame.bottom = 480;
	}
    }

    /* XXX approximations... */
    *screen_w = (int) frame.right - 2 * gui.scrollbar_width - 20;
    *screen_h = (int) frame.bottom - gui.scrollbar_height
#ifdef FEAT_MENU
	- gui.menu_height
#endif
	- 30;
}

    void
gui_mch_set_text_area_pos(
    int		x,
    int		y,
    int		w,
    int		h)
{
    if (!gui.vimTextArea)
	return;

    if (gui.vimWindow->Lock()) {
	gui.vimTextArea->MoveTo(x, y);
	gui.vimTextArea->ResizeTo(w - PEN_WIDTH, h - PEN_WIDTH);
	gui.vimWindow->Unlock();
    }
}


/*
 * Scrollbar stuff:
 */

    void
gui_mch_enable_scrollbar(
    scrollbar_T	*sb,
    int		flag)
{
    VimScrollBar *vsb = sb->id;
    if (gui.vimWindow->Lock()) {
	/*
	 * This function is supposed to be idempotent, but Show()/Hide()
	 * is not. Therefore we test if they are needed.
	 */
	if (flag) {
	    if (vsb->IsHidden()) {
		vsb->Show();
	    }
	} else {
	    if (!vsb->IsHidden()) {
		vsb->Hide();
	    }
	}
	gui.vimWindow->Unlock();
    }
}

    void
gui_mch_set_scrollbar_thumb(
    scrollbar_T *sb,
    int		val,
    int		size,
    int		max)
{
    if (gui.vimWindow->Lock()) {
	VimScrollBar *s = sb->id;
	if (max == 0) {
	    s->SetValue(0);
	    s->SetRange(0.0, 0.0);
	} else {
	    s->SetProportion((float)size / (max + 1.0));
	    s->SetSteps(1.0, size > 5 ? size - 2 : size);
#ifndef SCROLL_PAST_END		// really only defined in gui.c...
	    max = max + 1 - size;
#endif
	    if (max < s->Value()) {
		/*
		 * If the new maximum is lower than the current value,
		 * setting it would cause the value to be clipped and
		 * therefore a ValueChanged() call.
		 * We avoid this by setting the value first, because
		 * it presumably is <= max.
		 */
		s->SetValue(val);
		s->SetRange(0.0, max);
	    } else {
		/*
		 * In the other case, set the range first, since the
		 * new value might be higher than the current max.
		 */
		s->SetRange(0.0, max);
		s->SetValue(val);
	    }
	}
	gui.vimWindow->Unlock();
    }
}

    void
gui_mch_set_scrollbar_pos(
    scrollbar_T *sb,
    int		x,
    int		y,
    int		w,
    int		h)
{
    if (gui.vimWindow->Lock()) {
	VimScrollBar *vsb = sb->id;
	vsb->MoveTo(x, y);
	vsb->ResizeTo(w - PEN_WIDTH, h - PEN_WIDTH);
	gui.vimWindow->Unlock();
    }
}

    void
gui_mch_create_scrollbar(
    scrollbar_T *sb,
    int		orient)		/* SBAR_VERT or SBAR_HORIZ */
{
    orientation posture =
	(orient == SBAR_HORIZ) ? B_HORIZONTAL : B_VERTICAL;

    VimScrollBar *vsb = sb->id = new VimScrollBar(sb, posture);
    if (gui.vimWindow->Lock()) {
	vsb->SetTarget(gui.vimTextArea);
	vsb->Hide();
	gui.vimForm->AddChild(vsb);
	gui.vimWindow->Unlock();
    }
}

#if defined(FEAT_WINDOWS) || defined(PROTO)
    void
gui_mch_destroy_scrollbar(
    scrollbar_T	*sb)
{
    if (gui.vimWindow->Lock()) {
	sb->id->RemoveSelf();
	delete sb->id;
	gui.vimWindow->Unlock();
    }
}
#endif

/*
 * Cursor blink functions.
 *
 * This is a simple state machine:
 * BLINK_NONE	not blinking at all
 * BLINK_OFF	blinking, cursor is not shown
 * BLINK_ON	blinking, cursor is shown
 */

#define BLINK_NONE  0
#define BLINK_OFF   1
#define BLINK_ON    2

static int		blink_state = BLINK_NONE;
static long_u		blink_waittime = 700;
static long_u		blink_ontime = 400;
static long_u		blink_offtime = 250;
static int	blink_timer = 0;

    void
gui_mch_set_blinking(
    long    waittime,
    long    on,
    long    off)
{
	/* TODO */
    blink_waittime = waittime;
    blink_ontime = on;
    blink_offtime = off;
}

/*
 * Stop the cursor blinking.  Show the cursor if it wasn't shown.
 */
    void
gui_mch_stop_blink()
{
	/* TODO */
    if (blink_timer != 0)
    {
	//XtRemoveTimeOut(blink_timer);
	blink_timer = 0;
    }
    if (blink_state == BLINK_OFF)
	gui_update_cursor(TRUE, FALSE);
    blink_state = BLINK_NONE;
}

/*
 * Start the cursor blinking.  If it was already blinking, this restarts the
 * waiting time and shows the cursor.
 */
    void
gui_mch_start_blink()
{
	/* TODO */
    if (blink_timer != 0)
	;//XtRemoveTimeOut(blink_timer);
    /* Only switch blinking on if none of the times is zero */
    if (blink_waittime && blink_ontime && blink_offtime && gui.in_focus)
    {
	blink_timer = 1; //XtAppAddTimeOut(app_context, blink_waittime,
	blink_state = BLINK_ON;
	gui_update_cursor(TRUE, FALSE);
    }
}

/*
 * Initialise vim to use the font with the given name.	Return FAIL if the font
 * could not be loaded, OK otherwise.
 */
    int
gui_mch_init_font(
    char_u		*font_name,
    int			fontset)
{
    if (gui.vimWindow->Lock())
    {
	int rc = gui.vimTextArea->mchInitFont(font_name);
	gui.vimWindow->Unlock();

	return rc;
    }

    return FAIL;
}

    int
gui_mch_adjust_charsize()
{
    return FAIL;
}

    GuiFont
gui_mch_get_font(
    char_u		*name,
    int			giveErrorIfMissing)
{
    VimFont		*font = 0;
    static VimFont *fontList = NULL;

    if (!gui.in_use)		    /* can't do this when GUI not running */
	return NOFONT;

    if (!name)
	name = (char_u *)"be_fixed_font";

    VimFont *flp;
    for (flp = fontList; flp; flp = flp->next) {
	if (STRCMP(name, flp->name) == 0) {
	    flp->refcount++;
	    return (GuiFont)flp;
	}
    }

    font = new VimFont(be_fixed_font);

    /* Set some universal features: */
    font->SetSpacing(B_FIXED_SPACING);
    font->SetEncoding(B_ISO_8859_1);

    /* Remember font for later use */
    font->name = vim_strsave(name);
    font->next = fontList;
    fontList = font;

    font_family family;
    font_style style;
    int size;
    int len;
    char_u *end;

#ifdef never
    // This leads to SEGV/BUS on R4+
    // Replace underscores with spaces, and I can't see why ?
    // richard@whitequeen.com jul 99
    while (end = (char_u *)strchr((char *)name, '_'))
	*end = ' ';
#endif
    /*
     *  Parse font names as Family/Style/Size.
     *  On errors, just keep the be_fixed_font.
     */
    end = (char_u *)strchr((char *)name, '/');
    if (!end)
	goto error;
    strncpy(family, (char *)name, len = end - name);
    family[len] = '\0';

    name = end + 1;
    end = (char_u *)strchr((char *)name, '/');
    if (!end)
	goto error;
    strncpy(style, (char *)name, len = end - name);
    style[len] = '\0';

    name = end + 1;
    size = atoi((char *)name);
    if (size <= 0)
	goto error;

    font->SetFamilyAndStyle(family, style);
    font->SetSize(size);
    font->SetSpacing(B_FIXED_SPACING);
    font->SetEncoding(B_ISO_8859_1);
    //font->PrintToStream();

    return (GuiFont)font;

error:
    if (giveErrorIfMissing)
	EMSG2("(fe0) Unknown font: %s", name);

    return (GuiFont)font;
}

/*
 * Return the name of font "font" in allocated memory.
 */
    char_u *
gui_mch_get_fontname(GuiFont font, char_u *name)
{
    return vim_strsave(((VimFont *)font)->name);
}

/*
 * Set the current text font.
 */
    void
gui_mch_set_font(
    GuiFont	font)
{
    if (gui.vimWindow->Lock()) {
	VimFont *vf = (VimFont *)font;

	gui.vimTextArea->SetFont(vf);

	gui.char_width = (int) vf->StringWidth("n");
	font_height fh;
	vf->GetHeight(&fh);
	gui.char_height = (int)(fh.ascent + 0.9999)
		    + (int)(fh.descent + 0.9999) + (int)(fh.leading + 0.9999);
	gui.char_ascent = (int)(fh.ascent + 0.9999);

	gui.vimWindow->Unlock();
    }
}

#if 0 /* not used */
/*
 * Return TRUE if the two fonts given are equivalent.
 */
    int
gui_mch_same_font(
    GuiFont	f1,
    GuiFont	f2)
{
    VimFont *vf1 = (VimFont *)f1;
    VimFont *vf2 = (VimFont *)f2;

    return f1 == f2 ||
	    (vf1->FamilyAndStyle() == vf2->FamilyAndStyle() &&
	     vf1->Size() == vf2->Size());
}
#endif

/* XXX TODO This is apparently never called... */
    void
gui_mch_free_font(
    GuiFont	font)
{
    VimFont *f = (VimFont *)font;
    if (--f->refcount <= 0) {
	if (f->refcount < 0)
	    fprintf(stderr, "VimFont: refcount < 0\n");
	delete f;
    }
}

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

/*
 * This function has been lifted from gui_w32.c and extended a bit.
 *
 * Return the Pixel value (color) for the given color name.
 * Return INVALCOLOR for error.
 */
    guicolor_T
gui_mch_get_color(
    char_u	*name)
{
    typedef struct GuiColourTable
    {
	char	    *name;
	guicolor_T     colour;
    } GuiColourTable;

#define NSTATIC_COLOURS		32
#define NDYNAMIC_COLOURS	33
#define NCOLOURS		(NSTATIC_COLOURS + NDYNAMIC_COLOURS)

    static GuiColourTable table[NCOLOURS] =
    {
	{"Black",	    RGB(0x00, 0x00, 0x00)},
	{"DarkGray",	    RGB(0x80, 0x80, 0x80)},
	{"DarkGrey",	    RGB(0x80, 0x80, 0x80)},
	{"Gray",	    RGB(0xC0, 0xC0, 0xC0)},
	{"Grey",	    RGB(0xC0, 0xC0, 0xC0)},
	{"LightGray",	    RGB(0xD3, 0xD3, 0xD3)},
	{"LightGrey",	    RGB(0xD3, 0xD3, 0xD3)},
	{"White",	    RGB(0xFF, 0xFF, 0xFF)},
	{"DarkRed",	    RGB(0x80, 0x00, 0x00)},
	{"Red",		    RGB(0xFF, 0x00, 0x00)},
	{"LightRed",	    RGB(0xFF, 0xA0, 0xA0)},
	{"DarkBlue",	    RGB(0x00, 0x00, 0x80)},
	{"Blue",	    RGB(0x00, 0x00, 0xFF)},
	{"LightBlue",	    RGB(0xA0, 0xA0, 0xFF)},
	{"DarkGreen",	    RGB(0x00, 0x80, 0x00)},
	{"Green",	    RGB(0x00, 0xFF, 0x00)},
	{"LightGreen",	    RGB(0xA0, 0xFF, 0xA0)},
	{"DarkCyan",	    RGB(0x00, 0x80, 0x80)},
	{"Cyan",	    RGB(0x00, 0xFF, 0xFF)},
	{"LightCyan",	    RGB(0xA0, 0xFF, 0xFF)},
	{"DarkMagenta",	    RGB(0x80, 0x00, 0x80)},
	{"Magenta",	    RGB(0xFF, 0x00, 0xFF)},
	{"LightMagenta",    RGB(0xFF, 0xA0, 0xFF)},
	{"Brown",	    RGB(0x80, 0x40, 0x40)},
	{"Yellow",	    RGB(0xFF, 0xFF, 0x00)},
	{"LightYellow",	    RGB(0xFF, 0xFF, 0xA0)},
	{"DarkYellow",	    RGB(0xBB, 0xBB, 0x00)},
	{"SeaGreen",	    RGB(0x2E, 0x8B, 0x57)},
	{"Orange",	    RGB(0xFF, 0xA5, 0x00)},
	{"Purple",	    RGB(0xA0, 0x20, 0xF0)},
	{"SlateBlue",	    RGB(0x6A, 0x5A, 0xCD)},
	{"Violet",	    RGB(0xEE, 0x82, 0xEE)},
    };

    static int endColour = NSTATIC_COLOURS;
    static int newColour = NSTATIC_COLOURS;

    int		    r, g, b;
    int		    i;

    if (name[0] == '#' && STRLEN(name) == 7)
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
	/* Check if the name is one of the colours we know */
	for (i = 0; i < endColour; i++)
	    if (STRICMP(name, table[i].name) == 0)
		return table[i].colour;
    }

    /*
     * Last attempt. Look in the file "$VIM/rgb.txt".
     */
    {
#define LINE_LEN 100
	FILE	*fd;
	char	line[LINE_LEN];
	char_u	*fname;

	fname = expand_env_save((char_u *)"$VIM/rgb.txt");
	if (fname == NULL)
	    return INVALCOLOR;

	fd = fopen((char *)fname, "rt");
	vim_free(fname);
	if (fd == NULL)
	    return INVALCOLOR;

	while (!feof(fd))
	{
	    int	    len;
	    int	    pos;
	    char    *colour;

	    fgets(line, LINE_LEN, fd);
	    len = strlen(line);

	    if (len <= 1 || line[len-1] != '\n')
		continue;

	    line[len-1] = '\0';

	    i = sscanf(line, "%d %d %d %n", &r, &g, &b, &pos);
	    if (i != 3)
		continue;

	    colour = line + pos;

	    if (STRICMP(colour, name) == 0)
	    {
		fclose(fd);
		/*
		 * Now remember this colour in the table.
		 * A LRU scheme might be better but this is simpler.
		 * Or could use a growing array.
		 */
		guicolor_T gcolour = RGB(r,g,b);

		vim_free(table[newColour].name);
		table[newColour].name = (char *)vim_strsave((char_u *)colour);
		table[newColour].colour = gcolour;

		newColour++;
		if (newColour >= NCOLOURS)
		    newColour = NSTATIC_COLOURS;
		if (endColour < NCOLOURS)
		    endColour = newColour;

		return gcolour;
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
gui_mch_set_fg_color(
    guicolor_T	color)
{
    rgb_color rgb = GUI_TO_RGB(color);
    if (gui.vimWindow->Lock()) {
	gui.vimTextArea->SetHighColor(rgb);
	gui.vimWindow->Unlock();
    }
}

/*
 * Set the current text background color.
 */
    void
gui_mch_set_bg_color(
    guicolor_T	color)
{
    rgb_color rgb = GUI_TO_RGB(color);
    if (gui.vimWindow->Lock()) {
	gui.vimTextArea->SetLowColor(rgb);
	gui.vimWindow->Unlock();
    }
}

    void
gui_mch_draw_string(
    int		row,
    int		col,
    char_u	*s,
    int		len,
    int		flags)
{
    if (gui.vimWindow->Lock()) {
	gui.vimTextArea->mchDrawString(row, col, s, len, flags);
	gui.vimWindow->Unlock();
    }
}

/*
 * Return OK if the key with the termcap name "name" is supported.
 */
    int
gui_mch_haskey(
    char_u	*name)
{
    int i;

    for (i = 0; special_keys[i].BeKeys != 0; i++)
	if (name[0] == special_keys[i].vim_code0 &&
					 name[1] == special_keys[i].vim_code1)
	    return OK;
    return FAIL;
}

    void
gui_mch_beep()
{
    ::beep();
}

    void
gui_mch_flash(int msec)
{
    /* Do a visual beep by reversing the foreground and background colors */

    if (gui.vimWindow->Lock()) {
	BRect rect = gui.vimTextArea->Bounds();

	gui.vimTextArea->SetDrawingMode(B_OP_INVERT);
	gui.vimTextArea->FillRect(rect);
	gui.vimTextArea->Sync();
	snooze(msec * 1000);	 /* wait for a few msec */
	gui.vimTextArea->FillRect(rect);
	gui.vimTextArea->SetDrawingMode(B_OP_COPY);
	gui.vimTextArea->Flush();
	gui.vimWindow->Unlock();
    }
}

/*
 * Invert a rectangle from row r, column c, for nr rows and nc columns.
 */
    void
gui_mch_invert_rectangle(
    int		r,
    int		c,
    int		nr,
    int		nc)
{
    BRect rect;
    rect.left = FILL_X(c);
    rect.top = FILL_Y(r);
    rect.right = rect.left + nc * gui.char_width - PEN_WIDTH;
    rect.bottom = rect.top + nr * gui.char_height - PEN_WIDTH;

    if (gui.vimWindow->Lock()) {
	gui.vimTextArea->SetDrawingMode(B_OP_INVERT);
	gui.vimTextArea->FillRect(rect);
	gui.vimTextArea->SetDrawingMode(B_OP_COPY);
	gui.vimWindow->Unlock();
    }
}

/*
 * Iconify the GUI window.
 */
    void
gui_mch_iconify()
{
    if (gui.vimWindow->Lock()) {
	gui.vimWindow->Minimize(true);
	gui.vimWindow->Unlock();
    }
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
 * Set the window title
 */
    void
gui_mch_settitle(
    char_u	*title,
    char_u	*icon)
{
    if (gui.vimWindow->Lock()) {
	gui.vimWindow->SetTitle((char *)title);
	gui.vimWindow->Unlock();
    }
}

/*
 * Draw a cursor without focus.
 */
    void
gui_mch_draw_hollow_cursor(guicolor_T color)
{
    gui_mch_set_fg_color(color);

    BRect r;
    r.left = FILL_X(gui.col);
    r.top = FILL_Y(gui.row);
    r.right = r.left + gui.char_width - PEN_WIDTH;
    r.bottom = r.top + gui.char_height - PEN_WIDTH;

    if (gui.vimWindow->Lock()) {
	gui.vimTextArea->StrokeRect(r);
	gui.vimWindow->Unlock();
	//gui_mch_flush();
    }
}

/*
 * Draw part of a cursor, only w pixels wide, and h pixels high.
 */
    void
gui_mch_draw_part_cursor(
    int		w,
    int		h,
    guicolor_T	color)
{
    gui_mch_set_fg_color(color);

    BRect r;
    r.left =
#ifdef FEAT_RIGHTLEFT
	/* vertical line should be on the right of current point */
	CURSOR_BAR_RIGHT ? FILL_X(gui.col + 1) - w :
#endif
	    FILL_X(gui.col);
    r.right = r.left + w - PEN_WIDTH;
    r.bottom = FILL_Y(gui.row + 1) - PEN_WIDTH;
    r.top = r.bottom - h + PEN_WIDTH;

    if (gui.vimWindow->Lock()) {
	gui.vimTextArea->FillRect(r);
	gui.vimWindow->Unlock();
	//gui_mch_flush();
    }
}

/*
 * Catch up with any queued events.  This may put keyboard input into the
 * input buffer, call resize call-backs, trigger timers etc.  If there is
 * nothing in the event queue (& no timers pending), then we return
 * immediately.
 */
    void
gui_mch_update()
{
    gui_mch_flush();
    while (port_count(gui.vdcmp) > 0 &&
	    !vim_is_input_buf_full() &&
	    gui_beos_process_event(0) >= B_OK)
	/* nothing */ ;
}

/*
 * GUI input routine called by gui_wait_for_chars().  Waits for a character
 * from the keyboard.
 *	wtime == -1		Wait forever.
 *	wtime == 0		This should never happen.
 *	wtime > 0		Wait wtime milliseconds for a character.
 * Returns OK if a character was found to be available within the given time,
 * or FAIL otherwise.
 */
    int
gui_mch_wait_for_chars(
    int		wtime)
{
    int		    focus;
    bigtime_t	    until, timeout;
    status_t	    st;

    if (wtime >= 0) {
	timeout = wtime * 1000;
	until = system_time() + timeout;
    } else {
	timeout = B_INFINITE_TIMEOUT;
    }

    focus = gui.in_focus;
    for (;;)
    {
	/* Stop or start blinking when focus changes */
	if (gui.in_focus != focus)
	{
	    if (gui.in_focus)
		gui_mch_start_blink();
	    else
		gui_mch_stop_blink();
	    focus = gui.in_focus;
	}

	gui_mch_flush();
	/*
	 * Don't use gui_mch_update() because then we will spin-lock until a
	 * char arrives, instead we use gui_beos_process_event() to hang until
	 * an event arrives.  No need to check for input_buf_full because we
	 * are returning as soon as it contains a single char.
	 */
	st = gui_beos_process_event(timeout);

	if (input_available())
	    return OK;
	if (st < B_OK)		    /* includes B_TIMED_OUT */
	    return FAIL;

	/*
	 * Calculate how much longer we're willing to wait for the
	 * next event.
	 */
	if (wtime >= 0) {
	    timeout = until - system_time();
	    if (timeout < 0)
		break;
	}
    }
    return FAIL;

}

/*
 * Output routines.
 */

/*
 * Flush any output to the screen. This is typically called before
 * the app goes to sleep.
 */
    void
gui_mch_flush()
{
    // does this need to lock the window? Apparently not but be safe.
    if (gui.vimWindow->Lock()) {
	gui.vimWindow->Flush();
	gui.vimWindow->Unlock();
    }
    return;
}

/*
 * Clear a rectangular region of the screen from text pos (row1, col1) to
 * (row2, col2) inclusive.
 */
    void
gui_mch_clear_block(
    int		row1,
    int		col1,
    int		row2,
    int		col2)
{
    if (gui.vimWindow->Lock()) {
	gui.vimTextArea->mchClearBlock(row1, col1, row2, col2);
	gui.vimWindow->Unlock();
    }
}

    void
gui_mch_clear_all()
{
    if (gui.vimWindow->Lock()) {
	gui.vimTextArea->mchClearAll();
	gui.vimWindow->Unlock();
    }
}

/*
 * Delete the given number of lines from the given row, scrolling up any
 * text further down within the scroll region.
 */
    void
gui_mch_delete_lines(
    int		row,
    int		num_lines)
{
    gui.vimTextArea->mchDeleteLines(row, num_lines);
}

/*
 * Insert the given number of lines before the given row, scrolling down any
 * following text within the scroll region.
 */
    void
gui_mch_insert_lines(
    int		row,
    int		num_lines)
{
    gui.vimTextArea->mchInsertLines(row, num_lines);
}

#if defined(FEAT_MENU) || defined(PROTO)
/*
 * Menu stuff.
 */

    void
gui_mch_enable_menu(
    int		flag)
{
    if (gui.vimWindow->Lock())
    {
	BMenuBar *menubar = gui.vimForm->MenuBar();
	menubar->SetEnabled(flag);
	gui.vimWindow->Unlock();
    }
}

    void
gui_mch_set_menu_pos(
    int		x,
    int		y,
    int		w,
    int		h)
{
    /* It will be in the right place anyway */
}

/*
 * Add a sub menu to the menu bar.
 */
    void
gui_mch_add_menu(
    vimmenu_T	*menu,
    int		idx)
{
    vimmenu_T	*parent = menu->parent;

    if (!menu_is_menubar(menu->name)
	    || (parent != NULL && parent->submenu_id == NULL))
	return;

    if (gui.vimWindow->Lock())
    {
/* Major re-write of the menu code, it was failing with memory corruption when
 * we started loading multiple files (the Buffer menu)
 *
 * Note we don't use the preference values yet, all are inserted into the
 * menubar on a first come-first served basis...
 *
 * richard@whitequeen.com jul 99
 */

	BMenu *tmp;

	if ( parent )
	    tmp = parent->submenu_id;
	else
	    tmp = gui.vimForm->MenuBar();
// make sure we don't try and add the same menu twice. The Buffers menu tries to
// do this and Be starts to crash...

	if ( ! tmp->FindItem((const char *) menu->dname)) {

	    BMenu *bmenu = new BMenu((char *)menu->dname);

	    menu->submenu_id = bmenu;

// when we add a BMenu to another Menu, it creates the interconnecting BMenuItem
	    tmp->AddItem(bmenu);

// Now its safe to query the menu for the associated MenuItem....
	    menu->id = tmp->FindItem((const char *) menu->dname);

	}
	gui.vimWindow->Unlock();
    }
}

    void
gui_mch_toggle_tearoffs(int enable)
{
    /* no tearoff menus */
}

    static BMessage *
MenuMessage(vimmenu_T *menu)
{
    BMessage *m = new BMessage('menu');
    m->AddPointer("VimMenu", (void *)menu);

    return m;
}

/*
 * Add a menu item to a menu
 */
    void
gui_mch_add_menu_item(
    vimmenu_T	*menu,
    int		idx)
{
    int		mnemonic = 0;
    vimmenu_T	*parent = menu->parent;

    if (parent->submenu_id == NULL)
	return;

#ifdef never
    /* why not add separators ?
     * richard
     */
    /* Don't add menu separator */
    if (menu_is_separator(menu->name))
	return;
#endif

    /* TODO: use menu->actext */
    /* This is difficult, since on Be, an accelerator must be a single char
     * and a lot of Vim ones are the standard VI commands.
     *
     * Punt for Now...
     * richard@whiequeen.com jul 99
     */
    if (gui.vimWindow->Lock())
    {
	if ( menu_is_separator(menu->name)) {
	    BSeparatorItem *item = new BSeparatorItem();
	    parent->submenu_id->AddItem(item);
	    menu->id = item;
	    menu->submenu_id = NULL;
	}
	else {
	    BMenuItem *item = new BMenuItem((char *)menu->dname,
		    MenuMessage(menu));
	    item->SetTarget(gui.vimTextArea);
	    item->SetTrigger((char) menu->mnemonic);
	    parent->submenu_id->AddItem(item);
	    menu->id = item;
	    menu->submenu_id = NULL;
	}
	gui.vimWindow->Unlock();
    }
}

/*
 * Destroy the machine specific menu widget.
 */
    void
gui_mch_destroy_menu(
    vimmenu_T	*menu)
{
    if (gui.vimWindow->Lock())
    {
	assert(menu->submenu_id == NULL || menu->submenu_id->CountItems() == 0);
	/*
	 * Detach this menu from its parent, so that it is not deleted
	 * twice once we get to delete that parent.
	 * Deleting a BMenuItem also deletes the associated BMenu, if any
	 * (which does not have any items anymore since they were
	 * removed and deleted before).
	 */
	BMenu *bmenu = menu->id->Menu();
	if (bmenu)
	{
	    bmenu->RemoveItem(menu->id);
	    /*
	     * If we removed the last item from the menu bar,
	     * resize it out of sight.
	     */
	    if (bmenu == gui.vimForm->MenuBar() && bmenu->CountItems() == 0)
	    {
		bmenu->ResizeTo(-MENUBAR_MARGIN, -MENUBAR_MARGIN);
	    }
	}
	delete menu->id;
	menu->id = NULL;
	menu->submenu_id = NULL;

	gui.menu_height = (int) gui.vimForm->MenuHeight();
	gui.vimWindow->Unlock();
    }
}

/*
 * Make a menu either grey or not grey.
 */
    void
gui_mch_menu_grey(
    vimmenu_T	*menu,
    int		grey)
{
    if (menu->id != NULL)
	menu->id->SetEnabled(!grey);
}

/*
 * Make menu item hidden or not hidden
 */
    void
gui_mch_menu_hidden(
    vimmenu_T	*menu,
    int		hidden)
{
    if (menu->id != NULL)
	menu->id->SetEnabled(!hidden);
}

/*
 * This is called after setting all the menus to grey/hidden or not.
 */
    void
gui_mch_draw_menubar()
{
    /* Nothing to do in BeOS */
}

#endif /* FEAT_MENU */

/* Mouse stuff */

#ifdef FEAT_CLIPBOARD
/*
 * Clipboard stuff, for cutting and pasting text to other windows.
 */
char textplain[] = "text/plain";
char vimselectiontype[] = "application/x-vnd.Rhialto-Vim-selectiontype";

/*
 * Get the current selection and put it in the clipboard register.
 */
    void
clip_mch_request_selection(VimClipboard *cbd)
{
    if (be_clipboard->Lock())
    {
	BMessage *m = be_clipboard->Data();
	//m->PrintToStream();

	char_u *string = NULL;
	ssize_t stringlen = -1;

	if (m->FindData(textplain, B_MIME_TYPE,
				   (const void **)&string, &stringlen) == B_OK
		|| m->FindString("text", (const char **)&string) == B_OK)
	{
	    if (stringlen == -1)
		stringlen = STRLEN(string);

	    int type;
	    char *seltype;
	    ssize_t seltypelen;

	    /*
	     * Try to get the special vim selection type first
	     */
	    if (m->FindData(vimselectiontype, B_MIME_TYPE,
		    (const void **)&seltype, &seltypelen) == B_OK)
	    {
		switch (*seltype)
		{
		    default:
		    case 'L':	type = MLINE;	break;
		    case 'C':	type = MCHAR;	break;
#ifdef FEAT_VISUAL
		    case 'B':	type = MBLOCK;	break;
#endif
		}
	    }
	    else
	    {
		/* Otherwise use heuristic as documented */
		type = memchr(string, stringlen, '\n') ? MLINE : MCHAR;
	    }
	    clip_yank_selection(type, string, (long)stringlen, cbd);
	}
	be_clipboard->Unlock();
    }
}
/*
 * Make vim the owner of the current selection.
 */
    void
clip_mch_lose_selection(VimClipboard *cbd)
{
    /* Nothing needs to be done here */
}

/*
 * Make vim the owner of the current selection.  Return OK upon success.
 */
    int
clip_mch_own_selection(VimClipboard *cbd)
{
    /*
     * Never actually own the clipboard.  If another application sets the
     * clipboard, we don't want to think that we still own it.
     */
    return FAIL;
}

/*
 * Send the current selection to the clipboard.
 */
    void
clip_mch_set_selection(VimClipboard *cbd)
{
    if (be_clipboard->Lock())
    {
	be_clipboard->Clear();
	BMessage *m = be_clipboard->Data();
	assert(m);

	/* If the '*' register isn't already filled in, fill it in now */
	cbd->owned = TRUE;
	clip_get_selection(cbd);
	cbd->owned = FALSE;

	char_u  *str = NULL;
	long_u  count;
	int	type;

	type = clip_convert_selection(&str, &count, cbd);

	if (type < 0)
	    return;

	m->AddData(textplain, B_MIME_TYPE, (void *)str, count);

	/* Add type of selection */
	char    vtype;
	switch (type)
	{
	    default:
	    case MLINE:    vtype = 'L';    break;
	    case MCHAR:    vtype = 'C';    break;
#ifdef FEAT_VISUAL
	    case MBLOCK:   vtype = 'B';    break;
#endif
	}
	m->AddData(vimselectiontype, B_MIME_TYPE, (void *)&vtype, 1);

	vim_free(str);

	be_clipboard->Commit();
	be_clipboard->Unlock();
    }
}

#endif	/* FEAT_CLIPBOARD */

/*
 * Return the RGB value of a pixel as long.
 */
    long_u
gui_mch_get_rgb(guicolor_T pixel)
{
    rgb_color rgb = GUI_TO_RGB(pixel);

    return ((rgb.red & 0xff) << 16) + ((rgb.green & 0xff) << 8)
							  + (rgb.blue & 0xff);
}

    void
gui_mch_setmouse(int x, int y)
{
    TRACE();
    /* TODO */
}

    void
gui_mch_show_popupmenu(vimmenu_T *menu)
{
    TRACE();
    /* TODO */
}

int
gui_mch_get_mouse_x()
{
    TRACE();
    return 0;
}


int
gui_mch_get_mouse_y()
{
    TRACE();
    return 0;
}

} /* extern "C" */
