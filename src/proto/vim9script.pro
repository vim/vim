/* vim9script.c */
int in_vim9script(void);
void ex_vim9script(exarg_T *eap);
void ex_export(exarg_T *eap);
void free_imports(int sid);
void ex_import(exarg_T *eap);
int find_exported(int sid, char_u **argp, int *name_len, ufunc_T **ufunc, type_T **type);
char_u *handle_import(char_u *arg_start, garray_T *gap, int import_sid);
/* vim: set ft=c : */
