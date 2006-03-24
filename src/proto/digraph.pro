/* digraph.c */
extern int do_digraph __ARGS((int c));
extern int get_digraph __ARGS((int cmdline));
extern int getdigraph __ARGS((int char1, int char2, int meta));
extern void putdigraph __ARGS((char_u *str));
extern void listdigraphs __ARGS((void));
extern char_u *keymap_init __ARGS((void));
extern void ex_loadkeymap __ARGS((exarg_T *eap));
/* vim: set ft=c : */
