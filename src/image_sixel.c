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
} image_sixel_T;

typedef struct
{
    // These are offsets relative to the top left of the image.
    int	    row_off;
    int	    col_off;
    char_u  *seq;
} sixel_chunk_T;

typedef struct
{
    pixman_region32_t	visible_region; // Region used for the previous redraw
    bool		visible_init;
    sixel_chunk_T	*chunks;    // Cached sixel sequence (stored as multiple
				    // chunks). Length is number of rects in
				    // "visible_region".
} image_placement_sixel_T;

static sixel_allocator_t    *sixel_allocator;
static sixel_output_t	    *sixel_output;
static garray_T		    sixel_buf;

    static int
sixel_write(char *data, int size, void *udata UNUSED)
{
    ga_concat_len(&sixel_buf, (char_u *)data, size);

    // No idea what the return value is supposed to be. Looking through
    // libsixel source code, it seems to be unused?
    return SIXEL_OK;
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
    image_sixel_T   *ctx;
    int		    sixel_fmt;

    if (sixel_init() == FAIL)
	return FAIL;


    ctx = ALLOC_CLEAR_ONE(image_sixel_T);
    if (ctx == NULL)
	return FAIL;

    switch (img->fmt)
    {
	case IMAGE_FORMAT_RGB:
	    sixel_fmt = SIXEL_PIXELFORMAT_RGB888;
	    break;
	case IMAGE_FORMAT_RGBA:
	    sixel_fmt = SIXEL_PIXELFORMAT_RGBA8888;
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
		sixel_fmt, SIXEL_LARGE_AUTO,
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
image_placement_sixel_init(image_placement_T *place)
{
    image_placement_sixel_T *ctx = ALLOC_CLEAR_ONE(image_placement_sixel_T);

    if (ctx == NULL)
	return FAIL;
    place->backend_data = ctx;
    return OK;
}

    static void
clear_chunks(image_placement_sixel_T *ctx)
{
    if (ctx->chunks == NULL)
	return;

    for (int i = 0; i < pixman_region32_n_rects(&ctx->visible_region); i++)
	vim_free(ctx->chunks[i].seq);
    VIM_CLEAR(ctx->chunks);
}

    void
image_placement_sixel_uninit(image_placement_T *place UNUSED)
{
    image_placement_sixel_T *ctx = place->backend_data;

    if (ctx->visible_init)
	pixman_region32_fini(&ctx->visible_region);
    clear_chunks(ctx);
    vim_free(ctx);
}

    void
image_placement_sixel_draw(image_placement_T *place)
{
    image_T		    *img = place->img;
    image_placement_sixel_T *ctx = place->backend_data;
    pixman_box32_t	    *rects;
    int			    n_rects;
    garray_T		    buf;

    // Check if visible region is still the same, if so then use the cached
    // sixel sequences.
    if (ctx->visible_init
	    && pixman_region32_equal(&ctx->visible_region, &place->visible))
    {
	for (int i = 0; i < pixman_region32_n_rects(&ctx->visible_region); i++)
	{
	    sixel_chunk_T *chunk = ctx->chunks + i;

	    term_windgoto(
		    place->row + chunk->row_off, place->col + chunk->col_off);
	    out_str(chunk->seq);
	}
	goto exit;
    }

    rects = pixman_region32_rectangles(&place->visible, &n_rects);
    if (rects == NULL || n_rects == 0)
	return;

    clear_chunks(ctx);
    ctx->chunks = ALLOC_CLEAR_MULT(sixel_chunk_T, n_rects);

    if (ctx->chunks == NULL)
	return;

    ga_init2(&sixel_buf, 1, 4096);
    ga_init2(&buf, 1, 32768);

    cursor_off();

    for (int i = 0; i < n_rects; i++)
    {
	pixman_box32_t	rect = rects[i];
	int		row, col;
	int		x, y, w, h;
	int		size;
	pixman_image_t  *tmp_image;
	sixel_dither_t	*dither;
	sixel_chunk_T	*chunk = ctx->chunks + i;

	row = place->row + rect.y1;
	col = place->col + rect.x1;

	x = (rect.x1 + place->crop_box.x1) * cell_width;
	y = (rect.y1 + place->crop_box.y1) * cell_height;
	w = (rect.x2 - rect.x1) * cell_width;
	h = (rect.y2 - rect.y1) * cell_height;

	// Crop the image into "buf"
	size = w * h * img->fmt;
	if (ga_grow(&buf, size) == FAIL)
	    continue;

	// Create temporary image to composite the image into
	tmp_image = pixman_image_create_bits(
		pixman_image_get_format(img->image),
		w, h, buf.ga_data, w * img->fmt);
	if (tmp_image == NULL)
	    continue;

	pixman_image_composite32(PIXMAN_OP_SRC,
		img->image, NULL, tmp_image, x, y, 0, 0, 0, 0, w, h);
	pixman_image_unref(tmp_image);

	if (sixel_dither_new(&dither, 256, sixel_allocator) == SIXEL_FALSE)
	    continue;

	// We have to create a new dither for each subrect, because creating a
	// single one based on the entire image seems to mess up the final sixel
	// result. Probably something to do with libsixel?
	if (sixel_dither_initialize(
		    dither, buf.ga_data, w, h,
		    img->fmt == IMAGE_FORMAT_RGB
		    ? SIXEL_PIXELFORMAT_RGB888 : SIXEL_PIXELFORMAT_RGBA8888,
		    SIXEL_LARGE_AUTO, SIXEL_REP_AUTO,
		    SIXEL_QUALITY_HIGH) == SIXEL_FALSE)
	{
	    sixel_dither_unref(dither);
	    continue;
	}

	if (sixel_encode(buf.ga_data, w, h, 0, dither, sixel_output)
		== SIXEL_FALSE)
	{
	    sixel_dither_unref(dither);
	    continue;
	}
	sixel_dither_unref(dither);

	if (ga_append(&sixel_buf, NUL) == FAIL)
	{
	    sixel_buf.ga_len = 0;
	    continue;
	}

	term_windgoto(row, col);
	out_str(sixel_buf.ga_data);

	chunk->row_off = rect.y1;
	chunk->col_off = rect.x1;
	chunk->seq = sixel_buf.ga_data;
	ga_init(&sixel_buf);
    }

    if (ctx->visible_init)
	pixman_region32_clear(&ctx->visible_region);
    pixman_region32_copy(&ctx->visible_region, &place->visible);
    ctx->visible_init = true;

    ga_clear(&sixel_buf);
    ga_clear(&buf);
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
