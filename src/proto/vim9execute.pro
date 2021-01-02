/* vim9execute.c */
void to_string_error(vartype_T vartype);
void funcstack_check_refcount(funcstack_T *funcstack);
int fill_partial_and_closure(partial_T *pt, ufunc_T *ufunc, ectx_T *ectx);
int call_def_function(ufunc_T *ufunc, int argc_arg, typval_T *argv, partial_T *partial, typval_T *rettv);
void ex_disassemble(exarg_T *eap);
int tv2bool(typval_T *tv);
void emsg_using_string_as(typval_T *tv, int as_number);
int check_not_string(typval_T *tv);
/* vim: set ft=c : */
