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

#define DRAW_ATTR_DIRTY 1 // Recreate render node on next snapshot vfunc

/*
 * Represents the visual attributes of a span of cells
 */
typedef struct
{
    int		refcount;
    GdkRGBA	bg_color;
    GdkRGBA	fg_color;
    GdkRGBA	sp_color;
    int		draw_flags; // DRAW_* flags
} DrawAttr;

/*
 * Represents a single "character"
 */
typedef struct
{
    int	    refcount;

    PangoFont	*font;
    GArray	*glyphs;
} DrawGlyphs;

/*
 * Always in contiguous chunks, never separated.
 */
typedef struct
{
    int		    refcount;
    GskRenderNode   *node; // May be NULL if nothing to display
    int		    start_col;
    int		    n_cells;
} DrawNode;

#define END_COL(dn) ((dn)->start_col + (dn)->n_cells - 1)
#define DRAW_NODE_DIRTY(dn) g_clear_pointer(&(dn), draw_node_unref)
// Convert cell count into pango units
#define CELLS2PANGO(c) ((c) * gui.char_width * PANGO_SCALE)

#define REMOVE_NODE(dn, dr) \
    do { \
	if ((dn) != NULL) \
	{ \
	    int start_col_ = (dn)->start_col; \
	    int end_col_ = END_COL((dn)); \
	    for (int c_ = start_col_; c_ <= end_col_; c_++) \
	    DRAW_NODE_DIRTY((dr)[c_].dnode); \
	} \
    } while (FALSE)

#if defined(FEAT_NETBEANS_INTG) || defined(FEAT_SIGN_ICONS)
/*
 * Either represents a sign icon or a netbean multi sign indicator.
 */
typedef struct
{
    int		    refcount;
    GdkTexture	    *texture;
    int		    width;
    int		    height;
    GskRenderNode   *node; // Cached or NULL if dirty.
} DrawSign;
#endif

#define DRAW_CELL_INVERT 1

typedef struct
{
    DrawNode	*dnode;
    DrawGlyphs	*dglyphs; // If "dattr" is set, then this will always be set.
    DrawAttr	*dattr;
#if defined(FEAT_NETBEANS_INTG) || defined(FEAT_SIGN_ICONS)
    DrawSign	*dsign;
#endif
    char_u	flags; // DRAW_CELL_* flags
} DrawCell;

#define GET_ROW(da, n) ((da)->cells + (da)->n_cols * (n))

#ifdef FEAT_IMAGE_GDK
/*
 * Struct containing information about an image. This is designed to map well
 * with how Vim handles the kitty graphics protocol.
 */
typedef struct
{
    int id;
    int zindex;
    GskRenderNode *node; // Cached clip node, which has the texture node as its
			 // child. May be NULL
} DrawImage;
#endif

struct _VimDrawArea
{
    GtkWidget parent;

    DrawCell	*cells;
    int         n_rows;
    int		n_cols;

    int		bleed_right;

    // Used for hollow and part style cursors. For the block cursor, that is
    // simply rendered as a cell using vim_draw_area_add_glyphs(). May be NULL.
    GskRenderNode *cursor_node;

#ifdef FEAT_IMAGE_GDK
    // Queue of DrawImage structs. Sorted in ascending order of zindex, so that
    // images with a higher zindex are rendered over ones with lower zindex.
    GQueue *images;
#endif
};

G_DEFINE_TYPE(VimDrawArea, vim_draw_area, GTK_TYPE_WIDGET)

#ifdef FEAT_IMAGE_GDK
    static void draw_image_free(DrawImage *dimg);
#endif
    static void vim_draw_area_snapshot(GtkWidget *widget, GtkSnapshot *snapshot);

    static void
vim_draw_area_finalize(GObject *obj)
{
    VimDrawArea *self = VIM_DRAW_AREA(obj);

    // "multisign_node" and "cursor_node" will be freed in
    // vim_draw_area_clear_block().
    vim_draw_area_clear(self);

    g_free(self->cells);

#ifdef FEAT_IMAGE_GDK
    g_queue_free_full(self->images, (GDestroyNotify)draw_image_free);
#endif

    G_OBJECT_CLASS(vim_draw_area_parent_class)->finalize(obj);
}

    static void
vim_draw_area_class_init(VimDrawAreaClass *class)
{
    GtkWidgetClass  *widget_class = GTK_WIDGET_CLASS(class);
    GObjectClass    *obj_class = G_OBJECT_CLASS(class);

    widget_class->snapshot = vim_draw_area_snapshot;

    obj_class->finalize = vim_draw_area_finalize;

    // Add a layout manager so it can handle child popovers
    gtk_widget_class_set_layout_manager_type(widget_class, GTK_TYPE_BIN_LAYOUT);
}

    static void
vim_draw_area_init(VimDrawArea *self)
{
    self->bleed_right = -1;
#ifdef FEAT_IMAGE_GDK
    self->images = g_queue_new();
#endif
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

    self->cells = g_realloc_n(self->cells, cols * rows, sizeof(DrawCell));
    memset(self->cells, 0, sizeof(DrawCell) * cols * rows);

    self->n_rows = rows;
    self->n_cols = cols;
}

/*
 * Return TRUE if "rgba" is the same as "color".
 */
    static gboolean
color_rgba_equal(GdkRGBA rgba, guicolor_T color)
{
    guicolor_T rgbac = ((guicolor_T)(rgba.red * 255) << 16)
	| ((guicolor_T)(rgba.green * 255) << 8)
	|  (guicolor_T)(rgba.blue * 255);
    return rgbac == color;
}

/*
 * Create a new under decoration node with the given flags. Returns NULL if no
 * under decorations are needed.
 */
    static GskRenderNode *
create_under_decor_node(
	int		row,
	int		start_col,
	int		n_cells,
	int		flags,
	const GdkRGBA	*fg_color,
	const GdkRGBA   *sp_color)
{
    GskRenderNode   *nodes[3];
    int		    n_nodes = 0;
    GskRenderNode   *container;

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

	// GskPath was added in GSK 4.14, otherwise use cairo
#if GTK_CHECK_VERSION(4, 14, 0)
	GskPathBuilder	*builder;
	GskPath		*path;
	GskStroke	*stroke;
	GskRenderNode	*color_node;
	graphene_rect_t bounds;

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

	gsk_path_get_stroke_bounds (path, stroke, &bounds);
	color_node = gsk_color_node_new(sp_color, &bounds);

	nodes[n_nodes++] = gsk_stroke_node_new(color_node, path, stroke);
	gsk_stroke_free(stroke);
	gsk_path_unref(path);
	gsk_render_node_unref(color_node);
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
    }

    if (n_nodes == 0)
	return NULL;
    if (n_nodes == 1)
	return nodes[0];

    container = gsk_container_node_new(nodes, n_nodes);
    for (int i = 0; i < n_nodes; i++)
	// Container node takes its own reference to each.
	gsk_render_node_unref(nodes[i]);
    return container;
}

/*
 * Return TRUE if "glyphs" has ink (not whitespace).
 */
    static gboolean
glyphs_has_ink(PangoFont *font, const PangoGlyphInfo *glyphs, int n_glyphs)
{
    for (int i = 0; i < n_glyphs; i++)
    {
	PangoRectangle ink;

	pango_font_get_glyph_extents (font, glyphs[i].glyph, &ink, NULL);

	if (ink.width > 0 && ink.height > 0)
	    return TRUE;
    }
    return FALSE;
}

/*
 * Create a new draw attr that is initially dirty.
 */
    static DrawAttr *
draw_attr_new(
	const GdkRGBA	*bg_color,
	const GdkRGBA	*fg_color,
	const GdkRGBA	*sp_color,
	int		draw_flags)
{
    DrawAttr *dattr = g_new0(DrawAttr, 1);

    dattr->refcount = 1;

    dattr->bg_color = *bg_color;
    dattr->fg_color = *fg_color;
    dattr->sp_color = *sp_color;
    dattr->draw_flags = draw_flags;

    return dattr;
}

    static DrawAttr *
draw_attr_ref(DrawAttr *dattr)
{
    dattr->refcount++;
    return dattr;
}

    static void
draw_attr_unref(DrawAttr *dattr)
{
    if (--dattr->refcount <= 0)
	g_free(dattr);
}

/*
 * Check if "dattr" has the same visual attributes
 */
    static gboolean
draw_attr_match(DrawAttr *a, int draw_flags)
{
    if (a == NULL)
	return FALSE;
    if (a->draw_flags != draw_flags)
	return FALSE;
    if (!gdk_rgba_equal(&a->bg_color, gui.bgcolor)
	    || !gdk_rgba_equal(&a->fg_color, gui.fgcolor)
	    || !gdk_rgba_equal(&a->sp_color, gui.spcolor))
	return FALSE;
    return TRUE;
}

/*
 * Return TRUE if both draw attrs are visually the same
 */
    static gboolean
draw_attr_equal(DrawAttr *a, DrawAttr *b)
{
    if (a->draw_flags != b->draw_flags)
	return FALSE;
    if (!gdk_rgba_equal(&a->bg_color, &b->bg_color)
	    || !gdk_rgba_equal(&a->fg_color, &b->fg_color)
	    || !gdk_rgba_equal(&a->sp_color, &b->sp_color))
	return FALSE;
    return TRUE;
}

/*
 * Return TRUE if "dattr" can be combined with a blank cell with a draw attr.
 */
    static gboolean
draw_attr_equal_blank(DrawAttr *dattr, DrawAttr *blank)
{
    if (!gdk_rgba_equal(&dattr->bg_color, &blank->bg_color))
	return FALSE;
    if ((dattr->draw_flags & (DRAW_UNDERC | DRAW_UNDERL | DRAW_STRIKE)) !=
	    (blank->draw_flags & (DRAW_UNDERC | DRAW_UNDERL | DRAW_STRIKE)))
	return FALSE;
    return TRUE;
}

/*
 * Return TRUE if "dattr" can be combined with a blank cell without a draw attr.
 */
    static gboolean
draw_attr_blank_compatible(DrawAttr *dattr)
{
    if (dattr == NULL)
	return TRUE;
    if (!color_rgba_equal(dattr->bg_color, gui.back_pixel))
	return FALSE;
    if (dattr->draw_flags & (DRAW_UNDERC | DRAW_UNDERL | DRAW_STRIKE))
	return FALSE;
    return TRUE;
}

    static DrawGlyphs *
draw_glyphs_new(PangoFont *font)
{
    DrawGlyphs *dglyphs = g_new0(DrawGlyphs, 1);

    dglyphs->refcount = 1;
    dglyphs->font = g_object_ref(font);
    dglyphs->glyphs = g_array_new(FALSE, TRUE, sizeof(PangoGlyphInfo));

    return dglyphs;
}

    static DrawGlyphs *
draw_glyphs_ref(DrawGlyphs *dglyphs)
{
    dglyphs->refcount++;
    return dglyphs;
}

    static void
draw_glyphs_unref(DrawGlyphs *dglyphs)
{
    if (--dglyphs->refcount <= 0)
    {
	g_object_unref(dglyphs->font);
	g_array_free(dglyphs->glyphs, TRUE);
	g_free(dglyphs);
    }
}

    static void
draw_glyphs_add(DrawGlyphs *dglyphs, PangoGlyphInfo glyph)
{
    g_array_append_val(dglyphs->glyphs, glyph);
}

/*
 * Create a new draw node at the given position,
 */
    static DrawNode *
draw_node_new(int row, int start_col)
{
    DrawNode *dnode = g_new0(DrawNode, 1);

    dnode->refcount = 1;
    dnode->start_col = start_col;
    dnode->n_cells = 1;

    return dnode;
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
	if (dnode->node != NULL)
	    gsk_render_node_unref(dnode->node);
	g_free(dnode);
    }
}

    static void
draw_node_add_cell(DrawNode *dnode)
{
    dnode->n_cells++;
}

/*
 * Render the draw node and create the render node. If the glyphs array is
 * empty, then "font" will not be used. Note that render node may be NULL if
 * there is no ink/nothing to display.
 */
    static void
draw_node_render(
	DrawNode    *dnode,
	int	    row,
	GArray	    *glyphs,
	DrawAttr    *dattr,
	PangoFont   *font,
	VimDrawArea *da)
{
    PangoGlyphString	gstr;
    GskRenderNode	*node;
    GskRenderNode	*nodes[4];
    int			n_nodes = 0;

    // Don't render background color if its the same as the global color,
    // reduces number of render nodes.
    if (!color_rgba_equal(dattr->bg_color, gui.back_pixel))
    {
	int width = dnode->n_cells * gui.char_width;

	// If this draw node touches the end of the draw area. Bleed its
	// background to the right if the space the draw area covers is slightly
	// bigger than its actual visible area (that all cells cover). This just
	// makes things like status bars look a bit nicer when draw area size is
	// not an exact multiple of the cell size.
	//
	// Don't do this for the bottom, because that will make the cursor in
	// the cmdline look weird. Instead only bleed downwards when drawing the
	// global background color (see vim_draw_area_snapshot())
	if (END_COL(dnode) + 1 == da->n_cols)
	    width += gui.bleed_right;

	nodes[n_nodes++] = gsk_color_node_new(&dattr->bg_color,
		&GRAPHENE_RECT_INIT(FILL_X(dnode->start_col), FILL_Y(row),
		    width, gui.char_height));
    }

    if (glyphs->len > 0)
    {
	// GskTextNode does not use the "log_clusters" array in
	// PangoGlyphString, so we don't need to set it (or store it).
	gstr.glyphs = (PangoGlyphInfo *)glyphs->data;
	gstr.num_glyphs = glyphs->len;

	node = gsk_text_node_new(font, &gstr, &dattr->fg_color,
		&GRAPHENE_POINT_INIT(TEXT_X(dnode->start_col), TEXT_Y(row)));

	if (node != NULL)
	    nodes[n_nodes++] = node;

	// Emulate bold by shifting text by one pixel
	if (node != NULL && (dattr->draw_flags & DRAW_BOLD)
		&& !gui.font_can_bold)
	{
	    node = gsk_text_node_new(font, &gstr, &dattr->fg_color,
		    &GRAPHENE_POINT_INIT(TEXT_X(dnode->start_col) + 1,
			TEXT_Y(row)));
	    nodes[n_nodes++] = node;
	}
    }

    node = create_under_decor_node(row, dnode->start_col, dnode->n_cells,
	    dattr->draw_flags, &dattr->fg_color, &dattr->sp_color);
    if (node != NULL)
	nodes[n_nodes++] = node;


    if (n_nodes == 1)
	dnode->node = nodes[0];
    else if (n_nodes > 1)
    {
	dnode->node = gsk_container_node_new(nodes, n_nodes);
	// gsk_container_node_new() takes its own reference
	for (int i = 0; i < n_nodes; i++)
	    gsk_render_node_unref(nodes[i]);
    }
}

#if defined(FEAT_NETBEANS_INTG) || defined(FEAT_SIGN_ICONS)
/*
 * Create a new draw sign icon with the given texture.
 */
    static DrawSign *
draw_sign_new(GdkTexture *texture, int width, int height)
{
    DrawSign *dsign = g_new0(DrawSign, 1);

    dsign->refcount = 1;
    dsign->texture = g_object_ref(texture);
    dsign->width = width;
    dsign->height = height;

    return dsign;
}

    static DrawSign *
draw_sign_ref(DrawSign *dsign)
{
    dsign->refcount++;
    return dsign;
}

    static void
draw_sign_unref(DrawSign *dsign)
{
    if (--dsign->refcount <= 0)
    {
	if (dsign->node != NULL)
	    gsk_render_node_unref(dsign->node);
	g_object_unref(dsign->texture);
	g_free(dsign);
    }
}

/*
 * Make the draw sign icon dirty, so that it is rerendered at the correct
 * position on the next snapshot vfunc.
 */
    static void
draw_sign_dirty(DrawSign *dsign)
{
    g_clear_pointer(&dsign->node, gsk_render_node_unref);
}

/*
 * Render the draw sign at the given position if it is dirty.
 */
    static void
draw_sign_render(DrawSign *dsign, int start_col, int row)
{
    if (dsign->node != NULL)
	return;

    dsign->node = gsk_texture_scale_node_new(dsign->texture,
	    &GRAPHENE_RECT_INIT(FILL_X(start_col), FILL_Y(row),
		dsign->width, dsign->height),
	    GSK_SCALING_FILTER_TRILINEAR);
}
#endif

/*
 * Set the cell to the draw node and glyphs (that represents the cell). Make
 * sure to dirty the overall changed region in the row.
 */
    static void
draw_cell_set(DrawCell *dcell, DrawGlyphs *dglyphs, DrawAttr *dattr)
{
    if (dcell->dattr != NULL)
	draw_attr_unref(dcell->dattr);
    dcell->dattr = dattr == NULL ? NULL : draw_attr_ref(dattr);

    if (dcell->dglyphs != dglyphs)
    {
	if (dcell->dglyphs != NULL)
	    draw_glyphs_unref(dcell->dglyphs);
	dcell->dglyphs = dglyphs == NULL ? NULL : draw_glyphs_ref(dglyphs);
    }

    g_clear_pointer(&dcell->dsign, draw_sign_unref);
    dcell->flags &= ~DRAW_CELL_INVERT;
}

#ifdef FEAT_IMAGE_GDK
    static void
draw_image_free(DrawImage *dimg)
{
    gsk_render_node_unref(dimg->node);
    g_free(dimg);
}
#endif

/*
 * Fill the cells between "col1" and "col2" (inclusive) with "dattr" and
 * "dglyphs".
 */
    static void
draw_row_fill(
	DrawCell    *drow,
	int	    col1,
	int	    col2,
	DrawGlyphs  *dglyphs,
	DrawAttr    *dattr)
{
    for (int c = col1; c <= col2; c++)
    {
	// If a draw node overlaps onto the filled region, must remove all
	// instances of it, so that the region it covers is rerendered again.
	REMOVE_NODE(drow[c].dnode, drow);
	draw_cell_set(drow + c, dglyphs, dattr);
    }
}

/*
 * Clear all draw nodes that intercept the given region.
 */
    static void
draw_row_dirty(DrawCell *drow, int col1, int col2)
{
    for (int c = col1; c <= col2; c++)
    {
	// If a draw node overlaps onto the filled region, must remove all
	// instances of it, so that the region it covers is rerendered again.
	REMOVE_NODE(drow[c].dnode, drow);
	if (drow[c].dsign != NULL)
	    draw_sign_dirty(drow[c].dsign);
    }
}

#if defined(FEAT_NETBEANS_INTG) || defined(FEAT_SIGN_ICONS)
/*
 * Set "dsign" to cells between "col1" and "col2" (inclusive).
 */
    static void
draw_row_fill_sign(DrawCell *drow, int col1, int col2, DrawSign *dsign)
{
    for (int c = col1; c <= col2; c++)
    {
	if (drow[c].dsign != NULL)
	    draw_sign_unref(drow[c].dsign);
	drow[c].dsign = draw_sign_ref(dsign);
    }
}
#endif

/*
 * Replace all instances of "target" starting at "col" with "src".
 */
    static void
draw_row_replace_attr(
	DrawCell    *drow,
	int	    col,
	DrawAttr    *target,
	DrawAttr    *src,
	VimDrawArea *da)
{
    while (col < da->n_cols && drow[col].dattr == target)
    {
	REMOVE_NODE(drow[col].dnode, drow);
	draw_cell_set(drow + col, drow[col].dglyphs, src);
	col++;
    }
}

/*
 * Move the cells between "col1" and "col2" from "src" to "dest", overwriting
 * the existing cells.
 */
    static void
draw_row_move_to(
	DrawCell    *dest_row,
	DrawCell    *src_row,
	int	    col1,
	int	    col2,
	VimDrawArea *da)
{
    int move_size = (col2 - col1 + 1) * sizeof(DrawCell);

    draw_row_fill(dest_row, col1, col2, NULL, NULL);
    memmove(dest_row + col1, src_row + col1, move_size);
    draw_row_dirty(dest_row, col1, col2);

    // NULL the draw cells so we don't double unreference.
    memset(src_row + col1, 0, (col2 - col1 + 1) * sizeof(DrawCell));
}

/*
 * Should be called after modifying draw nodes within the given region.
 * Shouldn't be called if cells are moved however.
 */
    static void
vim_draw_area_check_bounds(
	VimDrawArea *self,
	int	    row1,
	int	    row2,
	int	    col1,
	int	    col2)
{
    if (self->cursor_node != NULL)
	// Check if cursor node is within the the updated region. If so, then
	// remove the render node. This only applies to the part and hollow
	// cursor, the block cursor will be cleared in draw_row_make_space().
	if (gui.row >= row1 && gui.row <= row2
		&& gui.col >= col1 && gui.col <= col2)
	    g_clear_pointer(&self->cursor_node, gsk_render_node_unref);
}

/*
 * Add the glyphs to the given position on the draw area. Returns number of
 * display cells used.
 */
    int
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
    DrawAttr	*dattr;
    DrawGlyphs	*dglyphs = NULL;
    gboolean	need_parent = FALSE;
    int		c = col;
    int		cells_used = 0;

    if (unlikely(self->cells == NULL
		|| row >= self->n_rows
		|| col >= self->n_cols
		|| col + num_cells > self->n_cols))
	return 0;

    drow = GET_ROW(self, row);

    // If draw node at the left of the region has same visual attributes, then
    // expand it instead of creating a new one.
    if (col > 0 && draw_attr_match(drow[col - 1].dattr, flags))
	dattr = draw_attr_ref(drow[col - 1].dattr);
    else
	dattr = draw_attr_new(gui.bgcolor, gui.fgcolor, gui.spcolor, flags);

    // If draw node at right of the region has same visual attributes, then
    // replace it with the draw node on its left.
    if (col + num_cells < self->n_cols
	    && draw_attr_match(drow[col + num_cells].dattr, flags))
	draw_row_replace_attr(drow, col + num_cells,
		drow[col + num_cells].dattr, dattr, self);

    // If there is no ink, then just set the "dglyphs" of each cell to NULL.
    if (!glyphs_has_ink(font, glyphs->glyphs, glyphs->num_glyphs))
    {
	draw_row_fill(drow, col, col + num_cells - 1, NULL, dattr);
	cells_used += num_cells;
	goto exit;
    }

    // Put glyphs into groups where each group represents a single continuous
    // "character" (e.g. decomposed and composing characters).
    for (int i = 0; i < glyphs->num_glyphs && c < self->n_cols; i++)
    {
	PangoGlyphInfo	glyph = glyphs->glyphs[i];
	int		cellcount;

	cellcount = glyph.geometry.width / (gui.char_width * PANGO_SCALE);

	if (cellcount > 0)
	{
	    if (!need_parent)
		dglyphs = draw_glyphs_new(font);

	    draw_glyphs_add(dglyphs, glyph);

	    draw_row_fill(drow, c, c + cellcount - 1, dglyphs, dattr);
	    draw_glyphs_unref(dglyphs);
	    c += cellcount;
	    cells_used += cellcount;
	    need_parent = FALSE;
	    dglyphs = NULL;
	}
	else
	{
	    if (dglyphs == NULL)
	    {
		// Zero width character before a parent/decomposed character,
		// allocate a new array and indicate that it should be used when
		// we reach a decomposed character instead of creating a new
		// one.
		dglyphs = draw_glyphs_new(font);
		need_parent = TRUE;
	    }

            draw_glyphs_add(dglyphs, glyph);
        }
    }

    if (need_parent)
    {
	// Zero width characters that do not have a parent, just put them in
	// their own cell.
	draw_row_fill(drow, c, c, dglyphs, dattr);
	cells_used++;
	draw_glyphs_unref(dglyphs);
    }

exit:
    draw_attr_unref(dattr);

    vim_draw_area_check_bounds(self, row, row, col, col + num_cells - 1);
    return cells_used;
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
	draw_row_fill(GET_ROW(self, r), col1, col2, NULL, NULL);

    vim_draw_area_check_bounds(self, row1, row2, col1, col2);
}

/*
 * Clear out the entire draw area
 */
    void
vim_draw_area_clear(VimDrawArea *self)
{
    vim_draw_area_clear_block(self, 0, 0, self->n_rows - 1, self->n_cols - 1);
}

/*
 * Move the given rows between "row1" and "row2", within the column "col1" and
 * "col2" (making a rectangle region), to the row "to". The previous region that
 * was moved is cleared.
 */
    void
vim_draw_area_move_block(
	VimDrawArea *self,
	int	    to,
	int	    row1,
	int	    row2,
	int	    col1,
	int	    col2)
{
    int offset = row2 - row1;

    if (unlikely(self->cells == NULL
		|| row1 >= self->n_rows
		|| row2 >= self->n_rows
		|| to >= self->n_rows
		|| col1 >= self->n_cols
		|| col2 >= self->n_cols))
	return;

    assert(row2 >= row1);
    assert(col2 >= col1);
    assert(row1 != to);

    if (row1 > to)
    {
	// "row1" is below "to", start moving rows starting at "row1". Rows are
	// being shifted upwards.
	for (int o = 0; o <= offset; o++)
	    draw_row_move_to(GET_ROW(self, to + o), GET_ROW(self, row1 + o),
		    col1, col2, self);
    }
    else
    {
	// "row1" is above "to", must start moving rows starting at "row2". Rows
	// are being shifted downwards.
	for (int o = offset; o >= 0; o--)
	    if (to + o >= self->n_rows)
		// "src_row" is being "moved" off the screen, no need to move
		// it physically.
		gui_clear_block(row1 + o, col1, row1 + o, col2);
	    else
		draw_row_move_to(GET_ROW(self, to + o), GET_ROW(self, row1 + o),
			col1, col2, self);
    }
}

/*
 * Draw a hollow cursor at the cursor position using the current foreground
 * color. Note that this does not queue a redraw
 */
    void
vim_draw_area_set_hollow_cursor(VimDrawArea *self)
{
    GskRoundedRect	outline;
    int			i = 1;
    static const float	border[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    const GdkRGBA color[4] = {
	*gui.fgcolor, *gui.fgcolor,
	*gui.fgcolor, *gui.fgcolor
    } ;

    // Double cursor width if double width character
    if (mb_lefthalve(gui.row, gui.col))
	i = 2;

    gsk_rounded_rect_init_from_rect(&outline,
	    &GRAPHENE_RECT_INIT(FILL_X(gui.col), FILL_Y(gui.row),
		i * gui.char_width, gui.char_height),
	    0.0f);

    if (self->cursor_node != NULL)
	gsk_render_node_unref(self->cursor_node);
    self->cursor_node = gsk_border_node_new(&outline, border, color);
}

/*
 * Draw a part cursor with width "w" and height "h". Note that this does not
 * queue a redraw
 */
    void
vim_draw_area_set_part_cursor(VimDrawArea *self, int w, int h)
{
    if (self->cursor_node != NULL)
	gsk_render_node_unref(self->cursor_node);
    self->cursor_node = gsk_color_node_new(gui.fgcolor,
	    &GRAPHENE_RECT_INIT(
#ifdef FEAT_RIGHTLEFT
		CURSOR_BAR_RIGHT ? FILL_X(gui.col + 1) - w :
#endif
		FILL_X(gui.col), FILL_Y(gui.row) + gui.char_height - h,
		w, h));
}

/*
 * Invert the rectangle in the draw area.
 */
    void
vim_draw_area_invert_block(
	VimDrawArea	*self,
	int		row,
	int		col,
	int		nrows,
	int		ncols)
{
    if (unlikely(self->cells == NULL
		|| row >= self->n_rows
		|| col >= self->n_cols
		|| row + nrows - 1 >= self->n_rows
		|| col + ncols - 1 >= self->n_cols))
	return;

    for (int r = row; r < row + nrows; r++)
    {
	DrawCell *drow = GET_ROW(self, r);

	for (int c = col; c < col + ncols; c++)
	{
	    DrawCell *dcell = drow + c;

	    dcell->flags ^= DRAW_CELL_INVERT;
	}
    }
}

#if defined(FEAT_SIGN_ICONS)
/*
 * Add a sign texture at the given row and column, and scale it to "width" and
 * "height".
 */
    void
vim_draw_area_add_sign(
	VimDrawArea *self,
	GdkTexture  *sign,
	int	    row,
	int	    col,
	int	    width,
	int	    height)
{
    DrawCell	*drow;
    DrawSign	*dsign;
    int		cells;

    if (unlikely(self->cells == NULL
		|| row >= self->n_rows
		|| col >= self->n_cols))
	return;

    drow = GET_ROW(self, row);
    dsign = draw_sign_new(sign, width, height);
    cells = width / gui.char_width;

    draw_row_fill_sign(drow, col, col + cells - 1, dsign);
    draw_sign_unref(dsign);
}
#endif

#ifdef FEAT_IMAGE_GDK
/*
 * Get the draw image with the given id, return NULL if not exists.
 */
    static GList *
vim_draw_area_get_image(VimDrawArea *self, int id)
{
    for (GList *s = self->images->head; s != NULL; s = s->next)
    {
	DrawImage *sdimg = s->data;

	if (sdimg->id == id)
	    return s;
    }
    return NULL;
}

/*
 * Queue the given image to the correct position in the queue using its zindex.
 */
    static void
vim_draw_area_queue_image(VimDrawArea *self, GList *link)
{
    DrawImage *dimg = link->data;

    for (GList *s = self->images->head; s != NULL; s = s->next)
    {
	DrawImage *sdimg = s->data;

	if (sdimg->zindex >= dimg->zindex)
	{
	    g_queue_insert_before_link(self->images, s, link);
	    return;
	}
    }
    // Queue is empty or image has new highest zindex
    g_queue_push_tail_link(self->images, link);
}

/*
 * Add an image at the given row and column with the specified zindex and id.
 * (src_x, src_y, draw_w, draw_h) describe which pixel sub-rect of the source
 * texture should be drawn. If there is an image that has the same id, then it
 * is re-rendered with the new texture. If zindex of an image changed, then the
 * queue will be updated accordingly.
 */
    void
vim_draw_area_add_image(
	VimDrawArea *self,
	GdkTexture  *image,
	int	    row,
	int	    col,
	int	    src_x,
	int	    src_y,
	int	    draw_w,
	int	    draw_h,
	int	    zindex,
	int	    id)
{
    GskRenderNode   *node, *old;
    int		    w, h;
    graphene_rect_t clip;
    GList	    *link;
    DrawImage	    *dimg;

    if (unlikely(self->cells == NULL
		|| row >= self->n_rows
		|| col >= self->n_cols))
	return;

    w = gdk_texture_get_width(image);
    h = gdk_texture_get_height(image);

    node = gsk_texture_node_new(image,
	    &GRAPHENE_RECT_INIT(FILL_X(col) - src_x, FILL_Y(row) - src_y,
		w, h));

    if (node != NULL)
    {
	graphene_rect_init(&clip, FILL_X(col), FILL_Y(row), draw_w, draw_h);

	old = node;
	node = gsk_clip_node_new(node, &clip);
	gsk_render_node_unref(old);
    }

    link = vim_draw_area_get_image(self, id);
    if (link == NULL)
    {
	dimg = g_new(DrawImage, 1);

	dimg->id = id;
	dimg->zindex = zindex;
	dimg->node = node;

	link = g_list_alloc();
	link->data = dimg;
    }
    else
    {
	dimg = link->data;

	gsk_render_node_unref(dimg->node);
	dimg->node = node;

	if (dimg->zindex == zindex)
	    return;
	else
	{
	    dimg->zindex = zindex;
	    g_queue_unlink(self->images, link);
	}
    }

    vim_draw_area_queue_image(self, link);
}

/*
 * Remove the image with the given id if it exists
 */
    void
vim_draw_area_remove_image(VimDrawArea *self, int id)
{
    GList *link = vim_draw_area_get_image(self, id);

    if (link == NULL)
	return;

    draw_image_free(link->data);
    g_queue_delete_link(self->images, link);
}
#endif

#ifdef FEAT_NETBEANS_INTG
/*
 * Add a multi sign indicator at the given position. "surf" should be in ARGB
 * format.
 */
    void
vim_draw_area_add_multisign(
	VimDrawArea	*self,
	cairo_surface_t *surf,
	int		row,
	int		col,
	int		width,
	int		height)
{
    DrawCell	*drow;
    DrawSign	*dsign;
    int		cells;
    GdkTexture	*texture;
    GBytes	*bytes;

    if (unlikely(self->cells == NULL
		|| row >= self->n_rows
		|| col >= self->n_cols))
	return;

    cairo_surface_flush(surf);
    bytes = g_bytes_new_with_free_func(cairo_image_surface_get_data(surf),
	    cairo_image_surface_get_height(surf)
	    * cairo_image_surface_get_stride(surf),
	    (GDestroyNotify)cairo_surface_destroy,
	    cairo_surface_reference(surf));

    texture = gdk_memory_texture_new(cairo_image_surface_get_width(surf),
	    cairo_image_surface_get_height(surf),
	    GDK_MEMORY_A8R8G8B8,
	    bytes, cairo_image_surface_get_stride(surf));

    drow = GET_ROW(self, row);
    dsign = draw_sign_new(texture, width, height);
    g_object_unref(texture);
    cells = width / gui.char_width;

    draw_row_fill_sign(drow, col, col + cells - 1, dsign);
    draw_sign_unref(dsign);
}
#endif

    static void
flush_invert_ga(garray_T *invert_ga, int row, int start, int len)
{
    if (ga_grow(invert_ga, 1) == OK)
    {
	graphene_rect_t *arr = (graphene_rect_t *)invert_ga->ga_data;

	graphene_rect_init(arr + invert_ga->ga_len++,
		FILL_X(start), FILL_Y(row),
		len * gui.char_width, gui.char_height);
    }
}

    static void
vim_draw_area_snapshot(GtkWidget *widget, GtkSnapshot *snapshot)
{
    VimDrawArea		    *self = VIM_DRAW_AREA(widget);
    int			    height, width;
    GArray		    *glyphs_buf;
    static const GdkRGBA    white = {1, 1, 1, 1};
    garray_T		    invert_ga;

    gui_mch_set_bg_color(gui.back_pixel);
    height = gtk_widget_get_height(widget) + gui.bleed_bot;
    width = gtk_widget_get_width(widget) + gui.bleed_right;

    if (self->cells == NULL)
    {
	gtk_snapshot_append_color(snapshot, gui.bgcolor,
		&GRAPHENE_RECT_INIT(0, 0, width, height));
	return;
    }

    // If number of pixels to bleed has changed, then dirty the nodes at the
    // right edge of the draw area.
    if (self->bleed_right == -1 || self->bleed_right != gui.bleed_right)
    {
	self->bleed_right = gui.bleed_right;
	for (int r = 0; r < self->n_rows; r++)
	{
	    DrawCell	*drow = GET_ROW(self, r);
	    DrawCell	*dcell = &drow[self->n_cols - 1];

	    REMOVE_NODE(dcell->dnode, drow);
	}
    }

    // For inverted cells, we first build an array of bounds that represent
    // blocks of inverted cells. Then we apply a white color to each of those
    // bounds and then finish the blend.
    gtk_snapshot_push_blend(snapshot, GSK_BLEND_MODE_DIFFERENCE);
    ga_init2(&invert_ga, sizeof(graphene_rect_t), 8);

    gtk_snapshot_append_color(snapshot, gui.bgcolor,
	    &GRAPHENE_RECT_INIT(0, 0, width, height));

    // When creating text nodes, batch individual cells that have the same
    // DrawNode object.
    glyphs_buf = g_array_new(FALSE, FALSE, sizeof(PangoGlyphInfo));

    for (int r = 0; r < self->n_rows; r++)
    {
	DrawCell    *drow = GET_ROW(self, r);
	int	    inv_len = 0;
	int	    inv_start;
	DrawNode    *cur_dnode = NULL;
	PangoFont   *cur_font = NULL;
	DrawAttr    *cur_dattr = NULL;
	int	    empty_cells = 0;

	for (int c = 0; c < self->n_cols; c++)
	{
	    DrawCell	*dcell = drow + c;
	    DrawNode	*dnode = dcell->dnode;
	    DrawAttr	*dattr = dcell->dattr;
	    DrawGlyphs	*dglyphs = dcell->dglyphs;

	    // Batch inverted cells as single row rectangles.
	    if (dcell->flags & DRAW_CELL_INVERT)
	    {
		if (inv_len == 0)
		    inv_start = c;
		inv_len++;
	    }
	    else if (!(dcell->flags & DRAW_CELL_INVERT) && inv_len > 0)
	    {
		flush_invert_ga(&invert_ga, r, inv_start, inv_len);
		inv_len = 0;
	    }

#define FLUSH_NODE() \
	    do { \
		if (cur_dnode != NULL) \
		{ \
		    draw_node_render(cur_dnode, r, glyphs_buf, \
			    cur_dattr, cur_font, self); \
		    if (cur_dnode->node != NULL) \
		    gtk_snapshot_append_node(snapshot, cur_dnode->node); \
		    cur_dnode = NULL; \
		    cur_font = NULL; \
		} \
		g_array_set_size(glyphs_buf, 0); \
		cur_dattr = NULL; \
		empty_cells = 0; \
	    } while (FALSE)

	    if (dnode != NULL)
	    {
		FLUSH_NODE();

		if (dnode->node != NULL
			&& (c == 0 || drow[c - 1].dnode != dnode))
		    gtk_snapshot_append_node(snapshot, dnode->node);
	    }
	    else if (dattr != NULL)
	    {
		// Continue using the previous node if:
		// - If there is ink, draw attrs are exactly the same
		// - If there is no ink, bg and decor are the same
		// and font must be the same (unless there is no ink).
		if (cur_dattr != NULL
			&& (cur_dattr == dattr
			    || draw_attr_equal(cur_dattr, dattr)
			    || (dglyphs == NULL
				&& draw_attr_equal_blank(cur_dattr, dattr)))
			&& (dglyphs == NULL || cur_font == dglyphs->font))
		{
		    if (cur_font == NULL && dglyphs != NULL)
			cur_font = dglyphs->font;
		    draw_node_add_cell(cur_dnode);
		    dcell->dnode = draw_node_ref(cur_dnode);
		}
		else
		{
		    // Flush previous draw node if any
		    FLUSH_NODE();

		    dcell->dnode = cur_dnode = draw_node_new(r, c);
		    cur_dattr = dattr;
		    cur_font = dglyphs == NULL ? NULL : dglyphs->font;
		}

		// Don't want to render double width characters twice.
		if (dglyphs != NULL
			&& (c == 0 || drow[c - 1].dglyphs != dglyphs))
		{
		    int prev_len = glyphs_buf->len;

		    g_array_append_vals(glyphs_buf, dglyphs->glyphs->data,
			    dglyphs->glyphs->len);

		    // Inject the accumulated space width into the glyphs that
		    // have ink.
		    if (empty_cells > 0)
		    {
			if (prev_len > 0)
			{
			    // Spaces occurred between valid text: extend
			    // previous character's advance
			    PangoGlyphInfo *last = &g_array_index(
				    glyphs_buf, PangoGlyphInfo,
				    prev_len - 1);
			    last->geometry.width += CELLS2PANGO(empty_cells);
			}
			else
			{
			    // Spaces occurred at the beginning of the DrawNode:
			    // push the new text right
			    PangoGlyphInfo *first = &g_array_index(
				    glyphs_buf, PangoGlyphInfo, prev_len);
			    first->geometry.x_offset +=
				CELLS2PANGO(empty_cells);
			}
			empty_cells = 0; // Width consumed
		    }
		}
		else if (dglyphs == NULL)
		    empty_cells++;
	    }
	    else if (cur_dnode != NULL
		    && draw_attr_blank_compatible(cur_dattr))
	    {
		draw_node_add_cell(cur_dnode);
		dcell->dnode = draw_node_ref(cur_dnode);
		empty_cells++;
	    }
	    else
		FLUSH_NODE();

#if defined(FEAT_NETBEANS_INTG) || defined(FEAT_SIGN_ICONS)
	    // Add sign icon if any, make sure to draw this on top of the text.
	    // If the sign icon spans multiple cells, make sure to only add it
	    // once.
	    if (dcell->dsign != NULL
		    && (c == 0 || (dcell - 1)->dsign != dcell->dsign))
	    {
		draw_sign_render(dcell->dsign, c, r);
		gtk_snapshot_append_node(snapshot, dcell->dsign->node);
	    }
#endif
	}

	FLUSH_NODE();

	// Flush trailing inverted blocks at end of row loop
	if (inv_len > 0)
	    flush_invert_ga(&invert_ga, r, inv_start, inv_len);
#undef FLUSH_NODE
    }

    g_array_free(glyphs_buf, TRUE);

    if (self->cursor_node != NULL)
	gtk_snapshot_append_node(snapshot, self->cursor_node);

    gtk_snapshot_pop(snapshot);
    for (int i = 0; i < invert_ga.ga_len; i++)
    {
	graphene_rect_t *rect = &((graphene_rect_t *)invert_ga.ga_data)[i];
	gtk_snapshot_append_color(snapshot, &white, rect);
    }
    gtk_snapshot_pop(snapshot);
    ga_clear(&invert_ga);

#ifdef FEAT_IMAGE_GDK
    // Draw images after any possible inversions
    for (GList *s = self->images->head; s != NULL; s = s->next)
    {
	DrawImage *dimg = s->data;

	if (dimg->node != NULL)
	    gtk_snapshot_append_node(snapshot, dimg->node);
    }
#endif
}

#endif // USE_GTK4_SNAPSHOT
