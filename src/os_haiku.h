/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *		 Haiku port by Siarzhuk Zharski
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * os_haiku.h
 */

#define USE_TERM_CONSOLE

#define USR_VIM_DIR "$BE_USER_SETTINGS/vim"

#define USR_EXRC_FILE	USR_VIM_DIR "/exrc"
#define USR_VIMRC_FILE	USR_VIM_DIR "/vimrc"
#define USR_GVIMRC_FILE	USR_VIM_DIR "/gvimrc"
#define VIMINFO_FILE	USR_VIM_DIR "/viminfo"

#ifdef RUNTIME_GLOBAL
# define DFLT_RUNTIMEPATH	USR_VIM_DIR "/vimfiles,"  RUNTIME_GLOBAL \
			",$VIMRUNTIME," RUNTIME_GLOBAL "/after," USR_VIM_DIR "/vimfiles/after"
#else
# define DFLT_RUNTIMEPATH	USR_VIM_DIR "/vimfiles," "$VIM/vimfiles" \
			",$VIMRUNTIME," "$VIM/vimfiles/after,"   USR_VIM_DIR "/vimfiles/after"
#endif
