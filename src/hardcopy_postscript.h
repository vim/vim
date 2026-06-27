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

#define OPT_MBFONT_USECOURIER  0
#define OPT_MBFONT_ASCII       1
#define OPT_MBFONT_REGULAR     2
#define OPT_MBFONT_BOLD	3
#define OPT_MBFONT_OBLIQUE     4
#define OPT_MBFONT_BOLDOBLIQUE 5
#define OPT_MBFONT_NUM_OPTIONS 6

extern option_table_T mbfont_opts[OPT_MBFONT_NUM_OPTIONS];

void mch_print_cleanup(void);
int mch_print_init(prt_settings_T *psettings, char_u *jobname, int forceit);
int mch_print_begin(prt_settings_T *psettings);
void mch_print_end(prt_settings_T *psettings);
int mch_print_end_page(void);
int mch_print_begin_page(char_u *str);
int mch_print_blank_page(void);
void mch_print_start_line(int margin, int page_line);
int mch_print_text_out(char_u *textp, int len);
void mch_print_set_font(int iBold, int iItalic, int iUnderline);
void mch_print_set_bg(long_u bgcol);
void mch_print_set_fg(long_u fgcol);

#endif

#endif
