/* cmdexpand.c */
int nextwild(expand_T *xp, int type, int options, int escape);
char_u *ExpandOne(expand_T *xp, char_u *str, char_u *orig, int options, int mode);
void ExpandInit(expand_T *xp);
void ExpandCleanup(expand_T *xp);
int showmatches(expand_T *xp, int wildmenu);
char_u *sm_gettail(char_u *s);
char_u *addstar(char_u *fname, int len, int context);
void set_cmd_context(expand_T *xp, char_u *str, int len, int col, int use_ccline);
int expand_cmdline(expand_T *xp, char_u *str, int col, int *matchcount, char_u ***matches);
void globpath(char_u *path, char_u *file, garray_T *ga, int expand_options);
void f_getcompletion(typval_T *argvars, typval_T *rettv);
/* vim: set ft=c : */
