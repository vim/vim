/* if_tcl.c */
void vim_tcl_init __ARGS((char *arg));
int tcl_enabled __ARGS((int verbose));
void tcl_end __ARGS((void));
void ex_tcl __ARGS((exarg_T *eap));
void ex_tclfile __ARGS((exarg_T *eap));
void ex_tcldo __ARGS((exarg_T *eap));
void tcl_buffer_free __ARGS((buf_T *buf));
void tcl_window_free __ARGS((win_T *win));
/* vim: set ft=c : */
