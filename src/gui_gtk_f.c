/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * (C) 1998,1999 by Marcin Dalecki <martin@dalecki.de>
 *
 * Support for GTK+ 2 was added by:
 *
 * (C) 2002,2003  Jason Hildebrand  <jason@peaceworks.ca>
 *		  Daniel Elstner  <daniel.elstner@gmx.net>
 *
 * This is a special purpose container widget, which manages arbitrary
 * children at arbitrary positions width arbitrary sizes.  This finally puts
 * an end on our resize problems with which we where struggling for such a
 * long time.
 *
 * Support for GTK+ 3 was added by:
 *
 * 2016  Kazunobu Kuriyama  <kazunobu.kuriyama@gmail.com>
 */

#include "vim.h"
#include <gtk/gtk.h>	/* without this it compiles, but gives errors at
			   runtime! */
#include "gui_gtk_f.h"
#if !GTK_CHECK_VERSION(3,0,0)
# include <gtk/gtksignal.h>
#endif
#ifdef WIN3264
# include <gdk/gdkwin32.h>
#else
# include <gdk/gdkx.h>
#endif

typedef struct _GtkFormChild GtkFormChild;

struct _GtkFormChild
{
    GtkWidget *widget;
    GdkWindow *window;
    gint x;		/* relative subwidget x position */
    gint y;		/* relative subwidget y position */
    gint mapped;
};


static void gtk_form_class_init(GtkFormClass *klass);
static void gtk_form_init(GtkForm *form);

static void gtk_form_realize(GtkWidget *widget);
static void gtk_form_unrealize(GtkWidget *widget);
static void gtk_form_map(GtkWidget *widget);
static void gtk_form_size_request(GtkWidget *widget,
				  GtkRequisition *requisition);
#if GTK_CHECK_VERSION(3,0,0)
static void gtk_form_get_preferred_width(GtkWidget *widget,
					 gint *minimal_width,
					 gint *natural_width);
static void gtk_form_get_preferred_height(GtkWidget *widget,
					  gint *minimal_height,
					  gint *natural_height);
#endif
static void gtk_form_size_allocate(GtkWidget *widget,
				   GtkAllocation *allocation);
#if GTK_CHECK_VERSION(3,0,0)
static gboolean gtk_form_draw(GtkWidget *widget,
			      cairo_t *cr);
#else
static gint gtk_form_expose(GtkWidget *widget,
			    GdkEventExpose *event);
#endif

static void gtk_form_remove(GtkContainer *container,
			    GtkWidget *widget);
static void gtk_form_forall(GtkContainer *container,
			    gboolean include_internals,
			    GtkCallback callback,
			    gpointer callback_data);

static void gtk_form_attach_child_window(GtkForm *form,
					 GtkFormChild *child);
static void gtk_form_realize_child(GtkForm *form,
				   GtkFormChild *child);
static void gtk_form_position_child(GtkForm *form,
				    GtkFormChild *child,
				    gboolean force_allocate);
static void gtk_form_position_children(GtkForm *form);

#if !GTK_CHECK_VERSION(3,16,0)
static void gtk_form_set_static_gravity(GdkWindow *window,
					gboolean use_static);
#endif

static void gtk_form_send_configure(GtkForm *form);

static void gtk_form_child_map(GtkWidget *widget, gpointer user_data);
static void gtk_form_child_unmap(GtkWidget *widget, gpointer user_data);

#if !GTK_CHECK_VERSION(3,0,0)
static GtkWidgetClass *parent_class = NULL;
#endif

/* Public interface
 */

    GtkWidget *
gtk_form_new(void)
{
    GtkForm *form;

#if GTK_CHECK_VERSION(3,0,0)
    form = g_object_new(GTK_TYPE_FORM, NULL);
#else
    form = gtk_type_new(gtk_form_get_type());
#endif

    return GTK_WIDGET(form);
}

    void
gtk_form_put(GtkForm	*form,
	     GtkWidget	*child_widget,
	     gint	x,
	     gint	y)
{
    GtkFormChild *child;

    g_return_if_fail(GTK_IS_FORM(form));

    /* LINTED: avoid warning: conversion to 'unsigned long' */
    child = g_new(GtkFormChild, 1);

    child->widget = child_widget;
    child->window = NULL;
    child->x = x;
    child->y = y;
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_size_request(child->widget, -1, -1);
#else
    child->widget->requisition.width = 0;
    child->widget->requisition.height = 0;
#endif
    child->mapped = FALSE;

    form->children = g_list_append(form->children, child);

    /* child->window must be created and attached to the widget _before_
     * it has been realized, or else things will break with GTK2.  Note
     * that gtk_widget_set_parent() realizes the widget if it's visible
     * and its parent is mapped.
     */
#if GTK_CHECK_VERSION(3,0,0)
    if (gtk_widget_get_realized(GTK_WIDGET(form)))
#else
    if (GTK_WIDGET_REALIZED(form))
#endif
	gtk_form_attach_child_window(form, child);

    gtk_widget_set_parent(child_widget, GTK_WIDGET(form));

#if GTK_CHECK_VERSION(3,0,0)
    if (gtk_widget_get_realized(GTK_WIDGET(form))
	    && !gtk_widget_get_realized(child_widget))
#else
    if (GTK_WIDGET_REALIZED(form) && !GTK_WIDGET_REALIZED(child_widget))
#endif
	gtk_form_realize_child(form, child);

    gtk_form_position_child(form, child, TRUE);
}

    void
gtk_form_move(GtkForm	*form,
	      GtkWidget	*child_widget,
	      gint	x,
	      gint	y)
{
    GList *tmp_list;
    GtkFormChild *child;

    g_return_if_fail(GTK_IS_FORM(form));

    for (tmp_list = form->children; tmp_list; tmp_list = tmp_list->next)
    {
	child = tmp_list->data;
	if (child->widget == child_widget)
	{
	    child->x = x;
	    child->y = y;

	    gtk_form_position_child(form, child, TRUE);
	    return;
	}
    }
}

    void
gtk_form_freeze(GtkForm *form)
{
    g_return_if_fail(GTK_IS_FORM(form));

    ++form->freeze_count;
}

    void
gtk_form_thaw(GtkForm *form)
{
    g_return_if_fail(GTK_IS_FORM(form));

    if (form->freeze_count)
    {
	if (!(--form->freeze_count))
	{
	    gtk_form_position_children(form);
	    gtk_widget_queue_draw(GTK_WIDGET(form));
	}
    }
}

/* Basic Object handling procedures
 */
#if GTK_CHECK_VERSION(3,0,0)
G_DEFINE_TYPE(GtkForm, gtk_form, GTK_TYPE_CONTAINER)
#else
    GtkType
gtk_form_get_type(void)
{
    static GtkType form_type = 0;

    if (!form_type)
    {
	GtkTypeInfo form_info;

	vim_memset(&form_info, 0, sizeof(form_info));
	form_info.type_name = "GtkForm";
	form_info.object_size = sizeof(GtkForm);
	form_info.class_size = sizeof(GtkFormClass);
	form_info.class_init_func = (GtkClassInitFunc)gtk_form_class_init;
	form_info.object_init_func = (GtkObjectInitFunc)gtk_form_init;

	form_type = gtk_type_unique(GTK_TYPE_CONTAINER, &form_info);
    }
    return form_type;
}
#endif /* !GTK_CHECK_VERSION(3,0,0) */

    static void
gtk_form_class_init(GtkFormClass *klass)
{
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;

    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;

#if !GTK_CHECK_VERSION(3,0,0)
    parent_class = gtk_type_class(gtk_container_get_type());
#endif

    widget_class->realize = gtk_form_realize;
    widget_class->unrealize = gtk_form_unrealize;
    widget_class->map = gtk_form_map;
#if GTK_CHECK_VERSION(3,0,0)
    widget_class->get_preferred_width = gtk_form_get_preferred_width;
    widget_class->get_preferred_height = gtk_form_get_preferred_height;
#else
    widget_class->size_request = gtk_form_size_request;
#endif
    widget_class->size_allocate = gtk_form_size_allocate;
#if GTK_CHECK_VERSION(3,0,0)
    widget_class->draw = gtk_form_draw;
#else
    widget_class->expose_event = gtk_form_expose;
#endif

    container_class->remove = gtk_form_remove;
    container_class->forall = gtk_form_forall;
}

    static void
gtk_form_init(GtkForm *form)
{
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_has_window(GTK_WIDGET(form), TRUE);
#endif
    form->children = NULL;
    form->bin_window = NULL;
    form->freeze_count = 0;
}

/*
 * Widget methods
 */

    static void
gtk_form_realize(GtkWidget *widget)
{
    GList *tmp_list;
    GtkForm *form;
    GdkWindowAttr attributes;
    gint attributes_mask;

    g_return_if_fail(GTK_IS_FORM(widget));

    form = GTK_FORM(widget);
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_realized(widget, TRUE);
#else
    GTK_WIDGET_SET_FLAGS(form, GTK_REALIZED);
#endif

    attributes.window_type = GDK_WINDOW_CHILD;
#if GTK_CHECK_VERSION(3,0,0)
    {
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
    }
#else
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
#endif
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual(widget);
#if GTK_CHECK_VERSION(3,0,0)
    attributes.event_mask = GDK_EXPOSURE_MASK;
#else
    attributes.colormap = gtk_widget_get_colormap(widget);
    attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK;
#endif

#if GTK_CHECK_VERSION(3,0,0)
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
#else
    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
#endif

#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_window(widget,
			  gdk_window_new(gtk_widget_get_parent_window(widget),
					 &attributes, attributes_mask));
    gdk_window_set_user_data(gtk_widget_get_window(widget), widget);
#else
    widget->window = gdk_window_new(gtk_widget_get_parent_window(widget),
				    &attributes, attributes_mask);
    gdk_window_set_user_data(widget->window, widget);
#endif

    attributes.x = 0;
    attributes.y = 0;
    attributes.event_mask = gtk_widget_get_events(widget);

#if GTK_CHECK_VERSION(3,0,0)
    form->bin_window = gdk_window_new(gtk_widget_get_window(widget),
				      &attributes, attributes_mask);
#else
    form->bin_window = gdk_window_new(widget->window,
				      &attributes, attributes_mask);
#endif
    gdk_window_set_user_data(form->bin_window, widget);

#if !GTK_CHECK_VERSION(3,16,0)
    gtk_form_set_static_gravity(form->bin_window, TRUE);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    {
	GtkStyleContext * const sctx = gtk_widget_get_style_context(widget);

	gtk_style_context_add_class(sctx, "gtk-form");
	gtk_style_context_set_state(sctx, GTK_STATE_FLAG_NORMAL);
# if !GTK_CHECK_VERSION(3,18,0)
	gtk_style_context_set_background(sctx, gtk_widget_get_window(widget));
	gtk_style_context_set_background(sctx, form->bin_window);
# endif
    }
#else
    widget->style = gtk_style_attach(widget->style, widget->window);
    gtk_style_set_background(widget->style, widget->window, GTK_STATE_NORMAL);
    gtk_style_set_background(widget->style, form->bin_window, GTK_STATE_NORMAL);
#endif

    for (tmp_list = form->children; tmp_list; tmp_list = tmp_list->next)
    {
	GtkFormChild *child = tmp_list->data;

	gtk_form_attach_child_window(form, child);

#if GTK_CHECK_VERSION(3,0,0)
	if (gtk_widget_get_visible(child->widget))
#else
	if (GTK_WIDGET_VISIBLE(child->widget))
#endif
	    gtk_form_realize_child(form, child);
    }
}


/* After reading the documentation at
 * http://developer.gnome.org/doc/API/2.0/gtk/gtk-changes-2-0.html
 * I think it should be possible to remove this function when compiling
 * against gtk-2.0.  It doesn't seem to cause problems, though.
 *
 * Well, I reckon at least the gdk_window_show(form->bin_window)
 * is necessary.  GtkForm is anything but a usual container widget.
 */
    static void
gtk_form_map(GtkWidget *widget)
{
    GList *tmp_list;
    GtkForm *form;

    g_return_if_fail(GTK_IS_FORM(widget));

    form = GTK_FORM(widget);

#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_mapped(widget, TRUE);
#else
    GTK_WIDGET_SET_FLAGS(widget, GTK_MAPPED);
#endif

#if GTK_CHECK_VERSION(3,0,0)
    gdk_window_show(gtk_widget_get_window(widget));
#else
    gdk_window_show(widget->window);
#endif
    gdk_window_show(form->bin_window);

    for (tmp_list = form->children; tmp_list; tmp_list = tmp_list->next)
    {
	GtkFormChild *child = tmp_list->data;

#if GTK_CHECK_VERSION(3,0,0)
	if (gtk_widget_get_visible(child->widget)
		&& !gtk_widget_get_mapped(child->widget))
#else
	if (GTK_WIDGET_VISIBLE(child->widget)
		&& !GTK_WIDGET_MAPPED(child->widget))
#endif
	    gtk_widget_map(child->widget);
    }
}

    static void
gtk_form_unrealize(GtkWidget *widget)
{
    GList *tmp_list;
    GtkForm *form;

    g_return_if_fail(GTK_IS_FORM(widget));

    form = GTK_FORM(widget);

    tmp_list = form->children;

    gdk_window_set_user_data(form->bin_window, NULL);
    gdk_window_destroy(form->bin_window);
    form->bin_window = NULL;

    while (tmp_list)
    {
	GtkFormChild *child = tmp_list->data;

	if (child->window != NULL)
	{
#if GTK_CHECK_VERSION(3,0,0)
	    g_signal_handlers_disconnect_by_func(G_OBJECT(child->widget),
		    FUNC2GENERIC(gtk_form_child_map),
		    child);
	    g_signal_handlers_disconnect_by_func(G_OBJECT(child->widget),
		    FUNC2GENERIC(gtk_form_child_unmap),
		    child);
#else
	    gtk_signal_disconnect_by_func(GTK_OBJECT(child->widget),
		    GTK_SIGNAL_FUNC(gtk_form_child_map),
		    child);
	    gtk_signal_disconnect_by_func(GTK_OBJECT(child->widget),
		    GTK_SIGNAL_FUNC(gtk_form_child_unmap),
		    child);
#endif

	    gdk_window_set_user_data(child->window, NULL);
	    gdk_window_destroy(child->window);

	    child->window = NULL;
	}

	tmp_list = tmp_list->next;
    }

#if GTK_CHECK_VERSION(3,0,0)
    if (GTK_WIDGET_CLASS (gtk_form_parent_class)->unrealize)
	 (* GTK_WIDGET_CLASS (gtk_form_parent_class)->unrealize) (widget);
#else
    if (GTK_WIDGET_CLASS (parent_class)->unrealize)
	 (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
#endif
}

    static void
gtk_form_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
    g_return_if_fail(GTK_IS_FORM(widget));
    g_return_if_fail(requisition != NULL);

    requisition->width = 1;
    requisition->height = 1;
}

#if GTK_CHECK_VERSION(3,0,0)
    static void
gtk_form_get_preferred_width(GtkWidget *widget,
			     gint      *minimal_width,
			     gint      *natural_width)
{
    GtkRequisition requisition;

    gtk_form_size_request(widget, &requisition);

    *minimal_width = requisition.width;
    *natural_width = requisition.width;
}

    static void
gtk_form_get_preferred_height(GtkWidget *widget,
			      gint	*minimal_height,
			      gint	*natural_height)
{
    GtkRequisition requisition;

    gtk_form_size_request(widget, &requisition);

    *minimal_height = requisition.height;
    *natural_height = requisition.height;
}
#endif /* GTK_CHECK_VERSION(3,0,0) */

    static void
gtk_form_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
    GList *tmp_list;
    GtkForm *form;
    gboolean need_reposition;
#if GTK_CHECK_VERSION(3,0,0)
    GtkAllocation cur_alloc;
#endif

    g_return_if_fail(GTK_IS_FORM(widget));

#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_get_allocation(widget, &cur_alloc);

    if (cur_alloc.x == allocation->x
	    && cur_alloc.y == allocation->y
	    && cur_alloc.width == allocation->width
	    && cur_alloc.height == allocation->height)
#else
    if (widget->allocation.x == allocation->x
	    && widget->allocation.y == allocation->y
	    && widget->allocation.width == allocation->width
	    && widget->allocation.height == allocation->height)
#endif
	return;

#if GTK_CHECK_VERSION(3,0,0)
    need_reposition = cur_alloc.width != allocation->width
		   || cur_alloc.height != allocation->height;
#else
    need_reposition = widget->allocation.width != allocation->width
		   || widget->allocation.height != allocation->height;
#endif
    form = GTK_FORM(widget);

    if (need_reposition)
    {
	tmp_list = form->children;

	while (tmp_list)
	{
	    GtkFormChild *child = tmp_list->data;
	    gtk_form_position_child(form, child, TRUE);

	    tmp_list = tmp_list->next;
	}
    }

#if GTK_CHECK_VERSION(3,0,0)
    if (gtk_widget_get_realized(widget))
#else
    if (GTK_WIDGET_REALIZED(widget))
#endif
    {
#if GTK_CHECK_VERSION(3,0,0)
	gdk_window_move_resize(gtk_widget_get_window(widget),
			       allocation->x, allocation->y,
			       allocation->width, allocation->height);
#else
	gdk_window_move_resize(widget->window,
			       allocation->x, allocation->y,
			       allocation->width, allocation->height);
#endif
	gdk_window_move_resize(GTK_FORM(widget)->bin_window,
			       0, 0,
			       allocation->width, allocation->height);
    }
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_allocation(widget, allocation);
#else
    widget->allocation = *allocation;
#endif
    if (need_reposition)
	gtk_form_send_configure(form);
}

#if GTK_CHECK_VERSION(3,0,0)
    static void
gtk_form_render_background(GtkWidget *widget, cairo_t *cr)
{
    gtk_render_background(gtk_widget_get_style_context(widget), cr,
			  0, 0,
			  gtk_widget_get_allocated_width(widget),
			  gtk_widget_get_allocated_height(widget));
}

    static gboolean
gtk_form_draw(GtkWidget *widget, cairo_t *cr)
{
    GList   *tmp_list = NULL;
    GtkForm *form     = NULL;

    g_return_val_if_fail(GTK_IS_FORM(widget), FALSE);

    gtk_form_render_background(widget, cr);

    form = GTK_FORM(widget);
    for (tmp_list = form->children; tmp_list; tmp_list = tmp_list->next)
    {
	GtkFormChild * const formchild = tmp_list->data;

	if (!gtk_widget_get_has_window(formchild->widget) &&
		gtk_cairo_should_draw_window(cr, formchild->window))
	{
	    /* To get gtk_widget_draw() to work, it is required to call
	     * gtk_widget_size_allocate() in advance with a well-posed
	     * allocation for a given child widget in order to set a
	     * certain private GtkWidget variable, called
	     * widget->priv->alloc_need, to the proper value; othewise,
	     * gtk_widget_draw() fails and the relevant scrollbar won't
	     * appear on the screen.
	     *
	     * Calling gtk_form_position_child() like this is one of ways
	     * to make sure of that. */
	    gtk_form_position_child(form, formchild, TRUE);

	    gtk_form_render_background(formchild->widget, cr);
	}
    }

    return GTK_WIDGET_CLASS(gtk_form_parent_class)->draw(widget, cr);
}
#else /* !GTK_CHECK_VERSION(3,0,0) */
    static gint
gtk_form_expose(GtkWidget *widget, GdkEventExpose *event)
{
    GList   *tmp_list;
    GtkForm *form;

    g_return_val_if_fail(GTK_IS_FORM(widget), FALSE);

    form = GTK_FORM(widget);

    if (event->window == form->bin_window)
	return FALSE;

    for (tmp_list = form->children; tmp_list; tmp_list = tmp_list->next)
	gtk_container_propagate_expose(GTK_CONTAINER(widget),
		GTK_WIDGET(((GtkFormChild *)tmp_list->data)->widget),
		event);

    return FALSE;
}
#endif /* !GTK_CHECK_VERSION(3,0,0) */

/* Container method
 */
    static void
gtk_form_remove(GtkContainer *container, GtkWidget *widget)
{
    GList *tmp_list;
    GtkForm *form;
    GtkFormChild *child = NULL;	    /* init for gcc */

    g_return_if_fail(GTK_IS_FORM(container));

    form = GTK_FORM(container);

    tmp_list = form->children;
    while (tmp_list)
    {
	child = tmp_list->data;
	if (child->widget == widget)
	    break;
	tmp_list = tmp_list->next;
    }

    if (tmp_list)
    {
#if GTK_CHECK_VERSION(3,0,0)
	const gboolean was_visible = gtk_widget_get_visible(widget);
#endif
	if (child->window)
	{
#if GTK_CHECK_VERSION(3,0,0)
	    g_signal_handlers_disconnect_by_func(G_OBJECT(child->widget),
		    FUNC2GENERIC(&gtk_form_child_map), child);
	    g_signal_handlers_disconnect_by_func(G_OBJECT(child->widget),
		    FUNC2GENERIC(&gtk_form_child_unmap), child);
#else
	    gtk_signal_disconnect_by_func(GTK_OBJECT(child->widget),
		    GTK_SIGNAL_FUNC(&gtk_form_child_map), child);
	    gtk_signal_disconnect_by_func(GTK_OBJECT(child->widget),
		    GTK_SIGNAL_FUNC(&gtk_form_child_unmap), child);
#endif

	    /* FIXME: This will cause problems for reparenting NO_WINDOW
	     * widgets out of a GtkForm
	     */
	    gdk_window_set_user_data(child->window, NULL);
	    gdk_window_destroy(child->window);
	}
	gtk_widget_unparent(widget);
#if GTK_CHECK_VERSION(3,0,0)
	if (was_visible)
	    gtk_widget_queue_resize(GTK_WIDGET(container));
#endif
	form->children = g_list_remove_link(form->children, tmp_list);
	g_list_free_1(tmp_list);
	g_free(child);
    }
}

    static void
gtk_form_forall(GtkContainer	*container,
		gboolean	include_internals UNUSED,
		GtkCallback	callback,
		gpointer	callback_data)
{
    GtkForm *form;
    GtkFormChild *child;
    GList *tmp_list;

    g_return_if_fail(GTK_IS_FORM(container));
    g_return_if_fail(callback != NULL);

    form = GTK_FORM(container);

    tmp_list = form->children;
    while (tmp_list)
    {
	child = tmp_list->data;
	tmp_list = tmp_list->next;

	(*callback) (child->widget, callback_data);
    }
}

/* Operations on children
 */

    static void
gtk_form_attach_child_window(GtkForm *form, GtkFormChild *child)
{
    if (child->window != NULL)
	return; /* been there, done that */

#if GTK_CHECK_VERSION(3,0,0)
    if (!gtk_widget_get_has_window(child->widget))
#else
    if (GTK_WIDGET_NO_WINDOW(child->widget))
#endif
    {
	GtkWidget	*widget;
	GdkWindowAttr	attributes;
	gint		attributes_mask;

	widget = GTK_WIDGET(form);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = child->x;
	attributes.y = child->y;
#if GTK_CHECK_VERSION(3,0,0)
	{
	    GtkRequisition requisition;

	    gtk_widget_get_preferred_size(child->widget, &requisition, NULL);

	    attributes.width = requisition.width;
	    attributes.height = requisition.height;
	}
#else
	attributes.width = child->widget->requisition.width;
	attributes.height = child->widget->requisition.height;
#endif
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual(widget);
#if !GTK_CHECK_VERSION(3,0,0)
	attributes.colormap = gtk_widget_get_colormap(widget);
#endif
	attributes.event_mask = GDK_EXPOSURE_MASK;

#if GTK_CHECK_VERSION(3,0,0)
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;
#else
	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
#endif
	child->window = gdk_window_new(form->bin_window,
				       &attributes, attributes_mask);
	gdk_window_set_user_data(child->window, widget);

#if GTK_CHECK_VERSION(3,0,0)
	{
	    GtkStyleContext * const sctx = gtk_widget_get_style_context(widget);

	    gtk_style_context_set_state(sctx, GTK_STATE_FLAG_NORMAL);
# if !GTK_CHECK_VERSION(3,18,0)
	    gtk_style_context_set_background(sctx, child->window);
# endif
	}
#else
	gtk_style_set_background(widget->style,
				 child->window,
				 GTK_STATE_NORMAL);
#endif

	gtk_widget_set_parent_window(child->widget, child->window);
#if !GTK_CHECK_VERSION(3,16,0)
	gtk_form_set_static_gravity(child->window, TRUE);
#endif
	/*
	 * Install signal handlers to map/unmap child->window
	 * alongside with the actual widget.
	 */
#if GTK_CHECK_VERSION(3,0,0)
	g_signal_connect(G_OBJECT(child->widget), "map",
			 G_CALLBACK(&gtk_form_child_map), child);
	g_signal_connect(G_OBJECT(child->widget), "unmap",
			 G_CALLBACK(&gtk_form_child_unmap), child);
#else
	gtk_signal_connect(GTK_OBJECT(child->widget), "map",
			   GTK_SIGNAL_FUNC(&gtk_form_child_map), child);
	gtk_signal_connect(GTK_OBJECT(child->widget), "unmap",
			   GTK_SIGNAL_FUNC(&gtk_form_child_unmap), child);
#endif
    }
#if GTK_CHECK_VERSION(3,0,0)
    else if (!gtk_widget_get_realized(child->widget))
#else
    else if (!GTK_WIDGET_REALIZED(child->widget))
#endif
    {
	gtk_widget_set_parent_window(child->widget, form->bin_window);
    }
}

    static void
gtk_form_realize_child(GtkForm *form, GtkFormChild *child)
{
    gtk_form_attach_child_window(form, child);
    gtk_widget_realize(child->widget);

#if !GTK_CHECK_VERSION(3,16,0)
    if (child->window == NULL) /* might be already set, see above */
# if GTK_CHECK_VERSION(3,0,0)
	gtk_form_set_static_gravity(gtk_widget_get_window(child->widget), TRUE);
# else
	gtk_form_set_static_gravity(child->widget->window, TRUE);
# endif
#endif
}

    static void
gtk_form_position_child(GtkForm *form, GtkFormChild *child,
			gboolean force_allocate)
{
    gint x;
    gint y;

    x = child->x;
    y = child->y;

    if ((x >= G_MINSHORT) && (x <= G_MAXSHORT) &&
	(y >= G_MINSHORT) && (y <= G_MAXSHORT))
    {
	if (!child->mapped)
	{
#if GTK_CHECK_VERSION(3,0,0)
	    if (gtk_widget_get_mapped(GTK_WIDGET(form))
		    && gtk_widget_get_visible(child->widget))
#else
	    if (GTK_WIDGET_MAPPED(form) && GTK_WIDGET_VISIBLE(child->widget))
#endif
	    {
#if GTK_CHECK_VERSION(3,0,0)
		if (!gtk_widget_get_mapped(child->widget))
#else
		if (!GTK_WIDGET_MAPPED(child->widget))
#endif
		    gtk_widget_map(child->widget);

		child->mapped = TRUE;
		force_allocate = TRUE;
	    }
	}

	if (force_allocate)
	{
	    GtkAllocation allocation;
#if GTK_CHECK_VERSION(3,0,0)
	    GtkRequisition requisition;

	    gtk_widget_get_preferred_size(child->widget, &requisition, NULL);
#endif

#if GTK_CHECK_VERSION(3,0,0)
	    if (!gtk_widget_get_has_window(child->widget))
#else
	    if (GTK_WIDGET_NO_WINDOW(child->widget))
#endif
	    {
		if (child->window)
		{
#if GTK_CHECK_VERSION(3,0,0)
		    gdk_window_move_resize(child->window,
			    x, y,
			    requisition.width,
			    requisition.height);
#else
		    gdk_window_move_resize(child->window,
			    x, y,
			    child->widget->requisition.width,
			    child->widget->requisition.height);
#endif
		}

		allocation.x = 0;
		allocation.y = 0;
	    }
	    else
	    {
		allocation.x = x;
		allocation.y = y;
	    }

#if GTK_CHECK_VERSION(3,0,0)
	    allocation.width = requisition.width;
	    allocation.height = requisition.height;
#else
	    allocation.width = child->widget->requisition.width;
	    allocation.height = child->widget->requisition.height;
#endif

	    gtk_widget_size_allocate(child->widget, &allocation);
	}
    }
    else
    {
	if (child->mapped)
	{
	    child->mapped = FALSE;

#if GTK_CHECK_VERSION(3,0,0)
	    if (gtk_widget_get_mapped(child->widget))
#else
	    if (GTK_WIDGET_MAPPED(child->widget))
#endif
		gtk_widget_unmap(child->widget);
	}
    }
}

    static void
gtk_form_position_children(GtkForm *form)
{
    GList *tmp_list;

    for (tmp_list = form->children; tmp_list; tmp_list = tmp_list->next)
	gtk_form_position_child(form, tmp_list->data, FALSE);
}

#if !GTK_CHECK_VERSION(3,16,0)
    static void
gtk_form_set_static_gravity(GdkWindow *window, gboolean use_static)
{
    /* We don't check if static gravity is actually supported, because it
     * results in an annoying assertion error message. */
    gdk_window_set_static_gravities(window, use_static);
}
#endif /* !GTK_CHECK_VERSION(3,16,0) */

    void
gtk_form_move_resize(GtkForm *form, GtkWidget *widget,
		     gint x, gint y, gint w, gint h)
{
#if GTK_CHECK_VERSION(3,0,0)
    gtk_widget_set_size_request(widget, w, h);
#else
    widget->requisition.width  = w;
    widget->requisition.height = h;
#endif

    gtk_form_move(form, widget, x, y);
}

    static void
gtk_form_send_configure(GtkForm *form)
{
    GtkWidget *widget;
    GdkEventConfigure event;

    widget = GTK_WIDGET(form);

    event.type = GDK_CONFIGURE;
#if GTK_CHECK_VERSION(3,0,0)
    event.window = gtk_widget_get_window(widget);
    {
	GtkAllocation allocation;

	gtk_widget_get_allocation(widget, &allocation);
	event.x = allocation.x;
	event.y = allocation.y;
	event.width = allocation.width;
	event.height = allocation.height;
    }
#else
    event.window = widget->window;
    event.x = widget->allocation.x;
    event.y = widget->allocation.y;
    event.width = widget->allocation.width;
    event.height = widget->allocation.height;
#endif

    gtk_main_do_event((GdkEvent*)&event);
}

    static void
gtk_form_child_map(GtkWidget *widget UNUSED, gpointer user_data)
{
    GtkFormChild *child;

    child = (GtkFormChild *)user_data;

    child->mapped = TRUE;
    gdk_window_show(child->window);
}

    static void
gtk_form_child_unmap(GtkWidget *widget UNUSED, gpointer user_data)
{
    GtkFormChild *child;

    child = (GtkFormChild *)user_data;

    child->mapped = FALSE;
    gdk_window_hide(child->window);
}
