/* hangulin.c */
int hangul_input_state_get(void);
void hangul_input_state_set(int state);
int im_get_status(void);
void hangul_input_state_toggle(void);
void hangul_keyboard_set(void);
int hangul_input_process(char_u *s, int len);
char_u *hangul_string_convert(char_u *buf, int *p_len);
char_u *hangul_composing_buffer_get(int *p_len);
/* vim: set ft=c : */
