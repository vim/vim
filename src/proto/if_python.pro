/* if_python.c */
extern int python_enabled __ARGS((int verbose));
extern void python_end __ARGS((void));
extern void ex_python __ARGS((exarg_T *eap));
extern void ex_pyfile __ARGS((exarg_T *eap));
extern void python_buffer_free __ARGS((buf_T *buf));
extern void python_window_free __ARGS((win_T *win));
/* vim: set ft=c : */
