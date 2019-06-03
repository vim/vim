/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * libvim
 */

/*
 * Top-level API for libvim.
 *
 * This provides the API surface for consumers of `libvim`.
 */

#include "vim.h"

void vimInit(int argc, char** argv) {
    mparm_T params;
    vim_memset(&params, 0, sizeof(params));
    params.argc = argc;
    params.argv = argv;
    params.want_full_screen = TRUE;
    params.window_count = -1;

    mch_early_init();
    common_init(&params);
    init_normal_cmds();
}

buf_T *vimBufferOpen(char_u *ffname_arg, linenr_T lnum, int flags) {
    buf_T* buffer = buflist_new(ffname_arg, NULL, lnum, flags);
    set_curbuf(buffer, 0);
    return buffer;
}

char_u *vimBufferGetLine(buf_T *buf, linenr_T lnum) {
   char_u *result = ml_get_buf(buf, lnum, FALSE); 
   return result;
}

linenr_T vimWindowGetCursorLine(void) {
    return curwin->w_cursor.lnum;
};

void vimInput(char_u *input) {
    char_u *keys_esc = vim_strsave_escape_csi(input);
    ins_typebuf(keys_esc, REMAP_YES, 0, FALSE, FALSE);
    exec_normal(TRUE, FALSE, FALSE);
}
