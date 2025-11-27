/* treesitter.c */
int tsvim_init(void);
void tsvim_load_language(char_u *name, char_u *path, char_u *symbol_name);
void tsvim_state_free(TSVimState *state);
void tsvim_parse_buf(buf_T *buf);
bool tsvim_parse_pending(void);
/* vim: set ft=c : */
