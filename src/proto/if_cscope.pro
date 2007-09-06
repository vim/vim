/* if_cscope.c */
void do_cscope __ARGS((exarg_T *eap));
void do_scscope __ARGS((exarg_T *eap));
void do_cstag __ARGS((exarg_T *eap));
int cs_fgets __ARGS((char_u *buf, int size));
void cs_free_tags __ARGS((void));
void cs_print_tags __ARGS((void));
int cs_connection __ARGS((int num, char_u *dbpath, char_u *ppath));
void cs_end __ARGS((void));
/* vim: set ft=c : */
