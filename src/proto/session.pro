/* session.c */
int makeopens(FILE *fd, char_u *dirnow);
int put_view(FILE *fd, win_T *wp, int add_edit, unsigned *flagp, int current_arg_idx);
char_u *get_view_file(int c);
void ex_loadview(exarg_T *eap);
int store_session_globals(FILE *fd);
int write_session_file(char_u *filename);
void ex_mkrc(exarg_T *eap);
int put_eol(FILE *fd);
int put_line(FILE *fd, char *s);
/* vim: set ft=c : */
