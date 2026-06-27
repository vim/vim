/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * hardcopy.h: common stuff for hardcopy feature
 */

#ifndef HARDCOPY_H
#define HARDCOPY_H

#include "vim.h"

#if defined(FEAT_PRINTER)

# define PRCOLOR_BLACK	(long_u)0
# define PRCOLOR_WHITE	(long_u)0xFFFFFFL

extern int	prt_curr_italic;
extern int	prt_curr_bold;
extern int	prt_curr_underline;
extern long_u	prt_curr_bg;
extern long_u	prt_curr_fg;
extern int	prt_page_count;

# if defined(FEAT_POSTSCRIPT) || defined(FEAT_PRINT_PANGO)

typedef struct
{
    char	*name;
    double	width; // Width and height in points for portrait
    double	height;
} prt_mediasize_T;

#  define PRT_MEDIASIZE_LEN  14

extern prt_mediasize_T prt_mediasize[PRT_MEDIASIZE_LEN];
# endif

void prt_message(char_u *s);

#endif

#endif
