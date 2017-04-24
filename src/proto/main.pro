/* main.c */
int vim_main2(void);
void common_init(mparm_T *paramp);
int is_not_a_term(void);
void main_loop(int cmdwin, int noexmode);
void getout_preserve_modified(int exitval);
void getout(int exitval);
int process_env(char_u *env, int is_viminit);
void mainerr_arg_missing(char_u *str);
void time_push(void *tv_rel, void *tv_start);
void time_pop(void *tp);
void time_msg(char *mesg, void *tv_start);
void server_to_input_buf(char_u *str);
char_u *eval_client_expr_to_string(char_u *expr);
int sendToLocalVim(char_u *cmd, int asExpr, char_u **result);
char_u *serverConvert(char_u *client_enc, char_u *data, char_u **tofree);
/* vim: set ft=c : */
