/* netbeans.c */
void netbeans_parse_messages __ARGS((void));
void messageFromNetbeansW32 __ARGS((void));
int isNetbeansBuffer __ARGS((buf_T *bufp));
int isNetbeansModified __ARGS((buf_T *bufp));
void netbeans_end __ARGS((void));
void ex_nbkey __ARGS((exarg_T *eap));
void netbeans_beval_cb __ARGS((BalloonEval *beval, int state));
void netbeans_startup_done __ARGS((void));
void netbeans_send_disconnect __ARGS((void));
void netbeans_frame_moved __ARGS((int new_x, int new_y));
void netbeans_file_activated __ARGS((buf_T *bufp));
void netbeans_file_opened __ARGS((buf_T *bufp));
void netbeans_file_closed __ARGS((buf_T *bufp));
void netbeans_inserted __ARGS((buf_T *bufp, linenr_T linenr, colnr_T col, char_u *txt, int newlen));
void netbeans_removed __ARGS((buf_T *bufp, linenr_T linenr, colnr_T col, long len));
void netbeans_unmodified __ARGS((buf_T *bufp));
void netbeans_button_release __ARGS((int button));
void netbeans_keycommand __ARGS((int key));
void netbeans_save_buffer __ARGS((buf_T *bufp));
void netbeans_deleted_all_lines __ARGS((buf_T *bufp));
int netbeans_is_guarded __ARGS((linenr_T top, linenr_T bot));
void netbeans_draw_multisign_indicator __ARGS((int row));
void netbeans_draw_multisign_indicator __ARGS((int row));
void netbeans_gutter_click __ARGS((linenr_T lnum));
/* vim: set ft=c : */
