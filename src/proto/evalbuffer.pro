/* evalbuffer.c */
int set_ref_in_buffers(int copyID);
buf_T * buflist_find_by_name(char_u *name, int curtab_only);
buf_T * find_buffer(typval_T *avar);
void f_append(typval_T *argvars, typval_T *rettv);
void f_appendbufline(typval_T *argvars, typval_T *rettv);
void f_bufadd(typval_T *argvars, typval_T *rettv);
void f_bufexists(typval_T *argvars, typval_T *rettv);
void f_buflisted(typval_T *argvars, typval_T *rettv);
void f_bufload(typval_T *argvars, typval_T *rettv);
void f_bufloaded(typval_T *argvars, typval_T *rettv);
void f_bufname(typval_T *argvars, typval_T *rettv);
void f_bufnr(typval_T *argvars, typval_T *rettv);
void f_bufwinid(typval_T *argvars, typval_T *rettv);
void f_bufwinnr(typval_T *argvars, typval_T *rettv);
void f_deletebufline(typval_T *argvars, typval_T *rettv);
void f_getbufinfo(typval_T *argvars, typval_T *rettv);
void f_getbufline(typval_T *argvars, typval_T *rettv);
void f_getline(typval_T *argvars, typval_T *rettv);
void f_setbufline(typval_T *argvars, typval_T *rettv);
void f_setline(typval_T *argvars, typval_T *rettv);
/* vim: set ft=c : */

