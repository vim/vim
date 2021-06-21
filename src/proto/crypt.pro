/* crypt.c */
int crypt_method_nr_from_name(char_u *name);
int crypt_method_nr_from_magic(char *ptr, int len);
int crypt_works_inplace(cryptstate_T *state);
int crypt_get_method_nr(buf_T *buf);
int crypt_whole_undofile(int method_nr);
int crypt_get_header_len(int method_nr);
int crypt_get_max_header_len(void);
void crypt_set_cm_option(buf_T *buf, int method_nr);
int crypt_self_test(void);
cryptstate_T *crypt_create(int method_nr, char_u *key, char_u *salt, int salt_len, char_u *seed, int seed_len);
cryptstate_T *crypt_create_from_header(int method_nr, char_u *key, char_u *header);
cryptstate_T *crypt_create_from_file(FILE *fp, char_u *key);
cryptstate_T *crypt_create_for_writing(int method_nr, char_u *key, char_u **header, int *header_len);
void crypt_free_state(cryptstate_T *state);
long crypt_encode_alloc(cryptstate_T *state, char_u *from, size_t len, char_u **newptr, int last);
long crypt_decode_alloc(cryptstate_T *state, char_u *ptr, long len, char_u **newptr, int last);
void crypt_encode(cryptstate_T *state, char_u *from, size_t len, char_u *to, int last);
void crypt_encode_inplace(cryptstate_T *state, char_u *buf, size_t len, int last);
void crypt_decode_inplace(cryptstate_T *state, char_u *buf, size_t len, int last);
void crypt_free_key(char_u *key);
void crypt_check_method(int method);
void crypt_check_current_method(void);
char_u *crypt_get_key(int store, int twice);
void crypt_append_msg(buf_T *buf);
int crypt_sodium_init(cryptstate_T *state, char_u *key, char_u *salt, int salt_len, char_u *seed, int seed_len);
long crypt_sodium_buffer_encode(cryptstate_T *state, char_u *from, size_t len, char_u **buf_out, int last);
long crypt_sodium_buffer_decode(cryptstate_T *state, char_u *from, size_t len, char_u **buf_out, int last);
/* vim: set ft=c : */
