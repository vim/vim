/* if_tcl.c */
extern void vim_tcl_init __ARGS((char *arg));
extern int tcl_enabled __ARGS((int verbose));
extern void tcl_end __ARGS((void));
extern void ex_tcl __ARGS((exarg_T *eap));
extern void ex_tclfile __ARGS((exarg_T *eap));
extern void ex_tcldo __ARGS((exarg_T *eap));
extern void tcl_buffer_free __ARGS((buf_T *buf));
extern void tcl_window_free __ARGS((win_T *win));
/* vim: set ft=c : */
