/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * Definitions of various common control characters.
 * For EBCDIC we have to use different values.
 */

#ifndef EBCDIC

/* IF_EB(ASCII_constant, EBCDIC_constant) */
#define IF_EB(a, b)	a

#define CharOrd(x)	((x) < 'a' ? (x) - 'A' : (x) - 'a')
#define CharOrdLow(x)	((x) - 'a')
#define CharOrdUp(x)	((x) - 'A')
#define ROT13(c, a)	(((((c) - (a)) + 13) % 26) + (a))

#define NUL		'\000'
#define BELL		'\007'
#define BS		'\010'
#define TAB		'\011'
#define NL		'\012'
#define NL_STR		(char_u *)"\012"
#define FF		'\014'
#define CAR		'\015'	/* CR is used by Mac OS X */
#define ESC		'\033'
#define ESC_STR		(char_u *)"\033"
#define ESC_STR_nc	"\033"
#define DEL		0x7f
#define DEL_STR		(char_u *)"\177"
#define CSI		0x9b	/* Control Sequence Introducer */
#define CSI_STR		"\233"
#define DCS		0x90	/* Device Control String */
#define STERM		0x9c	/* String Terminator */

#define POUND		0xA3

#define Ctrl_chr(x)	(TOUPPER_ASC(x) ^ 0x40) /* '?' -> DEL, '@' -> ^@, etc. */
#define Meta(x)		((x) | 0x80)

#define CTRL_F_STR	"\006"
#define CTRL_H_STR	"\010"
#define CTRL_V_STR	"\026"

#define Ctrl_AT		0   /* @ */
#define Ctrl_A		1
#define Ctrl_B		2
#define Ctrl_C		3
#define Ctrl_D		4
#define Ctrl_E		5
#define Ctrl_F		6
#define Ctrl_G		7
#define Ctrl_H		8
#define Ctrl_I		9
#define Ctrl_J		10
#define Ctrl_K		11
#define Ctrl_L		12
#define Ctrl_M		13
#define Ctrl_N		14
#define Ctrl_O		15
#define Ctrl_P		16
#define Ctrl_Q		17
#define Ctrl_R		18
#define Ctrl_S		19
#define Ctrl_T		20
#define Ctrl_U		21
#define Ctrl_V		22
#define Ctrl_W		23
#define Ctrl_X		24
#define Ctrl_Y		25
#define Ctrl_Z		26
			    /* CTRL- [ Left Square Bracket == ESC*/
#define Ctrl_BSL	28  /* \ BackSLash */
#define Ctrl_RSB	29  /* ] Right Square Bracket */
#define Ctrl_HAT	30  /* ^ */
#define Ctrl__		31

#else

/* EBCDIC */

/* IF_EB(ASCII_constant, EBCDIC_constant) */
#define IF_EB(a, b)	b

/*
 * Finding the position in the alphabet is not straightforward in EBCDIC.
 * There are gaps in the code table.
 * 'a' + 1 == 'b', but: 'i' + 7 == 'j' and 'r' + 8 == 's'
 */
#define CharOrd__(c) ((c) < ('j' - 'a') ? (c) : ((c) < ('s' - 'a') ? (c) - 7 : (c) - 7 - 8))
#define CharOrdLow(x) (CharOrd__((x) - 'a'))
#define CharOrdUp(x) (CharOrd__((x) - 'A'))
#define CharOrd(x) (isupper(x) ? CharOrdUp(x) : CharOrdLow(x))

#define EBCDIC_CHAR_ADD_(x) ((x) < 0?'a':(x)>25?'z':"abcdefghijklmnopqrstuvwxyz"[x])
#define EBCDIC_CHAR_ADD(c,s) (isupper(c) ? toupper(EBCDIC_CHAR_ADD_(CharOrdUp(c)+(s))) : EBCDIC_CHAR_ADD_(CharOrdLow(c)+(s)))

#define R13_(c) ("abcdefghijklmnopqrstuvwxyz"[((c) + 13) % 26])
#define ROT13(c, a)  (isupper(c) ? toupper(R13_(CharOrdUp(c))) : R13_(CharOrdLow(c)))

#define NUL		'\000'
#define BELL		'\x2f'
#define BS		'\x16'
#define TAB		'\x05'
#define NL		'\x15'
#define NL_STR		(char_u *)"\x15"
#define FF		'\x0C'
#define CAR		'\x0D'
#define ESC		'\x27'
#define ESC_STR		(char_u *)"\x27"
#define ESC_STR_nc	"\x27"
#define DEL		0x07
#define DEL_STR		(char_u *)"\007"
/* TODO: EBCDIC Code page dependent (here 1047) */
#define CSI		0x9b	/* Control Sequence Introducer */
#define CSI_STR		"\233"
#define DCS		0x90	/* Device Control String */
#define STERM		0x9c	/* String Terminator */

#define POUND		'£'

#define CTRL_F_STR	"\056"
#define CTRL_H_STR	"\026"
#define CTRL_V_STR	"\062"

#define Ctrl_AT		0x00   /* @ */
#define Ctrl_A		0x01
#define Ctrl_B		0x02
#define Ctrl_C		0x03
#define Ctrl_D		0x37
#define Ctrl_E		0x2D
#define Ctrl_F		0x2E
#define Ctrl_G		0x2F
#define Ctrl_H		0x16
#define Ctrl_I		0x05
#define Ctrl_J		0x15
#define Ctrl_K		0x0B
#define Ctrl_L		0x0C
#define Ctrl_M		0x0D
#define Ctrl_N		0x0E
#define Ctrl_O		0x0F
#define Ctrl_P		0x10
#define Ctrl_Q		0x11
#define Ctrl_R		0x12
#define Ctrl_S		0x13
#define Ctrl_T		0x3C
#define Ctrl_U		0x3D
#define Ctrl_V		0x32
#define Ctrl_W		0x26
#define Ctrl_X		0x18
#define Ctrl_Y		0x19
#define Ctrl_Z		0x3F
			    /* CTRL- [ Left Square Bracket == ESC*/
#define Ctrl_RSB	0x1D  /* ] Right Square Bracket */
#define Ctrl_BSL	0x1C  /* \ BackSLash */
#define Ctrl_HAT	0x1E  /* ^ */
#define Ctrl__		0x1F

#define Ctrl_chr(x)	(CtrlTable[(x)])
extern char CtrlTable[];

#define CtrlChar(x)	((x < ' ') ? CtrlCharTable[(x)] : 0)
extern char CtrlCharTable[];

#define MetaChar(x)	((x < ' ') ? MetaCharTable[(x)] : 0)
extern char MetaCharTable[];

#endif /* defined EBCDIC */

/*
 * Character that separates dir names in a path.
 * For MS-DOS, WIN32 and OS/2 we use a backslash.  A slash mostly works
 * fine, but there are places where it doesn't (e.g. in a command name).
 * For Acorn we use a dot.
 */
#ifdef BACKSLASH_IN_FILENAME
# define PATHSEP	psepc
# define PATHSEPSTR	pseps
#else
# ifdef RISCOS
#  define PATHSEP	'.'
#  define PATHSEPSTR	"."
# else
#  define PATHSEP	'/'
#  define PATHSEPSTR	"/"
# endif
#endif
