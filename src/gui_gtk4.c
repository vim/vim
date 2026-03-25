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

/*
 * ============================================================
 * Menu functions
 * ============================================================
 * TODO: Implement using GMenu + GtkPopoverMenuBar
 */

    void
gui_mch_add_menu(vimmenu_T *menu UNUSED, int idx UNUSED)
{
    // TODO: GMenu-based menus
}

    void
gui_mch_add_menu_item(vimmenu_T *menu UNUSED, int idx UNUSED)
{
    // TODO
}

    void
gui_mch_toggle_tearoffs(int enable UNUSED)
{
    // GTK4: tearoff menus don't exist
}

    void
gui_mch_menu_set_tip(vimmenu_T *menu UNUSED)
{
    // TODO
}

    void
gui_mch_destroy_menu(vimmenu_T *menu UNUSED)
{
    // TODO
}

    void
gui_mch_show_popupmenu(vimmenu_T *menu UNUSED)
{
    // TODO: use GtkPopoverMenu
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
    return 0;
}

    int
gui_mch_get_scrollbar_ypadding(void)
{
    return 0;
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
    gui_gtk_form_move_resize(GTK_FORM(gui.formwin), gui.drawarea, x, y, w, h);
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

    void
gui_mch_find_dialog(exarg_T *eap UNUSED)
{
    // TODO: implement Find dialog
}

    void
gui_mch_replace_dialog(exarg_T *eap UNUSED)
{
    // TODO: implement Replace dialog
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
