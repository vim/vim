/* gui_beval.c */
BalloonEval *gui_mch_create_beval_area __ARGS((void *target, char_u *mesg, void (*mesgCB)(BalloonEval *, int), void *clientData));
void gui_mch_destroy_beval_area __ARGS((BalloonEval *beval));
void gui_mch_enable_beval_area __ARGS((BalloonEval *beval));
void gui_mch_disable_beval_area __ARGS((BalloonEval *beval));
BalloonEval *gui_mch_currently_showing_beval __ARGS((void));
int gui_mch_get_beval_info __ARGS((BalloonEval *beval, char_u **filename, int *line, char_u **text, int *idx));
void gui_mch_post_balloon __ARGS((BalloonEval *beval, char_u *mesg));
void gui_mch_unpost_balloon __ARGS((BalloonEval *beval));
/* vim: set ft=c : */
