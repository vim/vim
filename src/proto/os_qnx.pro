/* os_qnx.c */
void qnx_init __ARGS((void));
void qnx_clip_init __ARGS((void));
int clip_mch_own_selection __ARGS((VimClipboard *cbd));
void clip_mch_lose_selection __ARGS((VimClipboard *cbd));
void clip_mch_request_selection __ARGS((VimClipboard *cbd));
void clip_mch_set_selection __ARGS((VimClipboard *cbd));
/* vim: set ft=c : */
