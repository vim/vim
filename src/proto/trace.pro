/* trace.c */
char_u *trace_format_input(int c);
char_u *trace_format_command(cmdarg_T *ca);
char_u *trace_format_ex(exarg_T *ea);
char_u *trace_format_mapping(mapblock_T *mp);
char_u *trace_format_typebuf(char_u *buf, size_t buflen);
int trace_is_enabled(trace_event_kind_T kind);
int trace_verbose(trace_verbosity_T level);
int trace_is_active(void);
void trace_apply_opt(char_u *value);
void trace_init(void);
trace_entry_T *trace_get_recent(size_t idx);
void trace_ingest(char_u *buf);
void trace_clear_all(void);
int trace_resize_ring(size_t new_size);
void trace_dump_range(size_t start, size_t end);
void f_ch_traceget(typval_T *argvars, typval_T *rettv);
void f_ch_traceclear(typval_T *argvars, typval_T *rettv);
/* vim: set ft=c : */
