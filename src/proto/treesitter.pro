/* treesitter.c */
int init_treesitter(void);
void ts_vim_load_language(char_u *name, char_u *path, char_u *symbol_name);
void ts_vim_parser_free(ts_vim_parser_T *vparser);
void ts_vim_parse_buf(buf_T *buf);
/* vim: set ft=c : */
