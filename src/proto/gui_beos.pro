/* gui_beos.cc - hand crafted */
int vim_lock_screen __ARGS((void));
void vim_unlock_screen __ARGS((void));
void gui_mch_prepare __ARGS((int *argc, char **argv));
int gui_mch_init __ARGS((void));
void gui_mch_new_colors __ARGS((void));
int gui_mch_open __ARGS((void));
void gui_mch_exit __ARGS((int vim_exitcode));
GuiFont gui_mch_get_font __ARGS((char_u *name, int giveErrorIfMissing));
void gui_mch_set_bg_color __ARGS((guicolor_T color));
void gui_mch_set_font __ARGS((GuiFont font));
void gui_mch_flush __ARGS((void));
long_u gui_mch_get_rgb __ARGS((guicolor_T pixel));
void gui_mch_set_winpos __ARGS((int x, int y));
