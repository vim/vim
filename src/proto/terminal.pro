/* terminal.c */
void ex_terminal(exarg_T *eap);
void free_terminal(term_T *term);
void write_to_term(buf_T *buffer, char_u *msg, channel_T *channel);
void terminal_loop(void);
void term_job_ended(job_T *job);
int term_job_running(buf_T *buf);
void term_update_window(win_T *wp);
/* vim: set ft=c : */
