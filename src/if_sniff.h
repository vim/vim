/*
 * if_sniff.h Interface between Vim and SNiFF+
 */

#ifndef __if_sniff_h__
#define __if_sniff_h__

extern int  want_sniff_request;
extern int  sniff_request_waiting;
extern int  sniff_connected;
extern int  fd_from_sniff;
extern void sniff_disconnect __ARGS((int immediately));
extern void ProcessSniffRequests __ARGS((void));
extern void ex_sniff __ARGS((exarg_T *eap));

#endif
