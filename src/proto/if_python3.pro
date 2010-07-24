/* if_python3.c */
int python3_enabled __ARGS((int verbose));
void python3_end __ARGS((void));
int python3_loaded __ARGS((void));
void ex_py3 __ARGS((exarg_T *eap));
void ex_py3file __ARGS((exarg_T *eap));
void python3_buffer_free __ARGS((buf_T *buf));
void python3_window_free __ARGS((win_T *win));
/* vim: set ft=c : */
