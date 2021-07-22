/* colornames.c */
guicolor_T decode_hex_color(char_u *hex);
guicolor_T colorname2rgb(char_u *name);
void save_colorname_hexstr(int r, int g, int b, char_u *name);
void load_rgb_txt(void);
guicolor_T gui_get_color_cmn(char_u *name);
guicolor_T gui_get_rgb_color_cmn(int r, int g, int b);
/* vim: set ft=c : */
