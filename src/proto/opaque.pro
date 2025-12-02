/* opaque.c */
opaque_T *opaque_new(char_u *type, bool isstatic, void *data, size_t data_sz);
void opaque_unref(opaque_T *op);
bool opaque_equal_ptr(opaque_T *a, opaque_T *b);
/* vim: set ft=c : */
