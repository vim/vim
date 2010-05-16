/* blowfish.c */
void bf_key_init __ARGS((char_u *password));
void bf_ofb_init __ARGS((char_u *iv, int iv_len));
void bf_ofb_update __ARGS((int c));
int bf_ranbyte __ARGS((void));
int blowfish_self_test __ARGS((void));
/* vim: set ft=c : */
