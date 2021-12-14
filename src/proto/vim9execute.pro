/* vim9execute.c */
void to_string_error(vartype_T vartype);
void funcstack_check_refcount(funcstack_T *funcstack);
int set_ref_in_funcstacks(int copyID);
char_u *char_from_string(char_u *str, varnumber_T index);
char_u *string_slice(char_u *str, varnumber_T first, varnumber_T last, int exclusive);
int fill_partial_and_closure(partial_T *pt, ufunc_T *ufunc, ectx_T *ectx);
typval_T *lookup_debug_var(char_u *name);
int may_break_in_function(ufunc_T *ufunc);
int exe_typval_instr(typval_T *tv, typval_T *rettv);
char_u *exe_substitute_instr(void);
int call_def_function(ufunc_T *ufunc, int argc_arg, typval_T *argv, partial_T *partial, typval_T *rettv);
void set_context_in_disassemble_cmd(expand_T *xp, char_u *arg);
char_u *get_disassemble_argument(expand_T *xp, int idx);
void ex_disassemble(exarg_T *eap);
int tv2bool(typval_T *tv);
void emsg_using_string_as(typval_T *tv, int as_number);
int check_not_string(typval_T *tv);
/* vim: set ft=c : */
