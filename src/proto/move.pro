/* move.c */
int adjust_plines_for_skipcol(win_T *wp);
int sms_marker_overlap(win_T *wp, int extra2);
void update_topline_redraw(void);
void update_topline(void);
void update_curswant_force(void);
void update_curswant(void);
void check_cursor_moved(win_T *wp);
void changed_window_setting(void);
void changed_window_setting_win(win_T *wp);
void changed_window_setting_buf(buf_T *buf);
void set_topline(win_T *wp, linenr_T lnum);
void changed_cline_bef_curs(void);
void changed_cline_bef_curs_win(win_T *wp);
void changed_line_abv_curs(void);
void changed_line_abv_curs_win(win_T *wp);
void changed_line_display_buf(buf_T *buf);
void validate_botline(void);
void validate_botline_win(win_T *wp);
void invalidate_botline(void);
void invalidate_botline_win(win_T *wp);
void approximate_botline_win(win_T *wp);
int cursor_valid(void);
void validate_cursor(void);
void validate_cline_row(void);
void validate_virtcol(void);
void validate_virtcol_win(win_T *wp);
void validate_cheight(void);
void validate_cursor_col(void);
int win_col_off(win_T *wp);
int curwin_col_off(void);
int win_col_off2(win_T *wp);
int curwin_col_off2(void);
void curs_columns(int may_scroll);
void textpos2screenpos(win_T *wp, pos_T *pos, int *rowp, int *scolp, int *ccolp, int *ecolp);
void f_screenpos(typval_T *argvars, typval_T *rettv);
void f_virtcol2col(typval_T *argvars, typval_T *rettv);
void scrolldown(long line_count, int byfold);
void scrollup(long line_count, int byfold);
void scroll_redraw(int up, long count);
void adjust_skipcol(void);
void check_topfill(win_T *wp, int down);
void scrolldown_clamp(void);
void scrollup_clamp(void);
void scroll_cursor_top(int min_scroll, int always);
void set_empty_rows(win_T *wp, int used);
void scroll_cursor_bot(int min_scroll, int set_topbot);
void scroll_cursor_halfway(int atend, int prefer_above);
void cursor_correct(void);
int pagescroll(int dir, long count, int half);
void do_check_cursorbind(void);
/* vim: set ft=c : */
