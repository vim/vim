/* cairo.c */
bool cairo_popup_image_ensure(win_T *wp);
bool cairo_popup_image_update(win_T *wp);
void cairo_popup_image_free(win_T *wp);
void cairo_popup_image_paint(win_T *wp, void *target, int x, int y, double src_x, double src_y, double draw_w, double draw_h);
/* vim: set ft=c : */
