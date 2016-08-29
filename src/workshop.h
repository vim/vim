/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *			Visual Workshop integration by Gordon Prieur
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#if !defined(WORKSHOP_H) && defined(FEAT_SUN_WORKSHOP)
#define WORKSHOP_H

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#include "integration.h"

#ifdef WSDEBUG
# include "wsdebug.h"
#else
# ifndef ASSERT
#  define ASSERT(c)
# endif
#endif

extern int		usingSunWorkShop;	/* set if -ws flag is used */

#endif
