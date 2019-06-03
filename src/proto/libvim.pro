/* libvim.c */

void vimInit(int argc, char **argv);

buf_T *vimBufferOpen(char_u *ffname_arg, linenr_T lnum, int flags);

char_u *vimBufferGetLine(buf_T* buf, linenr_T lnum);

void vimInput(char_u *input);

void vimExecute(char_u *cmd);

linenr_T vimWindowGetCursorLine(void);

/* vim: set ft=c : */
