/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *		 BeBox port by Olaf Seibert
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * os_beos.h
 */

#undef USE_SYSTEM
#define USE_THREAD_FOR_INPUT_WITH_TIMEOUT	1
#define USE_TERM_CONSOLE

#define HAVE_DROP_FILE

#undef	BEOS_DR8
#define	BEOS_PR_OR_BETTER

/* select emulation */

#include <net/socket.h>		/* for typedefs and #defines only */
