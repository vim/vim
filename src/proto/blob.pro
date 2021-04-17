/* blob.c */
blob_T *blob_alloc(void);
int rettv_blob_alloc(typval_T *rettv);
void rettv_blob_set(typval_T *rettv, blob_T *b);
int blob_copy(blob_T *from, typval_T *to);
void blob_free(blob_T *b);
void blob_unref(blob_T *b);
long blob_len(blob_T *b);
int blob_get(blob_T *b, int idx);
void blob_set(blob_T *blob, int idx, int byte);
void blob_set_append(blob_T *blob, int idx, int byte);
int blob_equal(blob_T *b1, blob_T *b2);
int read_blob(FILE *fd, blob_T *blob);
int write_blob(FILE *fd, blob_T *blob);
char_u *blob2string(blob_T *blob, char_u **tofree, char_u *numbuf);
blob_T *string2blob(char_u *str);
int blob_slice_or_index(blob_T *blob, int is_range, varnumber_T n1, varnumber_T n2, int exclusive, typval_T *rettv);
int check_blob_index(long bloblen, varnumber_T n1, int quiet);
int check_blob_range(long bloblen, varnumber_T n1, varnumber_T n2, int quiet);
int blob_set_range(blob_T *dest, long n1, long n2, typval_T *src);
void blob_remove(typval_T *argvars, typval_T *rettv);
/* vim: set ft=c : */
