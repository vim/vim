/* auto/if_perl.c */
int perl_enabled __ARGS((int verbose));
void perl_end __ARGS((void));
void msg_split __ARGS((char_u *s, int attr));
void perl_win_free __ARGS((win_T *wp));
void perl_buf_free __ARGS((buf_T *bp));
void ex_perl __ARGS((exarg_T *eap));
void ex_perldo __ARGS((exarg_T *eap));
