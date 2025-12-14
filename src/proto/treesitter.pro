/* treesitter.c */
int tsvim_init(void);
opaque_type_T *tsvim_lookup_opaque_type(char_u *name, size_t namelen);
void f_ts_load(typval_T *argvars, typval_T *rettv);
void f_tsparser_new(typval_T *argvars, typval_T *rettv);
void f_tsparser_set_language(typval_T *argvars, typval_T *rettv);
void f_tsparser_parse_buf(typval_T *argvars, typval_T *rettv);
void f_tsparser_set_included_ranges(typval_T *argvars, typval_T *rettv);
void f_tstree_edit(typval_T *argvars, typval_T *rettv);
void f_tstree_root_node(typval_T *argvars, typval_T *rettv);
void f_tsnode_child(typval_T *argvars, typval_T *rettv);
void f_tsnode_descendant_for_range(typval_T *argvars, typval_T *rettv);
void f_tsquery_new(typval_T *argvars, typval_T *rettv);
void f_tsquery_disable_capture(typval_T *argvars, typval_T *rettv);
void f_tsquery_disable_pattern(typval_T *argvars, typval_T *rettv);
void f_tsquery_inspect(typval_T *argvars, typval_T *rettv);
void f_tsquerycursor_new(typval_T *argvars, typval_T *rettv);
void f_tsquerycursor_exec(typval_T *argvars, typval_T *rettv);
void f_tsquerycursor_next_match(typval_T *argvars, typval_T *rettv);
/* vim: set ft=c : */
