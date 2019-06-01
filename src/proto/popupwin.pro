/* popupwin.c */
void popup_adjust_position(win_T *wp);
void f_popup_create(typval_T *argvars, typval_T *rettv);
void f_popup_atcursor(typval_T *argvars, typval_T *rettv);
int popup_any_visible(void);
void f_popup_close(typval_T *argvars, typval_T *rettv);
void f_popup_hide(typval_T *argvars, typval_T *rettv);
void f_popup_show(typval_T *argvars, typval_T *rettv);
void popup_close(int id);
void popup_close_tabpage(tabpage_T *tp, int id);
void close_all_popups(void);
void ex_popupclear(exarg_T *eap);
void f_popup_move(typval_T *argvars, typval_T *rettv);
void f_popup_getpos(typval_T *argvars, typval_T *rettv);
void f_popup_getoptions(typval_T *argvars, typval_T *rettv);
int not_in_popup_window(void);
void popup_reset_handled(void);
win_T *find_next_popup(int lowest);
int popup_do_filter(int c);
/* vim: set ft=c : */
