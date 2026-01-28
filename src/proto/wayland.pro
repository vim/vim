/* wayland.c */
int vwl_connection_flush(vwl_connection_T *self);
int vwl_connection_dispatch(vwl_connection_T *self);
int vwl_connection_roundtrip(vwl_connection_T *self);
vwl_seat_T *vwl_connection_get_seat(vwl_connection_T *self, const char *label);
struct wl_keyboard *vwl_seat_get_keyboard(vwl_seat_T *self);
int wayland_init_connection(const char *display);
void wayland_uninit_connection(void);
int wayland_prepare_read(void);
int wayland_update(void);
void wayland_poll_check(int revents);
void wayland_select_check(bool is_set);
void ex_wlrestore(exarg_T *eap);
vwl_data_device_manager_T *vwl_connection_get_data_device_manager(vwl_connection_T *self, wayland_selection_T req_sel, int_u *supported);
vwl_data_device_T *vwl_data_device_manager_get_data_device(vwl_data_device_manager_T *self, vwl_seat_T *seat);
vwl_data_source_T *vwl_data_device_manager_create_data_source(vwl_data_device_manager_T *self);
void vwl_data_device_destroy(vwl_data_device_T *self);
void vwl_data_source_destroy(vwl_data_source_T *self);
void vwl_data_offer_destroy(vwl_data_offer_T *self);
void vwl_data_device_manager_discard(vwl_data_device_manager_T *self);
void vwl_data_device_add_listener(vwl_data_device_T *self, const vwl_data_device_listener_T *listener, void *data);
void vwl_data_source_add_listener(vwl_data_source_T *self, const vwl_data_source_listener_T *listener, void *data);
void vwl_data_offer_add_listener(vwl_data_offer_T *self, const vwl_data_offer_listener_T *listener, void *data);
void vwl_data_device_set_selection(vwl_data_device_T *self, vwl_data_source_T *source, uint32_t serial, wayland_selection_T selection);
void vwl_data_source_offer(vwl_data_source_T *self, const char *mime_type);
void vwl_data_offer_receive(vwl_data_offer_T *self, const char *mime_type, int32_t fd);
/* vim: set ft=c : */
