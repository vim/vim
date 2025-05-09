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

typedef struct vwl_seat_S vwl_seat_T;
typedef struct vwl_registry_S vwl_registry_T;
typedef struct vwl_display_S vwl_display_T;

#ifdef FEAT_WAYLAND_CLIPBOARD

typedef enum {
    VWL_DATA_PROTOCOL_NONE,
    VWL_DATA_PROTOCOL_EXT,
    VWL_DATA_PROTOCOL_WLR
} vwl_data_protocol_T;

typedef struct vwl_data_offer_S vwl_data_offer_T;
typedef struct vwl_data_source_S vwl_data_source_T;
typedef struct vwl_data_device_S vwl_data_device_T;
typedef struct vwl_data_device_manager_S vwl_data_device_manager_T;

typedef struct vwl_data_device_listener_S vwl_data_device_listener_T;
typedef struct vwl_data_source_listener_S vwl_data_source_listener_T;
typedef struct vwl_data_offer_listener_S vwl_data_offer_listener_T;

typedef struct vwl_clipboard_S vwl_clipboard_T;

#endif // FEAT_WAYLAND_CLIPBOARD

// Struct that represents a seat. (Should be accessed via vwl_registry_get_seat()).
struct vwl_seat_S {
    struct wl_seat *proxy;
    vwl_registry_T *registry;

    uint32_t name;		    // Numerical name
    char *name_str;	    	    // Name of seat as text (e.g. seat0, seat1...).
    uint32_t capabilities;  	    // Bitmask of the capabilites of the seat
			    	    // (pointer, keyboard, touch).
};

// Struct that represents a registry, including its global objects.
struct vwl_registry_S {
    struct wl_registry *proxy;
    vwl_display_T *display;

    garray_T seats;		    // We can have more than one seat, so keep
				    // track of all of them.
#ifdef FEAT_WAYLAND_CLIPBOARD
    // Data control data device managers
    struct zwlr_data_control_manager_v1 *vzwlr_data_control_manager_v1;
    uint32_t vzwlr_data_control_manager_v1_name;

    struct ext_data_control_manager_v1 *vext_data_control_manager_v1;
    uint32_t vext_data_control_manager_v1_name;
#endif
};

// Struct that represents a wayland display connection
struct vwl_display_S {
    struct wl_display *proxy;
    int fd;			    // File descriptor for display

    vwl_registry_T registry;	    // Registry for display
};

#ifdef FEAT_WAYLAND_CLIPBOARD

struct vwl_data_device_listener_S {
    void (*data_offer)(vwl_data_device_T *device, vwl_data_offer_T *offer);
    void (*selection)(vwl_data_device_T *device, wayland_selection_T selection);
    // Does not need to be set if protocol doesn't have it.
    void (*finished)(vwl_data_device_T *device);
};

struct vwl_data_source_listener_S {
    void (*send)(vwl_data_source_T *source, const char *mime_type, int fd);
    void (*cancelled)(vwl_data_source_T *source);
};

struct vwl_data_offer_listener_S {
    void (*offer)(vwl_data_offer_T *offer, const char *mime_type);
};

struct vwl_data_offer_S {
    struct wl_proxy *proxy;
    void **data;
    garray_T mime_types; // mime types chosen to be received

    vwl_data_device_T *device;
    vwl_data_offer_listener_T listener;
};

struct vwl_data_source_S {
    struct wl_proxy *proxy;
    void **data;

    vwl_data_device_manager_T *manager;
    vwl_data_source_listener_T listener;
};

struct vwl_data_device_S {
    struct wl_proxy *proxy;
    void **data;

    vwl_data_device_manager_T *manager;
    vwl_data_device_listener_T listener;
    vwl_data_offer_T offer;	// Offer used by this data device

    wayland_sbitmask_T selection;	// Selection(s) that this device manages.
};

struct vwl_data_device_manager_S {
    struct wl_proxy *proxy;

    vwl_registry_T *registry;
    vwl_data_protocol_T protocol;

    wayland_sbitmask_T selections_supported;
};

// High level interface to the wayland selection(s)
struct vwl_clipboard_S {
    vwl_seat_T *seat;

    struct {
	vwl_data_device_manager_T manager;

	// Created and destroyed as needed when we want to receive data
	vwl_data_device_T receive_device;
	// Will only may be destroyed when a global_remove event happens.
	// Used for setting the selection of the seat.
	vwl_data_device_T source_device;

	vwl_data_source_T source;
    } regular;

    // If primary selection is not available then use a protocol that supports
    // the regular selection.
    struct {
	vwl_data_device_manager_T manager;

	vwl_data_device_T receive_device;
	vwl_data_device_T source_device;

	vwl_data_source_T source;
    } primary;
};

#endif // FEAT_WAYLAND_CLIPBOARD

static int vwl_display_flush(vwl_display_T *display);
static void vwl_callback_done(void *data, struct wl_callback *callback,
	uint32_t cb_data);
static int vwl_display_roundtrip(vwl_display_T *display);
static int vwl_display_dispatch(vwl_display_T *display, int *num_dispatched);

static void vwl_log_handler(const char *fmt, va_list args);
static int vwl_connect_display(vwl_display_T *display_s, const char *display);
static void vwl_disconnect_display(vwl_display_T *display);
static int vwl_init_registry(vwl_display_T *display);

static void vwl_registry_listener_global(void *data, struct wl_registry *registry,
	uint32_t name, const char *interface, uint32_t version);
static void vwl_registry_listener_global_remove(void *data,
	struct wl_registry *registry,  uint32_t name);

static void vwl_seat_listener_name(void *data, struct wl_seat *seat,
	const char *name);
static void vwl_seat_listener_capabilities(void *data, struct wl_seat *seat,
	uint32_t capabilities);

static vwl_seat_T *vwl_registry_get_seat(vwl_registry_T *registry, const char *name);
static void vwl_registry_remove_seat(vwl_registry_T *registry, int index);
static void vwl_registry_remove_all_seats(vwl_registry_T *registry);

#ifdef FEAT_WAYLAND_CLIPBOARD

static int vwl_registry_get_data_device_manager(vwl_data_device_manager_T *manager,
	vwl_registry_T *registry, wayland_sbitmask_T selection);

static int vwl_data_device_manager_get_data_device( vwl_data_device_T *device,
	vwl_data_device_manager_T *manager, vwl_seat_T *seat,
	wayland_sbitmask_T selection);
static int vwl_data_device_manager_create_data_source(vwl_data_source_T *source,
	vwl_data_device_manager_T *manager);

static void vwl_data_device_wrap_offer(vwl_data_offer_T *offer,
	vwl_data_device_T *device, struct wl_proxy *offer_proxy);

static void vwl_data_device_destroy(vwl_data_device_T *device);
static void vwl_data_offer_destroy(vwl_data_offer_T *offer);
static void vwl_data_source_destroy(vwl_data_source_T *source);

static void vwl_data_device_set_selection(vwl_data_device_T *device,
	vwl_data_source_T *source);

static void vwl_data_source_offer(vwl_data_source_T *source,
	const char *mime_type);

static void vwl_data_offer_receive(vwl_data_offer_T *offer,
	const char *mime_type, int fd);

static void vwl_data_device_listen(vwl_data_device_T *device);
static void vwl_data_source_listen(vwl_data_source_T *source);
static void vwl_data_offer_listen(vwl_data_offer_T *offer);

// Callbacks/handlers for listeners
static void wayland_cb_data_device_data_offer(vwl_data_device_T *device, vwl_data_offer_T *offer);
static void wayland_cb_data_offer_offer(vwl_data_offer_T *offer, const char *mime_type);
static void wayland_cb_data_device_selection(vwl_data_device_T *device,
	wayland_selection_T selection);
static void wayland_cb_data_device_finished(vwl_data_device_T *device);

static void wayland_set_display(const char *display);

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

#ifdef FEAT_WAYLAND_CLIPBOARD
vwl_clipboard_T vwl_clipboard;
#endif

/*
 * Helper function for wl_display_flush. Returns OK on sucess and
 * FAIL on failure.
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
 * Helper function for wl_display_roundtrip. Returns OK on sucess and
 * FAIL on failure.
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
    // have sent and handling eventss.
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
 * Helper function for wl_display_dispatch, returns OK on sucess and FAIL on
 * failure. if num_dispatched is not NULL then the variable it points to will be
 * set to the amount of events dispatched, unless FAIL is returned.
 */
    static int
vwl_display_dispatch(vwl_display_T *display, int *num_dispatched)
{
    int num;
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
    if (wl_display_read_events(display->proxy) == -1)
	return FAIL;

    // Dispatch those events (call the handlers associated for each event)
    if ((num = wl_display_dispatch_pending(display->proxy)) == -1)
	return FAIL;
    if (num_dispatched != NULL)
	*num_dispatched = num;

    return OK;
}

/*
 * Redirect libwayland logging to use ch_log + semsg instead.
 */
    static void
vwl_log_handler(const char *fmt, va_list args)
{
    size_t len = STRLEN(fmt) + STRLEN("LIBWAYLAND: ") + 1;
    char *format = alloc(len);

    if (format == NULL)
	return;
    // Subtract one to remove newline that libwayland puts
    vim_snprintf(format, len - 1, "libwayland internal error: %s", fmt);
    ch_vlog(NULL, format, args);
    semsg(format, args);
    vim_free(format);
}

/*
 * Initialize the display struct and connect to the display with name. Note that
 * passing NULL as the name will use libwayland's automatic way of getting
 * determining the name. Additionally, the caller should ensure that client-side
 * resources are freed in display, in case it is reconnecting. Returns OK on
 * sucess and FAIL on failure.
 */
    static int
vwl_connect_display(vwl_display_T *display_s, const char *display)
{
    if (wayland_no_connect)
	return FAIL;

    // Must set log handler before we connect display in order to work.
    // Pretty sure we just need to do this one time, but no harm doing more than
    // once right? (if we are reconnecting to the display again)?
    wl_log_set_handler_client(vwl_log_handler);

    display_s->proxy = wl_display_connect(display);

    if (display_s->proxy == NULL)
	return FAIL;

    wayland_set_display(display);
    display_s->fd = wl_display_get_fd(display_s->proxy);

    return OK;
}

/*
 * Disconnects the display and frees up all resources. Note that global objects
 * in the registry aren't destroyed, only the registry proxy itself is.
 */
    static void
vwl_disconnect_display(vwl_display_T *display)
{
    if (display->registry.proxy != NULL)
    {
	wl_registry_destroy(display->registry.proxy);
	display->registry.proxy = NULL;
    }
    if (display->proxy != NULL)
    {
	wl_display_disconnect(display->proxy);
	display->proxy = NULL;
    }
}

/*
 * Initializes a registry for display. vwl_init_display() should be called
 * before this.
 */
    static int
vwl_init_registry(vwl_display_T *display)
{
    display->registry.proxy = wl_display_get_registry(display->proxy);
    display->registry.display = display;

    if (display->registry.proxy == NULL)
	return FAIL;

    // Initialize grow array for seats
    ga_init2(&display->registry.seats, sizeof(vwl_seat_T), 1);

    wl_registry_add_listener(display->registry.proxy,
	    &vwl_registry_listener, display);

    if (vwl_display_roundtrip(display) == FAIL)
    {
	wl_registry_destroy(display->registry.proxy);
	display->registry.proxy = NULL;
	return FAIL;
    }

    // Check if all seats have been allocated correctly, and remove any that
    // aren't.
    for (int i = 0; i < display->registry.seats.ga_len; i++)
	if (((vwl_seat_T*)display->registry.seats.ga_data)[i].name_str == NULL)
	    vwl_registry_remove_seat(&display->registry, i);
    return OK;
}

#define VWL_REGISTRY_BIND_GLOBAL(type, min_ver) \
     if (STRCMP(#type, interface) == 0 && version >= (min_ver)) \
     { \
         registry->v##type = wl_registry_bind(registry->proxy, name, \
                                         &type##_interface, version); \
         registry->v##type##_name = name; \
         return; \
     }

/*
 * Called when we receive a global object from the compositor. Keep in sync with
 * vwl_registry_listener_global_remove().
 */
    static void
vwl_registry_listener_global(void *data, struct wl_registry *reg_proxy UNUSED,
	uint32_t name, const char *interface, uint32_t version)
{
    vwl_display_T *display = (vwl_display_T*) data;
    vwl_registry_T *registry = &display->registry;

#ifdef FEAT_WAYLAND_CLIPBOARD
    VWL_REGISTRY_BIND_GLOBAL(zwlr_data_control_manager_v1, 1)
    VWL_REGISTRY_BIND_GLOBAL(ext_data_control_manager_v1, 1)
#endif

    if (STRCMP(interface, "wl_seat") == 0 && version >= 2)
    {
	struct wl_seat *seat;
	vwl_seat_T *vseat;

	// Add seat to our array
	if (ga_grow(&registry->seats, 1) == FAIL)
	    return;
	seat = wl_registry_bind(registry->proxy, name, &wl_seat_interface, version);
	vseat = &((vwl_seat_T *)registry->seats.ga_data)[registry->seats.ga_len++];

        // Get the name of seat and its capabilities.
	wl_seat_add_listener(seat, &vwl_seat_listener, vseat);

        *vseat = (vwl_seat_T) { .proxy = seat, .name = name };
	vwl_display_roundtrip(display);
    }
}

#define VWL_REGISTRY_REMOVE_GLOBAL(type) \
     if (registry->v##type##_name == name) \
     { \
	 type##_destroy(registry->v##type); \
	 registry->v##type = NULL; \
	 return; \
     }
/*
 * Called when the compositor notifies us that a global object is now
 * unavailable. Therefore we should destroy the object with that numerical name.
 */
    static void
vwl_registry_listener_global_remove(void *data,
	struct wl_registry *reg_proxy UNUSED, uint32_t name)
{
    vwl_display_T *display = (vwl_display_T*) data;
    vwl_registry_T *registry = &display->registry;

    // Destroy matching seat if any
    for (int i = 0; i < display->registry.seats.ga_len; i++)
	if (((vwl_seat_T *)registry->seats.ga_data)[i].name == name)
	{
	    vwl_registry_remove_seat(&display->registry, i);
	    return;
	}

#ifdef FEAT_WAYLAND_CLIPBOARD
    VWL_REGISTRY_REMOVE_GLOBAL(zwlr_data_control_manager_v1)
    VWL_REGISTRY_REMOVE_GLOBAL(ext_data_control_manager_v1)
#endif
    vwl_display_roundtrip(display);
}

/*
 * Get the text name of the seat
 */
    static void
vwl_seat_listener_name(void *data, struct wl_seat *seat UNUSED,
	const char *name)
{
    ((vwl_seat_T*)data)->name_str = (char*)vim_strsave((char_u*)name);
}

/*
 * Get the capabilities of the seat
 */
    static void
vwl_seat_listener_capabilities(void *data,
	struct wl_seat *seat UNUSED, uint32_t capabilities)
{
    ((vwl_seat_T*)data)->capabilities = capabilities;
}

/*
 * Get pointer to a seat from registry with given name. Returns NULL if there
 * are no available seats with that name. Use seat0 if you want the default
 * seat.
 */
    static vwl_seat_T *
vwl_registry_get_seat(vwl_registry_T *registry, const char *name)
{
    for (int i = 0; i < registry->seats.ga_len; i++)
    {
	vwl_seat_T *seat = &((vwl_seat_T*)registry->seats.ga_data)[i];

	if (STRCMP(seat->name_str, name) == 0)
	    return seat;
    }
    return NULL;
}

/*
 * Destroy seat at specified index for the array of seats that registry
 * contains.
 */
    static void
vwl_registry_remove_seat(vwl_registry_T *registry, int index)
{
    vwl_seat_T *vseat = &((vwl_seat_T *)registry->seats.ga_data)[index];

    if (vseat->proxy == NULL)
	return;

    // Release seat if version greater or equal to 5. From the looks of
    // it, releasing a seat just helps the compositor manage resources
    // better. Let's be nice :)
    if (wl_seat_get_version(vseat->proxy) >= 5)
	wl_seat_release(vseat->proxy);
    else
	// Else just destroy it normally on the client side
	wl_seat_destroy(vseat->proxy);
    vim_free(vseat->name_str);

    // Move all seats after it forward one index
    for (int i = index + 1; i < registry->seats.ga_len; i++)
	((vwl_seat_T*)registry->seats.ga_data)[i - 1] =
	    ((vwl_seat_T*)registry->seats.ga_data)[i];
    registry->seats.ga_len--;
}

    static void
vwl_registry_remove_all_seats(vwl_registry_T *registry)
{
    for (int i = 0; i < registry->seats.ga_len; i++)
	vwl_registry_remove_seat(registry, i);
}

/*
 * Connects to the wayland display with given name and binds to global objects
 * as needed. If display is NULL then use the $WAYLAND_DISPLAY environment
 * variable instead (handled by libwayland).
 */
    int
wayland_init_client(const char *display)
{
    if (vwl_connect_display(&vwl_display, display) == FAIL)
	return FAIL;
    if (vwl_init_registry(&vwl_display) == FAIL)
	return FAIL;

    wayland_display_fd = vwl_display.fd;

    return OK;
}

/*
 * Disconnect wayland client and free up resources.
 */
    void
wayland_uninit_client(void)
{
#ifdef FEAT_WAYLAND_CLIPBOARD
    wayland_cb_uninit();
#endif
    vwl_registry_remove_all_seats(&vwl_display.registry);
    vwl_disconnect_display(&vwl_display);
}

/*
 *
 */
    int
wayland_client_is_connected(void)
{
    if (vwl_display.proxy == NULL)
	return FALSE;

    // Display errors are always fatal
    if (wl_display_get_error(vwl_display.proxy) != 0
	    || vwl_display_flush(&vwl_display) == FAIL)
	return FALSE;
    return TRUE;
}

/*
 *
 */
    int
wayland_client_update(void)
{
    return vwl_display_dispatch(&vwl_display, NULL);
}

#ifdef FEAT_WAYLAND_CLIPBOARD

/*
 * Setup a data device manager and chooses the proper protocol to
 * use for the data device manager. If primary is TRUE then only use protocols
 * that support the primary selection. If there is no available protocol that
 * supports the primary selection, then return FAIL. Returns OK on success and
 * FAIL on failure
 */
    static int
vwl_registry_get_data_device_manager(vwl_data_device_manager_T *manager,
	vwl_registry_T *registry, wayland_sbitmask_T selection)
{
    int r = FALSE, p = FALSE;

    // ext protocol can do both
    if (registry->vext_data_control_manager_v1 != NULL)
    {
        manager->proxy = (struct wl_proxy *)
	    registry->vext_data_control_manager_v1;
	manager->protocol = VWL_DATA_PROTOCOL_EXT;

	p = r = TRUE;
	goto success;
    }
    if (registry->vzwlr_data_control_manager_v1 != NULL)
    {
	// Only version 2 or greater supports the primary selection
        uint32_t ver = zwlr_data_control_manager_v1_get_version(
            registry->vzwlr_data_control_manager_v1);

        if ((selection & WAYLAND_SELECTION_PRIMARY && ver > 1)
		|| selection & WAYLAND_SELECTION_REGULAR)
        {
	    if (ver > 1)
		p = TRUE;

	    manager->proxy = (struct wl_proxy *)
		registry->vzwlr_data_control_manager_v1;
	    manager->protocol = VWL_DATA_PROTOCOL_WLR;

	    r = TRUE;
	    goto success;
        }
    }

    return FAIL;
success:
    if (r)
	manager->selections_supported |= WAYLAND_SELECTION_REGULAR;
    if (p)
	manager->selections_supported |= WAYLAND_SELECTION_PRIMARY;

    return OK;
}

/*
 * Setup a data device that manages either the primary, regular, or both
 * selection of seat. Returns NULL if primary is TRUE but manager protocol
 * doesn't support primary selection, vice versa for regular selection too.
 */
    static int
vwl_data_device_manager_get_data_device(vwl_data_device_T *device,
	vwl_data_device_manager_T *manager, vwl_seat_T *seat,
	wayland_sbitmask_T selection)
{
    if (seat->proxy == NULL)
	return FAIL;

    // TODO: set_selection should have an argument on which selection to use
    // Check if we can actually support the given selection(s) using manager
    if ((manager->selections_supported & selection) != selection)
	return FAIL;

    device->proxy = NULL;
    if (manager->protocol == VWL_DATA_PROTOCOL_WLR)
    {
        device->proxy =
            (struct wl_proxy *)zwlr_data_control_manager_v1_get_data_device(
		    (struct zwlr_data_control_manager_v1 *)manager->proxy,
		    (struct wl_seat *)seat->proxy);
    }
    else if (manager->protocol == VWL_DATA_PROTOCOL_EXT)
    {
        device->proxy =
            (struct wl_proxy *)ext_data_control_manager_v1_get_data_device(
		    (struct ext_data_control_manager_v1 *)manager->proxy,
		    (struct wl_seat *)seat->proxy);
    }
    if (device->proxy == NULL)
	return FAIL;

    device->manager = manager;
    device->selection = selection;

    return OK;
}

/*
 * Setup a data source.
 */
    static int
vwl_data_device_manager_create_data_source(vwl_data_source_T *source,
	vwl_data_device_manager_T *manager)
{
    source->proxy = NULL;

    if (manager->protocol == VWL_DATA_PROTOCOL_WLR)
    {
        source->proxy =
            (struct wl_proxy *)zwlr_data_control_manager_v1_create_data_source(
		    (struct zwlr_data_control_manager_v1 *)manager->proxy);
    }
    else if (manager->protocol == VWL_DATA_PROTOCOL_EXT)
    {
        source->proxy =
            (struct wl_proxy *)ext_data_control_manager_v1_create_data_source(
		    (struct ext_data_control_manager_v1 *)manager->proxy);
    }
    if (source->proxy == NULL)
	return FAIL;
    source->manager = manager;

    return OK;
}

/*
 * Setup a data offer struct with the given offer proxy object.
 */
    static void
vwl_data_device_wrap_offer(vwl_data_offer_T *offer, vwl_data_device_T *device,
	struct wl_proxy *offer_proxy)
{
    offer->proxy = offer_proxy;
    offer->device = device;
    ga_init2(&offer->mime_types, sizeof(char *), 1);
}

#define VWL_CODE_DATA_OBJECT_DESTROY(type, protocol) \
{ \
    if (type->proxy == NULL) \
	return; \
    if (protocol == VWL_DATA_PROTOCOL_WLR) \
	zwlr_data_control_##type##_v1_destroy( \
		(struct zwlr_data_control_##type##_v1 *)type->proxy); \
    else if (protocol == VWL_DATA_PROTOCOL_WLR) \
	ext_data_control_##type##_v1_destroy( \
		(struct ext_data_control_##type##_v1 *)type->proxy); \
}

    static void
vwl_data_device_destroy(vwl_data_device_T *device)
{
    VWL_CODE_DATA_OBJECT_DESTROY(device, device->manager->protocol)
}

    static void
vwl_data_offer_destroy(vwl_data_offer_T *offer)
{
    VWL_CODE_DATA_OBJECT_DESTROY(offer, offer->device->manager->protocol)
}

    static void
vwl_data_source_destroy(vwl_data_source_T *source)
{
    VWL_CODE_DATA_OBJECT_DESTROY(source, source->manager->protocol)
}

    static void
vwl_data_device_manager_destroy(vwl_data_device_manager_T *manager)
{
    VWL_CODE_DATA_OBJECT_DESTROY(manager, manager->protocol)
}

/*
 * Sets the selection using the given data device. The primary or regular
 * selection is chosen automatically.
 */
    static void
vwl_data_device_set_selection(vwl_data_device_T *device,
	vwl_data_source_T *source)
{
    if (device->manager->protocol == VWL_DATA_PROTOCOL_EXT)
    {
	if (device->selection & WAYLAND_SELECTION_PRIMARY)
	    ext_data_control_device_v1_set_primary_selection(
		    (struct ext_data_control_device_v1 *)device->proxy,
		    (struct ext_data_control_source_v1 *)source->proxy);
	else
	    ext_data_control_device_v1_set_selection(
		    (struct ext_data_control_device_v1 *)device->proxy,
		    (struct ext_data_control_source_v1 *)source->proxy);
    }
    else if (device->manager->protocol == VWL_DATA_PROTOCOL_WLR)
    {
	if (device->selection & WAYLAND_SELECTION_PRIMARY)
	    zwlr_data_control_device_v1_set_primary_selection(
		    (struct zwlr_data_control_device_v1 *)device->proxy,
		    (struct zwlr_data_control_source_v1 *)source->proxy);
	else
	    zwlr_data_control_device_v1_set_selection(
		    (struct zwlr_data_control_device_v1 *)device->proxy,
		    (struct zwlr_data_control_source_v1 *)source->proxy);
    }
}

/*
 * Adds mime type to the list of mime types that will be advertised by source.
 */
    static void
vwl_data_source_offer(vwl_data_source_T *source, const char *mime_type)
{
    if (source->manager->protocol == VWL_DATA_PROTOCOL_EXT)
        ext_data_control_source_v1_offer(
            (struct ext_data_control_source_v1 *)source->proxy, mime_type);
    else if (source->manager->protocol == VWL_DATA_PROTOCOL_WLR)
        zwlr_data_control_source_v1_offer(
            (struct zwlr_data_control_source_v1 *)source->proxy, mime_type);
}

/*
 * Starts the data transfer through fd, which should be created by the receiving
 * client, for mime_type.
 */
    static void
vwl_data_offer_receive(vwl_data_offer_T *offer, const char *mime_type, int fd)
{
    if (offer->device->manager->protocol == VWL_DATA_PROTOCOL_EXT)
        ext_data_control_offer_v1_receive(
            (struct ext_data_control_offer_v1 *)offer->proxy, mime_type, fd);
    else if (offer->device->manager->protocol == VWL_DATA_PROTOCOL_WLR)
        zwlr_data_control_offer_v1_receive(
            (struct zwlr_data_control_offer_v1 *)offer->proxy, mime_type, fd);
}

// Handlers for data events boilerplate macros.
// All these handlers do is just do some NULL checks and then call the
// respective generic event handler.
#define VWL_FUNC_DATA_DEVICE_DATA_OFFER(device_interface, offer_interface) \
static void v##device_interface##_data_offer( \
	void *data, struct device_interface *device_proxy UNUSED, \
	struct offer_interface *offer_proxy) \
{ \
    vwl_data_device_T *device = data; \
    if (device->listener.data_offer != NULL) \
    { \
	vwl_data_device_wrap_offer(&device->offer, device, \
		(struct wl_proxy *)offer_proxy); \
	device->listener.data_offer(device, &device->offer); \
    } \
}
#define VWL_FUNC_DATA_DEVICE_SELECTION( \
	device_interface, offer_interface, type, selection_type) \
static void v##device_interface##_##type( \
	void *data, struct device_interface *device_proxy UNUSED, \
	struct offer_interface *offer_proxy UNUSED) \
{ \
    vwl_data_device_T *device = data; \
    if (device->listener.selection != NULL) \
    { \
	if (offer_proxy != NULL) \
	    device->listener.selection(device, selection_type); \
    } \
}
#define VWL_FUNC_DATA_DEVICE_FINISHED(device_interface) \
static void v##device_interface##_finished( \
	void *data, struct device_interface *device_proxy UNUSED) \
{ \
    vwl_data_device_T *device = data; \
    if (device->listener.finished != NULL) \
	device->listener.finished(device); \
}
#define VWL_FUNC_DATA_SOURCE_SEND(source_interface) \
static void v##source_interface##_send(void *data, \
	struct source_interface *source_proxy UNUSED, \
	const char *mime_type, int fd) \
{ \
    vwl_data_source_T *source = data; \
    if (source->listener.send != NULL) \
	source->listener.send(source, mime_type, fd); \
}
#define VWL_FUNC_DATA_SOURCE_CANCELLED(source_interface) \
static void v##source_interface##_cancelled(void *data, \
	struct source_interface *source_proxy UNUSED) \
{ \
    vwl_data_source_T *source = data; \
    if (source->listener.cancelled != NULL) \
	source->listener.cancelled(source); \
}
#define VWL_FUNC_DATA_OFFER_OFFER(offer_interface) \
static void v##offer_interface##_offer(void *data, \
	struct offer_interface *offer_proxy UNUSED, \
	const char *mime_type) \
{ \
    vwl_data_offer_T *offer = data; \
    if (offer->listener.offer != NULL) \
	offer->listener.offer(offer, mime_type); \
}

VWL_FUNC_DATA_DEVICE_DATA_OFFER(
	ext_data_control_device_v1, ext_data_control_offer_v1)
VWL_FUNC_DATA_DEVICE_DATA_OFFER(
	zwlr_data_control_device_v1, zwlr_data_control_offer_v1)

VWL_FUNC_DATA_DEVICE_SELECTION(
	ext_data_control_device_v1, ext_data_control_offer_v1,
	selection, WAYLAND_SELECTION_REGULAR)
VWL_FUNC_DATA_DEVICE_SELECTION(
	zwlr_data_control_device_v1, zwlr_data_control_offer_v1,
	selection, WAYLAND_SELECTION_REGULAR)

VWL_FUNC_DATA_DEVICE_SELECTION(
	ext_data_control_device_v1, ext_data_control_offer_v1,
	primary_selection, WAYLAND_SELECTION_PRIMARY)
VWL_FUNC_DATA_DEVICE_SELECTION(
	zwlr_data_control_device_v1, zwlr_data_control_offer_v1,
	primary_selection, WAYLAND_SELECTION_PRIMARY)

VWL_FUNC_DATA_DEVICE_FINISHED(ext_data_control_device_v1)
VWL_FUNC_DATA_DEVICE_FINISHED(zwlr_data_control_device_v1)

VWL_FUNC_DATA_SOURCE_SEND(ext_data_control_source_v1)
VWL_FUNC_DATA_SOURCE_SEND(zwlr_data_control_source_v1)

VWL_FUNC_DATA_SOURCE_CANCELLED(ext_data_control_source_v1)
VWL_FUNC_DATA_SOURCE_CANCELLED(zwlr_data_control_source_v1)

VWL_FUNC_DATA_OFFER_OFFER(ext_data_control_offer_v1)
VWL_FUNC_DATA_OFFER_OFFER(zwlr_data_control_offer_v1)

// Listener handlers
struct zwlr_data_control_device_v1_listener
    vzwlr_data_control_device_v1_listener = {
	.data_offer = vzwlr_data_control_device_v1_data_offer,
	.selection = vzwlr_data_control_device_v1_selection,
	.primary_selection = vzwlr_data_control_device_v1_primary_selection,
	.finished = vzwlr_data_control_device_v1_finished
    };

struct ext_data_control_device_v1_listener
    vext_data_control_device_v1_listener = {
	.data_offer = vext_data_control_device_v1_data_offer,
	.selection = vext_data_control_device_v1_selection,
	.primary_selection = vext_data_control_device_v1_primary_selection,
	.finished = vext_data_control_device_v1_finished
    };

struct zwlr_data_control_source_v1_listener
    vzwlr_data_control_source_v1_listener = {
	.send = vzwlr_data_control_source_v1_send,
	.cancelled = vzwlr_data_control_source_v1_cancelled
    };

struct ext_data_control_source_v1_listener
    vext_data_control_source_v1_listener = {
	.send = vext_data_control_source_v1_send,
	.cancelled = vext_data_control_source_v1_cancelled
    };

struct zwlr_data_control_offer_v1_listener
    vzwlr_data_control_offer_v1_listener = {
	.offer = vzwlr_data_control_offer_v1_offer
    };

struct ext_data_control_offer_v1_listener
    vext_data_control_offer_v1_listener = {
	.offer = vext_data_control_offer_v1_offer
    };

#define VWL_CODE_DATA_OBJECT_ADD_LISTENER(type, protocol) \
{ \
    if (type->proxy == NULL) \
	return; \
    if (protocol == VWL_DATA_PROTOCOL_WLR) \
	zwlr_data_control_##type##_v1_add_listener( \
		(struct zwlr_data_control_##type##_v1 *)type->proxy, \
		&vzwlr_data_control_##type##_v1_listener, \
		type); \
    else if (protocol == VWL_DATA_PROTOCOL_WLR) \
	ext_data_control_##type##_v1_add_listener( \
		(struct ext_data_control_##type##_v1 *)type->proxy, \
		&vext_data_control_##type##_v1_listener, \
		type); \
}

    static void
vwl_data_device_listen(vwl_data_device_T *device)
{
    VWL_CODE_DATA_OBJECT_ADD_LISTENER(device, device->manager->protocol)
}

    static void
vwl_data_source_listen(vwl_data_source_T *source)
{
    VWL_CODE_DATA_OBJECT_ADD_LISTENER(source, source->manager->protocol)
}

    static void
vwl_data_offer_listen(vwl_data_offer_T *offer)
{
    VWL_CODE_DATA_OBJECT_ADD_LISTENER(offer, offer->device->manager->protocol)
}

/*
 * Setup required objects to interact with wayland selections/clipboard on given
 * seat. Returns OK on success and FAIL on failure.
 */
    int
wayland_cb_init(const char *seat)
{
    vwl_seat_T *vseat;
    int ret;

    vseat = vwl_registry_get_seat(&vwl_display.registry, seat);

    if (vseat == NULL)
	goto error;
    vwl_clipboard.seat = vseat;

    // We get the data device manager for each selection and then retrieve a
    // data device from each manager used for setting the selection.
    ret = vwl_registry_get_data_device_manager(&vwl_clipboard.regular.manager,
	    &vwl_display.registry, WAYLAND_SELECTION_REGULAR);

    if (ret == FAIL)
	goto error;

    ret = vwl_registry_get_data_device_manager(&vwl_clipboard.primary.manager,
	    &vwl_display.registry, WAYLAND_SELECTION_PRIMARY);

    ret = vwl_data_device_manager_get_data_device(
	    &vwl_clipboard.primary.source_device,
	    &vwl_clipboard.primary.manager, vseat,
	    ret == FAIL ? WAYLAND_SELECTION_REGULAR : WAYLAND_SELECTION_PRIMARY);

    if (ret == FAIL)
	goto error;

    ret = vwl_data_device_manager_get_data_device(
	    &vwl_clipboard.regular.source_device,
	    &vwl_clipboard.regular.manager, vseat, WAYLAND_SELECTION_REGULAR);

    if (ret == FAIL)
	goto error;

    vwl_display_roundtrip(&vwl_display);

    return OK;
error:
    wayland_cb_uninit();
    return FAIL;
}


/*
 * Free up resources used for wayland selections.
 */
    void
wayland_cb_uninit(void)
{
    vwl_data_device_T *devices[] = {
	&vwl_clipboard.regular.receive_device,
	&vwl_clipboard.regular.source_device,
	&vwl_clipboard.primary.receive_device,
	&vwl_clipboard.primary.source_device,
    };
    int primary_same_as_regular = FALSE;

    for (size_t i = 0; i < sizeof(devices)/sizeof(*devices); i++)
	if (devices[i]->proxy != NULL)
	{
	    vwl_data_device_destroy(devices[i]);
	    devices[i]->proxy = NULL;
	}

    vwl_clipboard.seat = NULL;

    if (vwl_clipboard.regular.manager.proxy == vwl_clipboard.primary.manager.proxy)
	primary_same_as_regular = TRUE;

    // Free data device managers manually because they can refer to the same
    // object if primary selection is not supported.
    if (vwl_clipboard.regular.manager.proxy != NULL)
	vwl_data_device_manager_destroy(&vwl_clipboard.regular.manager);
    vwl_clipboard.regular.manager.proxy = NULL;

    if (!primary_same_as_regular)
	if (vwl_clipboard.primary.manager.proxy != NULL)
	    vwl_data_device_manager_destroy(&vwl_clipboard.primary.manager);
    vwl_clipboard.primary.manager.proxy = NULL;

    vwl_display_flush(&vwl_display);
}

/*
 * Receive data from either the regular or primary selection. If the primary
 * selection is not available then use the regular selection will be used in its
 * place. Returns OK on success and FAIL on failure.
 */
    int
wayland_cb_receive_selection(
	wayland_cb_receive_data_func_T receive_callback,
	wayland_cb_choose_offer_func_T offer_callback,
	wayland_selection_T selection,
	void *user_data,
	int free_strings) // if TRUE then assume the garray_T for mime types is
			  // an array of heap allocated strings.
{
    vwl_data_device_T *device;
    void *data[] = {receive_callback, offer_callback, user_data, &free_strings};
    int ret;

    if (selection == WAYLAND_SELECTION_PRIMARY)
	device = &vwl_clipboard.primary.receive_device;
    if (selection == WAYLAND_SELECTION_REGULAR)
	device = &vwl_clipboard.regular.receive_device;

    ret = vwl_data_device_manager_get_data_device(device,
	    &vwl_clipboard.regular.manager, vwl_clipboard.seat,
	    selection);
    if (ret == FAIL)
	return FAIL;

    device->data = data;
    device->listener.data_offer = wayland_cb_data_device_data_offer;
    device->listener.selection = wayland_cb_data_device_selection;
    device->listener.finished = wayland_cb_data_device_finished;

    vwl_data_device_listen(device);

    vwl_display_roundtrip(&vwl_display);

    vwl_data_device_destroy(device);
    vim_memset(device, 0, sizeof(*device));

    vwl_display_flush(&vwl_display);

    return OK;
}

/*
 * Offer argument is just a pointer to device->offer
 */
static void
wayland_cb_data_device_data_offer(
	vwl_data_device_T *device,
	vwl_data_offer_T *offer)
{
    offer->listener.offer = wayland_cb_data_offer_offer;
    offer->data = device->data;

    vwl_data_offer_listen(offer);
}

    static void
wayland_cb_data_offer_offer(vwl_data_offer_T *offer, const char *mime_type)
{
    wayland_cb_choose_offer_func_T callback = offer->data[1];

    callback(offer->data[2], mime_type, &offer->mime_types);
}

    static void
wayland_cb_data_device_selection(vwl_data_device_T *device, wayland_selection_T selection)
{
    wayland_cb_receive_data_func_T callback = device->data[0];
    vwl_data_offer_T *offer = &device->offer;
    int fds[2];

    if (!(device->selection & selection) || offer->mime_types.ga_len == 0)
	goto exit;

    // Receive data for each mime type we chose previously
    for (int i = 0; i < offer->mime_types.ga_len; i++)
    {
	const char *mime_type = ((char **)offer->mime_types.ga_data)[i];
	if (pipe(fds) == -1)
	    continue;

	vwl_data_offer_receive(offer, mime_type, fds[1]);
	close(fds[1]); // Close before we read data so that we actually receive
		       // EOF from read end.

	if (vwl_display_flush(&vwl_display) == OK)
	    callback(device->data[2], mime_type, fds[0], selection);
    }

    close(fds[0]);
exit:
    vwl_data_offer_destroy(&device->offer);

    if (*(int *)device->data[3])
	ga_clear_strings(&device->offer.mime_types);
    else
	ga_clear(&device->offer.mime_types);

    device->offer.proxy = NULL;
}

/*
 *
 */
    static void
wayland_cb_data_device_finished(vwl_data_device_T *device)
{
    if (device->offer.proxy != NULL)
	vwl_data_offer_destroy(&device->offer);
    if (*(int *)device->data[3])
	ga_clear_strings(&device->offer.mime_types);
    else
	ga_clear(&device->offer.mime_types);
}

    int
wayland_cb_own_selection(
	wayland_cb_send_data_func_T send_callback,
	wayland_selection_T selection,
	const char **mime_types,
	int len,
	void *user_data)
{
    vwl_data_device_T *device;
    void *data[] = {};
    int ret;

    if (selection == WAYLAND_SELECTION_PRIMARY)
	device = &vwl_clipboard.primary.source_device;
    if (selection == WAYLAND_SELECTION_REGULAR)
	device = &vwl_clipboard.regular.source_device;

    ret = vwl_data_device_manager_get_data_device(device,
	    &vwl_clipboard.regular.manager, vwl_clipboard.seat,
	    selection);
    if (ret == FAIL)
	return FAIL;

    device->data = data;
    device->listener.data_offer = wayland_cb_data_device_data_offer;
    device->listener.selection = wayland_cb_data_device_selection;
    device->listener.finished = wayland_cb_data_device_finished;

    vwl_data_device_listen(device);

    vwl_display_roundtrip(&vwl_display);

    vwl_data_device_destroy(device);
    vim_memset(device, 0, sizeof(*device));

    vwl_display_flush(&vwl_display);


    return OK;
}

/*
 *
 */
    int
wayland_cb_is_ready(void)
{
    vwl_clipboard_T *c = &vwl_clipboard;

    if (!wayland_client_is_connected())
	return FALSE;

    return !(c->seat->proxy == NULL || c->primary.manager.proxy == NULL ||
           c->regular.manager.proxy == NULL ||
           c->regular.source_device.proxy == NULL ||
           c->primary.source_device.proxy == NULL);
}

#endif // FEAT_WAYLAND_CLIPBOARD

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

    /* vwl_disconnect_client(); */
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
#ifdef FEAT_CLIPBOARD
    choose_clipmethod();
#endif
}

/*
 * Set wayland_display_name to display. Note that this allocate a copy of the
 * string, unless NULL is passed. Note that if NULL is passed then
 * v:wayland_display is set to $WAYLAND_DISPLAY. If only_vvar is TRUE then only
 * change the v:wayland_display variable, and not the internal global variable.
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
