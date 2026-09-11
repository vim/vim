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

//
static pixman_region32_t    dirty_region; // In cells
static bool		    dirty_region_init = false;
static bool		    dirty_region_finalized = false;

#define FOR_ALL_IMAGES(v) for ((v) = images; (v) != NULL; (v) = (v)->next)
#define FOR_ALL_PLACEMENTS(v) \
    for ((v) = placements; (v) != NULL; (v) = (v)->next)

#define IMAGE_FUNC(b, n) (image_backends[b].image.n)
#define PLACEMENT_FUNC(b, n) (image_backends[b].placement.n)

static void redraw_region(pixman_region32_t *region);

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
    if (dirty_region_init)
	pixman_region32_fini(&dirty_region);
    dirty_region_finalized = true;
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

    if (!backend_available(true))
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

    img->backend = image_backend;

    if (IMAGE_FUNC(image_backend, init)(img) == FAIL)
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
    if (backend_available(false))
	IMAGE_FUNC(image_backend, uninit)(img);

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

/*
 * Get image width and height in cells, rounding up so that every pixel is
 * covered.
 */
    void
image_get_cell_dimensions(image_T *img, int *cw, int *ch)
{
    int w = pixman_image_get_width(img->image);
    int h = pixman_image_get_height(img->image);

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
    int w = pixman_image_get_width(img->image);
    int h = pixman_image_get_height(img->image);

    *cw = w / cell_width;
    *ch = h / cell_height;
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

    if (!backend_available(true))
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

    place->backend = image_backend;

    if (PLACEMENT_FUNC(image_backend, init)(place) == FAIL)
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
    if (backend_available(false) && place->img != NULL)
    {
	if (place->visible_init && PLACEMENT_FUNC(image_backend, blit))
	    redraw_region(&place->visible_abs);
	PLACEMENT_FUNC(image_backend, clear)(place);
	place->dirty = true;
	redraw_all_later(UPD_VALID);
    }
}

    void
image_placement_free(image_placement_T *place)
{
    if (backend_available(false))
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
	int		    col,
	int		    row_height,
	int		    col_width)
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

    void
image_placement_subrect(
	image_placement_T   *place,
	pixman_box32_t	    rect,
	int		    *row,
	int		    *col,
	int		    *x,
	int		    *y,
	int		    *w,
	int		    *h)
{
    int iw, ih;

    image_get_dimensions(place->img, &iw, &ih);

    *row = place->row + rect.y1;
    *col = place->col + rect.x1;

    *x = (rect.x1 + place->crop_box.x1) * cell_width;
    *y = (rect.y1 + place->crop_box.y1) * cell_height;
    *w = (rect.x2 - rect.x1) * cell_width;
    *h = (rect.y2 - rect.y1) * cell_height;

    // Make that all the values are valid. "w" and "h" can especially be
    // invalid, because the image dimensions (in pixels) are converted to cells,
    // rounding *up*.
    *row = MIN(*row, Rows);
    *col = MIN(*col, Columns);

    *x = MIN(iw, MAX(*x, 0));
    *y = MIN(ih, MAX(*y, 0));
    *w = MIN(iw, *w);
    *h = MIN(ih, *h);
}


/*
 * Redraw the cells in the given region. This is only relevant for image
 * backends that blit pixels.
 */
    static void
redraw_region(pixman_region32_t *region)
{
    pixman_box32_t  *rects;
    int		    n_rects;

    rects = pixman_region32_rectangles(region, &n_rects);
    if (rects == NULL)
	return;

    for (int i = 0; i < n_rects; i++)
    {
	pixman_box32_t rect = rects[i];

	// If we are currently updating the screen, redraw the characters now.
	// Otherwise defer it later
	if (updating_screen)
	{
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
    if (!updating_screen)
	redraw_all_later(UPD_VALID);
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
    int			pending_len = 0;

    if (!backend_available(false))
	return;

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
    FOR_ALL_PLACEMENTS(place)
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
	    pixman_region32_t	visible_abs;
	    pixman_region32_t	dirty;
	    bool		has_dirty_cells;

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

	    // Check if visible region touches the current global dirty region.
	    // If it does, then redraw the image.
	    pixman_region32_init(&dirty);
	    (void)pixman_region32_intersect(&dirty, &visible_abs, &dirty_region);
	    has_dirty_cells = pixman_region32_not_empty(&dirty);
	    pixman_region32_fini(&dirty);

	    // Only redraw the image if it has changed (or if we haven't drawn
	    // it yet).
	    if (place->dirty || has_dirty_cells
		    || (place->visible_init && !pixman_region32_equal(
			    &visible_region, &place->visible)))
	    {
		if (place->visible_init)
		    pixman_region32_fini(&place->visible);
		place->visible = visible_region;

		if (PLACEMENT_FUNC(image_backend, blit) && place->visible_init)
		{
		    // Must redraw the stale regions that will not be composited
		    // over (for this specific image).
		    pixman_region32_t stale_region;

		    pixman_region32_init(&stale_region);

		    if (pixman_region32_subtract(&stale_region,
				&place->visible_abs, &visible_abs))
		    {
			int		    min_w, min_h;
			pixman_region32_t   min_region;

			// Also subtract "subtract_region", so we don't
			// redundantly redraw cells that will have images
			// painted over them after.
			(void)pixman_region32_subtract(&stale_region,
				&stale_region, &subtract_region);

			// The visible region is guaranteed to cover every
			// single pixel. However if the visible region is
			// converted to pixels, that means the resulting
			// rectangles may be bigger than the image itself.
			//
			// We clamp the values in image_placement_subrect(),
			// however that means partially covered cells will not
			// be drawn over, and therefore could contain stale
			// content. As such, subtract the minimum region
			// from the visible region to get the resulting region
			// containing partially covered cells that may have
			// stale pixels still on them.
			image_get_cell_dimensions_min(img, &min_w, &min_h);

			pixman_region32_init_rect(&min_region,
				x, y, min_w, min_h);

			(void)pixman_region32_subtract(&min_region,
				&visible_abs, &min_region);
			(void)pixman_region32_union(&stale_region,
				&stale_region, &min_region);
			pixman_region32_fini(&min_region);

			redraw_region(&stale_region);
		    }
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
	    PLACEMENT_FUNC(image_backend, draw)(pending_placements[i]);
    }

    vim_free(pending_placements);
    pixman_region32_fini(&subtract_region);

    if (dirty_region_init)
	pixman_region32_clear(&dirty_region);
}

/*
 * Mark the given region as dirty (text has been drawn over it).
 */
    void
mark_dirty_region_for_images(int row, int col, int row_height, int col_width)
{
    // This function may be called in mch_exit().
    if (dirty_region_finalized || n_placements == 0)
	return;
    if (!dirty_region_init)
    {
	pixman_region32_init(&dirty_region);
	dirty_region_init = true;
    }

    pixman_region32_union_rect(&dirty_region, &dirty_region,
	    col, row, col_width, row_height);
    if (!updating_screen)
	redraw_all_later(UPD_VALID);
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

    static int
match_imageprotocol(image_backend_T *backend)
{
    int		    len = (int)STRLEN(p_ipc) + 1;
    char_u	    *buf = alloc(len);
    char_u	    *p = p_ipc;
    int		    ret = FAIL;

    if (buf == NULL)
	return FAIL;

    *backend = IMAGE_BACKEND_NONE;

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

	if (prot == IMAGE_BACKEND_NONE)
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
	{
	    *backend = prot;
	    break;
	}

    }

    ret = OK;
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

#ifdef FEAT_IMAGE_GUI
    if (gui.in_use)
	new = IMAGE_BACKEND_GUI;
    else
#endif
    {
	if (match_imageprotocol(&new) == FAIL)
	    return FAIL;
    }

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
	if (image_backend != IMAGE_BACKEND_NONE)
	    PLACEMENT_FUNC(image_backend, uninit)(place);
	place->backend_data = NULL;
	if (new != IMAGE_BACKEND_NONE)
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

#endif // FEAT_IMAGE
