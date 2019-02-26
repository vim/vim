/* insexpand.c */
void ins_ctrl_x(void);
int ctrl_x_mode_not_default(void);
int ctrl_x_mode_not_defined_yet(void);
int has_compl_option(int dict_opt);
int vim_is_ctrl_x_key(int c);
int ins_compl_accept_char(int c);
int ins_compl_add_infercase(char_u *str, int len, int icase, char_u *fname, int dir, int flags);
void completeopt_was_set(void);
void set_completion(colnr_T startcol, list_T *list);
int  pum_wanted(void);
void ins_compl_show_pum(void);
char_u *find_word_start(char_u *ptr);
char_u *find_word_end(char_u *ptr);
void ins_compl_clear(void);
int ins_compl_active(void);
int  ins_compl_bs(void);
void ins_compl_addleader(int c);
void ins_compl_addfrommatch(void);
int ins_compl_prep(int c);
void get_complete_info(list_T *what_list, dict_T *retdict);
int ins_compl_add_tv(typval_T *tv, int dir);
void ins_compl_delete(void);
void ins_compl_insert(int in_compl_func);
void ins_compl_check_keys(int frequency, int in_compl_func);
int ins_complete(int c, int enable_pum);
void free_insexpand_stuff(void);
/* vim: set ft=c : */
