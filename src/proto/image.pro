/* image.c */
image_T *image_new(uint8_t *data, imgpx_T width, imgpx_T height, image_format_T fmt);
image_T *image_ref(image_T *img);
void image_unref(image_T *img);
void image_cell_size(image_T *img, colnr_T *cw, linenr_T *ch);
image_placement_T *image_placement_new(image_T *img);
void image_placement_free(image_placement_T *place);
void image_placement_dirty(image_placement_T *place);
void image_placement_clear(image_placement_T *place);
void image_placement_draw(image_placement_T *place);
void image_placement_set_z(image_placement_T *place, int zindex);
void image_placement_set_position(image_placement_T *place, linenr_T row, colnr_T col);
void image_placement_crop(image_placement_T *place, imgpx_T x, imgpx_T y, imgpx_T width, imgpx_T height);
void image_crop_cell_size(image_crop_T *crop, colnr_T *cw, linenr_T *ch);
bool image_crop_equal(image_crop_T *a, image_crop_T *b);
bool image_geometry_equal(image_geometry_T *a, image_geometry_T *b);
image_T *add_image(dict_T *dict);
int update_image_backend(void);
/* vim: set ft=c : */
