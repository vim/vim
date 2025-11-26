/* treesitter.c */
int init_treesitter(void);
void vts_load_language(char_u *name, char_u *path, char_u *symbol_name);
vts_parser_T *vts_parser_new(char_u *language);
void vts_parser_free(vts_parser_T *self);
/* vim: set ft=c : */
