/* blob.c */
blob_T *blob_alloc(void);
<<<<<<< HEAD
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
char_u blob_get(blob_T *b, int idx);
void blob_set(blob_T *b, int idx, char_u c);
=======
int rettv_blob_alloc(typval_T *rettv);
void rettv_blob_set(typval_T *rettv, blob_T *b);
int blob_copy(typval_T *from, typval_T *to);
void blob_free(blob_T *b);
void blob_unref(blob_T *b);
long blob_len(blob_T *b);
int blob_get(blob_T *b, int idx);
void blob_set(blob_T *b, int idx, char_u c);
int blob_equal(blob_T *b1, blob_T *b2);
int read_blob(FILE *fd, blob_T *blob);
int write_blob(FILE *fd, blob_T *blob);
char_u *blob2string(blob_T *blob, char_u **tofree, char_u *numbuf);
blob_T *string2blob(char_u *str);
>>>>>>> master
/* vim: set ft=c : */
