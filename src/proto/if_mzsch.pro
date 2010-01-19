/* if_mzsch.c */
int mzscheme_enabled __ARGS((int verbose));
void mzscheme_end __ARGS((void));
void ex_mzscheme __ARGS((exarg_T *eap));
void ex_mzfile __ARGS((exarg_T *eap));
void mzscheme_buffer_free __ARGS((buf_T *buf));
void mzscheme_window_free __ARGS((win_T *win));
char *mzscheme_version __ARGS((void));
void raise_vim_exn(const char *add_info);
void raise_if_error __ARGS((void));
buf_T *get_valid_buffer __ARGS((void *));
win_T *get_valid_window __ARGS((void *));
void mzvim_check_threads __ARGS((void));
void mzvim_reset_timer __ARGS((void));
void *mzvim_eval_string __ARGS((char_u *str));
int mzthreads_allowed __ARGS((void));
void mzscheme_main __ARGS((void));
void do_mzeval __ARGS((char_u *str, typval_T *rettv));
/* vim: set ft=c : */
