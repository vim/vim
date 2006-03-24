/* spell.c */
extern int spell_check __ARGS((win_T *wp, char_u *ptr, hlf_T *attrp, int *capcol, int docount));
extern int spell_move_to __ARGS((win_T *wp, int dir, int allwords, int curline, hlf_T *attrp));
extern void spell_cat_line __ARGS((char_u *buf, char_u *line, int maxlen));
extern char_u *did_set_spelllang __ARGS((buf_T *buf));
extern void spell_free_all __ARGS((void));
extern void spell_reload __ARGS((void));
extern int spell_check_msm __ARGS((void));
extern void put_bytes __ARGS((FILE *fd, long_u nr, int len));
extern void ex_mkspell __ARGS((exarg_T *eap));
extern void ex_spell __ARGS((exarg_T *eap));
extern void spell_add_word __ARGS((char_u *word, int len, int bad, int index, int undo));
extern void init_spell_chartab __ARGS((void));
extern int spell_check_sps __ARGS((void));
extern void spell_suggest __ARGS((int count));
extern void ex_spellrepall __ARGS((exarg_T *eap));
extern void spell_suggest_list __ARGS((garray_T *gap, char_u *word, int maxcount, int need_cap, int interactive));
extern char_u *eval_soundfold __ARGS((char_u *word));
extern void ex_spellinfo __ARGS((exarg_T *eap));
extern void ex_spelldump __ARGS((exarg_T *eap));
extern void spell_dump_compl __ARGS((buf_T *buf, char_u *pat, int ic, int *dir, int dumpflags_arg));
extern char_u *spell_to_word_end __ARGS((char_u *start, buf_T *buf));
extern int spell_word_start __ARGS((int startcol));
extern void spell_expand_check_cap __ARGS((colnr_T col));
extern int expand_spelling __ARGS((linenr_T lnum, int col, char_u *pat, char_u ***matchp));
/* vim: set ft=c : */
