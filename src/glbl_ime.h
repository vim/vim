/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#ifdef GLOBAL_IME
#ifndef _INC_GLOBAL_IME
#define _INC_GLOBAL_IME

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    void global_ime_init(ATOM, HWND);
    void global_ime_end();
    LRESULT WINAPI global_ime_DefWindowProc(HWND, UINT, WPARAM, LPARAM);
    BOOL WINAPI global_ime_TranslateMessage(CONST MSG *);
    void WINAPI global_ime_set_position(POINT*);
    void WINAPI global_ime_set_font(LOGFONT*);
    void WINAPI global_ime_status_evacuate();
    void WINAPI global_ime_status_restore();
    void WINAPI global_ime_set_status(int status);
    int WINAPI global_ime_get_status();
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _INC_GLOBAL_IME */
#endif /* GLOBAL_IME */
