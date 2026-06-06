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

typedef struct
{
    int refcount;

    GskRenderNode *text_node; // May be NULL if no ink
    GskRenderNode *bg_node;
    GskRenderNode *decor_node;

    // Saved values at creation time
    PangoFont	*font;
    GdkRGBA	fg_color;
    GdkRGBA 	bg_color;
    GdkRGBA 	sp_color;
    int	    	flags;	    // DRAW_* flags

    int row;
    int start_col;
    int n_cells;
} DrawNode;

#define END_COL(dn) ((dn)->start_col + (dn)->n_cells - 1)

/*
 * Each cell holds its own reference to a draw node if any.
 */
typedef struct
{
    DrawNode	*dnode;	    // May be NULL
    gboolean	inverted;   // If this cell is inverted
} DrawCell;

struct _VimDrawArea
{
    GtkWidget parent;

    DrawCell	*cells; // May be NULL, always check!
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

    static void
node_unref(GskRenderNode *node)
{
    if (node != NULL)
	gsk_render_node_unref(node);
}

    static DrawNode *
draw_node_ref(DrawNode *dnode)
{
    dnode->refcount++;
    return dnode;
}

    static void
draw_node_unref(DrawNode *dnode)
{
    if (--dnode->refcount <= 0)
    {
	node_unref(dnode->text_node);
	node_unref(dnode->bg_node);
	node_unref(dnode->decor_node);
	g_object_unref(dnode->font);
	g_free(dnode);
    }
}

/*
 * Create a new under decoration node with the given flags. Returns NULL if no
 * under decorations are needed.
 */
    static GskRenderNode *
create_decor_node(
	int		row,
	int 	    	start_col,
	int 	    	n_cells,
	int	    	flags,
	const GdkRGBA	*fg_color,
	const GdkRGBA   *sp_color)
{
    static GskRenderNode    *nodes[3];
    int			    n_nodes = 0;
    GskRenderNode	    *container;

    if (flags & DRAW_UNDERL)
	nodes[n_nodes++] = gsk_color_node_new(fg_color,
		&GRAPHENE_RECT_INIT(FILL_X(start_col),
		    FILL_Y(row + 1) - 1,
		    FILL_X(start_col + n_cells) - FILL_X(start_col), 1));

    if (flags & DRAW_STRIKE)
	nodes[n_nodes++] = gsk_color_node_new(fg_color,
		&GRAPHENE_RECT_INIT(FILL_X(start_col),
		    FILL_Y(row) + (int)(gui.char_height / 2),
		    FILL_X(start_col + n_cells) - FILL_X(start_col), 1));

    if (flags & DRAW_UNDERC)
    {
	int             y = FILL_Y(row + 1) - 1; // Top of underneath line,
						 // upwards by one pixel.
	int             x_start = FILL_X(start_col);
	int             x_end = FILL_X(start_col + n_cells);
	GskRenderNode	*color_node;

	color_node = gsk_color_node_new(sp_color, &GRAPHENE_RECT_INIT(0, 0, 1, 1));

	// GskPath was added in GSK 4.14, otherwise use cairo
#if GTK_CHECK_VERSION(4, 14, 0)
	GskPathBuilder	*builder;
	GskPath		*path;
	GskStroke	*stroke;

	const int	half_wave = 4;  // Half-cycle width (e.g., 4px up, 4px
					// down)
	const int	amplitude = 2;	// Peak height from baseline
	int		toggle = -1;    // Start by pulling up (-Y is up in GTK)

	builder = gsk_path_builder_new();
	gsk_path_builder_move_to(builder, x_start, y);

	// Each cycle contains two quadratic bezier curves, one going up and one
	// going down.
	for (int x = x_start; x < x_end; x += half_wave)
	{
	    int current_half = half_wave;
	    if (x + current_half > x_end)
	    {
		current_half = x_end - x;
	    }

	    // The control point sits exactly halfway horizontally through the arc
	    int cp_x = x + (current_half / 2);
	    int cp_y = y + (toggle * amplitude);
	    int end_x = x + current_half;

	    gsk_path_builder_quad_to(builder, cp_x, cp_y, end_x, y);

	    toggle = -toggle; // Flip direction for the next half-wave
	}

	path = gsk_path_builder_free_to_path(builder);
	stroke = gsk_stroke_new(1.0f);

	nodes[n_nodes++] = gsk_stroke_node_new(color_node, path, stroke);
	gsk_stroke_free(stroke);
	gsk_path_unref(path);
#else
	static const int    val[8] = {1, 0, 0, 0, 1, 2, 2, 2};
	cairo_t		    *cr;
	GskRenderNode	    *node;

	node = gsk_cairo_node_new(
		&GRAPHENE_RECT_INIT(x_start, y - 3, x_end - x_start, 5));
	cr = gsk_cairo_node_get_draw_context(node);

	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgba(cr, sp_color->red, sp_color->green,
		sp_color->blue, sp_color->alpha);

	cairo_move_to(cr, x_start + 1, y - 2 + 0.5);

	for (int i = x_start + 1; i < x_end; ++i)
	{
	    int offset = val[i % 8];
	    cairo_line_to(cr, i, y - offset + 0.5);
	}

	cairo_stroke(cr);
	cairo_destroy(cr);
	nodes[n_nodes++] = node;
#endif
	gsk_render_node_unref(color_node);
    }

    if (n_nodes == 0)
	return NULL;

    container = gsk_container_node_new(nodes, n_nodes);
    for (int i = 0; i < n_nodes; i++)
	// Container node takes its own reference to each.
	gsk_render_node_unref(nodes[i]);
    return container;
}

/*
 * Create the render nodes for the background and under decorations. Remove
 * existing ones if any.
 */
    static void
draw_node_reset(DrawNode *dnode)
{
    node_unref(dnode->bg_node);
    node_unref(dnode->decor_node);

    if (dnode->flags & DRAW_TRANSP)
	dnode->bg_node = NULL;
    else
    {
        GdkRGBA prev_bg = *gui.bgcolor;
	gboolean bg_is_def;

        gui_mch_set_bg_color(gui.back_pixel);
	bg_is_def = gdk_rgba_equal(&dnode->bg_color, gui.bgcolor);
        *gui.bgcolor = prev_bg;

        dnode->bg_node = bg_is_def ? NULL : gsk_color_node_new(
            &dnode->bg_color,
            &GRAPHENE_RECT_INIT(FILL_X(dnode->start_col), FILL_Y(dnode->row),
                                dnode->n_cells * gui.char_width,
                                gui.char_height));
    }

    dnode->decor_node =
        create_decor_node(dnode->row, dnode->start_col, dnode->n_cells,
                          dnode->flags, &dnode->fg_color, &dnode->sp_color);
}

/*
 * If "node" is NULL and "bg" is the same as the default background
 * color return TRUE, then return FALSE.
 */
    static gboolean
node_is_needed(GskRenderNode *node, const GdkRGBA *bg, int flags)
{
    if (node == NULL)
    {
	GdkRGBA prev_bg = *gui.bgcolor;
	GdkRGBA target_bg = *bg; // Must copy "bg", because it may be a pointer
				 // to "gui.bgcolor".
	bool	equal;

	if (flags & DRAW_TRANSP)
	    // No background color
	    return FALSE;

	gui_mch_set_bg_color(gui.back_pixel);
	equal = gdk_rgba_equal(&target_bg, gui.bgcolor);
	*gui.bgcolor = prev_bg;
	return !equal;
    }
    return TRUE;
}

/*
 * Create a new draw node and return it with reference count of 1. Note that
 * NULL may be returned. Note that ownership of "node" (which may be NULL) is
 * taken.
 */
    static DrawNode *
draw_node_new_node(
	int		row,
	int 		start_col,
	int 		n_cells,
	PangoFont	*font,
	GskRenderNode	*node,
	int		flags,
	const GdkRGBA	*bg_color,
	const GdkRGBA	*fg_color,
	const GdkRGBA	*sp_color)
{
    DrawNode *dnode;

    // If text node has no ink (NULL), and bg color is same as default bg color,
    // then return NULL, because there is nothing unique to show anyways.
    if (!node_is_needed(node, bg_color, flags))
    {
	node_unref(node);
	return NULL;
    }

    dnode = g_new0(DrawNode, 1);

    dnode->refcount = 1;
    dnode->text_node = node;

    dnode->bg_color = *bg_color;
    dnode->fg_color = *fg_color;
    dnode->sp_color = *sp_color;
    dnode->flags = flags;
    dnode->font = g_object_ref(font);

    dnode->row = row;
    dnode->start_col = start_col;
    dnode->n_cells = n_cells;

    draw_node_reset(dnode);

    return dnode;
}

/*
 * Same as draw_node_new_node() but creates the text node using the given
 * glyphs. May return NULL.
 */
    static DrawNode *
draw_node_new(
	int		    row,
	int 		    start_col,
	int 		    n_cells,
	PangoFont	    *font,
	PangoGlyphString    *string,
	int		    flags)
{
    GskRenderNode *text_node;

    text_node = gsk_text_node_new(font, string, gui.fgcolor,
	    &GRAPHENE_POINT_INIT(TEXT_X(start_col), TEXT_Y(row)));
    return draw_node_new_node(row, start_col, n_cells, font, text_node, flags,
	    gui.bgcolor, gui.fgcolor, gui.spcolor);
}

/*
 * Convert the given cell offset into an index in the "glyphs" array.
 */
    static int
cell_offset_to_glyph(const PangoGlyphInfo *glyphs, int n_glyphs, int cell_offset)
{
    int cells_seen = 0;

    for (int i = 0; i < n_glyphs; i++)
    {
	const PangoGlyphInfo *glyph = glyphs + i;

	if (cells_seen >= cell_offset)
	    return i;

	cells_seen += glyph->geometry.width / (gui.char_width * PANGO_SCALE);
    }
    return n_glyphs;
}

/*
 * Extract the left and right glyphs on the sides of the region between "start"
 * and "end" (inclusive) relative to the start column. 
 */
    static void
draw_node_extract(
	const DrawNode	*dnode,
	int		row,
	int		start,
	int		end,
	GskRenderNode	**out_left,
	GskRenderNode	**out_right)
{
    GskRenderNode	    *text_node = dnode->text_node;
    PangoFont		    *font = gsk_text_node_get_font(text_node);
    const PangoGlyphInfo    *glyphs;
    guint		    n_glyphs;
    const GdkRGBA	    *color;
    int			    dnode_end = dnode->start_col + dnode->n_cells - 1;
    PangoGlyphString	    *glyph_str = pango_glyph_string_new();

    glyphs = gsk_text_node_get_glyphs(text_node, &n_glyphs);
    color = gsk_text_node_get_color(text_node);

    if (out_left != NULL && start > 0)
    {
	// Extract left part from glyphs.
	int glyph_offset = cell_offset_to_glyph(glyphs, n_glyphs, start);

	// We don't need to set "log_clusters" array, since GskTextNode does not
	// use it.
	pango_glyph_string_set_size(glyph_str, glyph_offset);
	memcpy(glyph_str->glyphs, glyphs, glyph_offset * sizeof(PangoGlyphInfo));

	*out_left = gsk_text_node_new(font, glyph_str, color,
		&GRAPHENE_POINT_INIT(TEXT_X(dnode->start_col), TEXT_Y(row)));
    }
    else if (out_left != NULL)
	*out_left = NULL;

    if (out_right != NULL && dnode->start_col + end < dnode_end)
    {
	// Extract right part from glyphs
	int glyph_offset = cell_offset_to_glyph(glyphs, n_glyphs, end + 1);
	int n_right_glyphs = n_glyphs - glyph_offset;

	pango_glyph_string_set_size(glyph_str, n_right_glyphs);
        memcpy(glyph_str->glyphs, glyphs + glyph_offset,
		n_right_glyphs * sizeof(PangoGlyphInfo));

	*out_right = gsk_text_node_new(font, glyph_str, color,
		&GRAPHENE_POINT_INIT(TEXT_X(dnode->start_col + end + 1),
		    TEXT_Y(row)));
    }
    else if (out_right != NULL)
	*out_right = NULL;

    pango_glyph_string_free(glyph_str);
}

/*
 * Set the cell to the draw node (which may be NULL), adding a new reference.
 * Remove existing draw node if any.
 */
    static void
draw_cell_set(DrawCell *dcell, DrawNode *dnode)
{
    if (dcell->dnode != NULL)
	draw_node_unref(dcell->dnode);
    dcell->dnode = dnode == NULL ? NULL : draw_node_ref(dnode);
    dcell->inverted = FALSE;
}

/*
 * Set the cells between "col1" and "col2" (inclusive) for "row" to "dnode".
 */
    static void
draw_row_fill(DrawCell *drow, int col1, int col2, DrawNode *dnode)
{
    for (int c = col1; c <= col2; c++)
	draw_cell_set(drow + c, dnode);
}

/*
 * Similar to draw_row_fill(), but clip any nodes on the edges that overlap with
 * the region between "col1" and "col2".
 */
    static void
draw_row_set(DrawCell *drow, int row, int col1, int col2, DrawNode *dnode)
{
    DrawNode *ldnode = NULL, *rdnode = NULL;

    ldnode = drow[col1].dnode;
    rdnode = drow[col2].dnode;

    if (ldnode != NULL && ldnode == rdnode
	    && (ldnode->start_col < col1 || END_COL(rdnode) > col2))
    {
	// Set region is completely within a single draw node. Split the draw
	// node into left and right parts.
	GskRenderNode	*lnode = NULL;
	GskRenderNode	*rnode = NULL;
	DrawNode	*right;
	int		end_col = END_COL(ldnode);

	// If the text node is NULL, then still split, there may be under
	// decorations or a different bg color.
	if (ldnode->text_node != NULL)
	    draw_node_extract(ldnode, row, col1 - ldnode->start_col,
		    col2 - ldnode->start_col, &lnode, &rnode);

	if (node_is_needed(lnode, &ldnode->bg_color, ldnode->flags))
	{
	    node_unref(ldnode->text_node);
	    ldnode->text_node = lnode;
	    ldnode->n_cells = col1 - ldnode->start_col;
	    draw_node_reset(ldnode);
	}
	else
	{
	    // Draw node is not needed anymore, remove it from the row.
	    node_unref(lnode);
	    draw_row_fill(drow, ldnode->start_col, col1 - 1, NULL);
	}

        right = draw_node_new_node(row, col2 + 1,
		end_col - col2, ldnode->font, rnode, ldnode->flags,
		&ldnode->bg_color, &ldnode->fg_color, &ldnode->sp_color);

	draw_row_fill(drow, col2 + 1, end_col, right);
	if (right != NULL)
	    draw_node_unref(right);
    }

    // Check if left node (if any) overlaps with the set region. If so, then
    // split it and discard right halve.
    if (ldnode != NULL && ldnode->start_col != col1)
    {
	GskRenderNode *lnode = NULL;

	if (ldnode->text_node != NULL)
	    draw_node_extract(ldnode, row, col1 - ldnode->start_col,
		    END_COL(ldnode) - ldnode->start_col, &lnode, NULL);

	if (node_is_needed(lnode, &ldnode->bg_color, ldnode->flags))
	{
	    node_unref(ldnode->text_node);
	    ldnode->text_node = lnode;
	    ldnode->n_cells = col1 - ldnode->start_col;
	    draw_node_reset(ldnode);
	}
	else
	{
	    node_unref(lnode);
	    draw_row_fill(drow, ldnode->start_col, col1 - 1, NULL);
	}
    }

    // Check if right node (if any) overlaps with the set region. If so, then
    // split it and discard left halve.
    if (rdnode != NULL && END_COL(rdnode) > col2)
    {
	GskRenderNode *rnode = NULL;

	if (rdnode->text_node != NULL)
	    draw_node_extract(rdnode, row, 0, col2 - rdnode->start_col, NULL, &rnode);

	if (node_is_needed(rnode, &rdnode->bg_color, rdnode->flags))
	{
	    node_unref(rdnode->text_node);
	    rdnode->text_node = rnode;
	    rdnode->n_cells = END_COL(rdnode) - col2;
	    rdnode->start_col = col2 + 1;
	    draw_node_reset(rdnode);
	}
	else
	{
	    node_unref(rnode);
	    draw_row_fill(drow, col2 + 1, END_COL(rdnode), NULL);
	}
    }

    draw_row_fill(drow, col1, col2, dnode);
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
    DrawCell	*drow;
    DrawNode	*dnode;

    if (unlikely(self->cells == NULL
		|| row >= self->n_rows
		|| col >= self->n_cols
		|| col + num_cells > self->n_cols))
	return;

    drow = GET_ROW(self, row);

    dnode = draw_node_new(row, col, num_cells, font, glyphs, flags);
    draw_row_set(drow, row, col, col + num_cells - 1, dnode);
    if (dnode != NULL)
	draw_node_unref(dnode);
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
	draw_row_set(GET_ROW(self, r), r, col1, col2, NULL);
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
	DrawCell    *drow = GET_ROW(self, r);
	DrawNode    *last_dnode = NULL;

	for (int c = 0; c < self->n_cols; c++)
	{
	    DrawCell	*dcell = drow + c;
	    DrawNode	*dnode = dcell->dnode;

	    if (dnode == NULL)
		continue;

	    if (dnode != last_dnode)
	    {
		if (dnode->bg_node != NULL)
		    gtk_snapshot_append_node(snapshot, dnode->bg_node);
		if (dnode->text_node != NULL)
		    gtk_snapshot_append_node(snapshot, dnode->text_node);
		if (dnode->decor_node != NULL)
		    gtk_snapshot_append_node(snapshot, dnode->decor_node);
		last_dnode = dnode;
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
