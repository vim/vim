/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read a list of people who contributed.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * Abstracts each image backend into a common interface. This common interface
 * works around the idea of images being blitted to the screen, similar to how
 * characters are written to the terminal/GUI. For backends that do not natively
 * blit pixels ("images" are objects), the behaviour is emulated.
 */

#include "vim.h"

#if defined(FEAT_IMAGE)

struct
{
    const char *name;

    struct
    {
	// Return OK on success and FAIL on failure
	int (*init)(image_T *);
	void (*uninit)(image_T *);
    } image;

    struct
    {
	// Return OK on success and FAIL on failure
	int (*init)(image_placement_T *);
	void (*uninit)(image_placement_T *);

	void (*draw)(image_placement_T *);
	void (*clear)(image_placement_T *);

	// If backend blits the image pixels to the screen. Images are not
	// treated as "objects".
	bool blit;
    } placement;
} image_backends[] = {
#ifdef FEAT_IMAGE_GUI
    [IMAGE_BACKEND_GUI] = {
	.name = "gui",
	.image = {
	    .init = image_gui_init,
	    .uninit = image_gui_uninit
	},
	.placement = {
	    .init = image_placement_gui_init,
	    .uninit = image_placement_gui_uninit,
	    .draw = image_placement_gui_draw,
	    .clear = image_placement_gui_clear,
# ifdef FEAT_GUI_GTK
#  ifdef USE_GTK4
	    .blit = false,
#  else
	    .blit = true,
#  endif
# elif FEAT_GUI_MSWIN
	    .blit = true,
# else
	    .blit = false,
# endif
	}
    },
#endif
#ifdef FEAT_IMAGE_KITTY
    [IMAGE_BACKEND_KITTY] = {
	.name = "kitty",
	.image = {
	    .init = image_kitty_init,
	    .uninit = image_kitty_uninit
	},
	.placement = {
	    .init = image_placement_kitty_init,
	    .uninit = image_placement_kitty_uninit,
	    .draw = image_placement_kitty_draw,
	    .clear = image_placement_kitty_clear,
	    .blit = false
	}
    },
#endif
#ifdef FEAT_IMAGE_SIXEL
    [IMAGE_BACKEND_SIXEL] = {
	.name = "sixel",
	.image = {
	    .init = image_sixel_init,
	    .uninit = image_sixel_uninit
	},
	.placement = {
	    .init = image_placement_sixel_init,
	    .uninit = image_placement_sixel_uninit,
	    .draw = image_placement_sixel_draw,
	    .clear = image_placement_sixel_clear,
	    .blit = true
	}
    }
#endif
};

// Current image backend being used
static image_backend_T image_backend = IMAGE_BACKEND_NONE;

static image_T *images = NULL;
// Sorted from highest zindex to lowest zindex
static image_placement_T    *placements = NULL;
static int		    n_placements = 0;

static int shift_top = 0;
static int shift_bot = 0;
static int shift = 0;

#define FOR_ALL_IMAGES(v) for ((v) = images; (v) != NULL; (v) = (v)->next)
#define FOR_ALL_PLACEMENTS(v) \
    for ((v) = placements; (v) != NULL; (v) = (v)->next)

#define IMAGE_FUNC(b, n) (image_backends[b].image.n)
#define PLACEMENT_FUNC(b, n) (image_backends[b].placement.n)

static void redraw_region(pixman_region32_t *region, bool now, bool restore);

    void
init_image_state(void)
{
    (void)update_image_backend();
}

    void
uninit_image_state(void)
{
#ifdef FEAT_IMAGE_SIXEL
    sixel_uninit();
#endif
}

/*
 * Return true if the there image backend is ready, otherwise emit error.
 */
    static bool
backend_available(bool msg)
{
    if (image_backend == IMAGE_BACKEND_NONE)
    {
	if (msg)
	    emsg(_(e_no_image_backend_available));
	return false;
    }
    return true;
}

/*
 * Create a new image using the given data (creates a copy). Returns NULL on
 * failure.
 */
    image_T *
image_new(uint8_t *data, int width, int height, image_format_T fmt)
{
    image_T	*img;
    static int	id;

    if (!backend_available(true))
	return NULL;

    img = ALLOC_CLEAR_ONE(image_T);
    if (img == NULL)
	return NULL;

    img->data = vim_memsave(data, (size_t)width * height * fmt);
    if (img->data == NULL)
    {
	vim_free(img);
	return NULL;
    }

    img->state = IMAGE_STATE_PRIVATE;
    img->width = width;
    img->height = height;
    img->fmt = fmt;

    // Mix in PID to prevent ID collisions when using kitty graphics protocol
    img->id = ((((int)mch_get_pid() & 0x7fff) + 1) << 16) | (id++ & 0xffff);
    img->refcount = 1;

    img->backend = image_backend;

    if (IMAGE_FUNC(image_backend, init)(img) == FAIL)
    {
	vim_free(img->data);
	vim_free(img);
	return NULL;
    }

    if (images != NULL)
	images->prev = img;
    img->next = images;
    images = img;

    return img;
}

    static void
image_free(image_T *img)
{
    if (backend_available(false))
	IMAGE_FUNC(image_backend, uninit)(img);

    if (img->prev != NULL)
	img->prev->next = img->next;
    if (img->next != NULL)
	img->next->prev = img->prev;
    if (images == img)
	images = img->next;

    vim_free(img->data);
    vim_free(img);
}

    void
image_unref(image_T *img)
{
    if (--img->refcount <= 0)
	image_free(img);
}

    image_T *
image_ref(image_T *img)
{
    img->refcount++;
    return img;
}

/*
 * Get image width and height in cells, rounding up so that every pixel is
 * covered.
 */
    void
image_get_cell_dimensions(image_T *img, int *cw, int *ch)
{
    int w = PHY2LOG(img->width);
    int h = PHY2LOG(img->height);

    *cw = (w + cell_width - 1) / cell_width;
    *ch = (h + cell_height - 1) / cell_height;
}

/*
 * Similar, but return the dimensions of all cells that are *fully* covered by
 * pixels. Pixels that are partially covered are not counted.
 */
    static void
image_get_cell_dimensions_min(image_T *img, int *cw, int *ch)
{
    int w = PHY2LOG(img->width);
    int h = PHY2LOG(img->height);

    *cw = w / cell_width;
    *ch = h / cell_height;
}

    void
image_get_dimensions(image_T *img, int *w, int *h)
{
    *w = img->width;
    *h = img->height;
}

/*
 * Replace the current image contents with "data". "data" should be the exact
 * same format, width, and height. Return OK on success and FAIL on failure.
 */
    int
image_update(image_T *img, uint8_t *data)
{
    int w, h;

    if (!backend_available(true))
	return FAIL;

    image_get_dimensions(img, &w, &h);
    mch_memmove(img->data, data, (size_t)w * h * img->fmt);

    IMAGE_FUNC(image_backend, uninit)(img);

    if (IMAGE_FUNC(image_backend, init)(img) == FAIL)
    {
	emsg(_(e_out_of_memory));
	VIM_CLEAR(img->data);
	return FAIL;
    }

    img->ver++;
    redraw_all_later(UPD_VALID);

    return OK;
}

    static void
image_placement_unlink(image_placement_T *place)
{
    if (place->prev != NULL)
	place->prev->next = place->next;
    if (place->next != NULL)
	place->next->prev = place->prev;
    if (placements == place)
	placements = place->next;
    n_placements--;
}

/*
 * If "first" is true, then always place the placement before placements with
 * the same zindex.
 */
    static void
image_placement_link(image_placement_T *place, bool first)
{
    image_placement_T *p = placements;
    image_placement_T *prev = NULL;

    // Add placement before the placement with the lower zindex.
    while (p != NULL)
    {
	if (first)
	{
	    if (p->zindex <= place->zindex)
		break;
	}
	else if (p->zindex < place->zindex)
	    break;
	prev = p;
	p = p->next;
    }

    place->next = p;
    place->prev = prev;

    if (p != NULL)
	p->prev = place;

    if (prev != NULL)
	prev->next = place;
    else
	placements = place;

    n_placements++;
}

/*
 * Create a new placement for the image, taking ownership of it. By default it
 * will be at the top left corner of the screen, no crop, zindex of 0, and the
 * bounding box will cover the entire image. "img" may be NULL to create a
 * placement with no backing image. Returns NULL on failure.
 */
    image_placement_T *
image_placement_new(image_T *img, bool quiet)
{
    image_placement_T	*place;
    static int		id = 1; // Kitty placements id must be > 1

    if (!backend_available(!quiet))
	return NULL;

    place = ALLOC_CLEAR_ONE(image_placement_T);
    if (place == NULL)
	return NULL;

    place->id = id;
    id += PLACEMENT_ID_INC; // Allocate 1000 free placement ids to be used to
			    // draw this image placement. Only relevant for
			    // kitty graphics protocol.

    place->img = img;
    place->dirty = true;

    if (img != NULL)
    {
	place->img_ver = img->ver;
	place->backend = image_backend;

	// Bounding box is in cells
	image_get_cell_dimensions(img,
		&place->bounding_box.x2, &place->bounding_box.y2);
	place->crop_box.x2 = place->bounding_box.x2;
	place->crop_box.y2 = place->bounding_box.y2;

	if (PLACEMENT_FUNC(image_backend, init)(place) == FAIL)
	{
	    id -= 1000;
	    vim_free(place);
	    return NULL;
	}
    }
    else
	place->backend = IMAGE_BACKEND_NONE;

    image_placement_link(place, false);

    return place;
}

    static void
image_placement_clear_int(image_placement_T *place, bool now)
{
    if (backend_available(false) && place->img != NULL)
    {
	if (place->visible_init && PLACEMENT_FUNC(image_backend, blit))
	    redraw_region(&place->visible_abs, now, true);
	PLACEMENT_FUNC(image_backend, clear)(place);
	place->dirty = true;
	place->hidden = true;
	if (!now && !updating_screen)
	    redraw_all_later(UPD_VALID);
    }
}

    void
image_placement_clear(image_placement_T *place)
{
    image_placement_clear_int(place, updating_screen);
}

    void
image_placement_free(image_placement_T *place)
{
    if (backend_available(false) && place->backend_data != NULL)
    {
	image_placement_clear(place);
	PLACEMENT_FUNC(image_backend, uninit)(place);
    }

    if (place->visible_init)
    {
	pixman_region32_fini(&place->visible);
	pixman_region32_fini(&place->visible_abs);
    }
    image_placement_unlink(place);

    if (place->img != NULL)
	image_unref(place->img);
    vim_free(place);
}

/*
 * See "image_placement_link" for "first"
 */
    void
image_placement_set_zindex(image_placement_T *place, int zindex, bool first)
{
    place->hidden = false;

    if (place->zindex == zindex)
	return;
    place->zindex = zindex;
    place->dirty = true;

    // Must re-add the placement back so it is in the correct order
    image_placement_unlink(place);
    image_placement_link(place, first);
}

/*
 * Make sure to also update the bounding box!
 */
    void
image_placement_set_position(image_placement_T *place, int row, int col)
{
    place->hidden = false;

    if (place->row == row && place->col == col)
	return;
    place->row = row;
    place->col = col;
    place->dirty = true;
}

    void
image_placement_set_crop(image_placement_T *place, int x, int y, int w, int h)
{
    place->hidden = false;

    if (place->crop_box.x1 == x && place->crop_box.y1 == y
	    && place->crop_box.x2 == x + w && place->crop_box.y2 == y + h)
	return;

    place->crop_box.x1 = x;
    place->crop_box.y1 = y;

    place->crop_box.x2 = x + w;
    place->crop_box.y2 = y + h;
    place->dirty = true;
}

    void
image_placement_set_bounding_box(
	image_placement_T   *place,
	int		    row,
	int		    col,
	int		    row_height,
	int		    col_width)
{
    place->hidden = false;

    if (place->bounding_box.x1 == col && place->bounding_box.y1 == row
	    && place->bounding_box.x2 == col + col_width
	    && place->bounding_box.y2 == row + row_height)
	return;

    place->bounding_box.x1 = col;
    place->bounding_box.y1 = row;

    place->bounding_box.x2 = col + col_width;
    place->bounding_box.y2 = row + row_height;
    place->dirty = true;
}

/*
 * Note that the returned crop region (x, y, w, h) is in physical pixels if
 * "physical" is true. Not relevant when using terminal protocols (sixel, KGP).
 */
    void
image_placement_subrect(
	image_placement_T   *place,
	pixman_box32_t	    rect,
	int		    *row,
	int		    *col,
	int		    *x,
	int		    *y,
	int		    *w,
	int		    *h,
	bool		    physical)
{
    int iw, ih;

    image_get_dimensions(place->img, &iw, &ih);

    *row = place->row + rect.y1 + place->row_off;
    *col = place->col + rect.x1;

    *x = (rect.x1 + place->crop_box.x1) * cell_width;
    *y = (rect.y1 + place->crop_box.y1) * cell_height;
    *w = (rect.x2 - rect.x1) * cell_width;
    *h = (rect.y2 - rect.y1) * cell_height;

    if (physical)
    {
	*x = LOG2PHY(*x);
	*y = LOG2PHY(*y);
	*w = LOG2PHY(*w);
	*h = LOG2PHY(*h);
    }
    else
    {
	iw = PHY2LOG(iw);
	ih = PHY2LOG(ih);
    }

    // Make that all the values are valid. "w" and "h" can especially be
    // invalid, because the image dimensions (in pixels) are converted to cells,
    // rounding *up*.
    *row = MIN(*row, Rows);
    *col = MIN(*col, Columns);

    *x = MIN(iw, MAX(*x, 0));
    *y = MIN(ih, MAX(*y, 0));
    *w = MIN(*w, iw - *x);
    *h = MIN(*h, ih - *y);
}

/*
 * Redraw the cells in the given region. This is only relevant for image
 * backends that blit pixels. If "now" is true, then draw the characters now
 * instead of deferring to next redraw. If "restore" is true, restore the cursor
 * position.
 */
    static void
redraw_region(pixman_region32_t *region, bool now, bool restore)
{
    pixman_box32_t  *rects;
    int		    n_rects;
    int		    cur_row = screen_cur_row;
    int		    cur_col = screen_cur_col;

    rects = pixman_region32_rectangles(region, &n_rects);
    if (rects == NULL)
	return;

    for (int i = 0; i < n_rects; i++)
    {
	pixman_box32_t rect = rects[i];

	if (now)
	{
	    // Tip: if you want to debug stale pixels appearing, set "inverse"
	    // of screen_draw_rectangle() to TRUE.
	    screen_draw_rectangle(rect.y1, rect.x1,
		    rect.y2 - rect.y1, rect.x2 - rect.x1, FALSE, TRUE);
	    continue;
	}

	for (int r = rect.y1; r < rect.y2; r++)
	{
	    if (r >= screen_Rows)
		break;

	    for (int c = rect.x1; c < rect.x2; c++)
	    {
		if (c >= screen_Columns)
		    break;

		ScreenAttrs[LineOffset[r] + c] = (sattr_T)-1;
	    }
	}
    }
    if (!now)
	redraw_all_later(UPD_VALID);

    if (restore)
	windgoto(cur_row, cur_col);
}

/*
 * Draw all image placements to the screen. This should be done after all text
 * have been drawn to the screen, and image placements positioned correctly.
 */
    void
draw_image_placements(void)
{
    image_placement_T	*place;
    pixman_region32_t	subtract_region; // In pixels
    image_placement_T	**pending_placements;
    int			pending_len = -1;
    // Save current cursor position
    int			cur_row = screen_cur_row;
    int			cur_col = screen_cur_col;

    // Check if all placements have no backing image. If so, then do nothing
    FOR_ALL_PLACEMENTS(place)
	if (place->img != NULL)
	{
	    pending_len = 0;
	    break;
	}

    if (pending_len == -1)
	return;

    if (!backend_available(false))
	return;

    if (n_placements == 0)
	return;

    pending_placements = ALLOC_CLEAR_MULT(image_placement_T *, n_placements);
    if (pending_placements == NULL)
	return;

    pixman_region32_init(&subtract_region);

    cursor_off();

    // Go through each image placement, from highest to lowests zindex. For each
    // image, subtract the bounding boxes of the images with higher zindexes
    // from its own image region. The result is a region containing rectangles
    // that represent only the visible regions of the image that should be
    // drawn.
    //
    // We must do two passes, one to find what images need to be redrawn, and
    // also what regions are stale and redraw them. If we did everything in one
    // pass, stale regions of one image that overlap another image with a higher
    // index, would overwrite the overlapping image.
    FOR_ALL_PLACEMENTS(place)
    {
	image_T *img = place->img;

	pixman_region32_t   image_region;
	pixman_region32_t   visible_region;

	int x, y;

	if (place->hidden)
	    continue;

	if (img != NULL)
	{
	    pixman_region32_t	visible_abs;
	    int			crop_w, crop_h;
	    bool		need_redraw;

	    // Processing changed image data failed, skip this image
	    if (place->img->data == NULL)
	    {
		image_placement_clear(place);
		continue;
	    }

	    // Only apply row shift if image is within the shifted region.
	    if ((place->bounding_box.y1 >= shift_top
			|| place->bounding_box.y2 <= shift_bot)
		    && place->row_off != shift)
	    {
		place->row_off = shift;
		place->dirty = true;
	    }

	    x = place->col;
	    y = place->row + place->row_off;

	    crop_w = place->crop_box.x2 - place->crop_box.x1;
	    crop_h = place->crop_box.y2 - place->crop_box.y1;

	    // Don't add "crop_box" x1 and y1, because "row" and "col" use the
	    // top left of the final cropped image.
	    pixman_region32_init_rect(&image_region, x, y, crop_w, crop_h);

	    pixman_region32_init(&visible_region);
	    pixman_region32_init(&visible_abs);

	    if (!pixman_region32_intersect_rect(&image_region, &image_region,
			0, 0, Columns, Rows)
		    || !pixman_region32_subtract(&visible_region,
			&image_region, &subtract_region)
		    || !pixman_region32_copy(&visible_abs, &visible_region))
	    {
		pixman_region32_fini(&image_region);
		pixman_region32_fini(&visible_region);
		continue;
	    }

	    pixman_region32_fini(&image_region);

	    // The visible region is in absolute coordinates, must convert it
	    // into image relative coordinates.
	    pixman_region32_translate(&visible_region, -x, -y);

	    // Only redraw the image if it has changed (or if we haven't drawn
	    // it yet). Or if image data has changed
	    need_redraw = place->dirty || (place->visible_init
		    && !pixman_region32_equal(&visible_region, &place->visible));

	    if (place->img_ver != img->ver)
	    {
		// Image data changed
		need_redraw = true;
		place->img_ver = img->ver;

		image_placement_clear(place);
		PLACEMENT_FUNC(image_backend, uninit)(place);
		if (PLACEMENT_FUNC(image_backend, init)(place) == FAIL)
		{
		    emsg(_(e_out_of_memory));
		    image_unref(place->img);
		    place->img = NULL;
		    continue;
		}
	    }

	    if (need_redraw)
	    {
		pixman_region32_t old_visible_abs;

		// Update the old absolute visible region now. This is so that
		// in redraw_region() which may call
		// mark_dirty_region_for_images(), does not dirty the current
		// image again and cause another UPD_VALID redraw.
		if (place->visible_init)
		{
		    pixman_region32_fini(&place->visible);
		    old_visible_abs = place->visible_abs;
		}
		place->visible = visible_region;
		place->visible_abs = visible_abs;

		if (PLACEMENT_FUNC(image_backend, blit))
		{
		    // Must redraw the stale regions that will not be composited
		    // over (for this specific image).
		    pixman_region32_t stale_region;

		    pixman_region32_init(&stale_region);

		    if (place->visible_init)
		    {
			(void)pixman_region32_subtract(&stale_region,
				&old_visible_abs, &visible_abs);

			// Also subtract "subtract_region", so we don't
			// redundantly redraw cells that will have images
			// painted over them after. Make sure to do this before
			// we check partially covered cells, since
			// "subtract_region" may also have partially covered
			// cells.
			(void)pixman_region32_subtract(&stale_region,
				&stale_region, &subtract_region);

			// Since pixels may be translucent/transparent, we must
			// redraw the cells behind the image every time, so that
			// no stale pixels remain from the previous redraw.
			if (img->fmt == IMAGE_FORMAT_RGBA)
			    (void)pixman_region32_union(&stale_region,
				    &stale_region, &old_visible_abs);
		    }

		    // The visible region is guaranteed to cover every single
		    // pixel. However when the visible region is converted to
		    // pixels, that means the resulting rectangles may be bigger
		    // than the image itself.
		    //
		    // We clamp the values in image_placement_subrect(), however
		    // that means partially covered cells will not be drawn over
		    // fully, and therefore could contain stale content. As
		    // such, subtract the minimum region from the visible region
		    // to get the resulting region containing partially covered
		    // cells that may have stale pixels still on them.
		    //
		    // Not needed for RGBA images, because we redraw the visible
		    // region everytime anyways.
		    if (img->fmt != IMAGE_FORMAT_RGBA)
		    {
			int		    min_w, min_h;
			int		    rect_w, rect_h;
			pixman_region32_t   min_region;

			image_get_cell_dimensions_min(img, &min_w, &min_h);

			// Ensure "min_region" only covers the fully covered
			// cells of the cropped region.
			rect_w = min_w - place->crop_box.x1;
			rect_h = min_h - place->crop_box.y1;

			if (rect_w > 0 && rect_h > 0)
			    // pixman_region32_init_rect() expects a non zero
			    // width and height (otherwise it outputs error
			    // message).
			    pixman_region32_init_rect(&min_region,
				    x, y, rect_w, rect_h);
			else
			    pixman_region32_init(&min_region);

			(void)pixman_region32_subtract(&min_region,
				&visible_abs, &min_region);
			(void)pixman_region32_union(&stale_region,
				&stale_region, &min_region);
			pixman_region32_fini(&min_region);
		    }

		    redraw_region(&stale_region, true, false);
		    pixman_region32_fini(&stale_region);
		}
		if (place->visible_init)
		    pixman_region32_fini(&old_visible_abs);

		pending_placements[pending_len++] = place;
		place->visible_init = true;
		place->dirty = false;
	    }
	    else
	    {
		pixman_region32_fini(&visible_region);
		pixman_region32_fini(&visible_abs);
	    }
	}

	// If this is last image, no need to union its bounds.
	if (place->next == NULL)
	    break;

	pixman_region32_union_rect(&subtract_region, &subtract_region,
		place->bounding_box.x1,
		place->bounding_box.y1 + place->row_off,
		place->bounding_box.x2 - place->bounding_box.x1,
		place->bounding_box.y2 - place->bounding_box.y1);
    }

    if (pending_len > 0)
    {
#ifdef FEAT_GUI
	// Make sure to flush any output! We write directly to the draw
	// area/whatever, meaning draw commands outputted before this (as escape
	// sequences), will still draw over the image surface if we don't flush
	// now.
	if (gui.in_use)
	    out_flush();
#endif

	for (int i = 0; i < pending_len; i++)
	    PLACEMENT_FUNC(image_backend, draw)(pending_placements[i]);
    }

    // Restore cursor position, however do not turn on the cursor, since that
    // causes it to flicker at the wrong position when using sixel. It will be
    // turned on later somewhere...
    windgoto(cur_row, cur_col);
    out_flush();

    vim_free(pending_placements);
    pixman_region32_fini(&subtract_region);
}

/*
 * Mark any images that touches the region dirty. Only used for backends that
 * blit pixels.
 */
    void
mark_dirty_region_for_images(int row, int col, int row_height, int col_width)
{
    image_placement_T	*place;
    pixman_box32_t	rect;

    if (!backend_available(false) || !PLACEMENT_FUNC(image_backend, blit)
	    || n_placements == 0)
	return;

    rect.x1 = col;
    rect.y1 = row;
    rect.x2 = col + col_width;
    rect.y2 = row + row_height;

    FOR_ALL_PLACEMENTS(place)
    {
	if (pixman_region32_contains_rectangle(&place->visible_abs, &rect)
		== PIXMAN_REGION_OUT)
	    continue;

	place->dirty = true;
	if (!updating_screen)
	    redraw_all_later(UPD_VALID);
    }
}

    void
clear_all_image_placements(void)
{
    image_placement_T *place;

    FOR_ALL_PLACEMENTS(place)
	image_placement_clear(place);
    redraw_all_later(UPD_VALID);
}

/*
 * Used for screen_ins_lines() and screen_del_lines(), because they directly
 * shift rows. Note that this does not queue a screen redraw.
 */
    void
shift_image_placements(int top, int bot, int amount)
{
    shift_top = top;
    shift_bot = bot;
    shift += amount;
}

    static image_T *
find_image(int id)
{
    image_T *img;

    FOR_ALL_IMAGES(img)
	if (img->state == IMAGE_STATE_PUBLIC && img->id == id)
	    return img;
    return NULL;
}

/*
 * Add an image using the given information in "dict". If "existing" is not
 * NULL, try updating it instead. If "find" is TRUE, then accept the "id" field.
 */
    image_T *
add_image(dict_T *dict, image_T *existing, bool find)
{
    dictitem_T	    *di;
    blob_T	    *data;
    varnumber_T     w, h;
    varnumber_T     n_pixels;
    image_format_T  fmt;

    if (find && dict_has_key(dict, "id"))
    {
	image_T *img = find_image(dict_get_number(dict, "id"));

	if (img == NULL)
	    return NULL;
	return image_ref(img);
    }

    di = dict_find(dict, (char_u *)"data", -1);
    w = dict_get_number(dict, "width");
    h = dict_get_number(dict, "height");

    if (di == NULL || di->di_tv.v_type != VAR_BLOB || w <= 0 || h <= 0)
    {
	emsg(_(e_invalid_argument));
	return NULL;
    }

    // Check for overflow
    n_pixels = w * h;
    if (w <= 0 || h <= 0 || n_pixels * IMAGE_FORMAT_RGBA > UINT_MAX)
    {
	emsg(_(e_invalid_image_dimensions));
	return NULL;
    }

    data = di->di_tv.vval.v_blob;

    if (blob_len(data) == n_pixels * 3)
	fmt = IMAGE_FORMAT_RGB;
    else if (blob_len(data) == n_pixels * 4)
	fmt = IMAGE_FORMAT_RGBA;
    else
    {
	semsg(_(e_invalid_value_for_argument_str_str), "data",
		"data length must equal width*height*3 or width*height*4");
	return NULL;
    }

    if (existing != NULL)
    {
	if (existing->width == w && existing->height == h
		&& existing->fmt == fmt)
	{
	    if (image_update(existing, data->bv_ga.ga_data) == FAIL)
		return NULL;
	    return existing;
	}
    }

    return image_new(data->bv_ga.ga_data, w, h, fmt);
}

/*
 * Return a dict containing information about the given dict. Note that
 * reference count of dict is not set. Returns NULL on failure.
 */
    dict_T *
get_image_info(image_T *img)
{
    dict_T	*dict;
    blob_T	*blob;
    int		w, h;
    dictitem_T	*di;

    dict = dict_alloc();
    if (dict == NULL)
	return NULL;

    blob = blob_alloc();
    if (blob == NULL)
    {
	dict_unref(dict);
	return NULL;
    }

    image_get_dimensions(img, &w, &h);
    (void)ga_concat_bytes(&blob->bv_ga, (char *)img->data, w * h * img->fmt);

    di = dictitem_alloc((char_u *)"data");

    if (di != NULL)
    {
	di->di_tv.v_type = VAR_BLOB;
	di->di_tv.vval.v_blob = blob;
    }
    if (di == NULL || dict_add(dict, di) == FAIL)
    {
	if (di != NULL)
	    dictitem_free(di);
	dict_unref(dict);
	blob_unref(blob);
	return NULL;
    }

    if (img->state == IMAGE_STATE_PUBLIC)
	dict_add_number(dict, "id", img->id);
    dict_add_number(dict, "width", img->width);
    dict_add_number(dict, "height", img->height);
    dict_add_number(dict, "alpha", img->fmt == IMAGE_FORMAT_RGBA);
    dict_add_string(dict, "format",
	    (char_u *)(img->fmt == IMAGE_FORMAT_RGB ? "rgb" : "rgba"));

    blob->bv_refcount = 1;

    return dict;
}

    static int
match_imageprotocol(image_backend_T *backend)
{
    int		    len = (int)STRLEN(p_ipc) + 1;
    char_u	    *buf = alloc(len);
    char_u	    *p = p_ipc;
    int		    ret = FAIL;
    image_backend_T res = IMAGE_BACKEND_NONE;

    if (buf == NULL)
	return FAIL;

    res = IMAGE_BACKEND_NONE;

    while (*p != NUL)
    {
	char_u		*colon;
	regmatch_T	regmatch;
	image_backend_T prot = IMAGE_BACKEND_NONE;

	// Isolate one comma separated item.
	(void)copy_option_part(&p, buf, len, ",");

	colon = vim_strchr(buf, ':');
	if (colon == NULL || colon == buf || colon[1] == NUL)
	    goto exit;

	*colon = NUL;

	// Note: Keep this in sync with p_ipc_protocol_values.
	if (STRCMP(colon + 1, "none") == 0)
	    prot = IMAGE_BACKEND_NONE;
	else if (STRCMP(colon + 1, "kitty") == 0)
	{
#ifdef FEAT_IMAGE_KITTY
	    prot = IMAGE_BACKEND_KITTY;
#endif
	}
	else if (STRCMP(colon + 1, "sixel") == 0)
	{
#ifdef FEAT_IMAGE_SIXEL
	    prot = IMAGE_BACKEND_SIXEL;
#endif
	}
	else
	    goto exit;

	if (prot == IMAGE_BACKEND_NONE || res != IMAGE_BACKEND_NONE)
	    continue;

	CLEAR_FIELD(regmatch);
	regmatch.rm_ic = TRUE;
	regmatch.regprog = vim_regcomp(buf, RE_MAGIC);

	if (regmatch.regprog == NULL)
	    goto exit;

	bool match = T_NAME != NULL
	    && vim_regexec(&regmatch, T_NAME, (colnr_T)0);

	vim_regfree(regmatch.regprog);
	if (match)
	    // Keep going to catch any errors
	    res = prot;
    }

    ret = OK;
    *backend = res;
exit:
    vim_free(buf);
    return ret;
}

/*
 * Update the current image backend to use depending on 'imageprotocol' and if
 * GUI is being used. Returns OK on success and FAIL on failure.
 */
    int
update_image_backend(void)
{
    image_backend_T new;

    image_T		*img;
    image_placement_T	*place;

    // Always process 'imageprotocol' option to check for errors
    if (match_imageprotocol(&new) == FAIL)
	return FAIL;

#ifdef FEAT_IMAGE_GUI
    if (gui.in_use)
	new = IMAGE_BACKEND_GUI;
#endif

    if (image_backend == new)
	return OK;

    // Must uninit the backends of every image/placement, then init the new
    // backend for each.
    FOR_ALL_IMAGES(img)
    {
	if (image_backend != IMAGE_BACKEND_NONE)
	    IMAGE_FUNC(image_backend, uninit)(img);
	img->backend_data = NULL;
	if (new != IMAGE_BACKEND_NONE)
	{
	    if (IMAGE_FUNC(new, init)(img) == FAIL)
		goto fail;
	    img->backend = new;
	}
    }
    FOR_ALL_PLACEMENTS(place)
    {
	image_placement_clear(place);
	if (image_backend != IMAGE_BACKEND_NONE && place->img != NULL)
	    PLACEMENT_FUNC(image_backend, uninit)(place);
	place->backend_data = NULL;
	if (new != IMAGE_BACKEND_NONE && place->img != NULL)
	{
	    if (PLACEMENT_FUNC(new, init)(place) == FAIL)
		goto fail;
	    place->backend = new;
	}
    }

    redraw_all_later(UPD_VALID);
    image_backend = new;

    if (new == IMAGE_BACKEND_NONE)
	set_vim_var_string(VV_IMAGEBACKEND, (char_u *)"none", -1);
    else
	set_vim_var_string(VV_IMAGEBACKEND,
		(char_u *)image_backends[image_backend].name, -1);

    return OK;
fail:
    FOR_ALL_IMAGES(img)
	if (img->backend != IMAGE_BACKEND_NONE)
	{
	    IMAGE_FUNC(img->backend, uninit)(img);
	    img->backend = IMAGE_BACKEND_NONE;
	}

    FOR_ALL_PLACEMENTS(place)
	if (img->backend != IMAGE_BACKEND_NONE)
	{
	    image_placement_clear(place);
	    PLACEMENT_FUNC(place->backend, uninit)(place);
	    place->backend = IMAGE_BACKEND_NONE;
	}

    redraw_all_later(UPD_VALID);

    image_backend = IMAGE_BACKEND_NONE;
    set_vim_var_string(VV_IMAGEBACKEND, (char_u *)"none", -1);
    emsg(_(e_changing_image_backend_failed));
    return FAIL;
}

    void
f_image_add(typval_T *argvars, typval_T *rettv)
{
    dict_T  *dict;
    image_T *img;

    if (in_vim9script() && check_for_dict_arg(argvars, 0) == FAIL)
	return;

    dict = argvars[0].vval.v_dict;

    img = add_image(dict, NULL, false);
    if (img != NULL)
	img->state = IMAGE_STATE_PUBLIC;

    rettv->v_type = VAR_NUMBER;
    rettv->vval.v_number = img == NULL ? -1 : img->id;
}

    void
f_image_discard(typval_T *argvars, typval_T *rettv UNUSED)
{
    image_T *img;

    if (in_vim9script() && check_for_number_arg(argvars, 0) == FAIL)
	return;

    img = find_image(argvars[0].vval.v_number);

    if (img != NULL)
    {
	img->state = IMAGE_STATE_PRIVATE;
	image_unref(img);
    }
    else
	semsg(_(e_image_id_nr_does_not_exist), argvars[0].vval.v_number);
}

    void
f_image_info(typval_T *argvars, typval_T *rettv)
{
    image_T *img;
    list_T  *list;
    dict_T  *dict;

    if (in_vim9script() && check_for_opt_number_arg(argvars, 0) == FAIL)
	return;

    list = list_alloc();
    if (list == NULL)
	return;

    if (argvars[0].v_type == VAR_NUMBER)
    {
	img = find_image(argvars[0].vval.v_number);

	if (img == NULL)
	{
	    semsg(_(e_image_id_nr_does_not_exist), argvars[0].vval.v_number);
	    list_unref(list);
	    return;
	}

	dict = get_image_info(img);
	if (dict == NULL || list_append_dict(list, dict) == FAIL)
	{
	    if (dict != NULL)
		dict_unref(dict);
	    list_unref(list);
	    return;
	}
    }
    else
    {
	FOR_ALL_IMAGES(img)
	{
	    if (img->state == IMAGE_STATE_PRIVATE)
		continue;

	    dict = get_image_info(img);

	    if (dict == NULL || list_append_dict(list, dict) == FAIL)
	    {
		if (dict != NULL)
		    dict_unref(dict);
		list_unref(list);
		return;
	    }
	}
    }

    list->lv_refcount = 1;

    rettv->v_type = VAR_LIST;
    rettv->vval.v_list = list;
}

#endif // FEAT_IMAGE
