/* spell.c */
int spell_check __ARGS((win_T *wp, char_u *ptr, int *attrp));
int spell_move_to __ARGS((int dir, int allwords, int curline));
void spell_cat_line __ARGS((char_u *buf, char_u *line, int maxlen));
char_u *did_set_spelllang __ARGS((buf_T *buf));
void spell_free_all __ARGS((void));
void spell_reload __ARGS((void));
void put_bytes __ARGS((FILE *fd, long_u nr, int len));
void ex_mkspell __ARGS((exarg_T *eap));
void ex_spell __ARGS((exarg_T *eap));
void spell_add_word __ARGS((char_u *word, int len, int bad));
void init_spell_chartab __ARGS((void));
void spell_suggest __ARGS((void));
void spell_suggest_list __ARGS((garray_T *gap, char_u *word, int maxcount));
void ex_spelldump __ARGS((exarg_T *eap));
/* vim: set ft=c : */
