/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * See https://www.cairographics.org/documentation/using_the_postscript_surface/
 */

#include "vim.h"
#include "version.h"

#ifdef FEAT_PRINT_PANGO

#include "hardcopy.h"
#include "hardcopy_pango.h"

#include <cairo/cairo-ps.h>
#include <pango/pango.h>

#define PRINT_DPI 72.0

#define PRT_DRAW_BOLD 1
#define PRT_DRAW_ITALIC 2
#define PRT_DRAW_UNDERLINE 4

#define INSERT_PANGO_ATTR(Attribute, AttrList, Start, End)  \
    G_STMT_START{					    \
	PangoAttribute *tmp_attr_;			    \
	tmp_attr_ = (Attribute);			    \
	tmp_attr_->start_index = (Start);		    \
	tmp_attr_->end_index = (End);			    \
	pango_attr_list_insert((AttrList), tmp_attr_);	    \
    }G_STMT_END

/*
 * Global state for printing
 */
static struct
{
    // Cairo surface to draw on (postscript backend). Note that surface is
    // scaled in points (1/72 of an inch).
    cairo_surface_t *surface;
    cairo_t	    *cr;
    PangoContext    *text_context;

    PangoFontDescription    *font;
    bool		    font_can_bold;

    char_u *filename;

    int media;
    bool portrait;
    bool collate;
    bool duplex;
    bool tumble; // Short edge duplex

    double char_width;
    double char_height;
    double line_height;
    double underline_pos;
    double underline_thickness;

    double page_width;
    double page_height;

    double left_margin;
    double right_margin;
    double top_margin;
    double bottom_margin;

    double number_width;

    // Current position to draw from
    double cur_x;
    double cur_y;
    long_u cur_fg;
    long_u cur_bg;

    char_u draw_flags;

    // Used for generating DSC comments
#define DSC_BUFSIZE 257
    char buf[DSC_BUFSIZE];
} pctx;

/*
 * Convert the margin specified in "idx" to device units. "physsize" is the size
 * (width or height).
 */
    static double
to_device_units(int idx, double physsize, int def_number)
{
    float	ret;
    int		u;
    int		nr;

    u = prt_get_unit(idx);
    if (u == PRT_UNIT_NONE)
    {
	u = PRT_UNIT_PERC;
	nr = def_number;
    }
    else
	nr = printer_opts[idx].number;

    switch (u)
    {
	case PRT_UNIT_INCH:
	    ret = (nr * PRINT_DPI);
	    break;
	case PRT_UNIT_MM:
	    ret = (nr * PRINT_DPI) / 25.4;
	    break;
	case PRT_UNIT_POINT:
	    ret = (double)nr;
	    break;
	case PRT_UNIT_PERC:
	default:
	    ret = physsize * (nr / 100.0);
	    break;
    }

    return ret;
}

/*
 * Calculate margins for given width and height from printoptions settings.
 */
    static void
get_page_margins(
	double	width,
	double	height,
	double	*left,
	double	*right,
	double	*top,
	double	*bottom)
{
    *left   = to_device_units(OPT_PRINT_LEFT, width, 10);
    *right  = width - to_device_units(OPT_PRINT_RIGHT, width, 5);

    *top    = to_device_units(OPT_PRINT_TOP, height, 5);
    *bottom = height - to_device_units(OPT_PRINT_BOT, height, 5);
}

/*
 * Get characters per line (excluding margins).
 */
    static int
get_cpl(void)
{
    if (prt_use_number())
    {
	pctx.number_width = PRINT_NUMBER_WIDTH * pctx.char_width;
	pctx.left_margin += pctx.number_width;
    }
    else
	pctx.number_width = 0.0;

    return (int)((pctx.right_margin - pctx.left_margin) / pctx.char_width);
}

/*
 * Get number of lines of text that fit on a page (excluding the header).
 */
    static int
get_lpp(void)
{
    int lpp;

    // Calculate lpp
    lpp = (int)((pctx.bottom_margin - pctx.top_margin) / pctx.line_height);

    // Adjust top margin if there is a header
    pctx.top_margin += pctx.line_height * prt_header_height();

    return lpp - prt_header_height();
}

/*
 * Get the font descrption from the given name. Returns NULL on failure.
 */
    static PangoFontDescription *
get_font(char_u *name)
{
    PangoFontDescription *font;

    if (name == NULL)
	return NULL;

    font = pango_font_description_from_string((const char *)name);
    if (font == NULL)
    {
	semsg(_(e_unknown_printer_font_str), name);
	return NULL;
    }

    // Ensure a size is set
    if (pango_font_description_get_size(font) <= 0)
	pango_font_description_set_size(font, 10 * PANGO_SCALE);

    return font;
}

    int
mch_print_init(
	prt_settings_T  *psettings,
	char_u	    *jobname,
	int		    forceit UNUSED)
{
    char_u	*paper_name;
    int		paper_strlen;
    bool	portrait;
    int		media;
    char_u	*filename = NULL;

#if defined(FEAT_GUI_GTK) && defined(USE_GTK4)
    if (!forceit)
	;
    // TODO
#endif

    // Get paper type to use
    portrait = (!printer_opts[OPT_PRINT_PORTRAIT].present
	    || TOLOWER_ASC(printer_opts[OPT_PRINT_PORTRAIT].string[0]) == 'y');
    pctx.portrait = portrait;
    if (printer_opts[OPT_PRINT_PAPER].present)
    {
	paper_name = printer_opts[OPT_PRINT_PAPER].string;
	paper_strlen = printer_opts[OPT_PRINT_PAPER].strlen;
    }
    else
    {
	paper_name = (char_u *)"A4";
	paper_strlen = 2;
    }

    // Set width and height using given paper type
    for (media = 0; media < (int)PRT_MEDIASIZE_LEN; ++media)
	if ((int)STRLEN(prt_mediasize[media].name) == paper_strlen
		&& STRNICMP(prt_mediasize[media].name,
		    paper_name, paper_strlen) == 0)
	    break;
    if (media == PRT_MEDIASIZE_LEN)
	media = 0;
    pctx.media = media;

    if (portrait)
    {
	pctx.page_width = prt_mediasize[media].width;
	pctx.page_height = prt_mediasize[media].height;
    }
    else
    {
	pctx.page_width = prt_mediasize[media].height;
	pctx.page_height = prt_mediasize[media].width;
    }

    psettings->user_abort = FALSE;

    // If the user didn't specify a file name, use a temp file.
    if (psettings->outfile == NULL)
    {
	filename = vim_tempname('p', TRUE);
	if (filename == NULL)
	{
	    emsg(_(e_cant_get_temp_file_name));
	    return FAIL;
	}
    }
    else
	filename = expand_env_save(psettings->outfile);

    pctx.surface = cairo_ps_surface_create((const char *)filename,
	    pctx.page_width, pctx.page_height);
    pctx.filename = (char_u *)filename;

    if (cairo_surface_status(pctx.surface) != CAIRO_STATUS_SUCCESS)
    {
	emsg(_(e_cant_open_postscript_output_file));
	mch_print_cleanup();
	return FAIL;
    }

    pctx.cr = cairo_create(pctx.surface);
    pctx.text_context = pango_cairo_create_context(pctx.cr);
    pctx.font = get_font(p_pfn);

    if (pctx.font == NULL)
    {
	mch_print_cleanup();
	return FAIL;
    }

    pango_context_set_font_description(pctx.text_context, pctx.font);
    // Pango DPI is 96, must adjust
    pango_cairo_context_set_resolution(pctx.text_context, 72.0);

    // Check if bold font is supported, if not then emulate it
    {
	PangoFontDescription    *bold_font_desc;
	PangoFont		*plain_font;
	PangoFont		*bold_font;

	pctx.font_can_bold = FALSE;

	plain_font = pango_context_load_font(pctx.text_context, pctx.font);
	if (plain_font != NULL)
	{
	    bold_font_desc = pango_font_description_copy_static(pctx.font);
	    pango_font_description_set_weight(bold_font_desc,
		    PANGO_WEIGHT_BOLD);

	    bold_font = pango_context_load_font(pctx.text_context,
		    bold_font_desc);
	    if (bold_font != NULL)
	    {
		pctx.font_can_bold = (bold_font != plain_font);
		g_object_unref(bold_font);
	    }

	    pango_font_description_free(bold_font_desc);
	    g_object_unref(plain_font);
	}
    }

    // Get line height and char width
    {
	PangoFontMetrics    *metrics;
	int		    ascent, descent;

	metrics = pango_context_get_metrics(pctx.text_context, pctx.font, NULL);

	pctx.char_width =
	    (double)pango_font_metrics_get_approximate_char_width(metrics)
	    / PANGO_SCALE;

	ascent = pango_font_metrics_get_ascent(metrics);
	descent = pango_font_metrics_get_descent(metrics);
	pctx.char_height = (double)(ascent + descent) / PANGO_SCALE;

	pctx.line_height = (double)pango_font_metrics_get_height(metrics)
	    / PANGO_SCALE;

	pctx.underline_pos =
	    (double)pango_font_metrics_get_underline_position(metrics)
	    / PANGO_SCALE;
	pctx.underline_thickness =
	    (double)pango_font_metrics_get_underline_thickness(metrics)
	    / PANGO_SCALE;

	pango_font_metrics_unref(metrics);
    }

    // Calculate margins
    {
	double  left;
	double	right;
	double	top;
	double	bottom;

	get_page_margins(pctx.page_width, pctx.page_height, &left, &right, &top,
		&bottom);
	pctx.left_margin = left;
	pctx.right_margin = right;
	pctx.top_margin = top;
	pctx.bottom_margin = bottom;
    }

    psettings->chars_per_line = get_cpl();
    psettings->lines_per_page = get_lpp();

    pctx.collate = (!printer_opts[OPT_PRINT_COLLATE].present
	    || TOLOWER_ASC(printer_opts[OPT_PRINT_COLLATE].string[0]) == 'y');

    // Postscript handles collating pages, don't let Vim handle it.
    psettings->n_collated_copies = 1;
    psettings->n_uncollated_copies = 1;

    psettings->jobname = jobname;

    pctx.duplex = TRUE;
    pctx.tumble = FALSE;
    psettings->duplex = TRUE;
    if (printer_opts[OPT_PRINT_DUPLEX].present)
    {
	if (STRNICMP(printer_opts[OPT_PRINT_DUPLEX].string, "off", 3) == 0)
	{
	    pctx.duplex = FALSE;
	    psettings->duplex = 0;
	}
	else if (STRNICMP(printer_opts[OPT_PRINT_DUPLEX].string, "short", 5)
		== 0)
	    pctx.tumble = TRUE;
    }

    pctx.cur_x = pctx.cur_y = 0;
    pctx.draw_flags = 0;

    return OK;
}

/*
 * Emit a DSC comment, "fmt" may be NULL to indicate no arguments.
 */
    static void
emit_dsc_comment(char *comment, char *fmt, ...)
{
    va_list ap;
    int	    len = DSC_BUFSIZE;
    int	    o;
    char    *s = pctx.buf;

    o = vim_snprintf(s, DSC_BUFSIZE, "%%%%%s: ", comment);

    if (fmt == NULL)
	goto exit;

    len -= o;
    s += o;
    if (len <= 0)
	return;

    va_start(ap, fmt);
    vim_vsnprintf(s, len, fmt, ap);
    va_end(ap);

exit:
    cairo_ps_surface_dsc_comment(pctx.surface, pctx.buf);
}

    int
mch_print_begin(prt_settings_T *psettings)
{
    char buf[256];

    // In header section
    emit_dsc_comment("Title", (char *)psettings->jobname);
    if (!get_user_name((char_u *)buf, sizeof(buf)))
	STRCPY(buf, "Unknown");
    emit_dsc_comment("For", buf);
    emit_dsc_comment("Orientation", pctx.portrait ? "Portrait" : "Landscape");
    emit_dsc_comment("PageOrder", "Ascend");

    emit_dsc_comment("DocumentMedia",
	    "%s %.2lf %.2lf 0 () ()",
	    prt_mediasize[pctx.media].name,
	    prt_mediasize[pctx.media].width,
	    prt_mediasize[pctx.media].height);

    emit_dsc_comment("Requirements", "%s%s %s %s",
	    pctx.duplex ? "duplex" : "",
	    pctx.tumble ? "(tumble)" : "",
	    pctx.collate ? "collate" : "",
	    psettings->do_syntax ? "color" : "");

    // In defaults section (defaults for every page)
    emit_dsc_comment("BeginDefaults", NULL);
    emit_dsc_comment("PageMedia", prt_mediasize[pctx.media].name);
    emit_dsc_comment("EndDefaults", NULL);

    return TRUE;
}

    void
mch_print_end(prt_settings_T *psettings)
{
    // Make sure surface flushes to file or stream
    cairo_surface_finish(pctx.surface);

    if (psettings->outfile == NULL && !got_int && !psettings->user_abort
	    && pctx.filename != NULL)
    {
	msg(_("Sending to printer..."));

	// Not printing to a file: use 'printexpr' to print the file.
	if (eval_printexpr(pctx.filename, psettings->arguments) == FAIL)
	    emsg(_(e_failed_to_print_postscript_file));
	else
	    msg(_("Print job sent."));
    }

    mch_print_cleanup();
}

    int
mch_print_end_page(void)
{
    cairo_show_page(pctx.cr);
    return TRUE;
}

/*
 * Do nothing because we create a new page for the cairo surface at the end of
 * every page.
 */
    int
mch_print_begin_page(char_u *str UNUSED)
{
    return TRUE;
}

    int
mch_print_blank_page(void)
{
    mch_print_end_page();
    return TRUE;
}

    void
mch_print_start_line(int margin, int page_line)
{
    pctx.cur_x = pctx.left_margin;
    if (margin)
	pctx.cur_x -= pctx.number_width;

    pctx.cur_y = pctx.top_margin + page_line * pctx.line_height;
}

    static void
long_to_rgb(long_u c, double *r, double *g, double *b)
{
    *r = ((c >> 16) & 0xFF) / 255.0;
    *g = ((c >> 8) & 0xFF) / 255.0;
    *b = ((c >> 0) & 0xFF) / 255.0;
}

    int
mch_print_text_out(char_u *textp, int len)
{
    char_u		*str;
    PangoLayout		*layout;
    PangoAttrList	*attr_list;
    int			width_int;
    double		width;
    double		start_y;
    double		r, g, b;
    double		next_x;

    // Always convert to UTF-8, Pango expects that
    if (output_conv.vc_type == CONV_NONE)
	str = textp;
    else
	str = string_convert(&output_conv, textp, &len);

    if (str == NULL)
	return TRUE;

    layout = pango_layout_new(pctx.text_context);
    pango_layout_set_text(layout, (const char *)textp, len);

    // Set layout attributes. apply for entire layout. "textp" represents a
    // single character/glyph, so its not really a "string" of text.
    attr_list = pango_attr_list_new();

    if ((pctx.draw_flags & PRT_DRAW_BOLD) && pctx.font_can_bold)
	INSERT_PANGO_ATTR(pango_attr_weight_new(PANGO_WEIGHT_BOLD),
		attr_list, 0, len);
    if (pctx.draw_flags & PRT_DRAW_ITALIC)
	INSERT_PANGO_ATTR(pango_attr_style_new(PANGO_STYLE_ITALIC),
		attr_list, 0, len);

    pango_layout_set_attributes(layout, attr_list);
    pango_attr_list_unref(attr_list);

    // Get logical width to advance by (and also draw bg color).
    pango_layout_get_size(layout, &width_int, NULL);
    width = width_int;
    width /= PANGO_SCALE;

    // Draw background color
    long_to_rgb(pctx.cur_bg, &r, &g, &b);
    cairo_set_source_rgb(pctx.cr, r, g, b);
    // Don't start at line bottom of previous line, since we don't want to draw
    // over any ink. Instead offset by the "line spacing".
    start_y = pctx.cur_y + (pctx.line_height - pctx.char_height);
    cairo_rectangle(pctx.cr, pctx.cur_x, start_y, width, pctx.char_height);
    cairo_fill(pctx.cr);

    // Set foreground and draw layout text
    long_to_rgb(pctx.cur_fg, &r, &g, &b);
    cairo_set_source_rgb(pctx.cr, r, g, b);
    cairo_move_to(pctx.cr, pctx.cur_x, pctx.cur_y);
    pango_cairo_show_layout(pctx.cr, layout);

    // Draw underline last (if needed), use foreground color
    if (pctx.draw_flags & PRT_DRAW_UNDERLINE)
    {
	double baseline = (double)pango_layout_get_baseline(layout) / PANGO_SCALE;
	double uy = start_y + baseline - pctx.underline_pos;

	cairo_set_line_width(pctx.cr, pctx.underline_thickness);
	cairo_move_to(pctx.cr, pctx.cur_x, uy);
	cairo_line_to(pctx.cr, pctx.cur_x + width, uy);
	cairo_stroke(pctx.cr);
    }

    g_object_unref(layout);
    if (output_conv.vc_type != CONV_NONE)
	vim_free(str);

    pctx.cur_x += width;
    next_x = pctx.cur_x + pctx.char_width;

    return (next_x > pctx.right_margin) &&
	((next_x - pctx.right_margin) > (pctx.right_margin * 1e-5));
}

    void
mch_print_set_font(int bold, int italic, int underline)
{
    pctx.draw_flags = 0;

    if (bold)
	pctx.draw_flags |= PRT_DRAW_BOLD;
    if (italic)
	pctx.draw_flags |= PRT_DRAW_ITALIC;
    if (underline)
	pctx.draw_flags |= PRT_DRAW_UNDERLINE;
}

    void
mch_print_set_bg(long_u bgcol)
{
    pctx.cur_bg = bgcol;
}

    void
mch_print_set_fg(long_u fgcol)
{
    pctx.cur_fg = fgcol;
}

    void
mch_print_cleanup(void)
{
    g_clear_pointer(&pctx.cr, cairo_destroy);
    g_clear_pointer(&pctx.surface, cairo_surface_destroy);
    g_clear_object(&pctx.text_context);
    g_clear_pointer(&pctx.font, pango_font_description_free);
    g_clear_pointer(&pctx.filename, vim_free);
}

#endif // FEAT_PRINT_PANGO
