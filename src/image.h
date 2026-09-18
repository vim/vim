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

typedef enum
{
    IMAGE_STATE_PUBLIC,
    IMAGE_STATE_PRIVATE
} image_state_T;

/*
 *
 */
typedef struct image_S image_T;
struct image_S
{
    int id;
    int refcount;

    // If image state is public, then it can be modified using the builtin
    // functions.
    image_state_T state;

    uint8_t	    *data; // If NULL, then image is invalid and should not be
			   // used.
    int		    width;  // In physical pixels
    int		    height; // In physical pixels
    image_format_T  fmt;

    // Incremented every time the image data changes.
    int_u	    ver;

    image_backend_T backend;
    void	    *backend_data;

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

    image_backend_T backend;
    void	    *backend_data;

    // Note that positioning uses the top left of the final cropped image
    linenr_T	row;
    colnr_T	col;
    int		zindex;
    bool	dirty;	// If image positioning/geometry has been modified
    bool	hidden;	// If image should not be drawn

    pixman_box32_t crop_box; // In cells

    // The bounding box represents a region that images (including their
    // bounding boxes) under it with lower zindexes will have their overlapping
    // region not be rendered at all. This is used to render text (e.g. borders)
    // ontop of images.
    pixman_box32_t bounding_box; // In cells

    // Cached region that represents the parts of the image that have been drawn
    // to the screen. Used to check if image should be redrawn at all (if
    // nothing has been changed).
    pixman_region32_t	visible;	// In cells
    pixman_region32_t	visible_abs;    // In cells, uses absolute coordinates
					// (only used for blit image backends),
    bool		visible_init;	// If "visible" is valid

    // Current image version, if it is different from the image, then must
    // redraw the image.
    int_u img_ver;

    // Cell dimensions used to draw this placement
    int cell_width;
    int cell_height;

    int row_off;

    image_placement_T *next;
    image_placement_T *prev;
};

#else

// Dummy structs for .pro files when GUI Vim is compiled without image support
typedef struct
{
    int dummy;
} image_T;

typedef struct
{
    int dummy;
} image_placement_T;

#endif // FEAT_IMAGE
