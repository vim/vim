/* popupmenu.c */
void pum_display __ARGS((pumitem_T *array, int size, int selected, int row, int height, int col));
void pum_redraw __ARGS((void));
void pum_set_selected __ARGS((int n));
void pum_undisplay __ARGS((void));
void pum_clear __ARGS((void));
int pum_visible __ARGS((void));
int pum_get_height __ARGS((void));
/* vim: set ft=c : */
