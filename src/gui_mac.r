/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read a list of people who contributed.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#ifdef environ_os_mac /* or 1 for Carbon, 0 for non-Carbon */
# include <Carbon/Carbon.r>
#else
# include <SysTypes.r>
# include <Types.r>
#endif
#include "version.h"

/* Used as application version */
resource 'vers' (1) {
	VIM_VERSION_MAJOR, VIM_VERSION_BUILD_BCD, VIM_VERSION_RELEASE, VIM_VERSION_PATCHLEVEL,
	verUS,
	VIM_VERSION_MEDIUM,
	VIM_VERSION_LONG_DATE $$date " " $$time ")"
};

/* Used as application group version */
resource 'vers' (2) {
	VIM_VERSION_MAJOR, VIM_VERSION_BUILD_BCD, VIM_VERSION_RELEASE, VIM_VERSION_PATCHLEVEL,
	verUS,
	VIM_VERSION_MEDIUM,
	VIM_VERSION_LONG
};

/* TODO: Small About box with compile time */
