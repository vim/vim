/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 * Multibyte extensions partly by Sung-Hoon Baek
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */
/*
 * mbyte.c: Code specifically for handling multi-byte characters.
 *
 * The encoding used in the core is set with 'encoding'.  When 'encoding' is
 * changed, the following four variables are set (for speed).
 * Currently these types of character encodings are supported:
 *
 * "enc_dbcs"	    When non-zero it tells the type of double byte character
 *		    encoding (Chinese, Korean, Japanese, etc.).
 *		    The cell width on the display is equal to the number of
 *		    bytes.  (exception: DBCS_JPNU with first byte 0x8e)
 *		    Recognizing the first or second byte is difficult, it
 *		    requires checking a byte sequence from the start.
 * "enc_utf8"	    When TRUE use Unicode characters in UTF-8 encoding.
 *		    The cell width on the display needs to be determined from
 *		    the character value.
 *		    Recognizing bytes is easy: 0xxx.xxxx is a single-byte
 *		    char, 10xx.xxxx is a trailing byte, 11xx.xxxx is a leading
 *		    byte of a multi-byte character.
 *		    To make things complicated, up to two composing characters
 *		    are allowed.  These are drawn on top of the first char.
 *		    For most editing the sequence of bytes with composing
 *		    characters included is considered to be one character.
 * "enc_unicode"    When 2 use 16-bit Unicode characters (or UTF-16).
 *		    When 4 use 32-but Unicode characters.
 *		    Internally characters are stored in UTF-8 encoding to
 *		    avoid NUL bytes.  Conversion happens when doing I/O.
 *		    "enc_utf8" will also be TRUE.
 *
 * "has_mbyte" is set when "enc_dbcs" or "enc_utf8" is non-zero.
 *
 * If none of these is TRUE, 8-bit bytes are used for a character.  The
 * encoding isn't currently specified (TODO).
 *
 * 'encoding' specifies the encoding used in the core.  This is in registers,
 * text manipulation, buffers, etc.  Conversion has to be done when characters
 * in another encoding are received or send:
 *
 *		       clipboard
 *			   ^
 *			   | (2)
 *			   V
 *		   +---------------+
 *	      (1)  |		   | (3)
 *  keyboard ----->|	 core	   |-----> display
 *		   |		   |
 *		   +---------------+
 *			   ^
 *			   | (4)
 *			   V
 *			 file
 *
 * (1) Typed characters arrive in the current locale.  Conversion is to be
 *     done when 'encoding' is different from 'termencoding'.
 * (2) Text will be made available with the encoding specified with
 *     'encoding'.  If this is not sufficient, system-specific conversion
 *     might be required.
 * (3) For the GUI the correct font must be selected, no conversion done.
 *     Otherwise, conversion is to be done when 'encoding' differs from
 *     'termencoding'.  (Different in the GTK+ 2 port -- 'termencoding'
 *     is always used for both input and output and must always be set to
 *     "utf-8".  gui_mch_init() does this automatically.)
 * (4) The encoding of the file is specified with 'fileencoding'.  Conversion
 *     is to be done when it's different from 'encoding'.
 *
 * The viminfo file is a special case: Only text is converted, not file names.
 * Vim scripts may contain an ":encoding" command.  This has an effect for
 * some commands, like ":menutrans"
 */

#include "vim.h"

#ifdef WIN32UNIX
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <windows.h>
# ifdef WIN32
#  undef WIN32	    /* Some windows.h define WIN32, we don't want that here. */
# endif
#endif

#if (defined(WIN3264) || defined(WIN32UNIX)) && !defined(__MINGW32__)
# include <winnls.h>
#endif

#ifdef FEAT_GUI_X11
# include <X11/Intrinsic.h>
#endif
#ifdef X_LOCALE
#include <X11/Xlocale.h>
#endif

#if defined(FEAT_GUI_GTK) && defined(FEAT_XIM) && defined(HAVE_GTK2)
# include <gdk/gdkkeysyms.h>
# ifdef WIN3264
#  include <gdk/gdkwin32.h>
# else
#  include <gdk/gdkx.h>
# endif
#endif

#ifdef HAVE_WCHAR_H
# include <wchar.h>
#endif

#if 0
/* This has been disabled, because several people reported problems with the
 * wcwidth() and iswprint() library functions, esp. for Hebrew. */
# ifdef __STDC_ISO_10646__
#  define USE_WCHAR_FUNCTIONS
# endif
#endif

#if defined(FEAT_MBYTE) || defined(PROTO)

static int enc_canon_search __ARGS((char_u *name));
static int dbcs_char2len __ARGS((int c));
static int dbcs_char2bytes __ARGS((int c, char_u *buf));
static int dbcs_ptr2len __ARGS((char_u *p));
static int dbcs_char2cells __ARGS((int c));
static int dbcs_ptr2char __ARGS((char_u *p));

/* Lookup table to quickly get the length in bytes of a UTF-8 character from
 * the first byte of a UTF-8 string.  Bytes which are illegal when used as the
 * first byte have a one, because these will be used separately. */
static char utf8len_tab[256] =
{
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /*bogus*/
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, /*bogus*/
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1,
};

/*
 * XIM often causes trouble.  Define XIM_DEBUG to get a log of XIM callbacks
 * in the "xim.log" file.
 */
/* #define XIM_DEBUG */
#ifdef XIM_DEBUG
    static void
xim_log(char *s, ...)
{
    va_list arglist;
    static FILE *fd = NULL;

    if (fd == (FILE *)-1)
	return;
    if (fd == NULL)
    {
	fd = mch_fopen("xim.log", "w");
	if (fd == NULL)
	{
	    EMSG("Cannot open xim.log");
	    fd = (FILE *)-1;
	    return;
	}
    }

    va_start(arglist, s);
    vfprintf(fd, s, arglist);
    va_end(arglist);
}
#endif

#endif

#if defined(FEAT_MBYTE) || defined(FEAT_POSTSCRIPT) || defined(PROTO)
/*
 * Canonical encoding names and their properties.
 * "iso-8859-n" is handled by enc_canonize() directly.
 */
static struct
{   char *name;		int prop;		int codepage;}
enc_canon_table[] =
{
#define IDX_LATIN_1	0
    {"latin1",		ENC_8BIT + ENC_LATIN1,	1252},
#define IDX_ISO_2	1
    {"iso-8859-2",	ENC_8BIT,		0},
#define IDX_ISO_3	2
    {"iso-8859-3",	ENC_8BIT,		0},
#define IDX_ISO_4	3
    {"iso-8859-4",	ENC_8BIT,		0},
#define IDX_ISO_5	4
    {"iso-8859-5",	ENC_8BIT,		0},
#define IDX_ISO_6	5
    {"iso-8859-6",	ENC_8BIT,		0},
#define IDX_ISO_7	6
    {"iso-8859-7",	ENC_8BIT,		0},
#define IDX_ISO_8	7
    {"iso-8859-8",	ENC_8BIT,		0},
#define IDX_ISO_9	8
    {"iso-8859-9",	ENC_8BIT,		0},
#define IDX_ISO_10	9
    {"iso-8859-10",	ENC_8BIT,		0},
#define IDX_ISO_11	10
    {"iso-8859-11",	ENC_8BIT,		0},
#define IDX_ISO_13	11
    {"iso-8859-13",	ENC_8BIT,		0},
#define IDX_ISO_14	12
    {"iso-8859-14",	ENC_8BIT,		0},
#define IDX_ISO_15	13
    {"iso-8859-15",	ENC_8BIT + ENC_LATIN9,	0},
#define IDX_KOI8_R	14
    {"koi8-r",		ENC_8BIT,		0},
#define IDX_KOI8_U	15
    {"koi8-u",		ENC_8BIT,		0},
#define IDX_UTF8	16
    {"utf-8",		ENC_UNICODE,		0},
#define IDX_UCS2	17
    {"ucs-2",		ENC_UNICODE + ENC_ENDIAN_B + ENC_2BYTE, 0},
#define IDX_UCS2LE	18
    {"ucs-2le",		ENC_UNICODE + ENC_ENDIAN_L + ENC_2BYTE, 0},
#define IDX_UTF16	19
    {"utf-16",		ENC_UNICODE + ENC_ENDIAN_B + ENC_2WORD, 0},
#define IDX_UTF16LE	20
    {"utf-16le",	ENC_UNICODE + ENC_ENDIAN_L + ENC_2WORD, 0},
#define IDX_UCS4	21
    {"ucs-4",		ENC_UNICODE + ENC_ENDIAN_B + ENC_4BYTE, 0},
#define IDX_UCS4LE	22
    {"ucs-4le",		ENC_UNICODE + ENC_ENDIAN_L + ENC_4BYTE, 0},

    /* For debugging DBCS encoding on Unix. */
#define IDX_DEBUG	23
    {"debug",		ENC_DBCS,		DBCS_DEBUG},
#define IDX_EUC_JP	24
    {"euc-jp",		ENC_DBCS,		DBCS_JPNU},
#define IDX_SJIS	25
    {"sjis",		ENC_DBCS,		DBCS_JPN},
#define IDX_EUC_KR	26
    {"euc-kr",		ENC_DBCS,		DBCS_KORU},
#define IDX_EUC_CN	27
    {"euc-cn",		ENC_DBCS,		DBCS_CHSU},
#define IDX_EUC_TW	28
    {"euc-tw",		ENC_DBCS,		DBCS_CHTU},
#define IDX_BIG5	29
    {"big5",		ENC_DBCS,		DBCS_CHT},

    /* MS-DOS and MS-Windows codepages are included here, so that they can be
     * used on Unix too.  Most of them are similar to ISO-8859 encodings, but
     * not exactly the same. */
#define IDX_CP437	30
    {"cp437",		ENC_8BIT,		437}, /* like iso-8859-1 */
#define IDX_CP737	31
    {"cp737",		ENC_8BIT,		737}, /* like iso-8859-7 */
#define IDX_CP775	32
    {"cp775",		ENC_8BIT,		775}, /* Baltic */
#define IDX_CP850	33
    {"cp850",		ENC_8BIT,		850}, /* like iso-8859-4 */
#define IDX_CP852	34
    {"cp852",		ENC_8BIT,		852}, /* like iso-8859-1 */
#define IDX_CP855	35
    {"cp855",		ENC_8BIT,		855}, /* like iso-8859-2 */
#define IDX_CP857	36
    {"cp857",		ENC_8BIT,		857}, /* like iso-8859-5 */
#define IDX_CP860	37
    {"cp860",		ENC_8BIT,		860}, /* like iso-8859-9 */
#define IDX_CP861	38
    {"cp861",		ENC_8BIT,		861}, /* like iso-8859-1 */
#define IDX_CP862	39
    {"cp862",		ENC_8BIT,		862}, /* like iso-8859-1 */
#define IDX_CP863	40
    {"cp863",		ENC_8BIT,		863}, /* like iso-8859-8 */
#define IDX_CP865	41
    {"cp865",		ENC_8BIT,		865}, /* like iso-8859-1 */
#define IDX_CP866	42
    {"cp866",		ENC_8BIT,		866}, /* like iso-8859-5 */
#define IDX_CP869	43
    {"cp869",		ENC_8BIT,		869}, /* like iso-8859-7 */
#define IDX_CP874	44
    {"cp874",		ENC_8BIT,		874}, /* Thai */
#define IDX_CP932	45
    {"cp932",		ENC_DBCS,		DBCS_JPN},
#define IDX_CP936	46
    {"cp936",		ENC_DBCS,		DBCS_CHS},
#define IDX_CP949	47
    {"cp949",		ENC_DBCS,		DBCS_KOR},
#define IDX_CP950	48
    {"cp950",		ENC_DBCS,		DBCS_CHT},
#define IDX_CP1250	49
    {"cp1250",		ENC_8BIT,		1250}, /* Czech, Polish, etc. */
#define IDX_CP1251	50
    {"cp1251",		ENC_8BIT,		1251}, /* Cyrillic */
    /* cp1252 is considered to be equal to latin1 */
#define IDX_CP1253	51
    {"cp1253",		ENC_8BIT,		1253}, /* Greek */
#define IDX_CP1254	52
    {"cp1254",		ENC_8BIT,		1254}, /* Turkish */
#define IDX_CP1255	53
    {"cp1255",		ENC_8BIT,		1255}, /* Hebrew */
#define IDX_CP1256	54
    {"cp1256",		ENC_8BIT,		1256}, /* Arabic */
#define IDX_CP1257	55
    {"cp1257",		ENC_8BIT,		1257}, /* Baltic */
#define IDX_CP1258	56
    {"cp1258",		ENC_8BIT,		1258}, /* Vietnamese */

#define IDX_MACROMAN	57
    {"macroman",	ENC_8BIT + ENC_MACROMAN, 0},	/* Mac OS */
#define IDX_DECMCS	58
    {"dec-mcs",		ENC_8BIT,		0},	/* DEC MCS */
#define IDX_HPROMAN8	59
    {"hp-roman8",	ENC_8BIT,		0},	/* HP Roman8 */
#define IDX_COUNT	60
};

/*
 * Aliases for encoding names.
 */
static struct
{   char *name;		int canon;}
enc_alias_table[] =
{
    {"ansi",		IDX_LATIN_1},
    {"iso-8859-1",	IDX_LATIN_1},
    {"latin2",		IDX_ISO_2},
    {"latin3",		IDX_ISO_3},
    {"latin4",		IDX_ISO_4},
    {"cyrillic",	IDX_ISO_5},
    {"arabic",		IDX_ISO_6},
    {"greek",		IDX_ISO_7},
#ifdef WIN3264
    {"hebrew",		IDX_CP1255},
#else
    {"hebrew",		IDX_ISO_8},
#endif
    {"latin5",		IDX_ISO_9},
    {"turkish",		IDX_ISO_9}, /* ? */
    {"latin6",		IDX_ISO_10},
    {"nordic",		IDX_ISO_10}, /* ? */
    {"thai",		IDX_ISO_11}, /* ? */
    {"latin7",		IDX_ISO_13},
    {"latin8",		IDX_ISO_14},
    {"latin9",		IDX_ISO_15},
    {"utf8",		IDX_UTF8},
    {"unicode",		IDX_UCS2},
    {"ucs2",		IDX_UCS2},
    {"ucs2be",		IDX_UCS2},
    {"ucs-2be",		IDX_UCS2},
    {"ucs2le",		IDX_UCS2LE},
    {"utf16",		IDX_UTF16},
    {"utf16be",		IDX_UTF16},
    {"utf-16be",	IDX_UTF16},
    {"utf16le",		IDX_UTF16LE},
    {"ucs4",		IDX_UCS4},
    {"ucs4be",		IDX_UCS4},
    {"ucs-4be",		IDX_UCS4},
    {"ucs4le",		IDX_UCS4LE},
    {"utf32",		IDX_UCS4},
    {"utf-32",		IDX_UCS4},
    {"utf32be",		IDX_UCS4},
    {"utf-32be",	IDX_UCS4},
    {"utf32le",		IDX_UCS4LE},
    {"utf-32le",	IDX_UCS4LE},
    {"932",		IDX_CP932},
    {"949",		IDX_CP949},
    {"936",		IDX_CP936},
    {"gbk",		IDX_CP936},
    {"950",		IDX_CP950},
    {"eucjp",		IDX_EUC_JP},
    {"unix-jis",	IDX_EUC_JP},
    {"ujis",		IDX_EUC_JP},
    {"shift-jis",	IDX_SJIS},
    {"euckr",		IDX_EUC_KR},
    {"5601",		IDX_EUC_KR},	/* Sun: KS C 5601 */
    {"euccn",		IDX_EUC_CN},
    {"gb2312",		IDX_EUC_CN},
    {"euctw",		IDX_EUC_TW},
#if defined(WIN3264) || defined(WIN32UNIX) || defined(MACOS)
    {"japan",		IDX_CP932},
    {"korea",		IDX_CP949},
    {"prc",		IDX_CP936},
    {"chinese",		IDX_CP936},
    {"taiwan",		IDX_CP950},
    {"big5",		IDX_CP950},
#else
    {"japan",		IDX_EUC_JP},
    {"korea",		IDX_EUC_KR},
    {"prc",		IDX_EUC_CN},
    {"chinese",		IDX_EUC_CN},
    {"taiwan",		IDX_EUC_TW},
    {"cp950",		IDX_BIG5},
    {"950",		IDX_BIG5},
#endif
    {"mac",		IDX_MACROMAN},
    {"mac-roman",	IDX_MACROMAN},
    {NULL,		0}
};

#ifndef CP_UTF8
# define CP_UTF8 65001	/* magic number from winnls.h */
#endif

/*
 * Find encoding "name" in the list of canonical encoding names.
 * Returns -1 if not found.
 */
    static int
enc_canon_search(name)
    char_u	*name;
{
    int		i;

    for (i = 0; i < IDX_COUNT; ++i)
	if (STRCMP(name, enc_canon_table[i].name) == 0)
	    return i;
    return -1;
}

#endif

#if defined(FEAT_MBYTE) || defined(PROTO)

/*
 * Find canonical encoding "name" in the list and return its properties.
 * Returns 0 if not found.
 */
    int
enc_canon_props(name)
    char_u	*name;
{
    int		i;

    i = enc_canon_search(name);
    if (i >= 0)
	return enc_canon_table[i].prop;
#ifdef WIN3264
    if (name[0] == 'c' && name[1] == 'p' && VIM_ISDIGIT(name[2]))
    {
	CPINFO	cpinfo;

	/* Get info on this codepage to find out what it is. */
	if (GetCPInfo(atoi(name + 2), &cpinfo) != 0)
	{
	    if (cpinfo.MaxCharSize == 1) /* some single-byte encoding */
		return ENC_8BIT;
	    if (cpinfo.MaxCharSize == 2
		    && (cpinfo.LeadByte[0] != 0 || cpinfo.LeadByte[1] != 0))
		/* must be a DBCS encoding */
		return ENC_DBCS;
	}
	return 0;
    }
#endif
    if (STRNCMP(name, "2byte-", 6) == 0)
	return ENC_DBCS;
    if (STRNCMP(name, "8bit-", 5) == 0 || STRNCMP(name, "iso-8859-", 9) == 0)
	return ENC_8BIT;
    return 0;
}

/*
 * Set up for using multi-byte characters.
 * Called in three cases:
 * - by main() to initialize (p_enc == NULL)
 * - by set_init_1() after 'encoding' was set to its default.
 * - by do_set() when 'encoding' has been set.
 * p_enc must have been passed through enc_canonize() already.
 * Sets the "enc_unicode", "enc_utf8", "enc_dbcs" and "has_mbyte" flags.
 * Fills mb_bytelen_tab[] and returns NULL when there are no problems.
 * When there is something wrong: Returns an error message and doesn't change
 * anything.
 */
    char_u *
mb_init()
{
    int		i;
    int		idx;
    int		n;
    int		enc_dbcs_new = 0;
#if defined(USE_ICONV) && !defined(WIN3264) && !defined(WIN32UNIX) \
	&& !defined(MACOS)
# define LEN_FROM_CONV
    vimconv_T	vimconv;
    char_u	*p;
#endif

    if (p_enc == NULL)
    {
	/* Just starting up: set the whole table to one's. */
	for (i = 0; i < 256; ++i)
	    mb_bytelen_tab[i] = 1;
	input_conv.vc_type = CONV_NONE;
	input_conv.vc_factor = 1;
	output_conv.vc_type = CONV_NONE;
	return NULL;
    }

#ifdef WIN3264
    if (p_enc[0] == 'c' && p_enc[1] == 'p' && VIM_ISDIGIT(p_enc[2]))
    {
	CPINFO	cpinfo;

	/* Get info on this codepage to find out what it is. */
	if (GetCPInfo(atoi(p_enc + 2), &cpinfo) != 0)
	{
	    if (cpinfo.MaxCharSize == 1)
	    {
		/* some single-byte encoding */
		enc_unicode = 0;
		enc_utf8 = FALSE;
	    }
	    else if (cpinfo.MaxCharSize == 2
		    && (cpinfo.LeadByte[0] != 0 || cpinfo.LeadByte[1] != 0))
	    {
		/* must be a DBCS encoding, check below */
		enc_dbcs_new = atoi(p_enc + 2);
	    }
	    else
		goto codepage_invalid;
	}
	else if (GetLastError() == ERROR_INVALID_PARAMETER)
	{
codepage_invalid:
	    return (char_u *)N_("E543: Not a valid codepage");
	}
    }
#endif
    else if (STRNCMP(p_enc, "8bit-", 5) == 0
	    || STRNCMP(p_enc, "iso-8859-", 9) == 0)
    {
	/* Accept any "8bit-" or "iso-8859-" name. */
	enc_unicode = 0;
	enc_utf8 = FALSE;
    }
    else if (STRNCMP(p_enc, "2byte-", 6) == 0)
    {
#ifdef WIN3264
	/* Windows: accept only valid codepage numbers, check below. */
	if (p_enc[6] != 'c' || p_enc[7] != 'p'
				      || (enc_dbcs_new = atoi(p_enc + 8)) == 0)
	    return e_invarg;
#else
	/* Unix: accept any "2byte-" name, assume current locale. */
	enc_dbcs_new = DBCS_2BYTE;
#endif
    }
    else if ((idx = enc_canon_search(p_enc)) >= 0)
    {
	i = enc_canon_table[idx].prop;
	if (i & ENC_UNICODE)
	{
	    /* Unicode */
	    enc_utf8 = TRUE;
	    if (i & (ENC_2BYTE | ENC_2WORD))
		enc_unicode = 2;
	    else if (i & ENC_4BYTE)
		enc_unicode = 4;
	    else
		enc_unicode = 0;
	}
	else if (i & ENC_DBCS)
	{
	    /* 2byte, handle below */
	    enc_dbcs_new = enc_canon_table[idx].codepage;
	}
	else
	{
	    /* Must be 8-bit. */
	    enc_unicode = 0;
	    enc_utf8 = FALSE;
	}
    }
    else    /* Don't know what encoding this is, reject it. */
	return e_invarg;

    if (enc_dbcs_new != 0)
    {
#ifdef WIN3264
	/* Check if the DBCS code page is OK. */
	if (!IsValidCodePage(enc_dbcs_new))
	    goto codepage_invalid;
#endif
	enc_unicode = 0;
	enc_utf8 = FALSE;
    }
    enc_dbcs = enc_dbcs_new;
    has_mbyte = (enc_dbcs != 0 || enc_utf8);

#ifdef WIN3264
    enc_codepage = encname2codepage(p_enc);
    enc_latin9 = (STRCMP(p_enc, "iso-8859-15") == 0);
#endif

    /* Detect an encoding that uses latin1 characters. */
    enc_latin1like = (enc_utf8 || STRCMP(p_enc, "latin1") == 0
					|| STRCMP(p_enc, "iso-8859-15") == 0);

    /*
     * Set the function pointers.
     */
    if (enc_utf8)
    {
	mb_ptr2len = utfc_ptr2len;
	mb_char2len = utf_char2len;
	mb_char2bytes = utf_char2bytes;
	mb_ptr2cells = utf_ptr2cells;
	mb_char2cells = utf_char2cells;
	mb_off2cells = utf_off2cells;
	mb_ptr2char = utf_ptr2char;
	mb_head_off = utf_head_off;
    }
    else if (enc_dbcs != 0)
    {
	mb_ptr2len = dbcs_ptr2len;
	mb_char2len = dbcs_char2len;
	mb_char2bytes = dbcs_char2bytes;
	mb_ptr2cells = dbcs_ptr2cells;
	mb_char2cells = dbcs_char2cells;
	mb_off2cells = dbcs_off2cells;
	mb_ptr2char = dbcs_ptr2char;
	mb_head_off = dbcs_head_off;
    }
    else
    {
	mb_ptr2len = latin_ptr2len;
	mb_char2len = latin_char2len;
	mb_char2bytes = latin_char2bytes;
	mb_ptr2cells = latin_ptr2cells;
	mb_char2cells = latin_char2cells;
	mb_off2cells = latin_off2cells;
	mb_ptr2char = latin_ptr2char;
	mb_head_off = latin_head_off;
    }

    /*
     * Fill the mb_bytelen_tab[] for MB_BYTE2LEN().
     */
#ifdef LEN_FROM_CONV
    /* When 'encoding' is different from the current locale mblen() won't
     * work.  Use conversion to "utf-8" instead. */
    vimconv.vc_type = CONV_NONE;
    if (enc_dbcs)
    {
	p = enc_locale();
	if (p == NULL || STRCMP(p, p_enc) != 0)
	{
	    convert_setup(&vimconv, p_enc, (char_u *)"utf-8");
	    vimconv.vc_fail = TRUE;
	}
	vim_free(p);
    }
#endif

    for (i = 0; i < 256; ++i)
    {
	/* Our own function to reliably check the length of UTF-8 characters,
	 * independent of mblen(). */
	if (enc_utf8)
	    n = utf8len_tab[i];
	else if (enc_dbcs == 0)
	    n = 1;
	else
	{
#if defined(WIN3264) || defined(WIN32UNIX)
	    /* enc_dbcs is set by setting 'fileencoding'.  It becomes a Windows
	     * CodePage identifier, which we can pass directly in to Windows
	     * API */
	    n = IsDBCSLeadByteEx(enc_dbcs, (BYTE)i) ? 2 : 1;
#else
# if defined(MACOS) || defined(__amigaos4__)
	    /*
	     * if mblen() is not available, character which MSB is turned on
	     * are treated as leading byte character. (note : This assumption
	     * is not always true.)
	     */
	    n = (i & 0x80) ? 2 : 1;
# else
	    char buf[MB_MAXBYTES];
# ifdef X_LOCALE
#  ifndef mblen
#   define mblen _Xmblen
#  endif
# endif
	    if (i == NUL)	/* just in case mblen() can't handle "" */
		n = 1;
	    else
	    {
		buf[0] = i;
		buf[1] = 0;
#ifdef LEN_FROM_CONV
		if (vimconv.vc_type != CONV_NONE)
		{
		    /*
		     * string_convert() should fail when converting the first
		     * byte of a double-byte character.
		     */
		    p = string_convert(&vimconv, (char_u *)buf, NULL);
		    if (p != NULL)
		    {
			vim_free(p);
			n = 1;
		    }
		    else
			n = 2;
		}
		else
#endif
		{
		    /*
		     * mblen() should return -1 for invalid (means the leading
		     * multibyte) character.  However there are some platforms
		     * where mblen() returns 0 for invalid character.
		     * Therefore, following condition includes 0.
		     */
		    ignored = mblen(NULL, 0);	/* First reset the state. */
		    if (mblen(buf, (size_t)1) <= 0)
			n = 2;
		    else
			n = 1;
		}
	    }
# endif
#endif
	}

	mb_bytelen_tab[i] = n;
    }

#ifdef LEN_FROM_CONV
    convert_setup(&vimconv, NULL, NULL);
#endif

    /* The cell width depends on the type of multi-byte characters. */
    (void)init_chartab();

    /* When enc_utf8 is set or reset, (de)allocate ScreenLinesUC[] */
    screenalloc(FALSE);

    /* When using Unicode, set default for 'fileencodings'. */
    if (enc_utf8 && !option_was_set((char_u *)"fencs"))
	set_string_option_direct((char_u *)"fencs", -1,
		       (char_u *)"ucs-bom,utf-8,default,latin1", OPT_FREE, 0);

#if defined(HAVE_BIND_TEXTDOMAIN_CODESET) && defined(FEAT_GETTEXT)
    /* GNU gettext 0.10.37 supports this feature: set the codeset used for
     * translated messages independently from the current locale. */
    (void)bind_textdomain_codeset(VIMPACKAGE,
					  enc_utf8 ? "utf-8" : (char *)p_enc);
#endif

#ifdef WIN32
    /* When changing 'encoding' while starting up, then convert the command
     * line arguments from the active codepage to 'encoding'. */
    if (starting != 0)
	fix_arg_enc();
#endif

#ifdef FEAT_AUTOCMD
    /* Fire an autocommand to let people do custom font setup. This must be
     * after Vim has been setup for the new encoding. */
    apply_autocmds(EVENT_ENCODINGCHANGED, NULL, (char_u *)"", FALSE, curbuf);
#endif

#ifdef FEAT_SPELL
    /* Need to reload spell dictionaries */
    spell_reload();
#endif

    return NULL;
}

/*
 * Return the size of the BOM for the current buffer:
 * 0 - no BOM
 * 2 - UCS-2 or UTF-16 BOM
 * 4 - UCS-4 BOM
 * 3 - UTF-8 BOM
 */
    int
bomb_size()
{
    int n = 0;

    if (curbuf->b_p_bomb && !curbuf->b_p_bin)
    {
	if (*curbuf->b_p_fenc == NUL)
	{
	    if (enc_utf8)
	    {
		if (enc_unicode != 0)
		    n = enc_unicode;
		else
		    n = 3;
	    }
	}
	else if (STRCMP(curbuf->b_p_fenc, "utf-8") == 0)
	    n = 3;
	else if (STRNCMP(curbuf->b_p_fenc, "ucs-2", 5) == 0
		|| STRNCMP(curbuf->b_p_fenc, "utf-16", 6) == 0)
	    n = 2;
	else if (STRNCMP(curbuf->b_p_fenc, "ucs-4", 5) == 0)
	    n = 4;
    }
    return n;
}

/*
 * Get class of pointer:
 * 0 for blank or NUL
 * 1 for punctuation
 * 2 for an (ASCII) word character
 * >2 for other word characters
 */
    int
mb_get_class(p)
    char_u	*p;
{
    if (MB_BYTE2LEN(p[0]) == 1)
    {
	if (p[0] == NUL || vim_iswhite(p[0]))
	    return 0;
	if (vim_iswordc(p[0]))
	    return 2;
	return 1;
    }
    if (enc_dbcs != 0 && p[0] != NUL && p[1] != NUL)
	return dbcs_class(p[0], p[1]);
    if (enc_utf8)
	return utf_class(utf_ptr2char(p));
    return 0;
}

/*
 * Get class of a double-byte character.  This always returns 3 or bigger.
 * TODO: Should return 1 for punctuation.
 */
    int
dbcs_class(lead, trail)
    unsigned	lead;
    unsigned	trail;
{
    switch (enc_dbcs)
    {
	/* please add classfy routine for your language in here */

	case DBCS_JPNU:	/* ? */
	case DBCS_JPN:
	    {
		/* JIS code classification */
		unsigned char lb = lead;
		unsigned char tb = trail;

		/* convert process code to JIS */
# if defined(WIN3264) || defined(WIN32UNIX) || defined(MACOS)
		/* process code is SJIS */
		if (lb <= 0x9f)
		    lb = (lb - 0x81) * 2 + 0x21;
		else
		    lb = (lb - 0xc1) * 2 + 0x21;
		if (tb <= 0x7e)
		    tb -= 0x1f;
		else if (tb <= 0x9e)
		    tb -= 0x20;
		else
		{
		    tb -= 0x7e;
		    lb += 1;
		}
# else
		/*
		 * XXX: Code page identification can not use with all
		 *	    system! So, some other encoding information
		 *	    will be needed.
		 *	    In japanese: SJIS,EUC,UNICODE,(JIS)
		 *	    Note that JIS-code system don't use as
		 *	    process code in most system because it uses
		 *	    escape sequences(JIS is context depend encoding).
		 */
		/* assume process code is JAPANESE-EUC */
		lb &= 0x7f;
		tb &= 0x7f;
# endif
		/* exceptions */
		switch (lb << 8 | tb)
		{
		    case 0x2121: /* ZENKAKU space */
			return 0;
		    case 0x2122: /* KU-TEN (Japanese comma) */
		    case 0x2123: /* TOU-TEN (Japanese period) */
		    case 0x2124: /* ZENKAKU comma */
		    case 0x2125: /* ZENKAKU period */
			return 1;
		    case 0x213c: /* prolongedsound handled as KATAKANA */
			return 13;
		}
		/* sieved by KU code */
		switch (lb)
		{
		    case 0x21:
		    case 0x22:
			/* special symbols */
			return 10;
		    case 0x23:
			/* alpha-numeric */
			return 11;
		    case 0x24:
			/* hiragana */
			return 12;
		    case 0x25:
			/* katakana */
			return 13;
		    case 0x26:
			/* greek */
			return 14;
		    case 0x27:
			/* russian */
			return 15;
		    case 0x28:
			/* lines */
			return 16;
		    default:
			/* kanji */
			return 17;
		}
	    }

	case DBCS_KORU:	/* ? */
	case DBCS_KOR:
	    {
		/* KS code classification */
		unsigned char c1 = lead;
		unsigned char c2 = trail;

		/*
		 * 20 : Hangul
		 * 21 : Hanja
		 * 22 : Symbols
		 * 23 : Alpha-numeric/Roman Letter (Full width)
		 * 24 : Hangul Letter(Alphabet)
		 * 25 : Roman Numeral/Greek Letter
		 * 26 : Box Drawings
		 * 27 : Unit Symbols
		 * 28 : Circled/Parenthesized Letter
		 * 29 : Hirigana/Katakana
		 * 30 : Cyrillic Letter
		 */

		if (c1 >= 0xB0 && c1 <= 0xC8)
		    /* Hangul */
		    return 20;
#if defined(WIN3264) || defined(WIN32UNIX)
		else if (c1 <= 0xA0 || c2 <= 0xA0)
		    /* Extended Hangul Region : MS UHC(Unified Hangul Code) */
		    /* c1: 0x81-0xA0 with c2: 0x41-0x5A, 0x61-0x7A, 0x81-0xFE
		     * c1: 0xA1-0xC6 with c2: 0x41-0x5A, 0x61-0x7A, 0x81-0xA0
		     */
		    return 20;
#endif

		else if (c1 >= 0xCA && c1 <= 0xFD)
		    /* Hanja */
		    return 21;
		else switch (c1)
		{
		    case 0xA1:
		    case 0xA2:
			/* Symbols */
			return 22;
		    case 0xA3:
			/* Alpha-numeric */
			return 23;
		    case 0xA4:
			/* Hangul Letter(Alphabet) */
			return 24;
		    case 0xA5:
			/* Roman Numeral/Greek Letter */
			return 25;
		    case 0xA6:
			/* Box Drawings */
			return 26;
		    case 0xA7:
			/* Unit Symbols */
			return 27;
		    case 0xA8:
		    case 0xA9:
			if (c2 <= 0xAF)
			    return 25;  /* Roman Letter */
			else if (c2 >= 0xF6)
			    return 22;  /* Symbols */
			else
			    /* Circled/Parenthesized Letter */
			    return 28;
		    case 0xAA:
		    case 0xAB:
			/* Hirigana/Katakana */
			return 29;
		    case 0xAC:
			/* Cyrillic Letter */
			return 30;
		}
	    }
	default:
	    break;
    }
    return 3;
}

/*
 * mb_char2len() function pointer.
 * Return length in bytes of character "c".
 * Returns 1 for a single-byte character.
 */
/* ARGSUSED */
    int
latin_char2len(c)
    int		c;
{
    return 1;
}

    static int
dbcs_char2len(c)
    int		c;
{
    if (c >= 0x100)
	return 2;
    return 1;
}

/*
 * mb_char2bytes() function pointer.
 * Convert a character to its bytes.
 * Returns the length in bytes.
 */
    int
latin_char2bytes(c, buf)
    int		c;
    char_u	*buf;
{
    buf[0] = c;
    return 1;
}

    static int
dbcs_char2bytes(c, buf)
    int		c;
    char_u	*buf;
{
    if (c >= 0x100)
    {
	buf[0] = (unsigned)c >> 8;
	buf[1] = c;
	/* Never use a NUL byte, it causes lots of trouble.  It's an invalid
	 * character anyway. */
	if (buf[1] == NUL)
	    buf[1] = '\n';
	return 2;
    }
    buf[0] = c;
    return 1;
}

/*
 * mb_ptr2len() function pointer.
 * Get byte length of character at "*p" but stop at a NUL.
 * For UTF-8 this includes following composing characters.
 * Returns 0 when *p is NUL.
 *
 */
    int
latin_ptr2len(p)
    char_u	*p;
{
    return MB_BYTE2LEN(*p);
}

    static int
dbcs_ptr2len(p)
    char_u	*p;
{
    int		len;

    /* Check if second byte is not missing. */
    len = MB_BYTE2LEN(*p);
    if (len == 2 && p[1] == NUL)
	len = 1;
    return len;
}

struct interval
{
    unsigned short first;
    unsigned short last;
};
static int intable __ARGS((struct interval *table, size_t size, int c));

/*
 * Return TRUE if "c" is in "table[size / sizeof(struct interval)]".
 */
    static int
intable(table, size, c)
    struct interval	*table;
    size_t		size;
    int			c;
{
    int mid, bot, top;

    /* first quick check for Latin1 etc. characters */
    if (c < table[0].first)
	return FALSE;

    /* binary search in table */
    bot = 0;
    top = (int)(size / sizeof(struct interval) - 1);
    while (top >= bot)
    {
	mid = (bot + top) / 2;
	if (table[mid].last < c)
	    bot = mid + 1;
	else if (table[mid].first > c)
	    top = mid - 1;
	else
	    return TRUE;
    }
    return FALSE;
}

/*
 * For UTF-8 character "c" return 2 for a double-width character, 1 for others.
 * Returns 4 or 6 for an unprintable character.
 * Is only correct for characters >= 0x80.
 * When p_ambw is "double", return 2 for a character with East Asian Width
 * class 'A'(mbiguous).
 */
    int
utf_char2cells(c)
    int		c;
{
    /* sorted list of non-overlapping intervals of East Asian Ambiguous
     * characters, generated with:
     * "uniset +WIDTH-A -cat=Me -cat=Mn -cat=Cf c" */
    static struct interval ambiguous[] = {
	{0x00A1, 0x00A1}, {0x00A4, 0x00A4}, {0x00A7, 0x00A8},
	{0x00AA, 0x00AA}, {0x00AE, 0x00AE}, {0x00B0, 0x00B4},
	{0x00B6, 0x00BA}, {0x00BC, 0x00BF}, {0x00C6, 0x00C6},
	{0x00D0, 0x00D0}, {0x00D7, 0x00D8}, {0x00DE, 0x00E1},
	{0x00E6, 0x00E6}, {0x00E8, 0x00EA}, {0x00EC, 0x00ED},
	{0x00F0, 0x00F0}, {0x00F2, 0x00F3}, {0x00F7, 0x00FA},
	{0x00FC, 0x00FC}, {0x00FE, 0x00FE}, {0x0101, 0x0101},
	{0x0111, 0x0111}, {0x0113, 0x0113}, {0x011B, 0x011B},
	{0x0126, 0x0127}, {0x012B, 0x012B}, {0x0131, 0x0133},
	{0x0138, 0x0138}, {0x013F, 0x0142}, {0x0144, 0x0144},
	{0x0148, 0x014B}, {0x014D, 0x014D}, {0x0152, 0x0153},
	{0x0166, 0x0167}, {0x016B, 0x016B}, {0x01CE, 0x01CE},
	{0x01D0, 0x01D0}, {0x01D2, 0x01D2}, {0x01D4, 0x01D4},
	{0x01D6, 0x01D6}, {0x01D8, 0x01D8}, {0x01DA, 0x01DA},
	{0x01DC, 0x01DC}, {0x0251, 0x0251}, {0x0261, 0x0261},
	{0x02C4, 0x02C4}, {0x02C7, 0x02C7}, {0x02C9, 0x02CB},
	{0x02CD, 0x02CD}, {0x02D0, 0x02D0}, {0x02D8, 0x02DB},
	{0x02DD, 0x02DD}, {0x02DF, 0x02DF}, {0x0391, 0x03A1},
	{0x03A3, 0x03A9}, {0x03B1, 0x03C1}, {0x03C3, 0x03C9},
	{0x0401, 0x0401}, {0x0410, 0x044F}, {0x0451, 0x0451},
	{0x2010, 0x2010}, {0x2013, 0x2016}, {0x2018, 0x2019},
	{0x201C, 0x201D}, {0x2020, 0x2022}, {0x2024, 0x2027},
	{0x2030, 0x2030}, {0x2032, 0x2033}, {0x2035, 0x2035},
	{0x203B, 0x203B}, {0x203E, 0x203E}, {0x2074, 0x2074},
	{0x207F, 0x207F}, {0x2081, 0x2084}, {0x20AC, 0x20AC},
	{0x2103, 0x2103}, {0x2105, 0x2105}, {0x2109, 0x2109},
	{0x2113, 0x2113}, {0x2116, 0x2116}, {0x2121, 0x2122},
	{0x2126, 0x2126}, {0x212B, 0x212B}, {0x2153, 0x2154},
	{0x215B, 0x215E}, {0x2160, 0x216B}, {0x2170, 0x2179},
	{0x2190, 0x2199}, {0x21B8, 0x21B9}, {0x21D2, 0x21D2},
	{0x21D4, 0x21D4}, {0x21E7, 0x21E7}, {0x2200, 0x2200},
	{0x2202, 0x2203}, {0x2207, 0x2208}, {0x220B, 0x220B},
	{0x220F, 0x220F}, {0x2211, 0x2211}, {0x2215, 0x2215},
	{0x221A, 0x221A}, {0x221D, 0x2220}, {0x2223, 0x2223},
	{0x2225, 0x2225}, {0x2227, 0x222C}, {0x222E, 0x222E},
	{0x2234, 0x2237}, {0x223C, 0x223D}, {0x2248, 0x2248},
	{0x224C, 0x224C}, {0x2252, 0x2252}, {0x2260, 0x2261},
	{0x2264, 0x2267}, {0x226A, 0x226B}, {0x226E, 0x226F},
	{0x2282, 0x2283}, {0x2286, 0x2287}, {0x2295, 0x2295},
	{0x2299, 0x2299}, {0x22A5, 0x22A5}, {0x22BF, 0x22BF},
	{0x2312, 0x2312}, {0x2460, 0x24E9}, {0x24EB, 0x254B},
	{0x2550, 0x2573}, {0x2580, 0x258F}, {0x2592, 0x2595},
	{0x25A0, 0x25A1}, {0x25A3, 0x25A9}, {0x25B2, 0x25B3},
	{0x25B6, 0x25B7}, {0x25BC, 0x25BD}, {0x25C0, 0x25C1},
	{0x25C6, 0x25C8}, {0x25CB, 0x25CB}, {0x25CE, 0x25D1},
	{0x25E2, 0x25E5}, {0x25EF, 0x25EF}, {0x2605, 0x2606},
	{0x2609, 0x2609}, {0x260E, 0x260F}, {0x2614, 0x2615},
	{0x261C, 0x261C}, {0x261E, 0x261E}, {0x2640, 0x2640},
	{0x2642, 0x2642}, {0x2660, 0x2661}, {0x2663, 0x2665},
	{0x2667, 0x266A}, {0x266C, 0x266D}, {0x266F, 0x266F},
	{0x273D, 0x273D}, {0x2776, 0x277F}, {0xE000, 0xF8FF},
	{0xFFFD, 0xFFFD}, /* {0xF0000, 0xFFFFD}, {0x100000, 0x10FFFD} */
    };

    if (c >= 0x100)
    {
#ifdef USE_WCHAR_FUNCTIONS
	/*
	 * Assume the library function wcwidth() works better than our own
	 * stuff.  It should return 1 for ambiguous width chars!
	 */
	int	n = wcwidth(c);

	if (n < 0)
	    return 6;		/* unprintable, displays <xxxx> */
	if (n > 1)
	    return n;
#else
	if (!utf_printable(c))
	    return 6;		/* unprintable, displays <xxxx> */
	if (c >= 0x1100
	    && (c <= 0x115f			/* Hangul Jamo */
		|| c == 0x2329
		|| c == 0x232a
		|| (c >= 0x2e80 && c <= 0xa4cf
		    && c != 0x303f)		/* CJK ... Yi */
		|| (c >= 0xac00 && c <= 0xd7a3)	/* Hangul Syllables */
		|| (c >= 0xf900 && c <= 0xfaff)	/* CJK Compatibility
						   Ideographs */
		|| (c >= 0xfe30 && c <= 0xfe6f)	/* CJK Compatibility Forms */
		|| (c >= 0xff00 && c <= 0xff60)	/* Fullwidth Forms */
		|| (c >= 0xffe0 && c <= 0xffe6)
		|| (c >= 0x20000 && c <= 0x2fffd)
		|| (c >= 0x30000 && c <= 0x3fffd)))
	    return 2;
#endif
    }

    /* Characters below 0x100 are influenced by 'isprint' option */
    else if (c >= 0x80 && !vim_isprintc(c))
	return 4;		/* unprintable, displays <xx> */

    if (c >= 0x80 && *p_ambw == 'd' && intable(ambiguous, sizeof(ambiguous), c))
	return 2;

    return 1;
}

/*
 * mb_ptr2cells() function pointer.
 * Return the number of display cells character at "*p" occupies.
 * This doesn't take care of unprintable characters, use ptr2cells() for that.
 */
/*ARGSUSED*/
    int
latin_ptr2cells(p)
    char_u	*p;
{
    return 1;
}

    int
utf_ptr2cells(p)
    char_u	*p;
{
    int		c;

    /* Need to convert to a wide character. */
    if (*p >= 0x80)
    {
	c = utf_ptr2char(p);
	/* An illegal byte is displayed as <xx>. */
	if (utf_ptr2len(p) == 1 || c == NUL)
	    return 4;
	/* If the char is ASCII it must be an overlong sequence. */
	if (c < 0x80)
	    return char2cells(c);
	return utf_char2cells(c);
    }
    return 1;
}

    int
dbcs_ptr2cells(p)
    char_u	*p;
{
    /* Number of cells is equal to number of bytes, except for euc-jp when
     * the first byte is 0x8e. */
    if (enc_dbcs == DBCS_JPNU && *p == 0x8e)
	return 1;
    return MB_BYTE2LEN(*p);
}

/*
 * mb_char2cells() function pointer.
 * Return the number of display cells character "c" occupies.
 * Only takes care of multi-byte chars, not "^C" and such.
 */
/*ARGSUSED*/
    int
latin_char2cells(c)
    int		c;
{
    return 1;
}

    static int
dbcs_char2cells(c)
    int		c;
{
    /* Number of cells is equal to number of bytes, except for euc-jp when
     * the first byte is 0x8e. */
    if (enc_dbcs == DBCS_JPNU && ((unsigned)c >> 8) == 0x8e)
	return 1;
    /* use the first byte */
    return MB_BYTE2LEN((unsigned)c >> 8);
}

/*
 * mb_off2cells() function pointer.
 * Return number of display cells for char at ScreenLines[off].
 * We make sure that the offset used is less than "max_off".
 */
/*ARGSUSED*/
    int
latin_off2cells(off, max_off)
    unsigned	off;
    unsigned	max_off;
{
    return 1;
}

    int
dbcs_off2cells(off, max_off)
    unsigned	off;
    unsigned	max_off;
{
    /* never check beyond end of the line */
    if (off >= max_off)
	return 1;

    /* Number of cells is equal to number of bytes, except for euc-jp when
     * the first byte is 0x8e. */
    if (enc_dbcs == DBCS_JPNU && ScreenLines[off] == 0x8e)
	return 1;
    return MB_BYTE2LEN(ScreenLines[off]);
}

    int
utf_off2cells(off, max_off)
    unsigned	off;
    unsigned	max_off;
{
    return (off + 1 < max_off && ScreenLines[off + 1] == 0) ? 2 : 1;
}

/*
 * mb_ptr2char() function pointer.
 * Convert a byte sequence into a character.
 */
    int
latin_ptr2char(p)
    char_u	*p;
{
    return *p;
}

    static int
dbcs_ptr2char(p)
    char_u	*p;
{
    if (MB_BYTE2LEN(*p) > 1 && p[1] != NUL)
	return (p[0] << 8) + p[1];
    return *p;
}

/*
 * Convert a UTF-8 byte sequence to a wide character.
 * If the sequence is illegal or truncated by a NUL the first byte is
 * returned.
 * Does not include composing characters, of course.
 */
    int
utf_ptr2char(p)
    char_u	*p;
{
    int		len;

    if (p[0] < 0x80)	/* be quick for ASCII */
	return p[0];

    len = utf8len_tab[p[0]];
    if (len > 1 && (p[1] & 0xc0) == 0x80)
    {
	if (len == 2)
	    return ((p[0] & 0x1f) << 6) + (p[1] & 0x3f);
	if ((p[2] & 0xc0) == 0x80)
	{
	    if (len == 3)
		return ((p[0] & 0x0f) << 12) + ((p[1] & 0x3f) << 6)
		    + (p[2] & 0x3f);
	    if ((p[3] & 0xc0) == 0x80)
	    {
		if (len == 4)
		    return ((p[0] & 0x07) << 18) + ((p[1] & 0x3f) << 12)
			+ ((p[2] & 0x3f) << 6) + (p[3] & 0x3f);
		if ((p[4] & 0xc0) == 0x80)
		{
		    if (len == 5)
			return ((p[0] & 0x03) << 24) + ((p[1] & 0x3f) << 18)
			    + ((p[2] & 0x3f) << 12) + ((p[3] & 0x3f) << 6)
			    + (p[4] & 0x3f);
		    if ((p[5] & 0xc0) == 0x80 && len == 6)
			return ((p[0] & 0x01) << 30) + ((p[1] & 0x3f) << 24)
			    + ((p[2] & 0x3f) << 18) + ((p[3] & 0x3f) << 12)
			    + ((p[4] & 0x3f) << 6) + (p[5] & 0x3f);
		}
	    }
	}
    }
    /* Illegal value, just return the first byte */
    return p[0];
}

/*
 * Get character at **pp and advance *pp to the next character.
 * Note: composing characters are skipped!
 */
    int
mb_ptr2char_adv(pp)
    char_u	**pp;
{
    int		c;

    c = (*mb_ptr2char)(*pp);
    *pp += (*mb_ptr2len)(*pp);
    return c;
}

/*
 * Get character at **pp and advance *pp to the next character.
 * Note: composing characters are returned as separate characters.
 */
    int
mb_cptr2char_adv(pp)
    char_u	**pp;
{
    int		c;

    c = (*mb_ptr2char)(*pp);
    if (enc_utf8)
	*pp += utf_ptr2len(*pp);
    else
	*pp += (*mb_ptr2len)(*pp);
    return c;
}

#if defined(FEAT_ARABIC) || defined(PROTO)
/*
 * Check whether we are dealing with Arabic combining characters.
 * Note: these are NOT really composing characters!
 */
    int
arabic_combine(one, two)
    int		one;	    /* first character */
    int		two;	    /* character just after "one" */
{
    if (one == a_LAM)
	return arabic_maycombine(two);
    return FALSE;
}

/*
 * Check whether we are dealing with a character that could be regarded as an
 * Arabic combining character, need to check the character before this.
 */
    int
arabic_maycombine(two)
    int		two;
{
    if (p_arshape && !p_tbidi)
	return (two == a_ALEF_MADDA
		    || two == a_ALEF_HAMZA_ABOVE
		    || two == a_ALEF_HAMZA_BELOW
		    || two == a_ALEF);
    return FALSE;
}

/*
 * Check if the character pointed to by "p2" is a composing character when it
 * comes after "p1".  For Arabic sometimes "ab" is replaced with "c", which
 * behaves like a composing character.
 */
    int
utf_composinglike(p1, p2)
    char_u	*p1;
    char_u	*p2;
{
    int		c2;

    c2 = utf_ptr2char(p2);
    if (utf_iscomposing(c2))
	return TRUE;
    if (!arabic_maycombine(c2))
	return FALSE;
    return arabic_combine(utf_ptr2char(p1), c2);
}
#endif

/*
 * Convert a UTF-8 byte string to a wide character.  Also get up to MAX_MCO
 * composing characters.
 */
    int
utfc_ptr2char(p, pcc)
    char_u	*p;
    int		*pcc;	/* return: composing chars, last one is 0 */
{
    int		len;
    int		c;
    int		cc;
    int		i = 0;

    c = utf_ptr2char(p);
    len = utf_ptr2len(p);

    /* Only accept a composing char when the first char isn't illegal. */
    if ((len > 1 || *p < 0x80)
	    && p[len] >= 0x80
	    && UTF_COMPOSINGLIKE(p, p + len))
    {
	cc = utf_ptr2char(p + len);
	for (;;)
	{
	    pcc[i++] = cc;
	    if (i == MAX_MCO)
		break;
	    len += utf_ptr2len(p + len);
	    if (p[len] < 0x80 || !utf_iscomposing(cc = utf_ptr2char(p + len)))
		break;
	}
    }

    if (i < MAX_MCO)	/* last composing char must be 0 */
	pcc[i] = 0;

    return c;
}

/*
 * Convert a UTF-8 byte string to a wide character.  Also get up to MAX_MCO
 * composing characters.  Use no more than p[maxlen].
 */
    int
utfc_ptr2char_len(p, pcc, maxlen)
    char_u	*p;
    int		*pcc;	/* return: composing chars, last one is 0 */
    int		maxlen;
{
    int		len;
    int		c;
    int		cc;
    int		i = 0;

    c = utf_ptr2char(p);
    len = utf_ptr2len_len(p, maxlen);
    /* Only accept a composing char when the first char isn't illegal. */
    if ((len > 1 || *p < 0x80)
	    && len < maxlen
	    && p[len] >= 0x80
	    && UTF_COMPOSINGLIKE(p, p + len))
    {
	cc = utf_ptr2char(p + len);
	for (;;)
	{
	    pcc[i++] = cc;
	    if (i == MAX_MCO)
		break;
	    len += utf_ptr2len_len(p + len, maxlen - len);
	    if (len >= maxlen
		    || p[len] < 0x80
		    || !utf_iscomposing(cc = utf_ptr2char(p + len)))
		break;
	}
    }

    if (i < MAX_MCO)	/* last composing char must be 0 */
	pcc[i] = 0;

    return c;
}

/*
 * Convert the character at screen position "off" to a sequence of bytes.
 * Includes the composing characters.
 * "buf" must at least have the length MB_MAXBYTES.
 * Returns the produced number of bytes.
 */
    int
utfc_char2bytes(off, buf)
    int		off;
    char_u	*buf;
{
    int		len;
    int		i;

    len = utf_char2bytes(ScreenLinesUC[off], buf);
    for (i = 0; i < Screen_mco; ++i)
    {
	if (ScreenLinesC[i][off] == 0)
	    break;
	len += utf_char2bytes(ScreenLinesC[i][off], buf + len);
    }
    return len;
}

/*
 * Get the length of a UTF-8 byte sequence, not including any following
 * composing characters.
 * Returns 0 for "".
 * Returns 1 for an illegal byte sequence.
 */
    int
utf_ptr2len(p)
    char_u	*p;
{
    int		len;
    int		i;

    if (*p == NUL)
	return 0;
    len = utf8len_tab[*p];
    for (i = 1; i < len; ++i)
	if ((p[i] & 0xc0) != 0x80)
	    return 1;
    return len;
}

/*
 * Return length of UTF-8 character, obtained from the first byte.
 * "b" must be between 0 and 255!
 */
    int
utf_byte2len(b)
    int		b;
{
    return utf8len_tab[b];
}

/*
 * Get the length of UTF-8 byte sequence "p[size]".  Does not include any
 * following composing characters.
 * Returns 1 for "".
 * Returns 1 for an illegal byte sequence (also in incomplete byte seq.).
 * Returns number > "size" for an incomplete byte sequence.
 */
    int
utf_ptr2len_len(p, size)
    char_u	*p;
    int		size;
{
    int		len;
    int		i;
    int		m;

    if (*p == NUL)
	return 1;
    m = len = utf8len_tab[*p];
    if (len > size)
	m = size;	/* incomplete byte sequence. */
    for (i = 1; i < m; ++i)
	if ((p[i] & 0xc0) != 0x80)
	    return 1;
    return len;
}

/*
 * Return the number of bytes the UTF-8 encoding of the character at "p" takes.
 * This includes following composing characters.
 */
    int
utfc_ptr2len(p)
    char_u	*p;
{
    int		len;
    int		b0 = *p;
#ifdef FEAT_ARABIC
    int		prevlen;
#endif

    if (b0 == NUL)
	return 0;
    if (b0 < 0x80 && p[1] < 0x80)	/* be quick for ASCII */
	return 1;

    /* Skip over first UTF-8 char, stopping at a NUL byte. */
    len = utf_ptr2len(p);

    /* Check for illegal byte. */
    if (len == 1 && b0 >= 0x80)
	return 1;

    /*
     * Check for composing characters.  We can handle only the first two, but
     * skip all of them (otherwise the cursor would get stuck).
     */
#ifdef FEAT_ARABIC
    prevlen = 0;
#endif
    for (;;)
    {
	if (p[len] < 0x80 || !UTF_COMPOSINGLIKE(p + prevlen, p + len))
	    return len;

	/* Skip over composing char */
#ifdef FEAT_ARABIC
	prevlen = len;
#endif
	len += utf_ptr2len(p + len);
    }
}

/*
 * Return the number of bytes the UTF-8 encoding of the character at "p[size]"
 * takes.  This includes following composing characters.
 * Returns 1 for an illegal char or an incomplete byte sequence.
 */
    int
utfc_ptr2len_len(p, size)
    char_u	*p;
    int		size;
{
    int		len;
#ifdef FEAT_ARABIC
    int		prevlen;
#endif

    if (*p == NUL)
	return 0;
    if (p[0] < 0x80 && (size == 1 || p[1] < 0x80)) /* be quick for ASCII */
	return 1;

    /* Skip over first UTF-8 char, stopping at a NUL byte. */
    len = utf_ptr2len_len(p, size);

    /* Check for illegal byte and incomplete byte sequence. */
    if ((len == 1 && p[0] >= 0x80) || len > size)
	return 1;

    /*
     * Check for composing characters.  We can handle only the first two, but
     * skip all of them (otherwise the cursor would get stuck).
     */
#ifdef FEAT_ARABIC
    prevlen = 0;
#endif
    while (len < size)
    {
	int	len_next_char;

	if (p[len] < 0x80)
	    break;

	/*
	 * Next character length should not go beyond size to ensure that
	 * UTF_COMPOSINGLIKE(...) does not read beyond size.
	 */
	len_next_char = utf_ptr2len_len(p + len, size - len);
	if (len_next_char > size - len)
	    break;

	if (!UTF_COMPOSINGLIKE(p + prevlen, p + len))
	    break;

	/* Skip over composing char */
#ifdef FEAT_ARABIC
	prevlen = len;
#endif
	len += len_next_char;
    }
    return len;
}

/*
 * Return the number of bytes the UTF-8 encoding of character "c" takes.
 * This does not include composing characters.
 */
    int
utf_char2len(c)
    int		c;
{
    if (c < 0x80)
	return 1;
    if (c < 0x800)
	return 2;
    if (c < 0x10000)
	return 3;
    if (c < 0x200000)
	return 4;
    if (c < 0x4000000)
	return 5;
    return 6;
}

/*
 * Convert Unicode character "c" to UTF-8 string in "buf[]".
 * Returns the number of bytes.
 * This does not include composing characters.
 */
    int
utf_char2bytes(c, buf)
    int		c;
    char_u	*buf;
{
    if (c < 0x80)		/* 7 bits */
    {
	buf[0] = c;
	return 1;
    }
    if (c < 0x800)		/* 11 bits */
    {
	buf[0] = 0xc0 + ((unsigned)c >> 6);
	buf[1] = 0x80 + (c & 0x3f);
	return 2;
    }
    if (c < 0x10000)		/* 16 bits */
    {
	buf[0] = 0xe0 + ((unsigned)c >> 12);
	buf[1] = 0x80 + (((unsigned)c >> 6) & 0x3f);
	buf[2] = 0x80 + (c & 0x3f);
	return 3;
    }
    if (c < 0x200000)		/* 21 bits */
    {
	buf[0] = 0xf0 + ((unsigned)c >> 18);
	buf[1] = 0x80 + (((unsigned)c >> 12) & 0x3f);
	buf[2] = 0x80 + (((unsigned)c >> 6) & 0x3f);
	buf[3] = 0x80 + (c & 0x3f);
	return 4;
    }
    if (c < 0x4000000)		/* 26 bits */
    {
	buf[0] = 0xf8 + ((unsigned)c >> 24);
	buf[1] = 0x80 + (((unsigned)c >> 18) & 0x3f);
	buf[2] = 0x80 + (((unsigned)c >> 12) & 0x3f);
	buf[3] = 0x80 + (((unsigned)c >> 6) & 0x3f);
	buf[4] = 0x80 + (c & 0x3f);
	return 5;
    }
				/* 31 bits */
    buf[0] = 0xfc + ((unsigned)c >> 30);
    buf[1] = 0x80 + (((unsigned)c >> 24) & 0x3f);
    buf[2] = 0x80 + (((unsigned)c >> 18) & 0x3f);
    buf[3] = 0x80 + (((unsigned)c >> 12) & 0x3f);
    buf[4] = 0x80 + (((unsigned)c >> 6) & 0x3f);
    buf[5] = 0x80 + (c & 0x3f);
    return 6;
}

/*
 * Return TRUE if "c" is a composing UTF-8 character.  This means it will be
 * drawn on top of the preceding character.
 * Based on code from Markus Kuhn.
 */
    int
utf_iscomposing(c)
    int		c;
{
    /* sorted list of non-overlapping intervals */
    static struct interval combining[] =
    {
	{0x0300, 0x034f}, {0x0360, 0x036f}, {0x0483, 0x0486}, {0x0488, 0x0489},
	{0x0591, 0x05a1}, {0x05a3, 0x05b9}, {0x05bb, 0x05bd}, {0x05bf, 0x05bf},
	{0x05c1, 0x05c2}, {0x05c4, 0x05c4}, {0x0610, 0x0615}, {0x064b, 0x0658},
	{0x0670, 0x0670}, {0x06d6, 0x06dc}, {0x06de, 0x06e4}, {0x06e7, 0x06e8},
	{0x06ea, 0x06ed}, {0x0711, 0x0711}, {0x0730, 0x074a}, {0x07a6, 0x07b0},
	{0x0901, 0x0903}, {0x093c, 0x093c}, {0x093e, 0x094d}, {0x0951, 0x0954},
	{0x0962, 0x0963}, {0x0981, 0x0983}, {0x09bc, 0x09bc}, {0x09be, 0x09c4},
	{0x09c7, 0x09c8}, {0x09cb, 0x09cd}, {0x09d7, 0x09d7}, {0x09e2, 0x09e3},
	{0x0a01, 0x0a03}, {0x0a3c, 0x0a3c}, {0x0a3e, 0x0a42}, {0x0a47, 0x0a48},
	{0x0a4b, 0x0a4d}, {0x0a70, 0x0a71}, {0x0a81, 0x0a83}, {0x0abc, 0x0abc},
	{0x0abe, 0x0ac5}, {0x0ac7, 0x0ac9}, {0x0acb, 0x0acd}, {0x0ae2, 0x0ae3},
	{0x0b01, 0x0b03}, {0x0b3c, 0x0b3c}, {0x0b3e, 0x0b43}, {0x0b47, 0x0b48},
	{0x0b4b, 0x0b4d}, {0x0b56, 0x0b57}, {0x0b82, 0x0b82}, {0x0bbe, 0x0bc2},
	{0x0bc6, 0x0bc8}, {0x0bca, 0x0bcd}, {0x0bd7, 0x0bd7}, {0x0c01, 0x0c03},
	{0x0c3e, 0x0c44}, {0x0c46, 0x0c48}, {0x0c4a, 0x0c4d}, {0x0c55, 0x0c56},
	{0x0c82, 0x0c83}, {0x0cbc, 0x0cbc}, {0x0cbe, 0x0cc4}, {0x0cc6, 0x0cc8},
	{0x0cca, 0x0ccd}, {0x0cd5, 0x0cd6}, {0x0d02, 0x0d03}, {0x0d3e, 0x0d43},
	{0x0d46, 0x0d48}, {0x0d4a, 0x0d4d}, {0x0d57, 0x0d57}, {0x0d82, 0x0d83},
	{0x0dca, 0x0dca}, {0x0dcf, 0x0dd4}, {0x0dd6, 0x0dd6}, {0x0dd8, 0x0ddf},
	{0x0df2, 0x0df3}, {0x0e31, 0x0e31}, {0x0e34, 0x0e3a}, {0x0e47, 0x0e4e},
	{0x0eb1, 0x0eb1}, {0x0eb4, 0x0eb9}, {0x0ebb, 0x0ebc}, {0x0ec8, 0x0ecd},
	{0x0f18, 0x0f19}, {0x0f35, 0x0f35}, {0x0f37, 0x0f37}, {0x0f39, 0x0f39},
	{0x0f3e, 0x0f3f}, {0x0f71, 0x0f84}, {0x0f86, 0x0f87}, {0x0f90, 0x0f97},
	{0x0f99, 0x0fbc}, {0x0fc6, 0x0fc6}, {0x102c, 0x1032}, {0x1036, 0x1039},
	{0x1056, 0x1059}, {0x1712, 0x1714}, {0x1732, 0x1734}, {0x1752, 0x1753},
	{0x1772, 0x1773}, {0x17b6, 0x17d3}, {0x17dd, 0x17dd}, {0x180b, 0x180d},
	{0x18a9, 0x18a9}, {0x1920, 0x192b}, {0x1930, 0x193b}, {0x20d0, 0x20ea},
	{0x302a, 0x302f}, {0x3099, 0x309a}, {0xfb1e, 0xfb1e}, {0xfe00, 0xfe0f},
	{0xfe20, 0xfe23},
    };

    return intable(combining, sizeof(combining), c);
}

/*
 * Return TRUE for characters that can be displayed in a normal way.
 * Only for characters of 0x100 and above!
 */
    int
utf_printable(c)
    int		c;
{
#ifdef USE_WCHAR_FUNCTIONS
    /*
     * Assume the iswprint() library function works better than our own stuff.
     */
    return iswprint(c);
#else
    /* Sorted list of non-overlapping intervals.
     * 0xd800-0xdfff is reserved for UTF-16, actually illegal. */
    static struct interval nonprint[] =
    {
	{0x070f, 0x070f}, {0x180b, 0x180e}, {0x200b, 0x200f}, {0x202a, 0x202e},
	{0x206a, 0x206f}, {0xd800, 0xdfff}, {0xfeff, 0xfeff}, {0xfff9, 0xfffb},
	{0xfffe, 0xffff}
    };

    return !intable(nonprint, sizeof(nonprint), c);
#endif
}

/*
 * Get class of a Unicode character.
 * 0: white space
 * 1: punctuation
 * 2 or bigger: some class of word character.
 */
    int
utf_class(c)
    int		c;
{
    /* sorted list of non-overlapping intervals */
    static struct clinterval
    {
	unsigned short first;
	unsigned short last;
	unsigned short class;
    } classes[] =
    {
	{0x037e, 0x037e, 1},		/* Greek question mark */
	{0x0387, 0x0387, 1},		/* Greek ano teleia */
	{0x055a, 0x055f, 1},		/* Armenian punctuation */
	{0x0589, 0x0589, 1},		/* Armenian full stop */
	{0x05be, 0x05be, 1},
	{0x05c0, 0x05c0, 1},
	{0x05c3, 0x05c3, 1},
	{0x05f3, 0x05f4, 1},
	{0x060c, 0x060c, 1},
	{0x061b, 0x061b, 1},
	{0x061f, 0x061f, 1},
	{0x066a, 0x066d, 1},
	{0x06d4, 0x06d4, 1},
	{0x0700, 0x070d, 1},		/* Syriac punctuation */
	{0x0964, 0x0965, 1},
	{0x0970, 0x0970, 1},
	{0x0df4, 0x0df4, 1},
	{0x0e4f, 0x0e4f, 1},
	{0x0e5a, 0x0e5b, 1},
	{0x0f04, 0x0f12, 1},
	{0x0f3a, 0x0f3d, 1},
	{0x0f85, 0x0f85, 1},
	{0x104a, 0x104f, 1},		/* Myanmar punctuation */
	{0x10fb, 0x10fb, 1},		/* Georgian punctuation */
	{0x1361, 0x1368, 1},		/* Ethiopic punctuation */
	{0x166d, 0x166e, 1},		/* Canadian Syl. punctuation */
	{0x1680, 0x1680, 0},
	{0x169b, 0x169c, 1},
	{0x16eb, 0x16ed, 1},
	{0x1735, 0x1736, 1},
	{0x17d4, 0x17dc, 1},		/* Khmer punctuation */
	{0x1800, 0x180a, 1},		/* Mongolian punctuation */
	{0x2000, 0x200b, 0},		/* spaces */
	{0x200c, 0x2027, 1},		/* punctuation and symbols */
	{0x2028, 0x2029, 0},
	{0x202a, 0x202e, 1},		/* punctuation and symbols */
	{0x202f, 0x202f, 0},
	{0x2030, 0x205e, 1},		/* punctuation and symbols */
	{0x205f, 0x205f, 0},
	{0x2060, 0x27ff, 1},		/* punctuation and symbols */
	{0x2070, 0x207f, 0x2070},	/* superscript */
	{0x2080, 0x2094, 0x2080},	/* subscript */
	{0x20a0, 0x27ff, 1},		/* all kinds of symbols */
	{0x2800, 0x28ff, 0x2800},	/* braille */
	{0x2900, 0x2998, 1},		/* arrows, brackets, etc. */
	{0x29d8, 0x29db, 1},
	{0x29fc, 0x29fd, 1},
	{0x3000, 0x3000, 0},		/* ideographic space */
	{0x3001, 0x3020, 1},		/* ideographic punctuation */
	{0x3030, 0x3030, 1},
	{0x303d, 0x303d, 1},
	{0x3040, 0x309f, 0x3040},	/* Hiragana */
	{0x30a0, 0x30ff, 0x30a0},	/* Katakana */
	{0x3300, 0x9fff, 0x4e00},	/* CJK Ideographs */
	{0xac00, 0xd7a3, 0xac00},	/* Hangul Syllables */
	{0xf900, 0xfaff, 0x4e00},	/* CJK Ideographs */
	{0xfd3e, 0xfd3f, 1},
	{0xfe30, 0xfe6b, 1},		/* punctuation forms */
	{0xff00, 0xff0f, 1},		/* half/fullwidth ASCII */
	{0xff1a, 0xff20, 1},		/* half/fullwidth ASCII */
	{0xff3b, 0xff40, 1},		/* half/fullwidth ASCII */
	{0xff5b, 0xff65, 1},		/* half/fullwidth ASCII */
    };
    int bot = 0;
    int top = sizeof(classes) / sizeof(struct clinterval) - 1;
    int mid;

    /* First quick check for Latin1 characters, use 'iskeyword'. */
    if (c < 0x100)
    {
	if (c == ' ' || c == '\t' || c == NUL || c == 0xa0)
	    return 0;	    /* blank */
	if (vim_iswordc(c))
	    return 2;	    /* word character */
	return 1;	    /* punctuation */
    }

    /* binary search in table */
    while (top >= bot)
    {
	mid = (bot + top) / 2;
	if (classes[mid].last < c)
	    bot = mid + 1;
	else if (classes[mid].first > c)
	    top = mid - 1;
	else
	    return (int)classes[mid].class;
    }

    /* most other characters are "word" characters */
    return 2;
}

/*
 * Code for Unicode case-dependent operations.  Based on notes in
 * http://www.unicode.org/Public/UNIDATA/CaseFolding.txt
 * This code uses simple case folding, not full case folding.
 */

/*
 * The following table is built by foldExtract.pl < CaseFolding.txt .
 * It must be in numeric order, because we use binary search on it.
 * An entry such as {0x41,0x5a,1,32} means that UCS-4 characters in the range
 * from 0x41 to 0x5a inclusive, stepping by 1, are folded by adding 32.
 */

typedef struct
{
    int rangeStart;
    int rangeEnd;
    int step;
    int offset;
} convertStruct;

static convertStruct foldCase[] =
{
	{0x41,0x5a,1,32}, {0xc0,0xd6,1,32}, {0xd8,0xde,1,32},
	{0x100,0x12e,2,1}, {0x130,0x130,-1,-199}, {0x132,0x136,2,1},
	{0x139,0x147,2,1}, {0x14a,0x176,2,1}, {0x178,0x178,-1,-121},
	{0x179,0x17d,2,1}, {0x181,0x181,-1,210}, {0x182,0x184,2,1},
	{0x186,0x186,-1,206}, {0x187,0x187,-1,1}, {0x189,0x18a,1,205},
	{0x18b,0x18b,-1,1}, {0x18e,0x18e,-1,79}, {0x18f,0x18f,-1,202},
	{0x190,0x190,-1,203}, {0x191,0x191,-1,1}, {0x193,0x193,-1,205},
	{0x194,0x194,-1,207}, {0x196,0x196,-1,211}, {0x197,0x197,-1,209},
	{0x198,0x198,-1,1}, {0x19c,0x19c,-1,211}, {0x19d,0x19d,-1,213},
	{0x19f,0x19f,-1,214}, {0x1a0,0x1a4,2,1}, {0x1a6,0x1a6,-1,218},
	{0x1a7,0x1a7,-1,1}, {0x1a9,0x1a9,-1,218}, {0x1ac,0x1ac,-1,1},
	{0x1ae,0x1ae,-1,218}, {0x1af,0x1af,-1,1}, {0x1b1,0x1b2,1,217},
	{0x1b3,0x1b5,2,1}, {0x1b7,0x1b7,-1,219}, {0x1b8,0x1bc,4,1},
	{0x1c4,0x1c4,-1,2}, {0x1c5,0x1c5,-1,1}, {0x1c7,0x1c7,-1,2},
	{0x1c8,0x1c8,-1,1}, {0x1ca,0x1ca,-1,2}, {0x1cb,0x1db,2,1},
	{0x1de,0x1ee,2,1}, {0x1f1,0x1f1,-1,2}, {0x1f2,0x1f4,2,1},
	{0x1f6,0x1f6,-1,-97}, {0x1f7,0x1f7,-1,-56}, {0x1f8,0x21e,2,1},
	{0x220,0x220,-1,-130}, {0x222,0x232,2,1}, {0x386,0x386,-1,38},
	{0x388,0x38a,1,37}, {0x38c,0x38c,-1,64}, {0x38e,0x38f,1,63},
	{0x391,0x3a1,1,32}, {0x3a3,0x3ab,1,32}, {0x3d8,0x3ee,2,1},
	{0x3f4,0x3f4,-1,-60}, {0x3f7,0x3f7,-1,1}, {0x3f9,0x3f9,-1,-7},
	{0x3fa,0x3fa,-1,1}, {0x400,0x40f,1,80}, {0x410,0x42f,1,32},
	{0x460,0x480,2,1}, {0x48a,0x4be,2,1}, {0x4c1,0x4cd,2,1},
	{0x4d0,0x4f4,2,1}, {0x4f8,0x500,8,1}, {0x502,0x50e,2,1},
	{0x531,0x556,1,48}, {0x1e00,0x1e94,2,1}, {0x1ea0,0x1ef8,2,1},
	{0x1f08,0x1f0f,1,-8}, {0x1f18,0x1f1d,1,-8}, {0x1f28,0x1f2f,1,-8},
	{0x1f38,0x1f3f,1,-8}, {0x1f48,0x1f4d,1,-8}, {0x1f59,0x1f5f,2,-8},
	{0x1f68,0x1f6f,1,-8}, {0x1f88,0x1f8f,1,-8}, {0x1f98,0x1f9f,1,-8},
	{0x1fa8,0x1faf,1,-8}, {0x1fb8,0x1fb9,1,-8}, {0x1fba,0x1fbb,1,-74},
	{0x1fbc,0x1fbc,-1,-9}, {0x1fc8,0x1fcb,1,-86}, {0x1fcc,0x1fcc,-1,-9},
	{0x1fd8,0x1fd9,1,-8}, {0x1fda,0x1fdb,1,-100}, {0x1fe8,0x1fe9,1,-8},
	{0x1fea,0x1feb,1,-112}, {0x1fec,0x1fec,-1,-7}, {0x1ff8,0x1ff9,1,-128},
	{0x1ffa,0x1ffb,1,-126}, {0x1ffc,0x1ffc,-1,-9}, {0x2126,0x2126,-1,-7517},
	{0x212a,0x212a,-1,-8383}, {0x212b,0x212b,-1,-8262},
	{0x2160,0x216f,1,16}, {0x24b6,0x24cf,1,26}, {0xff21,0xff3a,1,32},
	{0x10400,0x10427,1,40}
};

static int utf_convert(int a, convertStruct table[], int tableSize);

/*
 * Generic conversion function for case operations.
 * Return the converted equivalent of "a", which is a UCS-4 character.  Use
 * the given conversion "table".  Uses binary search on "table".
 */
    static int
utf_convert(a, table, tableSize)
    int			a;
    convertStruct	table[];
    int			tableSize;
{
    int start, mid, end; /* indices into table */

    start = 0;
    end = tableSize / sizeof(convertStruct);
    while (start < end)
    {
	/* need to search further */
	mid = (end + start) /2;
	if (table[mid].rangeEnd < a)
	    start = mid + 1;
	else
	    end = mid;
    }
    if (table[start].rangeStart <= a && a <= table[start].rangeEnd
	    && (a - table[start].rangeStart) % table[start].step == 0)
	return (a + table[start].offset);
    else
	return a;
}

/*
 * Return the folded-case equivalent of "a", which is a UCS-4 character.  Uses
 * simple case folding.
 */
    int
utf_fold(a)
    int		a;
{
    return utf_convert(a, foldCase, sizeof(foldCase));
}

/*
 * The following tables are built by upperLowerExtract.pl < UnicodeData.txt .
 * They must be in numeric order, because we use binary search on them.
 * An entry such as {0x41,0x5a,1,32} means that UCS-4 characters in the range
 * from 0x41 to 0x5a inclusive, stepping by 1, are switched to lower (for
 * example) by adding 32.
 */
static convertStruct toLower[] =
{
	{0x41,0x5a,1,32}, {0xc0,0xd6,1,32}, {0xd8,0xde,1,32},
	{0x100,0x12e,2,1}, {0x130,0x130,-1,-199}, {0x132,0x136,2,1},
	{0x139,0x147,2,1}, {0x14a,0x176,2,1}, {0x178,0x178,-1,-121},
	{0x179,0x17d,2,1}, {0x181,0x181,-1,210}, {0x182,0x184,2,1},
	{0x186,0x186,-1,206}, {0x187,0x187,-1,1}, {0x189,0x18a,1,205},
	{0x18b,0x18b,-1,1}, {0x18e,0x18e,-1,79}, {0x18f,0x18f,-1,202},
	{0x190,0x190,-1,203}, {0x191,0x191,-1,1}, {0x193,0x193,-1,205},
	{0x194,0x194,-1,207}, {0x196,0x196,-1,211}, {0x197,0x197,-1,209},
	{0x198,0x198,-1,1}, {0x19c,0x19c,-1,211}, {0x19d,0x19d,-1,213},
	{0x19f,0x19f,-1,214}, {0x1a0,0x1a4,2,1}, {0x1a6,0x1a6,-1,218},
	{0x1a7,0x1a7,-1,1}, {0x1a9,0x1a9,-1,218}, {0x1ac,0x1ac,-1,1},
	{0x1ae,0x1ae,-1,218}, {0x1af,0x1af,-1,1}, {0x1b1,0x1b2,1,217},
	{0x1b3,0x1b5,2,1}, {0x1b7,0x1b7,-1,219}, {0x1b8,0x1bc,4,1},
	{0x1c4,0x1ca,3,2}, {0x1cd,0x1db,2,1}, {0x1de,0x1ee,2,1},
	{0x1f1,0x1f1,-1,2}, {0x1f4,0x1f4,-1,1}, {0x1f6,0x1f6,-1,-97},
	{0x1f7,0x1f7,-1,-56}, {0x1f8,0x21e,2,1}, {0x220,0x220,-1,-130},
	{0x222,0x232,2,1}, {0x386,0x386,-1,38}, {0x388,0x38a,1,37},
	{0x38c,0x38c,-1,64}, {0x38e,0x38f,1,63}, {0x391,0x3a1,1,32},
	{0x3a3,0x3ab,1,32}, {0x3d8,0x3ee,2,1}, {0x3f4,0x3f4,-1,-60},
	{0x3f7,0x3f7,-1,1}, {0x3f9,0x3f9,-1,-7}, {0x3fa,0x3fa,-1,1},
	{0x400,0x40f,1,80}, {0x410,0x42f,1,32}, {0x460,0x480,2,1},
	{0x48a,0x4be,2,1}, {0x4c1,0x4cd,2,1}, {0x4d0,0x4f4,2,1},
	{0x4f8,0x500,8,1}, {0x502,0x50e,2,1}, {0x531,0x556,1,48},
	{0x1e00,0x1e94,2,1}, {0x1ea0,0x1ef8,2,1}, {0x1f08,0x1f0f,1,-8},
	{0x1f18,0x1f1d,1,-8}, {0x1f28,0x1f2f,1,-8}, {0x1f38,0x1f3f,1,-8},
	{0x1f48,0x1f4d,1,-8}, {0x1f59,0x1f5f,2,-8}, {0x1f68,0x1f6f,1,-8},
	{0x1fb8,0x1fb9,1,-8}, {0x1fba,0x1fbb,1,-74}, {0x1fc8,0x1fcb,1,-86},
	{0x1fd8,0x1fd9,1,-8}, {0x1fda,0x1fdb,1,-100}, {0x1fe8,0x1fe9,1,-8},
	{0x1fea,0x1feb,1,-112}, {0x1fec,0x1fec,-1,-7}, {0x1ff8,0x1ff9,1,-128},
	{0x1ffa,0x1ffb,1,-126}, {0x2126,0x2126,-1,-7517}, {0x212a,0x212a,-1,-8383},
	{0x212b,0x212b,-1,-8262}, {0xff21,0xff3a,1,32}, {0x10400,0x10427,1,40}
};

static convertStruct toUpper[] =
{
	{0x61,0x7a,1,-32}, {0xb5,0xb5,-1,743}, {0xe0,0xf6,1,-32},
	{0xf8,0xfe,1,-32}, {0xff,0xff,-1,121}, {0x101,0x12f,2,-1},
	{0x131,0x131,-1,-232}, {0x133,0x137,2,-1}, {0x13a,0x148,2,-1},
	{0x14b,0x177,2,-1}, {0x17a,0x17e,2,-1}, {0x17f,0x17f,-1,-300},
	{0x183,0x185,2,-1}, {0x188,0x18c,4,-1}, {0x192,0x192,-1,-1},
	{0x195,0x195,-1,97}, {0x199,0x199,-1,-1}, {0x19e,0x19e,-1,130},
	{0x1a1,0x1a5,2,-1}, {0x1a8,0x1ad,5,-1}, {0x1b0,0x1b4,4,-1},
	{0x1b6,0x1b9,3,-1}, {0x1bd,0x1bd,-1,-1}, {0x1bf,0x1bf,-1,56},
	{0x1c5,0x1c6,1,-1}, {0x1c8,0x1c9,1,-1}, {0x1cb,0x1cc,1,-1},
	{0x1ce,0x1dc,2,-1}, {0x1dd,0x1dd,-1,-79}, {0x1df,0x1ef,2,-1},
	{0x1f2,0x1f3,1,-1}, {0x1f5,0x1f9,4,-1}, {0x1fb,0x21f,2,-1},
	{0x223,0x233,2,-1}, {0x253,0x253,-1,-210}, {0x254,0x254,-1,-206},
	{0x256,0x257,1,-205}, {0x259,0x259,-1,-202}, {0x25b,0x25b,-1,-203},
	{0x260,0x260,-1,-205}, {0x263,0x263,-1,-207}, {0x268,0x268,-1,-209},
	{0x269,0x26f,6,-211}, {0x272,0x272,-1,-213}, {0x275,0x275,-1,-214},
	{0x280,0x283,3,-218}, {0x288,0x288,-1,-218}, {0x28a,0x28b,1,-217},
	{0x292,0x292,-1,-219}, {0x3ac,0x3ac,-1,-38}, {0x3ad,0x3af,1,-37},
	{0x3b1,0x3c1,1,-32}, {0x3c2,0x3c2,-1,-31}, {0x3c3,0x3cb,1,-32},
	{0x3cc,0x3cc,-1,-64}, {0x3cd,0x3ce,1,-63}, {0x3d0,0x3d0,-1,-62},
	{0x3d1,0x3d1,-1,-57}, {0x3d5,0x3d5,-1,-47}, {0x3d6,0x3d6,-1,-54},
	{0x3d9,0x3ef,2,-1}, {0x3f0,0x3f0,-1,-86}, {0x3f1,0x3f1,-1,-80},
	{0x3f2,0x3f2,-1,7}, {0x3f5,0x3f5,-1,-96}, {0x3f8,0x3fb,3,-1},
	{0x430,0x44f,1,-32}, {0x450,0x45f,1,-80}, {0x461,0x481,2,-1},
	{0x48b,0x4bf,2,-1}, {0x4c2,0x4ce,2,-1}, {0x4d1,0x4f5,2,-1},
	{0x4f9,0x501,8,-1}, {0x503,0x50f,2,-1}, {0x561,0x586,1,-48},
	{0x1e01,0x1e95,2,-1}, {0x1e9b,0x1e9b,-1,-59}, {0x1ea1,0x1ef9,2,-1},
	{0x1f00,0x1f07,1,8}, {0x1f10,0x1f15,1,8}, {0x1f20,0x1f27,1,8},
	{0x1f30,0x1f37,1,8}, {0x1f40,0x1f45,1,8}, {0x1f51,0x1f57,2,8},
	{0x1f60,0x1f67,1,8}, {0x1f70,0x1f71,1,74}, {0x1f72,0x1f75,1,86},
	{0x1f76,0x1f77,1,100}, {0x1f78,0x1f79,1,128}, {0x1f7a,0x1f7b,1,112},
	{0x1f7c,0x1f7d,1,126}, {0x1f80,0x1f87,1,8}, {0x1f90,0x1f97,1,8},
	{0x1fa0,0x1fa7,1,8}, {0x1fb0,0x1fb1,1,8}, {0x1fb3,0x1fb3,-1,9},
	{0x1fbe,0x1fbe,-1,-7205}, {0x1fc3,0x1fc3,-1,9}, {0x1fd0,0x1fd1,1,8},
	{0x1fe0,0x1fe1,1,8}, {0x1fe5,0x1fe5,-1,7}, {0x1ff3,0x1ff3,-1,9},
	{0xff41,0xff5a,1,-32}, {0x10428,0x1044f,1,-40}
};

/*
 * Return the upper-case equivalent of "a", which is a UCS-4 character.  Use
 * simple case folding.
 */
    int
utf_toupper(a)
    int		a;
{
    /* If 'casemap' contains "keepascii" use ASCII style toupper(). */
    if (a < 128 && (cmp_flags & CMP_KEEPASCII))
	return TOUPPER_ASC(a);

#if defined(HAVE_TOWUPPER) && defined(__STDC_ISO_10646__)
    /* If towupper() is available and handles Unicode, use it. */
    if (!(cmp_flags & CMP_INTERNAL))
	return towupper(a);
#endif

    /* For characters below 128 use locale sensitive toupper(). */
    if (a < 128)
	return TOUPPER_LOC(a);

    /* For any other characters use the above mapping table. */
    return utf_convert(a, toUpper, sizeof(toUpper));
}

    int
utf_islower(a)
    int		a;
{
    return (utf_toupper(a) != a);
}

/*
 * Return the lower-case equivalent of "a", which is a UCS-4 character.  Use
 * simple case folding.
 */
    int
utf_tolower(a)
    int		a;
{
    /* If 'casemap' contains "keepascii" use ASCII style tolower(). */
    if (a < 128 && (cmp_flags & CMP_KEEPASCII))
	return TOLOWER_ASC(a);

#if defined(HAVE_TOWLOWER) && defined(__STDC_ISO_10646__)
    /* If towlower() is available and handles Unicode, use it. */
    if (!(cmp_flags & CMP_INTERNAL))
	return towlower(a);
#endif

    /* For characters below 128 use locale sensitive tolower(). */
    if (a < 128)
	return TOLOWER_LOC(a);

    /* For any other characters use the above mapping table. */
    return utf_convert(a, toLower, sizeof(toLower));
}

    int
utf_isupper(a)
    int		a;
{
    return (utf_tolower(a) != a);
}

/*
 * Version of strnicmp() that handles multi-byte characters.
 * Needed for Big5, Sjift-JIS and UTF-8 encoding.  Other DBCS encodings can
 * probably use strnicmp(), because there are no ASCII characters in the
 * second byte.
 * Returns zero if s1 and s2 are equal (ignoring case), the difference between
 * two characters otherwise.
 */
    int
mb_strnicmp(s1, s2, nn)
    char_u	*s1, *s2;
    size_t	nn;
{
    int		i, j, l;
    int		cdiff;
    int		incomplete = FALSE;
    int		n = (int)nn;

    for (i = 0; i < n; i += l)
    {
	if (s1[i] == NUL && s2[i] == NUL)   /* both strings end */
	    return 0;
	if (enc_utf8)
	{
	    l = utf_byte2len(s1[i]);
	    if (l > n - i)
	    {
		l = n - i;		    /* incomplete character */
		incomplete = TRUE;
	    }
	    /* Check directly first, it's faster. */
	    for (j = 0; j < l; ++j)
	    {
		if (s1[i + j] != s2[i + j])
		    break;
		if (s1[i + j] == 0)
		    /* Both stings have the same bytes but are incomplete or
		     * have illegal bytes, accept them as equal. */
		    l = j;
	    }
	    if (j < l)
	    {
		/* If one of the two characters is incomplete return -1. */
		if (incomplete || i + utf_byte2len(s2[i]) > n)
		    return -1;
		cdiff = utf_fold(utf_ptr2char(s1 + i))
					     - utf_fold(utf_ptr2char(s2 + i));
		if (cdiff != 0)
		    return cdiff;
	    }
	}
	else
	{
	    l = (*mb_ptr2len)(s1 + i);
	    if (l <= 1)
	    {
		/* Single byte: first check normally, then with ignore case. */
		if (s1[i] != s2[i])
		{
		    cdiff = MB_TOLOWER(s1[i]) - MB_TOLOWER(s2[i]);
		    if (cdiff != 0)
			return cdiff;
		}
	    }
	    else
	    {
		/* For non-Unicode multi-byte don't ignore case. */
		if (l > n - i)
		    l = n - i;
		cdiff = STRNCMP(s1 + i, s2 + i, l);
		if (cdiff != 0)
		    return cdiff;
	    }
	}
    }
    return 0;
}

/*
 * "g8": show bytes of the UTF-8 char under the cursor.  Doesn't matter what
 * 'encoding' has been set to.
 */
    void
show_utf8()
{
    int		len;
    int		rlen = 0;
    char_u	*line;
    int		clen;
    int		i;

    /* Get the byte length of the char under the cursor, including composing
     * characters. */
    line = ml_get_cursor();
    len = utfc_ptr2len(line);
    if (len == 0)
    {
	MSG("NUL");
	return;
    }

    clen = 0;
    for (i = 0; i < len; ++i)
    {
	if (clen == 0)
	{
	    /* start of (composing) character, get its length */
	    if (i > 0)
	    {
		STRCPY(IObuff + rlen, "+ ");
		rlen += 2;
	    }
	    clen = utf_ptr2len(line + i);
	}
	sprintf((char *)IObuff + rlen, "%02x ", line[i]);
	--clen;
	rlen += (int)STRLEN(IObuff + rlen);
	if (rlen > IOSIZE - 20)
	    break;
    }

    msg(IObuff);
}

/*
 * mb_head_off() function pointer.
 * Return offset from "p" to the first byte of the character it points into.
 * Returns 0 when already at the first byte of a character.
 */
/*ARGSUSED*/
    int
latin_head_off(base, p)
    char_u	*base;
    char_u	*p;
{
    return 0;
}

    int
dbcs_head_off(base, p)
    char_u	*base;
    char_u	*p;
{
    char_u	*q;

    /* It can't be a trailing byte when not using DBCS, at the start of the
     * string or the previous byte can't start a double-byte. */
    if (p <= base || MB_BYTE2LEN(p[-1]) == 1)
	return 0;

    /* This is slow: need to start at the base and go forward until the
     * byte we are looking for.  Return 1 when we went past it, 0 otherwise. */
    q = base;
    while (q < p)
	q += dbcs_ptr2len(q);
    return (q == p) ? 0 : 1;
}

/*
 * Special version of dbcs_head_off() that works for ScreenLines[], where
 * single-width DBCS_JPNU characters are stored separately.
 */
    int
dbcs_screen_head_off(base, p)
    char_u	*base;
    char_u	*p;
{
    char_u	*q;

    /* It can't be a trailing byte when not using DBCS, at the start of the
     * string or the previous byte can't start a double-byte.
     * For euc-jp an 0x8e byte in the previous cell always means we have a
     * lead byte in the current cell. */
    if (p <= base
	    || (enc_dbcs == DBCS_JPNU && p[-1] == 0x8e)
	    || MB_BYTE2LEN(p[-1]) == 1)
	return 0;

    /* This is slow: need to start at the base and go forward until the
     * byte we are looking for.  Return 1 when we went past it, 0 otherwise.
     * For DBCS_JPNU look out for 0x8e, which means the second byte is not
     * stored as the next byte. */
    q = base;
    while (q < p)
    {
	if (enc_dbcs == DBCS_JPNU && *q == 0x8e)
	    ++q;
	else
	    q += dbcs_ptr2len(q);
    }
    return (q == p) ? 0 : 1;
}

    int
utf_head_off(base, p)
    char_u	*base;
    char_u	*p;
{
    char_u	*q;
    char_u	*s;
    int		c;
#ifdef FEAT_ARABIC
    char_u	*j;
#endif

    if (*p < 0x80)		/* be quick for ASCII */
	return 0;

    /* Skip backwards over trailing bytes: 10xx.xxxx
     * Skip backwards again if on a composing char. */
    for (q = p; ; --q)
    {
	/* Move s to the last byte of this char. */
	for (s = q; (s[1] & 0xc0) == 0x80; ++s)
	    ;
	/* Move q to the first byte of this char. */
	while (q > base && (*q & 0xc0) == 0x80)
	    --q;
	/* Check for illegal sequence. Do allow an illegal byte after where we
	 * started. */
	if (utf8len_tab[*q] != (int)(s - q + 1)
				       && utf8len_tab[*q] != (int)(p - q + 1))
	    return 0;

	if (q <= base)
	    break;

	c = utf_ptr2char(q);
	if (utf_iscomposing(c))
	    continue;

#ifdef FEAT_ARABIC
	if (arabic_maycombine(c))
	{
	    /* Advance to get a sneak-peak at the next char */
	    j = q;
	    --j;
	    /* Move j to the first byte of this char. */
	    while (j > base && (*j & 0xc0) == 0x80)
		--j;
	    if (arabic_combine(utf_ptr2char(j), c))
		continue;
	}
#endif
	break;
    }

    return (int)(p - q);
}

/*
 * Copy a character from "*fp" to "*tp" and advance the pointers.
 */
    void
mb_copy_char(fp, tp)
    char_u	**fp;
    char_u	**tp;
{
    int	    l = (*mb_ptr2len)(*fp);

    mch_memmove(*tp, *fp, (size_t)l);
    *tp += l;
    *fp += l;
}

/*
 * Return the offset from "p" to the first byte of a character.  When "p" is
 * at the start of a character 0 is returned, otherwise the offset to the next
 * character.  Can start anywhere in a stream of bytes.
 */
    int
mb_off_next(base, p)
    char_u	*base;
    char_u	*p;
{
    int		i;
    int		j;

    if (enc_utf8)
    {
	if (*p < 0x80)		/* be quick for ASCII */
	    return 0;

	/* Find the next character that isn't 10xx.xxxx */
	for (i = 0; (p[i] & 0xc0) == 0x80; ++i)
	    ;
	if (i > 0)
	{
	    /* Check for illegal sequence. */
	    for (j = 0; p - j > base; ++j)
		if ((p[-j] & 0xc0) != 0x80)
		    break;
	    if (utf8len_tab[p[-j]] != i + j)
		return 0;
	}
	return i;
    }

    /* Only need to check if we're on a trail byte, it doesn't matter if we
     * want the offset to the next or current character. */
    return (*mb_head_off)(base, p);
}

/*
 * Return the offset from "p" to the last byte of the character it points
 * into.  Can start anywhere in a stream of bytes.
 */
    int
mb_tail_off(base, p)
    char_u	*base;
    char_u	*p;
{
    int		i;
    int		j;

    if (*p == NUL)
	return 0;

    if (enc_utf8)
    {
	/* Find the last character that is 10xx.xxxx */
	for (i = 0; (p[i + 1] & 0xc0) == 0x80; ++i)
	    ;
	/* Check for illegal sequence. */
	for (j = 0; p - j > base; ++j)
	    if ((p[-j] & 0xc0) != 0x80)
		break;
	if (utf8len_tab[p[-j]] != i + j + 1)
	    return 0;
	return i;
    }

    /* It can't be the first byte if a double-byte when not using DBCS, at the
     * end of the string or the byte can't start a double-byte. */
    if (enc_dbcs == 0 || p[1] == NUL || MB_BYTE2LEN(*p) == 1)
	return 0;

    /* Return 1 when on the lead byte, 0 when on the tail byte. */
    return 1 - dbcs_head_off(base, p);
}

/*
 * Find the next illegal byte sequence.
 */
    void
utf_find_illegal()
{
    pos_T	pos = curwin->w_cursor;
    char_u	*p;
    int		len;
    vimconv_T	vimconv;
    char_u	*tofree = NULL;

    vimconv.vc_type = CONV_NONE;
    if (enc_utf8 && (enc_canon_props(curbuf->b_p_fenc) & ENC_8BIT))
    {
	/* 'encoding' is "utf-8" but we are editing a 8-bit encoded file,
	 * possibly a utf-8 file with illegal bytes.  Setup for conversion
	 * from utf-8 to 'fileencoding'. */
	convert_setup(&vimconv, p_enc, curbuf->b_p_fenc);
    }

#ifdef FEAT_VIRTUALEDIT
    curwin->w_cursor.coladd = 0;
#endif
    for (;;)
    {
	p = ml_get_cursor();
	if (vimconv.vc_type != CONV_NONE)
	{
	    vim_free(tofree);
	    tofree = string_convert(&vimconv, p, NULL);
	    if (tofree == NULL)
		break;
	    p = tofree;
	}

	while (*p != NUL)
	{
	    /* Illegal means that there are not enough trail bytes (checked by
	     * utf_ptr2len()) or too many of them (overlong sequence). */
	    len = utf_ptr2len(p);
	    if (*p >= 0x80 && (len == 1
				     || utf_char2len(utf_ptr2char(p)) != len))
	    {
		if (vimconv.vc_type == CONV_NONE)
		    curwin->w_cursor.col += (colnr_T)(p - ml_get_cursor());
		else
		{
		    int	    l;

		    len = (int)(p - tofree);
		    for (p = ml_get_cursor(); *p != NUL && len-- > 0; p += l)
		    {
			l = utf_ptr2len(p);
			curwin->w_cursor.col += l;
		    }
		}
		goto theend;
	    }
	    p += len;
	}
	if (curwin->w_cursor.lnum == curbuf->b_ml.ml_line_count)
	    break;
	++curwin->w_cursor.lnum;
	curwin->w_cursor.col = 0;
    }

    /* didn't find it: don't move and beep */
    curwin->w_cursor = pos;
    beep_flush();

theend:
    vim_free(tofree);
    convert_setup(&vimconv, NULL, NULL);
}

#if defined(HAVE_GTK2) || defined(PROTO)
/*
 * Return TRUE if string "s" is a valid utf-8 string.
 * When "end" is NULL stop at the first NUL.
 * When "end" is positive stop there.
 */
    int
utf_valid_string(s, end)
    char_u	*s;
    char_u	*end;
{
    int		l;
    char_u	*p = s;

    while (end == NULL ? *p != NUL : p < end)
    {
	if ((*p & 0xc0) == 0x80)
	    return FALSE;	/* invalid lead byte */
	l = utf8len_tab[*p];
	if (end != NULL && p + l > end)
	    return FALSE;	/* incomplete byte sequence */
	++p;
	while (--l > 0)
	    if ((*p++ & 0xc0) != 0x80)
		return FALSE;	/* invalid trail byte */
    }
    return TRUE;
}
#endif

#if defined(FEAT_GUI) || defined(PROTO)
/*
 * Special version of mb_tail_off() for use in ScreenLines[].
 */
    int
dbcs_screen_tail_off(base, p)
    char_u	*base;
    char_u	*p;
{
    /* It can't be the first byte if a double-byte when not using DBCS, at the
     * end of the string or the byte can't start a double-byte.
     * For euc-jp an 0x8e byte always means we have a lead byte in the current
     * cell. */
    if (*p == NUL || p[1] == NUL
	    || (enc_dbcs == DBCS_JPNU && *p == 0x8e)
	    || MB_BYTE2LEN(*p) == 1)
	return 0;

    /* Return 1 when on the lead byte, 0 when on the tail byte. */
    return 1 - dbcs_screen_head_off(base, p);
}
#endif

/*
 * If the cursor moves on an trail byte, set the cursor on the lead byte.
 * Thus it moves left if necessary.
 * Return TRUE when the cursor was adjusted.
 */
    void
mb_adjust_cursor()
{
    mb_adjustpos(&curwin->w_cursor);
}

/*
 * Adjust position "*lp" to point to the first byte of a multi-byte character.
 * If it points to a tail byte it's moved backwards to the head byte.
 */
    void
mb_adjustpos(lp)
    pos_T	*lp;
{
    char_u	*p;

    if (lp->col > 0
#ifdef FEAT_VIRTUALEDIT
	    || lp->coladd > 1
#endif
	    )
    {
	p = ml_get(lp->lnum);
	lp->col -= (*mb_head_off)(p, p + lp->col);
#ifdef FEAT_VIRTUALEDIT
	/* Reset "coladd" when the cursor would be on the right half of a
	 * double-wide character. */
	if (lp->coladd == 1
		&& p[lp->col] != TAB
		&& vim_isprintc((*mb_ptr2char)(p + lp->col))
		&& ptr2cells(p + lp->col) > 1)
	    lp->coladd = 0;
#endif
    }
}

/*
 * Return a pointer to the character before "*p", if there is one.
 */
    char_u *
mb_prevptr(line, p)
    char_u *line;	/* start of the string */
    char_u *p;
{
    if (p > line)
	mb_ptr_back(line, p);
    return p;
}

/*
 * Return the character length of "str".  Each multi-byte character (with
 * following composing characters) counts as one.
 */
    int
mb_charlen(str)
    char_u	*str;
{
    char_u	*p = str;
    int		count;

    if (p == NULL)
	return 0;

    for (count = 0; *p != NUL; count++)
	p += (*mb_ptr2len)(p);

    return count;
}

#if defined(FEAT_SPELL) || defined(PROTO)
/*
 * Like mb_charlen() but for a string with specified length.
 */
    int
mb_charlen_len(str, len)
    char_u	*str;
    int		len;
{
    char_u	*p = str;
    int		count;

    for (count = 0; *p != NUL && p < str + len; count++)
	p += (*mb_ptr2len)(p);

    return count;
}
#endif

/*
 * Try to un-escape a multi-byte character.
 * Used for the "to" and "from" part of a mapping.
 * Return the un-escaped string if it is a multi-byte character, and advance
 * "pp" to just after the bytes that formed it.
 * Return NULL if no multi-byte char was found.
 */
    char_u *
mb_unescape(pp)
    char_u **pp;
{
    static char_u	buf[MB_MAXBYTES + 1];
    int			n, m = 0;
    char_u		*str = *pp;

    /* Must translate K_SPECIAL KS_SPECIAL KE_FILLER to K_SPECIAL and CSI
     * KS_EXTRA KE_CSI to CSI. */
    for (n = 0; str[n] != NUL && m <= MB_MAXBYTES; ++n)
    {
	if (str[n] == K_SPECIAL
		&& str[n + 1] == KS_SPECIAL
		&& str[n + 2] == KE_FILLER)
	{
	    buf[m++] = K_SPECIAL;
	    n += 2;
	}
	else if ((str[n] == K_SPECIAL
# ifdef FEAT_GUI
		    || str[n] == CSI
# endif
		 )
		&& str[n + 1] == KS_EXTRA
		&& str[n + 2] == (int)KE_CSI)
	{
	    buf[m++] = CSI;
	    n += 2;
	}
	else if (str[n] == K_SPECIAL
# ifdef FEAT_GUI
		|| str[n] == CSI
# endif
		)
	    break;		/* a special key can't be a multibyte char */
	else
	    buf[m++] = str[n];
	buf[m] = NUL;

	/* Return a multi-byte character if it's found.  An illegal sequence
	 * will result in a 1 here. */
	if ((*mb_ptr2len)(buf) > 1)
	{
	    *pp = str + n + 1;
	    return buf;
	}
    }
    return NULL;
}

/*
 * Return TRUE if the character at "row"/"col" on the screen is the left side
 * of a double-width character.
 * Caller must make sure "row" and "col" are not invalid!
 */
    int
mb_lefthalve(row, col)
    int	    row;
    int	    col;
{
#ifdef FEAT_HANGULIN
    if (composing_hangul)
	return TRUE;
#endif
    return (*mb_off2cells)(LineOffset[row] + col,
					LineOffset[row] + screen_Columns) > 1;
}

/*
 * Correct a position on the screen, if it's the right half of a double-wide
 * char move it to the left half.  Returns the corrected column.
 */
    int
mb_fix_col(col, row)
    int		col;
    int		row;
{
    col = check_col(col);
    row = check_row(row);
    if (has_mbyte && ScreenLines != NULL && col > 0
	    && ((enc_dbcs
		    && ScreenLines[LineOffset[row] + col] != NUL
		    && dbcs_screen_head_off(ScreenLines + LineOffset[row],
					 ScreenLines + LineOffset[row] + col))
		|| (enc_utf8 && ScreenLines[LineOffset[row] + col] == 0)))
	return col - 1;
    return col;
}
#endif

#if defined(FEAT_MBYTE) || defined(FEAT_POSTSCRIPT) || defined(PROTO)
static int enc_alias_search __ARGS((char_u *name));

/*
 * Skip the Vim specific head of a 'encoding' name.
 */
    char_u *
enc_skip(p)
    char_u	*p;
{
    if (STRNCMP(p, "2byte-", 6) == 0)
	return p + 6;
    if (STRNCMP(p, "8bit-", 5) == 0)
	return p + 5;
    return p;
}

/*
 * Find the canonical name for encoding "enc".
 * When the name isn't recognized, returns "enc" itself, but with all lower
 * case characters and '_' replaced with '-'.
 * Returns an allocated string.  NULL for out-of-memory.
 */
    char_u *
enc_canonize(enc)
    char_u	*enc;
{
    char_u	*r;
    char_u	*p, *s;
    int		i;

# ifdef FEAT_MBYTE
    if (STRCMP(enc, "default") == 0)
    {
	/* Use the default encoding as it's found by set_init_1(). */
	r = get_encoding_default();
	if (r == NULL)
	    r = (char_u *)"latin1";
	return vim_strsave(r);
    }
# endif

    /* copy "enc" to allocated memory, with room for two '-' */
    r = alloc((unsigned)(STRLEN(enc) + 3));
    if (r != NULL)
    {
	/* Make it all lower case and replace '_' with '-'. */
	p = r;
	for (s = enc; *s != NUL; ++s)
	{
	    if (*s == '_')
		*p++ = '-';
	    else
		*p++ = TOLOWER_ASC(*s);
	}
	*p = NUL;

	/* Skip "2byte-" and "8bit-". */
	p = enc_skip(r);

	/* Change "microsoft-cp" to "cp".  Used in some spell files. */
	if (STRNCMP(p, "microsoft-cp", 12) == 0)
	    STRMOVE(p, p + 10);

	/* "iso8859" -> "iso-8859" */
	if (STRNCMP(p, "iso8859", 7) == 0)
	{
	    STRMOVE(p + 4, p + 3);
	    p[3] = '-';
	}

	/* "iso-8859n" -> "iso-8859-n" */
	if (STRNCMP(p, "iso-8859", 8) == 0 && p[8] != '-')
	{
	    STRMOVE(p + 9, p + 8);
	    p[8] = '-';
	}

	/* "latin-N" -> "latinN" */
	if (STRNCMP(p, "latin-", 6) == 0)
	    STRMOVE(p + 5, p + 6);

	if (enc_canon_search(p) >= 0)
	{
	    /* canonical name can be used unmodified */
	    if (p != r)
		STRMOVE(r, p);
	}
	else if ((i = enc_alias_search(p)) >= 0)
	{
	    /* alias recognized, get canonical name */
	    vim_free(r);
	    r = vim_strsave((char_u *)enc_canon_table[i].name);
	}
    }
    return r;
}

/*
 * Search for an encoding alias of "name".
 * Returns -1 when not found.
 */
    static int
enc_alias_search(name)
    char_u	*name;
{
    int		i;

    for (i = 0; enc_alias_table[i].name != NULL; ++i)
	if (STRCMP(name, enc_alias_table[i].name) == 0)
	    return enc_alias_table[i].canon;
    return -1;
}
#endif

#if defined(FEAT_MBYTE) || defined(PROTO)

#ifdef HAVE_LANGINFO_H
# include <langinfo.h>
#endif

/*
 * Get the canonicalized encoding of the current locale.
 * Returns an allocated string when successful, NULL when not.
 */
    char_u *
enc_locale()
{
#ifndef WIN3264
    char	*s;
    char	*p;
    int		i;
#endif
    char	buf[50];
#ifdef WIN3264
    long	acp = GetACP();

    if (acp == 1200)
	STRCPY(buf, "ucs-2le");
    else if (acp == 1252)	    /* cp1252 is used as latin1 */
	STRCPY(buf, "latin1");
    else
	sprintf(buf, "cp%ld", acp);
#else
# ifdef HAVE_NL_LANGINFO_CODESET
    if ((s = nl_langinfo(CODESET)) == NULL || *s == NUL)
# endif
#  if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
	if ((s = setlocale(LC_CTYPE, NULL)) == NULL || *s == NUL)
#  endif
	    if ((s = getenv("LC_ALL")) == NULL || *s == NUL)
		if ((s = getenv("LC_CTYPE")) == NULL || *s == NUL)
		    s = getenv("LANG");

    if (s == NULL || *s == NUL)
	return FAIL;

    /* The most generic locale format is:
     * language[_territory][.codeset][@modifier][+special][,[sponsor][_revision]]
     * If there is a '.' remove the part before it.
     * if there is something after the codeset, remove it.
     * Make the name lowercase and replace '_' with '-'.
     * Exception: "ja_JP.EUC" == "euc-jp", "zh_CN.EUC" = "euc-cn",
     * "ko_KR.EUC" == "euc-kr"
     */
    if ((p = (char *)vim_strchr((char_u *)s, '.')) != NULL)
    {
	if (p > s + 2 && STRNICMP(p + 1, "EUC", 3) == 0
			&& !isalnum((int)p[4]) && p[4] != '-' && p[-3] == '_')
	{
	    /* copy "XY.EUC" to "euc-XY" to buf[10] */
	    STRCPY(buf + 10, "euc-");
	    buf[14] = p[-2];
	    buf[15] = p[-1];
	    buf[16] = 0;
	    s = buf + 10;
	}
	else
	    s = p + 1;
    }
    for (i = 0; s[i] != NUL && i < sizeof(buf) - 1; ++i)
    {
	if (s[i] == '_' || s[i] == '-')
	    buf[i] = '-';
	else if (isalnum((int)s[i]))
	    buf[i] = TOLOWER_ASC(s[i]);
	else
	    break;
    }
    buf[i] = NUL;
#endif

    return enc_canonize((char_u *)buf);
}

#if defined(WIN3264) || defined(PROTO)
/*
 * Convert an encoding name to an MS-Windows codepage.
 * Returns zero if no codepage can be figured out.
 */
    int
encname2codepage(name)
    char_u	*name;
{
    int		cp;
    char_u	*p = name;
    int		idx;

    if (STRNCMP(p, "8bit-", 5) == 0)
	p += 5;
    else if (STRNCMP(p_enc, "2byte-", 6) == 0)
	p += 6;

    if (p[0] == 'c' && p[1] == 'p')
	cp = atoi(p + 2);
    else if ((idx = enc_canon_search(p)) >= 0)
	cp = enc_canon_table[idx].codepage;
    else
	return 0;
    if (IsValidCodePage(cp))
	return cp;
    return 0;
}
#endif

# if defined(USE_ICONV) || defined(PROTO)

static char_u *iconv_string __ARGS((vimconv_T *vcp, char_u *str, int slen, int *unconvlenp));

/*
 * Call iconv_open() with a check if iconv() works properly (there are broken
 * versions).
 * Returns (void *)-1 if failed.
 * (should return iconv_t, but that causes problems with prototypes).
 */
    void *
my_iconv_open(to, from)
    char_u	*to;
    char_u	*from;
{
    iconv_t	fd;
#define ICONV_TESTLEN 400
    char_u	tobuf[ICONV_TESTLEN];
    char	*p;
    size_t	tolen;
    static int	iconv_ok = -1;

    if (iconv_ok == FALSE)
	return (void *)-1;	/* detected a broken iconv() previously */

#ifdef DYNAMIC_ICONV
    /* Check if the iconv.dll can be found. */
    if (!iconv_enabled(TRUE))
	return (void *)-1;
#endif

    fd = iconv_open((char *)enc_skip(to), (char *)enc_skip(from));

    if (fd != (iconv_t)-1 && iconv_ok == -1)
    {
	/*
	 * Do a dummy iconv() call to check if it actually works.  There is a
	 * version of iconv() on Linux that is broken.  We can't ignore it,
	 * because it's wide-spread.  The symptoms are that after outputting
	 * the initial shift state the "to" pointer is NULL and conversion
	 * stops for no apparent reason after about 8160 characters.
	 */
	p = (char *)tobuf;
	tolen = ICONV_TESTLEN;
	(void)iconv(fd, NULL, NULL, &p, &tolen);
	if (p == NULL)
	{
	    iconv_ok = FALSE;
	    iconv_close(fd);
	    fd = (iconv_t)-1;
	}
	else
	    iconv_ok = TRUE;
    }

    return (void *)fd;
}

/*
 * Convert the string "str[slen]" with iconv().
 * If "unconvlenp" is not NULL handle the string ending in an incomplete
 * sequence and set "*unconvlenp" to the length of it.
 * Returns the converted string in allocated memory.  NULL for an error.
 */
    static char_u *
iconv_string(vcp, str, slen, unconvlenp)
    vimconv_T	*vcp;
    char_u	*str;
    int		slen;
    int		*unconvlenp;
{
    const char	*from;
    size_t	fromlen;
    char	*to;
    size_t	tolen;
    size_t	len = 0;
    size_t	done = 0;
    char_u	*result = NULL;
    char_u	*p;
    int		l;

    from = (char *)str;
    fromlen = slen;
    for (;;)
    {
	if (len == 0 || ICONV_ERRNO == ICONV_E2BIG)
	{
	    /* Allocate enough room for most conversions.  When re-allocating
	     * increase the buffer size. */
	    len = len + fromlen * 2 + 40;
	    p = alloc((unsigned)len);
	    if (p != NULL && done > 0)
		mch_memmove(p, result, done);
	    vim_free(result);
	    result = p;
	    if (result == NULL)	/* out of memory */
		break;
	}

	to = (char *)result + done;
	tolen = len - done - 2;
	/* Avoid a warning for systems with a wrong iconv() prototype by
	 * casting the second argument to void *. */
	if (iconv(vcp->vc_fd, (void *)&from, &fromlen, &to, &tolen)
								!= (size_t)-1)
	{
	    /* Finished, append a NUL. */
	    *to = NUL;
	    break;
	}

	/* Check both ICONV_EINVAL and EINVAL, because the dynamically loaded
	 * iconv library may use one of them. */
	if (!vcp->vc_fail && unconvlenp != NULL
		&& (ICONV_ERRNO == ICONV_EINVAL || ICONV_ERRNO == EINVAL))
	{
	    /* Handle an incomplete sequence at the end. */
	    *to = NUL;
	    *unconvlenp = (int)fromlen;
	    break;
	}

	/* Check both ICONV_EILSEQ and EILSEQ, because the dynamically loaded
	 * iconv library may use one of them. */
	else if (!vcp->vc_fail
		&& (ICONV_ERRNO == ICONV_EILSEQ || ICONV_ERRNO == EILSEQ
		    || ICONV_ERRNO == ICONV_EINVAL || ICONV_ERRNO == EINVAL))
	{
	    /* Can't convert: insert a '?' and skip a character.  This assumes
	     * conversion from 'encoding' to something else.  In other
	     * situations we don't know what to skip anyway. */
	    *to++ = '?';
	    if ((*mb_ptr2cells)((char_u *)from) > 1)
		*to++ = '?';
	    if (enc_utf8)
		l = utfc_ptr2len_len((char_u *)from, (int)fromlen);
	    else
	    {
		l = (*mb_ptr2len)((char_u *)from);
		if (l > (int)fromlen)
		    l = (int)fromlen;
	    }
	    from += l;
	    fromlen -= l;
	}
	else if (ICONV_ERRNO != ICONV_E2BIG)
	{
	    /* conversion failed */
	    vim_free(result);
	    result = NULL;
	    break;
	}
	/* Not enough room or skipping illegal sequence. */
	done = to - (char *)result;
    }
    return result;
}

#  if defined(DYNAMIC_ICONV) || defined(PROTO)
/*
 * Dynamically load the "iconv.dll" on Win32.
 */

#ifndef DYNAMIC_ICONV	    /* just generating prototypes */
# define HINSTANCE int
#endif
static HINSTANCE hIconvDLL = 0;
static HINSTANCE hMsvcrtDLL = 0;

#  ifndef DYNAMIC_ICONV_DLL
#   define DYNAMIC_ICONV_DLL "iconv.dll"
#   define DYNAMIC_ICONV_DLL_ALT "libiconv.dll"
#  endif
#  ifndef DYNAMIC_MSVCRT_DLL
#   define DYNAMIC_MSVCRT_DLL "msvcrt.dll"
#  endif

/*
 * Try opening the iconv.dll and return TRUE if iconv() can be used.
 */
    int
iconv_enabled(verbose)
    int		verbose;
{
    if (hIconvDLL != 0 && hMsvcrtDLL != 0)
	return TRUE;
    hIconvDLL = LoadLibrary(DYNAMIC_ICONV_DLL);
    if (hIconvDLL == 0)		/* sometimes it's called libiconv.dll */
	hIconvDLL = LoadLibrary(DYNAMIC_ICONV_DLL_ALT);
    if (hIconvDLL != 0)
	hMsvcrtDLL = LoadLibrary(DYNAMIC_MSVCRT_DLL);
    if (hIconvDLL == 0 || hMsvcrtDLL == 0)
    {
	/* Only give the message when 'verbose' is set, otherwise it might be
	 * done whenever a conversion is attempted. */
	if (verbose && p_verbose > 0)
	{
	    verbose_enter();
	    EMSG2(_(e_loadlib),
		    hIconvDLL == 0 ? DYNAMIC_ICONV_DLL : DYNAMIC_MSVCRT_DLL);
	    verbose_leave();
	}
	iconv_end();
	return FALSE;
    }

    iconv	= (void *)GetProcAddress(hIconvDLL, "libiconv");
    iconv_open	= (void *)GetProcAddress(hIconvDLL, "libiconv_open");
    iconv_close	= (void *)GetProcAddress(hIconvDLL, "libiconv_close");
    iconvctl	= (void *)GetProcAddress(hIconvDLL, "libiconvctl");
    iconv_errno	= (void *)GetProcAddress(hMsvcrtDLL, "_errno");
    if (iconv == NULL || iconv_open == NULL || iconv_close == NULL
	    || iconvctl == NULL || iconv_errno == NULL)
    {
	iconv_end();
	if (verbose && p_verbose > 0)
	{
	    verbose_enter();
	    EMSG2(_(e_loadfunc), "for libiconv");
	    verbose_leave();
	}
	return FALSE;
    }
    return TRUE;
}

    void
iconv_end()
{
    /* Don't use iconv() when inputting or outputting characters. */
    if (input_conv.vc_type == CONV_ICONV)
	convert_setup(&input_conv, NULL, NULL);
    if (output_conv.vc_type == CONV_ICONV)
	convert_setup(&output_conv, NULL, NULL);

    if (hIconvDLL != 0)
	FreeLibrary(hIconvDLL);
    if (hMsvcrtDLL != 0)
	FreeLibrary(hMsvcrtDLL);
    hIconvDLL = 0;
    hMsvcrtDLL = 0;
}
#  endif /* DYNAMIC_ICONV */
# endif /* USE_ICONV */

#endif /* FEAT_MBYTE */

#if defined(FEAT_XIM) || defined(PROTO)

# ifdef FEAT_GUI_GTK
static int xim_has_preediting INIT(= FALSE);  /* IM current status */

/*
 * Set preedit_start_col to the current cursor position.
 */
    static void
init_preedit_start_col(void)
{
    if (State & CMDLINE)
	preedit_start_col = cmdline_getvcol_cursor();
    else if (curwin != NULL)
	getvcol(curwin, &curwin->w_cursor, &preedit_start_col, NULL, NULL);
    /* Prevent that preediting marks the buffer as changed. */
    xim_changed_while_preediting = curbuf->b_changed;
}
# endif

# if defined(HAVE_GTK2) && !defined(PROTO)

static int im_is_active	       = FALSE;	/* IM is enabled for current mode    */
static int preedit_is_active   = FALSE;
static int im_preedit_cursor   = 0;	/* cursor offset in characters       */
static int im_preedit_trailing = 0;	/* number of characters after cursor */

static unsigned long im_commit_handler_id  = 0;
static unsigned int  im_activatekey_keyval = GDK_VoidSymbol;
static unsigned int  im_activatekey_state  = 0;

    void
im_set_active(int active)
{
    int was_active;

    was_active = !!im_is_active;
    im_is_active = (active && !p_imdisable);

    if (im_is_active != was_active)
	xim_reset();
}

    void
xim_set_focus(int focus)
{
    if (xic != NULL)
    {
	if (focus)
	    gtk_im_context_focus_in(xic);
	else
	    gtk_im_context_focus_out(xic);
    }
}

    void
im_set_position(int row, int col)
{
    if (xic != NULL)
    {
	GdkRectangle area;

	area.x = FILL_X(col);
	area.y = FILL_Y(row);
	area.width  = gui.char_width * (mb_lefthalve(row, col) ? 2 : 1);
	area.height = gui.char_height;

	gtk_im_context_set_cursor_location(xic, &area);
    }
}

#  if 0 || defined(PROTO) /* apparently only used in gui_x11.c */
    void
xim_set_preedit(void)
{
    im_set_position(gui.row, gui.col);
}
#  endif

    static void
im_add_to_input(char_u *str, int len)
{
    /* Convert from 'termencoding' (always "utf-8") to 'encoding' */
    if (input_conv.vc_type != CONV_NONE)
    {
	str = string_convert(&input_conv, str, &len);
	g_return_if_fail(str != NULL);
    }

    add_to_input_buf_csi(str, len);

    if (input_conv.vc_type != CONV_NONE)
	vim_free(str);

    if (p_mh) /* blank out the pointer if necessary */
	gui_mch_mousehide(TRUE);
}

    static void
im_delete_preedit(void)
{
    char_u bskey[]  = {CSI, 'k', 'b'};
    char_u delkey[] = {CSI, 'k', 'D'};

    if (State & NORMAL)
    {
	im_preedit_cursor = 0;
	return;
    }
    for (; im_preedit_cursor > 0; --im_preedit_cursor)
	add_to_input_buf(bskey, (int)sizeof(bskey));

    for (; im_preedit_trailing > 0; --im_preedit_trailing)
	add_to_input_buf(delkey, (int)sizeof(delkey));
}

/*
 * Move the cursor left by "num_move_back" characters.
 * Note that ins_left() checks im_is_preediting() to avoid breaking undo for
 * these K_LEFT keys.
 */
    static void
im_correct_cursor(int num_move_back)
{
    char_u backkey[] = {CSI, 'k', 'l'};

    if (State & NORMAL)
	return;
#  ifdef FEAT_RIGHTLEFT
    if ((State & CMDLINE) == 0 && curwin != NULL && curwin->w_p_rl)
	backkey[2] = 'r';
#  endif
    for (; num_move_back > 0; --num_move_back)
	add_to_input_buf(backkey, (int)sizeof(backkey));
}

static int xim_expected_char = NUL;
static int xim_ignored_char = FALSE;

/*
 * Update the mode and cursor while in an IM callback.
 */
    static void
im_show_info(void)
{
    int	    old_vgetc_busy;

    old_vgetc_busy = vgetc_busy;
    vgetc_busy = TRUE;
    showmode();
    vgetc_busy = old_vgetc_busy;
    setcursor();
    out_flush();
}

/*
 * Callback invoked when the user finished preediting.
 * Put the final string into the input buffer.
 */
/*ARGSUSED0*/
    static void
im_commit_cb(GtkIMContext *context, const gchar *str, gpointer data)
{
    int	slen = (int)STRLEN(str);
    int	add_to_input = TRUE;
    int	clen;
    int	len = slen;
    int	commit_with_preedit = TRUE;
    char_u	*im_str, *p;

#ifdef XIM_DEBUG
    xim_log("im_commit_cb(): %s\n", str);
#endif

    /* The imhangul module doesn't reset the preedit string before
     * committing.  Call im_delete_preedit() to work around that. */
    im_delete_preedit();

    /* Indicate that preediting has finished. */
    if (preedit_start_col == MAXCOL)
    {
	init_preedit_start_col();
	commit_with_preedit = FALSE;
    }

    /* The thing which setting "preedit_start_col" to MAXCOL means that
     * "preedit_start_col" will be set forcely when calling
     * preedit_changed_cb() next time.
     * "preedit_start_col" should not reset with MAXCOL on this part. Vim
     * is simulating the preediting by using add_to_input_str(). when
     * preedit begin immediately before committed, the typebuf is not
     * flushed to screen, then it can't get correct "preedit_start_col".
     * Thus, it should calculate the cells by adding cells of the committed
     * string. */
    if (input_conv.vc_type != CONV_NONE)
    {
	im_str = string_convert(&input_conv, (char_u *)str, &len);
	g_return_if_fail(im_str != NULL);
    }
    else
	im_str = (char_u *)str;
    clen = 0;
    for (p = im_str; p < im_str + len; p += (*mb_ptr2len)(p))
	clen += (*mb_ptr2cells)(p);
    if (input_conv.vc_type != CONV_NONE)
	vim_free(im_str);
    preedit_start_col += clen;

    /* Is this a single character that matches a keypad key that's just
     * been pressed?  If so, we don't want it to be entered as such - let
     * us carry on processing the raw keycode so that it may be used in
     * mappings as <kSomething>. */
    if (xim_expected_char != NUL)
    {
	/* We're currently processing a keypad or other special key */
	if (slen == 1 && str[0] == xim_expected_char)
	{
	    /* It's a match - don't do it here */
	    xim_ignored_char = TRUE;
	    add_to_input = FALSE;
	}
	else
	{
	    /* Not a match */
	    xim_ignored_char = FALSE;
	}
    }

    if (add_to_input)
	im_add_to_input((char_u *)str, slen);

    /* Inserting chars while "im_is_active" is set does not cause a change of
     * buffer.  When the chars are committed the buffer must be marked as
     * changed. */
    if (!commit_with_preedit)
	preedit_start_col = MAXCOL;

    /* This flag is used in changed() at next call. */
    xim_changed_while_preediting = TRUE;

    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*
 * Callback invoked after start to the preedit.
 */
/*ARGSUSED*/
    static void
im_preedit_start_cb(GtkIMContext *context, gpointer data)
{
#ifdef XIM_DEBUG
    xim_log("im_preedit_start_cb()\n");
#endif

    im_is_active = TRUE;
    preedit_is_active = TRUE;
    gui_update_cursor(TRUE, FALSE);
    im_show_info();
}

/*
 * Callback invoked after end to the preedit.
 */
/*ARGSUSED*/
    static void
im_preedit_end_cb(GtkIMContext *context, gpointer data)
{
#ifdef XIM_DEBUG
    xim_log("im_preedit_end_cb()\n");
#endif
    im_delete_preedit();

    /* Indicate that preediting has finished */
    preedit_start_col = MAXCOL;
    xim_has_preediting = FALSE;

#if 0
    /* Removal of this line suggested by Takuhiro Nishioka.  Fixes that IM was
     * switched off unintentionally.  We now use preedit_is_active (added by
     * SungHyun Nam). */
    im_is_active = FALSE;
#endif
    preedit_is_active = FALSE;
    gui_update_cursor(TRUE, FALSE);
    im_show_info();
}

/*
 * Callback invoked after changes to the preedit string.  If the preedit
 * string was empty before, remember the preedit start column so we know
 * where to apply feedback attributes.  Delete the previous preedit string
 * if there was one, save the new preedit cursor offset, and put the new
 * string into the input buffer.
 *
 * TODO: The pragmatic "put into input buffer" approach used here has
 *       several fundamental problems:
 *
 * - The characters in the preedit string are subject to remapping.
 *   That's broken, only the finally committed string should be remapped.
 *
 * - There is a race condition involved:  The retrieved value for the
 *   current cursor position will be wrong if any unprocessed characters
 *   are still queued in the input buffer.
 *
 * - Due to the lack of synchronization between the file buffer in memory
 *   and any typed characters, it's practically impossible to implement the
 *   "retrieve_surrounding" and "delete_surrounding" signals reliably.  IM
 *   modules for languages such as Thai are likely to rely on this feature
 *   for proper operation.
 *
 * Conclusions:  I think support for preediting needs to be moved to the
 * core parts of Vim.  Ideally, until it has been committed, the preediting
 * string should only be displayed and not affect the buffer content at all.
 * The question how to deal with the synchronization issue still remains.
 * Circumventing the input buffer is probably not desirable.  Anyway, I think
 * implementing "retrieve_surrounding" is the only hard problem.
 *
 * One way to solve all of this in a clean manner would be to queue all key
 * press/release events "as is" in the input buffer, and apply the IM filtering
 * at the receiving end of the queue.  This, however, would have a rather large
 * impact on the code base.  If there is an easy way to force processing of all
 * remaining input from within the "retrieve_surrounding" signal handler, this
 * might not be necessary.  Gotta ask on vim-dev for opinions.
 */
/*ARGSUSED1*/
    static void
im_preedit_changed_cb(GtkIMContext *context, gpointer data)
{
    char    *preedit_string = NULL;
    int	    cursor_index    = 0;
    int	    num_move_back   = 0;
    char_u  *str;
    char_u  *p;
    int	    i;

    gtk_im_context_get_preedit_string(context,
				      &preedit_string, NULL,
				      &cursor_index);

#ifdef XIM_DEBUG
    xim_log("im_preedit_changed_cb(): %s\n", preedit_string);
#endif

    g_return_if_fail(preedit_string != NULL); /* just in case */

    /* If preedit_start_col is MAXCOL set it to the current cursor position. */
    if (preedit_start_col == MAXCOL && preedit_string[0] != '\0')
    {
	xim_has_preediting = TRUE;

	/* Urgh, this breaks if the input buffer isn't empty now */
	init_preedit_start_col();
    }
    else if (cursor_index == 0 && preedit_string[0] == '\0')
    {
	xim_has_preediting = FALSE;

	/* If at the start position (after typing backspace)
	 * preedit_start_col must be reset. */
	preedit_start_col = MAXCOL;
    }

    im_delete_preedit();

    /*
     * Compute the end of the preediting area: "preedit_end_col".
     * According to the documentation of gtk_im_context_get_preedit_string(),
     * the cursor_pos output argument returns the offset in bytes.  This is
     * unfortunately not true -- real life shows the offset is in characters,
     * and the GTK+ source code agrees with me.  Will file a bug later.
     */
    if (preedit_start_col != MAXCOL)
	preedit_end_col = preedit_start_col;
    str = (char_u *)preedit_string;
    for (p = str, i = 0; *p != NUL; p += utf_byte2len(*p), ++i)
    {
	int is_composing;

	is_composing = ((*p & 0x80) != 0 && utf_iscomposing(utf_ptr2char(p)));
	/*
	 * These offsets are used as counters when generating <BS> and <Del>
	 * to delete the preedit string.  So don't count composing characters
	 * unless 'delcombine' is enabled.
	 */
	if (!is_composing || p_deco)
	{
	    if (i < cursor_index)
		++im_preedit_cursor;
	    else
		++im_preedit_trailing;
	}
	if (!is_composing && i >= cursor_index)
	{
	    /* This is essentially the same as im_preedit_trailing, except
	     * composing characters are not counted even if p_deco is set. */
	    ++num_move_back;
	}
	if (preedit_start_col != MAXCOL)
	    preedit_end_col += utf_ptr2cells(p);
    }

    if (p > str)
    {
	im_add_to_input(str, (int)(p - str));
	im_correct_cursor(num_move_back);
    }

    g_free(preedit_string);

    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*
 * Translate the Pango attributes at iter to Vim highlighting attributes.
 * Ignore attributes not supported by Vim highlighting.  This shouldn't have
 * too much impact -- right now we handle even more attributes than necessary
 * for the IM modules I tested with.
 */
    static int
translate_pango_attributes(PangoAttrIterator *iter)
{
    PangoAttribute  *attr;
    int		    char_attr = HL_NORMAL;

    attr = pango_attr_iterator_get(iter, PANGO_ATTR_UNDERLINE);
    if (attr != NULL && ((PangoAttrInt *)attr)->value
						 != (int)PANGO_UNDERLINE_NONE)
	char_attr |= HL_UNDERLINE;

    attr = pango_attr_iterator_get(iter, PANGO_ATTR_WEIGHT);
    if (attr != NULL && ((PangoAttrInt *)attr)->value >= (int)PANGO_WEIGHT_BOLD)
	char_attr |= HL_BOLD;

    attr = pango_attr_iterator_get(iter, PANGO_ATTR_STYLE);
    if (attr != NULL && ((PangoAttrInt *)attr)->value
						   != (int)PANGO_STYLE_NORMAL)
	char_attr |= HL_ITALIC;

    attr = pango_attr_iterator_get(iter, PANGO_ATTR_BACKGROUND);
    if (attr != NULL)
    {
	const PangoColor *color = &((PangoAttrColor *)attr)->color;

	/* Assume inverse if black background is requested */
	if ((color->red | color->green | color->blue) == 0)
	    char_attr |= HL_INVERSE;
    }

    return char_attr;
}

/*
 * Retrieve the highlighting attributes at column col in the preedit string.
 * Return -1 if not in preediting mode or if col is out of range.
 */
    int
im_get_feedback_attr(int col)
{
    char	    *preedit_string = NULL;
    PangoAttrList   *attr_list	    = NULL;
    int		    char_attr	    = -1;

    if (xic == NULL)
	return char_attr;

    gtk_im_context_get_preedit_string(xic, &preedit_string, &attr_list, NULL);

    if (preedit_string != NULL && attr_list != NULL)
    {
	int idx;

	/* Get the byte index as used by PangoAttrIterator */
	for (idx = 0; col > 0 && preedit_string[idx] != '\0'; --col)
	    idx += utfc_ptr2len((char_u *)preedit_string + idx);

	if (preedit_string[idx] != '\0')
	{
	    PangoAttrIterator	*iter;
	    int			start, end;

	    char_attr = HL_NORMAL;
	    iter = pango_attr_list_get_iterator(attr_list);

	    /* Extract all relevant attributes from the list. */
	    do
	    {
		pango_attr_iterator_range(iter, &start, &end);

		if (idx >= start && idx < end)
		    char_attr |= translate_pango_attributes(iter);
	    }
	    while (pango_attr_iterator_next(iter));

	    pango_attr_iterator_destroy(iter);
	}
    }

    if (attr_list != NULL)
	pango_attr_list_unref(attr_list);
    g_free(preedit_string);

    return char_attr;
}

    void
xim_init(void)
{
#ifdef XIM_DEBUG
    xim_log("xim_init()\n");
#endif

    g_return_if_fail(gui.drawarea != NULL);
    g_return_if_fail(gui.drawarea->window != NULL);

    xic = gtk_im_multicontext_new();
    g_object_ref(xic);

    im_commit_handler_id = g_signal_connect(G_OBJECT(xic), "commit",
					    G_CALLBACK(&im_commit_cb), NULL);
    g_signal_connect(G_OBJECT(xic), "preedit_changed",
		     G_CALLBACK(&im_preedit_changed_cb), NULL);
    g_signal_connect(G_OBJECT(xic), "preedit_start",
		     G_CALLBACK(&im_preedit_start_cb), NULL);
    g_signal_connect(G_OBJECT(xic), "preedit_end",
		     G_CALLBACK(&im_preedit_end_cb), NULL);

    gtk_im_context_set_client_window(xic, gui.drawarea->window);
}

    void
im_shutdown(void)
{
#ifdef XIM_DEBUG
    xim_log("im_shutdown()\n");
#endif

    if (xic != NULL)
    {
	gtk_im_context_focus_out(xic);
	g_object_unref(xic);
	xic = NULL;
    }
    im_is_active = FALSE;
    im_commit_handler_id = 0;
    preedit_start_col = MAXCOL;
    xim_has_preediting = FALSE;
}

/*
 * Convert the string argument to keyval and state for GdkEventKey.
 * If str is valid return TRUE, otherwise FALSE.
 *
 * See 'imactivatekey' for documentation of the format.
 */
    static int
im_string_to_keyval(const char *str, unsigned int *keyval, unsigned int *state)
{
    const char	    *mods_end;
    unsigned	    tmp_keyval;
    unsigned	    tmp_state = 0;

    mods_end = strrchr(str, '-');
    mods_end = (mods_end != NULL) ? mods_end + 1 : str;

    /* Parse modifier keys */
    while (str < mods_end)
	switch (*str++)
	{
	    case '-':							break;
	    case 'S': case 's': tmp_state |= (unsigned)GDK_SHIFT_MASK;	break;
	    case 'L': case 'l': tmp_state |= (unsigned)GDK_LOCK_MASK;	break;
	    case 'C': case 'c': tmp_state |= (unsigned)GDK_CONTROL_MASK;break;
	    case '1':		tmp_state |= (unsigned)GDK_MOD1_MASK;	break;
	    case '2':		tmp_state |= (unsigned)GDK_MOD2_MASK;	break;
	    case '3':		tmp_state |= (unsigned)GDK_MOD3_MASK;	break;
	    case '4':		tmp_state |= (unsigned)GDK_MOD4_MASK;	break;
	    case '5':		tmp_state |= (unsigned)GDK_MOD5_MASK;	break;
	    default:
		return FALSE;
	}

    tmp_keyval = gdk_keyval_from_name(str);

    if (tmp_keyval == 0 || tmp_keyval == GDK_VoidSymbol)
	return FALSE;

    if (keyval != NULL)
	*keyval = tmp_keyval;
    if (state != NULL)
	*state = tmp_state;

    return TRUE;
}

/*
 * Return TRUE if p_imak is valid, otherwise FALSE.  As a special case, an
 * empty string is also regarded as valid.
 *
 * Note: The numerical key value of p_imak is cached if it was valid; thus
 * boldly assuming im_xim_isvalid_imactivate() will always be called whenever
 * 'imak' changes.  This is currently the case but not obvious -- should
 * probably rename the function for clarity.
 */
    int
im_xim_isvalid_imactivate(void)
{
    if (p_imak[0] == NUL)
    {
	im_activatekey_keyval = GDK_VoidSymbol;
	im_activatekey_state  = 0;
	return TRUE;
    }

    return im_string_to_keyval((const char *)p_imak,
			       &im_activatekey_keyval,
			       &im_activatekey_state);
}

    static void
im_synthesize_keypress(unsigned int keyval, unsigned int state)
{
    GdkEventKey *event;

#  ifdef HAVE_GTK_MULTIHEAD
    event = (GdkEventKey *)gdk_event_new(GDK_KEY_PRESS);
    g_object_ref(gui.drawarea->window); /* unreffed by gdk_event_free() */
#  else
    event = (GdkEventKey *)g_malloc0((gulong)sizeof(GdkEvent));
    event->type = GDK_KEY_PRESS;
#  endif
    event->window = gui.drawarea->window;
    event->send_event = TRUE;
    event->time = GDK_CURRENT_TIME;
    event->state  = state;
    event->keyval = keyval;
    event->hardware_keycode = /* needed for XIM */
	XKeysymToKeycode(GDK_WINDOW_XDISPLAY(event->window), (KeySym)keyval);
    event->length = 0;
    event->string = NULL;

    gtk_im_context_filter_keypress(xic, event);

    /* For consistency, also send the corresponding release event. */
    event->type = GDK_KEY_RELEASE;
    event->send_event = FALSE;
    gtk_im_context_filter_keypress(xic, event);

#  ifdef HAVE_GTK_MULTIHEAD
    gdk_event_free((GdkEvent *)event);
#  else
    g_free(event);
#  endif
}

    void
xim_reset(void)
{
    if (xic != NULL)
    {
	/*
	 * The third-party imhangul module (and maybe others too) ignores
	 * gtk_im_context_reset() or at least doesn't reset the active state.
	 * Thus sending imactivatekey would turn it off if it was on before,
	 * which is clearly not what we want.  Fortunately we can work around
	 * that for imhangul by sending GDK_Escape, but I don't know if it
	 * works with all IM modules that support an activation key :/
	 *
	 * An alternative approach would be to destroy the IM context and
	 * recreate it.  But that means loading/unloading the IM module on
	 * every mode switch, which causes a quite noticable delay even on
	 * my rather fast box...
	 * *
	 * Moreover, there are some XIM which cannot respond to
	 * im_synthesize_keypress(). we hope that they reset by
	 * xim_shutdown().
	 */
	if (im_activatekey_keyval != GDK_VoidSymbol && im_is_active)
	    im_synthesize_keypress(GDK_Escape, 0U);

	gtk_im_context_reset(xic);

	/*
	 * HACK for Ami: This sequence of function calls makes Ami handle
	 * the IM reset graciously, without breaking loads of other stuff.
	 * It seems to force English mode as well, which is exactly what we
	 * want because it makes the Ami status display work reliably.
	 */
	gtk_im_context_set_use_preedit(xic, FALSE);

	if (p_imdisable)
	    im_shutdown();
	else
	{
	    gtk_im_context_set_use_preedit(xic, TRUE);
	    xim_set_focus(gui.in_focus);

	    if (im_activatekey_keyval != GDK_VoidSymbol)
	    {
		if (im_is_active)
		{
		    g_signal_handler_block(xic, im_commit_handler_id);
		    im_synthesize_keypress(im_activatekey_keyval,
						    im_activatekey_state);
		    g_signal_handler_unblock(xic, im_commit_handler_id);
		}
	    }
	    else
	    {
		im_shutdown();
		xim_init();
		xim_set_focus(gui.in_focus);
	    }
	}
    }

    preedit_start_col = MAXCOL;
    xim_has_preediting = FALSE;
}

    int
xim_queue_key_press_event(GdkEventKey *event, int down)
{
    if (down)
    {
	/*
	 * Workaround GTK2 XIM 'feature' that always converts keypad keys to
	 * chars., even when not part of an IM sequence (ref. feature of
	 * gdk/gdkkeyuni.c).
	 * Flag any keypad keys that might represent a single char.
	 * If this (on its own - i.e., not part of an IM sequence) is
	 * committed while we're processing one of these keys, we can ignore
	 * that commit and go ahead & process it ourselves.  That way we can
	 * still distinguish keypad keys for use in mappings.
	 * Also add GDK_space to make <S-Space> work.
	 */
	switch (event->keyval)
	{
	    case GDK_KP_Add:      xim_expected_char = '+';  break;
	    case GDK_KP_Subtract: xim_expected_char = '-';  break;
	    case GDK_KP_Divide:   xim_expected_char = '/';  break;
	    case GDK_KP_Multiply: xim_expected_char = '*';  break;
	    case GDK_KP_Decimal:  xim_expected_char = '.';  break;
	    case GDK_KP_Equal:    xim_expected_char = '=';  break;
	    case GDK_KP_0:	  xim_expected_char = '0';  break;
	    case GDK_KP_1:	  xim_expected_char = '1';  break;
	    case GDK_KP_2:	  xim_expected_char = '2';  break;
	    case GDK_KP_3:	  xim_expected_char = '3';  break;
	    case GDK_KP_4:	  xim_expected_char = '4';  break;
	    case GDK_KP_5:	  xim_expected_char = '5';  break;
	    case GDK_KP_6:	  xim_expected_char = '6';  break;
	    case GDK_KP_7:	  xim_expected_char = '7';  break;
	    case GDK_KP_8:	  xim_expected_char = '8';  break;
	    case GDK_KP_9:	  xim_expected_char = '9';  break;
	    case GDK_space:	  xim_expected_char = ' ';  break;
	    default:		  xim_expected_char = NUL;
	}
	xim_ignored_char = FALSE;
    }

    /*
     * When typing fFtT, XIM may be activated. Thus it must pass
     * gtk_im_context_filter_keypress() in Normal mode.
     * And while doing :sh too.
     */
    if (xic != NULL && !p_imdisable
		    && (State & (INSERT | CMDLINE | NORMAL | EXTERNCMD)) != 0)
    {
	/*
	 * Filter 'imactivatekey' and map it to CTRL-^.  This way, Vim is
	 * always aware of the current status of IM, and can even emulate
	 * the activation key for modules that don't support one.
	 */
	if (event->keyval == im_activatekey_keyval
	     && (event->state & im_activatekey_state) == im_activatekey_state)
	{
	    unsigned int state_mask;

	    /* Require the state of the 3 most used modifiers to match exactly.
	     * Otherwise e.g. <S-C-space> would be unusable for other purposes
	     * if the IM activate key is <S-space>. */
	    state_mask  = im_activatekey_state;
	    state_mask |= ((int)GDK_SHIFT_MASK | (int)GDK_CONTROL_MASK
							| (int)GDK_MOD1_MASK);

	    if ((event->state & state_mask) != im_activatekey_state)
		return FALSE;

	    /* Don't send it a second time on GDK_KEY_RELEASE. */
	    if (event->type != GDK_KEY_PRESS)
		return TRUE;

	    if (map_to_exists_mode((char_u *)"", LANGMAP, FALSE))
	    {
		im_set_active(FALSE);

		/* ":lmap" mappings exists, toggle use of mappings. */
		State ^= LANGMAP;
		if (State & LANGMAP)
		{
		    curbuf->b_p_iminsert = B_IMODE_NONE;
		    State &= ~LANGMAP;
		}
		else
		{
		    curbuf->b_p_iminsert = B_IMODE_LMAP;
		    State |= LANGMAP;
		}
		return TRUE;
	    }

	    return gtk_im_context_filter_keypress(xic, event);
	}

	/* Don't filter events through the IM context if IM isn't active
	 * right now.  Unlike with GTK+ 1.2 we cannot rely on the IM module
	 * not doing anything before the activation key was sent. */
	if (im_activatekey_keyval == GDK_VoidSymbol || im_is_active)
	{
	    int imresult = gtk_im_context_filter_keypress(xic, event);

	    /* Some XIM send following sequence:
	     * 1. preedited string.
	     * 2. committed string.
	     * 3. line changed key.
	     * 4. preedited string.
	     * 5. remove preedited string.
	     * if 3, Vim can't move back the above line for 5.
	     * thus, this part should not parse the key. */
	    if (!imresult && preedit_start_col != MAXCOL
					       && event->keyval == GDK_Return)
	    {
		im_synthesize_keypress(GDK_Return, 0U);
		return FALSE;
	    }

	    /* If XIM tried to commit a keypad key as a single char.,
	     * ignore it so we can use the keypad key 'raw', for mappings. */
	    if (xim_expected_char != NUL && xim_ignored_char)
		/* We had a keypad key, and XIM tried to thieve it */
		return FALSE;

	    /* Normal processing */
	    return imresult;
	}
    }

    return FALSE;
}

    int
im_get_status(void)
{
    return im_is_active;
}

# else /* !HAVE_GTK2 */

static int	xim_is_active = FALSE;  /* XIM should be active in the current
					   mode */
static int	xim_has_focus = FALSE;	/* XIM is really being used for Vim */
#ifdef FEAT_GUI_X11
static XIMStyle	input_style;
static int	status_area_enabled = TRUE;
#endif

#ifdef FEAT_GUI_GTK
# ifdef WIN3264
#  include <gdk/gdkwin32.h>
# else
#  include <gdk/gdkx.h>
# endif
#else
# ifdef PROTO
/* Define a few things to be able to generate prototypes while not configured
 * for GTK. */
#  define GSList int
#  define gboolean int
   typedef int GdkEvent;
   typedef int GdkEventKey;
#  define GdkIC int
# endif
#endif

#if defined(FEAT_GUI_GTK) || defined(PROTO)
static int	preedit_buf_len = 0;
static int	xim_can_preediting INIT(= FALSE);	/* XIM in showmode() */
static int	xim_input_style;
#ifndef FEAT_GUI_GTK
# define gboolean int
#endif
static gboolean	use_status_area = 0;

static int im_xim_str2keycode __ARGS((unsigned int *code, unsigned int *state));
static void im_xim_send_event_imactivate __ARGS((void));

/*
 * Convert string to keycode and state for XKeyEvent.
 * When string is valid return OK, when invalid return FAIL.
 *
 * See 'imactivatekey' documentation for the format.
 */
    static int
im_xim_str2keycode(code, state)
    unsigned int *code;
    unsigned int *state;
{
    int		retval = OK;
    int		len;
    unsigned	keycode = 0, keystate = 0;
    Window	window;
    Display	*display;
    char_u	*flag_end;
    char_u	*str;

    if (*p_imak != NUL)
    {
	len = STRLEN(p_imak);
	for (flag_end = p_imak + len - 1;
			    flag_end > p_imak && *flag_end != '-'; --flag_end)
	    ;

	/* Parse modifier keys */
	for (str = p_imak; str < flag_end; ++str)
	{
	    switch (*str)
	    {
		case 's': case 'S':
		    keystate |= ShiftMask;
		    break;
		case 'l': case 'L':
		    keystate |= LockMask;
		    break;
		case 'c': case 'C':
		    keystate |= ControlMask;
		    break;
		case '1':
		    keystate |= Mod1Mask;
		    break;
		case '2':
		    keystate |= Mod2Mask;
		    break;
		case '3':
		    keystate |= Mod3Mask;
		    break;
		case '4':
		    keystate |= Mod4Mask;
		    break;
		case '5':
		    keystate |= Mod5Mask;
		    break;
		case '-':
		    break;
		default:
		    retval = FAIL;
	    }
	}
	if (*str == '-')
	    ++str;

	/* Get keycode from string. */
	gui_get_x11_windis(&window, &display);
	if (display)
	    keycode = XKeysymToKeycode(display, XStringToKeysym((char *)str));
	if (keycode == 0)
	    retval = FAIL;

	if (code != NULL)
	    *code = keycode;
	if (state != NULL)
	    *state = keystate;
    }
    return retval;
}

    static void
im_xim_send_event_imactivate()
{
    /* Force turn on preedit state by symulate keypress event.
     * Keycode and state is specified by 'imactivatekey'.
     */
    XKeyEvent ev;

    gui_get_x11_windis(&ev.window, &ev.display);
    ev.root = RootWindow(ev.display, DefaultScreen(ev.display));
    ev.subwindow = None;
    ev.time = CurrentTime;
    ev.x = 1;
    ev.y = 1;
    ev.x_root = 1;
    ev.y_root = 1;
    ev.same_screen = 1;
    ev.type = KeyPress;
    if (im_xim_str2keycode(&ev.keycode, &ev.state) == OK)
	XSendEvent(ev.display, ev.window, 1, KeyPressMask, (XEvent*)&ev);
}

/*
 * Return TRUE if 'imactivatekey' has a valid value.
 */
    int
im_xim_isvalid_imactivate()
{
    return im_xim_str2keycode(NULL, NULL) == OK;
}
#endif /* FEAT_GUI_GTK */

/*
 * Switch using XIM on/off.  This is used by the code that changes "State".
 */
    void
im_set_active(active)
    int		active;
{
    if (xic == NULL)
	return;

    /* If 'imdisable' is set, XIM is never active. */
    if (p_imdisable)
	active = FALSE;
#if !defined (FEAT_GUI_GTK)
    else if (input_style & XIMPreeditPosition)
	/* There is a problem in switching XIM off when preediting is used,
	 * and it is not clear how this can be solved.  For now, keep XIM on
	 * all the time, like it was done in Vim 5.8. */
	active = TRUE;
#endif

    /* Remember the active state, it is needed when Vim gets keyboard focus. */
    xim_is_active = active;

#ifdef FEAT_GUI_GTK
    /* When 'imactivatekey' has valid key-string, try to control XIM preedit
     * state.  When 'imactivatekey' has no or invalid string, try old XIM
     * focus control.
     */
    if (*p_imak != NUL)
    {
	/* BASIC STRATEGY:
	 * Destroy old Input Context (XIC), and create new one.  New XIC
	 * would have a state of preedit that is off.  When argument:active
	 * is false, that's all.  Else argument:active is true, send a key
	 * event specified by 'imactivatekey' to activate XIM preedit state.
	 */

	xim_is_active = TRUE; /* Disable old XIM focus control */
	/* If we can monitor preedit state with preedit callback functions,
	 * try least creation of new XIC.
	 */
	if (xim_input_style & (int)GDK_IM_PREEDIT_CALLBACKS)
	{
	    if (xim_can_preediting && !active)
	    {
		/* Force turn off preedit state.  With some IM
		 * implementations, we cannot turn off preedit state by
		 * symulate keypress event.  It is why using such a method
		 * that destroy old IC (input context), and create new one.
		 * When create new IC, its preedit state is usually off.
		 */
		xim_reset();
		xim_set_focus(FALSE);
		gdk_ic_destroy(xic);
		xim_init();
		xim_can_preediting = FALSE;
	    }
	    else if (!xim_can_preediting && active)
		im_xim_send_event_imactivate();
	}
	else
	{
	    /* First, force destroy old IC, and create new one.  It
	     * symulates "turning off preedit state".
	     */
	    xim_set_focus(FALSE);
	    gdk_ic_destroy(xic);
	    xim_init();
	    xim_can_preediting = FALSE;

	    /* 2nd, when requested to activate IM, symulate this by sending
	     * the event.
	     */
	    if (active)
	    {
		im_xim_send_event_imactivate();
		xim_can_preediting = TRUE;
	    }
	}
    }
    else
    {
# ifndef XIMPreeditUnKnown
	/* X11R5 doesn't have these, it looks safe enough to define here. */
	typedef unsigned long XIMPreeditState;
#  define XIMPreeditUnKnown	0L
#  define XIMPreeditEnable	1L
#  define XIMPreeditDisable	(1L<<1)
#  define XNPreeditState	"preeditState"
# endif
	XIMPreeditState preedit_state = XIMPreeditUnKnown;
	XVaNestedList preedit_attr;
	XIC pxic;

	preedit_attr = XVaCreateNestedList(0,
				XNPreeditState, &preedit_state,
				NULL);
	pxic = ((GdkICPrivate *)xic)->xic;

	if (!XGetICValues(pxic, XNPreeditAttributes, preedit_attr, NULL))
	{
	    XFree(preedit_attr);
	    preedit_attr = XVaCreateNestedList(0,
				XNPreeditState,
				active ? XIMPreeditEnable : XIMPreeditDisable,
				NULL);
	    XSetICValues(pxic, XNPreeditAttributes, preedit_attr, NULL);
	    xim_can_preediting = active;
	    xim_is_active = active;
	}
	XFree(preedit_attr);
    }
    if (xim_input_style & XIMPreeditCallbacks)
    {
	preedit_buf_len = 0;
	init_preedit_start_col();
    }
#else
# if 0
	/* When had tested kinput2 + canna + Athena GUI version with
	 * 'imactivatekey' is "s-space", im_xim_send_event_imactivate() did not
	 * work correctly.  It just inserted one space.  I don't know why we
	 * couldn't switch state of XIM preediting.  This is reason why these
	 * codes are commented out.
	 */
	/* First, force destroy old IC, and create new one.  It symulates
	 * "turning off preedit state".
	 */
	xim_set_focus(FALSE);
	XDestroyIC(xic);
	xic = NULL;
	xim_init();

	/* 2nd, when requested to activate IM, symulate this by sending the
	 * event.
	 */
	if (active)
	    im_xim_send_event_imactivate();
# endif
#endif
    xim_set_preedit();
}

/*
 * Adjust using XIM for gaining or losing keyboard focus.  Also called when
 * "xim_is_active" changes.
 */
    void
xim_set_focus(focus)
    int		focus;
{
    if (xic == NULL)
	return;

    /*
     * XIM only gets focus when the Vim window has keyboard focus and XIM has
     * been set active for the current mode.
     */
    if (focus && xim_is_active)
    {
	if (!xim_has_focus)
	{
	    xim_has_focus = TRUE;
#ifdef FEAT_GUI_GTK
	    gdk_im_begin(xic, gui.drawarea->window);
#else
	    XSetICFocus(xic);
#endif
	}
    }
    else
    {
	if (xim_has_focus)
	{
	    xim_has_focus = FALSE;
#ifdef FEAT_GUI_GTK
	    gdk_im_end();
#else
	    XUnsetICFocus(xic);
#endif
	}
    }
}

/*ARGSUSED*/
    void
im_set_position(row, col)
    int		row;
    int		col;
{
    xim_set_preedit();
}

/*
 * Set the XIM to the current cursor position.
 */
    void
xim_set_preedit()
{
    if (xic == NULL)
	return;

    xim_set_focus(TRUE);

#ifdef FEAT_GUI_GTK
    if (gdk_im_ready())
    {
	int		attrmask;
	GdkICAttr	*attr;

	if (!xic_attr)
	    return;

	attr = xic_attr;
	attrmask = 0;

# ifdef FEAT_XFONTSET
	if ((xim_input_style & (int)GDK_IM_PREEDIT_POSITION)
		&& gui.fontset != NOFONTSET
		&& gui.fontset->type == GDK_FONT_FONTSET)
	{
	    if (!xim_has_focus)
	    {
		if (attr->spot_location.y >= 0)
		{
		    attr->spot_location.x = 0;
		    attr->spot_location.y = -100;
		    attrmask |= (int)GDK_IC_SPOT_LOCATION;
		}
	    }
	    else
	    {
		gint	width, height;

		if (attr->spot_location.x != TEXT_X(gui.col)
		    || attr->spot_location.y != TEXT_Y(gui.row))
		{
		    attr->spot_location.x = TEXT_X(gui.col);
		    attr->spot_location.y = TEXT_Y(gui.row);
		    attrmask |= (int)GDK_IC_SPOT_LOCATION;
		}

		gdk_window_get_size(gui.drawarea->window, &width, &height);
		width -= 2 * gui.border_offset;
		height -= 2 * gui.border_offset;
		if (xim_input_style & (int)GDK_IM_STATUS_AREA)
		    height -= gui.char_height;
		if (attr->preedit_area.width != width
		    || attr->preedit_area.height != height)
		{
		    attr->preedit_area.x = gui.border_offset;
		    attr->preedit_area.y = gui.border_offset;
		    attr->preedit_area.width = width;
		    attr->preedit_area.height = height;
		    attrmask |= (int)GDK_IC_PREEDIT_AREA;
		}

		if (attr->preedit_fontset != gui.current_font)
		{
		    attr->preedit_fontset = gui.current_font;
		    attrmask |= (int)GDK_IC_PREEDIT_FONTSET;
		}
	    }
	}
# endif /* FEAT_XFONTSET */

	if (xim_fg_color == INVALCOLOR)
	{
	    xim_fg_color = gui.def_norm_pixel;
	    xim_bg_color = gui.def_back_pixel;
	}
	if (attr->preedit_foreground.pixel != xim_fg_color)
	{
	    attr->preedit_foreground.pixel = xim_fg_color;
	    attrmask |= (int)GDK_IC_PREEDIT_FOREGROUND;
	}
	if (attr->preedit_background.pixel != xim_bg_color)
	{
	    attr->preedit_background.pixel = xim_bg_color;
	    attrmask |= (int)GDK_IC_PREEDIT_BACKGROUND;
	}

	if (attrmask != 0)
	    gdk_ic_set_attr(xic, attr, (GdkICAttributesType)attrmask);
    }
#else /* FEAT_GUI_GTK */
    {
	XVaNestedList attr_list;
	XRectangle spot_area;
	XPoint over_spot;
	int line_space;

	if (!xim_has_focus)
	{
	    /* hide XIM cursor */
	    over_spot.x = 0;
	    over_spot.y = -100; /* arbitrary invisible position */
	    attr_list = (XVaNestedList) XVaCreateNestedList(0,
							    XNSpotLocation,
							    &over_spot,
							    NULL);
	    XSetICValues(xic, XNPreeditAttributes, attr_list, NULL);
	    XFree(attr_list);
	    return;
	}

	if (input_style & XIMPreeditPosition)
	{
	    if (xim_fg_color == INVALCOLOR)
	    {
		xim_fg_color = gui.def_norm_pixel;
		xim_bg_color = gui.def_back_pixel;
	    }
	    over_spot.x = TEXT_X(gui.col);
	    over_spot.y = TEXT_Y(gui.row);
	    spot_area.x = 0;
	    spot_area.y = 0;
	    spot_area.height = gui.char_height * Rows;
	    spot_area.width  = gui.char_width * Columns;
	    line_space = gui.char_height;
	    attr_list = (XVaNestedList) XVaCreateNestedList(0,
					    XNSpotLocation, &over_spot,
					    XNForeground, (Pixel) xim_fg_color,
					    XNBackground, (Pixel) xim_bg_color,
					    XNArea, &spot_area,
					    XNLineSpace, line_space,
					    NULL);
	    if (XSetICValues(xic, XNPreeditAttributes, attr_list, NULL))
		EMSG(_("E284: Cannot set IC values"));
	    XFree(attr_list);
	}
    }
#endif /* FEAT_GUI_GTK */
}

/*
 * Set up the status area.
 *
 * This should use a separate Widget, but that seems not possible, because
 * preedit_area and status_area should be set to the same window as for the
 * text input.  Unfortunately this means the status area pollutes the text
 * window...
 */
    void
xim_set_status_area()
{
    if (xic == NULL)
	return;

#ifdef FEAT_GUI_GTK
# if defined(FEAT_XFONTSET)
    if (use_status_area)
    {
	GdkICAttr	*attr;
	int		style;
	gint		width, height;
	GtkWidget	*widget;
	int		attrmask;

	if (!xic_attr)
	    return;

	attr = xic_attr;
	attrmask = 0;
	style = (int)gdk_ic_get_style(xic);
	if ((style & (int)GDK_IM_STATUS_MASK) == (int)GDK_IM_STATUS_AREA)
	{
	    if (gui.fontset != NOFONTSET
		    && gui.fontset->type == GDK_FONT_FONTSET)
	    {
		widget = gui.mainwin;
		gdk_window_get_size(widget->window, &width, &height);

		attrmask |= (int)GDK_IC_STATUS_AREA;
		attr->status_area.x = 0;
		attr->status_area.y = height - gui.char_height - 1;
		attr->status_area.width = width;
		attr->status_area.height = gui.char_height;
	    }
	}
	if (attrmask != 0)
	    gdk_ic_set_attr(xic, attr, (GdkICAttributesType)attrmask);
    }
# endif
#else
    {
	XVaNestedList preedit_list = 0, status_list = 0, list = 0;
	XRectangle pre_area, status_area;

	if (input_style & XIMStatusArea)
	{
	    if (input_style & XIMPreeditArea)
	    {
		XRectangle *needed_rect;

		/* to get status_area width */
		status_list = XVaCreateNestedList(0, XNAreaNeeded,
						  &needed_rect, NULL);
		XGetICValues(xic, XNStatusAttributes, status_list, NULL);
		XFree(status_list);

		status_area.width = needed_rect->width;
	    }
	    else
		status_area.width = gui.char_width * Columns;

	    status_area.x = 0;
	    status_area.y = gui.char_height * Rows + gui.border_offset;
	    if (gui.which_scrollbars[SBAR_BOTTOM])
		status_area.y += gui.scrollbar_height;
#ifdef FEAT_MENU
	    if (gui.menu_is_active)
		status_area.y += gui.menu_height;
#endif
	    status_area.height = gui.char_height;
	    status_list = XVaCreateNestedList(0, XNArea, &status_area, NULL);
	}
	else
	{
	    status_area.x = 0;
	    status_area.y = gui.char_height * Rows + gui.border_offset;
	    if (gui.which_scrollbars[SBAR_BOTTOM])
		status_area.y += gui.scrollbar_height;
#ifdef FEAT_MENU
	    if (gui.menu_is_active)
		status_area.y += gui.menu_height;
#endif
	    status_area.width = 0;
	    status_area.height = gui.char_height;
	}

	if (input_style & XIMPreeditArea)   /* off-the-spot */
	{
	    pre_area.x = status_area.x + status_area.width;
	    pre_area.y = gui.char_height * Rows + gui.border_offset;
	    pre_area.width = gui.char_width * Columns - pre_area.x;
	    if (gui.which_scrollbars[SBAR_BOTTOM])
		pre_area.y += gui.scrollbar_height;
#ifdef FEAT_MENU
	    if (gui.menu_is_active)
		pre_area.y += gui.menu_height;
#endif
	    pre_area.height = gui.char_height;
	    preedit_list = XVaCreateNestedList(0, XNArea, &pre_area, NULL);
	}
	else if (input_style & XIMPreeditPosition)   /* over-the-spot */
	{
	    pre_area.x = 0;
	    pre_area.y = 0;
	    pre_area.height = gui.char_height * Rows;
	    pre_area.width = gui.char_width * Columns;
	    preedit_list = XVaCreateNestedList(0, XNArea, &pre_area, NULL);
	}

	if (preedit_list && status_list)
	    list = XVaCreateNestedList(0, XNPreeditAttributes, preedit_list,
				       XNStatusAttributes, status_list, NULL);
	else if (preedit_list)
	    list = XVaCreateNestedList(0, XNPreeditAttributes, preedit_list,
				       NULL);
	else if (status_list)
	    list = XVaCreateNestedList(0, XNStatusAttributes, status_list,
				       NULL);
	else
	    list = NULL;

	if (list)
	{
	    XSetICValues(xic, XNVaNestedList, list, NULL);
	    XFree(list);
	}
	if (status_list)
	    XFree(status_list);
	if (preedit_list)
	    XFree(preedit_list);
    }
#endif
}

#if defined(FEAT_GUI_X11) || defined(FEAT_GUI_GTK)
static char e_xim[] = N_("E285: Failed to create input context");
#endif

#if defined(FEAT_GUI_X11) || defined(PROTO)
# if defined(XtSpecificationRelease) && XtSpecificationRelease >= 6 && !defined(sun)
#  define USE_X11R6_XIM
# endif

static int xim_real_init __ARGS((Window x11_window, Display *x11_display));


#ifdef USE_X11R6_XIM
static void xim_instantiate_cb __ARGS((Display *display, XPointer client_data, XPointer	call_data));
static void xim_destroy_cb __ARGS((XIM im, XPointer client_data, XPointer call_data));

/*ARGSUSED*/
    static void
xim_instantiate_cb(display, client_data, call_data)
    Display	*display;
    XPointer	client_data;
    XPointer	call_data;
{
    Window	x11_window;
    Display	*x11_display;

#ifdef XIM_DEBUG
    xim_log("xim_instantiate_cb()\n");
#endif

    gui_get_x11_windis(&x11_window, &x11_display);
    if (display != x11_display)
	return;

    xim_real_init(x11_window, x11_display);
    gui_set_shellsize(FALSE, FALSE, RESIZE_BOTH);
    if (xic != NULL)
	XUnregisterIMInstantiateCallback(x11_display, NULL, NULL, NULL,
					 xim_instantiate_cb, NULL);
}

/*ARGSUSED*/
    static void
xim_destroy_cb(im, client_data, call_data)
    XIM		im;
    XPointer	client_data;
    XPointer	call_data;
{
    Window	x11_window;
    Display	*x11_display;

#ifdef XIM_DEBUG
    xim_log("xim_destroy_cb()\n");
#endif
    gui_get_x11_windis(&x11_window, &x11_display);

    xic = NULL;
    status_area_enabled = FALSE;

    gui_set_shellsize(FALSE, FALSE, RESIZE_BOTH);

    XRegisterIMInstantiateCallback(x11_display, NULL, NULL, NULL,
				   xim_instantiate_cb, NULL);
}
#endif

    void
xim_init()
{
    Window	x11_window;
    Display	*x11_display;

#ifdef XIM_DEBUG
    xim_log("xim_init()\n");
#endif

    gui_get_x11_windis(&x11_window, &x11_display);

    xic = NULL;

    if (xim_real_init(x11_window, x11_display))
	return;

    gui_set_shellsize(FALSE, FALSE, RESIZE_BOTH);

#ifdef USE_X11R6_XIM
    XRegisterIMInstantiateCallback(x11_display, NULL, NULL, NULL,
				   xim_instantiate_cb, NULL);
#endif
}

    static int
xim_real_init(x11_window, x11_display)
    Window	x11_window;
    Display	*x11_display;
{
    int		i;
    char	*p,
		*s,
		*ns,
		*end,
		tmp[1024];
#define IMLEN_MAX 40
    char	buf[IMLEN_MAX + 7];
    XIM		xim = NULL;
    XIMStyles	*xim_styles;
    XIMStyle	this_input_style = 0;
    Boolean	found;
    XPoint	over_spot;
    XVaNestedList preedit_list, status_list;

    input_style = 0;
    status_area_enabled = FALSE;

    if (xic != NULL)
	return FALSE;

    if (gui.rsrc_input_method != NULL && *gui.rsrc_input_method != NUL)
    {
	strcpy(tmp, gui.rsrc_input_method);
	for (ns = s = tmp; ns != NULL && *s != NUL;)
	{
	    s = (char *)skipwhite((char_u *)s);
	    if (*s == NUL)
		break;
	    if ((ns = end = strchr(s, ',')) == NULL)
		end = s + strlen(s);
	    while (isspace(((char_u *)end)[-1]))
		end--;
	    *end = NUL;

	    if (strlen(s) <= IMLEN_MAX)
	    {
		strcpy(buf, "@im=");
		strcat(buf, s);
		if ((p = XSetLocaleModifiers(buf)) != NULL && *p != NUL
			&& (xim = XOpenIM(x11_display, NULL, NULL, NULL))
								      != NULL)
		    break;
	    }

	    s = ns + 1;
	}
    }

    if (xim == NULL && (p = XSetLocaleModifiers("")) != NULL && *p != NUL)
	xim = XOpenIM(x11_display, NULL, NULL, NULL);

    /* This is supposed to be useful to obtain characters through
     * XmbLookupString() without really using a XIM. */
    if (xim == NULL && (p = XSetLocaleModifiers("@im=none")) != NULL
								 && *p != NUL)
	xim = XOpenIM(x11_display, NULL, NULL, NULL);

    if (xim == NULL)
    {
	/* Only give this message when verbose is set, because too many people
	 * got this message when they didn't want to use a XIM. */
	if (p_verbose > 0)
	{
	    verbose_enter();
	    EMSG(_("E286: Failed to open input method"));
	    verbose_leave();
	}
	return FALSE;
    }

#ifdef USE_X11R6_XIM
    {
	XIMCallback destroy_cb;

	destroy_cb.callback = xim_destroy_cb;
	destroy_cb.client_data = NULL;
	if (XSetIMValues(xim, XNDestroyCallback, &destroy_cb, NULL))
	    EMSG(_("E287: Warning: Could not set destroy callback to IM"));
    }
#endif

    if (XGetIMValues(xim, XNQueryInputStyle, &xim_styles, NULL) || !xim_styles)
    {
	EMSG(_("E288: input method doesn't support any style"));
	XCloseIM(xim);
	return FALSE;
    }

    found = False;
    strcpy(tmp, gui.rsrc_preedit_type_name);
    for (s = tmp; s && !found; )
    {
	while (*s && isspace((unsigned char)*s))
	    s++;
	if (!*s)
	    break;
	if ((ns = end = strchr(s, ',')) != 0)
	    ns++;
	else
	    end = s + strlen(s);
	while (isspace((unsigned char)*end))
	    end--;
	*end = '\0';

	if (!strcmp(s, "OverTheSpot"))
	    this_input_style = (XIMPreeditPosition | XIMStatusArea);
	else if (!strcmp(s, "OffTheSpot"))
	    this_input_style = (XIMPreeditArea | XIMStatusArea);
	else if (!strcmp(s, "Root"))
	    this_input_style = (XIMPreeditNothing | XIMStatusNothing);

	for (i = 0; (unsigned short)i < xim_styles->count_styles; i++)
	{
	    if (this_input_style == xim_styles->supported_styles[i])
	    {
		found = True;
		break;
	    }
	}
	if (!found)
	    for (i = 0; (unsigned short)i < xim_styles->count_styles; i++)
	    {
		if ((xim_styles->supported_styles[i] & this_input_style)
			== (this_input_style & ~XIMStatusArea))
		{
		    this_input_style &= ~XIMStatusArea;
		    found = True;
		    break;
		}
	    }

	s = ns;
    }
    XFree(xim_styles);

    if (!found)
    {
	/* Only give this message when verbose is set, because too many people
	 * got this message when they didn't want to use a XIM. */
	if (p_verbose > 0)
	{
	    verbose_enter();
	    EMSG(_("E289: input method doesn't support my preedit type"));
	    verbose_leave();
	}
	XCloseIM(xim);
	return FALSE;
    }

    over_spot.x = TEXT_X(gui.col);
    over_spot.y = TEXT_Y(gui.row);
    input_style = this_input_style;

    /* A crash was reported when trying to pass gui.norm_font as XNFontSet,
     * thus that has been removed.  Hopefully the default works... */
#ifdef FEAT_XFONTSET
    if (gui.fontset != NOFONTSET)
    {
	preedit_list = XVaCreateNestedList(0,
				XNSpotLocation, &over_spot,
				XNForeground, (Pixel)gui.def_norm_pixel,
				XNBackground, (Pixel)gui.def_back_pixel,
				XNFontSet, (XFontSet)gui.fontset,
				NULL);
	status_list = XVaCreateNestedList(0,
				XNForeground, (Pixel)gui.def_norm_pixel,
				XNBackground, (Pixel)gui.def_back_pixel,
				XNFontSet, (XFontSet)gui.fontset,
				NULL);
    }
    else
#endif
    {
	preedit_list = XVaCreateNestedList(0,
				XNSpotLocation, &over_spot,
				XNForeground, (Pixel)gui.def_norm_pixel,
				XNBackground, (Pixel)gui.def_back_pixel,
				NULL);
	status_list = XVaCreateNestedList(0,
				XNForeground, (Pixel)gui.def_norm_pixel,
				XNBackground, (Pixel)gui.def_back_pixel,
				NULL);
    }

    xic = XCreateIC(xim,
		    XNInputStyle, input_style,
		    XNClientWindow, x11_window,
		    XNFocusWindow, gui.wid,
		    XNPreeditAttributes, preedit_list,
		    XNStatusAttributes, status_list,
		    NULL);
    XFree(status_list);
    XFree(preedit_list);
    if (xic != NULL)
    {
	if (input_style & XIMStatusArea)
	{
	    xim_set_status_area();
	    status_area_enabled = TRUE;
	}
	else
	    gui_set_shellsize(FALSE, FALSE, RESIZE_BOTH);
    }
    else
    {
	EMSG(_(e_xim));
	XCloseIM(xim);
	return FALSE;
    }

    return TRUE;
}

#endif /* FEAT_GUI_X11 */

#if defined(FEAT_GUI_GTK) || defined(PROTO)

# ifdef FEAT_XFONTSET
static char e_overthespot[] = N_("E290: over-the-spot style requires fontset");
# endif

# ifdef PROTO
typedef int GdkIC;
# endif

    void
xim_decide_input_style()
{
    /* GDK_IM_STATUS_CALLBACKS was disabled, enabled it to allow Japanese
     * OverTheSpot. */
    int supported_style = (int)GDK_IM_PREEDIT_NONE |
				 (int)GDK_IM_PREEDIT_NOTHING |
				 (int)GDK_IM_PREEDIT_POSITION |
				 (int)GDK_IM_PREEDIT_CALLBACKS |
				 (int)GDK_IM_STATUS_CALLBACKS |
				 (int)GDK_IM_STATUS_AREA |
				 (int)GDK_IM_STATUS_NONE |
				 (int)GDK_IM_STATUS_NOTHING;

#ifdef XIM_DEBUG
    xim_log("xim_decide_input_style()\n");
#endif

    if (!gdk_im_ready())
	xim_input_style = 0;
    else
    {
	if (gtk_major_version > 1
		|| (gtk_major_version == 1
		    && (gtk_minor_version > 2
			|| (gtk_minor_version == 2 && gtk_micro_version >= 3))))
	    use_status_area = TRUE;
	else
	{
	    EMSG(_("E291: Your GTK+ is older than 1.2.3. Status area disabled"));
	    use_status_area = FALSE;
	}
#ifdef FEAT_XFONTSET
	if (gui.fontset == NOFONTSET || gui.fontset->type != GDK_FONT_FONTSET)
#endif
	    supported_style &= ~((int)GDK_IM_PREEDIT_POSITION
						   | (int)GDK_IM_STATUS_AREA);
	if (!use_status_area)
	    supported_style &= ~(int)GDK_IM_STATUS_AREA;
	xim_input_style = (int)gdk_im_decide_style((GdkIMStyle)supported_style);
    }
}

/*ARGSUSED*/
    static void
preedit_start_cbproc(XIC thexic, XPointer client_data, XPointer call_data)
{
#ifdef XIM_DEBUG
    xim_log("xim_decide_input_style()\n");
#endif

    draw_feedback = NULL;
    xim_can_preediting = TRUE;
    xim_has_preediting = TRUE;
    gui_update_cursor(TRUE, FALSE);
    if (showmode() > 0)
    {
	setcursor();
	out_flush();
    }
}

    static void
xim_back_delete(int n)
{
    char_u str[3];

    str[0] = CSI;
    str[1] = 'k';
    str[2] = 'b';
    while (n-- > 0)
	add_to_input_buf(str, 3);
}

static GSList *key_press_event_queue = NULL;
static gboolean processing_queued_event = FALSE;

/*ARGSUSED*/
    static void
preedit_draw_cbproc(XIC thexic, XPointer client_data, XPointer call_data)
{
    XIMPreeditDrawCallbackStruct *draw_data;
    XIMText	*text;
    char	*src;
    GSList	*event_queue;

#ifdef XIM_DEBUG
    xim_log("preedit_draw_cbproc()\n");
#endif

    draw_data = (XIMPreeditDrawCallbackStruct *) call_data;
    text = (XIMText *) draw_data->text;

    if ((text == NULL && draw_data->chg_length == preedit_buf_len)
						      || preedit_buf_len == 0)
    {
	init_preedit_start_col();
	vim_free(draw_feedback);
	draw_feedback = NULL;
    }
    if (draw_data->chg_length > 0)
    {
	int bs_cnt;

	if (draw_data->chg_length > preedit_buf_len)
	    bs_cnt = preedit_buf_len;
	else
	    bs_cnt = draw_data->chg_length;
	xim_back_delete(bs_cnt);
	preedit_buf_len -= bs_cnt;
    }
    if (text != NULL)
    {
	int		len;
#ifdef FEAT_MBYTE
	char_u		*buf = NULL;
	unsigned int	nfeedback = 0;
#endif
	char_u		*ptr;

	src = text->string.multi_byte;
	if (src != NULL && !text->encoding_is_wchar)
	{
	    len = strlen(src);
	    ptr = (char_u *)src;
	    /* Avoid the enter for decision */
	    if (*ptr == '\n')
		return;

#ifdef FEAT_MBYTE
	    if (input_conv.vc_type != CONV_NONE
		    && (buf = string_convert(&input_conv,
						 (char_u *)src, &len)) != NULL)
	    {
		/* Converted from 'termencoding' to 'encoding'. */
		add_to_input_buf_csi(buf, len);
		ptr = buf;
	    }
	    else
#endif
		add_to_input_buf_csi((char_u *)src, len);
	    /* Add count of character to preedit_buf_len  */
	    while (*ptr != NUL)
	    {
#ifdef FEAT_MBYTE
		if (draw_data->text->feedback != NULL)
		{
		    if (draw_feedback == NULL)
			draw_feedback = (char *)alloc(draw_data->chg_first
							      + text->length);
		    else
			draw_feedback = vim_realloc(draw_feedback,
					 draw_data->chg_first + text->length);
		    if (draw_feedback != NULL)
		    {
			draw_feedback[nfeedback + draw_data->chg_first]
				       = draw_data->text->feedback[nfeedback];
			nfeedback++;
		    }
		}
		if (has_mbyte)
		    ptr += (*mb_ptr2len)(ptr);
		else
#endif
		    ptr++;
		preedit_buf_len++;
	    }
#ifdef FEAT_MBYTE
	    vim_free(buf);
#endif
	    preedit_end_col = MAXCOL;
	}
    }
    if (text != NULL || draw_data->chg_length > 0)
    {
	event_queue = key_press_event_queue;
	processing_queued_event = TRUE;
	while (event_queue != NULL && processing_queued_event)
	{
	    GdkEvent *ev = event_queue->data;

	    gboolean *ret;
	    gtk_signal_emit_by_name((GtkObject*)gui.mainwin, "key_press_event",
								    ev, &ret);
	    gdk_event_free(ev);
	    event_queue = event_queue->next;
	}
	processing_queued_event = FALSE;
	if (key_press_event_queue)
	{
	    g_slist_free(key_press_event_queue);
	    key_press_event_queue = NULL;
	}
    }
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*
 * Retrieve the highlighting attributes at column col in the preedit string.
 * Return -1 if not in preediting mode or if col is out of range.
 */
    int
im_get_feedback_attr(int col)
{
    if (draw_feedback != NULL && col < preedit_buf_len)
    {
	if (draw_feedback[col] & XIMReverse)
	    return HL_INVERSE;
	else if (draw_feedback[col] & XIMUnderline)
	    return HL_UNDERLINE;
	else
	    return hl_attr(HLF_V);
    }

    return -1;
}

/*ARGSUSED*/
    static void
preedit_caret_cbproc(XIC thexic, XPointer client_data, XPointer call_data)
{
#ifdef XIM_DEBUG
    xim_log("preedit_caret_cbproc()\n");
#endif
}

/*ARGSUSED*/
    static void
preedit_done_cbproc(XIC thexic, XPointer client_data, XPointer call_data)
{
#ifdef XIM_DEBUG
    xim_log("preedit_done_cbproc()\n");
#endif

    vim_free(draw_feedback);
    draw_feedback = NULL;
    xim_can_preediting = FALSE;
    xim_has_preediting = FALSE;
    gui_update_cursor(TRUE, FALSE);
    if (showmode() > 0)
    {
	setcursor();
	out_flush();
    }
}

    void
xim_reset(void)
{
    char *text;

#ifdef XIM_DEBUG
    xim_log("xim_reset()\n");
#endif

    if (xic != NULL)
    {
	text = XmbResetIC(((GdkICPrivate *)xic)->xic);
	if (text != NULL && !(xim_input_style & (int)GDK_IM_PREEDIT_CALLBACKS))
	    add_to_input_buf_csi((char_u *)text, strlen(text));
	else
	    preedit_buf_len = 0;
	if (text != NULL)
	    XFree(text);
    }
}

/*ARGSUSED*/
    int
xim_queue_key_press_event(GdkEventKey *event, int down)
{
#ifdef XIM_DEBUG
    xim_log("xim_queue_key_press_event()\n");
#endif

    if (preedit_buf_len <= 0)
	return FALSE;
    if (processing_queued_event)
	processing_queued_event = FALSE;

    key_press_event_queue = g_slist_append(key_press_event_queue,
					   gdk_event_copy((GdkEvent *)event));
    return TRUE;
}

/*ARGSUSED*/
    static void
preedit_callback_setup(GdkIC *ic)
{
    XIC xxic;
    XVaNestedList preedit_attr;
    XIMCallback preedit_start_cb;
    XIMCallback preedit_draw_cb;
    XIMCallback preedit_caret_cb;
    XIMCallback preedit_done_cb;

    xxic = ((GdkICPrivate*)xic)->xic;
    preedit_start_cb.callback = (XIMProc)preedit_start_cbproc;
    preedit_draw_cb.callback = (XIMProc)preedit_draw_cbproc;
    preedit_caret_cb.callback = (XIMProc)preedit_caret_cbproc;
    preedit_done_cb.callback = (XIMProc)preedit_done_cbproc;
    preedit_attr
	 = XVaCreateNestedList(0,
			       XNPreeditStartCallback, &preedit_start_cb,
			       XNPreeditDrawCallback, &preedit_draw_cb,
			       XNPreeditCaretCallback, &preedit_caret_cb,
			       XNPreeditDoneCallback, &preedit_done_cb,
			       NULL);
    XSetICValues(xxic, XNPreeditAttributes, preedit_attr, NULL);
    XFree(preedit_attr);
}

/*ARGSUSED*/
    static void
reset_state_setup(GdkIC *ic)
{
#ifdef USE_X11R6_XIM
    /* don't change the input context when we call reset */
    XSetICValues(((GdkICPrivate *)ic)->xic, XNResetState, XIMPreserveState,
									NULL);
#endif
}

    void
xim_init(void)
{
#ifdef XIM_DEBUG
    xim_log("xim_init()\n");
#endif

    xic = NULL;
    xic_attr = NULL;

    if (!gdk_im_ready())
    {
	if (p_verbose > 0)
	{
	    verbose_enter();
	    EMSG(_("E292: Input Method Server is not running"));
	    verbose_leave();
	}
	return;
    }
    if ((xic_attr = gdk_ic_attr_new()) != NULL)
    {
#ifdef FEAT_XFONTSET
	gint width, height;
#endif
	int		mask;
	GdkColormap	*colormap;
	GdkICAttr	*attr = xic_attr;
	int		attrmask = (int)GDK_IC_ALL_REQ;
	GtkWidget	*widget = gui.drawarea;

	attr->style = (GdkIMStyle)xim_input_style;
	attr->client_window = gui.mainwin->window;

	if ((colormap = gtk_widget_get_colormap(widget)) !=
	    gtk_widget_get_default_colormap())
	{
	    attrmask |= (int)GDK_IC_PREEDIT_COLORMAP;
	    attr->preedit_colormap = colormap;
	}
	attrmask |= (int)GDK_IC_PREEDIT_FOREGROUND;
	attrmask |= (int)GDK_IC_PREEDIT_BACKGROUND;
	attr->preedit_foreground = widget->style->fg[GTK_STATE_NORMAL];
	attr->preedit_background = widget->style->base[GTK_STATE_NORMAL];

#ifdef FEAT_XFONTSET
	if ((xim_input_style & (int)GDK_IM_PREEDIT_MASK)
					      == (int)GDK_IM_PREEDIT_POSITION)
	{
	    if (gui.fontset == NOFONTSET
		    || gui.fontset->type != GDK_FONT_FONTSET)
	    {
		EMSG(_(e_overthespot));
	    }
	    else
	    {
		gdk_window_get_size(widget->window, &width, &height);

		attrmask |= (int)GDK_IC_PREEDIT_POSITION_REQ;
		attr->spot_location.x = TEXT_X(0);
		attr->spot_location.y = TEXT_Y(0);
		attr->preedit_area.x = gui.border_offset;
		attr->preedit_area.y = gui.border_offset;
		attr->preedit_area.width = width - 2*gui.border_offset;
		attr->preedit_area.height = height - 2*gui.border_offset;
		attr->preedit_fontset = gui.fontset;
	    }
	}

	if ((xim_input_style & (int)GDK_IM_STATUS_MASK)
						   == (int)GDK_IM_STATUS_AREA)
	{
	    if (gui.fontset == NOFONTSET
		    || gui.fontset->type != GDK_FONT_FONTSET)
	    {
		EMSG(_(e_overthespot));
	    }
	    else
	    {
		gdk_window_get_size(gui.mainwin->window, &width, &height);
		attrmask |= (int)GDK_IC_STATUS_AREA_REQ;
		attr->status_area.x = 0;
		attr->status_area.y = height - gui.char_height - 1;
		attr->status_area.width = width;
		attr->status_area.height = gui.char_height;
		attr->status_fontset = gui.fontset;
	    }
	}
	else if ((xim_input_style & (int)GDK_IM_STATUS_MASK)
					      == (int)GDK_IM_STATUS_CALLBACKS)
	{
	    /* FIXME */
	}
#endif

	xic = gdk_ic_new(attr, (GdkICAttributesType)attrmask);

	if (xic == NULL)
	    EMSG(_(e_xim));
	else
	{
	    mask = (int)gdk_window_get_events(widget->window);
	    mask |= (int)gdk_ic_get_events(xic);
	    gdk_window_set_events(widget->window, (GdkEventMask)mask);
	    if (xim_input_style & (int)GDK_IM_PREEDIT_CALLBACKS)
		preedit_callback_setup(xic);
	    reset_state_setup(xic);
	}
    }
}

    void
im_shutdown(void)
{
#ifdef XIM_DEBUG
    xim_log("im_shutdown()\n");
#endif

    if (xic != NULL)
    {
	gdk_im_end();
	gdk_ic_destroy(xic);
	xic = NULL;
    }
    xim_is_active = FALSE;
    xim_can_preediting = FALSE;
    preedit_start_col = MAXCOL;
    xim_has_preediting = FALSE;
}

#endif /* FEAT_GUI_GTK */

    int
xim_get_status_area_height()
{
#ifdef FEAT_GUI_GTK
    if (xim_input_style & (int)GDK_IM_STATUS_AREA)
	return gui.char_height;
#else
    if (status_area_enabled)
	return gui.char_height;
#endif
    return 0;
}

/*
 * Get IM status.  When IM is on, return TRUE.  Else return FALSE.
 * FIXME: This doesn't work correctly: Having focus doesn't always mean XIM is
 * active, when not having focus XIM may still be active (e.g., when using a
 * tear-off menu item).
 */
    int
im_get_status()
{
#  ifdef FEAT_GUI_GTK
    if (xim_input_style & (int)GDK_IM_PREEDIT_CALLBACKS)
	return xim_can_preediting;
#  endif
    return xim_has_focus;
}

# endif /* !HAVE_GTK2 */

# if defined(HAVE_GTK2) || defined(PROTO)
    int
preedit_get_status(void)
{
    return preedit_is_active;
}
# endif

# if defined(FEAT_GUI_GTK) || defined(PROTO)
    int
im_is_preediting()
{
    return xim_has_preediting;
}
# endif
#endif /* FEAT_XIM */

#if defined(FEAT_MBYTE) || defined(PROTO)

/*
 * Setup "vcp" for conversion from "from" to "to".
 * The names must have been made canonical with enc_canonize().
 * vcp->vc_type must have been initialized to CONV_NONE.
 * Note: cannot be used for conversion from/to ucs-2 and ucs-4 (will use utf-8
 * instead).
 * Afterwards invoke with "from" and "to" equal to NULL to cleanup.
 * Return FAIL when conversion is not supported, OK otherwise.
 */
    int
convert_setup(vcp, from, to)
    vimconv_T	*vcp;
    char_u	*from;
    char_u	*to;
{
    int		from_prop;
    int		to_prop;

    /* Reset to no conversion. */
# ifdef USE_ICONV
    if (vcp->vc_type == CONV_ICONV && vcp->vc_fd != (iconv_t)-1)
	iconv_close(vcp->vc_fd);
# endif
    vcp->vc_type = CONV_NONE;
    vcp->vc_factor = 1;
    vcp->vc_fail = FALSE;

    /* No conversion when one of the names is empty or they are equal. */
    if (from == NULL || *from == NUL || to == NULL || *to == NUL
						     || STRCMP(from, to) == 0)
	return OK;

    from_prop = enc_canon_props(from);
    to_prop = enc_canon_props(to);
    if ((from_prop & ENC_LATIN1) && (to_prop & ENC_UNICODE))
    {
	/* Internal latin1 -> utf-8 conversion. */
	vcp->vc_type = CONV_TO_UTF8;
	vcp->vc_factor = 2;	/* up to twice as long */
    }
    else if ((from_prop & ENC_LATIN9) && (to_prop & ENC_UNICODE))
    {
	/* Internal latin9 -> utf-8 conversion. */
	vcp->vc_type = CONV_9_TO_UTF8;
	vcp->vc_factor = 3;	/* up to three as long (euro sign) */
    }
    else if ((from_prop & ENC_UNICODE) && (to_prop & ENC_LATIN1))
    {
	/* Internal utf-8 -> latin1 conversion. */
	vcp->vc_type = CONV_TO_LATIN1;
    }
    else if ((from_prop & ENC_UNICODE) && (to_prop & ENC_LATIN9))
    {
	/* Internal utf-8 -> latin9 conversion. */
	vcp->vc_type = CONV_TO_LATIN9;
    }
#ifdef WIN3264
    /* Win32-specific codepage <-> codepage conversion without iconv. */
    else if (((from_prop & ENC_UNICODE) || encname2codepage(from) > 0)
	    && ((to_prop & ENC_UNICODE) || encname2codepage(to) > 0))
    {
	vcp->vc_type = CONV_CODEPAGE;
	vcp->vc_factor = 2;	/* up to twice as long */
	vcp->vc_cpfrom = (from_prop & ENC_UNICODE) ? 0 : encname2codepage(from);
	vcp->vc_cpto = (to_prop & ENC_UNICODE) ? 0 : encname2codepage(to);
    }
#endif
#ifdef MACOS_X
    else if ((from_prop & ENC_MACROMAN) && (to_prop & ENC_LATIN1))
    {
	vcp->vc_type = CONV_MAC_LATIN1;
    }
    else if ((from_prop & ENC_MACROMAN) && (to_prop & ENC_UNICODE))
    {
	vcp->vc_type = CONV_MAC_UTF8;
	vcp->vc_factor = 2;	/* up to twice as long */
    }
    else if ((from_prop & ENC_LATIN1) && (to_prop & ENC_MACROMAN))
    {
	vcp->vc_type = CONV_LATIN1_MAC;
    }
    else if ((from_prop & ENC_UNICODE) && (to_prop & ENC_MACROMAN))
    {
	vcp->vc_type = CONV_UTF8_MAC;
    }
#endif
# ifdef USE_ICONV
    else
    {
	/* Use iconv() for conversion. */
	vcp->vc_fd = (iconv_t)my_iconv_open(
		(to_prop & ENC_UNICODE) ? (char_u *)"utf-8" : to,
		(from_prop & ENC_UNICODE) ? (char_u *)"utf-8" : from);
	if (vcp->vc_fd != (iconv_t)-1)
	{
	    vcp->vc_type = CONV_ICONV;
	    vcp->vc_factor = 4;	/* could be longer too... */
	}
    }
# endif
    if (vcp->vc_type == CONV_NONE)
	return FAIL;

    return OK;
}

#if defined(FEAT_GUI) || defined(AMIGA) || defined(WIN3264) \
	|| defined(MSDOS) || defined(PROTO)
/*
 * Do conversion on typed input characters in-place.
 * The input and output are not NUL terminated!
 * Returns the length after conversion.
 */
    int
convert_input(ptr, len, maxlen)
    char_u	*ptr;
    int		len;
    int		maxlen;
{
    return convert_input_safe(ptr, len, maxlen, NULL, NULL);
}
#endif

/*
 * Like convert_input(), but when there is an incomplete byte sequence at the
 * end return that as an allocated string in "restp" and set "*restlenp" to
 * the length.  If "restp" is NULL it is not used.
 */
    int
convert_input_safe(ptr, len, maxlen, restp, restlenp)
    char_u	*ptr;
    int		len;
    int		maxlen;
    char_u	**restp;
    int		*restlenp;
{
    char_u	*d;
    int		dlen = len;
    int		unconvertlen = 0;

    d = string_convert_ext(&input_conv, ptr, &dlen,
					restp == NULL ? NULL : &unconvertlen);
    if (d != NULL)
    {
	if (dlen <= maxlen)
	{
	    if (unconvertlen > 0)
	    {
		/* Move the unconverted characters to allocated memory. */
		*restp = alloc(unconvertlen);
		if (*restp != NULL)
		    mch_memmove(*restp, ptr + len - unconvertlen, unconvertlen);
		*restlenp = unconvertlen;
	    }
	    mch_memmove(ptr, d, dlen);
	}
	else
	    /* result is too long, keep the unconverted text (the caller must
	     * have done something wrong!) */
	    dlen = len;
	vim_free(d);
    }
    return dlen;
}

/*
 * Convert text "ptr[*lenp]" according to "vcp".
 * Returns the result in allocated memory and sets "*lenp".
 * When "lenp" is NULL, use NUL terminated strings.
 * Illegal chars are often changed to "?", unless vcp->vc_fail is set.
 * When something goes wrong, NULL is returned and "*lenp" is unchanged.
 */
    char_u *
string_convert(vcp, ptr, lenp)
    vimconv_T	*vcp;
    char_u	*ptr;
    int		*lenp;
{
    return string_convert_ext(vcp, ptr, lenp, NULL);
}

/*
 * Like string_convert(), but when "unconvlenp" is not NULL and there are is
 * an incomplete sequence at the end it is not converted and "*unconvlenp" is
 * set to the number of remaining bytes.
 */
    char_u *
string_convert_ext(vcp, ptr, lenp, unconvlenp)
    vimconv_T	*vcp;
    char_u	*ptr;
    int		*lenp;
    int		*unconvlenp;
{
    char_u	*retval = NULL;
    char_u	*d;
    int		len;
    int		i;
    int		l;
    int		c;

    if (lenp == NULL)
	len = (int)STRLEN(ptr);
    else
	len = *lenp;
    if (len == 0)
	return vim_strsave((char_u *)"");

    switch (vcp->vc_type)
    {
	case CONV_TO_UTF8:	/* latin1 to utf-8 conversion */
	    retval = alloc(len * 2 + 1);
	    if (retval == NULL)
		break;
	    d = retval;
	    for (i = 0; i < len; ++i)
	    {
		c = ptr[i];
		if (c < 0x80)
		    *d++ = c;
		else
		{
		    *d++ = 0xc0 + ((unsigned)c >> 6);
		    *d++ = 0x80 + (c & 0x3f);
		}
	    }
	    *d = NUL;
	    if (lenp != NULL)
		*lenp = (int)(d - retval);
	    break;

	case CONV_9_TO_UTF8:	/* latin9 to utf-8 conversion */
	    retval = alloc(len * 3 + 1);
	    if (retval == NULL)
		break;
	    d = retval;
	    for (i = 0; i < len; ++i)
	    {
		c = ptr[i];
		switch (c)
		{
		    case 0xa4: c = 0x20ac; break;   /* euro */
		    case 0xa6: c = 0x0160; break;   /* S hat */
		    case 0xa8: c = 0x0161; break;   /* S -hat */
		    case 0xb4: c = 0x017d; break;   /* Z hat */
		    case 0xb8: c = 0x017e; break;   /* Z -hat */
		    case 0xbc: c = 0x0152; break;   /* OE */
		    case 0xbd: c = 0x0153; break;   /* oe */
		    case 0xbe: c = 0x0178; break;   /* Y */
		}
		d += utf_char2bytes(c, d);
	    }
	    *d = NUL;
	    if (lenp != NULL)
		*lenp = (int)(d - retval);
	    break;

	case CONV_TO_LATIN1:	/* utf-8 to latin1 conversion */
	case CONV_TO_LATIN9:	/* utf-8 to latin9 conversion */
	    retval = alloc(len + 1);
	    if (retval == NULL)
		break;
	    d = retval;
	    for (i = 0; i < len; ++i)
	    {
		l = utf_ptr2len(ptr + i);
		if (l == 0)
		    *d++ = NUL;
		else if (l == 1)
		{
		    if (unconvlenp != NULL && utf8len_tab[ptr[i]] > len - i)
		    {
			/* Incomplete sequence at the end. */
			*unconvlenp = len - i;
			break;
		    }
		    *d++ = ptr[i];
		}
		else
		{
		    c = utf_ptr2char(ptr + i);
		    if (vcp->vc_type == CONV_TO_LATIN9)
			switch (c)
			{
			    case 0x20ac: c = 0xa4; break;   /* euro */
			    case 0x0160: c = 0xa6; break;   /* S hat */
			    case 0x0161: c = 0xa8; break;   /* S -hat */
			    case 0x017d: c = 0xb4; break;   /* Z hat */
			    case 0x017e: c = 0xb8; break;   /* Z -hat */
			    case 0x0152: c = 0xbc; break;   /* OE */
			    case 0x0153: c = 0xbd; break;   /* oe */
			    case 0x0178: c = 0xbe; break;   /* Y */
			    case 0xa4:
			    case 0xa6:
			    case 0xa8:
			    case 0xb4:
			    case 0xb8:
			    case 0xbc:
			    case 0xbd:
			    case 0xbe: c = 0x100; break; /* not in latin9 */
			}
		    if (!utf_iscomposing(c))	/* skip composing chars */
		    {
			if (c < 0x100)
			    *d++ = c;
			else if (vcp->vc_fail)
			{
			    vim_free(retval);
			    return NULL;
			}
			else
			{
			    *d++ = 0xbf;
			    if (utf_char2cells(c) > 1)
				*d++ = '?';
			}
		    }
		    i += l - 1;
		}
	    }
	    *d = NUL;
	    if (lenp != NULL)
		*lenp = (int)(d - retval);
	    break;

# ifdef MACOS_CONVERT
	case CONV_MAC_LATIN1:
	    retval = mac_string_convert(ptr, len, lenp, vcp->vc_fail,
					'm', 'l', unconvlenp);
	    break;

	case CONV_LATIN1_MAC:
	    retval = mac_string_convert(ptr, len, lenp, vcp->vc_fail,
					'l', 'm', unconvlenp);
	    break;

	case CONV_MAC_UTF8:
	    retval = mac_string_convert(ptr, len, lenp, vcp->vc_fail,
					'm', 'u', unconvlenp);
	    break;

	case CONV_UTF8_MAC:
	    retval = mac_string_convert(ptr, len, lenp, vcp->vc_fail,
					'u', 'm', unconvlenp);
	    break;
# endif

# ifdef USE_ICONV
	case CONV_ICONV:	/* conversion with output_conv.vc_fd */
	    retval = iconv_string(vcp, ptr, len, unconvlenp);
	    if (retval != NULL && lenp != NULL)
		*lenp = (int)STRLEN(retval);
	    break;
# endif
# ifdef WIN3264
	case CONV_CODEPAGE:		/* codepage -> codepage */
	{
	    int		retlen;
	    int		tmp_len;
	    short_u	*tmp;

	    /* 1. codepage/UTF-8  ->  ucs-2. */
	    if (vcp->vc_cpfrom == 0)
		tmp_len = utf8_to_utf16(ptr, len, NULL, NULL);
	    else
		tmp_len = MultiByteToWideChar(vcp->vc_cpfrom, 0,
							      ptr, len, 0, 0);
	    tmp = (short_u *)alloc(sizeof(short_u) * tmp_len);
	    if (tmp == NULL)
		break;
	    if (vcp->vc_cpfrom == 0)
		utf8_to_utf16(ptr, len, tmp, unconvlenp);
	    else
		MultiByteToWideChar(vcp->vc_cpfrom, 0, ptr, len, tmp, tmp_len);

	    /* 2. ucs-2  ->  codepage/UTF-8. */
	    if (vcp->vc_cpto == 0)
		retlen = utf16_to_utf8(tmp, tmp_len, NULL);
	    else
		retlen = WideCharToMultiByte(vcp->vc_cpto, 0,
						    tmp, tmp_len, 0, 0, 0, 0);
	    retval = alloc(retlen + 1);
	    if (retval != NULL)
	    {
		if (vcp->vc_cpto == 0)
		    utf16_to_utf8(tmp, tmp_len, retval);
		else
		    WideCharToMultiByte(vcp->vc_cpto, 0,
					  tmp, tmp_len, retval, retlen, 0, 0);
		retval[retlen] = NUL;
		if (lenp != NULL)
		    *lenp = retlen;
	    }
	    vim_free(tmp);
	    break;
	}
# endif
    }

    return retval;
}
#endif
