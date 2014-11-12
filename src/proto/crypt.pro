/* crypt.c */
int crypt_method_nr_from_name __ARGS((char_u *name));
int crypt_method_nr_from_magic __ARGS((char *ptr, int len));
int crypt_works_inplace __ARGS((cryptstate_T *state));
int crypt_get_method_nr __ARGS((buf_T *buf));
int crypt_whole_undofile __ARGS((int method_nr));
int crypt_get_header_len __ARGS((int method_nr));
void crypt_set_cm_option __ARGS((buf_T *buf, int method_nr));
int crypt_self_test __ARGS((void));
cryptstate_T *crypt_create __ARGS((int method_nr, char_u *key, char_u *salt, int salt_len, char_u *seed, int seed_len));
cryptstate_T *crypt_create_from_header __ARGS((int method_nr, char_u *key, char_u *header));
cryptstate_T *crypt_create_from_file __ARGS((FILE *fp, char_u *key));
cryptstate_T *crypt_create_for_writing __ARGS((int method_nr, char_u *key, char_u **header, int *header_len));
void crypt_free_state __ARGS((cryptstate_T *state));
long crypt_encode_alloc __ARGS((cryptstate_T *state, char_u *from, size_t len, char_u **newptr));
long crypt_decode_alloc __ARGS((cryptstate_T *state, char_u *ptr, long len, char_u **newptr));
void crypt_encode __ARGS((cryptstate_T *state, char_u *from, size_t len, char_u *to));
void crypt_decode __ARGS((cryptstate_T *state, char_u *from, size_t len, char_u *to));
void crypt_encode_inplace __ARGS((cryptstate_T *state, char_u *buf, size_t len));
void crypt_decode_inplace __ARGS((cryptstate_T *state, char_u *buf, size_t len));
void crypt_free_key __ARGS((char_u *key));
void crypt_check_method __ARGS((int method));
void crypt_check_current_method __ARGS((void));
char_u *crypt_get_key __ARGS((int store, int twice));
void crypt_append_msg __ARGS((buf_T *buf));
/* vim: set ft=c : */
