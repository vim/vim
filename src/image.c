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

static image_T *images = NULL;
static image_placement_T *placements = NULL;
static image_backend_T backend = IMAGE_BACKEND_KITTY; // Temporary TODO

# define FOR_ALL_IMAGES(img) \
    for ((img) = images; (img) != NULL; (img) = (img)->next)
# define FOR_ALL_PLACEMENTS(place) \
    for ((place) = placements; (place) != NULL; (place) = (place)->next)

/*
 * Allocate new image structure for backend. Returns NULL on failure.
 */
    static image_T *
image_backend_alloc(void)
{
    switch (backend)
    {
	case IMAGE_BACKEND_CAIRO:
	    return NULL;
	case IMAGE_BACKEND_GDI:
	    return NULL;
	case IMAGE_BACKEND_GDK:
	    return NULL;
	case IMAGE_BACKEND_KITTY:
#ifdef FEAT_IMAGE_KITTY
	    return image_kitty_alloc();
#endif
	case IMAGE_BACKEND_SIXEL:
	    return NULL;
	default:
	    emsg(_(e_no_image_backend_available));
	    return NULL;
    }
}

/*
 * Initialize the image backend
 */
    static void
image_backend_init(image_T *img)
{
    switch (backend)
    {
	case IMAGE_BACKEND_CAIRO:
	    break;
	case IMAGE_BACKEND_GDI:
	    break;
	case IMAGE_BACKEND_GDK:
	    break;
	case IMAGE_BACKEND_KITTY:
#ifdef FEAT_IMAGE_KITTY
	    image_kitty_init(img);
#endif
	    break;
	case IMAGE_BACKEND_SIXEL:
	    break;
	default:
	    emsg(_(e_no_image_backend_available));
	    break;
    }
}

/*
 * Uninitialize the image backend. Note that this is not guaranteed to clear any
 * existing placements for this image.
 */
    static void
image_backend_uninit(image_T *img)
{
    switch (backend)
    {
	case IMAGE_BACKEND_CAIRO:
	    break;
	case IMAGE_BACKEND_GDI:
	    break;
	case IMAGE_BACKEND_GDK:
	    break;
	case IMAGE_BACKEND_KITTY:
#ifdef FEAT_IMAGE_KITTY
	    image_kitty_uninit(img);
#endif
	    break;
	case IMAGE_BACKEND_SIXEL:
	    break;
	default:
	    emsg(_(e_no_image_backend_available));
	    break;
    }
}

/*
 * Create a new image with the given data (creates a copy). If there is an
 * existing images that is the exact same, then use that (add a new reference).
 * Returns NULL on failure.
 */
    image_T *
image_new(uint8_t *data, imgpx_T width, imgpx_T height, image_format_T fmt)
{
    image_T *img;

    FOR_ALL_IMAGES(img)
	if (img->width == width && img->height == height &&
		img->fmt == fmt &&
		memcmp(img->data, data, fmt * width * height) == 0)
	    return image_ref(img);

    img = image_backend_alloc();
    if (img == NULL)
	return NULL;

    img->data = vim_memsave(data, width * height * fmt);
    if (img->data == NULL)
    {
	semsg(_(e_out_of_memory_allocating_nr_bytes), width * height * fmt);
	vim_free(img);
	return NULL;
    }

    img->width = width;
    img->height = height;
    img->fmt = fmt;

    // Mix in PID to prevent ID collisions when using kitty graphics protocol
    static int id;

    img->id = ((((int)mch_get_pid() & 0x7fff) + 1) << 16) | (id++ & 0xffff);
    img->refcount = 1;

    if (images != NULL)
	images->next = img;
    img->prev = images;
    images = img;

    image_backend_init(img);

    return img;
}

    image_T *
image_ref(image_T *img)
{
    img->refcount++;
    return img;
}

    static void
image_free(image_T *img)
{
    image_backend_uninit(img);

    if (img->prev != NULL)
	img->prev->next = img->next;
    if (img->next != NULL)
	img->next->prev = img->prev;
    if (images == img)
	images = img->prev;

    vim_free(img->data);
    vim_free(img);
}

    void
image_unref(image_T *img)
{
    if (--img->refcount <= 0)
	image_free(img);
}

/*
 * Get image width and height in cells.
 */
    void
image_cell_size(image_T *img, colnr_T *cw, linenr_T *ch)
{
    // Always round upwards
    *cw = (img->width + cell_width - 1) / cell_width;
    *ch = (img->height + cell_height- 1) / cell_height;
}

/*
 * Draw the image placement to the screen using the image backend
 */
    static void
image_placement_backend_draw(image_placement_T *place)
{
    switch (backend)
    {
	case IMAGE_BACKEND_CAIRO:
	    break;
	case IMAGE_BACKEND_GDI:
	    break;
	case IMAGE_BACKEND_GDK:
	    break;
	case IMAGE_BACKEND_KITTY:
#ifdef FEAT_IMAGE_KITTY
	    image_kitty_draw(place->img, &place->geometry, place->id);
#endif
	    break;
	case IMAGE_BACKEND_SIXEL:
	    break;
	default:
	    emsg(_(e_no_image_backend_available));
	    break;
    }
}

/*
 * Clear the image placement from the screen using the image backend
 */
    static void
image_placement_backend_clear(image_placement_T *place)
{
    switch (backend)
    {
	case IMAGE_BACKEND_CAIRO:
	    break;
	case IMAGE_BACKEND_GDI:
	    break;
	case IMAGE_BACKEND_GDK:
	    break;
	case IMAGE_BACKEND_KITTY:
#ifdef FEAT_IMAGE_KITTY
	    image_kitty_clear(place->img, place->id);
#endif
	    break;
	case IMAGE_BACKEND_SIXEL:
	    break;
	default:
	    emsg(_(e_no_image_backend_available));
	    break;
    }
}

/*
 * Initialize geometry to default values (top left corner, no crop).
 */
    static void
image_geometry_init(image_geometry_T *geometry, imgpx_T width, imgpx_T height)
{
    geometry->col = geometry->row = 0;
    geometry->zindex = 0;

    geometry->crop.x = geometry->crop.y = 0;
    geometry->crop.width = width;
    geometry->crop.height = height;
}

/*
 * Create a new placement for the image. Note that this takes ownership of the
 * image. Returns NULL on failure.
 */
    image_placement_T *
image_placement_new(image_T *img)
{
    image_placement_T *place = ALLOC_CLEAR_ONE(image_placement_T);

    if (place == NULL)
	return NULL;

    // Kitty graphics protocol expects placement id to be > 0
    static int id = 1;

    place->id = id++;
    place->img = img;
    place->dirty = true;

    image_geometry_init(&place->geometry, img->width, img->height);

    if (placements != NULL)
	placements->next = place;
    place->prev = placements;
    placements = place;

    return place;
}

    void
image_placement_free(image_placement_T *place)
{
    if (place->prev != NULL)
	place->prev->next = place->next;
    if (place->next != NULL)
	place->next->prev = place->prev;
    if (placements == place)
	placements = place->prev;

    image_placement_clear(place);
    image_unref(place->img);
    free(place);
}

    void
image_placement_dirty(image_placement_T *place)
{
    place->dirty = true;
}

/*
 * Clear the image placement from the screen. This will make the placement dirty
 */
    void
image_placement_clear(image_placement_T *place)
{
    image_placement_backend_clear(place);
    image_placement_dirty(place);
}

/*
 * Draw the image placement to the screen, if it is dirty.
 */
    void
image_placement_draw(image_placement_T *place)
{
    if (!place->dirty)
	return;

    image_placement_backend_draw(place);
    place->dirty = false;
}

    void
image_placement_set_z(image_placement_T *place, int zindex)
{
    place->geometry.zindex = zindex;
    image_placement_dirty(place);
}

    void
image_placement_set_position(
	image_placement_T   *place,
	linenr_T	    row,
	colnr_T		    col)
{
    place->geometry.row = row;
    place->geometry.col = col;
    image_placement_dirty(place);
}

    void
image_placement_crop(
	image_placement_T   *place,
	imgpx_T		    x,
	imgpx_T		    y,
	imgpx_T		    width,
	imgpx_T		    height)
{
    place->geometry.crop.x = x;
    place->geometry.crop.y = y;
    place->geometry.crop.width = width;
    place->geometry.crop.height = height;
    image_placement_dirty(place);
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
