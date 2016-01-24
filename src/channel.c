/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * Implements communication through a socket or any file handle.
 */

#include "vim.h"

#if defined(FEAT_CHANNEL) || defined(PROTO)

typedef struct {
    sock_T  ch_fd;
    int	    ch_idx;
} channel_T;

static channel_T *channels = NULL;
static int channel_count = 0;

/*
 * Add a new channel slot, return the index.
 * Returns -1 if out of space.
 */
    static int
add_channel(void)
{
    int		idx;
    channel_T	*new_channels;

    if (channels != NULL)
	for (idx = 0; idx < channel_count; ++idx)
	    if (channels[idx].ch_fd < 0)
		/* re-use a closed channel slot */
		return idx;
    if (channel_count == MAX_OPEN_CHANNELS)
	return -1;
    new_channels = (channel_T *)alloc(sizeof(channel_T) * channel_count + 1);
    if (new_channels == NULL)
	return -1;
    if (channels != NULL)
	mch_memmove(new_channels, channels, sizeof(channel_T) * channel_count);
    channels = new_channels;
    channels[channel_count].ch_fd = (sock_T)-1;

    return channel_count++;
}

#if defined(FEAT_NETBEANS_INTG) || defined(PROTO)
static int netbeans_channel = -1;

/*
 * Add the netbeans socket to the channels.
 * Return the channel index.
 */
    int
channel_add_netbeans(sock_T fd)
{
    int idx = add_channel();

    if (idx >= 0)
    {
	channels[idx].ch_fd = fd;
	netbeans_channel = idx;
    }
    return idx;
}

    void
channel_remove_netbeans()
{
    channels[netbeans_channel].ch_fd = (sock_T)-1;
    netbeans_channel = -1;
}
#endif

    static void
channel_read(int idx)
{
# ifdef FEAT_NETBEANS_INTG
    if (idx == netbeans_channel)
	netbeans_read();
    else
# endif
    {
	; /* TODO: read */
    }
}

#if (defined(UNIX) && !defined(HAVE_SELECT)) || defined(PROTO)
/*
 * Add open channels to the poll struct.
 * Return the adjusted struct index.
 * The type of "fds" is hidden to avoid problems with the function proto.
 */
    int
channel_poll_setup(int nfd_in, void *fds_in)
{
    int nfd = nfd_in;
    int i;
    struct pollfd *fds = fds_in;

    for (i = 0; i < channel_count; ++i)
	if (channels[i].ch_fd >= 0)
	{
	    channels[i].ch_idx = nfd;
	    fds[nfd].fd = channels[i].ch_fd;
	    fds[nfd].events = POLLIN;
	    nfd++;
	}
	else
	    channels[i].ch_idx = -1;

    return nfd;
}

/*
 * The type of "fds" is hidden to avoid problems with the function proto.
 */
    int
channel_poll_check(int ret_in, void *fds_in)
{
    int ret = ret_in;
    int i;
    struct pollfd *fds = fds_in;

    for (i = 0; i < channel_count; ++i)
	if (ret > 0 && channels[i].ch_idx != -1
				 && fds[channels[i].ch_idx].revents & POLLIN)
	{
	    channel_read(i);
	    --ret;
	}

    return ret;
}
#endif /* UNIX && !HAVE_SELECT */

#if (defined(UNIX) && defined(HAVE_SELECT)) || defined(PROTO)
/*
 * The type of "rfds" is hidden to avoid problems with the function proto.
 */
    int
channel_select_setup(int maxfd_in, void *rfds_in)
{
    int	    maxfd = maxfd_in;
    int	    i;
    fd_set  *rfds = rfds_in;

    for (i = 0; i < channel_count; ++i)
	if (channels[i].ch_fd >= 0)
	{
	    FD_SET(channels[i].ch_fd, rfds);
	    if (maxfd < channels[i].ch_fd)
		maxfd = channels[i].ch_fd;
	}

    return maxfd;
}

/*
 * The type of "rfds" is hidden to avoid problems with the function proto.
 */
    int
channel_select_check(int ret_in, void *rfds_in)
{
    int	    ret = ret_in;
    int	    i;
    fd_set  *rfds = rfds_in;

    for (i = 0; i < channel_count; ++i)
	if (ret > 0 && channels[i].ch_fd >= 0
				       && FD_ISSET(channels[i].ch_fd, rfds))
	{
	    channel_read(i);
	    --ret;
	}

    return ret;
}
#endif /* UNIX && HAVE_SELECT */

#endif /* FEAT_CHANNEL */
