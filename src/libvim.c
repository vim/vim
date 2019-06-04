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

    win_setwidth(80);
    win_setheight(40);
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
    sm_execute_normal(input);
}

void vimExecute(char_u *cmd) {
    do_cmdline_cmd(cmd);
}

int vimGetMode(void) {
    return State;
}
