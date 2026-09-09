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
    // Palette for this image
    sixel_dither_t *dither;
    int		    sixel_fmt;
} image_sixel_T;

static sixel_allocator_t    *sixel_allocator;
static sixel_output_t	    *sixel_output;

    static int
sixel_write(char *data, int size, void *udata UNUSED)
{
    for (int i = 0; i < size; i++)
	out_char(data[i]);

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
	if (sixel_allocator_new(&sixel_allocator, alloc, calloc, realloc,
		    vim_free) == SIXEL_FALSE)
	    return FAIL;

    if (sixel_output == NULL)
	if (sixel_output_new(&sixel_output, sixel_write, NULL,
		    sixel_allocator) == SIXEL_FALSE)
	    return FAIL;

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

    int
image_sixel_init(image_T *img)
{
    image_sixel_T *ctx;

    if (sixel_init() == FAIL)
	return FAIL;


    ctx = ALLOC_CLEAR_ONE(image_sixel_T);
    if (ctx == NULL)
	return FAIL;

    switch (img->fmt)
    {
	case IMAGE_FORMAT_RGB:
	    ctx->sixel_fmt = SIXEL_PIXELFORMAT_RGB888;
	    break;
	case IMAGE_FORMAT_RGBA:
	    ctx->sixel_fmt = SIXEL_PIXELFORMAT_RGBA8888;
	    break;
    }

    if (sixel_dither_new(&ctx->dither, 256, sixel_allocator) == SIXEL_FALSE)
    {
	vim_free(ctx);
	return FAIL;
    }

    if (sixel_dither_initialize(ctx->dither,
		(uint8_t *)pixman_image_get_data(img->image),
		pixman_image_get_width(img->image),
		pixman_image_get_height(img->image),
		ctx->sixel_fmt,
		SIXEL_LARGE_AUTO,
		SIXEL_REP_AUTO, SIXEL_QUALITY_HIGH) == SIXEL_FALSE)
    {
	sixel_dither_unref(ctx->dither);
	vim_free(ctx);
	return FAIL;
    }

    img->backend_data = ctx;
    return OK;
}

    void
image_sixel_uninit(image_T *img)
{
    image_sixel_T *ctx = img->backend_data;

    if (ctx->dither != NULL)
	sixel_dither_unref(ctx->dither);
    vim_free(ctx);
}

    int
image_placement_sixel_init(image_placement_T *place UNUSED)
{
    return OK;
}

    void
image_placement_sixel_uninit(image_placement_T *place UNUSED)
{
}

    void
image_placement_sixel_draw(image_placement_T *place, garray_T *buf)
{
    image_T		    *img = place->img;
    pixman_box32_t	    *rects;
    int			    n_rects;

    rects = pixman_region32_rectangles(&place->visible, &n_rects);
    if (rects == NULL)
	return;

    cursor_off();

    for (int i = 0; i < n_rects; i++)
    {
	pixman_box32_t	rect = rects[i];
	int		row, col;
	int		w, h;
	int		size;
	pixman_image_t  *tmp_image;

	// Each rectangle position and dimensions *should* be a multiple of
	// "cell_width" and "cell_height".
	image_placement_get_subrect_pos(place, rect, &row, &col);

	w = rect.x2 - rect.x1;
	h = rect.y2 - rect.y1;

	// Crop the image into "buf"
	size = w * h * img->fmt;
	if (ga_grow(buf, size) == FAIL)
	    goto exit;

	// Create temporary image to composite image into
	tmp_image = pixman_image_create_bits(
		pixman_image_get_format(img->image),
		w, h, buf->ga_data, w * img->fmt);
	if (tmp_image == NULL)
	    goto exit;

	pixman_image_composite32(PIXMAN_OP_SRC,
		img->image, NULL, tmp_image, rect.x1, rect.y1, 0, 0,
		0, 0, w, h);

	term_windgoto(row, col);
	(void)sixel_encode(buf->ga_data, w, h, 0,
		((image_sixel_T *)img->backend_data)->dither, sixel_output);

	pixman_image_unref(tmp_image);
	buf->ga_len = 0;
    }

exit:
    screen_start();
    setcursor_mayforce(TRUE);
    cursor_on();
    out_flush();
}

    void
image_placement_sixel_clear(image_placement_T *place UNUSED)
{
}

#endif // FEAT_IMAGE_SIXEL
