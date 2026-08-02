/* hardcopy.c */
char *parse_printoptions(optset_T *args);
char *parse_printmbfont(optset_T *args);
int prt_header_height(void);
int prt_use_number(void);
int prt_get_unit(int idx);
void prt_message(char_u *s);
void ex_hardcopy(exarg_T *eap);
/* vim: set ft=c : */
