/* auto/if_perl.c */
extern int perl_enabled __ARGS((int verbose));
extern void perl_end __ARGS((void));
extern void msg_split __ARGS((char_u *s, int attr));
extern void perl_win_free __ARGS((win_T *wp));
extern void perl_buf_free __ARGS((buf_T *bp));
extern void ex_perl __ARGS((exarg_T *eap));
extern void ex_perldo __ARGS((exarg_T *eap));
