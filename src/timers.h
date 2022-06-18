/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#ifndef TIMERS__H
# define TIMERS__H

#include "vim.h"

/*
 * time.h: functions related to time and timers
 */

#if defined(FEAT_RELTIME)
EXTERN long timeout_level;
EXTERN int  *timeout_flag;

// Check for whether a timeout has occurred.
    static inline int
timeout_occurred()
{
    if (timeout_level == 0)
	return FALSE;
    else
	return *timeout_flag;
}
#else
    static inline int
timeout_occurred()
{
    return FALSE;
}
#endif

#endif // TIMERS__H
