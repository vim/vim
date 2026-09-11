/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read a list of people who contributed.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#ifdef FEAT_IMAGE

struct
{
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
	.image = {
	    .init = NULL,
	    .uninit = NULL
	},
	.placement = {
	    .init = NULL,
	    .uninit = NULL,
	    .draw = NULL,
	    .clear = NULL,
# ifdef FEAT_GUI_GTK
#  ifdef USE_GTK4
	    .blit = false
#  else
		.blit = true
#  endif
# elif FEAT_GUI_MSWIN
		.blit = true
# else
		.blit = false
# endif
	}
    },
#endif
#ifdef FEAT_IMAGE_KITTY
    [IMAGE_BACKEND_KITTY] = {
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
static image_backend_T image_backend = IMAGE_BACKEND_KITTY; // Temporary

static image_T *images = NULL;
// Sorted from highest zindex to lowest zindex
static image_placement_T    *placements = NULL;
static int		    n_placements = 0;

static void invalidate_region(pixman_region32_t *region);

/*
 * Return true if the there image backend is ready, otherwise emit error.
 */
    static bool
backend_available(void)
{
    if (image_backend == IMAGE_BACKEND_NONE)
    {
	emsg(_(e_no_image_backend_available));
	return false;
    }
    return true;
}

    static void
pixman_destroy_func(pixman_image_t *image UNUSED, void *data)
{
    vim_free(data);
}

/*
 * Create a new image using the given data (creates a copy). Returns NULL on
 * failure.
 */
    image_T *
image_new(uint8_t *data, int width, int height, image_format_T fmt)
{
    image_T	*img;
    uint32_t	*copy;
    static int	id;

    if (!backend_available())
	return NULL;

    img = ALLOC_CLEAR_ONE(image_T);
    if (img == NULL)
	return NULL;

    copy = (uint32_t *)vim_memsave(data, (size_t)width * height * fmt);
    if (copy == NULL)
    {
	vim_free(img);
	return NULL;
    }

    img->image = pixman_image_create_bits(
	    fmt == IMAGE_FORMAT_RGB ? PIXMAN_r8g8b8 : PIXMAN_r8g8b8a8,
	    width, height, copy, width * fmt);
    if (img->image == NULL)
    {
	vim_free(copy);
	vim_free(img);
	return NULL;
    }

    // pixman_image_t does not take ownership of the data
    pixman_image_set_destroy_function(img->image, pixman_destroy_func, copy);

    img->fmt = fmt;

    // Mix in PID to prevent ID collisions when using kitty graphics protocol
    img->id = ((((int)mch_get_pid() & 0x7fff) + 1) << 16) | (id++ & 0xffff);
    img->refcount = 1;

    if (image_backends[image_backend].image.init(img) == FAIL)
    {
	pixman_image_unref(img->image);
	vim_free(img);
	return NULL;
    }

    if (images != NULL)
	images->next = img;
    img->prev = images;
    images = img;

    return img;
}

    static void
image_free(image_T *img)
{
    if (backend_available())
	image_backends[image_backend].image.uninit(img);

    if (img->prev != NULL)
	img->prev->next = img->next;
    if (img->next != NULL)
	img->next->prev = img->prev;
    if (images == img)
	images = img->prev;

    pixman_image_unref(img->image);
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

    static void
pixels2cells(int x, int y, int *rx, int *ry)
{
    // Always round upwards
    *rx = (x + cell_width - 1) / cell_width;
    *ry = (y + cell_height - 1) / cell_height;
}

/*
 * Get image width and height in cells.
 */
    void
image_get_cell_dimensions(image_T *img, int *cw, int *ch)
{
    pixels2cells(pixman_image_get_width(img->image),
	    pixman_image_get_height(img->image), cw, ch);
}

    void
image_get_dimensions(image_T *img, int *w, int *h)
{
    *w = pixman_image_get_width(img->image);
    *h = pixman_image_get_height(img->image);
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

    static void
image_placement_link(image_placement_T *place)
{
    image_placement_T *p = placements;
    image_placement_T *prev = NULL;

    // Add image before the image with the same or lower zindex.
    while (p != NULL)
    {
	if (p->zindex <= place->zindex)
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
 * bounding box will cover the entire image. Returns NULL on failure.
 */
    image_placement_T *
image_placement_new(image_T *img)
{
    image_placement_T	*place;
    static int		id = 1; // Kitty placements id must be > 1

    if (!backend_available())
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

    // Bounding box is in cells
    image_get_cell_dimensions(img,
	    &place->bounding_box.x2, &place->bounding_box.y2);
    place->crop_box.x2 = place->bounding_box.x2;
    place->crop_box.y2 = place->bounding_box.y2;

    if (image_backends[image_backend].placement.init(place) == FAIL)
    {
	id -= 1000;
	vim_free(place);
	return NULL;
    }

    image_placement_link(place);

    return place;
}

    void
image_placement_clear(image_placement_T *place)
{
    if (backend_available() && place->img != NULL)
    {
	if (place->visible_init
		&& image_backends[image_backend].placement.blit)
	    invalidate_region(&place->visible_abs);
	image_backends[image_backend].placement.clear(place);
	place->dirty = true;
	redraw_all_later(UPD_VALID);
    }
}

    void
image_placement_free(image_placement_T *place)
{
    if (backend_available())
    {
	image_placement_clear(place);
	image_backends[image_backend].placement.uninit(place);
    }

    if (place->visible_init)
    {
	pixman_region32_fini(&place->visible);
	pixman_region32_fini(&place->visible_abs);
    }
    image_placement_unlink(place);

    image_unref(place->img);
    vim_free(place);
}

    void
image_placement_set_zindex(image_placement_T *place, int zindex)
{
    if (place->zindex == zindex)
	return;
    place->zindex = zindex;
    place->dirty = true;

    // Must re-add the placement back so it is in the correct order
    image_placement_unlink(place);
    image_placement_link(place);
}

/*
 * Make sure to also update the bounding box!
 */
    void
image_placement_set_position(image_placement_T *place, int row, int col)
{
    if (place->row == row && place->col == col)
	return;
    place->row = row;
    place->col = col;
    place->dirty = true;
}

    void
image_placement_set_crop(image_placement_T *place, int x, int y, int w, int h)
{
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
image_placement_do_draw(image_placement_T *place)
{
    place->draw = true;
}

    void
image_placement_set_bounding_box(
	image_placement_T   *place,
	int		    row,
	int 		    col,
	int 		    row_height,
	int 		    col_width)
{
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
 *
 */
    static void
invalidate_region(pixman_region32_t *region)
{
    pixman_box32_t  *rects;
    int		    n_rects;

    rects = pixman_region32_rectangles(region, &n_rects);
    if (rects == NULL)
	return;

    for (int i = 0; i < n_rects; i++)
    {
	pixman_box32_t rect = rects[i];

	for (int r = rect.y1; r < rect.y2; r++)
	{
	    if (r >= screen_Rows)
		break;

	    for (int c = rect.x1; c < rect.x2; c++)
	    {
		if (c >= screen_Columns)
		    break;

		screen_char(LineOffset[r] + c, r, c);
	    }
	}
    }
}

/*
 * Draw all image placements to the screen. This should be done after all text
 * have been drawn to the screen.
 */
    void
draw_image_placements(void)
{
    pixman_region32_t	subtract_region; // In pixels
    image_placement_T	**pending_placements;
    int			pending_len = 0;

    if (n_placements == 0)
	return;

    pending_placements = ALLOC_CLEAR_MULT(image_placement_T *, n_placements);
    if (pending_placements == NULL)
	return;

    pixman_region32_init(&subtract_region);

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
    for (image_placement_T *place = placements;
	    place != NULL;
	    place = place->next)
    {
	image_T *img = place->img;

	pixman_region32_t   image_region;
	pixman_region32_t   visible_region;

	int x, y;

	if (!place->draw)
	    continue;
	place->draw = false;

	if (img != NULL)
	{
	    pixman_region32_t visible_abs;

	    x = place->col;
	    y = place->row;

	    // Don't add the crop_box x1 and y1, because "row" and "col" use the
	    // top left of the final cropped image.
	    pixman_region32_init_rect(&image_region,
		    x, y,
		    place->crop_box.x2 - place->crop_box.x1,
		    place->crop_box.y2 - place->crop_box.y1);

	    pixman_region32_init(&visible_region);
	    pixman_region32_init(&visible_abs);

	    if (!pixman_region32_subtract(&visible_region,
			&image_region, &subtract_region)
		    || !pixman_region32_copy(&visible_abs, &visible_region))
	    {
		pixman_region32_fini(&visible_region);
		continue;
	    }

	    // The visible region is in absolute coordinates, must convert it
	    // into image relative coordinates.
	    pixman_region32_translate(&visible_region, -x, -y);

	    // Only redraw the image if it has changed (or if we haven't drawn
	    // it yet).
	    if (place->dirty || (place->visible_init
			&& !pixman_region32_equal(
			    &visible_region, &place->visible)))
	    {
		if (place->visible_init)
		    pixman_region32_fini(&place->visible);
		place->visible = visible_region;

		if (image_backends[image_backend].placement.blit
			&& place->visible_init)
		{
		    // Must redraw the stale regions that will not be composited
		    // over (for this specific image).
		    pixman_region32_t stale_region;

		    pixman_region32_init(&stale_region);

		    if (pixman_region32_subtract(&stale_region,
				&place->visible_abs, &visible_abs))
			invalidate_region(&stale_region);
		    pixman_region32_fini(&stale_region);
		}
		if (place->visible_init)
		    pixman_region32_fini(&place->visible_abs);
		place->visible_abs = visible_abs;

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

	pixman_region32_union_rect(&subtract_region, &subtract_region,
		place->bounding_box.x1, place->bounding_box.y1,
		place->bounding_box.x2 - place->bounding_box.x1,
		place->bounding_box.y2 - place->bounding_box.y1);
    }

    if (pending_len > 0)
    {
	for (int i = 0; i < pending_len; i++)
	    image_backends[image_backend].placement.draw(pending_placements[i]);
    }

    vim_free(pending_placements);
    pixman_region32_fini(&subtract_region);
}

/*
 * Dirty all image placements. If "only_blit" is true, then only clear if the
 * current image backend blits to the screen.
 */
    void
dirty_image_placements(bool only_blit)
{
    if (only_blit && !image_backends[image_backend].placement.blit)
	return;
    for (image_placement_T *place = placements;
	    place != NULL;
	    place = place->next)
	place->dirty = true;
}

/*
 * Add an image using the given information in "dict"
 */
    image_T *
add_image(dict_T *dict)
{
    dictitem_T	    *di;
    blob_T	    *data;
    varnumber_T     w, h;
    varnumber_T     n_pixels;
    image_format_T  fmt;

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

    return image_new(data->bv_ga.ga_data, w, h, fmt);
}

#endif // FEAT_IMAGE
