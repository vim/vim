/* if_xcmdsrv.c */
int serverRegisterName __ARGS((Display *dpy, char_u *name));
void serverChangeRegisteredWindow __ARGS((Display *dpy, Window newwin));
int serverSendToVim __ARGS((Display *dpy, char_u *name, char_u *cmd, char_u **result, Window *server, int asExpr, int localLoop, int silent));
char_u *serverGetVimNames __ARGS((Display *dpy));
Window serverStrToWin __ARGS((char_u *str));
int serverSendReply __ARGS((char_u *name, char_u *str));
int serverReadReply __ARGS((Display *dpy, Window win, char_u **str, int localLoop));
int serverPeekReply __ARGS((Display *dpy, Window win, char_u **str));
void serverEventProc __ARGS((Display *dpy, XEvent *eventPtr));
/* vim: set ft=c : */
