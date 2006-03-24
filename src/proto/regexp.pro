/* regexp.c */
extern void free_regexp_stuff __ARGS((void));
extern int re_multiline __ARGS((regprog_T *prog));
extern int re_lookbehind __ARGS((regprog_T *prog));
extern char_u *skip_regexp __ARGS((char_u *startp, int dirc, int magic, char_u **newp));
extern regprog_T *vim_regcomp __ARGS((char_u *expr, int re_flags));
extern int vim_regcomp_had_eol __ARGS((void));
extern int vim_regexec __ARGS((regmatch_T *rmp, char_u *line, colnr_T col));
extern int vim_regexec_nl __ARGS((regmatch_T *rmp, char_u *line, colnr_T col));
extern long vim_regexec_multi __ARGS((regmmatch_T *rmp, win_T *win, buf_T *buf, linenr_T lnum, colnr_T col));
extern reg_extmatch_T *ref_extmatch __ARGS((reg_extmatch_T *em));
extern void unref_extmatch __ARGS((reg_extmatch_T *em));
extern char_u *regtilde __ARGS((char_u *source, int magic));
extern int vim_regsub __ARGS((regmatch_T *rmp, char_u *source, char_u *dest, int copy, int magic, int backslash));
extern int vim_regsub_multi __ARGS((regmmatch_T *rmp, linenr_T lnum, char_u *source, char_u *dest, int copy, int magic, int backslash));
extern char_u *reg_submatch __ARGS((int no));
/* vim: set ft=c : */
