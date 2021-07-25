/* vim9compile.c */
int check_defined(char_u *p, size_t len, cctx_T *cctx, int is_arg);
int check_compare_types(exprtype_T type, typval_T *tv1, typval_T *tv2);
int use_typecheck(type_T *actual, type_T *expected);
int need_type(type_T *actual, type_T *expected, int offset, int arg_idx, cctx_T *cctx, int silent, int actual_is_const);
int func_needs_compiling(ufunc_T *ufunc, compiletype_T compile_type);
int get_script_item_idx(int sid, char_u *name, int check_writable, cctx_T *cctx);
imported_T *find_imported(char_u *name, size_t len, cctx_T *cctx);
imported_T *find_imported_in_script(char_u *name, size_t len, int sid);
char_u *peek_next_line_from_context(cctx_T *cctx);
char_u *next_line_from_context(cctx_T *cctx, int skip_comment);
char_u *to_name_end(char_u *arg, int use_namespace);
char_u *to_name_const_end(char_u *arg);
int get_lambda_tv_and_compile(char_u **arg, typval_T *rettv, int types_optional, evalarg_T *evalarg);
exprtype_T get_compare_type(char_u *p, int *len, int *type_is);
void error_white_both(char_u *op, int len);
void fill_exarg_from_cctx(exarg_T *eap, cctx_T *cctx);
int assignment_len(char_u *p, int *heredoc);
void vim9_declare_error(char_u *name);
int check_vim9_unlet(char_u *name);
int compile_def_function(ufunc_T *ufunc, int check_return_type, compiletype_T compile_type, cctx_T *outer_cctx);
void set_function_type(ufunc_T *ufunc);
void delete_instr(isn_T *isn);
void unlink_def_function(ufunc_T *ufunc);
void link_def_function(ufunc_T *ufunc);
void free_def_functions(void);
/* vim: set ft=c : */
