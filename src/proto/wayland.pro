/* wayland.c */
int wayland_init_client(const char *display);
void wayland_uninit_client(void);
int wayland_client_is_connected(void);
int wayland_client_update(void);
int wayland_cb_init(const char *seat);
void wayland_cb_uninit(void);
int wayland_cb_receive_selection( wayland_cb_receive_data_func_T receive_callback, wayland_cb_choose_offer_func_T offer_callback, wayland_selection_T selection, void *user_data, int free_strings);
int wayland_cb_is_ready(void);
void ex_wlrestore(exarg_T *eap);
/* vim: set ft=c : */
