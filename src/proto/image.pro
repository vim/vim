/* image.c */
image_T *image_new(uint8_t *data, int width, int height, image_format_T fmt);
void image_unref(image_T *img);
image_T *image_ref(image_T *img);
void pixel2cells(int x, int y, int *rx, int *ry);
void image_get_cell_dimensions(image_T *img, int *cw, int *ch);
void image_get_dimensions(image_T *img, int *w, int *h);
image_placement_T *image_placement_new(image_T *img);
void image_placement_free(image_placement_T *place);
void image_placement_hide(image_placement_T *place, bool state);
void image_placement_set_zindex(image_placement_T *place, int zindex);
void image_placement_set_position(image_placement_T *place, int row, int col);
void image_placement_set_crop(image_placement_T *place, int x, int y, int w, int h);
void image_placement_set_bounding_box(image_placement_T *place, int row, int col, int row_height, int col_width);
void draw_image_placements(void);
image_T *add_image(dict_T *dict);
/* vim: set ft=c : */
