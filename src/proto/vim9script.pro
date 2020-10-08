/* vim9script.c */
int in_vim9script(void);
void ex_vim9script(exarg_T *eap);
int not_in_vim9(exarg_T *eap);
void ex_export(exarg_T *eap);
void free_imports(int sid);
void ex_import(exarg_T *eap);
int find_exported(int sid, char_u *name, ufunc_T **ufunc, type_T **type);
char_u *handle_import(char_u *arg_start, garray_T *gap, int import_sid, evalarg_T *evalarg, void *cctx);
char_u *vim9_declare_scriptvar(exarg_T *eap, char_u *arg);
svar_T *find_typval_in_script(typval_T *dest);
int check_script_var_type(typval_T *dest, typval_T *value, char_u *name);
/* vim: set ft=c : */
