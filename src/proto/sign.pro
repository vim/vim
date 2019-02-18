/* sign.c */
void init_signs(void);
int buf_getsigntype(buf_T *buf, linenr_T lnum, int type);
linenr_T buf_delsign(buf_T *buf, linenr_T atlnum, int id, char_u *group);
int buf_findsign(buf_T *buf, int id, char_u *group);
int buf_findsign_id(buf_T *buf, linenr_T lnum, char_u *groupname);
int buf_findsigntype_id(buf_T *buf, linenr_T lnum, int typenr);
int buf_signcount(buf_T *buf, linenr_T lnum);
void buf_delete_signs(buf_T *buf, char_u *group);
void sign_mark_adjust(linenr_T line1, linenr_T line2, long amount, long amount_after);
int sign_define_by_name(char_u *name, char_u *icon, char_u *linehl, char_u *text, char_u *texthl);
int sign_undefine_by_name(char_u *name);
int sign_place(int *sign_id, char_u *sign_group, char_u *sign_name, buf_T *buf, linenr_T lnum, int prio);
int sign_unplace(int sign_id, char_u *sign_group, buf_T *buf, linenr_T atlnum);
linenr_T sign_jump(int sign_id, char_u *sign_group, buf_T *buf);
void ex_sign(exarg_T *eap);
void sign_getlist(char_u *name, list_T *retlist);
void get_buffer_signs(buf_T *buf, list_T *l);
void sign_get_placed(buf_T *buf, linenr_T lnum, int sign_id, char_u *sign_group, list_T *retlist);
void sign_gui_started(void);
int sign_get_attr(int typenr, int line);
char_u *sign_get_text(int typenr);
void *sign_get_image(int typenr);
void free_signs(void);
char_u *get_sign_name(expand_T *xp, int idx);
void set_context_in_sign_cmd(expand_T *xp, char_u *arg);
/* vim: set ft=c : */
