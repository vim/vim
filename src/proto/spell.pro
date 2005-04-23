/* spell.c */
int spell_check __ARGS((win_T *wp, char_u *line, char_u *ptr, int *attrp));
int spell_move_to __ARGS((int dir, int allwords));
char_u *did_set_spelllang __ARGS((buf_T *buf));
void spell_reload __ARGS((void));
void put_bytes __ARGS((FILE *fd, long_u nr, int len));
void ex_mkspell __ARGS((exarg_T *eap));
/* vim: set ft=c : */
