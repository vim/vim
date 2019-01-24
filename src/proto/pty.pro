/* pty.c */
int setup_slavepty(int fd);
int mch_openpty(char **ttyn);
int mch_isatty(int fd);
int mch_tcgetattr(int fd, void *term);
/* vim: set ft=c : */
