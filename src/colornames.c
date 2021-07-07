/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 *
 * colornames.c: functions for naming colors for easier recall
 *
 */

#include "vim.h"

#if defined(FEAT_GUI) || defined(FEAT_TERMGUICOLORS) || defined(PROTO)

// On MS-Windows an RGB macro is available and it produces 0x00bbggrr color
// values as used by the MS-Windows GDI api.  It should be used only for
// MS-Windows GDI builds.
# if defined(RGB) && defined(MSWIN) && !defined(FEAT_GUI)
#  undef RGB
# endif
# ifndef RGB
#  define RGB(r, g, b)	((r<<16) | (g<<8) | (b))
# endif

# ifdef VIMDLL
    static guicolor_T
gui_adjust_rgb(guicolor_T c)
{
    if (gui.in_use)
	return c;
    else
	return ((c & 0xff) << 16) | (c & 0x00ff00) | ((c >> 16) & 0xff);
}
# else
#  define gui_adjust_rgb(c) (c)
# endif

    static int
hex_digit(int c)
{
    if (isdigit(c))
	return c - '0';
    c = TOLOWER_ASC(c);
    if (c >= 'a' && c <= 'f')
	return c - 'a' + 10;
    return 0x1ffffff;
}

    guicolor_T
decode_hex_color(char_u *hex)
{
    guicolor_T color;

    if (hex[0] != '#' || STRLEN(hex) != 7)
	return INVALCOLOR;

    // Name is in "#rrggbb" format
    color = RGB(((hex_digit(hex[1]) << 4) + hex_digit(hex[2])),
		((hex_digit(hex[3]) << 4) + hex_digit(hex[4])),
		((hex_digit(hex[5]) << 4) + hex_digit(hex[6])));
    if (color > 0xffffff)
	return INVALCOLOR;
    return gui_adjust_rgb(color);
}

#if defined(FEAT_EVAL)
// Returns the color currently mapped to the given name or INVALCOLOR if no
// such name exists in the color table. The convention is to use lowercase for
// all keys in the v:colornames dictionary. The value can be either a string in
// the form #rrggbb or a number, either of which is converted to a guicolor_T.
    guicolor_T
colorname2rgb(char_u *name)
{
    dict_T      *colornames_table = get_vim_var_dict(VV_COLORNAMES);
    char_u      *lc_name;
    dictitem_T  *colentry;
    char_u      *colstr;
    varnumber_T colnum;

    lc_name = strlow_save(name);
    if (lc_name == NULL)
	return INVALCOLOR;

    colentry = dict_find(colornames_table, lc_name, -1);
    vim_free(lc_name);
    if (colentry == NULL)
	return INVALCOLOR;

    if (colentry->di_tv.v_type == VAR_STRING)
    {
	colstr = tv_get_string_strict(&colentry->di_tv);
	if ((STRLEN(colstr) == 7) && (*colstr == '#'))
	{
	    return decode_hex_color(colstr);
	}
	else
	{
	    semsg("E1205: Bad color string: %s", colstr);
	    return INVALCOLOR;
	}
    }

    if (colentry->di_tv.v_type == VAR_NUMBER)
    {
	colnum = tv_get_number(&colentry->di_tv);
	return (guicolor_T)colnum;
    }

    return INVALCOLOR;
}

// Maps the given name to the given color value, overwriting any current
// mapping. If allocation fails the named color will no longer exist in the
// table and the user will receive an error message.
    void
save_colorname_hexstr(int r, int g, int b, char_u *name)
{
    int        result;
    dict_T     *colornames_table;
    dictitem_T *existing;
    char_u     hexstr[8];

    if (snprintf((char *)hexstr, sizeof(hexstr), "#%02x%02x%02x", r, g, b) < 0)
    {
	semsg(_(e_alloc_color), name);
	return;
    }

    colornames_table = get_vim_var_dict(VV_COLORNAMES);
    // The colornames_table dict is safe to use here because it is allocated at
    // startup in evalvars.c
    existing = dict_find(colornames_table, name, -1);
    if (existing != NULL)
    {
	dictitem_remove(colornames_table, existing);
	existing = NULL; // dictitem_remove freed the item
    }

    result = dict_add_string(colornames_table, (char *)name, hexstr);
    if (result == FAIL)
	semsg(_(e_alloc_color), name);
}

// Establishes a color alias for each entry in rgb.txt. Repeated invocations
// will only load the color data once.
    void
load_rgb_txt() {
# define LINE_LEN 100
    FILE	*fd;
    char	line[LINE_LEN];
    char_u	*fname;
    int rgb_lines = 0;
    int		r, g, b, i;
    static char already_loaded = 0;

    if (already_loaded == 1)
	return;


    fname = expand_env_save((char_u *)"$VIMRUNTIME/rgb.txt");
    if (fname == NULL)
    {
	// Not really loaded but don't try again.
	already_loaded = 1;
	return;
    }

    fd = fopen((char *)fname, "rt");
    vim_free(fname);
    if (fd == NULL)
    {
	if (p_verbose > 1)
	    verb_msg(_("Cannot open $VIMRUNTIME/rgb.txt"));
	already_loaded = 1;
	return;
    }

    while (!feof(fd))
    {
	size_t	len;
	int	pos;
	char_u  *s;

	vim_ignoredp = fgets(line, LINE_LEN, fd);
	len = strlen(line);

	if (len <= 1 || line[len - 1] != '\n')
	    continue;

	line[len - 1] = '\0';

	i = sscanf(line, "%d %d %d %n", &r, &g, &b, &pos);
	if (i != 3)
	    continue;

	s = strlow_save((char_u *)line + pos);

	if (s == NULL)
	{
	    fclose(fd);
	    return;
	}
	save_colorname_hexstr(r, g, b, s);
	vim_free(s);
	s = NULL;
	rgb_lines++;

	// The distributed rgb.txt has less than 1000 entries. Limit to
	// 10000, just in case the file was messed up.
	if (rgb_lines == 10000)
	    break;
    }

    already_loaded = 1;
    fclose(fd);
}
#endif

    guicolor_T
gui_get_color_cmn(char_u *name)
{
    int         i;
    guicolor_T  color;

    struct rgbcolor_table_S {
	char_u	    *color_name;
	guicolor_T  color;
    };

    // Only non X11 colors (not present in rgb.txt) and colors in
    // color_names[], useful when $VIMRUNTIME is not found,.
    static struct rgbcolor_table_S rgb_table[] = {
	    {(char_u *)"black",		RGB(0x00, 0x00, 0x00)},
	    {(char_u *)"blue",		RGB(0x00, 0x00, 0xFF)},
	    {(char_u *)"brown",		RGB(0xA5, 0x2A, 0x2A)},
	    {(char_u *)"cyan",		RGB(0x00, 0xFF, 0xFF)},
	    {(char_u *)"darkblue",	RGB(0x00, 0x00, 0x8B)},
	    {(char_u *)"darkcyan",	RGB(0x00, 0x8B, 0x8B)},
	    {(char_u *)"darkgray",	RGB(0xA9, 0xA9, 0xA9)},
	    {(char_u *)"darkgreen",	RGB(0x00, 0x64, 0x00)},
	    {(char_u *)"darkgrey",	RGB(0xA9, 0xA9, 0xA9)},
	    {(char_u *)"darkmagenta",	RGB(0x8B, 0x00, 0x8B)},
	    {(char_u *)"darkred",	RGB(0x8B, 0x00, 0x00)},
	    {(char_u *)"darkyellow",	RGB(0x8B, 0x8B, 0x00)}, // No X11
	    {(char_u *)"gray",		RGB(0xBE, 0xBE, 0xBE)},
	    {(char_u *)"green",		RGB(0x00, 0xFF, 0x00)},
	    {(char_u *)"grey",		RGB(0xBE, 0xBE, 0xBE)},
	    {(char_u *)"grey40",	RGB(0x66, 0x66, 0x66)},
	    {(char_u *)"grey50",	RGB(0x7F, 0x7F, 0x7F)},
	    {(char_u *)"grey90",	RGB(0xE5, 0xE5, 0xE5)},
	    {(char_u *)"lightblue",	RGB(0xAD, 0xD8, 0xE6)},
	    {(char_u *)"lightcyan",	RGB(0xE0, 0xFF, 0xFF)},
	    {(char_u *)"lightgray",	RGB(0xD3, 0xD3, 0xD3)},
	    {(char_u *)"lightgreen",	RGB(0x90, 0xEE, 0x90)},
	    {(char_u *)"lightgrey",	RGB(0xD3, 0xD3, 0xD3)},
	    {(char_u *)"lightmagenta",	RGB(0xFF, 0x8B, 0xFF)}, // No X11
	    {(char_u *)"lightred",	RGB(0xFF, 0x8B, 0x8B)}, // No X11
	    {(char_u *)"lightyellow",	RGB(0xFF, 0xFF, 0xE0)},
	    {(char_u *)"magenta",	RGB(0xFF, 0x00, 0xFF)},
	    {(char_u *)"red",		RGB(0xFF, 0x00, 0x00)},
	    {(char_u *)"seagreen",	RGB(0x2E, 0x8B, 0x57)},
	    {(char_u *)"white",		RGB(0xFF, 0xFF, 0xFF)},
	    {(char_u *)"yellow",	RGB(0xFF, 0xFF, 0x00)},
    };

    color = decode_hex_color(name);
    if (color != INVALCOLOR)
	return color;

    // Check if the name is one of the colors we know
    for (i = 0; i < (int)ARRAY_LENGTH(rgb_table); i++)
	if (STRICMP(name, rgb_table[i].color_name) == 0)
	    return gui_adjust_rgb(rgb_table[i].color);

#if defined(FEAT_EVAL)
    /*
     * Not a traditional color. Load rgb.txt color aliases and then consult the alias table.
     */
    load_rgb_txt();

    return colorname2rgb(name);
#else
    return INVALCOLOR;
#endif
}

    guicolor_T
gui_get_rgb_color_cmn(int r, int g, int b)
{
    guicolor_T  color = RGB(r, g, b);

    if (color > 0xffffff)
	return INVALCOLOR;
    return gui_adjust_rgb(color);
}

#endif
