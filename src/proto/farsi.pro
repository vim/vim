/* farsi.c */
int fkmap(int c);
char_u *lrswap(char_u *ibuf);
char_u *lrFswap(char_u *cmdbuf, int len);
char_u *lrF_sub(char_u *ibuf);
int cmdl_fkmap(int c);
int F_isalpha(int c);
int F_isdigit(int c);
int F_ischar(int c);
void farsi_f8(cmdarg_T *cap);
void farsi_f9(cmdarg_T *cap);
/* vim: set ft=c : */
