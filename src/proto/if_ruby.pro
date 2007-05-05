/* if_ruby.c */
int ruby_enabled __ARGS((int verbose));
void ruby_end __ARGS((void));
void ex_ruby __ARGS((exarg_T *eap));
void ex_rubydo __ARGS((exarg_T *eap));
void ex_rubyfile __ARGS((exarg_T *eap));
void ruby_buffer_free __ARGS((buf_T *buf));
void ruby_window_free __ARGS((win_T *win));
/* vim: set ft=c : */
