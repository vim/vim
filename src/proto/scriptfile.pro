/* runtime.c */
void ex_runtime(exarg_T *eap);
int do_in_path(char_u *path, char_u *name, int flags, void (*callback)(char_u *fname, void *ck), void *cookie);
int do_in_runtimepath(char_u *name, int flags, void (*callback)(char_u *fname, void *ck), void *cookie);
int source_runtime(char_u *name, int flags);
int source_in_path(char_u *path, char_u *name, int flags);
void add_pack_start_dirs(void);
void load_start_packages(void);
void ex_packloadall(exarg_T *eap);
void ex_packadd(exarg_T *eap);
int ExpandRTDir(char_u *pat, int flags, int *num_file, char_u ***file, char *dirname[]);
int ExpandPackAddDir(char_u *pat, int *num_file, char_u ***file);
void ex_source(exarg_T *eap);
void cmd_source(char_u *fname, exarg_T *eap);
linenr_T *source_breakpoint(void *cookie);
int *source_dbg_tick(void *cookie);
int source_level(void *cookie);
int do_source(char_u *fname, int check_other, int is_vimrc);
void ex_scriptnames(exarg_T *eap);
void scriptnames_slash_adjust(void);
char_u *get_scriptname(scid_T id);
void free_scriptnames(void);
linenr_T get_sourced_lnum(char_u *(*fgetline)(int, void *, int, int), void *cookie);
char_u *getsourceline(int c, void *cookie, int indent, int do_concat);
void ex_scriptencoding(exarg_T *eap);
void ex_scriptversion(exarg_T *eap);
void ex_finish(exarg_T *eap);
void do_finish(exarg_T *eap, int reanimate);
int source_finished(char_u *(*fgetline)(int, void *, int, int), void *cookie);
/* vim: set ft=c : */
