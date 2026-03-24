/* socketserver.c */
int socketserver_start(char_u *name, bool quiet);
void socketserver_stop(void);
int set_ref_in_socketserver_channel(int copyID);
void socketserver_parse_messages(void);
int socketserver_send(char_u *name, char_u *str, char_u **result, bool is_expr, int timeout, bool silent);
int socketserver_send_reply(char_u *client, char_u *str);
int socketserver_read_reply(char_u *client, char_u **str, int timeout);
int socketserver_peek_reply(char_u *sender, char_u **str);
/* vim: set ft=c : */
