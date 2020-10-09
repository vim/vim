/* vim9execute.c */
void to_string_error(vartype_T vartype);
int call_def_function(ufunc_T *ufunc, int argc_arg, typval_T *argv, partial_T *partial, typval_T *rettv);
void ex_disassemble(exarg_T *eap);
int tv2bool(typval_T *tv);
int check_not_string(typval_T *tv);
/* vim: set ft=c : */
