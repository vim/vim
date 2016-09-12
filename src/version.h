/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * Define the version number, name, etc.
 * The patchlevel is in included_patches[], in version.c.
 *
 * This doesn't use string concatenation, some compilers don't support it.
 */

#define VIM_VERSION_MAJOR		 8
#define VIM_VERSION_MAJOR_STR		"8"
#define VIM_VERSION_MINOR		 0
#define VIM_VERSION_MINOR_STR		"0"
#define VIM_VERSION_100	    (VIM_VERSION_MAJOR * 100 + VIM_VERSION_MINOR)

#define VIM_VERSION_BUILD		 281
#define VIM_VERSION_BUILD_BCD		0x119
#define VIM_VERSION_BUILD_STR		"281"
#define VIM_VERSION_PATCHLEVEL		 0
#define VIM_VERSION_PATCHLEVEL_STR	"0"
/* Used by MacOS port should be one of: development, alpha, beta, final */
#define VIM_VERSION_RELEASE		final

/*
 * VIM_VERSION_NODOT is used for the runtime directory name.
 * VIM_VERSION_SHORT is copied into the swap file (max. length is 6 chars).
 * VIM_VERSION_MEDIUM is used for the startup-screen.
 * VIM_VERSION_LONG is used for the ":version" command and "Vim -h".
 */
#define VIM_VERSION_NODOT	"vim80"
#define VIM_VERSION_SHORT	"8.0"
#define VIM_VERSION_MEDIUM	"8.0"
#define VIM_VERSION_LONG	"VIM - Vi IMproved 8.0 (2016 Sep 12)"
#define VIM_VERSION_LONG_DATE	"VIM - Vi IMproved 8.0 (2016 Sep 12, compiled "
