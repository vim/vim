/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 *
 * GTK4 GUI implementation.
 * This is a clean implementation for GTK4, separate from gui_gtk_x11.c
 * which handles GTK2/GTK3.
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

    // If GDK_BACKEND is not set and both X11 and Wayland are available,
    // prefer X11 for now.  Wayland support in GTK4 has issues with
    // keyboard layout detection on some platforms (e.g., WSL2).
    if (g_getenv("GDK_BACKEND") == NULL && g_getenv("DISPLAY") != NULL)
	g_setenv("GDK_BACKEND", "x11", FALSE);

    gtk_init();
}

/*
 * Free all GUI related resources.
 */
    void
gui_mch_free_all(void)
{
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

    void
gui_mch_set_blinking(long waittime UNUSED, long on UNUSED, long off UNUSED)
{
    // TODO: implement cursor blinking
}

    void
gui_mch_stop_blink(int may_call_gui_update_cursor UNUSED)
{
    // TODO
}

    void
gui_mch_start_blink(void)
{
    // TODO
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
    gui.menubar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(gui.menubar, "vim-menubar");
    // Don't add to vbox until actually shown
#endif

#ifdef FEAT_TOOLBAR
    gui.toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_name(gui.toolbar, "vim-toolbar");
    // Don't add to vbox until actually shown
#endif

#ifdef FEAT_GUI_TABLINE
    gui.tabline = gtk_notebook_new();
    gtk_notebook_set_show_border(GTK_NOTEBOOK(gui.tabline), FALSE);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(gui.tabline), FALSE);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(gui.tabline), TRUE);
    // Don't add to vbox until actually shown
#endif

    // The form widget manages absolute positioning of drawarea + scrollbars.
    gui.formwin = gui_gtk_form_new();
    gtk_widget_set_name(gui.formwin, "vim-gtk-form");
    gtk_widget_set_vexpand(gui.formwin, TRUE);
    gtk_widget_set_hexpand(gui.formwin, TRUE);
    gtk_box_append(GTK_BOX(vbox), gui.formwin);

    // The drawing area for the editor content.
    gui.drawarea = gtk_drawing_area_new();
    gui.surface = NULL;
    gtk_widget_set_focusable(gui.drawarea, TRUE);
    gui_gtk_form_put(GTK_FORM(gui.formwin), gui.drawarea, 0, 0);

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
	// TODO: parse geometry
	VIM_CLEAR(gui.geom);
    }

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

    gtk_widget_show(gui.mainwin);

    // Make sure the drawing area gets keyboard focus.
    gtk_widget_grab_focus(gui.drawarea);

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
    // TODO
}

    void
gui_mch_settitle(char_u *title, char_u *icon UNUSED)
{
    if (title != NULL && gui.mainwin != NULL)
	gtk_window_set_title(GTK_WINDOW(gui.mainwin), (const char *)title);
}

    void
gui_mch_set_shellsize(int width, int height,
	int min_width UNUSED, int min_height UNUSED,
	int base_width UNUSED, int base_height UNUSED,
	int direction UNUSED)
{
    // Add menu/toolbar size
    width += get_menu_tool_width();
    height += get_menu_tool_height();

    gtk_window_set_default_size(GTK_WINDOW(gui.mainwin), width, height);
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
    // Fill background with Vim's background color first
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

    void
gui_mch_delete_lines(int row, int num_lines)
{
    // TODO: implement
}

    void
gui_mch_insert_lines(int row, int num_lines)
{
    // TODO: implement
}

    void
gui_mch_draw_hollow_cursor(guicolor_T color)
{
    // TODO
}

    void
gui_mch_draw_part_cursor(int w, int h, guicolor_T color)
{
    // TODO
}

    void
gui_mch_flash(int msec UNUSED)
{
    // TODO
}

    void
gui_mch_invert_rectangle(int r, int c, int nr, int nc)
{
    // TODO
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

    static void
button_press_event(GtkGestureClick *gesture UNUSED, int n_press UNUSED,
	double x UNUSED, double y UNUSED, gpointer data UNUSED)
{
    // TODO
}

    static void
button_release_event(GtkGestureClick *gesture UNUSED, int n_press UNUSED,
	double x UNUSED, double y UNUSED, gpointer data UNUSED)
{
    // TODO
}

    static void
motion_notify_event(GtkEventControllerMotion *controller UNUSED,
	double x UNUSED, double y UNUSED, gpointer data UNUSED)
{
    // TODO
}

    static void
enter_notify_event(GtkEventControllerMotion *controller UNUSED,
	double x UNUSED, double y UNUSED, gpointer data UNUSED)
{
    // TODO
}

    static void
leave_notify_event(GtkEventControllerMotion *controller UNUSED,
	gpointer data UNUSED)
{
    // TODO
}

    static gboolean
scroll_event(GtkEventControllerScroll *controller UNUSED,
	double dx UNUSED, double dy UNUSED, gpointer data UNUSED)
{
    // TODO
    return FALSE;
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
drawarea_realize_cb(GtkWidget *widget, gpointer data UNUSED)
{
    int w = gtk_widget_get_width(widget);
    int h = gtk_widget_get_height(widget);

    if (w <= 0) w = 800;
    if (h <= 0) h = 600;

    if (gui.surface != NULL)
	cairo_surface_destroy(gui.surface);
    gui.surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);

    gui_mch_new_colors();
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
    // TODO
    *x = 0;
    *y = 0;
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
	    gtk_widget_set_cursor(gui.drawarea, NULL);
    }
}

    int
gui_mch_haskey(char_u *name)
{
    // TODO
    return OK;
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
    // TODO
}

    void
gui_mch_menu_hidden(vimmenu_T *menu UNUSED, int hidden UNUSED)
{
    // TODO
}

    void
gui_mch_draw_menubar(void)
{
    // TODO
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

    void
gui_mch_update_tabline(void)
{
    // TODO
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
    void
gui_mch_drawsign(int row UNUSED, int col UNUSED, int typenr UNUSED)
{
    // TODO
}

    void *
gui_mch_register_sign(char_u *signfile UNUSED)
{
    // TODO
    return NULL;
}

    void
gui_mch_destroy_sign(void *sign UNUSED)
{
    // TODO
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

	    if (item_list != NULL)
	    {
		PangoItem *item = (PangoItem *)item_list->data;

		font = item->analysis.font;
		pango_shape((const char *)s, len, &item->analysis, glyphs);

		// Force each glyph to char_width
		{
		    int i;
		    int width = gui.char_width * PANGO_SCALE;

		    for (i = 0; i < glyphs->num_glyphs; ++i)
		    {
			PangoGlyphGeometry *geom = &glyphs->glyphs[i].geometry;
			geom->x_offset += MAX(0, width - geom->width) / 2;
			geom->width = width;
		    }
		}
		column_offset = cells;

		g_list_foreach(item_list,
			(GFunc)(void *)&pango_item_free, NULL);
		g_list_free(item_list);
	    }
	    else
		font = gui.ascii_font;

	    pango_attr_list_unref(attr_list);
	}

	// Draw background
	if (!(flags & DRAW_TRANSP))
	{
	    // Already drawn above
	}

	// Draw foreground text
	cairo_set_source_rgba(cr,
		gui.fgcolor->red, gui.fgcolor->green,
		gui.fgcolor->blue, gui.fgcolor->alpha);
	cairo_move_to(cr, TEXT_X(col), TEXT_Y(row));
	if (font != NULL)
	    pango_cairo_show_glyph_string(cr, font, glyphs);

	// Emulate bold by drawing with offset
	if ((flags & DRAW_BOLD) && !gui.font_can_bold && font != NULL)
	{
	    cairo_move_to(cr, TEXT_X(col) + 1, TEXT_Y(row));
	    pango_cairo_show_glyph_string(cr, font, glyphs);
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

    void
gui_get_x11_windis(Window *win UNUSED, Display **dis UNUSED)
{
    // GTK4: not applicable
}

    void
gui_gtk_init_socket_server(void)
{
    // TODO
}

    void
gui_gtk_uninit_socket_server(void)
{
    // TODO
}

    void
gui_gtk_set_mnemonics(int enable UNUSED)
{
    // TODO
}

    void
gui_make_popup(char_u *path_name UNUSED, int mouse_pos UNUSED)
{
    // TODO
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

// Clipboard stubs - GTK4 clipboard uses GdkClipboard, not GdkAtom
    void
clip_mch_request_selection(Clipboard_T *cbd UNUSED)
{
    // TODO: implement with GdkClipboard
}

    void
clip_mch_set_selection(Clipboard_T *cbd UNUSED)
{
    // TODO
}

    int
clip_mch_own_selection(Clipboard_T *cbd UNUSED)
{
    // TODO
    return FAIL;
}

    void
clip_mch_lose_selection(Clipboard_T *cbd UNUSED)
{
    // TODO
}

// Balloon eval stubs
    void
gui_mch_post_balloon(BalloonEval *beval UNUSED, char_u *mesg UNUSED)
{
    // TODO
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

// Mouse shape stub
    void
mch_set_mouse_shape(int shape UNUSED)
{
    // TODO
}

// GTK4 does not have GtkMenuShell
    GType
GTK_MENU_SHELL(void)
{
    return G_TYPE_NONE;
}

    void
gtk_menu_shell_select_first(void *shell UNUSED, gboolean sr UNUSED)
{
    // GTK4: no menu shell
}

#endif // FEAT_GUI_GTK
