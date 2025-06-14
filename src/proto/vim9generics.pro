/* vim9generics.c */
char_u *generic_func_find_open_angle_bracket(char_u *name);
char_u *generic_func_find_close_angle_bracket(char_u *s);
char_u *parse_generic_func_type_args(char_u *func_name, size_t len, char_u *start, garray_T *types_gap, garray_T *gtfn_gap, cctx_T *cctx);
char_u *parse_generic_func_type_params(char_u *func_name, char_u *p, garray_T *gtl_gap, garray_T *gt_gap, cctx_T *cctx);
ufunc_T *eval_generic_func(ufunc_T *ufunc, char_u *name, char_u **arg);
int generic_func_call(char_u **arg);
ufunc_T *generic_func_get(ufunc_T *fp, garray_T *gftn_gap);
ufunc_T *find_generic_func(ufunc_T *ufunc, char_u *name, char_u **arg);
type_T *find_generic_type(char_u *type_name, ufunc_T *ufunc, cctx_T *cctx);
void generic_func_clear_items(ufunc_T *fp);
/* vim: set ft=c : */
