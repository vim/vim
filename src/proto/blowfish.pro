/* blowfish.c */
void crypt_blowfish_encode __ARGS((cryptstate_T *state, char_u *from, size_t len, char_u *to));
void crypt_blowfish_decode __ARGS((cryptstate_T *state, char_u *from, size_t len, char_u *to));
void crypt_blowfish_init __ARGS((cryptstate_T *state, char_u *key, char_u *salt, int salt_len, char_u *seed, int seed_len));
int blowfish_self_test __ARGS((void));
/* vim: set ft=c : */
