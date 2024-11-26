/* linematch.c */
typedef struct diffcmppath_S diffcmppath_T;
size_t line_len(const mmfile_t *m);
int matching_chars_iwhite(const mmfile_t *s1, const mmfile_t *s2);
int matching_chars(const mmfile_t *m1, const mmfile_t *m2);
int count_n_matched_chars(mmfile_t **sp, const size_t n, Bool iwhite);
void try_possible_paths(const int *df_iters, const size_t *paths, const int npaths, const int path_idx, int *choice, diffcmppath_T *diffcmppath, const int *diff_len, const size_t ndiffs, const mmfile_t **diff_blk, Bool iwhite);
size_t unwrap_indexes(const int *values, const int *diff_len, const size_t ndiffs);
void populate_tensor(int *df_iters, const size_t ch_dim, diffcmppath_T *diffcmppath, const int *diff_len, const size_t ndiffs, const mmfile_t **diff_blk, Bool iwhite);
size_t linematch_nbuffers(const mmfile_t **diff_blk, const int *diff_len, const size_t ndiffs, int **decisions, Bool iwhite);
size_t test_charmatch_paths(diffcmppath_T *node, int lastdecision);
/* vim: set ft=c : */
