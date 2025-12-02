/* opaque.c */
opaque_T *opaque_new(char_u *type, void *data, size_t data_sz);
void opaque_unref(opaque_T *op);
/* vim: set ft=c : */
