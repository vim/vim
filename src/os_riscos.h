/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stdlib.h>
#include <unixlib/local.h>
#include <errno.h>
#include <fcntl.h>

#define CASE_INSENSITIVE_FILENAME
#define FEAT_MODIFY_FNAME
#define FEAT_OSFILETYPE
#define DFLT_OFT	"Text"
#define USE_TERM_CONSOLE
#define HAVE_AVAIL_MEM

/* Longer filenames now accessible to all */
#ifndef BASENAMELEN
# define BASENAMELEN 64 /* Same length as unzip */
#endif

#ifndef TEMNAME
# define TEMPNAME	"<Wimp$ScrapDir>.v?XXXXXX"
# define TEMPNAMELEN	25
#endif

#ifndef DFLT_HELPFILE
# define DFLT_HELPFILE "Vim:doc.help"
#endif

#ifndef DFLT_BDIR
# define DFLT_BDIR	".,<Wimp$ScrapDir>."	/* default for 'backupdir' */
#endif

/* Paths to try putting swap file in. */
#ifndef DFLT_DIR
# define DFLT_DIR	"<Wimp$ScrapDir>.,."	/* default for 'directory' */
#endif

#ifndef DFLT_VDIR
# define DFLT_VDIR	"Choices:Vim.view"	/* default for 'viewdir' */
#endif

#ifndef TERMCAPFILE
# define TERMCAPFILE	"Vim:TermCap"
#endif
#define HAVE_TGETENT

#ifndef SYNTAX_FNAME
# define SYNTAX_FNAME	"Vim:Syntax.%s"
#endif

#ifndef EVIM_FILE
# define EVIM_FILE	"Vim:Evim"
#endif

#define FEAT_VIMINFO

#ifndef VIMINFO_FILE
# define VIMINFO_FILE	"<Choices$Write>.Vim.VimInfo"
#endif
#ifndef VIMINFO_FILE2
# define VIMINFO_FILE2	"Choices:Vim.VimInfo"
#endif

#ifndef VIMRC_FILE
# define VIMRC_FILE	"/vimrc"
#endif
#ifndef EXRC_FILE
# define EXRC_FILE	"/exrc"
#endif
#ifndef GVIMRC_FILE
# define GVIMRC_FILE	"/gvimrc"
#endif
#ifndef USR_VIMRC_FILE
# define USR_VIMRC_FILE	"Vim:Evim"
#endif
#ifndef SESSION_FILE
# define SESSION_FILE	"/Session.vim"
#endif
#ifndef USR_VIMRC_FILE
# define USR_VIMRC_FILE	"Choices:Vim.VimRC"
#endif
#ifndef USR_GVIMRC_FILE
# define USR_GVIMRC_FILE    "Choices:Vim.GVimRC"
#endif
#ifndef USR_EXRC_FILE
# define USR_EXRC_FILE    "Choices:Vim.ExRC"
#endif
#ifndef SYS_VIMRC_FILE
# define SYS_VIMRC_FILE	    "Vim:VimRC"
#endif
#ifndef SYS_GVIMRC_FILE
# define SYS_GVIMRC_FILE    "Vim:GVimRC"
#endif
#ifndef SYS_MENU_FILE
# define SYS_MENU_FILE	    "Vim:Menu"
#endif
#ifndef SYS_OPTWIN_FILE
# define SYS_OPTWIN_FILE    "Vim:Optwin"
#endif
#ifndef FILETYPE_FILE
# define FILETYPE_FILE	    "Vim:Filetype"
#endif
#ifndef FTPLUGIN_FILE
# define FTPLUGIN_FILE	    "Vim:Ftplugin/vim"
#endif
#ifndef INDENT_FILE
# define INDENT_FILE	    "Vim:Indent/vim"
#endif
#ifndef FTOFF_FILE
# define FTOFF_FILE	    "Vim:Ftoff"
#endif
#ifndef FTPLUGOF_FILE
# define FTPLUGOF_FILE	    "Vim:Ftplugof"
#endif
#ifndef INDOFF_FILE
# define INDOFF_FILE	    "Vim:Indoff"
#endif

#define DFLT_ERRORFILE		"errors/vim"
#define DFLT_RUNTIMEPATH	"Choices:Vim,Vim:,Choices:Vim.after"

/*
 * RISC PCs have plenty of memory, use large buffers
 */
#define CMDBUFFSIZE 1024	/* size of the command processing buffer */
#define MAXPATHL    256		/* paths are always quite short though */

#ifndef DFLT_MAXMEM
# define DFLT_MAXMEM	(5*1024)    /* use up to 5 Mbyte for a buffer */
#endif

#ifndef DFLT_MAXMEMTOT
# define DFLT_MAXMEMTOT	(10*1024)    /* use up to 10 Mbyte for Vim */
#endif

#ifdef HAVE_SIGSET
# define signal sigset
#endif

#define n_flag (1<<31)
#define z_flag (1<<30)
#define c_flag (1<<29)
#define v_flag (1<<28)

/* These take r0-r7 as inputs, returns r0-r7 in global variables. */
void swi(int swinum, ...);      /* Handles errors itself */
int xswi(int swinum, ...);      /* Returns errors using v flag */
extern int r0, r1, r2, r3, r4, r5, r6, r7;  /* For return values */

#include <kernel.h>
#include <swis.h>

#define mch_memmove(to, from, len) memmove((char *)(to), (char *)(from), len)
#define mch_rename(src, dst) rename(src, dst)
#define mch_getenv(x) (char_u *)getenv((char *)x)
#define mch_setenv(name, val, x) setenv(name, val, x)
