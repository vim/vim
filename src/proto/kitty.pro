// kitty.c
int kitty_transmit(image_rgb_T *img, int id);
void kitty_place(int id, int placement, int row, int col, int src_x, int src_y, int w, int h, int z);
void kitty_delete(int id, bool del_data);
void kitty_delete_placement(int id, int placement);
int kitty_probe_parse(char *buf, int n);
// vim: ft=c
