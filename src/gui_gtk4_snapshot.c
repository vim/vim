/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#include "vim.h"

#ifdef USE_GTK4_SNAPSHOT

#include <gtk/gtk.h>
#include "gui_gtk4_snapshot.h"

/*
 * We don't rebuild the entire scene on very snapshot vfunc call, that would be
 * slow. Instead cache rows which contain a list of pango items representing the
 * text/glyphs of that row.
 */

/*
 * Represents a single continuous string of glyphs with the same properties. The
 * size of this shouldn't matter, GTK consumes hundreds of megabytes anyyways.
 */
typedef struct
{
    PangoGlyphString	*glyphs;
    int			start_col;
    int			n_cells;
    int			flags; // DRAW_* flags
    char_u		invalid;

    // Background color for this item. This is just a saved value of gui.bgcolor
    // at the time of creation time for this item.
    GdkRGBA		bg_color;

    // Same as "bg_color", but for foreground/special color. Used for
    // "decor_node" since the creation of the render node is deferred later.
    GdkRGBA		fg_color;
    GdkRGBA		sp_color;

    // When this item is modified, this is set to NULL to invalidate it.
    GskRenderNode	*render_node;

    // Font that this DrawItem always uses. We could make this a union with
    // "render_node" (since we can get the font from the render node), but that
    // would add more complexity with little benefit.
    PangoFont		*font;

    // Underline, undercurl, and strikethrough decorations render node which
    // will be overlaid ontop of the text, cached. NULL if invalid or none.
    GskRenderNode	*decor_node;
} DrawItem;

typedef struct
{
    DrawItem	    *item;	    // Item occupying this cell, else NULL.
    gboolean	    invert;	    // If cell color should be inverted.
} DrawCell;

/*
 * Contains the render nodes for a single row on the screen.
 */
typedef struct
{
    // After this struct is an inline array of pointers to DrawCells with length
    // "VimDrawArea->n_cols", each element in the array representing a cell.
    //
    // Each cell may have a DrawItem or be a pointer to an existing one if it is
    // part of a DrawItem that spans multiple cells. It may also be NULL, if
    // there is nothing to be rendered at that cell. If "invert" is true for a
    // cell, then it should be inverted.
} DrawRow;

typedef struct
{
    int		    row;
    int		    col;
    GskRenderNode   *render_node;
} DrawSign;

struct _VimDrawArea
{
    GtkWidget parent;

    DrawRow	    *rows;	// May be NULL
    int		    n_rows;
    int		    n_cols;

    // Used for hollow and part style cursors. For the block cursor, that is
    // simply rendered as a cell using vim_draw_area_add_glyphs(). May be NULL.
    GskRenderNode *cursor_node;

    // See vim_draw_area_size_allocate()
    guint   resize_timeout_id;
    int	    pending_width;
    int	    pending_height;

    // Array of sign icon render nodes (DrawSign)
    GArray *signs;
};

// Make sure everything is aligned, this isn't needed because DrawRow is empty,
// but new fields may be added to it in the future.
#define DRAW_ROW_HDR_SIZE \
    (((sizeof(DrawRow) + G_ALIGNOF(DrawCell) - 1) \
      / G_ALIGNOF(DrawCell)) * G_ALIGNOF(DrawCell))

#define DRAW_CELLS_SIZE(n) ((n) * sizeof(DrawCell))
#define DRAW_ROW_SIZE(da) (DRAW_ROW_HDR_SIZE + DRAW_CELLS_SIZE(da->n_cols))
#define DRAW_ROW_CELLS(r) ((DrawCell *)((char *)(r) + DRAW_ROW_HDR_SIZE))
#define GET_ROW(da, n) \
    ((DrawRow *)((char *)((da)->rows) + ((n) * DRAW_ROW_SIZE(da))))

G_DEFINE_FINAL_TYPE(VimDrawArea, vim_draw_area, GTK_TYPE_WIDGET)

static void vim_draw_area_free(VimDrawArea *self);
static void draw_sign_clear(DrawSign *dsign);
static void vim_draw_area_snapshot(GtkWidget *widget, GtkSnapshot *snapshot);
static void vim_draw_area_size_allocate(GtkWidget *widget, int width, int height, int baseline);

    static void
vim_draw_area_finalize(GObject *obj)
{
    VimDrawArea *self = VIM_DRAW_AREA(obj);

    vim_draw_area_free(self);
    g_array_free(self->signs, TRUE);

    G_OBJECT_CLASS(vim_draw_area_parent_class)->finalize(obj);
}

    static void
vim_draw_area_class_init(VimDrawAreaClass *class)
{
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(class);
    GObjectClass    *obj_class = G_OBJECT_CLASS(class);

    widget_class->snapshot = vim_draw_area_snapshot;
    widget_class->size_allocate = vim_draw_area_size_allocate;

    obj_class->finalize = vim_draw_area_finalize;
}

    static void
vim_draw_area_init(VimDrawArea *self)
{
    self->signs = g_array_new(FALSE, FALSE, sizeof(DrawSign));
    g_array_set_clear_func(self->signs, (GDestroyNotify)draw_sign_clear);
}

    GtkWidget *
vim_draw_area_new(void)
{
    return g_object_new(VIM_TYPE_DRAW_AREA, NULL);
}

    static void
vim_draw_area_free(VimDrawArea *self)
{
    if (self->rows != NULL)
    {
	vim_draw_area_clear(self);
	g_free(self->rows);
    }
    g_clear_pointer(&self->cursor_node, gsk_render_node_unref);
    g_array_remove_range(self->signs, 0, self->signs->len);
}

/*
 * Set the size of the drawing area to "rows" and "cols". This fully clears
 * everything.
 */
    void
vim_draw_area_set_size(VimDrawArea *self, int rows, int cols)
{
    if (self->rows != NULL && self->n_rows == rows && self->n_cols == cols)
	return;

    vim_draw_area_free(self);

    self->n_rows = rows;
    self->n_cols = cols;
    self->rows = g_malloc0_n(rows, DRAW_ROW_SIZE(self));
}

    static void
draw_sign_clear(DrawSign *dsign)
{
    gsk_render_node_unref(dsign->render_node);
}

/*
 * Should always be called before accessing the rows and cells.
 */
    static gboolean
vim_draw_area_valid(VimDrawArea *self)
{
    return self->rows != NULL;
}

/*
 * Invalidate the given DrawItem.
 */
    static void
draw_item_invalidate(DrawItem *ditem)
{
    ditem->invalid = TRUE;
    g_clear_pointer(&ditem->render_node, gsk_render_node_unref);
    g_clear_pointer(&ditem->decor_node, gsk_render_node_unref);
}

/*
 * Modify "ditem" so that it is truncated at "cell_offset" (exclusive). If
 * "to_right" is true, then the left half of "ditem" will be "cleared", and the
 * right half will be updated as if the item started at "cell_offset". Otherwise
 * if "to_right" is false, truncate "ditem" to "cell_offset" size. Note that it
 * is assumed that "cell_offset" is not in the middle of a double width
 * character. This will mark the item as invalid. Additionally if "copy" is
 * true, then the contents of "ditem" will be copied.
 */
    static void
draw_item_truncate(DrawItem *ditem, int cell_offset, gboolean to_right)
{
    int cells_seen = 0;
    int prev_cluster = -1;
    // Default to num_glyphs: if cell_offset equals the total cell count the
    // loop below will exhaust all glyphs without hitting the break, and we want
    // to split at (or truncate to) the full glyph array in that case.
    int glyph_split_idx = ditem->glyphs->num_glyphs;

    for (int i = 0; i < ditem->glyphs->num_glyphs; i++)
    {
	if (ditem->glyphs->log_clusters[i] != prev_cluster)
	{
	    if (cells_seen == cell_offset)
	    {
		glyph_split_idx = i;
		break;
	    }
	    // Count cells this cluster occupies from glyph geometry. The width
	    // of the glyph should have a fixed width for all glyphs.
	    int cluster_cells = ditem->glyphs->glyphs[i].geometry.width
		/ (gui.char_width * PANGO_SCALE);
	    cells_seen += cluster_cells;
	    prev_cluster = ditem->glyphs->log_clusters[i];
	}
    }

    if (to_right)
    {
	int n_right_glyphs = ditem->glyphs->num_glyphs - glyph_split_idx;
	int cluster_adjust;

	if (n_right_glyphs == 0)
	{
	    // "cell_offset" covers the entire item; right half is empty.
	    pango_glyph_string_set_size(ditem->glyphs, 0);
	    ditem->start_col += cell_offset;
	    ditem->n_cells -= cell_offset;
	    draw_item_invalidate(ditem);
	    return;
	}

	cluster_adjust = ditem->glyphs->log_clusters[glyph_split_idx];

	// Move glyphs after "glyph_split_idx" in "item" to the start of the
	// array.
	memmove(ditem->glyphs->glyphs,
		ditem->glyphs->glyphs + glyph_split_idx,
		sizeof(PangoGlyphInfo) * n_right_glyphs);
	memmove(ditem->glyphs->log_clusters,
		ditem->glyphs->log_clusters + glyph_split_idx,
		sizeof(*ditem->glyphs->log_clusters) * n_right_glyphs);

	pango_glyph_string_set_size(ditem->glyphs, n_right_glyphs);

	// Adjust log_clusters to be relative to the right item's offset. Should
	// work with RTL text, because Vim handles that beforehand internally.
	for (int i = 0; i < n_right_glyphs; i++)
	    ditem->glyphs->log_clusters[i] -= cluster_adjust;

	ditem->start_col = ditem->start_col + cell_offset;
	ditem->n_cells = ditem->n_cells - cell_offset;
    }
    else
    {
	// We can simply truncate "item", as it will always be on the left.
	pango_glyph_string_set_size(ditem->glyphs, glyph_split_idx);

	ditem->n_cells = cell_offset;
    }
    draw_item_invalidate(ditem);
}

/*
 * NULL out the given cells between "start" and "end" for "drow" (inclusive).
 */
    static void
draw_row_null(DrawRow *drow, int start, int end)
{
    memset(DRAW_ROW_CELLS(drow) + start, 0, DRAW_CELLS_SIZE(end - start + 1));
}

/*
 * Fill the given cells between "start" and "end" with "ditem" (inclusive).
 */
    static void
draw_row_fill(DrawRow *drow, int start, int end, DrawItem *ditem)
{
    for (int c = start; c <= end; c++)
	DRAW_ROW_CELLS(drow)[c].item = ditem;
}

/*
 * Create a copy of "ditem"
 */
    static DrawItem *
draw_item_copy(DrawItem *ditem)
{
    DrawItem *new_ditem = g_new0(DrawItem, 1);

    new_ditem->glyphs = pango_glyph_string_copy(ditem->glyphs);
    new_ditem->start_col = ditem->start_col;
    new_ditem->n_cells = ditem->n_cells;
    new_ditem->bg_color = ditem->bg_color;
    new_ditem->fg_color = ditem->fg_color;
    new_ditem->sp_color = ditem->sp_color;
    new_ditem->flags = ditem->flags;
    new_ditem->font = g_object_ref(ditem->font);

    if (ditem->render_node != NULL)
	new_ditem->render_node = gsk_render_node_ref(ditem->render_node);
    if (ditem->decor_node != NULL)
	new_ditem->decor_node = gsk_render_node_ref(ditem->decor_node);

    return new_ditem;
}

    static void
draw_item_free(DrawItem *ditem)
{
    pango_glyph_string_free(ditem->glyphs);
    g_object_unref(ditem->font);
    if (ditem->render_node != NULL)
	gsk_render_node_unref(ditem->render_node);
    if (ditem->decor_node != NULL)
	gsk_render_node_unref(ditem->decor_node);
    g_free(ditem);
}

/*
 * Make space for "n" cells starting at "col". Split or clear any existing
 * render nodes that overlap with the new space. Null out the cells/items that
 * were made space for.
 */
    static void
draw_row_make_space(VimDrawArea *da, DrawRow *drow, int col, int n)
{
    int end = col + n;

    // Fast path: when clearing whole row, no need to worry about truncating
    // items.
    if (col == 0 && n == da->n_cols)
    {
	for (int c = 0; c < n;)
	{
	    DrawItem	*ditem = DRAW_ROW_CELLS(drow)[c].item;
	    int		n_cells;

	    if (ditem == NULL)
	    {
		c++;
		continue;
	    }
	    n_cells = ditem->n_cells;

	    draw_item_free(ditem);
	    c += n_cells;
	}
	draw_row_null(drow, 0, n - 1);
	return;
    }

    while (col < end)
    {
	DrawItem *ditem = DRAW_ROW_CELLS(drow)[col].item;

	if (ditem == NULL)
	{
	    // Advance until we reach an existing render node or until we go
	    // past "n" cells.
	    draw_row_null(drow, col, col);
	    col++;
	    continue;
	}

	if (ditem->start_col < col)
	{
	    int item_end = ditem->start_col + ditem->n_cells;

	    if (item_end > end)
	    {
		// New item is entirely inside existing item, split the existing
		// item into two. Reuse the existing item for a left halve of
		// it. Then create a new item for the right halve, placed after
		// the cleared space.
		DrawItem *right = draw_item_copy(ditem);

		// These calls will mark both nodes as invalid.
		draw_item_truncate(right, end - ditem->start_col, TRUE);
		draw_item_truncate(ditem, col - ditem->start_col, FALSE);

		draw_row_null(drow, col, end - 1);

		for (int i = end; i < item_end; i++)
		    DRAW_ROW_CELLS(drow)[i].item = right;

		col = end;
	    }
	    else
	    {
		// "ditem" starts before "col", must split it, discard right
		// halve. This should only happen one time, since we then move
		// forwards afterwards always.
		draw_item_truncate(ditem, col - ditem->start_col, FALSE);

		draw_row_null(drow, col, item_end - 1);

		col++; // ditem now ends at col, so next cell to check is col+1
	    }
	}
	else
	{
	    assert(ditem->start_col == col);

	    int remaining = end - col;

	    // If "remaining" is greater or equal than the number of cells
	    // "ditem" takes, then just clear "ditem" entirely. Otherwise
	    // truncate "ditem" by "remaining" cells.
	    if (remaining >= ditem->n_cells)
	    {
		int n_cells = ditem->n_cells;

		draw_item_free(ditem);
		draw_row_null(drow, col, col + n_cells - 1);
		col += n_cells;
	    }
	    else
	    {
		int old_start = ditem->start_col;

		draw_item_truncate(ditem, remaining, TRUE);
		draw_row_null(drow, old_start, ditem->start_col - 1);
		col = end;
	    }
	}
    }
}

/*
 * Returns true if "ditem" matches "font" + "flags" in terms of
 * color/decorations.
 */
    static gboolean
draw_item_match(DrawItem *ditem, PangoFont *font, int flags)
{
    if (ditem->flags != flags)
	return FALSE;

    if (!(flags & DRAW_TRANSP)
	    && !gdk_rgba_equal(&ditem->bg_color, gui.bgcolor))
	return FALSE;

    if (!gdk_rgba_equal(&ditem->fg_color, gui.fgcolor))
	return FALSE;

    // Special color is only used for undercurls
    if (flags & DRAW_UNDERC && !gdk_rgba_equal(&ditem->sp_color, gui.spcolor))
	return FALSE;

    // This may not work all the time, but creating two PangoFontDescription
    // each time to compare equality seems slow...
    return ditem->font == font;
}

/*
 * Extend "glyphs" by adding the glyphs in "extend" to the end of "glyphs".
 */
    static void
glyph_string_extend(PangoGlyphString *glyphs, PangoGlyphString *extend)
{
    int	    left_n = glyphs->num_glyphs;
    int	    src_n  = extend->num_glyphs;
    int	    cluster_offset = 0;
    int	    i;

    // Choose an offset large enough that no right cluster aliases a left one.
    for (i = 0; i < left_n; i++)
	if (glyphs->log_clusters[i] >= cluster_offset)
	    cluster_offset = glyphs->log_clusters[i] + 1;

    pango_glyph_string_set_size(glyphs, left_n + src_n);

    // Append right's glyph geometry.
    memcpy(glyphs->glyphs + left_n,
	    extend->glyphs, sizeof(PangoGlyphInfo) * src_n);

    // Append right's log_clusters, shifted into a non-overlapping range.
    for (i = 0; i < src_n; i++)
	glyphs->log_clusters[left_n + i] =
	    extend->log_clusters[i] + cluster_offset;
}

/*
 * Add the given glyphs to "row" starting at column "col". If "flags" contains
 * decorations such as underlines, then they will also be applied. Note that
 * this does not queue a redraw for the widget.
 */
    void
vim_draw_area_add_glyphs(
	VimDrawArea	    *self,
	int		    row,
	int		    col,
	int		    num_cells,
	int		    flags,
	PangoFont	    *font,
	PangoGlyphString    *glyphs)
{
    DrawRow	    *drow;
    DrawItem	    *ditem;
    gboolean	    merged_left = FALSE;
    GdkRGBA	    prev_bg;
    PangoRectangle  ink_rect;
    

    if (!vim_draw_area_valid(self)
	    || row >= self->n_rows
	    || col >= self->n_cols)
	return;

    // Clip "num_cells" so we never write past the end of the row.
    if (col + num_cells > self->n_cols)
	num_cells = self->n_cols - col;

    drow = GET_ROW(self, row);
    draw_row_make_space(self, drow, col, num_cells);

    // Check if we can use the existing DrawItem on the left of the new space,
    // and extend it. This prevents fragmentation of DrawItem's over time,
    // and reduces number of render nodes in scene + saves memory.
    if (col > 0)
    {
	DrawItem *left = DRAW_ROW_CELLS(drow)[col - 1].item;

	if (left != NULL && draw_item_match(left, font, flags))
	{
	    glyph_string_extend(left->glyphs, glyphs);
	    left->n_cells += num_cells;
	    draw_item_invalidate(left);
	    ditem = left;
	    merged_left = TRUE;

	    // Try merging the DrawItem on the right as well. If we can't, then
	    // just return early.
	}
    }

    // Check if we can use the existing DrawItem on the right. If so, then shift
    // "right" to the "col", and extend it. If we merged the left DrawItem, then
    // instead extend it normally and free the right DrawItem.
    if (col + num_cells < self->n_cols)
    {
	DrawItem *right = DRAW_ROW_CELLS(drow)[col + num_cells].item;

	if (right != NULL && draw_item_match(right, font, flags))
	{
	    if (merged_left)
	    {
		glyph_string_extend(ditem->glyphs, right->glyphs);
		ditem->n_cells += right->n_cells;

		draw_row_fill(drow, right->start_col,
			right->start_col + right->n_cells - 1, ditem);
		draw_item_free(right);
	    }
	    else
	    {
		// Since "right" is on the right, we have to make a copy of
		// "glyphs" to be used as the new "left".
		PangoGlyphString *new_left = pango_glyph_string_copy(glyphs);

		glyph_string_extend(new_left, right->glyphs);
		pango_glyph_string_free(right->glyphs);
		right->glyphs = new_left;

		right->start_col = col;
		right->n_cells += num_cells;
		draw_item_invalidate(right);
		ditem = right;
	    }
	    goto exit;
	}
    }

    if (merged_left)
	goto exit;

    // If glyphs is just whitespace (no ink), and the background color is the
    // same as the global background color, do not create a new item.
    pango_glyph_string_extents(glyphs, font, &ink_rect, NULL);

    if (ink_rect.height == 0 && ink_rect.width == 0)
    {
	prev_bg = *gui.bgcolor;
	gui_mch_set_bg_color(gui.back_pixel);
	if (gdk_rgba_equal(gui.bgcolor, &prev_bg))
	    return;
	*gui.bgcolor = prev_bg;
    }

    ditem = g_new0(DrawItem, 1);

    ditem->glyphs = pango_glyph_string_copy(glyphs);
    ditem->n_cells = num_cells;
    ditem->start_col = col;
    ditem->font = g_object_ref(font);

    if (!(flags & DRAW_TRANSP))
	ditem->bg_color = *gui.bgcolor;
    ditem->fg_color = *gui.fgcolor;
    ditem->sp_color = *gui.spcolor;
    ditem->flags = flags;
    // We will create render node in snapshot vfunc
    ditem->invalid = TRUE;

#if defined(FEAT_SIGN_ICONS)
    // Overwrite any sign icons (clear them)
    for (guint i = 0; i < self->signs->len;)
    {
	DrawSign *dsign = &g_array_index(self->signs, DrawSign, i);

	if (dsign->row == row && dsign->col >= col
		&& dsign->col < col + num_cells)
	    g_array_remove_index_fast(self->signs, i);
	else
	    i++;
    }
#endif

exit:
    draw_row_fill(drow, col, col + num_cells - 1, ditem);
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

    if (unlikely(!vim_draw_area_valid(self)
		|| row1 >= self->n_rows
		|| col1 >= self->n_cols
		|| row2 >= self->n_rows
		|| col2 >= self->n_cols))
	return;

    for (int r = row1; r <= row2; r++)
    {
	DrawRow	*drow = GET_ROW(self, r);
	draw_row_make_space(self, drow, col1, col2 - col1 + 1);
    }

    if (self->cursor_node != NULL)
	// Check if cursor node is within the the cleared region. If so, then
	// remove the render node. This only applies to the part and hollow
	// cursor, the block cursor will be cleared in draw_row_make_space().
	if (gui.cursor_row >= row1 && gui.cursor_row <= row2
		&& gui.cursor_col >= col1 && gui.cursor_col <= col2)
	    g_clear_pointer(&self->cursor_node, gsk_render_node_unref);

#if defined(FEAT_SIGN_ICONS)
    // Clear any sign icons if any
    for (guint i = 0; i < self->signs->len;)
    {
	DrawSign *dsign = &g_array_index(self->signs, DrawSign, i);

	if (dsign->row >= row1 && dsign->row <= row2
		&& dsign->col >= col1 && dsign->col <= col2)
	    // Keep going in case there are multiple sign icons within this
	    // blokc. I don't think that can happen, but just do it anyways.
	    g_array_remove_index_fast(self->signs, i);
	else
	    i++;
    }
#endif
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
 * Clip "drow" in between "col1" and "col2" (inclusive), truncating DrawItem's
 * as necessary.
 */
    static void
draw_row_clip(VimDrawArea *da, DrawRow *drow, int col1, int col2)
{
    DrawCell *cells = DRAW_ROW_CELLS(drow);
    DrawItem *left = cells[col1].item;
    DrawItem *right = cells[col2].item;

    // THe items fully inside the row are fine, we must handle ones that overlap
    // the boundary of "col1" or "col2".
    if (left != NULL && col1 > 0 && cells[col1 - 1].item == left)
    {
	DrawItem    *new_left = draw_item_copy(left);
	int	    start = left->start_col;

	draw_item_truncate(new_left, col1 - left->start_col, FALSE);
	draw_item_truncate(left, col1 - left->start_col, TRUE);
	for (int c = start; c < col1; c++)
	    cells[c].item = new_left;
    }
    if (right != NULL && col2 < da->n_cols - 1 && cells[col2 + 1].item == right)
    {
	DrawItem *new_right = draw_item_copy(right);

	draw_item_truncate(right, col2 - right->start_col + 1, FALSE);
	draw_item_truncate(new_right, col2 - right->start_col + 1, TRUE);
	for (int c = col2 + 1; c < new_right->start_col + new_right->n_cells; c++)
	    cells[c].item = new_right;
    }
}

/*
 * Move the cells between "col1" and "col2" from "src" to "dest", overwriting
 * the existing cells.
 */
    static void
draw_row_move_to(DrawRow *dest, DrawRow *src, int col1, int col2)
{
    DrawCell	*src_cells = DRAW_ROW_CELLS(src);
    DrawCell	*dest_cells = DRAW_ROW_CELLS(dest);
    int		move_size = DRAW_CELLS_SIZE(col2 - col1 + 1);

    memmove(dest_cells + col1, src_cells + col1, move_size);
    // Invalidate the moved cells
    for (int c = col1; c <= col2;)
	if (dest_cells[c].item != NULL)
	{
	    draw_item_invalidate(dest_cells[c].item);
	    c += dest_cells[c].item->n_cells;
	}
	else
	    c++;
}

/*
 * Move the rows between "row1" and "row2" to "to", between columns "col1" and
 * "col2" (inclusive). Rows that have been moved are cleared.
 */
    void
vim_draw_area_move_block(
	VimDrawArea	*self,
	int		to,
	int		row1,
	int		row2,
	int		col1,
	int		col2)
{
    int offset = row2 - row1;

    if (unlikely(!vim_draw_area_valid(self)
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
	// being shifted upwards, must make sure to free the rows that are being
	// overwritten.
	for (int o = 0; o <= offset; o++)
	{
	    DrawRow	*src_row = GET_ROW(self, row1 + o);
	    DrawRow	*dest_row = GET_ROW(self, to + o);

	    // Make sure that we free/truncate any items before we overwrite
	    // them.
	    draw_row_make_space(self, dest_row, col1, col2 - col1 + 1);

	    // Make sure that items at the "col1" and "col2" of "src_row" are
	    // truncated.
	    draw_row_clip(self, src_row, col1, col2);
	    draw_row_move_to(dest_row, src_row, col1, col2);
	    // Null the items so we don't double free.
	    draw_row_null(src_row, col1, col2);
	}
    }
    else
    {
	// "row1" is above "to", must start moving rows starting at "row2". Rows
	// are being shifted downards, must free rows that clip off the bottom.
	for (int o = offset; o >= 0; o--)
	{
	    DrawRow	*src_row;
	    DrawRow	*dest_row;

	    if (to + o >= self->n_rows)
	    {
		gui_clear_block(row1 + o, col1, row1 + o, col2);
		continue;
	    }

	    src_row = GET_ROW(self, row1 + o);
	    dest_row = GET_ROW(self, to + o);

	    draw_row_make_space(self, dest_row, col1, col2 - col1 + 1);
	    draw_row_clip(self, src_row, col1, col2);
	    draw_row_move_to(dest_row, src_row, col1, col2);
	    draw_row_null(src_row, col1, col2);
	}
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
 * Clear the part or hollow cursor if it exists.
 */
    void
vim_draw_area_clear_cursor(VimDrawArea *self)
{
    g_clear_pointer(&self->cursor_node, gsk_render_node_unref);
}

/*
 * Invert the rectangle in the draw area.
 */
    void
vim_draw_area_invert(VimDrawArea *self, int row, int col, int nrows, int ncols)
{
    if (unlikely(!vim_draw_area_valid(self)
		|| row >= self->n_rows
		|| col >= self->n_cols
		|| row + nrows - 1 >= self->n_rows
		|| col + ncols - 1 >= self->n_cols))
	return;

    for (int r = row; r < row + nrows; r++)
    {
	DrawRow *drow = GET_ROW(self, r);

	for (int c = col; c < col + ncols; c++)
	{
	    DrawCell *cell = DRAW_ROW_CELLS(drow) + c;

	    cell->invert = !cell->invert;
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
    DrawSign	    new_dsign;

    if (unlikely(!vim_draw_area_valid(self)
		|| row >= self->n_rows
		|| col >= self->n_cols))
	return;

    node = gsk_texture_scale_node_new(sign,
	    &GRAPHENE_RECT_INIT(FILL_X(col), FILL_Y(row), width, height),
	    GSK_SCALING_FILTER_TRILINEAR);

    for (guint i = 0; i < self->signs->len; i++)
    {
	DrawSign *dsign = &g_array_index(self->signs, DrawSign, i);

	if (dsign->row == row && dsign->col == col)
	{
	    gsk_render_node_unref(dsign->render_node);
	    dsign->render_node = node;
	    return;
	}
    }

    new_dsign.row = row;
    new_dsign.col = col;
    new_dsign.render_node = node;

    g_array_append_val(self->signs, new_dsign);
}

/*
 * Remove the sign texture if it exists.
 */
    void
vim_draw_area_remove_sign(VimDrawArea *self, GdkTexture *sign)
{
    for (guint i = 0; i < self->signs->len; i++)
    {
	DrawSign *dsign = &g_array_index(self->signs, DrawSign, i);

	if (gsk_texture_scale_node_get_texture(dsign->render_node) == sign)
	{
	    g_array_remove_index_fast(self->signs, i);
	    return;
	}
    }
}
#endif

/*
 * Apply decorations to snapshot using the given DrawItem.
 */
    static void
draw_item_apply_decor(GtkSnapshot *main_snapshot, int row, DrawItem *ditem)
{
    int col = ditem->start_col;
    int cells = ditem->n_cells;
    GtkSnapshot *snapshot;

    // Check if node is already cached.
    if (ditem->decor_node != NULL)
	goto exit;
    if (!(ditem->flags & (DRAW_UNDERL | DRAW_UNDERC | DRAW_STRIKE)))
	return;

    snapshot = gtk_snapshot_new();

    if (ditem->flags & DRAW_UNDERL)
    {
	const graphene_rect_t rect = GRAPHENE_RECT_INIT(
		FILL_X(col),
		FILL_Y(row + 1) - 1,
		FILL_X(col + cells) - FILL_X(col),
		1);

	gtk_snapshot_append_color(snapshot, &ditem->fg_color, &rect);
    }
    if (ditem->flags & DRAW_STRIKE)
    {
	const graphene_rect_t rect = GRAPHENE_RECT_INIT(
		FILL_X(col),
		FILL_Y(row) + (int)(gui.char_height / 2),
		FILL_X(col + cells) - FILL_X(col),
		1);

	gtk_snapshot_append_color(snapshot, &ditem->fg_color, &rect);
    }
    if (ditem->flags & DRAW_UNDERC)
    {
	int             y = FILL_Y(row + 1) - 1; // Top of underneath line,
						 // upwards by one pixel.
	int             x_start = FILL_X(col);
	int             x_end = FILL_X(col + cells);

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

	gtk_snapshot_append_stroke(snapshot, path, stroke, &ditem->sp_color);
	gsk_stroke_free(stroke);
	gsk_path_unref(path);
#else
	static const int    val[8] = {1, 0, 0, 0, 1, 2, 2, 2};
	graphene_rect_t	    bounds;
	cairo_t		    *cr;

	graphene_rect_init(&bounds, x_start, y - 3, x_end - x_start, 5);
	cr = gtk_snapshot_append_cairo(snapshot, &bounds);

	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgba(cr, ditem->sp_color.red, ditem->sp_color.green,
		ditem->sp_color.blue, ditem->sp_color.alpha);

	cairo_move_to(cr, x_start + 1, y - 2 + 0.5);

	for (int i = x_start + 1; i < x_end; ++i)
	{
	    int offset = val[i % 8];
	    cairo_line_to(cr, i, y - offset + 0.5);
	}

	cairo_stroke(cr);
	cairo_destroy(cr);
#endif
    }

    // Cache the node for later use
    ditem->decor_node = gtk_snapshot_free_to_node(snapshot);
    if (ditem->decor_node == NULL)
	return;
exit:
    gtk_snapshot_append_node(main_snapshot, ditem->decor_node);
}

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
    height = gtk_widget_get_height(widget);
    width = gtk_widget_get_width(widget);

    if (self->rows == NULL)
    {
	gtk_snapshot_append_color(snapshot, gui.bgcolor,
		&GRAPHENE_RECT_INIT(0, 0, width, height));
	return;
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
	DrawRow	    *drow = GET_ROW(self, r);
	DrawCell    *cells = DRAW_ROW_CELLS(drow);
	int		inv_len = 0;
	int		inv_start;

	for (int c = 0; c < self->n_cols; c++)
	{
	    DrawCell	    *cell = cells + c;
	    DrawItem	    *ditem = cell->item;

	    // Batch inverted cells as single row rectangles.
	    if (cell->invert)
	    {
		if (inv_len == 0)
		    inv_start = c;
		inv_len++;
	    }
	    else if (!cell->invert && inv_len > 0)
	    {
		flush_invert_ga(&invert_ga, r, inv_start, inv_len);
		inv_len = 0;
	    }

	    if (ditem == NULL || ditem->start_col != c)
		continue;

	    // No point in appending a color behind text if its just the same as
	    // background color anyways. Reduces number of render nodes in
	    // scene.
	    if (unlikely(!(ditem->flags & DRAW_TRANSP)
			&& !gdk_rgba_equal(&ditem->bg_color, gui.bgcolor)))
	    {
		gtk_snapshot_append_color(snapshot, &ditem->bg_color,
			&GRAPHENE_RECT_INIT(FILL_X(ditem->start_col), FILL_Y(r),
			    ditem->n_cells * gui.char_width, gui.char_height));
	    }

	    if (ditem->invalid)
	    {
		// Rebuild the render node at the correct position. it may be
		// NULL for whitespace items that produced no ink, in which case
		// we leave render_node as NULL after rebuilding.
		GskRenderNode *new = gsk_text_node_new(ditem->font,
			ditem->glyphs, &ditem->fg_color,
			&GRAPHENE_POINT_INIT(TEXT_X(ditem->start_col),
			    TEXT_Y(r)));

		if (ditem->render_node != NULL)
		    gsk_render_node_unref(ditem->render_node);
		ditem->render_node = new;
		ditem->invalid = FALSE;
	    }

	    if (ditem->render_node != NULL)
	    {
		gtk_snapshot_append_node(snapshot, ditem->render_node);
		if (unlikely(!gui.font_can_bold && ditem->flags & DRAW_BOLD))
		{
		    // Shift a copy of the text node by one pixel to make
		    // stroke look thicker.
		    gtk_snapshot_save(snapshot);

		    gtk_snapshot_translate(snapshot,
			    &GRAPHENE_POINT_INIT(1, 0));
		    gtk_snapshot_append_node(snapshot, ditem->render_node);

		    gtk_snapshot_restore(snapshot);
		}
	    }
	    draw_item_apply_decor(snapshot, r, ditem);
	}
	// Flush trailing inverted blocks at end of row loop
	if (inv_len > 0)
	    flush_invert_ga(&invert_ga, r, inv_start, inv_len);
    }

    // Order of where the sign icon should be placed shouldn't matter,
    // since caller will add whitespace padding in the region it covers.
    // Probably should put it behind cursor though.
    for (guint i = 0; i < self->signs->len; i++)
	gtk_snapshot_append_node(snapshot,
		g_array_index(self->signs, DrawSign, i).render_node);

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
