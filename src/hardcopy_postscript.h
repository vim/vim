/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#ifndef HARDCOPY_POSTSCRIPT_H
#define HARDCOPY_POSTSCRIPT_H

#include "vim.h"

#if defined(FEAT_POSTSCRIPT)

# define OPT_MBFONT_USECOURIER  0
# define OPT_MBFONT_ASCII       1
# define OPT_MBFONT_REGULAR     2
# define OPT_MBFONT_BOLD	3
# define OPT_MBFONT_OBLIQUE     4
# define OPT_MBFONT_BOLDOBLIQUE 5
# define OPT_MBFONT_NUM_OPTIONS 6

extern option_table_T mbfont_opts[OPT_MBFONT_NUM_OPTIONS];

#endif

#endif
