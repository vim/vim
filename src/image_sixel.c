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

typedef struct
{
    // These are offsets relative to the top left of the image.
    int	    row_off;
    int	    col_off;
    char_u  *hdr_seq;
    char_u  *seq;
} sixel_chunk_T;

typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
} sixel_rgb_T;

typedef struct
{
    uint8_t min_r, max_r;
    uint8_t min_g, max_g;
    uint8_t min_b, max_b;

    int_u start;
    int_u count;
} color_box_T;

typedef struct
{
    uint8_t *index; // Each byte (pixel) points to an entry in the palette
    int	    index_size;

    sixel_rgb_T palette[256]; // Number of palette entries is 256
    int		palette_len;
} image_sixel_T;

typedef struct
{
    pixman_region32_t	visible_region; // Region used for the previous redraw
    bool		visible_init;
    sixel_chunk_T	*chunks;    // Cached sixel sequence (stored as multiple
				    // chunks). Length is number of rects in
				    // "visible_region".
} image_placement_sixel_T;

static image_T	*cur_img = NULL; // Current image to generate palette for
static int	colour_off;

static int
pixel_compare_func(const void *a, const void *b)
{
    const int *ai = a, *bi = b;

    uint8_t *px_a = &cur_img->data[*ai * cur_img->fmt];
    uint8_t *px_b = &cur_img->data[*bi * cur_img->fmt];

    // Ascending order (doesn't matter though)
    if (px_a[colour_off] > px_b[colour_off])
	return 1;
    else if (px_a[colour_off] < px_b[colour_off])
	return -1;
    return 0;
}

/*
 * Generate the palette based on the image. Returns OK on success and FAIL on
 * failure.
 */
    static int
generate_palette(image_T *img, image_sixel_T *ctx)
{
    int w, h;
    int n_pixels;

    color_box_T boxes[256];
    int		boxes_len = 1;
    int		*pixel_index;

    // Use median cut algorithm
    image_get_dimensions(img, &w, &h);
    n_pixels = w * h;

    pixel_index = ALLOC_MULT(int, n_pixels);
    if (pixel_index == NULL)
	return FAIL;

    // Pixel index is initially integers from 0 to "n_pixels"
    for (int i = 0; i < n_pixels; i++)
	pixel_index[i] = i;

    // Initial box covers every pixel
    boxes[0].start = 0;
    boxes[0].count = n_pixels;

    cur_img = img;

    while (true)
    {
	int	    axis_off;
	bool	    first = true;
	color_box_T box_to_split;
	int	    best_idx = -1;

	// Find box with most pixels (that can still be split)
	for (int i = 0; i < boxes_len; i++)
	{
	    if (boxes[i].count > 1 && (best_idx == -1
			|| boxes[i].count > boxes[best_idx].count))
		best_idx = i;
	}
	if (best_idx == -1)
	    break;

	box_to_split = boxes[best_idx];

	// Find the min and max value of each colour (r, g, b) within this box
	for (int i = box_to_split.start;
		i < box_to_split.start + box_to_split.count; i++)
	{
	    int	    px_index = pixel_index[i];
	    uint8_t *p = &img->data[px_index * img->fmt];

	    uint8_t r = p[0], g = p[1], b = p[2];


	    if (first)
	    {
		box_to_split.min_r = r;
		box_to_split.max_r = r;
		box_to_split.min_g = g;
		box_to_split.max_g = g;
		box_to_split.min_b = b;
		box_to_split.max_b = b;
		first = false;
	    }
	    else
	    {
		if (r < box_to_split.min_r)
		    box_to_split.min_r = r;
		else if (r > box_to_split.max_r)
		    box_to_split.max_r = r;

		if (g < box_to_split.min_g)
		    box_to_split.min_g = g;
		else if (g > box_to_split.max_g)
		    box_to_split.max_g = g;

		if (b < box_to_split.min_b)
		    box_to_split.min_b = b;
		else if (b > box_to_split.max_b)
		    box_to_split.max_b = b;
	    }
	}

	// Calculate the range of each colour axis and use the one with the
	// biggest range
	{
	    int range_r, range_g, range_b;

	    range_r = box_to_split.max_r - box_to_split.min_r;
	    range_g = box_to_split.max_g - box_to_split.min_g;
	    range_b = box_to_split.max_b - box_to_split.min_b;

	    if (range_r > range_g && range_r > range_b)
		axis_off = 0;
	    else if (range_g > range_r && range_g > range_b)
		axis_off = 1;
	    else
		axis_off = 2;
	}
	colour_off = axis_off;

	// Sort the pixel indices by the chosen color axis
	qsort(pixel_index + box_to_split.start, box_to_split.count,
		sizeof(int), pixel_compare_func);

	// Split the current box, pick the resulting box that has the most
	// pixels. If equal amount of pixels, then just pick any. Add the other
	// box to the array of boxes.
	{
	    color_box_T left, right;

	    left.start = box_to_split.start;
	    left.count = box_to_split.count / 2;
	    right.start = left.start + left.count;
	    right.count = box_to_split.count - left.count;

	    // Overwrite split box with left box
	    boxes[best_idx] = left;
	    boxes[boxes_len++] = right;
	}

	if (boxes_len == 256)
	    break;
    }

    // Generate palette. Each box is averaged into a single rgb value, which is
    // the palette entry.
    for (int i = 0; i < boxes_len; i++)
    {
	long sum_r = 0, sum_g = 0, sum_b = 0;

	for (int k = boxes[i].start; k < boxes[i].start + boxes[i].count; k++)
	{
	    uint8_t *p = &img->data[pixel_index[k] * img->fmt];
	    sum_r += p[0];
	    sum_g += p[1];
	    sum_b += p[2];
	}

	int n = boxes[i].count;
	ctx->palette[i].r = (uint8_t)(sum_r / n);
	ctx->palette[i].g = (uint8_t)(sum_g / n);
	ctx->palette[i].b = (uint8_t)(sum_b / n);
    }
    ctx->palette_len = boxes_len;

    vim_free(pixel_index);

    return OK;
}

/*
 * Generate the indexed array, each byte (pixel) is an index into the palette.
 * Returns OK on success and FAIL on failure.
 */
    static int
generate_index(image_T *img, image_sixel_T *ctx)
{
    int		w, h;
    int		n_pixels;
    garray_T	cache;

    image_get_dimensions(img, &w, &h);
    n_pixels = w * h;

    // For each pixel in the image, find the palette color that is closest to
    // it (euclidean distance)
    ctx->index = ALLOC_MULT(uint8_t, n_pixels);
    if (ctx->index == NULL)
	return FAIL;

    // Each cache entry is a 4 bytes, first 3 bytes is the rgb value, last byte
    // is the index into the palette.
    ga_init2(&cache, 4, 1024);

    for (int i = 0; i < n_pixels; i++)
    {
	uint8_t	    *px = &img->data[i * img->fmt];
	int	    index = 0, index_dist = -1;
	uint8_t	    *cp;

	// Check cache first
	for (int k = 0; k < cache.ga_len; k++)
	    if (memcmp((uint32_t *)cache.ga_data + k, px, 3) == 0)
	    {
		index = ((uint8_t *)((uint32_t *)cache.ga_data + k))[3];
		goto next;
	    }

	for (int k = 0; k < ctx->palette_len; k++)
	{
	    long dr = (long)px[0] - ctx->palette[k].r;
	    long dg = (long)px[1] - ctx->palette[k].g;
	    long db = (long)px[2] - ctx->palette[k].b;
	    long dist = dr*dr + dg*dg + db*db;

	    if (index_dist < 0 || dist < index_dist)
	    {
		index_dist = dist;
		index = k;
	    }
	}

	if (ga_grow(&cache, 1) == FAIL)
	{
	    ga_clear(&cache);
	    VIM_CLEAR(ctx->index);
	    return FAIL;
	}

	// Add to cache
	cp = (uint8_t *)((uint32_t *)cache.ga_data + cache.ga_len++);
	memcpy(cp, px, 3);
	cp[3] = (uint8_t)index;
next:
	ctx->index[i] = (uint8_t)index;
    }

    ga_clear(&cache);
    return OK;
}

    int
image_sixel_init(image_T *img)
{
    image_sixel_T *ctx;

    ctx = ALLOC_CLEAR_ONE(image_sixel_T);
    if (ctx == NULL || generate_palette(img, ctx) == FAIL
	    || generate_index(img, ctx) == FAIL)
    {
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

    vim_free(ctx->index);
    vim_free(img->backend_data);
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
    {
	vim_free(ctx->chunks[i].hdr_seq);
	vim_free(ctx->chunks[i].seq);
    }
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

/*
 * Sixel encode the region covered by "crop" in "img" into "hdr_buf" and "buf".
 * Note that "crop" should be fully valid (no out of bounds dimensions)! Returns
 * OK on success and FAIL on failure.
 */
    static int
encode_img(image_T *img, pixman_box32_t crop, garray_T *hdr_buf, garray_T *buf)
{
    image_sixel_T *ctx = img->backend_data;

    char    nr_buf[16];
    int	    nr_len;
    int	    img_w, img_h;
    int	    crop_w, crop_h;
    uint8_t *band;
    bool    *total_seen;
    bool    *seen;
    int	    *seen_list;
    int	    seen_count = 0;

    image_get_dimensions(img, &img_w, &img_h);
    crop_w = crop.x2 - crop.x1;
    crop_h = crop.y2 - crop.y1;

    // First add sixel header
    ga_concat_len(hdr_buf, (char_u *)"\033P0;1;8q\"1;1;", 13);

#define CONCAT_NR(b, n) \
    do \
    { \
	nr_len = vim_snprintf(nr_buf, sizeof(nr_buf), "%d", n); \
	ga_concat_len(b, (char_u *)nr_buf, nr_len); \
    } while (false)

    CONCAT_NR(hdr_buf, crop_w);
    (void)ga_append(hdr_buf, ';');
    CONCAT_NR(hdr_buf, crop_h);

    // Band buffer: one bitmask byte per (color, column) pair, reused per band.
    band = ALLOC_MULT(uint8_t, (size_t)ctx->palette_len * crop_w);
    if (band == NULL)
    {
	ga_clear(buf);
	return FAIL;
    }

    // Track which colors actually appear in the current band, so the emit pass
    // only visits colors that are present instead of looping over the entire
    // palette (which can be up to 256 entries, even when a band only uses a
    // handful of them).
    seen = ALLOC_CLEAR_MULT(bool, ctx->palette_len);
    total_seen = ALLOC_CLEAR_MULT(bool, ctx->palette_len);
    if (seen == NULL || total_seen == NULL)
    {
	vim_free(band);
	vim_free(seen);
	ga_clear(buf);
	return FAIL;
    }

    seen_list = ALLOC_MULT(int, ctx->palette_len);
    if (seen_list == NULL)
    {
	vim_free(seen);
	vim_free(total_seen);
	vim_free(band);
	ga_clear(buf);
	return FAIL;
    }

    for (int y0 = 0; y0 < crop_h; y0 += 6)
    {
	int band_h = (crop_h - y0 < 6) ? (crop_h - y0) : 6;
	int first_color_in_band = 1;

	memset(band, 0, (size_t)ctx->palette_len * crop_w);

	// Reset "seen" bookkeeping for just the colors we marked last band,
	// rather than the whole palette.
	for (int si = 0; si < seen_count; si++)
	    seen[seen_list[si]] = false;
	seen_count = 0;

	// Build the 6-row bitmask per color, per column, reading from the crop
	// region within ctx->index (which is img_w wide). Track which colors
	// are seen as a side effect of this same pass.
	for (int dy = 0; dy < band_h; dy++)
	{
	    int y = crop.y1 + y0 + dy;

	    for (int x = 0; x < crop_w; x++)
	    {
		uint8_t idx = ctx->index[y * img_w + (crop.x1 + x)];

		band[(size_t)idx * crop_w + x] |= (1 << dy);

		if (!seen[idx])
		{
		    seen[idx] = true;
		    total_seen[idx] = true;
		    seen_list[seen_count++] = idx;
		}
	    }
	}

	// Emit only the colors that actually appeared in this band, instead of
	// looping over the entire palette and checking each one.
	for (int si = 0; si < seen_count; si++)
	{
	    int	    c = seen_list[si];
	    uint8_t *row = band + (size_t)c * crop_w;
	    int	    x = 0;

	    if (!first_color_in_band)
		ga_append(buf, '$');
	    first_color_in_band = 0;

	    ga_append(buf, '#');
	    CONCAT_NR(buf, c);

	    // Run-length encode this color's sixel characters.
	    while (x < crop_w)
	    {
		uint8_t val = row[x];
		int	run = 1;
		char	ch;

		while (x + run < crop_w && row[x + run] == val)
		    run++;

		ch = (char)(val + 63);
		if (run > 3)
		{
		    ga_append(buf, '!');
		    CONCAT_NR(buf, run);
		    ga_append(buf, ch);
		}
		else
		{
		    for (int k = 0; k < run; k++)
			ga_append(buf, ch);
		}

		x += run;
	    }
	}

	ga_append(buf, '-'); // Advance to next band
    }

    // Color register definitions  #N;2;R;G;B  (RGB scaled to 0..100). Round to
    // nearest, not truncate, so the round-trip 8bit -> 0..100 -> 8bit stays
    // within ~1 level instead of drifting up to 2-3 levels darker.  This
    // matters when the popup blends RGBA alpha onto the terminal background:
    // truncation made the flattened bg visibly darker than the surrounding
    // terminal cells.
    //
    // Do this after we have encoded the main body. This is so we can only
    // define palette entries that have actually been used.
    for (int i = 0; i < ctx->palette_len; i++)
    {
	int r, g, b;

	if (!total_seen[i])
	    continue;

	r = (ctx->palette[i].r * 100 + 127) / 255;
	g = (ctx->palette[i].g * 100 + 127) / 255;
	b = (ctx->palette[i].b * 100 + 127) / 255;

	// "2" indicates RGB color space
	ga_append(hdr_buf, '#');
	CONCAT_NR(hdr_buf, i);
	ga_concat_len(hdr_buf, (char_u *)";2;", 3);
	CONCAT_NR(hdr_buf, r);
	ga_append(hdr_buf, ';');
	CONCAT_NR(hdr_buf, g);
	ga_append(hdr_buf, ';');
	CONCAT_NR(hdr_buf, b);
    }

    vim_free(seen_list);
    vim_free(total_seen);
    vim_free(seen);
    vim_free(band);

    // ST: end DCS sequence
    ga_concat_len(buf, (char_u *)"\033\\", 2);
    if (ga_append(hdr_buf, NUL) == FAIL || ga_append(buf, NUL) == FAIL)
	return FAIL;

#undef CONCAT_NR

    return OK;
}

    void
image_placement_sixel_draw(image_placement_T *place)
{
    image_T		    *img = place->img;
    image_placement_sixel_T *ctx = place->backend_data;
    pixman_box32_t	    *rects;
    int			    n_rects;

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
	    out_str(chunk->hdr_seq);
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

    cursor_off();

    for (int i = 0; i < n_rects; i++)
    {
	pixman_box32_t	rect = rects[i];
	pixman_box32_t	pixel_crop;
	int		row, col;
	int		x, y, w, h;
	sixel_chunk_T	*chunk = ctx->chunks + i;
	garray_T	buf;
	garray_T	hdr_buf;

	image_placement_subrect(place, rect, &row, &col, &x, &y, &w, &h);

	pixel_crop.x1 = x;
	pixel_crop.y1 = y;
	pixel_crop.x2 = x + w;
	pixel_crop.y2 = y + h;

	ga_init2(&hdr_buf, 1, 512);
	ga_init2(&buf, 1, 32768);
	if (encode_img(img, pixel_crop, &hdr_buf, &buf) == FAIL)
	{
	    ga_clear(&buf);
	    ga_clear(&hdr_buf);
	    continue;
	}

	term_windgoto(row, col);
	out_str(hdr_buf.ga_data);
	out_str(buf.ga_data);

	chunk->row_off = rect.y1;
	chunk->col_off = rect.x1;
	chunk->hdr_seq = hdr_buf.ga_data;
	chunk->seq = buf.ga_data;
    }

    if (ctx->visible_init)
	pixman_region32_clear(&ctx->visible_region);
    pixman_region32_copy(&ctx->visible_region, &place->visible);
    ctx->visible_init = true;

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
