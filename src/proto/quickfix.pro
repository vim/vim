/* quickfix.c */
int qf_init __ARGS((char_u *efile, char_u *errorformat, int newlist));
void qf_jump __ARGS((int dir, int errornr, int forceit));
void qf_list __ARGS((exarg_T *eap));
void qf_age __ARGS((exarg_T *eap));
void qf_mark_adjust __ARGS((linenr_T line1, linenr_T line2, long amount, long amount_after));
void ex_cwindow __ARGS((exarg_T *eap));
void ex_cclose __ARGS((exarg_T *eap));
void ex_copen __ARGS((exarg_T *eap));
linenr_T qf_current_entry __ARGS((void));
int bt_quickfix __ARGS((buf_T *buf));
int bt_nofile __ARGS((buf_T *buf));
int bt_dontwrite __ARGS((buf_T *buf));
int bt_dontwrite_msg __ARGS((buf_T *buf));
int buf_hide __ARGS((buf_T *buf));
void ex_make __ARGS((exarg_T *eap));
void ex_cc __ARGS((exarg_T *eap));
void ex_cnext __ARGS((exarg_T *eap));
void ex_cfile __ARGS((exarg_T *eap));
void ex_helpgrep __ARGS((exarg_T *eap));
/* vim: set ft=c : */
