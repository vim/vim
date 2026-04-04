/* trust.c */
int trust_check_dir(char_u *dir, char_u *permission);
int trust_prompt(char_u *dir, char_u *permission);
int has_modeline(buf_T *buf);
char_u *trust_get_buf_dir(buf_T *buf);
void trust_clear_session(void);
