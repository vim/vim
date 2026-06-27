/* cairo.c */
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
void mch_print_cleanup(void);
/* vim: set ft=c : */
