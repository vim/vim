/* channel.c */
void channel_gui_register_all(void);
int channel_open(char *hostname, int port_in, void (*close_cb)(void));
int channel_is_open(int idx);
void channel_close(int idx);
void channel_save(int idx, char_u *buf, int len);
char_u *channel_peek(int idx);
char_u *channel_get(int idx);
int channel_collapse(int idx);
void channel_clear(int idx);
void channel_read(int idx);
int channel_socket2idx(sock_T fd);
void channel_send(int idx, char_u *buf, char *fun);
int channel_poll_setup(int nfd_in, void *fds_in);
int channel_poll_check(int ret_in, void *fds_in);
int channel_select_setup(int maxfd_in, void *rfds_in);
int channel_select_check(int ret_in, void *rfds_in);
/* vim: set ft=c : */
