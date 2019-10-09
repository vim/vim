/* cindent.c */
int cin_is_cinword(char_u *line);
pos_T *find_start_comment(int ind_maxcomment);
int cindent_on(void);
void parse_cino(buf_T *buf);
int get_c_indent(void);
int get_expr_indent(void);
int in_cinkeys(int keytyped, int when, int line_is_empty);
void do_c_expr_indent(void);
int get_lisp_indent(void);
void fixthisline(int (*get_the_indent)(void));
void fix_indent(void);
void f_cindent(typval_T *argvars UNUSED, typval_T *rettv);
void f_lispindent(typval_T *argvars UNUSED, typval_T *rettv);
/* vim: set ft=c : */
