/* digraph.c */
int do_digraph(int c);
char_u *get_digraph_for_char(int val_arg);
int get_digraph(int cmdline);
int getdigraph(int char1, int char2, int meta_char);
int check_digraph_chars_valid(int char1, int char2);
void putdigraph(char_u *str);
void listdigraphs(int use_headers);
void getdigraphlist_common(int list_all, typval_T *rettv);
void f_getdigraph(typval_T *argvars, typval_T *rettv);
void f_getdigraphlist(typval_T *argvars, typval_T *rettv);
void f_setdigraph(typval_T *argvars, typval_T *rettv);
void f_setdigraphlist(typval_T *argvars, typval_T *rettv);
char *keymap_init(void);
void ex_loadkeymap(exarg_T *eap);
void keymap_clear(garray_T *kmap);
/* vim: set ft=c : */
