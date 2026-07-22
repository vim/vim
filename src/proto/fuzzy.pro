/* fuzzy.c */
int fuzzy_match(char_u *str, char_u *pat_arg, int matchseq, int *outScore, int_u *matches, int maxMatches);
void f_matchfuzzy(typval_T *argvars, typval_T *rettv);
void f_matchfuzzypos(typval_T *argvars, typval_T *rettv);
int fuzzy_match_str(char_u *str, char_u *pat);
garray_T *fuzzy_match_str_with_pos(char_u *str, char_u *pat);
int fuzzy_match_str_in_line(char_u **ptr, char_u *pat, int *len, pos_T *current_pos, int *score);
int search_for_fuzzy_match(buf_T *buf, pos_T *pos, char_u *pattern, int dir, pos_T *start_pos, int *len, char_u **ptr, int *score);
void fuzmatch_str_free(fuzmatch_str_T *fuzmatch, int count);
int fuzzymatches_to_strmatches(fuzmatch_str_T *fuzmatch, char_u ***matches, int count, int funcsort);
/* vim: set ft=c : */
