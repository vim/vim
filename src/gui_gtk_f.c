/* vi:set ts=8 sts=4 sw=4:
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
 */

#include "vim.h"
#include <gtk/gtk.h>	/* without this it compiles, but gives errors at
			   runtime! */
#include "gui_gtk_f.h"
#include <gtk/gtksignal.h>
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
static void gtk_form_size_allocate(GtkWidget *widget,
				   GtkAllocation *allocation);
#ifndef HAVE_GTK2  /* this isn't needed in gtk2 */
static void gtk_form_draw(GtkWidget *widget,
			  GdkRectangle *area);
#endif
static gint gtk_form_expose(GtkWidget *widget,
			    GdkEventExpose *event);

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

static GdkFilterReturn gtk_form_filter(GdkXEvent *gdk_xevent,
				       GdkEvent *event,
				       gpointer data);
static GdkFilterReturn gtk_form_main_filter(GdkXEvent *gdk_xevent,
					    GdkEvent *event,
					    gpointer data);

static void gtk_form_set_static_gravity(GdkWindow *window,
					gboolean use_static);

static void gtk_form_send_configure(GtkForm *form);

static void gtk_form_child_map(GtkWidget *widget, gpointer user_data);
static void gtk_form_child_unmap(GtkWidget *widget, gpointer user_data);

static GtkWidgetClass *parent_class = NULL;

/* Public interface
 */

    GtkWidget *
gtk_form_new(void)
{
    GtkForm *form;

    form = gtk_type_new(gtk_form_get_type());

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
    child->widget->requisition.width = 0;
    child->widget->requisition.height = 0;
    child->mapped = FALSE;

    form->children = g_list_append(form->children, child);

    /* child->window must be created and attached to the widget _before_
     * it has been realized, or else things will break with GTK2.  Note
     * that gtk_widget_set_parent() realizes the widget if it's visible
     * and its parent is mapped.
     */
    if (GTK_WIDGET_REALIZED(form))
	gtk_form_attach_child_window(form, child);

    gtk_widget_set_parent(child_widget, GTK_WIDGET(form));
    gtk_widget_size_request(child->widget, NULL);

    if (GTK_WIDGET_REALIZED(form) && !GTK_WIDGET_REALIZED(child_widget))
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
gtk_form_set_size(GtkForm *form, guint width, guint height)
{
    g_return_if_fail(GTK_IS_FORM(form));

    /* prevent unneccessary calls */
    if (form->width == width && form->height == height)
	return;
    form->width = width;
    form->height = height;

    /* signal the change */
#ifdef HAVE_GTK2
    gtk_widget_queue_resize(gtk_widget_get_parent(GTK_WIDGET(form)));
#else
    gtk_container_queue_resize(GTK_CONTAINER(GTK_WIDGET(form)->parent));
#endif
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
#ifdef HAVE_GTK2
	    gtk_widget_queue_draw(GTK_WIDGET(form));
#else
	    gtk_widget_draw(GTK_WIDGET(form), NULL);
#endif
	}
    }
}

/* Basic Object handling procedures
 */
    GtkType
gtk_form_get_type(void)
{
    static GtkType form_type = 0;

    if (!form_type)
    {
	GtkTypeInfo form_info =
	{
	    "GtkForm",
	    sizeof(GtkForm),
	    sizeof(GtkFormClass),
	    (GtkClassInitFunc) gtk_form_class_init,
	    (GtkObjectInitFunc) gtk_form_init
	};

	form_type = gtk_type_unique(GTK_TYPE_CONTAINER, &form_info);
    }
    return form_type;
}

    static void
gtk_form_class_init(GtkFormClass *klass)
{
    GtkWidgetClass *widget_class;
    GtkContainerClass *container_class;

    widget_class = (GtkWidgetClass *) klass;
    container_class = (GtkContainerClass *) klass;

    parent_class = gtk_type_class(gtk_container_get_type());

    widget_class->realize = gtk_form_realize;
    widget_class->unrealize = gtk_form_unrealize;
    widget_class->map = gtk_form_map;
    widget_class->size_request = gtk_form_size_request;
    widget_class->size_allocate = gtk_form_size_allocate;
#ifndef HAVE_GTK2 /* not needed for GTK2 */
    widget_class->draw = gtk_form_draw;
#endif
    widget_class->expose_event = gtk_form_expose;

    container_class->remove = gtk_form_remove;
    container_class->forall = gtk_form_forall;
}

    static void
gtk_form_init(GtkForm *form)
{
    form->children = NULL;

    form->width = 1;
    form->height = 1;

    form->bin_window = NULL;

    form->configure_serial = 0;
    form->visibility = GDK_VISIBILITY_PARTIAL;

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
    GTK_WIDGET_SET_FLAGS(form, GTK_REALIZED);

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = widget->allocation.x;
    attributes.y = widget->allocation.y;
    attributes.width = widget->allocation.width;
    attributes.height = widget->allocation.height;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.visual = gtk_widget_get_visual(widget);
    attributes.colormap = gtk_widget_get_colormap(widget);
    attributes.event_mask = GDK_VISIBILITY_NOTIFY_MASK;

    attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

    widget->window = gdk_window_new(gtk_widget_get_parent_window(widget),
				    &attributes, attributes_mask);
    gdk_window_set_user_data(widget->window, widget);

    attributes.x = 0;
    attributes.y = 0;
    attributes.event_mask = gtk_widget_get_events(widget);

    form->bin_window = gdk_window_new(widget->window,
				      &attributes, attributes_mask);
    gdk_window_set_user_data(form->bin_window, widget);

    gtk_form_set_static_gravity(form->bin_window, TRUE);

    widget->style = gtk_style_attach(widget->style, widget->window);
    gtk_style_set_background(widget->style, widget->window, GTK_STATE_NORMAL);
    gtk_style_set_background(widget->style, form->bin_window, GTK_STATE_NORMAL);

    gdk_window_add_filter(widget->window, gtk_form_main_filter, form);
    gdk_window_add_filter(form->bin_window, gtk_form_filter, form);

    for (tmp_list = form->children; tmp_list; tmp_list = tmp_list->next)
    {
	GtkFormChild *child = tmp_list->data;

	gtk_form_attach_child_window(form, child);

	if (GTK_WIDGET_VISIBLE(child->widget))
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

    GTK_WIDGET_SET_FLAGS(widget, GTK_MAPPED);

    gdk_window_show(widget->window);
    gdk_window_show(form->bin_window);

    for (tmp_list = form->children; tmp_list; tmp_list = tmp_list->next)
    {
	GtkFormChild *child = tmp_list->data;

	if (GTK_WIDGET_VISIBLE(child->widget)
		&& !GTK_WIDGET_MAPPED(child->widget))
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
	    gtk_signal_disconnect_by_func(GTK_OBJECT(child->widget),
					  GTK_SIGNAL_FUNC(gtk_form_child_map),
					  child);
	    gtk_signal_disconnect_by_func(GTK_OBJECT(child->widget),
					  GTK_SIGNAL_FUNC(gtk_form_child_unmap),
					  child);

	    gdk_window_set_user_data(child->window, NULL);
	    gdk_window_destroy(child->window);

	    child->window = NULL;
	}

	tmp_list = tmp_list->next;
    }

    if (GTK_WIDGET_CLASS (parent_class)->unrealize)
	 (* GTK_WIDGET_CLASS (parent_class)->unrealize) (widget);
}

#ifndef HAVE_GTK2
    static void
gtk_form_draw(GtkWidget *widget, GdkRectangle *area)
{
    GtkForm		*form;
    GList		*children;
    GtkFormChild	*child;
    GdkRectangle	child_area;

    g_return_if_fail(GTK_IS_FORM(widget));

    if (GTK_WIDGET_DRAWABLE(widget))
    {
	form = GTK_FORM(widget);

	children = form->children;

	while (children)
	{
	    child = children->data;

	    if (GTK_WIDGET_DRAWABLE(child->widget)
		    && gtk_widget_intersect(child->widget, area, &child_area))
		gtk_widget_draw(child->widget, &child_area);

	    children = children->next;
	}
    }
}
#endif /* !HAVE_GTK2 */

    static void
gtk_form_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
    GList *tmp_list;
    GtkForm *form;

    g_return_if_fail(GTK_IS_FORM(widget));

    form = GTK_FORM(widget);

    requisition->width = form->width;
    requisition->height = form->height;

    tmp_list = form->children;

    while (tmp_list)
    {
	GtkFormChild *child = tmp_list->data;
	gtk_widget_size_request(child->widget, NULL);
	tmp_list = tmp_list->next;
    }
}

    static void
gtk_form_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
    GList *tmp_list;
    GtkForm *form;
    gboolean need_reposition;

    g_return_if_fail(GTK_IS_FORM(widget));

    if (widget->allocation.x == allocation->x
	    && widget->allocation.y == allocation->y
	    && widget->allocation.width == allocation->width
	    && widget->allocation.height == allocation->height)
	return;

    need_reposition = widget->allocation.width != allocation->width
		   || widget->allocation.height != allocation->height;
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

    if (GTK_WIDGET_REALIZED(widget))
    {
	gdk_window_move_resize(widget->window,
			       allocation->x, allocation->y,
			       allocation->width, allocation->height);
	gdk_window_move_resize(GTK_FORM(widget)->bin_window,
			       0, 0,
			       allocation->width, allocation->height);
    }
    widget->allocation = *allocation;
    if (need_reposition)
	gtk_form_send_configure(form);
}

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
    {
#ifdef HAVE_GTK2
	GtkFormChild	*formchild = tmp_list->data;
	GtkWidget	*child	   = formchild->widget;
	/*
	 * The following chunk of code is taken from gtkcontainer.c.  The
	 * gtk1.x code synthesized expose events directly on the child widgets,
	 * which can't be done in gtk2
	 */
	if (GTK_WIDGET_DRAWABLE(child) && GTK_WIDGET_NO_WINDOW(child)
		&& child->window == event->window)
	{
	    GdkEventExpose child_event;
	    child_event = *event;

	    child_event.region = gtk_widget_region_intersect(child, event->region);
	    if (!gdk_region_empty(child_event.region))
	    {
		gdk_region_get_clipbox(child_event.region, &child_event.area);
		gtk_widget_send_expose(child, (GdkEvent *)&child_event);
	    }
	}
#else /* !HAVE_GTK2 */
	GtkFormChild *child = tmp_list->data;

	if (event->window == child->window)
	    return gtk_widget_event(child->widget, (GdkEvent *) event);
#endif /* !HAVE_GTK2 */
    }

    return FALSE;
}

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
	if (child->window)
	{
	    gtk_signal_disconnect_by_func(GTK_OBJECT(child->widget),
					  GTK_SIGNAL_FUNC(&gtk_form_child_map), child);
	    gtk_signal_disconnect_by_func(GTK_OBJECT(child->widget),
					  GTK_SIGNAL_FUNC(&gtk_form_child_unmap), child);

	    /* FIXME: This will cause problems for reparenting NO_WINDOW
	     * widgets out of a GtkForm
	     */
	    gdk_window_set_user_data(child->window, NULL);
	    gdk_window_destroy(child->window);
	}
	gtk_widget_unparent(widget);

	form->children = g_list_remove_link(form->children, tmp_list);
	g_list_free_1(tmp_list);
	g_free(child);
    }
}

/*ARGSUSED1*/
    static void
gtk_form_forall(GtkContainer	*container,
		gboolean	include_internals,
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

    if (GTK_WIDGET_NO_WINDOW(child->widget))
    {
	GtkWidget	*widget;
	GdkWindowAttr	attributes;
	gint		attributes_mask;

	widget = GTK_WIDGET(form);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = child->x;
	attributes.y = child->y;
	attributes.width = child->widget->requisition.width;
	attributes.height = child->widget->requisition.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual(widget);
	attributes.colormap = gtk_widget_get_colormap(widget);
	attributes.event_mask = GDK_EXPOSURE_MASK;

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
	child->window = gdk_window_new(form->bin_window,
				       &attributes, attributes_mask);
	gdk_window_set_user_data(child->window, widget);

	gtk_style_set_background(widget->style,
				 child->window,
				 GTK_STATE_NORMAL);

	gtk_widget_set_parent_window(child->widget, child->window);
	gtk_form_set_static_gravity(child->window, TRUE);
	/*
	 * Install signal handlers to map/unmap child->window
	 * alongside with the actual widget.
	 */
	gtk_signal_connect(GTK_OBJECT(child->widget), "map",
			   GTK_SIGNAL_FUNC(&gtk_form_child_map), child);
	gtk_signal_connect(GTK_OBJECT(child->widget), "unmap",
			   GTK_SIGNAL_FUNC(&gtk_form_child_unmap), child);
    }
    else if (!GTK_WIDGET_REALIZED(child->widget))
    {
	gtk_widget_set_parent_window(child->widget, form->bin_window);
    }
}

    static void
gtk_form_realize_child(GtkForm *form, GtkFormChild *child)
{
    gtk_form_attach_child_window(form, child);
    gtk_widget_realize(child->widget);

    if (child->window == NULL) /* might be already set, see above */
	gtk_form_set_static_gravity(child->widget->window, TRUE);
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
	    if (GTK_WIDGET_MAPPED(form) && GTK_WIDGET_VISIBLE(child->widget))
	    {
		if (!GTK_WIDGET_MAPPED(child->widget))
		    gtk_widget_map(child->widget);

		child->mapped = TRUE;
		force_allocate = TRUE;
	    }
	}

	if (force_allocate)
	{
	    GtkAllocation allocation;

	    if (GTK_WIDGET_NO_WINDOW(child->widget))
	    {
		if (child->window)
		{
		    gdk_window_move_resize(child->window,
			    x, y,
			    child->widget->requisition.width,
			    child->widget->requisition.height);
		}

		allocation.x = 0;
		allocation.y = 0;
	    }
	    else
	    {
		allocation.x = x;
		allocation.y = y;
	    }

	    allocation.width = child->widget->requisition.width;
	    allocation.height = child->widget->requisition.height;

	    gtk_widget_size_allocate(child->widget, &allocation);
	}
    }
    else
    {
	if (child->mapped)
	{
	    child->mapped = FALSE;

	    if (GTK_WIDGET_MAPPED(child->widget))
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

/* Callbacks */

/* The main event filter. Actually, we probably don't really need
 * to install this as a filter at all, since we are calling it
 * directly above in the expose-handling hack.
 *
 * This routine identifies expose events that are generated when
 * we've temporarily moved the bin_window_origin, and translates
 * them or discards them, depending on whether we are obscured
 * or not.
 */
/*ARGSUSED1*/
    static GdkFilterReturn
gtk_form_filter(GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data)
{
    XEvent *xevent;
    GtkForm *form;

    xevent = (XEvent *) gdk_xevent;
    form = GTK_FORM(data);

    switch (xevent->type)
    {
    case Expose:
	if (xevent->xexpose.serial == form->configure_serial)
	{
	    if (form->visibility == GDK_VISIBILITY_UNOBSCURED)
		return GDK_FILTER_REMOVE;
	    else
		break;
	}
	break;

    case ConfigureNotify:
	if ((xevent->xconfigure.x != 0) || (xevent->xconfigure.y != 0))
	    form->configure_serial = xevent->xconfigure.serial;
	break;
    }

    return GDK_FILTER_CONTINUE;
}

/* Although GDK does have a GDK_VISIBILITY_NOTIFY event,
 * there is no corresponding event in GTK, so we have
 * to get the events from a filter
 */
/*ARGSUSED1*/
    static GdkFilterReturn
gtk_form_main_filter(GdkXEvent *gdk_xevent, GdkEvent *event, gpointer data)
{
    XEvent *xevent;
    GtkForm *form;

    xevent = (XEvent *) gdk_xevent;
    form = GTK_FORM(data);

    if (xevent->type == VisibilityNotify)
    {
	switch (xevent->xvisibility.state)
	{
	case VisibilityFullyObscured:
	    form->visibility = GDK_VISIBILITY_FULLY_OBSCURED;
	    break;

	case VisibilityPartiallyObscured:
	    form->visibility = GDK_VISIBILITY_PARTIAL;
	    break;

	case VisibilityUnobscured:
	    form->visibility = GDK_VISIBILITY_UNOBSCURED;
	    break;
	}

	return GDK_FILTER_REMOVE;
    }
    return GDK_FILTER_CONTINUE;
}

/* Routines to set the window gravity, and check whether it is
 * functional. Extra capabilities need to be added to GDK, so
 * we don't have to use Xlib here.
 */
    static void
gtk_form_set_static_gravity(GdkWindow *window, gboolean use_static)
{
#ifdef HAVE_GTK2
    gboolean static_gravity_supported;

    static_gravity_supported = gdk_window_set_static_gravities(window,
							       use_static);
    g_return_if_fail(static_gravity_supported);
#else
    XSetWindowAttributes xattributes;

    xattributes.win_gravity = (use_static) ? StaticGravity : NorthWestGravity;
    xattributes.bit_gravity = (use_static) ? StaticGravity : NorthWestGravity;

    XChangeWindowAttributes(GDK_WINDOW_XDISPLAY(window),
			    GDK_WINDOW_XWINDOW(window),
			    CWBitGravity | CWWinGravity,
			    &xattributes);
#endif
}

    void
gtk_form_move_resize(GtkForm *form, GtkWidget *widget,
		     gint x, gint y, gint w, gint h)
{
    widget->requisition.width  = w;
    widget->requisition.height = h;

    gtk_form_move(form, widget, x, y);
}

    static void
gtk_form_send_configure(GtkForm *form)
{
    GtkWidget *widget;
    GdkEventConfigure event;

    widget = GTK_WIDGET(form);

    event.type = GDK_CONFIGURE;
    event.window = widget->window;
    event.x = widget->allocation.x;
    event.y = widget->allocation.y;
    event.width = widget->allocation.width;
    event.height = widget->allocation.height;

#ifdef HAVE_GTK2
    gtk_main_do_event((GdkEvent*)&event);
#else
    gtk_widget_event(widget, (GdkEvent*)&event);
#endif
}

/*ARGSUSED0*/
    static void
gtk_form_child_map(GtkWidget *widget, gpointer user_data)
{
    GtkFormChild *child;

    child = (GtkFormChild *)user_data;

    child->mapped = TRUE;
    gdk_window_show(child->window);
}

/*ARGSUSED0*/
    static void
gtk_form_child_unmap(GtkWidget *widget, gpointer user_data)
{
    GtkFormChild *child;

    child = (GtkFormChild *)user_data;

    child->mapped = FALSE;
    gdk_window_hide(child->window);
}

