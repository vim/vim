/* vim9compile.c */
int lookup_local(char_u *name, size_t len, lvar_T *lvar, cctx_T *cctx);
int arg_exists(char_u *name, size_t len, int *idxp, type_T **type, int *gen_load_outer, cctx_T *cctx);
int script_is_vim9(void);
int script_var_exists(char_u *name, size_t len, cctx_T *cctx);
int check_defined(char_u *p, size_t len, cctx_T *cctx, int is_arg);
int need_type(type_T *actual, type_T *expected, int offset, int arg_idx, cctx_T *cctx, int silent, int actual_is_const);
lvar_T *reserve_local(cctx_T *cctx, char_u *name, size_t len, int isConst, type_T *type);
int get_script_item_idx(int sid, char_u *name, int check_writable, cctx_T *cctx);
imported_T *find_imported(char_u *name, size_t len, cctx_T *cctx);
char_u *may_peek_next_line(cctx_T *cctx, char_u *arg, char_u **nextp);
char_u *peek_next_line_from_context(cctx_T *cctx);
char_u *next_line_from_context(cctx_T *cctx, int skip_comment);
int may_get_next_line(char_u *whitep, char_u **arg, cctx_T *cctx);
int may_get_next_line_error(char_u *whitep, char_u **arg, cctx_T *cctx);
void fill_exarg_from_cctx(exarg_T *eap, cctx_T *cctx);
int func_needs_compiling(ufunc_T *ufunc, compiletype_T compile_type);
int assignment_len(char_u *p, int *heredoc);
void vim9_declare_error(char_u *name);
int get_var_dest(char_u *name, assign_dest_T *dest, int cmdidx, int *option_scope, int *vimvaridx, type_T **type, cctx_T *cctx);
int compile_lhs(char_u *var_start, lhs_T *lhs, int cmdidx, int heredoc, int oplen, cctx_T *cctx);
int compile_assign_lhs(char_u *var_start, lhs_T *lhs, int cmdidx, int is_decl, int heredoc, int oplen, cctx_T *cctx);
int compile_load_lhs_with_index(lhs_T *lhs, char_u *var_start, cctx_T *cctx);
int compile_assign_unlet(char_u *var_start, lhs_T *lhs, int is_assign, type_T *rhs_type, cctx_T *cctx);
int compile_def_function(ufunc_T *ufunc, int check_return_type, compiletype_T compile_type, cctx_T *outer_cctx);
void set_function_type(ufunc_T *ufunc);
void unlink_def_function(ufunc_T *ufunc);
void link_def_function(ufunc_T *ufunc);
void free_def_functions(void);
/* vim: set ft=c : */
