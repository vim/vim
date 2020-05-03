/* vim9execute.c */
int call_def_function(ufunc_T *ufunc, int argc_arg, typval_T *argv, typval_T *rettv);
void ex_disassemble(exarg_T *eap);
int tv2bool(typval_T *tv);
int check_not_string(typval_T *tv);
int set_ref_in_dfunc(ufunc_T *ufunc, int copyID);
/* vim: set ft=c : */
