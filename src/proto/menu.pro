/* menu.c */
extern void ex_menu __ARGS((exarg_T *eap));
extern char_u *set_context_in_menu_cmd __ARGS((expand_T *xp, char_u *cmd, char_u *arg, int forceit));
extern char_u *get_menu_name __ARGS((expand_T *xp, int idx));
extern char_u *get_menu_names __ARGS((expand_T *xp, int idx));
extern char_u *menu_name_skip __ARGS((char_u *name));
extern int get_menu_index __ARGS((vimmenu_T *menu, int state));
extern int menu_is_menubar __ARGS((char_u *name));
extern int menu_is_popup __ARGS((char_u *name));
extern int menu_is_child_of_popup __ARGS((vimmenu_T *menu));
extern int menu_is_toolbar __ARGS((char_u *name));
extern int menu_is_separator __ARGS((char_u *name));
extern void gui_create_initial_menus __ARGS((vimmenu_T *menu));
extern void gui_update_menus __ARGS((int modes));
extern int gui_is_menu_shortcut __ARGS((int key));
extern void gui_show_popupmenu __ARGS((void));
extern void gui_mch_toggle_tearoffs __ARGS((int enable));
extern void ex_emenu __ARGS((exarg_T *eap));
extern vimmenu_T *gui_find_menu __ARGS((char_u *path_name));
extern void ex_menutranslate __ARGS((exarg_T *eap));
/* vim: set ft=c : */
