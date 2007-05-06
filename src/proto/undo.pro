/* undo.c */
int u_save_cursor __ARGS((void));
int u_save __ARGS((linenr_T top, linenr_T bot));
int u_savesub __ARGS((linenr_T lnum));
int u_inssub __ARGS((linenr_T lnum));
int u_savedel __ARGS((linenr_T lnum, long nlines));
int undo_allowed __ARGS((void));
void u_undo __ARGS((int count));
void u_redo __ARGS((int count));
void undo_time __ARGS((long step, int sec, int absolute));
void u_sync __ARGS((int force));
void ex_undolist __ARGS((exarg_T *eap));
void ex_undojoin __ARGS((exarg_T *eap));
void u_unchanged __ARGS((buf_T *buf));
void u_clearall __ARGS((buf_T *buf));
void u_saveline __ARGS((linenr_T lnum));
void u_clearline __ARGS((void));
void u_undoline __ARGS((void));
void u_blockfree __ARGS((buf_T *buf));
int bufIsChanged __ARGS((buf_T *buf));
int curbufIsChanged __ARGS((void));
/* vim: set ft=c : */
