/* beval.c */
int get_beval_info(BalloonEval *beval, int getword, win_T **winp, linenr_T *lnump, char_u **textp, int *colp);
void post_balloon(BalloonEval *beval, char_u *mesg, list_T *list);
int can_use_beval(void);
void general_beval_cb(BalloonEval *beval, int state);
/* vim: set ft=c : */
