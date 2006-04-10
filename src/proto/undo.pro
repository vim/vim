/* undo.c */
extern int u_save_cursor __ARGS((void));
extern int u_save __ARGS((linenr_T top, linenr_T bot));
extern int u_savesub __ARGS((linenr_T lnum));
extern int u_inssub __ARGS((linenr_T lnum));
extern int u_savedel __ARGS((linenr_T lnum, long nlines));
extern void u_undo __ARGS((int count));
extern void u_redo __ARGS((int count));
extern void undo_time __ARGS((long step, int sec, int absolute));
extern void u_sync __ARGS((int force));
extern void ex_undolist __ARGS((exarg_T *eap));
extern void ex_undojoin __ARGS((exarg_T *eap));
extern void u_unchanged __ARGS((buf_T *buf));
extern void u_clearall __ARGS((buf_T *buf));
extern void u_saveline __ARGS((linenr_T lnum));
extern void u_clearline __ARGS((void));
extern void u_undoline __ARGS((void));
extern void u_blockfree __ARGS((buf_T *buf));
extern int bufIsChanged __ARGS((buf_T *buf));
extern int curbufIsChanged __ARGS((void));
/* vim: set ft=c : */
