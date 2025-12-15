/* opaque.c */
opaque_type_T *lookup_opaque_type(char_u *name, size_t namelen);
opaque_T *opaque_new(opaque_type_T *type, size_t struct_sz);
void opaque_unref(opaque_T *op);
opaque_property_T *lookup_opaque_property(opaque_type_T *ot, char_u *name, size_t namelen, int *idx);
int opaque_property_index(char_u **arg, typval_T *rettv);
bool opaque_direct_equal_func(opaque_T *a, opaque_T *b);
/* vim: set ft=c : */
