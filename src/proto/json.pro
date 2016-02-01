/* json.c */
char_u *json_encode(typval_T *val);
char_u *json_encode_nr_expr(int nr, typval_T *val);
int json_decode(js_read_T *reader, typval_T *res);
/* vim: set ft=c : */
