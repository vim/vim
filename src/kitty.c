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
 *	    No external dependency; the base64 alphabet is inlined here.
 */

#include "vim.h"

#if defined(FEAT_IMAGE_KITTY) || defined(PROTO)

static const char_u kitty_b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * Append a NUL-terminated string to "ga".  Returns OK / FAIL so the caller
 * can abort on allocation failure (unlike ga_concat(), which silently no-ops
 * and would leave a truncated, invalid kitty APC sequence behind).
 */
    static int
kitty_ga_concat(garray_T *ga, const char_u *s)
{
    int	    len = (int)STRLEN(s);

    if (len == 0)
	return OK;
    if (ga_grow(ga, len) == FAIL)
	return FAIL;
    mch_memmove((char_u *)ga->ga_data + ga->ga_len, s, (size_t)len);
    ga->ga_len += len;
    return OK;
}

/*
 * Append base64-encoded bytes from "src[len]" to growarray "ga".
 * Returns OK / FAIL so the caller can propagate OOM.
 */
    static int
kitty_b64_append(garray_T *ga, char_u *src, long len)
{
    long    i;
    long    out_len = ((len + 2) / 3) * 4;
    char_u  *dst;

    if (out_len == 0)
	return OK;
    if (ga_grow(ga, (int)out_len) == FAIL)
	return FAIL;
    dst = (char_u *)ga->ga_data + ga->ga_len;
    for (i = 0; i < len; i += 3)
    {
	unsigned a = src[i];
	unsigned b = (i + 1 < len) ? src[i + 1] : 0;
	unsigned c = (i + 2 < len) ? src[i + 2] : 0;
	unsigned triple = (a << 16) | (b << 8) | c;

	*dst++ = kitty_b64_table[(triple >> 18) & 0x3f];
	*dst++ = kitty_b64_table[(triple >> 12) & 0x3f];
	*dst++ = (i + 1 < len)
			    ? kitty_b64_table[(triple >> 6) & 0x3f] : '=';
	*dst++ = (i + 2 < len)
			    ? kitty_b64_table[triple & 0x3f]	    : '=';
    }
    ga->ga_len += (int)out_len;
    return OK;
}

/*
 * Encode an RGB(A) image into a kitty graphics protocol APC sequence.
 * Returns a malloced char_u* containing the full sequence
 * (one or more `\e_G...\e\\` envelopes), or NULL on OOM.
 *
 * The sequence is emitted with `a=T` (transmit + display), `q=2` (no
 * status responses), `f=24` for RGB or `f=32` for RGBA, and chunked
 * via `m=1`/`m=0` so the per-envelope payload stays under kitty's
 * 4096-byte limit.  When "id" is non-zero it is sent as `i=<id>` so
 * the resulting placement can later be removed via kitty_delete().
 * "zindex" is sent as `z=<zindex>` so overlapping placements stack in
 * popup zindex order no matter in which order they were (re)created.
 */
    char_u *
kitty_encode(image_rgb_T *img, int id, int zindex)
{
    garray_T	ga;
    long	pix_bytes;
    long	payload_len;
    long	b64_total;
    long	offset = 0;
    int		fmt;
    int		first = TRUE;
    char_u	hdr[80];

    if (img == NULL || img->data == NULL || img->width <= 0 || img->height <= 0)
	return NULL;

    pix_bytes = img->has_alpha ? 4 : 3;
    payload_len = (long)img->width * img->height * pix_bytes;
    b64_total = ((payload_len + 2) / 3) * 4;
    fmt = img->has_alpha ? 32 : 24;

    ga_init2(&ga, 1, (int)b64_total + 256);

    // Emit one envelope per 4096 base64 chars.  The first envelope
    // carries the full geometry/format header; later envelopes only
    // need the chunk-continuation marker `m=`.
    while (offset < b64_total)
    {
	long	this_chunk = b64_total - offset;
	int	more;

	if (this_chunk > 4096)
	    this_chunk = 4096;
	more = (offset + this_chunk < b64_total);

	if (first)
	{
	    if (id != 0)
		vim_snprintf((char *)hdr, sizeof(hdr),
			"\033_Ga=T,f=%d,s=%d,v=%d,i=%d,z=%d,q=2,m=%d;",
			fmt, img->width, img->height, id, zindex,
			more ? 1 : 0);
	    else
		vim_snprintf((char *)hdr, sizeof(hdr),
			"\033_Ga=T,f=%d,s=%d,v=%d,z=%d,q=2,m=%d;",
			fmt, img->width, img->height, zindex, more ? 1 : 0);
	    first = FALSE;
	}
	else
	{
	    vim_snprintf((char *)hdr, sizeof(hdr),
		    "\033_Gm=%d;", more ? 1 : 0);
	}
	if (kitty_ga_concat(&ga, hdr) == FAIL)
	    goto fail;

	// Encode the matching slice of the source bytes.  Each base64
	// chunk consumes (this_chunk / 4) base64 quartets, which means
	// (this_chunk * 3 / 4) source bytes.
	{
	    long	src_offset = offset * 3 / 4;
	    long	src_len = this_chunk * 3 / 4;

	    if (src_offset + src_len > payload_len)
		src_len = payload_len - src_offset;
	    if (kitty_b64_append(&ga, img->data + src_offset, src_len) == FAIL)
		goto fail;
	}

	if (kitty_ga_concat(&ga, (char_u *)"\033\\") == FAIL)
	    goto fail;

	offset += this_chunk;
    }

    if (ga_append(&ga, NUL) == FAIL)
	goto fail;
    return (char_u *)ga.ga_data;

fail:
    ga_clear(&ga);
    return NULL;
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

/*
 * Build a kitty "delete image" APC sequence for the placement created
 * by kitty_encode() with the matching `id`.  The caller must
 * vim_free() the returned buffer.  Returns NULL on OOM or id <= 0.
 *
 * Sequence: `\e_Ga=d,i=<id>,q=2\e\\`
 *	a=d  -> action: delete
 *	i=   -> image id (target placement)
 *	q=2  -> suppress status reply
 */
    char_u *
kitty_delete(int id)
{
    char_u  buf[40];

    if (id <= 0)
	return NULL;
    vim_snprintf((char *)buf, sizeof(buf), "\033_Ga=d,i=%d,q=2\033\\", id);
    return vim_strsave(buf);
}

#endif // FEAT_IMAGE_KITTY || PROTO
