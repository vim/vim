/* os_mac.c */
void mch_setmouse __ARGS((int on));

void mch_windexit __ARGS((int r));
int mch_check_win __ARGS((int argc, char **argv));
int mch_input_isatty __ARGS((void));

void fname_case __ARGS((char_u *name, int len));
void mch_early_init __ARGS((void));
void mch_exit __ARGS((int r));
void mch_settitle __ARGS((char_u *title, char_u *icon));
void mch_restore_title __ARGS((int which));

int mch_get_user_name __ARGS((char_u *s, int len));
void mch_get_host_name __ARGS((char_u *s, int len));
long mch_get_pid __ARGS((void));

int mch_dirname __ARGS((char_u *buf, int len));
int mch_FullName __ARGS((char_u *fname, char_u *buf, int len, int force));
int mch_isFullName __ARGS((char_u *fname));
void slash_adjust __ARGS((char_u *p));
long mch_getperm __ARGS((char_u *name));
int mch_setperm __ARGS((char_u *name, long perm));
void mch_hide __ARGS((char_u *name));
int mch_isdir __ARGS((char_u *name));
int mch_can_exe __ARGS((char_u *name));
int mch_nodetype __ARGS((char_u *name));
void mch_init __ARGS((void));
void mch_settmode __ARGS((int raw));

int mch_chdir __ARGS((char *p_name));
#if defined(__MRC__) || defined(__SC__)
int stat __ARGS((char *p, struct stat *p_st));
#endif

int mch_call_shell __ARGS((char_u *cmd, int options));
int mch_has_wildcard __ARGS((char_u *s));
int mch_expandpath __ARGS((struct growarray *gap, char_u *path, int flags));
int mac_expandpath __ARGS((struct growarray *gap, char_u *path, int flags, short start_at, short as_full));
/*int vim_chdir __ARGS((char *path));*/
void mch_delay __ARGS((long msec, int ignoreinput));
void mch_breakcheck __ARGS((void));
long_u mch_avail_mem __ARGS((int special));
int mch_screenmode __ARGS((char_u *arg));
int mch_has_exp_wildcard __ARGS((char_u *p));

void slash_n_colon_adjust __ARGS((char_u *buf));
int mch_copy_file(char_u *from, char_u *to);

int mch_has_resource_fork (char_u *file);
int mch_copy_file_attribute(char_u *from, char_u *to);

void mch_shellinit __ARGS((void));
int mch_get_shellsize __ARGS((void));
void mch_set_shellsize __ARGS((void));
void mch_new_shellsize __ARGS((void));
void mch_suspend __ARGS((void));
int mch_can_restore_title __ARGS((void));
int mch_can_restore_icon __ARGS((void));

void slash_to_colon __ARGS((char_u *p));
char_u *slash_to_colon_save __ARGS((char_u *p));

/* vim: set ft=c : */
