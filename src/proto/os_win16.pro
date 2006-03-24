/* os_win16.c */
extern void mch_setmouse __ARGS((int on));
extern void mch_init __ARGS((void));
extern int mch_check_win __ARGS((int argc, char **argv));
extern long mch_get_pid __ARGS((void));
extern int mch_call_shell __ARGS((char_u *cmd, int options));
extern void mch_delay __ARGS((long msec, int ignoreinput));
extern void mch_breakcheck __ARGS((void));
extern long_u mch_avail_mem __ARGS((int special));
extern int mch_rename __ARGS((const char *pszOldFile, const char *pszNewFile));
extern char *default_shell __ARGS((void));
/* vim: set ft=c : */
