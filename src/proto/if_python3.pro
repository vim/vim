/* if_python.c */
int python3_enabled __ARGS((int verbose));
void python3_end __ARGS((void));
void ex_python3 __ARGS((exarg_T *eap));
void ex_py3file __ARGS((exarg_T *eap));
void python3_buffer_free __ARGS((buf_T *buf));
void python3_window_free __ARGS((win_T *win));
/* vim: set ft=c : */
