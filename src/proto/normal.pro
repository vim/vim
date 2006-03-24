/* normal.c */
extern void init_normal_cmds __ARGS((void));
extern void normal_cmd __ARGS((oparg_T *oap, int toplevel));
extern void do_pending_operator __ARGS((cmdarg_T *cap, int old_col, int gui_yank));
extern int do_mouse __ARGS((oparg_T *oap, int c, int dir, long count, int fixindent));
extern void check_visual_highlight __ARGS((void));
extern void end_visual_mode __ARGS((void));
extern void reset_VIsual_and_resel __ARGS((void));
extern void reset_VIsual __ARGS((void));
extern int find_ident_under_cursor __ARGS((char_u **string, int find_type));
extern int find_ident_at_pos __ARGS((win_T *wp, linenr_T lnum, colnr_T startcol, char_u **string, int find_type));
extern void clear_showcmd __ARGS((void));
extern int add_to_showcmd __ARGS((int c));
extern void add_to_showcmd_c __ARGS((int c));
extern void push_showcmd __ARGS((void));
extern void pop_showcmd __ARGS((void));
extern void do_check_scrollbind __ARGS((int check));
extern void check_scrollbind __ARGS((linenr_T topline_diff, long leftcol_diff));
extern int find_decl __ARGS((char_u *ptr, int len, int locally, int thisblock, int searchflags));
extern void scroll_redraw __ARGS((int up, long count));
extern void handle_tabmenu __ARGS((void));
extern void do_nv_ident __ARGS((int c1, int c2));
extern int get_visual_text __ARGS((cmdarg_T *cap, char_u **pp, int *lenp));
extern void start_selection __ARGS((void));
extern void may_start_select __ARGS((int c));
/* vim: set ft=c : */
