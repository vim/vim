/* usercmd.c */
char_u *find_ucmd(exarg_T *eap, char_u *p, int *full, expand_T *xp, int *complp);
char_u *set_context_in_user_cmd(expand_T *xp, char_u *arg_in);
char_u *get_user_command_name(int idx);
char_u *get_user_commands(expand_T *xp, int idx);
char_u *get_user_cmd_addr_type(expand_T *xp, int idx);
char_u *get_user_cmd_flags(expand_T *xp, int idx);
char_u *get_user_cmd_nargs(expand_T *xp, int idx);
char_u *get_user_cmd_complete(expand_T *xp, int idx);
int cmdcomplete_str_to_type(char_u *complete_str);
char *uc_fun_cmd(void);
int parse_compl_arg(char_u *value, int vallen, int *complp, long *argt, char_u **compl_arg);
void ex_command(exarg_T *eap);
void ex_comclear(exarg_T *eap);
void uc_clear(garray_T *gap);
void ex_delcommand(exarg_T *eap);
void do_ucmd(exarg_T *eap);
/* vim: set ft=c : */
