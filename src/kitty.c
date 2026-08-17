/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * kitty.c: encode an RGB(A) image into a kitty graphics protocol
 *	    APC sequence, by Yasuhiro Matsumoto.
 *	    The popup's image bytes are sent in 4096-byte chunks of base64
 *	    inside `\e_G...;<chunk>\e\\` envelopes.
 *	    Spec: https://sw.kovidgoyal.net/kitty/graphics-protocol/
 *	    Base64 encoding is shared with misc2 base64_encode()/decode()
 */

#include "vim.h"

#if defined(FEAT_IMAGE_KITTY) || defined(PROTO)

// Max base64 chars per envelope, per the kitty graphics protocol.
#define KITTY_CHUNK_B64		4096
// Source bytes that encode into KITTY_CHUNK_B64 base64 chars.
#define KITTY_CHUNK_SRC		(KITTY_CHUNK_B64 * 3 / 4)
// header + base64 chunk + "\033\\" trailer + NUL. Use 128 extra bytes padding
// for header, should be more than enough.
#define KITTY_BUF_SIZE		(128 + KITTY_CHUNK_B64 + 2 + 1)

/*
 * Return the kitty image id to use for window id "id".  Kitty image ids are
 * global to the terminal, so mix in the process id: another Vim in the same
 * terminal would otherwise use the same ids and its "a=d,d=I" would free our
 * image data.
 */
    static int
kitty_image_id(int id)
{
    static int	base = 0;

    if (base == 0)
	base = (((int)mch_get_pid() & 0x7fff) + 1) << 16;
    return base | (id & 0xffff);
}

/*
 * Transmit an RGB(A) image to the terminal (does not display it!). It will
 * have an id of "id", so that it can be placed later.
 */
    int
kitty_transmit(image_rgb_T *img, int id)
{
    static char buf[KITTY_BUF_SIZE];

    long	pix_bytes;
    long	payload_len;
    long	offset = 0;
    int		fmt;
    int		first = TRUE;

    if (img == NULL || img->data == NULL || img->width <= 0 || img->height <= 0)
	return FAIL;

    pix_bytes = img->has_alpha ? 4 : 3;
    payload_len = (long)img->width * img->height * pix_bytes;
    fmt = img->has_alpha ? 32 : 24;

    // Emit one envelope per KITTY_CHUNK_SRC source bytes (= 4096 base64
    // chars).  The first envelope carries the full geometry/format
    // header; later envelopes only need the chunk-continuation marker
    // `m=`.
    while (offset < payload_len)
    {
	long	this_chunk = payload_len - offset;
	int	more;
	int	hdr_len;
	long	b64_len;

	if (this_chunk > KITTY_CHUNK_SRC)
	    this_chunk = KITTY_CHUNK_SRC;
	more = (offset + this_chunk < payload_len);

	if (first)
	{
	    hdr_len = vim_snprintf(buf, sizeof(buf),
		    "\033_Ga=t,i=%d,f=%d,s=%d,v=%d,q=2,m=%d;",
		    kitty_image_id(id), fmt, img->width,
		    img->height, more ? 1 : 0);
	    first = FALSE;
	}
	else
	    hdr_len = vim_snprintf(buf, sizeof(buf), "\033_Gm=%d;",
		    more ? 1 : 0);

	b64_len = base64_encode_buf((char_u *)buf + hdr_len,
					img->data + offset, this_chunk);

	buf[hdr_len + b64_len] = '\033';
	buf[hdr_len + b64_len + 1] = '\\';
	buf[hdr_len + b64_len + 2] = NUL;

	out_str((char_u *)buf);
	offset += this_chunk;
    }

    out_flush();
    return OK;
}

/*
 * Place the image with the given id, which should have already been
 * transmitted. Its placement id will always be its image id, so that the image
 * is moved if it was previously placed.
 */
    void
kitty_place(int id, int row, int col, int src_x, int src_y, int w, int h, int z)
{
    vim_snprintf((char *)IObuff, IOSIZE,
	    "\033_Ga=p,i=%d,p=%d,x=%d,y=%d,w=%d,h=%d,z=%d,q=2\033\\",
	    kitty_image_id(id), kitty_image_id(id), src_x, src_y, w, h, z);

    term_windgoto(row, col);
    out_str((char_u *)IObuff);
    screen_start();
    setcursor_mayforce(TRUE);
    out_flush();
}

/*
 * Delete image placement with image id "id" (which is also its placement id).
 * If "del_data" is true, then its data will be freed by the terminal (see
 * https://sw.kovidgoyal.net/kitty/graphics-protocol/#deleting-images).
 */
    void
kitty_delete(int id, bool del_data)
{
    char d_key = del_data ? 'I' : 'i';

    vim_snprintf((char *)IObuff, IOSIZE,
	    "\033_Ga=d,d=%c,i=%d,p=%d,q=2\033\\", d_key, kitty_image_id(id),
	    kitty_image_id(id));

    out_str((char_u *)IObuff);
    out_flush();
}

/*
 * Shared tail of the kitty-graphics-support probe (see popup_kitty_probe()
 * in popupwin.c for the UNIX side and mch_kitty_probe() in os_win32.c for
 * the Windows console side).  "buf[n]" holds the raw, NUL-terminated bytes
 * read back from the terminal after sending the `\e_Gi=31...a=q` query
 * followed by the DA1 (`\e[c`) sentinel.
 *
 * Push everything except the kitty (APC \e_G...\e\\) and DA1 (\e[?...c)
 * response chunks back into the input buffer so user keystrokes typed
 * during the probe are not swallowed.  Scan the buffer linearly, emitting
 * any byte that is not inside a recognised response range.
 *
 * Returns TRUE when the buffer contains the positive "_Gi=31;OK" reply.
 */
    int
kitty_probe_parse(char *buf, int n)
{
    int i = 0;
    int seg_start = 0;

    while (i < n)
    {
	int response_end = -1;

	if (buf[i] == '\033' && i + 1 < n)
	{
	    if (buf[i + 1] == '_')
	    {
		// Kitty APC reply: scan to terminating ESC '\\'.
		int j;
		for (j = i + 2; j + 1 < n; ++j)
		    if (buf[j] == '\033' && buf[j + 1] == '\\')
		    {
			response_end = j + 2;
			break;
		    }
	    }
	    else if (buf[i + 1] == '[' && i + 2 < n && buf[i + 2] == '?')
	    {
		// DA1 reply: ESC [ ? ... c (the '?' distinguishes the
		// primary device-attributes answer from an arrow key
		// sequence like ESC [ A that the user might have typed).
		int j;
		for (j = i + 3; j < n; ++j)
		    if (buf[j] == 'c')
		    {
			response_end = j + 1;
			break;
		    }
	    }
	}
	if (response_end > 0)
	{
	    if (i > seg_start)
		add_to_input_buf((char_u *)(buf + seg_start), i - seg_start);
	    i = response_end;
	    seg_start = i;
	}
	else
	    ++i;
    }
    if (n > seg_start)
	add_to_input_buf((char_u *)(buf + seg_start), n - seg_start);

    // A positive kitty reply contains the literal "_Gi=31;OK".
    return strstr(buf, "_Gi=31;OK") != NULL;
}

#endif // FEAT_IMAGE_KITTY || PROTO
