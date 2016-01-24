/* channel.c */
int channel_add_netbeans(sock_T fd);
void channel_remove_netbeans(void);
int channel_poll_setup(int nfd_in, void *fds_in);
int channel_poll_check(int ret_in, void *fds_in);
int channel_select_setup(int maxfd_in, void *rfds_in);
int channel_select_check(int ret_in, void *rfds_in);
/* vim: set ft=c : */
