/* textprop.c */
void f_prop_add(typval_T *argvars, typval_T *rettv);
int get_text_props(buf_T *buf, linenr_T lnum, char_u **props, int will_change);
proptype_T *text_prop_type_by_id(buf_T *buf, int id);
void f_prop_clear(typval_T *argvars, typval_T *rettv);
void f_prop_list(typval_T *argvars, typval_T *rettv);
void f_prop_remove(typval_T *argvars, typval_T *rettv);
void prop_type_set(typval_T *argvars, int add);
void f_prop_type_add(typval_T *argvars, typval_T *rettv);
void f_prop_type_change(typval_T *argvars, typval_T *rettv);
void f_prop_type_delete(typval_T *argvars, typval_T *rettv);
void f_prop_type_get(typval_T *argvars, typval_T *rettv);
void f_prop_type_list(typval_T *argvars, typval_T *rettv);
void clear_global_prop_types(void);
void clear_buf_prop_types(buf_T *buf);
void adjust_prop_columns(linenr_T lnum, colnr_T col, int bytes_added);
/* vim: set ft=c : */
