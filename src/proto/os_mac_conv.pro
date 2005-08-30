extern char_u *mac_string_convert __ARGS((char_u *ptr, int len, int *lenp, int fail_on_error, int from, int to, int *unconvlenp));
extern int macroman2enc __ARGS((char_u *ptr, long *sizep, long real_size));
extern int enc2macroman __ARGS((char_u *from, size_t fromlen, char_u *to, int *tolenp, int maxtolen, char_u *rest, int *restlenp));

extern void	    mac_conv_init __ARGS((void));
extern void	    mac_conv_cleanup __ARGS((void));
extern char_u	    *mac_utf16_to_enc __ARGS((UniChar *from, size_t fromLen, size_t *actualLen));
extern UniChar	    *mac_enc_to_utf16 __ARGS((char_u *from, size_t fromLen, size_t *actualLen));
extern CFStringRef  mac_enc_to_cfstring __ARGS((char_u *from, size_t fromLen));
extern char_u	    *mac_precompose_path __ARGS((char_u *decompPath, size_t decompLen, size_t *precompLen));
