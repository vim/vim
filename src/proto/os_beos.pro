/* os_beos.c */
void beos_cleanup_read_thread(void);
int beos_select(int nbits, struct fd_set *rbits, struct fd_set *wbits, struct fd_set *ebits, struct timeval *timeout);
/* vim: set ft=c : */
