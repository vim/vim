/* image_kitty.c */
image_T *image_kitty_alloc(void);
void image_kitty_init(image_T *img);
void image_kitty_uninit(image_T *img);
void image_kitty_draw(image_T *img, image_geometry_T *geometry, int id);
void image_kitty_clear(image_T *img, int id);
/* vim: set ft=c : */
