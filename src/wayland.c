/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * wayland.c: Stuff related to wayland
 */

#include "vim.h"

#ifdef FEAT_WAYLAND

#include <wayland-client.h>

#ifdef FEAT_WAYLAND_CLIPBOARD
# include "auto/wayland/wlr-data-control-unstable-v1.h"
# include "auto/wayland/ext-data-control-v1.h"
# include "auto/wayland/xdg-shell.h"
# include "auto/wayland/primary-selection-unstable-v1.h"
#endif

// Struct that represents a seat. (Should be accessed via
// vwl_get_seat()).
typedef struct {
    struct wl_seat  *proxy;
    char	    *label;	    // Name of seat as text (e.g. seat0,
				    // seat1...).
    uint32_t	    capabilities;   // Bitmask of the capabilites of the seat
				    // (pointer, keyboard, touch).
} vwl_seat_T;

// Global objects
typedef struct {
#ifdef FEAT_WAYLAND_CLIPBOARD
    // Data control protocols
    struct zwlr_data_control_manager_v1 *zwlr_data_control_manager_v1;
    struct ext_data_control_manager_v1	*ext_data_control_manager_v1;
    struct wl_data_device_manager	*wl_data_device_manager;
    struct wl_shm			*wl_shm;
    struct wl_compositor		*wl_compositor;
    struct xdg_wm_base			*xdg_wm_base;
    struct zwp_primary_selection_device_manager_v1
	*zwp_primary_selection_device_manager_v1;
#endif
} vwl_global_objects_T;

// Struct wrapper for wayland display and registry
typedef struct {
    struct wl_display	*proxy;
    int			fd;	// File descriptor for display

    struct {
	struct wl_registry *proxy;
    } registry;
} vwl_display_T;

#ifdef FEAT_WAYLAND_CLIPBOARD

typedef struct {
    struct wl_shm_pool	*pool;
    int			fd;

    struct wl_buffer	*buffer;
    int			available;

    int			width;
    int			height;
    int			stride;
    int			size;
} vwl_buffer_store_T;

typedef struct {
    void		    *user_data;
    void		    (*on_focus)(void *data, uint32_t serial);

    struct wl_surface	    *surface;
    struct wl_keyboard	    *keyboard;

    struct {
	struct xdg_surface  *surface;
	struct xdg_toplevel *toplevel;
    } shell;

    int got_focus;
} vwl_fs_surface_T; // fs = focus steal

// Wayland protocols for accessing the selection
typedef enum {
    VWL_DATA_PROTOCOL_NONE,
    VWL_DATA_PROTOCOL_EXT,
    VWL_DATA_PROTOCOL_WLR,
    VWL_DATA_PROTOCOL_CORE,
    VWL_DATA_PROTOCOL_PRIMARY
} vwl_data_protocol_T;

// DATA RELATED OBJECT WRAPPERS
// These wrap around a proxy and act as a generic container.
// The `data` member is used to pass other needed stuff around such as a
// vwl_clipboard_selection_T pointer.

typedef struct {
    void		*proxy;
    void		*data; // Is not set when a new offer is created on a
			       // data_offer event. Only set when listening to a
			       // data offer.
    vwl_data_protocol_T protocol;
} vwl_data_offer_T;

typedef struct {
    void		*proxy;
    void		*data;
    vwl_data_protocol_T protocol;
} vwl_data_source_T;

typedef struct {
    void		*proxy;
    void		*data;
    vwl_data_protocol_T protocol;
} vwl_data_device_T;

typedef struct {
    void		*proxy;
    vwl_data_protocol_T protocol;
} vwl_data_device_manager_T;

// LISTENER WRAPPERS

typedef struct {
    void (*data_offer)(vwl_data_device_T *device, vwl_data_offer_T *offer);

    // If the protocol that the data device uses doesn't support a specific
    // selection, then this callback will never be called with that selection.
    void (*selection)(
	    vwl_data_device_T *device,
	    vwl_data_offer_T *offer,
	    wayland_selection_T selection);

    // This event is only relevant for data control protocols
    void (*finished)(vwl_data_device_T *device);
} vwl_data_device_listener_T;

typedef struct {
    void (*send)(vwl_data_source_T *source, const char *mime_type, int fd);
    void (*cancelled)(vwl_data_source_T *source);
} vwl_data_source_listener_T;

typedef struct {
    void (*offer)(vwl_data_offer_T *offer, const char *mime_type);
} vwl_data_offer_listener_T;

typedef struct
{
    // What selection this refers to
    wayland_selection_T		selection;

    // Do not destroy here
    vwl_data_device_manager_T	manager;

    vwl_data_device_T		device;
    vwl_data_source_T		source;
    vwl_data_offer_T		*offer;	// Current offer for the selection

    garray_T			mime_types;	// Mime types supported by the
						// current offer

    garray_T			tmp_mime_types;	// Temporary array for mime
						// types when we are receiving
						// them. When the selection
						// event arrives and it is the
						// one we want, then copy it
						// over to mime_types

    // To be populated by callbacks from outside this file
    wayland_cb_send_data_func_T		    send_cb;
    wayland_cb_selection_cancelled_func_T   cancelled_cb;

    int requires_focus;		// If focus needs to be given to us to work
} vwl_clipboard_selection_T;

// Holds stuff related to the clipboard/selections
typedef struct {
    // Do not destroy here, will be destroyed when vwl_disconnect_display() is
    // called.
    vwl_seat_T			*seat;

    vwl_clipboard_selection_T	regular;
    vwl_clipboard_selection_T	primary;

    vwl_buffer_store_T		*fs_buffer;
} vwl_clipboard_T;

#endif // FEAT_WAYLAND_CLIPBOARD

static int	vwl_display_flush(vwl_display_T *display);
static void	vwl_callback_done(void *data, struct wl_callback *callback,
		    uint32_t cb_data);
static int	vwl_display_roundtrip(vwl_display_T *display);
static int	vwl_display_dispatch(vwl_display_T *display);
static int vwl_display_dispatch_any(vwl_display_T *display);

static void	vwl_log_handler(const char *fmt, va_list args);
static int	vwl_connect_display(const char *display);
static void	vwl_disconnect_display(void);

static void	vwl_xdg_wm_base_listener_ping(void *data,
		    struct xdg_wm_base *base, uint32_t serial);
static int	vwl_listen_to_registry(void);

static void	vwl_registry_listener_global(void *data,
		    struct wl_registry *registry, uint32_t name,
		    const char *interface, uint32_t version);
static void	vwl_registry_listener_global_remove(void *data,
		    struct wl_registry *registry,  uint32_t name);

static void	vwl_add_seat(struct wl_seat *seat);
static void	vwl_seat_listener_name(void *data, struct wl_seat *seat,
		    const char *name);
static void	vwl_seat_listener_capabilities(void *data, struct wl_seat *seat,
		    uint32_t capabilities);
static void	vwl_destroy_seat(vwl_seat_T *seat);

static vwl_seat_T	    *vwl_get_seat(const char *label);
static struct wl_keyboard   *vwl_seat_get_keyboard(vwl_seat_T *seat);

#ifdef FEAT_WAYLAND_CLIPBOARD

static int	vwl_focus_stealing_available(void);
static void	vwl_xdg_surface_listener_configure(void *data,
		    struct xdg_surface *surface, uint32_t serial);

static void	vwl_bs_buffer_listener_release(void *data,
		    struct wl_buffer *buffer);
static void	vwl_destroy_buffer_store(vwl_buffer_store_T *store);
static vwl_buffer_store_T *vwl_init_buffer_store(int width, int height);

static void	vwl_destroy_fs_surface(vwl_fs_surface_T *store);
static int	vwl_init_fs_surface(vwl_seat_T *seat,
		    vwl_buffer_store_T *buffer_store,
		    void (*on_focus)(void *, uint32_t), void *user_data);

static void	vwl_fs_keyboard_listener_enter(void *data,
		    struct wl_keyboard *keyboard, uint32_t serial,
		    struct wl_surface *surface, struct wl_array *keys);
static void	vwl_fs_keyboard_listener_keymap(void *data,
		    struct wl_keyboard *keyboard, uint32_t format,
		    int fd, uint32_t size);
static void	vwl_fs_keyboard_listener_leave(void *data,
		    struct wl_keyboard *keyboard, uint32_t serial,
		    struct wl_surface *surface);
static void	vwl_fs_keyboard_listener_key(void *data,
		    struct wl_keyboard *keyboard, uint32_t serial,
		    uint32_t time, uint32_t key, uint32_t state);
static void	vwl_fs_keyboard_listener_modifiers(void *data,
		    struct wl_keyboard *keyboard, uint32_t serial,
		    uint32_t mods_depressed, uint32_t mods_latched,
		    uint32_t mods_locked, uint32_t group);
static void	vwl_fs_keyboard_listener_repeat_info(void *data,
		    struct wl_keyboard *keyboard, int32_t rate, int32_t delay);

static void	vwl_gen_data_device_listener_data_offer(void *data,
		    void *offer_proxy);
static void	vwl_gen_data_device_listener_selection(void *data,
		    void *offer_proxy, wayland_selection_T selection,
		    vwl_data_protocol_T protocol);

static void	vwl_data_device_destroy(vwl_data_device_T *device, int alloced);
static void	vwl_data_offer_destroy(vwl_data_offer_T *offer, int alloced);
static void	vwl_data_source_destroy(vwl_data_source_T *source, int alloced);

static void	vwl_data_device_add_listener(vwl_data_device_T *device,
		    void *data);
static void	vwl_data_source_add_listener(vwl_data_source_T *source,
		    void *data);
static void	vwl_data_offer_add_listener(vwl_data_offer_T *offer,
		    void *data);

static void	vwl_data_device_set_selection(vwl_data_device_T *device,
		    vwl_data_source_T *source, uint32_t serial,
		    wayland_selection_T selection);
static void	vwl_data_offer_receive(vwl_data_offer_T *offer,
		    const char *mime_type, int fd);
static int	vwl_get_data_device_manager(vwl_data_device_manager_T *manager,
		    wayland_selection_T selection);
static void	vwl_get_data_device(vwl_data_device_manager_T *manager,
		    vwl_seat_T *seat, vwl_data_device_T *device);
static void	vwl_create_data_source(vwl_data_device_manager_T *manager,
		    vwl_data_source_T *source);
static void	vwl_data_source_offer(vwl_data_source_T *source,
		    const char *mime_type);

static void	vwl_clipboard_free_mime_types(
		    vwl_clipboard_selection_T *clip_sel);
static int	vwl_clipboard_selection_is_ready(
		    vwl_clipboard_selection_T *clip_sel);

static void	vwl_data_device_listener_data_offer(
		    vwl_data_device_T *device, vwl_data_offer_T *offer);
static void	vwl_data_offer_listener_offer(vwl_data_offer_T *offer,
		    const char *mime_type);
static void	vwl_data_device_listener_selection(vwl_data_device_T *device,
		    vwl_data_offer_T *offer, wayland_selection_T selection);
static void	vwl_data_device_listener_finished(vwl_data_device_T *device);

static void	vwl_data_source_listener_send(vwl_data_source_T *source,
		    const char *mime_type, int fd);
static void	vwl_data_source_listener_cancelled(vwl_data_source_T *source);

static void	vwl_on_focus_set_selection(void *data, uint32_t serial);

static void	wayland_set_display(const char *display);

static vwl_data_device_listener_T   vwl_data_device_listener = {
    .data_offer	    = vwl_data_device_listener_data_offer,
    .selection	    = vwl_data_device_listener_selection,
    .finished	    = vwl_data_device_listener_finished
};

static vwl_data_source_listener_T   vwl_data_source_listener = {
    .send	    = vwl_data_source_listener_send,
    .cancelled	    = vwl_data_source_listener_cancelled
};

static vwl_data_offer_listener_T    vwl_data_offer_listener = {
    .offer	    = vwl_data_offer_listener_offer
};

static struct xdg_wm_base_listener  vwl_xdg_wm_base_listener = {
    .ping	    = vwl_xdg_wm_base_listener_ping
};

static struct xdg_surface_listener  vwl_xdg_surface_listener = {
    .configure	    = vwl_xdg_surface_listener_configure
};

static struct wl_buffer_listener    vwl_cb_buffer_listener = {
    .release	    = vwl_bs_buffer_listener_release
};

static struct wl_keyboard_listener  vwl_fs_keyboard_listener = {
    .enter	    = vwl_fs_keyboard_listener_enter,
    .key	    = vwl_fs_keyboard_listener_key,
    .keymap	    = vwl_fs_keyboard_listener_keymap,
    .leave	    = vwl_fs_keyboard_listener_leave,
    .modifiers	    = vwl_fs_keyboard_listener_modifiers,
    .repeat_info    = vwl_fs_keyboard_listener_repeat_info
};

#endif // FEAT_WAYLAND_CLIPBOARD

static struct wl_callback_listener  vwl_callback_listener = {
    .done	    = vwl_callback_done
};

static struct wl_registry_listener  vwl_registry_listener = {
    .global	    = vwl_registry_listener_global,
    .global_remove  = vwl_registry_listener_global_remove
};

static struct wl_seat_listener	    vwl_seat_listener = {
    .name	    = vwl_seat_listener_name,
    .capabilities   = vwl_seat_listener_capabilities
};

static vwl_display_T		    vwl_display;
static vwl_global_objects_T	    vwl_gobjects;
static garray_T			    vwl_seats;

#ifdef FEAT_WAYLAND_CLIPBOARD
// Make sure to sync this with vwl_cb_uninit since it memsets this to zero
static vwl_clipboard_T	vwl_clipboard = {
    .regular.selection = WAYLAND_SELECTION_REGULAR,
    .primary.selection = WAYLAND_SELECTION_PRIMARY,
};

// Only really used for debugging/testing purposes in order to force focus
// stealing even when a data control protocol is available.
static int force_fs  = FALSE;
#endif

/*
 * Like wl_display_flush but always writes all the data in the buffer to the
 * display fd. Returns FAIL on failure and OK on success.
 */
    static int
vwl_display_flush(vwl_display_T *display)
{
    int ret;

#ifndef HAVE_SELECT
    struct pollfd fds;

    fds.fd	= display->fd;
    fds.events	= POLLOUT;
#else
    fd_set	    wfds;
    struct timeval  tv;

    FD_ZERO(&wfds);
    FD_SET(display->fd, &wfds);

    tv.tv_sec	= 0;
    tv.tv_usec	= p_wtm * 1000;
#endif

    if (display->proxy == NULL)
	return FAIL;

    // Send the requests we have made to the compositor, until we have written
    // all the data. Poll in order to check if the display fd is writable, if
    // not, then wait until it is and continue writing or until we timeout.
    while (errno = 0, (ret = wl_display_flush(display->proxy)) == -1
	    && errno == EAGAIN)
    {
#ifndef HAVE_SELECT
	if (poll(&fds, 1, p_wtm) <= 0)
#else
	    if (select(display->fd + 1, NULL, &wfds, NULL, &tv) <= 0)
#endif
		return FAIL;
    }
    // Return FAIL on error or timeout
    if ((errno != 0 && errno != EAGAIN) || ret == -1)
	return FAIL;

    return OK;
}

/*
 * Called when compositor is done processing requests/events.
 */
    static void
vwl_callback_done(void *data, struct wl_callback *callback,
	uint32_t cb_data UNUSED)
{
    *((int*)data) = TRUE;
    wl_callback_destroy(callback);
}

/*
 * Like wl_display_roundtrip but polls the display fd with a timeout. Returns
 * FAIL on failure and OK on success.
 */
    static int
vwl_display_roundtrip(vwl_display_T *display)
{
    struct wl_callback	*callback;
    int			ret, done = FALSE;
    struct timeval start, now;

    if (display->proxy == NULL)
	return FAIL;

    // Tell compositor to emit 'done' event after processing all requests we
    // have sent and handling events.
    callback = wl_display_sync(display->proxy);

    if (callback == NULL)
	return FAIL;

    wl_callback_add_listener(callback, &vwl_callback_listener, &done);

    gettimeofday(&start, NULL);

    // Wait till we get the done event (which will set `done` to TRUE), unless
    // we timeout
    while (TRUE)
    {
	ret = vwl_display_dispatch(display);

	if (done || ret == -1)
	    break;

	gettimeofday(&now, NULL);

	if ((now.tv_sec * 1000000 + now.tv_usec) -
		(start.tv_sec * 1000000 + start.tv_usec) >= p_wtm * 1000)
	{
	    ret = -1;
	    break;
	}
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
 * Like wl_display_roundtrip but polls the display fd with a timeout. Returns
 * number of events dispatched on success else -1 on failure.
 */
    static int
vwl_display_dispatch(vwl_display_T *display)
{
#ifndef HAVE_SELECT
    struct pollfd   fds;

    fds.fd	    = display->fd;
    fds.events	    = POLLIN;
#else
    fd_set          rfds;
    struct timeval  tv;

    FD_ZERO(&rfds);
    FD_SET(display->fd, &rfds);

    tv.tv_sec	    = 0;
    tv.tv_usec	    = p_wtm * 1000;
#endif

    if (display->proxy == NULL)
	return -1;

    while (wl_display_prepare_read(display->proxy) == -1)
	// Dispatch any queued events so that we can start reading
	if (wl_display_dispatch_pending(display->proxy) == -1)
	    return -1;

    // Send any requests before we starting blocking to read display fd
    if (vwl_display_flush(display) == FAIL)
    {
	wl_display_cancel_read(display->proxy);
	return -1;
    }

    // Poll until there is data to read from the display fd.
#ifndef HAVE_SELECT
    if (poll(&fds, 1, p_wtm) <= 0)
#else
    if (select(display->fd + 1, &rfds, NULL, NULL, &tv) <= 0)
#endif
	{
	    wl_display_cancel_read(display->proxy);
	    return -1;
	}

    // Read events into the queue
    if (wl_display_read_events(display->proxy) == -1)
	return -1;

    // Dispatch those events (call the handlers associated for each event)
    return wl_display_dispatch_pending(display->proxy);
}

/*
 * Same as vwl_display_dispatch but poll/select is never called. This is useful
 * is poll/select was already called before or if you just want to dispatch any
 * events that happen to be waiting to be dispatched on the display fd.
 */
    static int
vwl_display_dispatch_any(vwl_display_T *display)
{
    if (display->proxy == NULL)
	return -1;

    while (wl_display_prepare_read(display->proxy) == -1)
	// Dispatch any queued events so that we can start reading
	if (wl_display_dispatch_pending(display->proxy) == -1)
	    return -1;

    // Send any requests before we starting blocking to read display fd
    if (vwl_display_flush(display) == FAIL)
    {
	wl_display_cancel_read(display->proxy);
	return -1;
    }

    // Read events into the queue
    if (wl_display_read_events(display->proxy) == -1)
	return -1;

    // Dispatch those events (call the handlers associated for each event)
    return wl_display_dispatch_pending(display->proxy);
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

    ch_log(NULL, "%s", buf);
    emsg(buf);

    vim_free(buf);
}

/*
 * Connect to the display with name; passing NULL will use libwayland's way of
 * getting the display. Additionally get the registry object but will not
 * starting listening. Returns OK on sucess and FAIL on failure.
 */
    static int
vwl_connect_display(const char *display)
{
    if (wayland_no_connect)
	return FAIL;

    // We will get an error if XDG_RUNTIME_DIR is not set.
    if (mch_getenv("XDG_RUNTIME_DIR") == NULL)
	return FAIL;

    // Must set log handler before we connect display in order to work.
    wl_log_set_handler_client(vwl_log_handler);

    vwl_display.proxy = wl_display_connect(display);

    if (vwl_display.proxy == NULL)
	return FAIL;

    wayland_set_display(display);
    vwl_display.fd = wl_display_get_fd(vwl_display.proxy);

    vwl_display.registry.proxy = wl_display_get_registry(vwl_display.proxy);

    if (vwl_display.registry.proxy == NULL)
    {
	vwl_disconnect_display();
	return FAIL;
    }

    return OK;
}

#define destroy_gobject(object) \
    if (vwl_gobjects.object != NULL) \
    { \
	object##_destroy(vwl_gobjects.object); \
	vwl_gobjects.object = NULL; \
    }

/*
 * Disconnects the display and frees up all resources, including all global
 * objects.
 */
    static void
vwl_disconnect_display(void)
{

    destroy_gobject(ext_data_control_manager_v1)
    destroy_gobject(zwlr_data_control_manager_v1)
    destroy_gobject(wl_data_device_manager)
    destroy_gobject(wl_shm)
    destroy_gobject(wl_compositor)
    destroy_gobject(xdg_wm_base)
    destroy_gobject(zwp_primary_selection_device_manager_v1)

    for (int i = 0; i < vwl_seats.ga_len; i++)
	vwl_destroy_seat(&((vwl_seat_T *)vwl_seats.ga_data)[i]);
    ga_clear(&vwl_seats);
    vwl_seats.ga_len = 0;

    if (vwl_display.registry.proxy != NULL)
    {
	wl_registry_destroy(vwl_display.registry.proxy);
	vwl_display.registry.proxy = NULL;
    }
    if (vwl_display.proxy != NULL)
    {
	wl_display_disconnect(vwl_display.proxy);
	vwl_display.proxy = NULL;
    }
}

/*
 * Tells the compositor we are still responsive.
 */
    static void
vwl_xdg_wm_base_listener_ping(
	void *data UNUSED,
	struct xdg_wm_base *base,
	uint32_t serial)
{
    xdg_wm_base_pong(base, serial);
}

/*
 * Start listening to the registry and get initial set of global
 * objects/interfaces.
 */
    static int
vwl_listen_to_registry(void)
{
    // Only meant for debugging/testing purposes
    char_u *env = mch_getenv("VIM_WAYLAND_FORCE_FS");

    if (env != NULL && STRCMP(env, "1") == 0)
	force_fs = TRUE;
    else
	force_fs = FALSE;

    ga_init2(&vwl_seats, sizeof(vwl_seat_T), 1);

    wl_registry_add_listener(
	    vwl_display.registry.proxy,
	    &vwl_registry_listener,
	    NULL);

    if (vwl_display_roundtrip(&vwl_display) == FAIL)
	return FAIL;

#ifdef FEAT_WAYLAND_CLIPBOARD
    // If we have a suitable data control protocol discard the rest. If we only
    // have wlr data control protocol but its version is 1, then don't discard
    // globals if we also have the primary selection protocol.
    if (!force_fs &&
	    (vwl_gobjects.ext_data_control_manager_v1 != NULL ||
	     (vwl_gobjects.zwlr_data_control_manager_v1 != NULL &&
	      zwlr_data_control_manager_v1_get_version(
		  vwl_gobjects.zwlr_data_control_manager_v1) > 1)))
    {
	destroy_gobject(wl_data_device_manager)
	destroy_gobject(wl_shm)
	destroy_gobject(wl_compositor)
	destroy_gobject(xdg_wm_base)
    }
    else
	// Be ready for ping events
	xdg_wm_base_add_listener(
		vwl_gobjects.xdg_wm_base,
		&vwl_xdg_wm_base_listener,
		NULL);
#endif
    return OK;
}

#define SET_GOBJECT(object, min_ver) \
    do { \
	chosen_interface = &object##_interface; \
	object_member = (void*)&vwl_gobjects.object; \
	min_version = min_ver; \
    } while (0)

/*
 * Callback for global event, for each global interface the compositor supports.
 * Keep in sync with vwl_disconnect_display().
 */
    static void
vwl_registry_listener_global(
	void		    *data UNUSED,
	struct wl_registry  *registry UNUSED,
	uint32_t	    name,
	const char	    *interface,
	uint32_t	    version)
{

    const struct wl_interface	*chosen_interface = NULL;
    void			*proxy;
    uint32_t			min_version;
    void			**object_member;

    if (STRCMP(interface, wl_seat_interface.name) == 0)
    {
	chosen_interface = &wl_seat_interface;
	min_version = 2;
    }
#ifdef FEAT_WAYLAND_CLIPBOARD
    else if (STRCMP(interface, zwlr_data_control_manager_v1_interface.name) == 0)
	SET_GOBJECT(zwlr_data_control_manager_v1, 1);

    else if (STRCMP(interface, ext_data_control_manager_v1_interface.name) == 0)
	SET_GOBJECT(ext_data_control_manager_v1, 1);

    else if (STRCMP(interface, wl_data_device_manager_interface.name) == 0)
	SET_GOBJECT(wl_data_device_manager, 1);

    else if (STRCMP(interface, wl_shm_interface.name) == 0)
	SET_GOBJECT(wl_shm, 1);

    else if (STRCMP(interface, wl_compositor_interface.name) == 0)
	SET_GOBJECT(wl_compositor, 2);

    else if (STRCMP(interface, xdg_wm_base_interface.name) == 0)
	SET_GOBJECT(xdg_wm_base, 1);

    else if (STRCMP(interface,
		zwp_primary_selection_device_manager_v1_interface.name) == 0)
	SET_GOBJECT(zwp_primary_selection_device_manager_v1, 1);
#endif

    if (chosen_interface == NULL || version < min_version)
	return;

    proxy = wl_registry_bind(vwl_display.registry.proxy, name, chosen_interface,
	    version);

    if (chosen_interface == &wl_seat_interface)
	// Add seat to vwl_seats array, as we can have multiple seats.
	vwl_add_seat(proxy);
    else
	// Hold proxy & name in the vwl_gobject struct
	*object_member = proxy;
}

/*
 * Called when a global object is removed, if so, then do nothing. This is to
 * avoid a global being removed while it is in the process of being used. Let
 * the user call :wlrestore in order to reset everything. Requests to that
 * global will just be ignored on the compositor side.
 */
    static void
vwl_registry_listener_global_remove(
	void		    *data UNUSED,
	struct wl_registry  *registry UNUSED,
	uint32_t	    name UNUSED)
{
}

/*
 * Add a new seat given its proxy to the global grow array
 */
    static void
vwl_add_seat(struct wl_seat *seat_proxy)
{
    vwl_seat_T *seat;

    if (ga_grow(&vwl_seats, 1) == FAIL)
	return;

    seat = &((vwl_seat_T *)vwl_seats.ga_data)[vwl_seats.ga_len];

    seat->proxy = seat_proxy;

    // Get label and capabilities
    wl_seat_add_listener(seat_proxy, &vwl_seat_listener, seat);

    if (vwl_display_roundtrip(&vwl_display) == FAIL)
	return;

    // Check if label has been allocated
    if (seat->label == NULL)
	return;

    vwl_seats.ga_len++;
}

/*
 * Callback for seat text label/name
 */
    static void
vwl_seat_listener_name(
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
vwl_seat_listener_capabilities(
	void		*data,
	struct wl_seat	*seat_proxy UNUSED,
	uint32_t	capabilities)
{
    vwl_seat_T *seat = data;

    seat->capabilities = capabilities;
}

/*
 * Destroy/free seat.
 */
    static void
vwl_destroy_seat(vwl_seat_T *seat)
{
    if (seat->proxy != NULL)
    {
	if (wl_seat_get_version(seat->proxy) >= 5)
	    // Helpful for the compositor
	    wl_seat_release(seat->proxy);
	else
	    wl_seat_destroy(seat->proxy);
	seat->proxy = NULL;
    }
    vim_free(seat->label);
    seat->label = NULL;
}

/*
 * Return a seat with the give name/label. If none exists then NULL is returned.
 * If NULL or an empty string is passed as the label then $XDG_SEAT is used
 * else the first available seat found is used.
 */
    static vwl_seat_T *
vwl_get_seat(const char *label)
{
    if ((STRCMP(label, "") == 0 || label == NULL) && vwl_seats.ga_len > 0)
    {
	const char *xdg_seat = (char*)mch_getenv("XDG_SEAT");

	if (xdg_seat == NULL)
	    return &((vwl_seat_T *)vwl_seats.ga_data)[0];
	else
	    label = xdg_seat;
    }

    for (int i = 0; i < vwl_seats.ga_len; i++)
    {
	vwl_seat_T *seat = &((vwl_seat_T *)vwl_seats.ga_data)[i];
	if (STRCMP(seat->label, label) == 0)
	    return seat;
    }
    return NULL;
}

/*
 * Get keyboard object from seat and return it. NULL is returned on
 * failure such as when a keyboard is not available for seat.
 */
    static struct wl_keyboard *
vwl_seat_get_keyboard(vwl_seat_T *seat)
{
    if (!(seat->capabilities & WL_SEAT_CAPABILITY_KEYBOARD))
	return NULL;

    return wl_seat_get_keyboard(seat->proxy);
}

/*
 * Connects to the wayland display with given name and binds to global objects
 * as needed. If display is NULL then the $WAYLAND_DISPLAY environment variable
 * will be used (handled by libwayland). Returns FAIL on failure and OK on
 * success
 */
    int
wayland_init_client(const char *display)
{
    wayland_set_display(display);

    if (vwl_connect_display(display) == FAIL ||
	    vwl_listen_to_registry() == FAIL)
	goto fail;

    wayland_display_fd = vwl_display.fd;

    return OK;
fail:
    // Set v:wayland_display to empty string (but not wayland_display_name)
    wayland_set_display("");
    return FAIL;
}

/*
 * Disconnect wayland client and free up all resources used.
 */
    void
wayland_uninit_client(void)
{
#ifdef FEAT_WAYLAND_CLIPBOARD
    wayland_cb_uninit();
#endif
    vwl_disconnect_display();

    wayland_set_display("");
}

/*
 * Return TRUE if wayland display connection is valid and ready.
 */
    int
wayland_client_is_connected(int quiet)
{
    if (vwl_display.proxy == NULL)
	goto error;

    // Display errors are always fatal
    if (wl_display_get_error(vwl_display.proxy) != 0
	    || vwl_display_flush(&vwl_display) == FAIL)
	goto error;

    return TRUE;
error:
    if (!quiet)
	emsg(e_wayland_connection_unavailable);
    return FALSE;
}

/*
 * Flush requests and process new Wayland events, does not poll the display file
 * descriptor.
 */
    int
wayland_client_update(void)
{
    return vwl_display_dispatch_any(&vwl_display) == -1 ? FAIL : OK;
}

#ifdef FEAT_WAYLAND_CLIPBOARD

/*
 * If globals required for focus stealing method is available.
 */
    static int
vwl_focus_stealing_available(void)
{
    return (p_wst || force_fs) &&
	vwl_gobjects.wl_compositor != NULL &&
	vwl_gobjects.wl_shm != NULL &&
	vwl_gobjects.xdg_wm_base != NULL;
}

/*
 * Configure xdg_surface
 */
    static void
vwl_xdg_surface_listener_configure(
	void		    *data UNUSED,
	struct xdg_surface  *surface,
	uint32_t	    serial)
{
    xdg_surface_ack_configure(surface, serial);
}

/*
 * Called when compositor isn't using the buffer anymore, we can reuse it again.
 */
    static void
vwl_bs_buffer_listener_release(
	void		    *data,
	struct wl_buffer    *buffer UNUSED)
{
    vwl_buffer_store_T *store = data;

    store->available = TRUE;
}

/*
 * Destroy a buffer store structure.
 */
    static void
vwl_destroy_buffer_store(vwl_buffer_store_T *store)
{
    if (store->buffer != NULL)
	wl_buffer_destroy(store->buffer);
    if (store->pool != NULL)
	wl_shm_pool_destroy(store->pool);

    close(store->fd);

    vim_free(store);
}

/*
 * Initialize a buffer and its backing memory pool.
 */
    static vwl_buffer_store_T *
vwl_init_buffer_store(int width, int height)
{
    int			fd, r;
    vwl_buffer_store_T	*store;

    if (vwl_gobjects.wl_shm == NULL)
	return NULL;

    store = alloc(sizeof(*store));

    if (store == NULL)
	return NULL;

    store->available = FALSE;

    store->width = width;
    store->height = height;
    store->stride = store->width * 4;
    store->size = store->stride * store->height;

    fd = mch_create_anon_file();
    r = ftruncate(fd, store->size);

    if (r == -1)
    {
	if (fd >= 0)
	    close(fd);
	return NULL;
    }

    store->pool = wl_shm_create_pool(vwl_gobjects.wl_shm, fd, store->size);
    store->buffer = wl_shm_pool_create_buffer(
	    store->pool,
	    0,
	    store->width,
	    store->height,
	    store->stride,
	    WL_SHM_FORMAT_ARGB8888);

    store->fd = fd;

    wl_buffer_add_listener(store->buffer, &vwl_cb_buffer_listener, store);

    if (vwl_display_roundtrip(&vwl_display) == -1)
    {
	vwl_destroy_buffer_store(store);
	return NULL;
    }

    store->available = TRUE;

    return store;
}

/*
 * Destroy a focus stealing store structure.
 */
    static void
vwl_destroy_fs_surface(vwl_fs_surface_T *store)
{
    if (store->shell.toplevel != NULL)
	xdg_toplevel_destroy(store->shell.toplevel);
    if (store->shell.surface != NULL)
	xdg_surface_destroy(store->shell.surface);
    if (store->surface != NULL)
	wl_surface_destroy(store->surface);
    if (store->keyboard != NULL)
    {
	if (wl_keyboard_get_version(store->keyboard) >= 3)
	    wl_keyboard_release(store->keyboard);
	else
	    wl_keyboard_destroy(store->keyboard);
    }
    vim_free(store);
}

/*
 * Create an invisible surface in order to gain focus and call on_focus() with
 * serial that was given.
 */
    static int
vwl_init_fs_surface(
	vwl_seat_T	    *seat,
	vwl_buffer_store_T  *buffer_store,
	void		    (*on_focus)(void *, uint32_t),
	void		    *user_data)
{
    vwl_fs_surface_T *store;

    if (vwl_gobjects.wl_compositor == NULL || vwl_gobjects.xdg_wm_base == NULL)
	return FAIL;
    if (buffer_store == NULL || seat == NULL)
	return FAIL;

    store = alloc_clear(sizeof(*store));

    if (store == NULL)
	return FAIL;

    // Get keyboard
    store->keyboard = vwl_seat_get_keyboard(seat);

    if (store->keyboard == NULL)
	goto fail;

    wl_keyboard_add_listener(store->keyboard, &vwl_fs_keyboard_listener, store);

    if (vwl_display_dispatch(&vwl_display) == -1)
	goto fail;

    store->surface = wl_compositor_create_surface(vwl_gobjects.wl_compositor);
    store->shell.surface = xdg_wm_base_get_xdg_surface(
	    vwl_gobjects.xdg_wm_base, store->surface);
    store->shell.toplevel = xdg_surface_get_toplevel(store->shell.surface);

    xdg_toplevel_set_title(store->shell.toplevel, "Vim clipboard");

    xdg_surface_add_listener(store->shell.surface,
	    &vwl_xdg_surface_listener, NULL);

    wl_surface_commit(store->surface);

    store->on_focus = on_focus;
    store->user_data = user_data;
    store->got_focus = FALSE;

    if (vwl_display_roundtrip(&vwl_display) == FAIL)
	goto fail;

    // We may get the enter event early, if we do then we will set `got_focus`
    // to TRUE.
    if (store->got_focus)
	goto early_exit;

    // Buffer hasn't been released yet, abort. This shouldn't happen but still
    // check for it.
    if (!buffer_store->available)
	goto fail;

    buffer_store->available = FALSE;

    wl_surface_attach(store->surface, buffer_store->buffer, 0, 0);
    wl_surface_damage(store->surface, 0, 0,
	    buffer_store->width, buffer_store->height);
    wl_surface_commit(store->surface);

    {
	// Dispatch events until we receive the enter event. Add a max delay of
	// 'p_wtm' when waiting for it (may be longer depending on how long we
	// poll when dispatching events)
	struct timeval start, now;

	gettimeofday(&start, NULL);

	while (vwl_display_dispatch(&vwl_display) != -1)
	{
	    if (store->got_focus)
		break;

	    gettimeofday(&now, NULL);

	    if ((now.tv_sec * 1000000 + now.tv_usec) -
		    (start.tv_sec * 1000000 + start.tv_usec)
		    >= p_wtm * 1000)
		goto fail;
	}
    }
early_exit:
    vwl_destroy_fs_surface(store);
    vwl_display_flush(&vwl_display);

    return OK;
fail:
    vwl_destroy_fs_surface(store);
    vwl_display_flush(&vwl_display);

    return FAIL;
}

/*
 * Called when the keyboard focus is on our surface
 */
    static void
vwl_fs_keyboard_listener_enter(
    void		*data,
    struct wl_keyboard	*keyboard UNUSED,
    uint32_t		serial,
    struct wl_surface	*surface UNUSED,
    struct wl_array	*keys UNUSED)
{
    vwl_fs_surface_T *store = data;

    store->got_focus = TRUE;

    if (store->on_focus != NULL)
	store->on_focus(store->user_data, serial);
}

// Dummy functions to handle keyboard events we don't care about.

    static void
vwl_fs_keyboard_listener_keymap(
    void		*data UNUSED,
    struct wl_keyboard	*keyboard UNUSED,
    uint32_t		format UNUSED,
    int			fd,
    uint32_t		size UNUSED)
{
    close(fd);
}

    static void
vwl_fs_keyboard_listener_leave(
    void		*data UNUSED,
    struct wl_keyboard	*keyboard UNUSED,
    uint32_t		serial UNUSED,
    struct wl_surface	*surface UNUSED)
{
}

    static void
vwl_fs_keyboard_listener_key(
    void		*data UNUSED,
    struct wl_keyboard	*keyboard UNUSED,
    uint32_t		serial UNUSED,
    uint32_t		time UNUSED,
    uint32_t		key UNUSED,
    uint32_t		state UNUSED)
{
}

    static void
vwl_fs_keyboard_listener_modifiers(
    void		*data UNUSED,
    struct wl_keyboard	*keyboard UNUSED,
    uint32_t		serial UNUSED,
    uint32_t		mods_depressed UNUSED,
    uint32_t		mods_latched UNUSED,
    uint32_t		mods_locked UNUSED,
    uint32_t		group UNUSED)
{
}

    static void
vwl_fs_keyboard_listener_repeat_info(
    void		*data UNUSED,
    struct wl_keyboard	*keyboard UNUSED,
    int32_t		rate UNUSED,
    int32_t		delay UNUSED)
{
}

#define VWL_CODE_DATA_OBJECT_DESTROY(type) \
do { \
    if (type == NULL || type->proxy == NULL) \
	return; \
    switch (type->protocol) \
    { \
	case VWL_DATA_PROTOCOL_WLR: \
	    zwlr_data_control_##type##_v1_destroy(type->proxy); \
	    break; \
	case VWL_DATA_PROTOCOL_EXT:  \
	    ext_data_control_##type##_v1_destroy(type->proxy); \
	    break; \
	case VWL_DATA_PROTOCOL_CORE: \
	    wl_data_##type##_destroy(type->proxy); \
	    break; \
	case VWL_DATA_PROTOCOL_PRIMARY: \
	    zwp_primary_selection_##type##_v1_destroy(type->proxy); \
	    break; \
	default: \
	    break; \
    } \
    if (alloced) \
	vim_free(type); \
    else \
	type->proxy = NULL; \
} while (0)

    static void
vwl_data_device_destroy(vwl_data_device_T *device, int alloced)
{
    VWL_CODE_DATA_OBJECT_DESTROY(device);
}

    static void
vwl_data_offer_destroy(vwl_data_offer_T *offer, int alloced)
{
    VWL_CODE_DATA_OBJECT_DESTROY(offer);
}

    static void
vwl_data_source_destroy(vwl_data_source_T *source, int alloced)
{
    VWL_CODE_DATA_OBJECT_DESTROY(source);
}


// Used to pass a vwl_data_offer_T struct from the data_offer event to the offer
// event and to the selection event.
static vwl_data_offer_T *tmp_vwl_offer;

// These functions handle the more complicated data_offer and selection events.

    static void
vwl_gen_data_device_listener_data_offer(void *data, void *offer_proxy)
{
    vwl_data_device_T *device = data;

    tmp_vwl_offer = alloc(sizeof(*tmp_vwl_offer));

    if (tmp_vwl_offer != NULL)
    {
	tmp_vwl_offer->proxy = offer_proxy;
	tmp_vwl_offer->protocol = device->protocol;

	vwl_data_device_listener.data_offer(device, tmp_vwl_offer);
    }
}

    static void
vwl_gen_data_device_listener_selection(
	void		    *data,
	void		    *offer_proxy,
	wayland_selection_T selection,
	vwl_data_protocol_T protocol)
{
    if (tmp_vwl_offer == NULL)
    {
	// Memory allocation failed or selection cleared (data_offer is never
	// sent when selection is cleared/empty).
	vwl_data_offer_T tmp = {
	    .proxy = offer_proxy,
	    .protocol = protocol
	};

	vwl_data_offer_destroy(&tmp, FALSE);

	// If offer proxy is NULL then we know the selection has been cleared.
	if (offer_proxy == NULL)
	    vwl_data_device_listener.selection(data, NULL, selection);
    }
    else
    {
	vwl_data_device_listener.selection(data, tmp_vwl_offer, selection);
	tmp_vwl_offer = NULL;
    }
}

// Boilerplate macros. Each just calls its respective generic callback.
//
#define VWL_FUNC_DATA_DEVICE_DATA_OFFER(device_name, offer_name) \
    static void device_name##_listener_data_offer( \
	    void *data, struct device_name *device_proxy UNUSED, \
	    struct offer_name *offer_proxy) \
{ \
    vwl_gen_data_device_listener_data_offer(data, offer_proxy); \
}
#define VWL_FUNC_DATA_DEVICE_SELECTION( \
	device_name, offer_name, type, selection_type, protocol) \
	static void device_name##_listener_##type( \
		void *data, struct device_name *device_proxy UNUSED, \
		struct offer_name *offer_proxy UNUSED) \
{ \
    vwl_gen_data_device_listener_selection( \
	    data, offer_proxy, selection_type, protocol); \
}
#define VWL_FUNC_DATA_DEVICE_FINISHED(device_name) \
    static void device_name##_listener_finished( \
	    void *data, struct device_name *device_proxy UNUSED) \
{ \
    vwl_data_device_listener.finished(data); \
}
#define VWL_FUNC_DATA_SOURCE_SEND(source_name) \
    static void source_name##_listener_send(void *data, \
	    struct source_name *source_proxy UNUSED, \
	    const char *mime_type, int fd) \
{ \
    vwl_data_source_listener.send(data, mime_type, fd); \
}
#define VWL_FUNC_DATA_SOURCE_CANCELLED(source_name) \
    static void source_name##_listener_cancelled(void *data, \
	    struct source_name *source_proxy UNUSED) \
{ \
    vwl_data_source_listener.cancelled(data); \
}
#define VWL_FUNC_DATA_OFFER_OFFER(offer_name) \
    static void offer_name##_listener_offer(void *data, \
	    struct offer_name *offer_proxy UNUSED, \
	    const char *mime_type) \
{ \
    vwl_data_offer_listener.offer(data, mime_type); \
}

VWL_FUNC_DATA_DEVICE_DATA_OFFER(
	ext_data_control_device_v1, ext_data_control_offer_v1)
VWL_FUNC_DATA_DEVICE_DATA_OFFER(
	zwlr_data_control_device_v1, zwlr_data_control_offer_v1)
VWL_FUNC_DATA_DEVICE_DATA_OFFER(wl_data_device, wl_data_offer)
VWL_FUNC_DATA_DEVICE_DATA_OFFER(
	zwp_primary_selection_device_v1, zwp_primary_selection_offer_v1)

VWL_FUNC_DATA_DEVICE_SELECTION(
	ext_data_control_device_v1, ext_data_control_offer_v1,
	selection, WAYLAND_SELECTION_REGULAR, VWL_DATA_PROTOCOL_EXT)
VWL_FUNC_DATA_DEVICE_SELECTION(
	zwlr_data_control_device_v1, zwlr_data_control_offer_v1,
	selection, WAYLAND_SELECTION_REGULAR, VWL_DATA_PROTOCOL_WLR)
VWL_FUNC_DATA_DEVICE_SELECTION(
	wl_data_device, wl_data_offer, selection,
	WAYLAND_SELECTION_REGULAR, VWL_DATA_PROTOCOL_CORE)

VWL_FUNC_DATA_DEVICE_SELECTION(
	ext_data_control_device_v1, ext_data_control_offer_v1,
	primary_selection, WAYLAND_SELECTION_PRIMARY, VWL_DATA_PROTOCOL_EXT)
VWL_FUNC_DATA_DEVICE_SELECTION(
	zwlr_data_control_device_v1, zwlr_data_control_offer_v1,
	primary_selection, WAYLAND_SELECTION_PRIMARY, VWL_DATA_PROTOCOL_WLR)
VWL_FUNC_DATA_DEVICE_SELECTION(
	zwp_primary_selection_device_v1, zwp_primary_selection_offer_v1,
	primary_selection, WAYLAND_SELECTION_PRIMARY, VWL_DATA_PROTOCOL_PRIMARY)

VWL_FUNC_DATA_DEVICE_FINISHED(ext_data_control_device_v1)
VWL_FUNC_DATA_DEVICE_FINISHED(zwlr_data_control_device_v1)

VWL_FUNC_DATA_SOURCE_SEND(ext_data_control_source_v1)
VWL_FUNC_DATA_SOURCE_SEND(zwlr_data_control_source_v1)
VWL_FUNC_DATA_SOURCE_SEND(wl_data_source)
VWL_FUNC_DATA_SOURCE_SEND(zwp_primary_selection_source_v1)

VWL_FUNC_DATA_SOURCE_CANCELLED(ext_data_control_source_v1)
VWL_FUNC_DATA_SOURCE_CANCELLED(zwlr_data_control_source_v1)
VWL_FUNC_DATA_SOURCE_CANCELLED(wl_data_source)
VWL_FUNC_DATA_SOURCE_CANCELLED(zwp_primary_selection_source_v1)

VWL_FUNC_DATA_OFFER_OFFER(ext_data_control_offer_v1)
VWL_FUNC_DATA_OFFER_OFFER(zwlr_data_control_offer_v1)
VWL_FUNC_DATA_OFFER_OFFER(wl_data_offer)
VWL_FUNC_DATA_OFFER_OFFER(zwp_primary_selection_offer_v1)

// Listener handlers

// DATA DEVICES
struct zwlr_data_control_device_v1_listener
zwlr_data_control_device_v1_listener = {
    .data_offer	    = zwlr_data_control_device_v1_listener_data_offer,
    .selection	    = zwlr_data_control_device_v1_listener_selection,
    .primary_selection = zwlr_data_control_device_v1_listener_primary_selection,
    .finished	    = zwlr_data_control_device_v1_listener_finished
};

struct ext_data_control_device_v1_listener
ext_data_control_device_v1_listener = {
    .data_offer	    = ext_data_control_device_v1_listener_data_offer,
    .selection	    = ext_data_control_device_v1_listener_selection,
    .primary_selection = ext_data_control_device_v1_listener_primary_selection,
    .finished	    = ext_data_control_device_v1_listener_finished
};

struct wl_data_device_listener wl_data_device_listener = {
    .data_offer	    = wl_data_device_listener_data_offer,
    .selection	    = wl_data_device_listener_selection,
};

struct zwp_primary_selection_device_v1_listener
zwp_primary_selection_device_v1_listener = {
    .selection	= zwp_primary_selection_device_v1_listener_primary_selection,
    .data_offer	    = zwp_primary_selection_device_v1_listener_data_offer
};

// DATA SOURCES
struct zwlr_data_control_source_v1_listener
zwlr_data_control_source_v1_listener = {
    .send	    = zwlr_data_control_source_v1_listener_send,
    .cancelled	    = zwlr_data_control_source_v1_listener_cancelled
};

struct ext_data_control_source_v1_listener
ext_data_control_source_v1_listener = {
    .send	    = ext_data_control_source_v1_listener_send,
    .cancelled	    = ext_data_control_source_v1_listener_cancelled
};

struct wl_data_source_listener wl_data_source_listener = {
    .send	    = wl_data_source_listener_send,
    .cancelled	    = wl_data_source_listener_cancelled
};

struct zwp_primary_selection_source_v1_listener
zwp_primary_selection_source_v1_listener = {
    .send	    = zwp_primary_selection_source_v1_listener_send,
    .cancelled	    = zwp_primary_selection_source_v1_listener_cancelled,
};

// OFFERS
struct zwlr_data_control_offer_v1_listener
zwlr_data_control_offer_v1_listener = {
    .offer	    = zwlr_data_control_offer_v1_listener_offer
};

struct ext_data_control_offer_v1_listener
ext_data_control_offer_v1_listener = {
    .offer	    = ext_data_control_offer_v1_listener_offer
};

struct wl_data_offer_listener wl_data_offer_listener = {
    .offer	    = wl_data_offer_listener_offer
};

struct zwp_primary_selection_offer_v1_listener
zwp_primary_selection_offer_v1_listener = {
    .offer	    = zwp_primary_selection_offer_v1_listener_offer
};

// `type` is also used as the user data
#define VWL_CODE_DATA_OBJECT_ADD_LISTENER(type) \
do { \
    if (type->proxy == NULL) \
	return; \
    type->data = data; \
    switch (type->protocol) \
    { \
	case VWL_DATA_PROTOCOL_WLR: \
	    zwlr_data_control_##type##_v1_add_listener( type->proxy, \
		    &zwlr_data_control_##type##_v1_listener, type); \
	    break; \
	case VWL_DATA_PROTOCOL_EXT: \
	    ext_data_control_##type##_v1_add_listener(type->proxy, \
		    &ext_data_control_##type##_v1_listener, type); \
	    break; \
	case VWL_DATA_PROTOCOL_CORE: \
	    wl_data_##type##_add_listener(type->proxy, \
		    &wl_data_##type##_listener, type); \
	    break; \
	case VWL_DATA_PROTOCOL_PRIMARY: \
	    zwp_primary_selection_##type##_v1_add_listener(type->proxy, \
		    &zwp_primary_selection_##type##_v1_listener, type); \
	    break; \
	default: \
	    break; \
    } \
} while (0)

    static void
vwl_data_device_add_listener(vwl_data_device_T *device, void *data)
{
    VWL_CODE_DATA_OBJECT_ADD_LISTENER(device);
}

    static void
vwl_data_source_add_listener(vwl_data_source_T *source, void *data)
{
    VWL_CODE_DATA_OBJECT_ADD_LISTENER(source);
}

    static void
vwl_data_offer_add_listener(vwl_data_offer_T *offer, void *data)
{
    VWL_CODE_DATA_OBJECT_ADD_LISTENER(offer);
}

/*
 * Sets the selection using the given data device with the given selection. If
 * the device does not support the selection then nothing happens. For data
 * control protocols the serial argument is ignored.
 */
    static void
vwl_data_device_set_selection(
	vwl_data_device_T   *device,
	vwl_data_source_T   *source,
	uint32_t	    serial,
	wayland_selection_T selection)
{
    if (selection == WAYLAND_SELECTION_REGULAR)
    {
	switch (device->protocol)
	{
	    case VWL_DATA_PROTOCOL_WLR:
		zwlr_data_control_device_v1_set_selection(
			device->proxy, source->proxy);
		break;
	    case VWL_DATA_PROTOCOL_EXT:
		ext_data_control_device_v1_set_selection(
			device->proxy, source->proxy);
		break;
	    case VWL_DATA_PROTOCOL_CORE:
		wl_data_device_set_selection(
			device->proxy, source->proxy, serial);
		break;
	    default:
		break;
	}
    }
    else if (selection == WAYLAND_SELECTION_PRIMARY)
    {
	switch (device->protocol)
	{
	    case VWL_DATA_PROTOCOL_WLR:
		zwlr_data_control_device_v1_set_primary_selection(
			device->proxy, source->proxy);
		break;
	    case VWL_DATA_PROTOCOL_EXT:
		ext_data_control_device_v1_set_primary_selection(
			device->proxy, source->proxy);
		break;
	    case VWL_DATA_PROTOCOL_PRIMARY:
		zwp_primary_selection_device_v1_set_selection(
			device->proxy, source->proxy, serial);
		break;
	    default:
		break;
	}
    }
}

/*
 * Start receiving data from offer object, which sends the given fd to the
 * source client to write into.
 */
    static void
vwl_data_offer_receive(vwl_data_offer_T *offer, const char *mime_type, int fd)
{
    switch (offer->protocol)
    {
	case VWL_DATA_PROTOCOL_WLR:
	    zwlr_data_control_offer_v1_receive(offer->proxy, mime_type, fd);
	    break;
	case VWL_DATA_PROTOCOL_EXT:
	    ext_data_control_offer_v1_receive(offer->proxy, mime_type, fd);
	    break;
	case VWL_DATA_PROTOCOL_CORE:
	    wl_data_offer_receive(offer->proxy, mime_type, fd);
	    break;
	case VWL_DATA_PROTOCOL_PRIMARY:
	    zwp_primary_selection_offer_v1_receive(offer->proxy, mime_type, fd);
	    break;
	default:
	    break;
    }
}

#define SET_MANAGER(manager_name, protocol_enum, focus) \
    do { \
	manager->proxy = vwl_gobjects.manager_name; \
	manager->protocol = protocol_enum; \
	return focus; \
    } while (0)

/*
 * Get a data device manager that supports the given selection. If none if found
 * then the manager protocol is set to VWL_DATA_PROTOCOL_NONE. TRUE is returned
 * if the given data device manager requires focus to work else FALSE.
 */
    static int
vwl_get_data_device_manager(
	vwl_data_device_manager_T   *manager,
	wayland_selection_T	    selection)
{
    // Prioritize data control protocols first then try using the focus steal
    // method with the core protocol data objects.
    if (force_fs)
	goto focus_steal;

    // Ext data control protocol supports both selections, try it first
    if (vwl_gobjects.ext_data_control_manager_v1 != NULL)
	SET_MANAGER(ext_data_control_manager_v1, VWL_DATA_PROTOCOL_EXT, FALSE);
    if (vwl_gobjects.zwlr_data_control_manager_v1 != NULL)
    {
	int ver = zwlr_data_control_manager_v1_get_version(
		vwl_gobjects.zwlr_data_control_manager_v1);

	// version 2 or greater supports the primary selection
	if ((selection == WAYLAND_SELECTION_PRIMARY && ver >= 2)
		|| selection == WAYLAND_SELECTION_REGULAR)
	    SET_MANAGER(zwlr_data_control_manager_v1,
		    VWL_DATA_PROTOCOL_WLR, FALSE);
    }

focus_steal:
    if (vwl_focus_stealing_available())
    {
	if (vwl_gobjects.wl_data_device_manager != NULL
		&& selection == WAYLAND_SELECTION_REGULAR)
	    SET_MANAGER(wl_data_device_manager, VWL_DATA_PROTOCOL_CORE, TRUE);

	else if (vwl_gobjects.zwp_primary_selection_device_manager_v1 != NULL
		&& selection == WAYLAND_SELECTION_PRIMARY)
	    SET_MANAGER(zwp_primary_selection_device_manager_v1,
		    VWL_DATA_PROTOCOL_PRIMARY, TRUE);
    }

    manager->protocol = VWL_DATA_PROTOCOL_NONE;

    return FALSE;
}

/*
 * Get a data device that manages the given seat's selection.
 */
    static void
vwl_get_data_device(
	vwl_data_device_manager_T   *manager,
	vwl_seat_T		    *seat,
	vwl_data_device_T	    *device)
{
    switch (manager->protocol)
    {
	case VWL_DATA_PROTOCOL_WLR:
	    device->proxy =
		zwlr_data_control_manager_v1_get_data_device(
			manager->proxy, seat->proxy);
	    break;
	case VWL_DATA_PROTOCOL_EXT:
	    device->proxy =
		ext_data_control_manager_v1_get_data_device(
			manager->proxy, seat->proxy);
	    break;
	case VWL_DATA_PROTOCOL_CORE:
	    device->proxy = wl_data_device_manager_get_data_device(
		    manager->proxy, seat->proxy);
	    break;
	case VWL_DATA_PROTOCOL_PRIMARY:
	    device->proxy = zwp_primary_selection_device_manager_v1_get_device(
		    manager->proxy, seat->proxy);
	    break;
	default:
	    device->protocol = VWL_DATA_PROTOCOL_NONE;
	    return;
    }
    device->protocol = manager->protocol;
}

/*
 * Create a data source
 */
    static void
vwl_create_data_source(
	vwl_data_device_manager_T   *manager,
	vwl_data_source_T	    *source)
{
    switch (manager->protocol)
    {
	case VWL_DATA_PROTOCOL_WLR:
	    source->proxy =
		zwlr_data_control_manager_v1_create_data_source(manager->proxy);
	    break;
	case VWL_DATA_PROTOCOL_EXT:
	    source->proxy =
		ext_data_control_manager_v1_create_data_source(manager->proxy);
	    break;
	case VWL_DATA_PROTOCOL_CORE:
	    source->proxy =
		wl_data_device_manager_create_data_source(manager->proxy);
	    break;
	case VWL_DATA_PROTOCOL_PRIMARY:
	    source->proxy =
		zwp_primary_selection_device_manager_v1_create_source(
			manager->proxy);
	    break;
	default:
	    source->protocol = VWL_DATA_PROTOCOL_NONE;
	    return;
    }
    source->protocol = manager->protocol;
}

/*
 * Offer a new mime type to be advertised by us to other clients.
 */
    static void
vwl_data_source_offer(vwl_data_source_T *source, const char *mime_type)
{
    switch (source->protocol)
    {
	case VWL_DATA_PROTOCOL_WLR:
	    zwlr_data_control_source_v1_offer(source->proxy, mime_type);
	    break;
	case VWL_DATA_PROTOCOL_EXT:
	    ext_data_control_source_v1_offer(source->proxy, mime_type);
	    break;
	case VWL_DATA_PROTOCOL_CORE:
	    wl_data_source_offer(source->proxy, mime_type);
	    break;
	case VWL_DATA_PROTOCOL_PRIMARY:
	    zwp_primary_selection_source_v1_offer(source->proxy, mime_type);
	    break;
	default:
	    break;
    }
}

/*
 * Free the mime types grow arrays in the given clip_sel struct.
 */
    static void
vwl_clipboard_free_mime_types(vwl_clipboard_selection_T *clip_sel)
{
    // Don't want to be double freeing
    if (clip_sel->mime_types.ga_data == clip_sel->tmp_mime_types.ga_data)
    {
	ga_clear_strings(&clip_sel->mime_types);
	ga_init(&vwl_clipboard.primary.tmp_mime_types);
    }
    else
    {
	ga_clear_strings(&clip_sel->mime_types);
	ga_clear_strings(&clip_sel->tmp_mime_types);
    }
}

/*
 * Setup required objects to interact with wayland selections/clipboard on given
 * seat. Returns OK on success and FAIL on failure.
 */
    int
wayland_cb_init(const char *seat)
{
    vwl_clipboard.seat = vwl_get_seat(seat);

    if (vwl_clipboard.seat == NULL)
	return FAIL;

    // Get data device managers for each selection. If there wasn't any manager
    // that could be found that supports the given selection, then it will be
    // unavailable.
    vwl_clipboard.regular.requires_focus = vwl_get_data_device_manager(
	    &vwl_clipboard.regular.manager,
	    WAYLAND_SELECTION_REGULAR);
    vwl_clipboard.primary.requires_focus = vwl_get_data_device_manager(
	    &vwl_clipboard.primary.manager,
	    WAYLAND_SELECTION_PRIMARY);

    // Initialize shm pool and buffer if core data protocol is available
    if (vwl_focus_stealing_available() &&
	    (vwl_clipboard.regular.requires_focus ||
	     vwl_clipboard.primary.requires_focus))
	vwl_clipboard.fs_buffer = vwl_init_buffer_store(1, 1);

    // Get data devices for each selection. If one of the above function calls
    // results in an unavailable manager, then the device coming from it will
    // have its protocol set to VWL_DATA_PROTOCOL_NONE.
    vwl_get_data_device(
	    &vwl_clipboard.regular.manager,
	    vwl_clipboard.seat,
	    &vwl_clipboard.regular.device);
    vwl_get_data_device(
	    &vwl_clipboard.primary.manager,
	    vwl_clipboard.seat,
	    &vwl_clipboard.primary.device);

    // Initialize grow arrays for the offer mime types.
    // I find most applications to have below 10 mime types that they offer.
    ga_init2(&vwl_clipboard.regular.tmp_mime_types, sizeof(char*), 10);
    ga_init2(&vwl_clipboard.primary.tmp_mime_types, sizeof(char*), 10);

    // We dont need to use ga_init2 because tmp_mime_types will be copied over
    // to mime_types anyways.
    ga_init(&vwl_clipboard.regular.mime_types);
    ga_init(&vwl_clipboard.primary.mime_types);

    // Start listening for data offers/new selections. Don't do anything when we
    // get a new data offer other than saving the mime types and saving the data
    // offer. Then when we want the data we use the saved data offer to receive
    // data from it along with the saved mime_types. For each new selection just
    // destroy the previous offer/free mime_types, if any.
    vwl_data_device_add_listener(
	    &vwl_clipboard.regular.device,
	    &vwl_clipboard.regular);
    vwl_data_device_add_listener(
	    &vwl_clipboard.primary.device,
	    &vwl_clipboard.primary);

    if (vwl_display_roundtrip(&vwl_display) == FAIL)
    {
	wayland_cb_uninit();
	return FAIL;
    }
    clip_init(TRUE);

    return OK;
}

/*
 * Free up resources used for Wayland selections. Does not destroy global
 * objects such as data device managers.
 */
    void
wayland_cb_uninit(void)
{
    if (vwl_clipboard.fs_buffer != NULL)
    {
	vwl_destroy_buffer_store(vwl_clipboard.fs_buffer);
	vwl_clipboard.fs_buffer = NULL;
    }

    // Destroy the current offer if it exists
    vwl_data_offer_destroy(vwl_clipboard.regular.offer, TRUE);
    vwl_data_offer_destroy(vwl_clipboard.primary.offer, TRUE);

    // Destroy any devices or sources
    vwl_data_device_destroy(&vwl_clipboard.regular.device, FALSE);
    vwl_data_device_destroy(&vwl_clipboard.primary.device, FALSE);
    vwl_data_source_destroy(&vwl_clipboard.regular.source, FALSE);
    vwl_data_source_destroy(&vwl_clipboard.primary.source, FALSE);

    // Free mime types
    vwl_clipboard_free_mime_types(&vwl_clipboard.regular);
    vwl_clipboard_free_mime_types(&vwl_clipboard.primary);

    vwl_display_flush(&vwl_display);

    vim_memset(&vwl_clipboard, 0, sizeof(vwl_clipboard));
    vwl_clipboard.regular.selection = WAYLAND_SELECTION_REGULAR;
    vwl_clipboard.primary.selection = WAYLAND_SELECTION_PRIMARY;
}

/*
 * If the given selection can be used.
 */
    static int
vwl_clipboard_selection_is_ready(vwl_clipboard_selection_T *clip_sel)
{
    return clip_sel->manager.protocol != VWL_DATA_PROTOCOL_NONE &&
	clip_sel->device.protocol != VWL_DATA_PROTOCOL_NONE;
}

/*
 * Callback for data offer event. Start listening to the given offer immediately
 * in order to get mime types.
 */
    static void
vwl_data_device_listener_data_offer(
	vwl_data_device_T   *device,
	vwl_data_offer_T    *offer)
{
    vwl_clipboard_selection_T *clip_sel = device->data;

    // Get mime types and save them so we can use them when we want to paste the
    // selection.
    if (clip_sel->source.proxy != NULL)
	// We own the selection, no point in getting mime types
	return;

    vwl_data_offer_add_listener(offer, device->data);
}

/*
 * Callback for offer event. Save each mime type given to be used later.
 */
    static void
vwl_data_offer_listener_offer(vwl_data_offer_T *offer, const char *mime_type)
{
    vwl_clipboard_selection_T *clip_sel = offer->data;

    // Save string into temporary grow array, which will be finalized into the
    // actual grow array if the selection matches with the selection that the
    // device manages.
    ga_copy_string(&clip_sel->tmp_mime_types, (char_u*)mime_type);
}

/*
 * Callback for selection event, for either the regular or primary selection.
 * Don't try receiving data from the offer, instead destroy the previous offer
 * if any and set the current offer to the given offer, along with the
 * respective mime types.
 */
    static void
vwl_data_device_listener_selection(
	vwl_data_device_T   *device UNUSED,
	vwl_data_offer_T    *offer,
	wayland_selection_T selection)
{
    vwl_clipboard_selection_T	*clip_sel = device->data;
    vwl_data_offer_T		*prev_offer = clip_sel->offer;

    // Save offer if it selection and clip_sel match, else discard it
    if (clip_sel->selection == selection)
	clip_sel->offer = offer;
    else
    {
	// Example: selection event is for the primary selection but this device
	// is only for the regular selection, if so then just discard the offer
	// and tmp_mime_types.
	vwl_data_offer_destroy(offer, TRUE);
	tmp_vwl_offer = NULL;
	ga_clear_strings(&clip_sel->tmp_mime_types);
	return;
    }

    // There are two cases when clip_sel->offer is NULL
    // 1. No one owns the selection
    // 2. We own the selection (we'll just access the register directly)
    if (offer == NULL)
    {
	// Selection cleared/empty
	ga_clear_strings(&clip_sel->tmp_mime_types);
	clip_sel->offer = NULL;
	goto exit;
    }
    else if (clip_sel->source.proxy != NULL)
    {
	// We own the selection, ignore it
	vwl_data_offer_destroy(offer, TRUE);
	ga_clear_strings(&clip_sel->tmp_mime_types);
	clip_sel->offer = NULL;
	goto exit;
    }

exit:
    // Destroy previous offer if any
    vwl_data_offer_destroy(prev_offer, TRUE);
    ga_clear_strings(&clip_sel->mime_types);

    // Copy the grow array over
    clip_sel->mime_types = clip_sel->tmp_mime_types;

    // Clear tmp_mime_types so next data_offer doesn't try to resize/grow it
    // (Don't free it though using ga_clear() because mime_types->ga_data is the
    // same pointer)r
    if (clip_sel->offer != NULL)
	ga_init(&clip_sel->tmp_mime_types);
}

/*
 * Callback for finished event. Destroy device and all related objects/resources
 * such as offers and mime types.
 */
    static void
vwl_data_device_listener_finished(vwl_data_device_T *device)
{
    vwl_clipboard_selection_T *clip_sel = device->data;

    vwl_data_device_destroy(device, FALSE);
    vwl_data_offer_destroy(clip_sel->offer, TRUE);
    vwl_data_source_destroy(&clip_sel->source, FALSE);
    vwl_clipboard_free_mime_types(clip_sel);
}

/*
 * Return a pointer to a grow array of mime types that the current offer
 * supports sending. If the returned garray has NULL for ga_data or a ga_len of
 * 0, then the selection is cleared. If focus stealing is required, a surface
 * will be created to steal focus first.
 */
    garray_T *
wayland_cb_get_mime_types(wayland_selection_T selection)
{
    vwl_clipboard_selection_T *clip_sel;

    if (selection == WAYLAND_SELECTION_REGULAR)
	clip_sel = &vwl_clipboard.regular;
    else if (selection == WAYLAND_SELECTION_PRIMARY)
	clip_sel = &vwl_clipboard.primary;
    else
	return NULL;

    if (clip_sel->requires_focus)
    {
	// We don't care about the on_focus callback since once we gain focus
	// the data offer events will come immediately.
	if (vwl_init_fs_surface(vwl_clipboard.seat,
		    vwl_clipboard.fs_buffer, NULL, NULL) == FAIL)
	    return NULL;
    }
    else if (vwl_display_roundtrip(&vwl_display) == FAIL)
	return NULL;

    return &clip_sel->mime_types;
}

/*
 * Receive data from the given selection, and return the fd to read data from.
 * On failure -1 is returned.
 */
    int
wayland_cb_receive_data(const char *mime_type, wayland_selection_T selection)
{
    vwl_clipboard_selection_T *clip_sel;

    // Create pipe that source client will write to
    int fds[2];

    if (selection == WAYLAND_SELECTION_REGULAR)
	clip_sel = &vwl_clipboard.regular;
    else if (selection == WAYLAND_SELECTION_PRIMARY)
	clip_sel = &vwl_clipboard.primary;
    else
	return -1;

    if (!wayland_client_is_connected(FALSE) ||
	    !vwl_clipboard_selection_is_ready(clip_sel))
	return -1;

    if (clip_sel->offer == NULL || clip_sel->offer->proxy == NULL)
	return -1;

    if (pipe(fds) == -1)
	return -1;

    vwl_data_offer_receive(clip_sel->offer, mime_type, fds[1]);

    close(fds[1]); // Close before we read data so that when the source client
		   // closes their end we receive an EOF.

    if (vwl_display_flush(&vwl_display) == OK)
	return fds[0];

    close(fds[0]);

    return -1;
}

/*
 * Callback for send event. Just call the user callback which will handle it
 * and do the writing stuff.
 */
    static void
vwl_data_source_listener_send(
	vwl_data_source_T   *source,
	const char	    *mime_type,
	int32_t		    fd)
{
    vwl_clipboard_selection_T *clip_sel = source->data;

    if (clip_sel->send_cb != NULL)
	clip_sel->send_cb(mime_type, fd, clip_sel->selection);
    close(fd);
}

/*
 * Callback for cancelled event, just call the user callback.
 */
    static void
vwl_data_source_listener_cancelled(vwl_data_source_T *source)
{
    vwl_clipboard_selection_T *clip_sel = source->data;

    if (clip_sel->send_cb != NULL)
	clip_sel->cancelled_cb(clip_sel->selection);
    vwl_data_source_destroy(source, FALSE);
}

/*
 * Set the selection when we gain focus
 */
    static void
vwl_on_focus_set_selection(void *data, uint32_t serial)
{
    vwl_clipboard_selection_T *clip_sel = data;

    vwl_data_device_set_selection(
	    &clip_sel->device,
	    &clip_sel->source,
	    serial,
	    clip_sel->selection);
    vwl_display_roundtrip(&vwl_display);
}

/*
 * Become the given selection's owner, and advertise to other clients the mime
 * types found in mime_types array. Returns FAIL on failure and OK on success.
 */
    int
wayland_cb_own_selection(
	wayland_cb_send_data_func_T		send_cb,
	wayland_cb_selection_cancelled_func_T	cancelled_cb,
	const char				**mime_types,
	int					len,
	wayland_selection_T			selection)
{
    vwl_clipboard_selection_T *clip_sel;

    if (selection == WAYLAND_SELECTION_REGULAR)
	clip_sel = &vwl_clipboard.regular;
    else if (selection == WAYLAND_SELECTION_PRIMARY)
	clip_sel = &vwl_clipboard.primary;
    else
	return FAIL;

    if (clip_sel->source.proxy != NULL)
	// We already own the selection
	return OK;

    if (!wayland_client_is_connected(FALSE) ||
	    !vwl_clipboard_selection_is_ready(clip_sel))
	return FAIL;

    clip_sel->send_cb = send_cb;
    clip_sel->cancelled_cb = cancelled_cb;

    vwl_create_data_source(&clip_sel->manager, &clip_sel->source);

    vwl_data_source_add_listener(&clip_sel->source, clip_sel);

    // Advertise mime types
    for (int i = 0; i < len; i++)
	vwl_data_source_offer(&clip_sel->source, mime_types[i]);

    if (clip_sel->requires_focus)
    {
	// Call set_selection later when we gain focus
	if (vwl_init_fs_surface(vwl_clipboard.seat, vwl_clipboard.fs_buffer,
		    vwl_on_focus_set_selection, clip_sel) == FAIL)
	    goto fail;
    }
    else
    {
	vwl_data_device_set_selection(&clip_sel->device,
		&clip_sel->source, 0, selection);
	if (vwl_display_roundtrip(&vwl_display) == FAIL)
	    goto fail;
    }

    return OK;
fail:
    vwl_data_source_destroy(&clip_sel->source, FALSE);
    return FAIL;
}

/*
 * Disown the given selection, so that we are not the source client that other
 * clients receive data from.
 */
    void
wayland_cb_lose_selection(wayland_selection_T selection)
{
    if (selection == WAYLAND_SELECTION_REGULAR)
	vwl_data_source_destroy(&vwl_clipboard.regular.source, FALSE);
    else if (selection == WAYLAND_SELECTION_PRIMARY)
	vwl_data_source_destroy(&vwl_clipboard.primary.source, FALSE);
    vwl_display_flush(&vwl_display);
}

/*
 * Return TRUE if the selection is owned by either us or another client.
 */
    int
wayland_cb_selection_is_owned(wayland_selection_T selection)
{
    vwl_display_roundtrip(&vwl_display);

    if (selection == WAYLAND_SELECTION_REGULAR)
	return vwl_clipboard.regular.source.proxy != NULL
	    || vwl_clipboard.regular.offer != NULL;
    else if (selection == WAYLAND_SELECTION_PRIMARY)
	return vwl_clipboard.primary.source.proxy != NULL
	    || vwl_clipboard.primary.offer != NULL;
    else
	return FALSE;
}

/*
 * Return TRUE if the wayland clipboard/selections are ready to use.
 */
    int
wayland_cb_is_ready(void)
{
    vwl_display_roundtrip(&vwl_display);

    // Clipboard is ready if we have at least one selection available
    return wayland_client_is_connected(TRUE) &&
	    (vwl_clipboard_selection_is_ready(&vwl_clipboard.regular) ||
	    vwl_clipboard_selection_is_ready(&vwl_clipboard.primary));
}

/*
 * Reload wayland clipboard, useful if changing seat.
 */
    int
wayland_cb_reload(void)
{
    // Lose any selections we own
    if (clipmethod == CLIPMETHOD_WAYLAND)
    {
	if (clip_star.owned)
	    clip_lose_selection(&clip_star);
	if (clip_plus.owned)
	    clip_lose_selection(&clip_plus);
    }

    wayland_cb_uninit();

    if (wayland_cb_init((char*)p_wse) == FAIL)
	return FAIL;

    choose_clipmethod();
    return OK;
}

#endif // FEAT_WAYLAND_CLIPBOARD

static int wayland_ct_restore_count = 0;

/*
 * Attempts to restore the Wayland display connection. Returns OK if display
 * connection was/is now valid, else FAIL if the display connection is invalid.
 */
    int
wayland_may_restore_connection(void)
{
    // No point if we still are already connected properly
    if (wayland_client_is_connected(TRUE))
	return OK;

    // No point in restoring the connection if we are exiting or dying.
    if (exiting || v_dying || wayland_ct_restore_count <= 0)
    {
	wayland_set_display("");
	return FAIL;
    }

    --wayland_ct_restore_count;
    wayland_uninit_client();

    return wayland_init_client(wayland_display_name);
}

/*
 * Disconnect then reconnect wayland connection, and update clipmethod.
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
    // changing to a new wayland display.
    if (!eap->forceit && wayland_client_is_connected(TRUE) &&
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

    wayland_uninit_client();

    // Reset amount of available tries to reconnect the display to 5
    wayland_ct_restore_count = 5;

    if (wayland_init_client(display) == OK)
    {
	smsg(_("restoring wayland display %s"), wayland_display_name);

#ifdef FEAT_WAYLAND_CLIPBOARD
	wayland_cb_init((char*)p_wse);
#endif
    }
    else
	msg(_("failed restoring, lost connection to wayland display"));

    vim_free(display);

    choose_clipmethod();
}

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

#endif // FEAT_WAYLAND
