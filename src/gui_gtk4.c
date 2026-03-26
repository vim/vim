/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 *
 * GTK4 GUI: menus, scrollbars, dialogs, toolbar.
 * This is a clean implementation for GTK4, separate from gui_gtk.c.
 *
 * GTK4 differences:
 * - No GtkMenuBar/GtkMenu/GtkMenuItem (use GMenu + GtkPopoverMenuBar)
 * - No gtk_dialog_run (async)
 * - No GtkTearoffMenuItem
 * - GtkFileChooserNative instead of GtkFileChooserDialog
 */

#include "vim.h"

#ifdef FEAT_GUI_GTK

#include <gtk/gtk.h>
#include "gui_gtk4_f.h"

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
    if (sb->id == NULL || !GTK_IS_RANGE(sb->id))
	return;

    GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(sb->id));
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

    if (sb->id != NULL)
    {
	GtkAdjustment *adj = gtk_range_get_adjustment(GTK_RANGE(sb->id));

	gtk_widget_set_visible(sb->id, FALSE);
	gui_gtk_form_put(GTK_FORM(gui.formwin), sb->id, 0, 0);
	g_object_set_data(G_OBJECT(adj), "vim-sb", (gpointer)sb);
	g_signal_connect(G_OBJECT(adj), "value-changed",
		G_CALLBACK(adjustment_value_changed), NULL);
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
