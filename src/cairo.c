/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * cairo.c: Cairo-based image backend for popup_create({image: ...}),
 *	    by Yasuhiro Matsumoto.
 *
 * Builds a cairo_image_surface_t from the popup's RGB / RGBA pixel
 * buffer and composites it onto a target cairo surface (typically the
 * front-end's offscreen `gui.surface`).  Used today by the GTK2/3
 * backend in gui_gtk_x11.c, and intended to be reusable by other
 * cairo-based front-ends (e.g. a future GTK4 port).
 *
 * The cached surface is stored as void* on win_T->w_popup_image_surface
 * so structs.h does not need to pull in <cairo.h>.
 */

#include "vim.h"

#if defined(FEAT_IMAGE_CAIRO) || defined(PROTO)
# include <cairo.h>

/*
 * Convert the popup's RGB / RGBA pixel buffer into the cached cairo
 * surface's pixel layout.  Cairo's CAIRO_FORMAT_ARGB32 / RGB24 wants
 * native-endian uint32 quartets shaped as 0xAARRGGBB, with ARGB32
 * pixels premultiplied; we build the integer per pixel so the
 * resulting bytes come out correct on either endianness.
 */
    static void
cairo_popup_image_fill_surface(win_T *wp, cairo_surface_t *surf)
{
    unsigned char   *src = wp->w_popup_image_data;
    unsigned char   *dst;
    int		     w = wp->w_popup_image_w;
    int		     h = wp->w_popup_image_h;
    int		     stride;
    int		     x, y;

    cairo_surface_flush(surf);
    dst = cairo_image_surface_get_data(surf);
    stride = cairo_image_surface_get_stride(surf);

    if (wp->w_popup_image_alpha)
    {
	for (y = 0; y < h; ++y)
	{
	    unsigned int *p = (unsigned int *)(dst + y * stride);

	    for (x = 0; x < w; ++x)
	    {
		int idx = (y * w + x) * 4;
		unsigned r = src[idx + 0];
		unsigned g = src[idx + 1];
		unsigned b = src[idx + 2];
		unsigned a = src[idx + 3];

		// Premultiply alpha for ARGB32.
		r = (r * a + 127) / 255;
		g = (g * a + 127) / 255;
		b = (b * a + 127) / 255;
		p[x] = ((unsigned int)a << 24)
		     | ((unsigned int)r << 16)
		     | ((unsigned int)g << 8)
		     |  (unsigned int)b;
	    }
	}
    }
    else
    {
	for (y = 0; y < h; ++y)
	{
	    unsigned int *p = (unsigned int *)(dst + y * stride);

	    for (x = 0; x < w; ++x)
	    {
		int idx = (y * w + x) * 3;
		unsigned r = src[idx + 0];
		unsigned g = src[idx + 1];
		unsigned b = src[idx + 2];

		// RGB24's high byte is unused but conventionally 0xff.
		p[x] = (0xffu << 24)
		     | ((unsigned int)r << 16)
		     | ((unsigned int)g << 8)
		     |  (unsigned int)b;
	    }
	}
    }
    cairo_surface_mark_dirty(surf);
}

/*
 * Build the cached cairo_image_surface_t for "wp" from its RGB / RGBA
 * pixel buffer.  Existing cache is freed first.  Returns true on
 * success, false on bad input or out-of-memory.
 */
    bool
cairo_popup_image_ensure(win_T *wp)
{
    cairo_surface_t *surf;
    cairo_format_t   fmt;

    if (wp->w_popup_image_data == NULL
	    || wp->w_popup_image_w <= 0 || wp->w_popup_image_h <= 0)
	return false;

    if (wp->w_popup_image_surface != NULL)
	return true;

    fmt = wp->w_popup_image_alpha ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24;
    surf = cairo_image_surface_create(fmt,
		wp->w_popup_image_w, wp->w_popup_image_h);
    if (cairo_surface_status(surf) != CAIRO_STATUS_SUCCESS)
    {
	cairo_surface_destroy(surf);
	return false;
    }
    cairo_popup_image_fill_surface(wp, surf);
    wp->w_popup_image_surface = surf;
    return true;
}

/*
 * Same-size pixel swap: refill the existing cached surface in place
 * with new bytes from wp->w_popup_image_data.  Returns false when
 * there is no cache yet (caller should fall back to a full rebuild).
 */
    bool
cairo_popup_image_update(win_T *wp)
{
    if (wp->w_popup_image_surface == NULL || wp->w_popup_image_data == NULL)
	return false;
    cairo_popup_image_fill_surface(wp,
	    (cairo_surface_t *)wp->w_popup_image_surface);
    return true;
}

/*
 * Release the cached cairo surface attached to "wp".  Safe to call
 * when no cache is present.
 */
    void
cairo_popup_image_free(win_T *wp)
{
    if (wp->w_popup_image_surface != NULL)
    {
	cairo_surface_destroy((cairo_surface_t *)wp->w_popup_image_surface);
	wp->w_popup_image_surface = NULL;
    }
}

/*
 * Composite a sub-rect of the popup's cached image onto "target" at pixel
 * position (x, y).  (src_x, src_y, draw_w, draw_h) describe which pixel
 * sub-rect of the source bitmap should be drawn -- "clipwindow" popups pass
 * non-zero offsets to crop the portion that falls outside the host window.
 * Builds the cache on first call.  Caller is responsible for scheduling a
 * redraw of the target's owning widget if needed (e.g.
 * gtk_widget_queue_draw_area on GTK).
 */
    void
cairo_popup_image_paint(
	win_T	*wp,
	void	*target,
	int	 x,
	int	 y,
	int	 src_x,
	int	 src_y,
	int	 draw_w,
	int	 draw_h)
{
    cairo_t *cr;

    if (target == NULL || draw_w <= 0 || draw_h <= 0)
	return;
    if (!cairo_popup_image_ensure(wp))
	return;

    cr = cairo_create((cairo_surface_t *)target);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    // Position the source so its (src_x, src_y) pixel lands at (x, y),
    // then clip to the visible (draw_w x draw_h) rectangle.
    cairo_set_source_surface(cr,
	    (cairo_surface_t *)wp->w_popup_image_surface,
	    x - src_x, y - src_y);
    cairo_rectangle(cr, x, y, draw_w, draw_h);
    cairo_fill(cr);
    cairo_destroy(cr);
}

#endif // FEAT_IMAGE_CAIRO || PROTO
