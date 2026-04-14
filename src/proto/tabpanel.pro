/* tabpanel.c */
int tabpanelopt_changed(void);
int tabpanel_width(void);
int tabpanel_leftcol(void);
void draw_tabpanel(void);
int get_tabpagenr_on_tabpanel(void);
int mouse_on_tabpanel(void);
int mouse_on_tabpanel_scrollbar(void);
int tabpanel_drag_scrollbar(int screen_row);
int tabpanel_scroll(int dir, int count);
/* vim: set ft=c : */
