/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 *
 * GTK4 GtkForm widget - a simple container for absolute child positioning.
 * This is a clean rewrite of gui_gtk_f.c for GTK4.
 *
 * In GTK4, widgets no longer have their own GdkWindows (now GdkSurface),
 * GtkContainer is removed, and child positioning uses GskTransform via
 * gtk_widget_allocate().  This makes the form widget much simpler.
 */

#include "vim.h"
#include <gtk/gtk.h>
#include "gui_gtk4_f.h"

typedef struct _GtkFormChild GtkFormChild;

struct _GtkFormChild
{
    GtkWidget *widget;
    gint x;
    gint y;
};

// Forward declarations
static void gui_gtk_form_class_init(GtkFormClass *klass);
static void gui_gtk_form_init(GtkForm *form);
static void form_measure(GtkWidget *widget, GtkOrientation orientation,
	int for_size, int *minimum, int *natural,
	int *minimum_baseline, int *natural_baseline);
static void form_size_allocate(GtkWidget *widget, int width, int height,
	int baseline);
static void form_snapshot(GtkWidget *widget, GtkSnapshot *snapshot);
static void form_dispose(GObject *object);
static void form_position_child(GtkForm *form, GtkFormChild *child,
	gboolean force_allocate);

G_DEFINE_TYPE(GtkForm, gui_gtk_form, GTK_TYPE_WIDGET)

// Public interface

    GtkWidget *
gui_gtk_form_new(void)
{
    return GTK_WIDGET(g_object_new(GTK_TYPE_FORM, NULL));
}

    void
gui_gtk_form_put(
	GtkForm	    *form,
	GtkWidget   *child_widget,
	gint	    x,
	gint	    y)
{
    GtkFormChild *child;

    g_return_if_fail(GTK_IS_FORM(form));

    child = g_new(GtkFormChild, 1);
    if (child == NULL)
	return;

    child->widget = child_widget;
    child->x = x;
    child->y = y;

    gtk_widget_set_size_request(child->widget, -1, -1);

    form->children = g_list_append(form->children, child);

    gtk_widget_set_parent(child_widget, GTK_WIDGET(form));
    form_position_child(form, child, TRUE);
}

    void
gui_gtk_form_move(
	GtkForm	    *form,
	GtkWidget   *child_widget,
	gint	    x,
	gint	    y)
{
    GList *tmp_list;

    g_return_if_fail(GTK_IS_FORM(form));

    for (tmp_list = form->children; tmp_list; tmp_list = tmp_list->next)
    {
	GtkFormChild *child = tmp_list->data;
	if (child->widget == child_widget)
	{
	    child->x = x;
	    child->y = y;
	    form_position_child(form, child, TRUE);
	    return;
	}
    }
}

    void
gui_gtk_form_move_resize(
	GtkForm	    *form,
	GtkWidget   *widget,
	gint	    x,
	gint	    y,
	gint	    w,
	gint	    h)
{
    gtk_widget_set_size_request(widget, w, h);
    gui_gtk_form_move(form, widget, x, y);
}

    void
gui_gtk_form_remove(GtkForm *form, GtkWidget *child_widget)
{
    GList *tmp_list;

    g_return_if_fail(GTK_IS_FORM(form));

    for (tmp_list = form->children; tmp_list; tmp_list = tmp_list->next)
    {
	GtkFormChild *child = tmp_list->data;
	if (child->widget == child_widget)
	{
	    form->children = g_list_remove_link(form->children, tmp_list);
	    g_list_free_1(tmp_list);
	    gtk_widget_unparent(child_widget);
	    g_free(child);
	    return;
	}
    }
}

    void
gui_gtk_form_freeze(GtkForm *form)
{
    g_return_if_fail(GTK_IS_FORM(form));
    ++form->freeze_count;
}

    void
gui_gtk_form_thaw(GtkForm *form)
{
    g_return_if_fail(GTK_IS_FORM(form));

    if (!form->freeze_count)
	return;

    if (!(--form->freeze_count))
    {
	GList *tmp_list;

	for (tmp_list = form->children; tmp_list; tmp_list = tmp_list->next)
	    form_position_child(form, tmp_list->data, FALSE);
	gtk_widget_queue_draw(GTK_WIDGET(form));
    }
}

// GObject/GtkWidget class implementation

    static void
gui_gtk_form_class_init(GtkFormClass *klass)
{
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);

    gobject_class->dispose = form_dispose;

    widget_class->measure = form_measure;
    widget_class->size_allocate = form_size_allocate;
    widget_class->snapshot = form_snapshot;
}

    static void
gui_gtk_form_init(GtkForm *form)
{
    form->children = NULL;
    form->freeze_count = 0;
}

    static void
form_measure(
	GtkWidget	*widget UNUSED,
	GtkOrientation	orientation UNUSED,
	int		for_size UNUSED,
	int		*minimum,
	int		*natural,
	int		*minimum_baseline,
	int		*natural_baseline)
{
    *minimum = 1;
    *natural = 1;
    *minimum_baseline = -1;
    *natural_baseline = -1;
}

static guint form_resize_idle_id = 0;
static int form_last_width = 0;
static int form_last_height = 0;

    static gboolean
form_resize_idle_cb(gpointer data UNUSED)
{
    int w, h;

    form_resize_idle_id = 0;

    // Use drawarea's actual allocation, not formwin's
    if (gui.drawarea == NULL)
	return FALSE;
    w = gtk_widget_get_width(gui.drawarea);
    h = gtk_widget_get_height(gui.drawarea);

    if (w > 1 && h > 1)
	gui_resize_shell(w, h);

    return FALSE;
}

    static void
form_size_allocate(GtkWidget *widget, int width, int height,
	int baseline UNUSED)
{
    GtkForm *form;
    GList *tmp_list;

    g_return_if_fail(GTK_IS_FORM(widget));

    form = GTK_FORM(widget);

    for (tmp_list = form->children; tmp_list; tmp_list = tmp_list->next)
	form_position_child(form, tmp_list->data, TRUE);

    // Notify Vim about size change via idle callback
    if (width != form_last_width || height != form_last_height)
    {
	form_last_width = width;
	form_last_height = height;
	if (form_resize_idle_id == 0)
	    form_resize_idle_id = g_idle_add(form_resize_idle_cb, NULL);
    }
}

    static void
form_snapshot(GtkWidget *widget, GtkSnapshot *snapshot)
{
    GtkForm *form;
    GList *tmp_list;

    g_return_if_fail(GTK_IS_FORM(widget));

    form = GTK_FORM(widget);

    for (tmp_list = form->children; tmp_list; tmp_list = tmp_list->next)
    {
	GtkFormChild *child = tmp_list->data;
	if (child->widget != NULL
		&& GTK_IS_WIDGET(child->widget)
		&& gtk_widget_get_parent(child->widget) == widget)
	    gtk_widget_snapshot_child(widget, child->widget, snapshot);
    }
}

    static void
form_dispose(GObject *object)
{
    GtkForm *form = GTK_FORM(object);
    GList *tmp_list;

    tmp_list = form->children;
    while (tmp_list)
    {
	GtkFormChild *child = tmp_list->data;
	tmp_list = tmp_list->next;

	gtk_widget_unparent(child->widget);
	g_free(child);
    }
    g_list_free(form->children);
    form->children = NULL;

    G_OBJECT_CLASS(gui_gtk_form_parent_class)->dispose(object);
}

// Child positioning using GskTransform

    static void
form_position_child(
	GtkForm		*form UNUSED,
	GtkFormChild	*child,
	gboolean	force_allocate)
{
    if (!force_allocate)
	return;

    if (child->widget == NULL || !GTK_IS_WIDGET(child->widget))
	return;

    {
	GtkRequisition requisition;
	GskTransform *transform;
	int w, h;

	gtk_widget_get_preferred_size(child->widget, &requisition, NULL);
	w = requisition.width;
	h = requisition.height;

	// If widget has no size request (e.g. drawarea), use parent size
	if (w <= 0)
	    w = gtk_widget_get_width(GTK_WIDGET(form));
	if (h <= 0)
	    h = gtk_widget_get_height(GTK_WIDGET(form));
	if (w <= 0) w = 1;
	if (h <= 0) h = 1;

	transform = gsk_transform_translate(NULL,
		&GRAPHENE_POINT_INIT((float)child->x, (float)child->y));
	gtk_widget_allocate(child->widget, w, h, -1, transform);
    }
}
