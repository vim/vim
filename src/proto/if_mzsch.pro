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
struct Scheme_Object *mzvim_apply __ARGS((struct Scheme_Object *, int argc,
    struct Scheme_Object **));
int mzthreads_allowed (void);
#ifdef FEAT_GUI_KDE
void timer_proc (void);
void mzscheme_kde_start_timer (void);
void mzscheme_kde_stop_timer (void);
#endif
/* vim: set ft=c : */
