/* gui_beval.c */
void general_beval_cb(BalloonEval *beval, int state);
BalloonEval *gui_mch_create_beval_area(void *target, char_u *mesg, void (*mesgCB)(BalloonEval *, int), void *clientData);
void gui_mch_destroy_beval_area(BalloonEval *beval);
void gui_mch_enable_beval_area(BalloonEval *beval);
void gui_mch_disable_beval_area(BalloonEval *beval);
BalloonEval *gui_mch_currently_showing_beval(void);
int get_beval_info(BalloonEval *beval, int getword, win_T **winp, linenr_T *lnump, char_u **textp, int *colp);
void gui_mch_post_balloon(BalloonEval *beval, char_u *mesg);
void gui_mch_unpost_balloon(BalloonEval *beval);
/* vim: set ft=c : */
