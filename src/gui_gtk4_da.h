/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#ifndef GUI_GTK4_DRAW_AREA_H
#define GUI_GTK4_DRAW_AREA_H

#include "vim.h"

#ifdef USE_GTK4_SNAPSHOT

# include <gtk/gtk.h>

# define VIM_TYPE_DRAW_AREA (vim_draw_area_get_type())
G_DECLARE_FINAL_TYPE(VimDrawArea, vim_draw_area, VIM, DRAW_AREA, GtkWidget)

GtkWidget *vim_draw_area_new(void);
void vim_draw_area_set_size(VimDrawArea *self, int rows, int cols);
int vim_draw_area_draw_string(VimDrawArea *self, int row, int col, char_u *s, int len, int flags);
void vim_draw_area_clear_block(VimDrawArea *self, int row1, int col1, int row2, int col2);
void vim_draw_area_clear(VimDrawArea *self);
void vim_draw_area_move_block(VimDrawArea *self, int to, int row1, int row2, int col1, int col2);
void vim_draw_area_set_cursor(VimDrawArea *self, int w, int h);
void vim_draw_area_invert_block(VimDrawArea *self, int row, int col, int nrows, int ncols);
# if defined(FEAT_SIGN_ICONS)
void vim_draw_area_add_sign(VimDrawArea *self, GdkTexture *sign, int row, int col, int width, int height);
# endif
# ifdef FEAT_NETBEANS_INTG
void vim_draw_area_add_multisign(VimDrawArea *self, cairo_surface_t *surf, int row, int col, int width, int height);
# endif
# ifdef FEAT_IMAGE_GDK
void vim_draw_area_add_image(VimDrawArea *self, GdkTexture *image, int row, int col, int src_x, int src_y, int draw_w, int draw_h, int zindex, int id);
void vim_draw_area_remove_image(VimDrawArea *self, int id);
# endif

#endif

#endif
