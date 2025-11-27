// vim9generics.c
char_u *generic_func_find_open_bracket(char_u *name);
int skip_generic_type_args(char_u **argp);
char_u *append_generic_func_type_args(char_u *funcname, size_t namelen, char_u **argp);
char_u *get_generic_func_name(ufunc_T *fp, char_u **argp);
char_u *parse_generic_type_args(char_u *name, size_t namelen, char_u *start, generic_args_tab_T *gatab, cctx_T *cctx);
char_u *parse_generic_type_params(char_u *name, char_u *p, generic_args_tab_T *gatab, cctx_T *cctx);
void generic_func_init(ufunc_T *fp, generic_args_tab_T *gatab);
void generic_args_table_init(generic_args_tab_T *gatab);
int generic_args_table_size(generic_args_tab_T *gatab);
void generic_args_table_clear(generic_args_tab_T *gatab);
int copy_generic_function(ufunc_T *fp, ufunc_T *new_fp);
ufunc_T *eval_generic_func(ufunc_T *ufunc, char_u *name, char_u **argp);
int generic_func_call(char_u **argp);
void update_generic_type(class_T *cl, class_T *new_cl, ufunc_T *fp, ufunc_T *new_fp, type_T *generic_type, type_T **specific_type, type_T **func_type);
void generic_args_key(generic_args_tab_T *gatab, garray_T *gap, size_t *typeslen);
hashitem_T *generic_args_lookup(hashtab_T *ht, generic_args_tab_T *gatab, garray_T *gkey_gap, size_t *typeslen);
int update_func_generic_types(class_T *cl, class_T *new_cl, ufunc_T *fp, ufunc_T *new_fp);
ufunc_T *generic_func_get(ufunc_T *fp, generic_args_tab_T *gatab);
ufunc_T *generic_func_get_same_types(ufunc_T *fp, ufunc_T *inst);
ufunc_T *find_generic_func(ufunc_T *ufunc, char_u *name, char_u **argp);
type_T *find_generic_type(char_u *gt_name, size_t name_len, cctx_T *cctx);
void generic_func_clear_items(ufunc_T *fp);
// vim: ft=c
