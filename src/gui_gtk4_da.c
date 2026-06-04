/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#ifdef USE_GTK4_SNAPSHOT

#include <gtk/gtk.h>
#include "gui_gtk4_da.h"

/*
 * Each cell holds its own reference to a render node if any.
 */
typedef struct
{
    GskRenderNode   *node;	// May be NULL
    gboolean	    inverted;   // If this cell is inverted
} DrawCell;

struct _VimDrawArea
{
    GtkWidget parent;

    DrawCell	*cells; // May be NULL, always checK!
    int         n_rows;
    int		n_cols;

    // See vim_draw_area_size_allocate()
    guint   resize_timeout_id;
    int	    pending_width;
    int	    pending_height;
};

#define GET_ROW(da, n) ((da)->cells + (da)->n_cols * (n))

G_DEFINE_TYPE(VimDrawArea, vim_draw_area, GTK_TYPE_WIDGET)

static void vim_draw_area_snapshot(GtkWidget *widget, GtkSnapshot *snapshot);
static void vim_draw_area_size_allocate(GtkWidget *widget, int width, int height, int baseline);

    static void
vim_draw_area_finalize(GObject *obj)
{
    VimDrawArea *self = VIM_DRAW_AREA(obj);

    vim_draw_area_clear(self);
    g_free(self->cells);

    G_OBJECT_CLASS(vim_draw_area_parent_class)->finalize(obj);
}

    static void
vim_draw_area_class_init(VimDrawAreaClass *class)
{
    GtkWidgetClass  *widget_class = GTK_WIDGET_CLASS(class);
    GObjectClass    *obj_class = G_OBJECT_CLASS(class);

    widget_class->snapshot = vim_draw_area_snapshot;
    widget_class->size_allocate = vim_draw_area_size_allocate;

    obj_class->finalize = vim_draw_area_finalize;

}

    static void
vim_draw_area_init(VimDrawArea *class)
{
}

    GtkWidget *
vim_draw_area_new(void)
{
    return g_object_new(VIM_TYPE_DRAW_AREA, NULL);
}

/*
 * Set the size of the draw area to "rows" and "cols".
 */
    void
vim_draw_area_set_size(VimDrawArea *self, int rows, int cols)
{
    if (self->cells != NULL && self->n_rows == rows && self->n_cols == cols)
	return;
    if (rows == 0 || cols == 0)
	return;

    vim_draw_area_clear(self);

    self->n_rows = rows;
    self->n_cols = cols;
    self->cells = g_realloc_n(self->cells, rows, sizeof(DrawCell) * cols);
    memset(self->cells, 0, rows * (sizeof(DrawCell) * cols));
}

/*
 * Add the glyph string starting at column "col" in row "row". This will handle
 * any background colours, fake bold, and under decorations. This does not queue
 * a redraw for the widget.
 */
    void
vim_draw_area_add_glyphs(
	VimDrawArea *self,
	int row,
	int col,
	int num_cells,
	int flags,
	PangoFont *font,
	PangoGlyphString *glyphs)
{
    if (unlikely(self->cells == NULL
		|| row >= self->n_rows
		|| col >= self->n_cols
		|| col + num_cells > self->n_cols))
	return;
}

/*
 * Clear out the block with the given bounds (inclusive).
 */
    void
vim_draw_area_clear_block(
	VimDrawArea	*self,
	int		row1,
	int		col1,
	int		row2,
	int		col2)
{

    if (unlikely(self->cells == NULL
		|| row1 >= self->n_rows
		|| col1 >= self->n_cols
		|| row2 >= self->n_rows
		|| col2 >= self->n_cols))
	return;

    for (int r = row1; r <= row2; r++)
	;
}

/*
 * Clear out the entire draw area
 */
    void
vim_draw_area_clear(VimDrawArea *self)
{
    vim_draw_area_clear_block(self, 0, 0, self->n_rows - 1, self->n_cols - 1);
}

    static void
vim_draw_area_snapshot(GtkWidget *widget, GtkSnapshot *snapshot)
{
    VimDrawArea		    *self = VIM_DRAW_AREA(widget);
    int			    height, width;

    gui_mch_set_bg_color(gui.back_pixel);
    height = gtk_widget_get_height(widget);
    width = gtk_widget_get_width(widget);

    gtk_snapshot_append_color(snapshot, gui.bgcolor,
	    &GRAPHENE_RECT_INIT(0, 0, width, height));

    if (self->cells == NULL)
	return;

    for (int r = 0; r < self->n_rows; r++)
    {
	DrawCell	*drow = GET_ROW(self, r);
	GskRenderNode	*last_node = NULL;

	for (int c = 0; c < self->n_cols; c++)
	{
	    DrawCell	    *dcell = drow + c;
	    GskRenderNode   *node = dcell->node;

	    if (node == NULL)
		continue;

	    if (node != last_node)
	    {
		gtk_snapshot_append_node(snapshot, node);
		last_node = node;
	    }
	}
    }
}

    static gboolean
vim_draw_area_size_apply_cb(VimDrawArea *self)
{
    self->resize_timeout_id = 0;

    if (updating_screen)
    {
	// Wait again
	self->resize_timeout_id = g_timeout_add(50,
		(GSourceFunc)vim_draw_area_size_apply_cb, self);
	return G_SOURCE_REMOVE;
    }

    gui_resize_shell(self->pending_width, self->pending_height);

    g_object_unref(self);
    return G_SOURCE_REMOVE;
}

    static void
vim_draw_area_size_allocate(
	GtkWidget   *widget,
	int	    width,
	int	    height,
	int	    baseline)
{
    VimDrawArea *self = VIM_DRAW_AREA(widget);

    self->pending_width = width;
    self->pending_height = height;

    // Don't resize immediately, add a debounce. This seems to prevent the
    // current shell size from being outdated.
    if (self->resize_timeout_id != 0)
	g_source_remove(self->resize_timeout_id);
    else
	g_object_ref(self);

    self->resize_timeout_id = g_timeout_add(100,
	    (GSourceFunc)vim_draw_area_size_apply_cb, self);
}

#endif // USE_GTK4_SNAPSHOT
