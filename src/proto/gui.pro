/* gui.c */
void gui_start(void);
void gui_prepare(int *argc, char **argv);
int gui_init_check(void);
void gui_init(void);
void gui_exit(int rc);
void gui_shell_closed(void);
int gui_init_font(char_u *font_list, int fontset);
int gui_get_wide_font(void);
void gui_set_cursor(int row, int col);
void gui_update_cursor(int force, int clear_selection);
void gui_position_menu(void);
int gui_get_base_width(void);
int gui_get_base_height(void);
void gui_resize_shell(int pixel_width, int pixel_height);
void gui_may_resize_shell(void);
int gui_get_shellsize(void);
void gui_set_shellsize(int mustset, int fit_to_display, int direction);
void gui_new_shellsize(void);
void gui_reset_scroll_region(void);
void gui_start_highlight(int mask);
void gui_stop_highlight(int mask);
void gui_clear_block(int row1, int col1, int row2, int col2);
void gui_update_cursor_later(void);
void gui_write(char_u *s, int len);
void gui_dont_update_cursor(int undraw);
void gui_can_update_cursor(void);
void gui_disable_flush(void);
void gui_enable_flush(void);
void gui_may_flush(void);
int gui_outstr_nowrap(char_u *s, int len, int flags, guicolor_T fg, guicolor_T bg, int back);
void gui_undraw_cursor(void);
void gui_redraw(int x, int y, int w, int h);
int gui_redraw_block(int row1, int col1, int row2, int col2, int flags);
int gui_wait_for_chars(long wtime);
void gui_send_mouse_event(int button, int x, int y, int repeated_click, int_u modifiers);
int gui_xy2colrow(int x, int y, int *colp);
void gui_menu_cb(vimmenu_T *menu);
void gui_init_which_components(char_u *oldval);
int gui_use_tabline(void);
void gui_update_tabline(void);
void get_tabline_label(tabpage_T *tp, int tooltip);
int send_tabline_event(int nr);
void send_tabline_menu_event(int tabidx, int event);
void gui_remove_scrollbars(void);
void gui_create_scrollbar(scrollbar_T *sb, int type, win_T *wp);
scrollbar_T *gui_find_scrollbar(long ident);
void gui_drag_scrollbar(scrollbar_T *sb, long value, int still_dragging);
void gui_may_update_scrollbars(void);
void gui_update_scrollbars(int force);
int gui_do_scroll(void);
int gui_do_horiz_scroll(long_u leftcol, int compute_longest_lnum);
void gui_check_colors(void);
guicolor_T gui_get_color(char_u *name);
int gui_get_lightness(guicolor_T pixel);
void gui_new_scrollbar_colors(void);
void gui_focus_change(int in_focus);
void gui_mouse_moved(int x, int y);
void gui_mouse_correct(void);
void ex_gui(exarg_T *eap);
int gui_find_bitmap(char_u *name, char_u *buffer, char *ext);
void gui_find_iconfile(char_u *name, char_u *buffer, char *ext);
void display_errors(void);
int no_console_input(void);
void gui_update_screen(void);
char_u *get_find_dialog_text(char_u *arg, int *wwordp, int *mcasep);
int gui_do_findrepl(int flags, char_u *find_text, char_u *repl_text, int down);
void gui_handle_drop(int x, int y, int_u modifiers, char_u **fnames, int count);
/* vim: set ft=c : */
