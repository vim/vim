/* crypt_zip.c */
int crypt_zip_init(cryptstate_T *state, char_u *key, char_u *salt, int salt_len, char_u *seed, int seed_len);
void crypt_zip_encode(cryptstate_T *state, char_u *from, size_t len, char_u *to);
void crypt_zip_decode(cryptstate_T *state, char_u *from, size_t len, char_u *to);
/* vim: set ft=c : */
