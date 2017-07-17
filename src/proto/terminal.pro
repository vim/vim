/* terminal.c */
void ex_terminal(exarg_T *eap);
void free_terminal(term_T *term);
void write_to_term(buf_T *buffer, char_u *msg, channel_T *channel);
void term_update_window(win_T *wp);
void terminal_loop(void);
/* vim: set ft=c : */
