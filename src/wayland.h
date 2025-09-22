/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * wayland.h: Common definitions for Wayland code
 */


#ifdef FEAT_WAYLAND

#include <wayland-client.h>

#ifdef FEAT_WAYLAND_CLIPBOARD
# include "auto/wayland/wlr-data-control-unstable-v1.h"
# include "auto/wayland/ext-data-control-v1.h"
# ifdef FEAT_WAYLAND_CLIPBOARD_FS
#  include "auto/wayland/xdg-shell.h"
#  include "auto/wayland/primary-selection-unstable-v1.h"
# endif
#endif

#ifdef FEAT_WAYLAND_CLIPBOARD

// Wayland protocols for accessing the selection
typedef enum {
    VWL_DATA_PROTOCOL_NONE,
    VWL_DATA_PROTOCOL_EXT,
    VWL_DATA_PROTOCOL_WLR,
#ifdef FEAT_WAYLAND_CLIPBOARD_FS
    VWL_DATA_PROTOCOL_CORE,
    VWL_DATA_PROTOCOL_PRIMARY
#endif
} vwl_data_protocol_T;

#endif // FEAT_WAYLAND_CLIPBOARD

// Struct that represents a seat. (Should be accessed via
// vwl_get_seat()).
struct vwl_seat_S {
    struct wl_seat  *proxy;
    char	    *label;	    // Name of seat as text (e.g. seat0,
				    // seat1...).
    uint32_t	    capabilities;   // Bitmask of the capabilites of the seat
				    // (pointer, keyboard, touch).
};

// Struct wrapper for a Wayland connection
struct vwl_connection_S {
    struct {
	struct wl_display   *proxy;
	int		    fd;	// File descriptor for display
    } display;

    struct {
	struct wl_registry *proxy;
    } registry;

    // Global objects
    struct {
	garray_T seats;

#ifdef FEAT_WAYLAND_CLIPBOARD
	struct zwlr_data_control_manager_v1 *zwlr_data_control_manager_v1;
	struct ext_data_control_manager_v1  *ext_data_control_manager_v1;
# ifdef FEAT_WAYLAND_CLIPBOARD_FS
	struct wl_data_device_manager	    *wl_data_device_manager;
	struct wl_shm			    *wl_shm;
	struct wl_compositor		    *wl_compositor;
	struct xdg_wm_base		    *xdg_wm_base;
	struct zwp_primary_selection_device_manager_v1
	    *zwp_primary_selection_device_manager_v1;
# endif
#endif
    } gobjects;
};

#ifdef FEAT_WAYLAND_CLIPBOARD

// LISTENER WRAPPERS

struct vwl_data_device_listener_S {
    void (*data_offer)(void *data,
		       vwl_data_device_T *device,
		       vwl_data_offer_T *offer);
    void (*selection)(void *data,
		      vwl_data_device_T *device,
		      vwl_data_offer_T *offer,
		      wayland_selection_T selection);

    // This event is only relevant for data control protocols
    void (*finished)(void *data, vwl_data_device_T *device);
};

struct vwl_data_source_listener_S {
    void (*send)(void *data,
		 vwl_data_source_T *source,
		 const char *mime_type,
		 int fd);
    void (*cancelled)(void *data, vwl_data_source_T *source);
};

struct vwl_data_offer_listener_S {
    // Return TRUE to add mime type to internal array in data offer. Note that
    // this is not called for the special Vim mime type
    // (wayland_vim_special_mime), but offer->from_vim is set to true.
    // Additionally when the special mime type is received, any offer events
    // after are ignored.
    bool (*offer)(void *data, vwl_data_offer_T *offer, const char *mime_type);
};

// DATA RELATED OBJECT WRAPPERS
// These wrap around a proxy and act as a generic container.
// The `data` member is used to pass other needed stuff around such as a
// vwl_clipboard_selection_T pointer.

struct vwl_data_offer_S {
    void			*proxy;
    void			*data;	    // Should be same as parent data
					    // device.
    garray_T			mime_types;
    bool			from_vim;   // If offer came from us setting the
					    // selection.

    const vwl_data_offer_listener_T *listener;
    vwl_data_protocol_T		    protocol;
};

struct vwl_data_source_S {
    void				*proxy;
    void				*data;
    const vwl_data_source_listener_T	*listener;
    vwl_data_protocol_T	    protocol;
};

struct vwl_data_device_S {
    void				*proxy;
    void				*data;
    vwl_data_offer_T			*offer;
    const vwl_data_device_listener_T	*listener;
    vwl_data_protocol_T			protocol;
};

struct vwl_data_device_manager_S {
    void		*proxy;
    vwl_data_protocol_T protocol;
};

#ifdef FEAT_WAYLAND_CLIPBOARD_FS

// Dummy functions to handle keyboard events we don't care about.

#define VWL_FUNCS_DUMMY_KEYBOARD_EVENTS() \
    static void \
clip_wl_fs_keyboard_listener_keymap( \
    void		*data UNUSED, \
    struct wl_keyboard	*keyboard UNUSED, \
    uint32_t		format UNUSED, \
    int			fd, \
    uint32_t		size UNUSED) \
{ \
    close(fd); \
} \
    static void \
clip_wl_fs_keyboard_listener_leave( \
    void		*data UNUSED, \
    struct wl_keyboard	*keyboard UNUSED, \
    uint32_t		serial UNUSED, \
    struct wl_surface	*surface UNUSED) \
{ \
} \
    static void \
clip_wl_fs_keyboard_listener_key( \
    void		*data UNUSED, \
    struct wl_keyboard	*keyboard UNUSED, \
    uint32_t		serial UNUSED, \
    uint32_t		time UNUSED, \
    uint32_t		key UNUSED, \
    uint32_t		state UNUSED) \
{ \
} \
    static void \
clip_wl_fs_keyboard_listener_modifiers( \
    void		*data UNUSED, \
    struct wl_keyboard	*keyboard UNUSED, \
    uint32_t		serial UNUSED, \
    uint32_t		mods_depressed UNUSED, \
    uint32_t		mods_latched UNUSED, \
    uint32_t		mods_locked UNUSED, \
    uint32_t		group UNUSED) \
{ \
} \
    static void \
clip_wl_fs_keyboard_listener_repeat_info( \
    void		*data UNUSED, \
    struct wl_keyboard	*keyboard UNUSED, \
    int32_t		rate UNUSED, \
    int32_t		delay UNUSED) \
{ \
}

#endif

#endif // FEAT_WAYLAND_CLIPBOARD

// Global Wayland connection. Is also set to NULL when the connection is lost.
extern vwl_connection_T *wayland_ct;

#endif // FEAT_WAYLAND
