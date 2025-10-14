/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * wayland.c: Stuff related to Wayland. Functions that prefixed with "vwl_"
 *	      handle/provide abstractions and building blocks to create more
 *	      complex things. The "wayland_" functions handle the global
 *	      Wayland connection.
 *
 *	      At the end of this file, there are a bunch of macro definitions
 *	      that abstract away all the different protocols for the clipboard
 *	      we need to support under one single interface. This is then used
 *	      by clipboard.c to implement the Wayland clipboard functionality.
 *
 *	      The clipboard functionality monitors the Wayland display at all
 *	      times, and saves new selections/offers as events come in. When we
 *	      want retrieve the selection, the currently saved data offer is
 *	      used from the respective data device.
 *
 *	      The focus stealing code is implemented in clipboard.c, and is
 *	      based off of wl-clipboard's implementation. The idea using of
 *	      extensive macros to reduce boilerplate code also comes from
 *	      wl-clipboard as well. The project page for wl-clipboard can be
 *	      found here: https://github.com/bugaevc/wl-clipboard
 */

#include "vim.h"

#ifdef FEAT_WAYLAND

#include "wayland.h"

vwl_connection_T    *wayland_ct;
bool		    is_reading = false;

/*
 * Like wl_display_flush but always writes all the data in the buffer to the
 * display fd. Returns FAIL on failure and OK on success.
 */
    int
vwl_connection_flush(vwl_connection_T *self)
{
#ifndef HAVE_SELECT
    struct pollfd fds;

    fds.fd = self->display.fd;
    fds.events = POLLOUT;
#else
    fd_set	    wfds;
    struct timeval  tv;

    FD_ZERO(&wfds);
    FD_SET(self->display.fd, &wfds);
#endif

    if (self->display.proxy == NULL)
	return FAIL;

    // Send the requests we have made to the compositor, until we have written
    // all the data. Poll in order to check if the display fd is writable, if
    // not, then wait until it is and continue writing or until we timeout.
    while (true)
    {
	int ret = wl_display_flush(self->display.proxy);

	if (ret == -1 && errno == EAGAIN)
	{
#ifndef HAVE_SELECT
	    if (poll(&fds, 1, p_wtm) <= 0)
#else
	    tv.tv_sec = p_wtm / 1000;
	    tv.tv_usec = (p_wtm % 1000) * 1000;
	    if (select(self->display.fd + 1, NULL, &wfds, NULL, &tv) <= 0)
		return FAIL;
#endif
	}
	else if (ret == -1)
	    return FAIL;
	else
	    break;
    }

    return OK;
}

/*
 * Like wl_display_roundtrip but polls the display fd with a timeout. Returns
 * number of events dispatched on success else -1 on failure.
 */
    int
vwl_connection_dispatch(vwl_connection_T *self)
{
#ifndef HAVE_SELECT
    struct pollfd   fds;

    fds.fd = self->display.fd;
    fds.events = POLLIN;
#else
    fd_set	    rfds;
    struct timeval  tv;

    FD_ZERO(&rfds);
    FD_SET(self->display.fd, &rfds);
#endif

    if (self->display.proxy == NULL)
	return -1;

    while (wl_display_prepare_read(self->display.proxy) == -1)
	// Dispatch any queued events so that we can start reading
	if (wl_display_dispatch_pending(self->display.proxy) == -1)
	    return -1;

    // Send any requests before we starting blocking to read display fd
    if (vwl_connection_flush(self) == FAIL)
    {
	wl_display_cancel_read(self->display.proxy);
	return -1;
    }

    // Poll until there is data to read from the display fd.
#ifndef HAVE_SELECT
    if (poll(&fds, 1, p_wtm) <= 0)
#else
    tv.tv_sec = p_wtm / 1000;
    tv.tv_usec = (p_wtm % 1000) * 1000;
    if (select(self->display.fd + 1, &rfds, NULL, NULL, &tv) <= 0)
#endif
    {
	wl_display_cancel_read(self->display.proxy);
	return -1;
    }

    // Read events into the queue
    if (wl_display_read_events(self->display.proxy) == -1)
	// No need to cancel
	return -1;

    // Dispatch those events (call the handlers associated for each event)
    return wl_display_dispatch_pending(self->display.proxy);
}

/*
 * Called when compositor is done processing requests/events.
 */
    static void
vwl_callback_event_done(void *data, struct wl_callback *callback,
	uint32_t callback_data UNUSED)
{
    *((bool*)data) = true;
    wl_callback_destroy(callback);
}

static const struct wl_callback_listener vwl_callback_listener = {
    .done = vwl_callback_event_done
};

/*
 * Like wl_display_roundtrip but polls the display fd with a timeout. Returns
 * FAIL on failure and OK on success.
 */
    int
vwl_connection_roundtrip(vwl_connection_T *self)
{
    struct wl_callback	*callback;
    int			ret;
    bool		done = false;
#ifdef ELAPSED_FUNC
    elapsed_T		start_tv;
#endif

    if (self->display.proxy == NULL)
	return FAIL;

    // Tell compositor to emit 'done' event after processing all requests we
    // have sent and handling events.
    callback = wl_display_sync(self->display.proxy);

    if (callback == NULL)
	return FAIL;

    wl_callback_add_listener(callback, &vwl_callback_listener, &done);

#ifdef ELAPSED_FUNC
    ELAPSED_INIT(start_tv);
#endif

    // Wait till we get the done event (which will set `done` to TRUE), unless
    // we timeout
    while (true)
    {
	ret = vwl_connection_dispatch(self);

	if (done || ret == -1)
	    break;

#ifdef ELAPSED_FUNC
	if (ELAPSED_FUNC(start_tv) >= p_wtm)
	{
	    ret = -1;
	    break;
	}
#endif
    }

    if (ret == -1)
    {
	if (!done)
	    wl_callback_destroy(callback);
	return FAIL;
    }

    return OK;
}

/*
 * Redirect libwayland logging to use ch_log + emsg instead.
 */
    static void
vwl_log_handler(const char *fmt, va_list args)
{
    // 512 bytes should be big enough
    char	*buf	= alloc(512);
    char	*prefix = _("wayland protocol error -> ");
    size_t	len	= STRLEN(prefix);

    if (buf == NULL)
	return;

    vim_strncpy((char_u*)buf, (char_u*)prefix, len);
    vim_vsnprintf(buf + len, 4096 - len, fmt, args);

    // Remove newline that libwayland puts
    buf[STRLEN(buf) - 1] = NUL;

#ifdef FEAT_EVAL
    ch_log(NULL, "%s", buf);
#endif
    emsg(buf);

    vim_free(buf);
}

/*
 * Callback for seat text label/name
 */
    static void
wl_seat_listener_event_name(
	void		*data,
	struct wl_seat	*seat_proxy UNUSED,
	const char	*name)
{
    vwl_seat_T *seat = data;

    seat->label = (char *)vim_strsave((char_u *)name);
}

/*
 * Callback for seat capabilities
 */
    static void
wl_seat_listener_event_capabilities(
	void		*data,
	struct wl_seat	*seat_proxy UNUSED,
	uint32_t	capabilities)
{
    vwl_seat_T *seat = data;

    seat->capabilities = capabilities;
}

static const struct wl_seat_listener wl_seat_listener = {
    .name = wl_seat_listener_event_name,
    .capabilities = wl_seat_listener_event_capabilities
};

static void vwl_seat_destroy(vwl_seat_T *self);

/*
 * Callback for global event, for each global interface the compositor supports.
 * Keep in sync with vwl_disconnect_display().
 */
    static void
wl_registry_listener_event_global(
	void		    *data,
	struct wl_registry  *registry,
	uint32_t	    name,
	const char	    *interface,
	uint32_t	    version)
{
    vwl_connection_T *ct = data;

    if (STRCMP(interface, wl_seat_interface.name) == 0)
    {
	struct wl_seat	*seat_proxy = wl_registry_bind(registry, name,
		&wl_seat_interface, version > 5 ? 5 : version);
	vwl_seat_T	*seat;

	if (seat_proxy == NULL)
	    return;

	seat = ALLOC_CLEAR_ONE(vwl_seat_T);

	if (seat == NULL || ga_grow(&ct->gobjects.seats, 1) == FAIL)
	{
	    vwl_seat_destroy(seat);
	    return;
	}

	seat->proxy = seat_proxy;
	wl_seat_add_listener(seat_proxy, &wl_seat_listener, seat);

	if (vwl_connection_roundtrip(ct) == FAIL || seat->label == NULL)
	{
	    vwl_seat_destroy(seat);
	    return;
	}

	((vwl_seat_T **)ct->gobjects.seats.ga_data)[ct->gobjects.seats.ga_len++]
	    = seat;
    }
#ifdef FEAT_WAYLAND_CLIPBOARD
    else if (STRCMP(interface, zwlr_data_control_manager_v1_interface.name) == 0)
	ct->gobjects.zwlr_data_control_manager_v1 =
	    wl_registry_bind(registry, name,
		    &zwlr_data_control_manager_v1_interface,
		    version > 2 ? 2 : version);

    else if (STRCMP(interface, ext_data_control_manager_v1_interface.name) == 0)
	ct->gobjects.ext_data_control_manager_v1 =
	    wl_registry_bind(registry, name,
		    &ext_data_control_manager_v1_interface, 1);

# ifdef FEAT_WAYLAND_CLIPBOARD_FS
    else if (p_wst)
    {
	if (STRCMP(interface, wl_data_device_manager_interface.name) == 0)
	    ct->gobjects.wl_data_device_manager =
		wl_registry_bind(registry, name,
			&wl_data_device_manager_interface, 1);

	else if (STRCMP(interface, wl_shm_interface.name) == 0)
	    ct->gobjects.wl_shm =
		wl_registry_bind(registry, name,
			&wl_shm_interface, 1);

	else if (STRCMP(interface, wl_compositor_interface.name) == 0)
	    ct->gobjects.wl_compositor =
		wl_registry_bind(registry, name,
			&wl_compositor_interface, 1);

	else if (STRCMP(interface, xdg_wm_base_interface.name) == 0)
	    ct->gobjects.xdg_wm_base =
		wl_registry_bind(registry, name,
			&xdg_wm_base_interface, 1);

	else if (STRCMP(interface,
		    zwp_primary_selection_device_manager_v1_interface.name)
		    == 0)
	    ct->gobjects.zwp_primary_selection_device_manager_v1 =
		wl_registry_bind(registry, name,
			&zwp_primary_selection_device_manager_v1_interface, 1);
    }
# endif // FEAT_WAYLAND_CLIPBOARD_FS
#endif // FEAT_WAYLAND_CLIPBOARD

}

/*
 * Called when a global object is removed, if so, then do nothing. This is to
 * avoid a global being removed while it is in the process of being used. Let
 * the user call :wlrestore in order to reset everything. Requests to that
 * global will just be ignored on the compositor side.
 */
    static void
wl_registry_listener_event_global_remove(
	void		    *data UNUSED,
	struct wl_registry  *registry UNUSED,
	uint32_t	    name UNUSED)
{
}

#ifdef FEAT_WAYLAND_CLIPBOARD_FS
    static void
xdg_wm_base_listener_event_ping(
	void *data UNUSED,
	struct xdg_wm_base *xdg_base,
	uint32_t serial)
{
    xdg_wm_base_pong(xdg_base, serial);
}
#endif

static const struct wl_registry_listener wl_registry_listener = {
    .global = wl_registry_listener_event_global,
    .global_remove = wl_registry_listener_event_global_remove
};

#ifdef FEAT_WAYLAND_CLIPBOARD_FS
static const struct xdg_wm_base_listener xdg_wm_base_listener  = {
    .ping = xdg_wm_base_listener_event_ping
};
#endif

static void vwl_connection_destroy(vwl_connection_T *self);

#ifdef FEAT_WAYLAND_CLIPBOARD

# define VWL_DESTROY_GOBJECT(ct, object) \
    if (ct->gobjects.object != NULL) \
    { \
	object##_destroy(ct->gobjects.object); \
	ct->gobjects.object = NULL; \
    }

# define VWL_GOBJECT_AVAIL(ct, object) (ct->gobjects.object != NULL)

#endif

// Make sure to call wayland_set_display(display);
    static vwl_connection_T *
vwl_connection_new(const char *display)
{
    vwl_connection_T	*ct;
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
    const char_u	*env;
    bool		force_fs;
#endif
    if (wayland_no_connect)
	return NULL;

    // We will get an error if XDG_RUNTIME_DIR is not set.
    if (mch_getenv("XDG_RUNTIME_DIR") == NULL)
	return NULL;

    ct = ALLOC_CLEAR_ONE(vwl_connection_T);

    if (ct == NULL)
	return NULL;

    // Must set log handler before we connect display in order to work.
    wl_log_set_handler_client(vwl_log_handler);

    ct->display.proxy = wl_display_connect(display);

    if (ct->display.proxy == NULL)
    {
	vim_free(ct);
	return NULL;
    }

    ct->display.fd = wl_display_get_fd(ct->display.proxy);
    ct->registry.proxy = wl_display_get_registry(ct->display.proxy);

    if (ct->registry.proxy == NULL)
    {
	wl_display_disconnect(ct->display.proxy);
	vim_free(ct);
	return NULL;
    }

    ga_init2(&ct->gobjects.seats, sizeof(vwl_seat_T *), 1);

    wl_registry_add_listener(ct->registry.proxy, &wl_registry_listener, ct);

#ifdef FEAT_WAYLAND_CLIPBOARD_FS
    env = mch_getenv("VIM_WAYLAND_FORCE_FS");
    force_fs = (env != NULL && STRCMP(env, "1") == 0);

    if (force_fs)
	p_wst = TRUE;
#endif

    if (vwl_connection_roundtrip(ct) == FAIL)
    {
	vwl_connection_destroy(ct);
	return NULL;
    }

#ifdef FEAT_WAYLAND_CLIPBOARD_FS
    if (force_fs)
    {
	// Force using focus stealing method
	VWL_DESTROY_GOBJECT(ct, ext_data_control_manager_v1)
	VWL_DESTROY_GOBJECT(ct, zwlr_data_control_manager_v1)
    }

    // If data control protocols are available, we don't need the other global
    // objects.
    else if (VWL_GOBJECT_AVAIL(ct, ext_data_control_manager_v1)
	    || VWL_GOBJECT_AVAIL(ct, zwlr_data_control_manager_v1))
    {
	VWL_DESTROY_GOBJECT(ct, wl_data_device_manager)
	VWL_DESTROY_GOBJECT(ct, wl_shm)
	VWL_DESTROY_GOBJECT(ct, wl_compositor)
	VWL_DESTROY_GOBJECT(ct, xdg_wm_base)
	VWL_DESTROY_GOBJECT(ct, zwp_primary_selection_device_manager_v1)
    }

    // Start responding to pings from the compositor if we have xdg_wm_base
    if (VWL_GOBJECT_AVAIL(ct, xdg_wm_base))
	xdg_wm_base_add_listener(ct->gobjects.xdg_wm_base,
		&xdg_wm_base_listener, NULL);
#endif

    return ct;
}

#ifdef FEAT_WAYLAND_CLIPBOARD

/*
 * Destroy/free seat.
 */
    static void
vwl_seat_destroy(vwl_seat_T *self)
{
    if (self == NULL)
	return;
    if (self->proxy != NULL)
    {
	if (wl_seat_get_version(self->proxy) >= 5)
	    // Helpful for the compositor
	    wl_seat_release(self->proxy);
	else
	    wl_seat_destroy(self->proxy);
    }
    vim_free(self->label);
    vim_free(self);
}

/*
 * Disconnects the display and frees up all resources, including all global
 * objects.
 */
    static void
vwl_connection_destroy(vwl_connection_T *self)
{
#ifdef FEAT_WAYLAND_CLIPBOARD
    VWL_DESTROY_GOBJECT(self, ext_data_control_manager_v1)
    VWL_DESTROY_GOBJECT(self, zwlr_data_control_manager_v1)
# ifdef FEAT_WAYLAND_CLIPBOARD_FS
    VWL_DESTROY_GOBJECT(self, wl_data_device_manager)
    VWL_DESTROY_GOBJECT(self, wl_shm)
    VWL_DESTROY_GOBJECT(self, wl_compositor)
    VWL_DESTROY_GOBJECT(self, xdg_wm_base)
    VWL_DESTROY_GOBJECT(self, zwp_primary_selection_device_manager_v1)
# endif
#endif

    for (int i = 0; i < self->gobjects.seats.ga_len; i++)
	vwl_seat_destroy(((vwl_seat_T **)self->gobjects.seats.ga_data)[i]);
    ga_clear(&self->gobjects.seats);
    self->gobjects.seats.ga_len = 0;

    if (self->registry.proxy != NULL)
    {
	wl_registry_destroy(self->registry.proxy);
	self->registry.proxy = NULL;
    }
    if (self->display.proxy != NULL)
    {
	wl_display_disconnect(self->display.proxy);
	self->display.proxy = NULL;
    }
    vim_free(self);
}

/*
 * Return a seat with the give name/label. If none exists then NULL is returned.
 * If NULL or an empty string is passed as the label then the first available
 * seat found is used.
 */
    vwl_seat_T *
vwl_connection_get_seat(vwl_connection_T *self, const char *label)
{
    if ((STRCMP(label, "") == 0 || label == NULL)
	    && self->gobjects.seats.ga_len > 0)
	return ((vwl_seat_T **)self->gobjects.seats.ga_data)[0];

    for (int i = 0; i < self->gobjects.seats.ga_len; i++)
    {
	vwl_seat_T *seat = ((vwl_seat_T **)self->gobjects.seats.ga_data)[i];
	if (STRCMP(seat->label, label) == 0)
	    return seat;
    }
    return NULL;
}

#ifdef FEAT_WAYLAND_CLIPBOARD_FS
/*
 * Get keyboard object from seat and return it. NULL is returned on
 * failure such as when a keyboard is not available for seat.
 */
    struct wl_keyboard *
vwl_seat_get_keyboard(vwl_seat_T *self)
{
    if (!(self->capabilities & WL_SEAT_CAPABILITY_KEYBOARD))
	return NULL;

    return wl_seat_get_keyboard(self->proxy);
}
#endif

#endif

/*
 * Set wayland_display_name to display. Note that this allocate a copy of the
 * string, unless NULL is passed. If NULL is passed then v:wayland_display is
 * set to $WAYLAND_DISPLAY, but wayland_display_name is set to NULL.
 */
    static void
wayland_set_display(const char *display)
{
    if (display == NULL)
	display = (char*)mch_getenv((char_u*)"WAYLAND_DISPLAY");
    else if (display == wayland_display_name)
	// Don't want to be freeing vwl_display_strname then trying to copy it
	// after.
	goto exit;

    if (display == NULL)
	// $WAYLAND_DISPLAY is not set
	display = "";

    // Leave unchanged if display is empty (but not NULL)
    if (STRCMP(display, "") != 0)
    {
	vim_free(wayland_display_name);
	wayland_display_name = (char*)vim_strsave((char_u*)display);
    }

exit:
#ifdef FEAT_EVAL
    set_vim_var_string(VV_WAYLAND_DISPLAY, (char_u*)display, -1);
#endif
}

/*
 * Initializes the global Wayland connection. Connects to the Wayland display
 * with given name and binds to global objects as needed. If display is NULL
 * then the $WAYLAND_DISPLAY environment variable will be used (handled by
 * libwayland). Returns FAIL on failure and OK on
 * success
 */
    int
wayland_init_connection(const char *display)
{
    wayland_set_display(display);

    wayland_ct = vwl_connection_new(display);

    if (wayland_ct == NULL)
	goto fail;

    return OK;
fail:
    // Set v:wayland_display to empty string (but not wayland_display_name)
    wayland_set_display("");
    return FAIL;
}

/*
 * Disconnect global Wayland connection and free up all resources used.
 */
    void
wayland_uninit_connection(void)
{
    if (wayland_ct == NULL)
	return;
#ifdef FEAT_WAYLAND_CLIPBOARD
    clip_uninit_wayland();
#endif
    vwl_connection_destroy(wayland_ct);
    wayland_ct = NULL;
    is_reading = false;
    wayland_set_display("");
}

static int wayland_ct_restore_count = 0;

/*
 * Attempts to restore the Wayland display connection.
 */
    static void
wayland_restore_connection(void)
{
    // No point in restoring the connection if we are exiting or dying.
    if (exiting || v_dying || wayland_ct_restore_count <= 0)
	wayland_set_display("");

    --wayland_ct_restore_count;
    wayland_uninit_connection();

    if (wayland_init_connection(wayland_display_name) == OK)
    {
#ifdef FEAT_WAYLAND_CLIPBOARD
	clip_init_wayland();
#endif
    }
}

/*
 * Should be called before polling (select or poll) the global Wayland
 * connection display fd. Returns fd on success and -1 on failure.
 */
    int
wayland_prepare_read(void)
{
    if (wayland_ct == NULL)
	return -1;

    if (is_reading)
    {
	wl_display_cancel_read(wayland_ct->display.proxy);
	is_reading = false;
    }

    while (wl_display_prepare_read(wayland_ct->display.proxy) == -1)
	// Event queue not empty, dispatch the events
	if (wl_display_dispatch_pending(wayland_ct->display.proxy) == -1)
	    return -1;

    if (vwl_connection_flush(wayland_ct) < 0)
    {
	wl_display_cancel_read(wayland_ct->display.proxy);
	return -1;
    }

    is_reading = true;

    return wayland_ct->display.fd;
}

/*
 * Catch up on any queued events
 */
    int
wayland_update(void)
{
    if (wayland_ct == NULL)
	return FAIL;
    return vwl_connection_roundtrip(wayland_ct);
}

#if !defined(HAVE_SELECT)

    void
wayland_poll_check(int revents)
{
    if (wayland_ct == NULL)
	return;

    is_reading = false;
    if (revents & POLLIN)
    {
	if (wl_display_read_events(wayland_ct->display.proxy) != -1)
	{
	    wl_display_dispatch_pending(wayland_ct->display.proxy);
	    return;
	}
    }
    else if (revents & (POLLHUP | POLLERR))
	wl_display_cancel_read(wayland_ct->display.proxy);
    else
    {
	// Nothing happened
	wl_display_cancel_read(wayland_ct->display.proxy);
	return;
    }
    wayland_restore_connection();
}

#endif
#if defined(HAVE_SELECT)

    void
wayland_select_check(bool is_set)
{
    if (wayland_ct == NULL)
	return;

    is_reading = false;
    if (is_set)
    {
	if (wl_display_read_events(wayland_ct->display.proxy) != -1)
	    wl_display_dispatch_pending(wayland_ct->display.proxy);
	else
	{
	    wl_display_cancel_read(wayland_ct->display.proxy);
	    wayland_restore_connection();
	}
    }
    else
	wl_display_cancel_read(wayland_ct->display.proxy);
}

#endif

/*
 * Disconnect then reconnect Wayland connection, and update clipmethod.
 */
    void
ex_wlrestore(exarg_T *eap)
{
    char *display;

    if (eap->arg == NULL || STRLEN(eap->arg) == 0)
	// Use current display name if none given
	display = wayland_display_name;
    else
	display = (char*)eap->arg;

    // Return early if shebang is not passed, we are still connected, and if not
    // changing to a new Wayland display.
    if (!eap->forceit && wayland_ct != NULL &&
	    (display == wayland_display_name ||
	     (wayland_display_name != NULL &&
	      STRCMP(wayland_display_name, display) == 0)))
	return;

#ifdef FEAT_WAYLAND_CLIPBOARD
    if (clipmethod == CLIPMETHOD_WAYLAND)
    {
	// Lose any selections we own
	if (clip_star.owned)
	    clip_lose_selection(&clip_star);
	if (clip_plus.owned)
	    clip_lose_selection(&clip_plus);
    }
#endif

    if (display != NULL)
	display = (char*)vim_strsave((char_u*)display);

    // Will lose any selections we own
    wayland_uninit_connection();

    // Reset amount of available tries to reconnect the display to 5
    wayland_ct_restore_count = 5;

    if (wayland_init_connection(display) == OK)
    {
	smsg(_("restoring Wayland display %s"), wayland_display_name);

#ifdef FEAT_WAYLAND_CLIPBOARD
	clip_plus.did_warn = false;
	clip_star.did_warn = false;
	clip_init_wayland();
#endif
    }
    else
	msg(_("failed restoring, lost connection to Wayland display"));

    vim_free(display);

    choose_clipmethod();
}

#if defined(FEAT_WAYLAND_CLIPBOARD)

/*
 * Get a suitable data device manager from connection. "supported" should be
 * initialized to VWL_DATA_PROTOCOL_NONE beforehand. Returns NULL if there are
 * no data device manager available with the required selection.
 */
    vwl_data_device_manager_T *
vwl_connection_get_data_device_manager(
	vwl_connection_T *self,
	wayland_selection_T req_sel,
	int_u *supported)
{
    vwl_data_device_manager_T *manager =
	ALLOC_CLEAR_ONE(vwl_data_device_manager_T);

    // Prioritize ext-data-control-v1 over wlr-data-control-unstable-v1 because
    // it is newer.
    if (self->gobjects.ext_data_control_manager_v1 != NULL)
    {
	manager->proxy = self->gobjects.ext_data_control_manager_v1;
	manager->protocol = VWL_DATA_PROTOCOL_EXT;

	*supported |= (WAYLAND_SELECTION_REGULAR | WAYLAND_SELECTION_PRIMARY);
    }
    else if (self->gobjects.zwlr_data_control_manager_v1 != NULL)
    {
	manager->proxy = self->gobjects.zwlr_data_control_manager_v1;
	manager->protocol = VWL_DATA_PROTOCOL_WLR;

	*supported |= WAYLAND_SELECTION_REGULAR;

	// Only version 2 or greater supports the primary selection
	if (zwlr_data_control_manager_v1_get_version(manager->proxy) >= 2)
	    *supported |= WAYLAND_SELECTION_PRIMARY;
    }
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
    else if (self->gobjects.wl_data_device_manager != NULL
	    && req_sel == WAYLAND_SELECTION_REGULAR)
    {
	manager->proxy = self->gobjects.wl_data_device_manager;
	manager->protocol = VWL_DATA_PROTOCOL_CORE;

	*supported |= WAYLAND_SELECTION_REGULAR;
    }

    if (req_sel == WAYLAND_SELECTION_PRIMARY
	    && !(*supported & WAYLAND_SELECTION_PRIMARY))
	if (self->gobjects.zwp_primary_selection_device_manager_v1 != NULL)
	{
	    manager->proxy =
		self->gobjects.zwp_primary_selection_device_manager_v1;
	    manager->protocol = VWL_DATA_PROTOCOL_PRIMARY;

	    *supported |= WAYLAND_SELECTION_PRIMARY;
	}
#endif

    if (!(*supported & req_sel))
    {
	vim_free(manager);
	return NULL;
    }

    return manager;
}

    vwl_data_device_T *
vwl_data_device_manager_get_data_device(
	vwl_data_device_manager_T *self,
	vwl_seat_T *seat)
{
    vwl_data_device_T *device = ALLOC_CLEAR_ONE(vwl_data_device_T);

    switch (self->protocol)
    {
	case VWL_DATA_PROTOCOL_EXT:
	    device->proxy = ext_data_control_manager_v1_get_data_device(
		    self->proxy, seat->proxy);
	    break;
	case VWL_DATA_PROTOCOL_WLR:
	    device->proxy = zwlr_data_control_manager_v1_get_data_device(
		    self->proxy, seat->proxy);
	    break;
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
	case VWL_DATA_PROTOCOL_CORE:
	    device->proxy = wl_data_device_manager_get_data_device(
		    self->proxy, seat->proxy);
	    break;
	case VWL_DATA_PROTOCOL_PRIMARY:
	    device->proxy = zwp_primary_selection_device_manager_v1_get_device(
		    self->proxy, seat->proxy);
	    break;
#endif
	default:
	    vim_free(device);
	    return NULL;
    }
    device->protocol = self->protocol;

    return device;
}

    vwl_data_source_T *
vwl_data_device_manager_create_data_source(vwl_data_device_manager_T *self)
{
    vwl_data_source_T *source = ALLOC_CLEAR_ONE(vwl_data_source_T);

    switch (self->protocol)
    {
	case VWL_DATA_PROTOCOL_EXT:
	    source->proxy = ext_data_control_manager_v1_create_data_source(
		    self->proxy);
	    break;
	case VWL_DATA_PROTOCOL_WLR:
	    source->proxy = zwlr_data_control_manager_v1_create_data_source(
		    self->proxy);
	    break;
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
	case VWL_DATA_PROTOCOL_CORE:
	    source->proxy = wl_data_device_manager_create_data_source(
		    self->proxy);
	    break;
	case VWL_DATA_PROTOCOL_PRIMARY:
	    source->proxy =
		zwp_primary_selection_device_manager_v1_create_source(
			self->proxy);
	    break;
#endif
	default:
	    vim_free(source);
	    return NULL;
    }
    source->protocol = self->protocol;

    return source;
}

    static vwl_data_offer_T *
vwl_data_device_wrap_offer_proxy(vwl_data_device_T *self, void *proxy)
{
    vwl_data_offer_T *offer = ALLOC_CLEAR_ONE(vwl_data_offer_T);

    if (offer == NULL)
	return NULL;

    offer->proxy = proxy;
    offer->protocol = self->protocol;
    offer->data = self->data;
    ga_init2(&offer->mime_types, sizeof(char *), 10);

    // Try pre allocating the array, 10 mime types seems to usually be the
    // maximum from experience.
    ga_grow(&offer->mime_types, 10);

    return offer;
}

#ifdef FEAT_WAYLAND_CLIPBOARD_FS
# define VWL_CODE_DATA_PROXY_FS_DESTROY(type) \
    case VWL_DATA_PROTOCOL_CORE: \
	wl_data_##type##_destroy(self->proxy); \
	break; \
    case VWL_DATA_PROTOCOL_PRIMARY: \
	zwp_primary_selection_##type##_v1_destroy(self->proxy); \
	break;
#else
# define VWL_CODE_DATA_PROXY_FS_DESTROY(type)
#endif

#define VWL_FUNC_DATA_PROXY_DESTROY(type) \
	void \
    vwl_data_##type##_destroy(vwl_data_##type##_T *self) \
    { \
	if (self == NULL) \
	    return; \
	switch (self->protocol) \
	{ \
	    case VWL_DATA_PROTOCOL_EXT: \
		ext_data_control_##type##_v1_destroy(self->proxy); \
		break; \
	    case VWL_DATA_PROTOCOL_WLR: \
		zwlr_data_control_##type##_v1_destroy(self->proxy); \
		break; \
	    VWL_CODE_DATA_PROXY_FS_DESTROY(type) \
	    default: \
		break; \
	} \
	vim_free(self); \
    }

VWL_FUNC_DATA_PROXY_DESTROY(device)
VWL_FUNC_DATA_PROXY_DESTROY(source)

    void
vwl_data_offer_destroy(vwl_data_offer_T *self)
{
    if (self == NULL)
	return;
    switch (self->protocol)
    {
	case VWL_DATA_PROTOCOL_EXT:
	    ext_data_control_offer_v1_destroy(self->proxy);
	    break;
	case VWL_DATA_PROTOCOL_WLR:
	    zwlr_data_control_offer_v1_destroy(self->proxy);
	    break;
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
	case VWL_DATA_PROTOCOL_CORE:
	    wl_data_offer_destroy(self->proxy);
	    break;
	case VWL_DATA_PROTOCOL_PRIMARY:
	    zwp_primary_selection_offer_v1_destroy(self->proxy);
	    break;
#endif
	default:
	    break;
    }
    ga_clear_strings(&self->mime_types);
    vim_free(self);
}

/*
 * Doesn't destroy the actual global object proxy, only frees the structure.
 */
void
vwl_data_device_manager_discard(vwl_data_device_manager_T *self)
{
    if (self == NULL)
	return;
    vim_free(self);
}

#define VWL_FUNC_DATA_DEVICE_EVENT_DATA_OFFER(device_type, offer_type) \
	static void \
    device_type##_listener_event_data_offer( \
	    void *data, \
	    struct device_type *device UNUSED, \
	    struct offer_type *offer) \
    { \
	vwl_data_device_T *self = data; \
	self->offer = vwl_data_device_wrap_offer_proxy(self, offer); \
	self->listener->data_offer(self->data, self, self->offer); \
    }

// We want to set the offer to NULL after the selection callback, because the
// callback may free the offer, and we don't want a dangling pointer.
#define VWL_FUNC_DATA_DEVICE_EVENT_SELECTION(device_type, offer_type) \
	static void \
    device_type##_listener_event_selection( \
	    void *data, \
	    struct device_type *device UNUSED, \
	    struct offer_type *offer UNUSED) \
    { \
	vwl_data_device_T *self = data; \
	self->listener->selection(self->data, self, self->offer, \
		WAYLAND_SELECTION_REGULAR); \
	self->offer = NULL; \
    } \

#define VWL_FUNC_DATA_DEVICE_EVENT_PRIMARY_SELECTION(device_type, offer_type) \
	static void \
    device_type##_listener_event_primary_selection( \
	    void *data, \
	    struct device_type *device UNUSED, \
	    struct offer_type *offer UNUSED) \
    { \
	vwl_data_device_T *self = data; \
	self->listener->selection(self->data, self, self->offer, \
		WAYLAND_SELECTION_PRIMARY); \
	self->offer = NULL; \
    }

#define VWL_FUNC_DATA_DEVICE_EVENT_FINISHED(device_type) \
	static void \
    device_type##_listener_event_finished( \
	    void *data, \
	    struct device_type *device UNUSED) \
    { \
	vwl_data_device_T *self = data; \
	self->listener->finished(self->data, self); \
    }

VWL_FUNC_DATA_DEVICE_EVENT_DATA_OFFER(
    ext_data_control_device_v1, ext_data_control_offer_v1)
VWL_FUNC_DATA_DEVICE_EVENT_DATA_OFFER(
    zwlr_data_control_device_v1, zwlr_data_control_offer_v1)
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
VWL_FUNC_DATA_DEVICE_EVENT_DATA_OFFER(wl_data_device, wl_data_offer)
VWL_FUNC_DATA_DEVICE_EVENT_DATA_OFFER(
	zwp_primary_selection_device_v1, zwp_primary_selection_offer_v1)
#endif

VWL_FUNC_DATA_DEVICE_EVENT_SELECTION(
    ext_data_control_device_v1, ext_data_control_offer_v1
)
VWL_FUNC_DATA_DEVICE_EVENT_SELECTION(
    zwlr_data_control_device_v1, zwlr_data_control_offer_v1
)
VWL_FUNC_DATA_DEVICE_EVENT_PRIMARY_SELECTION(
    ext_data_control_device_v1, ext_data_control_offer_v1
)
VWL_FUNC_DATA_DEVICE_EVENT_PRIMARY_SELECTION(
    zwlr_data_control_device_v1, zwlr_data_control_offer_v1
)
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
VWL_FUNC_DATA_DEVICE_EVENT_SELECTION(
    wl_data_device, wl_data_offer
)
VWL_FUNC_DATA_DEVICE_EVENT_PRIMARY_SELECTION(
	zwp_primary_selection_device_v1, zwp_primary_selection_offer_v1)
#endif

VWL_FUNC_DATA_DEVICE_EVENT_FINISHED(ext_data_control_device_v1)
VWL_FUNC_DATA_DEVICE_EVENT_FINISHED(zwlr_data_control_device_v1)

static struct ext_data_control_device_v1_listener
    ext_data_control_device_v1_listener = {
	.data_offer = ext_data_control_device_v1_listener_event_data_offer,
	.selection = ext_data_control_device_v1_listener_event_selection,
	.primary_selection =
	    ext_data_control_device_v1_listener_event_primary_selection,
	.finished = ext_data_control_device_v1_listener_event_finished
};
static const struct zwlr_data_control_device_v1_listener
    zwlr_data_control_device_v1_listener = {
	.data_offer = zwlr_data_control_device_v1_listener_event_data_offer,
	.selection = zwlr_data_control_device_v1_listener_event_selection,
	.primary_selection =
	    zwlr_data_control_device_v1_listener_event_primary_selection,
	.finished = zwlr_data_control_device_v1_listener_event_finished
};
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
static const struct wl_data_device_listener wl_data_device_listener = {
    .data_offer = wl_data_device_listener_event_data_offer,
    .selection = wl_data_device_listener_event_selection,
};
static const struct zwp_primary_selection_device_v1_listener
    zwp_primary_selection_device_v1_listener = {
	.data_offer = zwp_primary_selection_device_v1_listener_event_data_offer,
	.selection =
	    zwp_primary_selection_device_v1_listener_event_primary_selection,
};
#  endif

#  define VWL_FUNC_DATA_SOURCE_EVENT_SEND(source_type) \
	static void \
    source_type##_listener_event_send( \
	    void *data, struct source_type *source UNUSED, \
	    const char *mime_type, int fd) \
    { \
	vwl_data_source_T *self = data; \
	self->listener->send(self->data, self, mime_type, fd); \
    }

#  define VWL_FUNC_DATA_SOURCE_EVENT_CANCELLED(source_type) \
	static void \
    source_type##_listener_event_cancelled( \
	    void *data, struct source_type *source UNUSED) \
    { \
	vwl_data_source_T *self = data; \
	self->listener->cancelled(self->data, self); \
    }

VWL_FUNC_DATA_SOURCE_EVENT_SEND(ext_data_control_source_v1)
VWL_FUNC_DATA_SOURCE_EVENT_SEND(zwlr_data_control_source_v1)
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
VWL_FUNC_DATA_SOURCE_EVENT_SEND(wl_data_source)
VWL_FUNC_DATA_SOURCE_EVENT_SEND(zwp_primary_selection_source_v1)
#endif

VWL_FUNC_DATA_SOURCE_EVENT_CANCELLED(ext_data_control_source_v1)
VWL_FUNC_DATA_SOURCE_EVENT_CANCELLED(zwlr_data_control_source_v1)
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
VWL_FUNC_DATA_SOURCE_EVENT_CANCELLED(wl_data_source)
VWL_FUNC_DATA_SOURCE_EVENT_CANCELLED(zwp_primary_selection_source_v1)
#endif

static const struct ext_data_control_source_v1_listener
    ext_data_control_source_v1_listener = {
	.send = ext_data_control_source_v1_listener_event_send,
	.cancelled = ext_data_control_source_v1_listener_event_cancelled
};
static const struct zwlr_data_control_source_v1_listener
    zwlr_data_control_source_v1_listener = {
	.send = zwlr_data_control_source_v1_listener_event_send,
	.cancelled = zwlr_data_control_source_v1_listener_event_cancelled
};
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
static const struct wl_data_source_listener wl_data_source_listener = {
    .send = wl_data_source_listener_event_send,
    .cancelled = wl_data_source_listener_event_cancelled
};
static const struct zwp_primary_selection_source_v1_listener
    zwp_primary_selection_source_v1_listener = {
	.send = zwp_primary_selection_source_v1_listener_event_send,
	.cancelled = zwp_primary_selection_source_v1_listener_event_cancelled
};
#endif

#define VWL_FUNC_DATA_OFFER_EVENT_OFFER(offer_type) \
	static void \
    offer_type##_listener_event_offer( \
	    void *data, \
	    struct offer_type *offer UNUSED, \
	    const char *mime_type) \
    { \
	vwl_data_offer_T *self = data; \
	if (STRCMP(mime_type, wayland_vim_special_mime) == 0) \
	    self->from_vim = true; \
	else if (!self->from_vim && \
		self->listener->offer(self->data, self, mime_type)) \
	{ \
	    char *mime = (char *)vim_strsave((char_u *)mime_type); \
	    if (ga_grow(&self->mime_types, 1) == FAIL) \
		vim_free(mime); \
	    else \
		if (mime != NULL) \
		    ((char **)self->mime_types.ga_data) \
					[self->mime_types.ga_len++] = mime; \
	} \
    }

VWL_FUNC_DATA_OFFER_EVENT_OFFER(ext_data_control_offer_v1)
VWL_FUNC_DATA_OFFER_EVENT_OFFER(zwlr_data_control_offer_v1)
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
VWL_FUNC_DATA_OFFER_EVENT_OFFER(wl_data_offer)
VWL_FUNC_DATA_OFFER_EVENT_OFFER(zwp_primary_selection_offer_v1)
#endif

static const struct ext_data_control_offer_v1_listener
    ext_data_control_offer_v1_listener = {
	.offer = ext_data_control_offer_v1_listener_event_offer
};
static const struct zwlr_data_control_offer_v1_listener
    zwlr_data_control_offer_v1_listener = {
	.offer = zwlr_data_control_offer_v1_listener_event_offer
};
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
static const struct wl_data_offer_listener
    wl_data_offer_listener = {
	.offer = wl_data_offer_listener_event_offer
};
static const struct zwp_primary_selection_offer_v1_listener
    zwp_primary_selection_offer_v1_listener = {
	.offer = zwp_primary_selection_offer_v1_listener_event_offer
};
#endif

#ifdef FEAT_WAYLAND_CLIPBOARD_FS
# define VWL_CODE_DATA_PROXY_FS_ADD_LISTENER(type) \
    case VWL_DATA_PROTOCOL_CORE: \
	wl_data_##type##_add_listener(self->proxy, \
		&wl_data_##type##_listener, self); \
	break; \
    case VWL_DATA_PROTOCOL_PRIMARY: \
	zwp_primary_selection_##type##_v1_add_listener(self->proxy, \
		&zwp_primary_selection_##type##_v1_listener, self); \
	break;
#else
# define VWL_CODE_DATA_PROXY_FS_ADD_LISTENER(type)
#endif

#define VWL_FUNC_DATA_PROXY_ADD_LISTENER(type) \
	void \
    vwl_data_##type##_add_listener( \
	    vwl_data_##type##_T *self, \
	    const vwl_data_##type##_listener_T *listener, \
	    void *data) \
    { \
	if (self == NULL) \
	    return; \
	self->data = data; \
	self->listener = listener; \
	switch (self->protocol) \
	{ \
	    case VWL_DATA_PROTOCOL_EXT: \
		ext_data_control_##type##_v1_add_listener(self->proxy, \
			&ext_data_control_##type##_v1_listener, self); \
		break; \
	    case VWL_DATA_PROTOCOL_WLR: \
		zwlr_data_control_##type##_v1_add_listener(self->proxy, \
			&zwlr_data_control_##type##_v1_listener, self); \
		break; \
	    VWL_CODE_DATA_PROXY_FS_ADD_LISTENER(type) \
	    default: \
		break; \
	} \
    }

VWL_FUNC_DATA_PROXY_ADD_LISTENER(device)
VWL_FUNC_DATA_PROXY_ADD_LISTENER(source)
VWL_FUNC_DATA_PROXY_ADD_LISTENER(offer)

/*
 * Set the given selection to source. If a data control protocol is being used,
 * "serial" is ignored.
 */
    void
vwl_data_device_set_selection(
    vwl_data_device_T *self,
    vwl_data_source_T *source,
    uint32_t serial UNUSED,
    wayland_selection_T selection
)
{
    void *proxy = source == NULL ? NULL : source->proxy;

    if (selection == WAYLAND_SELECTION_REGULAR)
    {
	switch (self->protocol)
	{
	    case VWL_DATA_PROTOCOL_EXT:
		ext_data_control_device_v1_set_selection(self->proxy, proxy);
		break;
	    case VWL_DATA_PROTOCOL_WLR:
		zwlr_data_control_device_v1_set_selection(self->proxy, proxy);
		break;
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
	    case VWL_DATA_PROTOCOL_CORE:
		wl_data_device_set_selection(self->proxy, proxy, serial);
		break;
#endif
	    default:
		break;
	}
    }
    else if (selection == WAYLAND_SELECTION_PRIMARY)
    {
	switch (self->protocol)
	{
	    case VWL_DATA_PROTOCOL_EXT:
		ext_data_control_device_v1_set_primary_selection(
			self->proxy, proxy
			);
		break;
	    case VWL_DATA_PROTOCOL_WLR:
		zwlr_data_control_device_v1_set_primary_selection(
			self->proxy, proxy
			);
		break;
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
	    case VWL_DATA_PROTOCOL_PRIMARY:
		zwp_primary_selection_device_v1_set_selection(
			self->proxy, proxy, serial);
		break;
#endif
	    default:
		break;
	}
    }
}

    void
vwl_data_source_offer(vwl_data_source_T *self, const char *mime_type)
{
    switch (self->protocol)
    {
	case VWL_DATA_PROTOCOL_EXT:
	    ext_data_control_source_v1_offer(self->proxy, mime_type);
	    break;
	case VWL_DATA_PROTOCOL_WLR:
	    zwlr_data_control_source_v1_offer(self->proxy, mime_type);
	    break;
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
	case VWL_DATA_PROTOCOL_CORE:
	    wl_data_source_offer(self->proxy, mime_type);
	    break;
	case VWL_DATA_PROTOCOL_PRIMARY:
	    zwp_primary_selection_source_v1_offer(self->proxy, mime_type);
	    break;
#endif
	default:
	    break;
    }
}

    void
vwl_data_offer_receive(
	vwl_data_offer_T *self,
	const char *mime_type,
	int32_t fd)
{
    switch (self->protocol)
    {
	case VWL_DATA_PROTOCOL_EXT:
	    ext_data_control_offer_v1_receive(self->proxy, mime_type, fd);
	    break;
	case VWL_DATA_PROTOCOL_WLR:
	    zwlr_data_control_offer_v1_receive(self->proxy, mime_type, fd);
	    break;
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
	case VWL_DATA_PROTOCOL_CORE:
	    wl_data_offer_receive(self->proxy, mime_type, fd);
	    break;
	case VWL_DATA_PROTOCOL_PRIMARY:
	    zwp_primary_selection_offer_v1_receive(self->proxy, mime_type, fd);
	    break;
#endif
	default:
	    break;
    }
}

#endif // FEAT_WAYLAND_CLIPBOARD

#endif // FEAT_WAYLAND
