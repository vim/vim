/* vim9compile.c */
int check_defined(char_u *p, size_t len, cctx_T *cctx);
type_T *typval2type(typval_T *tv);
int check_type(type_T *expected, type_T *actual, int give_msg);
char_u *skip_type(char_u *start);
type_T *parse_type(char_u **arg, garray_T *type_gap);
char *vartype_name(vartype_T type);
char *type_name(type_T *type, char **tofree);
int get_script_item_idx(int sid, char_u *name, int check_writable);
imported_T *find_imported(char_u *name, size_t len, cctx_T *cctx);
char_u *to_name_const_end(char_u *arg);
int assignment_len(char_u *p, int *heredoc);
void vim9_declare_error(char_u *name);
int check_vim9_unlet(char_u *name);
int compile_def_function(ufunc_T *ufunc, int set_return_type, cctx_T *outer_cctx);
void set_function_type(ufunc_T *ufunc);
void delete_instr(isn_T *isn);
void clear_def_function(ufunc_T *ufunc);
void free_def_functions(void);
/* vim: set ft=c : */
