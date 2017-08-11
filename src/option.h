/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * option.h: definition of global variables for settable options
 */

/*
 * Default values for 'errorformat'.
 * The "%f|%l| %m" one is used for when the contents of the quickfix window is
 * written to a file.
 */
#ifdef AMIGA
# define DFLT_EFM	"%f>%l:%c:%t:%n:%m,%f:%l: %t%*\\D%n: %m,%f %l %t%*\\D%n: %m,%*[^\"]\"%f\"%*\\D%l: %m,%f:%l:%m,%f|%l| %m"
#else
# if defined(WIN3264)
#  define DFLT_EFM	"%f(%l) : %t%*\\D%n: %m,%*[^\"]\"%f\"%*\\D%l: %m,%f(%l) : %m,%*[^ ] %f %l: %m,%f:%l:%c:%m,%f(%l):%m,%f:%l:%m,%f|%l| %m"
# else
#  if defined(__QNX__)
#   define DFLT_EFM	"%f(%l):%*[^WE]%t%*\\D%n:%m,%f|%l| %m"
#  else
#   ifdef VMS
#    define DFLT_EFM	"%A%p^,%C%%CC-%t-%m,%Cat line number %l in file %f,%f|%l| %m"
#   else /* Unix, probably */
#    ifdef EBCDIC
#define DFLT_EFM	"%*[^ ] %*[^ ] %f:%l%*[ ]%m,%*[^\"]\"%f\"%*\\D%l: %m,\"%f\"%*\\D%l: %m,%f:%l:%c:%m,%f(%l):%m,%f:%l:%m,\"%f\"\\, line %l%*\\D%c%*[^ ] %m,%D%*\\a[%*\\d]: Entering directory %*[`']%f',%X%*\\a[%*\\d]: Leaving directory %*[`']%f',%DMaking %*\\a in %f,%f|%l| %m"
#     else
#define DFLT_EFM	"%*[^\"]\"%f\"%*\\D%l: %m,\"%f\"%*\\D%l: %m,%-G%f:%l: (Each undeclared identifier is reported only once,%-G%f:%l: for each function it appears in.),%-GIn file included from %f:%l:%c:,%-GIn file included from %f:%l:%c\\,,%-GIn file included from %f:%l:%c,%-GIn file included from %f:%l,%-G%*[ ]from %f:%l:%c,%-G%*[ ]from %f:%l:,%-G%*[ ]from %f:%l\\,,%-G%*[ ]from %f:%l,%f:%l:%c:%m,%f(%l):%m,%f:%l:%m,\"%f\"\\, line %l%*\\D%c%*[^ ] %m,%D%*\\a[%*\\d]: Entering directory %*[`']%f',%X%*\\a[%*\\d]: Leaving directory %*[`']%f',%D%*\\a: Entering directory %*[`']%f',%X%*\\a: Leaving directory %*[`']%f',%DMaking %*\\a in %f,%f|%l| %m"
#    endif
#   endif
#  endif
# endif
#endif

#define DFLT_GREPFORMAT	"%f:%l:%m,%f:%l%m,%f  %l%m"

/* default values for b_p_ff 'fileformat' and p_ffs 'fileformats' */
#define FF_DOS		"dos"
#define FF_MAC		"mac"
#define FF_UNIX		"unix"

#ifdef USE_CRNL
# define DFLT_FF	"dos"
# define DFLT_FFS_VIM	"dos,unix"
# define DFLT_FFS_VI	"dos,unix"	/* also autodetect in compatible mode */
# define DFLT_TEXTAUTO	TRUE
#else
# ifdef USE_CR
#  define DFLT_FF	"mac"
#  define DFLT_FFS_VIM	"mac,unix,dos"
#  define DFLT_FFS_VI	"mac,unix,dos"
#  define DFLT_TEXTAUTO	TRUE
# else
#  define DFLT_FF	"unix"
#  define DFLT_FFS_VIM	"unix,dos"
#  ifdef __CYGWIN__
#   define DFLT_FFS_VI	"unix,dos"	/* Cygwin always needs file detection */
#   define DFLT_TEXTAUTO TRUE
#  else
#   define DFLT_FFS_VI	""
#   define DFLT_TEXTAUTO FALSE
#  endif
# endif
#endif


#ifdef FEAT_MBYTE
/* Possible values for 'encoding' */
# define ENC_UCSBOM	"ucs-bom"	/* check for BOM at start of file */

/* default value for 'encoding' */
# define ENC_DFLT	"latin1"
#endif

/* end-of-line style */
#define EOL_UNKNOWN	-1	/* not defined yet */
#define EOL_UNIX	0	/* NL */
#define EOL_DOS		1	/* CR NL */
#define EOL_MAC		2	/* CR */

/* Formatting options for p_fo 'formatoptions' */
#define FO_WRAP		't'
#define FO_WRAP_COMS	'c'
#define FO_RET_COMS	'r'
#define FO_OPEN_COMS	'o'
#define FO_Q_COMS	'q'
#define FO_Q_NUMBER	'n'
#define FO_Q_SECOND	'2'
#define FO_INS_VI	'v'
#define FO_INS_LONG	'l'
#define FO_INS_BLANK	'b'
#define FO_MBYTE_BREAK	'm'	/* break before/after multi-byte char */
#define FO_MBYTE_JOIN	'M'	/* no space before/after multi-byte char */
#define FO_MBYTE_JOIN2	'B'	/* no space between multi-byte chars */
#define FO_ONE_LETTER	'1'
#define FO_WHITE_PAR	'w'	/* trailing white space continues paragr. */
#define FO_AUTO		'a'	/* automatic formatting */
#define FO_REMOVE_COMS	'j'	/* remove comment leaders when joining lines */

#define DFLT_FO_VI	"vt"
#define DFLT_FO_VIM	"tcq"
#define FO_ALL		"tcroq2vlb1mMBn,awj"	/* for do_set() */

/* characters for the p_cpo option: */
#define CPO_ALTREAD	'a'	/* ":read" sets alternate file name */
#define CPO_ALTWRITE	'A'	/* ":write" sets alternate file name */
#define CPO_BAR		'b'	/* "\|" ends a mapping */
#define CPO_BSLASH	'B'	/* backslash in mapping is not special */
#define CPO_SEARCH	'c'
#define CPO_CONCAT	'C'	/* Don't concatenate sourced lines */
#define CPO_DOTTAG	'd'	/* "./tags" in 'tags' is in current dir */
#define CPO_DIGRAPH	'D'	/* No digraph after "r", "f", etc. */
#define CPO_EXECBUF	'e'
#define CPO_EMPTYREGION	'E'	/* operating on empty region is an error */
#define CPO_FNAMER	'f'	/* set file name for ":r file" */
#define CPO_FNAMEW	'F'	/* set file name for ":w file" */
#define CPO_GOTO1	'g'	/* goto line 1 for ":edit" */
#define CPO_INSEND	'H'	/* "I" inserts before last blank in line */
#define CPO_INTMOD	'i'	/* interrupt a read makes buffer modified */
#define CPO_INDENT	'I'	/* remove auto-indent more often */
#define CPO_JOINSP	'j'	/* only use two spaces for join after '.' */
#define CPO_ENDOFSENT	'J'	/* need two spaces to detect end of sentence */
#define CPO_KEYCODE	'k'	/* don't recognize raw key code in mappings */
#define CPO_KOFFSET	'K'	/* don't wait for key code in mappings */
#define CPO_LITERAL	'l'	/* take char after backslash in [] literal */
#define CPO_LISTWM	'L'	/* 'list' changes wrapmargin */
#define CPO_SHOWMATCH	'm'
#define CPO_MATCHBSL	'M'	/* "%" ignores use of backslashes */
#define CPO_NUMCOL	'n'	/* 'number' column also used for text */
#define CPO_LINEOFF	'o'
#define CPO_OVERNEW	'O'	/* silently overwrite new file */
#define CPO_LISP	'p'	/* 'lisp' indenting */
#define CPO_FNAMEAPP	'P'	/* set file name for ":w >>file" */
#define CPO_JOINCOL	'q'	/* with "3J" use column after first join */
#define CPO_REDO	'r'
#define CPO_REMMARK	'R'	/* remove marks when filtering */
#define CPO_BUFOPT	's'
#define CPO_BUFOPTGLOB	'S'
#define CPO_TAGPAT	't'
#define CPO_UNDO	'u'	/* "u" undoes itself */
#define CPO_BACKSPACE	'v'	/* "v" keep deleted text */
#define CPO_CW		'w'	/* "cw" only changes one blank */
#define CPO_FWRITE	'W'	/* "w!" doesn't overwrite readonly files */
#define CPO_ESC		'x'
#define CPO_REPLCNT	'X'	/* "R" with a count only deletes chars once */
#define CPO_YANK	'y'
#define CPO_KEEPRO	'Z'	/* don't reset 'readonly' on ":w!" */
#define CPO_DOLLAR	'$'
#define CPO_FILTER	'!'
#define CPO_MATCH	'%'
#define CPO_STAR	'*'	/* ":*" means ":@" */
#define CPO_PLUS	'+'	/* ":write file" resets 'modified' */
#define CPO_MINUS	'-'	/* "9-" fails at and before line 9 */
#define CPO_SPECI	'<'	/* don't recognize <> in mappings */
#define CPO_REGAPPEND	'>'	/* insert NL when appending to a register */
/* POSIX flags */
#define CPO_HASH	'#'	/* "D", "o" and "O" do not use a count */
#define CPO_PARA	'{'	/* "{" is also a paragraph boundary */
#define CPO_TSIZE	'|'	/* $LINES and $COLUMNS overrule term size */
#define CPO_PRESERVE	'&'	/* keep swap file after :preserve */
#define CPO_SUBPERCENT	'/'	/* % in :s string uses previous one */
#define CPO_BACKSL	'\\'	/* \ is not special in [] */
#define CPO_CHDIR	'.'	/* don't chdir if buffer is modified */
#define CPO_SCOLON	';'	/* using "," and ";" will skip over char if
				 * cursor would not move */
/* default values for Vim, Vi and POSIX */
#define CPO_VIM		"aABceFs"
#define CPO_VI		"aAbBcCdDeEfFgHiIjJkKlLmMnoOpPqrRsStuvwWxXyZ$!%*-+<>;"
#define CPO_ALL		"aAbBcCdDeEfFgHiIjJkKlLmMnoOpPqrRsStuvwWxXyZ$!%*-+<>#{|&/\\.;"

/* characters for p_ww option: */
#define WW_ALL		"bshl<>[],~"

/* characters for p_mouse option: */
#define MOUSE_NORMAL	'n'		/* use mouse in Normal mode */
#define MOUSE_VISUAL	'v'		/* use mouse in Visual/Select mode */
#define MOUSE_INSERT	'i'		/* use mouse in Insert mode */
#define MOUSE_COMMAND	'c'		/* use mouse in Command-line mode */
#define MOUSE_HELP	'h'		/* use mouse in help buffers */
#define MOUSE_RETURN	'r'		/* use mouse for hit-return message */
#define MOUSE_A		"nvich"		/* used for 'a' flag */
#define MOUSE_ALL	"anvichr"	/* all possible characters */
#define MOUSE_NONE	' '		/* don't use Visual selection */
#define MOUSE_NONEF	'x'		/* forced modeless selection */

#define COCU_ALL	"nvic"		/* flags for 'concealcursor' */

/* characters for p_shm option: */
#define SHM_RO		'r'		/* readonly */
#define SHM_MOD		'm'		/* modified */
#define SHM_FILE	'f'		/* (file 1 of 2) */
#define SHM_LAST	'i'		/* last line incomplete */
#define SHM_TEXT	'x'		/* tx instead of textmode */
#define SHM_LINES	'l'		/* "L" instead of "lines" */
#define SHM_NEW		'n'		/* "[New]" instead of "[New file]" */
#define SHM_WRI		'w'		/* "[w]" instead of "written" */
#define SHM_A		"rmfixlnw"	/* represented by 'a' flag */
#define SHM_WRITE	'W'		/* don't use "written" at all */
#define SHM_TRUNC	't'		/* trunctate file messages */
#define SHM_TRUNCALL	'T'		/* trunctate all messages */
#define SHM_OVER	'o'		/* overwrite file messages */
#define SHM_OVERALL	'O'		/* overwrite more messages */
#define SHM_SEARCH	's'		/* no search hit bottom messages */
#define SHM_ATTENTION	'A'		/* no ATTENTION messages */
#define SHM_INTRO	'I'		/* intro messages */
#define SHM_COMPLETIONMENU  'c'		/* completion menu messages */
#define SHM_RECORDING	'q'		/* short recording message */
#define SHM_FILEINFO	'F'		/* no file info messages */
#define SHM_ALL		"rmfixlnwaWtToOsAIcqF" /* all possible flags for 'shm' */

/* characters for p_go: */
#define GO_ASEL		'a'		/* autoselect */
#define GO_ASELML	'A'		/* autoselect modeless selection */
#define GO_BOT		'b'		/* use bottom scrollbar */
#define GO_CONDIALOG	'c'		/* use console dialog */
#define GO_TABLINE	'e'		/* may show tabline */
#define GO_FORG		'f'		/* start GUI in foreground */
#define GO_GREY		'g'		/* use grey menu items */
#define GO_HORSCROLL	'h'		/* flexible horizontal scrolling */
#define GO_ICON		'i'		/* use Vim icon */
#define GO_LEFT		'l'		/* use left scrollbar */
#define GO_VLEFT	'L'		/* left scrollbar with vert split */
#define GO_MENUS	'm'		/* use menu bar */
#define GO_NOSYSMENU	'M'		/* don't source system menu */
#define GO_POINTER	'p'		/* pointer enter/leave callbacks */
#define GO_ASELPLUS	'P'		/* autoselectPlus */
#define GO_RIGHT	'r'		/* use right scrollbar */
#define GO_VRIGHT	'R'		/* right scrollbar with vert split */
#define GO_TEAROFF	't'		/* add tear-off menu items */
#define GO_TOOLBAR	'T'		/* add toolbar */
#define GO_FOOTER	'F'		/* add footer */
#define GO_VERTICAL	'v'		/* arrange dialog buttons vertically */
#define GO_ALL		"aAbcefFghilmMprtTv" /* all possible flags for 'go' */

/* flags for 'comments' option */
#define COM_NEST	'n'		/* comments strings nest */
#define COM_BLANK	'b'		/* needs blank after string */
#define COM_START	's'		/* start of comment */
#define COM_MIDDLE	'm'		/* middle of comment */
#define COM_END		'e'		/* end of comment */
#define COM_AUTO_END	'x'		/* last char of end closes comment */
#define COM_FIRST	'f'		/* first line comment only */
#define COM_LEFT	'l'		/* left adjusted */
#define COM_RIGHT	'r'		/* right adjusted */
#define COM_NOBACK	'O'		/* don't use for "O" command */
#define COM_ALL		"nbsmexflrO"	/* all flags for 'comments' option */
#define COM_MAX_LEN	50		/* maximum length of a part */

/* flags for 'statusline' option */
#define STL_FILEPATH	'f'		/* path of file in buffer */
#define STL_FULLPATH	'F'		/* full path of file in buffer */
#define STL_FILENAME	't'		/* last part (tail) of file path */
#define STL_COLUMN	'c'		/* column og cursor*/
#define STL_VIRTCOL	'v'		/* virtual column */
#define STL_VIRTCOL_ALT	'V'		/* - with 'if different' display */
#define STL_LINE	'l'		/* line number of cursor */
#define STL_NUMLINES	'L'		/* number of lines in buffer */
#define STL_BUFNO	'n'		/* current buffer number */
#define STL_KEYMAP	'k'		/* 'keymap' when active */
#define STL_OFFSET	'o'		/* offset of character under cursor*/
#define STL_OFFSET_X	'O'		/* - in hexadecimal */
#define STL_BYTEVAL	'b'		/* byte value of character */
#define STL_BYTEVAL_X	'B'		/* - in hexadecimal */
#define STL_ROFLAG	'r'		/* readonly flag */
#define STL_ROFLAG_ALT	'R'		/* - other display */
#define STL_HELPFLAG	'h'		/* window is showing a help file */
#define STL_HELPFLAG_ALT 'H'		/* - other display */
#define STL_FILETYPE	'y'		/* 'filetype' */
#define STL_FILETYPE_ALT 'Y'		/* - other display */
#define STL_PREVIEWFLAG	'w'		/* window is showing the preview buf */
#define STL_PREVIEWFLAG_ALT 'W'		/* - other display */
#define STL_MODIFIED	'm'		/* modified flag */
#define STL_MODIFIED_ALT 'M'		/* - other display */
#define STL_QUICKFIX	'q'		/* quickfix window description */
#define STL_PERCENTAGE	'p'		/* percentage through file */
#define STL_ALTPERCENT	'P'		/* percentage as TOP BOT ALL or NN% */
#define STL_ARGLISTSTAT	'a'		/* argument list status as (x of y) */
#define STL_PAGENUM	'N'		/* page number (when printing)*/
#define STL_VIM_EXPR	'{'		/* start of expression to substitute */
#define STL_MIDDLEMARK	'='		/* separation between left and right */
#define STL_TRUNCMARK	'<'		/* truncation mark if line is too long*/
#define STL_USER_HL	'*'		/* highlight from (User)1..9 or 0 */
#define STL_HIGHLIGHT	'#'		/* highlight name */
#define STL_TABPAGENR	'T'		/* tab page label nr */
#define STL_TABCLOSENR	'X'		/* tab page close nr */
#define STL_ALL		((char_u *) "fFtcvVlLknoObBrRhHmYyWwMqpPaN{#")

/* flags used for parsed 'wildmode' */
#define WIM_FULL	1
#define WIM_LONGEST	2
#define WIM_LIST	4

/* arguments for can_bs() */
#define BS_INDENT	'i'	/* "Indent" */
#define BS_EOL		'o'	/* "eOl" */
#define BS_START	's'	/* "Start" */

#define LISPWORD_VALUE	"defun,define,defmacro,set!,lambda,if,case,let,flet,let*,letrec,do,do*,define-syntax,let-syntax,letrec-syntax,destructuring-bind,defpackage,defparameter,defstruct,deftype,defvar,do-all-symbols,do-external-symbols,do-symbols,dolist,dotimes,ecase,etypecase,eval-when,labels,macrolet,multiple-value-bind,multiple-value-call,multiple-value-prog1,multiple-value-setq,prog1,progv,typecase,unless,unwind-protect,when,with-input-from-string,with-open-file,with-open-stream,with-output-to-string,with-package-iterator,define-condition,handler-bind,handler-case,restart-bind,restart-case,with-simple-restart,store-value,use-value,muffle-warning,abort,continue,with-slots,with-slots*,with-accessors,with-accessors*,defclass,defmethod,print-unreadable-object"

/*
 * The following are actual variables for the options
 */

#ifdef FEAT_RIGHTLEFT
EXTERN long	p_aleph;	/* 'aleph' */
#endif
#ifdef FEAT_AUTOCHDIR
EXTERN int	p_acd;		/* 'autochdir' */
#endif
#ifdef FEAT_MBYTE
EXTERN char_u	*p_ambw;	/* 'ambiwidth' */
EXTERN char_u	*p_emoji;	/* 'emoji' */
#endif
#if defined(FEAT_GUI) && defined(MACOS_X)
EXTERN int	*p_antialias;	/* 'antialias' */
#endif
EXTERN int	p_ar;		/* 'autoread' */
EXTERN int	p_aw;		/* 'autowrite' */
EXTERN int	p_awa;		/* 'autowriteall' */
EXTERN char_u	*p_bs;		/* 'backspace' */
EXTERN char_u	*p_bg;		/* 'background' */
EXTERN int	p_bk;		/* 'backup' */
EXTERN char_u	*p_bkc;		/* 'backupcopy' */
EXTERN unsigned	bkc_flags;	/* flags from 'backupcopy' */
#ifdef IN_OPTION_C
static char *(p_bkc_values[]) = {"yes", "auto", "no", "breaksymlink", "breakhardlink", NULL};
#endif
# define BKC_YES		0x001
# define BKC_AUTO		0x002
# define BKC_NO			0x004
# define BKC_BREAKSYMLINK	0x008
# define BKC_BREAKHARDLINK	0x010
EXTERN char_u	*p_bdir;	/* 'backupdir' */
EXTERN char_u	*p_bex;		/* 'backupext' */
EXTERN char_u	*p_bo;		/* 'belloff' */
EXTERN unsigned	bo_flags;
# ifdef IN_OPTION_C
static char *(p_bo_values[]) = {"all", "backspace", "cursor", "complete",
				 "copy", "ctrlg", "error", "esc", "ex",
				 "hangul", "insertmode", "lang", "mess",
				 "showmatch", "operator", "register", "shell",
				 "spell", "wildmode", NULL};
# endif

/* values for the 'beepon' option */
#define BO_ALL		0x0001
#define BO_BS		0x0002
#define BO_CRSR		0x0004
#define BO_COMPL	0x0008
#define BO_COPY		0x0010
#define BO_CTRLG	0x0020
#define BO_ERROR	0x0040
#define BO_ESC		0x0080
#define BO_EX		0x0100
#define BO_HANGUL	0x0200
#define BO_IM		0x0400
#define BO_LANG		0x0800
#define BO_MESS		0x1000
#define BO_MATCH	0x2000
#define BO_OPER		0x4000
#define BO_REG		0x8000
#define BO_SH		0x10000
#define BO_SPELL	0x20000
#define BO_WILD		0x40000

#ifdef FEAT_WILDIGN
EXTERN char_u	*p_bsk;		/* 'backupskip' */
#endif
#ifdef FEAT_CRYPT
EXTERN char_u	*p_cm;		/* 'cryptmethod' */
#endif
#ifdef FEAT_BEVAL
EXTERN long	p_bdlay;	/* 'balloondelay' */
EXTERN int	p_beval;	/* 'ballooneval' */
# ifdef FEAT_EVAL
EXTERN char_u	*p_bexpr;
# endif
#endif
#ifdef FEAT_BROWSE
EXTERN char_u	*p_bsdir;	/* 'browsedir' */
#endif
#ifdef FEAT_LINEBREAK
EXTERN char_u	*p_breakat;	/* 'breakat' */
#endif
#ifdef FEAT_MBYTE
EXTERN char_u	*p_cmp;		/* 'casemap' */
EXTERN unsigned	cmp_flags;
# ifdef IN_OPTION_C
static char *(p_cmp_values[]) = {"internal", "keepascii", NULL};
# endif
# define CMP_INTERNAL		0x001
# define CMP_KEEPASCII		0x002
#endif
#ifdef FEAT_MBYTE
EXTERN char_u	*p_enc;		/* 'encoding' */
EXTERN int	p_deco;		/* 'delcombine' */
# ifdef FEAT_EVAL
EXTERN char_u	*p_ccv;		/* 'charconvert' */
# endif
#endif
#ifdef FEAT_CMDWIN
EXTERN char_u	*p_cedit;	/* 'cedit' */
EXTERN long	p_cwh;		/* 'cmdwinheight' */
#endif
#ifdef FEAT_CLIPBOARD
EXTERN char_u	*p_cb;		/* 'clipboard' */
#endif
EXTERN long	p_ch;		/* 'cmdheight' */
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
EXTERN int	p_confirm;	/* 'confirm' */
#endif
EXTERN int	p_cp;		/* 'compatible' */
#ifdef FEAT_INS_EXPAND
EXTERN char_u	*p_cot;		/* 'completeopt' */
EXTERN long	p_ph;		/* 'pumheight' */
#endif
EXTERN char_u	*p_cpo;		/* 'cpoptions' */
#ifdef FEAT_CSCOPE
EXTERN char_u	*p_csprg;	/* 'cscopeprg' */
EXTERN int	p_csre;		/* 'cscoperelative' */
# ifdef FEAT_QUICKFIX
EXTERN char_u	*p_csqf;	/* 'cscopequickfix' */
#  define	CSQF_CMDS   "sgdctefia"
#  define	CSQF_FLAGS  "+-0"
# endif
EXTERN int	p_cst;		/* 'cscopetag' */
EXTERN long	p_csto;		/* 'cscopetagorder' */
EXTERN long	p_cspc;		/* 'cscopepathcomp' */
EXTERN int	p_csverbose;	/* 'cscopeverbose' */
#endif
EXTERN char_u	*p_debug;	/* 'debug' */
#ifdef FEAT_FIND_ID
EXTERN char_u	*p_def;		/* 'define' */
EXTERN char_u	*p_inc;
#endif
#ifdef FEAT_DIFF
EXTERN char_u	*p_dip;		/* 'diffopt' */
# ifdef FEAT_EVAL
EXTERN char_u	*p_dex;		/* 'diffexpr' */
# endif
#endif
#ifdef FEAT_INS_EXPAND
EXTERN char_u	*p_dict;	/* 'dictionary' */
#endif
#ifdef FEAT_DIGRAPHS
EXTERN int	p_dg;		/* 'digraph' */
#endif
EXTERN char_u	*p_dir;		/* 'directory' */
EXTERN char_u	*p_dy;		/* 'display' */
EXTERN unsigned	dy_flags;
#ifdef IN_OPTION_C
static char *(p_dy_values[]) = {"lastline", "truncate", "uhex", NULL};
#endif
#define DY_LASTLINE		0x001
#define DY_TRUNCATE		0x002
#define DY_UHEX			0x004
EXTERN int	p_ed;		/* 'edcompatible' */
#ifdef FEAT_WINDOWS
EXTERN char_u	*p_ead;		/* 'eadirection' */
#endif
EXTERN int	p_ea;		/* 'equalalways' */
EXTERN char_u	*p_ep;		/* 'equalprg' */
EXTERN int	p_eb;		/* 'errorbells' */
#ifdef FEAT_QUICKFIX
EXTERN char_u	*p_ef;		/* 'errorfile' */
EXTERN char_u	*p_efm;		/* 'errorformat' */
EXTERN char_u	*p_gefm;	/* 'grepformat' */
EXTERN char_u	*p_gp;		/* 'grepprg' */
#endif
#ifdef FEAT_AUTOCMD
EXTERN char_u	*p_ei;		/* 'eventignore' */
#endif
EXTERN int	p_ek;		/* 'esckeys' */
EXTERN int	p_exrc;		/* 'exrc' */
#ifdef FEAT_MBYTE
EXTERN char_u	*p_fencs;	/* 'fileencodings' */
#endif
EXTERN char_u	*p_ffs;		/* 'fileformats' */
EXTERN long	p_fic;		/* 'fileignorecase' */
#ifdef FEAT_FOLDING
EXTERN char_u	*p_fcl;		/* 'foldclose' */
EXTERN long	p_fdls;		/* 'foldlevelstart' */
EXTERN char_u	*p_fdo;		/* 'foldopen' */
EXTERN unsigned	fdo_flags;
# ifdef IN_OPTION_C
static char *(p_fdo_values[]) = {"all", "block", "hor", "mark", "percent",
				 "quickfix", "search", "tag", "insert",
				 "undo", "jump", NULL};
# endif
# define FDO_ALL		0x001
# define FDO_BLOCK		0x002
# define FDO_HOR		0x004
# define FDO_MARK		0x008
# define FDO_PERCENT		0x010
# define FDO_QUICKFIX		0x020
# define FDO_SEARCH		0x040
# define FDO_TAG		0x080
# define FDO_INSERT		0x100
# define FDO_UNDO		0x200
# define FDO_JUMP		0x400
#endif
EXTERN char_u	*p_fp;		/* 'formatprg' */
#ifdef HAVE_FSYNC
EXTERN int	p_fs;		/* 'fsync' */
#endif
EXTERN int	p_gd;		/* 'gdefault' */
#ifdef FEAT_PRINTER
EXTERN char_u	*p_pdev;	/* 'printdevice' */
# ifdef FEAT_POSTSCRIPT
EXTERN char_u	*p_penc;	/* 'printencoding' */
EXTERN char_u	*p_pexpr;	/* 'printexpr' */
#   ifdef FEAT_MBYTE
EXTERN char_u	*p_pmfn;	/* 'printmbfont' */
EXTERN char_u	*p_pmcs;	/* 'printmbcharset' */
#   endif
# endif
EXTERN char_u	*p_pfn;		/* 'printfont' */
EXTERN char_u	*p_popt;	/* 'printoptions' */
EXTERN char_u	*p_header;	/* 'printheader' */
#endif
EXTERN int	p_prompt;	/* 'prompt' */
#ifdef FEAT_GUI
EXTERN char_u	*p_guifont;	/* 'guifont' */
# ifdef FEAT_XFONTSET
EXTERN char_u	*p_guifontset;	/* 'guifontset' */
# endif
# ifdef FEAT_MBYTE
EXTERN char_u	*p_guifontwide;	/* 'guifontwide' */
# endif
EXTERN int	p_guipty;	/* 'guipty' */
#endif
#if defined(FEAT_GUI_GTK) || defined(FEAT_GUI_X11)
EXTERN long	p_ghr;		/* 'guiheadroom' */
#endif
#ifdef CURSOR_SHAPE
EXTERN char_u	*p_guicursor;	/* 'guicursor' */
#endif
#ifdef FEAT_MOUSESHAPE
EXTERN char_u	*p_mouseshape;	/* 'mouseshape' */
#endif
#if defined(FEAT_GUI)
EXTERN char_u	*p_go;		/* 'guioptions' */
#endif
#if defined(FEAT_GUI_TABLINE)
EXTERN char_u	*p_gtl;		/* 'guitablabel' */
EXTERN char_u	*p_gtt;		/* 'guitabtooltip' */
#endif
EXTERN char_u	*p_hf;		/* 'helpfile' */
#ifdef FEAT_WINDOWS
EXTERN long	p_hh;		/* 'helpheight' */
#endif
#ifdef FEAT_MULTI_LANG
EXTERN char_u	*p_hlg;		/* 'helplang' */
#endif
EXTERN int	p_hid;		/* 'hidden' */
EXTERN char_u	*p_hl;		/* 'highlight' */
EXTERN int	p_hls;		/* 'hlsearch' */
EXTERN long	p_hi;		/* 'history' */
#ifdef FEAT_RIGHTLEFT
EXTERN int	p_hkmap;	/* 'hkmap' */
EXTERN int	p_hkmapp;	/* 'hkmapp' */
# ifdef FEAT_FKMAP
EXTERN int	p_fkmap;	/* 'fkmap' */
EXTERN int	p_altkeymap;	/* 'altkeymap' */
# endif
# ifdef FEAT_ARABIC
EXTERN int	p_arshape;	/* 'arabicshape' */
# endif
#endif
#ifdef FEAT_TITLE
EXTERN int	p_icon;		/* 'icon' */
EXTERN char_u	*p_iconstring;	/* 'iconstring' */
#endif
EXTERN int	p_ic;		/* 'ignorecase' */
#if defined(FEAT_XIM) && defined(FEAT_GUI_GTK)
EXTERN char_u	*p_imak;	/* 'imactivatekey' */
EXTERN char_u	*p_imaf;	/* 'imactivatefunc' */
EXTERN char_u	*p_imsf;	/* 'imstatusfunc' */
#endif
#ifdef USE_IM_CONTROL
EXTERN int	p_imcmdline;	/* 'imcmdline' */
EXTERN int	p_imdisable;	/* 'imdisable' */
#endif
EXTERN int	p_is;		/* 'incsearch' */
EXTERN int	p_im;		/* 'insertmode' */
EXTERN char_u	*p_isf;		/* 'isfname' */
EXTERN char_u	*p_isi;		/* 'isident' */
EXTERN char_u	*p_isp;		/* 'isprint' */
EXTERN int	p_js;		/* 'joinspaces' */
EXTERN char_u	*p_kp;		/* 'keywordprg' */
EXTERN char_u	*p_km;		/* 'keymodel' */
#ifdef FEAT_LANGMAP
EXTERN char_u	*p_langmap;	/* 'langmap'*/
EXTERN int	p_lnr;		/* 'langnoremap' */
EXTERN int	p_lrm;		/* 'langremap' */
#endif
#if defined(FEAT_MENU) && defined(FEAT_MULTI_LANG)
EXTERN char_u	*p_lm;		/* 'langmenu' */
#endif
#ifdef FEAT_GUI
EXTERN long	p_linespace;	/* 'linespace' */
#endif
#ifdef FEAT_LISP
EXTERN char_u	*p_lispwords;	/* 'lispwords' */
#endif
#ifdef FEAT_WINDOWS
EXTERN long	p_ls;		/* 'laststatus' */
EXTERN long	p_stal;		/* 'showtabline' */
#endif
EXTERN char_u	*p_lcs;		/* 'listchars' */

EXTERN int	p_lz;		/* 'lazyredraw' */
EXTERN int	p_lpl;		/* 'loadplugins' */
#if defined(DYNAMIC_LUA)
EXTERN char_u	*p_luadll;	/* 'luadll' */
#endif
#ifdef FEAT_GUI_MAC
EXTERN int	p_macatsui;	/* 'macatsui' */
#endif
EXTERN int	p_magic;	/* 'magic' */
#ifdef FEAT_MBYTE
EXTERN char_u	*p_menc;	/* 'makeencoding' */
#endif
#ifdef FEAT_QUICKFIX
EXTERN char_u	*p_mef;		/* 'makeef' */
EXTERN char_u	*p_mp;		/* 'makeprg' */
#endif
#ifdef FEAT_SIGNS
EXTERN char_u  *p_scl;		/* signcolumn */
#endif
#ifdef FEAT_SYN_HL
EXTERN char_u   *p_cc;		/* 'colorcolumn' */
EXTERN int      p_cc_cols[256]; /* array for 'colorcolumn' columns */
#endif
EXTERN long	p_mat;		/* 'matchtime' */
#ifdef FEAT_MBYTE
EXTERN long	p_mco;		/* 'maxcombine' */
#endif
#ifdef FEAT_EVAL
EXTERN long	p_mfd;		/* 'maxfuncdepth' */
#endif
EXTERN long	p_mmd;		/* 'maxmapdepth' */
EXTERN long	p_mm;		/* 'maxmem' */
EXTERN long	p_mmp;		/* 'maxmempattern' */
EXTERN long	p_mmt;		/* 'maxmemtot' */
#ifdef FEAT_MENU
EXTERN long	p_mis;		/* 'menuitems' */
#endif
#ifdef FEAT_SPELL
EXTERN char_u	*p_msm;		/* 'mkspellmem' */
#endif
EXTERN long	p_mls;		/* 'modelines' */
EXTERN char_u	*p_mouse;	/* 'mouse' */
#ifdef FEAT_GUI
EXTERN int	p_mousef;	/* 'mousefocus' */
EXTERN int	p_mh;		/* 'mousehide' */
#endif
EXTERN char_u	*p_mousem;	/* 'mousemodel' */
EXTERN long	p_mouset;	/* 'mousetime' */
EXTERN int	p_more;		/* 'more' */
#ifdef FEAT_MZSCHEME
EXTERN long	p_mzq;		/* 'mzquantum */
#endif
#if defined(MSWIN)
EXTERN int	p_odev;		/* 'opendevice' */
#endif
EXTERN char_u	*p_opfunc;	/* 'operatorfunc' */
EXTERN char_u	*p_para;	/* 'paragraphs' */
EXTERN int	p_paste;	/* 'paste' */
EXTERN char_u	*p_pt;		/* 'pastetoggle' */
#if defined(FEAT_EVAL) && defined(FEAT_DIFF)
EXTERN char_u	*p_pex;		/* 'patchexpr' */
#endif
EXTERN char_u	*p_pm;		/* 'patchmode' */
EXTERN char_u	*p_path;	/* 'path' */
#ifdef FEAT_SEARCHPATH
EXTERN char_u	*p_cdpath;	/* 'cdpath' */
#endif
#if defined(DYNAMIC_PERL)
EXTERN char_u	*p_perldll;	/* 'perldll' */
#endif
#if defined(DYNAMIC_PYTHON3)
EXTERN char_u	*p_py3dll;	/* 'pythonthreedll' */
#endif
#if defined(DYNAMIC_PYTHON)
EXTERN char_u	*p_pydll;	/* 'pythondll' */
#endif
#if defined(FEAT_PYTHON) || defined(FEAT_PYTHON3)
EXTERN long	p_pyx;		/* 'pyxversion' */
#endif
#ifdef FEAT_RELTIME
EXTERN long	p_rdt;		/* 'redrawtime' */
#endif
EXTERN int	p_remap;	/* 'remap' */
EXTERN long	p_re;		/* 'regexpengine' */
#ifdef FEAT_RENDER_OPTIONS
EXTERN char_u	*p_rop;		/* 'renderoptions' */
#endif
EXTERN long	p_report;	/* 'report' */
#if defined(FEAT_WINDOWS) && defined(FEAT_QUICKFIX)
EXTERN long	p_pvh;		/* 'previewheight' */
#endif
#ifdef WIN3264
EXTERN int	p_rs;		/* 'restorescreen' */
#endif
#ifdef FEAT_RIGHTLEFT
EXTERN int	p_ari;		/* 'allowrevins' */
EXTERN int	p_ri;		/* 'revins' */
#endif
#if defined(DYNAMIC_RUBY)
EXTERN char_u	*p_rubydll;	/* 'rubydll' */
#endif
#ifdef FEAT_CMDL_INFO
EXTERN int	p_ru;		/* 'ruler' */
#endif
#ifdef FEAT_STL_OPT
EXTERN char_u	*p_ruf;		/* 'rulerformat' */
#endif
EXTERN char_u	*p_pp;		/* 'packpath' */
EXTERN char_u	*p_rtp;		/* 'runtimepath' */
EXTERN long	p_sj;		/* 'scrolljump' */
EXTERN long	p_so;		/* 'scrolloff' */
#ifdef FEAT_SCROLLBIND
EXTERN char_u	*p_sbo;		/* 'scrollopt' */
#endif
EXTERN char_u	*p_sections;	/* 'sections' */
EXTERN int	p_secure;	/* 'secure' */
EXTERN char_u	*p_sel;		/* 'selection' */
EXTERN char_u	*p_slm;		/* 'selectmode' */
#ifdef FEAT_SESSION
EXTERN char_u	*p_ssop;	/* 'sessionoptions' */
EXTERN unsigned	ssop_flags;
# ifdef IN_OPTION_C
/* Also used for 'viewoptions'! */
static char *(p_ssop_values[]) = {"buffers", "winpos", "resize", "winsize",
    "localoptions", "options", "help", "blank", "globals", "slash", "unix",
    "sesdir", "curdir", "folds", "cursor", "tabpages", NULL};
# endif
# define SSOP_BUFFERS		0x001
# define SSOP_WINPOS		0x002
# define SSOP_RESIZE		0x004
# define SSOP_WINSIZE		0x008
# define SSOP_LOCALOPTIONS	0x010
# define SSOP_OPTIONS		0x020
# define SSOP_HELP		0x040
# define SSOP_BLANK		0x080
# define SSOP_GLOBALS		0x100
# define SSOP_SLASH		0x200
# define SSOP_UNIX		0x400
# define SSOP_SESDIR		0x800
# define SSOP_CURDIR		0x1000
# define SSOP_FOLDS		0x2000
# define SSOP_CURSOR		0x4000
# define SSOP_TABPAGES		0x8000
#endif
EXTERN char_u	*p_sh;		/* 'shell' */
EXTERN char_u	*p_shcf;	/* 'shellcmdflag' */
#ifdef FEAT_QUICKFIX
EXTERN char_u	*p_sp;		/* 'shellpipe' */
#endif
EXTERN char_u	*p_shq;		/* 'shellquote' */
EXTERN char_u	*p_sxq;		/* 'shellxquote' */
EXTERN char_u	*p_sxe;		/* 'shellxescape' */
EXTERN char_u	*p_srr;		/* 'shellredir' */
#ifdef AMIGA
EXTERN long	p_st;		/* 'shelltype' */
#endif
EXTERN int	p_stmp;		/* 'shelltemp' */
#ifdef BACKSLASH_IN_FILENAME
EXTERN int	p_ssl;		/* 'shellslash' */
#endif
#ifdef FEAT_STL_OPT
EXTERN char_u	*p_stl;		/* 'statusline' */
#endif
EXTERN int	p_sr;		/* 'shiftround' */
EXTERN char_u	*p_shm;		/* 'shortmess' */
#ifdef FEAT_LINEBREAK
EXTERN char_u	*p_sbr;		/* 'showbreak' */
#endif
#ifdef FEAT_CMDL_INFO
EXTERN int	p_sc;		/* 'showcmd' */
#endif
EXTERN int	p_sft;		/* 'showfulltag' */
EXTERN int	p_sm;		/* 'showmatch' */
EXTERN int	p_smd;		/* 'showmode' */
EXTERN long	p_ss;		/* 'sidescroll' */
EXTERN long	p_siso;		/* 'sidescrolloff' */
EXTERN int	p_scs;		/* 'smartcase' */
EXTERN int	p_sta;		/* 'smarttab' */
#ifdef FEAT_WINDOWS
EXTERN int	p_sb;		/* 'splitbelow' */
EXTERN long	p_tpm;		/* 'tabpagemax' */
# if defined(FEAT_STL_OPT)
EXTERN char_u	*p_tal;		/* 'tabline' */
# endif
#endif
#ifdef FEAT_SPELL
EXTERN char_u	*p_sps;		/* 'spellsuggest' */
#endif
#ifdef FEAT_WINDOWS
EXTERN int	p_spr;		/* 'splitright' */
#endif
EXTERN int	p_sol;		/* 'startofline' */
EXTERN char_u	*p_su;		/* 'suffixes' */
EXTERN char_u	*p_sws;		/* 'swapsync' */
EXTERN char_u	*p_swb;		/* 'switchbuf' */
EXTERN unsigned	swb_flags;
#ifdef IN_OPTION_C
static char *(p_swb_values[]) = {"useopen", "usetab", "split", "newtab", "vsplit", NULL};
#endif
#define SWB_USEOPEN		0x001
#define SWB_USETAB		0x002
#define SWB_SPLIT		0x004
#define SWB_NEWTAB		0x008
#define SWB_VSPLIT		0x010
EXTERN int	p_tbs;		/* 'tagbsearch' */
EXTERN char_u	*p_tc;		/* 'tagcase' */
EXTERN unsigned tc_flags;       /* flags from 'tagcase' */
#ifdef IN_OPTION_C
static char *(p_tc_values[]) = {"followic", "ignore", "match", "followscs", "smart", NULL};
#endif
#define TC_FOLLOWIC		0x01
#define TC_IGNORE		0x02
#define TC_MATCH		0x04
#define TC_FOLLOWSCS		0x08
#define TC_SMART		0x10
EXTERN long	p_tl;		/* 'taglength' */
EXTERN int	p_tr;		/* 'tagrelative' */
EXTERN char_u	*p_tags;	/* 'tags' */
EXTERN int	p_tgst;		/* 'tagstack' */
#if defined(DYNAMIC_TCL)
EXTERN char_u	*p_tcldll;	/* 'tcldll' */
#endif
#ifdef FEAT_ARABIC
EXTERN int	p_tbidi;	/* 'termbidi' */
#endif
#ifdef FEAT_MBYTE
EXTERN char_u	*p_tenc;	/* 'termencoding' */
#endif
#ifdef FEAT_TERMGUICOLORS
EXTERN int	p_tgc;		/* 'termguicolors' */
#endif
EXTERN int	p_terse;	/* 'terse' */
EXTERN int	p_ta;		/* 'textauto' */
EXTERN int	p_to;		/* 'tildeop' */
EXTERN int	p_timeout;	/* 'timeout' */
EXTERN long	p_tm;		/* 'timeoutlen' */
#ifdef FEAT_TITLE
EXTERN int	p_title;	/* 'title' */
EXTERN long	p_titlelen;	/* 'titlelen' */
EXTERN char_u	*p_titleold;	/* 'titleold' */
EXTERN char_u	*p_titlestring;	/* 'titlestring' */
#endif
#ifdef FEAT_INS_EXPAND
EXTERN char_u	*p_tsr;		/* 'thesaurus' */
#endif
EXTERN int	p_ttimeout;	/* 'ttimeout' */
EXTERN long	p_ttm;		/* 'ttimeoutlen' */
EXTERN int	p_tbi;		/* 'ttybuiltin' */
EXTERN int	p_tf;		/* 'ttyfast' */
#if defined(FEAT_TOOLBAR) && !defined(FEAT_GUI_W32)
EXTERN char_u	*p_toolbar;	/* 'toolbar' */
EXTERN unsigned toolbar_flags;
# ifdef IN_OPTION_C
static char *(p_toolbar_values[]) = {"text", "icons", "tooltips", "horiz", NULL};
# endif
# define TOOLBAR_TEXT		0x01
# define TOOLBAR_ICONS		0x02
# define TOOLBAR_TOOLTIPS	0x04
# define TOOLBAR_HORIZ		0x08
#endif
#if defined(FEAT_TOOLBAR) && defined(FEAT_GUI_GTK)
EXTERN char_u	*p_tbis;	/* 'toolbariconsize' */
EXTERN unsigned tbis_flags;
# ifdef IN_OPTION_C
static char *(p_tbis_values[]) = {"tiny", "small", "medium", "large", "huge", "giant", NULL};
# endif
# define TBIS_TINY		0x01
# define TBIS_SMALL		0x02
# define TBIS_MEDIUM		0x04
# define TBIS_LARGE		0x08
# define TBIS_HUGE		0x10
# define TBIS_GIANT		0x20
#endif
EXTERN long	p_ttyscroll;	/* 'ttyscroll' */
#if defined(FEAT_MOUSE) && (defined(UNIX) || defined(VMS))
EXTERN char_u	*p_ttym;	/* 'ttymouse' */
EXTERN unsigned ttym_flags;
# ifdef IN_OPTION_C
static char *(p_ttym_values[]) = {"xterm", "xterm2", "dec", "netterm", "jsbterm", "pterm", "urxvt", "sgr", NULL};
# endif
# define TTYM_XTERM		0x01
# define TTYM_XTERM2		0x02
# define TTYM_DEC		0x04
# define TTYM_NETTERM		0x08
# define TTYM_JSBTERM		0x10
# define TTYM_PTERM		0x20
# define TTYM_URXVT		0x40
# define TTYM_SGR		0x80
#endif
EXTERN char_u	*p_udir;	/* 'undodir' */
EXTERN long	p_ul;		/* 'undolevels' */
EXTERN long	p_ur;		/* 'undoreload' */
EXTERN long	p_uc;		/* 'updatecount' */
EXTERN long	p_ut;		/* 'updatetime' */
#if defined(FEAT_WINDOWS) || defined(FEAT_FOLDING)
EXTERN char_u	*p_fcs;		/* 'fillchar' */
#endif
#ifdef FEAT_VIMINFO
EXTERN char_u	*p_viminfo;	/* 'viminfo' */
EXTERN char_u	*p_viminfofile;	/* 'viminfofile' */
#endif
#ifdef FEAT_SESSION
EXTERN char_u	*p_vdir;	/* 'viewdir' */
EXTERN char_u	*p_vop;		/* 'viewoptions' */
EXTERN unsigned	vop_flags;	/* uses SSOP_ flags */
#endif
EXTERN int	p_vb;		/* 'visualbell' */
#ifdef FEAT_VIRTUALEDIT
EXTERN char_u	*p_ve;		/* 'virtualedit' */
EXTERN unsigned ve_flags;
# ifdef IN_OPTION_C
static char *(p_ve_values[]) = {"block", "insert", "all", "onemore", NULL};
# endif
# define VE_BLOCK	5	/* includes "all" */
# define VE_INSERT	6	/* includes "all" */
# define VE_ALL		4
# define VE_ONEMORE	8
#endif
EXTERN long	p_verbose;	/* 'verbose' */
#ifdef IN_OPTION_C
char_u	*p_vfile = (char_u *)""; /* used before options are initialized */
#else
extern char_u	*p_vfile;	/* 'verbosefile' */
#endif
EXTERN int	p_warn;		/* 'warn' */
#ifdef FEAT_CMDL_COMPL
EXTERN char_u	*p_wop;		/* 'wildoptions' */
#endif
EXTERN long	p_window;	/* 'window' */
#if defined(FEAT_GUI_MSWIN) || defined(FEAT_GUI_MOTIF) || defined(LINT) \
	|| defined (FEAT_GUI_GTK) || defined(FEAT_GUI_PHOTON)
#define FEAT_WAK
EXTERN char_u	*p_wak;		/* 'winaltkeys' */
#endif
#ifdef FEAT_WILDIGN
EXTERN char_u	*p_wig;		/* 'wildignore' */
#endif
EXTERN int	p_wiv;		/* 'weirdinvert' */
EXTERN char_u	*p_ww;		/* 'whichwrap' */
EXTERN long	p_wc;		/* 'wildchar' */
EXTERN long	p_wcm;		/* 'wildcharm' */
EXTERN long	p_wic;		/* 'wildignorecase' */
EXTERN char_u	*p_wim;		/* 'wildmode' */
#ifdef FEAT_WILDMENU
EXTERN int	p_wmnu;		/* 'wildmenu' */
#endif
#ifdef FEAT_WINDOWS
EXTERN long	p_wh;		/* 'winheight' */
EXTERN long	p_wmh;		/* 'winminheight' */
EXTERN long	p_wmw;		/* 'winminwidth' */
EXTERN long	p_wiw;		/* 'winwidth' */
#endif
EXTERN int	p_ws;		/* 'wrapscan' */
EXTERN int	p_write;	/* 'write' */
EXTERN int	p_wa;		/* 'writeany' */
EXTERN int	p_wb;		/* 'writebackup' */
EXTERN long	p_wd;		/* 'writedelay' */

/*
 * "indir" values for buffer-local opions.
 * These need to be defined globally, so that the BV_COUNT can be used with
 * b_p_scriptID[].
 */
enum
{
    BV_AI = 0
    , BV_AR
    , BV_BH
    , BV_BKC
    , BV_BT
#ifdef FEAT_QUICKFIX
    , BV_EFM
    , BV_GP
    , BV_MP
#endif
    , BV_BIN
    , BV_BL
#ifdef FEAT_MBYTE
    , BV_BOMB
#endif
    , BV_CI
#ifdef FEAT_CINDENT
    , BV_CIN
    , BV_CINK
    , BV_CINO
#endif
#if defined(FEAT_SMARTINDENT) || defined(FEAT_CINDENT)
    , BV_CINW
#endif
    , BV_CM
#ifdef FEAT_FOLDING
    , BV_CMS
#endif
#ifdef FEAT_COMMENTS
    , BV_COM
#endif
#ifdef FEAT_INS_EXPAND
    , BV_CPT
    , BV_DICT
    , BV_TSR
#endif
#ifdef FEAT_COMPL_FUNC
    , BV_CFU
#endif
#ifdef FEAT_FIND_ID
    , BV_DEF
    , BV_INC
#endif
    , BV_EOL
    , BV_FIXEOL
    , BV_EP
    , BV_ET
    , BV_FENC
    , BV_FP
#ifdef FEAT_EVAL
    , BV_BEXPR
    , BV_FEX
#endif
    , BV_FF
    , BV_FLP
    , BV_FO
#ifdef FEAT_AUTOCMD
    , BV_FT
#endif
    , BV_IMI
    , BV_IMS
#if defined(FEAT_CINDENT) && defined(FEAT_EVAL)
    , BV_INDE
    , BV_INDK
#endif
#if defined(FEAT_FIND_ID) && defined(FEAT_EVAL)
    , BV_INEX
#endif
    , BV_INF
    , BV_ISK
#ifdef FEAT_CRYPT
    , BV_KEY
#endif
#ifdef FEAT_KEYMAP
    , BV_KMAP
#endif
    , BV_KP
#ifdef FEAT_LISP
    , BV_LISP
    , BV_LW
#endif
#ifdef FEAT_MBYTE
    , BV_MENC
#endif
    , BV_MA
    , BV_ML
    , BV_MOD
    , BV_MPS
    , BV_NF
#ifdef FEAT_COMPL_FUNC
    , BV_OFU
#endif
    , BV_PATH
    , BV_PI
#ifdef FEAT_TEXTOBJ
    , BV_QE
#endif
    , BV_RO
#ifdef FEAT_SMARTINDENT
    , BV_SI
#endif
    , BV_SN
#ifdef FEAT_SYN_HL
    , BV_SMC
    , BV_SYN
#endif
#ifdef FEAT_SPELL
    , BV_SPC
    , BV_SPF
    , BV_SPL
#endif
    , BV_STS
#ifdef FEAT_SEARCHPATH
    , BV_SUA
#endif
    , BV_SW
    , BV_SWF
    , BV_TAGS
    , BV_TC
    , BV_TS
    , BV_TW
    , BV_TX
    , BV_UDF
    , BV_UL
    , BV_WM
    , BV_COUNT	    /* must be the last one */
};

/*
 * "indir" values for window-local options.
 * These need to be defined globally, so that the WV_COUNT can be used in the
 * window structure.
 */
enum
{
    WV_LIST = 0
#ifdef FEAT_ARABIC
    , WV_ARAB
#endif
#ifdef FEAT_CONCEAL
    , WV_COCU
    , WV_COLE
#endif
#ifdef FEAT_TERMINAL
    , WV_TK
    , WV_TMS
#endif
#ifdef FEAT_CURSORBIND
    , WV_CRBIND
#endif
#ifdef FEAT_LINEBREAK
    , WV_BRI
    , WV_BRIOPT
#endif
#ifdef FEAT_DIFF
    , WV_DIFF
#endif
#ifdef FEAT_FOLDING
    , WV_FDC
    , WV_FEN
    , WV_FDI
    , WV_FDL
    , WV_FDM
    , WV_FML
    , WV_FDN
# ifdef FEAT_EVAL
    , WV_FDE
    , WV_FDT
# endif
    , WV_FMR
#endif
#ifdef FEAT_LINEBREAK
    , WV_LBR
#endif
    , WV_NU
    , WV_RNU
#ifdef FEAT_LINEBREAK
    , WV_NUW
#endif
#if defined(FEAT_WINDOWS) && defined(FEAT_QUICKFIX)
    , WV_PVW
#endif
#ifdef FEAT_RIGHTLEFT
    , WV_RL
    , WV_RLC
#endif
#ifdef FEAT_SCROLLBIND
    , WV_SCBIND
#endif
    , WV_SCROLL
#ifdef FEAT_SPELL
    , WV_SPELL
#endif
#ifdef FEAT_SYN_HL
    , WV_CUC
    , WV_CUL
    , WV_CC
#endif
#ifdef FEAT_STL_OPT
    , WV_STL
#endif
#ifdef FEAT_WINDOWS
    , WV_WFH
    , WV_WFW
#endif
    , WV_WRAP
#ifdef FEAT_SIGNS
    , WV_SCL
#endif
    , WV_COUNT	    /* must be the last one */
};

/* Value for b_p_ul indicating the global value must be used. */
#define NO_LOCAL_UNDOLEVEL -123456
