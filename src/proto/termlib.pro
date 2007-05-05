/* termlib.c */
int tgetent __ARGS((char *tbuf, char *term));
int tgetflag __ARGS((char *id));
int tgetnum __ARGS((char *id));
char *tgetstr __ARGS((char *id, char **buf));
char *tgoto __ARGS((char *cm, int col, int line));
int tputs __ARGS((char *cp, int affcnt, void (*outc)(unsigned int)));
/* vim: set ft=c : */
