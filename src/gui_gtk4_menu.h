/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#ifndef GUI_GTK4_MENU_H
#define GUI_GTK4_MENU_H

#include "vim.h"

#ifdef FEAT_MENU

# include <gtk/gtk.h>

# define VIM_TYPE_MENU_BAR_ITEM (vim_menu_bar_item_get_type())
G_DECLARE_FINAL_TYPE(VimMenuBarItem, vim_menu_bar_item, VIM, MENU_BAR_ITEM, GtkButton)

# define VIM_TYPE_MENU_BAR (vim_menu_bar_get_type())
G_DECLARE_FINAL_TYPE(VimMenuBar, vim_menu_bar, VIM, MENU_BAR, GtkWidget)

# define VIM_TYPE_MENU_ITEM (vim_menu_item_get_type())
G_DECLARE_FINAL_TYPE(VimMenuItem, vim_menu_item, VIM, MENU_ITEM, GtkButton)

# define VIM_TYPE_MENU (vim_menu_get_type())
G_DECLARE_FINAL_TYPE(VimMenu, vim_menu, VIM, MENU, GtkPopover)

typedef enum
{
    VIM_MENU_ITEM_CLICKED,
    VIM_MENU_ITEM_SELECTED
} VimMenuItemEvent;

typedef void (*VimMenuItemFunc)(VimMenuItem *item, VimMenuItemEvent event, void *udata);

GtkWidget *vim_menu_bar_item_new(const char *text, VimMenu *menu);
void vim_menu_bar_item_set_text(VimMenuBarItem *self, const char *text);

GtkWidget *vim_menu_bar_new(void);
GtkWidget *vim_menu_bar_to_menu(VimMenuBar *self);
void vim_menu_bar_insert_item(VimMenuBar *self, VimMenuBarItem *item, int idx);
void vim_menu_bar_remove(VimMenuBar *self, GtkWidget *item);
void vim_menu_bar_show(VimMenuBar *self, VimMenuBarItem *item);

GtkWidget *vim_menu_item_new(const char *text, VimMenuItemFunc func, void *udata);
void vim_menu_item_set_text(VimMenuItem *self, const char *text);
void vim_menu_item_set_accel(VimMenuItem *self, const char *accel_text);
void vim_menu_item_set_submenu(VimMenuItem *self, VimMenu *submenu);

GtkWidget *vim_menu_new(void);
void vim_menu_insert_item(VimMenu *self, VimMenuItem *item, int idx);
GtkWidget *vim_menu_insert_separator(VimMenu *self, int idx);
void vim_menu_remove(VimMenu *self, GtkWidget *item);
GtkWidget *vim_menu_copy(VimMenu *self);

#endif

#endif
