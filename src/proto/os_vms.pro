/* os_vms.c */
void mch_settmode __ARGS((int tmode));
int mch_get_shellsize __ARGS((void));
void mch_set_shellsize __ARGS((void));
char_u *mch_getenv __ARGS((char_u *lognam));
int mch_setenv __ARGS((char *var, char *value, int x));
int vms_sys __ARGS((char *cmd, char *out, char *inp));
int vms_sys_status __ARGS((int status));
int vms_read __ARGS((char *inbuf, size_t nbytes));
int mch_expand_wildcards __ARGS((int num_pat, char_u **pat, int *num_file, char_u ***file, int flags));
int mch_expandpath __ARGS((garray_T *gap, char_u *path, int flags));
void *vms_fixfilename __ARGS((void *instring));
void vms_remove_version __ARGS((void *fname));
/* vim: set ft=c : */
