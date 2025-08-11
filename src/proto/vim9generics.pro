/* vim9generics.c */
char_u *generic_func_find_open_bracket(char_u *name);
int skip_generic_func_type_args(char_u **argp);
char_u *append_generic_func_type_args(char_u *funcname, size_t namelen, char_u **argp);
char_u *get_generic_func_name(ufunc_T *fp, char_u **argp);
char_u *parse_generic_func_type_args(char_u *func_name, size_t namelen, char_u *start, gfargs_tab_T *gfatab, cctx_T *cctx);
char_u *parse_generic_func_type_params(char_u *func_name, char_u *p, gfargs_tab_T *gfatab, cctx_T *cctx);
void generic_func_init(ufunc_T *fp, gfargs_tab_T *gfatab);
void generic_func_args_table_init(gfargs_tab_T *gfatab);
int generic_func_args_table_size(gfargs_tab_T *gfatab);
void generic_func_args_table_clear(gfargs_tab_T *gfatab);
void copy_generic_function(ufunc_T *fp, ufunc_T *new_fp);
ufunc_T *eval_generic_func(ufunc_T *ufunc, char_u *name, char_u **argp);
int generic_func_call(char_u **argp);
ufunc_T *generic_func_get(ufunc_T *fp, gfargs_tab_T *gfatab);
ufunc_T *find_generic_func(ufunc_T *ufunc, char_u *name, char_u **argp);
type_T *find_generic_type(char_u *gt_name, size_t name_len, ufunc_T *ufunc, cctx_T *cctx);
void generic_func_clear_items(ufunc_T *fp);
/* vim: set ft=c : */
