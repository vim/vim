/* socketserver.c */
int socketserver_start(char_u *name, bool quiet);
void socketserver_stop(void);
int set_ref_in_socketserver_channel(int copyID);
void socketserver_parse_messages(void);
/* vim: set ft=c : */
