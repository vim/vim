/* evalfunc.c */
char_u *get_function_name(expand_T *xp, int idx);
char_u *get_expr_name(expand_T *xp, int idx);
int find_internal_func(char_u *name);
int has_internal_func(char_u *name);
char *internal_func_name(int idx);
int internal_func_check_arg_types(type2_T *types, int idx, int argcount, cctx_T *cctx);
void internal_func_get_argcount(int idx, int *argcount, int *min_argcount);
type_T *internal_func_ret_type(int idx, int argcount, type2_T *argtypes, type_T **decl_type);
int internal_func_is_map(int idx);
int check_internal_func(int idx, int argcount);
int call_internal_func(char_u *name, int argcount, typval_T *argvars, typval_T *rettv);
void call_internal_func_by_idx(int idx, typval_T *argvars, typval_T *rettv);
int call_internal_method(char_u *name, int argcount, typval_T *argvars, typval_T *rettv, typval_T *basetv);
int non_zero_arg(typval_T *argvars);
buf_T *get_buf_arg(typval_T *arg);
win_T *get_optional_window(typval_T *argvars, int idx);
void execute_redir_str(char_u *value, int value_len);
void execute_cmds_from_string(char_u *str);
void execute_common(typval_T *argvars, typval_T *rettv, int arg_off);
void f_exists(typval_T *argvars, typval_T *rettv);
void f_has(typval_T *argvars, typval_T *rettv);
void f_len(typval_T *argvars, typval_T *rettv);
int dynamic_feature(char_u *feature);
void mzscheme_call_vim(char_u *name, typval_T *args, typval_T *rettv);
void range_list_materialize(list_T *list);
long do_searchpair(char_u *spat, char_u *mpat, char_u *epat, int dir, typval_T *skip, int flags, pos_T *match_pos, linenr_T lnum_stop, long time_limit);
/* vim: set ft=c : */
