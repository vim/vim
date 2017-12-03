/* term.c */
guicolor_T termgui_mch_get_color(char_u *name);
guicolor_T termgui_get_color(char_u *name);
guicolor_T termgui_mch_get_rgb(guicolor_T color);
int set_termname(char_u *term);
void set_mouse_termcode(int n, char_u *s);
void del_mouse_termcode(int n);
void getlinecol(long *cp, long *rp);
int add_termcap_entry(char_u *name, int force);
int term_is_8bit(char_u *name);
int term_is_gui(char_u *name);
char_u *tltoa(unsigned long i);
void termcapinit(char_u *name);
void out_flush(void);
void out_flush_check(void);
void out_trash(void);
void out_char(unsigned c);
void out_str_nf(char_u *s);
void out_str_cf(char_u *s);
void out_str(char_u *s);
void term_windgoto(int row, int col);
void term_cursor_right(int i);
void term_append_lines(int line_count);
void term_delete_lines(int line_count);
void term_set_winpos(int x, int y);
int term_get_winpos(int *x, int *y);
void term_set_winsize(int height, int width);
void term_fg_color(int n);
void term_bg_color(int n);
void term_fg_rgb_color(guicolor_T rgb);
void term_bg_rgb_color(guicolor_T rgb);
void term_ul_rgb_color(guicolor_T rgb);
void term_settitle(char_u *title);
void ttest(int pairs);
void add_long_to_buf(long_u val, char_u *dst);
void check_shellsize(void);
void limit_screen_size(void);
void win_new_shellsize(void);
void shell_resized(void);
void shell_resized_check(void);
void set_shellsize(int width, int height, int mustset);
void settmode(int tmode);
void starttermcap(void);
void stoptermcap(void);
void may_req_termresponse(void);
void may_req_ambiguous_char_width(void);
void may_req_bg_color(void);
int swapping_screen(void);
void setmouse(void);
int mouse_has(int c);
int mouse_model_popup(void);
void scroll_start(void);
void cursor_on(void);
void cursor_off(void);
void term_cursor_mode(int forced);
void term_cursor_color(char_u *color);
int blink_state_is_inverted(void);
void term_cursor_shape(int shape, int blink);
void scroll_region_set(win_T *wp, int off);
void scroll_region_reset(void);
void clear_termcodes(void);
void add_termcode(char_u *name, char_u *string, int flags);
char_u *find_termcode(char_u *name);
char_u *get_termcode(int i);
void del_termcode(char_u *name);
void set_mouse_topline(win_T *wp);
int check_termcode(int max_offset, char_u *buf, int bufsize, int *buflen);
void term_get_fg_color(char_u *r, char_u *g, char_u *b);
void term_get_bg_color(char_u *r, char_u *g, char_u *b);
char_u *replace_termcodes(char_u *from, char_u **bufp, int from_part, int do_lt, int special);
int find_term_bykeys(char_u *src);
void show_termcodes(void);
int show_one_termcode(char_u *name, char_u *code, int printit);
char_u *translate_mapping(char_u *str, int expmap);
void update_tcap(int attr);
guicolor_T gui_get_color_cmn(char_u *name);
guicolor_T gui_get_rgb_color_cmn(int r, int g, int b);
/* vim: set ft=c : */
