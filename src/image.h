/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read a list of people who contributed.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#ifdef FEAT_IMAGE

// Used for image dimensions
typedef unsigned int imgpx_T;

/*
 * Cairo - GTK3/2 GUI
 * GDI - MS-Windows GUI
 * GDK - GTK4 GUI
 * Kitty - Kitty graphics protocol (terminal)
 * Sixel - Six pixels (terminal)
 */
typedef enum
{
    IMAGE_BACKEND_NONE,
    IMAGE_BACKEND_CAIRO,
    IMAGE_BACKEND_GDI,
    IMAGE_BACKEND_GDK,
    IMAGE_BACKEND_KITTY,
    IMAGE_BACKEND_SIXEL
} image_backend_T;

/*
 * Enums represent the amount of bytes a single pixel takes up
 */
typedef enum
{
    IMAGE_FORMAT_RGB = 3ULL,
    IMAGE_FORMAT_RGBA = 4ULL
} image_format_T;

/*
 * Base class of images, which backends subclass.
 */
typedef struct image_S image_T;
struct image_S
{
    int id; /// Unique ID used for this image
    int refcount;

    uint8_t	    *data;
    imgpx_T	    width;
    imgpx_T	    height;
    image_format_T  fmt;

    image_T *next;
    image_T *prev;
};

/*
 * Represents positioning and cropping of image
 */
typedef struct
{
    // Note that the position uses the final cropped image, not the top left of
    // the original image.
    linenr_T	row;
    colnr_T	col;
    int		zindex;

    struct
    {
	imgpx_T x;
	imgpx_T y;
	imgpx_T width;
	imgpx_T height;
    } crop;
} image_geometry_T;

/*
 * Represents a placement of an image, that is rendered onto the screen. An
 * image can have multiple placements, each showing a different part of the
 * image (possibly) at different positions.
 */
typedef struct image_placement_S image_placement_T;
struct image_placement_S
{
    int	    id;
    image_T *img;

    // Should not be modified directly, do it via the image_placement_*
    // methods.
    image_geometry_T geometry;
    bool dirty; // If placement should be redrawn

    image_placement_T *next;
    image_placement_T *prev;
};

#endif // FEAT_IMAGE
