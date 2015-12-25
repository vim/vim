/* hangulin.c */
int hangul_input_state_get __ARGS((void));
void hangul_input_state_set __ARGS((int state));
int im_get_status __ARGS((void));
void hangul_input_state_toggle __ARGS((void));
void hangul_keyboard_set __ARGS((void));
int hangul_input_process __ARGS((char_u *s, int len));
void hangul_input_clear __ARGS((void));
char_u *hangul_string_convert __ARGS((char_u *buf, int *p_len));
char_u *hangul_composing_buffer_get __ARGS((int *p_len));
/* vim: set ft=c : */
