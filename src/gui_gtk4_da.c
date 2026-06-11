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

#define DRAW_NODE_DIRTY 1   // Draw node is dirty
#define DRAW_NODE_NOBG 2    // Don't create background node
#define DRAW_NODE_NOINK 4   // Draw node has no ink
#define DRAW_NODE_UNDER 8   // Has under decorations (for convenience)
#define DRAW_NODE_CLIP 16   // Text node should be clipped to draw node bounds.

typedef struct
{
    int refcount;

    PangoGlyphInfo  *glyphs;
    int		    n_glyphs;
    char_u	    dnode_flags; // DRAW_NODE_* flags
    GskRenderNode   *node;  // This is either a text node, or a container node
			    // (if there is more than one node).

    PangoFont	*font;
    GdkRGBA	fg_color;
    GdkRGBA	bg_color;
    GdkRGBA	sp_color;
    int		flags;	    // DRAW_* flags

    int start_col;
    int n_cells;
} DrawNode;

#define END_COL(dn) ((dn)->start_col + (dn)->n_cells - 1)
#define HAS_INK(r) ((r)->width != 0 || (r)->height != 0)

/*
 * Each cell holds its own reference to a draw node if any. A draw node may span
 * multiple cells, which represents how many cells it takes up on screen.
 */
typedef struct
{
    DrawNode	*dnode;	// May be NULL
    gboolean	invert; // If this cell is inverted
} DrawCell;

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

    DrawCell	*cells; // May be NULL, always check!
    int         n_rows;
    int		n_cols;

    int		bleed_right;

    // Used for hollow and part style cursors. For the block cursor, that is
    // simply rendered as a cell using vim_draw_area_add_glyphs(). May be NULL.
    GskRenderNode *cursor_node;

#if defined(FEAT_SIGN_ICONS)
    // Queue of sign icon render nodes. Icons at the end of the queue are drawn
    // ontop of earlier ones.
    GQueue *signs;
#endif

#ifdef FEAT_NETBEANS_INTG
    // Cairo render node for multi sign indicator for Netbeans. May be NULL
    GskRenderNode *multisign_node;
#endif

#ifdef FEAT_IMAGE_GDK
    // Queue of DrawImage structs. Sorted in ascending order of zindex, so that
    // images with a higher zindex are rendered over ones with lower zindex.
    GQueue *images;
#endif
};

#define GET_ROW(da, n) ((da)->cells + (da)->n_cols * (n))

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
#ifdef FEAT_SIGN_ICONS
    // vim_draw_area_clear_block() should have removed all the sign icons
    assert(g_queue_is_empty(self->signs));
    g_queue_free(self->signs);
#endif
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

    gtk_widget_class_set_layout_manager_type(widget_class, GTK_TYPE_BIN_LAYOUT);
}

    static void
vim_draw_area_init(VimDrawArea *self)
{
#ifdef FEAT_SIGN_ICONS
    self->signs = g_queue_new();
#endif
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

    self->n_rows = rows;
    self->n_cols = cols;
    self->cells = g_realloc_n(self->cells, rows * cols, sizeof(DrawCell));
    memset(self->cells, 0, rows * (sizeof(DrawCell) * cols));
}

    static void
node_unref(GskRenderNode *node)
{
    if (node != NULL)
	gsk_render_node_unref(node);
}

/*
 * Return TRUE if "glyphs" take up space (not entirely whitespace).
 */
    static gboolean
glyphs_has_ink(PangoFont *font, const PangoGlyphInfo *glyphs, int n_glyphs)
{
    for (int i = 0; i < n_glyphs; i++)
    {
	PangoRectangle glyph_ink;

	pango_font_get_glyph_extents (font, glyphs[i].glyph, &glyph_ink, NULL);

	if (HAS_INK(&glyph_ink))
	    return TRUE;
    }
    return FALSE;
}

/*
 * Realloc "glyphs" to "n_glyphs" and return the new reallocated pointer.
 */
    static PangoGlyphInfo *
glyphs_resize(PangoGlyphInfo *glyphs, int n_glyphs)
{
    return g_realloc_n(glyphs, n_glyphs, sizeof(PangoGlyphInfo));
}

/*
 * Return TRUE if "bg" is the same as the default background color.
 */
    static gboolean
color_is_default_bg(const GdkRGBA *bg)
{
    guicolor_T bgc = ((guicolor_T)(bg->red * 255) << 16)
	| ((guicolor_T)(bg->green * 255) << 8)
	|  (guicolor_T)(bg->blue * 255);
    return bgc == gui.back_pixel;
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
 * Create a new draw node with a reference count of 1. Note that this may be
 * NULL if creating a new draw node is not necessary.
 */
    static DrawNode *
draw_node_new(
	PangoFont		*font,
	const PangoGlyphInfo	*glyphs,
	int			n_glyphs,
	const GdkRGBA		*bg_color,
	const GdkRGBA		*fg_color,
	const GdkRGBA		*sp_color,
	int			flags,
	int			start_col,
	int			n_cells)
{
    DrawNode	*dnode;
    gboolean	has_ink = glyphs_has_ink(font, glyphs, n_glyphs);
    gboolean	is_def_bg = color_is_default_bg(bg_color);
    gboolean	has_under = flags & (DRAW_UNDERL | DRAW_UNDERC | DRAW_STRIKE);

    // If there is no ink to be displayed, and the background color is the same
    // as the default background color (the color that will be displayed behind
    // everything), then there is no point in creating a new draw node.
    if (!has_ink && !has_under && (flags & DRAW_TRANSP || is_def_bg))
	return NULL;

    dnode = g_new0(DrawNode, 1);

    dnode->refcount = 1;

    dnode->glyphs = g_memdup2(glyphs, sizeof(PangoGlyphInfo) * n_glyphs);
    dnode->n_glyphs = n_glyphs;
    dnode->dnode_flags |= DRAW_NODE_DIRTY;
    if (is_def_bg || flags & DRAW_TRANSP)
	dnode->dnode_flags |= DRAW_NODE_NOBG;
    if (!has_ink)
	dnode->dnode_flags |= DRAW_NODE_NOINK;
    if (has_under)
	dnode->dnode_flags |= DRAW_NODE_UNDER;

    dnode->font = g_object_ref(font);
    dnode->bg_color = *bg_color;
    dnode->fg_color = *fg_color;
    dnode->sp_color = *sp_color;
    dnode->flags = flags;

    dnode->start_col = start_col;
    dnode->n_cells = n_cells;

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
    if (dnode != NULL && --dnode->refcount <= 0)
    {
	g_free(dnode->glyphs);
	node_unref(dnode->node);
	g_object_unref(dnode->font);
	g_free(dnode);
    }
}

/*
 * Dirty the draw node. This will remove the render node if any, and mark it to
 * have a new render node created for it on the next snapshot vfunc call.
 * Returns TRUE if draw node is not necessary anymore.
 */
    static gboolean
draw_node_make_dirty(DrawNode *dnode)
{
    int flags = dnode->dnode_flags;

    g_clear_pointer(&dnode->node, gsk_render_node_unref);
    dnode->dnode_flags |= DRAW_NODE_DIRTY;

    return (flags & DRAW_NODE_NOINK) && !(flags & (DRAW_NODE_UNDER))
	&& flags & DRAW_NODE_NOBG;
}

    static DrawNode *
draw_node_copy(DrawNode *dnode)
{
    DrawNode *copy = draw_node_new(
	    dnode->font, dnode->glyphs, dnode->n_glyphs,
	    &dnode->bg_color, &dnode->fg_color, &dnode->sp_color,
	    dnode->flags, dnode->start_col, dnode->n_cells
	    );

    // "copy" should never be NULL, so we don't need to check for NULL.
    if (unlikely(dnode->dnode_flags & DRAW_NODE_CLIP))
	copy->dnode_flags |= DRAW_NODE_CLIP;

    return copy;
}

/*
 * Split the draw node at the given cell offset in place (exclusive). If
 * "keep_left" is TRUE, then keep the left halve (discard right halve), and vice
 * versa. This will dirty the draw node.
 *
 * Returns TRUE if the new split draw node is not necessary anymore (see
 * draw_node_new()), otherwise FALSE.
 */
    static gboolean
draw_node_split(DrawNode *dnode, int cell_offset, gboolean keep_left)
{
    int		glyph_offset;
    gboolean	split = TRUE;
    gboolean	clip = FALSE;

    glyph_offset = cell_offset_to_glyph(dnode->glyphs,
	    dnode->n_glyphs, cell_offset);

    // Some fonts emulate ligatures by having spacer glyphs followed by a glyph
    // that contains all the ink. If we tried splitting this type of ligature,
    // then one side will incorrectly be empty.
    //
    // To handle this case, always clip the draw node so that the extra ink does
    // not bleed out. If we are keeping the left side, then do not split,
    // because we want to keep the glyph with all the ink. If we are keeping the
    // right side, then we can split because the glyph with the ink will be on
    // the right side always anyways.
    for (int i = glyph_offset; i < dnode->n_glyphs; i++)
    {
	PangoRectangle ink;

	pango_font_get_glyph_extents(dnode->font, dnode->glyphs[i].glyph,
		&ink, NULL);

	if (HAS_INK(&ink))
	{
	    if (ink.x < 0)
	    {
		split = !keep_left;
		clip = TRUE;
	    }
	    break;
	}
    }

    if (unlikely(clip))
	dnode->dnode_flags |= DRAW_NODE_CLIP;

    if (keep_left)
    {
	if (likely(split))
	    dnode->n_glyphs = glyph_offset;
	dnode->n_cells = cell_offset;
    }
    else
    {
	if (likely(split))
	{
	    // If this results in zero, then glyphs_has_ink() will return FALSE
	    // so it is fine.
	    dnode->n_glyphs -= glyph_offset;
	    // Shift glyphs after offset to beginning
	    memmove(dnode->glyphs, dnode->glyphs + glyph_offset,
		    sizeof(PangoGlyphInfo) * dnode->n_glyphs);
	}

	dnode->n_cells -= cell_offset;
	dnode->start_col += cell_offset;
    }

    if (likely(split))
    {
	dnode->glyphs = glyphs_resize(dnode->glyphs, dnode->n_glyphs);

	// Recheck if new split glyphs has ink
	if (glyphs_has_ink(dnode->font, dnode->glyphs, dnode->n_glyphs))
	    dnode->dnode_flags &= ~DRAW_NODE_NOINK;
	else
	    dnode->dnode_flags |= DRAW_NODE_NOINK;
    }

    return draw_node_make_dirty(dnode);
}

/*
 * If "dnode" is dirty, create a new render node for it at the given row and
 * store it, then undirty it.
 */
    static void
draw_node_render(DrawNode *dnode, int row, VimDrawArea *da)
{
    GskRenderNode	*nodes[3];
    int			n_nodes = 0;
    GskRenderNode	*decor_node;

    if (!(dnode->dnode_flags & DRAW_NODE_DIRTY))
	return;

    if (!(dnode->dnode_flags & DRAW_NODE_NOBG))
    {
	int width = dnode->n_cells * gui.char_width;

	// If this draw node touches the end of the draw area. Bleed its
	// background to the right if the space the draw area covers is slightly
	// bigger than its actual visible area (that all cells cover). This just
	// makes things like status bars look a bit nicer
	//
	// Don't do this for the bottom, because that will make the cursor in
	// the cmdline look weird. Instead only bleed downwards when drawing the
	// global background color (see vim_draw_area_snapshot())
	if (END_COL(dnode) == da->n_cols - 1)
	    width += gui.bleed_right;

	nodes[n_nodes++] = gsk_color_node_new(&dnode->bg_color,
		&GRAPHENE_RECT_INIT(FILL_X(dnode->start_col), FILL_Y(row),
		    width, gui.char_height));
    }

    if (!(dnode->dnode_flags & DRAW_NODE_NOINK))
    {
	GskRenderNode	    *text_node;
	PangoGlyphString    glyphs_str;

	// gsk_text_node_new() only uses the "glyphs" field, don't need to worry
	// about the "log_clusters" array.
	glyphs_str.glyphs = dnode->glyphs;
	glyphs_str.num_glyphs = dnode->n_glyphs;
	text_node = gsk_text_node_new(dnode->font, &glyphs_str, &dnode->fg_color,
		&GRAPHENE_POINT_INIT(TEXT_X(dnode->start_col), TEXT_Y(row)));
	// Should never be NULL since we check beforehand if there is ink.
	assert(text_node != NULL);

	if (dnode->dnode_flags & DRAW_NODE_CLIP)
	{
	    GskRenderNode *old = text_node;

	    text_node = gsk_clip_node_new(text_node,
		    &GRAPHENE_RECT_INIT(FILL_X(dnode->start_col), FILL_Y(row),
			dnode->n_cells * gui.char_width, gui.char_height));
	    gsk_render_node_unref(old);
	    assert(text_node != NULL);
	}

	nodes[n_nodes++] = text_node;
    }

    decor_node = create_under_decor_node(row, dnode->start_col, dnode->n_cells,
	    dnode->flags, &dnode->fg_color, &dnode->sp_color);
    if (decor_node != NULL)
	nodes[n_nodes++] = decor_node;

    // Should never be zero
    assert(n_nodes > 0);

    if (likely(n_nodes == 1))
	dnode->node = nodes[0];
    else
    {
	dnode->node = gsk_container_node_new(nodes, n_nodes);
	// gsk_container_node_new() takes its own reference
	for (int i = 0; i < n_nodes; i++)
	    gsk_render_node_unref(nodes[i]);
    }

    dnode->dnode_flags &= ~DRAW_NODE_DIRTY;
}

/*
 * Returns true if "dnode" matches "font" + "flags" in terms of
 * color/visual attributes.
 */
    static gboolean
draw_node_match(DrawNode *dnode, PangoFont *font, int flags)
{
    if (dnode->flags != flags)
	return FALSE;

    if (!(flags & DRAW_TRANSP)
	    && !gdk_rgba_equal(&dnode->bg_color, gui.bgcolor))
	return FALSE;

    if (!gdk_rgba_equal(&dnode->fg_color, gui.fgcolor))
	return FALSE;

    // Special color is only used for undercurls
    if (flags & DRAW_UNDERC && !gdk_rgba_equal(&dnode->sp_color, gui.spcolor))
	return FALSE;

    // This may not work all the time, but creating two PangoFontDescription
    // each time to compare equality seems slow...
    return dnode->font == font;
}

/*
 * Append or prepend the given glyphs to the draw node. If "start" is TRUE, then
 * prepend, otherwise append. This will invalidate the draw node. Note that
 * prepending does not update "start_col" or "n_cells".
 */
    static void
draw_node_extend(
	DrawNode		*dnode,
	const PangoGlyphInfo	*glyphs,
	int			n_glyphs,
	bool			start)
{
    dnode->glyphs = glyphs_resize(dnode->glyphs, dnode->n_glyphs + n_glyphs);

    if (start)
    {
	// Move the existing glyphs forward first
	memmove(dnode->glyphs + n_glyphs, dnode->glyphs,
		dnode->n_glyphs * sizeof(PangoGlyphInfo));
	memcpy(dnode->glyphs, glyphs, n_glyphs * sizeof(PangoGlyphInfo));
    }
    else
	memcpy(dnode->glyphs + dnode->n_glyphs, glyphs,
		n_glyphs * sizeof(PangoGlyphInfo));

    dnode->n_glyphs += n_glyphs;

    if (glyphs_has_ink(dnode->font, dnode->glyphs, dnode->n_glyphs))
	dnode->dnode_flags &= ~DRAW_NODE_NOINK;
    else
	dnode->dnode_flags |= DRAW_NODE_NOINK;
    (void)draw_node_make_dirty(dnode);
}

/*
 * Set the given cell to the draw node (which may be NULL), adding a new
 * reference to it.
 */
    static void
draw_cell_set(DrawCell *dcell, DrawNode *dnode)
{
    draw_node_unref(dcell->dnode);
    dcell->dnode = dnode == NULL ? NULL : draw_node_ref(dnode);
    dcell->invert = FALSE;
}

/*
 * Set the cells between "col1" and "col2" (inclusive) to "dnode" (which may be
 * NULL).
 */
    static void
draw_row_fill(DrawCell *drow, int col1, int col2, DrawNode *dnode)
{
    for (int c = col1; c <= col2; c++)
	draw_cell_set(drow + c, dnode);
}

/*
 * Same as draw_row_fill(), but also handle truncating/splitting any draw nodes
 * that overlap onto the set region. If "split" is TRUE, then only
 * truncating/splitting is done.
 *
 * If "copy" is TRUE, then "dnode" is ignored and instead any draw nodes in the
 * region that overlap outside of it are copied and clipped in addition to
 * truncating draw nodes outside the region.
 */
    static void
draw_row_set(
	DrawCell    *drow,
	int	    col1,
	int	    col2,
	DrawNode    *dnode,
	gboolean    copy,
	gboolean    split)
{
    DrawNode	*ldnode = drow[col1].dnode;
    DrawNode	*rdnode = drow[col2].dnode;
    DrawNode	*new_dnode = NULL;

    if (ldnode != NULL && ldnode == rdnode
	    && (ldnode->start_col != col1 || END_COL(ldnode) > col2))
    {
	// Region in completely inside a single draw node. Truncate the existing
	// draw node, and create a new draw node to be used as the right split.
	if (END_COL(ldnode) > col2)
	{
	    rdnode = draw_node_copy(ldnode);
	    draw_row_fill(drow, col2 + 1, END_COL(rdnode), rdnode);
	    draw_node_unref(rdnode);
	}
	else
	    // "ldnode" does not extend past "col2", no point in creating a new
	    // draw node on the right.
	    rdnode = NULL;

	if (copy)
	    // Make another copy for the new draw node inside the set region.
	    // Must fill it in the row after, since "ldnode" may be unreferenced
	    // fully.
	    new_dnode = draw_node_copy(ldnode);
    }

    if (ldnode != NULL && ldnode->start_col != col1)
    {
	if (copy && new_dnode == NULL)
	{
	    // Make a copy for the right halve.
	    DrawNode *new_right = draw_node_copy(ldnode);

	    if (draw_node_split(new_right,  col1 - ldnode->start_col, FALSE))
		g_clear_pointer(&new_right, draw_node_unref);
	    draw_row_fill(drow, col1, END_COL(ldnode), new_right);
	    draw_node_unref(new_right);
	}

	// Leftmost draw node overlaps onto region, split it and discard right
	// halve.
	if (draw_node_split(ldnode, col1 - ldnode->start_col, TRUE))
	    // Draw node is not necessary anymore, clear it from the row.
	    draw_row_fill(drow, ldnode->start_col, col1 - 1, NULL);
    }
    if (rdnode != NULL && END_COL(rdnode) > col2)
    {
	if (copy && new_dnode == NULL)
	{
	    // Make a copy for the left halve.
	    DrawNode *new_left = draw_node_copy(rdnode);

	    if (draw_node_split(new_left,  col2 - rdnode->start_col + 1, TRUE))
		g_clear_pointer(&new_left, draw_node_unref);
	    draw_row_fill(drow, rdnode->start_col, col2, new_left);
	    draw_node_unref(new_left);
	}

	// Rightmost draw node overlaps onto region, split it and discard left
	// halve.
	if (draw_node_split(rdnode, col2 - rdnode->start_col + 1, FALSE))
	    draw_row_fill(drow, col2 + 1, END_COL(rdnode), NULL);
    }

    if (copy)
    {
	if (new_dnode != NULL)
	{
	    if (draw_node_split(new_dnode, col1 - new_dnode->start_col, FALSE)
		    || draw_node_split(new_dnode,
			col2 - new_dnode->start_col + 1, TRUE))
		g_clear_pointer(&new_dnode, draw_node_unref);

	    draw_row_fill(drow, col1, col2, new_dnode);
	    draw_node_unref(new_dnode);
	}
    }
    else if (!split)
	draw_row_fill(drow, col1, col2, dnode);
}

/*
 * Move the cells between "col1" and "col2" from "src" to "dest", overwriting
 * the existing cells. This will handle clipping any draw nodes.
 */
    static void
draw_row_move_to(DrawCell *dest_row, DrawCell *src_row, int col1, int col2)
{
    int move_size = (col2 - col1 + 1) * sizeof(DrawCell);

    // Make sure that we free/truncate any draw nodes before we overwrite
    // them.
    draw_row_set(dest_row, col1, col2, NULL, FALSE, FALSE);

    // Make sure that draw nodes at the "col1" and "col2" of "src_row" are
    // clipped so that they all fit in the region being moved.
    draw_row_set(src_row, col1, col2, NULL, TRUE, FALSE);

    memmove(dest_row + col1, src_row + col1, move_size);

    // Dirty the moved cells
    for (int c = col1; c <= col2;)
	if (dest_row[c].dnode != NULL)
	{
	    (void)draw_node_make_dirty(dest_row[c].dnode);
	    c += dest_row[c].dnode->n_cells;
	}
	else
	    c++;

    // NULL the draw nodes so we don't double unreference.
    memset(src_row + col1, 0, (col2 - col1 + 1) * sizeof(DrawCell));
}

/*
 * Should be called after modifying draw nodes within the given region.
 */
static void
vim_draw_area_check_bounds(
	VimDrawArea *self,
	int	    row1,
	int	    row2,
	int	    col1,
	int	    col2)
{
#if defined(FEAT_SIGN_ICONS) || defined(FEAT_NETBEANS_INTG)
    graphene_rect_t bounds = GRAPHENE_RECT_INIT(
	    FILL_X(col1), FILL_Y(row1),
	    gui.char_width * (col2 - col1 + 1),
	    gui.char_height * (row2 - row1 + 1));
#endif

    if (self->cursor_node != NULL)
	// Check if cursor node is within the the updated region. If so, then
	// remove the render node. This only applies to the part and hollow
	// cursor, the block cursor will be cleared in draw_row_make_space().
	if (gui.row >= row1 && gui.row <= row2
		&& gui.col >= col1 && gui.col <= col2)
	    g_clear_pointer(&self->cursor_node, gsk_render_node_unref);

#ifdef FEAT_SIGN_ICONS
    // Clear any sign icons within the modified block if any
    for (GList *s = self->signs->head; s != NULL;)
    {
	GList		*next = s->next;
	graphene_rect_t rect;

	gsk_render_node_get_bounds(s->data, &rect);

	if (graphene_rect_contains_rect(&bounds, &rect))
	{
	    // Keep going in case there are multiple sign icons within this
	    // block.
	    gsk_render_node_unref(s->data);
	    g_queue_delete_link(self->signs, s);
	}
	s = next;
    }
#endif
#ifdef FEAT_NETBEANS_INTG
    // Remove multi sign indicator if it is within the modified region.
    if (self->multisign_node != NULL)
    {
	graphene_rect_t rect;

	gsk_render_node_get_bounds(self->multisign_node, &rect);
	if (graphene_rect_contains_rect(&bounds, &rect))
	    g_clear_pointer(&self->multisign_node, gsk_render_node_unref);
    }
#endif
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
    DrawNode	*dnode = NULL;
    int		end_col = col + num_cells - 1;

    if (unlikely(self->cells == NULL
		|| row >= self->n_rows
		|| col >= self->n_cols
		|| col + num_cells > self->n_cols))
	return;

    drow = GET_ROW(self, row);

    draw_row_set(drow, col, end_col, NULL, FALSE, TRUE);

    // Check if leftmost draw node (if any) has the same visual
    // attributes/colours as the glyph string being added. If so, then just
    // extend that draw node with the new glyphs.
    if (col > 0)
    {
	DrawNode *ldnode = drow[col - 1].dnode;

	// Don't want to try merging draw nodes that are clipped, because the
	// glyphs in them may not match one to one with the actual bounds of the
	// draw node.
	if (ldnode != NULL && !(ldnode->dnode_flags & DRAW_NODE_CLIP)
		&& draw_node_match(ldnode, font, flags))
	{
	    draw_node_extend(ldnode, glyphs->glyphs, glyphs->num_glyphs, FALSE);
	    draw_row_fill(drow, col, end_col, ldnode);
	    ldnode->n_cells += num_cells;
	    dnode = ldnode;
	}
    }

    // Check if we can use the existing draw node on the right. If so, then shift
    // "rdnode" to the "col", and extend it. If we merged the left draw node, then
    // instead extend it normally and unreference the right draw node.
    if (col + num_cells < self->n_cols)
    {
	DrawNode *rdnode = drow[col + num_cells].dnode;

	if (rdnode != NULL && !(rdnode->dnode_flags & DRAW_NODE_CLIP)
		&& draw_node_match(rdnode, font, flags))
	{
	    if (dnode != NULL)
	    {
		assert(rdnode->start_col == col + num_cells);
		draw_node_extend(dnode, rdnode->glyphs, rdnode->n_glyphs, FALSE);
		dnode->n_cells += rdnode->n_cells;
		draw_row_fill(drow, rdnode->start_col, END_COL(rdnode), dnode);
	    }
	    else
	    {
		draw_node_extend(rdnode, glyphs->glyphs, glyphs->num_glyphs, TRUE);
		draw_row_fill(drow, col, end_col, rdnode);
		rdnode->start_col = col;
		rdnode->n_cells += num_cells;
		dnode = rdnode;
	    }
	}
    }

    if (dnode != NULL)
	return;

    dnode = draw_node_new(
	    font, glyphs->glyphs, glyphs->num_glyphs, gui.bgcolor,
	    gui.fgcolor, gui.spcolor, flags, col, num_cells
	    );
    draw_row_fill(drow, col, end_col, dnode);
    draw_node_unref(dnode);

    vim_draw_area_check_bounds(self, row, row, col, col + num_cells - 1);
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
	draw_row_set(GET_ROW(self, r), col1, col2, NULL, FALSE, FALSE);

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

    int		    offset = row2 - row1;
#if defined(FEAT_SIGN_ICONS) || defined(FEAT_NETBEANS_INTG)
    graphene_rect_t bounds = GRAPHENE_RECT_INIT(
	    FILL_X(col1), FILL_Y(row1),
	    gui.char_width * (col2 - col1 + 1),
	    gui.char_height * (row2 - row1 + 1));
    graphene_rect_t clear_rect;
#endif

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
		    col1, col2);
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
			col1, col2);
    }

    // Do not call vim_draw_area_check_bounds(), because we moved cells, not
    // modified them.

#if defined(FEAT_SIGN_ICONS) || defined(FEAT_NETBEANS_INTG)
    if (row1 > to)
	clear_rect = GRAPHENE_RECT_INIT(
		FILL_X(col1), FILL_Y(to),
		gui.char_width * (col2 - col1 + 1),
		gui.char_height * (row1 - to));
    else
	clear_rect = GRAPHENE_RECT_INIT(
		FILL_X(col1), FILL_Y(row2 + 1),
		gui.char_width * (col2 - col1 + 1),
		gui.char_height * (to - row1));
#endif

#ifdef FEAT_SIGN_ICONS
    // Move sign icons if they are in the moved region
    for (GList *s = self->signs->head; s != NULL;)
    {
	GList           *next = s->next;
	GskRenderNode   *node = s->data;
	graphene_rect_t  rect;

	gsk_render_node_get_bounds(node, &rect);

	// Check if icon moved off screen, if so then remove it.
	if (graphene_rect_contains_rect(&clear_rect, &rect))
	{
	    gsk_render_node_unref(s->data);
	    g_queue_delete_link(self->signs, s);
	    s = next;
	    continue;
	}

	if (graphene_rect_contains_rect(&bounds, &rect))
	{
	    GdkTexture    *texture;
	    GskRenderNode *new;
	    float          new_y;

	    texture = gsk_texture_scale_node_get_texture(node);
	    new_y = graphene_rect_get_y(&rect) - graphene_rect_get_y(&bounds);
	    new_y += FILL_Y(to);

	    if (new_y >= 0 && new_y < gtk_widget_get_height(GTK_WIDGET(self)))
	    {
		rect.origin.y = new_y;
		new = gsk_texture_scale_node_new(texture, &rect,
			GSK_SCALING_FILTER_TRILINEAR);
		gsk_render_node_unref(node);
		s->data = new;
	    }
	    else
	    {
		gsk_render_node_unref(s->data);
		g_queue_delete_link(self->signs, s);
	    }
	}
	s = next;
    }
#endif
#ifdef FEAT_NETBEANS_INTG
    // Move multisign indicator node if needed
    if (self->multisign_node != NULL)
    {
	graphene_rect_t rect;

	gsk_render_node_get_bounds(self->multisign_node, &rect);

	if (graphene_rect_contains_rect(&clear_rect, &rect))
	    g_clear_pointer(&self->multisign_node, gsk_render_node_unref);
	else if (graphene_rect_contains_rect(&bounds, &rect))
	{
	    float new_y =
		graphene_rect_get_y(&rect) - graphene_rect_get_y(&bounds);

	    new_y += FILL_Y(to);

	    if (new_y >= 0 && new_y < gtk_widget_get_height(GTK_WIDGET(self)))
	    {
		cairo_surface_t *surface;
		GskRenderNode   *new;
		cairo_t         *cr;

		surface = gsk_cairo_node_get_surface(self->multisign_node);
		rect.origin.y = new_y;
		new = gsk_cairo_node_new(&rect);
		cr = gsk_cairo_node_get_draw_context(new);
		cairo_set_source_surface(cr, surface, 0, 0);
		cairo_paint(cr);
		cairo_destroy(cr);

		gsk_render_node_unref(self->multisign_node);
		self->multisign_node = new;
	    }
	    else
		g_clear_pointer(&self->multisign_node, gsk_render_node_unref);
	}
    }
#endif
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

    node_unref(self->cursor_node);
    self->cursor_node = gsk_border_node_new(&outline, border, color);
}

/*
 * Draw a part cursor with width "w" and height "h". Note that this does not
 * queue a redraw
 */
    void
vim_draw_area_set_part_cursor(VimDrawArea *self, int w, int h)
{
    node_unref(self->cursor_node);
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

	    dcell->invert = !dcell->invert;
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
	GdkTexture *sign,
	int row,
	int col,
	int width,
	int height)
{
    GskRenderNode   *node;

    if (unlikely(self->cells == NULL
		|| row >= self->n_rows
		|| col >= self->n_cols))
	return;

    node = gsk_texture_scale_node_new(sign,
	    &GRAPHENE_RECT_INIT(FILL_X(col), FILL_Y(row), width, height),
	    GSK_SCALING_FILTER_TRILINEAR);
    if (node == NULL)
	return;
    g_queue_push_tail(self->signs, node);
}
#endif

#ifdef FEAT_NETBEANS_INTG
    cairo_t *
vim_draw_area_get_multisign_cairo(VimDrawArea *self, int x, int y, int w, int h)
{
    node_unref(self->multisign_node);
    self->multisign_node = gsk_cairo_node_new(
	    &GRAPHENE_RECT_INIT( x, y, w, h));
    return gsk_cairo_node_get_draw_context(self->multisign_node);
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

    static void
draw_image_free(DrawImage *dimg)
{
    gsk_render_node_unref(dimg->node);
    g_free(dimg);
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
    if (self->bleed_right != gui.bleed_right)
    {
	self->bleed_right = gui.bleed_right;
	for (int r = 0; r < self->n_rows; r++)
	{
	    DrawCell *dcell = &GET_ROW(self, r)[self->n_cols - 1];

	    if (dcell->dnode != NULL)
	    {
		(void)draw_node_make_dirty(dcell->dnode);
		draw_node_render(dcell->dnode, r, self);
	    }
	}
    }

    // For inverted cells, we first build an array of bounds that represent
    // blocks of inverted cells. Then we apply a white color to each of those
    // bounds and then finish the blend.
    gtk_snapshot_push_blend(snapshot, GSK_BLEND_MODE_DIFFERENCE);
    ga_init2(&invert_ga, sizeof(graphene_rect_t), 8);

    gtk_snapshot_append_color(snapshot, gui.bgcolor,
	    &GRAPHENE_RECT_INIT(0, 0, width, height));

    for (int r = 0; r < self->n_rows; r++)
    {
	DrawCell    *drow = GET_ROW(self, r);
	int	    inv_len = 0;
	int	    inv_start;

	for (int c = 0; c < self->n_cols; c++)
	{
	    DrawCell	*dcell = drow + c;
	    DrawNode	*dnode = dcell->dnode;

	    // Batch inverted cells as single row rectangles.
	    if (dcell->invert)
	    {
		if (inv_len == 0)
		    inv_start = c;
		inv_len++;
	    }
	    else if (!dcell->invert && inv_len > 0)
	    {
		flush_invert_ga(&invert_ga, r, inv_start, inv_len);
		inv_len = 0;
	    }

	    if (dnode == NULL)
		continue;

	    if (dnode->start_col == c)
	    {
		draw_node_render(dnode, r, self);
		assert(dnode->node != NULL);
		gtk_snapshot_append_node(snapshot, dnode->node);
	    }
	}
	// Flush trailing inverted blocks at end of row loop
	if (inv_len > 0)
	    flush_invert_ga(&invert_ga, r, inv_start, inv_len);
    }

#ifdef FEAT_SIGN_ICONS
    // Order of where the sign icon should be placed shouldn't matter,
    // since caller will add whitespace padding in the region it covers.
    // Probably should put it behind cursor though.
    for (GList *s = self->signs->head; s != NULL; s = s->next)
	gtk_snapshot_append_node(snapshot, s->data);
#endif

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
