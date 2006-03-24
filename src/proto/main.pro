/* main.c */
extern void main_loop __ARGS((int cmdwin, int noexmode));
extern void getout_preserve_modified __ARGS((int exitval));
extern void getout __ARGS((int exitval));
extern int process_env __ARGS((char_u *env, int is_viminit));
extern void mainerr_arg_missing __ARGS((char_u *str));
extern void time_push __ARGS((void *tv_rel, void *tv_start));
extern void time_pop __ARGS((void *tp));
extern void time_msg __ARGS((char *msg, void *tv_start));
extern void server_to_input_buf __ARGS((char_u *str));
extern char_u *eval_client_expr_to_string __ARGS((char_u *expr));
extern char_u *serverConvert __ARGS((char_u *client_enc, char_u *data, char_u **tofree));
extern int toF_TyA __ARGS((int c));
extern int fkmap __ARGS((int c));
extern void conv_to_pvim __ARGS((void));
extern void conv_to_pstd __ARGS((void));
extern char_u *lrswap __ARGS((char_u *ibuf));
extern char_u *lrFswap __ARGS((char_u *cmdbuf, int len));
extern char_u *lrF_sub __ARGS((char_u *ibuf));
extern int cmdl_fkmap __ARGS((int c));
extern int F_isalpha __ARGS((int c));
extern int F_isdigit __ARGS((int c));
extern int F_ischar __ARGS((int c));
extern void farsi_fkey __ARGS((cmdarg_T *cap));
extern int arabic_shape __ARGS((int c, int *ccp, int *c1p, int prev_c, int prev_c1, int next_c));
/* vim: set ft=c : */
