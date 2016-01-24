/* json.c */
char_u *json_encode(typval_T *val);
void json_encode_item(garray_T *gap, typval_T *val, int copyID);
void json_decode(js_read_T *reader, typval_T *res);
/* vim: set ft=c : */
