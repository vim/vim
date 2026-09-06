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
    image_T base;

    // If image data has been transmitted to terminal
    bool transmitted;
} image_kitty_T;

#define IMG(img) ((image_kitty_T *)(img))

    image_T *
image_kitty_alloc(void)
{
    return (image_T *)ALLOC_CLEAR_ONE(image_kitty_T);
}

    void
image_kitty_init(image_T *img)
{
    IMG(img)->transmitted = false;
}

    void
image_kitty_uninit(image_T *img)
{
    // Delete image data from terminal
    vim_snprintf((char *)IObuff, IOSIZE,
	    "\033_Ga=d,d=I,i=%d,q=2\033\\", img->id);

    out_str((char_u *)IObuff);
    out_flush();
    IMG(img)->transmitted = false;
}

/*
 * Transmit image data to terminal if it hasn't already been done.
 */
    static void
transmit_image(image_kitty_T *img)
{
    static char buf[4096 + 3]; // Extra bytes for NUL and terminator

    int_u   off = 0;
    int_u   sz = img->base.fmt * img->base.width * img->base.height;
    bool    first = true;

    if (img->transmitted)
	return;

    while (off < sz)
    {
	int_u chunk_sz = MIN(4096 * 3 / 4, sz - off);
	bool more = off + chunk_sz < sz;
	long encoded;

	if (first)
	{
	    vim_snprintf(buf, sizeof(buf),
		    "\033_Ga=t,i=%d,f=%d,s=%u,v=%u,m=%d,q=2;", img->base.id,
		    img->base.fmt * 8, img->base.width, img->base.height, more);
	    first = false;
	}
	else
	    vim_snprintf(buf, sizeof(buf), "\033_Gm=%d;", more);
	out_str((char_u *)buf);

	encoded =
	    base64_encode_buf((char_u *)buf, img->base.data + off, chunk_sz);

	buf[encoded] = '\033';
	buf[encoded + 1] = '\\';
	buf[encoded + 2] = NUL;

	out_str((char_u *)buf);
	off += chunk_sz;
    }

    img->transmitted = true;
    out_flush();
}

    void
image_kitty_draw(image_T *img, image_geometry_T *geometry, int id)
{
    transmit_image(IMG(img));

    // Since we send the image id and the placement id, the existing
    // placement (if any) will be replaced by this one (essentially
    // moving it).
    //
    // Add C=1 (don't move the cursor), since as said in the specification:
    // ```
    // After placing an image on the screen the cursor must be moved to the right
    // by the number of cols in the image placement rectangle and down by the
    // number of rows in the image placement rectangle. If either of these cause
    // the cursor to leave either the screen or the scroll area, the exact
    // positioning of the cursor is undefined, and up to implementations. The
    // client can ask the terminal emulator to not move the cursor at all by
    // specifying C=1 in the command, which sets the cursor movement policy to no
    // movement for placing the current image.
    // ```
    //
    // On kitty terminal it seems to scroll the terminal, messing up the screen.
    vim_snprintf(
	    (char *)IObuff, IOSIZE,
	    "\033_Ga=p,i=%d,p=%d,x=%u,y=%u,w=%u,h=%u,z=%d,C=1,q=2\033\\",
	    img->id, id, geometry->crop.x, geometry->crop.y,
	    geometry->crop.width, geometry->crop.height,
	    geometry->zindex);

    term_windgoto(geometry->row, geometry->col);
    out_str((char_u *)IObuff);
    screen_start();
    setcursor_mayforce(TRUE);
    out_flush();
}

    void
image_kitty_clear(image_T *img, int id)
{
    vim_snprintf((char *)IObuff, IOSIZE,
	    "\033_Ga=d,d=I,i=%d,p=%d,q=2\033\\", img->id, id);

    out_str((char_u *)IObuff);
    out_flush();
}

#endif // FEAT_IMAGE_KITTY
