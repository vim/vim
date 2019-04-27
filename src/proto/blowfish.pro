/* blowfish.c */
void crypt_blowfish_encode(cryptstate_T *state, char_u *from, size_t len, char_u *to);
void crypt_blowfish_decode(cryptstate_T *state, char_u *from, size_t len, char_u *to);
int crypt_blowfish_init(cryptstate_T *state, char_u *key, char_u *salt, int salt_len, char_u *seed, int seed_len);
int blowfish_self_test(void);
/* vim: set ft=c : */
