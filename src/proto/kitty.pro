/* kitty.c */
int kitty_transmit(image_rgb_T *img, int id);
void kitty_place(int id, int row, int col, int src_x, int src_y, int w, int h, int z);
void kitty_delete(int id, bool del_data);
int kitty_probe_parse(char *buf, int n);
/* vim: set ft=c : */
