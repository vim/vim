/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#ifndef __GTK_FORM_H__
#define __GTK_FORM_H__

#include <gdk/gdk.h>
#include <gtk/gtkcontainer.h>


#ifdef __cplusplus
extern "C" {
#endif

#define GTK_TYPE_FORM		       (gtk_form_get_type ())
#define GTK_FORM(obj)		       (GTK_CHECK_CAST ((obj), GTK_TYPE_FORM, GtkForm))
#define GTK_FORM_CLASS(klass)	       (GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_FORM, GtkFormClass))
#define GTK_IS_FORM(obj)	       (GTK_CHECK_TYPE ((obj), GTK_TYPE_FORM))
#define GTK_IS_FORM_CLASS(klass)       (GTK_CHECK_CLASS_TYPE ((klass), GTK_TYPE_FORM))


typedef struct _GtkForm GtkForm;
typedef struct _GtkFormClass GtkFormClass;

struct _GtkForm
{
    GtkContainer container;

    GList *children;

    guint width;
    guint height;

    GdkWindow *bin_window;

    GdkVisibilityState visibility;
    gulong configure_serial;

    gint freeze_count;
};

struct _GtkFormClass
{
    GtkContainerClass parent_class;
};

GtkType gtk_form_get_type(void);

GtkWidget *gtk_form_new(void);

void gtk_form_put(GtkForm * form, GtkWidget * widget,
	gint x, gint y);

void gtk_form_move(GtkForm *form, GtkWidget * widget,
	gint x, gint y);

void gtk_form_move_resize(GtkForm * form, GtkWidget * widget,
	gint x, gint y,
	gint w, gint h);

/* These disable and enable moving and repainting respectively.  If you
 * want to update the layout's offsets but do not want it to repaint
 * itself, you should use these functions.
 */

void gtk_form_freeze(GtkForm *form);
void gtk_form_thaw(GtkForm *form);


#ifdef __cplusplus
}
#endif
#endif	/* __GTK_FORM_H__ */
