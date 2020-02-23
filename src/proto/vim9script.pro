/* vim9script.c */
int in_vim9script(void);
void ex_vim9script(exarg_T *eap);
void ex_export(exarg_T *eap);
void free_imports(int sid);
void ex_import(exarg_T *eap);
char_u *handle_import(char_u *arg_start, garray_T *gap, int sid);
/* vim: set ft=c : */
