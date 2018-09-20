/* blob.c */
blob_T *blob_alloc(void);
long blob_len(blob_T *b);
int rettv_blob_alloc(typval_T *rettv);
void rettv_blob_set(typval_T *rettv, blob_T *l);
void blob_unref(blob_T *l);
int blob_free_nonref(int copyID);
void blob_free(blob_T *l);
blob_T *blob_copy(blob_T *orig, int deep, int copyID);
char_u *blob2string(typval_T *tv, int copyID, int restore_copyID);
int get_blob_tv(char_u **arg, typval_T *rettv, int evaluate);
int write_blob(FILE *fd, blob_T *blob);
int read_blob(FILE *fd, blob_T *blob);
int blob_equal(blob_T *b1, blob_T *b2);
/* vim: set ft=c : */
