/* sha256.c */
void sha256_start __ARGS((context_sha256_T *ctx));
void sha256_update __ARGS((context_sha256_T *ctx, char_u *input, uint32_t length));
void sha256_finish __ARGS((context_sha256_T *ctx, char_u digest[32]));
char_u *sha256_key __ARGS((char_u *buf));
int sha256_self_test __ARGS((void));
void sha2_seed __ARGS((char_u header[], int header_len));
/* vim: set ft=c : */
