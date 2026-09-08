/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read a list of people who contributed.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#ifdef FEAT_IMAGE_KITTY

typedef struct
{
    bool transmitted; // If image data has been transmitted to terminal
} image_kitty_T;

typedef struct
{
    int n_ids; // Number of placements ids used for previous redraw
} image_placement_kitty_T;

    int
image_kitty_init(image_T *img)
{
    image_kitty_T *ctx = ALLOC_CLEAR_ONE(image_kitty_T);

    if (ctx == NULL)
	return FAIL;

    img->backend_data = ctx;
    return OK;
}

    void
image_kitty_uninit(image_T *img)
{
    vim_free(img->backend_data);
}

    int
image_placement_kitty_init(image_placement_T *place)
{
    image_placement_kitty_T *ctx = ALLOC_CLEAR_ONE(image_placement_kitty_T);

    if (ctx == NULL)
	return FAIL;

    place->backend_data = ctx;
    return OK;
}

    void
image_placement_kitty_uninit(image_placement_T *place)
{
    vim_free(place->backend_data);
}

/*
 * Transmit image data to terminal if it hasn't already been done.
 */
    static void
transmit_image(image_T *img)
{
    static char buf[4096 + 3]; // Extra bytes for NUL and terminator

    int_u   off = 0;
    int_u   sz;
    bool    first = true;
    int	    width, height;

    if (likely(((image_kitty_T *)img->backend_data)->transmitted))
	return;

    width = pixman_image_get_width(img->image);
    height = pixman_image_get_height(img->image);
    sz = img->fmt * width * height;

    while (off < sz)
    {
	int_u chunk_sz = MIN(4096 * 3 / 4, sz - off);
	bool more = off + chunk_sz < sz;
	long encoded;

	if (unlikely(first))
	{
	    vim_snprintf(buf, sizeof(buf),
		    "\033_Ga=t,i=%d,f=%d,s=%u,v=%u,m=%d,q=2;", img->id,
		    img->fmt * 8, width, height, more);
	    first = false;
	}
	else
	    vim_snprintf(buf, sizeof(buf), "\033_Gm=%d;", more);
	out_str((char_u *)buf);

	encoded = base64_encode_buf((char_u *)buf,
		(uint8_t *)pixman_image_get_data(img->image) + off,
		chunk_sz);

	buf[encoded] = '\033';
	buf[encoded + 1] = '\\';
	buf[encoded + 2] = NUL;

	out_str((char_u *)buf);
	off += chunk_sz;
    }

    ((image_kitty_T *)img->backend_data)->transmitted = true;
    out_flush();
}

    static void
clear_placement(int id, int place_id)
{
    vim_snprintf((char *)IObuff, IOSIZE,
	    "\033_Ga=d,d=i,i=%d,p=%d,q=2\033\\", id, place_id);

    out_str((char_u *)IObuff);
}

/*
 * Don't need to use "buf", because we can just tell the terminal what to crop.
 */
    void
image_placement_kitty_draw(image_placement_T *place, garray_T *buf UNUSED)
{
    image_T		    *img = place->img;
    image_placement_kitty_T *ctx = place->backend_data;
    pixman_box32_t	    *rects;
    int			    n_rects;

    rects = pixman_region32_rectangles(&place->visible, &n_rects);
    if (unlikely(n_rects > PLACEMENT_ID_INC))
	return;

    transmit_image(img);

    if (unlikely(rects == NULL))
	// Not sure if this can happen...
	return;

    for (int i = 0; i < n_rects; i++)
    {
	pixman_box32_t rect = rects[i];

	int rect_id = place->id + i; // The placement id of each rect is
				     // "place->id + i".
	int row, col;

	// Each rectangle position and dimensions *should* be a multiple of
	// "cell_width" and "cell_height".
	pixel2cells(rect.x1, rect.y1, &col, &row);
	row += place->row;
	col += place->col;

	vim_snprintf(
		(char *)IObuff, IOSIZE,
		"\033_Ga=p,i=%d,p=%d,x=%u,y=%u,w=%u,h=%u,z=0,q=2,C=1\033\\",
		img->id, rect_id, rect.x1, rect.y1,
		rect.x2 - rect.x1, rect.y2 - rect.y1);

	term_windgoto(row, col);
	out_str((char_u *)IObuff);
    }

    // Clear all placements that have not been reused.
    for (int i = n_rects; i < ctx->n_ids; i++)
	clear_placement(img->id, place->id + i);

    ctx->n_ids = n_rects;

    if (n_rects > 0)
    {
	screen_start();
	setcursor_mayforce(TRUE);
	out_flush();
    }
}

    void
image_placement_kitty_clear(image_placement_T *place)
{
    image_placement_kitty_T *ctx = place->backend_data;

    for (int i = 0; i < ctx->n_ids; i++)
	clear_placement(place->img->id, place->id + i);
    out_flush();
    ctx->n_ids = 0;
}

#endif // FEAT_IMAGE_KITTY
