/* vi:set ts=8 sts=4 sw=4:
 *
 * MzScheme interface for Vim, wrapper around scheme.h
 */
#ifndef _IF_MZSCH_H_
#define _IF_MZSCH_H_
#ifdef __MINGW32__
/* Hack to engage Cygwin-specific settings */
# define __CYGWIN32__
#endif

/* #ifdef needed for "make depend" */
#ifdef FEAT_MZSCHEME
# include <scheme.h>
#endif

#ifdef __MINGW32__
# undef __CYGWIN32__
#endif

#if MZSCHEME_VERSION_MAJOR >= 299
/* macros to be compatible with 20x versions */
# define scheme_config scheme_current_config()
# define scheme_make_string scheme_make_byte_string
# define scheme_make_string_output_port scheme_make_byte_string_output_port
# define scheme_get_sized_string_output scheme_get_sized_byte_string_output
# define scheme_write_string scheme_write_byte_string
# define scheme_make_sized_string scheme_make_sized_byte_string

# define SCHEME_STRINGP(obj) (SCHEME_BYTE_STRINGP(obj) || SCHEME_CHAR_STRINGP(obj))
# define SCHEME_STR_VAL(obj) SCHEME_BYTE_STR_VAL(   \
		(SCHEME_BYTE_STRINGP(obj) ? obj : scheme_char_string_to_byte_string(obj)))
# define GUARANTEE_STRING(fname, argnum) GUARANTEE_TYPE(fname, argnum, SCHEME_STRINGP, "string")

# ifdef scheme_format
#  undef scheme_format
# endif
# define scheme_format scheme_format_utf8

# define SCHEME_GET_BYTE_STRING(obj) (SCHEME_BYTE_STRINGP(obj) ? obj :   \
	scheme_char_string_to_byte_string(obj))
#else
# define SCHEME_GET_BYTE_STRING(obj) (obj)
# define SCHEME_BYTE_STRLEN_VAL SCHEME_STRLEN_VAL
# define SCHEME_BYTE_STR_VAL SCHEME_STR_VAL
# define scheme_byte_string_to_char_string(obj) (obj)
#endif

#endif /* _IF_MZSCH_H_ */
