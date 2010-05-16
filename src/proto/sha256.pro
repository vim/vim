/* sha256.c */
char_u *sha256_key __ARGS((char_u *buf));
int sha256_self_test __ARGS((void));
void sha2_seed __ARGS((char_u header[], int header_len));
/* vim: set ft=c : */
