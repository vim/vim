/* json.c */
char_u *json_encode(typval_T *val);
char_u *json_encode_nr_expr(int nr, typval_T *val);
int json_decode_all(js_read_T *reader, typval_T *res);
int json_decode(js_read_T *reader, typval_T *res);
int json_find_end(js_read_T *reader);
/* vim: set ft=c : */
