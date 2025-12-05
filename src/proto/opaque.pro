/* opaque.c */
opaque_type_T *lookup_opaque_type(char_u *name, size_t namelen);
opaque_T *opaque_new(opaque_type_T *type, void *data, size_t data_sz);
void opaque_unref(opaque_T *op);
bool opaque_equal_ptr(opaque_T *a, opaque_T *b);
opaque_property_T *lookup_opaque_property(opaque_type_T *op, char_u *name, size_t namelen, int *idx);
int opaque_property_index(char_u **arg, typval_T *rettv);
/* vim: set ft=c : */
