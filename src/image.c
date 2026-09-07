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

typedef struct
{
    const char	*name;
    bool	available;

    struct
    {
	image_T *(*alloc)(void);
	void (*init)(image_T *);
	void (*uninit)(image_T *); // Note that this is not guaranteed to clear
				   // all image placements from the screen.
    } image;

    struct
    {
	image_placement_T *(*alloc)(void);
	void (*init)(image_placement_T *);
	void (*uninit)(image_placement_T *);
	void (*draw)(image_placement_T *);
	void (*clear)(image_placement_T *);
    } placement;
} image_backend_handler_T;

static image_backend_handler_T backends[] = {
    [IMAGE_BACKEND_CAIRO] = {
	.name = "cairo",
    },
    [IMAGE_BACKEND_GDI] = {
	.name = "gdi",
    },
    [IMAGE_BACKEND_GDK] = {
	.name = "gdk",
    },
    [IMAGE_BACKEND_KITTY] = {
	.name = "kitty",
#ifdef FEAT_IMAGE_KITTY
	.available = true,
	.image = {
	    .alloc = image_kitty_alloc,
	    .init = image_kitty_init,
	    .uninit = image_kitty_uninit
	},
	.placement = {
	    .alloc = image_placement_kitty_alloc,
	    .init = image_placement_kitty_init,
	    .uninit = image_placement_kitty_uninit,
	    .draw = image_placement_kitty_draw,
	    .clear = image_placement_kitty_clear
	}
#else
	.available = false
#endif
    },
    [IMAGE_BACKEND_SIXEL] = {
	.name = "sixel",
#ifdef FEAT_IMAGE_SIXEL
	.available = true,
	.image = {
	    .alloc = image_sixel_alloc,
	    .init = image_sixel_init,
	    .uninit = image_sixel_uninit
	},
	.placement = {
	    .alloc = image_placement_sixel_alloc,
	    .init = image_placement_sixel_init,
	    .uninit = image_placement_sixel_uninit,
	    .draw = image_placement_sixel_draw,
	    .clear = image_placement_sixel_clear
	}
#else
	.available = false
#endif
    },
    [IMAGE_BACKEND_NONE] = {
	.name = "none",
	.available = false
    }
};

static image_T *images = NULL;
static image_placement_T *placements = NULL;
static image_backend_T image_backend = IMAGE_BACKEND_NONE;

#define IMG_FUNC(t, f) (backends[t].image.f)
#define PLACE_FUNC(t, f) (backends[t].placement.f)

# define FOR_ALL_IMAGES(img) \
    for ((img) = images; (img) != NULL; (img) = (img)->next)
# define FOR_ALL_PLACEMENTS(place) \
    for ((place) = placements; (place) != NULL; (place) = (place)->next)

/*
 * Return true if the current image backend is available.
 */
    static bool
backend_avail(void)
{
    bool avail = backends[image_backend].available;

    if (!avail)
	emsg(_(e_no_image_backend_available));
    return avail;
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

    if (!backend_avail())
	return NULL;

    FOR_ALL_IMAGES(img)
	if (img->width == width && img->height == height &&
		img->fmt == fmt &&
		memcmp(img->data, data, (size_t)width * height * fmt) == 0)
	    return image_ref(img);

    img = IMG_FUNC(image_backend, alloc)();
    if (img == NULL)
	return NULL;

    img->data = vim_memsave(data, (size_t)width * height * fmt);
    if (img->data == NULL)
    {
	semsg(_(e_out_of_memory_allocating_nr_bytes),
		(size_t)width * height * fmt);
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

    IMG_FUNC(image_backend, init)(img);

    return img;
}

    image_T *
image_ref(image_T *img)
{
    img->refcount++;
    return img;
}

    static void
image_free_struct(image_T *img)
{
    vim_free(img);
}

    static void
image_free(image_T *img)
{
    if (!backend_avail())
	return;

    IMG_FUNC(image_backend, uninit)(img);

    if (img->prev != NULL)
	img->prev->next = img->next;
    if (img->next != NULL)
	img->next->prev = img->prev;
    if (images == img)
	images = img->prev;

    vim_free(img->data);
    image_free_struct(img);
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
    image_placement_T *place;

    if (!backend_avail())
	return NULL;

    place = PLACE_FUNC(image_backend, alloc)();
    if (place == NULL)
	return NULL;

    // Kitty graphics protocol expects placement id to be > 0
    static int id = 1;

    place->id = id++;
    place->img = img;
    place->dirty = true;

    image_geometry_init(&place->geometry, img->width, img->height);

    PLACE_FUNC(image_backend, init)(place);

    if (placements != NULL)
	placements->next = place;
    place->prev = placements;
    placements = place;

    return place;
}

    void
image_placement_free(image_placement_T *place)
{
    if (!backend_avail())
	return;

    if (place->prev != NULL)
	place->prev->next = place->next;
    if (place->next != NULL)
	place->next->prev = place->prev;
    if (placements == place)
	placements = place->prev;

    // This should clear placement from screen as well.
    PLACE_FUNC(image_backend, uninit)(place);
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
    if (!backend_avail())
	return;
    PLACE_FUNC(image_backend, clear)(place);
    image_placement_dirty(place);
}

/*
 * Draw the image placement to the screen, if it is dirty.
 */
    void
image_placement_draw(image_placement_T *place)
{
    if (!place->dirty || !backend_avail())
	return;

    PLACE_FUNC(image_backend, draw)(place);
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
	char_u *colon;
	image_backend_T prot;

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
	    prot = IMAGE_BACKEND_KITTY;
	else if (STRCMP(colon + 1, "sixel") == 0)
	    prot = IMAGE_BACKEND_SIXEL;
	else
	    goto exit;

	regmatch_T regmatch;
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
    int ret = OK;

    if (image_backend == IMAGE_BACKEND_CAIRO
	    || image_backend == IMAGE_BACKEND_GDI
	    || image_backend == IMAGE_BACKEND_GDK)
	return OK;

    // If we are switching from a different image backend, must clear the images
    // created with that backend first. This is important when switching from
    // terminal to GUI via `:gui`
    if (image_backend != IMAGE_BACKEND_NONE)
    {
	image_T		    *img;
	image_placement_T   *place;

	// Delete placements first, since they may depend on their associated
	// images.
	FOR_ALL_PLACEMENTS(place)
	    PLACE_FUNC(image_backend, uninit)(place);

	FOR_ALL_IMAGES(img)
	    IMG_FUNC(image_backend, uninit)(img);
    }

#ifdef FEAT_GUI
    if (gui.in_use)
    {
# if defined(FEAT_IMAGE_CAIRO)
	image_backend = IMAGE_BACKEND_CAIRO;
# elif defined(FEAT_IMAGE_GDI)
	image_backend = IMAGE_BACKEND_GDI;
# elif defined(FEAT_IMAGE_GDK)
	image_backend = IMAGE_BACKEND_GDK;
# endif
	goto exit;
    }
#endif

    // Do nothing if switch from a terminal backend to another terminal backend
    // (kitty <-> sixel). Theres nothing stopping us from being able to switch
    // image protocols in place, however it would take a bit of work with little
    // benefit.
    if (image_backend != IMAGE_BACKEND_NONE)
	return OK;

    ret = match_imageprotocol(&image_backend);

#ifdef FEAT_GUI
exit:
#endif
#ifdef FEAT_EVAL
    set_vim_var_string(VV_IMAGEBACKEND,
	    (char_u *)backends[image_backend].name, -1);
#endif
    return ret;
}

#endif // FEAT_IMAGE
