/* viminfo.c */
int read_viminfo_bufferlist(vir_T *virp, int writing);
void write_viminfo_bufferlist(FILE *fp);
int viminfo_error(char *errnum, char *message, char_u *line);
int read_viminfo(char_u *file, int flags);
void write_viminfo(char_u *file, int forceit);
int read_viminfo_sub_string(vir_T *virp, int force);
void write_viminfo_sub_string(FILE *fp);
int viminfo_readline(vir_T *virp);
char_u *viminfo_readstring(vir_T *virp, int off, int convert);
void viminfo_writestring(FILE *fd, char_u *p);
int barline_writestring(FILE *fd, char_u *s, int remaining_start);
void ex_viminfo(exarg_T *eap);
int read_viminfo_varlist(vir_T *virp, int writing);
void write_viminfo_varlist(FILE *fp);
/* vim: set ft=c : */
