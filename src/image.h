/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read a list of people who contributed.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#ifdef FEAT_IMAGE

#include <pixman.h>

#define PLACEMENT_ID_INC 1000

typedef enum
{
    IMAGE_BACKEND_NONE = -1,
    IMAGE_BACKEND_GUI = 0,
    IMAGE_BACKEND_KITTY = 1,
    IMAGE_BACKEND_SIXEL = 2
} image_backend_T;

/*
 * Enum value is the number of bytes a pixel takes up
 */
typedef enum
{
    IMAGE_FORMAT_RGB = 3,
    IMAGE_FORMAT_RGBA = 4
} image_format_T;

/*
 *
 */
typedef struct image_S image_T;
struct image_S
{
    int id;
    int refcount;

    pixman_image_t  *image;
    image_format_T  fmt;

    void *backend_data;

    image_T *next;
    image_T *prev;
};

/*
 *
 */
typedef struct image_placement_S image_placement_T;
struct image_placement_S
{
    int	    id;
    image_T *img; // May be NULL, if so then only "bounding_box" is relevant (and
		  // the position + zindex).
    bool    hidden; // If image should not be drawn for the next redraw. Reset
		    // when the redraw is done.

    void *backend_data;

    // Note that positioning uses the top left of the final cropped image
    linenr_T	row;
    colnr_T	col;
    int		zindex;
    bool	dirty; // If image positioning/geometry has been modified

    pixman_box32_t crop_box; // In pixels

    // The bounding box represents a region that images (including their
    // bounding boxes) under it with lower zindexes will have their overlapping
    // region not be rendered at all. This is used to render text (e.g. borders)
    // ontop of images.
    pixman_box32_t bounding_box; // In cells

    // Cached region that represents the parts of the image that have been drawn
    // to the screen. Used to check if image should be redrawn at all (if
    // nothing has been changed).
    pixman_region32_t	visible;	// In pixels
    pixman_region32_t	visible_abs;    // In pixels, uses absolute coordinates
					// (only used for composited image
					// backends),
					// TODO scale region when cell size
					// changes.
    bool		visible_init;	// If "visible" is valid

    image_placement_T *next;
    image_placement_T *prev;
};

#endif // FEAT_IMAGE
