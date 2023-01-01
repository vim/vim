/* vim9class.c */
void ex_class(exarg_T *eap);
type_T *class_member_type(class_T *cl, char_u *name, char_u *name_end, int *member_idx);
void ex_interface(exarg_T *eap);
void ex_enum(exarg_T *eap);
void ex_type(exarg_T *eap);
int class_object_index(char_u **arg, typval_T *rettv, evalarg_T *evalarg, int verbose);
ufunc_T *find_class_func(char_u **arg);
int class_member_exists(char_u *name, class_T **cl_ret, int *idx_ret, cctx_T *cctx);
void copy_object(typval_T *from, typval_T *to);
void object_unref(object_T *obj);
void copy_class(typval_T *from, typval_T *to);
void class_unref(class_T *cl);
void object_created(object_T *obj);
void object_cleared(object_T *obj);
int object_free_nonref(int copyID);
/* vim: set ft=c : */
