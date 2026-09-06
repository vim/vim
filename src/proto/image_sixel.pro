/* image_sixel.c */
void sixel_uninit(void);
image_T *image_sixel_alloc(void);
void image_sixel_init(image_T *img);
void image_sixel_uninit(image_T *img);
image_placement_T *image_placement_sixel_alloc(void);
void image_placement_sixel_init(image_placement_T *place);
void image_placement_sixel_uninit(image_placement_T *place);
void image_placement_sixel_draw(image_placement_T *place);
void image_placement_sixel_clear(image_placement_T *place);
/* vim: set ft=c : */
