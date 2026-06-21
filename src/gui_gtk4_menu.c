/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#ifdef FEAT_MENU

#include <gtk/gtk.h>
#include "gui_gtk4_menu.h"

/*
 * Similar as GtkButton but set CSS name to "item" to emulate GtkPopoverMenuBar
 * styling. Always has a submenu.
 */
struct _VimMenuBarItem
{
    GtkButton parent;

    GtkWidget *menu;
};

G_DEFINE_TYPE(VimMenuBarItem, vim_menu_bar_item, GTK_TYPE_BUTTON)

    static void
vim_menu_bar_item_dispose(GObject *object)
{
    VimMenuBarItem *self = VIM_MENU_BAR_ITEM(object);

    g_clear_pointer((GtkWidget **)&self->menu, gtk_widget_unparent);

    G_OBJECT_CLASS(vim_menu_bar_item_parent_class)->dispose(object);
}

    static void
vim_menu_bar_item_class_init(VimMenuBarItemClass *class)
{
    GtkWidgetClass  *widget_class = GTK_WIDGET_CLASS(class);
    GObjectClass    *obj_class = G_OBJECT_CLASS(class);

    obj_class->dispose = vim_menu_bar_item_dispose;

    gtk_widget_class_set_css_name(widget_class, "item");
}

    static void
vim_menu_bar_item_init(VimMenuBarItem *self)
{
    // Enable mnemonics
    gtk_button_set_use_underline(GTK_BUTTON(self), TRUE);
}

    GtkWidget *
vim_menu_bar_item_new(const char *text, VimMenu *menu)
{
    VimMenuBarItem *item = g_object_new(VIM_TYPE_MENU_BAR_ITEM, NULL);

    gtk_button_set_label(GTK_BUTTON(item), text);

    assert(item->menu == NULL);
    item->menu = GTK_WIDGET(menu);
    gtk_popover_set_position(GTK_POPOVER(menu), GTK_POS_BOTTOM);
    gtk_widget_set_parent(GTK_WIDGET(menu), GTK_WIDGET(item));

    return GTK_WIDGET(item);
}

/*
 * Similar to GtkPopoverMenuBar
 */
struct _VimMenuBar
{
    GtkWidget parent;

    GList *items;

    // Currently visible item that has submenu popped up, else NULL
    GtkWidget *active_item;
};

G_DEFINE_TYPE(VimMenuBar, vim_menu_bar, GTK_TYPE_WIDGET)

    static void
vim_menu_bar_dispose(GObject *object)
{
    VimMenuBar *self = VIM_MENU_BAR(object);

    g_clear_list(&self->items, (GDestroyNotify)gtk_widget_unparent);

    G_OBJECT_CLASS(vim_menu_bar_parent_class)->dispose(object);
}

    static void
vim_menu_bar_class_init(VimMenuBarClass *class)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);
    GObjectClass *obj_class = G_OBJECT_CLASS(class);

    obj_class->dispose = vim_menu_bar_dispose;

    gtk_widget_class_set_layout_manager_type(widget_class, GTK_TYPE_BOX_LAYOUT);
    gtk_widget_class_set_css_name(widget_class, "menubar");
}

    static void
vim_menu_bar_init(VimMenuBar *self)
{
    GtkLayoutManager *lm = gtk_widget_get_layout_manager(GTK_WIDGET(self));

    gtk_orientable_set_orientation(GTK_ORIENTABLE(lm),
	    GTK_ORIENTATION_HORIZONTAL);
    gtk_box_layout_set_spacing(GTK_BOX_LAYOUT(lm), 0);
}

    GtkWidget *
vim_menu_bar_new(void)
{
    return g_object_new(VIM_TYPE_MENU_BAR, NULL);
}

/*
 * Set the currently active menu of the menubar to "item". If NULL, then close
 * any submenus.
 */
    static void
vim_menu_bar_set_active_item(
	VimMenuBar	*self,
	VimMenuBarItem	*item,
	gboolean	force)
{
    // Do nothing if currently active item is "item", or if there is not
    // currently active item. User must click a menu item first for menus to
    // automatically appear on hover. This is unless "force" is TRUE.
    if ((!force && self->active_item == NULL)
	    || self->active_item == GTK_WIDGET(item))
	return;

    if (self->active_item != NULL)
	gtk_popover_popdown(GTK_POPOVER(
		    VIM_MENU_BAR_ITEM(self->active_item)->menu)
		);

    self->active_item = GTK_WIDGET(item);
    if (item != NULL)
	gtk_popover_popup(GTK_POPOVER(item->menu));
}

    static void
vim_menu_bar_item_enter_cb(
	GtkEventController  *controller,
	double		    x UNUSED,
	double		    y UNUSED,
	VimMenuBar	    *menubar)
{
    VimMenuBarItem  *self;
    GtkWidget	    *cur;

    self = VIM_MENU_BAR_ITEM(gtk_event_controller_get_widget(controller));
    cur = menubar->active_item;

    vim_menu_bar_set_active_item(menubar, self, FALSE);

    // Only make item selected if there is no active item (no submenu open), or
    // if the active item is the item.
    if (menubar->active_item == NULL
	    || menubar->active_item == GTK_WIDGET(self))
	gtk_widget_set_state_flags(GTK_WIDGET(self),
		GTK_STATE_FLAG_SELECTED, FALSE);

    // Deselect previous item if the active item changeed.
    if (cur != NULL && cur != GTK_WIDGET(self))
	gtk_widget_unset_state_flags(cur, GTK_STATE_FLAG_SELECTED);
}

    static void
vim_menu_bar_item_leave_cb(
	GtkEventController  *controller UNUSED,
	VimMenuBar	    *menubar)
{
    VimMenuBarItem *self;

    self = VIM_MENU_BAR_ITEM(gtk_event_controller_get_widget(controller));

    // If the item is the currently active item, then don't deselect it.
    if (menubar->active_item != GTK_WIDGET(self))
	gtk_widget_unset_state_flags(GTK_WIDGET(self),
		GTK_STATE_FLAG_SELECTED);
}

    static void
vim_menu_bar_item_clicked_cb(VimMenuBarItem *self, VimMenuBar *menubar)
{
    vim_menu_bar_set_active_item(menubar, self, TRUE);
}

    static void
vim_menu_bar_item_menu_closed_cb(VimMenu *menu UNUSED, VimMenuBar *menubar)
{
    gtk_widget_unset_state_flags(GTK_WIDGET(menubar->active_item),
	    GTK_STATE_FLAG_SELECTED);
    vim_menu_bar_set_active_item(menubar, NULL, FALSE);
}

/*
 * Insert the menu item at the given index in the menu bar.
 */
    void
vim_menu_bar_insert_item(VimMenuBar *self, VimMenuBarItem *item, int idx)
{
    GtkEventController	*controller;
    GList		*next_sibling;

    next_sibling = g_list_nth(self->items, idx);
    gtk_widget_insert_before(GTK_WIDGET(item), GTK_WIDGET(self),
	    next_sibling == NULL ? NULL : next_sibling->data);

    self->items = g_list_insert(self->items, item, idx);

    controller = gtk_event_controller_motion_new();
    g_signal_connect_object(controller, "enter",
	    G_CALLBACK(vim_menu_bar_item_enter_cb), self, G_CONNECT_DEFAULT);
    g_signal_connect_object(controller, "leave",
	    G_CALLBACK(vim_menu_bar_item_leave_cb), self, G_CONNECT_DEFAULT);
    gtk_widget_add_controller(GTK_WIDGET(item), controller);

    g_signal_connect_object(item, "clicked",
	    G_CALLBACK(vim_menu_bar_item_clicked_cb), self, G_CONNECT_DEFAULT);

    g_signal_connect_object(item->menu, "closed",
	    G_CALLBACK(vim_menu_bar_item_menu_closed_cb),
	    self, G_CONNECT_DEFAULT);
}

/*
 * Remove the menu item or separator from the menu bar
 */
    void
vim_menu_bar_remove(VimMenuBar *self, GtkWidget *item)
{
    self->items = g_list_remove(self->items, item);
    gtk_widget_unparent(item);
}

/*
 * Menu button that can be used to perform actions, or if there is a submenu,
 * toggle the state of the submenu popover. CSS name is "modelbutton" to make it
 * styled like GtkPopoverMenu
 */
struct _VimMenuItem
{
    GtkButton parent;

    GtkWidget *label;	    // Displays text for button.
    GtkWidget *aux_widget;  // Either an icon or a label showing the accelerator
			    // text.

    GtkWidget *submenu;	    // Submenu popover if any (VimMenu)
};

G_DEFINE_TYPE(VimMenuItem, vim_menu_item, GTK_TYPE_BUTTON)

    static void
vim_menu_item_dispose(GObject *object)
{
    VimMenuItem *self = VIM_MENU_ITEM(object);

    g_clear_pointer(&self->label, gtk_widget_unparent);
    g_clear_pointer(&self->aux_widget, gtk_widget_unparent);
    g_clear_pointer((GtkWidget **)&self->submenu, gtk_widget_unparent);

    G_OBJECT_CLASS(vim_menu_item_parent_class)->dispose(object);
}

    static void
vim_menu_item_class_init(VimMenuItemClass *class)
{
    GtkWidgetClass  *widget_class = GTK_WIDGET_CLASS(class);
    GObjectClass    *obj_class = G_OBJECT_CLASS(class);

    obj_class->dispose = vim_menu_item_dispose;

    gtk_widget_class_set_layout_manager_type(widget_class, GTK_TYPE_BOX_LAYOUT);
    gtk_widget_class_set_css_name(widget_class, "modelbutton");
}

    static void
vim_menu_item_init(VimMenuItem *self)
{
    GtkLayoutManager *lm = gtk_widget_get_layout_manager(GTK_WIDGET(self));

    gtk_orientable_set_orientation(GTK_ORIENTABLE(lm),
	    GTK_ORIENTATION_HORIZONTAL);
    gtk_box_layout_set_spacing(GTK_BOX_LAYOUT(lm), 0);
}

/*
 * Create a new menu item with the given text to display.
 */
    GtkWidget *
vim_menu_item_new(const char *text)
{
    VimMenuItem *item = g_object_new(VIM_TYPE_MENU_ITEM, NULL);

    item->label = gtk_label_new_with_mnemonic(text);

    // Make sure label is on the right and pushes everything to the left
    gtk_widget_set_halign(item->label, GTK_ALIGN_START);
    gtk_widget_set_hexpand(item->label, TRUE);
    gtk_widget_set_parent(item->label, GTK_WIDGET(item));

    return GTK_WIDGET(item);
}

/*
 * Update displayed text for menu item
 */
    void
vim_menu_item_set_text(VimMenuItem *self, const char *text)
{
    gtk_label_set_text_with_mnemonic(GTK_LABEL(self->label), text);
}

    static void
vim_menu_item_set_aux_widget(VimMenuItem *self, GtkWidget *aux)
{
    self->aux_widget = aux;
    gtk_widget_set_halign(self->aux_widget, GTK_ALIGN_END);
    gtk_widget_set_hexpand(self->aux_widget, FALSE);
    gtk_widget_set_margin_start(self->aux_widget, 50);
}

/*
 * Set the accelerator text for the menu item.
 */
    void
vim_menu_item_set_accel(VimMenuItem *self, const char *accel_text)
{
    assert(self->aux_widget == NULL);

    vim_menu_item_set_aux_widget(self, gtk_label_new(accel_text));
    gtk_widget_insert_after(self->aux_widget, GTK_WIDGET(self), self->label);
}

/*
 * Set the submenu popover for the menu item
 */
    void
vim_menu_item_set_submenu(VimMenuItem *self, VimMenu *submenu)
{
    GtkWidget *icon;

    assert(self->submenu == NULL);
    assert(self->aux_widget == NULL);

    // Add arrow icon pointing to right
    icon = gtk_image_new_from_icon_name("pan-end-symbolic");
    // Only make the icon sensitive when the submenu popover is open
    gtk_widget_set_sensitive(icon, FALSE);
    vim_menu_item_set_aux_widget(self, icon);
    gtk_widget_insert_after(self->aux_widget, GTK_WIDGET(self), self->label);

    gtk_popover_set_position(GTK_POPOVER(submenu), GTK_POS_RIGHT);
    // Make top of popover be aligned with button.
    gtk_widget_set_valign(GTK_WIDGET(submenu), GTK_ALIGN_START);

    self->submenu = GTK_WIDGET(submenu);
    gtk_widget_set_parent(GTK_WIDGET(submenu), GTK_WIDGET(self));
}

/*
 * Similar to GtkPopoverMenu, except uses GtkWidgets directly like GTK3, instead
 * of abstracting it into GMenuModel.
 */
struct _VimMenu
{
    GtkPopover parent;

    GtkWidget *box;
    GtkWidget *scr;

    GList *items;

    // Currently active item showing submenu popover, or being hovered on, or
    // NULL.
    GtkWidget *active_item;
};

G_DEFINE_TYPE(VimMenu, vim_menu, GTK_TYPE_POPOVER)

    static void
vim_menu_dispose(GObject *object)
{
    VimMenu *self = VIM_MENU(object);

    g_clear_list(&self->items, (GDestroyNotify)gtk_widget_unparent);
    g_clear_pointer(&self->scr, gtk_widget_unparent);

    G_OBJECT_CLASS(vim_menu_parent_class)->dispose(object);
}

    static void
vim_menu_class_init(VimMenuClass *class)
{
    GObjectClass *obj_class = G_OBJECT_CLASS(class);

    obj_class->dispose = vim_menu_dispose;
}

/*
 * Set the active item of the menu to "item". If "item" is NULL, then close any
 * submenus.
 */
    static void
vim_menu_set_active_item(VimMenu *self, VimMenuItem *item)
{
    if (self->active_item == GTK_WIDGET(item))
	return;

    if (self->active_item != NULL
	    && VIM_MENU_ITEM(self->active_item)->submenu != NULL)
    {
	gtk_popover_popdown(GTK_POPOVER(
		    VIM_MENU_ITEM(self->active_item)->submenu
		    ));
	gtk_widget_unset_state_flags(GTK_WIDGET(self->active_item),
		GTK_STATE_FLAG_SELECTED);
    }

    self->active_item = GTK_WIDGET(item);
    if (item != NULL && item->submenu != NULL)
    {
	gtk_popover_popup(GTK_POPOVER(item->submenu));
	gtk_widget_set_state_flags(GTK_WIDGET(item),
		GTK_STATE_FLAG_SELECTED, FALSE);
    }
}

    static void
vim_menu_closed_cb(VimMenu *self, void *udata UNUSED)
{
    vim_menu_set_active_item(self, NULL);
}

    static void
vim_menu_init(VimMenu *self)
{
    gtk_popover_set_has_arrow(GTK_POPOVER(self), FALSE);

    // Do not make child popovers close parent popovers when they are closed.
    gtk_popover_set_cascade_popdown(GTK_POPOVER(self), FALSE);

    self->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(self->box, TRUE);
    gtk_widget_set_vexpand(self->box, TRUE);

    self->scr = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(self->scr),
	    GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_propagate_natural_width(
	    GTK_SCROLLED_WINDOW(self->scr), TRUE);
    gtk_scrolled_window_set_propagate_natural_height(
	    GTK_SCROLLED_WINDOW(self->scr), TRUE);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(self->scr),
	    GTK_WIDGET(self->box));
    gtk_popover_set_child(GTK_POPOVER(self), self->scr);

    gtk_widget_add_css_class(GTK_WIDGET(self), "menu");

    g_signal_connect(self, "closed", G_CALLBACK(vim_menu_closed_cb), NULL);
}

    GtkWidget *
vim_menu_new(void)
{
    return g_object_new(VIM_TYPE_MENU, NULL);
}

    static GtkWidget *
vim_menu_insert(VimMenu *self, GtkWidget *item, int idx)
{
    if (idx > 0)
    {
	GList *prev = g_list_nth(self->items, idx - 1);
	if (prev == NULL)
	    gtk_box_append(GTK_BOX(self->box), item);
	else
	    gtk_box_insert_child_after(GTK_BOX(self->box), item,
		    prev->data);
    }
    else
	gtk_box_prepend(GTK_BOX(self->box), item);

    self->items = g_list_insert(self->items, item, idx);
    return item;
}

    static void
vim_menu_item_clicked_cb(VimMenuItem *self, VimMenu *menu)
{
    GtkWidget *menubar;

    // Only close all menus if item is a regular button (no submenu). If item
    // has a submenu, then just toggle it on and off.
    if (self->submenu != NULL)
    {
	if (gtk_widget_is_visible(self->submenu))
	    gtk_popover_popdown(GTK_POPOVER(self->submenu));
	else
	    gtk_popover_popup(GTK_POPOVER(self->submenu));
	return;
    }

    // Since we set the "cascade-popdown" property to FALSE, we must popdown the
    // toplevel menu/popover.
    gtk_popover_popdown(GTK_POPOVER(menu));
    menubar = gtk_widget_get_ancestor(GTK_WIDGET(menu), VIM_TYPE_MENU_BAR);
    vim_menu_bar_set_active_item(VIM_MENU_BAR(menubar), NULL, TRUE);
}

    static void
vim_menu_item_enter_cb(
	GtkEventController  *controller,
	double		    x UNUSED,
	double		    y UNUSED,
	VimMenu		    *menu)
{
    VimMenuItem  *self;

    if (!gtk_event_controller_motion_contains_pointer(
		GTK_EVENT_CONTROLLER_MOTION(controller)))
	    return;

    self = VIM_MENU_ITEM(gtk_event_controller_get_widget(controller));
    vim_menu_set_active_item(menu, self);
}

    static void
vim_menu_item_leave_cb(GtkEventController *controller UNUSED, VimMenu *menu)
{
    VimMenuItem  *self;

    if (gtk_event_controller_motion_contains_pointer(
		GTK_EVENT_CONTROLLER_MOTION(controller)))
	    return;

    self = VIM_MENU_ITEM(gtk_event_controller_get_widget(controller));
    if (menu->active_item == GTK_WIDGET(self))
	vim_menu_set_active_item(menu, NULL);
}

    static void
vim_menu_item_submenu_key_pressed_cb(
	GtkEventController  *controller,
	guint		    keyval,
	guint		    keycode,
	GdkModifierType	    state,
	VimMenuItem	    *item)
{
    VimMenu	*menu;
    GtkWidget	*widget;

    menu = VIM_MENU(gtk_event_controller_get_widget(controller));

    switch (keyval)
    {
	case GDK_KEY_Down:
	case GDK_KEY_Tab:
	    if (state & GDK_SHIFT_MASK)
	    {
		// Go to the previous item if any
		widget = gtk_widget_get_prev_sibling(GTK_WIDGET(item));
		vim_menu_set_active_item(menu, VIM_MENU_ITEM(widget));
	    }
	    else
	    {
	    }
	    break;
	case GDK_KEY_Up:
	    // Same as Shift-Tab
	    break;
	default:
	    break;
    }
}

/*
 * Insert the menu item at the given index in the menu.
 */
    void
vim_menu_insert_item(VimMenu *self, VimMenuItem *item, int idx)
{
    GtkEventController *controller;

    vim_menu_insert(self, GTK_WIDGET(item), idx);

    controller = gtk_event_controller_motion_new();
    g_signal_connect_object(controller, "enter",
	    G_CALLBACK(vim_menu_item_enter_cb), self, G_CONNECT_DEFAULT);
    g_signal_connect_object(controller, "leave",
	    G_CALLBACK(vim_menu_item_leave_cb), self, G_CONNECT_DEFAULT);
    gtk_widget_add_controller(GTK_WIDGET(item), controller);

    // Add key controller for basic movement
    controller = gtk_event_controller_key_new();
    g_signal_connect_object(controller, "key-pressed",
	    G_CALLBACK(vim_menu_item_submenu_key_pressed_cb),
	    item, G_CONNECT_DEFAULT);
    gtk_widget_add_controller(GTK_WIDGET(self), controller);

    g_signal_connect_object(item, "clicked",
	    G_CALLBACK(vim_menu_item_clicked_cb),
	    self, G_CONNECT_DEFAULT);
}

/*
 * Insert a separator at the given position and return it.
 */
    GtkWidget *
vim_menu_insert_separator(VimMenu *self, int idx)
{
    return vim_menu_insert(self,
	    gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), idx);
}

/*
 * Remove the menu item or separator from the menu
 */
    void
vim_menu_remove(VimMenu *self, GtkWidget *item)
{
    self->items = g_list_remove(self->items, item);
    gtk_box_remove(GTK_BOX(self->box), item);
}

#endif // FEAT_MENU
