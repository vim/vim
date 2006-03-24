/* gui_gtk.c */
extern void gui_gtk_register_stock_icons __ARGS((void));
extern void gui_mch_add_menu __ARGS((vimmenu_T *menu, int idx));
extern void gui_mch_add_menu_item __ARGS((vimmenu_T *menu, int idx));
extern void gui_mch_set_text_area_pos __ARGS((int x, int y, int w, int h));
extern void gui_gtk_set_mnemonics __ARGS((int enable));
extern void gui_mch_toggle_tearoffs __ARGS((int enable));
extern void gui_mch_menu_set_tip __ARGS((vimmenu_T *menu));
extern void gui_mch_destroy_menu __ARGS((vimmenu_T *menu));
extern void gui_mch_set_scrollbar_thumb __ARGS((scrollbar_T *sb, long val, long size, long max));
extern void gui_mch_set_scrollbar_pos __ARGS((scrollbar_T *sb, int x, int y, int w, int h));
extern void gui_mch_create_scrollbar __ARGS((scrollbar_T *sb, int orient));
extern void gui_mch_destroy_scrollbar __ARGS((scrollbar_T *sb));
extern char_u *gui_mch_browse __ARGS((int saving, char_u *title, char_u *dflt, char_u *ext, char_u *initdir, char_u *filter));
extern char_u *gui_mch_browsedir __ARGS((char_u *title, char_u *initdir));
extern int gui_mch_dialog __ARGS((int type, char_u *title, char_u *message, char_u *buttons, int def_but, char_u *textfield));
extern void gui_mch_show_popupmenu __ARGS((vimmenu_T *menu));
extern void gui_make_popup __ARGS((char_u *path_name, int mouse_pos));
extern void gui_mch_find_dialog __ARGS((exarg_T *eap));
extern void gui_mch_replace_dialog __ARGS((exarg_T *eap));
extern void gui_gtk_synch_fonts __ARGS((void));
extern void ex_helpfind __ARGS((exarg_T *eap));
/* vim: set ft=c : */
