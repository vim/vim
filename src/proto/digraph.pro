/* digraph.c */
int do_digraph(int c);
char_u *get_digraph_for_char(int val_arg);
int get_digraph(int cmdline);
int getdigraph(int char1, int char2, int meta_char);
void putdigraph(char_u *str);
void listdigraphs(void);
char_u *keymap_init(void);
void ex_loadkeymap(exarg_T *eap);
void keymap_clear(garray_T *kmap);
/* vim: set ft=c : */
