/* socketserver.c */
int socketserver_start(char_u *name, bool quiet);
void socketserver_stop(void);
int set_ref_in_socketserver_channel(int copyID);
void socketserver_parse_messages(void);
int socketserver_send(char_u *name, char_u *str, char_u **result, int is_expr, int timeout, int silent);
/* vim: set ft=c : */
