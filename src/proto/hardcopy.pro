/* hardcopy.c */
extern char_u *parse_printoptions __ARGS((void));
extern char_u *parse_printmbfont __ARGS((void));
extern int prt_header_height __ARGS((void));
extern int prt_use_number __ARGS((void));
extern int prt_get_unit __ARGS((int idx));
extern void ex_hardcopy __ARGS((exarg_T *eap));
extern void mch_print_cleanup __ARGS((void));
extern int mch_print_init __ARGS((prt_settings_T *psettings, char_u *jobname, int forceit));
extern int mch_print_begin __ARGS((prt_settings_T *psettings));
extern void mch_print_end __ARGS((prt_settings_T *psettings));
extern int mch_print_end_page __ARGS((void));
extern int mch_print_begin_page __ARGS((char_u *str));
extern int mch_print_blank_page __ARGS((void));
extern void mch_print_start_line __ARGS((int margin, int page_line));
extern int mch_print_text_out __ARGS((char_u *p, int len));
extern void mch_print_set_font __ARGS((int iBold, int iItalic, int iUnderline));
extern void mch_print_set_bg __ARGS((long_u bgcol));
extern void mch_print_set_fg __ARGS((long_u fgcol));
/* vim: set ft=c : */
