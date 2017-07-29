/* evalfunc.c */
buf_T* get_buf_tv(typval_T *tv, int curtab_only);
char_u *get_function_name(expand_T *xp, int idx);
char_u *get_expr_name(expand_T *xp, int idx);
int find_internal_func(char_u *name);
int call_internal_func(char_u *name, int argcount, typval_T *argvars, typval_T *rettv);
buf_T *buflist_find_by_name(char_u *name, int curtab_only);
void execute_redir_str(char_u *value, int value_len);
void mzscheme_call_vim(char_u *name, typval_T *args, typval_T *rettv);
float_T vim_round(float_T f);
long do_searchpair(char_u *spat, char_u *mpat, char_u *epat, int dir, char_u *skip, int flags, pos_T *match_pos, linenr_T lnum_stop, long time_limit);
char_u *get_callback(typval_T *arg, partial_T **pp);
void free_callback(char_u *callback, partial_T *partial);
/* vim: set ft=c : */
