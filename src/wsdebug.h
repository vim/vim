/* vi:set ts=8 sts=8 sw=8:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *			Visual Workshop integration by Gordon Prieur
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */


#ifndef WSDEBUG_H
#define WSDEBUG_H

#ifdef WSDEBUG

#ifndef ASSERT
#define ASSERT(c) \
    if (!(c)) { \
	fprintf(stderr, "Assertion failed: line %d, file %s\n", \
		__LINE__, __FILE__); \
	fflush(stderr); \
	abort(); \
    }
#endif

#define WS_TRACE		0x00000001
#define WS_TRACE_VERBOSE	0x00000002
#define WS_TRACE_COLONCMD	0x00000004
#define WS_DEBUG_ALL		0xffffffff

#define WSDLEVEL(flags)		(ws_debug != NULL && (ws_dlevel & (flags)))

#ifdef USE_WDDUMP
#include "wdump.h"
#endif

#define WSDEBUG_TRACE	1
//#define WSDEBUG_SENSE	2

typedef enum {
		WT_ENV = 1,		/* look for env var if set */
		WT_WAIT,		/* look for ~/.gvimwait if set */
		WT_STOP			/* look for ~/.gvimstop if set */
} WtWait;


void		 wsdebug(char *, ...);
void		 wstrace(char *, ...);


extern FILE	*ws_debug;
extern u_int	 ws_dlevel;		/* ws_debug verbosity level */

# else		/* not WSDEBUG */

#ifndef ASSERT
# define ASSERT(c)
#endif

/*
 * The following 2 stubs are needed because a macro cannot be used because of
 * the variable number of arguments.
 */

void
wsdebug(
	char		*fmt,
	...)
{
}


void
wstrace(
	char		*fmt,
	...)
{
}

#endif /* WSDEBUG */
#endif /* WSDEBUG_H */
