/* diff.c */
extern void diff_buf_delete __ARGS((buf_T *buf));
extern void diff_buf_adjust __ARGS((win_T *win));
extern void diff_buf_add __ARGS((buf_T *buf));
extern void diff_invalidate __ARGS((buf_T *buf));
extern void diff_mark_adjust __ARGS((linenr_T line1, linenr_T line2, long amount, long amount_after));
extern void ex_diffupdate __ARGS((exarg_T *eap));
extern void ex_diffpatch __ARGS((exarg_T *eap));
extern void ex_diffsplit __ARGS((exarg_T *eap));
extern void ex_diffthis __ARGS((exarg_T *eap));
extern void diff_win_options __ARGS((win_T *wp, int addbuf));
extern void ex_diffoff __ARGS((exarg_T *eap));
extern void diff_clear __ARGS((tabpage_T *tp));
extern int diff_check __ARGS((win_T *wp, linenr_T lnum));
extern int diff_check_fill __ARGS((win_T *wp, linenr_T lnum));
extern void diff_set_topline __ARGS((win_T *fromwin, win_T *towin));
extern int diffopt_changed __ARGS((void));
extern int diffopt_horizontal __ARGS((void));
extern int diff_find_change __ARGS((win_T *wp, linenr_T lnum, int *startp, int *endp));
extern int diff_infold __ARGS((win_T *wp, linenr_T lnum));
extern void nv_diffgetput __ARGS((int put));
extern void ex_diffgetput __ARGS((exarg_T *eap));
extern int diff_mode_buf __ARGS((buf_T *buf));
extern int diff_move_to __ARGS((int dir, long count));
extern linenr_T diff_lnum_win __ARGS((linenr_T lnum, win_T *wp));
/* vim: set ft=c : */
