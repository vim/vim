/* vim9class.c */
int object_index_from_itf_index(class_T *itf, int is_method, int idx, class_T *cl, int is_static);
void ex_class(exarg_T *eap);
type_T *class_member_type(class_T *cl, int is_object, char_u *name, char_u *name_end, int *member_idx, ocmember_T **m);
void ex_enum(exarg_T *eap);
void ex_type(exarg_T *eap);
int class_object_index(char_u **arg, typval_T *rettv, evalarg_T *evalarg, int verbose);
ufunc_T *find_class_func(char_u **arg);
int class_member_idx(class_T *cl, char_u *name, size_t namelen);
ocmember_T *class_member_lookup(class_T *cl, char_u *name, size_t namelen, int *idx);
int class_method_idx(class_T *cl, char_u *name, size_t namelen);
ufunc_T *class_method_lookup(class_T *cl, char_u *name, size_t namelen, int *idx);
int class_obj_member_idx(class_T *cl, char_u *name, size_t namelen);
ocmember_T *class_obj_member_lookup(class_T *cl, char_u *name, size_t namelen, int *idx);
int class_obj_method_idx(class_T *cl, char_u *name, size_t namelen);
ufunc_T *class_obj_method_lookup(class_T *cl, char_u *name, size_t namelen, int *idx);
int inside_class(cctx_T *cctx_arg, class_T *cl);
void copy_object(typval_T *from, typval_T *to);
void object_unref(object_T *obj);
void copy_class(typval_T *from, typval_T *to);
void class_unref(class_T *cl);
int class_free_nonref(int copyID);
int set_ref_in_classes(int copyID);
void object_created(object_T *obj);
void object_cleared(object_T *obj);
int object_free_nonref(int copyID);
void f_instanceof(typval_T *argvars, typval_T *rettv);
int class_instance_of(class_T *cl, class_T *other_cl);
/* vim: set ft=c : */
