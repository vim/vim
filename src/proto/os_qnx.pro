/* os_qnx.c */
void qnx_init(void);
void qnx_clip_init (void);
int clip_mch_own_selection(VimClipboard *cbd);
void clip_mch_lose_selection(VimClipboard *cbd);
void clip_mch_request_selection(VimClipboard *cbd);
void clip_mch_set_selection(VimClipboard *cbd);
/* vim: set ft=c : */
