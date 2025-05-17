/* vi:set ts=8 sts=4 sw=4 noet:
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
# include "wayland/wlr-data-control-unstable-v1.h"
# include "wayland/ext-data-control-unstable-v1.h"
#endif
# include "wayland/xdg-shell.h"

// Struct that represents a seat. (Should be accessed via
// vwl_get_seat()).
typedef struct {
    struct wl_seat *proxy;

    uint32_t name;	    // Numerical name
    char *label;	    // Name of seat as text (e.g. seat0, seat1...).
    uint32_t capabilities;  // Bitmask of the capabilites of the seat
			    // (pointer, keyboard, touch).
} vwl_seat_T;

// Global objects
typedef struct {
#ifdef FEAT_WAYLAND_CLIPBOARD
    // Data control protocols
    struct zwlr_data_control_manager_v1 *zwlr_data_control_manager_v1;
    uint32_t zwlr_data_control_manager_v1_name;

    struct ext_data_control_manager_v1 *ext_data_control_manager_v1;
    uint32_t ext_data_control_manager_v1_name;
#endif
} vwl_global_objects_T;

// Struct wrapper for wayland display and registry
typedef struct {
    struct wl_display *proxy;
    int fd;			// File descriptor for display

    struct {
	struct wl_registry *proxy;
    } registry;
} vwl_display_T;

#ifdef FEAT_WAYLAND_CLIPBOARD

// Wayland protocols for accessing the selection
typedef enum {
    VWL_DATA_PROTOCOL_NONE,
    VWL_DATA_PROTOCOL_EXT,
    VWL_DATA_PROTOCOL_WLR,
    VWL_DATA_PROTOCOL_CORE,	    // To be implemented
    VWL_DATA_PROTOCOL_PRIMARY,	    // To be implemented
    VWL_DATA_PROTOCOL_GTK_PRIMARY   // To be implemented, wl-clipboard supports
				    // this so we should as well.
} vwl_data_protocol_T;

// DATA RELATED OBJECT WRAPPERS
// These wrap around a proxy and act as a generic container.
// The `data` member is used to pass other needed stuff around such as a
// vwl_clipboard_selection_T pointer.

typedef struct {
    void *proxy;
    void *data; // Is not set when a new offer is created on a data_offer event.
		// Only set when listening to a data offer.
    vwl_data_protocol_T protocol;
} vwl_data_offer_T;

typedef struct {
    void *proxy;
    void *data;
    vwl_data_protocol_T protocol;
} vwl_data_source_T;

typedef struct {
    void *proxy;
    void *data;
    vwl_data_protocol_T protocol;
} vwl_data_device_T;

typedef struct {
    void *proxy;
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
    // Do not destroy here
    vwl_data_device_manager_T manager;

    vwl_data_device_T device;
    vwl_data_source_T source;
    vwl_data_offer_T *offer;	// Current offer for the selection

    garray_T mime_types;	// Mime types supported by the current offer

    garray_T tmp_mime_types;	// Temporary array for mime types when we are
				// receiving them. When the selection event
				// arrives and it is the one we want, then copy it
				// over to mime_types

    // To be populated by callbacks from outside this file
    wayland_cb_send_data_func_T send_cb;
    wayland_cb_selection_cancelled_func_T cancelled_cb;
} vwl_clipboard_selection_T;

// Holds stuff related to the clipboard/selections
typedef struct {
    // Do not destroy here, will be destroyed when vwl_disconnect_display() is
    // called.
    vwl_seat_T *seat;

    vwl_clipboard_selection_T regular;
    vwl_clipboard_selection_T primary;
} vwl_clipboard_T;

#endif // FEAT_WAYLAND_CLIPBOARD

static int vwl_display_flush(vwl_display_T *display);
static void vwl_callback_done(void *data, struct wl_callback *callback,
	uint32_t cb_data);
static int vwl_display_roundtrip(vwl_display_T *display);
static int vwl_display_dispatch(vwl_display_T *display, int *num_dispatched);

static void vwl_log_handler(const char *fmt, va_list args);
static int vwl_connect_display(const char *display);
static void vwl_disconnect_display(void);
static int vwl_listen_to_registry(void);

static void vwl_registry_listener_global(void *data, struct wl_registry *registry,
	uint32_t name, const char *interface, uint32_t version);
static void vwl_registry_listener_global_remove(void *data,
	struct wl_registry *registry,  uint32_t name);

static void vwl_add_seat(struct wl_seat *seat, uint32_t name);
static void vwl_seat_listener_name(void *data, struct wl_seat *seat,
	const char *name);
static void vwl_seat_listener_capabilities(void *data, struct wl_seat *seat,
	uint32_t capabilities);
static void vwl_destroy_seat(vwl_seat_T *seat);
static vwl_seat_T * vwl_get_seat(const char *label);

#ifdef FEAT_WAYLAND_CLIPBOARD

static void vwl_gen_data_device_listener_data_offer(void *data,
	void *offer_proxy);
static void vwl_gen_data_device_listener_selection(void *data,
	void *offer_proxy, wayland_selection_T selection,
	vwl_data_protocol_T protocol);

static void vwl_data_device_destroy(vwl_data_device_T *device, int alloced);
static void vwl_data_offer_destroy(vwl_data_offer_T *offer, int alloced);
static void vwl_data_source_destroy(vwl_data_source_T *source, int alloced);

static void vwl_data_device_add_listener(vwl_data_device_T *device, void *data);
static void vwl_data_source_add_listener(vwl_data_source_T *source, void *data);
static void vwl_data_offer_add_listener(vwl_data_offer_T *offer, void *data);

static void vwl_data_device_set_selection(vwl_data_device_T *device,
	vwl_data_source_T *source, wayland_selection_T selection);
static void vwl_data_offer_receive(vwl_data_offer_T *offer,
	const char *mime_type, int fd);
static void vwl_get_data_device_manager(vwl_data_device_manager_T *manager,
	wayland_selection_T selection);
static void vwl_get_data_device(vwl_data_device_manager_T *manager,
	vwl_seat_T *seat, vwl_data_device_T *device);
static void vwl_create_data_source(vwl_data_device_manager_T *manager,
	vwl_data_source_T *source);
static void vwl_data_source_offer(vwl_data_source_T *source,
	const char *mime_type);

static void vwl_clipboard_free_mime_types(vwl_clipboard_selection_T *clip_sel);
static int vwl_clipboard_selection_is_ready(vwl_clipboard_selection_T *clip_sel);

static void vwl_data_device_listener_data_offer(
	vwl_data_device_T *device, vwl_data_offer_T *offer);
static void vwl_data_offer_listener_offer(vwl_data_offer_T *offer,
	const char *mime_type);
static void vwl_data_device_listener_selection( vwl_data_device_T *device,
	vwl_data_offer_T *offer, wayland_selection_T selection);
static void vwl_data_device_listener_finished(vwl_data_device_T *device);

static void vwl_data_source_listener_send( vwl_data_source_T *source,
	const char *mime_type, int fd);
static void vwl_data_source_listener_cancelled(vwl_data_source_T *source);

static void wayland_set_display(const char *display);

vwl_data_device_listener_T vwl_data_device_listener = {
    .data_offer = vwl_data_device_listener_data_offer,
    .selection = vwl_data_device_listener_selection,
    .finished = vwl_data_device_listener_finished
};

vwl_data_source_listener_T vwl_data_source_listener = {
    .send = vwl_data_source_listener_send,
    .cancelled = vwl_data_source_listener_cancelled
};

vwl_data_offer_listener_T vwl_data_offer_listener = {
    .offer = vwl_data_offer_listener_offer
};

# endif // FEAT_WAYLAND_CLIPBOARD

struct wl_callback_listener vwl_callback_listener = {
    .done = vwl_callback_done
};

struct wl_registry_listener vwl_registry_listener = {
    .global = vwl_registry_listener_global,
    .global_remove = vwl_registry_listener_global_remove
};

struct wl_seat_listener vwl_seat_listener = {
    .name = vwl_seat_listener_name,
    .capabilities = vwl_seat_listener_capabilities
};

vwl_display_T vwl_display;
vwl_global_objects_T vwl_gobjects;
garray_T vwl_seats;

#ifdef FEAT_WAYLAND_CLIPBOARD
vwl_clipboard_T vwl_clipboard;
#endif

/*
 * Like wl_display_flush but polls the display fd with a timeout. Returns FAIL
 * on failure and OK on success.
 */
    static int
vwl_display_flush(vwl_display_T *display)
{
    int ret;
#ifndef HAVE_SELECT
    struct pollfd fds = {
	.fd = display->fd,
	.events = POLLOUT
    };
#else
    fd_set wfds;
    struct timeval tv;

    FD_ZERO(&wfds);
    FD_SET(display->fd, &wfds);

    tv.tv_sec = 0;
    tv.tv_usec = p_wtm * 1000;
#endif

    if (display->proxy == NULL)
	return FAIL;

    // Send the requests we have made to the compositor, until we have written
    // nall the data. Poll in order to check if the display fd is writable, if
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
    struct wl_callback *callback;
    int ret = OK, num = 0, done = FALSE;
    struct timeval start, now;

    if (display->proxy == NULL)
	return FAIL;

    // Tell compositor to emit 'done' event after processing all requests we
    // have sent and handling events.
    callback = wl_display_sync(display->proxy);

    if (callback == NULL)
	return FAIL;

    wl_callback_add_listener(callback, &vwl_callback_listener, &done);

    // If the compositor keeps sending us events but never sends us the 'done'
    // event, then we will just loop indefinitely. This practically has a zero
    // chance of happening, but lets handle it by checking the elapsed time.
    gettimeofday(&start, NULL);

    // Wait till we get the done event (which will set 'done' to TRUE)
    while (ret == OK && !done && num >= 0)
    {
	ret = vwl_display_dispatch(display, &num);

	if (ret == OK)
	{
	    gettimeofday(&now, NULL);

	    if (now.tv_usec - start.tv_usec >= p_wtm * 1000)
		ret = FAIL;
	}
    }
    if (ret == FAIL)
    {
	if (!done)
	    wl_callback_destroy(callback);
	return FAIL;
    }

    return OK;
}

/*
 * Like wl_display_roundtrip but polls the display fd with a timeout. Returns
 * FAIL on failure and OK on success. If num_dispatched is not NULL then the
 * variable it points to will be set to the amount of events dispatched, unless
 * FAIL is returned.
 */
    static int
vwl_display_dispatch(vwl_display_T *display, int *num_dispatched)
{
    int num, ret = 0;
#ifndef HAVE_SELECT
    struct pollfd fds = {
	.fd = display->fd,
	.events = POLLOUT
    };
#else
    fd_set wfds;
    struct timeval tv;

    FD_ZERO(&wfds);
    FD_SET(display->fd, &wfds);

    tv.tv_sec = 0;
    tv.tv_usec = p_wtm * 1000;
#endif

    if (display->proxy == NULL)
	return FAIL;

    while (wl_display_prepare_read(display->proxy) == -1)
	// Dispatch any queued events so that we can start reading
	if (wl_display_dispatch_pending(display->proxy) == -1)
	    return FAIL;

    // Send any requests before we starting blocking to read display fd
    if (vwl_display_flush(display) == FAIL)
    {
	wl_display_cancel_read(display->proxy);
	return FAIL;
    }

    // Poll until there is data to read from the display fd.
#ifndef HAVE_SELECT
    if (poll(&fds, 1, p_wtm) <= 0)
#else
	if (select(display->fd + 1, NULL, &wfds, NULL, &tv) <= 0)
#endif
	{
	    wl_display_cancel_read(display->proxy);
	    return FAIL;
	}

    // Read events into the queue
    ret = wl_display_read_events(display->proxy);

    // Always dispatch events

    // Dispatch those events (call the handlers associated for each event)
    if ((num = wl_display_dispatch_pending(display->proxy)) == -1)
	return FAIL;

    if (ret == -1)
	return FAIL;

    if (num_dispatched != NULL)
	*num_dispatched = num;

    return OK;
}

/*
 * Redirect libwayland logging to use ch_log + emsg instead.
 */
    static void
vwl_log_handler(const char *fmt, va_list args)
{
    // 512 bytes should be big enough
    char *buf = alloc(512);
    char *prefix = _("wayland protocol error -> ");
    size_t len = STRLEN(prefix);

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
 * Connect to the display with name; passing NULL will use $WAYLAND_DISPLAY.
 * Additionally get the registry object but will not starting listening. Returns
 * OK on sucess and FAIL on failure.
 */
    static int
vwl_connect_display(const char *display)
{
    if (wayland_no_connect)
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

/*
 * Disconnects the display and frees up all resources, including all global
 * objects.
 */
    static void
vwl_disconnect_display(void)
{
#define destroy_gobject(object) \
    if (vwl_gobjects.object != NULL) \
    { \
	object##_destroy(vwl_gobjects.object); \
	vwl_gobjects.object = NULL; \
    }

    destroy_gobject(ext_data_control_manager_v1)
    destroy_gobject(zwlr_data_control_manager_v1)

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
 * Start listening to the registry and get initial set of global
 * objects/interfaces.
 */
    static int
vwl_listen_to_registry(void)
{
    ga_init2(&vwl_seats, sizeof(vwl_seat_T), 1);

    wl_registry_add_listener(
	    vwl_display.registry.proxy,
	    &vwl_registry_listener,
	    NULL);

    if (vwl_display_roundtrip(&vwl_display) == FAIL)
	return FAIL;

    // TODO: CHECK ANY FAILS
    return OK;
}

/*
 * Callback for global event, for each global interface the compositor supports.
 * Keep in sync with global remove callback and vwl_disconnect_display().
 */
    static void
vwl_registry_listener_global(
	void *data UNUSED,
	struct wl_registry *registry UNUSED,
	uint32_t name,
	const char *interface,
	uint32_t version)
{
#define set_gobject(object, min_ver) \
    { \
	chosen_interface = &object##_interface; \
	object_member = (void*)&vwl_gobjects.object; \
	name_member = (void*)&vwl_gobjects.object##_name; \
	min_version = min_ver; \
    }

    const struct wl_interface *chosen_interface = NULL;
    void *proxy;
    uint32_t min_version;
    void **object_member;
    uint32_t *name_member;

    if (STRCMP(interface, wl_seat_interface.name) == 0)
    {
	chosen_interface =	&wl_seat_interface;
	min_version =		2;
    }
    else if (STRCMP(interface, zwlr_data_control_manager_v1_interface.name) == 0)
	set_gobject(zwlr_data_control_manager_v1, 1)
    else if (STRCMP(interface, ext_data_control_manager_v1_interface.name) == 0)
	set_gobject(ext_data_control_manager_v1, 1)

    if (chosen_interface == NULL || version < min_version)
	return;

    proxy = wl_registry_bind(vwl_display.registry.proxy, name, chosen_interface,
	    version);

    if (chosen_interface == &wl_seat_interface)
	// Add seat to vwl_seats array, as we can have multiple seats.
	vwl_add_seat(proxy, name);
    else
    {
	// Hold proxy & name in the vwl_gobject struct
	*object_member = proxy;
	*name_member = name;
    }
}

/*
 * Called when a global object is removed, if so, then destroy it on our side.
 */
    static void
vwl_registry_listener_global_remove(
	void *data UNUSED,
	struct wl_registry *registry UNUSED,
	uint32_t name)
{
#define rm_gobject(object) \
    if (vwl_gobjects.object##_name == name) \
    { \
	object##_destroy(vwl_gobjects.object); \
	vwl_gobjects.object = NULL; \
	return; \
    }  \

    rm_gobject(zwlr_data_control_manager_v1)
    rm_gobject(ext_data_control_manager_v1)

    // Find seat with matching numerical name and destroy it.
    for (int i = 0; i < vwl_seats.ga_len; i++)
    {
	vwl_seat_T *seat = &((vwl_seat_T *)vwl_seats.ga_data)[i];

	if (seat->name == name)
	{
	    vwl_destroy_seat(seat);

	    // Move all seats after it forward to fill gap
	    for (int k = i + 1; i < vwl_seats.ga_len; k++)
	    {
		vwl_seat_T *cur = &((vwl_seat_T *)vwl_seats.ga_data)[k];
		vwl_seat_T *new = &((vwl_seat_T *)vwl_seats.ga_data)[k - 1];

		*new = *cur;
	    }
	    vwl_seats.ga_len--;
	    return;
	}
    }
}

/*
 * Add a new seat given its proxy to the global grow array
 */
    static void
vwl_add_seat(struct wl_seat *seat_proxy, uint32_t name)
{
    vwl_seat_T *seat;

    if (ga_grow(&vwl_seats, 1) == FAIL)
	return;

    seat = &((vwl_seat_T *)vwl_seats.ga_data)[vwl_seats.ga_len];

    seat->proxy = seat_proxy;
    seat->name = name;

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
vwl_seat_listener_name(void *data, struct wl_seat *seat_proxy UNUSED,
	const char *name)
{
    vwl_seat_T *seat = data;

    seat->label = (char *)vim_strsave((char_u *)name);
}

/*
 * Callback for seat capabilities
 */
    static void
vwl_seat_listener_capabilities(void *data, struct wl_seat *seat_proxy UNUSED,
	uint32_t capabilities)
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
	wl_seat_destroy(seat->proxy);
	seat->proxy = NULL;
    }
    vim_free(seat->label);
}

/*
 * Return a seat with the give name/label. If none exists then NULL is returned.
 */
    static vwl_seat_T *
vwl_get_seat(const char *label)
{
    for (int i = 0; i < vwl_seats.ga_len; i++)
    {
	vwl_seat_T *seat = &((vwl_seat_T *)vwl_seats.ga_data)[i];
	if (STRCMP(seat->label, label) == 0)
	    return seat;
    }
    return NULL;
}

/*
 * Connects to the wayland display with given name and binds to global objects
 * as needed. If display is NULL then the $WAYLAND_DISPLAY environment variable
 * will be used (handled by libwayland).
 */
    int
wayland_init_client(const char *display)
{
    if (vwl_connect_display(display) == FAIL)
	return FAIL;
    if (vwl_listen_to_registry() == FAIL)
	return FAIL;

    wayland_display_fd = vwl_display.fd;

    return OK;
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
 * Flush requests and process new Wayland events.
 */
    int
wayland_client_update(void)
{
    return vwl_display_dispatch(&vwl_display, NULL);
}

#ifdef FEAT_WAYLAND_CLIPBOARD

#define VWL_CODE_DATA_OBJECT_DESTROY(type) \
{ \
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
	default: \
	    break; \
    } \
    if (alloced) \
	vim_free(type); \
    else \
	type->proxy = NULL; \
}

    static void
vwl_data_device_destroy(vwl_data_device_T *device, int alloced)
{
    VWL_CODE_DATA_OBJECT_DESTROY(device)
}

    static void
vwl_data_offer_destroy(vwl_data_offer_T *offer, int alloced)
{
    VWL_CODE_DATA_OBJECT_DESTROY(offer)
}

    static void
vwl_data_source_destroy(vwl_data_source_T *source, int alloced)
{
    VWL_CODE_DATA_OBJECT_DESTROY(source)
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
    { \
	tmp_vwl_offer->proxy = offer_proxy; \
	tmp_vwl_offer->protocol = device->protocol; \

	vwl_data_device_listener.data_offer(device, tmp_vwl_offer);
    } \
}

    static void
vwl_gen_data_device_listener_selection(
	void *data,
	void *offer_proxy,
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
    static void v##device_name##_listener_data_offer( \
	    void *data, struct device_name *device_proxy UNUSED, \
	    struct offer_name *offer_proxy) \
{ \
    vwl_gen_data_device_listener_data_offer(data, offer_proxy); \
}
#define VWL_FUNC_DATA_DEVICE_SELECTION( \
	device_name, offer_name, type, selection_type, protocol) \
	static void v##device_name##_listener_##type( \
		void *data, struct device_name *device_proxy UNUSED, \
		struct offer_name *offer_proxy UNUSED) \
{ \
    vwl_gen_data_device_listener_selection( \
	    data, offer_proxy, selection_type, protocol); \
}
#define VWL_FUNC_DATA_DEVICE_FINISHED(device_name) \
    static void v##device_name##_listener_finished( \
	    void *data, struct device_name *device_proxy UNUSED) \
{ \
    vwl_data_device_listener.finished(data); \
}
#define VWL_FUNC_DATA_SOURCE_SEND(source_name) \
    static void v##source_name##_listener_send(void *data, \
	    struct source_name *source_proxy UNUSED, \
	    const char *mime_type, int fd) \
{ \
    vwl_data_source_listener.send(data, mime_type, fd); \
}
#define VWL_FUNC_DATA_SOURCE_CANCELLED(source_name) \
    static void v##source_name##_listener_cancelled(void *data, \
	    struct source_name *source_proxy UNUSED) \
{ \
    vwl_data_source_listener.cancelled(data); \
}
#define VWL_FUNC_DATA_OFFER_OFFER(offer_name) \
    static void v##offer_name##_listener_offer(void *data, \
	    struct offer_name *offer_proxy UNUSED, \
	    const char *mime_type) \
{ \
    vwl_data_offer_listener.offer(data, mime_type); \
}

VWL_FUNC_DATA_DEVICE_DATA_OFFER(
	ext_data_control_device_v1, ext_data_control_offer_v1)
VWL_FUNC_DATA_DEVICE_DATA_OFFER(
	zwlr_data_control_device_v1, zwlr_data_control_offer_v1)

VWL_FUNC_DATA_DEVICE_SELECTION(
	ext_data_control_device_v1, ext_data_control_offer_v1,
	selection, WAYLAND_SELECTION_REGULAR, VWL_DATA_PROTOCOL_EXT)
VWL_FUNC_DATA_DEVICE_SELECTION(
	zwlr_data_control_device_v1, zwlr_data_control_offer_v1,
	selection, WAYLAND_SELECTION_REGULAR, VWL_DATA_PROTOCOL_WLR)

VWL_FUNC_DATA_DEVICE_SELECTION(
	ext_data_control_device_v1, ext_data_control_offer_v1,
	primary_selection, WAYLAND_SELECTION_PRIMARY, VWL_DATA_PROTOCOL_EXT)
VWL_FUNC_DATA_DEVICE_SELECTION(
	zwlr_data_control_device_v1, zwlr_data_control_offer_v1,
	primary_selection, WAYLAND_SELECTION_PRIMARY, VWL_DATA_PROTOCOL_WLR)

VWL_FUNC_DATA_DEVICE_FINISHED(ext_data_control_device_v1)
VWL_FUNC_DATA_DEVICE_FINISHED(zwlr_data_control_device_v1)

VWL_FUNC_DATA_SOURCE_SEND(ext_data_control_source_v1)
VWL_FUNC_DATA_SOURCE_SEND(zwlr_data_control_source_v1)

VWL_FUNC_DATA_SOURCE_CANCELLED(ext_data_control_source_v1) VWL_FUNC_DATA_SOURCE_CANCELLED(zwlr_data_control_source_v1)

VWL_FUNC_DATA_OFFER_OFFER(ext_data_control_offer_v1)
VWL_FUNC_DATA_OFFER_OFFER(zwlr_data_control_offer_v1)

// Listener handlers
struct zwlr_data_control_device_v1_listener
vzwlr_data_control_device_v1_listener = {
    .data_offer = vzwlr_data_control_device_v1_listener_data_offer,
    .selection = vzwlr_data_control_device_v1_listener_selection,
    .primary_selection = vzwlr_data_control_device_v1_listener_primary_selection,
    .finished = vzwlr_data_control_device_v1_listener_finished
};

struct ext_data_control_device_v1_listener
vext_data_control_device_v1_listener = {
    .data_offer = vext_data_control_device_v1_listener_data_offer,
    .selection = vext_data_control_device_v1_listener_selection,
    .primary_selection = vext_data_control_device_v1_listener_primary_selection,
    .finished = vext_data_control_device_v1_listener_finished
};

struct zwlr_data_control_source_v1_listener
vzwlr_data_control_source_v1_listener = {
    .send = vzwlr_data_control_source_v1_listener_send,
    .cancelled = vzwlr_data_control_source_v1_listener_cancelled
};

struct ext_data_control_source_v1_listener
vext_data_control_source_v1_listener = {
    .send = vext_data_control_source_v1_listener_send,
    .cancelled = vext_data_control_source_v1_listener_cancelled
};

struct zwlr_data_control_offer_v1_listener
vzwlr_data_control_offer_v1_listener = {
    .offer = vzwlr_data_control_offer_v1_listener_offer
};

struct ext_data_control_offer_v1_listener
vext_data_control_offer_v1_listener = {
    .offer = vext_data_control_offer_v1_listener_offer
};

// `type` is also used as the user data
#define VWL_CODE_DATA_OBJECT_ADD_LISTENER(type) \
{ \
    if (type->proxy == NULL) \
	return; \
    type->data = data; \
    switch (type->protocol) \
    { \
	case VWL_DATA_PROTOCOL_WLR: \
	    zwlr_data_control_##type##_v1_add_listener( type->proxy, \
		    &vzwlr_data_control_##type##_v1_listener, type); \
	    break; \
	case VWL_DATA_PROTOCOL_EXT:  \
	    ext_data_control_##type##_v1_add_listener(type->proxy, \
		    &vext_data_control_##type##_v1_listener, type); \
	    break; \
	default: \
	    break; \
    } \
}

    static void
vwl_data_device_add_listener(vwl_data_device_T *device, void *data)
{
    VWL_CODE_DATA_OBJECT_ADD_LISTENER(device)
}

    static void
vwl_data_source_add_listener(vwl_data_source_T *source, void *data)
{
    VWL_CODE_DATA_OBJECT_ADD_LISTENER(source)
}

    static void
vwl_data_offer_add_listener(vwl_data_offer_T *offer, void *data)
{
    VWL_CODE_DATA_OBJECT_ADD_LISTENER(offer)
}

/*
 * Sets the selection using the given data device with the given selection. If
 * the device does not support the selection then nothing happens.
 */
    static void
vwl_data_device_set_selection(
	vwl_data_device_T *device,
	vwl_data_source_T *source,
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
    if (offer->protocol == VWL_DATA_PROTOCOL_WLR)
	zwlr_data_control_offer_v1_receive(offer->proxy, mime_type, fd);
    else if (offer->protocol == VWL_DATA_PROTOCOL_EXT)
	ext_data_control_offer_v1_receive(offer->proxy, mime_type, fd);
}

/*
 * Get a data device manager that supports the given selection. If none if found
 * then the manager protocol is set to VWL_DATA_PROTOCOL_NONE.
 */
    static void
vwl_get_data_device_manager(
	vwl_data_device_manager_T *manager,
	wayland_selection_T selection)
{
#define set_manager(manager_name, protocol_enum) \
    { \
	manager->proxy = vwl_gobjects.manager_name; \
	manager->protocol = protocol_enum; \
	return; \
    } \

    // ext data control protocol supports both selections, try it first
    if (vwl_gobjects.ext_data_control_manager_v1 != NULL)
	set_manager(ext_data_control_manager_v1, VWL_DATA_PROTOCOL_EXT)
    if (vwl_gobjects.zwlr_data_control_manager_v1 != NULL)
    {
	int ver = zwlr_data_control_manager_v1_get_version(
		vwl_gobjects.zwlr_data_control_manager_v1);

	// version 2 or greater supports the primary selection
	if ((selection == WAYLAND_SELECTION_PRIMARY && ver >= 2)
		|| selection == WAYLAND_SELECTION_REGULAR)
	    set_manager(zwlr_data_control_manager_v1, VWL_DATA_PROTOCOL_WLR)
    }
    manager->protocol = VWL_DATA_PROTOCOL_NONE;
}

/*
 * Get a data device that manages the given seat's selection.
 */
    static void
vwl_get_data_device(
	vwl_data_device_manager_T *manager,
	vwl_seat_T *seat,
	vwl_data_device_T *device)
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
	vwl_data_device_manager_T *manager,
	vwl_data_source_T *source)
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
    vwl_get_data_device_manager(
	    &vwl_clipboard.regular.manager,
		WAYLAND_SELECTION_REGULAR);
    vwl_get_data_device_manager(
	    &vwl_clipboard.primary.manager,
	    WAYLAND_SELECTION_PRIMARY);

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

    return OK;
}

/*
 * Free up resources used for Wayland selections. Does not destroy global
 * objects such as data device managers.
 */
    void
wayland_cb_uninit(void)
{
    // Free mime types
    ga_clear_strings(&vwl_clipboard.regular.mime_types);
    ga_clear_strings(&vwl_clipboard.primary.mime_types);

    // Destroy any devices or sources
    vwl_data_device_destroy(&vwl_clipboard.regular.device, FALSE);
    vwl_data_device_destroy(&vwl_clipboard.primary.device, FALSE);

    vwl_data_source_destroy(&vwl_clipboard.regular.source, FALSE);
    vwl_data_source_destroy(&vwl_clipboard.primary.source, FALSE);

    // Destroy the current offer if it exists
    vwl_data_offer_destroy(vwl_clipboard.regular.offer, TRUE);
    vwl_data_offer_destroy(vwl_clipboard.primary.offer, TRUE);
    vwl_clipboard_free_mime_types(&vwl_clipboard.regular);
    vwl_clipboard_free_mime_types(&vwl_clipboard.primary);

    vim_memset(&vwl_clipboard, 0, sizeof(vwl_clipboard));
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
	vwl_data_device_T *device,
	vwl_data_offer_T *offer)
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
	vwl_data_device_T *device UNUSED,
	vwl_data_offer_T *offer,
	wayland_selection_T selection)
{
    vwl_clipboard_selection_T *clip_sel = device->data;
    vwl_data_offer_T *prev_offer = clip_sel->offer;

    // Save offer if it selection and clip_sel match, else discard it
    if (clip_sel == &vwl_clipboard.regular
	    && selection == WAYLAND_SELECTION_REGULAR)
	clip_sel->offer = offer;
    else if (clip_sel == &vwl_clipboard.primary
	    && selection == WAYLAND_SELECTION_PRIMARY)
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
    // 1. We own the selection (we'll just access the register directly)
    // 2. No one owns the selection
    if (offer == NULL)
    {
	// Selection cleared/empty
	ga_clear_strings(&clip_sel->tmp_mime_types);
	clip_sel->offer = NULL;
	goto exit;
    }
    else if (clip_sel->source.proxy != NULL)
    {
	// We own the selection, ignore and destroy offer and the offer in
	// clip_sel if any
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
 * 0, then the selection is cleared.
 */
    garray_T *
wayland_cb_get_mime_types(wayland_selection_T selection)
{
    if (vwl_display_roundtrip(&vwl_display) == FAIL)
	return NULL;

    if (selection == WAYLAND_SELECTION_REGULAR)
	return &vwl_clipboard.regular.mime_types;
    else if (selection == WAYLAND_SELECTION_PRIMARY)
	return &vwl_clipboard.primary.mime_types;

    return NULL;
}

/*
 * Receive data from the given selection, and return the fd to read data from.
 * On failure -1 is returned.
 */
    int
wayland_cb_receive_data(const char *mime_type, wayland_selection_T selection)
{
    vwl_clipboard_selection_T *clip_sel;
    vwl_data_offer_T *offer;
    // Create pipe that source client will write to
    int fds[2];

    if (selection == WAYLAND_SELECTION_REGULAR)
    {
	clip_sel = &vwl_clipboard.regular;
	offer = vwl_clipboard.regular.offer;
    }
    else if (selection == WAYLAND_SELECTION_PRIMARY)
    {
	clip_sel = &vwl_clipboard.primary;
	offer = vwl_clipboard.primary.offer;
    }
    else
	return -1;

    if (!wayland_client_is_connected(FALSE) ||
	    !vwl_clipboard_selection_is_ready(clip_sel))
	return -1;

    if (offer == NULL || offer->proxy == NULL)
	return -1;

    if (pipe(fds) == -1)
	return -1;

    vwl_data_offer_receive(offer, mime_type, fds[1]);

    close(fds[1]); // Close before we read data so that when the source client
		   // closes their end we receive an EOF.

    if (vwl_display_flush(&vwl_display) == OK)
	return fds[0];

    close(fds[0]);

    return -1;
}


/*
 * Callback for send event. Just call the user callback which will handle it and
 * do the writing stuff.
 */
    static void
vwl_data_source_listener_send(
	vwl_data_source_T *source,
	const char *mime_type,
	int fd)
{
    vwl_clipboard_selection_T *clip_sel = source->data;

    if (clip_sel->send_cb != NULL)
    {
	if (clip_sel == &vwl_clipboard.regular)
	    clip_sel->send_cb(mime_type, fd, WAYLAND_SELECTION_REGULAR);
	else if (clip_sel == &vwl_clipboard.primary)
	    clip_sel->send_cb(mime_type, fd, WAYLAND_SELECTION_PRIMARY);
    }
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
    {
	if (clip_sel == &vwl_clipboard.regular)
	    clip_sel->cancelled_cb(WAYLAND_SELECTION_REGULAR);
	else if (clip_sel == &vwl_clipboard.primary)
	    clip_sel->cancelled_cb(WAYLAND_SELECTION_PRIMARY);
    }
    vwl_data_source_destroy(source, FALSE);
}

/*
 * Become the given selection's owner, and advertise to other clients the mime
 * types found in mime_types array. Returns FAIL on failure and OK on success.
 */
    int
wayland_cb_own_selection(
	wayland_cb_send_data_func_T send_cb,
	wayland_cb_selection_cancelled_func_T cancelled_cb,
	const char **mime_types,
	int len,
	wayland_selection_T selection)
{
    vwl_clipboard_selection_T *clip_sel;

    if (selection == WAYLAND_SELECTION_REGULAR)
	clip_sel = &vwl_clipboard.regular;
    else if (selection == WAYLAND_SELECTION_PRIMARY)
	clip_sel = &vwl_clipboard.primary;
    else
	return FAIL;

    if (!wayland_client_is_connected(FALSE) ||
	    !vwl_clipboard_selection_is_ready(clip_sel))
	return FAIL;

    clip_sel->send_cb = send_cb;
    clip_sel->cancelled_cb = cancelled_cb;

    if (clip_sel->source.proxy != NULL)
	return OK;

    vwl_create_data_source(&clip_sel->manager, &clip_sel->source);

    vwl_data_source_add_listener(&clip_sel->source, clip_sel);

    // Advertise mime types
    for (int i = 0; i < len; i++)
	vwl_data_source_offer(&clip_sel->source, mime_types[i]);

    vwl_data_device_set_selection(
	    &clip_sel->device,
	    &clip_sel->source,
	    selection);

    if (vwl_display_dispatch(&vwl_display, NULL) == FAIL)
    {
	vwl_data_source_destroy(&clip_sel->source, FALSE);
	return FAIL;
    }

    return OK;
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

    return OK;
}

#endif // FEAT_WAYLAND_CLIPBOARD

/*
 * Disconnect then reconnect wayland connection, and update clipmethod, only if
 * current display connection is invalid, if shebang is passed, or if switching
 * to a new display.
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

    if (eap->forceit || !wayland_client_is_connected(TRUE) ||
	    STRCMP(wayland_display_name, display == NULL ? "" : display))
    {
#ifdef FEAT_WAYLAND_CLIPBOARD
	// Lose any selections we own
	if (clipmethod == CLIPMETHOD_WAYLAND)
	{
	    if (clip_star.owned)
		clip_lose_selection(&clip_star);
	    if (clip_plus.owned)
		clip_lose_selection(&clip_plus);
	}
#endif

	wayland_uninit_client();
	wayland_set_display(display);

	if (wayland_init_client(wayland_display_name) == OK)
	{
	    smsg(_("restoring wayland display %s"), wayland_display_name);

#ifdef FEAT_WAYLAND_CLIPBOARD
	    wayland_cb_init("seat0");
#endif
	}
	else
	{
	    wayland_set_display("");
	    smsg(_("failed restoring, lost connection to wayland display %s"),
		    wayland_display_name);
	}
    }
}

/*
 * Set wayland_display_name to display. Note that this allocate a copy of the
 * string, unless NULL is passed. Note that if NULL is passed then
 * v:wayland_display is set to $WAYLAND_DISPLAY, but wayland_display_name is
 * set to NULL.
 */
    static void
wayland_set_display(const char *display)
{
    if (display == NULL)
	display = (char*)mch_getenv((char_u*)"WAYLAND_DISPLAY");
    else if (display == wayland_display_name)
	// Don't want to be freeing vwl_display_strname then trying to copy it
	// after.
	return;

    vim_free(wayland_display_name);
    wayland_display_name = display == NULL ? NULL :
	(char*)vim_strsave((char_u*)display);

#ifdef FEAT_EVAL
    set_vim_var_string(VV_WAYLAND_DISPLAY,
	    display == NULL ? (char_u*)"" : (char_u*)display, -1);
#endif
}

#endif // FEAT_WAYLAND
