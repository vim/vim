/* clipboard.c */
void clip_init(int can_use);
void clip_update_selection(Clipboard_T *clip);
void clip_own_selection(Clipboard_T *cbd);
void clip_lose_selection(Clipboard_T *cbd);
void start_global_changes(void);
void end_global_changes(void);
void clip_auto_select(void);
int clip_isautosel_star(void);
int clip_isautosel_plus(void);
void clip_modeless(int button, int is_click, int is_drag);
void clip_start_selection(int col, int row, int repeated_click);
void clip_process_selection(int button, int col, int row, int_u repeated_click);
void clip_may_redraw_selection(int row, int col, int len);
void clip_clear_selection(Clipboard_T *cbd);
void clip_may_clear_selection(int row1, int row2);
void clip_scroll_selection(int rows);
void clip_copy_modeless_selection(int both);
void clip_gen_set_selection(Clipboard_T *cbd);
int clip_gen_owner_exists(Clipboard_T *cbd);
char *did_set_clipboard(optset_T *args);
/* vim: set ft=c : */
