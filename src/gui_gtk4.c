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

    void
gui_mch_create_scrollbar(scrollbar_T *sb, int orient)
{
    if (orient == SBAR_HORIZ)
	sb->id = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, NULL);
    else
	sb->id = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, NULL);

    if (sb->id != NULL)
    {
	gtk_widget_set_visible(sb->id, FALSE);
	gui_gtk_form_put(GTK_FORM(gui.formwin), sb->id, 0, 0);
	// TODO: connect value-changed signal
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

    char_u *
gui_mch_browse(int saving UNUSED,
	char_u *title UNUSED,
	char_u *dflt UNUSED,
	char_u *ext UNUSED,
	char_u *initdir UNUSED,
	char_u *filter UNUSED)
{
    // TODO: implement with GtkFileDialog (GTK 4.10+) or GtkFileChooserNative
    return NULL;
}

    char_u *
gui_mch_browsedir(char_u *title UNUSED, char_u *initdir UNUSED)
{
    // TODO
    return NULL;
}

/*
 * ============================================================
 * Message dialog
 * ============================================================
 */

    int
gui_mch_dialog(
	int	type UNUSED,
	char_u	*title UNUSED,
	char_u	*message UNUSED,
	char_u	*buttons UNUSED,
	int	dfltbutton UNUSED,
	char_u	*textfield UNUSED,
	int	ex_cmd UNUSED)
{
    // TODO: implement with GtkAlertDialog (GTK 4.10+) or async GtkDialog
    return 1;
}

/*
 * ============================================================
 * Find/Replace dialogs
 * ============================================================
 */

    void
gui_mch_find_dialog(exarg_T *eap UNUSED)
{
    // TODO
}

    void
gui_mch_replace_dialog(exarg_T *eap UNUSED)
{
    // TODO
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
