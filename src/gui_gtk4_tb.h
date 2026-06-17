/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#ifndef GUI_GTK4_TB_H
#define GUI_GTK4_TB_H

#include "vim.h"

#ifdef FEAT_TOOLBAR

# include <gtk/gtk.h>

# define VIM_TYPE_TOOLBAR (vim_toolbar_get_type())
G_DECLARE_FINAL_TYPE(VimToolbar, vim_toolbar, VIM, TOOLBAR, GtkWidget)

typedef struct _VimToolbarItem VimToolbarItem;

GtkWidget *vim_toolbar_new(void);
GtkWidget *vim_toolbar_insert_button(VimToolbar *self, GtkWidget *icon, const char *text, int idx);
GtkWidget *vim_toolbar_insert_separator(VimToolbar *self, int idx);
void vim_toolbar_set_style(VimToolbar *self, int style, int iconsize);

#endif

#endif
