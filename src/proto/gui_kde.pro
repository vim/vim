/* gui_kde.cc */
void gui_mch_add_menu __ARGS((vimmenu_T *menu, int idx));
void gui_mch_add_menu_item __ARGS((vimmenu_T *menu, int idx));
void gui_mch_set_text_area_pos __ARGS((int x, int y, int w, int h));
void gui_gtk_set_mnemonics __ARGS((int enable));
void gui_mch_toggle_tearoffs __ARGS((int enable));
void gui_mch_destroy_menu __ARGS((vimmenu_T *menu));
void gui_mch_set_scrollbar_thumb __ARGS((scrollbar_T *sb, long val, long size, long max));
void gui_mch_set_scrollbar_pos __ARGS((scrollbar_T *sb, int x, int y, int w, int h));
void gui_mch_create_scrollbar __ARGS((scrollbar_T *sb, int orient));
void gui_mch_destroy_scrollbar __ARGS((scrollbar_T *sb));
char_u *gui_mch_browse __ARGS((int saving, char_u *title, char_u *dflt, char_u *ext, char_u *initdir, char_u *filter));
int gui_mch_dialog __ARGS((int type, char_u *title, char_u *message, char_u *buttons, int def_but, char_u *textfield));
void gui_mch_show_popupmenu __ARGS((vimmenu_T *menu));
void gui_mch_find_dialog __ARGS((exarg_T *eap));
void gui_mch_replace_dialog __ARGS((exarg_T *eap));
void ex_helpfind __ARGS((exarg_T *eap));
void gui_make_popup __ARGS((char_u *path_name, int mouse_pos));
/* vim: set ft=cpp : */
