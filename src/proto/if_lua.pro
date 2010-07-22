/* if_lua.c */
int lua_enabled __ARGS((int verbose));
void lua_end __ARGS((void));
void ex_lua __ARGS((exarg_T *eap));
void ex_luado __ARGS((exarg_T *eap));
void ex_luafile __ARGS((exarg_T *eap));
void lua_buffer_free __ARGS((buf_T *buf));
void lua_window_free __ARGS((win_T *win));
/* vim: set ft=c : */
