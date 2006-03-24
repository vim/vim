/* termlib.c */
extern int tgetent __ARGS((char *tbuf, char *term));
extern int tgetflag __ARGS((char *id));
extern int tgetnum __ARGS((char *id));
extern char *tgetstr __ARGS((char *id, char **buf));
extern char *tgoto __ARGS((char *cm, int col, int line));
extern int tputs __ARGS((char *cp, int affcnt, void (*outc)(unsigned int)));
/* vim: set ft=c : */
