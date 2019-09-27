/* indent.c */
int cin_is_cinword(char_u *line);
pos_T *find_start_comment(int ind_maxcomment);
int cindent_on(void);
void parse_cino(buf_T *buf);
int get_c_indent(void);
int get_expr_indent(void);
int in_cinkeys(int keytyped, int when, int line_is_empty);
int get_lisp_indent(void);
void do_c_expr_indent(void);
void fixthisline(int (*get_the_indent)(void));
void fix_indent(void);
int tabstop_set(char_u *var, int **array);
int tabstop_padding(colnr_T col, int ts_arg, int *vts);
int tabstop_at(colnr_T col, int ts, int *vts);
colnr_T tabstop_start(colnr_T col, int ts, int *vts);
void tabstop_fromto(colnr_T start_col, colnr_T end_col, int ts_arg, int *vts, int *ntabs, int *nspcs);
int tabstop_eq(int *ts1, int *ts2);
int *tabstop_copy(int *oldts);
int tabstop_count(int *ts);
int tabstop_first(int *ts);
long get_sw_value(buf_T *buf);
long get_sw_value_indent(buf_T *buf);
long get_sw_value_col(buf_T *buf, colnr_T col);
long get_sts_value(void);
/* vim: set ft=c : */
