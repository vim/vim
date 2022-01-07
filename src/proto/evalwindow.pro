/* evalwindow.c */
win_T *win_id2wp(int id);
win_T *win_id2wp_tp(int id, tabpage_T **tpp);
void win_findbuf(typval_T *argvars, list_T *list);
win_T *find_win_by_nr(typval_T *vp, tabpage_T *tp);
win_T *find_win_by_nr_or_id(typval_T *vp);
win_T *find_tabwin(typval_T *wvp, typval_T *tvp, tabpage_T **ptp);
void f_gettabinfo(typval_T *argvars, typval_T *rettv);
void f_getwininfo(typval_T *argvars, typval_T *rettv);
void f_getwinpos(typval_T *argvars, typval_T *rettv);
void f_getwinposx(typval_T *argvars, typval_T *rettv);
void f_getwinposy(typval_T *argvars, typval_T *rettv);
void f_tabpagenr(typval_T *argvars, typval_T *rettv);
void f_tabpagewinnr(typval_T *argvars, typval_T *rettv);
void f_win_execute(typval_T *argvars, typval_T *rettv);
void f_win_findbuf(typval_T *argvars, typval_T *rettv);
void f_win_getid(typval_T *argvars, typval_T *rettv);
void f_win_gotoid(typval_T *argvars, typval_T *rettv);
void f_win_id2tabwin(typval_T *argvars, typval_T *rettv);
void f_win_id2win(typval_T *argvars, typval_T *rettv);
void f_win_move_separator(typval_T *argvars, typval_T *rettv);
void f_win_move_statusline(typval_T *argvars, typval_T *rettv);
void f_win_screenpos(typval_T *argvars, typval_T *rettv);
void f_win_splitmove(typval_T *argvars, typval_T *rettv);
void f_win_gettype(typval_T *argvars, typval_T *rettv);
void f_getcmdwintype(typval_T *argvars, typval_T *rettv);
void f_winbufnr(typval_T *argvars, typval_T *rettv);
void f_wincol(typval_T *argvars, typval_T *rettv);
void f_winheight(typval_T *argvars, typval_T *rettv);
void f_winlayout(typval_T *argvars, typval_T *rettv);
void f_winline(typval_T *argvars, typval_T *rettv);
void f_winnr(typval_T *argvars, typval_T *rettv);
void f_winrestcmd(typval_T *argvars, typval_T *rettv);
void f_winrestview(typval_T *argvars, typval_T *rettv);
void f_winsaveview(typval_T *argvars, typval_T *rettv);
void f_winwidth(typval_T *argvars, typval_T *rettv);
int switch_win(win_T **save_curwin, tabpage_T **save_curtab, win_T *win, tabpage_T *tp, int no_display);
int switch_win_noblock(win_T **save_curwin, tabpage_T **save_curtab, win_T *win, tabpage_T *tp, int no_display);
void restore_win(win_T *save_curwin, tabpage_T *save_curtab, int no_display);
void restore_win_noblock(win_T *save_curwin, tabpage_T *save_curtab, int no_display);
/* vim: set ft=c : */
