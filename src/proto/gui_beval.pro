/* gui_beval.c */
extern void general_beval_cb __ARGS((BalloonEval *beval, int state));
extern BalloonEval *gui_mch_create_beval_area __ARGS((void *target, char_u *mesg, void (*mesgCB)(BalloonEval *, int), void *clientData));
extern void gui_mch_destroy_beval_area __ARGS((BalloonEval *beval));
extern void gui_mch_enable_beval_area __ARGS((BalloonEval *beval));
extern void gui_mch_disable_beval_area __ARGS((BalloonEval *beval));
extern BalloonEval *gui_mch_currently_showing_beval __ARGS((void));
extern int get_beval_info __ARGS((BalloonEval *beval, int getword, win_T **winp, linenr_T *lnump, char_u **textp, int *colp));
extern void gui_mch_post_balloon __ARGS((BalloonEval *beval, char_u *mesg));
extern void gui_mch_unpost_balloon __ARGS((BalloonEval *beval));
/* vim: set ft=c : */
