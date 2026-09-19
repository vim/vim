/* image.c */
void init_image_state(void);
void uninit_image_state(void);
image_T *image_new(uint8_t *data, int width, int height, image_format_T fmt);
void image_unref(image_T *img);
image_T *image_ref(image_T *img);
void image_get_cell_dimensions(image_T *img, int *cw, int *ch);
void image_get_dimensions(image_T *img, int *w, int *h);
int image_update(image_T *img, uint8_t *data);
void image_placement_clear(image_placement_T *place);
image_placement_T *image_placement_new(image_T *img, int_u gen, image_placement_use_T use, bool quiet);
void image_placement_free(image_placement_T *place);
void image_placement_set_zindex(image_placement_T *place, int zindex);
void image_placement_set_position(image_placement_T *place, int row, int col);
void image_placement_set_crop(image_placement_T *place, int x, int y, int w, int h);
void image_placement_set_bounding_box(image_placement_T *place, int row, int col, int row_height, int col_width);
void image_placement_set_enable(image_placement_T *place, bool enable);
void image_placement_subrect(image_placement_T *place, pixman_box32_t rect, int *row, int *col, int *x, int *y, int *w, int *h, bool physical);
void draw_image_placements(void);
void mark_dirty_region_for_images(int row, int col, int row_height, int col_width);
void clear_all_image_placements(void);
void shift_image_placements(int top, int bot, int amount);
image_T *add_image(dict_T *dict, image_T *existing, bool find);
dict_T *get_image_info(image_T *img);
int update_image_backend(void);
void f_image_add(typval_T *argvars, typval_T *rettv);
void f_image_discard(typval_T *argvars, typval_T *rettv);
void f_image_info(typval_T *argvars, typval_T *rettv);
/* vim: set ft=c : */
