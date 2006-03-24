/* if_cscope.c */
extern void do_cscope __ARGS((exarg_T *eap));
extern void do_scscope __ARGS((exarg_T *eap));
extern void do_cstag __ARGS((exarg_T *eap));
extern int cs_fgets __ARGS((char_u *buf, int size));
extern void cs_free_tags __ARGS((void));
extern void cs_print_tags __ARGS((void));
extern int cs_connection __ARGS((int num, char_u *dbpath, char_u *ppath));
/* vim: set ft=c : */
