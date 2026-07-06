/* drawline.c */
int text_prop_position(win_T *wp, textprop_T *tp, int vcol, int scr_col, int *n_extra, char_u **p_extra, int *n_attr, int *n_attr_skip, int do_skip);
int text_prop_no_showbreak(textprop_T *tp);
int win_line(win_T *wp, linenr_T lnum, int startrow, int endrow, int number_only, spellvars_T *spv);
int win_line_conceal_screenline_iter(win_T *wp, linenr_T lnum, conceal_screenline_cb_T cb, void *cb_ctx, bool *has_concealp, int *rowsp);
/* vim: set ft=c : */
