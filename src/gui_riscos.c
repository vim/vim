/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"
#include <string.h>

/*
 * gui_riscos.c
 *
 * Thomas Leonard <tal197@ecs.soton.ac.uk>
 * Updated by Andy Wingate <andy@sparse.net>
 */

extern int time_of_last_poll;

int task_handle = 0;		/* Zero means we are not yet a Wimp task */
int child_handle = 0;		/* Task handle of our child process (zero if none). */
int *wimp_menu = (int *) -1;	/* Pointer to a Wimp menu structure (or -1) */
int save_window = -1;		/* Save As window handle */

int *redraw_block = NULL;	/* NULL means not in a redraw loop. */
int ro_return_early = FALSE;	/* Break out of gui_mch_wait_for_chars() */

int leaf_ref = 0;		/* Wimp message number - send via Wimp$Scrap */
char_u *leaf_name = NULL;	/* Leaf name from DataSave */

int default_columns = 120;	/* These values are used if the --rows and --columns */
int default_rows = 32;		/* options aren't used on startup. */

#define DRAG_FALSE	    0
#define DRAG_SELECTION	    1
#define DRAG_RESIZE_WINDOW  2
int ro_dragging = DRAG_FALSE;
int drag_button;
int drag_modifiers;
int drag_x_offset;
int drag_y_offset;

int nested_wimp = FALSE;	/* Bool - can we use the new wimp? */

int changed_mode = FALSE;
int x_eigen_factor;
int y_eigen_factor;

/* If ro_current_font is non-zero then use the outline font with that handle,
 * otherwise, if zap_redraw is TRUE then use ZapRedraw, otherwise use the
 * system font.
 *
 * If zap_redraw is TRUE then zap_file[] contains valid Zap font file
 * pointers (or NULLs).
 */
int ro_current_font = 0;	/* 0 is system font, or ZapRedraw */
int font_x_offset   = 0;	/* Where to position each char in its box */
int font_y_offset   = 0;

int zap_redraw	    = FALSE;
int double_height   = FALSE;	/* Plot each line twice? */

#define grgb(r,g,b) ((b<<16) + (g<<8) + (r))
#define UNUSED_COLOUR (gui.back_pixel)

#define RO_LOAD_CLIPBOARD -2	/* Internal handle for DataSave message. */

/* Changes by John Kortink, 22-23 July 1998
 *
 * Stuff to make redraw a lot faster. Almost all of it is right here below,
 * elsewhere changes are marked with 'JK230798'. Apart from a small change in
 * 'gui.c' all changes are limited to this file, 'gui_riscos.c'. The change in
 * 'gui.c' is to make Vim stop being 'smart' not redrawing characters that are
 * 'already there' (i.e. from the previous line, by coincidence). This caused a
 * lot more calls to the redraw code, which we want to avoid because a few nice
 * big strings at a time is a lot faster than a truckload of small ones. ('Dear
 * Bram ...').
 */

/* The ZapRedraw structure */

static struct
{
    int		r_flags;
    int		r_minx;
    int		r_miny;
    int		r_maxx;
    int		r_maxy;
    int		r_screen;
    int		r_bpl;
    int		r_bpp;
    int		r_charw;
    int		r_charh;
    char	*r_caddr;
    int		r_cbpl;
    int		r_cbpc;
    int		r_linesp;
    int		r_data;
    int		r_scrollx;
    int		r_scrolly;
    int		*r_palette;
    int		r_for;
    int		r_bac;
    char	*r_workarea;
    int		r_magx;
    int		r_magy;
    int		r_xsize;
    int		r_ysize;
    int		r_mode;
}
zap_redraw_block;

/* Other globals */

static int	zap_redraw_initialised = FALSE;
static int	zap_redraw_update_colours;
static int	zap_redraw_colours[2];
static int	zap_redraw_palette[16];

/* Holds the current Zap font file(s).
 * The font is recreated from this block on a mode change.
 * When using zap, element ZAP_NORMAL is always valid, but
 * the others can be NULL.
 */

#define ZAP_NORMAL  0
#define ZAP_BOLD    1
#define ZAP_ITALIC  2
#define ZAP_BITALIC 3
#define ZAP_STYLES  4

/* Zap font file format data */
static char	*zap_file[ZAP_STYLES] = {NULL, NULL, NULL, NULL};

/* r_caddr format for current mode */
static char	*zap_caddr[ZAP_STYLES] = {NULL, NULL, NULL, NULL};

static void ro_remove_menu(int *menu);

/*
 * Initialise all the ZapRedraw stuff.
 * Call this when changing font and after each mode change.
 * zap_redraw_bitmap must contain a valid Zap font file (possibly
 * created from the system font).
 *
 * Return FAIL to revert to system font (if we can't use ZapRedraw).
 */
    int
ro_zap_redraw_initialise()
{
    int	    bytes_per_bitmap_char;
    int	    first, last;
    int	    i;

    /* Can't have initialisers for struct members :-(, ok, this way then ... */
    if (!zap_redraw_initialised)
    {
	zap_redraw_block.r_workarea = NULL;
	zap_redraw_initialised = TRUE;
    }

    /* We redraw in DSA mode */
    zap_redraw_block.r_flags = 0x0;

    /* Let ZapRedraw get the screen address for us */
    zap_redraw_block.r_screen = 0;

    /* Read the font width and height from the font file header.
     * Assume that all styles are the same size.
     * ZAP_NORMAL is always present.
     */
    zap_redraw_block.r_charw = ((int *) zap_file[ZAP_NORMAL])[2];
    zap_redraw_block.r_charh = ((int *) zap_file[ZAP_NORMAL])[3];

    /* We have no linespacing */
    zap_redraw_block.r_linesp = 0;

    /* Fix foreground = colour 1 */
    zap_redraw_block.r_for = 1;

    /* Fix background = colour 0 */
    zap_redraw_block.r_bac = 0;

    /* Colour mask buffer */
    zap_redraw_block.r_palette = zap_redraw_palette;

    /* Allocate local workspace (for the few calls following here) */
    if (zap_redraw_block.r_workarea != NULL)
	free(zap_redraw_block.r_workarea);
    zap_redraw_block.r_workarea = (char*) malloc(128);
    if (!zap_redraw_block.r_workarea)
	return FAIL;	/* Out of memory */

    /* Fill in VDU variables */
    if (xswi(ZapRedraw_ReadVduVars, 0, &zap_redraw_block) & v_flag)
	return FAIL;	    /* Can't find ZapRedraw module - use VDU instead */

    /* Determine cbpl and cbpc */
    swi(ZapRedraw_CachedCharSize, zap_redraw_block.r_bpp, 0,
	zap_redraw_block.r_charw, zap_redraw_block.r_charh);
    zap_redraw_block.r_cbpl = r2;
    zap_redraw_block.r_cbpc = r3;

    /* Allocate general workspace (for the calls outside) */
    if (zap_redraw_block.r_workarea != NULL)
	free(zap_redraw_block.r_workarea);
    zap_redraw_block.r_workarea = (char*) malloc(128 + zap_redraw_block.r_cbpl);
    if (!zap_redraw_block.r_workarea)
	return FAIL;	/* Out of memory */

    /* Now convert the 1 bpp character data ready for the current mode */

    bytes_per_bitmap_char = (zap_redraw_block.r_charw * zap_redraw_block.r_charh + 7) / 8;

    /* Convert the fonts from 1bpp to a format suitable for the
     * current mode.
     */
    for (i = 0; i < ZAP_STYLES; i++)
    {
	first = ((int *) zap_file[i])[4];
	last  = ((int *) zap_file[i])[5];

	if (last > 255)
	    last = 255;	/* Don't convert cursors (overwrites memory!) */

	/* Allocate the font cache */
	vim_free(zap_caddr[i]);
	if (zap_file[i])
	    zap_caddr[i] = (char*) malloc(zap_redraw_block.r_cbpc * 256);
	else
	    zap_caddr[i] = NULL;    /* No file for this style */

	if (zap_caddr[i])
	{
	    zap_redraw_block.r_caddr = zap_caddr[i];

	    swi(ZapRedraw_ConvertBitmap, 0, &zap_redraw_block,
		    first, last,		/* Range of characters to convert */
		    zap_file[i] + 0x20	/* Addr of first char provided by font */
		    - first * bytes_per_bitmap_char);
	}
    }

    if (!zap_caddr[ZAP_NORMAL])
    {
	zap_redraw = FALSE;	/* Out of memory */
	return FAIL;
    }

    /* Next time we need them, we have to update the colour masks */
    zap_redraw_update_colours = TRUE;

    return OK;
}

/*
 * Redraw a string at OS coordinates <x,y> (top-left, x inclusive, y exclusive).
 * Graphics clip window is window[0..3] as in R1+28..40 of Wimp_RedrawWindow.
 * Returns (possibly modified) flags.
 */
    int
ro_zap_redraw_draw_string(x, y, string, length, flags, clip)
    int	    x;
    int	    y;
    char    *string;
    int	    length;
    int	    flags;	/* DRAW_TRANSP, DRAW_BOLD, DRAW_UNDERL, DRAW_ITALIC */
    int	    *clip;
{
    char redraw_data[1024];
    int clip_minx;
    int clip_miny;
    int clip_maxx;
    int clip_maxy;
    int os_xshift = zap_redraw_block.r_magx;
    int os_yshift = zap_redraw_block.r_magy;

    if (flags & DRAW_TRANSP)
	return flags;	/* We don't do transparent plotting yet. */

    if (flags & DRAW_BOLD)
    {
	if (flags & DRAW_ITALIC && zap_caddr[ZAP_BITALIC])
	    zap_redraw_block.r_caddr = zap_caddr[ZAP_BITALIC];
	else
	    zap_redraw_block.r_caddr = zap_caddr[ZAP_BOLD];
    }
    else
    {
	if (flags & DRAW_ITALIC)
	    zap_redraw_block.r_caddr = zap_caddr[ZAP_ITALIC];
	else
	    zap_redraw_block.r_caddr = zap_caddr[ZAP_NORMAL];
    }
    if (!zap_redraw_block.r_caddr)
    {
	zap_redraw_block.r_caddr = zap_caddr[ZAP_NORMAL];
	flags |= DRAW_UNDERL;	    /* Style missing - we can always underline */
    }

    /* Set the vertical scaling flag */
    if (double_height)
	zap_redraw_block.r_flags = 1 << 1;
    else
	zap_redraw_block.r_flags = 0;

    /* Update the colour masks (if needed) */
    if (zap_redraw_update_colours)
    {
	swi(ZapRedraw_CreatePalette, 2,
		&zap_redraw_block,
		zap_redraw_colours,
		zap_redraw_block.r_palette, 2);
	zap_redraw_update_colours = FALSE;
    }

    /* Target rectangle in ZapRedraw rectangle coordinates (pixels, Y-min/max reversed !!!) */
    zap_redraw_block.r_minx = x >> os_xshift;					/* inclusive */
    zap_redraw_block.r_miny = zap_redraw_block.r_ysize - (y >> os_yshift);	/* inclusive */
    zap_redraw_block.r_maxx = (x + length * gui.char_width) >> os_xshift;	/* exclusive */
    zap_redraw_block.r_maxy = zap_redraw_block.r_ysize - ((y - gui.char_height) >> os_yshift);
										/* exclusive */

    /* Clip rectangle in ZapRedraw rectangle coordinates (pixels, Y-min/max reversed !!!) */
    clip_minx = clip[0] >> os_xshift;					/* inclusive */
    clip_miny = zap_redraw_block.r_ysize - (clip[3] >> os_yshift);	/* inclusive */
    clip_maxx = clip[2] >> os_xshift;					/* exclusive */
    clip_maxy = zap_redraw_block.r_ysize - (clip[1] >> os_yshift);	/* exclusive */

    /* Clip target rectangle against the current graphics window */
    if (zap_redraw_block.r_minx < clip_minx)
    {
	zap_redraw_block.r_scrollx = clip_minx - zap_redraw_block.r_minx;
	zap_redraw_block.r_minx = clip_minx;
    }
    else
	zap_redraw_block.r_scrollx = 0;
    if (zap_redraw_block.r_miny < clip_miny)
    {
	zap_redraw_block.r_scrolly = clip_miny - zap_redraw_block.r_miny;
	zap_redraw_block.r_miny = clip_miny;
    }
    else
	zap_redraw_block.r_scrolly = 0;
    if (zap_redraw_block.r_maxx > clip_maxx)
	zap_redraw_block.r_maxx = clip_maxx;
    if (zap_redraw_block.r_maxy > clip_maxy)
	zap_redraw_block.r_maxy = clip_maxy;

    /* Fill in the character data structure */
    if (length > (sizeof(redraw_data) - 2 * 4 - 2))
	length = sizeof(redraw_data) - 2 * 4 - 2;
    ((int*) redraw_data)[0] = 2 * 4;
    ((int*) redraw_data)[1] = 0;
    strncpy(redraw_data + 2 * 4, string, length);
    redraw_data[2 * 4 + length + 0] = '\0';
    redraw_data[2 * 4 + length + 1] = '\x2';
    zap_redraw_block.r_data = (int) redraw_data;

    /* Perform the draw */
    swi(ZapRedraw_RedrawArea, 0, &zap_redraw_block);

    return flags;
}

/*
 * Okay that was it from me, back to Thomas ...
 */

/*
 * Parse the GUI related command-line arguments.  Any arguments used are
 * deleted from argv, and *argc is decremented accordingly.  This is called
 * when vim is started, whether or not the GUI has been started.
 */
    void
gui_mch_prepare(int *argc, char **argv)
{
    int	    arg = 1;

    while (arg < *argc - 1)
    {
	if (strcmp(argv[arg], "--rows") == 0 || strcmp(argv[arg], "--columns") == 0)
	{
	    int	    value;

	    value = atoi(argv[arg + 1]);

	    if (argv[arg][2] == 'r')
		default_rows = value;
	    else
		default_columns = value;

	    /* Delete argument from argv[]. (hope this is read/write!) */

	    *argc -= 2;
	    if (*argc > arg)
	    mch_memmove(&argv[arg], &argv[arg + 2], (*argc - arg)
		    * sizeof(char *));
	}
	else
	    arg++;
    }
}

/* Fatal error on initialisation - report it and die. */
    void
ro_die(error)
    char_u *error;	/* RISC OS error block */
{
    swi(Wimp_ReportError, error, 5, "GVim");
    exit(EXIT_FAILURE);
}

/* Find the sizes of the window tools:
 *
 * Create a test window.
 * Find inner and outer sizes.
 * Find the difference.
 * Delete window.
 *
 * While we're here, find the eigen values too.
 */
    void
ro_measure_tools()
{
    int block[10];
    int vdu[] = { 4, 5, -1};
    int test_window[] =
	{
	    -100, -100,		/* Visible area : min X,Y */
	    -50, -50,		/*		  max X,Y */
	    0,   0,		/* Scroll offsets */
	    -1,			/* Window in front */
	    0xd0800150,		/* Window flags */
	    0xff070207,		/* Colours */
	    0x000c0103,		/* More colours */
	    0, -0x4000,		/* Workarea extent */
	    0x4000, 0,		/* max X,Y */
	    0x00000000,		/* No title */
	    0 << 12,		/* No workarea button type */
	    1,			/* Wimp sprite area */
	    0x00010001,		/* Minimum width, height */
	    0, 0, 0,		/* Title data (none) */
	    0			/* No icons */
	};
    int inner_max_x, inner_min_y;

    swi(Wimp_CreateWindow, 0, test_window);

    block[0] = r0;
    /* Open the window (and read state).
     * GetWindowOutline needs it too if the wimp isn't nested.
     */
    swi(Wimp_OpenWindow, 0, block);
    inner_max_x = block[3];
    inner_min_y = block[2];

    swi(Wimp_GetWindowOutline, 0, block);

    gui.scrollbar_width = block[3] - inner_max_x;
    gui.scrollbar_height = inner_min_y - block[2];

    swi(Wimp_DeleteWindow, 0, block);

    /* Read the size of one pixel. */
    swi(OS_ReadVduVariables, vdu, vdu);
    x_eigen_factor = vdu[0];
    y_eigen_factor = vdu[1];
}

/* Load a template from the current templates file.
 * Create the window and return its handle.
 */
    int
ro_load_template(str_name, title, title_size)
    char_u  *str_name;      /* Identifier of window in file (max 12 chars)   */
    char_u  **title;	    /* If not NULL then return pointer to title here */
    int     *title_size;    /* If not NULL then return the title length here */
{
    int     *window;
    char    *data;
    int     name[4];

    strcpy( (char *) name, str_name);

    /* Find how big we must make the buffers */

    if (xswi(Wimp_LoadTemplate, 0, 0, 0, 0, -1, name, 0) & v_flag)
	ro_die( (char *) r0);

    window = malloc(r1);	/* Don't print text messages from alloc() */
    data = malloc(r2);
    if (window == NULL || data == NULL)
	ro_die("\0\0\0\0Out of memory - Can't load templates");

    /* Load the template into the buffers */

    swi(Wimp_LoadTemplate, 0,
				window,		/* Temp block */
				data,		/* Icon data */
				data + r2 + 1,	/* End of icon data */
				-1,		/* No fonts */
				name, 0);	/* First match */
    if (r6 == 0)
	ro_die("\0\0\0\0Can't find window in Templates file");

    /* Create the window */

    if (xswi(Wimp_CreateWindow, 0, window) & v_flag)
	ro_die( (char *) r0);

    if (title)
	*title = (char_u *) window[18];
    if (title_size)
	*title_size = window[20];

    free(window);	/* Free temp block */
    return r0;		/* Return the window handle */
}

/*
 * Check if the GUI can be started.  Called before gvimrc is sourced.
 * Return OK or FAIL.
 */
    int
gui_mch_init_check()
{
    return OK;		/* TODO: GUI can always be started? */
}

/*
 * Initialise the RISC OS GUI.
 * Create all the windows.
 * Returns OK for success, FAIL when the GUI can't be started.
 */
    int
gui_mch_init()
{
    int     messages[] = {
	    1, 2, 3, 4,	/* DataSave, DataSaveAck, DataLoad, DataLoadAck */
	    8,		/* PreQuit */
	    0xf,	/* ClaimEntity (for clipboard) */
	    0x10,	/* DataRequest (for clipboard) */
	    0x400c1,	/* Mode change */
	    0x400c3,	/* TaskCloseDown */
	    0x400c9,	/* MenusDeleted */
	    0x808c1,	/* TW_Output */
	    0x808c2,    /* TW_Ego */
	    0x808c3,	/* TW_Morio */
	    0x808c4,	/* TW_Morite */
	    0};		/* End-of-list. */


    /* There may have been some errors reported in the
     * command window before we get here. Wait if so.
     */
    swi(Wimp_ReadSysInfo, 3);
    if (r0 == 0)
	swi(Wimp_CommandWindow, 0);	/* Window opened - close with prompt */

    if (xswi(Wimp_Initialise, 310, 0x4b534154, "GVim", messages) & v_flag)
	return FAIL;
    nested_wimp = r0 >= 397;
    task_handle = r1;

    /* Load the templates. */

    if (xswi(Wimp_OpenTemplate, 0, "Vim:Templates") & v_flag)
	ro_die( (char *) r0);

    gui.window_handle = ro_load_template("editor",
	    &gui.window_title,
	    &gui.window_title_size);

    save_window = ro_load_template("save", NULL, NULL);

    swi(Wimp_CloseTemplate);

    /* Set default foreground and background colours. */

    gui.norm_pixel = gui.def_norm_pixel;
    gui.back_pixel = gui.def_back_pixel;

    /* Get the colours from the "Normal" and "Menu" group (set in syntax.c or
     * in a vimrc file) */

    set_normal_colors();

    /*
     * Check that none of the colors are the same as the background color
     */

    gui_check_colors();

    /* Get the colours for the highlight groups (gui_check_colors() might have
     * changed them) */

    highlight_gui_started();		/* re-init colours and fonts */

    /* Set geometry based on values read on initialisation. */

    gui.num_cols = Columns = default_columns;
    gui.num_rows = Rows    = default_rows;

    /* Get some information about our environment. */

    ro_measure_tools();

    return OK;
}

/*
 * Called when the foreground or background colour has been changed.
 */
    void
gui_mch_new_colors()
{
}

/*
 * Open the GUI window which was created by a call to gui_mch_init().
 */
    int
gui_mch_open(void)
{
    int block[10];

    block[0] = gui.window_handle;
    swi(Wimp_GetWindowState, 0, block);
    block[7] = -1;		    /* Open at the top of the stack */
    swi(Wimp_OpenWindow, 0, block);

    /* Give the new window the input focus */
    swi(Wimp_SetCaretPosition, gui.window_handle, -1, 0, 0, -1, -1);

    if (gui_win_x != -1 && gui_win_y != -1)
	gui_mch_set_winpos(gui_win_x, gui_win_y);

    return OK;
}

    void
gui_mch_exit(int rc)
{
    int	    block[64];

    /* Close window. Stops us from getting troublesome events
     * if we take a while to die.
     */
    block[0] = gui.window_handle;
    swi(Wimp_CloseWindow, 0, block);

    if (child_handle)
    {
	/* We still have a sub-task running - kill it */
	block[0] = 20;
	block[3] = 0;
	block[4] = 0;	    /* Quit */
	if ((xswi(Wimp_SendMessage, 17, block, child_handle) & v_flag) == 0)
	{
	    /* Idle until child dies. */
	    while (child_handle)
	    {
		process_event(wimp_poll(1, block), block);
	    }
	}
    }

    exit(rc);
}

/*
 * Get the position of the top left corner of the window.
 */
    int
gui_mch_get_winpos(int *x, int *y)
{
    /* TODO */
    return FAIL;
}

/*
 * Set the position of the top left corner of the window to the given
 * coordinates.
 */
    void
gui_mch_set_winpos(int x, int y)
{
    /* TODO */
}

    void
gui_mch_set_shellsize(width, height, min_width, min_height, base_width, base_height, direction)
    int width;		/* In OS units */
    int height;
    int min_width;	/* Smallest permissible window size (ignored) */
    int min_height;
    int base_width;	/* Space for scroll bars, etc */
    int base_height;
    int direction;
{
    int s_width, s_height;
    int block[] = {
	gui.window_handle,
	0,
	-height + 1,
	width,
	1};

    gui_mch_get_screen_dimensions(&s_width, &s_height);
    s_width -= base_width;
    s_height -= base_height;		    /* Underestimate - ignores titlebar */

    swi(Wimp_GetWindowState, 0, block);
    block[3]  = block[1] + width;
    block[2]  = block[4] - height;
    if (block[3] > s_width)
    {
	block[3] = s_width;
	block[1] = block[3] - width;
    }
    if (block[2] < gui.scrollbar_height)
    {
	block[2] = gui.scrollbar_height;
	block[4] = block[2] + height;
    }
    swi(Wimp_OpenWindow, 0, block);
    swi(Wimp_ForceRedraw, gui.window_handle, 0, -height, width, 0);
}

    void
gui_mch_get_screen_dimensions(int *screen_w, int *screen_h)
{
    int block[] = {4, 5, 11, 12, -1};

    swi(OS_ReadVduVariables, block, block);
    *screen_w = (block[2] + 1) << block[0];
    *screen_h = (block[3] + 1) << block[1];
}

/* Take a font name with options and return a font handle, or
 * zero for failure.
 * Replace extension with 'Bold' or 'Italic' depending on modifiers.
 */
    int
ro_get_font(fullname, weight)
    char_u	*fullname;
    int		weight;		/* Initial weights:
				 * BIT	    MEANING
				 * 0	    bold
				 * 1	    italic
				 */
{
    char_u	*arg;
    char_u	font[41];
    int		width = -1;
    int		height = -1;
    int		name_len;
    int		i;
    char_u	c;

    for (i = 0; i < 39;)
    {
	c = fullname[i];
	if (c == ':' || c == NUL || c == '.')
	    break;
	font[i++] = c;
    }

    /* find the first modifier, NULL if none */
    arg = strchr(fullname + i, ':');

    while (arg)
    {
	switch (*++arg)
	{
	    case 'h':
		height = strtol(arg + 1, (char **) &arg, 10);
		break;
	    case 'w':
		width = strtol(arg + 1, (char **) &arg, 10);
		break;
	    case 'b':
		weight |= 1;
		break;
	    case 'i':
		weight |= 2;
		break;
	    default:
		return 0;
	}
	arg = strchr(arg, ':');
    }

    if ((weight & 1) && i < 35)
    {
	/* Bold goes instead of given suffix */
	strncpy(font + i, ".Bold", 5);
	i += 5;
    }
    else
    {
	/* Copy rest of name unless we are using Bold */
	while (i < 39)
	{
	    c = fullname[i];
	    if (c == ':' || c == NUL)
		break;
	    font[i++] = c;
	}
    }
    if ((weight & 2) && i < 32)
    {
	strncpy(font + i, ".Oblique", 8);
	i += 8;
    }

    font[i] = 0;

    if (height < 1 && width < 1)
	height = width = 10;	/* Default to 10pt */
    else if (height < 1)
	height = width;
    else if (width < 1)
	width = height;

    if (xswi(Font_FindFont, 0, font, width << 4, height << 4, 0, 0) & v_flag)
	return NOFONT;		/* Can't find font */

    return r0;
}

/* Load a file into allocated memory and check it is valid.
 * Return a pointer to the allocated block on success.
 */
    char    *
zap_load_file(name, style)
    char_u  *name;	    /* Name of directory containing styles */
    char_u  *style;	    /* Name of style within directory */
{
    char_u  fname[256];
    char_u  *file;

    if (strlen(name) + strlen(style) > 254)
	return NULL;	    /* Names too long */

    sprintf(fname, "%s.%s", name, style);

    /* Load the named font in 1bpp format. */
    if (xswi(OS_File, 13, fname, 0, 0, "VimFonts:") & v_flag || r0 != 1)
	return NULL;	    /* Error reading file info, or not a file */

    /* Allocate enough memory to load the whole file */
    file = (char *) alloc(r4);
    if (!file)
	return NULL;	/* Out of memory */

    if (xswi(OS_File, 12, fname, file, 0, "VimFonts:") & v_flag)
	return NULL;	/* Unable to load file */

    if (strncmp(file, "ZapFont\015", 8) == 0)
	return file;	/* Loaded OK! */

    vim_free(file);
    return NULL;	/* Not a valid font file */
}

/* Load and convert the named font.
 * If name is NULL or a null string then convert the system font.
 * Return OK on success; FAIL and we revert to using the VDU drivers.
 *
 * 'name' is the name of a directory.
 * Tries to load 'name.0', 'name.B', 'name.I' and 'name.IB'.
 */
    int
zap_load_font(name)
    char_u  *name;
{
    int	    i;

    /* Free the existing font files, if any */
    for (i = 0; i < ZAP_STYLES; i++)
    {
	vim_free(zap_file[i]);
	zap_file[i] = NULL;
    }

    if (name && *name == '!')
    {
	name++;
	double_height = TRUE;
    }
    else
	double_height = FALSE;

    if (name && *name)
    {
	zap_file[ZAP_NORMAL]	= zap_load_file(name, "0");
	if (!zap_file[ZAP_NORMAL])
	    return FAIL;	/* Can't load the 'normal' style - error */

	zap_file[ZAP_BOLD]	= zap_load_file(name, "B");
	zap_file[ZAP_ITALIC]	= zap_load_file(name, "I");
	zap_file[ZAP_BITALIC]	= zap_load_file(name, "IB");
    }
    else
    {
	int	*header;
	char	workarea[16];
	char	*old_wa;

	/* Allocate memory for system font (8 x 8 x 256 bits, plus header) */
	header = (int *) alloc(0x20 + 8 * 256);
	if (header == NULL)
	    return FAIL;
	zap_file[ZAP_NORMAL] = (char *) header;

	/* Store details about the system font */
	header[2] = 8;	    /* Width */
	header[3] = 8;	    /* Height */
	header[4] = 0;	    /* First char */
	header[5] = 255;    /* Last char */
	header[6] = header[7] = 0;  /* Reserved */

	/* Get system font bitmap */
	old_wa = zap_redraw_block.r_workarea;
	zap_redraw_block.r_workarea = workarea;
	swi(ZapRedraw_ReadSystemChars, zap_file[ZAP_NORMAL] + 0x20, &zap_redraw_block);
	zap_redraw_block.r_workarea = old_wa;
    }

    return ro_zap_redraw_initialise();
}

/*
 * Initialise vim to use the font with the given name.
 * Return FAIL if the font could not be loaded, OK otherwise.
 */
    int
gui_mch_init_font(char_u *font_name, int fontset)
{
    int	    new_handle	= 0;	    /* Use the system font by default */

    if (font_name[0] == '!')
    {
	/* Select a ZapRedraw font */
	if (zap_load_font(font_name + 1))
	    zap_redraw = TRUE;
	else
	{
	    EMSG2(_("E610: Can't load Zap font '%s'"), font_name);
	    font_name = "System";   /* Error - use system font */
	    zap_redraw = FALSE;
	}
    }
    else
    {
	zap_redraw = FALSE;

	if (font_name)
	{
	    /* Extract any extra details about the font */
	    new_handle = ro_get_font(font_name, 0);
	    if (!new_handle)
		return FAIL;
	}
	else
	    font_name = "System";
    }

    /* Free the previous font, if any */
    gui_mch_free_font(gui.norm_font);
    gui.norm_font = new_handle;
    gui.char_ascent = 0;

    if (new_handle)
    {
	/* Read details about the chosen font */
	swi(Font_ReadInfo, new_handle);

	gui.char_width	= r3 - r1;
	gui.char_height = r4 - r2;

	font_x_offset = -r1;	/* Where to position each char in its box */
	font_y_offset = -r4;

	/* Try to load other fonts for bold, italic, and bold-italic */
	gui_mch_free_font(gui.bold_font);
	gui.bold_font = ro_get_font(font_name, 1);
	gui_mch_free_font(gui.ital_font);
	gui.ital_font = ro_get_font(font_name, 2);
	gui_mch_free_font(gui.boldital_font);
	gui.boldital_font = ro_get_font(font_name, 3);
    }
    else
    {
	/* Use the system font or ZapRedraw. */
	if (zap_redraw)
	{
	    gui.char_width	= zap_redraw_block.r_charw << zap_redraw_block.r_magx;
	    gui.char_height	= zap_redraw_block.r_charh << zap_redraw_block.r_magy;
	    if (double_height)
		gui.char_height <<= 1;
	}
	else
	{
	    gui.char_width	= 16;
	    gui.char_height	= 32;
	}

	gui_mch_free_font(gui.bold_font);
	gui.bold_font = 0;
	gui_mch_free_font(gui.ital_font);
	gui.ital_font = 0;
	gui_mch_free_font(gui.boldital_font);
	gui.boldital_font = 0;
    }
    hl_set_font_name(font_name);

    must_redraw = CLEAR;
    return OK;
}

/*
 * Adjust gui.char_height (after 'linespace' was changed).
 */
    int
gui_mch_adjust_charheight()
{
    return FAIL;
}

/*
 * Get a font structure for highlighting.
 */
    GuiFont
gui_mch_get_font(name, giveErrorIfMissing)
    char_u	*name;
    int		giveErrorIfMissing;
{
    int		handle;

    if (!name)
	return NOFONT;		/* System font if no name */

    handle = ro_get_font(name, 0);
    if (!handle)
    {
	if (giveErrorIfMissing)
	    EMSG2(_("E611: Can't use font %s"), name);
	return NOFONT;
    }

    return handle;
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Return the name of font "font" in allocated memory.
 * Don't know how to get the actual name, thus use the provided name.
 */
    char_u *
gui_mch_get_fontname(font, name)
    GuiFont font;
    char_u  *name;
{
    if (name == NULL)
	return NULL;
    return vim_strsave(name);
}
#endif

/*
 * Set the current text font.
 */
    void
gui_mch_set_font(GuiFont font)
{
    ro_current_font = font;

    if (font)
    {
	/* Not the system font or ZapRedraw font - select it */
	swi(Font_SetFont, font);
    }
}

/*
 * If a font is not going to be used, free its structure.
 */
    void
gui_mch_free_font(GuiFont font)
{
    if (font)
	swi(Font_LoseFont, font);
}

/*
 * Return the Pixel value (colour) for the given colour name.
 * Return INVALCOLOR for error.
 * NB: I've changed Green for now, since it looked really sick
 */
    guicolor_T
gui_mch_get_color(char_u *name)
{
    int		i;
    struct colour
    {
	char_u		*name;
	guicolor_T	value;
    } colours[] =
    {
	{ "Red",		grgb(255,	0,	0)	},
	{ "LightRed",		grgb(255,	0,	0)	},
	{ "DarkRed",		grgb(139,	0,	0)	},

	{ "Green",		grgb(50,	200,	50)	},
	{ "LightGreen",		grgb(144,	238,	144)	},
	{ "DarkGreen",		grgb(0,		100,	0)	},
	{ "SeaGreen",		grgb(46,	139,	87)	},

	{ "Blue",		grgb(0,		0,	255)	},
	{ "LightBlue",		grgb(173,	216,	230)	},
	{ "DarkBlue",		grgb(0,		0,	139)	},
	{ "SlateBlue",		grgb(160,	90,	205)	},

	{ "Cyan",		grgb(0,		255,	255)	},
	{ "LightCyan",		grgb(224,	255,	255)	},
	{ "DarkCyan",		grgb(0,		139,	139)	},

	{ "Magenta",		grgb(255,	0,	255)	},
	{ "LightMagenta",	grgb(255,	224,	255)	},
	{ "DarkMagenta",	grgb(139,	0,	139)	},

	{ "Yellow",		grgb(255,	255,	0)	},
	{ "LightYellow",	grgb(255,	255,	224)	},
	{ "DarkYellow",		grgb(139,	139,	0)	},
	{ "Brown",		grgb(165,	42,	42)	},

	{ "Gray",		grgb(190,	190,	190)	},
	{ "Grey",		grgb(190,	190,	190)	},
	{ "LightGray",		grgb(211,	211,	211)	},
	{ "LightGrey",		grgb(211,	211,	211)	},
	{ "DarkGray",		grgb(169,	169,	169)	},
	{ "DarkGrey",		grgb(169,	169,	169)	},
	{ "Gray10",		grgb(26,	26,	26)	},
	{ "Grey10",		grgb(26,	26,	26)	},
	{ "Gray20",		grgb(51,	51,	51)	},
	{ "Grey20",		grgb(51,	51,	51)	},
	{ "Gray30",		grgb(77,	77,	77)	},
	{ "Grey30",		grgb(77,	77,	77)	},
	{ "Gray40",		grgb(102,	102,	102)	},
	{ "Grey40",		grgb(102,	102,	102)	},
	{ "Gray50",		grgb(127,	127,	127)	},
	{ "Grey50",		grgb(127,	127,	127)	},
	{ "Gray60",		grgb(153,	153,	153)	},
	{ "Grey60",		grgb(153,	153,	153)	},
	{ "Gray70",		grgb(179,	179,	179)	},
	{ "Grey70",		grgb(179,	179,	179)	},
	{ "Gray80",		grgb(204,	204,	204)	},
	{ "Grey80",		grgb(204,	204,	204)	},
	{ "Gray90",		grgb(229,	229,	229)	},
	{ "Grey90",		grgb(229,	229,	229)	},

	{ "Black",		grgb(0,		0,	0)	},
	{ "White",		grgb(255,	255,	255)	},

	{ "Orange",		grgb(255,	165,	0)	},
	{ "Purple",		grgb(160,	32,	240)	},
	{ "Violet",		grgb(238,	130,	238)	},
	{NULL, 0}
    };

    if (name[0] == '#')
    {
	char	    *end;
	int	    c;

	c = strtol(name + 1, &end, 16);
	return (guicolor_T) ((c >> 16) & 0xff) | (c & 0xff00) | ((c & 0xff) << 16);
    }

    for (i = 0; colours[i].name != NULL; i++)
    {
	if (STRICMP(name, colours[i].name) == 0)
	    return colours[i].value;
    }
    if (strnicmp(name, "grey", 4) == 0 || strnicmp(name, "gray", 4) == 0)
    {
	int level = (255 * atoi(name + 4)) / 100;
	return (guicolor_T) grgb(level, level, level);
    }
    return INVALCOLOR;
}

/*
 * Set the current text colours.
 * If we are using fonts then set the antialiasing colours too.
 */
    void
gui_mch_set_colors(guicolor_T fg, guicolor_T bg)
{
    zap_redraw_colours[0] = bg << 8;	/* JK230798, register new background colour */
    zap_redraw_colours[1] = fg << 8;	/* JK230798, register new foreground colour */
    zap_redraw_update_colours = TRUE;	/* JK230798, need update of colour masks */

    swi(ColourTrans_ReturnGCOL, fg << 8);
    gui.fg_colour = r0;
    swi(ColourTrans_ReturnGCOL, bg << 8);
    gui.bg_colour = r0;

    if (ro_current_font)
	swi(ColourTrans_SetFontColours, 0, bg << 8, fg << 8, 14);
}

    void
ro_draw_string(x, y, s, len, flags, clip)
    int	    x;		/* Top-left coord to plot at (x incl, y excl) */
    int	    y;		/* (screen coords) */
    char_u  *s;		/* String to plot */
    int	    len;	/* Length of string */
    int	    flags;	/* DRAW_TRANSP, DRAW_BOLD, DRAW_UNDERL */
    int*    clip;	/* JK230798, added clip window */
{
    if (ro_current_font)
    {
	int	fx;
	int	flen = len;	/* Preserve for underline */

	/* Use the Font manager to paint the string.
	 * Must do one char at a time to get monospacing.
	 */

	if (flags & DRAW_ITALIC && !gui.ital_font)
	    flags |= DRAW_UNDERL;	/* No italic - underline instead */

	if ((flags & DRAW_TRANSP) == 0)
	{
	    swi(ColourTrans_SetColour, gui.bg_colour, 0, 0, 0, 0);
	    swi(OS_Plot, 4, x, y - gui.char_height);
	    swi(OS_Plot, 96 + 5, x + len * gui.char_width - 1, y - 1);
	}

	fx = x + font_x_offset;
	while (flen--)
	{
	    swi(Font_Paint, 0, s++, 0x90, fx, y + font_y_offset, 0, 0, 1);
	    fx += gui.char_width;
	}
    }
    else
    {
	if (zap_redraw)
	{
	    /* Using fast Zap redraw. */
	    flags = ro_zap_redraw_draw_string(x, y, s, len, flags, clip);
	}
	else
	{
	    /* Using the system font */
	    if (flags & DRAW_ITALIC)
		flags |= DRAW_UNDERL;

	    if ((flags & DRAW_TRANSP) == 0)
	    {
		swi(ColourTrans_SetColour, gui.bg_colour, 0, 0, 0, 0);
		swi(OS_Plot, 4, x, y - gui.char_height);
		swi(OS_Plot, 96 + 5, x + len * gui.char_width - 1, y - 1);
	    }
	    swi(OS_Plot, 4,			/* Move the drawing cursor */
		    x,
		    y - 1);
	    swi(ColourTrans_SetColour, gui.fg_colour, 0, 0, 0, 0);
	    swi(OS_WriteN, s, len);

	    if (flags & DRAW_BOLD)
	    {
		swi(OS_Plot, 4, x + (1 << x_eigen_factor), y - 1);
		swi(OS_WriteN, s, len);
	    }
	}
    }

    if (flags & DRAW_UNDERL)
    {
	if (ro_current_font || zap_redraw)
	    swi(ColourTrans_SetColour, gui.fg_colour, 0, 0, 0, 0);
	/* Underlined is the same with all plotting methods */
	swi(OS_Plot, 4, x, y - gui.char_height);
	swi(OS_Plot, 1, gui.char_width * len, 0);
    }
}

    void
gui_mch_draw_string(int row, int col, char_u *s, int len, int flags)
{
    int x, y;		/* Workarea x,y */
    x = col * gui.char_width;
    y = -row * gui.char_height;

    if (redraw_block)
    {
	ro_draw_string(x + redraw_block[1], y + redraw_block[4],
			s, len, flags, &redraw_block[7]);	/* JK230798, added clip window */
    }
    else
    {
	int block[44];
	block[0] = gui.window_handle;
	block[1] = x;
	block[2] = y - gui.char_height;
	block[3] = (col + len) * gui.char_width;
	block[4] = y;
	swi(Wimp_UpdateWindow, 0, block);
	while (r0)
	{
	    ro_draw_string(x + block[1], y + block[4],
			s, len, flags, &block[7]);	/* JK230798, added clip window */
	    swi(Wimp_GetRectangle, 0, block);
	}
    }
}

/*
 * Return OK if the key with the termcap name "name" is supported.
 */
    int
gui_mch_haskey(char_u *name)
{
    return FAIL;
}

    void
gui_mch_beep(void)
{
    swi(OS_WriteI + 7);
}

/*
 * Visual bell.
 */
    void
gui_mch_flash(int msec)
{
    /* TODO */
}


/*
 * Plot a solid rectangle using the given plot action and colour.
 * Coordinates are inclusive and window-relative.
 */
    void
plot_rectangle(plot, colour, minx, miny, maxx, maxy)
    int plot;		/* OS_Plot action */
    int colour;
    int minx;
    int miny;
    int maxx;
    int maxy;
{
    if (redraw_block)
    {
	swi(ColourTrans_SetColour, colour, 0, 0, 0, 0);
	swi(OS_Plot, 4, minx + redraw_block[1], miny + redraw_block[4]);
	swi(OS_Plot, plot, maxx + redraw_block[1], maxy + redraw_block[4]);
    }
    else
    {
	int block[44];
	block[0] = gui.window_handle;
	block[1] = minx;
	block[2] = miny;
	block[3] = maxx + 1;
	block[4] = maxy + 1;
	swi(Wimp_UpdateWindow, 0, block);
	while (r0)
	{
	    swi(ColourTrans_SetColour, colour, 0, 0, 0, 0);
	    swi(OS_Plot, 4, minx + block[1], miny + block[4]);
	    swi(OS_Plot, plot, maxx + block[1], maxy + block[4]);
	    swi(Wimp_GetRectangle, 0, block);
	}
    }
}

/*
 * Invert a rectangle from row r, column c, for nr rows and nc columns.
 */
    void
gui_mch_invert_rectangle(int r, int c, int nr, int nc)
{
    plot_rectangle(96 + 6, 0, FILL_X(c), -FILL_Y(r + nr), FILL_X(c + nc), -FILL_Y(r));
}

/*
 * Iconify the GUI window.
 */
    void
gui_mch_iconify(void)
{
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Bring the Vim window to the foreground.
 */
    void
gui_mch_set_foreground()
{
    /* TODO */
}
#endif

/* Draw a hollow rectangle relative to the current
 * graphics cursor position, with the given width
 * and height. Start position is top-left.
 */
    void
draw_hollow(w, h)
    int	w;
    int	h;
{
    swi(OS_Plot, 1, w - 1, 0);
    swi(OS_Plot, 1, 0, 1 - h);
    swi(OS_Plot, 1, 1 - w, 0);
    swi(OS_Plot, 1, 0, h - 1);
}

/*
 * Draw a cursor without focus.
 */
    void
gui_mch_draw_hollow_cursor(guicolor_T colour)
{
    int x = FILL_X(gui.cursor_col);	/* Window relative, top-left */
    int y = -FILL_Y(gui.cursor_row);
    if (redraw_block == NULL)
    {
	int block[11];

	block[0] = gui.window_handle;
	block[1] = x;
	block[2] = y - gui.char_height;
	block[3] = x + gui.char_width;
	block[4] = y;
	swi(Wimp_UpdateWindow, 0, block);
	while (r0)
	{
	    swi(ColourTrans_SetGCOL, colour << 8, 0, 0, 0, 0);

	    swi(OS_Plot, 4, x + block[1], y + block[4] - 1);
	    draw_hollow(gui.char_width, gui.char_height);

	    swi(Wimp_GetRectangle, 0, block);
	}
    }
    else
    {
	swi(ColourTrans_SetGCOL, colour << 8, 0, 0, 0, 0);

	swi(OS_Plot, 4, x + redraw_block[1], y + redraw_block[4] - 1);
	draw_hollow(gui.char_width, gui.char_height);
    }
}

/*
 * Draw part of a cursor, "w" pixels wide, and "h" pixels high, using
 * color "color".
 */
    void
gui_mch_draw_part_cursor(w, h, colour)
    int w;
    int h;
    guicolor_T colour;
{
    int x = FILL_X(gui.cursor_col);
    int y = -FILL_Y(gui.cursor_row);
    swi(ColourTrans_ReturnGCOL, colour << 8);
    plot_rectangle(96 + 5, r0, x, y - h, x + w - 1, y - 1);
}

/*
 * Catch up with any queued events.  This may put keyboard input into the
 * input buffer, call resize call-backs, trigger timers etc.
 * If there is nothing in the event queue(& no timers pending), then we return
 * immediately (well, after a Wimp_Poll).
 */
    void
gui_mch_update(void)
{
    int block[64];
    int reason;

    swi(OS_ReadMonotonicTime);
    if ((r0 - time_of_last_poll) < 50)
	return;			    /* Don't return too often */

    reason = wimp_poll(0, block);
    if (reason)
	process_event(reason, block);
    ro_return_early = FALSE;		/* We're returning anyway. */
}

    void
redraw_window(block)
    int *block;
{
    int x, y;			/* Vim workarea coords */
    int width, height;
    int blank_col;

    swi(ColourTrans_ReturnGCOL, UNUSED_COLOUR << 8, 0, 0, 1<<7, 0);
    blank_col = r0;

    swi(Wimp_RedrawWindow, 0, block);
    redraw_block = block;
    while (r0)
    {
	x = block[7] - block[1];
	y = block[4] - block[10];
	width  = block[9]  - block[7];
	height = block[10] - block[8];

	if (height + y > Rows * gui.char_height)
	{
	    /* Blank everything off the bottom. */
	    plot_rectangle(96 + 5, blank_col,
				0, block[8] - block[4],
				block[9] - block[1], -FILL_Y(Rows) - 1);
	    height = Rows * gui.char_height;
	}
	if (width + x> Columns * gui.char_width)
	{
	    /* Blank everything off to the right. */
	    plot_rectangle(96 + 5, blank_col,
				FILL_X(Columns), block[8] - block[4],
				block[9] - block[1], 0);
	    width = Columns * gui.char_width;
	}
	gui_redraw(x , y, width, height);
	swi(Wimp_GetRectangle, 0, block);
    }
    redraw_block = NULL;
}

/* Check if we have modified data.
 * If we do then ack the message to stop the shutdown.
 * Otherwise, ignore the message.
 */
    void
ro_prequit(block)
    int	    *block;
{
    if (!ro_ok_to_quit())
    {
	/* Not OK to quit - stop shutdown */
	block[3] = block[2];
	swi(Wimp_SendMessage, 19, block, block[1]);
    }
    /* Do nothing. We may get a Message_Quit later. */
}

/* If there is unsaved data then ask the user if they mind losing it.
 * Return TRUE if we can quit without saving, FALSE to halt the
 * shutdown.
 */
    int
ro_ok_to_quit()
{
    int	    old_confirm = cmdmod.confirm;

    cmdmod.confirm = FALSE;	    /* Use our own, single tasking, box */

    if (check_changed_any(FALSE))
    {
	swi(Wimp_ReportError,
		"\0\0\0\0Vim contains unsaved data - quit anyway?",
		0x17,
		"Vim");
	cmdmod.confirm = old_confirm;
	if (r1 != 1)
	    return FALSE;
    }
    cmdmod.confirm = old_confirm;
    return TRUE;
}

/* Quit without checking for unsaved data. */
    void
ro_quit()
{
    exiting = TRUE;
    getout(0);

    exiting = FALSE;		    /* probably can't get here */
    setcursor();		    /* position cursor */
    out_flush();
}

/* Insent the given vim special code into the input buffer */
    void
ro_press(a, b, modifier)
    char a;
    char b;
    int modifier;	/* %<Ctrl><Shift> 0000 0000 */
{
    char_u buf[6];
    int	    vim_mod;
    int	    key;


    /* Convert RISC OS modifier to Vim modifier. */
    vim_mod = ((modifier & 0x10) ? MOD_MASK_SHIFT : 0)
	       | ((modifier & 0x20) ? MOD_MASK_CTRL : 0);
    key = simplify_key(TERMCAP2KEY(a, b), &vim_mod);

    buf[3] = CSI;
    buf[4] = KEY2TERMCAP0(key);
    buf[5] = KEY2TERMCAP1(key);
    if (vim_mod)
    {
	buf[0] = CSI;
	buf[1] = KS_MODIFIER;
	buf[2] = vim_mod;
	add_to_input_buf(buf, 6);
    }
    else
	add_to_input_buf(buf + 3, 3);
}

/* Take a wimp key code and insert the vim equivalent
 * into vim's input buffer.
 * CTRL-C also sets got_int.
 */
    void
ro_insert_key(code)
    char_u *code;		/* Wimp_ProcessKey code (4 bytes) */
{
    char a = code[0];
    char b = code[1];
    int base, modifier;

    if (a == 3 && ctrl_c_interrupts)
	got_int = TRUE;

    /* Is it a normal key? */
    if (a > 31 && a < 127)
    {
	add_to_input_buf(code, 1);
	return;
    }

    /* We should pass any unrecognised keys on, but
     * for now just pass on F12 combinations.
     */
    switch (b)
    {
	case 0:
	    /* Home and Delete are the only special cases */
	    switch (a)
	    {
		case 0x1e:
		    ro_press('k','h', 0);	/* Home */
		    return;
		case 0x7f:
		    ro_press('k','D', 0);	/* Delete */
		    return;
		case CSI:
		    {
			/* Turn CSI into K_CSI.  Untested! */
			char_u string[3] = {CSI, KS_EXTRA, KE_CSI};

			add_to_input_buf(string, 3);
			return;
		    }
		default:
		    add_to_input_buf(code, 1);
		    return;
	    }
	case 1:
	    if ((a & 0xcf) == 0xcc)
	    {
		/* F12 pressed - pass it on (quick hack) */
		swi(Wimp_ProcessKey, a | 0x100);
		return;
	    }
	    base = a & 0xcf;
	    modifier = a & 0x30;
	    switch (base)
	    {
		case 0x8a:	/* Tab */
		    add_to_input_buf("\011", 1);
		    return;
		case 0x8b:	/* Copy (End) */
		    return ro_press('@', '7', modifier);
		case 0x8c:	/* Left */
		    return ro_press('k', 'l', modifier);
		case 0x8d:	/* Right */
		    return ro_press('k', 'r', modifier);
		case 0x8e:	/* Down */
		    if (modifier & 0x10)
			return ro_press('k', 'N', modifier ^ 0x10);
		    else
			return ro_press('k', 'd', modifier);
		case 0x8f:	/* Up */
		    if (modifier & 0x10)
			return ro_press('k', 'P', modifier ^ 0x10);
		    else
			return ro_press('k', 'u', modifier);
		case 0xca:	/* F10 */
		    return ro_press('k', ';', modifier);
		case 0xcb:	/* F11 */
		    return ro_press('F', '1', modifier);
		case 0xcd:	/* Insert */
		    return ro_press('k', 'I', modifier);
		default:
		    if (base > 0x80 && base < 0x18a)
		    {
			/* One of the other function keys */
			return ro_press('k', '0' + (base & 15), modifier);
		    }
	    }
    }
}

/* Process a mouse event. */
    void
ro_mouse(block)
    int *block;
{
    int x, y, button, vim_button;
    int modifiers = 0;
    int min_x, min_y;		/* Visible area of editor window */
    int max_x, max_y;

    if (block[3] != gui.window_handle || ro_dragging)
	return;			/* Not our window or ignoring clicks*/

    x = block[0];		/* Click position - screen coords */
    y = block[1];
    button = block[2];

    block[0] = gui.window_handle;
    swi(Wimp_GetWindowState, 0, block);
    min_x = block[1];
    min_y = block[2];
    max_x = block[3];
    max_y = block[4];

    if (block[3] - x < gui.scrollbar_width)
    {
	/* Click in that blank area under the scrollbars */

	if (button & 0x444)
	{
	    int	    front_block[10];
	    /* Dragging with Select - bring window to front first */
	    front_block[0] = gui.window_handle;
	    swi(Wimp_GetWindowState, 0, front_block);
	    front_block[7] = -1;
	    ro_open_main(front_block);
	}

	block[0] = gui.window_handle;
	block[1] = 7;			/* Drag point */
	block[2] = block[4] = 0;	/* Coords of point. */
	block[3] = block[5] = 0;
	drag_x_offset = max_x - x;
	drag_y_offset = min_y - y;

	/* Parent box. */
	block[6] = min_x +
			gui.scrollbar_width * 2 +
			MIN_COLUMNS * gui.char_width;
	block[7] = 0;
	gui_mch_get_screen_dimensions(&block[8], &block[9]);
	block[9] = max_y -
			4 * gui.char_height -
			gui.scrollbar_height;

	swi(Wimp_DragBox, 0, block);
	ro_dragging = DRAG_RESIZE_WINDOW;
	drag_button = vim_button;
	drag_modifiers = modifiers;
	return;
    }

    if (button & 0x111)
	vim_button = MOUSE_RIGHT;
    else if (button & 0x222)
	vim_button = MOUSE_MIDDLE;
    else
	vim_button = MOUSE_LEFT;

    swi(OS_Byte, 121, 0x80);
    if (r1 == 0xff)
	modifiers |= MOUSE_SHIFT;
    swi(OS_Byte, 121, 0x81);
    if (r1 == 0xff)
	modifiers |= MOUSE_CTRL;
    swi(OS_Byte, 121, 0x82);
    if (r1 == 0xff)
	modifiers |= MOUSE_ALT;

    if (button == 2)
    {
	/* Menu click:
	 * If shift was pressed then do the paste action.
	 * If not, then open the pop-up menu.
	 */
	modifiers ^= MOUSE_SHIFT;
	if (modifiers && MOUSE_SHIFT)
	{
	    vimmenu_T	main;
	    /* Shift was NOT pressed - show menu */
	    main.dname = (char_u *) "Vim";
	    main.children = root_menu;
	    gui_mch_show_popupmenu(&main);
	    return;
	}
    }

    /* Gain the input focus */
    swi(Wimp_SetCaretPosition, gui.window_handle, -1, 0, 0, -1, -1);

    if (button & 0xf0)
    {
	/* Drag operation:
	 *
	 * Tell the Wimp to start a drag.
	 * Monitor null events.
	 */
	block[1] = 7;			/* Drag a point. */
	block[2] = block[4] = x;	/* Coords of point. */
	block[3] = block[5] = y;
	block[6] = 0;			/* Coords of bounding box. */
	block[7] = 0;
	gui_mch_get_screen_dimensions(&block[8], &block[9]);

	drag_x_offset = drag_y_offset = 0;

	swi(Wimp_DragBox, 0, block);
	ro_dragging = DRAG_SELECTION;
	drag_button = vim_button;
	drag_modifiers = modifiers;

	vim_button |= MOUSE_DRAG;
    }

    gui_send_mouse_event(
		vim_button,
		x - min_x,
		max_y - y,
		button & 0xf ? TRUE : FALSE,	/* dclick */
		modifiers);
}

    void
ro_continue_drag(block)
    int *block;			/* Just used as scrap. */
{
    int x, y;

    /* Get screen coords of pointer. */
    swi(Wimp_GetPointerInfo, 0, block);
    x = block[0] + drag_x_offset;
    y = block[1] + drag_y_offset;

    block[0] = gui.window_handle;
    swi(Wimp_GetWindowState, 0, block);

    if (ro_dragging == DRAG_RESIZE_WINDOW)
    {
	/* Resizeing the main window. */
	block[2] = y;
	block[3] = x;
	ro_open_main(block);
    }
    else
    {
	/* Selecting some text. */
	gui_send_mouse_event(
	    drag_button | MOUSE_DRAG,	/* Always report the same button */
	    x - block[1],
	    block[4] - y,
	    FALSE,			/* Not a double click. */
	    drag_modifiers);
    }
}

/* User has released all mouse buttons, marking the end of a drag. */
    void
ro_drag_finished(block)
    int *block;
{
    int x;
    int y;
    int width, height;

    /* I don't trust the box returned by Wimp_Poll; look at the pointer
     * ourselves.
     */
    swi(Wimp_GetPointerInfo, 0, block);
    x = block[0] + drag_x_offset;
    y = block[1] + drag_y_offset;

    if (ro_dragging == DRAG_RESIZE_WINDOW)
    {
	block[0] = gui.window_handle;
	swi(Wimp_GetWindowState, 0, block);
	block[2] = y;
	block[3] = x;
	ro_open_main(block);

	width = (block[3] - block[1]);
	height = (block[4] - block[2]);

	swi(Wimp_ForceRedraw, gui.window_handle, 0, -height, width, 0);
	gui_resize_shell(width, height);
    }
    else
    {
	block[0] = gui.window_handle;
	swi(Wimp_GetWindowState, 0, block);
	gui_send_mouse_event(
		MOUSE_RELEASE,
		x - block[1],
		block[4] - y,
		FALSE,			/* not a double click */
		drag_modifiers);
    }
    ro_dragging = DRAG_FALSE;
}

/* Load the file/pathname given in block into a [new] buffer.
 *
 * Modifier	Action
 *
 * None		:confirm e <file>
 * Ctrl		:sp <file>
 * Shift	<file>
 *
 * Insert into typebuf, at the start.
 * If loading from !Scrap then use saved leafname instead, and
 * delete the scrap file. Also, ignore shift key.
 *
 * NB: Doesn't send DataLoadAck (other app might delete temp file?).
 */
    void
ro_dataload(block)
    int	    *block;
{
    char_u  new_path[MAXPATHL];
    char_u  *path = ((char_u *) block) + 44;
    int	    scrap = FALSE;

    if (block[3] == leaf_ref && leaf_name)
	scrap = TRUE;

    switch (get_real_state() & 0xff)
    {
	case INSERT:
	case CMDLINE:
	case CMDLINE+LANGMAP:
	    /* For insert mode we can only insert the pathname (currently)
	     * Make sure Shift is pressed.
	     */
	    swi(OS_Byte, 121, 0x80);	    /* Is Shift pressed? */
	    if (r1 == 0xff)
	    {
		ins_typebuf(" ", REMAP_NONE, 0, TRUE, FALSE);
		ins_typebuf(path, REMAP_NONE, 0, TRUE, FALSE);
		ro_return_early = TRUE;		    /* Return even though nothing was typed. */
	    }
	    else
		swi(Wimp_ReportError,
			"\0\0\0\0Sorry, you can only load text in normal mode", 5, "Vim");
	    break;

	case NORMAL:
	    ro_return_early = TRUE;	    /* Return even though nothing was typed. */

	    if (scrap)			    /* Remove <Wimp$Scrap>. Later. */
		ins_typebuf(":!~remove <Wimp$Scrap>\r", REMAP_NONE, 0, TRUE, FALSE);

	    /* Insert {:sp ,:confirm e }[+f\ <leaf> ]<file><CR> */
	    ins_typebuf("\r", REMAP_NONE, 0, TRUE, FALSE);
	    ins_typebuf(path, REMAP_NONE, 0, TRUE, FALSE);
	    ins_typebuf(" ", REMAP_NONE, 0, TRUE, FALSE);

	    if (scrap)
	    {
		/* Loading via !Scrap - change pathname to stored leafname */
		ins_typebuf(leaf_name, REMAP_NONE, 0, TRUE, FALSE);
		ins_typebuf(" +f\\ ", REMAP_NONE, 0, TRUE, FALSE);
		leaf_ref = 0;
		vim_free(leaf_name);
		leaf_name = NULL;
	    }

	    swi(OS_Byte, 121, 0x81);	    /* Is Ctrl pressed? */
	    if (r1 == 0xff)
		/* Yes, split window */
		ins_typebuf(":sp", REMAP_NONE, 0, TRUE, FALSE);
	    else
		ins_typebuf(":confirm e", REMAP_NONE, 0, TRUE, FALSE);
	    break;

	default:
	    swi(Wimp_ReportError, "\0\0\0\0You can only load text in normal mode.", 5, "Vim");
    }
    /* Send DataSaveAck so other program doesn't think we died
     * and delete <Wimp$Scrap>.
     */
    block[3] = block[2];
    block[4] = 4;
    swi(Wimp_SendMessage, 17, block, block[1]);
}

    void
ro_datasave(block)
    int	    *block;
{
    char_u *path = ((char_u *) block) + 44;

    /* Preserve the name given so we can use it, not <Wimp$Scrap> */
    if (leaf_name)
	vim_free(leaf_name);
    leaf_name = vim_strsave(path);

    block[9] = -1;	    /* File is unsafe. */
    strcpy(path, "<Wimp$Scrap>");
    block[0] = 60;
    block[3] = block[2];
    block[4] = 2;
    swi(Wimp_SendMessage, 17, block, block[1]);

    leaf_ref = block[2];
}

    void
ro_message(block)
    int *block;
{
    char_u	*buffer;
    long_u	len;

    if (block[1] == task_handle)
	return;			    /* Don't talk to ourself! */
    switch (block[4])
    {
	case 0:		/* Quit. */
	    if (block[4] == 0)
		ro_quit();
	    break;
	case 1:	/* DataSave */
	    ro_datasave(block);
	    break;
	case 2:		/* DataSaveAck. */
	    if (clip_convert_selection(&buffer, &len, &clip_star) == -1)
		return;

	    /* Save the clipboard contents to a file. */
	    swi(OS_File, 10, ((char_u *) block) + 44, 0xfff, 0, buffer, buffer + len);

	    /* Ack with DataLoad message. */
	    block[3] = block[2];
	    block[4] = 3;
	    block[9] = len;
	    swi(Wimp_SendMessage, 17, block, block[1]);

	    vim_free(buffer);
	    break;
	case 3:		/* DataLoad */
	    ro_dataload(block);
	    break;
	case 8:		/* PreQuit */
	    ro_prequit(block);
	    break;
	case 0xf:	/* Lose clipboard. */
	    if (block[5] & 4)
	    {
		clip_free_selection(&clip_star);
		clip_star.owned = FALSE;
	    }
	    break;
	case 0x10:	/* DataRequest (clip_star) */
	    if (clip_star.owned)
	    {
		int rows;

		/* Tell other program that we have the clipboard. */
		block[0] = 52;
		block[3] = block[2];	    /* Copy myref to yourref. */
		block[4] = 1;		    /* DataSave message. */
		/* Create an estimate for the size (larger or same as true
		 * value) */
		rows = clip_star.end.lnum - clip_star.start.lnum;
		if (rows < 0)
		    rows = -rows;
		block[9] = (rows + 1) * Columns + 1; /* Add one for possible
							final newline. */
		block[10] = 0xfff;	    /* Clipboard is text. */
		strcpy( ((char_u *) block) + 44, "VimClip");
		swi(Wimp_SendMessage, 17, block, block[1]);
	    }
	    break;
	case 0x400c1:	/* Mode change */
	    changed_mode = TRUE;		/* Flag - update on next OpenWindow */
	    if (zap_redraw)
	    {
		/* JK230798, re-initialise ZapRedraw stuff */
		if (ro_zap_redraw_initialise() == FAIL)
		    zap_redraw = FALSE;
	    }
	    break;
	case 0x400c3:	/* TaskCloseDown */
	    if (block[1] == child_handle)
		child_handle = 0;
	    break;
    }
}

/*
 * Converts a scrollbar's window handle into a scrollbar pointer.
 * NULL on failure.
 */
    scrollbar_T *
ro_find_sbar(id)
    int		id;
{
    win_T	*wp;

    if (gui.bottom_sbar.id == id)
	return &gui.bottom_sbar;
    FOR_ALL_WINDOWS(wp)
    {
	if (wp->w_scrollbars[SBAR_LEFT].id == id)
	    return &wp->w_scrollbars[SBAR_LEFT];
	if (wp->w_scrollbars[SBAR_RIGHT].id == id)
	    return &wp->w_scrollbars[SBAR_RIGHT];
    }
    return NULL;
}

    void
scroll_to(line, sb)
    int sb;	/* Scrollbar number */
    int line;
{
    char_u code[8];

    /* Don't put events in the input queue now. */
    if (hold_gui_events)
	return;

    /* Send a scroll event:
     *
     * A scrollbar event is CSI (NOT K_SPECIAL), KS_VER_SCROLLBAR,
     * KE_FILLER followed by:
     * one byte representing the scrollbar number, and then four bytes
     * representing a long_u which is the new value of the scrollbar.
     */
    code[0] = CSI;
    code[1] = KS_VER_SCROLLBAR;
    code[2] = KE_FILLER;
    code[3] = sb;
    code[4] = line >> 24;
    code[5] = line >> 16;
    code[6] = line >> 8;
    code[7] = line;
    add_to_input_buf(code, 8);
}

    void
h_scroll_to(col)
    int col;
{
    char_u code[8];

    /* Don't put events in the input queue now. */
    if (hold_gui_events)
	return;

    /* Send a scroll event:
     *
     * A scrollbar event is CSI (NOT K_SPECIAL)
     *
     * A horizontal scrollbar event is K_SPECIAL, KS_HOR_SCROLLBAR,
     * KE_FILLER followed by four bytes representing a long_u which is the
     * new value of the scrollbar.
     */
    code[0] = CSI;
    code[1] = KS_HOR_SCROLLBAR;
    code[2] = KE_FILLER;
    code[4] = col >> 24;
    code[5] = col >> 16;
    code[6] = col >> 8;
    code[7] = col;
    add_to_input_buf(code, 8);
}

    void
ro_scroll(block)
    int		*block;
{
    scrollbar_T	*sb;
    int		offset;
    win_T	*wp;

    /* Block is ready for Wimp_OpenWindow, and also contains:
     *
     * +32 = scroll X direction (-2 .. +2)
     * +36 = scroll Y direction (-2 .. +2)
     */

    sb = ro_find_sbar(block[0]);
    if (!sb)
	return;		/* Window not found (error). */

    wp = sb-> wp;

    if (wp == NULL)
    {
	/* Horizontal bar. */
	offset = block[8];
	if (offset == -2)
	    offset = (block[1] - block[3]) / gui.char_width;
	else if (offset == 2)
	    offset = (block[3] - block[1]) / gui.char_width;

	block[5] += offset * gui.char_width;

	gui_drag_scrollbar(sb, block[5] / gui.char_width, FALSE);

	swi(Wimp_OpenWindow, 0, block);
    }
    else
    {
	offset = -block[9];
	if (offset == -2)
	    offset = -(wp -> w_height - 1);
	else if (offset == 2)
	    offset = wp -> w_height - 1;

	/* Possibly we should reposition the scrollbar?
	 * Vim seems to update the bar anyway...
	 */
	gui_drag_scrollbar(sb, offset - (block[6] / gui.char_height), FALSE);
    }
}

/* Move a window by a given offset. Used to simulate the function of the
 * nested wimp.
 */
    void
ro_move_child(window, x, y, pos_wanted, pos_got)
    int	window;
    int x,y;		/* offset to move by */
    int	pos_wanted, pos_got;
{
    int	block[10];

    block[0] = window;
    swi(Wimp_GetWindowState, 0, block);
    block[1] += x;
    block[2] += y;
    block[3] += x;
    block[4] += y;
    if (pos_wanted == -1)
	block[7] = -1;
    else if (pos_wanted == -2)
	block[7] = pos_got;
    swi(Wimp_OpenWindow, 0, block);
}

/* Open the main window. Also updates scrollbars if we are not
 * using the nested Wimp.
 * If we have just changed mode then re-read all values.
 */
    void
ro_open_main(block)
    int	    *block;
{
    int	    toggle_size;

    /* Find out if the user clicked on the toggle size icon. */
    block[20] = block[0];
    swi(Wimp_GetWindowState, 0, block + 20);
    toggle_size = block[28] & (1 << 19);

    if (nested_wimp)
    {
	swi(Wimp_OpenWindow, 0, block);
    }
    else
    {
	int	old[10];
	int	x_offset, y_offset;	    /* Move children same as parent. */
	int	pos_wanted, pos_got;
	int	left_bar  = gui.which_scrollbars[SBAR_LEFT];
	int	right_bar = gui.which_scrollbars[SBAR_RIGHT];
	win_T	*wp;

	/* Three cases to think about:
	 * 1) Move to top. Open each window at the top.
	 * 2) Same stack position. Open each with same position.
	 * 3) Open at bottom. Open children with parent's new position.
	 */

	old[0] = block[0];
	swi(Wimp_GetWindowState, 0, old);
	pos_wanted = block[7];
	swi(Wimp_OpenWindow, 0, block);
	/* Block updated by OpenWindow? I don't think so! */
	swi(Wimp_GetWindowState, 0, block);
	pos_got = block[7];

	x_offset = block[1] - old[1];
	y_offset = block[4] - old[4];
	if (x_offset || y_offset || pos_wanted == -1 || pos_wanted == -2)
	{
	    /* If parent has moved, re-open all the child windows. */
	    FOR_ALL_WINDOWS(wp)
	    {
		/* Reopen scrollbars for this window. */
		if (left_bar)
		    ro_move_child(wp -> w_scrollbars[SBAR_LEFT].id,
				x_offset, y_offset,
				pos_wanted, pos_got);
		if (right_bar)
		    ro_move_child(wp -> w_scrollbars[SBAR_RIGHT].id,
				x_offset, y_offset,
				pos_wanted, pos_got);
	    }
	}
    }
    if (changed_mode || toggle_size)
    {
	int	width, height;

	if (changed_mode)
	    ro_measure_tools();
	block[0] = gui.window_handle;
	swi(Wimp_GetWindowState, 0, block);

	width = block[3] - block[1];
	height = block[4] - block[2];
	swi(Wimp_ForceRedraw, gui.window_handle, 0, -height, width, 0);
	gui_resize_shell(width, height);
	changed_mode = FALSE;
    }
}

    void
ro_open_window(block)
    int		*block;
{
    int		pos;
    scrollbar_T *sb;

    if (block[0] == gui.window_handle)
	ro_open_main(block);
    else
    {
	swi(Wimp_OpenWindow, 0, block);
	if (block[0] != gui.window_handle)
	{
	    sb = ro_find_sbar(block[0]);
	    if (sb)
	    {
		if (sb-> wp != NULL)
		    gui_drag_scrollbar(sb, -block[6] / gui.char_height, FALSE);
		else
		    gui_drag_scrollbar(sb, block[5] / gui.char_width, FALSE);
	    }
	}
    }
}

    void
ro_menu_selection(block)
    int		*block;
{
    int		*item = wimp_menu + 7;
    vimmenu_T	*menu;
    /* wimp_menu points to a wimp menu structure */

    for (;;)
    {
	while (block[0]--)
	    item += 6;
	if (block[1] == -1)
	    break;
	item = ((int *) item[1]) + 7;
	block++;
    }
    /* item points to the wimp menu item structure chosen */
    menu = (vimmenu_T *) item[5];

    swi(Wimp_GetPointerInfo, 0, block);
    if (block[2] == 1)
	/* Adjust used - keep menu open */
	swi(Wimp_CreateMenu, 0, wimp_menu);

    if (menu-> cb)
	menu-> cb(menu);
}

    void
ro_open_parent()
{
    int head;
    char_u *i = curbuf-> b_ffname;
    char_u  buffer[256];

    head = 0;
    for (; *i; i++)
    {
	if (*i == '.')
	    head = i - curbuf-> b_ffname;
    }

    /* Append head chars to buffer */
    if (head < 240 && curbuf-> b_ffname && head)
    {
	strcpy(buffer, "%filer_opendir ");
	strncpy(buffer + 15, curbuf-> b_ffname, head);
	buffer[15 + head] = '\0';
	swi(OS_CLI, buffer);
    }
}

    void
process_event(event, block)
    int event;
    int *block;
{
    switch (event)
    {
	case 0:		/* Nothing - update drag state. */
	    if (ro_dragging)
		ro_continue_drag(block);
	    break;
	case 1:		/* Redraw window. */
	    redraw_window(block);
	    break;
	case 2:		/* Open window. */
	    ro_open_window(block);
	    break;
	case 3:		/* Close window. */
	    swi(Wimp_GetPointerInfo, 0, block + 1);
	    if (block[3] == 1)
		ro_open_parent();
	    else
		if (ro_ok_to_quit())
		    ro_quit();
	    break;
	case 6:		/* Mouse click. */
	    ro_mouse(block);
	    break;
	case 7:		/* Finished drag. */
	    ro_drag_finished(block);
	    break;
	case 8:		/* Key pressed. */
	    ro_insert_key((char_u *) &block[6]);
	    break;
	case 9:
	    ro_menu_selection(block);
	    break;
	case 10:	/* Scroll request. */
	    ro_scroll(block);
	    break;
	case 11:	/* Lose caret. */
	    if (block[0] == gui.window_handle)
		gui_focus_change(FALSE);
	    break;
	case 12:	/* Gain caret. */
	    if (block[0] == gui.window_handle)
		gui_focus_change(TRUE);
	    break;
	case 17:	/* User message. */
	case 18:	/* User message recorded. */
	    ro_message(block);
	    break;
    }
}

/*
 * GUI input routine called by gui_wait_for_chars().  Waits for a character
 * from the keyboard.
 *  wtime == -1	    Wait forever.
 *  wtime == 0	    This should never happen.
 *  wtime > 0	    Wait wtime milliseconds for a character.
 * Returns OK if a character was found to be available within the given time,
 * or FAIL otherwise.
 */
    int
gui_mch_wait_for_chars(long wtime)
{
    int block[64];
    int	reason;
    int start_time = -1;
    int ctime = wtime / 10;	/* delay in cs */

    if (wtime != -1)
    {
	swi(OS_ReadMonotonicTime);
	start_time = r0;
    }

    for (;;)
    {
	if (ro_dragging)
	    reason = wimp_poll(0, block);	/* Always return immediately */
	else if (wtime == -1)
	    reason = wimp_poll(1, block);
	else
	    reason = wimp_pollidle(0, block, start_time + ctime);

	process_event(reason, block);

	if (input_available() || ro_return_early)
	{
	    ro_return_early = FALSE;
	    return OK;	    /* There is something to process (key / menu event) */
	}

	if (wtime != -1)
	{
	    swi(OS_ReadMonotonicTime);
	    if (r0 - start_time > ctime)
		return FAIL;	/* We've been waiting too long - return failure */
	}
    }
}

/* Flush any output to the screen */
    void
gui_mch_flush(void)
{
}

/*
 * Clear a rectangular region of the screen from text pos(row1, col1) to
 * (row2, col2) inclusive.
 */
    void
gui_mch_clear_block(int row1, int col1, int row2, int col2)
{
    swi(ColourTrans_ReturnGCOL, gui.back_pixel << 8, 0, 0, 1<<7, 0);
    plot_rectangle(96 + 5, r0,
			FILL_X(col1), -FILL_Y(row2 + 1),
			FILL_X(col2 + 1), -FILL_Y(row1));
}

    void
gui_mch_clear_all(void)
{
    if (redraw_block)
    {
	swi(ColourTrans_SetGCOL, gui.back_pixel << 8, 0, 0, 1<<7, 0);
	swi(OS_WriteI + 16);
    }
    else
    {
	int block[44];
	block[0] = gui.window_handle;
	block[1] = 0;
	block[2] = -gui.num_rows * gui.char_height;
	block[3] = gui.num_cols * gui.char_width;
	block[4] = 0;
	swi(Wimp_UpdateWindow, 0, block);
	while (r0)
	{
	    swi(ColourTrans_SetGCOL, gui.back_pixel << 8, 0, 0, 1<<7, 0);
	    swi(OS_WriteI + 16);
	    swi(Wimp_GetRectangle, 0, block);
	}
    }
}

/*
 * Delete the given number of lines from the given row, scrolling up any
 * text further down within the scroll region.
 */
    void
gui_mch_delete_lines(int row, int num_lines)
{
    int top_from = -row - num_lines;
    int bot_from = -gui.scroll_region_bot - 1;
    int bot_to   = bot_from + num_lines;

    swi(ColourTrans_SetGCOL, gui.back_pixel << 8, 0, 0, 0x80, 0);

    /* Changed without checking! */
    swi(Wimp_BlockCopy, gui.window_handle,
			    gui.scroll_region_left * gui.char_width,
			    bot_from * gui.char_height,
			    (gui.scroll_region_right - gui.scroll_region_left
							+ 1) * gui.char_width,
			    top_from * gui.char_height,

			    gui.scroll_region_left * gui.char_width,
			    bot_to * gui.char_height);

    gui_clear_block(gui.scroll_region_bot - num_lines + 1,
						       gui.scroll_region_left,
	gui.scroll_region_bot, gui.scroll_region_right);
}

/*
 * Insert the given number of lines before the given row, scrolling down any
 * following text within the scroll region.
 */
    void
gui_mch_insert_lines(int row, int num_lines)
{
    int top_from = -row;
    int bot_to   = -gui.scroll_region_bot - 1;
    int bot_from = bot_to + num_lines;

    swi(ColourTrans_SetGCOL, gui.back_pixel << 8, 0, 0, 0x80, 0);

    swi(Wimp_BlockCopy, gui.window_handle,
			    gui.scroll_region_left * gui.char_width,
			    bot_from * gui.char_height,
			    (gui.scroll_region_right - gui.scroll_region_left
							+ 1) * gui.char_width,
			    top_from * gui.char_height,

			    gui.scroll_region_left * gui.char_width,
			    bot_to * gui.char_height);

    gui_clear_block(row, gui.scroll_region_left,
				row + num_lines - 1, gui.scroll_region_right);
}

/* Put selection in clipboard buffer.
 * Should we become the new owner?
 */
    void
clip_mch_request_selection(VimClipboard *cbd)
{
    int		block[64];	/* Will be used in Wimp_Poll. */
    int		reason;
    char_u	*buffer;
    long_u	length;

    block[0] = 48;			/* Size of block. */
    block[3] = 0;			/* Orinial message. */
    block[4] = 0x10;			/* Data request. */
    block[5] = gui.window_handle;
    block[6] = RO_LOAD_CLIPBOARD;	/* Internal handle. */
    block[7] = block[8] = 0;		/* (x,y) not used. */
    block[9] = 4;
    block[10] = 0xfff;	    /* We want text files if possible, I think. */
    block[11] = -1;	    /* End of list. */
    swi(Wimp_SendMessage, 17, block, 0);    /* Broadcast request. */

    /* OK, we've sent the request. Poll until we get a null poll (failure) or
     * we load the clipboard.
     * If we receive a DataSave event with icon handle = -2 then put it on the
     * clipboard. RISC OS should ensure that key events will not be delivered
     * until the clipboard operation completes (unless the owner starts idling
     * - we can't wait forever!).
     */
    for (;;)
    {
	reason = wimp_poll(0, block);
	if (reason == 0)
	    return;	    /* Failed to get clipboard. */
	if ((reason == 17 || reason == 18) &&
		block[4] == 1 && block[6] == RO_LOAD_CLIPBOARD)
	    break;	    /* Got it - stop waiting. */
	process_event(reason, block);
	if (ro_return_early)
	    return;
    }
    /* Tell owner to save data in <Wimp$Scrap>. */
    block[0] = 60;
    block[3] = block[2];   /* Copy myref -> yourref */
    block[4] = 2;	    /* DataSaveAck. */
    block[9] = -1;	    /* Data is unsafe. */
    strcpy( ((char_u *) block) + 44, "<Wimp$Scrap>");
    swi(Wimp_SendMessage, 17, block, block[1]);

    /* Wait again for reply. */
    for (;;)
    {
	reason = wimp_poll(0, block);
	if (reason == 0)
	    return;	/* Other program has given up! */
	if ((reason == 17 || reason == 18) && block[4] == 3 && block[6] == RO_LOAD_CLIPBOARD)
	    break;	/* Clipboard data saved to <Wimp$Scrap> */
	process_event(reason, block);
	if (ro_return_early)
	    return;
    }

    /* <Wimp$Scrap> contains clipboard - load it. */
    if (xswi(OS_File, 17, "<Wimp$Scrap>") & v_flag)
	return;		/* Error! */
    if (r0 != 1 && r0 != 3)
	return;
    length = r4;

    buffer = lalloc(length, TRUE);  /* Claim memory (and report errors). */
    if (buffer == NULL)
	return;

    if (xswi(OS_File, 16, "<Wimp$Scrap>", buffer, 0) & v_flag)
	return;

    clip_yank_selection(MCHAR, buffer, length, cbd);

    vim_free(buffer);

    swi(OS_FSControl, 27, "<Wimp$Scrap>", 0, 0);    /* Delete temp file. */

    block[4] = 4;		    /* Send DataLoadAck. */
    block[3] = block[2];	    /* Copy myref -> yourref. */
    swi(Wimp_SendMessage, 17, block, block[1]);
}

/* Not sure what this means under RISC OS. */
    void
clip_mch_lose_selection(VimClipboard *cbd)
{
}

/* Tell everyone that we now own the clipboard.
 * Return OK if our claim is accepted (always, under RISC OS)
 */
    int
clip_mch_own_selection(VimClipboard *cbd)
{
    int block[6];
    block[0] = 24;	/* Length of block.  */
    block[3] = 0;	/* Original message. */
    block[4] = 0xf;	/* ClaimEntity. */
    block[5] = 0x4;	/* Claim clipboard only. */
    swi(Wimp_SendMessage, 17, block, 0);
    return OK;
}

/*
 * Send the current selection to the clipboard.  Do nothing for X because we
 * will fill in the selection only when requested by another app. Sounds good
 * for RISC OS too.
 */
    void
clip_mch_set_selection(VimClipboard *cbd)
{
    clip_get_selection(cbd);
}

/*
 * Make a menu either grey or not grey.
 */
    void
gui_mch_menu_grey(vimmenu_T *menu, int grey)
{
    menu-> greyed_out = grey;
}

/*
 * Make menu item hidden or not hidden
 */
    void
gui_mch_menu_hidden(vimmenu_T *menu, int hidden)
{
    menu-> hidden = hidden;
}

/*
 * This is called after setting all the menus to grey/hidden or not.
 */
    void
gui_mch_draw_menubar(void)
{
    swi(Wimp_CreateMenu, 0, -1);
    if (wimp_menu != (int *) -1)
    {
	ro_remove_menu(wimp_menu);
	wimp_menu = (int *) -1;
    }
}

/* Add or remove a scrollbar. Note that this is only called when
 * the scrollbar state is changing.
 * The scroll bar window has already been created.
 * We can't do anything except remove the scroll bar
 * until we know what size to use.
 */
    void
gui_mch_enable_scrollbar(sb, flag)
    scrollbar_T	*sb;
    int		flag;
{
    if (!flag)
	swi(Wimp_CloseWindow, 0, & (sb->id) );
    return;
}

    void
gui_mch_set_blinking(long waittime, long on, long off)
{
}

/*
 * Stop the cursor blinking.  Show the cursor if it wasn't shown.
 */
    void
gui_mch_stop_blink(void)
{
}

/*
 * Start the cursor blinking.  If it was already blinking, this restarts the
 * waiting time and shows the cursor.
 */
    void
gui_mch_start_blink(void)
{
}

/*
 * Return the RGB value of a pixel as a long.
 */
    long_u
gui_mch_get_rgb(guicolor_T pixel)
{
    return (long_u)pixel;
}

    void
gui_mch_set_text_area_pos(int x, int y, int w, int h)
{
}

    void
gui_mch_enable_menu(int flag)
{
}

    void
gui_mch_set_menu_pos(int x, int y, int w, int h)
{
}

    void
gui_mch_add_menu(vimmenu_T *menu, int idx)
{
}

    void
gui_mch_add_menu_item(vimmenu_T *menu, int idx)
{
}

    void
gui_mch_new_menu_colors(void)
{
}

    void
gui_mch_destroy_menu(vimmenu_T *menu)
{
}

/* Size of buffer has changed.
 * Add one to max since gui.c subtracts one more than it should!
 */
    void
gui_mch_set_scrollbar_thumb(sb, val, size, max)
    scrollbar_T	*sb;
    long	val;
    long	size;
    long	max;
{
    int		block[10], width, height;

    width = (max + 1) * gui.char_width;
    height = (max + 1 + W_STATUS_HEIGHT(sb->wp)) * gui.char_height;

    block[0] = block[3] = 0;
    block[1] = -height + (1 << y_eigen_factor);
    block[2] = width;

    swi(Wimp_SetExtent, sb -> id, block);

    block[0] = sb -> id;
    swi(Wimp_GetWindowState, 0, block);
    block[5] = val * gui.char_width;
    block[6] = -val * gui.char_height;
    swi(Wimp_OpenWindow, 0, block, 0x4b534154,
			gui.window_handle,	/* Parent window handle. */
			(CHILD_FIX_TO_RIGHT  << CHILD_LEFT  )   |
			(CHILD_FIX_TO_RIGHT  << CHILD_RIGHT )   |
			(CHILD_FIX_TO_BOTTOM << CHILD_TOP   )   |
			(CHILD_FIX_TO_BOTTOM << CHILD_BOTTOM)   |
			(CHILD_SELF_SCROLL   << CHILD_SCROLL_X) |
			(CHILD_SELF_SCROLL   << CHILD_SCROLL_Y)
			);
}

/* Set the position of the scrollbar within the editor
 * window. Note that, for vertical scrollbars, x and w
 * are ignored. For horizontal bars y and h are ignored.
 */
    void
gui_mch_set_scrollbar_pos(sb, x, y, w, h)
    scrollbar_T *sb;
    int		x;		/* Horizontal sb position */
    int		y;		/* Top of scroll bar */
    int		w;		/* Width */
    int		h;		/* Height */
{
    int		block[24];
    int		px1, py1;	/* Parent window min coords */
    int		px2, py2;	/* Parent window max coords */

    /* Find where the parent window is. */
    block[0] = gui.window_handle;
    swi(Wimp_GetWindowState, 0, block);
    px1 = block[1];
    py1 = block[2];
    px2 = block[3];
    py2 = block[4];

    block[0] = sb -> id;

    /* Find out how big the scroll window is at the moment. */
    swi(Wimp_GetWindowInfo, 0, ((char_u *)block) + 1);

    if (block[13] < w || block[12] > -h)
    {
	/* Current window is too small! */
	if (block[12] > -h)
	    block[12] = -h;
	if (block[13] < w)
	    block[13] = w;
	swi(Wimp_SetExtent, block[0], block + 11);
    }

    /* This works better on the nested_wimp. */
    if (sb-> wp)
    {
	/* This is a vertical scrollbar. */
	block[1] = block[3] = px2 - gui.scrollbar_width + (1 << x_eigen_factor);
	block[2] = 1 + py2 - (y + h) + (1 << y_eigen_factor);
	block[4] = 1 + py2 - y;
    }
    else
    {
	/* This is a horizontal scrollbar. */
	block[2] = block[4] = py1 + gui.scrollbar_height;
	block[1] = px1;
	block[3] = px2 - gui.scrollbar_width;
    }

    block[5] = 0;
    block[6] = 0;
    block[7] = -1;

    swi(Wimp_OpenWindow, 0, block, 0x4b534154,
	    gui.window_handle,	/* Parent window handle. */
	    (CHILD_FIX_TO_RIGHT  << CHILD_LEFT  )   |
	    (CHILD_FIX_TO_RIGHT  << CHILD_RIGHT )   |
	    (CHILD_FIX_TO_BOTTOM << CHILD_TOP   )   |
	    (CHILD_FIX_TO_BOTTOM << CHILD_BOTTOM)   |
	    (CHILD_SELF_SCROLL   << CHILD_SCROLL_X) |
	    (CHILD_SELF_SCROLL   << CHILD_SCROLL_Y)
       );
}

/* Create a window with no workarea to place inside editor window.
 * (what happens without the nested wimp?)
 * Data for scrollbar is invalid.
 */
    void
gui_mch_create_scrollbar(sb, orient)
    scrollbar_T *sb;
    int		orient;	/* orient is SBAR_HORIZ or SBAR_VERT */
{
    int bar[] =
	{
	    0,   0,		/* Visible area : min X,Y */
	    100, 100,		/*		  max X,Y */
	    0,   0,		/* Scroll offsets */
	    -1,			/* Window in front */
	    0x80800150 | (orient == SBAR_HORIZ ? (1 << 30) : (1 << 28)),
	    0xff070207,		/* Colours */
	    0x000c0103,		/* More colours */
	    0, -0x4000,		/* Workarea extent */
	    0x4000, 0,		/* max X,Y */
	    0x00000000,		/* No title */
	    0 << 12,		/* No workarea button type */
	    1,			/* Wimp sprite area */
	    0x00010001,		/* Minimum width, height */
	    0, 0, 0,		/* Title data (none) */
	    0			/* No icons */
	};
    swi(Wimp_CreateWindow, 0, bar);
    sb -> id = r0;
}

#if defined(FEAT_WINDOWS) || defined(PROTO)
    void
gui_mch_destroy_scrollbar(scrollbar_T *sb)
{
    swi(Wimp_DeleteWindow, 0, & (sb->id));
    sb -> id = -1;
}
#endif

    void
gui_mch_set_scrollbar_colors(scrollbar_T *sb)
{
    /* Always use default RO colour scheme. */
}

/*
 * Get current mouse coordinates in text window.
 * Note: (0,0) is the bottom left corner, positive y is UP.
 */
    void
gui_mch_getmouse(x, y)
    int *x;
    int *y;
{
    int left;
    int top;
    int block[10];

    block[0] = gui.window_handle;
    swi(Wimp_GetWindowState, 0, block);
    left = block[1];
    top = block[4];

    swi(Wimp_GetPointerInfo, 0, block);
    *x = block[0] - left;
    *y = top - block[1];
}

/* MouseTo(x, y) */
    void
gui_mch_setmouse(x, y)
    int		x;
    int		y;
{
}

    void
gui_mch_toggle_tearoffs(enable)
    int		enable;
{
    /* no tearoff menus */
}

/* Redraw a window's title.
 * For the nested wimp we use the new 'redraw-title-bar' reason code.
 * For older wimps we mark the area of the screen where the title bar
 * is as invalid.
 */
    void
ro_redraw_title(window)
    int window;
{
    if (nested_wimp)
    {
	swi(Wimp_ForceRedraw, window, 0x4b534154, 3);
    }
    else
    {
	int block[10];
	int miny;

	block[0] = window;
	swi(Wimp_GetWindowState, 0, block);
	miny = block[4];
	swi(Wimp_GetWindowOutline, 0, block);
	swi(Wimp_ForceRedraw, -1,
			block[1], miny,
			block[3], block[4]);
    }
}

/* Turn a vimmenu_T structure into a wimp menu structure.
 * -1 if resulting menu is empty.
 * Only the children and dname items in the root menu are used.
 */
    int *
ro_build_menu(menu)
    vimmenu_T	*menu;
{
    int		*wimp_menu;
    int		width = 4;
    int		w;
    int		size = 28;
    vimmenu_T	*item;
    int		*wimp_item;

    /* Find out how big the menu is so we can allocate memory for it */
    for (item = menu-> children; item; item = item-> next)
    {
	if (item-> hidden == FALSE && !menu_is_separator(item->name))
	    size += 24;
    }

    if (size <= 28)
	return (int *) -1;		/* No children - shouldn't happen */

    wimp_menu = (int *) alloc(size);

    wimp_menu[0] = (int) menu-> dname;
    wimp_menu[1] = -1;
    wimp_menu[2] = 0;
    wimp_menu[3] = 0x00070207;
    wimp_menu[5] = 44;
    wimp_menu[6] = 0;

    wimp_item = wimp_menu + 7;

    for (item = menu-> children; item; item = item-> next)
    {
	if (menu_is_separator(item-> name))
	{
	    /* This menu entry is actually a separator. If it is not the first
	     * menu entry then mark the previous menu item as needing a dotted
	     * line after it.
	     */
	    if (wimp_item > wimp_menu + 7)
		wimp_item[-6] |= 0x2;
	}
	else if (item-> hidden == FALSE)
	{
	    wimp_item[0] = 0;
	    wimp_item[1] = item-> children ? (int) ro_build_menu(item) : -1;
	    wimp_item[2] = 0x07009131 | (item-> greyed_out << 22);
	    wimp_item[3] = (int) item-> dname;
	    wimp_item[4] = -1;
	    wimp_item[5] = (int) item;  /* Stuff the menu address in this unused space */

	    w = strlen(item-> dname) + 1;
	    if (w > width)
		width = w;
	    wimp_item += 6;
	}
    }

    wimp_menu[4] = (width + 2) * 16;
    wimp_menu[7]  |= 0x100;	    /* Menu title is indirected */
    wimp_item[-6] |= 0x080;	    /* Last entry in menu */
    return wimp_menu;
}

    static void
ro_remove_menu(menu)
    int	    *menu;
{
    int	    *item = menu + 7;

    if (menu == NULL || menu == (int *) -1)
	return;

    for (;;)
    {
	if (item[1] != -1)
	    ro_remove_menu((int *) item[1]);	/* Remove sub-menu */
	if (item[0] & 0x80)
	    break;			/* This was the last entry */
	item += 6;
    }
    vim_free(menu);
}

    void
gui_mch_show_popupmenu(menu)
    vimmenu_T	*menu;
{
    int		block[10];

    /* Remove the existing menu, if any */
    if (wimp_menu != (int *) -1)
    {
	swi(Wimp_CreateMenu, 0, -1);
	ro_remove_menu(wimp_menu);
	wimp_menu = (int *) -1;
    }

    wimp_menu = ro_build_menu(menu);
    if (wimp_menu != (int *) -1)
    {
	swi(Wimp_GetPointerInfo, 0, block);
	swi(Wimp_CreateMenu, 0, wimp_menu, block[0] - 64, block[1] + 64);
    }
}

/* Run a command using the TaskWindow module.
 * If SHELL_FILTER is set then output is not echoed to the screen,
 * If it is not set, then \r is not sent to the output file.
 */
    int
gui_mch_call_shell(cmd, options)
    char_u  *cmd;
    int	    options;	/* SHELL_FILTER if called by do_filter() */
			/* SHELL_COOKED if term needs cooked mode */
{
    char_u  task_cmd[256];	/* Contains *TaskWindow command. */
    int	    block[64];
    int	    reason;
    char_u  *out;
    char_u  c;
    int	    old_msg_col;
    char_u  *out_redir;
    int	    length;
    FILE    *out_file = NULL;

    out_redir = strstr(cmd, " > ");
    if (out_redir == NULL)
	length = strlen(cmd);	/* No redirection. */
    else
    {
	length = out_redir - cmd;
	out_file = fopen(out_redir + 3, "wb");
	if (out_file == NULL)
	    smsg("WARNING : Can't open file %s for writing\n", out_redir + 3);
    }

    if (length > 180)
    {
	if (out_file)
	    fclose(out_file);
	return FAIL;		/* Command too long. */
    }

    strcpy(task_cmd, "TaskWindow \"");
    strncpy(task_cmd + 12, cmd, length);
    sprintf(task_cmd + 12 + length,
	    "\" -task &%08x -ctrl -quit -name \"Vim command\"",
	    task_handle);

    if (options & SHELL_COOKED)
	settmode(TMODE_COOK);

    if (xswi(Wimp_StartTask, task_cmd) & v_flag)
    {
	/* Failed to even start a new task (out of memory?) */
	settmode(TMODE_RAW);
	if (out_file)
	    fclose(out_file);
	return FAIL;
    }

    /* Wait for the child process to initialise. */
    child_handle = 0;
    while (!child_handle)
    {
	reason = wimp_poll(0, block);
	if ((reason == 17 || reason == 18) && block[4] == 0x808c2)
	    child_handle = block[1];
	else
	    process_event(reason, block);
    }

    /* Block until finished */
    while (child_handle)
    {
	reason = wimp_poll(1, block);
	if (reason == 3 || (reason == 8 && block[6] == 3))
	{
	    /* Close window request or CTRL-C - kill child task. */
	    block[0] = 20;
	    block[3] = 0;
	    block[4] = 0x808c4;	    /* Morite */
	    swi(Wimp_SendMessage, 17, block, child_handle);
	    MSG_PUTS(_("\nSending message to terminate child process.\n"));
	    continue;
	}
	else if (reason == 8)
	{
	    block[0] = 28;
	    block[3] = 0;
	    block[4] = 0x808c0;	    /* Input */
	    block[5] = 1;
	    /* Block[6] is OK as it is! */
	    swi(Wimp_SendMessage, 17, block, child_handle);
	    continue;
	}
	else if (reason == 17 || reason == 18)
	{
	    if (block[4] == 0x808c1)
	    {
		/* Ack message. */
		block[3] = block[2];
		swi(Wimp_SendMessage, 19, block, block[1]);
		out = (char_u *)block + 24;
		old_msg_col = msg_col;
		while (block[5]--)
		{
		    c = *out++;
		    if (out_file && (c != '\r' || (options & SHELL_FILTER)))
			fputc(c, out_file);
		    if ((options & SHELL_FILTER) == 0)
		    {
			if (c == 127)
			    msg_puts("\b \b");
			else if (c > 31)
			    msg_putchar(c);
			else if (c == 10)
			{
			    lines_left = 8;	/* Don't do More prompt! */
			    msg_putchar(10);
			}
		    }
		}
		/* Flush output to the screen. */
		windgoto(msg_row, msg_col);
		out_flush();
		continue;
	    }
	}
	process_event(reason, block);
    }
    msg_putchar('\n');
    settmode(TMODE_RAW);
    if (out_file)
	fclose(out_file);
    return OK;
}

/* Like strsave(), but stops at any control char */
    char_u *
wimp_strsave(str)
    char    *str;
{
    int	    strlen = 0;
    char_u  *retval;
    while (str[strlen] > 31)
	strlen++;
    retval = alloc(strlen + 1);
    if (retval)
    {
	memcpy(retval, str, strlen);
	retval[strlen] = '\0';
    }
    return retval;
}

/* If we are saving then pop up a standard RISC OS save box.
 * Otherwise, open a directory viewer on the given directory (and return NULL)
 * The string we return will be freed later.
 */
    char_u *
gui_mch_browse(saving, title, dflt, ext, initdir, filter)
    int		saving;		/* write action */
    char_u	*title;		/* title for the window */
    char_u	*dflt;		/* default file name */
    char_u	*ext;		/* extension added */
    char_u	*initdir;	/* initial directory, NULL for current dir */
    char_u	*filter;	/* file name filter */
{
    char command[256];
    int length;

    if (saving)
    {
	int	block[64];
	int	reason;
	int	done_save = FALSE;
	char_u	*retval = NULL;
	char_u  *sprname;
	char_u	*fname;
	int	dragging_icon = FALSE;
	int	filetype;

	if (!dflt)
	    dflt = "TextFile";

	block[0] = save_window;
	block[1] = 0;
	swi(Wimp_GetIconState, 0, block);
	sprname = ((char_u *) block[7]);
	block[1] = 1;
	swi(Wimp_GetIconState, 0, block);
	fname = ((char *) block[7]);
	strncpy(fname, dflt, 255);

	if (xswi(OS_FSControl, 31, curbuf->b_p_oft) & v_flag)
	{
	    filetype = 0xfff;
	    strcpy(sprname + 5, "xxx");
	}
	else
	{
	    filetype = r2;
	    sprintf(sprname + 5, "%03x", filetype);
	}

	/* Open the save box */

	swi(Wimp_GetPointerInfo, 0, block);
	swi(Wimp_CreateMenu, 0, save_window, block[0] - 64, block[1] + 64);
	swi(Wimp_SetCaretPosition, save_window, 1, 0, 0, -1, -1);

	while (!done_save)
	{
	    reason = wimp_poll(1, block);
	    switch (reason)
	    {
		case 1:
		    redraw_window(block);
		    break;
		case 2:
		    if (block[0] == save_window)
			swi(Wimp_OpenWindow, 0, block);
		    else
			ro_open_window(block);
		    break;
		case 3:
		    done_save = TRUE;
		    break;
		case 6:
		    if (block[3] != save_window)
			done_save = TRUE;
		    else
		    {
			int drag_box[4];
			int min_x, max_y;

			switch (block[4])
			{
			    case    0: /* Start drag */
				block[0] = save_window;
				swi(Wimp_GetWindowState, 0, block);
				min_x = block[1];
				max_y = block[4];
				block[1] = 0;
				swi(Wimp_GetIconState, 0, block);
				drag_box[0] = block[2] + min_x;
				drag_box[1] = block[3] + max_y;
				drag_box[2] = block[4] + min_x;
				drag_box[3] = block[5] + max_y;

				swi(DragASprite_Start,
					0x45,
					1,
					sprname,
					drag_box);
				dragging_icon = TRUE;
				break;
			    case    2: /* OK */
				retval = wimp_strsave(fname);
				done_save = TRUE;
				break;
			    case    3: /* Cancel */
				done_save = TRUE;
				break;
			}
		    }
		    break;
		case 7:
		    if (dragging_icon)
		    {
			int len = 0;

			dragging_icon = FALSE;
			swi(Wimp_GetPointerInfo, 0, block);
			block[5] = block[3];
			block[6] = block[4];
			block[7] = block[0];
			block[8] = block[1];
			block[9] = 0;		/* Don't know the size */
			block[10] = filetype;

			while (fname[len] > 31)
			{
			    if (fname[len] == '.')
			    {
				fname += len + 1;
				len = 0;
			    }
			    else
				len++;
			}
			if (len > 211)
			    len = 211;

			memcpy(((char_u *) block) + 44, fname, len);
			((char_u *)block)[44 + len] = '\0';

			block[0] = (len + 48) & 0xfc;
			block[3] = 0;
			block[4] = 1;	    /* DataSave */

			swi(Wimp_SendMessage, 17, block, block[5], block[6]);
		    }
		    else
			ro_drag_finished(block);
		    break;
		case 8:
		    if (block[6] == 13)
		    {
			retval = wimp_strsave(fname);
			done_save = TRUE;
		    }
		    else if (block[6] == 0x1b)
			done_save = TRUE;
		    else
			swi(Wimp_ProcessKey, block[6]);
		    break;
		case 17:
		case 18:
		    if (block[4] == 2 && block[9] != -1)
		    {
			/* DataSaveAck from dragging icon. */
			retval = wimp_strsave(((char_u *) block) + 44);
			done_save = TRUE;
		    }
		    else if (block[4] == 0x400c9)
		    {
			/* MenusDeleted */
			done_save = TRUE;
		    }
		    else
			ro_message(block);
		    break;
	    }
	}
	block[0] = save_window;
	swi(Wimp_CloseWindow, 0, block);
	swi(Wimp_GetCaretPosition, 0, block);
	if (block[0] == -1)
	    swi(Wimp_SetCaretPosition, gui.window_handle, -1, 0, 0, -1, -1);

	return retval;
    }
    else if (initdir)
    {
	/* Open a directory viewer */
	length = strlen(initdir);

	if (length > 240)
	    return NULL;	/* Path too long! */

	length = sprintf(command, "Filer_OpenDir %s", initdir);
	while (command[length - 1] == '.')
	    length--;
	command[length] = '\0';
	swi(OS_CLI, command);
    }
    return NULL;
}
