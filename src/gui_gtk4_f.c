/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"
#include <gtk/gtk.h>
#include "gui_gtk4_f.h"

/*
 * Child widget at position (x, y).
 */
typedef struct
{
    GtkWidget	*widget;
    int		x;
    int		y;
} VimFormChild;

/*
 * Similar to the GtkFixed widget, allows absolute position and sizing of child
 * widgets within. Vim already has logic for positioning and sizing UI elements,
 * so this is needed to take advantage of that. We don't use GtkFixed directly
 * since we need to override some vfuncs.
 */
struct _VimForm
{
    GtkWidget parent;

    GList *children;

    // See vim_form_size_allocate()
    guint resize_idle_id;
    int last_width;
    int last_height;
};

G_DEFINE_TYPE(VimForm, vim_form, GTK_TYPE_WIDGET)

static void vim_form_snapshot(GtkWidget *widget, GtkSnapshot *snapshot);
static void vim_form_size_allocate(GtkWidget *widget, int width, int height, int baseline);
static void vim_form_measure(GtkWidget *widget, GtkOrientation orientation, int for_size, int *minimum, int *natural, int *minimum_baseline, int *natural_baseline);
static gboolean vim_form_contains(GtkWidget *widget, double x, double y);

    static void
vim_form_dispose(GObject *obj)
{
    VimForm *self = VIM_FORM(obj);
    GList   *ele = self->children;

    while (ele != NULL)
    {
	VimFormChild *child = ele->data;

	ele = ele->next;
	gtk_widget_unparent(child->widget);
	g_free(child);
    }
    g_list_free(self->children);
    self->children = NULL;

    G_OBJECT_CLASS(vim_form_parent_class)->dispose(obj);
}

    static void
vim_form_class_init(VimFormClass *class)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);
    GObjectClass    *obj_class = G_OBJECT_CLASS(class);

    widget_class->snapshot = vim_form_snapshot;
    widget_class->size_allocate = vim_form_size_allocate;
    widget_class->measure = vim_form_measure;
    widget_class->contains = vim_form_contains;

    obj_class->dispose = vim_form_dispose;
}

    static void
vim_form_init(VimForm *self)
{

}

    GtkWidget *
vim_form_new(void)
{
    return g_object_new(VIM_TYPE_FORM, NULL);
}

/*
 * Transform the child
 */
    static void
vim_form_position_child(VimForm	*self, VimFormChild *child)
{
    GtkRequisition  requisition;
    GskTransform    *transform;
    int		    w, h;

    gtk_widget_get_preferred_size(child->widget, &requisition, NULL);
    w = requisition.width;
    h = requisition.height;

    // If widget has no size request, use parent size
    if (w <= 0)
	w = gtk_widget_get_width(GTK_WIDGET(self));
    if (h <= 0)
	h = gtk_widget_get_height(GTK_WIDGET(self));
    if (w <= 0) w = 1;
    if (h <= 0) h = 1;

    transform = gsk_transform_translate(NULL,
	    &GRAPHENE_POINT_INIT((float)child->x, (float)child->y));
    gtk_widget_allocate(child->widget, w, h, -1, transform);
}

/*
 * Place the given widget at the point (x, y).
 */
    void
vim_form_put(VimForm *self, GtkWidget *widget, int x, int y)
{
    VimFormChild *child;

    child = g_new(VimFormChild, 1);

    child->widget = widget;
    child->x = x;
    child->y = y;

    gtk_widget_set_size_request(child->widget, -1, -1);

    self->children = g_list_append(self->children, child);

    gtk_widget_set_parent(widget, GTK_WIDGET(self));
    vim_form_position_child(self, child);
}

/*
 * Move the widget (which should have already been added using vim_form_put())
 * to the given point (x, y).
 */
    void
vim_form_move(VimForm *self, GtkWidget *widget, int x, int y)
{
    for (GList *ele = self->children; ele != NULL; ele = ele->next)
    {
	VimFormChild *child = ele->data;
	if (child->widget == widget)
	{
	    child->x = x;
	    child->y = y;
	    vim_form_position_child(self, child);
	    return;
	}
    }
}

/*
 * Move and resize the child.
 */
    void
vim_form_move_resize(
	VimForm	    *self,
	GtkWidget   *widget,
	gint	    x,
	gint	    y,
	gint	    w,
	gint	    h)
{
    gtk_widget_set_size_request(widget, w, h);
    vim_form_move(self, widget, x, y);
}

    void
vim_form_remove(VimForm *self, GtkWidget *widget)
{
    for (GList *ele = self->children; ele != NULL; ele = ele->next)
    {
	VimFormChild *child = ele->data;
	if (child->widget == widget)
	{
	    self->children = g_list_remove_link(self->children, ele);
	    g_list_free_1(ele);
	    gtk_widget_unparent(widget);
	    g_free(child);
	    return;
	}
    }
}

    static void
vim_form_snapshot(GtkWidget *widget, GtkSnapshot *snapshot)
{
    VimForm *self = VIM_FORM(widget);

    for (GList *ele = self->children; ele != NULL; ele = ele->next)
    {
	VimFormChild *child = ele->data;
	gtk_widget_snapshot_child(widget, child->widget, snapshot);
    }
}

    static gboolean
vim_form_resize_idle_cb(VimForm *self)
{
    int w, h;

    self->resize_idle_id = 0;

    // Use drawarea's actual allocation, not formwin's
    if (gui.drawarea == NULL)
	goto exit;
    w = gtk_widget_get_width(gui.drawarea);
    h = gtk_widget_get_height(gui.drawarea);

    if (w > 1 && h > 1)
	gui_resize_shell(w, h);

exit:
    g_object_unref(self);
    return G_SOURCE_REMOVE;
}

    static void
vim_form_size_allocate(
	GtkWidget *widget,
	int width,
	int height,
	int baseline)
{
    VimForm *self = VIM_FORM(widget);

    for (GList *ele = self->children; ele != NULL; ele = ele->next)
	vim_form_position_child(self, ele->data);

    // Notify Vim about size change via idle callback
    if (width != self->last_width || height != self->last_height)
    {
	self->last_width = width;
	self->last_height = height;

	if (self->resize_idle_id == 0)
	    self->resize_idle_id = g_idle_add(
		    (GSourceFunc)vim_form_resize_idle_cb, g_object_ref(self));
    }
}

    static void
vim_form_measure(
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


/*
 * Make the form itself input-transparent so clicks on its empty area fall
 * through to the drawarea below, while the scrollbar children stay pickable.
 */
    static gboolean
vim_form_contains(GtkWidget *widget UNUSED, double x UNUSED, double y UNUSED)
{
    return FALSE;
}
