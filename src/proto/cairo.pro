/* cairo.c */
bool cairo_popup_image_ensure(win_T *wp);
bool cairo_popup_image_update(win_T *wp);
void cairo_popup_image_free(win_T *wp);
void cairo_popup_image_paint(win_T *wp, void *target, int x, int y, int src_x, int src_y, int draw_w, int draw_h);
/* vim: set ft=c : */
