/* hangulin.c */
extern int hangul_input_state_get __ARGS((void));
extern void hangul_input_state_set __ARGS((int state));
extern int im_get_status __ARGS((void));
extern void hangul_input_state_toggle __ARGS((void));
extern void hangul_keyboard_set __ARGS((void));
extern int hangul_input_process __ARGS((char_u *s, int len));
extern void hangul_input_clear __ARGS((void));
/* vim: set ft=c : */
