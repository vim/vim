/* image_kitty.c */
image_T *image_kitty_alloc(void);
void image_kitty_init(image_T *img);
void image_kitty_uninit(image_T *img);
image_placement_T *image_placement_kitty_alloc(void);
void image_placement_kitty_init(image_placement_T *place UNUSED);
void image_placement_kitty_uninit(image_placement_T *place);
void image_placement_kitty_draw(image_placement_T *place);
void image_placement_kitty_clear(image_placement_T *place);
/* vim: set ft=c : */
