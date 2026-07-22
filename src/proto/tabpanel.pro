/* tabpanel.c */
int tabpanelopt_changed(void);
void tabpanel_forget_tabpage(const tabpage_T *tp);
int tabpanel_width(void);
int tabpanel_leftcol(void);
void draw_tabpanel(void);
int get_tabpagenr_on_tabpanel(void);
bool mouse_on_tabpanel(void);
bool mouse_on_tabpanel_scrollbar(void);
bool tabpanel_drag_scrollbar(int screen_row);
bool tabpanel_scroll(int dir, int count);
bool tabpanel_set_offset(int offset);
void f_tabpanel_getinfo(typval_T *argvars, typval_T *rettv);
void f_tabpanel_scroll(typval_T *argvars, typval_T *rettv);
/* vim: set ft=c : */
