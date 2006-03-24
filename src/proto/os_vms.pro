/* os_vms.c */
extern void mch_settmode __ARGS((int tmode));
extern int mch_get_shellsize __ARGS((void));
extern void mch_set_shellsize __ARGS((void));
extern char_u *mch_getenv __ARGS((char_u *lognam));
extern int mch_setenv __ARGS((char *var, char *value, int x));
extern int vms_sys __ARGS((char *cmd, char *out, char *inp));
extern int vms_sys_status __ARGS((int status));
extern int vms_read __ARGS((char *inbuf, size_t nbytes));
extern int mch_expand_wildcards __ARGS((int num_pat, char_u **pat, int *num_file, char_u ***file, int flags));
extern int mch_expandpath __ARGS((garray_T *gap, char_u *path, int flags));
extern void *vms_fixfilename __ARGS((void *instring));
extern void vms_remove_version __ARGS((void *fname));
/* vim: set ft=c : */
