/* cmdexpand.c */
int cmdline_fuzzy_complete(char_u *fuzzystr);
int nextwild(expand_T *xp, int type, int options, int escape);
void cmdline_pum_display(void);
int cmdline_pum_active(void);
void cmdline_pum_remove(cmdline_info_T *cclp, int defer_redraw);
void cmdline_pum_cleanup(cmdline_info_T *cclp);
int cmdline_compl_startcol(void);
char_u *cmdline_compl_pattern(void);
int cmdline_compl_is_fuzzy(void);
char_u *ExpandOne(expand_T *xp, char_u *str, char_u *orig, int options, int mode);
void ExpandInit(expand_T *xp);
void ExpandCleanup(expand_T *xp);
void clear_cmdline_orig(void);
int showmatches(expand_T *xp, int display_wildmenu, int display_list, int noselect);
char_u *addstar(char_u *fname, int len, int context);
void set_expand_context(expand_T *xp);
void set_cmd_context(expand_T *xp, char_u *str, int len, int col, int use_ccline);
int expand_cmdline(expand_T *xp, char_u *str, int col, int *matchcount, char_u ***matches);
int ExpandGeneric(char_u *pat, expand_T *xp, regmatch_T *regmatch, char_u ***matches, int *numMatches, char_u *(*func)(expand_T *, int), int escaped);
int ExpandGenericExt(char_u *pat, expand_T *xp, regmatch_T *regmatch, char_u ***matches, int *numMatches, char_u *(*func)(expand_T *, int), int escaped, int sortStartIdx);
void globpath(char_u *path, char_u *file, garray_T *ga, int expand_options, int dirs);
int wildmenu_translate_key(cmdline_info_T *cclp, int key, expand_T *xp, int did_wild_list);
int wildmenu_process_key(cmdline_info_T *cclp, int key, expand_T *xp);
void wildmenu_cleanup(cmdline_info_T *cclp);
void f_getcompletion(typval_T *argvars, typval_T *rettv);
void f_getcompletiontype(typval_T *argvars, typval_T *rettv);
void f_cmdcomplete_info(typval_T *argvars, typval_T *rettv);
/* vim: set ft=c : */
