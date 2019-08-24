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
/* vim: set ft=c : */
