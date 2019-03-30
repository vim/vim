/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */
/*
 *
 * term.c: functions for controlling the terminal
 *
 * primitive termcap support for Amiga and Win32 included
 *
 * NOTE: padding and variable substitution is not performed,
 * when compiling without HAVE_TGETENT, we use tputs() and tgoto() dummies.
 */

/*
 * Some systems have a prototype for tgetstr() with (char *) instead of
 * (char **). This define removes that prototype. We include our own prototype
 * below.
 */

#define tgetstr tgetstr_defined_wrong
#include "vim.h"

#ifdef HAVE_TGETENT
# ifdef HAVE_TERMIOS_H
#  include <termios.h>	    /* seems to be required for some Linux */
# endif
# ifdef HAVE_TERMCAP_H
#  include <termcap.h>
# endif

/*
 * A few linux systems define outfuntype in termcap.h to be used as the third
 * argument for tputs().
 */
# ifdef VMS
#  define TPUTSFUNCAST
# else
#  ifdef HAVE_OUTFUNTYPE
#   define TPUTSFUNCAST (outfuntype)
#  else
#   define TPUTSFUNCAST (int (*)())
#  endif
# endif
#endif

#undef tgetstr

/*
 * Here are the builtin termcap entries.  They are not stored as complete
 * structures with all entries, as such a structure is too big.
 *
 * The entries are compact, therefore they normally are included even when
 * HAVE_TGETENT is defined. When HAVE_TGETENT is defined, the builtin entries
 * can be accessed with "builtin_amiga", "builtin_ansi", "builtin_debug", etc.
 *
 * Each termcap is a list of builtin_term structures. It always starts with
 * KS_NAME, which separates the entries.  See parse_builtin_tcap() for all
 * details.
 * bt_entry is either a KS_xxx code (>= 0), or a K_xxx code.
 *
 * Entries marked with "guessed" may be wrong.
 */
struct builtin_term
{
    int		bt_entry;
    char	*bt_string;
};

/* start of keys that are not directly used by Vim but can be mapped */
#define BT_EXTRA_KEYS	0x101

static void parse_builtin_tcap(char_u *s);
static void gather_termleader(void);
#ifdef FEAT_TERMRESPONSE
static void req_codes_from_term(void);
static void req_more_codes_from_term(void);
static void got_code_from_term(char_u *code, int len);
static void check_for_codes_from_term(void);
#endif
#if defined(FEAT_GUI) \
    || (defined(FEAT_MOUSE) && (!defined(UNIX) || defined(FEAT_MOUSE_XTERM) \
		|| defined(FEAT_MOUSE_GPM) || defined(FEAT_SYSMOUSE)))
static int get_bytes_from_buf(char_u *, char_u *, int);
#endif
static void del_termcode_idx(int idx);
static int term_is_builtin(char_u *name);
static int term_7to8bit(char_u *p);

#ifdef HAVE_TGETENT
static char *tgetent_error(char_u *, char_u *);

/*
 * Here is our own prototype for tgetstr(), any prototypes from the include
 * files have been disabled by the define at the start of this file.
 */
char		*tgetstr(char *, char **);

# ifdef FEAT_TERMRESPONSE
    /* Change this to "if 1" to debug what happens with termresponse. */
#  if 0
#   define DEBUG_TERMRESPONSE
static void log_tr(const char *fmt, ...);
#   define LOG_TR(msg) log_tr msg
#  else
#   define LOG_TR(msg) do { /**/ } while (0)
#  endif

#  define STATUS_GET	1	/* send request when switching to RAW mode */
#  define STATUS_SENT	2	/* did send request, waiting for response */
#  define STATUS_GOT	3	/* received response */

/* Request Terminal Version status: */
static int crv_status = STATUS_GET;

/* Request Cursor position report: */
static int u7_status = STATUS_GET;

#  ifdef FEAT_TERMINAL
/* Request foreground color report: */
static int rfg_status = STATUS_GET;
static int fg_r = 0;
static int fg_g = 0;
static int fg_b = 0;
static int bg_r = 255;
static int bg_g = 255;
static int bg_b = 255;
#  endif

/* Request background color report: */
static int rbg_status = STATUS_GET;

/* Request cursor blinking mode report: */
static int rbm_status = STATUS_GET;

/* Request cursor style report: */
static int rcs_status = STATUS_GET;

/* Request windos position report: */
static int winpos_status = STATUS_GET;
# endif

/*
 * Don't declare these variables if termcap.h contains them.
 * Autoconf checks if these variables should be declared extern (not all
 * systems have them).
 * Some versions define ospeed to be speed_t, but that is incompatible with
 * BSD, where ospeed is short and speed_t is long.
 */
# ifndef HAVE_OSPEED
#  ifdef OSPEED_EXTERN
extern short ospeed;
#   else
short ospeed;
#   endif
# endif
# ifndef HAVE_UP_BC_PC
#  ifdef UP_BC_PC_EXTERN
extern char *UP, *BC, PC;
#  else
char *UP, *BC, PC;
#  endif
# endif

# define TGETSTR(s, p)	vim_tgetstr((s), (p))
# define TGETENT(b, t)	tgetent((char *)(b), (char *)(t))
static char_u *vim_tgetstr(char *s, char_u **pp);
#endif /* HAVE_TGETENT */

static int  detected_8bit = FALSE;	/* detected 8-bit terminal */

#ifdef FEAT_TERMRESPONSE
/* When the cursor shape was detected these values are used:
 * 1: block, 2: underline, 3: vertical bar */
static int initial_cursor_shape = 0;

/* The blink flag from the style response may be inverted from the actual
 * blinking state, xterm XORs the flags. */
static int initial_cursor_shape_blink = FALSE;

/* The blink flag from the blinking-cursor mode response */
static int initial_cursor_blink = FALSE;
#endif

static struct builtin_term builtin_termcaps[] =
{

#if defined(FEAT_GUI)
/*
 * GUI pseudo term-cap.
 */
    {(int)KS_NAME,	"gui"},
    {(int)KS_CE,	IF_EB("\033|$", ESC_STR "|$")},
    {(int)KS_AL,	IF_EB("\033|i", ESC_STR "|i")},
# ifdef TERMINFO
    {(int)KS_CAL,	IF_EB("\033|%p1%dI", ESC_STR "|%p1%dI")},
# else
    {(int)KS_CAL,	IF_EB("\033|%dI", ESC_STR "|%dI")},
# endif
    {(int)KS_DL,	IF_EB("\033|d", ESC_STR "|d")},
# ifdef TERMINFO
    {(int)KS_CDL,	IF_EB("\033|%p1%dD", ESC_STR "|%p1%dD")},
    {(int)KS_CS,	IF_EB("\033|%p1%d;%p2%dR", ESC_STR "|%p1%d;%p2%dR")},
    {(int)KS_CSV,	IF_EB("\033|%p1%d;%p2%dV", ESC_STR "|%p1%d;%p2%dV")},
# else
    {(int)KS_CDL,	IF_EB("\033|%dD", ESC_STR "|%dD")},
    {(int)KS_CS,	IF_EB("\033|%d;%dR", ESC_STR "|%d;%dR")},
    {(int)KS_CSV,	IF_EB("\033|%d;%dV", ESC_STR "|%d;%dV")},
# endif
    {(int)KS_CL,	IF_EB("\033|C", ESC_STR "|C")},
			/* attributes switched on with 'h', off with * 'H' */
    {(int)KS_ME,	IF_EB("\033|31H", ESC_STR "|31H")}, /* HL_ALL */
    {(int)KS_MR,	IF_EB("\033|1h", ESC_STR "|1h")},   /* HL_INVERSE */
    {(int)KS_MD,	IF_EB("\033|2h", ESC_STR "|2h")},   /* HL_BOLD */
    {(int)KS_SE,	IF_EB("\033|16H", ESC_STR "|16H")}, /* HL_STANDOUT */
    {(int)KS_SO,	IF_EB("\033|16h", ESC_STR "|16h")}, /* HL_STANDOUT */
    {(int)KS_UE,	IF_EB("\033|8H", ESC_STR "|8H")},   /* HL_UNDERLINE */
    {(int)KS_US,	IF_EB("\033|8h", ESC_STR "|8h")},   /* HL_UNDERLINE */
    {(int)KS_UCE,	IF_EB("\033|8C", ESC_STR "|8C")},   /* HL_UNDERCURL */
    {(int)KS_UCS,	IF_EB("\033|8c", ESC_STR "|8c")},   /* HL_UNDERCURL */
    {(int)KS_STE,	IF_EB("\033|4C", ESC_STR "|4C")},   /* HL_STRIKETHROUGH */
    {(int)KS_STS,	IF_EB("\033|4c", ESC_STR "|4c")},   /* HL_STRIKETHROUGH */
    {(int)KS_CZR,	IF_EB("\033|4H", ESC_STR "|4H")},   /* HL_ITALIC */
    {(int)KS_CZH,	IF_EB("\033|4h", ESC_STR "|4h")},   /* HL_ITALIC */
    {(int)KS_VB,	IF_EB("\033|f", ESC_STR "|f")},
    {(int)KS_MS,	"y"},
    {(int)KS_UT,	"y"},
    {(int)KS_XN,	"y"},
    {(int)KS_LE,	"\b"},		/* cursor-left = BS */
    {(int)KS_ND,	"\014"},	/* cursor-right = CTRL-L */
# ifdef TERMINFO
    {(int)KS_CM,	IF_EB("\033|%p1%d;%p2%dM", ESC_STR "|%p1%d;%p2%dM")},
# else
    {(int)KS_CM,	IF_EB("\033|%d;%dM", ESC_STR "|%d;%dM")},
# endif
	/* there are no key sequences here, the GUI sequences are recognized
	 * in check_termcode() */
#endif

#ifndef NO_BUILTIN_TCAPS

# if defined(AMIGA) || defined(ALL_BUILTIN_TCAPS)
/*
 * Amiga console window, default for Amiga
 */
    {(int)KS_NAME,	"amiga"},
    {(int)KS_CE,	"\033[K"},
    {(int)KS_CD,	"\033[J"},
    {(int)KS_AL,	"\033[L"},
#  ifdef TERMINFO
    {(int)KS_CAL,	"\033[%p1%dL"},
#  else
    {(int)KS_CAL,	"\033[%dL"},
#  endif
    {(int)KS_DL,	"\033[M"},
#  ifdef TERMINFO
    {(int)KS_CDL,	"\033[%p1%dM"},
#  else
    {(int)KS_CDL,	"\033[%dM"},
#  endif
    {(int)KS_CL,	"\014"},
    {(int)KS_VI,	"\033[0 p"},
    {(int)KS_VE,	"\033[1 p"},
    {(int)KS_ME,	"\033[0m"},
    {(int)KS_MR,	"\033[7m"},
    {(int)KS_MD,	"\033[1m"},
    {(int)KS_SE,	"\033[0m"},
    {(int)KS_SO,	"\033[33m"},
    {(int)KS_US,	"\033[4m"},
    {(int)KS_UE,	"\033[0m"},
    {(int)KS_CZH,	"\033[3m"},
    {(int)KS_CZR,	"\033[0m"},
#if defined(__MORPHOS__) || defined(__AROS__)
    {(int)KS_CCO,	"8"},		/* allow 8 colors */
#  ifdef TERMINFO
    {(int)KS_CAB,	"\033[4%p1%dm"},/* set background color */
    {(int)KS_CAF,	"\033[3%p1%dm"},/* set foreground color */
#  else
    {(int)KS_CAB,	"\033[4%dm"},	/* set background color */
    {(int)KS_CAF,	"\033[3%dm"},	/* set foreground color */
#  endif
    {(int)KS_OP,	"\033[m"},	/* reset colors */
#endif
    {(int)KS_MS,	"y"},
    {(int)KS_UT,	"y"},		/* guessed */
    {(int)KS_LE,	"\b"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033[%i%p1%d;%p2%dH"},
#  else
    {(int)KS_CM,	"\033[%i%d;%dH"},
#  endif
#if defined(__MORPHOS__)
    {(int)KS_SR,	"\033M"},
#endif
#  ifdef TERMINFO
    {(int)KS_CRI,	"\033[%p1%dC"},
#  else
    {(int)KS_CRI,	"\033[%dC"},
#  endif
    {K_UP,		"\233A"},
    {K_DOWN,		"\233B"},
    {K_LEFT,		"\233D"},
    {K_RIGHT,		"\233C"},
    {K_S_UP,		"\233T"},
    {K_S_DOWN,		"\233S"},
    {K_S_LEFT,		"\233 A"},
    {K_S_RIGHT,		"\233 @"},
    {K_S_TAB,		"\233Z"},
    {K_F1,		"\233\060~"},/* some compilers don't dig "\2330" */
    {K_F2,		"\233\061~"},
    {K_F3,		"\233\062~"},
    {K_F4,		"\233\063~"},
    {K_F5,		"\233\064~"},
    {K_F6,		"\233\065~"},
    {K_F7,		"\233\066~"},
    {K_F8,		"\233\067~"},
    {K_F9,		"\233\070~"},
    {K_F10,		"\233\071~"},
    {K_S_F1,		"\233\061\060~"},
    {K_S_F2,		"\233\061\061~"},
    {K_S_F3,		"\233\061\062~"},
    {K_S_F4,		"\233\061\063~"},
    {K_S_F5,		"\233\061\064~"},
    {K_S_F6,		"\233\061\065~"},
    {K_S_F7,		"\233\061\066~"},
    {K_S_F8,		"\233\061\067~"},
    {K_S_F9,		"\233\061\070~"},
    {K_S_F10,		"\233\061\071~"},
    {K_HELP,		"\233?~"},
    {K_INS,		"\233\064\060~"},	/* 101 key keyboard */
    {K_PAGEUP,		"\233\064\061~"},	/* 101 key keyboard */
    {K_PAGEDOWN,	"\233\064\062~"},	/* 101 key keyboard */
    {K_HOME,		"\233\064\064~"},	/* 101 key keyboard */
    {K_END,		"\233\064\065~"},	/* 101 key keyboard */

    {BT_EXTRA_KEYS,	""},
    {TERMCAP2KEY('#', '2'), "\233\065\064~"},	/* shifted home key */
    {TERMCAP2KEY('#', '3'), "\233\065\060~"},	/* shifted insert key */
    {TERMCAP2KEY('*', '7'), "\233\065\065~"},	/* shifted end key */
# endif

# if defined(__BEOS__) || defined(ALL_BUILTIN_TCAPS)
/*
 * almost standard ANSI terminal, default for bebox
 */
    {(int)KS_NAME,	"beos-ansi"},
    {(int)KS_CE,	"\033[K"},
    {(int)KS_CD,	"\033[J"},
    {(int)KS_AL,	"\033[L"},
#  ifdef TERMINFO
    {(int)KS_CAL,	"\033[%p1%dL"},
#  else
    {(int)KS_CAL,	"\033[%dL"},
#  endif
    {(int)KS_DL,	"\033[M"},
#  ifdef TERMINFO
    {(int)KS_CDL,	"\033[%p1%dM"},
#  else
    {(int)KS_CDL,	"\033[%dM"},
#  endif
#ifdef BEOS_PR_OR_BETTER
#  ifdef TERMINFO
    {(int)KS_CS,	"\033[%i%p1%d;%p2%dr"},
#  else
    {(int)KS_CS,	"\033[%i%d;%dr"},	/* scroll region */
#  endif
#endif
    {(int)KS_CL,	"\033[H\033[2J"},
#ifdef notyet
    {(int)KS_VI,	"[VI]"}, /* cursor invisible, VT320: CSI ? 25 l */
    {(int)KS_VE,	"[VE]"}, /* cursor visible, VT320: CSI ? 25 h */
#endif
    {(int)KS_ME,	"\033[m"},	/* normal mode */
    {(int)KS_MR,	"\033[7m"},	/* reverse */
    {(int)KS_MD,	"\033[1m"},	/* bold */
    {(int)KS_SO,	"\033[31m"},	/* standout mode: red */
    {(int)KS_SE,	"\033[m"},	/* standout end */
    {(int)KS_CZH,	"\033[35m"},	/* italic: purple */
    {(int)KS_CZR,	"\033[m"},	/* italic end */
    {(int)KS_US,	"\033[4m"},	/* underscore mode */
    {(int)KS_UE,	"\033[m"},	/* underscore end */
    {(int)KS_CCO,	"8"},		/* allow 8 colors */
#  ifdef TERMINFO
    {(int)KS_CAB,	"\033[4%p1%dm"},/* set background color */
    {(int)KS_CAF,	"\033[3%p1%dm"},/* set foreground color */
#  else
    {(int)KS_CAB,	"\033[4%dm"},	/* set background color */
    {(int)KS_CAF,	"\033[3%dm"},	/* set foreground color */
#  endif
    {(int)KS_OP,	"\033[m"},	/* reset colors */
    {(int)KS_MS,	"y"},		/* safe to move cur in reverse mode */
    {(int)KS_UT,	"y"},		/* guessed */
    {(int)KS_LE,	"\b"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033[%i%p1%d;%p2%dH"},
#  else
    {(int)KS_CM,	"\033[%i%d;%dH"},
#  endif
    {(int)KS_SR,	"\033M"},
#  ifdef TERMINFO
    {(int)KS_CRI,	"\033[%p1%dC"},
#  else
    {(int)KS_CRI,	"\033[%dC"},
#  endif
#  if defined(BEOS_DR8)
    {(int)KS_DB,	""},		/* hack! see screen.c */
#  endif

    {K_UP,		"\033[A"},
    {K_DOWN,		"\033[B"},
    {K_LEFT,		"\033[D"},
    {K_RIGHT,		"\033[C"},
# endif

# if defined(UNIX) || defined(ALL_BUILTIN_TCAPS) || defined(SOME_BUILTIN_TCAPS)
/*
 * standard ANSI terminal, default for unix
 */
    {(int)KS_NAME,	"ansi"},
    {(int)KS_CE,	IF_EB("\033[K", ESC_STR "[K")},
    {(int)KS_AL,	IF_EB("\033[L", ESC_STR "[L")},
#  ifdef TERMINFO
    {(int)KS_CAL,	IF_EB("\033[%p1%dL", ESC_STR "[%p1%dL")},
#  else
    {(int)KS_CAL,	IF_EB("\033[%dL", ESC_STR "[%dL")},
#  endif
    {(int)KS_DL,	IF_EB("\033[M", ESC_STR "[M")},
#  ifdef TERMINFO
    {(int)KS_CDL,	IF_EB("\033[%p1%dM", ESC_STR "[%p1%dM")},
#  else
    {(int)KS_CDL,	IF_EB("\033[%dM", ESC_STR "[%dM")},
#  endif
    {(int)KS_CL,	IF_EB("\033[H\033[2J", ESC_STR "[H" ESC_STR_nc "[2J")},
    {(int)KS_ME,	IF_EB("\033[0m", ESC_STR "[0m")},
    {(int)KS_MR,	IF_EB("\033[7m", ESC_STR "[7m")},
    {(int)KS_MS,	"y"},
    {(int)KS_UT,	"y"},		/* guessed */
    {(int)KS_LE,	"\b"},
#  ifdef TERMINFO
    {(int)KS_CM,	IF_EB("\033[%i%p1%d;%p2%dH", ESC_STR "[%i%p1%d;%p2%dH")},
#  else
    {(int)KS_CM,	IF_EB("\033[%i%d;%dH", ESC_STR "[%i%d;%dH")},
#  endif
#  ifdef TERMINFO
    {(int)KS_CRI,	IF_EB("\033[%p1%dC", ESC_STR "[%p1%dC")},
#  else
    {(int)KS_CRI,	IF_EB("\033[%dC", ESC_STR "[%dC")},
#  endif
# endif

# if defined(ALL_BUILTIN_TCAPS)
/*
 * These codes are valid when nansi.sys or equivalent has been installed.
 * Function keys on a PC are preceded with a NUL. These are converted into
 * K_NUL '\316' in mch_inchar(), because we cannot handle NULs in key codes.
 * CTRL-arrow is used instead of SHIFT-arrow.
 */
    {(int)KS_NAME,	"pcansi"},
    {(int)KS_DL,	"\033[M"},
    {(int)KS_AL,	"\033[L"},
    {(int)KS_CE,	"\033[K"},
    {(int)KS_CL,	"\033[2J"},
    {(int)KS_ME,	"\033[0m"},
    {(int)KS_MR,	"\033[5m"},	/* reverse: black on lightgrey */
    {(int)KS_MD,	"\033[1m"},	/* bold: white text */
    {(int)KS_SE,	"\033[0m"},	/* standout end */
    {(int)KS_SO,	"\033[31m"},	/* standout: white on blue */
    {(int)KS_CZH,	"\033[34;43m"},	/* italic mode: blue text on yellow */
    {(int)KS_CZR,	"\033[0m"},	/* italic mode end */
    {(int)KS_US,	"\033[36;41m"},	/* underscore mode: cyan text on red */
    {(int)KS_UE,	"\033[0m"},	/* underscore mode end */
    {(int)KS_CCO,	"8"},		/* allow 8 colors */
#  ifdef TERMINFO
    {(int)KS_CAB,	"\033[4%p1%dm"},/* set background color */
    {(int)KS_CAF,	"\033[3%p1%dm"},/* set foreground color */
#  else
    {(int)KS_CAB,	"\033[4%dm"},	/* set background color */
    {(int)KS_CAF,	"\033[3%dm"},	/* set foreground color */
#  endif
    {(int)KS_OP,	"\033[0m"},	/* reset colors */
    {(int)KS_MS,	"y"},
    {(int)KS_UT,	"y"},		/* guessed */
    {(int)KS_LE,	"\b"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033[%i%p1%d;%p2%dH"},
#  else
    {(int)KS_CM,	"\033[%i%d;%dH"},
#  endif
#  ifdef TERMINFO
    {(int)KS_CRI,	"\033[%p1%dC"},
#  else
    {(int)KS_CRI,	"\033[%dC"},
#  endif
    {K_UP,		"\316H"},
    {K_DOWN,		"\316P"},
    {K_LEFT,		"\316K"},
    {K_RIGHT,		"\316M"},
    {K_S_LEFT,		"\316s"},
    {K_S_RIGHT,		"\316t"},
    {K_F1,		"\316;"},
    {K_F2,		"\316<"},
    {K_F3,		"\316="},
    {K_F4,		"\316>"},
    {K_F5,		"\316?"},
    {K_F6,		"\316@"},
    {K_F7,		"\316A"},
    {K_F8,		"\316B"},
    {K_F9,		"\316C"},
    {K_F10,		"\316D"},
    {K_F11,		"\316\205"},	/* guessed */
    {K_F12,		"\316\206"},	/* guessed */
    {K_S_F1,		"\316T"},
    {K_S_F2,		"\316U"},
    {K_S_F3,		"\316V"},
    {K_S_F4,		"\316W"},
    {K_S_F5,		"\316X"},
    {K_S_F6,		"\316Y"},
    {K_S_F7,		"\316Z"},
    {K_S_F8,		"\316["},
    {K_S_F9,		"\316\\"},
    {K_S_F10,		"\316]"},
    {K_S_F11,		"\316\207"},	/* guessed */
    {K_S_F12,		"\316\210"},	/* guessed */
    {K_INS,		"\316R"},
    {K_DEL,		"\316S"},
    {K_HOME,		"\316G"},
    {K_END,		"\316O"},
    {K_PAGEDOWN,	"\316Q"},
    {K_PAGEUP,		"\316I"},
# endif

# if defined(MSWIN) || defined(ALL_BUILTIN_TCAPS)
/*
 * These codes are valid for the Win32 Console .  The entries that start with
 * ESC | are translated into console calls in os_win32.c.  The function keys
 * are also translated in os_win32.c.
 */
    {(int)KS_NAME,	"win32"},
    {(int)KS_CE,	"\033|K"},	// clear to end of line
    {(int)KS_AL,	"\033|L"},	// add new blank line
#  ifdef TERMINFO
    {(int)KS_CAL,	"\033|%p1%dL"},	// add number of new blank lines
#  else
    {(int)KS_CAL,	"\033|%dL"},	// add number of new blank lines
#  endif
    {(int)KS_DL,	"\033|M"},	// delete line
#  ifdef TERMINFO
    {(int)KS_CDL,	"\033|%p1%dM"},	// delete number of lines
    {(int)KS_CSV,	"\033|%p1%d;%p2%dV"},
#  else
    {(int)KS_CDL,	"\033|%dM"},	// delete number of lines
    {(int)KS_CSV,	"\033|%d;%dV"},
#  endif
    {(int)KS_CL,	"\033|J"},	// clear screen
    {(int)KS_CD,	"\033|j"},	// clear to end of display
    {(int)KS_VI,	"\033|v"},	// cursor invisible
    {(int)KS_VE,	"\033|V"},	// cursor visible

    {(int)KS_ME,	"\033|0m"},	// normal
    {(int)KS_MR,	"\033|112m"},	// reverse: black on lightgray
    {(int)KS_MD,	"\033|15m"},	// bold: white on black
#if 1
    {(int)KS_SO,	"\033|31m"},	// standout: white on blue
    {(int)KS_SE,	"\033|0m"},	// standout end
#else
    {(int)KS_SO,	"\033|F"},	// standout: high intensity
    {(int)KS_SE,	"\033|f"},	// standout end
#endif
    {(int)KS_CZH,	"\033|225m"},	// italic: blue text on yellow
    {(int)KS_CZR,	"\033|0m"},	// italic end
    {(int)KS_US,	"\033|67m"},	// underscore: cyan text on red
    {(int)KS_UE,	"\033|0m"},	// underscore end
    {(int)KS_CCO,	"16"},		// allow 16 colors
#  ifdef TERMINFO
    {(int)KS_CAB,	"\033|%p1%db"},	// set background color
    {(int)KS_CAF,	"\033|%p1%df"},	// set foreground color
#  else
    {(int)KS_CAB,	"\033|%db"},	// set background color
    {(int)KS_CAF,	"\033|%df"},	// set foreground color
#  endif

    {(int)KS_MS,	"y"},		// save to move cur in reverse mode
    {(int)KS_UT,	"y"},
    {(int)KS_XN,	"y"},
    {(int)KS_LE,	"\b"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033|%i%p1%d;%p2%dH"}, // cursor motion
#  else
    {(int)KS_CM,	"\033|%i%d;%dH"}, // cursor motion
#  endif
    {(int)KS_VB,	"\033|B"},	// visual bell
    {(int)KS_TI,	"\033|S"},	// put terminal in termcap mode
    {(int)KS_TE,	"\033|E"},	// out of termcap mode
#  ifdef TERMINFO
    {(int)KS_CS,	"\033|%i%p1%d;%p2%dr"}, // scroll region
#  else
    {(int)KS_CS,	"\033|%i%d;%dr"}, // scroll region
#  endif
#  ifdef FEAT_TERMGUICOLORS
    {(int)KS_8F,	"\033|38;2;%lu;%lu;%lum"},
    {(int)KS_8B,	"\033|48;2;%lu;%lu;%lum"},
#  endif

    {K_UP,		"\316H"},
    {K_DOWN,		"\316P"},
    {K_LEFT,		"\316K"},
    {K_RIGHT,		"\316M"},
    {K_S_UP,		"\316\304"},
    {K_S_DOWN,		"\316\317"},
    {K_S_LEFT,		"\316\311"},
    {K_C_LEFT,		"\316s"},
    {K_S_RIGHT,		"\316\313"},
    {K_C_RIGHT,		"\316t"},
    {K_S_TAB,		"\316\017"},
    {K_F1,		"\316;"},
    {K_F2,		"\316<"},
    {K_F3,		"\316="},
    {K_F4,		"\316>"},
    {K_F5,		"\316?"},
    {K_F6,		"\316@"},
    {K_F7,		"\316A"},
    {K_F8,		"\316B"},
    {K_F9,		"\316C"},
    {K_F10,		"\316D"},
    {K_F11,		"\316\205"},
    {K_F12,		"\316\206"},
    {K_S_F1,		"\316T"},
    {K_S_F2,		"\316U"},
    {K_S_F3,		"\316V"},
    {K_S_F4,		"\316W"},
    {K_S_F5,		"\316X"},
    {K_S_F6,		"\316Y"},
    {K_S_F7,		"\316Z"},
    {K_S_F8,		"\316["},
    {K_S_F9,		"\316\\"},
    {K_S_F10,		"\316]"},
    {K_S_F11,		"\316\207"},
    {K_S_F12,		"\316\210"},
    {K_INS,		"\316R"},
    {K_DEL,		"\316S"},
    {K_HOME,		"\316G"},
    {K_S_HOME,		"\316\302"},
    {K_C_HOME,		"\316w"},
    {K_END,		"\316O"},
    {K_S_END,		"\316\315"},
    {K_C_END,		"\316u"},
    {K_PAGEDOWN,	"\316Q"},
    {K_PAGEUP,		"\316I"},
    {K_KPLUS,		"\316N"},
    {K_KMINUS,		"\316J"},
    {K_KMULTIPLY,	"\316\067"},
    {K_K0,		"\316\332"},
    {K_K1,		"\316\336"},
    {K_K2,		"\316\342"},
    {K_K3,		"\316\346"},
    {K_K4,		"\316\352"},
    {K_K5,		"\316\356"},
    {K_K6,		"\316\362"},
    {K_K7,		"\316\366"},
    {K_K8,		"\316\372"},
    {K_K9,		"\316\376"},
    {K_BS,		"\316x"},
# endif

# if defined(VMS) || defined(ALL_BUILTIN_TCAPS)
/*
 * VT320 is working as an ANSI terminal compatible DEC terminal.
 * (it covers VT1x0, VT2x0 and VT3x0 up to VT320 on VMS as well)
 * TODO:- rewrite ESC[ codes to CSI
 *      - keyboard languages (CSI ? 26 n)
 */
    {(int)KS_NAME,	"vt320"},
    {(int)KS_CE,	IF_EB("\033[K", ESC_STR "[K")},
    {(int)KS_AL,	IF_EB("\033[L", ESC_STR "[L")},
#  ifdef TERMINFO
    {(int)KS_CAL,	IF_EB("\033[%p1%dL", ESC_STR "[%p1%dL")},
#  else
    {(int)KS_CAL,	IF_EB("\033[%dL", ESC_STR "[%dL")},
#  endif
    {(int)KS_DL,	IF_EB("\033[M", ESC_STR "[M")},
#  ifdef TERMINFO
    {(int)KS_CDL,	IF_EB("\033[%p1%dM", ESC_STR "[%p1%dM")},
#  else
    {(int)KS_CDL,	IF_EB("\033[%dM", ESC_STR "[%dM")},
#  endif
    {(int)KS_CL,	IF_EB("\033[H\033[2J", ESC_STR "[H" ESC_STR_nc "[2J")},
    {(int)KS_CD,	IF_EB("\033[J", ESC_STR "[J")},
    {(int)KS_CCO,	"8"},			/* allow 8 colors */
    {(int)KS_ME,	IF_EB("\033[0m", ESC_STR "[0m")},
    {(int)KS_MR,	IF_EB("\033[7m", ESC_STR "[7m")},
    {(int)KS_MD,	IF_EB("\033[1m", ESC_STR "[1m")},  /* bold mode */
    {(int)KS_SE,	IF_EB("\033[22m", ESC_STR "[22m")},/* normal mode */
    {(int)KS_UE,	IF_EB("\033[24m", ESC_STR "[24m")},/* exit underscore mode */
    {(int)KS_US,	IF_EB("\033[4m", ESC_STR "[4m")},  /* underscore mode */
    {(int)KS_CZH,	IF_EB("\033[34;43m", ESC_STR "[34;43m")},  /* italic mode: blue text on yellow */
    {(int)KS_CZR,	IF_EB("\033[0m", ESC_STR "[0m")},	    /* italic mode end */
    {(int)KS_CAB,	IF_EB("\033[4%dm", ESC_STR "[4%dm")},	    /* set background color (ANSI) */
    {(int)KS_CAF,	IF_EB("\033[3%dm", ESC_STR "[3%dm")},	    /* set foreground color (ANSI) */
    {(int)KS_CSB,	IF_EB("\033[102;%dm", ESC_STR "[102;%dm")},	/* set screen background color */
    {(int)KS_CSF,	IF_EB("\033[101;%dm", ESC_STR "[101;%dm")},	/* set screen foreground color */
    {(int)KS_MS,	"y"},
    {(int)KS_UT,	"y"},
    {(int)KS_XN,	"y"},
    {(int)KS_LE,	"\b"},
#  ifdef TERMINFO
    {(int)KS_CM,	IF_EB("\033[%i%p1%d;%p2%dH",
						  ESC_STR "[%i%p1%d;%p2%dH")},
#  else
    {(int)KS_CM,	IF_EB("\033[%i%d;%dH", ESC_STR "[%i%d;%dH")},
#  endif
#  ifdef TERMINFO
    {(int)KS_CRI,	IF_EB("\033[%p1%dC", ESC_STR "[%p1%dC")},
#  else
    {(int)KS_CRI,	IF_EB("\033[%dC", ESC_STR "[%dC")},
#  endif
    {K_UP,		IF_EB("\033[A", ESC_STR "[A")},
    {K_DOWN,		IF_EB("\033[B", ESC_STR "[B")},
    {K_RIGHT,		IF_EB("\033[C", ESC_STR "[C")},
    {K_LEFT,		IF_EB("\033[D", ESC_STR "[D")},
    // Note: cursor key sequences for application cursor mode are omitted,
    // because they interfere with typed commands: <Esc>OA.
    {K_F1,		IF_EB("\033[11~", ESC_STR "[11~")},
    {K_F2,		IF_EB("\033[12~", ESC_STR "[12~")},
    {K_F3,		IF_EB("\033[13~", ESC_STR "[13~")},
    {K_F4,		IF_EB("\033[14~", ESC_STR "[14~")},
    {K_F5,		IF_EB("\033[15~", ESC_STR "[15~")},
    {K_F6,		IF_EB("\033[17~", ESC_STR "[17~")},
    {K_F7,		IF_EB("\033[18~", ESC_STR "[18~")},
    {K_F8,		IF_EB("\033[19~", ESC_STR "[19~")},
    {K_F9,		IF_EB("\033[20~", ESC_STR "[20~")},
    {K_F10,		IF_EB("\033[21~", ESC_STR "[21~")},
    {K_F11,		IF_EB("\033[23~", ESC_STR "[23~")},
    {K_F12,		IF_EB("\033[24~", ESC_STR "[24~")},
    {K_F13,		IF_EB("\033[25~", ESC_STR "[25~")},
    {K_F14,		IF_EB("\033[26~", ESC_STR "[26~")},
    {K_F15,		IF_EB("\033[28~", ESC_STR "[28~")},	/* Help */
    {K_F16,		IF_EB("\033[29~", ESC_STR "[29~")},	/* Select */
    {K_F17,		IF_EB("\033[31~", ESC_STR "[31~")},
    {K_F18,		IF_EB("\033[32~", ESC_STR "[32~")},
    {K_F19,		IF_EB("\033[33~", ESC_STR "[33~")},
    {K_F20,		IF_EB("\033[34~", ESC_STR "[34~")},
    {K_INS,		IF_EB("\033[2~", ESC_STR "[2~")},
    {K_DEL,		IF_EB("\033[3~", ESC_STR "[3~")},
    {K_HOME,		IF_EB("\033[1~", ESC_STR "[1~")},
    {K_END,		IF_EB("\033[4~", ESC_STR "[4~")},
    {K_PAGEUP,		IF_EB("\033[5~", ESC_STR "[5~")},
    {K_PAGEDOWN,	IF_EB("\033[6~", ESC_STR "[6~")},
    // These sequences starting with <Esc> O may interfere with what the user
    // is typing.  Remove these if that bothers you.
    {K_KPLUS,		IF_EB("\033Ok", ESC_STR "Ok")},	/* keypad plus */
    {K_KMINUS,		IF_EB("\033Om", ESC_STR "Om")},	/* keypad minus */
    {K_KDIVIDE,		IF_EB("\033Oo", ESC_STR "Oo")},	/* keypad / */
    {K_KMULTIPLY,	IF_EB("\033Oj", ESC_STR "Oj")},	/* keypad * */
    {K_KENTER,		IF_EB("\033OM", ESC_STR "OM")},	/* keypad Enter */
    {K_K0,		IF_EB("\033Op", ESC_STR "Op")},	/* keypad 0 */
    {K_K1,		IF_EB("\033Oq", ESC_STR "Oq")},	/* keypad 1 */
    {K_K2,		IF_EB("\033Or", ESC_STR "Or")},	/* keypad 2 */
    {K_K3,		IF_EB("\033Os", ESC_STR "Os")},	/* keypad 3 */
    {K_K4,		IF_EB("\033Ot", ESC_STR "Ot")},	/* keypad 4 */
    {K_K5,		IF_EB("\033Ou", ESC_STR "Ou")},	/* keypad 5 */
    {K_K6,		IF_EB("\033Ov", ESC_STR "Ov")},	/* keypad 6 */
    {K_K7,		IF_EB("\033Ow", ESC_STR "Ow")},	/* keypad 7 */
    {K_K8,		IF_EB("\033Ox", ESC_STR "Ox")},	/* keypad 8 */
    {K_K9,		IF_EB("\033Oy", ESC_STR "Oy")},	/* keypad 9 */
    {K_BS,		"\x7f"},	/* for some reason 0177 doesn't work */
# endif

# if defined(ALL_BUILTIN_TCAPS) || defined(__MINT__)
/*
 * Ordinary vt52
 */
    {(int)KS_NAME,	"vt52"},
    {(int)KS_CE,	IF_EB("\033K", ESC_STR "K")},
    {(int)KS_CD,	IF_EB("\033J", ESC_STR "J")},
#  ifdef TERMINFO
    {(int)KS_CM,	IF_EB("\033Y%p1%' '%+%c%p2%' '%+%c",
			    ESC_STR "Y%p1%' '%+%c%p2%' '%+%c")},
#  else
    {(int)KS_CM,	IF_EB("\033Y%+ %+ ", ESC_STR "Y%+ %+ ")},
#  endif
    {(int)KS_LE,	"\b"},
    {(int)KS_SR,	IF_EB("\033I", ESC_STR "I")},
    {(int)KS_AL,	IF_EB("\033L", ESC_STR "L")},
    {(int)KS_DL,	IF_EB("\033M", ESC_STR "M")},
    {K_UP,		IF_EB("\033A", ESC_STR "A")},
    {K_DOWN,		IF_EB("\033B", ESC_STR "B")},
    {K_LEFT,		IF_EB("\033D", ESC_STR "D")},
    {K_RIGHT,		IF_EB("\033C", ESC_STR "C")},
    {K_F1,		IF_EB("\033P", ESC_STR "P")},
    {K_F2,		IF_EB("\033Q", ESC_STR "Q")},
    {K_F3,		IF_EB("\033R", ESC_STR "R")},
#  ifdef __MINT__
    {(int)KS_CL,	IF_EB("\033E", ESC_STR "E")},
    {(int)KS_VE,	IF_EB("\033e", ESC_STR "e")},
    {(int)KS_VI,	IF_EB("\033f", ESC_STR "f")},
    {(int)KS_SO,	IF_EB("\033p", ESC_STR "p")},
    {(int)KS_SE,	IF_EB("\033q", ESC_STR "q")},
    {K_S_UP,		IF_EB("\033a", ESC_STR "a")},
    {K_S_DOWN,		IF_EB("\033b", ESC_STR "b")},
    {K_S_LEFT,		IF_EB("\033d", ESC_STR "d")},
    {K_S_RIGHT,		IF_EB("\033c", ESC_STR "c")},
    {K_F4,		IF_EB("\033S", ESC_STR "S")},
    {K_F5,		IF_EB("\033T", ESC_STR "T")},
    {K_F6,		IF_EB("\033U", ESC_STR "U")},
    {K_F7,		IF_EB("\033V", ESC_STR "V")},
    {K_F8,		IF_EB("\033W", ESC_STR "W")},
    {K_F9,		IF_EB("\033X", ESC_STR "X")},
    {K_F10,		IF_EB("\033Y", ESC_STR "Y")},
    {K_S_F1,		IF_EB("\033p", ESC_STR "p")},
    {K_S_F2,		IF_EB("\033q", ESC_STR "q")},
    {K_S_F3,		IF_EB("\033r", ESC_STR "r")},
    {K_S_F4,		IF_EB("\033s", ESC_STR "s")},
    {K_S_F5,		IF_EB("\033t", ESC_STR "t")},
    {K_S_F6,		IF_EB("\033u", ESC_STR "u")},
    {K_S_F7,		IF_EB("\033v", ESC_STR "v")},
    {K_S_F8,		IF_EB("\033w", ESC_STR "w")},
    {K_S_F9,		IF_EB("\033x", ESC_STR "x")},
    {K_S_F10,		IF_EB("\033y", ESC_STR "y")},
    {K_INS,		IF_EB("\033I", ESC_STR "I")},
    {K_HOME,		IF_EB("\033E", ESC_STR "E")},
    {K_PAGEDOWN,	IF_EB("\033b", ESC_STR "b")},
    {K_PAGEUP,		IF_EB("\033a", ESC_STR "a")},
#  else
    {(int)KS_CL,	IF_EB("\033H\033J", ESC_STR "H" ESC_STR_nc "J")},
    {(int)KS_MS,	"y"},
#  endif
# endif

# if defined(UNIX) || defined(ALL_BUILTIN_TCAPS) || defined(SOME_BUILTIN_TCAPS)
    {(int)KS_NAME,	"xterm"},
    {(int)KS_CE,	IF_EB("\033[K", ESC_STR "[K")},
    {(int)KS_AL,	IF_EB("\033[L", ESC_STR "[L")},
#  ifdef TERMINFO
    {(int)KS_CAL,	IF_EB("\033[%p1%dL", ESC_STR "[%p1%dL")},
#  else
    {(int)KS_CAL,	IF_EB("\033[%dL", ESC_STR "[%dL")},
#  endif
    {(int)KS_DL,	IF_EB("\033[M", ESC_STR "[M")},
#  ifdef TERMINFO
    {(int)KS_CDL,	IF_EB("\033[%p1%dM", ESC_STR "[%p1%dM")},
#  else
    {(int)KS_CDL,	IF_EB("\033[%dM", ESC_STR "[%dM")},
#  endif
#  ifdef TERMINFO
    {(int)KS_CS,	IF_EB("\033[%i%p1%d;%p2%dr",
						  ESC_STR "[%i%p1%d;%p2%dr")},
#  else
    {(int)KS_CS,	IF_EB("\033[%i%d;%dr", ESC_STR "[%i%d;%dr")},
#  endif
    {(int)KS_CL,	IF_EB("\033[H\033[2J", ESC_STR "[H" ESC_STR_nc "[2J")},
    {(int)KS_CD,	IF_EB("\033[J", ESC_STR "[J")},
    {(int)KS_ME,	IF_EB("\033[m", ESC_STR "[m")},
    {(int)KS_MR,	IF_EB("\033[7m", ESC_STR "[7m")},
    {(int)KS_MD,	IF_EB("\033[1m", ESC_STR "[1m")},
    {(int)KS_UE,	IF_EB("\033[m", ESC_STR "[m")},
    {(int)KS_US,	IF_EB("\033[4m", ESC_STR "[4m")},
    {(int)KS_STE,	IF_EB("\033[29m", ESC_STR "[29m")},
    {(int)KS_STS,	IF_EB("\033[9m", ESC_STR "[9m")},
    {(int)KS_MS,	"y"},
    {(int)KS_UT,	"y"},
    {(int)KS_LE,	"\b"},
    {(int)KS_VI,	IF_EB("\033[?25l", ESC_STR "[?25l")},
    {(int)KS_VE,	IF_EB("\033[?25h", ESC_STR "[?25h")},
    {(int)KS_VS,	IF_EB("\033[?12h", ESC_STR "[?12h")},
    {(int)KS_CVS,	IF_EB("\033[?12l", ESC_STR "[?12l")},
#  ifdef TERMINFO
    {(int)KS_CSH,	IF_EB("\033[%p1%d q", ESC_STR "[%p1%d q")},
#  else
    {(int)KS_CSH,	IF_EB("\033[%d q", ESC_STR "[%d q")},
#  endif
    {(int)KS_CRC,	IF_EB("\033[?12$p", ESC_STR "[?12$p")},
    {(int)KS_CRS,	IF_EB("\033P$q q\033\\", ESC_STR "P$q q" ESC_STR "\\")},
#  ifdef TERMINFO
    {(int)KS_CM,	IF_EB("\033[%i%p1%d;%p2%dH",
						  ESC_STR "[%i%p1%d;%p2%dH")},
#  else
    {(int)KS_CM,	IF_EB("\033[%i%d;%dH", ESC_STR "[%i%d;%dH")},
#  endif
    {(int)KS_SR,	IF_EB("\033M", ESC_STR "M")},
#  ifdef TERMINFO
    {(int)KS_CRI,	IF_EB("\033[%p1%dC", ESC_STR "[%p1%dC")},
#  else
    {(int)KS_CRI,	IF_EB("\033[%dC", ESC_STR "[%dC")},
#  endif
    {(int)KS_KS,	IF_EB("\033[?1h\033=", ESC_STR "[?1h" ESC_STR_nc "=")},
    {(int)KS_KE,	IF_EB("\033[?1l\033>", ESC_STR "[?1l" ESC_STR_nc ">")},
#  ifdef FEAT_XTERM_SAVE
    {(int)KS_TI,	IF_EB("\0337\033[?47h", ESC_STR "7" ESC_STR_nc "[?47h")},
    {(int)KS_TE,	IF_EB("\033[2J\033[?47l\0338",
				  ESC_STR "[2J" ESC_STR_nc "[?47l" ESC_STR_nc "8")},
#  endif
    {(int)KS_CIS,	IF_EB("\033]1;", ESC_STR "]1;")},
    {(int)KS_CIE,	"\007"},
    {(int)KS_TS,	IF_EB("\033]2;", ESC_STR "]2;")},
    {(int)KS_FS,	"\007"},
    {(int)KS_CSC,	IF_EB("\033]12;", ESC_STR "]12;")},
    {(int)KS_CEC,	"\007"},
#  ifdef TERMINFO
    {(int)KS_CWS,	IF_EB("\033[8;%p1%d;%p2%dt",
						  ESC_STR "[8;%p1%d;%p2%dt")},
    {(int)KS_CWP,	IF_EB("\033[3;%p1%d;%p2%dt",
						  ESC_STR "[3;%p1%d;%p2%dt")},
    {(int)KS_CGP,	IF_EB("\033[13t", ESC_STR "[13t")},
#  else
    {(int)KS_CWS,	IF_EB("\033[8;%d;%dt", ESC_STR "[8;%d;%dt")},
    {(int)KS_CWP,	IF_EB("\033[3;%d;%dt", ESC_STR "[3;%d;%dt")},
    {(int)KS_CGP,	IF_EB("\033[13t", ESC_STR "[13t")},
#  endif
    {(int)KS_CRV,	IF_EB("\033[>c", ESC_STR "[>c")},
    {(int)KS_RFG,	IF_EB("\033]10;?\007", ESC_STR "]10;?\007")},
    {(int)KS_RBG,	IF_EB("\033]11;?\007", ESC_STR "]11;?\007")},
    {(int)KS_U7,	IF_EB("\033[6n", ESC_STR "[6n")},
#  ifdef FEAT_TERMGUICOLORS
    /* These are printf strings, not terminal codes. */
    {(int)KS_8F,	IF_EB("\033[38;2;%lu;%lu;%lum", ESC_STR "[38;2;%lu;%lu;%lum")},
    {(int)KS_8B,	IF_EB("\033[48;2;%lu;%lu;%lum", ESC_STR "[48;2;%lu;%lu;%lum")},
#  endif
    {(int)KS_CBE,	IF_EB("\033[?2004h", ESC_STR "[?2004h")},
    {(int)KS_CBD,	IF_EB("\033[?2004l", ESC_STR "[?2004l")},
    {(int)KS_CST,	IF_EB("\033[22;2t", ESC_STR "[22;2t")},
    {(int)KS_CRT,	IF_EB("\033[23;2t", ESC_STR "[23;2t")},
    {(int)KS_SSI,	IF_EB("\033[22;1t", ESC_STR "[22;1t")},
    {(int)KS_SRI,	IF_EB("\033[23;1t", ESC_STR "[23;1t")},

    {K_UP,		IF_EB("\033O*A", ESC_STR "O*A")},
    {K_DOWN,		IF_EB("\033O*B", ESC_STR "O*B")},
    {K_RIGHT,		IF_EB("\033O*C", ESC_STR "O*C")},
    {K_LEFT,		IF_EB("\033O*D", ESC_STR "O*D")},
    /* An extra set of cursor keys for vt100 mode */
    {K_XUP,		IF_EB("\033[1;*A", ESC_STR "[1;*A")},
    {K_XDOWN,		IF_EB("\033[1;*B", ESC_STR "[1;*B")},
    {K_XRIGHT,		IF_EB("\033[1;*C", ESC_STR "[1;*C")},
    {K_XLEFT,		IF_EB("\033[1;*D", ESC_STR "[1;*D")},
    /* An extra set of function keys for vt100 mode */
    {K_XF1,		IF_EB("\033O*P", ESC_STR "O*P")},
    {K_XF2,		IF_EB("\033O*Q", ESC_STR "O*Q")},
    {K_XF3,		IF_EB("\033O*R", ESC_STR "O*R")},
    {K_XF4,		IF_EB("\033O*S", ESC_STR "O*S")},
    {K_F1,		IF_EB("\033[11;*~", ESC_STR "[11;*~")},
    {K_F2,		IF_EB("\033[12;*~", ESC_STR "[12;*~")},
    {K_F3,		IF_EB("\033[13;*~", ESC_STR "[13;*~")},
    {K_F4,		IF_EB("\033[14;*~", ESC_STR "[14;*~")},
    {K_F5,		IF_EB("\033[15;*~", ESC_STR "[15;*~")},
    {K_F6,		IF_EB("\033[17;*~", ESC_STR "[17;*~")},
    {K_F7,		IF_EB("\033[18;*~", ESC_STR "[18;*~")},
    {K_F8,		IF_EB("\033[19;*~", ESC_STR "[19;*~")},
    {K_F9,		IF_EB("\033[20;*~", ESC_STR "[20;*~")},
    {K_F10,		IF_EB("\033[21;*~", ESC_STR "[21;*~")},
    {K_F11,		IF_EB("\033[23;*~", ESC_STR "[23;*~")},
    {K_F12,		IF_EB("\033[24;*~", ESC_STR "[24;*~")},
    {K_S_TAB,		IF_EB("\033[Z", ESC_STR "[Z")},
    {K_HELP,		IF_EB("\033[28;*~", ESC_STR "[28;*~")},
    {K_UNDO,		IF_EB("\033[26;*~", ESC_STR "[26;*~")},
    {K_INS,		IF_EB("\033[2;*~", ESC_STR "[2;*~")},
    {K_HOME,		IF_EB("\033[1;*H", ESC_STR "[1;*H")},
    /* {K_S_HOME,		IF_EB("\033O2H", ESC_STR "O2H")}, */
    /* {K_C_HOME,		IF_EB("\033O5H", ESC_STR "O5H")}, */
    {K_KHOME,		IF_EB("\033[1;*~", ESC_STR "[1;*~")},
    {K_XHOME,		IF_EB("\033O*H", ESC_STR "O*H")},	/* other Home */
    {K_ZHOME,		IF_EB("\033[7;*~", ESC_STR "[7;*~")},	/* other Home */
    {K_END,		IF_EB("\033[1;*F", ESC_STR "[1;*F")},
    /* {K_S_END,		IF_EB("\033O2F", ESC_STR "O2F")}, */
    /* {K_C_END,		IF_EB("\033O5F", ESC_STR "O5F")}, */
    {K_KEND,		IF_EB("\033[4;*~", ESC_STR "[4;*~")},
    {K_XEND,		IF_EB("\033O*F", ESC_STR "O*F")},	/* other End */
    {K_ZEND,		IF_EB("\033[8;*~", ESC_STR "[8;*~")},
    {K_PAGEUP,		IF_EB("\033[5;*~", ESC_STR "[5;*~")},
    {K_PAGEDOWN,	IF_EB("\033[6;*~", ESC_STR "[6;*~")},
    {K_KPLUS,		IF_EB("\033O*k", ESC_STR "O*k")},     /* keypad plus */
    {K_KMINUS,		IF_EB("\033O*m", ESC_STR "O*m")},     /* keypad minus */
    {K_KDIVIDE,		IF_EB("\033O*o", ESC_STR "O*o")},     /* keypad / */
    {K_KMULTIPLY,	IF_EB("\033O*j", ESC_STR "O*j")},     /* keypad * */
    {K_KENTER,		IF_EB("\033O*M", ESC_STR "O*M")},     /* keypad Enter */
    {K_KPOINT,		IF_EB("\033O*n", ESC_STR "O*n")},     /* keypad . */
    {K_K0,		IF_EB("\033O*p", ESC_STR "O*p")},     /* keypad 0 */
    {K_K1,		IF_EB("\033O*q", ESC_STR "O*q")},     /* keypad 1 */
    {K_K2,		IF_EB("\033O*r", ESC_STR "O*r")},     /* keypad 2 */
    {K_K3,		IF_EB("\033O*s", ESC_STR "O*s")},     /* keypad 3 */
    {K_K4,		IF_EB("\033O*t", ESC_STR "O*t")},     /* keypad 4 */
    {K_K5,		IF_EB("\033O*u", ESC_STR "O*u")},     /* keypad 5 */
    {K_K6,		IF_EB("\033O*v", ESC_STR "O*v")},     /* keypad 6 */
    {K_K7,		IF_EB("\033O*w", ESC_STR "O*w")},     /* keypad 7 */
    {K_K8,		IF_EB("\033O*x", ESC_STR "O*x")},     /* keypad 8 */
    {K_K9,		IF_EB("\033O*y", ESC_STR "O*y")},     /* keypad 9 */
    {K_KDEL,		IF_EB("\033[3;*~", ESC_STR "[3;*~")}, /* keypad Del */
    {K_PS,		IF_EB("\033[200~", ESC_STR "[200~")}, /* paste start */
    {K_PE,		IF_EB("\033[201~", ESC_STR "[201~")}, /* paste end */

    {BT_EXTRA_KEYS,   ""},
    {TERMCAP2KEY('k', '0'), IF_EB("\033[10;*~", ESC_STR "[10;*~")}, /* F0 */
    {TERMCAP2KEY('F', '3'), IF_EB("\033[25;*~", ESC_STR "[25;*~")}, /* F13 */
    /* F14 and F15 are missing, because they send the same codes as the undo
     * and help key, although they don't work on all keyboards. */
    {TERMCAP2KEY('F', '6'), IF_EB("\033[29;*~", ESC_STR "[29;*~")}, /* F16 */
    {TERMCAP2KEY('F', '7'), IF_EB("\033[31;*~", ESC_STR "[31;*~")}, /* F17 */
    {TERMCAP2KEY('F', '8'), IF_EB("\033[32;*~", ESC_STR "[32;*~")}, /* F18 */
    {TERMCAP2KEY('F', '9'), IF_EB("\033[33;*~", ESC_STR "[33;*~")}, /* F19 */
    {TERMCAP2KEY('F', 'A'), IF_EB("\033[34;*~", ESC_STR "[34;*~")}, /* F20 */

    {TERMCAP2KEY('F', 'B'), IF_EB("\033[42;*~", ESC_STR "[42;*~")}, /* F21 */
    {TERMCAP2KEY('F', 'C'), IF_EB("\033[43;*~", ESC_STR "[43;*~")}, /* F22 */
    {TERMCAP2KEY('F', 'D'), IF_EB("\033[44;*~", ESC_STR "[44;*~")}, /* F23 */
    {TERMCAP2KEY('F', 'E'), IF_EB("\033[45;*~", ESC_STR "[45;*~")}, /* F24 */
    {TERMCAP2KEY('F', 'F'), IF_EB("\033[46;*~", ESC_STR "[46;*~")}, /* F25 */
    {TERMCAP2KEY('F', 'G'), IF_EB("\033[47;*~", ESC_STR "[47;*~")}, /* F26 */
    {TERMCAP2KEY('F', 'H'), IF_EB("\033[48;*~", ESC_STR "[48;*~")}, /* F27 */
    {TERMCAP2KEY('F', 'I'), IF_EB("\033[49;*~", ESC_STR "[49;*~")}, /* F28 */
    {TERMCAP2KEY('F', 'J'), IF_EB("\033[50;*~", ESC_STR "[50;*~")}, /* F29 */
    {TERMCAP2KEY('F', 'K'), IF_EB("\033[51;*~", ESC_STR "[51;*~")}, /* F30 */

    {TERMCAP2KEY('F', 'L'), IF_EB("\033[52;*~", ESC_STR "[52;*~")}, /* F31 */
    {TERMCAP2KEY('F', 'M'), IF_EB("\033[53;*~", ESC_STR "[53;*~")}, /* F32 */
    {TERMCAP2KEY('F', 'N'), IF_EB("\033[54;*~", ESC_STR "[54;*~")}, /* F33 */
    {TERMCAP2KEY('F', 'O'), IF_EB("\033[55;*~", ESC_STR "[55;*~")}, /* F34 */
    {TERMCAP2KEY('F', 'P'), IF_EB("\033[56;*~", ESC_STR "[56;*~")}, /* F35 */
    {TERMCAP2KEY('F', 'Q'), IF_EB("\033[57;*~", ESC_STR "[57;*~")}, /* F36 */
    {TERMCAP2KEY('F', 'R'), IF_EB("\033[58;*~", ESC_STR "[58;*~")}, /* F37 */
# endif

# if defined(UNIX) || defined(ALL_BUILTIN_TCAPS)
/*
 * iris-ansi for Silicon Graphics machines.
 */
    {(int)KS_NAME,	"iris-ansi"},
    {(int)KS_CE,	"\033[K"},
    {(int)KS_CD,	"\033[J"},
    {(int)KS_AL,	"\033[L"},
#  ifdef TERMINFO
    {(int)KS_CAL,	"\033[%p1%dL"},
#  else
    {(int)KS_CAL,	"\033[%dL"},
#  endif
    {(int)KS_DL,	"\033[M"},
#  ifdef TERMINFO
    {(int)KS_CDL,	"\033[%p1%dM"},
#  else
    {(int)KS_CDL,	"\033[%dM"},
#  endif
#if 0	/* The scroll region is not working as Vim expects. */
#  ifdef TERMINFO
    {(int)KS_CS,	"\033[%i%p1%d;%p2%dr"},
#  else
    {(int)KS_CS,	"\033[%i%d;%dr"},
#  endif
#endif
    {(int)KS_CL,	"\033[H\033[2J"},
    {(int)KS_VE,	"\033[9/y\033[12/y"},	/* These aren't documented */
    {(int)KS_VS,	"\033[10/y\033[=1h\033[=2l"}, /* These aren't documented */
    {(int)KS_TI,	"\033[=6h"},
    {(int)KS_TE,	"\033[=6l"},
    {(int)KS_SE,	"\033[21;27m"},
    {(int)KS_SO,	"\033[1;7m"},
    {(int)KS_ME,	"\033[m"},
    {(int)KS_MR,	"\033[7m"},
    {(int)KS_MD,	"\033[1m"},
    {(int)KS_CCO,	"8"},			/* allow 8 colors */
    {(int)KS_CZH,	"\033[3m"},		/* italic mode on */
    {(int)KS_CZR,	"\033[23m"},		/* italic mode off */
    {(int)KS_US,	"\033[4m"},		/* underline on */
    {(int)KS_UE,	"\033[24m"},		/* underline off */
#  ifdef TERMINFO
    {(int)KS_CAB,	"\033[4%p1%dm"},    /* set background color (ANSI) */
    {(int)KS_CAF,	"\033[3%p1%dm"},    /* set foreground color (ANSI) */
    {(int)KS_CSB,	"\033[102;%p1%dm"}, /* set screen background color */
    {(int)KS_CSF,	"\033[101;%p1%dm"}, /* set screen foreground color */
#  else
    {(int)KS_CAB,	"\033[4%dm"},	    /* set background color (ANSI) */
    {(int)KS_CAF,	"\033[3%dm"},	    /* set foreground color (ANSI) */
    {(int)KS_CSB,	"\033[102;%dm"},    /* set screen background color */
    {(int)KS_CSF,	"\033[101;%dm"},    /* set screen foreground color */
#  endif
    {(int)KS_MS,	"y"},		/* guessed */
    {(int)KS_UT,	"y"},		/* guessed */
    {(int)KS_LE,	"\b"},
#  ifdef TERMINFO
    {(int)KS_CM,	"\033[%i%p1%d;%p2%dH"},
#  else
    {(int)KS_CM,	"\033[%i%d;%dH"},
#  endif
    {(int)KS_SR,	"\033M"},
#  ifdef TERMINFO
    {(int)KS_CRI,	"\033[%p1%dC"},
#  else
    {(int)KS_CRI,	"\033[%dC"},
#  endif
    {(int)KS_CIS,	"\033P3.y"},
    {(int)KS_CIE,	"\234"},    /* ST "String Terminator" */
    {(int)KS_TS,	"\033P1.y"},
    {(int)KS_FS,	"\234"},    /* ST "String Terminator" */
#  ifdef TERMINFO
    {(int)KS_CWS,	"\033[203;%p1%d;%p2%d/y"},
    {(int)KS_CWP,	"\033[205;%p1%d;%p2%d/y"},
#  else
    {(int)KS_CWS,	"\033[203;%d;%d/y"},
    {(int)KS_CWP,	"\033[205;%d;%d/y"},
#  endif
    {K_UP,		"\033[A"},
    {K_DOWN,		"\033[B"},
    {K_LEFT,		"\033[D"},
    {K_RIGHT,		"\033[C"},
    {K_S_UP,		"\033[161q"},
    {K_S_DOWN,		"\033[164q"},
    {K_S_LEFT,		"\033[158q"},
    {K_S_RIGHT,		"\033[167q"},
    {K_F1,		"\033[001q"},
    {K_F2,		"\033[002q"},
    {K_F3,		"\033[003q"},
    {K_F4,		"\033[004q"},
    {K_F5,		"\033[005q"},
    {K_F6,		"\033[006q"},
    {K_F7,		"\033[007q"},
    {K_F8,		"\033[008q"},
    {K_F9,		"\033[009q"},
    {K_F10,		"\033[010q"},
    {K_F11,		"\033[011q"},
    {K_F12,		"\033[012q"},
    {K_S_F1,		"\033[013q"},
    {K_S_F2,		"\033[014q"},
    {K_S_F3,		"\033[015q"},
    {K_S_F4,		"\033[016q"},
    {K_S_F5,		"\033[017q"},
    {K_S_F6,		"\033[018q"},
    {K_S_F7,		"\033[019q"},
    {K_S_F8,		"\033[020q"},
    {K_S_F9,		"\033[021q"},
    {K_S_F10,		"\033[022q"},
    {K_S_F11,		"\033[023q"},
    {K_S_F12,		"\033[024q"},
    {K_INS,		"\033[139q"},
    {K_HOME,		"\033[H"},
    {K_END,		"\033[146q"},
    {K_PAGEUP,		"\033[150q"},
    {K_PAGEDOWN,	"\033[154q"},
# endif

# if defined(DEBUG) || defined(ALL_BUILTIN_TCAPS)
/*
 * for debugging
 */
    {(int)KS_NAME,	"debug"},
    {(int)KS_CE,	"[CE]"},
    {(int)KS_CD,	"[CD]"},
    {(int)KS_AL,	"[AL]"},
#  ifdef TERMINFO
    {(int)KS_CAL,	"[CAL%p1%d]"},
#  else
    {(int)KS_CAL,	"[CAL%d]"},
#  endif
    {(int)KS_DL,	"[DL]"},
#  ifdef TERMINFO
    {(int)KS_CDL,	"[CDL%p1%d]"},
#  else
    {(int)KS_CDL,	"[CDL%d]"},
#  endif
#  ifdef TERMINFO
    {(int)KS_CS,	"[%p1%dCS%p2%d]"},
#  else
    {(int)KS_CS,	"[%dCS%d]"},
#  endif
#  ifdef TERMINFO
    {(int)KS_CSV,	"[%p1%dCSV%p2%d]"},
#  else
    {(int)KS_CSV,	"[%dCSV%d]"},
#  endif
#  ifdef TERMINFO
    {(int)KS_CAB,	"[CAB%p1%d]"},
    {(int)KS_CAF,	"[CAF%p1%d]"},
    {(int)KS_CSB,	"[CSB%p1%d]"},
    {(int)KS_CSF,	"[CSF%p1%d]"},
#  else
    {(int)KS_CAB,	"[CAB%d]"},
    {(int)KS_CAF,	"[CAF%d]"},
    {(int)KS_CSB,	"[CSB%d]"},
    {(int)KS_CSF,	"[CSF%d]"},
#  endif
    {(int)KS_OP,	"[OP]"},
    {(int)KS_LE,	"[LE]"},
    {(int)KS_CL,	"[CL]"},
    {(int)KS_VI,	"[VI]"},
    {(int)KS_VE,	"[VE]"},
    {(int)KS_VS,	"[VS]"},
    {(int)KS_ME,	"[ME]"},
    {(int)KS_MR,	"[MR]"},
    {(int)KS_MB,	"[MB]"},
    {(int)KS_MD,	"[MD]"},
    {(int)KS_SE,	"[SE]"},
    {(int)KS_SO,	"[SO]"},
    {(int)KS_UE,	"[UE]"},
    {(int)KS_US,	"[US]"},
    {(int)KS_UCE,	"[UCE]"},
    {(int)KS_UCS,	"[UCS]"},
    {(int)KS_STE,	"[STE]"},
    {(int)KS_STS,	"[STS]"},
    {(int)KS_MS,	"[MS]"},
    {(int)KS_UT,	"[UT]"},
    {(int)KS_XN,	"[XN]"},
#  ifdef TERMINFO
    {(int)KS_CM,	"[%p1%dCM%p2%d]"},
#  else
    {(int)KS_CM,	"[%dCM%d]"},
#  endif
    {(int)KS_SR,	"[SR]"},
#  ifdef TERMINFO
    {(int)KS_CRI,	"[CRI%p1%d]"},
#  else
    {(int)KS_CRI,	"[CRI%d]"},
#  endif
    {(int)KS_VB,	"[VB]"},
    {(int)KS_KS,	"[KS]"},
    {(int)KS_KE,	"[KE]"},
    {(int)KS_TI,	"[TI]"},
    {(int)KS_TE,	"[TE]"},
    {(int)KS_CIS,	"[CIS]"},
    {(int)KS_CIE,	"[CIE]"},
    {(int)KS_CSC,	"[CSC]"},
    {(int)KS_CEC,	"[CEC]"},
    {(int)KS_TS,	"[TS]"},
    {(int)KS_FS,	"[FS]"},
#  ifdef TERMINFO
    {(int)KS_CWS,	"[%p1%dCWS%p2%d]"},
    {(int)KS_CWP,	"[%p1%dCWP%p2%d]"},
#  else
    {(int)KS_CWS,	"[%dCWS%d]"},
    {(int)KS_CWP,	"[%dCWP%d]"},
#  endif
    {(int)KS_CRV,	"[CRV]"},
    {(int)KS_U7,	"[U7]"},
    {(int)KS_RFG,	"[RFG]"},
    {(int)KS_RBG,	"[RBG]"},
    {K_UP,		"[KU]"},
    {K_DOWN,		"[KD]"},
    {K_LEFT,		"[KL]"},
    {K_RIGHT,		"[KR]"},
    {K_XUP,		"[xKU]"},
    {K_XDOWN,		"[xKD]"},
    {K_XLEFT,		"[xKL]"},
    {K_XRIGHT,		"[xKR]"},
    {K_S_UP,		"[S-KU]"},
    {K_S_DOWN,		"[S-KD]"},
    {K_S_LEFT,		"[S-KL]"},
    {K_C_LEFT,		"[C-KL]"},
    {K_S_RIGHT,		"[S-KR]"},
    {K_C_RIGHT,		"[C-KR]"},
    {K_F1,		"[F1]"},
    {K_XF1,		"[xF1]"},
    {K_F2,		"[F2]"},
    {K_XF2,		"[xF2]"},
    {K_F3,		"[F3]"},
    {K_XF3,		"[xF3]"},
    {K_F4,		"[F4]"},
    {K_XF4,		"[xF4]"},
    {K_F5,		"[F5]"},
    {K_F6,		"[F6]"},
    {K_F7,		"[F7]"},
    {K_F8,		"[F8]"},
    {K_F9,		"[F9]"},
    {K_F10,		"[F10]"},
    {K_F11,		"[F11]"},
    {K_F12,		"[F12]"},
    {K_S_F1,		"[S-F1]"},
    {K_S_XF1,		"[S-xF1]"},
    {K_S_F2,		"[S-F2]"},
    {K_S_XF2,		"[S-xF2]"},
    {K_S_F3,		"[S-F3]"},
    {K_S_XF3,		"[S-xF3]"},
    {K_S_F4,		"[S-F4]"},
    {K_S_XF4,		"[S-xF4]"},
    {K_S_F5,		"[S-F5]"},
    {K_S_F6,		"[S-F6]"},
    {K_S_F7,		"[S-F7]"},
    {K_S_F8,		"[S-F8]"},
    {K_S_F9,		"[S-F9]"},
    {K_S_F10,		"[S-F10]"},
    {K_S_F11,		"[S-F11]"},
    {K_S_F12,		"[S-F12]"},
    {K_HELP,		"[HELP]"},
    {K_UNDO,		"[UNDO]"},
    {K_BS,		"[BS]"},
    {K_INS,		"[INS]"},
    {K_KINS,		"[KINS]"},
    {K_DEL,		"[DEL]"},
    {K_KDEL,		"[KDEL]"},
    {K_HOME,		"[HOME]"},
    {K_S_HOME,		"[C-HOME]"},
    {K_C_HOME,		"[C-HOME]"},
    {K_KHOME,		"[KHOME]"},
    {K_XHOME,		"[XHOME]"},
    {K_ZHOME,		"[ZHOME]"},
    {K_END,		"[END]"},
    {K_S_END,		"[C-END]"},
    {K_C_END,		"[C-END]"},
    {K_KEND,		"[KEND]"},
    {K_XEND,		"[XEND]"},
    {K_ZEND,		"[ZEND]"},
    {K_PAGEUP,		"[PAGEUP]"},
    {K_PAGEDOWN,	"[PAGEDOWN]"},
    {K_KPAGEUP,		"[KPAGEUP]"},
    {K_KPAGEDOWN,	"[KPAGEDOWN]"},
    {K_MOUSE,		"[MOUSE]"},
    {K_KPLUS,		"[KPLUS]"},
    {K_KMINUS,		"[KMINUS]"},
    {K_KDIVIDE,		"[KDIVIDE]"},
    {K_KMULTIPLY,	"[KMULTIPLY]"},
    {K_KENTER,		"[KENTER]"},
    {K_KPOINT,		"[KPOINT]"},
    {K_PS,		"[PASTE-START]"},
    {K_PE,		"[PASTE-END]"},
    {K_K0,		"[K0]"},
    {K_K1,		"[K1]"},
    {K_K2,		"[K2]"},
    {K_K3,		"[K3]"},
    {K_K4,		"[K4]"},
    {K_K5,		"[K5]"},
    {K_K6,		"[K6]"},
    {K_K7,		"[K7]"},
    {K_K8,		"[K8]"},
    {K_K9,		"[K9]"},
# endif

#endif /* NO_BUILTIN_TCAPS */

/*
 * The most minimal terminal: only clear screen and cursor positioning
 * Always included.
 */
    {(int)KS_NAME,	"dumb"},
    {(int)KS_CL,	"\014"},
#ifdef TERMINFO
    {(int)KS_CM,	IF_EB("\033[%i%p1%d;%p2%dH",
						  ESC_STR "[%i%p1%d;%p2%dH")},
#else
    {(int)KS_CM,	IF_EB("\033[%i%d;%dH", ESC_STR "[%i%d;%dH")},
#endif

/*
 * end marker
 */
    {(int)KS_NAME,	NULL}

};	/* end of builtin_termcaps */

#if defined(FEAT_TERMGUICOLORS) || defined(PROTO)
    guicolor_T
termgui_mch_get_color(char_u *name)
{
    return gui_get_color_cmn(name);
}

    guicolor_T
termgui_get_color(char_u *name)
{
    guicolor_T	t;

    if (*name == NUL)
	return INVALCOLOR;
    t = termgui_mch_get_color(name);

    if (t == INVALCOLOR)
	semsg(_("E254: Cannot allocate color %s"), name);
    return t;
}

    guicolor_T
termgui_mch_get_rgb(guicolor_T color)
{
    return color;
}
#endif

/*
 * DEFAULT_TERM is used, when no terminal is specified with -T option or $TERM.
 */
#ifdef AMIGA
# define DEFAULT_TERM	(char_u *)"amiga"
#endif

#ifdef MSWIN
# define DEFAULT_TERM	(char_u *)"win32"
#endif

#if defined(UNIX) && !defined(__MINT__)
# define DEFAULT_TERM	(char_u *)"ansi"
#endif

#ifdef __MINT__
# define DEFAULT_TERM	(char_u *)"vt52"
#endif

#ifdef VMS
# define DEFAULT_TERM	(char_u *)"vt320"
#endif

#ifdef __BEOS__
# undef DEFAULT_TERM
# define DEFAULT_TERM	(char_u *)"beos-ansi"
#endif

#ifndef DEFAULT_TERM
# define DEFAULT_TERM	(char_u *)"dumb"
#endif

/*
 * Term_strings contains currently used terminal output strings.
 * It is initialized with the default values by parse_builtin_tcap().
 * The values can be changed by setting the option with the same name.
 */
char_u *(term_strings[(int)KS_LAST + 1]);

static int	need_gather = FALSE;	    /* need to fill termleader[] */
static char_u	termleader[256 + 1];	    /* for check_termcode() */
#ifdef FEAT_TERMRESPONSE
static int	check_for_codes = FALSE;    /* check for key code response */
static int	is_not_xterm = FALSE;	    /* recognized not-really-xterm */
#endif

    static struct builtin_term *
find_builtin_term(char_u *term)
{
    struct builtin_term *p;

    p = builtin_termcaps;
    while (p->bt_string != NULL)
    {
	if (p->bt_entry == (int)KS_NAME)
	{
#ifdef UNIX
	    if (STRCMP(p->bt_string, "iris-ansi") == 0 && vim_is_iris(term))
		return p;
	    else if (STRCMP(p->bt_string, "xterm") == 0 && vim_is_xterm(term))
		return p;
	    else
#endif
#ifdef VMS
		if (STRCMP(p->bt_string, "vt320") == 0 && vim_is_vt300(term))
		    return p;
		else
#endif
		  if (STRCMP(term, p->bt_string) == 0)
		    return p;
	}
	++p;
    }
    return p;
}

/*
 * Parsing of the builtin termcap entries.
 * Caller should check if 'name' is a valid builtin term.
 * The terminal's name is not set, as this is already done in termcapinit().
 */
    static void
parse_builtin_tcap(char_u *term)
{
    struct builtin_term	    *p;
    char_u		    name[2];
    int			    term_8bit;

    p = find_builtin_term(term);
    term_8bit = term_is_8bit(term);

    /* Do not parse if builtin term not found */
    if (p->bt_string == NULL)
	return;

    for (++p; p->bt_entry != (int)KS_NAME && p->bt_entry != BT_EXTRA_KEYS; ++p)
    {
	if ((int)p->bt_entry >= 0)	/* KS_xx entry */
	{
	    /* Only set the value if it wasn't set yet. */
	    if (term_strings[p->bt_entry] == NULL
				 || term_strings[p->bt_entry] == empty_option)
	    {
#ifdef FEAT_EVAL
		int opt_idx = -1;
#endif
		/* 8bit terminal: use CSI instead of <Esc>[ */
		if (term_8bit && term_7to8bit((char_u *)p->bt_string) != 0)
		{
		    char_u  *s, *t;

		    s = vim_strsave((char_u *)p->bt_string);
		    if (s != NULL)
		    {
			for (t = s; *t; ++t)
			    if (term_7to8bit(t))
			    {
				*t = term_7to8bit(t);
				STRMOVE(t + 1, t + 2);
			    }
			term_strings[p->bt_entry] = s;
#ifdef FEAT_EVAL
			opt_idx =
#endif
				  set_term_option_alloced(
						   &term_strings[p->bt_entry]);
		    }
		}
		else
		{
		    term_strings[p->bt_entry] = (char_u *)p->bt_string;
#ifdef FEAT_EVAL
		    opt_idx = get_term_opt_idx(&term_strings[p->bt_entry]);
#endif
		}
#ifdef FEAT_EVAL
		set_term_option_sctx_idx(NULL, opt_idx);
#endif
	    }
	}
	else
	{
	    name[0] = KEY2TERMCAP0((int)p->bt_entry);
	    name[1] = KEY2TERMCAP1((int)p->bt_entry);
	    if (find_termcode(name) == NULL)
		add_termcode(name, (char_u *)p->bt_string, term_8bit);
	}
    }
}

/*
 * Set number of colors.
 * Store it as a number in t_colors.
 * Store it as a string in T_CCO (using nr_colors[]).
 */
    static void
set_color_count(int nr)
{
    char_u	nr_colors[20];		/* string for number of colors */

    t_colors = nr;
    if (t_colors > 1)
	sprintf((char *)nr_colors, "%d", t_colors);
    else
	*nr_colors = NUL;
    set_string_option_direct((char_u *)"t_Co", -1, nr_colors, OPT_FREE, 0);
}

#if defined(FEAT_TERMRESPONSE)
/*
 * Set the color count to "val" and redraw if it changed.
 */
    static void
may_adjust_color_count(int val)
{
    if (val != t_colors)
    {
	/* Nr of colors changed, initialize highlighting and
	 * redraw everything.  This causes a redraw, which usually
	 * clears the message.  Try keeping the message if it
	 * might work. */
	set_keep_msg_from_hist();
	set_color_count(val);
	init_highlight(TRUE, FALSE);
# ifdef DEBUG_TERMRESPONSE
	{
	    int r = redraw_asap(CLEAR);

	    log_tr("Received t_Co, redraw_asap(): %d", r);
	}
#else
	redraw_asap(CLEAR);
#endif
    }
}
#endif

#ifdef HAVE_TGETENT
static char *(key_names[]) =
{
#ifdef FEAT_TERMRESPONSE
    /* Do this one first, it may cause a screen redraw. */
    "Co",
#endif
    "ku", "kd", "kr", "kl",
    "#2", "#4", "%i", "*7",
    "k1", "k2", "k3", "k4", "k5", "k6",
    "k7", "k8", "k9", "k;", "F1", "F2",
    "%1", "&8", "kb", "kI", "kD", "kh",
    "@7", "kP", "kN", "K1", "K3", "K4", "K5", "kB",
    NULL
};
#endif

#ifdef HAVE_TGETENT
    static void
get_term_entries(int *height, int *width)
{
    static struct {
		    enum SpecialKey dest; /* index in term_strings[] */
		    char *name;		  /* termcap name for string */
		  } string_names[] =
		    {	{KS_CE, "ce"}, {KS_AL, "al"}, {KS_CAL,"AL"},
			{KS_DL, "dl"}, {KS_CDL,"DL"}, {KS_CS, "cs"},
			{KS_CL, "cl"}, {KS_CD, "cd"},
			{KS_VI, "vi"}, {KS_VE, "ve"}, {KS_MB, "mb"},
			{KS_ME, "me"}, {KS_MR, "mr"},
			{KS_MD, "md"}, {KS_SE, "se"}, {KS_SO, "so"},
			{KS_CZH,"ZH"}, {KS_CZR,"ZR"}, {KS_UE, "ue"},
			{KS_US, "us"}, {KS_UCE, "Ce"}, {KS_UCS, "Cs"},
			{KS_STE,"Te"}, {KS_STS,"Ts"},
			{KS_CM, "cm"}, {KS_SR, "sr"},
			{KS_CRI,"RI"}, {KS_VB, "vb"}, {KS_KS, "ks"},
			{KS_KE, "ke"}, {KS_TI, "ti"}, {KS_TE, "te"},
			{KS_BC, "bc"}, {KS_CSB,"Sb"}, {KS_CSF,"Sf"},
			{KS_CAB,"AB"}, {KS_CAF,"AF"}, {KS_LE, "le"},
			{KS_ND, "nd"}, {KS_OP, "op"}, {KS_CRV, "RV"},
			{KS_VS, "vs"}, {KS_CVS, "VS"},
			{KS_CIS, "IS"}, {KS_CIE, "IE"},
			{KS_CSC, "SC"}, {KS_CEC, "EC"},
			{KS_TS, "ts"}, {KS_FS, "fs"},
			{KS_CWP, "WP"}, {KS_CWS, "WS"},
			{KS_CSI, "SI"}, {KS_CEI, "EI"},
			{KS_U7, "u7"}, {KS_RFG, "RF"}, {KS_RBG, "RB"},
			{KS_8F, "8f"}, {KS_8B, "8b"},
			{KS_CBE, "BE"}, {KS_CBD, "BD"},
			{KS_CPS, "PS"}, {KS_CPE, "PE"},
			{KS_CST, "ST"}, {KS_CRT, "RT"},
			{KS_SSI, "Si"}, {KS_SRI, "Ri"},
			{(enum SpecialKey)0, NULL}
		    };
    int		    i;
    char_u	    *p;
    static char_u   tstrbuf[TBUFSZ];
    char_u	    *tp = tstrbuf;

    /*
     * get output strings
     */
    for (i = 0; string_names[i].name != NULL; ++i)
    {
	if (TERM_STR(string_names[i].dest) == NULL
			     || TERM_STR(string_names[i].dest) == empty_option)
	{
	    TERM_STR(string_names[i].dest) = TGETSTR(string_names[i].name, &tp);
#ifdef FEAT_EVAL
	    set_term_option_sctx_idx(string_names[i].name, -1);
#endif
	}
    }

    /* tgetflag() returns 1 if the flag is present, 0 if not and
     * possibly -1 if the flag doesn't exist. */
    if ((T_MS == NULL || T_MS == empty_option) && tgetflag("ms") > 0)
	T_MS = (char_u *)"y";
    if ((T_XS == NULL || T_XS == empty_option) && tgetflag("xs") > 0)
	T_XS = (char_u *)"y";
    if ((T_XN == NULL || T_XN == empty_option) && tgetflag("xn") > 0)
	T_XN = (char_u *)"y";
    if ((T_DB == NULL || T_DB == empty_option) && tgetflag("db") > 0)
	T_DB = (char_u *)"y";
    if ((T_DA == NULL || T_DA == empty_option) && tgetflag("da") > 0)
	T_DA = (char_u *)"y";
    if ((T_UT == NULL || T_UT == empty_option) && tgetflag("ut") > 0)
	T_UT = (char_u *)"y";

    /*
     * get key codes
     */
    for (i = 0; key_names[i] != NULL; ++i)
	if (find_termcode((char_u *)key_names[i]) == NULL)
	{
	    p = TGETSTR(key_names[i], &tp);
	    /* if cursor-left == backspace, ignore it (televideo 925) */
	    if (p != NULL
		    && (*p != Ctrl_H
			|| key_names[i][0] != 'k'
			|| key_names[i][1] != 'l'))
		add_termcode((char_u *)key_names[i], p, FALSE);
	}

    if (*height == 0)
	*height = tgetnum("li");
    if (*width == 0)
	*width = tgetnum("co");

    /*
     * Get number of colors (if not done already).
     */
    if (TERM_STR(KS_CCO) == NULL || TERM_STR(KS_CCO) == empty_option)
    {
	set_color_count(tgetnum("Co"));
#ifdef FEAT_EVAL
	set_term_option_sctx_idx("Co", -1);
#endif
    }

# ifndef hpux
    BC = (char *)TGETSTR("bc", &tp);
    UP = (char *)TGETSTR("up", &tp);
    p = TGETSTR("pc", &tp);
    if (p)
	PC = *p;
# endif
}
#endif

    static void
report_term_error(char *error_msg, char_u *term)
{
    struct builtin_term *termp;

    mch_errmsg("\r\n");
    if (error_msg != NULL)
    {
	mch_errmsg(error_msg);
	mch_errmsg("\r\n");
    }
    mch_errmsg("'");
    mch_errmsg((char *)term);
    mch_errmsg(_("' not known. Available builtin terminals are:"));
    mch_errmsg("\r\n");
    for (termp = &(builtin_termcaps[0]); termp->bt_string != NULL; ++termp)
    {
	if (termp->bt_entry == (int)KS_NAME)
	{
#ifdef HAVE_TGETENT
	    mch_errmsg("    builtin_");
#else
	    mch_errmsg("    ");
#endif
	    mch_errmsg(termp->bt_string);
	    mch_errmsg("\r\n");
	}
    }
}

    static void
report_default_term(char_u *term)
{
    mch_errmsg(_("defaulting to '"));
    mch_errmsg((char *)term);
    mch_errmsg("'\r\n");
    if (emsg_silent == 0)
    {
	screen_start();	/* don't know where cursor is now */
	out_flush();
	if (!is_not_a_term())
	    ui_delay(2000L, TRUE);
    }
}

/*
 * Set terminal options for terminal "term".
 * Return OK if terminal 'term' was found in a termcap, FAIL otherwise.
 *
 * While doing this, until ttest(), some options may be NULL, be careful.
 */
    int
set_termname(char_u *term)
{
    struct builtin_term *termp;
#ifdef HAVE_TGETENT
    int		builtin_first = p_tbi;
    int		try;
    int		termcap_cleared = FALSE;
#endif
    int		width = 0, height = 0;
    char	*error_msg = NULL;
    char_u	*bs_p, *del_p;

    /* In silect mode (ex -s) we don't use the 'term' option. */
    if (silent_mode)
	return OK;

    detected_8bit = FALSE;		/* reset 8-bit detection */

    if (term_is_builtin(term))
    {
	term += 8;
#ifdef HAVE_TGETENT
	builtin_first = 1;
#endif
    }

/*
 * If HAVE_TGETENT is not defined, only the builtin termcap is used, otherwise:
 *   If builtin_first is TRUE:
 *     0. try builtin termcap
 *     1. try external termcap
 *     2. if both fail default to a builtin terminal
 *   If builtin_first is FALSE:
 *     1. try external termcap
 *     2. try builtin termcap, if both fail default to a builtin terminal
 */
#ifdef HAVE_TGETENT
    for (try = builtin_first ? 0 : 1; try < 3; ++try)
    {
	/*
	 * Use external termcap
	 */
	if (try == 1)
	{
	    char_u	    tbuf[TBUFSZ];

	    /*
	     * If the external termcap does not have a matching entry, try the
	     * builtin ones.
	     */
	    if ((error_msg = tgetent_error(tbuf, term)) == NULL)
	    {
		if (!termcap_cleared)
		{
		    clear_termoptions();	/* clear old options */
		    termcap_cleared = TRUE;
		}

		get_term_entries(&height, &width);
	    }
	}
	else	    /* try == 0 || try == 2 */
#endif /* HAVE_TGETENT */
	/*
	 * Use builtin termcap
	 */
	{
#ifdef HAVE_TGETENT
	    /*
	     * If builtin termcap was already used, there is no need to search
	     * for the builtin termcap again, quit now.
	     */
	    if (try == 2 && builtin_first && termcap_cleared)
		break;
#endif
	    /*
	     * search for 'term' in builtin_termcaps[]
	     */
	    termp = find_builtin_term(term);
	    if (termp->bt_string == NULL)	/* did not find it */
	    {
#ifdef HAVE_TGETENT
		/*
		 * If try == 0, first try the external termcap. If that is not
		 * found we'll get back here with try == 2.
		 * If termcap_cleared is set we used the external termcap,
		 * don't complain about not finding the term in the builtin
		 * termcap.
		 */
		if (try == 0)			/* try external one */
		    continue;
		if (termcap_cleared)		/* found in external termcap */
		    break;
#endif
		report_term_error(error_msg, term);

		/* when user typed :set term=xxx, quit here */
		if (starting != NO_SCREEN)
		{
		    screen_start();	/* don't know where cursor is now */
		    wait_return(TRUE);
		    return FAIL;
		}
		term = DEFAULT_TERM;
		report_default_term(term);
		set_string_option_direct((char_u *)"term", -1, term,
								 OPT_FREE, 0);
		display_errors();
	    }
	    out_flush();
#ifdef HAVE_TGETENT
	    if (!termcap_cleared)
	    {
#endif
		clear_termoptions();	    /* clear old options */
#ifdef HAVE_TGETENT
		termcap_cleared = TRUE;
	    }
#endif
	    parse_builtin_tcap(term);
#ifdef FEAT_GUI
	    if (term_is_gui(term))
	    {
		out_flush();
		gui_init();
		/* If starting the GUI failed, don't do any of the other
		 * things for this terminal */
		if (!gui.in_use)
		    return FAIL;
#ifdef HAVE_TGETENT
		break;		/* don't try using external termcap */
#endif
	    }
#endif /* FEAT_GUI */
	}
#ifdef HAVE_TGETENT
    }
#endif

/*
 * special: There is no info in the termcap about whether the cursor
 * positioning is relative to the start of the screen or to the start of the
 * scrolling region.  We just guess here. Only msdos pcterm is known to do it
 * relative.
 */
    if (STRCMP(term, "pcterm") == 0)
	T_CCS = (char_u *)"yes";
    else
	T_CCS = empty_option;

#ifdef UNIX
/*
 * Any "stty" settings override the default for t_kb from the termcap.
 * This is in os_unix.c, because it depends a lot on the version of unix that
 * is being used.
 * Don't do this when the GUI is active, it uses "t_kb" and "t_kD" directly.
 */
# ifdef FEAT_GUI
    if (!gui.in_use)
# endif
	get_stty();
#endif

/*
 * If the termcap has no entry for 'bs' and/or 'del' and the ioctl() also
 * didn't work, use the default CTRL-H
 * The default for t_kD is DEL, unless t_kb is DEL.
 * The vim_strsave'd strings are probably lost forever, well it's only two
 * bytes.  Don't do this when the GUI is active, it uses "t_kb" and "t_kD"
 * directly.
 */
#ifdef FEAT_GUI
    if (!gui.in_use)
#endif
    {
	bs_p = find_termcode((char_u *)"kb");
	del_p = find_termcode((char_u *)"kD");
	if (bs_p == NULL || *bs_p == NUL)
	    add_termcode((char_u *)"kb", (bs_p = (char_u *)CTRL_H_STR), FALSE);
	if ((del_p == NULL || *del_p == NUL) &&
					    (bs_p == NULL || *bs_p != DEL))
	    add_termcode((char_u *)"kD", (char_u *)DEL_STR, FALSE);
    }

#if defined(UNIX) || defined(VMS)
    term_is_xterm = vim_is_xterm(term);
#endif

#ifdef FEAT_MOUSE
# if defined(UNIX) || defined(VMS)
#  ifdef FEAT_MOUSE_TTY
    /*
     * For Unix, set the 'ttymouse' option to the type of mouse to be used.
     * The termcode for the mouse is added as a side effect in option.c.
     */
    {
	char_u	*p = (char_u *)"";

#  ifdef FEAT_MOUSE_XTERM
	if (use_xterm_like_mouse(term))
	{
	    if (use_xterm_mouse())
		p = NULL;	/* keep existing value, might be "xterm2" */
	    else
		p = (char_u *)"xterm";
	}
#  endif
	if (p != NULL)
	{
	    set_option_value((char_u *)"ttym", 0L, p, 0);
	    /* Reset the WAS_SET flag, 'ttymouse' can be set to "sgr" or
	     * "xterm2" in check_termcode(). */
	    reset_option_was_set((char_u *)"ttym");
	}
	if (p == NULL
#   ifdef FEAT_GUI
		|| gui.in_use
#   endif
		)
	    check_mouse_termcode();	/* set mouse termcode anyway */
    }
#  endif
# else
    set_mouse_termcode(KS_MOUSE, (char_u *)"\233M");
# endif
#endif	/* FEAT_MOUSE */

#ifdef USE_TERM_CONSOLE
    /* DEFAULT_TERM indicates that it is the machine console. */
    if (STRCMP(term, DEFAULT_TERM) != 0)
	term_console = FALSE;
    else
    {
	term_console = TRUE;
# ifdef AMIGA
	win_resize_on();	/* enable window resizing reports */
# endif
    }
#endif

#if defined(UNIX) || defined(VMS)
    /*
     * 'ttyfast' is default on for xterm, iris-ansi and a few others.
     */
    if (vim_is_fastterm(term))
	p_tf = TRUE;
#endif
#ifdef USE_TERM_CONSOLE
    /*
     * 'ttyfast' is default on consoles
     */
    if (term_console)
	p_tf = TRUE;
#endif

    ttest(TRUE);	/* make sure we have a valid set of terminal codes */

    full_screen = TRUE;		/* we can use termcap codes from now on */
    set_term_defaults();	/* use current values as defaults */
#ifdef FEAT_TERMRESPONSE
    LOG_TR(("setting crv_status to STATUS_GET"));
    crv_status = STATUS_GET;	/* Get terminal version later */
#endif

    /*
     * Initialize the terminal with the appropriate termcap codes.
     * Set the mouse and window title if possible.
     * Don't do this when starting, need to parse the .vimrc first, because it
     * may redefine t_TI etc.
     */
    if (starting != NO_SCREEN)
    {
	starttermcap();		/* may change terminal mode */
#ifdef FEAT_MOUSE
	setmouse();		/* may start using the mouse */
#endif
#ifdef FEAT_TITLE
	maketitle();		/* may display window title */
#endif
    }

	/* display initial screen after ttest() checking. jw. */
    if (width <= 0 || height <= 0)
    {
	/* termcap failed to report size */
	/* set defaults, in case ui_get_shellsize() also fails */
	width = 80;
#if defined(MSWIN)
	height = 25;	    /* console is often 25 lines */
#else
	height = 24;	    /* most terminals are 24 lines */
#endif
    }
    set_shellsize(width, height, FALSE);	/* may change Rows */
    if (starting != NO_SCREEN)
    {
	if (scroll_region)
	    scroll_region_reset();		/* In case Rows changed */
	check_map_keycodes();	/* check mappings for terminal codes used */

	{
	    bufref_T	old_curbuf;

	    /*
	     * Execute the TermChanged autocommands for each buffer that is
	     * loaded.
	     */
	    set_bufref(&old_curbuf, curbuf);
	    FOR_ALL_BUFFERS(curbuf)
	    {
		if (curbuf->b_ml.ml_mfp != NULL)
		    apply_autocmds(EVENT_TERMCHANGED, NULL, NULL, FALSE,
								      curbuf);
	    }
	    if (bufref_valid(&old_curbuf))
		curbuf = old_curbuf.br_buf;
	}
    }

#ifdef FEAT_TERMRESPONSE
    may_req_termresponse();
#endif

    return OK;
}

#if defined(FEAT_MOUSE) || defined(PROTO)

# ifdef FEAT_MOUSE_TTY
#  define HMT_NORMAL	1
#  define HMT_NETTERM	2
#  define HMT_DEC	4
#  define HMT_JSBTERM	8
#  define HMT_PTERM	16
#  define HMT_URXVT	32
#  define HMT_SGR	64
#  define HMT_SGR_REL	128
static int has_mouse_termcode = 0;
# endif

# if (!defined(UNIX) || defined(FEAT_MOUSE_TTY)) || defined(PROTO)
    void
set_mouse_termcode(
    int		n,	/* KS_MOUSE, KS_NETTERM_MOUSE or KS_DEC_MOUSE */
    char_u	*s)
{
    char_u	name[2];

    name[0] = n;
    name[1] = KE_FILLER;
    add_termcode(name, s, FALSE);
#  ifdef FEAT_MOUSE_TTY
#   ifdef FEAT_MOUSE_JSB
    if (n == KS_JSBTERM_MOUSE)
	has_mouse_termcode |= HMT_JSBTERM;
    else
#   endif
#   ifdef FEAT_MOUSE_NET
    if (n == KS_NETTERM_MOUSE)
	has_mouse_termcode |= HMT_NETTERM;
    else
#   endif
#   ifdef FEAT_MOUSE_DEC
    if (n == KS_DEC_MOUSE)
	has_mouse_termcode |= HMT_DEC;
    else
#   endif
#   ifdef FEAT_MOUSE_PTERM
    if (n == KS_PTERM_MOUSE)
	has_mouse_termcode |= HMT_PTERM;
    else
#   endif
#   ifdef FEAT_MOUSE_URXVT
    if (n == KS_URXVT_MOUSE)
	has_mouse_termcode |= HMT_URXVT;
    else
#   endif
    if (n == KS_SGR_MOUSE)
	has_mouse_termcode |= HMT_SGR;
    else if (n == KS_SGR_MOUSE_RELEASE)
	has_mouse_termcode |= HMT_SGR_REL;
    else
	has_mouse_termcode |= HMT_NORMAL;
#  endif
}
# endif

# if ((defined(UNIX) || defined(VMS)) \
	&& defined(FEAT_MOUSE_TTY)) || defined(PROTO)
    void
del_mouse_termcode(
    int		n)	/* KS_MOUSE, KS_NETTERM_MOUSE or KS_DEC_MOUSE */
{
    char_u	name[2];

    name[0] = n;
    name[1] = KE_FILLER;
    del_termcode(name);
#  ifdef FEAT_MOUSE_TTY
#   ifdef FEAT_MOUSE_JSB
    if (n == KS_JSBTERM_MOUSE)
	has_mouse_termcode &= ~HMT_JSBTERM;
    else
#   endif
#   ifdef FEAT_MOUSE_NET
    if (n == KS_NETTERM_MOUSE)
	has_mouse_termcode &= ~HMT_NETTERM;
    else
#   endif
#   ifdef FEAT_MOUSE_DEC
    if (n == KS_DEC_MOUSE)
	has_mouse_termcode &= ~HMT_DEC;
    else
#   endif
#   ifdef FEAT_MOUSE_PTERM
    if (n == KS_PTERM_MOUSE)
	has_mouse_termcode &= ~HMT_PTERM;
    else
#   endif
#   ifdef FEAT_MOUSE_URXVT
    if (n == KS_URXVT_MOUSE)
	has_mouse_termcode &= ~HMT_URXVT;
    else
#   endif
    if (n == KS_SGR_MOUSE)
	has_mouse_termcode &= ~HMT_SGR;
    else if (n == KS_SGR_MOUSE_RELEASE)
	has_mouse_termcode &= ~HMT_SGR_REL;
    else
	has_mouse_termcode &= ~HMT_NORMAL;
#  endif
}
# endif
#endif

#ifdef HAVE_TGETENT
/*
 * Call tgetent()
 * Return error message if it fails, NULL if it's OK.
 */
    static char *
tgetent_error(char_u *tbuf, char_u *term)
{
    int	    i;

    i = TGETENT(tbuf, term);
    if (i < 0		    /* -1 is always an error */
# ifdef TGETENT_ZERO_ERR
	    || i == 0	    /* sometimes zero is also an error */
# endif
       )
    {
	/* On FreeBSD tputs() gets a SEGV after a tgetent() which fails.  Call
	 * tgetent() with the always existing "dumb" entry to avoid a crash or
	 * hang. */
	(void)TGETENT(tbuf, "dumb");

	if (i < 0)
# ifdef TGETENT_ZERO_ERR
	    return _("E557: Cannot open termcap file");
	if (i == 0)
# endif
#ifdef TERMINFO
	    return _("E558: Terminal entry not found in terminfo");
#else
	    return _("E559: Terminal entry not found in termcap");
#endif
    }
    return NULL;
}

/*
 * Some versions of tgetstr() have been reported to return -1 instead of NULL.
 * Fix that here.
 */
    static char_u *
vim_tgetstr(char *s, char_u **pp)
{
    char	*p;

    p = tgetstr(s, (char **)pp);
    if (p == (char *)-1)
	p = NULL;
    return (char_u *)p;
}
#endif /* HAVE_TGETENT */

#if defined(HAVE_TGETENT) && (defined(UNIX) || defined(VMS) || defined(MACOS_X))
/*
 * Get Columns and Rows from the termcap. Used after a window signal if the
 * ioctl() fails. It doesn't make sense to call tgetent each time if the "co"
 * and "li" entries never change. But on some systems this works.
 * Errors while getting the entries are ignored.
 */
    void
getlinecol(
    long	*cp,	/* pointer to columns */
    long	*rp)	/* pointer to rows */
{
    char_u	tbuf[TBUFSZ];

    if (T_NAME != NULL && *T_NAME != NUL && tgetent_error(tbuf, T_NAME) == NULL)
    {
	if (*cp == 0)
	    *cp = tgetnum("co");
	if (*rp == 0)
	    *rp = tgetnum("li");
    }
}
#endif /* defined(HAVE_TGETENT) && defined(UNIX) */

/*
 * Get a string entry from the termcap and add it to the list of termcodes.
 * Used for <t_xx> special keys.
 * Give an error message for failure when not sourcing.
 * If force given, replace an existing entry.
 * Return FAIL if the entry was not found, OK if the entry was added.
 */
    int
add_termcap_entry(char_u *name, int force)
{
    char_u  *term;
    int	    key;
    struct builtin_term *termp;
#ifdef HAVE_TGETENT
    char_u  *string;
    int	    i;
    int	    builtin_first;
    char_u  tbuf[TBUFSZ];
    char_u  tstrbuf[TBUFSZ];
    char_u  *tp = tstrbuf;
    char    *error_msg = NULL;
#endif

/*
 * If the GUI is running or will start in a moment, we only support the keys
 * that the GUI can produce.
 */
#ifdef FEAT_GUI
    if (gui.in_use || gui.starting)
	return gui_mch_haskey(name);
#endif

    if (!force && find_termcode(name) != NULL)	    /* it's already there */
	return OK;

    term = T_NAME;
    if (term == NULL || *term == NUL)	    /* 'term' not defined yet */
	return FAIL;

    if (term_is_builtin(term))		    /* name starts with "builtin_" */
    {
	term += 8;
#ifdef HAVE_TGETENT
	builtin_first = TRUE;
#endif
    }
#ifdef HAVE_TGETENT
    else
	builtin_first = p_tbi;
#endif

#ifdef HAVE_TGETENT
/*
 * We can get the entry from the builtin termcap and from the external one.
 * If 'ttybuiltin' is on or the terminal name starts with "builtin_", try
 * builtin termcap first.
 * If 'ttybuiltin' is off, try external termcap first.
 */
    for (i = 0; i < 2; ++i)
    {
	if ((!builtin_first) == i)
#endif
	/*
	 * Search in builtin termcap
	 */
	{
	    termp = find_builtin_term(term);
	    if (termp->bt_string != NULL)	/* found it */
	    {
		key = TERMCAP2KEY(name[0], name[1]);
		++termp;
		while (termp->bt_entry != (int)KS_NAME)
		{
		    if ((int)termp->bt_entry == key)
		    {
			add_termcode(name, (char_u *)termp->bt_string,
							  term_is_8bit(term));
			return OK;
		    }
		    ++termp;
		}
	    }
	}
#ifdef HAVE_TGETENT
	else
	/*
	 * Search in external termcap
	 */
	{
	    error_msg = tgetent_error(tbuf, term);
	    if (error_msg == NULL)
	    {
		string = TGETSTR((char *)name, &tp);
		if (string != NULL && *string != NUL)
		{
		    add_termcode(name, string, FALSE);
		    return OK;
		}
	    }
	}
    }
#endif

    if (sourcing_name == NULL)
    {
#ifdef HAVE_TGETENT
	if (error_msg != NULL)
	    emsg(error_msg);
	else
#endif
	    semsg(_("E436: No \"%s\" entry in termcap"), name);
    }
    return FAIL;
}

    static int
term_is_builtin(char_u *name)
{
    return (STRNCMP(name, "builtin_", (size_t)8) == 0);
}

/*
 * Return TRUE if terminal "name" uses CSI instead of <Esc>[.
 * Assume that the terminal is using 8-bit controls when the name contains
 * "8bit", like in "xterm-8bit".
 */
    int
term_is_8bit(char_u *name)
{
    return (detected_8bit || strstr((char *)name, "8bit") != NULL);
}

/*
 * Translate terminal control chars from 7-bit to 8-bit:
 * <Esc>[ -> CSI  <M_C_[>
 * <Esc>] -> OSC  <M-C-]>
 * <Esc>O -> <M-C-O>
 */
    static int
term_7to8bit(char_u *p)
{
    if (*p == ESC)
    {
	if (p[1] == '[')
	    return CSI;
	if (p[1] == ']')
	    return OSC;
	if (p[1] == 'O')
	    return 0x8f;
    }
    return 0;
}

#if defined(FEAT_GUI) || defined(PROTO)
    int
term_is_gui(char_u *name)
{
    return (STRCMP(name, "builtin_gui") == 0 || STRCMP(name, "gui") == 0);
}
#endif

#if !defined(HAVE_TGETENT) || defined(AMIGA) || defined(PROTO)

    char_u *
tltoa(unsigned long i)
{
    static char_u buf[16];
    char_u	*p;

    p = buf + 15;
    *p = '\0';
    do
    {
	--p;
	*p = (char_u) (i % 10 + '0');
	i /= 10;
    }
    while (i > 0 && p > buf);
    return p;
}
#endif

#ifndef HAVE_TGETENT

/*
 * minimal tgoto() implementation.
 * no padding and we only parse for %i %d and %+char
 */
    static char *
tgoto(char *cm, int x, int y)
{
    static char buf[30];
    char *p, *s, *e;

    if (!cm)
	return "OOPS";
    e = buf + 29;
    for (s = buf; s < e && *cm; cm++)
    {
	if (*cm != '%')
	{
	    *s++ = *cm;
	    continue;
	}
	switch (*++cm)
	{
	case 'd':
	    p = (char *)tltoa((unsigned long)y);
	    y = x;
	    while (*p)
		*s++ = *p++;
	    break;
	case 'i':
	    x++;
	    y++;
	    break;
	case '+':
	    *s++ = (char)(*++cm + y);
	    y = x;
	    break;
	case '%':
	    *s++ = *cm;
	    break;
	default:
	    return "OOPS";
	}
    }
    *s = '\0';
    return buf;
}

#endif /* HAVE_TGETENT */

/*
 * Set the terminal name and initialize the terminal options.
 * If "name" is NULL or empty, get the terminal name from the environment.
 * If that fails, use the default terminal name.
 */
    void
termcapinit(char_u *name)
{
    char_u	*term;

    if (name != NULL && *name == NUL)
	name = NULL;	    /* empty name is equal to no name */
    term = name;

#ifdef __BEOS__
    /*
     * TERM environment variable is normally set to 'ansi' on the Bebox;
     * Since the BeBox doesn't quite support full ANSI yet, we use our
     * own custom 'ansi-beos' termcap instead, unless the -T option has
     * been given on the command line.
     */
    if (term == NULL
		 && strcmp((char *)mch_getenv((char_u *)"TERM"), "ansi") == 0)
	term = DEFAULT_TERM;
#endif
#ifndef MSWIN
    if (term == NULL)
	term = mch_getenv((char_u *)"TERM");
#endif
    if (term == NULL || *term == NUL)
	term = DEFAULT_TERM;
    set_string_option_direct((char_u *)"term", -1, term, OPT_FREE, 0);

    /* Set the default terminal name. */
    set_string_default("term", term);
    set_string_default("ttytype", term);

    /*
     * Avoid using "term" here, because the next mch_getenv() may overwrite it.
     */
    set_termname(T_NAME != NULL ? T_NAME : term);
}

/*
 * the number of calls to ui_write is reduced by using the buffer "out_buf"
 */
#define OUT_SIZE	2047
	    /* Add one to allow mch_write() in os_win32.c to append a NUL */
static char_u		out_buf[OUT_SIZE + 1];
static int		out_pos = 0;	/* number of chars in out_buf */

/*
 * out_flush(): flush the output buffer
 */
    void
out_flush(void)
{
    int	    len;

    if (out_pos != 0)
    {
	/* set out_pos to 0 before ui_write, to avoid recursiveness */
	len = out_pos;
	out_pos = 0;
	ui_write(out_buf, len);
    }
}

/*
 * out_flush_cursor(): flush the output buffer and redraw the cursor.
 * Does not flush recursively in the GUI to avoid slow drawing.
 */
    void
out_flush_cursor(
    int	    force UNUSED,   /* when TRUE, update cursor even when not moved */
    int	    clear_selection UNUSED) /* clear selection under cursor */
{
    mch_disable_flush();
    out_flush();
    mch_enable_flush();
#ifdef FEAT_GUI
    if (gui.in_use)
    {
	gui_update_cursor(force, clear_selection);
	gui_may_flush();
    }
#endif
}


/*
 * Sometimes a byte out of a multi-byte character is written with out_char().
 * To avoid flushing half of the character, call this function first.
 */
    void
out_flush_check(void)
{
    if (enc_dbcs != 0 && out_pos >= OUT_SIZE - MB_MAXBYTES)
	out_flush();
}

#ifdef FEAT_GUI
/*
 * out_trash(): Throw away the contents of the output buffer
 */
    void
out_trash(void)
{
    out_pos = 0;
}
#endif

/*
 * out_char(c): put a byte into the output buffer.
 *		Flush it if it becomes full.
 * This should not be used for outputting text on the screen (use functions
 * like msg_puts() and screen_putchar() for that).
 */
    void
out_char(unsigned c)
{
#if defined(UNIX) || defined(VMS) || defined(AMIGA) || defined(MACOS_X)
    if (c == '\n')	/* turn LF into CR-LF (CRMOD doesn't seem to do this) */
	out_char('\r');
#endif

    out_buf[out_pos++] = c;

    /* For testing we flush each time. */
    if (out_pos >= OUT_SIZE || p_wd)
	out_flush();
}

static void out_char_nf(unsigned);

/*
 * out_char_nf(c): like out_char(), but don't flush when p_wd is set
 */
    static void
out_char_nf(unsigned c)
{
#if defined(UNIX) || defined(VMS) || defined(AMIGA) || defined(MACOS_X)
    if (c == '\n')	/* turn LF into CR-LF (CRMOD doesn't seem to do this) */
	out_char_nf('\r');
#endif

    out_buf[out_pos++] = c;

    if (out_pos >= OUT_SIZE)
	out_flush();
}

#if defined(FEAT_TITLE) || defined(FEAT_MOUSE_TTY) || defined(FEAT_GUI) \
    || defined(FEAT_TERMRESPONSE) || defined(PROTO)
/*
 * A never-padding out_str.
 * use this whenever you don't want to run the string through tputs.
 * tputs above is harmless, but tputs from the termcap library
 * is likely to strip off leading digits, that it mistakes for padding
 * information, and "%i", "%d", etc.
 * This should only be used for writing terminal codes, not for outputting
 * normal text (use functions like msg_puts() and screen_putchar() for that).
 */
    void
out_str_nf(char_u *s)
{
    if (out_pos > OUT_SIZE - 20)  /* avoid terminal strings being split up */
	out_flush();
    while (*s)
	out_char_nf(*s++);

    /* For testing we write one string at a time. */
    if (p_wd)
	out_flush();
}
#endif

/*
 * A conditional-flushing out_str, mainly for visualbell.
 * Handles a delay internally, because termlib may not respect the delay or do
 * it at the wrong time.
 * Note: Only for terminal strings.
 */
    void
out_str_cf(char_u *s)
{
    if (s != NULL && *s)
    {
#ifdef HAVE_TGETENT
	char_u *p;
#endif

#ifdef FEAT_GUI
	/* Don't use tputs() when GUI is used, ncurses crashes. */
	if (gui.in_use)
	{
	    out_str_nf(s);
	    return;
	}
#endif
	if (out_pos > OUT_SIZE - 20)
	    out_flush();
#ifdef HAVE_TGETENT
	for (p = s; *s; ++s)
	{
	    /* flush just before delay command */
	    if (*s == '$' && *(s + 1) == '<')
	    {
		char_u save_c = *s;
		int duration = atoi((char *)s + 2);

		*s = NUL;
		tputs((char *)p, 1, TPUTSFUNCAST out_char_nf);
		*s = save_c;
		out_flush();
# ifdef ELAPSED_FUNC
		/* Only sleep here if we can limit this happening in
		 * vim_beep(). */
		p = vim_strchr(s, '>');
		if (p == NULL || duration <= 0)
		{
		    /* can't parse the time, don't sleep here */
		    p = s;
		}
		else
		{
		    ++p;
		    do_sleep(duration);
		}
# else
		/* Rely on the terminal library to sleep. */
		p = s;
# endif
		break;
	    }
	}
	tputs((char *)p, 1, TPUTSFUNCAST out_char_nf);
#else
	while (*s)
	    out_char_nf(*s++);
#endif

	/* For testing we write one string at a time. */
	if (p_wd)
	    out_flush();
    }
}

/*
 * out_str(s): Put a character string a byte at a time into the output buffer.
 * If HAVE_TGETENT is defined use the termcap parser. (jw)
 * This should only be used for writing terminal codes, not for outputting
 * normal text (use functions like msg_puts() and screen_putchar() for that).
 */
    void
out_str(char_u *s)
{
    if (s != NULL && *s)
    {
#ifdef FEAT_GUI
	/* Don't use tputs() when GUI is used, ncurses crashes. */
	if (gui.in_use)
	{
	    out_str_nf(s);
	    return;
	}
#endif
	/* avoid terminal strings being split up */
	if (out_pos > OUT_SIZE - 20)
	    out_flush();
#ifdef HAVE_TGETENT
	tputs((char *)s, 1, TPUTSFUNCAST out_char_nf);
#else
	while (*s)
	    out_char_nf(*s++);
#endif

	/* For testing we write one string at a time. */
	if (p_wd)
	    out_flush();
    }
}

/*
 * cursor positioning using termcap parser. (jw)
 */
    void
term_windgoto(int row, int col)
{
    OUT_STR(tgoto((char *)T_CM, col, row));
}

    void
term_cursor_right(int i)
{
    OUT_STR(tgoto((char *)T_CRI, 0, i));
}

    void
term_append_lines(int line_count)
{
    OUT_STR(tgoto((char *)T_CAL, 0, line_count));
}

    void
term_delete_lines(int line_count)
{
    OUT_STR(tgoto((char *)T_CDL, 0, line_count));
}

#if defined(HAVE_TGETENT) || defined(PROTO)
    void
term_set_winpos(int x, int y)
{
    /* Can't handle a negative value here */
    if (x < 0)
	x = 0;
    if (y < 0)
	y = 0;
    OUT_STR(tgoto((char *)T_CWP, y, x));
}

# if defined(FEAT_TERMRESPONSE) || defined(PROTO)
/*
 * Return TRUE if we can request the terminal for a response.
 */
    static int
can_get_termresponse()
{
    return cur_tmode == TMODE_RAW
	    && termcap_active
# ifdef UNIX
	    && (is_not_a_term() || (isatty(1) && isatty(read_cmd_fd)))
# endif
	    && p_ek;
}

static int winpos_x = -1;
static int winpos_y = -1;
static int did_request_winpos = 0;

#  if (defined(FEAT_EVAL) && defined(HAVE_TGETENT)) || defined(PROTO)
/*
 * Try getting the Vim window position from the terminal.
 * Returns OK or FAIL.
 */
    int
term_get_winpos(int *x, int *y, varnumber_T timeout)
{
    int count = 0;
    int prev_winpos_x = winpos_x;
    int prev_winpos_y = winpos_y;

    if (*T_CGP == NUL || !can_get_termresponse())
	return FAIL;
    winpos_x = -1;
    winpos_y = -1;
    ++did_request_winpos;
    winpos_status = STATUS_SENT;
    OUT_STR(T_CGP);
    out_flush();

    /* Try reading the result for "timeout" msec. */
    while (count++ <= timeout / 10 && !got_int)
    {
	(void)vpeekc_nomap();
	if (winpos_x >= 0 && winpos_y >= 0)
	{
	    *x = winpos_x;
	    *y = winpos_y;
	    return OK;
	}
	ui_delay(10, FALSE);
    }
    /* Do not reset "did_request_winpos", if we timed out the response might
     * still come later and we must consume it. */

    winpos_x = prev_winpos_x;
    winpos_y = prev_winpos_y;
    if (timeout < 10 && prev_winpos_y >= 0 && prev_winpos_x >= 0)
    {
	/* Polling: return previous values if we have them. */
	*x = winpos_x;
	*y = winpos_y;
	return OK;
    }

    return FALSE;
}
#  endif
# endif

    void
term_set_winsize(int height, int width)
{
    OUT_STR(tgoto((char *)T_CWS, width, height));
}
#endif

    static void
term_color(char_u *s, int n)
{
    char	buf[20];
    int		i = *s == CSI ? 1 : 2;
		/* index in s[] just after <Esc>[ or CSI */

    /* Special handling of 16 colors, because termcap can't handle it */
    /* Also accept "\e[3%dm" for TERMINFO, it is sometimes used */
    /* Also accept CSI instead of <Esc>[ */
    if (n >= 8 && t_colors >= 16
	      && ((s[0] == ESC && s[1] == '[')
#if defined(FEAT_VTP) && defined(FEAT_TERMGUICOLORS)
		  || (s[0] == ESC && s[1] == '|')
#endif
	          || (s[0] == CSI && (i = 1) == 1))
	      && s[i] != NUL
	      && (STRCMP(s + i + 1, "%p1%dm") == 0
		  || STRCMP(s + i + 1, "%dm") == 0)
	      && (s[i] == '3' || s[i] == '4'))
    {
#ifdef TERMINFO
	char *format = "%s%s%%p1%%dm";
#else
	char *format = "%s%s%%dm";
#endif
	char *lead = i == 2 ? (
#if defined(FEAT_VTP) && defined(FEAT_TERMGUICOLORS)
		    s[1] == '|' ? IF_EB("\033|", ESC_STR "|") :
#endif
		    IF_EB("\033[", ESC_STR "[")) : "\233";
	char *tail = s[i] == '3' ? (n >= 16 ? "38;5;" : "9")
				 : (n >= 16 ? "48;5;" : "10");

	sprintf(buf, format, lead, tail);
	OUT_STR(tgoto(buf, 0, n >= 16 ? n : n - 8));
    }
    else
	OUT_STR(tgoto((char *)s, 0, n));
}

    void
term_fg_color(int n)
{
    /* Use "AF" termcap entry if present, "Sf" entry otherwise */
    if (*T_CAF)
	term_color(T_CAF, n);
    else if (*T_CSF)
	term_color(T_CSF, n);
}

    void
term_bg_color(int n)
{
    /* Use "AB" termcap entry if present, "Sb" entry otherwise */
    if (*T_CAB)
	term_color(T_CAB, n);
    else if (*T_CSB)
	term_color(T_CSB, n);
}

#if defined(FEAT_TERMGUICOLORS) || defined(PROTO)

#define RED(rgb)   (((long_u)(rgb) >> 16) & 0xFF)
#define GREEN(rgb) (((long_u)(rgb) >>  8) & 0xFF)
#define BLUE(rgb)  (((long_u)(rgb)      ) & 0xFF)

    static void
term_rgb_color(char_u *s, guicolor_T rgb)
{
#define MAX_COLOR_STR_LEN 100
    char	buf[MAX_COLOR_STR_LEN];

    vim_snprintf(buf, MAX_COLOR_STR_LEN,
				  (char *)s, RED(rgb), GREEN(rgb), BLUE(rgb));
    OUT_STR(buf);
}

    void
term_fg_rgb_color(guicolor_T rgb)
{
    term_rgb_color(T_8F, rgb);
}

    void
term_bg_rgb_color(guicolor_T rgb)
{
    term_rgb_color(T_8B, rgb);
}
#endif

#if (defined(FEAT_TITLE) && (defined(UNIX) || defined(VMS) \
	|| defined(MACOS_X))) || defined(PROTO)
/*
 * Generic function to set window title, using t_ts and t_fs.
 */
    void
term_settitle(char_u *title)
{
    /* t_ts takes one argument: column in status line */
    OUT_STR(tgoto((char *)T_TS, 0, 0));	/* set title start */
    out_str_nf(title);
    out_str(T_FS);			/* set title end */
    out_flush();
}

/*
 * Tell the terminal to push (save) the title and/or icon, so that it can be
 * popped (restored) later.
 */
    void
term_push_title(int which)
{
    if ((which & SAVE_RESTORE_TITLE) && *T_CST != NUL)
    {
	OUT_STR(T_CST);
	out_flush();
    }

    if ((which & SAVE_RESTORE_ICON) && *T_SSI != NUL)
    {
	OUT_STR(T_SSI);
	out_flush();
    }
}

/*
 * Tell the terminal to pop the title and/or icon.
 */
    void
term_pop_title(int which)
{
    if ((which & SAVE_RESTORE_TITLE) && *T_CRT != NUL)
    {
	OUT_STR(T_CRT);
	out_flush();
    }

    if ((which & SAVE_RESTORE_ICON) && *T_SRI != NUL)
    {
	OUT_STR(T_SRI);
	out_flush();
    }
}
#endif

/*
 * Make sure we have a valid set or terminal options.
 * Replace all entries that are NULL by empty_option
 */
    void
ttest(int pairs)
{
    char_u *env_colors;

    check_options();		    /* make sure no options are NULL */

    /*
     * MUST have "cm": cursor motion.
     */
    if (*T_CM == NUL)
	emsg(_("E437: terminal capability \"cm\" required"));

    /*
     * if "cs" defined, use a scroll region, it's faster.
     */
    if (*T_CS != NUL)
	scroll_region = TRUE;
    else
	scroll_region = FALSE;

    if (pairs)
    {
	/*
	 * optional pairs
	 */
	/* TP goes to normal mode for TI (invert) and TB (bold) */
	if (*T_ME == NUL)
	    T_ME = T_MR = T_MD = T_MB = empty_option;
	if (*T_SO == NUL || *T_SE == NUL)
	    T_SO = T_SE = empty_option;
	if (*T_US == NUL || *T_UE == NUL)
	    T_US = T_UE = empty_option;
	if (*T_CZH == NUL || *T_CZR == NUL)
	    T_CZH = T_CZR = empty_option;

	/* T_VE is needed even though T_VI is not defined */
	if (*T_VE == NUL)
	    T_VI = empty_option;

	/* if 'mr' or 'me' is not defined use 'so' and 'se' */
	if (*T_ME == NUL)
	{
	    T_ME = T_SE;
	    T_MR = T_SO;
	    T_MD = T_SO;
	}

	/* if 'so' or 'se' is not defined use 'mr' and 'me' */
	if (*T_SO == NUL)
	{
	    T_SE = T_ME;
	    if (*T_MR == NUL)
		T_SO = T_MD;
	    else
		T_SO = T_MR;
	}

	/* if 'ZH' or 'ZR' is not defined use 'mr' and 'me' */
	if (*T_CZH == NUL)
	{
	    T_CZR = T_ME;
	    if (*T_MR == NUL)
		T_CZH = T_MD;
	    else
		T_CZH = T_MR;
	}

	/* "Sb" and "Sf" come in pairs */
	if (*T_CSB == NUL || *T_CSF == NUL)
	{
	    T_CSB = empty_option;
	    T_CSF = empty_option;
	}

	/* "AB" and "AF" come in pairs */
	if (*T_CAB == NUL || *T_CAF == NUL)
	{
	    T_CAB = empty_option;
	    T_CAF = empty_option;
	}

	/* if 'Sb' and 'AB' are not defined, reset "Co" */
	if (*T_CSB == NUL && *T_CAB == NUL)
	    free_one_termoption(T_CCO);

	/* Set 'weirdinvert' according to value of 't_xs' */
	p_wiv = (*T_XS != NUL);
    }
    need_gather = TRUE;

    /* Set t_colors to the value of $COLORS or t_Co. */
    t_colors = atoi((char *)T_CCO);
    env_colors = mch_getenv((char_u *)"COLORS");
    if (env_colors != NULL && isdigit(*env_colors))
    {
	int colors = atoi((char *)env_colors);

	if (colors != t_colors)
	    set_color_count(colors);
    }
}

#if (defined(FEAT_GUI) && (defined(FEAT_MENU) || !defined(USE_ON_FLY_SCROLL))) \
	|| defined(PROTO)
/*
 * Represent the given long_u as individual bytes, with the most significant
 * byte first, and store them in dst.
 */
    void
add_long_to_buf(long_u val, char_u *dst)
{
    int	    i;
    int	    shift;

    for (i = 1; i <= (int)sizeof(long_u); i++)
    {
	shift = 8 * (sizeof(long_u) - i);
	dst[i - 1] = (char_u) ((val >> shift) & 0xff);
    }
}

/*
 * Interpret the next string of bytes in buf as a long integer, with the most
 * significant byte first.  Note that it is assumed that buf has been through
 * inchar(), so that NUL and K_SPECIAL will be represented as three bytes each.
 * Puts result in val, and returns the number of bytes read from buf
 * (between sizeof(long_u) and 2 * sizeof(long_u)), or -1 if not enough bytes
 * were present.
 */
    static int
get_long_from_buf(char_u *buf, long_u *val)
{
    int	    len;
    char_u  bytes[sizeof(long_u)];
    int	    i;
    int	    shift;

    *val = 0;
    len = get_bytes_from_buf(buf, bytes, (int)sizeof(long_u));
    if (len != -1)
    {
	for (i = 0; i < (int)sizeof(long_u); i++)
	{
	    shift = 8 * (sizeof(long_u) - 1 - i);
	    *val += (long_u)bytes[i] << shift;
	}
    }
    return len;
}
#endif

#if defined(FEAT_GUI) \
    || (defined(FEAT_MOUSE) && (!defined(UNIX) || defined(FEAT_MOUSE_XTERM) \
		|| defined(FEAT_MOUSE_GPM) || defined(FEAT_SYSMOUSE)))
/*
 * Read the next num_bytes bytes from buf, and store them in bytes.  Assume
 * that buf has been through inchar().	Returns the actual number of bytes used
 * from buf (between num_bytes and num_bytes*2), or -1 if not enough bytes were
 * available.
 */
    static int
get_bytes_from_buf(char_u *buf, char_u *bytes, int num_bytes)
{
    int	    len = 0;
    int	    i;
    char_u  c;

    for (i = 0; i < num_bytes; i++)
    {
	if ((c = buf[len++]) == NUL)
	    return -1;
	if (c == K_SPECIAL)
	{
	    if (buf[len] == NUL || buf[len + 1] == NUL)	    /* cannot happen? */
		return -1;
	    if (buf[len++] == (int)KS_ZERO)
		c = NUL;
	    /* else it should be KS_SPECIAL; when followed by KE_FILLER c is
	     * K_SPECIAL, or followed by KE_CSI and c must be CSI. */
	    if (buf[len++] == (int)KE_CSI)
		c = CSI;
	}
	else if (c == CSI && buf[len] == KS_EXTRA
					       && buf[len + 1] == (int)KE_CSI)
	    /* CSI is stored as CSI KS_SPECIAL KE_CSI to avoid confusion with
	     * the start of a special key, see add_to_input_buf_csi(). */
	    len += 2;
	bytes[i] = c;
    }
    return len;
}
#endif

/*
 * Check if the new shell size is valid, correct it if it's too small or way
 * too big.
 */
    void
check_shellsize(void)
{
    if (Rows < min_rows())	/* need room for one window and command line */
	Rows = min_rows();
    limit_screen_size();
}

/*
 * Limit Rows and Columns to avoid an overflow in Rows * Columns.
 */
    void
limit_screen_size(void)
{
    if (Columns < MIN_COLUMNS)
	Columns = MIN_COLUMNS;
    else if (Columns > 10000)
	Columns = 10000;
    if (Rows > 1000)
	Rows = 1000;
}

/*
 * Invoked just before the screen structures are going to be (re)allocated.
 */
    void
win_new_shellsize(void)
{
    static int	old_Rows = 0;
    static int	old_Columns = 0;

    if (old_Rows != Rows || old_Columns != Columns)
	ui_new_shellsize();
    if (old_Rows != Rows)
    {
	/* if 'window' uses the whole screen, keep it using that */
	if (p_window == old_Rows - 1 || old_Rows == 0)
	    p_window = Rows - 1;
	old_Rows = Rows;
	shell_new_rows();	/* update window sizes */
    }
    if (old_Columns != Columns)
    {
	old_Columns = Columns;
	shell_new_columns();	/* update window sizes */
    }
}

/*
 * Call this function when the Vim shell has been resized in any way.
 * Will obtain the current size and redraw (also when size didn't change).
 */
    void
shell_resized(void)
{
    set_shellsize(0, 0, FALSE);
}

/*
 * Check if the shell size changed.  Handle a resize.
 * When the size didn't change, nothing happens.
 */
    void
shell_resized_check(void)
{
    int		old_Rows = Rows;
    int		old_Columns = Columns;

    if (!exiting
#ifdef FEAT_GUI
	    /* Do not get the size when executing a shell command during
	     * startup. */
	    && !gui.starting
#endif
	    )
    {
	(void)ui_get_shellsize();
	check_shellsize();
	if (old_Rows != Rows || old_Columns != Columns)
	    shell_resized();
    }
}

/*
 * Set size of the Vim shell.
 * If 'mustset' is TRUE, we must set Rows and Columns, do not get the real
 * window size (this is used for the :win command).
 * If 'mustset' is FALSE, we may try to get the real window size and if
 * it fails use 'width' and 'height'.
 */
    void
set_shellsize(int width, int height, int mustset)
{
    static int		busy = FALSE;

    /*
     * Avoid recursiveness, can happen when setting the window size causes
     * another window-changed signal.
     */
    if (busy)
	return;

    if (width < 0 || height < 0)    /* just checking... */
	return;

    if (State == HITRETURN || State == SETWSIZE)
    {
	/* postpone the resizing */
	State = SETWSIZE;
	return;
    }

    /* curwin->w_buffer can be NULL when we are closing a window and the
     * buffer has already been closed and removing a scrollbar causes a resize
     * event. Don't resize then, it will happen after entering another buffer.
     */
    if (curwin->w_buffer == NULL)
	return;

    ++busy;

#ifdef AMIGA
    out_flush();	    /* must do this before mch_get_shellsize() for
			       some obscure reason */
#endif

    if (mustset || (ui_get_shellsize() == FAIL && height != 0))
    {
	Rows = height;
	Columns = width;
	check_shellsize();
	ui_set_shellsize(mustset);
    }
    else
	check_shellsize();

    /* The window layout used to be adjusted here, but it now happens in
     * screenalloc() (also invoked from screenclear()).  That is because the
     * "busy" check above may skip this, but not screenalloc(). */

    if (State != ASKMORE && State != EXTERNCMD && State != CONFIRM)
	screenclear();
    else
	screen_start();	    /* don't know where cursor is now */

    if (starting != NO_SCREEN)
    {
#ifdef FEAT_TITLE
	maketitle();
#endif
	changed_line_abv_curs();
	invalidate_botline();

	/*
	 * We only redraw when it's needed:
	 * - While at the more prompt or executing an external command, don't
	 *   redraw, but position the cursor.
	 * - While editing the command line, only redraw that.
	 * - in Ex mode, don't redraw anything.
	 * - Otherwise, redraw right now, and position the cursor.
	 * Always need to call update_screen() or screenalloc(), to make
	 * sure Rows/Columns and the size of ScreenLines[] is correct!
	 */
	if (State == ASKMORE || State == EXTERNCMD || State == CONFIRM
							     || exmode_active)
	{
	    screenalloc(FALSE);
	    repeat_message();
	}
	else
	{
	    if (curwin->w_p_scb)
		do_check_scrollbind(TRUE);
	    if (State & CMDLINE)
	    {
		update_screen(NOT_VALID);
		redrawcmdline();
	    }
	    else
	    {
		update_topline();
#if defined(FEAT_INS_EXPAND)
		if (pum_visible())
		{
		    redraw_later(NOT_VALID);
		    ins_compl_show_pum();
		}
#endif
		update_screen(NOT_VALID);
		if (redrawing())
		    setcursor();
	    }
	}
	cursor_on();	    /* redrawing may have switched it off */
    }
    out_flush();
    --busy;
}

/*
 * Set the terminal to TMODE_RAW (for Normal mode) or TMODE_COOK (for external
 * commands and Ex mode).
 */
    void
settmode(int tmode)
{
#ifdef FEAT_GUI
    /* don't set the term where gvim was started to any mode */
    if (gui.in_use)
	return;
#endif

    if (full_screen)
    {
	/*
	 * When returning after calling a shell we want to really set the
	 * terminal to raw mode, even though we think it already is, because
	 * the shell program may have reset the terminal mode.
	 * When we think the terminal is normal, don't try to set it to
	 * normal again, because that causes problems (logout!) on some
	 * machines.
	 */
	if (tmode != TMODE_COOK || cur_tmode != TMODE_COOK)
	{
#ifdef FEAT_TERMRESPONSE
# ifdef FEAT_GUI
	    if (!gui.in_use && !gui.starting)
# endif
	    {
		/* May need to check for T_CRV response and termcodes, it
		 * doesn't work in Cooked mode, an external program may get
		 * them. */
		if (tmode != TMODE_RAW && (crv_status == STATUS_SENT
					 || u7_status == STATUS_SENT
#ifdef FEAT_TERMINAL
					 || rfg_status == STATUS_SENT
#endif
					 || rbg_status == STATUS_SENT
					 || rbm_status == STATUS_SENT
					 || rcs_status == STATUS_SENT
					 || winpos_status == STATUS_SENT))
		    (void)vpeekc_nomap();
		check_for_codes_from_term();
	    }
#endif
#ifdef FEAT_MOUSE_TTY
	    if (tmode != TMODE_RAW)
		mch_setmouse(FALSE);	/* switch mouse off */
#endif
	    if (tmode != TMODE_RAW)
		out_str(T_BD);		/* disable bracketed paste mode */
	    out_flush();
	    mch_settmode(tmode);	/* machine specific function */
	    cur_tmode = tmode;
#ifdef FEAT_MOUSE
	    if (tmode == TMODE_RAW)
		setmouse();		/* may switch mouse on */
#endif
	    if (tmode == TMODE_RAW)
		out_str(T_BE);		/* enable bracketed paste mode */
	    out_flush();
	}
#ifdef FEAT_TERMRESPONSE
	may_req_termresponse();
#endif
    }
}

    void
starttermcap(void)
{
    if (full_screen && !termcap_active)
    {
	out_str(T_TI);			/* start termcap mode */
	out_str(T_KS);			/* start "keypad transmit" mode */
	out_str(T_BE);			/* enable bracketed paste mode */
	out_flush();
	termcap_active = TRUE;
	screen_start();			/* don't know where cursor is now */
#ifdef FEAT_TERMRESPONSE
# ifdef FEAT_GUI
	if (!gui.in_use && !gui.starting)
# endif
	{
	    may_req_termresponse();
	    /* Immediately check for a response.  If t_Co changes, we don't
	     * want to redraw with wrong colors first. */
	    if (crv_status == STATUS_SENT)
		check_for_codes_from_term();
	}
#endif
    }
}

    void
stoptermcap(void)
{
    screen_stop_highlight();
    reset_cterm_colors();
    if (termcap_active)
    {
#ifdef FEAT_TERMRESPONSE
# ifdef FEAT_GUI
	if (!gui.in_use && !gui.starting)
# endif
	{
	    /* May need to discard T_CRV, T_U7 or T_RBG response. */
	    if (crv_status == STATUS_SENT
		    || u7_status == STATUS_SENT
# ifdef FEAT_TERMINAL
		    || rfg_status == STATUS_SENT
# endif
		    || rbg_status == STATUS_SENT
		    || rbm_status == STATUS_SENT
		    || rcs_status == STATUS_SENT
		    || winpos_status == STATUS_SENT)
	    {
# ifdef UNIX
		/* Give the terminal a chance to respond. */
		mch_delay(100L, FALSE);
# endif
# ifdef TCIFLUSH
		/* Discard data received but not read. */
		if (exiting)
		    tcflush(fileno(stdin), TCIFLUSH);
# endif
	    }
	    /* Check for termcodes first, otherwise an external program may
	     * get them. */
	    check_for_codes_from_term();
	}
#endif
	out_str(T_BD);			/* disable bracketed paste mode */
	out_str(T_KE);			/* stop "keypad transmit" mode */
	out_flush();
	termcap_active = FALSE;
	cursor_on();			/* just in case it is still off */
	out_str(T_TE);			/* stop termcap mode */
	screen_start();			/* don't know where cursor is now */
	out_flush();
    }
}

#if defined(FEAT_TERMRESPONSE) || defined(PROTO)
/*
 * Request version string (for xterm) when needed.
 * Only do this after switching to raw mode, otherwise the result will be
 * echoed.
 * Only do this after startup has finished, to avoid that the response comes
 * while executing "-c !cmd" or even after "-c quit".
 * Only do this after termcap mode has been started, otherwise the codes for
 * the cursor keys may be wrong.
 * Only do this when 'esckeys' is on, otherwise the response causes trouble in
 * Insert mode.
 * On Unix only do it when both output and input are a tty (avoid writing
 * request to terminal while reading from a file).
 * The result is caught in check_termcode().
 */
    void
may_req_termresponse(void)
{
    if (crv_status == STATUS_GET
	    && can_get_termresponse()
	    && starting == 0
	    && *T_CRV != NUL)
    {
	LOG_TR(("Sending CRV request"));
	out_str(T_CRV);
	crv_status = STATUS_SENT;
	/* check for the characters now, otherwise they might be eaten by
	 * get_keystroke() */
	out_flush();
	(void)vpeekc_nomap();
    }
}

/*
 * Check how the terminal treats ambiguous character width (UAX #11).
 * First, we move the cursor to (1, 0) and print a test ambiguous character
 * \u25bd (WHITE DOWN-POINTING TRIANGLE) and query current cursor position.
 * If the terminal treats \u25bd as single width, the position is (1, 1),
 * or if it is treated as double width, that will be (1, 2).
 * This function has the side effect that changes cursor position, so
 * it must be called immediately after entering termcap mode.
 */
    void
may_req_ambiguous_char_width(void)
{
    if (u7_status == STATUS_GET
	    && can_get_termresponse()
	    && starting == 0
	    && *T_U7 != NUL
	    && !option_was_set((char_u *)"ambiwidth"))
    {
	 char_u	buf[16];

	 LOG_TR(("Sending U7 request"));
	 /* Do this in the second row.  In the first row the returned sequence
	  * may be CSI 1;2R, which is the same as <S-F3>. */
	 term_windgoto(1, 0);
	 buf[mb_char2bytes(0x25bd, buf)] = 0;
	 out_str(buf);
	 out_str(T_U7);
	 u7_status = STATUS_SENT;
	 out_flush();

	 /* This overwrites a few characters on the screen, a redraw is needed
	  * after this. Clear them out for now. */
	 term_windgoto(1, 0);
	 out_str((char_u *)"  ");
	 term_windgoto(0, 0);

	 /* Need to reset the known cursor position. */
	 screen_start();

	 /* check for the characters now, otherwise they might be eaten by
	  * get_keystroke() */
	 out_flush();
	 (void)vpeekc_nomap();
    }
}

/*
 * Similar to requesting the version string: Request the terminal background
 * color when it is the right moment.
 */
    void
may_req_bg_color(void)
{
    if (can_get_termresponse() && starting == 0)
    {
	int didit = FALSE;

# ifdef FEAT_TERMINAL
	/* Only request foreground if t_RF is set. */
	if (rfg_status == STATUS_GET && *T_RFG != NUL)
	{
	    LOG_TR(("Sending FG request"));
	    out_str(T_RFG);
	    rfg_status = STATUS_SENT;
	    didit = TRUE;
	}
# endif

	/* Only request background if t_RB is set. */
	if (rbg_status == STATUS_GET && *T_RBG != NUL)
	{
	    LOG_TR(("Sending BG request"));
	    out_str(T_RBG);
	    rbg_status = STATUS_SENT;
	    didit = TRUE;
	}

	if (didit)
	{
	    /* check for the characters now, otherwise they might be eaten by
	     * get_keystroke() */
	    out_flush();
	    (void)vpeekc_nomap();
	}
    }
}

# ifdef DEBUG_TERMRESPONSE
    static void
log_tr(const char *fmt, ...)
{
    static FILE *fd_tr = NULL;
    static proftime_T start;
    proftime_T now;
    va_list ap;

    if (fd_tr == NULL)
    {
	fd_tr = fopen("termresponse.log", "w");
	profile_start(&start);
    }
    now = start;
    profile_end(&now);
    fprintf(fd_tr, "%s: %s ", profile_msg(&now),
					must_redraw == NOT_VALID ? "NV"
					: must_redraw == CLEAR ? "CL" : "  ");
    va_start(ap, fmt);
    vfprintf(fd_tr, fmt, ap);
    va_end(ap);
    fputc('\n', fd_tr);
    fflush(fd_tr);
}
# endif
#endif

/*
 * Return TRUE when saving and restoring the screen.
 */
    int
swapping_screen(void)
{
    return (full_screen && *T_TI != NUL);
}

#if defined(FEAT_MOUSE) || defined(PROTO)
/*
 * setmouse() - switch mouse on/off depending on current mode and 'mouse'
 */
    void
setmouse(void)
{
# ifdef FEAT_MOUSE_TTY
    int	    checkfor;
# endif

# ifdef FEAT_MOUSESHAPE
    update_mouseshape(-1);
# endif

# ifdef FEAT_MOUSE_TTY /* Should be outside proc, but may break MOUSESHAPE */
#  ifdef FEAT_GUI
    /* In the GUI the mouse is always enabled. */
    if (gui.in_use)
	return;
#  endif
    /* be quick when mouse is off */
    if (*p_mouse == NUL || has_mouse_termcode == 0)
	return;

    /* don't switch mouse on when not in raw mode (Ex mode) */
    if (cur_tmode != TMODE_RAW)
    {
	mch_setmouse(FALSE);
	return;
    }

    if (VIsual_active)
	checkfor = MOUSE_VISUAL;
    else if (State == HITRETURN || State == ASKMORE || State == SETWSIZE)
	checkfor = MOUSE_RETURN;
    else if (State & INSERT)
	checkfor = MOUSE_INSERT;
    else if (State & CMDLINE)
	checkfor = MOUSE_COMMAND;
    else if (State == CONFIRM || State == EXTERNCMD)
	checkfor = ' '; /* don't use mouse for ":confirm" or ":!cmd" */
    else
	checkfor = MOUSE_NORMAL;    /* assume normal mode */

    if (mouse_has(checkfor))
	mch_setmouse(TRUE);
    else
	mch_setmouse(FALSE);
# endif
}

/*
 * Return TRUE if
 * - "c" is in 'mouse', or
 * - 'a' is in 'mouse' and "c" is in MOUSE_A, or
 * - the current buffer is a help file and 'h' is in 'mouse' and we are in a
 *   normal editing mode (not at hit-return message).
 */
    int
mouse_has(int c)
{
    char_u	*p;

    for (p = p_mouse; *p; ++p)
	switch (*p)
	{
	    case 'a': if (vim_strchr((char_u *)MOUSE_A, c) != NULL)
			  return TRUE;
		      break;
	    case MOUSE_HELP: if (c != MOUSE_RETURN && curbuf->b_help)
				 return TRUE;
			     break;
	    default: if (c == *p) return TRUE; break;
	}
    return FALSE;
}

/*
 * Return TRUE when 'mousemodel' is set to "popup" or "popup_setpos".
 */
    int
mouse_model_popup(void)
{
    return (p_mousem[0] == 'p');
}
#endif

/*
 * By outputting the 'cursor very visible' termcap code, for some windowed
 * terminals this makes the screen scrolled to the correct position.
 * Used when starting Vim or returning from a shell.
 */
    void
scroll_start(void)
{
    if (*T_VS != NUL && *T_CVS != NUL)
    {
	out_str(T_VS);
	out_str(T_CVS);
	screen_start();		/* don't know where cursor is now */
    }
}

static int cursor_is_off = FALSE;

/*
 * Enable the cursor without checking if it's already enabled.
 */
    void
cursor_on_force(void)
{
    out_str(T_VE);
    cursor_is_off = FALSE;
}

/*
 * Enable the cursor if it's currently off.
 */
    void
cursor_on(void)
{
    if (cursor_is_off)
	cursor_on_force();
}

/*
 * Disable the cursor.
 */
    void
cursor_off(void)
{
    if (full_screen && !cursor_is_off)
    {
	out_str(T_VI);	    /* disable cursor */
	cursor_is_off = TRUE;
    }
}

#if defined(CURSOR_SHAPE) || defined(PROTO)
/*
 * Set cursor shape to match Insert or Replace mode.
 */
    void
term_cursor_mode(int forced)
{
    static int showing_mode = -1;
    char_u *p;

    /* Only do something when redrawing the screen and we can restore the
     * mode. */
    if (!full_screen || *T_CEI == NUL)
    {
# ifdef FEAT_TERMRESPONSE
	if (forced && initial_cursor_shape > 0)
	    /* Restore to initial values. */
	    term_cursor_shape(initial_cursor_shape, initial_cursor_blink);
# endif
	return;
    }

    if ((State & REPLACE) == REPLACE)
    {
	if (forced || showing_mode != REPLACE)
	{
	    if (*T_CSR != NUL)
		p = T_CSR;	/* Replace mode cursor */
	    else
		p = T_CSI;	/* fall back to Insert mode cursor */
	    if (*p != NUL)
	    {
		out_str(p);
		showing_mode = REPLACE;
	    }
	}
    }
    else if (State & INSERT)
    {
	if ((forced || showing_mode != INSERT) && *T_CSI != NUL)
	{
	    out_str(T_CSI);	    /* Insert mode cursor */
	    showing_mode = INSERT;
	}
    }
    else if (forced || showing_mode != NORMAL)
    {
	out_str(T_CEI);		    /* non-Insert mode cursor */
	showing_mode = NORMAL;
    }
}

# if defined(FEAT_TERMINAL) || defined(PROTO)
    void
term_cursor_color(char_u *color)
{
    if (*T_CSC != NUL)
    {
	out_str(T_CSC);			/* set cursor color start */
	out_str_nf(color);
	out_str(T_CEC);			/* set cursor color end */
	out_flush();
    }
}
# endif

    int
blink_state_is_inverted()
{
#ifdef FEAT_TERMRESPONSE
    return rbm_status == STATUS_GOT && rcs_status == STATUS_GOT
		&& initial_cursor_blink != initial_cursor_shape_blink;
#else
    return FALSE;
#endif
}

/*
 * "shape": 1 = block, 2 = underline, 3 = vertical bar
 */
    void
term_cursor_shape(int shape, int blink)
{
    if (*T_CSH != NUL)
    {
	OUT_STR(tgoto((char *)T_CSH, 0, shape * 2 - blink));
	out_flush();
    }
    else
    {
	int do_blink = blink;

	/* t_SH is empty: try setting just the blink state.
	 * The blink flags are XORed together, if the initial blinking from
	 * style and shape differs, we need to invert the flag here. */
	if (blink_state_is_inverted())
	    do_blink = !blink;

	if (do_blink && *T_VS != NUL)
	{
	    out_str(T_VS);
	    out_flush();
	}
	else if (!do_blink && *T_CVS != NUL)
	{
	    out_str(T_CVS);
	    out_flush();
	}
    }
}
#endif

/*
 * Set scrolling region for window 'wp'.
 * The region starts 'off' lines from the start of the window.
 * Also set the vertical scroll region for a vertically split window.  Always
 * the full width of the window, excluding the vertical separator.
 */
    void
scroll_region_set(win_T *wp, int off)
{
    OUT_STR(tgoto((char *)T_CS, W_WINROW(wp) + wp->w_height - 1,
							 W_WINROW(wp) + off));
    if (*T_CSV != NUL && wp->w_width != Columns)
	OUT_STR(tgoto((char *)T_CSV, wp->w_wincol + wp->w_width - 1,
							       wp->w_wincol));
    screen_start();		    /* don't know where cursor is now */
}

/*
 * Reset scrolling region to the whole screen.
 */
    void
scroll_region_reset(void)
{
    OUT_STR(tgoto((char *)T_CS, (int)Rows - 1, 0));
    if (*T_CSV != NUL)
	OUT_STR(tgoto((char *)T_CSV, (int)Columns - 1, 0));
    screen_start();		    /* don't know where cursor is now */
}


/*
 * List of terminal codes that are currently recognized.
 */

static struct termcode
{
    char_u  name[2];	    /* termcap name of entry */
    char_u  *code;	    /* terminal code (in allocated memory) */
    int	    len;	    /* STRLEN(code) */
    int	    modlen;	    /* length of part before ";*~". */
} *termcodes = NULL;

static int  tc_max_len = 0; /* number of entries that termcodes[] can hold */
static int  tc_len = 0;	    /* current number of entries in termcodes[] */

static int termcode_star(char_u *code, int len);

    void
clear_termcodes(void)
{
    while (tc_len > 0)
	vim_free(termcodes[--tc_len].code);
    VIM_CLEAR(termcodes);
    tc_max_len = 0;

#ifdef HAVE_TGETENT
    BC = (char *)empty_option;
    UP = (char *)empty_option;
    PC = NUL;			/* set pad character to NUL */
    ospeed = 0;
#endif

    need_gather = TRUE;		/* need to fill termleader[] */
}

#define ATC_FROM_TERM 55

/*
 * Add a new entry to the list of terminal codes.
 * The list is kept alphabetical for ":set termcap"
 * "flags" is TRUE when replacing 7-bit by 8-bit controls is desired.
 * "flags" can also be ATC_FROM_TERM for got_code_from_term().
 */
    void
add_termcode(char_u *name, char_u *string, int flags)
{
    struct termcode *new_tc;
    int		    i, j;
    char_u	    *s;
    int		    len;

    if (string == NULL || *string == NUL)
    {
	del_termcode(name);
	return;
    }

#if defined(MSWIN) && !defined(FEAT_GUI)
    s = vim_strnsave(string, (int)STRLEN(string) + 1);
#else
    s = vim_strsave(string);
#endif
    if (s == NULL)
	return;

    /* Change leading <Esc>[ to CSI, change <Esc>O to <M-O>. */
    if (flags != 0 && flags != ATC_FROM_TERM && term_7to8bit(string) != 0)
    {
	STRMOVE(s, s + 1);
	s[0] = term_7to8bit(string);
    }

#if defined(MSWIN) && !defined(FEAT_GUI)
    if (s[0] == K_NUL)
    {
	STRMOVE(s + 1, s);
	s[1] = 3;
    }
#endif

    len = (int)STRLEN(s);

    need_gather = TRUE;		/* need to fill termleader[] */

    /*
     * need to make space for more entries
     */
    if (tc_len == tc_max_len)
    {
	tc_max_len += 20;
	new_tc = (struct termcode *)alloc(
			    (unsigned)(tc_max_len * sizeof(struct termcode)));
	if (new_tc == NULL)
	{
	    tc_max_len -= 20;
	    return;
	}
	for (i = 0; i < tc_len; ++i)
	    new_tc[i] = termcodes[i];
	vim_free(termcodes);
	termcodes = new_tc;
    }

    /*
     * Look for existing entry with the same name, it is replaced.
     * Look for an existing entry that is alphabetical higher, the new entry
     * is inserted in front of it.
     */
    for (i = 0; i < tc_len; ++i)
    {
	if (termcodes[i].name[0] < name[0])
	    continue;
	if (termcodes[i].name[0] == name[0])
	{
	    if (termcodes[i].name[1] < name[1])
		continue;
	    /*
	     * Exact match: May replace old code.
	     */
	    if (termcodes[i].name[1] == name[1])
	    {
		if (flags == ATC_FROM_TERM && (j = termcode_star(
				    termcodes[i].code, termcodes[i].len)) > 0)
		{
		    /* Don't replace ESC[123;*X or ESC O*X with another when
		     * invoked from got_code_from_term(). */
		    if (len == termcodes[i].len - j
			    && STRNCMP(s, termcodes[i].code, len - 1) == 0
			    && s[len - 1]
				   == termcodes[i].code[termcodes[i].len - 1])
		    {
			/* They are equal but for the ";*": don't add it. */
			vim_free(s);
			return;
		    }
		}
		else
		{
		    /* Replace old code. */
		    vim_free(termcodes[i].code);
		    --tc_len;
		    break;
		}
	    }
	}
	/*
	 * Found alphabetical larger entry, move rest to insert new entry
	 */
	for (j = tc_len; j > i; --j)
	    termcodes[j] = termcodes[j - 1];
	break;
    }

    termcodes[i].name[0] = name[0];
    termcodes[i].name[1] = name[1];
    termcodes[i].code = s;
    termcodes[i].len = len;

    /* For xterm we recognize special codes like "ESC[42;*X" and "ESC O*X" that
     * accept modifiers. */
    termcodes[i].modlen = 0;
    j = termcode_star(s, len);
    if (j > 0)
	termcodes[i].modlen = len - 1 - j;
    ++tc_len;
}

/*
 * Check termcode "code[len]" for ending in ;*X or *X.
 * The "X" can be any character.
 * Return 0 if not found, 2 for ;*X and 1 for *X.
 */
    static int
termcode_star(char_u *code, int len)
{
    /* Shortest is <M-O>*X.  With ; shortest is <CSI>1;*X */
    if (len >= 3 && code[len - 2] == '*')
    {
	if (len >= 5 && code[len - 3] == ';')
	    return 2;
	else
	    return 1;
    }
    return 0;
}

    char_u  *
find_termcode(char_u *name)
{
    int	    i;

    for (i = 0; i < tc_len; ++i)
	if (termcodes[i].name[0] == name[0] && termcodes[i].name[1] == name[1])
	    return termcodes[i].code;
    return NULL;
}

#if defined(FEAT_CMDL_COMPL) || defined(PROTO)
    char_u *
get_termcode(int i)
{
    if (i >= tc_len)
	return NULL;
    return &termcodes[i].name[0];
}
#endif

    void
del_termcode(char_u *name)
{
    int	    i;

    if (termcodes == NULL)	/* nothing there yet */
	return;

    need_gather = TRUE;		/* need to fill termleader[] */

    for (i = 0; i < tc_len; ++i)
	if (termcodes[i].name[0] == name[0] && termcodes[i].name[1] == name[1])
	{
	    del_termcode_idx(i);
	    return;
	}
    /* not found. Give error message? */
}

    static void
del_termcode_idx(int idx)
{
    int		i;

    vim_free(termcodes[idx].code);
    --tc_len;
    for (i = idx; i < tc_len; ++i)
	termcodes[i] = termcodes[i + 1];
}

#ifdef FEAT_TERMRESPONSE
/*
 * Called when detected that the terminal sends 8-bit codes.
 * Convert all 7-bit codes to their 8-bit equivalent.
 */
    static void
switch_to_8bit(void)
{
    int		i;
    int		c;

    /* Only need to do something when not already using 8-bit codes. */
    if (!term_is_8bit(T_NAME))
    {
	for (i = 0; i < tc_len; ++i)
	{
	    c = term_7to8bit(termcodes[i].code);
	    if (c != 0)
	    {
		STRMOVE(termcodes[i].code + 1, termcodes[i].code + 2);
		termcodes[i].code[0] = c;
	    }
	}
	need_gather = TRUE;		/* need to fill termleader[] */
    }
    detected_8bit = TRUE;
    LOG_TR(("Switching to 8 bit"));
}
#endif

#ifdef CHECK_DOUBLE_CLICK
static linenr_T orig_topline = 0;
# ifdef FEAT_DIFF
static int orig_topfill = 0;
# endif
#endif
#if defined(CHECK_DOUBLE_CLICK) || defined(PROTO)
/*
 * Checking for double clicks ourselves.
 * "orig_topline" is used to avoid detecting a double-click when the window
 * contents scrolled (e.g., when 'scrolloff' is non-zero).
 */
/*
 * Set orig_topline.  Used when jumping to another window, so that a double
 * click still works.
 */
    void
set_mouse_topline(win_T *wp)
{
    orig_topline = wp->w_topline;
# ifdef FEAT_DIFF
    orig_topfill = wp->w_topfill;
# endif
}
#endif

/*
 * Check if typebuf.tb_buf[] contains a terminal key code.
 * Check from typebuf.tb_buf[typebuf.tb_off] to typebuf.tb_buf[typebuf.tb_off
 * + max_offset].
 * Return 0 for no match, -1 for partial match, > 0 for full match.
 * Return KEYLEN_REMOVED when a key code was deleted.
 * With a match, the match is removed, the replacement code is inserted in
 * typebuf.tb_buf[] and the number of characters in typebuf.tb_buf[] is
 * returned.
 * When "buf" is not NULL, buf[bufsize] is used instead of typebuf.tb_buf[].
 * "buflen" is then the length of the string in buf[] and is updated for
 * inserts and deletes.
 */
    int
check_termcode(
    int		max_offset,
    char_u	*buf,
    int		bufsize,
    int		*buflen)
{
    char_u	*tp;
    char_u	*p;
    int		slen = 0;	/* init for GCC */
    int		modslen;
    int		len;
    int		retval = 0;
    int		offset;
    char_u	key_name[2];
    int		modifiers;
    char_u	*modifiers_start = NULL;
    int		key;
    int		new_slen;
    int		extra;
    char_u	string[MAX_KEY_CODE_LEN + 1];
    int		i, j;
    int		idx = 0;
#ifdef FEAT_MOUSE
# if !defined(UNIX) || defined(FEAT_MOUSE_XTERM) || defined(FEAT_GUI) \
    || defined(FEAT_MOUSE_GPM) || defined(FEAT_SYSMOUSE)
    char_u	bytes[6];
    int		num_bytes;
# endif
    int		mouse_code = 0;	    /* init for GCC */
    int		is_click, is_drag;
    int		wheel_code = 0;
    int		current_button;
    static int	held_button = MOUSE_RELEASE;
    static int	orig_num_clicks = 1;
    static int	orig_mouse_code = 0x0;
# ifdef CHECK_DOUBLE_CLICK
    static int	orig_mouse_col = 0;
    static int	orig_mouse_row = 0;
    static struct timeval  orig_mouse_time = {0, 0};
					/* time of previous mouse click */
    struct timeval  mouse_time;		/* time of current mouse click */
    long	timediff;		/* elapsed time in msec */
# endif
#endif
    int		cpo_koffset;
#ifdef FEAT_MOUSE_GPM
    extern int	gpm_flag; /* gpm library variable */
#endif

    cpo_koffset = (vim_strchr(p_cpo, CPO_KOFFSET) != NULL);

    /*
     * Speed up the checks for terminal codes by gathering all first bytes
     * used in termleader[].  Often this is just a single <Esc>.
     */
    if (need_gather)
	gather_termleader();

    /*
     * Check at several positions in typebuf.tb_buf[], to catch something like
     * "x<Up>" that can be mapped. Stop at max_offset, because characters
     * after that cannot be used for mapping, and with @r commands
     * typebuf.tb_buf[] can become very long.
     * This is used often, KEEP IT FAST!
     */
    for (offset = 0; offset < max_offset; ++offset)
    {
	if (buf == NULL)
	{
	    if (offset >= typebuf.tb_len)
		break;
	    tp = typebuf.tb_buf + typebuf.tb_off + offset;
	    len = typebuf.tb_len - offset;	/* length of the input */
	}
	else
	{
	    if (offset >= *buflen)
		break;
	    tp = buf + offset;
	    len = *buflen - offset;
	}

	/*
	 * Don't check characters after K_SPECIAL, those are already
	 * translated terminal chars (avoid translating ~@^Hx).
	 */
	if (*tp == K_SPECIAL)
	{
	    offset += 2;	/* there are always 2 extra characters */
	    continue;
	}

	/*
	 * Skip this position if the character does not appear as the first
	 * character in term_strings. This speeds up a lot, since most
	 * termcodes start with the same character (ESC or CSI).
	 */
	i = *tp;
	for (p = termleader; *p && *p != i; ++p)
	    ;
	if (*p == NUL)
	    continue;

	/*
	 * Skip this position if p_ek is not set and tp[0] is an ESC and we
	 * are in Insert mode.
	 */
	if (*tp == ESC && !p_ek && (State & INSERT))
	    continue;

	key_name[0] = NUL;	/* no key name found yet */
	key_name[1] = NUL;	/* no key name found yet */
	modifiers = 0;		/* no modifiers yet */

#ifdef FEAT_GUI
	if (gui.in_use)
	{
	    /*
	     * GUI special key codes are all of the form [CSI xx].
	     */
	    if (*tp == CSI)	    /* Special key from GUI */
	    {
		if (len < 3)
		    return -1;	    /* Shouldn't happen */
		slen = 3;
		key_name[0] = tp[1];
		key_name[1] = tp[2];
	    }
	}
	else
#endif /* FEAT_GUI */
	{
	    for (idx = 0; idx < tc_len; ++idx)
	    {
		/*
		 * Ignore the entry if we are not at the start of
		 * typebuf.tb_buf[]
		 * and there are not enough characters to make a match.
		 * But only when the 'K' flag is in 'cpoptions'.
		 */
		slen = termcodes[idx].len;
		modifiers_start = NULL;
		if (cpo_koffset && offset && len < slen)
		    continue;
		if (STRNCMP(termcodes[idx].code, tp,
				     (size_t)(slen > len ? len : slen)) == 0)
		{
		    if (len < slen)		/* got a partial sequence */
			return -1;		/* need to get more chars */

		    /*
		     * When found a keypad key, check if there is another key
		     * that matches and use that one.  This makes <Home> to be
		     * found instead of <kHome> when they produce the same
		     * key code.
		     */
		    if (termcodes[idx].name[0] == 'K'
				       && VIM_ISDIGIT(termcodes[idx].name[1]))
		    {
			for (j = idx + 1; j < tc_len; ++j)
			    if (termcodes[j].len == slen &&
				    STRNCMP(termcodes[idx].code,
					    termcodes[j].code, slen) == 0)
			    {
				idx = j;
				break;
			    }
		    }

		    key_name[0] = termcodes[idx].name[0];
		    key_name[1] = termcodes[idx].name[1];
		    break;
		}

		/*
		 * Check for code with modifier, like xterm uses:
		 * <Esc>[123;*X  (modslen == slen - 3)
		 * Also <Esc>O*X and <M-O>*X (modslen == slen - 2).
		 * When there is a modifier the * matches a number.
		 * When there is no modifier the ;* or * is omitted.
		 */
		if (termcodes[idx].modlen > 0)
		{
		    modslen = termcodes[idx].modlen;
		    if (cpo_koffset && offset && len < modslen)
			continue;
		    if (STRNCMP(termcodes[idx].code, tp,
				(size_t)(modslen > len ? len : modslen)) == 0)
		    {
			int	    n;

			if (len <= modslen)	/* got a partial sequence */
			    return -1;		/* need to get more chars */

			if (tp[modslen] == termcodes[idx].code[slen - 1])
			    slen = modslen + 1;	/* no modifiers */
			else if (tp[modslen] != ';' && modslen == slen - 3)
			    continue;	/* no match */
			else
			{
			    /* Skip over the digits, the final char must
			     * follow. */
			    for (j = slen - 2; j < len && (isdigit(tp[j])
							 || tp[j] == ';'); ++j)
				;
			    ++j;
			    if (len < j)	/* got a partial sequence */
				return -1;	/* need to get more chars */
			    if (tp[j - 1] != termcodes[idx].code[slen - 1])
				continue;	/* no match */

			    modifiers_start = tp + slen - 2;

			    /* Match!  Convert modifier bits. */
			    n = atoi((char *)modifiers_start) - 1;
			    if (n & 1)
				modifiers |= MOD_MASK_SHIFT;
			    if (n & 2)
				modifiers |= MOD_MASK_ALT;
			    if (n & 4)
				modifiers |= MOD_MASK_CTRL;
			    if (n & 8)
				modifiers |= MOD_MASK_META;

			    slen = j;
			}
			key_name[0] = termcodes[idx].name[0];
			key_name[1] = termcodes[idx].name[1];
			break;
		    }
		}
	    }
	}

#ifdef FEAT_TERMRESPONSE
	if (key_name[0] == NUL
	    /* Mouse codes of DEC and pterm start with <ESC>[.  When
	     * detecting the start of these mouse codes they might as well be
	     * another key code or terminal response. */
# ifdef FEAT_MOUSE_DEC
	    || key_name[0] == KS_DEC_MOUSE
# endif
# ifdef FEAT_MOUSE_PTERM
	    || key_name[0] == KS_PTERM_MOUSE
# endif
	   )
	{
	    /* Check for some responses from the terminal starting with
	     * "<Esc>[" or CSI:
	     *
	     * - Xterm version string: <Esc>[>{x};{vers};{y}c
	     *   Libvterm returns {x} == 0, {vers} == 100, {y} == 0.
	     *   Also eat other possible responses to t_RV, rxvt returns
	     *   "<Esc>[?1;2c". Also accept CSI instead of <Esc>[.
	     *   mrxvt has been reported to have "+" in the version. Assume
	     *   the escape sequence ends with a letter or one of "{|}~".
	     *
	     * - Cursor position report: <Esc>[{row};{col}R
	     *   The final byte must be 'R'. It is used for checking the
	     *   ambiguous-width character state.
	     *
	     * - window position reply: <Esc>[3;{x};{y}t
	     */
	    char_u *argp = tp[0] == ESC ? tp + 2 : tp + 1;

	    if ((*T_CRV != NUL || *T_U7 != NUL || did_request_winpos)
			&& ((tp[0] == ESC && len >= 3 && tp[1] == '[')
			    || (tp[0] == CSI && len >= 2))
			&& (VIM_ISDIGIT(*argp) || *argp == '>' || *argp == '?'))
	    {
		int col = 0;
		int semicols = 0;
		int row_char = NUL;

		extra = 0;
		for (i = 2 + (tp[0] != CSI); i < len
				&& !(tp[i] >= '{' && tp[i] <= '~')
				&& !ASCII_ISALPHA(tp[i]); ++i)
		    if (tp[i] == ';' && ++semicols == 1)
		    {
			extra = i + 1;
			row_char = tp[i - 1];
		    }
		if (i == len)
		{
		    LOG_TR(("Not enough characters for CRV"));
		    return -1;
		}
		if (extra > 0)
		    col = atoi((char *)tp + extra);

		/* Eat it when it has 2 arguments and ends in 'R'. Also when
		 * u7_status is not "sent", it may be from a previous Vim that
		 * just exited.  But not for <S-F3>, it sends something
		 * similar, check for row and column to make sense. */
		if (semicols == 1 && tp[i] == 'R')
		{
		    if (row_char == '2' && col >= 2)
		    {
			char *aw = NULL;

			LOG_TR(("Received U7 status: %s", tp));
			u7_status = STATUS_GOT;
			did_cursorhold = TRUE;
			if (col == 2)
			    aw = "single";
			else if (col == 3)
			    aw = "double";
			if (aw != NULL && STRCMP(aw, p_ambw) != 0)
			{
			    /* Setting the option causes a screen redraw. Do
			     * that right away if possible, keeping any
			     * messages. */
			    set_option_value((char_u *)"ambw", 0L,
					     (char_u *)aw, 0);
# ifdef DEBUG_TERMRESPONSE
			    {
				int r = redraw_asap(CLEAR);

				log_tr("set 'ambiwidth', redraw_asap(): %d", r);
			    }
# else
			    redraw_asap(CLEAR);
# endif
			}
		    }
		    key_name[0] = (int)KS_EXTRA;
		    key_name[1] = (int)KE_IGNORE;
		    slen = i + 1;
# ifdef FEAT_EVAL
		    set_vim_var_string(VV_TERMU7RESP, tp, slen);
# endif
		}
		/* eat it when at least one digit and ending in 'c' */
		else if (*T_CRV != NUL && i > 2 + (tp[0] != CSI)
							       && tp[i] == 'c')
		{
		    int version = col;

		    LOG_TR(("Received CRV response: %s", tp));
		    crv_status = STATUS_GOT;
		    did_cursorhold = TRUE;

		    /* If this code starts with CSI, you can bet that the
		     * terminal uses 8-bit codes. */
		    if (tp[0] == CSI)
			switch_to_8bit();

		    /* rxvt sends its version number: "20703" is 2.7.3.
		     * Screen sends 40500.
		     * Ignore it for when the user has set 'term' to xterm,
		     * even though it's an rxvt. */
		    if (version > 20000)
			version = 0;

		    if (tp[1 + (tp[0] != CSI)] == '>' && semicols == 2)
		    {
			int need_flush = FALSE;
			int is_iterm2 = FALSE;
			int is_mintty = FALSE;

			// mintty 2.9.5 sends 77;20905;0c.
			// (77 is ASCII 'M' for mintty.)
			if (STRNCMP(tp + extra - 3, "77;", 3) == 0)
			    is_mintty = TRUE;

			/* if xterm version >= 141 try to get termcap codes */
			if (version >= 141)
			{
			    LOG_TR(("Enable checking for XT codes"));
			    check_for_codes = TRUE;
			    need_gather = TRUE;
			    req_codes_from_term();
			}

			/* libvterm sends 0;100;0 */
			if (version == 100
				&& STRNCMP(tp + extra - 2, "0;100;0c", 8) == 0)
			{
			    /* If run from Vim $COLORS is set to the number of
			     * colors the terminal supports.  Otherwise assume
			     * 256, libvterm supports even more. */
			    if (mch_getenv((char_u *)"COLORS") == NULL)
				may_adjust_color_count(256);
			    /* Libvterm can handle SGR mouse reporting. */
			    if (!option_was_set((char_u *)"ttym"))
				set_option_value((char_u *)"ttym", 0L,
							   (char_u *)"sgr", 0);
			}

			if (version == 95)
			{
			    // Mac Terminal.app sends 1;95;0
			    if (STRNCMP(tp + extra - 2, "1;95;0c", 7) == 0)
			    {
				is_not_xterm = TRUE;
				is_mac_terminal = TRUE;
			    }
			    // iTerm2 sends 0;95;0
			    if (STRNCMP(tp + extra - 2, "0;95;0c", 7) == 0)
				is_iterm2 = TRUE;
			    // old iTerm2 sends 0;95;
			    else if (STRNCMP(tp + extra - 2, "0;95;c", 6) == 0)
				is_not_xterm = TRUE;
			}

			/* Only set 'ttymouse' automatically if it was not set
			 * by the user already. */
			if (!option_was_set((char_u *)"ttym"))
			{
			    /* Xterm version 277 supports SGR.  Also support
			     * Terminal.app, iTerm2 and mintty. */
			    if (version >= 277 || is_iterm2 || is_mac_terminal
				    || is_mintty)
				set_option_value((char_u *)"ttym", 0L,
							  (char_u *)"sgr", 0);
			    /* if xterm version >= 95 use mouse dragging */
			    else if (version >= 95)
				set_option_value((char_u *)"ttym", 0L,
						       (char_u *)"xterm2", 0);
			}

			/* Detect terminals that set $TERM to something like
			 * "xterm-256colors"  but are not fully xterm
			 * compatible. */

			/* Gnome terminal sends 1;3801;0, 1;4402;0 or 1;2501;0.
			 * xfce4-terminal sends 1;2802;0.
			 * screen sends 83;40500;0
			 * Assuming any version number over 2500 is not an
			 * xterm (without the limit for rxvt and screen). */
			if (col >= 2500)
			    is_not_xterm = TRUE;

			/* PuTTY sends 0;136;0
			 * vandyke SecureCRT sends 1;136;0 */
			if (version == 136
				&& STRNCMP(tp + extra - 1, ";136;0c", 7) == 0)
			    is_not_xterm = TRUE;

			/* Konsole sends 0;115;0 */
			if (version == 115
				&& STRNCMP(tp + extra - 2, "0;115;0c", 8) == 0)
			    is_not_xterm = TRUE;

			// Xterm first responded to this request at patch level
			// 95, so assume anything below 95 is not xterm.
			if (version < 95)
			    is_not_xterm = TRUE;

			/* Only request the cursor style if t_SH and t_RS are
			 * set. Only supported properly by xterm since version
			 * 279 (otherwise it returns 0x18).
			 * Not for Terminal.app, it can't handle t_RS, it
			 * echoes the characters to the screen. */
			if (rcs_status == STATUS_GET
				&& version >= 279
				&& !is_not_xterm
				&& *T_CSH != NUL
				&& *T_CRS != NUL)
			{
			    LOG_TR(("Sending cursor style request"));
			    out_str(T_CRS);
			    rcs_status = STATUS_SENT;
			    need_flush = TRUE;
			}

			/* Only request the cursor blink mode if t_RC set. Not
			 * for Gnome terminal, it can't handle t_RC, it
			 * echoes the characters to the screen. */
			if (rbm_status == STATUS_GET
				&& !is_not_xterm
				&& *T_CRC != NUL)
			{
			    LOG_TR(("Sending cursor blink mode request"));
			    out_str(T_CRC);
			    rbm_status = STATUS_SENT;
			    need_flush = TRUE;
			}

			if (need_flush)
			    out_flush();
		    }
		    slen = i + 1;
# ifdef FEAT_EVAL
		    set_vim_var_string(VV_TERMRESPONSE, tp, slen);
# endif
		    apply_autocmds(EVENT_TERMRESPONSE,
						   NULL, NULL, FALSE, curbuf);
		    key_name[0] = (int)KS_EXTRA;
		    key_name[1] = (int)KE_IGNORE;
		}

		/* Check blinking cursor from xterm:
		 * {lead}?12;1$y       set
		 * {lead}?12;2$y       not set
		 *
		 * {lead} can be <Esc>[ or CSI
		 */
		else if (rbm_status == STATUS_SENT
			&& tp[(j = 1 + (tp[0] == ESC))] == '?'
			&& i == j + 6
			&& tp[j + 1] == '1'
			&& tp[j + 2] == '2'
			&& tp[j + 3] == ';'
			&& tp[i - 1] == '$'
			&& tp[i] == 'y')
		{
		    initial_cursor_blink = (tp[j + 4] == '1');
		    rbm_status = STATUS_GOT;
		    LOG_TR(("Received cursor blinking mode response: %s", tp));
		    key_name[0] = (int)KS_EXTRA;
		    key_name[1] = (int)KE_IGNORE;
		    slen = i + 1;
# ifdef FEAT_EVAL
		    set_vim_var_string(VV_TERMBLINKRESP, tp, slen);
# endif
		}

		/*
		 * Check for a window position response from the terminal:
		 *       {lead}3;{x}:{y}t
		 */
		else if (did_request_winpos
			    && ((len >= 4 && tp[0] == ESC && tp[1] == '[')
				|| (len >= 3 && tp[0] == CSI))
			    && tp[(j = 1 + (tp[0] == ESC))] == '3'
			    && tp[j + 1] == ';')
		{
		    j += 2;
		    for (i = j; i < len && vim_isdigit(tp[i]); ++i)
			;
		    if (i < len && tp[i] == ';')
		    {
			winpos_x = atoi((char *)tp + j);
			j = i + 1;
			for (i = j; i < len && vim_isdigit(tp[i]); ++i)
			    ;
			if (i < len && tp[i] == 't')
			{
			    winpos_y = atoi((char *)tp + j);
			    /* got finished code: consume it */
			    key_name[0] = (int)KS_EXTRA;
			    key_name[1] = (int)KE_IGNORE;
			    slen = i + 1;

			    if (--did_request_winpos <= 0)
				winpos_status = STATUS_GOT;
			}
		    }
		    if (i == len)
		    {
			LOG_TR(("not enough characters for winpos"));
			return -1;
		    }
		}
	    }

	    /* Check for fore/background color response from the terminal:
	     *
	     *       {lead}{code};rgb:{rrrr}/{gggg}/{bbbb}{tail}
	     *
	     * {code} is 10 for foreground, 11 for background
	     * {lead} can be <Esc>] or OSC
	     * {tail} can be '\007', <Esc>\ or STERM.
	     *
	     * Consume any code that starts with "{lead}11;", it's also
	     * possible that "rgba" is following.
	     */
	    else if ((*T_RBG != NUL || *T_RFG != NUL)
			&& ((tp[0] == ESC && len >= 2 && tp[1] == ']')
			    || tp[0] == OSC))
	    {
		j = 1 + (tp[0] == ESC);
		if (len >= j + 3 && (argp[0] != '1'
					 || (argp[1] != '1' && argp[1] != '0')
					 || argp[2] != ';'))
		  i = 0; /* no match */
		else
		  for (i = j; i < len; ++i)
		    if (tp[i] == '\007' || (tp[0] == OSC ? tp[i] == STERM
			: (tp[i] == ESC && i + 1 < len && tp[i + 1] == '\\')))
		    {
			int is_bg = argp[1] == '1';

			if (i - j >= 21 && STRNCMP(tp + j + 3, "rgb:", 4) == 0
			    && tp[j + 11] == '/' && tp[j + 16] == '/')
			{
#ifdef FEAT_TERMINAL
			    int rval = hexhex2nr(tp + j + 7);
			    int gval = hexhex2nr(tp + j + 12);
			    int bval = hexhex2nr(tp + j + 17);
#endif
			    if (is_bg)
			    {
				char *newval = (3 * '6' < tp[j+7] + tp[j+12]
						+ tp[j+17]) ? "light" : "dark";

				LOG_TR(("Received RBG response: %s", tp));
				rbg_status = STATUS_GOT;
#ifdef FEAT_TERMINAL
				bg_r = rval;
				bg_g = gval;
				bg_b = bval;
#endif
				if (!option_was_set((char_u *)"bg")
						  && STRCMP(p_bg, newval) != 0)
				{
				    /* value differs, apply it */
				    set_option_value((char_u *)"bg", 0L,
							  (char_u *)newval, 0);
				    reset_option_was_set((char_u *)"bg");
				    redraw_asap(CLEAR);
				}
			    }
#ifdef FEAT_TERMINAL
			    else
			    {
				LOG_TR(("Received RFG response: %s", tp));
				rfg_status = STATUS_GOT;
				fg_r = rval;
				fg_g = gval;
				fg_b = bval;
			    }
#endif
			}

			/* got finished code: consume it */
			key_name[0] = (int)KS_EXTRA;
			key_name[1] = (int)KE_IGNORE;
			slen = i + 1 + (tp[i] == ESC);
# ifdef FEAT_EVAL
			set_vim_var_string(is_bg ? VV_TERMRBGRESP
						   : VV_TERMRFGRESP, tp, slen);
# endif
			break;
		    }
		if (i == len)
		{
		    LOG_TR(("not enough characters for RB"));
		    return -1;
		}
	    }

	    /* Check for key code response from xterm:
	     * {lead}{flag}+r<hex bytes><{tail}
	     *
	     * {lead} can be <Esc>P or DCS
	     * {flag} can be '0' or '1'
	     * {tail} can be Esc>\ or STERM
	     *
	     * Check for cursor shape response from xterm:
	     * {lead}1$r<digit> q{tail}
	     *
	     * {lead} can be <Esc>P or DCS
	     * {tail} can be Esc>\ or STERM
	     *
	     * Consume any code that starts with "{lead}.+r" or "{lead}.$r".
	     */
	    else if ((check_for_codes || rcs_status == STATUS_SENT)
		    && ((tp[0] == ESC && len >= 2 && tp[1] == 'P')
			|| tp[0] == DCS))
	    {
		j = 1 + (tp[0] == ESC);
		if (len < j + 3)
		    i = len; /* need more chars */
		else if ((argp[1] != '+' && argp[1] != '$') || argp[2] != 'r')
		  i = 0; /* no match */
		else if (argp[1] == '+')
		  /* key code response */
		  for (i = j; i < len; ++i)
		  {
		    if ((tp[i] == ESC && i + 1 < len && tp[i + 1] == '\\')
			    || tp[i] == STERM)
		    {
			if (i - j >= 3)
			    got_code_from_term(tp + j, i);
			key_name[0] = (int)KS_EXTRA;
			key_name[1] = (int)KE_IGNORE;
			slen = i + 1 + (tp[i] == ESC);
			break;
		    }
		  }
		else
		{
		    /* Probably the cursor shape response.  Make sure that "i"
		     * is equal to "len" when there are not sufficient
		     * characters. */
		    for (i = j + 3; i < len; ++i)
		    {
			if (i - j == 3 && !isdigit(tp[i]))
			    break;
			if (i - j == 4 && tp[i] != ' ')
			    break;
			if (i - j == 5 && tp[i] != 'q')
			    break;
			if (i - j == 6 && tp[i] != ESC && tp[i] != STERM)
			    break;
			if ((i - j == 6 && tp[i] == STERM)
			 || (i - j == 7 && tp[i] == '\\'))
			{
			    int number = argp[3] - '0';

			    /* 0, 1 = block blink, 2 = block
			     * 3 = underline blink, 4 = underline
			     * 5 = vertical bar blink, 6 = vertical bar */
			    number = number == 0 ? 1 : number;
			    initial_cursor_shape = (number + 1) / 2;
			    /* The blink flag is actually inverted, compared to
			     * the value set with T_SH. */
			    initial_cursor_shape_blink =
						   (number & 1) ? FALSE : TRUE;
			    rcs_status = STATUS_GOT;
			    LOG_TR(("Received cursor shape response: %s", tp));

			    key_name[0] = (int)KS_EXTRA;
			    key_name[1] = (int)KE_IGNORE;
			    slen = i + 1;
# ifdef FEAT_EVAL
			    set_vim_var_string(VV_TERMSTYLERESP, tp, slen);
# endif
			    break;
			}
		    }
		}

		if (i == len)
		{
		    /* These codes arrive many together, each code can be
		     * truncated at any point. */
		    LOG_TR(("not enough characters for XT"));
		    return -1;
		}
	    }
	}
#endif

	if (key_name[0] == NUL)
	    continue;	    /* No match at this position, try next one */

	/* We only get here when we have a complete termcode match */

#ifdef FEAT_MOUSE
# ifdef FEAT_GUI
	/*
	 * Only in the GUI: Fetch the pointer coordinates of the scroll event
	 * so that we know which window to scroll later.
	 */
	if (gui.in_use
		&& key_name[0] == (int)KS_EXTRA
		&& (key_name[1] == (int)KE_X1MOUSE
		    || key_name[1] == (int)KE_X2MOUSE
		    || key_name[1] == (int)KE_MOUSELEFT
		    || key_name[1] == (int)KE_MOUSERIGHT
		    || key_name[1] == (int)KE_MOUSEDOWN
		    || key_name[1] == (int)KE_MOUSEUP))
	{
	    num_bytes = get_bytes_from_buf(tp + slen, bytes, 4);
	    if (num_bytes == -1)	/* not enough coordinates */
		return -1;
	    mouse_col = 128 * (bytes[0] - ' ' - 1) + bytes[1] - ' ' - 1;
	    mouse_row = 128 * (bytes[2] - ' ' - 1) + bytes[3] - ' ' - 1;
	    slen += num_bytes;
	}
	else
# endif
	/*
	 * If it is a mouse click, get the coordinates.
	 */
	if (key_name[0] == KS_MOUSE
# ifdef FEAT_MOUSE_JSB
		|| key_name[0] == KS_JSBTERM_MOUSE
# endif
# ifdef FEAT_MOUSE_NET
		|| key_name[0] == KS_NETTERM_MOUSE
# endif
# ifdef FEAT_MOUSE_DEC
		|| key_name[0] == KS_DEC_MOUSE
# endif
# ifdef FEAT_MOUSE_PTERM
		|| key_name[0] == KS_PTERM_MOUSE
# endif
# ifdef FEAT_MOUSE_URXVT
		|| key_name[0] == KS_URXVT_MOUSE
# endif
		|| key_name[0] == KS_SGR_MOUSE
		|| key_name[0] == KS_SGR_MOUSE_RELEASE)
	{
	    is_click = is_drag = FALSE;

# if !defined(UNIX) || defined(FEAT_MOUSE_XTERM) || defined(FEAT_GUI) \
	    || defined(FEAT_MOUSE_GPM) || defined(FEAT_SYSMOUSE)
	    if (key_name[0] == (int)KS_MOUSE)
	    {
		/*
		 * For xterm we get "<t_mouse>scr", where
		 *  s == encoded button state:
		 *	   0x20 = left button down
		 *	   0x21 = middle button down
		 *	   0x22 = right button down
		 *	   0x23 = any button release
		 *	   0x60 = button 4 down (scroll wheel down)
		 *	   0x61 = button 5 down (scroll wheel up)
		 *	add 0x04 for SHIFT
		 *	add 0x08 for ALT
		 *	add 0x10 for CTRL
		 *	add 0x20 for mouse drag (0x40 is drag with left button)
		 *	add 0x40 for mouse move (0x80 is move, 0x81 too)
		 *		 0x43 (drag + release) is also move
		 *  c == column + ' ' + 1 == column + 33
		 *  r == row + ' ' + 1 == row + 33
		 *
		 * The coordinates are passed on through global variables.
		 * Ugly, but this avoids trouble with mouse clicks at an
		 * unexpected moment and allows for mapping them.
		 */
		for (;;)
		{
#  ifdef FEAT_GUI
		    if (gui.in_use)
		    {
			/* GUI uses more bits for columns > 223 */
			num_bytes = get_bytes_from_buf(tp + slen, bytes, 5);
			if (num_bytes == -1)	/* not enough coordinates */
			    return -1;
			mouse_code = bytes[0];
			mouse_col = 128 * (bytes[1] - ' ' - 1)
							 + bytes[2] - ' ' - 1;
			mouse_row = 128 * (bytes[3] - ' ' - 1)
							 + bytes[4] - ' ' - 1;
		    }
		    else
#  endif
		    {
			num_bytes = get_bytes_from_buf(tp + slen, bytes, 3);
			if (num_bytes == -1)	/* not enough coordinates */
			    return -1;
			mouse_code = bytes[0];
			mouse_col = bytes[1] - ' ' - 1;
			mouse_row = bytes[2] - ' ' - 1;
		    }
		    slen += num_bytes;

		    /* If the following bytes is also a mouse code and it has
		     * the same code, dump this one and get the next.  This
		     * makes dragging a whole lot faster. */
#  ifdef FEAT_GUI
		    if (gui.in_use)
			j = 3;
		    else
#  endif
			j = termcodes[idx].len;
		    if (STRNCMP(tp, tp + slen, (size_t)j) == 0
			    && tp[slen + j] == mouse_code
			    && tp[slen + j + 1] != NUL
			    && tp[slen + j + 2] != NUL
#  ifdef FEAT_GUI
			    && (!gui.in_use
				|| (tp[slen + j + 3] != NUL
					&& tp[slen + j + 4] != NUL))
#  endif
			    )
			slen += j;
		    else
			break;
		}
	    }

	    if (key_name[0] == KS_URXVT_MOUSE
		|| key_name[0] == KS_SGR_MOUSE
		|| key_name[0] == KS_SGR_MOUSE_RELEASE)
	    {
		/* URXVT 1015 mouse reporting mode:
		 * Almost identical to xterm mouse mode, except the values
		 * are decimal instead of bytes.
		 *
		 * \033[%d;%d;%dM
		 *		  ^-- row
		 *	       ^----- column
		 *	    ^-------- code
		 *
		 * SGR 1006 mouse reporting mode:
		 * Almost identical to xterm mouse mode, except the values
		 * are decimal instead of bytes.
		 *
		 * \033[<%d;%d;%dM
		 *		   ^-- row
		 *	        ^----- column
		 *	     ^-------- code
		 *
		 * \033[<%d;%d;%dm        : mouse release event
		 *		   ^-- row
		 *	        ^----- column
		 *	     ^-------- code
		 */
		p = modifiers_start;
		if (p == NULL)
		    return -1;

		mouse_code = getdigits(&p);
		if (*p++ != ';')
		    return -1;

		/* when mouse reporting is SGR, add 32 to mouse code */
		if (key_name[0] == KS_SGR_MOUSE
				    || key_name[0] == KS_SGR_MOUSE_RELEASE)
		    mouse_code += 32;

		if (key_name[0] == KS_SGR_MOUSE_RELEASE)
		    mouse_code |= MOUSE_RELEASE;

		mouse_col = getdigits(&p) - 1;
		if (*p++ != ';')
		    return -1;

		mouse_row = getdigits(&p) - 1;

		/* The modifiers were the mouse coordinates, not the
		 * modifier keys (alt/shift/ctrl/meta) state. */
		modifiers = 0;
	    }

	if (key_name[0] == (int)KS_MOUSE
#  ifdef FEAT_MOUSE_URXVT
	    || key_name[0] == (int)KS_URXVT_MOUSE
#  endif
	    || key_name[0] == KS_SGR_MOUSE
	    || key_name[0] == KS_SGR_MOUSE_RELEASE)
	{
#  if !defined(MSWIN)
		/*
		 * Handle mouse events.
		 * Recognize the xterm mouse wheel, but not in the GUI, the
		 * Linux console with GPM and the MS-DOS or Win32 console
		 * (multi-clicks use >= 0x60).
		 */
		if (mouse_code >= MOUSEWHEEL_LOW
#   ifdef FEAT_GUI
			&& !gui.in_use
#   endif
#   ifdef FEAT_MOUSE_GPM
			&& gpm_flag == 0
#   endif
			)
		{
#   if defined(UNIX) && defined(FEAT_MOUSE_TTY)
		    if (use_xterm_mouse() > 1 && mouse_code >= 0x80)
			/* mouse-move event, using MOUSE_DRAG works */
			mouse_code = MOUSE_DRAG;
		    else
#   endif
			/* Keep the mouse_code before it's changed, so that we
			 * remember that it was a mouse wheel click. */
			wheel_code = mouse_code;
		}
#   ifdef FEAT_MOUSE_XTERM
		else if (held_button == MOUSE_RELEASE
#    ifdef FEAT_GUI
			&& !gui.in_use
#    endif
			&& (mouse_code == 0x23 || mouse_code == 0x24
			    || mouse_code == 0x40 || mouse_code == 0x41))
		{
		    /* Apparently 0x23 and 0x24 are used by rxvt scroll wheel.
		     * And 0x40 and 0x41 are used by some xterm emulator. */
		    wheel_code = mouse_code - (mouse_code >= 0x40 ? 0x40 : 0x23)
							      + MOUSEWHEEL_LOW;
		}
#   endif

#   if defined(UNIX) && defined(FEAT_MOUSE_TTY)
		else if (use_xterm_mouse() > 1)
		{
		    if (mouse_code & MOUSE_DRAG_XTERM)
			mouse_code |= MOUSE_DRAG;
		}
#   endif
#   ifdef FEAT_XCLIPBOARD
		else if (!(mouse_code & MOUSE_DRAG & ~MOUSE_CLICK_MASK))
		{
		    if ((mouse_code & MOUSE_RELEASE) == MOUSE_RELEASE)
			stop_xterm_trace();
		    else
			start_xterm_trace(mouse_code);
		}
#   endif
#  endif
	    }
# endif /* !UNIX || FEAT_MOUSE_XTERM */
# ifdef FEAT_MOUSE_NET
	    if (key_name[0] == (int)KS_NETTERM_MOUSE)
	    {
		int mc, mr;

		/* expect a rather limited sequence like: balancing {
		 * \033}6,45\r
		 * '6' is the row, 45 is the column
		 */
		p = tp + slen;
		mr = getdigits(&p);
		if (*p++ != ',')
		    return -1;
		mc = getdigits(&p);
		if (*p++ != '\r')
		    return -1;

		mouse_col = mc - 1;
		mouse_row = mr - 1;
		mouse_code = MOUSE_LEFT;
		slen += (int)(p - (tp + slen));
	    }
# endif	/* FEAT_MOUSE_NET */
# ifdef FEAT_MOUSE_JSB
	    if (key_name[0] == (int)KS_JSBTERM_MOUSE)
	    {
		int mult, val, iter, button, status;

		/* JSBTERM Input Model
		 * \033[0~zw uniq escape sequence
		 * (L-x)  Left button pressed - not pressed x not reporting
		 * (M-x)  Middle button pressed - not pressed x not reporting
		 * (R-x)  Right button pressed - not pressed x not reporting
		 * (SDmdu)  Single , Double click, m mouse move d button down
		 *						   u button up
		 *  ###   X cursor position padded to 3 digits
		 *  ###   Y cursor position padded to 3 digits
		 * (s-x)  SHIFT key pressed - not pressed x not reporting
		 * (c-x)  CTRL key pressed - not pressed x not reporting
		 * \033\\ terminating sequence
		 */

		p = tp + slen;
		button = mouse_code = 0;
		switch (*p++)
		{
		    case 'L': button = 1; break;
		    case '-': break;
		    case 'x': break; /* ignore sequence */
		    default:  return -1; /* Unknown Result */
		}
		switch (*p++)
		{
		    case 'M': button |= 2; break;
		    case '-': break;
		    case 'x': break; /* ignore sequence */
		    default:  return -1; /* Unknown Result */
		}
		switch (*p++)
		{
		    case 'R': button |= 4; break;
		    case '-': break;
		    case 'x': break; /* ignore sequence */
		    default:  return -1; /* Unknown Result */
		}
		status = *p++;
		for (val = 0, mult = 100, iter = 0; iter < 3; iter++,
							      mult /= 10, p++)
		    if (*p >= '0' && *p <= '9')
			val += (*p - '0') * mult;
		    else
			return -1;
		mouse_col = val;
		for (val = 0, mult = 100, iter = 0; iter < 3; iter++,
							      mult /= 10, p++)
		    if (*p >= '0' && *p <= '9')
			val += (*p - '0') * mult;
		    else
			return -1;
		mouse_row = val;
		switch (*p++)
		{
		    case 's': button |= 8; break;  /* SHIFT key Pressed */
		    case '-': break;  /* Not Pressed */
		    case 'x': break;  /* Not Reporting */
		    default:  return -1; /* Unknown Result */
		}
		switch (*p++)
		{
		    case 'c': button |= 16; break;  /* CTRL key Pressed */
		    case '-': break;  /* Not Pressed */
		    case 'x': break;  /* Not Reporting */
		    default:  return -1; /* Unknown Result */
		}
		if (*p++ != '\033')
		    return -1;
		if (*p++ != '\\')
		    return -1;
		switch (status)
		{
		    case 'D': /* Double Click */
		    case 'S': /* Single Click */
			if (button & 1) mouse_code |= MOUSE_LEFT;
			if (button & 2) mouse_code |= MOUSE_MIDDLE;
			if (button & 4) mouse_code |= MOUSE_RIGHT;
			if (button & 8) mouse_code |= MOUSE_SHIFT;
			if (button & 16) mouse_code |= MOUSE_CTRL;
			break;
		    case 'm': /* Mouse move */
			if (button & 1) mouse_code |= MOUSE_LEFT;
			if (button & 2) mouse_code |= MOUSE_MIDDLE;
			if (button & 4) mouse_code |= MOUSE_RIGHT;
			if (button & 8) mouse_code |= MOUSE_SHIFT;
			if (button & 16) mouse_code |= MOUSE_CTRL;
			if ((button & 7) != 0)
			{
			    held_button = mouse_code;
			    mouse_code |= MOUSE_DRAG;
			}
			is_drag = TRUE;
			showmode();
			break;
		    case 'd': /* Button Down */
			if (button & 1) mouse_code |= MOUSE_LEFT;
			if (button & 2) mouse_code |= MOUSE_MIDDLE;
			if (button & 4) mouse_code |= MOUSE_RIGHT;
			if (button & 8) mouse_code |= MOUSE_SHIFT;
			if (button & 16) mouse_code |= MOUSE_CTRL;
			break;
		    case 'u': /* Button Up */
			if (button & 1)
			    mouse_code |= MOUSE_LEFT | MOUSE_RELEASE;
			if (button & 2)
			    mouse_code |= MOUSE_MIDDLE | MOUSE_RELEASE;
			if (button & 4)
			    mouse_code |= MOUSE_RIGHT | MOUSE_RELEASE;
			if (button & 8)
			    mouse_code |= MOUSE_SHIFT;
			if (button & 16)
			    mouse_code |= MOUSE_CTRL;
			break;
		    default: return -1; /* Unknown Result */
		}

		slen += (p - (tp + slen));
	    }
# endif /* FEAT_MOUSE_JSB */
# ifdef FEAT_MOUSE_DEC
	    if (key_name[0] == (int)KS_DEC_MOUSE)
	    {
	       /* The DEC Locator Input Model
		* Netterm delivers the code sequence:
		*  \033[2;4;24;80&w  (left button down)
		*  \033[3;0;24;80&w  (left button up)
		*  \033[6;1;24;80&w  (right button down)
		*  \033[7;0;24;80&w  (right button up)
		* CSI Pe ; Pb ; Pr ; Pc ; Pp & w
		* Pe is the event code
		* Pb is the button code
		* Pr is the row coordinate
		* Pc is the column coordinate
		* Pp is the third coordinate (page number)
		* Pe, the event code indicates what event caused this report
		*    The following event codes are defined:
		*    0 - request, the terminal received an explicit request
		*	 for a locator report, but the locator is unavailable
		*    1 - request, the terminal received an explicit request
		*	 for a locator report
		*    2 - left button down
		*    3 - left button up
		*    4 - middle button down
		*    5 - middle button up
		*    6 - right button down
		*    7 - right button up
		*    8 - fourth button down
		*    9 - fourth button up
		*    10 - locator outside filter rectangle
		* Pb, the button code, ASCII decimal 0-15 indicating which
		*   buttons are down if any. The state of the four buttons
		*   on the locator correspond to the low four bits of the
		*   decimal value,
		*   "1" means button depressed
		*   0 - no buttons down,
		*   1 - right,
		*   2 - middle,
		*   4 - left,
		*   8 - fourth
		* Pr is the row coordinate of the locator position in the page,
		*   encoded as an ASCII decimal value.
		*   If Pr is omitted, the locator position is undefined
		*   (outside the terminal window for example).
		* Pc is the column coordinate of the locator position in the
		*   page, encoded as an ASCII decimal value.
		*   If Pc is omitted, the locator position is undefined
		*   (outside the terminal window for example).
		* Pp is the page coordinate of the locator position
		*   encoded as an ASCII decimal value.
		*   The page coordinate may be omitted if the locator is on
		*   page one (the default).  We ignore it anyway.
		*/
		int Pe, Pb, Pr, Pc;

		p = tp + slen;

		/* get event status */
		Pe = getdigits(&p);
		if (*p++ != ';')
		    return -1;

		/* get button status */
		Pb = getdigits(&p);
		if (*p++ != ';')
		    return -1;

		/* get row status */
		Pr = getdigits(&p);
		if (*p++ != ';')
		    return -1;

		/* get column status */
		Pc = getdigits(&p);

		/* the page parameter is optional */
		if (*p == ';')
		{
		    p++;
		    (void)getdigits(&p);
		}
		if (*p++ != '&')
		    return -1;
		if (*p++ != 'w')
		    return -1;

		mouse_code = 0;
		switch (Pe)
		{
		case  0: return -1; /* position request while unavailable */
		case  1: /* a response to a locator position request includes
			    the status of all buttons */
			 Pb &= 7;   /* mask off and ignore fourth button */
			 if (Pb & 4)
			     mouse_code  = MOUSE_LEFT;
			 if (Pb & 2)
			     mouse_code  = MOUSE_MIDDLE;
			 if (Pb & 1)
			     mouse_code  = MOUSE_RIGHT;
			 if (Pb)
			 {
			     held_button = mouse_code;
			     mouse_code |= MOUSE_DRAG;
			     WantQueryMouse = TRUE;
			 }
			 is_drag = TRUE;
			 showmode();
			 break;
		case  2: mouse_code = MOUSE_LEFT;
			 WantQueryMouse = TRUE;
			 break;
		case  3: mouse_code = MOUSE_RELEASE | MOUSE_LEFT;
			 break;
		case  4: mouse_code = MOUSE_MIDDLE;
			 WantQueryMouse = TRUE;
			 break;
		case  5: mouse_code = MOUSE_RELEASE | MOUSE_MIDDLE;
			 break;
		case  6: mouse_code = MOUSE_RIGHT;
			 WantQueryMouse = TRUE;
			 break;
		case  7: mouse_code = MOUSE_RELEASE | MOUSE_RIGHT;
			 break;
		case  8: return -1; /* fourth button down */
		case  9: return -1; /* fourth button up */
		case 10: return -1; /* mouse outside of filter rectangle */
		default: return -1; /* should never occur */
		}

		mouse_col = Pc - 1;
		mouse_row = Pr - 1;

		slen += (int)(p - (tp + slen));
	    }
# endif /* FEAT_MOUSE_DEC */
# ifdef FEAT_MOUSE_PTERM
	    if (key_name[0] == (int)KS_PTERM_MOUSE)
	    {
		int button, num_clicks, action;

		p = tp + slen;

		action = getdigits(&p);
		if (*p++ != ';')
		    return -1;

		mouse_row = getdigits(&p);
		if (*p++ != ';')
		    return -1;
		mouse_col = getdigits(&p);
		if (*p++ != ';')
		    return -1;

		button = getdigits(&p);
		mouse_code = 0;

		switch (button)
		{
		    case 4: mouse_code = MOUSE_LEFT; break;
		    case 1: mouse_code = MOUSE_RIGHT; break;
		    case 2: mouse_code = MOUSE_MIDDLE; break;
		    default: return -1;
		}

		switch (action)
		{
		    case 31: /* Initial press */
			if (*p++ != ';')
			    return -1;

			num_clicks = getdigits(&p); /* Not used */
			break;

		    case 32: /* Release */
			mouse_code |= MOUSE_RELEASE;
			break;

		    case 33: /* Drag */
			held_button = mouse_code;
			mouse_code |= MOUSE_DRAG;
			break;

		    default:
			return -1;
		}

		if (*p++ != 't')
		    return -1;

		slen += (p - (tp + slen));
	    }
# endif /* FEAT_MOUSE_PTERM */

	    /* Interpret the mouse code */
	    current_button = (mouse_code & MOUSE_CLICK_MASK);
	    if (current_button == MOUSE_RELEASE
# ifdef FEAT_MOUSE_XTERM
		    && wheel_code == 0
# endif
		    )
	    {
		/*
		 * If we get a mouse drag or release event when
		 * there is no mouse button held down (held_button ==
		 * MOUSE_RELEASE), produce a K_IGNORE below.
		 * (can happen when you hold down two buttons
		 * and then let them go, or click in the menu bar, but not
		 * on a menu, and drag into the text).
		 */
		if ((mouse_code & MOUSE_DRAG) == MOUSE_DRAG)
		    is_drag = TRUE;
		current_button = held_button;
	    }
	    else if (wheel_code == 0)
	    {
# ifdef CHECK_DOUBLE_CLICK
#  ifdef FEAT_MOUSE_GPM
#   ifdef FEAT_GUI
		/*
		 * Only for Unix, when GUI or gpm is not active, we handle
		 * multi-clicks here.
		 */
		if (gpm_flag == 0 && !gui.in_use)
#   else
		if (gpm_flag == 0)
#   endif
#  else
#   ifdef FEAT_GUI
		if (!gui.in_use)
#   endif
#  endif
		{
		    /*
		     * Compute the time elapsed since the previous mouse click.
		     */
		    gettimeofday(&mouse_time, NULL);
		    if (orig_mouse_time.tv_sec == 0)
		    {
			/*
			 * Avoid computing the difference between mouse_time
			 * and orig_mouse_time for the first click, as the
			 * difference would be huge and would cause
			 * multiplication overflow.
			 */
			timediff = p_mouset;
		    }
		    else
		    {
			timediff = (mouse_time.tv_usec
					     - orig_mouse_time.tv_usec) / 1000;
			if (timediff < 0)
			    --orig_mouse_time.tv_sec;
			timediff += (mouse_time.tv_sec
					      - orig_mouse_time.tv_sec) * 1000;
		    }
		    orig_mouse_time = mouse_time;
		    if (mouse_code == orig_mouse_code
			    && timediff < p_mouset
			    && orig_num_clicks != 4
			    && orig_mouse_col == mouse_col
			    && orig_mouse_row == mouse_row
			    && ((orig_topline == curwin->w_topline
#ifdef FEAT_DIFF
				    && orig_topfill == curwin->w_topfill
#endif
				)
				/* Double click in tab pages line also works
				 * when window contents changes. */
				|| (mouse_row == 0 && firstwin->w_winrow > 0))
			    )
			++orig_num_clicks;
		    else
			orig_num_clicks = 1;
		    orig_mouse_col = mouse_col;
		    orig_mouse_row = mouse_row;
		    orig_topline = curwin->w_topline;
#ifdef FEAT_DIFF
		    orig_topfill = curwin->w_topfill;
#endif
		}
#  if defined(FEAT_GUI) || defined(FEAT_MOUSE_GPM)
		else
		    orig_num_clicks = NUM_MOUSE_CLICKS(mouse_code);
#  endif
# else
		orig_num_clicks = NUM_MOUSE_CLICKS(mouse_code);
# endif
		is_click = TRUE;
		orig_mouse_code = mouse_code;
	    }
	    if (!is_drag)
		held_button = mouse_code & MOUSE_CLICK_MASK;

	    /*
	     * Translate the actual mouse event into a pseudo mouse event.
	     * First work out what modifiers are to be used.
	     */
	    if (orig_mouse_code & MOUSE_SHIFT)
		modifiers |= MOD_MASK_SHIFT;
	    if (orig_mouse_code & MOUSE_CTRL)
		modifiers |= MOD_MASK_CTRL;
	    if (orig_mouse_code & MOUSE_ALT)
		modifiers |= MOD_MASK_ALT;
	    if (orig_num_clicks == 2)
		modifiers |= MOD_MASK_2CLICK;
	    else if (orig_num_clicks == 3)
		modifiers |= MOD_MASK_3CLICK;
	    else if (orig_num_clicks == 4)
		modifiers |= MOD_MASK_4CLICK;

	    /* Work out our pseudo mouse event. Note that MOUSE_RELEASE gets
	     * added, then it's not mouse up/down. */
	    key_name[0] = (int)KS_EXTRA;
	    if (wheel_code != 0
			      && (wheel_code & MOUSE_RELEASE) != MOUSE_RELEASE)
	    {
		if (wheel_code & MOUSE_CTRL)
		    modifiers |= MOD_MASK_CTRL;
		if (wheel_code & MOUSE_ALT)
		    modifiers |= MOD_MASK_ALT;
		key_name[1] = (wheel_code & 1)
					? (int)KE_MOUSEUP : (int)KE_MOUSEDOWN;
		held_button = MOUSE_RELEASE;
	    }
	    else
		key_name[1] = get_pseudo_mouse_code(current_button,
							   is_click, is_drag);

	    /* Make sure the mouse position is valid.  Some terminals may
	     * return weird values. */
	    if (mouse_col >= Columns)
		mouse_col = Columns - 1;
	    if (mouse_row >= Rows)
		mouse_row = Rows - 1;
	}
#endif /* FEAT_MOUSE */

#ifdef FEAT_GUI
	/*
	 * If using the GUI, then we get menu and scrollbar events.
	 *
	 * A menu event is encoded as K_SPECIAL, KS_MENU, KE_FILLER followed by
	 * four bytes which are to be taken as a pointer to the vimmenu_T
	 * structure.
	 *
	 * A tab line event is encoded as K_SPECIAL KS_TABLINE nr, where "nr"
	 * is one byte with the tab index.
	 *
	 * A scrollbar event is K_SPECIAL, KS_VER_SCROLLBAR, KE_FILLER followed
	 * by one byte representing the scrollbar number, and then four bytes
	 * representing a long_u which is the new value of the scrollbar.
	 *
	 * A horizontal scrollbar event is K_SPECIAL, KS_HOR_SCROLLBAR,
	 * KE_FILLER followed by four bytes representing a long_u which is the
	 * new value of the scrollbar.
	 */
# ifdef FEAT_MENU
	else if (key_name[0] == (int)KS_MENU)
	{
	    long_u	val;

	    num_bytes = get_long_from_buf(tp + slen, &val);
	    if (num_bytes == -1)
		return -1;
	    current_menu = (vimmenu_T *)val;
	    slen += num_bytes;

	    /* The menu may have been deleted right after it was used, check
	     * for that. */
	    if (check_menu_pointer(root_menu, current_menu) == FAIL)
	    {
		key_name[0] = KS_EXTRA;
		key_name[1] = (int)KE_IGNORE;
	    }
	}
# endif
# ifdef FEAT_GUI_TABLINE
	else if (key_name[0] == (int)KS_TABLINE)
	{
	    /* Selecting tabline tab or using its menu. */
	    num_bytes = get_bytes_from_buf(tp + slen, bytes, 1);
	    if (num_bytes == -1)
		return -1;
	    current_tab = (int)bytes[0];
	    if (current_tab == 255)	/* -1 in a byte gives 255 */
		current_tab = -1;
	    slen += num_bytes;
	}
	else if (key_name[0] == (int)KS_TABMENU)
	{
	    /* Selecting tabline tab or using its menu. */
	    num_bytes = get_bytes_from_buf(tp + slen, bytes, 2);
	    if (num_bytes == -1)
		return -1;
	    current_tab = (int)bytes[0];
	    current_tabmenu = (int)bytes[1];
	    slen += num_bytes;
	}
# endif
# ifndef USE_ON_FLY_SCROLL
	else if (key_name[0] == (int)KS_VER_SCROLLBAR)
	{
	    long_u	val;

	    /* Get the last scrollbar event in the queue of the same type */
	    j = 0;
	    for (i = 0; tp[j] == CSI && tp[j + 1] == KS_VER_SCROLLBAR
						     && tp[j + 2] != NUL; ++i)
	    {
		j += 3;
		num_bytes = get_bytes_from_buf(tp + j, bytes, 1);
		if (num_bytes == -1)
		    break;
		if (i == 0)
		    current_scrollbar = (int)bytes[0];
		else if (current_scrollbar != (int)bytes[0])
		    break;
		j += num_bytes;
		num_bytes = get_long_from_buf(tp + j, &val);
		if (num_bytes == -1)
		    break;
		scrollbar_value = val;
		j += num_bytes;
		slen = j;
	    }
	    if (i == 0)		/* not enough characters to make one */
		return -1;
	}
	else if (key_name[0] == (int)KS_HOR_SCROLLBAR)
	{
	    long_u	val;

	    /* Get the last horiz. scrollbar event in the queue */
	    j = 0;
	    for (i = 0; tp[j] == CSI && tp[j + 1] == KS_HOR_SCROLLBAR
						     && tp[j + 2] != NUL; ++i)
	    {
		j += 3;
		num_bytes = get_long_from_buf(tp + j, &val);
		if (num_bytes == -1)
		    break;
		scrollbar_value = val;
		j += num_bytes;
		slen = j;
	    }
	    if (i == 0)		/* not enough characters to make one */
		return -1;
	}
# endif /* !USE_ON_FLY_SCROLL */
#endif /* FEAT_GUI */

	/*
	 * Change <xHome> to <Home>, <xUp> to <Up>, etc.
	 */
	key = handle_x_keys(TERMCAP2KEY(key_name[0], key_name[1]));

	/*
	 * Add any modifier codes to our string.
	 */
	new_slen = 0;		/* Length of what will replace the termcode */
	if (modifiers != 0)
	{
	    /* Some keys have the modifier included.  Need to handle that here
	     * to make mappings work. */
	    key = simplify_key(key, &modifiers);
	    if (modifiers != 0)
	    {
		string[new_slen++] = K_SPECIAL;
		string[new_slen++] = (int)KS_MODIFIER;
		string[new_slen++] = modifiers;
	    }
	}

	/* Finally, add the special key code to our string */
	key_name[0] = KEY2TERMCAP0(key);
	key_name[1] = KEY2TERMCAP1(key);
	if (key_name[0] == KS_KEY)
	{
	    /* from ":set <M-b>=xx" */
	    if (has_mbyte)
		new_slen += (*mb_char2bytes)(key_name[1], string + new_slen);
	    else
		string[new_slen++] = key_name[1];
	}
	else if (new_slen == 0 && key_name[0] == KS_EXTRA
						  && key_name[1] == KE_IGNORE)
	{
	    /* Do not put K_IGNORE into the buffer, do return KEYLEN_REMOVED
	     * to indicate what happened. */
	    retval = KEYLEN_REMOVED;
	}
	else
	{
	    string[new_slen++] = K_SPECIAL;
	    string[new_slen++] = key_name[0];
	    string[new_slen++] = key_name[1];
	}
	string[new_slen] = NUL;
	extra = new_slen - slen;
	if (buf == NULL)
	{
	    if (extra < 0)
		/* remove matched chars, taking care of noremap */
		del_typebuf(-extra, offset);
	    else if (extra > 0)
		/* insert the extra space we need */
		ins_typebuf(string + slen, REMAP_YES, offset, FALSE, FALSE);

	    /*
	     * Careful: del_typebuf() and ins_typebuf() may have reallocated
	     * typebuf.tb_buf[]!
	     */
	    mch_memmove(typebuf.tb_buf + typebuf.tb_off + offset, string,
							    (size_t)new_slen);
	}
	else
	{
	    if (extra < 0)
		/* remove matched characters */
		mch_memmove(buf + offset, buf + offset - extra,
					   (size_t)(*buflen + offset + extra));
	    else if (extra > 0)
	    {
		/* Insert the extra space we need.  If there is insufficient
		 * space return -1. */
		if (*buflen + extra + new_slen >= bufsize)
		    return -1;
		mch_memmove(buf + offset + extra, buf + offset,
						   (size_t)(*buflen - offset));
	    }
	    mch_memmove(buf + offset, string, (size_t)new_slen);
	    *buflen = *buflen + extra + new_slen;
	}
	return retval == 0 ? (len + extra + offset) : retval;
    }

#ifdef FEAT_TERMRESPONSE
    LOG_TR(("normal character"));
#endif

    return 0;			    /* no match found */
}

#if (defined(FEAT_TERMINAL) && defined(FEAT_TERMRESPONSE)) || defined(PROTO)
/*
 * Get the text foreground color, if known.
 */
    void
term_get_fg_color(char_u *r, char_u *g, char_u *b)
{
    if (rfg_status == STATUS_GOT)
    {
	*r = fg_r;
	*g = fg_g;
	*b = fg_b;
    }
}

/*
 * Get the text background color, if known.
 */
    void
term_get_bg_color(char_u *r, char_u *g, char_u *b)
{
    if (rbg_status == STATUS_GOT)
    {
	*r = bg_r;
	*g = bg_g;
	*b = bg_b;
    }
}
#endif

/*
 * Replace any terminal code strings in from[] with the equivalent internal
 * vim representation.	This is used for the "from" and "to" part of a
 * mapping, and the "to" part of a menu command.
 * Any strings like "<C-UP>" are also replaced, unless 'cpoptions' contains
 * '<'.
 * K_SPECIAL by itself is replaced by K_SPECIAL KS_SPECIAL KE_FILLER.
 *
 * The replacement is done in result[] and finally copied into allocated
 * memory. If this all works well *bufp is set to the allocated memory and a
 * pointer to it is returned. If something fails *bufp is set to NULL and from
 * is returned.
 *
 * CTRL-V characters are removed.  When "from_part" is TRUE, a trailing CTRL-V
 * is included, otherwise it is removed (for ":map xx ^V", maps xx to
 * nothing).  When 'cpoptions' does not contain 'B', a backslash can be used
 * instead of a CTRL-V.
 */
    char_u *
replace_termcodes(
    char_u	*from,
    char_u	**bufp,
    int		from_part,
    int		do_lt,		/* also translate <lt> */
    int		special)	/* always accept <key> notation */
{
    int		i;
    int		slen;
    int		key;
    int		dlen = 0;
    char_u	*src;
    int		do_backslash;	/* backslash is a special character */
    int		do_special;	/* recognize <> key codes */
    int		do_key_code;	/* recognize raw key codes */
    char_u	*result;	/* buffer for resulting string */

    do_backslash = (vim_strchr(p_cpo, CPO_BSLASH) == NULL);
    do_special = (vim_strchr(p_cpo, CPO_SPECI) == NULL) || special;
    do_key_code = (vim_strchr(p_cpo, CPO_KEYCODE) == NULL);

    /*
     * Allocate space for the translation.  Worst case a single character is
     * replaced by 6 bytes (shifted special key), plus a NUL at the end.
     */
    result = alloc((unsigned)STRLEN(from) * 6 + 1);
    if (result == NULL)		/* out of memory */
    {
	*bufp = NULL;
	return from;
    }

    src = from;

    /*
     * Check for #n at start only: function key n
     */
    if (from_part && src[0] == '#' && VIM_ISDIGIT(src[1]))  /* function key */
    {
	result[dlen++] = K_SPECIAL;
	result[dlen++] = 'k';
	if (src[1] == '0')
	    result[dlen++] = ';';	/* #0 is F10 is "k;" */
	else
	    result[dlen++] = src[1];	/* #3 is F3 is "k3" */
	src += 2;
    }

    /*
     * Copy each byte from *from to result[dlen]
     */
    while (*src != NUL)
    {
	/*
	 * If 'cpoptions' does not contain '<', check for special key codes,
	 * like "<C-S-LeftMouse>"
	 */
	if (do_special && (do_lt || STRNCMP(src, "<lt>", 4) != 0))
	{
#ifdef FEAT_EVAL
	    /*
	     * Replace <SID> by K_SNR <script-nr> _.
	     * (room: 5 * 6 = 30 bytes; needed: 3 + <nr> + 1 <= 14)
	     */
	    if (STRNICMP(src, "<SID>", 5) == 0)
	    {
		if (current_sctx.sc_sid <= 0)
		    emsg(_(e_usingsid));
		else
		{
		    src += 5;
		    result[dlen++] = K_SPECIAL;
		    result[dlen++] = (int)KS_EXTRA;
		    result[dlen++] = (int)KE_SNR;
		    sprintf((char *)result + dlen, "%ld",
						    (long)current_sctx.sc_sid);
		    dlen += (int)STRLEN(result + dlen);
		    result[dlen++] = '_';
		    continue;
		}
	    }
#endif

	    slen = trans_special(&src, result + dlen, TRUE, FALSE);
	    if (slen)
	    {
		dlen += slen;
		continue;
	    }
	}

	/*
	 * If 'cpoptions' does not contain 'k', see if it's an actual key-code.
	 * Note that this is also checked after replacing the <> form.
	 * Single character codes are NOT replaced (e.g. ^H or DEL), because
	 * it could be a character in the file.
	 */
	if (do_key_code)
	{
	    i = find_term_bykeys(src);
	    if (i >= 0)
	    {
		result[dlen++] = K_SPECIAL;
		result[dlen++] = termcodes[i].name[0];
		result[dlen++] = termcodes[i].name[1];
		src += termcodes[i].len;
		/* If terminal code matched, continue after it. */
		continue;
	    }
	}

#ifdef FEAT_EVAL
	if (do_special)
	{
	    char_u	*p, *s, len;

	    /*
	     * Replace <Leader> by the value of "mapleader".
	     * Replace <LocalLeader> by the value of "maplocalleader".
	     * If "mapleader" or "maplocalleader" isn't set use a backslash.
	     */
	    if (STRNICMP(src, "<Leader>", 8) == 0)
	    {
		len = 8;
		p = get_var_value((char_u *)"g:mapleader");
	    }
	    else if (STRNICMP(src, "<LocalLeader>", 13) == 0)
	    {
		len = 13;
		p = get_var_value((char_u *)"g:maplocalleader");
	    }
	    else
	    {
		len = 0;
		p = NULL;
	    }
	    if (len != 0)
	    {
		/* Allow up to 8 * 6 characters for "mapleader". */
		if (p == NULL || *p == NUL || STRLEN(p) > 8 * 6)
		    s = (char_u *)"\\";
		else
		    s = p;
		while (*s != NUL)
		    result[dlen++] = *s++;
		src += len;
		continue;
	    }
	}
#endif

	/*
	 * Remove CTRL-V and ignore the next character.
	 * For "from" side the CTRL-V at the end is included, for the "to"
	 * part it is removed.
	 * If 'cpoptions' does not contain 'B', also accept a backslash.
	 */
	key = *src;
	if (key == Ctrl_V || (do_backslash && key == '\\'))
	{
	    ++src;				/* skip CTRL-V or backslash */
	    if (*src == NUL)
	    {
		if (from_part)
		    result[dlen++] = key;
		break;
	    }
	}

	/* skip multibyte char correctly */
	for (i = (*mb_ptr2len)(src); i > 0; --i)
	{
	    /*
	     * If the character is K_SPECIAL, replace it with K_SPECIAL
	     * KS_SPECIAL KE_FILLER.
	     * If compiled with the GUI replace CSI with K_CSI.
	     */
	    if (*src == K_SPECIAL)
	    {
		result[dlen++] = K_SPECIAL;
		result[dlen++] = KS_SPECIAL;
		result[dlen++] = KE_FILLER;
	    }
# ifdef FEAT_GUI
	    else if (*src == CSI)
	    {
		result[dlen++] = K_SPECIAL;
		result[dlen++] = KS_EXTRA;
		result[dlen++] = (int)KE_CSI;
	    }
# endif
	    else
		result[dlen++] = *src;
	    ++src;
	}
    }
    result[dlen] = NUL;

    /*
     * Copy the new string to allocated memory.
     * If this fails, just return from.
     */
    if ((*bufp = vim_strsave(result)) != NULL)
	from = *bufp;
    vim_free(result);
    return from;
}

/*
 * Find a termcode with keys 'src' (must be NUL terminated).
 * Return the index in termcodes[], or -1 if not found.
 */
    int
find_term_bykeys(char_u *src)
{
    int		i;
    int		slen = (int)STRLEN(src);

    for (i = 0; i < tc_len; ++i)
    {
	if (slen == termcodes[i].len
			&& STRNCMP(termcodes[i].code, src, (size_t)slen) == 0)
	    return i;
    }
    return -1;
}

/*
 * Gather the first characters in the terminal key codes into a string.
 * Used to speed up check_termcode().
 */
    static void
gather_termleader(void)
{
    int	    i;
    int	    len = 0;

#ifdef FEAT_GUI
    if (gui.in_use)
	termleader[len++] = CSI;    /* the GUI codes are not in termcodes[] */
#endif
#ifdef FEAT_TERMRESPONSE
    if (check_for_codes || *T_CRS != NUL)
	termleader[len++] = DCS;    /* the termcode response starts with DCS
				       in 8-bit mode */
#endif
    termleader[len] = NUL;

    for (i = 0; i < tc_len; ++i)
	if (vim_strchr(termleader, termcodes[i].code[0]) == NULL)
	{
	    termleader[len++] = termcodes[i].code[0];
	    termleader[len] = NUL;
	}

    need_gather = FALSE;
}

/*
 * Show all termcodes (for ":set termcap")
 * This code looks a lot like showoptions(), but is different.
 */
    void
show_termcodes(void)
{
    int		col;
    int		*items;
    int		item_count;
    int		run;
    int		row, rows;
    int		cols;
    int		i;
    int		len;

#define INC3 27	    /* try to make three columns */
#define INC2 40	    /* try to make two columns */
#define GAP 2	    /* spaces between columns */

    if (tc_len == 0)	    /* no terminal codes (must be GUI) */
	return;
    items = (int *)alloc((unsigned)(sizeof(int) * tc_len));
    if (items == NULL)
	return;

    /* Highlight title */
    msg_puts_title(_("\n--- Terminal keys ---"));

    /*
     * do the loop two times:
     * 1. display the short items (non-strings and short strings)
     * 2. display the medium items (medium length strings)
     * 3. display the long items (remaining strings)
     */
    for (run = 1; run <= 3 && !got_int; ++run)
    {
	/*
	 * collect the items in items[]
	 */
	item_count = 0;
	for (i = 0; i < tc_len; i++)
	{
	    len = show_one_termcode(termcodes[i].name,
						    termcodes[i].code, FALSE);
	    if (len <= INC3 - GAP ? run == 1
			: len <= INC2 - GAP ? run == 2
			: run == 3)
		items[item_count++] = i;
	}

	/*
	 * display the items
	 */
	if (run <= 2)
	{
	    cols = (Columns + GAP) / (run == 1 ? INC3 : INC2);
	    if (cols == 0)
		cols = 1;
	    rows = (item_count + cols - 1) / cols;
	}
	else	/* run == 3 */
	    rows = item_count;
	for (row = 0; row < rows && !got_int; ++row)
	{
	    msg_putchar('\n');			/* go to next line */
	    if (got_int)			/* 'q' typed in more */
		break;
	    col = 0;
	    for (i = row; i < item_count; i += rows)
	    {
		msg_col = col;			/* make columns */
		show_one_termcode(termcodes[items[i]].name,
					      termcodes[items[i]].code, TRUE);
		if (run == 2)
		    col += INC2;
		else
		    col += INC3;
	    }
	    out_flush();
	    ui_breakcheck();
	}
    }
    vim_free(items);
}

/*
 * Show one termcode entry.
 * Output goes into IObuff[]
 */
    int
show_one_termcode(char_u *name, char_u *code, int printit)
{
    char_u	*p;
    int		len;

    if (name[0] > '~')
    {
	IObuff[0] = ' ';
	IObuff[1] = ' ';
	IObuff[2] = ' ';
	IObuff[3] = ' ';
    }
    else
    {
	IObuff[0] = 't';
	IObuff[1] = '_';
	IObuff[2] = name[0];
	IObuff[3] = name[1];
    }
    IObuff[4] = ' ';

    p = get_special_key_name(TERMCAP2KEY(name[0], name[1]), 0);
    if (p[1] != 't')
	STRCPY(IObuff + 5, p);
    else
	IObuff[5] = NUL;
    len = (int)STRLEN(IObuff);
    do
	IObuff[len++] = ' ';
    while (len < 17);
    IObuff[len] = NUL;
    if (code == NULL)
	len += 4;
    else
	len += vim_strsize(code);

    if (printit)
    {
	msg_puts((char *)IObuff);
	if (code == NULL)
	    msg_puts("NULL");
	else
	    msg_outtrans(code);
    }
    return len;
}

#if defined(FEAT_TERMRESPONSE) || defined(PROTO)
/*
 * For Xterm >= 140 compiled with OPT_TCAP_QUERY: Obtain the actually used
 * termcap codes from the terminal itself.
 * We get them one by one to avoid a very long response string.
 */
static int xt_index_in = 0;
static int xt_index_out = 0;

    static void
req_codes_from_term(void)
{
    xt_index_out = 0;
    xt_index_in = 0;
    req_more_codes_from_term();
}

    static void
req_more_codes_from_term(void)
{
    char	buf[11];
    int		old_idx = xt_index_out;

    /* Don't do anything when going to exit. */
    if (exiting)
	return;

    /* Send up to 10 more requests out than we received.  Avoid sending too
     * many, there can be a buffer overflow somewhere. */
    while (xt_index_out < xt_index_in + 10 && key_names[xt_index_out] != NULL)
    {
	char *key_name = key_names[xt_index_out];

	LOG_TR(("Requesting XT %d: %s", xt_index_out, key_name));
	sprintf(buf, "\033P+q%02x%02x\033\\", key_name[0], key_name[1]);
	out_str_nf((char_u *)buf);
	++xt_index_out;
    }

    /* Send the codes out right away. */
    if (xt_index_out != old_idx)
	out_flush();
}

/*
 * Decode key code response from xterm: '<Esc>P1+r<name>=<string><Esc>\'.
 * A "0" instead of the "1" indicates a code that isn't supported.
 * Both <name> and <string> are encoded in hex.
 * "code" points to the "0" or "1".
 */
    static void
got_code_from_term(char_u *code, int len)
{
#define XT_LEN 100
    char_u	name[3];
    char_u	str[XT_LEN];
    int		i;
    int		j = 0;
    int		c;

    /* A '1' means the code is supported, a '0' means it isn't.
     * When half the length is > XT_LEN we can't use it.
     * Our names are currently all 2 characters. */
    if (code[0] == '1' && code[7] == '=' && len / 2 < XT_LEN)
    {
	/* Get the name from the response and find it in the table. */
	name[0] = hexhex2nr(code + 3);
	name[1] = hexhex2nr(code + 5);
	name[2] = NUL;
	for (i = 0; key_names[i] != NULL; ++i)
	{
	    if (STRCMP(key_names[i], name) == 0)
	    {
		xt_index_in = i;
		break;
	    }
	}

	LOG_TR(("Received XT %d: %s", xt_index_in, (char *)name));

	if (key_names[i] != NULL)
	{
	    for (i = 8; (c = hexhex2nr(code + i)) >= 0; i += 2)
		str[j++] = c;
	    str[j] = NUL;
	    if (name[0] == 'C' && name[1] == 'o')
	    {
		/* Color count is not a key code. */
		i = atoi((char *)str);
		may_adjust_color_count(i);
	    }
	    else
	    {
		/* First delete any existing entry with the same code. */
		i = find_term_bykeys(str);
		if (i >= 0)
		    del_termcode_idx(i);
		add_termcode(name, str, ATC_FROM_TERM);
	    }
	}
    }

    /* May request more codes now that we received one. */
    ++xt_index_in;
    req_more_codes_from_term();
}

/*
 * Check if there are any unanswered requests and deal with them.
 * This is called before starting an external program or getting direct
 * keyboard input.  We don't want responses to be send to that program or
 * handled as typed text.
 */
    static void
check_for_codes_from_term(void)
{
    int		c;

    /* If no codes requested or all are answered, no need to wait. */
    if (xt_index_out == 0 || xt_index_out == xt_index_in)
	return;

    /* Vgetc() will check for and handle any response.
     * Keep calling vpeekc() until we don't get any responses. */
    ++no_mapping;
    ++allow_keys;
    for (;;)
    {
	c = vpeekc();
	if (c == NUL)	    /* nothing available */
	    break;

	/* If a response is recognized it's replaced with K_IGNORE, must read
	 * it from the input stream.  If there is no K_IGNORE we can't do
	 * anything, break here (there might be some responses further on, but
	 * we don't want to throw away any typed chars). */
	if (c != K_SPECIAL && c != K_IGNORE)
	    break;
	c = vgetc();
	if (c != K_IGNORE)
	{
	    vungetc(c);
	    break;
	}
    }
    --no_mapping;
    --allow_keys;
}
#endif

#if defined(FEAT_CMDL_COMPL) || defined(PROTO)
/*
 * Translate an internal mapping/abbreviation representation into the
 * corresponding external one recognized by :map/:abbrev commands;
 * respects the current B/k/< settings of 'cpoption'.
 *
 * This function is called when expanding mappings/abbreviations on the
 * command-line, and for building the "Ambiguous mapping..." error message.
 *
 * It uses a growarray to build the translation string since the
 * latter can be wider than the original description. The caller has to
 * free the string afterwards.
 *
 * Returns NULL when there is a problem.
 */
    char_u *
translate_mapping(
    char_u	*str,
    int		expmap)  /* TRUE when expanding mappings on command-line */
{
    garray_T	ga;
    int		c;
    int		modifiers;
    int		cpo_bslash;
    int		cpo_special;
    int		cpo_keycode;

    ga_init(&ga);
    ga.ga_itemsize = 1;
    ga.ga_growsize = 40;

    cpo_bslash = (vim_strchr(p_cpo, CPO_BSLASH) != NULL);
    cpo_special = (vim_strchr(p_cpo, CPO_SPECI) != NULL);
    cpo_keycode = (vim_strchr(p_cpo, CPO_KEYCODE) == NULL);

    for (; *str; ++str)
    {
	c = *str;
	if (c == K_SPECIAL && str[1] != NUL && str[2] != NUL)
	{
	    modifiers = 0;
	    if (str[1] == KS_MODIFIER)
	    {
		str++;
		modifiers = *++str;
		c = *++str;
	    }
	    if (cpo_special && cpo_keycode && c == K_SPECIAL && !modifiers)
	    {
		int	i;

		/* try to find special key in termcodes */
		for (i = 0; i < tc_len; ++i)
		    if (termcodes[i].name[0] == str[1]
					    && termcodes[i].name[1] == str[2])
			break;
		if (i < tc_len)
		{
		    ga_concat(&ga, termcodes[i].code);
		    str += 2;
		    continue; /* for (str) */
		}
	    }
	    if (c == K_SPECIAL && str[1] != NUL && str[2] != NUL)
	    {
		if (expmap && cpo_special)
		{
		    ga_clear(&ga);
		    return NULL;
		}
		c = TO_SPECIAL(str[1], str[2]);
		if (c == K_ZERO)	/* display <Nul> as ^@ */
		    c = NUL;
		str += 2;
	    }
	    if (IS_SPECIAL(c) || modifiers)	/* special key */
	    {
		if (expmap && cpo_special)
		{
		    ga_clear(&ga);
		    return NULL;
		}
		ga_concat(&ga, get_special_key_name(c, modifiers));
		continue; /* for (str) */
	    }
	}
	if (c == ' ' || c == '\t' || c == Ctrl_J || c == Ctrl_V
	    || (c == '<' && !cpo_special) || (c == '\\' && !cpo_bslash))
	    ga_append(&ga, cpo_bslash ? Ctrl_V : '\\');
	if (c)
	    ga_append(&ga, c);
    }
    ga_append(&ga, NUL);
    return (char_u *)(ga.ga_data);
}
#endif

#if (defined(MSWIN) && !defined(FEAT_GUI)) || defined(PROTO)
static char ksme_str[20];
static char ksmr_str[20];
static char ksmd_str[20];

/*
 * For Win32 console: update termcap codes for existing console attributes.
 */
    void
update_tcap(int attr)
{
    struct builtin_term *p;

    p = find_builtin_term(DEFAULT_TERM);
    sprintf(ksme_str, IF_EB("\033|%dm", ESC_STR "|%dm"), attr);
    sprintf(ksmd_str, IF_EB("\033|%dm", ESC_STR "|%dm"),
				     attr | 0x08);  /* FOREGROUND_INTENSITY */
    sprintf(ksmr_str, IF_EB("\033|%dm", ESC_STR "|%dm"),
				 ((attr & 0x0F) << 4) | ((attr & 0xF0) >> 4));

    while (p->bt_string != NULL)
    {
      if (p->bt_entry == (int)KS_ME)
	  p->bt_string = &ksme_str[0];
      else if (p->bt_entry == (int)KS_MR)
	  p->bt_string = &ksmr_str[0];
      else if (p->bt_entry == (int)KS_MD)
	  p->bt_string = &ksmd_str[0];
      ++p;
    }
}

# ifdef FEAT_TERMGUICOLORS
#  define KSSIZE 20
struct ks_tbl_s
{
    int  code;		// value of KS_
    char *vtp;		// code in vtp mode
    char *vtp2;		// code in vtp2 mode
    char buf[KSSIZE];   // save buffer in non-vtp mode
    char vbuf[KSSIZE];  // save buffer in vtp mode
    char v2buf[KSSIZE]; // save buffer in vtp2 mode
    char arr[KSSIZE];   // real buffer
};

static struct ks_tbl_s ks_tbl[] =
{
    {(int)KS_ME,  "\033|0m",  "\033|0m"},   // normal
    {(int)KS_MR,  "\033|7m",  "\033|7m"},   // reverse
    {(int)KS_MD,  "\033|1m",  "\033|1m"},   // bold
    {(int)KS_SO,  "\033|91m", "\033|91m"},  // standout: bright red text
    {(int)KS_SE,  "\033|39m", "\033|39m"},  // standout end: default color
    {(int)KS_CZH, "\033|95m", "\033|95m"},  // italic: bright magenta text
    {(int)KS_CZR, "\033|0m",  "\033|0m"},   // italic end
    {(int)KS_US,  "\033|4m",  "\033|4m"},   // underscore
    {(int)KS_UE,  "\033|24m", "\033|24m"},  // underscore end
#  ifdef TERMINFO
    {(int)KS_CAB, "\033|%p1%db", "\033|%p14%dm"}, // set background color
    {(int)KS_CAF, "\033|%p1%df", "\033|%p13%dm"}, // set foreground color
    {(int)KS_CS,  "\033|%p1%d;%p2%dR", "\033|%p1%d;%p2%dR"},
    {(int)KS_CSV, "\033|%p1%d;%p2%dV", "\033|%p1%d;%p2%dV"},
#  else
    {(int)KS_CAB, "\033|%db", "\033|4%dm"}, // set background color
    {(int)KS_CAF, "\033|%df", "\033|3%dm"}, // set foreground color
    {(int)KS_CS,  "\033|%d;%dR", "\033|%d;%dR"},
    {(int)KS_CSV, "\033|%d;%dV", "\033|%d;%dV"},
#  endif
    {(int)KS_CCO, "256", "256"},	    // colors
    {(int)KS_NAME}			    // terminator
};

    static struct builtin_term *
find_first_tcap(
    char_u *name,
    int	    code)
{
    struct builtin_term *p;

    for (p = find_builtin_term(name); p->bt_string != NULL; ++p)
	if (p->bt_entry == code)
	    return p;
    return NULL;
}
# endif

/*
 * For Win32 console: replace the sequence immediately after termguicolors.
 */
    void
swap_tcap(void)
{
# ifdef FEAT_TERMGUICOLORS
    static int		init_done = FALSE;
    static int		curr_mode;
    struct ks_tbl_s	*ks;
    struct builtin_term *bt;
    int			mode;
    enum
    {
	CMODEINDEX,
	CMODE24,
	CMODE256
    };

    /* buffer initialization */
    if (!init_done)
    {
	for (ks = ks_tbl; ks->code != (int)KS_NAME; ks++)
	{
	    bt = find_first_tcap(DEFAULT_TERM, ks->code);
	    if (bt != NULL)
	    {
		STRNCPY(ks->buf, bt->bt_string, KSSIZE);
		STRNCPY(ks->vbuf, ks->vtp, KSSIZE);
		STRNCPY(ks->v2buf, ks->vtp2, KSSIZE);

		STRNCPY(ks->arr, bt->bt_string, KSSIZE);
		bt->bt_string = &ks->arr[0];
	    }
	}
	init_done = TRUE;
	curr_mode = CMODEINDEX;
    }

    if (p_tgc)
	mode = CMODE24;
    else if (t_colors >= 256)
	mode = CMODE256;
    else
	mode = CMODEINDEX;

    for (ks = ks_tbl; ks->code != (int)KS_NAME; ks++)
    {
	bt = find_first_tcap(DEFAULT_TERM, ks->code);
	if (bt != NULL)
	{
	    switch (curr_mode)
	    {
	    case CMODEINDEX:
		STRNCPY(&ks->buf[0], bt->bt_string, KSSIZE);
		break;
	    case CMODE24:
		STRNCPY(&ks->vbuf[0], bt->bt_string, KSSIZE);
		break;
	    default:
		STRNCPY(&ks->v2buf[0], bt->bt_string, KSSIZE);
	    }
	}
    }

    if (mode != curr_mode)
    {
	for (ks = ks_tbl; ks->code != (int)KS_NAME; ks++)
	{
	    bt = find_first_tcap(DEFAULT_TERM, ks->code);
	    if (bt != NULL)
	    {
		switch (mode)
		{
		case CMODEINDEX:
		    STRNCPY(bt->bt_string, &ks->buf[0], KSSIZE);
		    break;
		case CMODE24:
		    STRNCPY(bt->bt_string, &ks->vbuf[0], KSSIZE);
		    break;
		default:
		    STRNCPY(bt->bt_string, &ks->v2buf[0], KSSIZE);
		}
	    }
	}

	curr_mode = mode;
    }
# endif
}

#endif

#if defined(FEAT_GUI) || defined(FEAT_TERMGUICOLORS) || defined(PROTO)
    static int
hex_digit(int c)
{
    if (isdigit(c))
	return c - '0';
    c = TOLOWER_ASC(c);
    if (c >= 'a' && c <= 'f')
	return c - 'a' + 10;
    return 0x1ffffff;
}

    guicolor_T
gui_get_color_cmn(char_u *name)
{
    /* On MS-Windows an RGB macro is available and it produces 0x00bbggrr color
     * values as used by the MS-Windows GDI api.  It should be used only for
     * MS-Windows GDI builds. */
# if defined(RGB) && defined(MSWIN) && !defined(FEAT_GUI)
#  undef RGB
# endif
# ifndef RGB
#  define RGB(r, g, b)	((r<<16) | (g<<8) | (b))
# endif
# define LINE_LEN 100
    FILE	*fd;
    char	line[LINE_LEN];
    char_u	*fname;
    int		r, g, b, i;
    guicolor_T  color;

    struct rgbcolor_table_S {
	char_u	    *color_name;
	guicolor_T  color;
    };

    /* Only non X11 colors (not present in rgb.txt) and colors in
     * color_names[], useful when $VIMRUNTIME is not found,. */
    static struct rgbcolor_table_S rgb_table[] = {
	    {(char_u *)"black",		RGB(0x00, 0x00, 0x00)},
	    {(char_u *)"blue",		RGB(0x00, 0x00, 0xFF)},
	    {(char_u *)"brown",		RGB(0xA5, 0x2A, 0x2A)},
	    {(char_u *)"cyan",		RGB(0x00, 0xFF, 0xFF)},
	    {(char_u *)"darkblue",	RGB(0x00, 0x00, 0x8B)},
	    {(char_u *)"darkcyan",	RGB(0x00, 0x8B, 0x8B)},
	    {(char_u *)"darkgray",	RGB(0xA9, 0xA9, 0xA9)},
	    {(char_u *)"darkgreen",	RGB(0x00, 0x64, 0x00)},
	    {(char_u *)"darkgrey",	RGB(0xA9, 0xA9, 0xA9)},
	    {(char_u *)"darkmagenta",	RGB(0x8B, 0x00, 0x8B)},
	    {(char_u *)"darkred",	RGB(0x8B, 0x00, 0x00)},
	    {(char_u *)"darkyellow",	RGB(0x8B, 0x8B, 0x00)}, /* No X11 */
	    {(char_u *)"gray",		RGB(0xBE, 0xBE, 0xBE)},
	    {(char_u *)"green",		RGB(0x00, 0xFF, 0x00)},
	    {(char_u *)"grey",		RGB(0xBE, 0xBE, 0xBE)},
	    {(char_u *)"grey40",	RGB(0x66, 0x66, 0x66)},
	    {(char_u *)"grey50",	RGB(0x7F, 0x7F, 0x7F)},
	    {(char_u *)"grey90",	RGB(0xE5, 0xE5, 0xE5)},
	    {(char_u *)"lightblue",	RGB(0xAD, 0xD8, 0xE6)},
	    {(char_u *)"lightcyan",	RGB(0xE0, 0xFF, 0xFF)},
	    {(char_u *)"lightgray",	RGB(0xD3, 0xD3, 0xD3)},
	    {(char_u *)"lightgreen",	RGB(0x90, 0xEE, 0x90)},
	    {(char_u *)"lightgrey",	RGB(0xD3, 0xD3, 0xD3)},
	    {(char_u *)"lightmagenta",	RGB(0xFF, 0x8B, 0xFF)}, /* No X11 */
	    {(char_u *)"lightred",	RGB(0xFF, 0x8B, 0x8B)}, /* No X11 */
	    {(char_u *)"lightyellow",	RGB(0xFF, 0xFF, 0xE0)},
	    {(char_u *)"magenta",	RGB(0xFF, 0x00, 0xFF)},
	    {(char_u *)"red",		RGB(0xFF, 0x00, 0x00)},
	    {(char_u *)"seagreen",	RGB(0x2E, 0x8B, 0x57)},
	    {(char_u *)"white",		RGB(0xFF, 0xFF, 0xFF)},
	    {(char_u *)"yellow",	RGB(0xFF, 0xFF, 0x00)},
    };

    static struct rgbcolor_table_S *colornames_table;
    static int size = 0;

    if (name[0] == '#' && STRLEN(name) == 7)
    {
	/* Name is in "#rrggbb" format */
	color = RGB(((hex_digit(name[1]) << 4) + hex_digit(name[2])),
		    ((hex_digit(name[3]) << 4) + hex_digit(name[4])),
		    ((hex_digit(name[5]) << 4) + hex_digit(name[6])));
	if (color > 0xffffff)
	    return INVALCOLOR;
	return color;
    }

    /* Check if the name is one of the colors we know */
    for (i = 0; i < (int)(sizeof(rgb_table) / sizeof(rgb_table[0])); i++)
	if (STRICMP(name, rgb_table[i].color_name) == 0)
	    return rgb_table[i].color;

    /*
     * Last attempt. Look in the file "$VIMRUNTIME/rgb.txt".
     */
    if (size == 0)
    {
	int counting;

	// colornames_table not yet initialized
	fname = expand_env_save((char_u *)"$VIMRUNTIME/rgb.txt");
	if (fname == NULL)
	    return INVALCOLOR;

	fd = fopen((char *)fname, "rt");
	vim_free(fname);
	if (fd == NULL)
	{
	    if (p_verbose > 1)
		verb_msg(_("Cannot open $VIMRUNTIME/rgb.txt"));
	    size = -1;  // don't try again
	    return INVALCOLOR;
	}

	for (counting = 1; counting >= 0; --counting)
	{
	    if (!counting)
	    {
		colornames_table = (struct rgbcolor_table_S *)alloc(
			   (unsigned)(sizeof(struct rgbcolor_table_S) * size));
		if (colornames_table == NULL)
		{
		    fclose(fd);
		    return INVALCOLOR;
		}
		rewind(fd);
	    }
	    size = 0;

	    while (!feof(fd))
	    {
		size_t	len;
		int	pos;

		vim_ignoredp = fgets(line, LINE_LEN, fd);
		len = strlen(line);

		if (len <= 1 || line[len - 1] != '\n')
		    continue;

		line[len - 1] = '\0';

		i = sscanf(line, "%d %d %d %n", &r, &g, &b, &pos);
		if (i != 3)
		    continue;

		if (!counting)
		{
		    char_u *s = vim_strsave((char_u *)line + pos);

		    if (s == NULL)
		    {
			fclose(fd);
			return INVALCOLOR;
		    }
		    colornames_table[size].color_name = s;
		    colornames_table[size].color = (guicolor_T)RGB(r, g, b);
		}
		size++;

		// The distributed rgb.txt has less than 1000 entries. Limit to
		// 10000, just in case the file was messed up.
		if (size == 10000)
		    break;
	    }
	}
	fclose(fd);
    }

    for (i = 0; i < size; i++)
	if (STRICMP(name, colornames_table[i].color_name) == 0)
	    return colornames_table[i].color;

    return INVALCOLOR;
}

    guicolor_T
gui_get_rgb_color_cmn(int r, int g, int b)
{
    guicolor_T  color = RGB(r, g, b);

    if (color > 0xffffff)
	return INVALCOLOR;
    return color;
}
#endif

#if (defined(MSWIN) && !defined(FEAT_GUI_MSWIN)) || defined(FEAT_TERMINAL) \
	|| defined(PROTO)
static int cube_value[] = {
    0x00, 0x5F, 0x87, 0xAF, 0xD7, 0xFF
};

static int grey_ramp[] = {
    0x08, 0x12, 0x1C, 0x26, 0x30, 0x3A, 0x44, 0x4E, 0x58, 0x62, 0x6C, 0x76,
    0x80, 0x8A, 0x94, 0x9E, 0xA8, 0xB2, 0xBC, 0xC6, 0xD0, 0xDA, 0xE4, 0xEE
};

# ifdef FEAT_TERMINAL
#  include "libvterm/include/vterm.h"  // for VTERM_ANSI_INDEX_NONE
# else
#  define VTERM_ANSI_INDEX_NONE 0
# endif

static char_u ansi_table[16][4] = {
//   R    G    B   idx
  {  0,   0,   0,  1}, // black
  {224,   0,   0,  2}, // dark red
  {  0, 224,   0,  3}, // dark green
  {224, 224,   0,  4}, // dark yellow / brown
  {  0,   0, 224,  5}, // dark blue
  {224,   0, 224,  6}, // dark magenta
  {  0, 224, 224,  7}, // dark cyan
  {224, 224, 224,  8}, // light grey

  {128, 128, 128,  9}, // dark grey
  {255,  64,  64, 10}, // light red
  { 64, 255,  64, 11}, // light green
  {255, 255,  64, 12}, // yellow
  { 64,  64, 255, 13}, // light blue
  {255,  64, 255, 14}, // light magenta
  { 64, 255, 255, 15}, // light cyan
  {255, 255, 255, 16}, // white
};

    void
cterm_color2rgb(int nr, char_u *r, char_u *g, char_u *b, char_u *ansi_idx)
{
    int idx;

    if (nr < 16)
    {
	*r = ansi_table[nr][0];
	*g = ansi_table[nr][1];
	*b = ansi_table[nr][2];
	*ansi_idx = ansi_table[nr][3];
    }
    else if (nr < 232)
    {
	/* 216 color cube */
	idx = nr - 16;
	*r = cube_value[idx / 36 % 6];
	*g = cube_value[idx / 6  % 6];
	*b = cube_value[idx      % 6];
	*ansi_idx = VTERM_ANSI_INDEX_NONE;
    }
    else if (nr < 256)
    {
	/* 24 grey scale ramp */
	idx = nr - 232;
	*r = grey_ramp[idx];
	*g = grey_ramp[idx];
	*b = grey_ramp[idx];
	*ansi_idx = VTERM_ANSI_INDEX_NONE;
    }
    else
    {
	*r = 0;
	*g = 0;
	*b = 0;
	*ansi_idx = 0;
    }
}
#endif

