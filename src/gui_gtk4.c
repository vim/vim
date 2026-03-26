/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 *
 * GTK4 GUI implementation: main window, events, drawing, menus,
 * This is a clean implementation for GTK4, separate from gui_gtk_x11.c
 * which handles GTK2/GTK3. Also includes scrollbars, dialogs, and toolbar.
 *
 * GTK4 differences from GTK3:
 * - No GdkWindow (use GdkSurface for top-level only)
 * - No GtkContainer (use gtk_widget_set_parent/gtk_box_append)
 * - Events via GtkEventController, not signal+mask
 * - Drawing via GtkSnapshot or gtk_drawing_area_set_draw_func
 * - No gtk_dialog_run (async dialogs)
 * - No GdkAtom (string-based content types)
 * - No GtkSocket/GtkPlug
 * - gtk_window_new() takes no arguments
 */

#include "vim.h"

#ifdef FEAT_GUI_GTK

#include <gdk/gdkkeysyms.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include "gui_gtk4_f.h"

/*
 * Geometry string parser, replacing XParseGeometry to remove X11 dependency.
 * Format: [WIDTHxHEIGHT][{+-}XOFF{+-}YOFF]
 */
#define NoValue		0x0000
#define XValue		0x0001
#define YValue		0x0002
#define WidthValue	0x0004
#define HeightValue	0x0008
#define XNegative	0x0010
#define YNegative	0x0020

    static int
vim_parse_geometry(const char *str, int *x, int *y,
	unsigned int *width, unsigned int *height)
{
    int mask = NoValue;
    char *end;

    if (str == NULL || *str == NUL)
	return mask;

    // Parse width
    if (*str != '+' && *str != '-')
    {
	*width = (unsigned int)strtol(str, &end, 10);
	if (end != str)
	{
	    mask |= WidthValue;
	    str = end;
	}
    }

    // Parse 'x' or 'X' separator and height
    if (*str == 'x' || *str == 'X')
    {
	str++;
	*height = (unsigned int)strtol(str, &end, 10);
	if (end != str)
	{
	    mask |= HeightValue;
	    str = end;
	}
    }

    // Parse x offset
    if (*str == '+' || *str == '-')
    {
	if (*str == '-')
	    mask |= XNegative;
	str++;
	*x = (int)strtol(str, &end, 10);
	if (mask & XNegative)
	    *x = -*x;
	if (end != str)
	{
	    mask |= XValue;
	    str = end;
	}
    }

    // Parse y offset
    if (*str == '+' || *str == '-')
    {
	if (*str == '-')
	    mask |= YNegative;
	str++;
	*y = (int)strtol(str, &end, 10);
	if (mask & YNegative)
	    *y = -*y;
	if (end != str)
	    mask |= YValue;
    }

    return mask;
}

#ifdef FEAT_SOCKETSERVER
# include <glib-unix.h>

// Used to track the source for the listening socket
static guint socket_server_source_id = 0;
#endif

#if defined(FEAT_MOUSESHAPE)
// Last set mouse pointer shape
static int last_shape = 0;
#endif

#define DEFAULT_FONT	"Monospace 10"

// Cursor blinking state
static enum {
    BLINK_NONE,
    BLINK_OFF,
    BLINK_ON
} blink_state = BLINK_NONE;

// GTK4 main loop compatibility
static int gtk4_main_loop_level = 0;
static int gtk4_main_loop_quit = FALSE;

#ifdef USE_GRESOURCE
# include "auto/gui_gtk_gresources.h"
#endif

typedef gboolean timeout_cb_type;

/*
 * Table of special key mappings.
 */
static struct special_key
{
    guint key_sym;
    char_u code0;
    char_u code1;
}
const special_keys[] =
{
    {GDK_KEY_Up,	'k', 'u'},
    {GDK_KEY_Down,	'k', 'd'},
    {GDK_KEY_Left,	'k', 'l'},
    {GDK_KEY_Right,	'k', 'r'},
    {GDK_KEY_F1,	'k', '1'},
    {GDK_KEY_F2,	'k', '2'},
    {GDK_KEY_F3,	'k', '3'},
    {GDK_KEY_F4,	'k', '4'},
    {GDK_KEY_F5,	'k', '5'},
    {GDK_KEY_F6,	'k', '6'},
    {GDK_KEY_F7,	'k', '7'},
    {GDK_KEY_F8,	'k', '8'},
    {GDK_KEY_F9,	'k', '9'},
    {GDK_KEY_F10,	'k', ';'},
    {GDK_KEY_F11,	'F', '1'},
    {GDK_KEY_F12,	'F', '2'},
    {GDK_KEY_Help,	'%', '1'},
    {GDK_KEY_Undo,	'&', '8'},
    {GDK_KEY_BackSpace,	'k', 'b'},
    {GDK_KEY_Insert,	'k', 'I'},
    {GDK_KEY_Delete,	'k', 'D'},
    {GDK_KEY_Home,	'k', 'h'},
    {GDK_KEY_End,	'@', '7'},
    {GDK_KEY_Prior,	'k', 'P'},
    {GDK_KEY_Next,	'k', 'N'},
    {GDK_KEY_Print,	'%', '9'},
    {GDK_KEY_KP_Left,	'k', 'l'},
    {GDK_KEY_KP_Right,	'k', 'r'},
    {GDK_KEY_KP_Up,	'k', 'u'},
    {GDK_KEY_KP_Down,	'k', 'd'},
    {GDK_KEY_KP_Insert,	KS_EXTRA, (char_u)KE_KINS},
    {GDK_KEY_KP_Delete,	KS_EXTRA, (char_u)KE_KDEL},
    {GDK_KEY_KP_Home,	'K', '1'},
    {GDK_KEY_KP_End,	'K', '4'},
    {GDK_KEY_KP_Prior,	'K', '3'},
    {GDK_KEY_KP_Next,	'K', '5'},
    {GDK_KEY_KP_Add,	'K', '6'},
    {GDK_KEY_KP_Subtract, 'K', '7'},
    {GDK_KEY_KP_Divide,	'K', '8'},
    {GDK_KEY_KP_Multiply, 'K', '9'},
    {GDK_KEY_KP_Enter,	'K', 'A'},
    {GDK_KEY_KP_Decimal,	'K', 'B'},
    {GDK_KEY_KP_0,	'K', 'C'},
    {GDK_KEY_KP_1,	'K', 'D'},
    {GDK_KEY_KP_2,	'K', 'E'},
    {GDK_KEY_KP_3,	'K', 'F'},
    {GDK_KEY_KP_4,	'K', 'G'},
    {GDK_KEY_KP_5,	'K', 'H'},
    {GDK_KEY_KP_6,	'K', 'I'},
    {GDK_KEY_KP_7,	'K', 'J'},
    {GDK_KEY_KP_8,	'K', 'K'},
    {GDK_KEY_KP_9,	'K', 'L'},
    {0, 0, 0}
};

    static int
keyval_to_string(unsigned int keyval, char_u *string)
{
    int		len;
    guint32	uc;

    uc = gdk_keyval_to_unicode(keyval);
    if (uc != 0)
    {
	len = utf_char2bytes((int)uc, string);
    }
    else
    {
	len = 1;
	switch (keyval)
	{
	    case GDK_KEY_Tab: case GDK_KEY_KP_Tab: case GDK_KEY_ISO_Left_Tab:
		string[0] = TAB;
		break;
	    case GDK_KEY_Linefeed:
		string[0] = NL;
		break;
	    case GDK_KEY_Return: case GDK_KEY_ISO_Enter: case GDK_KEY_3270_Enter:
		string[0] = CAR;
		break;
	    case GDK_KEY_Escape:
		string[0] = ESC;
		break;
	    default:
		len = 0;
		break;
	}
    }
    string[len] = NUL;
    return len;
}

    static int
modifiers_gdk2vim(guint state)
{
    int modifiers = 0;

    if (state & GDK_SHIFT_MASK)
	modifiers |= MOD_MASK_SHIFT;
    if (state & GDK_CONTROL_MASK)
	modifiers |= MOD_MASK_CTRL;
    if (state & GDK_ALT_MASK)
	modifiers |= MOD_MASK_ALT;
    if (state & GDK_META_MASK)
	modifiers |= MOD_MASK_META;
    if (state & GDK_SUPER_MASK)
	modifiers |= MOD_MASK_CMD;

    return modifiers;
}

static GtkWidget *vbox;		// the main vertical box

// Forward declarations for event callbacks
static void draw_event(GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer data);
static gboolean key_press_event(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data);
static void key_release_event(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer data);
static void button_press_event(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
static void button_release_event(GtkGestureClick *gesture, int n_press, double x, double y, gpointer data);
static void motion_notify_event(GtkEventControllerMotion *controller, double x, double y, gpointer data);
static void enter_notify_event(GtkEventControllerMotion *controller, double x, double y, gpointer data);
static void leave_notify_event(GtkEventControllerMotion *controller, gpointer data);
static gboolean scroll_event(GtkEventControllerScroll *controller, double dx, double dy, gpointer data);
static void focus_in_event(GtkEventControllerFocus *controller, gpointer data);
static void focus_out_event(GtkEventControllerFocus *controller, gpointer data);
#ifdef FEAT_DND
static gboolean drop_cb(GtkDropTarget *target, const GValue *value, double x, double y, gpointer data);
#endif
static void mainwin_destroy_cb(GObject *object, gpointer data);
static gboolean delete_event_cb(GtkWindow *window, gpointer data);
static void drawarea_realize_cb(GtkWidget *widget, gpointer data);
static void drawarea_unrealize_cb(GtkWidget *widget, gpointer data);
static void drawarea_resize_cb(GtkDrawingArea *area, int width, int height, gpointer data);

/*
 * Parse the GUI related command-line arguments.  Any arguments used are
 * deleted from argv, and *argc is decremented accordingly.  This is called
 * when vim is started, whether or not the GUI has been started.
 */
    void
gui_mch_prepare(int *argc, char **argv)
{
    // If GSK_RENDERER is not set, try cairo first since GL/Vulkan may not
    // be available (e.g., WSL2).
    if (g_getenv("GSK_RENDERER") == NULL)
	g_setenv("GSK_RENDERER", "cairo", FALSE);

    // Suppress noisy EGL warnings when GL is not available.
    if (g_getenv("EGL_LOG_LEVEL") == NULL)
	setenv("EGL_LOG_LEVEL", "fatal", 0);

    // Let GTK4 choose the best backend (Wayland or X11).

    gtk_init();
}

/*
 * Free all GUI related resources.
 */
    void
gui_mch_free_all(void)
{
}

    static guint
timeout_add(int time, timeout_cb_type (*callback)(gpointer), int *flagp)
{
    return g_timeout_add((guint)time, (GSourceFunc)callback, flagp);
}

    static void
timeout_remove(guint timer)
{
    g_source_remove(timer);
}

static long_u blink_waittime = 700;
static long_u blink_ontime = 400;
static long_u blink_offtime = 250;
static guint blink_timer = 0;

    static timeout_cb_type
blink_cb(gpointer data UNUSED)
{
    if (blink_state == BLINK_ON)
    {
	gui_undraw_cursor();
	blink_state = BLINK_OFF;
	blink_timer = timeout_add(blink_offtime, blink_cb, NULL);
    }
    else
    {
	gui_update_cursor(TRUE, FALSE);
	blink_state = BLINK_ON;
	blink_timer = timeout_add(blink_ontime, blink_cb, NULL);
    }
    return FALSE;
}

    int
gui_mch_is_blinking(void)
{
    return blink_state != BLINK_NONE;
}

    int
gui_mch_is_blink_off(void)
{
    return blink_state == BLINK_OFF;
}

    void
gui_mch_set_blinking(long waittime, long on, long off)
{
    blink_waittime = waittime;
    blink_ontime = on;
    blink_offtime = off;
}

    void
gui_mch_stop_blink(int may_call_gui_update_cursor)
{
    if (blink_timer)
    {
	timeout_remove(blink_timer);
	blink_timer = 0;
    }
    if (blink_state == BLINK_OFF && may_call_gui_update_cursor)
	gui_update_cursor(TRUE, FALSE);
    blink_state = BLINK_NONE;
}

    void
gui_mch_start_blink(void)
{
    if (blink_timer)
    {
	timeout_remove(blink_timer);
	blink_timer = 0;
    }
    if (blink_waittime && blink_ontime && blink_offtime && gui.in_focus)
    {
	blink_timer = timeout_add(blink_waittime, blink_cb, NULL);
	blink_state = BLINK_ON;
	gui_update_cursor(TRUE, FALSE);
    }
}

    int
gui_mch_early_init_check(int give_message UNUSED)
{
    return OK;
}

    int
gui_mch_init_check(void)
{
    return OK;
}

/*
 * Initialise the GUI.  Create all the windows, set up all the callbacks etc.
 * Returns OK for success, FAIL when the GUI can't be started.
 */
    int
gui_mch_init(void)
{
    // Allocate GdkRGBA color structs.
    gui.fgcolor = g_new(GdkRGBA, 1);
    gui.bgcolor = g_new(GdkRGBA, 1);
    gui.spcolor = g_new(GdkRGBA, 1);

    gui.def_norm_pixel = 0x00000000;	// black
    gui.def_back_pixel = 0x00ffffff;	// white
    gui.norm_pixel = gui.def_norm_pixel;
    gui.back_pixel = gui.def_back_pixel;

    gui.scrollbar_width = SB_DEFAULT_WIDTH;
    gui.scrollbar_height = SB_DEFAULT_WIDTH;

    // Create the main window.
    gui.mainwin = gtk_window_new();
    gtk_widget_set_name(gui.mainwin, "vim-main-window");

    // Create the PangoContext used for drawing all text.
    gui.text_context = gtk_widget_create_pango_context(gui.mainwin);
    pango_context_set_base_dir(gui.text_context, PANGO_DIRECTION_LTR);

    g_signal_connect(G_OBJECT(gui.mainwin), "close-request",
		     G_CALLBACK(delete_event_cb), NULL);

    // A vertical box holds the menubar, toolbar and main text window.
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);
    gtk_window_set_child(GTK_WINDOW(gui.mainwin), vbox);

#ifdef FEAT_MENU
    {
	GMenu *gmenu = g_menu_new();
	gui.menubar = gtk_popover_menu_bar_new_from_model(
		G_MENU_MODEL(gmenu));
	g_object_set_data_full(G_OBJECT(gui.menubar), "vim-gmenu",
		gmenu, g_object_unref);
	gtk_widget_set_name(gui.menubar, "vim-menubar");
	gtk_widget_set_visible(gui.menubar, FALSE);
	gtk_box_append(GTK_BOX(vbox), gui.menubar);
    }
#endif

#ifdef FEAT_TOOLBAR
    gui.toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(gui.toolbar, "vim-toolbar");
    gtk_widget_set_visible(gui.toolbar, FALSE);
    gtk_box_append(GTK_BOX(vbox), gui.toolbar);
#endif

#ifdef FEAT_GUI_TABLINE
    gui.tabline = gtk_notebook_new();
    gtk_notebook_set_show_border(GTK_NOTEBOOK(gui.tabline), FALSE);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(gui.tabline), FALSE);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(gui.tabline), TRUE);
    gtk_widget_set_visible(gui.tabline, FALSE);
    gtk_box_append(GTK_BOX(vbox), gui.tabline);
#endif

    // The form widget manages absolute positioning of scrollbars.
    gui.formwin = gui_gtk_form_new();
    gtk_widget_set_name(gui.formwin, "vim-gtk-form");

    // The drawing area for the editor content.
    // Placed in an overlay so it fills the formwin, with scrollbars on top.
    gui.drawarea = gtk_drawing_area_new();
    gui.surface = NULL;
    gtk_widget_set_focusable(gui.drawarea, TRUE);
    gtk_widget_set_vexpand(gui.drawarea, TRUE);
    gtk_widget_set_hexpand(gui.drawarea, TRUE);

    {
	// Use GtkOverlay: drawarea as the main child, formwin as overlay
	GtkWidget *overlay = gtk_overlay_new();
	gtk_overlay_set_child(GTK_OVERLAY(overlay), gui.drawarea);
	gtk_overlay_add_overlay(GTK_OVERLAY(overlay), gui.formwin);
	gtk_widget_set_vexpand(overlay, TRUE);
	gtk_widget_set_hexpand(overlay, TRUE);
	gtk_box_append(GTK_BOX(vbox), overlay);
    }

    // Set up drawing.
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(gui.drawarea),
	    (GtkDrawingAreaDrawFunc)draw_event, NULL, NULL);

    g_signal_connect(G_OBJECT(gui.drawarea), "realize",
		     G_CALLBACK(drawarea_realize_cb), NULL);
    g_signal_connect(G_OBJECT(gui.drawarea), "unrealize",
		     G_CALLBACK(drawarea_unrealize_cb), NULL);
    g_signal_connect(G_OBJECT(gui.drawarea), "resize",
		     G_CALLBACK(drawarea_resize_cb), NULL);

    // Set up event controllers.
    {
	GtkEventController *key_ctrl = gtk_event_controller_key_new();
	g_signal_connect(key_ctrl, "key-pressed",
			 G_CALLBACK(key_press_event), NULL);
	g_signal_connect(key_ctrl, "key-released",
			 G_CALLBACK(key_release_event), NULL);
	gtk_widget_add_controller(gui.mainwin, key_ctrl);
    }

    {
	GtkGesture *click = gtk_gesture_click_new();
	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(click), 0);
	g_signal_connect(click, "pressed",
			 G_CALLBACK(button_press_event), NULL);
	g_signal_connect(click, "released",
			 G_CALLBACK(button_release_event), NULL);
	gtk_widget_add_controller(gui.drawarea, GTK_EVENT_CONTROLLER(click));
    }

    {
	GtkEventController *motion = gtk_event_controller_motion_new();
	g_signal_connect(motion, "motion",
			 G_CALLBACK(motion_notify_event), NULL);
	g_signal_connect(motion, "enter",
			 G_CALLBACK(enter_notify_event), NULL);
	g_signal_connect(motion, "leave",
			 G_CALLBACK(leave_notify_event), NULL);
	gtk_widget_add_controller(gui.drawarea, motion);
    }

    {
	GtkEventController *scroll = gtk_event_controller_scroll_new(
		GTK_EVENT_CONTROLLER_SCROLL_BOTH_AXES);
	g_signal_connect(scroll, "scroll",
			 G_CALLBACK(scroll_event), NULL);
	gtk_widget_add_controller(gui.drawarea, scroll);
    }

    {
	GtkEventController *focus = gtk_event_controller_focus_new();
	g_signal_connect(focus, "enter",
			 G_CALLBACK(focus_in_event), NULL);
	g_signal_connect(focus, "leave",
			 G_CALLBACK(focus_out_event), NULL);
	gtk_widget_add_controller(gui.drawarea, focus);
    }

#ifdef FEAT_DND
    // Set up drag-and-drop target for files and text.
    {
	GtkDropTarget *drop = gtk_drop_target_new(G_TYPE_INVALID, GDK_ACTION_COPY);
	GType types[] = { GDK_TYPE_FILE_LIST, G_TYPE_STRING };
	gtk_drop_target_set_gtypes(drop, types, 2);
	g_signal_connect(drop, "drop",
			 G_CALLBACK(drop_cb), NULL);
	gtk_widget_add_controller(gui.drawarea, GTK_EVENT_CONTROLLER(drop));
    }
#endif

    gui.border_offset = gui.border_width;

    // Create a blank (invisible) cursor for hiding the mouse pointer.
    gui.blank_pointer = gdk_cursor_new_from_name("none", NULL);

    return OK;
}

/*
 * Called when the foreground or background color has been changed.
 */
    static void
surface_fill_bg(void)
{
    if (gui.surface != NULL)
    {
	cairo_t *cr = cairo_create(gui.surface);
	cairo_set_source_rgba(cr,
		gui.bgcolor->red, gui.bgcolor->green,
		gui.bgcolor->blue, gui.bgcolor->alpha);
	cairo_paint(cr);
	cairo_destroy(cr);
    }
}

    void
gui_mch_new_colors(void)
{
    surface_fill_bg();
    if (gui.drawarea != NULL && gtk_widget_get_realized(gui.drawarea))
	gtk_widget_queue_draw(gui.drawarea);
}

/*
 * Open the GUI window which was created by a call to gui_mch_init().
 */
    int
gui_mch_open(void)
{
    guicolor_T fg_pixel = INVALCOLOR;
    guicolor_T bg_pixel = INVALCOLOR;
    guint pixel_width;
    guint pixel_height;

    if (gui.geom != NULL)
    {
	int		mask;
	unsigned int	w, h;
	int		x = 0;
	int		y = 0;

	mask = vim_parse_geometry((char *)gui.geom, &x, &y, &w, &h);

	if (mask & WidthValue)
	    Columns = w;
	if (mask & HeightValue)
	{
	    if (p_window > (long)h - 1 || !option_was_set((char_u *)"window"))
		p_window = h - 1;
	    Rows = h;
	}
	limit_screen_size();

	VIM_CLEAR(gui.geom);
    }

    // Use 80x24 as the default GUI size, unless geometry was specified.
    if (Columns > 80 && gui.geom == NULL)
	Columns = 80;
    if (Rows > 24 && gui.geom == NULL)
	Rows = 24;
    pixel_width = (guint)(gui_get_base_width() + Columns * gui.char_width);
    pixel_height = (guint)(gui_get_base_height() + Rows * gui.char_height);
    gtk_window_set_default_size(GTK_WINDOW(gui.mainwin),
	    pixel_width, pixel_height);

    if (foreground_argument != NULL)
	fg_pixel = gui_get_color((char_u *)foreground_argument);
    if (fg_pixel == INVALCOLOR)
	fg_pixel = gui_get_color((char_u *)"Black");

    if (background_argument != NULL)
	bg_pixel = gui_get_color((char_u *)background_argument);
    if (bg_pixel == INVALCOLOR)
	bg_pixel = gui_get_color((char_u *)"White");

    if (found_reverse_arg)
    {
	gui.def_norm_pixel = bg_pixel;
	gui.def_back_pixel = fg_pixel;
    }
    else
    {
	gui.def_norm_pixel = fg_pixel;
	gui.def_back_pixel = bg_pixel;
    }

    set_normal_colors();
    gui_check_colors();
    highlight_gui_started();

    g_signal_connect(G_OBJECT(gui.mainwin), "destroy",
		     G_CALLBACK(mainwin_destroy_cb), NULL);
    // Resize is handled by GtkForm's size_allocate callback.

    gtk_widget_show(gui.mainwin);

    // Make sure the drawing area gets keyboard focus.
    gtk_widget_grab_focus(gui.drawarea);
    gui_focus_change(TRUE);

    return OK;
}

    void
gui_mch_exit(int rc UNUSED)
{
    if (gui.mainwin != NULL)
	gtk_window_destroy(GTK_WINDOW(gui.mainwin));
}

    int
gui_mch_get_winpos(int *x, int *y)
{
    // GTK4/Wayland: window positioning not available
    *x = 0;
    *y = 0;
    return OK;
}

    void
gui_mch_set_winpos(int x UNUSED, int y UNUSED)
{
    // GTK4/Wayland: window positioning not available
}

    int
gui_mch_maximized(void)
{
    return gtk_window_is_maximized(GTK_WINDOW(gui.mainwin));
}

    void
gui_mch_unmaximize(void)
{
    if (gui.mainwin != NULL)
	gtk_window_unmaximize(GTK_WINDOW(gui.mainwin));
}

    void
gui_mch_newfont(void)
{
    gui_set_shellsize(FALSE, TRUE, RESIZE_BOTH);
}

    void
gui_mch_settitle(char_u *title, char_u *icon UNUSED)
{
    if (title != NULL && gui.mainwin != NULL)
	gtk_window_set_title(GTK_WINDOW(gui.mainwin), (const char *)title);
}

static int in_set_shellsize = FALSE;

    void
gui_mch_set_shellsize(int width, int height,
	int min_width UNUSED, int min_height UNUSED,
	int base_width UNUSED, int base_height UNUSED,
	int direction UNUSED)
{
    // Only set window size if it hasn't been shown yet (initial sizing).
    // After that, the window size is controlled by the user/WM and
    // Vim adapts to it via form_size_allocate -> gui_resize_shell.
    if (!gtk_widget_get_realized(gui.mainwin))
    {
	width += get_menu_tool_width();
	height += get_menu_tool_height();
	gtk_window_set_default_size(GTK_WINDOW(gui.mainwin), width, height);
    }
}

    void
gui_mch_get_screen_dimensions(int *screen_w, int *screen_h)
{
    GdkDisplay *display = gtk_widget_get_display(gui.mainwin);
    GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(gui.mainwin));

    if (surface != NULL)
    {
	GdkMonitor *monitor = gdk_display_get_monitor_at_surface(display,
		surface);
	if (monitor != NULL)
	{
	    GdkRectangle geom;
	    gdk_monitor_get_geometry(monitor, &geom);
	    *screen_w = geom.width;
	    *screen_h = geom.height;
	    return;
	}
    }

    *screen_w = 800;
    *screen_h = 600;
}

#ifdef FEAT_MENU
    void
gui_mch_enable_menu(int showit)
{
    if (gui.menubar != NULL)
	gtk_widget_set_visible(gui.menubar, showit);
}
#endif

#ifdef FEAT_TOOLBAR
    void
gui_mch_show_toolbar(int showit)
{
    if (gui.toolbar != NULL)
	gtk_widget_set_visible(gui.toolbar, showit);
}
#endif

    void
gui_mch_set_dark_theme(int dark)
{
    // GTK4: use GtkSettings
    GtkSettings *settings = gtk_settings_get_default();
    if (settings != NULL)
	g_object_set(settings, "gtk-application-prefer-dark-theme",
		(gboolean)dark, NULL);
}

/*
 * ============================================================
 * Font handling
 * ============================================================
 */

    int
gui_mch_adjust_charheight(void)
{
    PangoFontMetrics *metrics;
    int ascent;
    int descent;

    metrics = pango_context_get_metrics(gui.text_context, gui.norm_font,
			    pango_context_get_language(gui.text_context));
    ascent = pango_font_metrics_get_ascent(metrics);
    descent = pango_font_metrics_get_descent(metrics);
    pango_font_metrics_unref(metrics);

    gui.char_height = (ascent + descent + (PANGO_SCALE * 15) / 16)
						   / PANGO_SCALE + p_linespace;
    gui.char_ascent = PANGO_PIXELS(ascent + p_linespace * PANGO_SCALE / 2);
    gui.char_ascent = MAX(gui.char_ascent, 0);
    gui.char_height = MAX(gui.char_height, gui.char_ascent + 1);

    return OK;
}

typedef struct {
    PangoFontDescription    *result;
    gboolean		    done;
} FontDialogData;

    static void
font_dialog_finish_cb(GObject *source, GAsyncResult *res, gpointer data)
{
    FontDialogData *fdd = (FontDialogData *)data;
    fdd->result = gtk_font_dialog_choose_font_finish(
		    GTK_FONT_DIALOG(source), res, NULL);
    fdd->done = TRUE;
}

    static gboolean
font_filter(gpointer item, gpointer data UNUSED)
{
    if (PANGO_IS_FONT_FAMILY(item))
	return pango_font_family_is_monospace(PANGO_FONT_FAMILY(item));
    if (PANGO_IS_FONT_FACE(item))
    {
	PangoFontFamily *family = pango_font_face_get_family(
		PANGO_FONT_FACE(item));
	if (family != NULL)
	    return pango_font_family_is_monospace(family);
    }
    return TRUE;
}

    char_u *
gui_mch_font_dialog(char_u *oldval)
{
    GtkFontDialog	*dlg;
    PangoFontDescription *initial = NULL;
    char_u		*fontname = NULL;
    FontDialogData	fdd;

    dlg = gtk_font_dialog_new();
    gtk_font_dialog_set_modal(dlg, TRUE);
    gtk_font_dialog_set_filter(dlg,
	    GTK_FILTER(gtk_custom_filter_new(
		    (GtkCustomFilterFunc)font_filter, NULL, NULL)));

    if (oldval != NULL && oldval[0] != NUL)
    {
	char_u *oldname;

	if (output_conv.vc_type != CONV_NONE)
	    oldname = string_convert(&output_conv, oldval, NULL);
	else
	    oldname = oldval;

	if (!vim_isdigit(oldname[STRLEN(oldname) - 1]))
	{
	    char_u *p = vim_strnsave(oldname, STRLEN(oldname) + 3);
	    if (p != NULL)
	    {
		STRCPY(p + STRLEN(p), " 10");
		if (oldname != oldval)
		    vim_free(oldname);
		oldname = p;
	    }
	}

	initial = pango_font_description_from_string((const char *)oldname);
	if (oldname != oldval)
	    vim_free(oldname);
    }
    else
	initial = pango_font_description_from_string(DEFAULT_FONT);

    fdd.result = NULL;
    fdd.done = FALSE;

    gtk_font_dialog_choose_font(dlg, GTK_WINDOW(gui.mainwin),
	    initial, NULL, font_dialog_finish_cb, &fdd);

    while (!fdd.done)
	g_main_context_iteration(NULL, TRUE);

    if (fdd.result != NULL)
    {
	char *name = pango_font_description_to_string(fdd.result);
	if (name != NULL)
	{
	    char_u *p;

	    p = vim_strsave_escaped((char_u *)name, (char_u *)",");
	    g_free(name);
	    if (p != NULL && input_conv.vc_type != CONV_NONE)
	    {
		fontname = string_convert(&input_conv, p, NULL);
		vim_free(p);
	    }
	    else
		fontname = p;
	}
	pango_font_description_free(fdd.result);
    }

    if (initial != NULL)
	pango_font_description_free(initial);
    g_object_unref(dlg);

    return fontname;
}

/*
 * Build a table of glyphs for ASCII characters 32..126.
 * This avoids the overhead of itemize+shape for the common case.
 */
    static void
ascii_glyph_table_init(void)
{
    char_u	    ascii_chars[2 * 128];
    PangoAttrList   *attr_list;
    GList	    *item_list;
    int		    i;

    if (gui.ascii_glyphs != NULL)
	pango_glyph_string_free(gui.ascii_glyphs);
    if (gui.ascii_font != NULL)
	g_object_unref(gui.ascii_font);

    gui.ascii_glyphs = NULL;
    gui.ascii_font   = NULL;

    for (i = 0; i < 128; ++i)
    {
	if (i >= 32 && i < 127)
	    ascii_chars[2 * i] = i;
	else
	    ascii_chars[2 * i] = '?';
	ascii_chars[2 * i + 1] = ' ';
    }

    attr_list = pango_attr_list_new();
    item_list = pango_itemize(gui.text_context, (const char *)ascii_chars,
			      0, sizeof(ascii_chars), attr_list, NULL);

    if (item_list != NULL && item_list->next == NULL)
    {
	PangoItem   *item;
	int	    width;

	item  = (PangoItem *)item_list->data;
	width = gui.char_width * PANGO_SCALE;

	gui.ascii_font = item->analysis.font;
	g_object_ref(gui.ascii_font);

	gui.ascii_glyphs = pango_glyph_string_new();

	pango_shape((const char *)ascii_chars, sizeof(ascii_chars),
		    &item->analysis, gui.ascii_glyphs);

	if (gui.ascii_glyphs->num_glyphs == (int)sizeof(ascii_chars))
	{
	    for (i = 0; i < gui.ascii_glyphs->num_glyphs; ++i)
	    {
		PangoGlyphGeometry *geom;

		geom = &gui.ascii_glyphs->glyphs[i].geometry;
		geom->x_offset += MAX(0, width - geom->width) / 2;
		geom->width = width;
	    }
	}
	else
	{
	    pango_glyph_string_free(gui.ascii_glyphs);
	    gui.ascii_glyphs = NULL;
	    g_object_unref(gui.ascii_font);
	    gui.ascii_font = NULL;
	}
    }

    g_list_foreach(item_list, (GFunc)(void *)&pango_item_free, NULL);
    g_list_free(item_list);
    pango_attr_list_unref(attr_list);
}

    static void
get_styled_font_variants(void)
{
    PangoFontDescription    *bold_font_desc;
    PangoFont		    *plain_font;
    PangoFont		    *bold_font;

    gui.font_can_bold = FALSE;

    plain_font = pango_context_load_font(gui.text_context, gui.norm_font);
    if (plain_font == NULL)
	return;

    bold_font_desc = pango_font_description_copy_static(gui.norm_font);
    pango_font_description_set_weight(bold_font_desc, PANGO_WEIGHT_BOLD);

    bold_font = pango_context_load_font(gui.text_context, bold_font_desc);
    if (bold_font != NULL)
    {
	gui.font_can_bold = (bold_font != plain_font);
	g_object_unref(bold_font);
    }

    pango_font_description_free(bold_font_desc);
    if (bold_font != NULL && gui.font_can_bold)
	g_object_unref(plain_font);
}

    int
gui_mch_init_font(char_u *font_name, int fontset UNUSED)
{
    PangoFontDescription    *font_desc;
    PangoLayout		    *layout;
    int			    width;

    if (font_name == NULL)
	font_name = (char_u *)DEFAULT_FONT;

    font_desc = gui_mch_get_font(font_name, FALSE);
    if (font_desc == NULL)
	return FAIL;

    gui_mch_free_font(gui.norm_font);
    gui.norm_font = font_desc;

    pango_context_set_font_description(gui.text_context, font_desc);

    layout = pango_layout_new(gui.text_context);
    pango_layout_set_text(layout, "MW", 2);
    pango_layout_get_size(layout, &width, NULL);
    g_object_unref(layout);

    gui.char_width = (width / 2 + PANGO_SCALE - 1) / PANGO_SCALE;
    if (gui.char_width <= 0)
	gui.char_width = 8;

    gui_mch_adjust_charheight();

    hl_set_font_name(font_name);

    get_styled_font_variants();
    ascii_glyph_table_init();

    return OK;
}

    GuiFont
gui_mch_get_font(char_u *name, int report_error)
{
    PangoFontDescription *font;

    if (name == NULL)
	return NULL;

    font = pango_font_description_from_string((const char *)name);
    if (font == NULL)
    {
	if (report_error)
	    semsg(_(e_unknown_font_str), name);
	return NULL;
    }

    // Ensure a size is set
    if (pango_font_description_get_size(font) <= 0)
	pango_font_description_set_size(font, 10 * PANGO_SCALE);

    return font;
}

    char_u *
gui_mch_get_fontname(GuiFont font, char_u *name UNUSED)
{
    if (font != NOFONT)
    {
	char *desc = pango_font_description_to_string(font);
	char_u *ret = vim_strsave((char_u *)desc);
	g_free(desc);
	return ret;
    }
    return NULL;
}

    void
gui_mch_free_font(GuiFont font)
{
    if (font != NOFONT)
	pango_font_description_free(font);
}

    int
gui_mch_expand_font(optexpand_T *args UNUSED, int *numMatches UNUSED,
	char_u ***matches UNUSED)
{
    return FAIL;
}

/*
 * ============================================================
 * Color handling
 * ============================================================
 */

    guicolor_T
gui_mch_get_color(char_u *name)
{
    if (!gui.in_use)
	return INVALCOLOR;

    if (name != NULL)
	return gui_get_color_cmn(name);

    return INVALCOLOR;
}

    guicolor_T
gui_mch_get_rgb_color(int r, int g, int b)
{
    return gui_get_rgb_color_cmn(r, g, b);
}

    static GdkRGBA
color_to_rgba(guicolor_T color)
{
    GdkRGBA rgba;
    rgba.red   = ((color & 0xff0000) >> 16) / 255.0;
    rgba.green = ((color & 0xff00) >> 8) / 255.0;
    rgba.blue  = (color & 0xff) / 255.0;
    rgba.alpha = 1.0;
    return rgba;
}

    void
gui_mch_set_fg_color(guicolor_T color)
{
    *gui.fgcolor = color_to_rgba(color);
}

    void
gui_mch_set_bg_color(guicolor_T color)
{
    *gui.bgcolor = color_to_rgba(color);
}

    void
gui_mch_set_sp_color(guicolor_T color)
{
    *gui.spcolor = color_to_rgba(color);
}

    guicolor_T
gui_mch_get_rgb(guicolor_T pixel)
{
    return pixel;
}

/*
 * ============================================================
 * Drawing
 * ============================================================
 */

    static void
draw_event(GtkDrawingArea *area UNUSED, cairo_t *cr,
	int width, int height, gpointer data UNUSED)
{
    // Ensure surface matches drawing area
    if (gui.surface != NULL)
    {
	int sw = cairo_image_surface_get_width(gui.surface);
	int sh = cairo_image_surface_get_height(gui.surface);
	if (sw != width || sh != height)
	{
	    cairo_surface_t *old = gui.surface;
	    gui.surface = cairo_image_surface_create(
		    CAIRO_FORMAT_ARGB32, width, height);
	    // Copy old content
	    cairo_t *tmp = cairo_create(gui.surface);
	    cairo_set_source_surface(tmp, old, 0, 0);
	    cairo_paint(tmp);
	    cairo_destroy(tmp);
	    cairo_surface_destroy(old);
	}
    }
    else if (width > 0 && height > 0)
	gui.surface = cairo_image_surface_create(
		CAIRO_FORMAT_ARGB32, width, height);

    // Fill background with Vim's background color
    guicolor_T bg = gui.back_pixel;
    cairo_set_source_rgb(cr,
	    ((bg & 0xff0000) >> 16) / 255.0,
	    ((bg & 0xff00) >> 8) / 255.0,
	    (bg & 0xff) / 255.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    // Paint the Vim surface on top
    if (gui.surface != NULL)
    {
	cairo_set_source_surface(cr, gui.surface, 0, 0);
	cairo_paint(cr);
    }
}

    static void
set_cairo_source_from_pixel(cairo_t *cr, guicolor_T pixel)
{
    cairo_set_source_rgb(cr,
	    ((pixel & 0xff0000) >> 16) / 255.0,
	    ((pixel & 0xff00) >> 8) / 255.0,
	    (pixel & 0xff) / 255.0);
}

    void
gui_mch_clear_block(int row1, int col1, int row2, int col2)
{
    cairo_t *cr;

    if (gui.surface == NULL)
	return;

    cr = cairo_create(gui.surface);
    set_cairo_source_from_pixel(cr, gui.back_pixel);
    cairo_rectangle(cr,
	    FILL_X(col1), FILL_Y(row1),
	    (col2 - col1 + 1) * gui.char_width,
	    (row2 - row1 + 1) * gui.char_height);
    cairo_fill(cr);
    cairo_destroy(cr);

    if (gui.drawarea != NULL)
	gtk_widget_queue_draw(gui.drawarea);
}

    void
gui_mch_clear_all(void)
{
    cairo_t *cr;

    if (gui.surface == NULL)
	return;

    cr = cairo_create(gui.surface);
    set_cairo_source_from_pixel(cr, gui.back_pixel);
    cairo_paint(cr);
    cairo_destroy(cr);

    if (gui.drawarea != NULL)
	gtk_widget_queue_draw(gui.drawarea);
}

    static void
surface_copy_rect(int dest_x, int dest_y,
	int src_x, int src_y,
	int width, int height)
{
    cairo_t *cr;
    cairo_surface_t *tmp;

    if (gui.surface == NULL || width <= 0 || height <= 0)
	return;

    // Use a temporary surface to avoid overlap issues
    tmp = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cr = cairo_create(tmp);
    cairo_set_source_surface(cr, gui.surface, -src_x, -src_y);
    cairo_paint(cr);
    cairo_destroy(cr);

    cr = cairo_create(gui.surface);
    cairo_set_source_surface(cr, tmp, dest_x, dest_y);
    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_destroy(tmp);
}

    void
gui_mch_delete_lines(int row, int num_lines)
{
    int ncols = gui.scroll_region_right - gui.scroll_region_left + 1;
    int nrows = gui.scroll_region_bot - row + 1;
    int src_nrows = nrows - num_lines;

    surface_copy_rect(
	    FILL_X(gui.scroll_region_left), FILL_Y(row),
	    FILL_X(gui.scroll_region_left), FILL_Y(row + num_lines),
	    gui.char_width * ncols + 1, gui.char_height * src_nrows);
    gui_clear_block(
	    gui.scroll_region_bot - num_lines + 1, gui.scroll_region_left,
	    gui.scroll_region_bot, gui.scroll_region_right);

    gtk_widget_queue_draw(gui.drawarea);
}

    void
gui_mch_insert_lines(int row, int num_lines)
{
    int ncols = gui.scroll_region_right - gui.scroll_region_left + 1;
    int nrows = gui.scroll_region_bot - row + 1;
    int src_nrows = nrows - num_lines;

    surface_copy_rect(
	    FILL_X(gui.scroll_region_left), FILL_Y(row + num_lines),
	    FILL_X(gui.scroll_region_left), FILL_Y(row),
	    gui.char_width * ncols + 1, gui.char_height * src_nrows);
    gui_clear_block(
	    row, gui.scroll_region_left,
	    row + num_lines - 1, gui.scroll_region_right);

    gtk_widget_queue_draw(gui.drawarea);
}

    void
gui_mch_draw_hollow_cursor(guicolor_T color)
{
    cairo_t *cr;
    int i = 1;

    if (gui.surface == NULL)
	return;

    cr = cairo_create(gui.surface);
    gui_mch_set_fg_color(color);
    cairo_set_source_rgba(cr,
	    gui.fgcolor->red, gui.fgcolor->green,
	    gui.fgcolor->blue, gui.fgcolor->alpha);
    if (mb_lefthalve(gui.row, gui.col))
	i = 2;
    cairo_set_line_width(cr, 1.0);
    cairo_rectangle(cr,
	    FILL_X(gui.col) + 0.5, FILL_Y(gui.row) + 0.5,
	    i * gui.char_width - 1, gui.char_height - 1);
    cairo_stroke(cr);
    cairo_destroy(cr);

    gtk_widget_queue_draw(gui.drawarea);
}

    void
gui_mch_draw_part_cursor(int w, int h, guicolor_T color)
{
    cairo_t *cr;

    if (gui.surface == NULL)
	return;

    gui_mch_set_fg_color(color);
    cr = cairo_create(gui.surface);
    cairo_set_source_rgba(cr,
	    gui.fgcolor->red, gui.fgcolor->green,
	    gui.fgcolor->blue, gui.fgcolor->alpha);
    cairo_rectangle(cr,
#ifdef FEAT_RIGHTLEFT
	    CURSOR_BAR_RIGHT ? FILL_X(gui.col + 1) - w :
#endif
	    FILL_X(gui.col), FILL_Y(gui.row) + gui.char_height - h,
	    w, h);
    cairo_fill(cr);
    cairo_destroy(cr);

    gtk_widget_queue_draw(gui.drawarea);
}

    void
gui_mch_flash(int msec)
{
    // Invert the screen, wait, then invert back
    if (gui.surface == NULL)
	return;

    gui_mch_invert_rectangle(0, 0, (int)Rows - 1, (int)Columns - 1);
    gui_mch_flush();
    ui_delay((long)msec, TRUE);
    gui_mch_invert_rectangle(0, 0, (int)Rows - 1, (int)Columns - 1);
}

    void
gui_mch_invert_rectangle(int r, int c, int nr, int nc)
{
    cairo_t *cr;

    if (gui.surface == NULL)
	return;

    cr = cairo_create(gui.surface);
    cairo_set_operator(cr, CAIRO_OPERATOR_DIFFERENCE);
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_rectangle(cr,
	    FILL_X(c), FILL_Y(r),
	    (nc + 1) * gui.char_width, (nr + 1) * gui.char_height);
    cairo_fill(cr);
    cairo_destroy(cr);

    gtk_widget_queue_draw(gui.drawarea);
}

/*
 * ============================================================
 * Event handling
 * ============================================================
 */

    static gboolean
key_press_event(GtkEventControllerKey *controller UNUSED,
	guint key_sym, guint keycode UNUSED,
	GdkModifierType state, gpointer data UNUSED)
{
    char_u	string[32], string2[32];
    int		len;
    int		i;
    int		modifiers;
    int		key;
    char_u	*s, *d;

#ifdef FEAT_XIM
    // Let the input method have a go at the key event.
    // If it consumed the event, we're done.
    if (xic != NULL)
    {
	GdkEvent *event = gtk_event_controller_get_current_event(
		GTK_EVENT_CONTROLLER(controller));
	if (event != NULL && gtk_im_context_filter_keypress(xic, event))
	    return TRUE;
    }
#endif

    len = keyval_to_string(key_sym, string2);

    if (len > 1 && input_conv.vc_type != CONV_NONE)
	len = convert_input(string2, len, sizeof(string2));

    s = string2;
    d = string;
    for (i = 0; i < len; ++i)
    {
	*d++ = s[i];
	if (d[-1] == CSI && d + 2 < string + sizeof(string))
	{
	    *d++ = KS_EXTRA;
	    *d++ = (int)KE_CSI;
	}
    }
    len = d - string;

    // Shift-Tab results in Left_Tab
    if (key_sym == GDK_KEY_ISO_Left_Tab)
    {
	key_sym = GDK_KEY_Tab;
	state |= GDK_SHIFT_MASK;
    }

    // Check for special keys
    if (len == 0 || len == 1)
    {
	for (i = 0; special_keys[i].key_sym != 0; i++)
	{
	    if (special_keys[i].key_sym == key_sym)
	    {
		string[0] = CSI;
		string[1] = special_keys[i].code0;
		string[2] = special_keys[i].code1;
		len = -3;
		break;
	    }
	}
    }

    if (len == 0)
	return TRUE;

    if (len == -3)
	key = TO_SPECIAL(string[1], string[2]);
    else
    {
	string[len] = NUL;
	key = mb_ptr2char(string);
    }

    modifiers = modifiers_gdk2vim(state);

    key = simplify_key(key, &modifiers);
    if (key == CSI)
	key = K_CSI;
    if (IS_SPECIAL(key))
    {
	string[0] = CSI;
	string[1] = K_SECOND(key);
	string[2] = K_THIRD(key);
	len = 3;
    }
    else
    {
	key = may_adjust_key_for_ctrl(modifiers, key);
	modifiers = may_remove_shift_modifier(modifiers, key);
	len = mb_char2bytes(key, string);
    }

    if (modifiers != 0)
    {
	string2[0] = CSI;
	string2[1] = KS_MODIFIER;
	string2[2] = modifiers;
	add_to_input_buf(string2, 3);
    }

    {
	int int_ch = check_for_interrupt(key, modifiers);
	if (int_ch != NUL)
	{
	    trash_input_buf();
	    string[0] = int_ch;
	    len = 1;
	}
    }

    add_to_input_buf(string, len);

    if (p_mh)
	gui_mch_mousehide(TRUE);

    return TRUE;
}

    static void
key_release_event(GtkEventControllerKey *controller UNUSED,
	guint keyval UNUSED, guint keycode UNUSED,
	GdkModifierType state UNUSED, gpointer data UNUSED)
{
}

static int mouse_timed_out = TRUE;
static guint mouse_click_timer = 0;

    static timeout_cb_type
mouse_click_timer_cb(gpointer data)
{
    *(int *)data = TRUE;
    return FALSE;
}

    static int
modifiers_gdk2mouse(guint state)
{
    int modifiers = 0;

    if (state & GDK_SHIFT_MASK)
	modifiers |= MOUSE_SHIFT;
    if (state & GDK_CONTROL_MASK)
	modifiers |= MOUSE_CTRL;
    if (state & GDK_ALT_MASK)
	modifiers |= MOUSE_ALT;

    return modifiers;
}

    static void
button_press_event(GtkGestureClick *gesture, int n_press UNUSED,
	double x, double y, gpointer data UNUSED)
{
    int button;
    int repeated_click = FALSE;
    int_u vim_modifiers;
    guint btn;
    GdkModifierType state;
    GdkEvent *event;

    event = gtk_event_controller_get_current_event(
	    GTK_EVENT_CONTROLLER(gesture));
    state = gdk_event_get_modifier_state(event);
    btn = gdk_button_event_get_button(event);

    if (!mouse_timed_out && mouse_click_timer)
    {
	timeout_remove(mouse_click_timer);
	mouse_click_timer = 0;
	repeated_click = TRUE;
    }

    mouse_timed_out = FALSE;
    mouse_click_timer = timeout_add(p_mouset, mouse_click_timer_cb,
	    &mouse_timed_out);

    switch (btn)
    {
	case 1: button = MOUSE_LEFT; break;
	case 2: button = MOUSE_MIDDLE; break;
	case 3: button = MOUSE_RIGHT; break;
	case 8: button = MOUSE_X1; break;
	case 9: button = MOUSE_X2; break;
	default: return;
    }

    vim_modifiers = modifiers_gdk2mouse(state);
    gui_send_mouse_event(button, (int)x, (int)y, repeated_click, vim_modifiers);
}

    static void
button_release_event(GtkGestureClick *gesture, int n_press UNUSED,
	double x, double y, gpointer data UNUSED)
{
    int vim_modifiers;
    GdkModifierType state;
    GdkEvent *event;

    event = gtk_event_controller_get_current_event(
	    GTK_EVENT_CONTROLLER(gesture));
    state = gdk_event_get_modifier_state(event);
    vim_modifiers = modifiers_gdk2mouse(state);

    gui_send_mouse_event(MOUSE_RELEASE, (int)x, (int)y, FALSE, vim_modifiers);
}

    static void
motion_notify_event(GtkEventControllerMotion *controller UNUSED,
	double x, double y, gpointer data UNUSED)
{
    GdkModifierType state;
    GdkEvent *event;

    event = gtk_event_controller_get_current_event(
	    GTK_EVENT_CONTROLLER(controller));
    if (event == NULL)
	return;
    state = gdk_event_get_modifier_state(event);

    int button = (state & GDK_BUTTON1_MASK) ? MOUSE_LEFT
	       : (state & GDK_BUTTON2_MASK) ? MOUSE_MIDDLE
	       : (state & GDK_BUTTON3_MASK) ? MOUSE_RIGHT
	       : 0;

    if (button)
	gui_send_mouse_event(MOUSE_DRAG, (int)x, (int)y, FALSE,
		modifiers_gdk2mouse(state));

    if (p_mh)
	gui_mch_mousehide(FALSE);
}

    static void
enter_notify_event(GtkEventControllerMotion *controller UNUSED,
	double x UNUSED, double y UNUSED, gpointer data UNUSED)
{
    if (blink_state == BLINK_NONE)
	gui_mch_start_blink();
}

    static void
leave_notify_event(GtkEventControllerMotion *controller UNUSED,
	gpointer data UNUSED)
{
    if (blink_state != BLINK_NONE)
	gui_mch_stop_blink(TRUE);
}

    static gboolean
scroll_event(GtkEventControllerScroll *controller UNUSED,
	double dx UNUSED, double dy, gpointer data UNUSED)
{
    int button;
    int_u vim_modifiers;
    GdkModifierType state;
    GdkEvent *event;

    event = gtk_event_controller_get_current_event(
	    GTK_EVENT_CONTROLLER(controller));
    if (event == NULL)
	return FALSE;
    state = gdk_event_get_modifier_state(event);

    if (dy < 0)
	button = MOUSE_4;	// scroll up
    else if (dy > 0)
	button = MOUSE_5;	// scroll down
    else if (dx < 0)
	button = MOUSE_7;	// scroll left
    else if (dx > 0)
	button = MOUSE_6;	// scroll right
    else
	return FALSE;

    vim_modifiers = modifiers_gdk2mouse(state);

    {
	double mx, my;
	gdk_event_get_position(event, &mx, &my);
	gui_send_mouse_event(button, (int)mx, (int)my, FALSE, vim_modifiers);
    }

    return TRUE;
}

    static void
focus_in_event(GtkEventControllerFocus *controller UNUSED,
	gpointer data UNUSED)
{
    if (blink_state != BLINK_NONE)
	gui_mch_stop_blink(TRUE);
    gui_focus_change(TRUE);
}

    static void
focus_out_event(GtkEventControllerFocus *controller UNUSED,
	gpointer data UNUSED)
{
    gui_mch_stop_blink(TRUE);
    gui_focus_change(FALSE);
}

    static void
drawarea_realize_cb(GtkWidget *widget UNUSED, gpointer data UNUSED)
{
    int w, h;

    // Use formwin size since drawarea may not have its final size yet
    if (gui.formwin != NULL)
    {
	w = gtk_widget_get_width(gui.formwin);
	h = gtk_widget_get_height(gui.formwin);
    }
    else
    {
	w = gtk_widget_get_width(widget);
	h = gtk_widget_get_height(widget);
    }

    if (w <= 0) w = 800;
    if (h <= 0) h = 600;

    if (gui.surface != NULL)
	cairo_surface_destroy(gui.surface);
    gui.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);

    gui_mch_new_colors();

#ifdef FEAT_XIM
    xim_init();
#endif
}

    static void
drawarea_unrealize_cb(GtkWidget *widget UNUSED, gpointer data UNUSED)
{
    if (gui.surface != NULL)
    {
	cairo_surface_destroy(gui.surface);
	gui.surface = NULL;
    }
}

    static void
drawarea_resize_cb(GtkDrawingArea *area UNUSED, int width, int height,
	gpointer data UNUSED)
{
    if (width <= 0 || height <= 0)
	return;

    if (gui.surface != NULL)
	cairo_surface_destroy(gui.surface);
    gui.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

    // Notify Vim about the new size - this will cause a full redraw
    gui_resize_shell(width, height);
}

#ifdef FEAT_DND
/*
 * Drag-and-drop handler for files and text.
 */
    static gboolean
drop_cb(GtkDropTarget *target UNUSED, const GValue *value,
	double x, double y, gpointer data UNUSED)
{
    if (G_VALUE_HOLDS(value, GDK_TYPE_FILE_LIST))
    {
	GSList	*files = g_value_get_boxed(value);
	int	nfiles = g_slist_length(files);
	char_u	**fnames;
	int	i;

	if (nfiles <= 0)
	    return FALSE;

	fnames = ALLOC_MULT(char_u *, nfiles);
	if (fnames == NULL)
	    return FALSE;

	i = 0;
	for (GSList *l = files; l != NULL; l = l->next)
	{
	    GFile *file = l->data;
	    char *path = g_file_get_path(file);
	    if (path != NULL)
		fnames[i++] = vim_strsave((char_u *)path);
	    g_free(path);
	}
	nfiles = i;

	if (nfiles > 0)
	    gui_handle_drop((int)x, (int)y, 0, fnames, nfiles);
	else
	    vim_free(fnames);

	return TRUE;
    }
    else if (G_VALUE_HOLDS(value, G_TYPE_STRING))
    {
	const char  *text = g_value_get_string(value);
	char_u	    dropkey[6] = {CSI, KS_MODIFIER, 0,
				  CSI, KS_EXTRA, (char_u)KE_DROP};

	if (text == NULL || *text == NUL)
	    return FALSE;

	dnd_yank_drag_data((char_u *)text, (long)STRLEN(text));
	add_to_input_buf(dropkey + 3, 3);

	return TRUE;
    }

    return FALSE;
}
#endif

    static void
mainwin_destroy_cb(GObject *object UNUSED, gpointer data UNUSED)
{
    gui.mainwin = NULL;
    gui.drawarea = NULL;
    if (!exiting)
	gui_shell_closed();
}

    static gboolean
delete_event_cb(GtkWindow *window UNUSED, gpointer data UNUSED)
{
    gui_shell_closed();
    return TRUE;
}

/*
 * ============================================================
 * Misc functions
 * ============================================================
 */

    static timeout_cb_type
input_timer_cb(gpointer data)
{
    int *timed_out = (int *)data;

    *timed_out = TRUE;
    return FALSE;	// don't call me again
}

    void
gui_mch_update(void)
{
    int cnt = 0;

    while (g_main_context_pending(NULL) && !vim_is_input_buf_full()
								&& ++cnt < 100)
	g_main_context_iteration(NULL, TRUE);
}

    int
gui_mch_wait_for_chars(long wtime)
{
    int		focus;
    guint	timer;
    static int	timed_out;
    int		retval = FAIL;

    timed_out = FALSE;

    if (wtime >= 0)
	timer = timeout_add(wtime == 0 ? 1L : wtime,
						   input_timer_cb, &timed_out);
    else
	timer = 0;

    focus = gui.in_focus;

    do
    {
	// Stop or start blinking when focus changes
	if (gui.in_focus != focus)
	{
	    if (gui.in_focus)
		gui_mch_start_blink();
	    else
		gui_mch_stop_blink(TRUE);
	    focus = gui.in_focus;
	}

#ifdef MESSAGE_QUEUE
# ifdef FEAT_TIMERS
	did_add_timer = FALSE;
# endif
	parse_queued_messages();
# ifdef FEAT_TIMERS
	if (did_add_timer)
	    goto theend;
# endif
#endif

	// If the GUI was destroyed or main loop quit requested, bail out.
	if (gui.mainwin == NULL || exiting || gtk4_main_loop_quit)
	    goto theend;

	// Loop processing until a timeout or input occurs.
	if (!input_available())
	    g_main_context_iteration(NULL, TRUE);

	if (input_available())
	{
	    retval = OK;
	    goto theend;
	}
    } while (wtime < 0 || !timed_out);

    gui_mch_update();

theend:
    if (timer != 0 && !timed_out)
	timeout_remove(timer);

    return retval;
}

    void
gui_mch_flush(void)
{
    if (gui.mainwin != NULL && gtk_widget_get_realized(gui.mainwin))
	gdk_display_flush(gtk_widget_get_display(gui.mainwin));
}

    void
gui_mch_beep(void)
{
    GdkDisplay *display;

    if (gui.mainwin != NULL && gtk_widget_get_realized(gui.mainwin))
    {
	display = gtk_widget_get_display(gui.mainwin);
	if (display != NULL)
	    gdk_display_beep(display);
    }
}

    void *
gui_mch_get_display(void)
{
    if (gui.mainwin != NULL && gtk_widget_get_display(gui.mainwin))
	return gtk_widget_get_display(gui.mainwin);
    return NULL;
}

    void
gui_mch_iconify(void)
{
    gtk_window_minimize(GTK_WINDOW(gui.mainwin));
}

    void
gui_mch_set_foreground(void)
{
    gtk_window_present(GTK_WINDOW(gui.mainwin));
}

    void
gui_mch_getmouse(int *x, int *y)
{
    *x = 0;
    *y = 0;
    // GTK4: No reliable way to query pointer position synchronously.
}

    void
gui_mch_setmouse(int x UNUSED, int y UNUSED)
{
    // GTK4/Wayland: cannot warp pointer
}

    void
gui_mch_mousehide(int hide)
{
    if (gui.pointer_hidden == hide)
	return;

    gui.pointer_hidden = hide;
    if (gui.blank_pointer != NULL)
    {
	if (hide)
	    gtk_widget_set_cursor(gui.drawarea, gui.blank_pointer);
	else
#ifdef FEAT_MOUSESHAPE
	    mch_set_mouse_shape(last_shape);
#else
	    gtk_widget_set_cursor(gui.drawarea, NULL);
#endif
    }
}

    int
gui_mch_haskey(char_u *name)
{
    int i;

    for (i = 0; special_keys[i].key_sym != 0; i++)
	if (name[0] == special_keys[i].code0
		&& name[1] == special_keys[i].code1)
	    return OK;
    return FAIL;
}

    void
gui_mch_forked(void)
{
}

/*
 * ============================================================
 * Scrollbar
 * ============================================================
 */

    void
gui_mch_enable_scrollbar(scrollbar_T *sb, int flag)
{
    if (sb->id != NULL)
	gtk_widget_set_visible(sb->id, flag);
}

/*
 * ============================================================
 * Menu stubs
 * ============================================================
 */

    void
gui_mch_menu_grey(vimmenu_T *menu UNUSED, int grey UNUSED)
{
    // No-op: menu system not yet implemented for GTK4.
}

    void
gui_mch_menu_hidden(vimmenu_T *menu UNUSED, int hidden UNUSED)
{
    // No-op: menu system not yet implemented for GTK4.
}

    void
gui_mch_draw_menubar(void)
{
    // No-op: menu system not yet implemented for GTK4.
}

/*
 * ============================================================
 * Tabline
 * ============================================================
 */

#ifdef FEAT_GUI_TABLINE
    void
gui_mch_show_tabline(int showit)
{
    if (gui.tabline != NULL)
	gtk_widget_set_visible(gui.tabline, showit);
}

    int
gui_mch_showing_tabline(void)
{
    return gui.tabline != NULL && gtk_widget_get_visible(gui.tabline);
}

static int ignore_tabline_evt = FALSE;

    void
gui_mch_update_tabline(void)
{
    GtkWidget	*page;
    GtkWidget	*event_box;
    GtkWidget	*label;
    tabpage_T	*tp;
    int		nr = 0;
    int		tab_num;
    int		curtabidx = 0;
    char_u	*labeltext;

    if (gui.tabline == NULL)
	return;

    ignore_tabline_evt = TRUE;

    for (tp = first_tabpage; tp != NULL; tp = tp->tp_next, ++nr)
    {
	if (tp == curtab)
	    curtabidx = nr;

	tab_num = nr + 1;

	page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(gui.tabline), nr);
	if (page == NULL)
	{
	    page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	    gtk_widget_show(page);
	    event_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	    gtk_widget_show(event_box);
	    label = gtk_label_new("-Empty-");
	    gtk_box_append(GTK_BOX(event_box), label);
	    gtk_widget_show(label);
	    gtk_notebook_insert_page(GTK_NOTEBOOK(gui.tabline),
		    page, event_box, nr++);
	    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(gui.tabline),
		    page, TRUE);
	}

	event_box = gtk_notebook_get_tab_label(GTK_NOTEBOOK(gui.tabline), page);
	g_object_set_data(G_OBJECT(event_box), "tab_num",
		GINT_TO_POINTER(tab_num));
	label = gtk_widget_get_first_child(event_box);
	get_tabline_label(tp, FALSE);
	labeltext = CONVERT_TO_UTF8(NameBuff);
	if (label != NULL && GTK_IS_LABEL(label))
	    gtk_label_set_text(GTK_LABEL(label), (const char *)labeltext);
	CONVERT_TO_UTF8_FREE(labeltext);

	get_tabline_label(tp, TRUE);
	labeltext = CONVERT_TO_UTF8(NameBuff);
	gtk_widget_set_tooltip_text(event_box, (const gchar *)labeltext);
	CONVERT_TO_UTF8_FREE(labeltext);
    }

    while (gtk_notebook_get_nth_page(GTK_NOTEBOOK(gui.tabline), nr) != NULL)
	gtk_notebook_remove_page(GTK_NOTEBOOK(gui.tabline), nr);

    if (gtk_notebook_get_current_page(GTK_NOTEBOOK(gui.tabline)) != curtabidx)
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gui.tabline), curtabidx);

    ignore_tabline_evt = FALSE;
}

    void
gui_mch_set_curtab(int nr)
{
    if (gui.tabline != NULL)
	gtk_notebook_set_current_page(GTK_NOTEBOOK(gui.tabline), nr - 1);
}
#endif

/*
 * ============================================================
 * Sign support
 * ============================================================
 */

#if defined(FEAT_SIGN_ICONS)
# define SIGN_WIDTH  (2 * gui.char_width)
# define SIGN_HEIGHT (gui.char_height)

    void
gui_mch_drawsign(int row, int col, int typenr)
{
    GdkPixbuf	*sign;
    cairo_t	*cr;
    int		width, height;

    sign = (GdkPixbuf *)sign_get_image(typenr);
    if (sign == NULL || gui.surface == NULL)
	return;

    cr = cairo_create(gui.surface);

    width = gdk_pixbuf_get_width(sign);
    height = gdk_pixbuf_get_height(sign);

    // Scale to fit the sign area if needed
    if (width != SIGN_WIDTH || height != SIGN_HEIGHT)
    {
	GdkPixbuf *scaled = gdk_pixbuf_scale_simple(sign,
		SIGN_WIDTH, SIGN_HEIGHT, GDK_INTERP_BILINEAR);
	if (scaled != NULL)
	{
	    gdk_cairo_set_source_pixbuf(cr, scaled,
		    FILL_X(col), FILL_Y(row));
	    g_object_unref(scaled);
	}
	else
	    gdk_cairo_set_source_pixbuf(cr, sign,
		    FILL_X(col), FILL_Y(row));
    }
    else
	gdk_cairo_set_source_pixbuf(cr, sign,
		FILL_X(col), FILL_Y(row));

    cairo_paint(cr);
    cairo_destroy(cr);

    gtk_widget_queue_draw(gui.drawarea);
}

    void *
gui_mch_register_sign(char_u *signfile)
{
    if (signfile[0] != NUL && signfile[0] != '-' && gui.in_use)
    {
	GdkPixbuf   *sign;
	GError	    *error = NULL;

	sign = gdk_pixbuf_new_from_file((const char *)signfile, &error);
	if (error == NULL)
	    return sign;

	semsg("E255: %s", error->message);
	g_error_free(error);
    }
    return NULL;
}

    void
gui_mch_destroy_sign(void *sign)
{
    if (sign != NULL)
	g_object_unref(sign);
}
#endif

/*
 * ============================================================
 * Stubs for functions not yet implemented or not applicable in GTK4
 * ============================================================
 */

/*
 * Draw a string of characters on the screen using the current font and colors.
 * Returns the number of display cells used.
 */
    int
gui_gtk_draw_string(int row, int col, char_u *s, int len, int flags)
{
    cairo_t	*cr;
    char_u	*conv_buf = NULL;
    int		convlen;
    int		cells;

    if (gui.text_context == NULL || gui.surface == NULL)
	return len;

    // Convert to UTF-8 if needed
    if (output_conv.vc_type != CONV_NONE)
    {
	convlen = len;
	conv_buf = string_convert(&output_conv, s, &convlen);
	if (conv_buf != NULL)
	{
	    s = conv_buf;
	    len = convlen;
	}
    }

    cells = mb_string2cells(s, len);

    cr = cairo_create(gui.surface);

    // Clip to the current row
    cairo_rectangle(cr,
	    gui.border_offset, FILL_Y(row),
	    gui.num_cols * gui.char_width, gui.char_height);
    cairo_clip(cr);

    // Draw background
    if (!(flags & DRAW_TRANSP))
    {
	cairo_set_source_rgba(cr,
		gui.bgcolor->red, gui.bgcolor->green,
		gui.bgcolor->blue, gui.bgcolor->alpha);
	cairo_rectangle(cr,
		FILL_X(col), FILL_Y(row),
		cells * gui.char_width, gui.char_height);
	cairo_fill(cr);
    }

    // Draw the text using Pango glyph strings for correct fixed-width rendering
    {
	PangoGlyphString *glyphs;
	PangoFont *font;
	int column_offset = 0;
	int is_ascii = TRUE;
	char_u *p;

	// Check if the string is pure ASCII
	for (p = s; p < s + len; ++p)
	    if (*p & 0x80)
	    {
		is_ascii = FALSE;
		break;
	    }

	glyphs = pango_glyph_string_new();

	if (is_ascii && gui.ascii_glyphs != NULL
		&& !(flags & DRAW_ITALIC)
		&& !((flags & DRAW_BOLD) && gui.font_can_bold))
	{
	    // Fast path for ASCII: use cached glyph table
	    int i;

	    font = gui.ascii_font;
	    pango_glyph_string_set_size(glyphs, len);
	    for (i = 0; i < len; ++i)
	    {
		glyphs->glyphs[i] = gui.ascii_glyphs->glyphs[2 * s[i]];
		glyphs->log_clusters[i] = i;
	    }
	    column_offset = len;

	    // Draw foreground text
	    cairo_set_source_rgba(cr,
		    gui.fgcolor->red, gui.fgcolor->green,
		    gui.fgcolor->blue, gui.fgcolor->alpha);
	    cairo_move_to(cr, TEXT_X(col), TEXT_Y(row));
	    pango_cairo_show_glyph_string(cr, font, glyphs);

	    if ((flags & DRAW_BOLD) && !gui.font_can_bold)
	    {
		cairo_move_to(cr, TEXT_X(col) + 1, TEXT_Y(row));
		pango_cairo_show_glyph_string(cr, font, glyphs);
	    }
	}
	else
	{
	    // Slow path: itemize and shape
	    PangoAttrList *attr_list;
	    GList *item_list;

	    attr_list = pango_attr_list_new();

	    if ((flags & DRAW_BOLD) && gui.font_can_bold)
		pango_attr_list_insert(attr_list,
			pango_attr_weight_new(PANGO_WEIGHT_BOLD));
	    if (flags & DRAW_ITALIC)
		pango_attr_list_insert(attr_list,
			pango_attr_style_new(PANGO_STYLE_ITALIC));

	    item_list = pango_itemize(gui.text_context,
			    (const char *)s, 0, len, attr_list, NULL);

	    // Draw foreground text
	    cairo_set_source_rgba(cr,
		    gui.fgcolor->red, gui.fgcolor->green,
		    gui.fgcolor->blue, gui.fgcolor->alpha);

	    {
		GList *item_iter;
		int col_offset = 0;

		for (item_iter = item_list; item_iter != NULL;
			item_iter = item_iter->next)
		{
		    PangoItem *item = (PangoItem *)item_iter->data;
		    int item_cells = 0;
		    int i;
		    int base_width = gui.char_width * PANGO_SCALE;

		    pango_shape((const char *)s + item->offset,
			    item->length, &item->analysis, glyphs);

		    // Adjust glyph widths for cell alignment
		    for (i = 0; i < glyphs->num_glyphs; ++i)
		    {
			PangoGlyphGeometry *geom;
			int byte_offset;
			int cell_w;

			geom = &glyphs->glyphs[i].geometry;
			byte_offset = item->offset + glyphs->log_clusters[i];
			cell_w = utf_ptr2cells(s + byte_offset);
			item_cells += cell_w;
			cell_w *= base_width;
			geom->x_offset += MAX(0, cell_w - geom->width) / 2;
			geom->width = cell_w;
		    }

		    cairo_move_to(cr,
			    TEXT_X(col + col_offset), TEXT_Y(row));
		    pango_cairo_show_glyph_string(cr,
			    item->analysis.font, glyphs);

		    if ((flags & DRAW_BOLD) && !gui.font_can_bold)
		    {
			cairo_move_to(cr,
				TEXT_X(col + col_offset) + 1, TEXT_Y(row));
			pango_cairo_show_glyph_string(cr,
				item->analysis.font, glyphs);
		    }

		    col_offset += item_cells;
		}
		column_offset = col_offset;
	    }

	    g_list_foreach(item_list,
		    (GFunc)(void *)&pango_item_free, NULL);
	    g_list_free(item_list);
	    pango_attr_list_unref(attr_list);
	}

	pango_glyph_string_free(glyphs);
    }

    // Draw underline
    if (flags & DRAW_UNDERL)
    {
	int y = FILL_Y(row + 1) - 1;
	cairo_set_source_rgba(cr,
		gui.fgcolor->red, gui.fgcolor->green,
		gui.fgcolor->blue, gui.fgcolor->alpha);
	cairo_set_line_width(cr, 1.0);
	cairo_move_to(cr, FILL_X(col), y + 0.5);
	cairo_line_to(cr, FILL_X(col + cells), y + 0.5);
	cairo_stroke(cr);
    }

    // Draw undercurl
    if (flags & DRAW_UNDERC)
    {
	static const int val[8] = {1, 0, 0, 0, 1, 2, 2, 2};
	int y = FILL_Y(row + 1) - 1;
	int i, offset;

	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgba(cr,
		gui.spcolor->red, gui.spcolor->green,
		gui.spcolor->blue, gui.spcolor->alpha);
	cairo_move_to(cr, FILL_X(col) + 1, y - 2 + 0.5);
	for (i = FILL_X(col) + 1; i < FILL_X(col + cells); ++i)
	{
	    offset = val[i % 8];
	    cairo_line_to(cr, i, y - offset + 0.5);
	}
	cairo_stroke(cr);
    }

    // Draw strikethrough
    if (flags & DRAW_STRIKE)
    {
	int y = FILL_Y(row) + gui.char_height / 2;
	cairo_set_source_rgba(cr,
		gui.fgcolor->red, gui.fgcolor->green,
		gui.fgcolor->blue, gui.fgcolor->alpha);
	cairo_set_line_width(cr, 1.0);
	cairo_move_to(cr, FILL_X(col), y + 0.5);
	cairo_line_to(cr, FILL_X(col + cells), y + 0.5);
	cairo_stroke(cr);
    }

    cairo_destroy(cr);
    vim_free(conv_buf);

    if (gui.drawarea != NULL)
	gtk_widget_queue_draw(gui.drawarea);

    return cells;
}

    int
gui_get_x11_windis(Window *win UNUSED, Display **dis UNUSED)
{
    // GTK4: not applicable
    return FAIL;
}

#if defined(FEAT_SOCKETSERVER)

/*
 * Callback for new events from the socket server listening socket.
 */
    static int
socket_server_poll_in(int fd UNUSED, GIOCondition cond,
		      void *user_data UNUSED)
{
    if (cond & G_IO_IN)
	socket_server_accept_client();
    else if (cond & (G_IO_ERR | G_IO_HUP))
    {
	socket_server_uninit();
	return FALSE;
    }

    return TRUE;
}

#endif // FEAT_SOCKETSERVER

/*
 * Initialize socket server for use in the GUI (does not actually initialize
 * the socket server, only attaches a source).
 */
    void
gui_gtk_init_socket_server(void)
{
#if defined(FEAT_SOCKETSERVER)
    if (socket_server_source_id > 0)
	return;
    // Register source for file descriptor to global default context
    socket_server_source_id = g_unix_fd_add(socket_server_get_fd(),
	    G_IO_IN | G_IO_ERR | G_IO_HUP, socket_server_poll_in, NULL);
#endif
}

/*
 * Remove the source for the socket server listening socket.
 */
    void
gui_gtk_uninit_socket_server(void)
{
#if defined(FEAT_SOCKETSERVER)
    if (socket_server_source_id > 0)
    {
	g_source_remove(socket_server_source_id);
	socket_server_source_id = 0;
    }
#endif
}

    void
gui_gtk_set_mnemonics(int enable UNUSED)
{
    // No-op: menu mnemonics depend on menu system, not yet implemented
    // for GTK4.
}

    void
gui_make_popup(char_u *path_name UNUSED, int mouse_pos UNUSED)
{
    // No-op: popup menus depend on menu system, not yet implemented
    // for GTK4.
}

    int
get_menu_tool_width(void)
{
    return 0;
}

    int
get_menu_tool_height(void)
{
    int height = 0;

#ifdef FEAT_MENU
    if (gui.menubar != NULL && gtk_widget_get_visible(gui.menubar))
    {
	GtkRequisition req;
	gtk_widget_get_preferred_size(gui.menubar, &req, NULL);
	height += req.height;
    }
#endif
#ifdef FEAT_TOOLBAR
    if (gui.toolbar != NULL && gtk_widget_get_visible(gui.toolbar))
    {
	GtkRequisition req;
	gtk_widget_get_preferred_size(gui.toolbar, &req, NULL);
	height += req.height;
    }
#endif
    return height;
}

/*
 * Get the GdkClipboard for the given Clipboard_T.
 * clip_star (*) uses PRIMARY, clip_plus (+) uses CLIPBOARD.
 */
    static GdkClipboard *
gtk4_get_clipboard(Clipboard_T *cbd)
{
    GdkDisplay *display;

    if (gui.mainwin == NULL)
	return NULL;

    display = gtk_widget_get_display(gui.mainwin);
    if (display == NULL)
	return NULL;

    if (cbd == &clip_plus)
	return gdk_display_get_clipboard(display);
    else
	return gdk_display_get_primary_clipboard(display);
}

/*
 * Callback for gdk_clipboard_read_text_async().
 */
    static void
clip_read_text_cb(GObject *source, GAsyncResult *result, gpointer user_data)
{
    GdkClipboard	*clipboard = GDK_CLIPBOARD(source);
    Clipboard_T		*cbd = (Clipboard_T *)user_data;
    char		*text;
    GError		*error = NULL;

    text = gdk_clipboard_read_text_finish(clipboard, result, &error);
    if (text != NULL)
    {
	char_u	*tmpbuf = NULL;
	char_u	*p;
	int	len;
	int	motion_type = MAUTO;

	len = (int)STRLEN(text);

	// Convert from UTF-8 to 'encoding' if needed.
	if (input_conv.vc_type != CONV_NONE)
	{
	    tmpbuf = string_convert(&input_conv, (char_u *)text, &len);
	    if (tmpbuf != NULL)
		p = tmpbuf;
	    else
		p = (char_u *)text;
	}
	else
	    p = (char_u *)text;

	// Chop off any trailing NUL bytes.
	while (len > 0 && p[len - 1] == NUL)
	    --len;

	clip_yank_selection(motion_type, p, (long)len, cbd);
	vim_free(tmpbuf);
	g_free(text);
    }
    else
    {
	if (error != NULL)
	    g_error_free(error);
    }
}

/*
 * Request the selection from the clipboard.
 */
    void
clip_mch_request_selection(Clipboard_T *cbd)
{
    GdkClipboard	*clipboard;
    time_t		start;

    clipboard = gtk4_get_clipboard(cbd);
    if (clipboard == NULL)
	return;

    gdk_clipboard_read_text_async(clipboard, NULL, clip_read_text_cb, cbd);

    // Wait up to three seconds for the clipboard response.
    start = time(NULL);
    while (time(NULL) < start + 3)
    {
	g_main_context_iteration(NULL, TRUE);
	// Check if the clipboard content was already yanked by the callback.
	// The callback calls clip_yank_selection() which sets cbd->owned.
	// We break out once an iteration completes without pending events,
	// giving the async callback time to fire.
	if (!g_main_context_pending(NULL))
	    break;
    }
}

/*
 * Send the current selection to the clipboard.
 */
    void
clip_mch_set_selection(Clipboard_T *cbd)
{
    GdkClipboard	*clipboard;
    char_u		*str = NULL;
    long_u		len;
    int			motion_type;

    clipboard = gtk4_get_clipboard(cbd);
    if (clipboard == NULL)
	return;

    // Get the selection text from the register.
    clip_get_selection(cbd);
    motion_type = clip_convert_selection(&str, &len, cbd);
    if (motion_type < 0 || str == NULL)
	return;

    // Convert from 'encoding' to UTF-8 if needed.
    if (output_conv.vc_type != CONV_NONE)
    {
	char_u	*conv_str;
	int	conv_len = (int)len;

	conv_str = string_convert(&output_conv, str, &conv_len);
	if (conv_str != NULL)
	{
	    vim_free(str);
	    str = conv_str;
	    len = conv_len;
	}
    }

    // Ensure NUL-terminated string for GTK.
    {
	char_u *nul_str = alloc(len + 1);

	if (nul_str != NULL)
	{
	    mch_memmove(nul_str, str, len);
	    nul_str[len] = NUL;
	    gdk_clipboard_set_text(clipboard, (const char *)nul_str);
	    vim_free(nul_str);
	}
    }

    vim_free(str);
}

/*
 * Own the selection.  In GTK4, ownership is implicit when content is set
 * on the clipboard.  Return OK to indicate we can own it.
 */
    int
clip_mch_own_selection(Clipboard_T *cbd UNUSED)
{
    return OK;
}

/*
 * Disown the selection.  In GTK4, we clear the clipboard content to
 * release ownership.
 */
    void
clip_mch_lose_selection(Clipboard_T *cbd)
{
    GdkClipboard *clipboard;

    clipboard = gtk4_get_clipboard(cbd);
    if (clipboard == NULL)
	return;

    // Setting NULL content provider releases ownership.
    gdk_clipboard_set_content(clipboard, NULL);
}

// Balloon eval - use GTK4 tooltip
    void
gui_mch_post_balloon(BalloonEval *beval UNUSED, char_u *mesg)
{
    if (mesg != NULL && gui.drawarea != NULL)
    {
	char_u *text = CONVERT_TO_UTF8(mesg);
	gtk_widget_set_tooltip_text(gui.drawarea, (const char *)text);
	CONVERT_TO_UTF8_FREE(text);
    }
    else if (gui.drawarea != NULL)
	gtk_widget_set_tooltip_text(gui.drawarea, NULL);
}

    BalloonEval *
gui_mch_create_beval_area(void *target UNUSED, char_u *mesg UNUSED,
	void (*mesgCB)(BalloonEval *, int) UNUSED, void *clientData UNUSED)
{
    return NULL;
}

    void
gui_mch_enable_beval_area(BalloonEval *beval UNUSED)
{
}

    void
gui_mch_disable_beval_area(BalloonEval *beval UNUSED)
{
}

// GTK4 does not have gtk_main_level/gtk_main_quit.
// Provide compatibility stubs using a simple flag.
    guint
gtk_main_level(void)
{
    return gtk4_main_loop_level;
}

    void
gtk_main_quit(void)
{
    gtk4_main_loop_quit = TRUE;
}

#if defined(FEAT_MOUSESHAPE)

// Table of CSS cursor names corresponding to Vim's mouse shape IDs.
// Keep in sync with the mshape_names[] table in misc2.c.
static const char *mshape_css_names[] =
{
    "default",			// arrow
    "none",			// blank
    "text",			// beam
    "ns-resize",		// updown
    "nwse-resize",		// udsizing
    "ew-resize",		// leftright
    "ew-resize",		// lrsizing
    "progress",			// busy
    "not-allowed",		// no
    "crosshair",		// crosshair
    "pointer",			// hand1
    "pointer",			// hand2
    "default",			// pencil (no CSS analogue)
    "help",			// question
    "default",			// right-arrow (no CSS analogue)
    "default",			// up-arrow (no CSS analogue)
    "default"			// last entry
};

    void
mch_set_mouse_shape(int shape)
{
    GdkCursor	*c;
    const char	*css_name = "default";

    if (gui.drawarea == NULL)
	return;

    if (shape == MSHAPE_HIDE || gui.pointer_hidden)
	gtk_widget_set_cursor(gui.drawarea, gui.blank_pointer);
    else
    {
	if (shape >= MSHAPE_NUMBERED)
	    css_name = "default";
	else if (shape < (int)ARRAY_LENGTH(mshape_css_names))
	    css_name = mshape_css_names[shape];
	else
	    return;

	// GTK4: gdk_cursor_new_from_name(name, fallback)
	c = gdk_cursor_new_from_name(css_name, NULL);
	gtk_widget_set_cursor(gui.drawarea, c);
	g_object_unref(G_OBJECT(c));
    }
    if (shape != MSHAPE_HIDE)
	last_shape = shape;
}

#else // !FEAT_MOUSESHAPE

    void
mch_set_mouse_shape(int shape UNUSED)
{
}

#endif // FEAT_MOUSESHAPE



/*
 * Menus, scrollbars, dialogs, toolbar.
 * (merged from gui_gtk4.c)
 */



static int last_text_area_w = 0;
static int last_text_area_h = 0;

/*
 * ============================================================
 * Menu functions
 * ============================================================
 * TODO: Implement using GMenu + GtkPopoverMenuBar
 */

/*
 * Icon name table for toolbar buttons.
 * Must match toolbar_names[] in menu.c.
 */
static const char * const toolbar_icon_names[] =
{
    /* 00 */ "document-new",
    /* 01 */ "document-open",
    /* 02 */ "document-save",
    /* 03 */ "edit-undo",
    /* 04 */ "edit-redo",
    /* 05 */ "edit-cut",
    /* 06 */ "edit-copy",
    /* 07 */ "edit-paste",
    /* 08 */ "document-print",
    /* 09 */ "help-browser",
    /* 10 */ "edit-find",
    /* 11 */ "document-save",		// save all (no standard icon)
    /* 12 */ "document-save",		// session save
    /* 13 */ "document-new",		// session new
    /* 14 */ "document-open",		// session load
    /* 15 */ "system-run",
    /* 16 */ "edit-find-replace",
    /* 17 */ "window-close",
    /* 18 */ "window-maximize-symbolic",	// maximize
    /* 19 */ "window-minimize-symbolic",	// minimize
    /* 20 */ "window-maximize-symbolic",	// split (no standard icon)
    /* 21 */ "utilities-terminal",	// shell
    /* 22 */ "go-previous",
    /* 23 */ "go-next",
    /* 24 */ "help-browser",		// find help
    /* 25 */ "edit-find",		// convert (no standard icon)
    /* 26 */ "go-jump",
    /* 27 */ "go-previous",		// back (reuse)
    /* 28 */ "go-next",			// forward (reuse)
    /* 29 */ "image-missing",
    /* 30 */ "image-missing",
};

    static void
toolbar_button_clicked_cb(GtkWidget *widget UNUSED, gpointer data)
{
    gui_menu_cb((vimmenu_T *)data);
}

    static GtkWidget *
create_toolbar_icon(vimmenu_T *menu)
{
    char_u	buf[MAXPATHL];
    GtkWidget	*image = NULL;

    // Try specified icon file first
    if (menu->iconfile != NULL)
    {
	expand_env(menu->iconfile, buf, MAXPATHL);
	if (vim_fexists(buf))
	{
	    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file_at_scale(
		    (const char *)buf, 24, 24, TRUE, NULL);
	    if (pixbuf != NULL)
	    {
		image = gtk_image_new_from_pixbuf(pixbuf);
		g_object_unref(pixbuf);
	    }
	}
    }

    // Use themed icon
    if (image == NULL)
    {
	const char *icon_name = "image-missing";
	int n = (int)ARRAY_LENGTH(toolbar_icon_names);

	if (menu->iconidx >= 0 && menu->iconidx < n)
	    icon_name = toolbar_icon_names[menu->iconidx];

	image = gtk_image_new_from_icon_name(icon_name);
    }

    return image;
}

/*
 * GTK4 Menu system using GMenu + GSimpleActionGroup + GtkPopoverMenuBar.
 *
 * Each menu/submenu has a GMenu stored in menu->submenu_id (cast to
 * GtkWidget* to fit the struct field type).
 * Actions are added to a GSimpleActionGroup attached to gui.mainwin.
 */

static GSimpleActionGroup *menu_action_group = NULL;
static int menu_action_id = 0;

    static void
menu_action_cb(GSimpleAction *action UNUSED, GVariant *parameter UNUSED,
	gpointer data)
{
    gui_menu_cb((vimmenu_T *)data);
}

    static char *
make_action_name(vimmenu_T *menu)
{
    // Create a unique action name from the menu pointer
    static char buf[64];
    vim_snprintf(buf, sizeof(buf), "menu%d", menu_action_id++);
    return buf;
}

    void
gui_mch_add_menu(vimmenu_T *menu, int idx UNUSED)
{
    GMenu *submenu;

    if (menu->name[0] == ']' || menu_is_popup(menu->name))
    {
	// Popup menus - just create a GMenu, don't add to menubar
	submenu = g_menu_new();
	menu->submenu_id = (GtkWidget *)(gpointer)submenu;
	return;
    }

    if (menu->parent != NULL && menu->parent->submenu_id == NULL)
	return;
    if (!menu_is_menubar(menu->name))
	return;

    // Create a submenu for this menu
    submenu = g_menu_new();
    menu->submenu_id = (GtkWidget *)(gpointer)submenu;

    // Add to parent menu or menubar's model
    {
	GMenu *parent_menu;
	char_u *label;

	label = CONVERT_TO_UTF8(menu->dname);

	if (menu->parent != NULL)
	    parent_menu = (GMenu *)(gpointer)menu->parent->submenu_id;
	else
	    parent_menu = (GMenu *)(gpointer)g_object_get_data(
		    G_OBJECT(gui.menubar), "vim-gmenu");

	if (parent_menu != NULL)
	    g_menu_append_submenu(parent_menu, (const char *)label,
		    G_MENU_MODEL(submenu));

	CONVERT_TO_UTF8_FREE(label);
    }
}

    void
gui_mch_add_menu_item(vimmenu_T *menu, int idx UNUSED)
{
    vimmenu_T *parent = menu->parent;

#ifdef FEAT_TOOLBAR
    if (parent != NULL && menu_is_toolbar(parent->name))
    {
	if (menu_is_separator(menu->name))
	{
	    GtkWidget *sep = gtk_separator_new(GTK_ORIENTATION_VERTICAL);
	    gtk_box_append(GTK_BOX(gui.toolbar), sep);
	    menu->id = sep;
	}
	else
	{
	    GtkWidget	*btn;
	    GtkWidget	*icon;
	    char_u	*tooltip;

	    icon = create_toolbar_icon(menu);
	    btn = gtk_button_new();
	    gtk_button_set_child(GTK_BUTTON(btn), icon);
	    gtk_widget_set_focusable(btn, FALSE);

	    tooltip = CONVERT_TO_UTF8(menu->strings[MENU_INDEX_TIP]);
	    if (tooltip != NULL && utf_valid_string(tooltip, NULL))
		gtk_widget_set_tooltip_text(btn, (const gchar *)tooltip);
	    CONVERT_TO_UTF8_FREE(tooltip);

	    g_signal_connect(btn, "clicked",
		    G_CALLBACK(toolbar_button_clicked_cb), menu);

	    gtk_box_append(GTK_BOX(gui.toolbar), btn);
	    menu->id = btn;
	}
	return;
    }
#endif

    // Menu items (non-toolbar)
    if (parent == NULL || parent->submenu_id == NULL)
	return;

    {
	GMenu *parent_menu = (GMenu *)(gpointer)parent->submenu_id;

	if (menu_is_separator(menu->name))
	{
	    // GMenu doesn't have real separators; use a section
	    GMenu *section = g_menu_new();
	    g_menu_append_section(parent_menu, NULL, G_MENU_MODEL(section));
	    g_object_unref(section);
	    menu->id = NULL;
	}
	else
	{
	    char	*action_name;
	    char	detailed[80];
	    char_u	*label;
	    GSimpleAction *action;

	    // Create a unique action
	    action_name = make_action_name(menu);
	    action = g_simple_action_new(action_name, NULL);
	    g_signal_connect(action, "activate",
		    G_CALLBACK(menu_action_cb), menu);

	    if (menu_action_group == NULL)
	    {
		menu_action_group = g_simple_action_group_new();
		gtk_widget_insert_action_group(gui.mainwin, "menu",
			G_ACTION_GROUP(menu_action_group));
	    }
	    g_action_map_add_action(G_ACTION_MAP(menu_action_group),
		    G_ACTION(action));
	    g_object_unref(action);

	    label = CONVERT_TO_UTF8(menu->dname);
	    vim_snprintf(detailed, sizeof(detailed), "menu.%s", action_name);
	    g_menu_append(parent_menu, (const char *)label, detailed);
	    CONVERT_TO_UTF8_FREE(label);

	    menu->id = (GtkWidget *)1;  // non-NULL marker
	}
    }
}

    void
gui_mch_toggle_tearoffs(int enable UNUSED)
{
    // GTK4: tearoff menus don't exist.
}

    void
gui_mch_menu_set_tip(vimmenu_T *menu UNUSED)
{
}

    void
gui_mch_destroy_menu(vimmenu_T *menu)
{
    // For toolbar buttons, remove from toolbar
    if (menu->id != NULL && menu->id != (GtkWidget *)1)
    {
	GtkWidget *parent_widget = gtk_widget_get_parent(menu->id);
	if (parent_widget != NULL)
	    gtk_box_remove(GTK_BOX(parent_widget), menu->id);
	menu->id = NULL;
    }
    else
	menu->id = NULL;

    // GMenu items cannot be individually removed easily.
    // The submenu GMenu is unreffed if present.
    if (menu->submenu_id != NULL)
    {
	// Don't unref - GMenu may be referenced by the model
	menu->submenu_id = NULL;
    }
}

    void
gui_mch_show_popupmenu(vimmenu_T *menu)
{
    GMenu *gmenu;
    GtkWidget *popover;

    if (menu == NULL || menu->submenu_id == NULL)
	return;

    gmenu = (GMenu *)(gpointer)menu->submenu_id;
    popover = gtk_popover_menu_new_from_model(G_MENU_MODEL(gmenu));
    gtk_widget_set_parent(popover, gui.drawarea);
    gtk_popover_popup(GTK_POPOVER(popover));
}

/*
 * ============================================================
 * Scrollbar functions
 * ============================================================
 */

    void
gui_mch_set_scrollbar_thumb(scrollbar_T *sb, long val, long size, long max)
{
    GtkAdjustment *adj;

    if (sb->id == NULL)
	return;
    if (!GTK_IS_WIDGET(sb->id) || !GTK_IS_RANGE(sb->id))
	return;

    adj = gtk_range_get_adjustment(GTK_RANGE(sb->id));
    gtk_adjustment_set_lower(adj, 0.0);
    gtk_adjustment_set_upper(adj, (gdouble)max + 1);
    gtk_adjustment_set_value(adj, (gdouble)val);
    gtk_adjustment_set_step_increment(adj, 1.0);
    gtk_adjustment_set_page_increment(adj, (gdouble)(size > 2 ? size - 2 : 1));
    gtk_adjustment_set_page_size(adj, (gdouble)size);
}

    void
gui_mch_set_scrollbar_pos(scrollbar_T *sb, int x, int y, int w, int h)
{
    if (sb->id != NULL)
    {
	gtk_widget_set_size_request(sb->id, w, h);
	gui_gtk_form_move(GTK_FORM(gui.formwin), sb->id, x, y);
    }
}

    int
gui_mch_get_scrollbar_xpadding(void)
{
    int formwin_w = gtk_widget_get_width(gui.formwin);
    int sbar_w = 0;
    int xpad;

    if (gui.which_scrollbars[SBAR_LEFT])
	sbar_w += gui.scrollbar_width;
    if (gui.which_scrollbars[SBAR_RIGHT])
	sbar_w += gui.scrollbar_width;

    xpad = formwin_w - last_text_area_w - sbar_w;
    return (xpad < 0) ? 0 : xpad;
}

    int
gui_mch_get_scrollbar_ypadding(void)
{
    int formwin_h = gtk_widget_get_height(gui.formwin);
    int ypad;

    ypad = formwin_h - last_text_area_h;
    if (gui.which_scrollbars[SBAR_BOTTOM])
	ypad -= gui.scrollbar_height;

    return (ypad < 0) ? 0 : ypad;
}

    static void
adjustment_value_changed(GtkAdjustment *adj, gpointer data UNUSED)
{
    scrollbar_T *sb = (scrollbar_T *)g_object_get_data(G_OBJECT(adj), "vim-sb");
    long value = (long)gtk_adjustment_get_value(adj);

    if (sb != NULL)
	gui_drag_scrollbar(sb, value, FALSE);
}

    void
gui_mch_create_scrollbar(scrollbar_T *sb, int orient)
{
    if (orient == SBAR_HORIZ)
	sb->id = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, NULL);
    else
	sb->id = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, NULL);

    if (sb->id != NULL && GTK_IS_RANGE(sb->id))
    {
	GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(sb->id));

	gtk_widget_set_visible(sb->id, FALSE);
	gui_gtk_form_put(GTK_FORM(gui.formwin), sb->id, 0, 0);
	if (adj != NULL && G_IS_OBJECT(adj))
	{
	    g_object_set_data(G_OBJECT(adj), "vim-sb", (gpointer)sb);
	    g_signal_connect(G_OBJECT(adj), "value-changed",
		    G_CALLBACK(adjustment_value_changed), NULL);
	}
    }
}

    void
gui_mch_destroy_scrollbar(scrollbar_T *sb)
{
    if (sb->id != NULL)
    {
	gui_gtk_form_remove(GTK_FORM(gui.formwin), sb->id);
	sb->id = NULL;
    }
}

/*
 * ============================================================
 * Text area position
 * ============================================================
 */

    void
gui_mch_set_text_area_pos(int x, int y, int w, int h)
{
    last_text_area_w = w;
    last_text_area_h = h;
    // Don't use gui_gtk_form_move_resize for drawarea because its
    // set_size_request would prevent the window from shrinking.
    // Just update position; the actual allocation is handled by
    // form_size_allocate which gives drawarea the formwin's full size.
    gui_gtk_form_move(GTK_FORM(gui.formwin), gui.drawarea, x, y);

    // Update surface to match new text area size
    if (w > 0 && h > 0)
    {
	if (gui.surface != NULL)
	{
	    int sw = cairo_image_surface_get_width(gui.surface);
	    int sh = cairo_image_surface_get_height(gui.surface);
	    if (sw == w && sh == h)
		return;
	    cairo_surface_destroy(gui.surface);
	}
	gui.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    }
}

/*
 * ============================================================
 * Browse dialogs
 * ============================================================
 */

/*
 * Blocking helper: run a GtkFileDialog and wait for result.
 */
typedef struct {
    GFile	*result;
    gboolean	done;
} FileDialogData;

    static void
file_dialog_open_cb(GObject *source, GAsyncResult *res, gpointer data)
{
    FileDialogData *fdd = (FileDialogData *)data;
    fdd->result = gtk_file_dialog_open_finish(
		    GTK_FILE_DIALOG(source), res, NULL);
    fdd->done = TRUE;
}

    static void
file_dialog_save_cb(GObject *source, GAsyncResult *res, gpointer data)
{
    FileDialogData *fdd = (FileDialogData *)data;
    fdd->result = gtk_file_dialog_save_finish(
		    GTK_FILE_DIALOG(source), res, NULL);
    fdd->done = TRUE;
}

    static void
file_dialog_folder_cb(GObject *source, GAsyncResult *res, gpointer data)
{
    FileDialogData *fdd = (FileDialogData *)data;
    fdd->result = gtk_file_dialog_select_folder_finish(
		    GTK_FILE_DIALOG(source), res, NULL);
    fdd->done = TRUE;
}

    char_u *
gui_mch_browse(int saving,
	char_u *title,
	char_u *dflt,
	char_u *ext UNUSED,
	char_u *initdir,
	char_u *filter UNUSED)
{
    GtkFileDialog	*dlg;
    FileDialogData	fdd;
    char_u		dirbuf[MAXPATHL];
    char_u		*result = NULL;

    title = CONVERT_TO_UTF8(title);

    if (initdir == NULL || *initdir == NUL)
	mch_dirname(dirbuf, MAXPATHL);
    else if (vim_FullName(initdir, dirbuf, MAXPATHL - 2, FALSE) == FAIL)
	dirbuf[0] = NUL;
    add_pathsep(dirbuf);

    gui_mch_mousehide(FALSE);

    dlg = gtk_file_dialog_new();
    gtk_file_dialog_set_modal(dlg, TRUE);
    if (title != NULL)
	gtk_file_dialog_set_title(dlg, (const char *)title);

    {
	GFile *dir = g_file_new_for_path((const char *)dirbuf);
	gtk_file_dialog_set_initial_folder(dlg, dir);
	g_object_unref(dir);
    }

    if (saving && dflt != NULL && *dflt != NUL)
	gtk_file_dialog_set_initial_name(dlg, (const char *)dflt);

    fdd.result = NULL;
    fdd.done = FALSE;

    if (saving)
	gtk_file_dialog_save(dlg, GTK_WINDOW(gui.mainwin), NULL,
		file_dialog_save_cb, &fdd);
    else
	gtk_file_dialog_open(dlg, GTK_WINDOW(gui.mainwin), NULL,
		file_dialog_open_cb, &fdd);

    while (!fdd.done)
	g_main_context_iteration(NULL, TRUE);

    if (fdd.result != NULL)
    {
	char *path = g_file_get_path(fdd.result);
	if (path != NULL)
	{
	    result = vim_strsave((char_u *)path);
	    g_free(path);
	}
	g_object_unref(fdd.result);
    }

    g_object_unref(dlg);
    CONVERT_TO_UTF8_FREE(title);

    return result;
}

    char_u *
gui_mch_browsedir(char_u *title, char_u *initdir)
{
    GtkFileDialog	*dlg;
    FileDialogData	fdd;
    char_u		*result = NULL;

    title = CONVERT_TO_UTF8(title);
    gui_mch_mousehide(FALSE);

    dlg = gtk_file_dialog_new();
    gtk_file_dialog_set_modal(dlg, TRUE);
    if (title != NULL)
	gtk_file_dialog_set_title(dlg, (const char *)title);

    if (initdir != NULL && *initdir != NUL)
    {
	GFile *dir = g_file_new_for_path((const char *)initdir);
	gtk_file_dialog_set_initial_folder(dlg, dir);
	g_object_unref(dir);
    }

    fdd.result = NULL;
    fdd.done = FALSE;

    gtk_file_dialog_select_folder(dlg, GTK_WINDOW(gui.mainwin), NULL,
	    file_dialog_folder_cb, &fdd);

    while (!fdd.done)
	g_main_context_iteration(NULL, TRUE);

    if (fdd.result != NULL)
    {
	char *path = g_file_get_path(fdd.result);
	if (path != NULL)
	{
	    result = vim_strsave((char_u *)path);
	    g_free(path);
	}
	g_object_unref(fdd.result);
    }

    g_object_unref(dlg);
    CONVERT_TO_UTF8_FREE(title);

    return result;
}

/*
 * ============================================================
 * Message dialog
 * ============================================================
 */

typedef struct {
    int		response;
    gboolean	done;
} AlertDialogData;

    static void
alert_dialog_cb(GObject *source, GAsyncResult *res, gpointer data)
{
    AlertDialogData *add = (AlertDialogData *)data;
    add->response = gtk_alert_dialog_choose_finish(
		    GTK_ALERT_DIALOG(source), res, NULL);
    add->done = TRUE;
}

    int
gui_mch_dialog(
	int	type UNUSED,
	char_u	*title,
	char_u	*message,
	char_u	*buttons,
	int	dfltbutton,
	char_u	*textfield UNUSED,
	int	ex_cmd UNUSED)
{
    GtkAlertDialog	*dlg;
    AlertDialogData	add;
    char_u		*p;
    int			butcount = 0;
    const char		*btn_labels[64];

    title = CONVERT_TO_UTF8(title);
    message = CONVERT_TO_UTF8(message);

    // Parse button labels from the "&Yes\n&No\n&Cancel" format
    if (buttons != NULL)
    {
	char_u *buf = vim_strsave(buttons);
	if (buf != NULL)
	{
	    p = buf;
	    while (*p != NUL && butcount < 63)
	    {
		char_u *start = p;
		while (*p != NUL && *p != '\n')
		    ++p;
		if (*p == '\n')
		    *p++ = NUL;
		// Skip '&' mnemonic marker
		if (*start == '&')
		    ++start;
		btn_labels[butcount++] = (const char *)
		    CONVERT_TO_UTF8(start);
	    }
	    vim_free(buf);
	}
    }
    btn_labels[butcount] = NULL;

    dlg = gtk_alert_dialog_new("%s", message ? (char *)message : "");
    if (title != NULL)
	gtk_alert_dialog_set_detail(dlg, (const char *)title);
    gtk_alert_dialog_set_buttons(dlg, btn_labels);
    gtk_alert_dialog_set_modal(dlg, TRUE);

    if (dfltbutton > 0 && dfltbutton <= butcount)
	gtk_alert_dialog_set_default_button(dlg, dfltbutton - 1);
    gtk_alert_dialog_set_cancel_button(dlg, butcount - 1);

    add.response = -1;
    add.done = FALSE;

    gtk_alert_dialog_choose(dlg, GTK_WINDOW(gui.mainwin), NULL,
	    alert_dialog_cb, &add);

    while (!add.done)
	g_main_context_iteration(NULL, TRUE);

    g_object_unref(dlg);

    CONVERT_TO_UTF8_FREE(title);
    CONVERT_TO_UTF8_FREE(message);

    // GTK returns 0-based index, Vim wants 1-based
    return add.response >= 0 ? add.response + 1 : 0;
}

/*
 * ============================================================
 * Find/Replace dialogs
 * ============================================================
 */

/*
 * ============================================================
 * Find/Replace dialog
 * ============================================================
 */

typedef struct
{
    GtkWidget *dialog;
    GtkWidget *what;	    // Find what entry
    GtkWidget *with;	    // Replace with entry
    GtkWidget *wword;	    // Whole word check
    GtkWidget *mcase;	    // Match case check
    GtkWidget *up;	    // Direction up radio
    GtkWidget *down;	    // Direction down radio
} SharedFindReplace;

static SharedFindReplace find_widgets = {0};
static SharedFindReplace repl_widgets = {0};

    static void
find_replace_cb(GtkWidget *widget UNUSED, gpointer data)
{
    int			flags;
    char_u		*find_text;
    char_u		*repl_text;
    gboolean		direction_down;
    SharedFindReplace	*sfr;

    flags = (int)(long)data;

    if (flags == FRD_FINDNEXT)
    {
	repl_text = NULL;
	sfr = &find_widgets;
    }
    else
    {
	repl_text = (char_u *)gtk_editable_get_text(
		GTK_EDITABLE(repl_widgets.with));
	sfr = &repl_widgets;
    }

    find_text = (char_u *)gtk_editable_get_text(GTK_EDITABLE(sfr->what));
    direction_down = gtk_check_button_get_active(
	    GTK_CHECK_BUTTON(sfr->down));

    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(sfr->wword)))
	flags |= FRD_WHOLE_WORD;
    if (gtk_check_button_get_active(GTK_CHECK_BUTTON(sfr->mcase)))
	flags |= FRD_MATCH_CASE;

    repl_text = CONVERT_FROM_UTF8(repl_text);
    find_text = CONVERT_FROM_UTF8(find_text);
    gui_do_findrepl(flags, find_text, repl_text, direction_down);
    CONVERT_FROM_UTF8_FREE(repl_text);
    CONVERT_FROM_UTF8_FREE(find_text);
}

    static void
dialog_destroyed_cb(GtkWidget *widget UNUSED, gpointer data)
{
    *(GtkWidget **)data = NULL;
}

    static void
find_replace_dialog_create(char_u *arg, int do_replace)
{
    SharedFindReplace	*frdp;
    char_u		*entry_text;
    int			wword = FALSE;
    int			mcase = !p_ic;
    GtkWidget		*vbox, *grid, *hbox, *tmp, *btn;
    gboolean		sensitive;

    frdp = do_replace ? &repl_widgets : &find_widgets;
    entry_text = get_find_dialog_text(arg, &wword, &mcase);

    if (entry_text != NULL && output_conv.vc_type != CONV_NONE)
    {
	char_u *old = entry_text;
	entry_text = string_convert(&output_conv, entry_text, NULL);
	vim_free(old);
    }

    // If the dialog already exists, just raise it.
    if (frdp->dialog)
    {
	if (entry_text != NULL)
	{
	    gtk_editable_set_text(GTK_EDITABLE(frdp->what),
		    (char *)entry_text);
	    gtk_check_button_set_active(GTK_CHECK_BUTTON(frdp->wword),
		    (gboolean)wword);
	    gtk_check_button_set_active(GTK_CHECK_BUTTON(frdp->mcase),
		    (gboolean)mcase);
	}
	gtk_window_present(GTK_WINDOW(frdp->dialog));
	gtk_widget_grab_focus(frdp->what);
	vim_free(entry_text);
	return;
    }

    // Create a new dialog window.
    frdp->dialog = gtk_window_new();
    gtk_window_set_transient_for(GTK_WINDOW(frdp->dialog),
	    GTK_WINDOW(gui.mainwin));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(frdp->dialog), TRUE);
    gtk_window_set_title(GTK_WINDOW(frdp->dialog),
	    do_replace ? _("VIM - Search and Replace...")
		       : _("VIM - Search..."));
    gtk_window_set_resizable(GTK_WINDOW(frdp->dialog), FALSE);

    g_signal_connect(frdp->dialog, "destroy",
	    G_CALLBACK(dialog_destroyed_cb), &frdp->dialog);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(vbox, 12);
    gtk_widget_set_margin_end(vbox, 12);
    gtk_widget_set_margin_top(vbox, 12);
    gtk_widget_set_margin_bottom(vbox, 12);
    gtk_window_set_child(GTK_WINDOW(frdp->dialog), vbox);

    // Grid for labels + entries
    grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
    gtk_box_append(GTK_BOX(vbox), grid);

    // "Find what:" label + entry
    tmp = gtk_label_new(_("Find what:"));
    gtk_label_set_xalign(GTK_LABEL(tmp), 0.0);
    gtk_grid_attach(GTK_GRID(grid), tmp, 0, 0, 1, 1);

    frdp->what = gtk_entry_new();
    gtk_widget_set_hexpand(frdp->what, TRUE);
    sensitive = (entry_text != NULL && entry_text[0] != NUL);
    if (entry_text != NULL)
	gtk_editable_set_text(GTK_EDITABLE(frdp->what), (char *)entry_text);
    gtk_grid_attach(GTK_GRID(grid), frdp->what, 1, 0, 1, 1);

    if (do_replace)
    {
	// "Replace with:" label + entry
	tmp = gtk_label_new(_("Replace with:"));
	gtk_label_set_xalign(GTK_LABEL(tmp), 0.0);
	gtk_grid_attach(GTK_GRID(grid), tmp, 0, 1, 1, 1);

	frdp->with = gtk_entry_new();
	gtk_widget_set_hexpand(frdp->with, TRUE);
	gtk_grid_attach(GTK_GRID(grid), frdp->with, 1, 1, 1, 1);
    }

    // Checkboxes
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_append(GTK_BOX(vbox), hbox);

    frdp->wword = gtk_check_button_new_with_label(_("Match whole word only"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(frdp->wword),
	    (gboolean)wword);
    gtk_box_append(GTK_BOX(hbox), frdp->wword);

    frdp->mcase = gtk_check_button_new_with_label(_("Match case"));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(frdp->mcase),
	    (gboolean)mcase);
    gtk_box_append(GTK_BOX(hbox), frdp->mcase);

    // Direction radio buttons
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_box_append(GTK_BOX(vbox), hbox);

    tmp = gtk_label_new(_("Direction:"));
    gtk_box_append(GTK_BOX(hbox), tmp);

    frdp->up = gtk_check_button_new_with_label(_("Up"));
    gtk_box_append(GTK_BOX(hbox), frdp->up);

    frdp->down = gtk_check_button_new_with_label(_("Down"));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(frdp->down),
	    GTK_CHECK_BUTTON(frdp->up));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(frdp->down), TRUE);
    gtk_box_append(GTK_BOX(hbox), frdp->down);

    // Action buttons
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_widget_set_halign(hbox, GTK_ALIGN_END);
    gtk_box_append(GTK_BOX(vbox), hbox);

    btn = gtk_button_new_with_label(_("Find Next"));
    gtk_widget_set_sensitive(btn, sensitive);
    g_signal_connect(btn, "clicked", G_CALLBACK(find_replace_cb),
	    GINT_TO_POINTER(do_replace ? FRD_R_FINDNEXT : FRD_FINDNEXT));
    gtk_box_append(GTK_BOX(hbox), btn);

    if (do_replace)
    {
	btn = gtk_button_new_with_label(_("Replace"));
	g_signal_connect(btn, "clicked", G_CALLBACK(find_replace_cb),
		GINT_TO_POINTER(FRD_REPLACE));
	gtk_box_append(GTK_BOX(hbox), btn);

	btn = gtk_button_new_with_label(_("Replace All"));
	g_signal_connect(btn, "clicked", G_CALLBACK(find_replace_cb),
		GINT_TO_POINTER(FRD_REPLACEALL));
	gtk_box_append(GTK_BOX(hbox), btn);
    }

    btn = gtk_button_new_with_label(_("Close"));
    g_signal_connect_swapped(btn, "clicked",
	    G_CALLBACK(gtk_window_destroy), frdp->dialog);
    gtk_box_append(GTK_BOX(hbox), btn);

    // Connect Enter key in entry to Find Next
    g_signal_connect_swapped(frdp->what, "activate",
	    G_CALLBACK(find_replace_cb),
	    GINT_TO_POINTER(do_replace ? FRD_R_FINDNEXT : FRD_FINDNEXT));

    gtk_window_present(GTK_WINDOW(frdp->dialog));
    gtk_widget_grab_focus(frdp->what);
    if (do_replace && entry_text != NULL && entry_text[0] != NUL)
	gtk_widget_grab_focus(frdp->with);

    vim_free(entry_text);
}

    void
gui_mch_find_dialog(exarg_T *eap)
{
    if (gui.in_use)
	find_replace_dialog_create(eap->arg, FALSE);
}

    void
gui_mch_replace_dialog(exarg_T *eap)
{
    if (gui.in_use)
	find_replace_dialog_create(eap->arg, TRUE);
}

/*
 * ============================================================
 * Help find (for :helpfind command)
 * ============================================================
 */

    void
ex_helpfind(exarg_T *eap UNUSED)
{
    do_cmdline_cmd((char_u *)"emenu ToolBar.FindHelp");
}


#endif // FEAT_GUI_GTK
