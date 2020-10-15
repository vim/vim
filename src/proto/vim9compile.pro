/* vim9compile.c */
int check_defined(char_u *p, size_t len, cctx_T *cctx);
int check_compare_types(exptype_T type, typval_T *tv1, typval_T *tv2);
int get_script_item_idx(int sid, char_u *name, int check_writable, cctx_T *cctx);
imported_T *find_imported(char_u *name, size_t len, cctx_T *cctx);
imported_T *find_imported_in_script(char_u *name, size_t len, int sid);
int vim9_comment_start(char_u *p);
char_u *peek_next_line_from_context(cctx_T *cctx);
char_u *next_line_from_context(cctx_T *cctx, int skip_comment);
char_u *to_name_const_end(char_u *arg);
exptype_T get_compare_type(char_u *p, int *len, int *type_is);
void error_white_both(char_u *op, int len);
int assignment_len(char_u *p, int *heredoc);
void vim9_declare_error(char_u *name);
int check_vim9_unlet(char_u *name);
int compile_def_function(ufunc_T *ufunc, int set_return_type, cctx_T *outer_cctx);
void set_function_type(ufunc_T *ufunc);
void delete_instr(isn_T *isn);
void clear_def_function(ufunc_T *ufunc);
void unlink_def_function(ufunc_T *ufunc);
void free_def_functions(void);
/* vim: set ft=c : */
