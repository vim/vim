/* winclip.c */
int utf8_to_utf16 __ARGS((char_u *instr, int inlen, short_u *outstr, int *unconvlenp));
int utf16_to_utf8 __ARGS((short_u *instr, int inlen, char_u *outstr));
void MultiByteToWideChar_alloc __ARGS((UINT cp, DWORD flags, LPCSTR in, int inlen, LPWSTR *out, int *outlen));
void WideCharToMultiByte_alloc __ARGS((UINT cp, DWORD flags, LPCWSTR in, int inlen, LPSTR *out, int *outlen, LPCSTR def, LPBOOL useddef));
void win_clip_init __ARGS((void));
int clip_mch_own_selection __ARGS((VimClipboard *cbd));
void clip_mch_lose_selection __ARGS((VimClipboard *cbd));
void clip_mch_request_selection __ARGS((VimClipboard *cbd));
void clip_mch_set_selection __ARGS((VimClipboard *cbd));
short_u *enc_to_utf16 __ARGS((char_u *str, int *lenp));
char_u *utf16_to_enc __ARGS((short_u *str, int *lenp));
void acp_to_enc __ARGS((char_u *str, int str_size, char_u **out, int *outlen));
/* vim: set ft=c : */
