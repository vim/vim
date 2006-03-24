/* if_ruby.c */
extern int ruby_enabled __ARGS((int verbose));
extern void ruby_end __ARGS((void));
extern void ex_ruby __ARGS((exarg_T *eap));
extern void ex_rubydo __ARGS((exarg_T *eap));
extern void ex_rubyfile __ARGS((exarg_T *eap));
extern void ruby_buffer_free __ARGS((buf_T *buf));
extern void ruby_window_free __ARGS((win_T *win));
/* vim: set ft=c : */
