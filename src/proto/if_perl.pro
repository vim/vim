/* auto/if_perl.c */
int perl_enabled(int verbose);
void perl_end(void);
void msg_split(char_u *s, int attr);
char_u *eval_to_string(char_u *arg, int convert, int use_simple_function);
void perl_win_free(win_T *wp);
void perl_buf_free(buf_T *bp);
void ex_perl(exarg_T *eap);
void do_perleval(char_u *str, typval_T *rettv);
void ex_perldo(exarg_T *eap);
/* vim: set ft=c : */
