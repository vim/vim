/* evalvars.c */
void ex_let(exarg_T *eap);
void ex_const(exarg_T *eap);
int ex_let_vars(char_u *arg_start, typval_T *tv, int copy, int semicolon, int var_count, int is_const, char_u *op);
char_u *skip_var_list(char_u *arg, int *var_count, int *semicolon);
void list_hashtable_vars(hashtab_T *ht, char *prefix, int empty, int *first);
void ex_unlet(exarg_T *eap);
void ex_lockvar(exarg_T *eap);
int do_unlet(char_u *name, int forceit);
int get_var_tv(char_u *name, int len, typval_T *rettv, dictitem_T **dip, int verbose, int no_autoload);
char_u *get_var_value(char_u *name);
void vars_clear(hashtab_T *ht);
void vars_clear_ext(hashtab_T *ht, int free_val);
void delete_var(hashtab_T *ht, hashitem_T *hi);
void set_var(char_u *name, typval_T *tv, int copy);
void set_var_const(char_u *name, typval_T *tv, int copy, int is_const);
int var_check_ro(int flags, char_u *name, int use_gettext);
int var_check_fixed(int flags, char_u *name, int use_gettext);
int var_check_func_name(char_u *name, int new_var);
int var_check_lock(int lock, char_u *name, int use_gettext);
int valid_varname(char_u *varname);
int var_exists(char_u *var);
void f_gettabvar(typval_T *argvars, typval_T *rettv);
void f_gettabwinvar(typval_T *argvars, typval_T *rettv);
void f_getwinvar(typval_T *argvars, typval_T *rettv);
void f_settabvar(typval_T *argvars, typval_T *rettv);
void f_settabwinvar(typval_T *argvars, typval_T *rettv);
void f_setwinvar(typval_T *argvars, typval_T *rettv);
/* vim: set ft=c : */
