/* if_xcmdsrv.c */
extern int serverRegisterName __ARGS((Display *dpy, char_u *name));
extern void serverChangeRegisteredWindow __ARGS((Display *dpy, Window newwin));
extern int serverSendToVim __ARGS((Display *dpy, char_u *name, char_u *cmd, char_u **result, Window *server, int asExpr, int localLoop, int silent));
extern char_u *serverGetVimNames __ARGS((Display *dpy));
extern Window serverStrToWin __ARGS((char_u *str));
extern int serverSendReply __ARGS((char_u *name, char_u *str));
extern int serverReadReply __ARGS((Display *dpy, Window win, char_u **str, int localLoop));
extern int serverPeekReply __ARGS((Display *dpy, Window win, char_u **str));
extern void serverEventProc __ARGS((Display *dpy, XEvent *eventPtr));
/* vim: set ft=c : */
