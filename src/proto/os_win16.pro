/* os_win16.c */
void mch_setmouse(int on);
void mch_init(void);
int mch_check_win(int argc, char **argv);
long mch_get_pid(void);
int mch_call_shell(char_u *cmd, int options);
void mch_delay(long msec, int ignoreinput);
void mch_breakcheck(void);
long_u mch_avail_mem(int special);
int mch_rename(const char *pszOldFile, const char *pszNewFile);
char *default_shell(void);
/* vim: set ft=c : */
