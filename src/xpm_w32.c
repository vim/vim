/*
 * Load XPM image.
 *
 * This function is placed in separate file because Xpm headers conflict with
 * Vim ones :(
 *
 * Written by Sergey Khorev.
 * http://iamphet.nm.ru/vim/index.html
 */

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

/* reduced def from Vim.h */
#ifndef __ARGS
# if defined(__STDC__) || defined(__GNUC__) || defined(WIN3264)
#  define __ARGS(x) x
# else
#  define __ARGS(x) ()
# endif
#endif

#include "xpm_w32.h"

/* Engage Windows support in libXpm */
#define FOR_MSW

#include "xpm.h"

/*
 * Tries to load Xpm image from file 'filename'.
 * If fails return -1.
 * success - 0 and image and mask BITMAPS
 */
    int
LoadXpmImage(filename, hImage, hShape)
    char    *filename;
    HBITMAP *hImage;
    HBITMAP *hShape;
{
    XImage	    *img;   /* loaded image */
    XImage	    *shp;  /* shapeimage */
    XpmAttributes   attr;
    int		    res;
    HDC		    hdc = CreateCompatibleDC(NULL);

    attr.valuemask = 0;
    res = XpmReadFileToImage(&hdc, filename, &img, &shp, &attr);
    DeleteDC(hdc);
    if (res < 0)
	return -1;
    else
    {
	*hImage = img->bitmap;
	*hShape = shp->bitmap;
	return 0;
    }
}
