/* tag.c */
int do_tag __ARGS((char_u *tag, int type, int count, int forceit, int verbose));
void tag_freematch __ARGS((void));
void do_tags __ARGS((exarg_T *eap));
int find_tags __ARGS((char_u *pat, int *num_matches, char_u ***matchesp, int flags, int mincount, char_u *buf_ffname));
void simplify_filename __ARGS((char_u *filename));
int expand_tags __ARGS((int tagnames, char_u *pat, int *num_file, char_u ***file));
/* vim: set ft=c : */
