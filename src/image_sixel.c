/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read a list of people who contributed.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#ifdef FEAT_IMAGE_SIXEL

#include <sixel.h>

typedef struct
{
    image_T base;

    // Palette for this image
    sixel_dither_t *dither;
    int		    sixel_fmt;
} image_sixel_T;

typedef struct
{
    image_placement_T base;

    // Cached sixel sequence
    char_u  *cache;
    int	    cache_len;

    // Crop used for the previous redraw. If this has been changed, the cache is
    // invalidated and the image is re-encoded.
    image_crop_T crop;
} image_placement_sixel_T;

static sixel_allocator_t    *sixel_allocator;
static sixel_output_t	    *sixel_output;
static garray_T		    sixel_buf; // Temporary buffer used to store sixel
				       // sequence.

#define IMG(img) ((image_sixel_T *)(img))
#define PLACE(place) ((image_placement_sixel_T *)(place))

    static int
sixel_write(char *data, int size, void *udata UNUSED)
{
    ga_concat_len(&sixel_buf, (char_u *)data, (size_t)size);

    // No idea what the return value is supposed to be. Looking through
    // libsixel source code, it seems to be unused?
    return 0;
}

/*
 * Initialize global sixel state if it hasn't been initialized. Returns FAIL on
 * failure.
 */
    static int
sixel_init(void)
{
    if (sixel_allocator == NULL)
    {
	if (sixel_allocator_new(&sixel_allocator, alloc, calloc, realloc,
		    vim_free) == SIXEL_FALSE)
	    return FAIL;
    }

    if (sixel_output == NULL)
    {
	if (sixel_output_new(&sixel_output, sixel_write, NULL,
		    sixel_allocator) == SIXEL_FALSE)
	    return FAIL;
    }

    return OK;
}

    void
sixel_uninit(void)
{
    if (sixel_allocator != NULL)
	sixel_allocator_unref(sixel_allocator);
    if (sixel_output != NULL)
	sixel_output_unref(sixel_output);
}

    image_T *
image_sixel_alloc(void)
{
    return (image_T *)ALLOC_CLEAR_ONE(image_sixel_T);
}

    void
image_sixel_init(image_T *img)
{
    if (sixel_init() == FAIL)
	return;

    switch (img->fmt)
    {
	case IMAGE_FORMAT_RGB:
	    IMG(img)->sixel_fmt = SIXEL_PIXELFORMAT_RGB888;
	    break;
	case IMAGE_FORMAT_RGBA:
	    IMG(img)->sixel_fmt = SIXEL_PIXELFORMAT_RGBA8888;
	    break;
    }

    if (sixel_dither_new(&IMG(img)->dither, 256, sixel_allocator) == SIXEL_FALSE)
    {
	IMG(img)->dither = NULL;
	return;
    }

    if (sixel_dither_initialize(IMG(img)->dither, img->data, img->width,
		img->height, IMG(img)->sixel_fmt, SIXEL_LARGE_AUTO,
		SIXEL_REP_AUTO, SIXEL_QUALITY_HIGH) == SIXEL_FALSE)
    {
	sixel_dither_unref(IMG(img)->dither);
	IMG(img)->dither = NULL;
	return;
    }
}

    void
image_sixel_uninit(image_T *img)
{
    if (IMG(img)->dither != NULL)
	sixel_dither_unref(IMG(img)->dither);
}

    image_placement_T *
image_placement_sixel_alloc(void)
{
    return (image_placement_T *)ALLOC_CLEAR_ONE(image_placement_sixel_T);
}

    void
image_placement_sixel_init(image_placement_T *place)
{
    PLACE(place)->cache = NULL;
}

    void
image_placement_sixel_uninit(image_placement_T *place)
{
    image_placement_sixel_clear(place);
    vim_free(PLACE(place)->cache);
}

    static bool
crop_equal(image_crop_T *a, image_crop_T *b)
{
    return a->height == b->height && a->width == b->width && a->x == b->x &&
	a->y == b->y;
}

    void
image_placement_sixel_draw(image_placement_T *place)
{
    if (IMG(place->img)->dither == NULL)
	return;

    // Encode image if we haven't, or if crop rectangle has changed.
    if (PLACE(place)->cache == NULL
	    || !crop_equal(&place->geometry.crop, &PLACE(place)->crop))
    {
	image_crop_T	*crop = &place->geometry.crop;
	sixel_frame_t	*frame;
	uint8_t		*copy;

	copy = vim_memsave(place->img->data,
		(size_t)place->img->width * place->img->height * place->img->fmt);
	if (copy == NULL)
	{
	    emsg(_(e_out_of_memory));
	    return;
	}

	if (sixel_frame_new(&frame, sixel_allocator) == SIXEL_FALSE)
	{
	    vim_free(copy);
	    return;
	}

	// Note that sixel_frame_init() takes ownership of "copy"
	if (sixel_frame_init(frame, copy, place->img->width,
		    place->img->height, IMG(place->img)->sixel_fmt,
		    NULL, 0) == SIXEL_FALSE
		|| sixel_frame_clip(frame, crop->x, crop->y, crop->width,
		    crop->height) == SIXEL_FALSE)
	{
	    sixel_frame_unref(frame);
	    return;
	}

	ga_init2(&sixel_buf, 1, 4096);

	if (sixel_encode(sixel_frame_get_pixels(frame),
		    sixel_frame_get_width(frame),
		    sixel_frame_get_height(frame), 0, IMG(place->img)->dither,
		    sixel_output) == SIXEL_FALSE)
	{
	    sixel_frame_unref(frame);
	    ga_clear(&sixel_buf);
	    return;
	}

	sixel_frame_unref(frame);

	if (ga_append(&sixel_buf, NUL) == FAIL)
	{
	    ga_clear(&sixel_buf);
	    return;
	}

	vim_free(PLACE(place)->cache);
	PLACE(place)->cache = sixel_buf.ga_data;
	PLACE(place)->cache_len = sixel_buf.ga_len;
	ga_init(&sixel_buf);
    }

    cursor_off();
    term_windgoto(place->geometry.row, place->geometry.col);
    out_str(PLACE(place)->cache);
    screen_start();
    setcursor_mayforce(TRUE);
    cursor_on();
    out_flush();

    PLACE(place)->crop = place->geometry.crop;
}

    void
image_placement_sixel_clear(image_placement_T *place UNUSED)
{
    image_crop_T    *crop = &place->geometry.crop;
    linenr_T	    cell_rows = (crop->height + cell_height - 1) / cell_height;
    linenr_T	    first_row = place->geometry.row;
    linenr_T	    last_row = first_row + cell_rows - 1;
    win_T	    *wp;

    FOR_ALL_WINDOWS(wp)
    {
	linenr_T    ov_first, ov_last;
	int	    height = wp->w_winrow + wp->w_height - 1;

	// Skip windows that don't overlap the placement's row range at all.
	if (last_row < wp->w_winrow
		|| first_row > height)
	    continue;

	ov_first = first_row > wp->w_winrow ? first_row : wp->w_winrow;
	ov_last  = last_row < height ? last_row : height;

	redraw_win_range_later(wp, ov_first, ov_last);
    }
}

#endif // FEAT_IMAGE_SIXEL
