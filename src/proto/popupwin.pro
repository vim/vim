/* popupwin.c */
void f_popup_create(typval_T *argvars, typval_T *rettv);
void f_popup_close(typval_T *argvars, typval_T *rettv);
void popup_close(int nr);
void close_all_popups(void);
void ex_popupclear(exarg_T *eap);
/* vim: set ft=c : */
