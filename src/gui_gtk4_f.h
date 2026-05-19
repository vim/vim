/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 *
 * GTK4 GtkForm widget - a simple container for absolute positioning.
 */

#ifndef GUI_GTK4_FORM_H
#define GUI_GTK4_FORM_H

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GTK_TYPE_FORM		(gui_gtk_form_get_type())
#define GTK_FORM(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_FORM, GtkForm))
#define GTK_FORM_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), GTK_TYPE_FORM, GtkFormClass))
#define GTK_IS_FORM(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_FORM))

typedef struct _GtkForm GtkForm;
typedef struct _GtkFormClass GtkFormClass;

struct _GtkForm
{
    GtkWidget widget;
    GList *children;
    gint freeze_count;
};

struct _GtkFormClass
{
    GtkWidgetClass parent_class;
};

GType gui_gtk_form_get_type(void);

GtkWidget *gui_gtk_form_new(void);

void gui_gtk_form_put(GtkForm *form, GtkWidget *widget, gint x, gint y);

void gui_gtk_form_move(GtkForm *form, GtkWidget *widget, gint x, gint y);

void gui_gtk_form_move_resize(GtkForm *form, GtkWidget *widget,
	gint x, gint y, gint w, gint h);

void gui_gtk_form_remove(GtkForm *form, GtkWidget *widget);

void gui_gtk_form_freeze(GtkForm *form);
void gui_gtk_form_thaw(GtkForm *form);

#ifdef __cplusplus
}
#endif
#endif // GUI_GTK4_FORM_H
