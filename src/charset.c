/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#ifdef FEAT_LINEBREAK
static int win_chartabsize __ARGS((win_T *wp, char_u *p, colnr_T col));
#endif

#ifdef FEAT_MBYTE
# if defined(HAVE_WCHAR_H)
#  include <wchar.h>	    /* for towupper() and towlower() */
# endif
static int win_nolbr_chartabsize __ARGS((win_T *wp, char_u *s, colnr_T col, int *headp));
#endif

static unsigned nr2hex __ARGS((unsigned c));

static int    chartab_initialized = FALSE;

/* b_chartab[] is an array of 32 bytes, each bit representing one of the
 * characters 0-255. */
#define SET_CHARTAB(buf, c) (buf)->b_chartab[(unsigned)(c) >> 3] |= (1 << ((c) & 0x7))
#define RESET_CHARTAB(buf, c) (buf)->b_chartab[(unsigned)(c) >> 3] &= ~(1 << ((c) & 0x7))
#define GET_CHARTAB(buf, c) ((buf)->b_chartab[(unsigned)(c) >> 3] & (1 << ((c) & 0x7)))

/*
 * Fill chartab[].  Also fills curbuf->b_chartab[] with flags for keyword
 * characters for current buffer.
 *
 * Depends on the option settings 'iskeyword', 'isident', 'isfname',
 * 'isprint' and 'encoding'.
 *
 * The index in chartab[] depends on 'encoding':
 * - For non-multi-byte index with the byte (same as the character).
 * - For DBCS index with the first byte.
 * - For UTF-8 index with the character (when first byte is up to 0x80 it is
 *   the same as the character, if the first byte is 0x80 and above it depends
 *   on further bytes).
 *
 * The contents of chartab[]:
 * - The lower two bits, masked by CT_CELL_MASK, give the number of display
 *   cells the character occupies (1 or 2).  Not valid for UTF-8 above 0x80.
 * - CT_PRINT_CHAR bit is set when the character is printable (no need to
 *   translate the character before displaying it).  Note that only DBCS
 *   characters can have 2 display cells and still be printable.
 * - CT_FNAME_CHAR bit is set when the character can be in a file name.
 * - CT_ID_CHAR bit is set when the character can be in an identifier.
 *
 * Return FAIL if 'iskeyword', 'isident', 'isfname' or 'isprint' option has an
 * error, OK otherwise.
 */
    int
init_chartab()
{
    return buf_init_chartab(curbuf, TRUE);
}

    int
buf_init_chartab(buf, global)
    buf_T	*buf;
    int		global;		/* FALSE: only set buf->b_chartab[] */
{
    int		c;
    int		c2;
    char_u	*p;
    int		i;
    int		tilde;
    int		do_isalpha;

    if (global)
    {
	/*
	 * Set the default size for printable characters:
	 * From <Space> to '~' is 1 (printable), others are 2 (not printable).
	 * This also inits all 'isident' and 'isfname' flags to FALSE.
	 *
	 * EBCDIC: all chars below ' ' are not printable, all others are
	 * printable.
	 */
	c = 0;
	while (c < ' ')
	    chartab[c++] = (dy_flags & DY_UHEX) ? 4 : 2;
#ifdef EBCDIC
	while (c < 255)
#else
	while (c <= '~')
#endif
	    chartab[c++] = 1 + CT_PRINT_CHAR;
#ifdef FEAT_FKMAP
	if (p_altkeymap)
	{
	    while (c < YE)
		chartab[c++] = 1 + CT_PRINT_CHAR;
	}
#endif
	while (c < 256)
	{
#ifdef FEAT_MBYTE
	    /* UTF-8: bytes 0xa0 - 0xff are printable (latin1) */
	    if (enc_utf8 && c >= 0xa0)
		chartab[c++] = CT_PRINT_CHAR + 1;
	    /* euc-jp characters starting with 0x8e are single width */
	    else if (enc_dbcs == DBCS_JPNU && c == 0x8e)
		chartab[c++] = CT_PRINT_CHAR + 1;
	    /* other double-byte chars can be printable AND double-width */
	    else if (enc_dbcs != 0 && MB_BYTE2LEN(c) == 2)
		chartab[c++] = CT_PRINT_CHAR + 2;
	    else
#endif
		/* the rest is unprintable by default */
		chartab[c++] = (dy_flags & DY_UHEX) ? 4 : 2;
	}

#ifdef FEAT_MBYTE
	/* Assume that every multi-byte char is a filename character. */
	for (c = 1; c < 256; ++c)
	    if ((enc_dbcs != 0 && MB_BYTE2LEN(c) > 1)
		    || (enc_dbcs == DBCS_JPNU && c == 0x8e)
		    || (enc_utf8 && c >= 0xa0))
		chartab[c] |= CT_FNAME_CHAR;
#endif
    }

    /*
     * Init word char flags all to FALSE
     */
    vim_memset(buf->b_chartab, 0, (size_t)32);
#ifdef FEAT_MBYTE
    if (enc_dbcs != 0)
	for (c = 0; c < 256; ++c)
	{
	    /* double-byte characters are probably word characters */
	    if (MB_BYTE2LEN(c) == 2)
		SET_CHARTAB(buf, c);
	}
#endif

#ifdef FEAT_LISP
    /*
     * In lisp mode the '-' character is included in keywords.
     */
    if (buf->b_p_lisp)
	SET_CHARTAB(buf, '-');
#endif

    /* Walk through the 'isident', 'iskeyword', 'isfname' and 'isprint'
     * options Each option is a list of characters, character numbers or
     * ranges, separated by commas, e.g.: "200-210,x,#-178,-"
     */
    for (i = global ? 0 : 3; i <= 3; ++i)
    {
	if (i == 0)
	    p = p_isi;		/* first round: 'isident' */
	else if (i == 1)
	    p = p_isp;		/* second round: 'isprint' */
	else if (i == 2)
	    p = p_isf;		/* third round: 'isfname' */
	else	/* i == 3 */
	    p = buf->b_p_isk;	/* fourth round: 'iskeyword' */

	while (*p)
	{
	    tilde = FALSE;
	    do_isalpha = FALSE;
	    if (*p == '^' && p[1] != NUL)
	    {
		tilde = TRUE;
		++p;
	    }
	    if (VIM_ISDIGIT(*p))
		c = getdigits(&p);
	    else
#ifdef FEAT_MBYTE
		 if (has_mbyte)
		c = mb_ptr2char_adv(&p);
	    else
#endif
		c = *p++;
	    c2 = -1;
	    if (*p == '-' && p[1] != NUL)
	    {
		++p;
		if (VIM_ISDIGIT(*p))
		    c2 = getdigits(&p);
		else
#ifdef FEAT_MBYTE
		     if (has_mbyte)
		    c2 = mb_ptr2char_adv(&p);
		else
#endif
		    c2 = *p++;
	    }
	    if (c <= 0 || c >= 256 || (c2 < c && c2 != -1) || c2 >= 256
						 || !(*p == NUL || *p == ','))
		return FAIL;

	    if (c2 == -1)	/* not a range */
	    {
		/*
		 * A single '@' (not "@-@"):
		 * Decide on letters being ID/printable/keyword chars with
		 * standard function isalpha(). This takes care of locale for
		 * single-byte characters).
		 */
		if (c == '@')
		{
		    do_isalpha = TRUE;
		    c = 1;
		    c2 = 255;
		}
		else
		    c2 = c;
	    }
	    while (c <= c2)
	    {
		/* Use the MB_ functions here, because isalpha() doesn't
		 * work properly when 'encoding' is "latin1" and the locale is
		 * "C".  */
		if (!do_isalpha || MB_ISLOWER(c) || MB_ISUPPER(c)
#ifdef FEAT_FKMAP
			|| (p_altkeymap && (F_isalpha(c) || F_isdigit(c)))
#endif
			    )
		{
		    if (i == 0)			/* (re)set ID flag */
		    {
			if (tilde)
			    chartab[c] &= ~CT_ID_CHAR;
			else
			    chartab[c] |= CT_ID_CHAR;
		    }
		    else if (i == 1)		/* (re)set printable */
		    {
			if ((c < ' '
#ifndef EBCDIC
				    || c > '~'
#endif
#ifdef FEAT_FKMAP
				    || (p_altkeymap
					&& (F_isalpha(c) || F_isdigit(c)))
#endif
			    )
#ifdef FEAT_MBYTE
				/* For double-byte we keep the cell width, so
				 * that we can detect it from the first byte. */
				&& !(enc_dbcs && MB_BYTE2LEN(c) == 2)
#endif
			   )
			{
			    if (tilde)
			    {
				chartab[c] = (chartab[c] & ~CT_CELL_MASK)
					     + ((dy_flags & DY_UHEX) ? 4 : 2);
				chartab[c] &= ~CT_PRINT_CHAR;
			    }
			    else
			    {
				chartab[c] = (chartab[c] & ~CT_CELL_MASK) + 1;
				chartab[c] |= CT_PRINT_CHAR;
			    }
			}
		    }
		    else if (i == 2)		/* (re)set fname flag */
		    {
			if (tilde)
			    chartab[c] &= ~CT_FNAME_CHAR;
			else
			    chartab[c] |= CT_FNAME_CHAR;
		    }
		    else /* i == 3 */		/* (re)set keyword flag */
		    {
			if (tilde)
			    RESET_CHARTAB(buf, c);
			else
			    SET_CHARTAB(buf, c);
		    }
		}
		++c;
	    }
	    p = skip_to_option_part(p);
	}
    }
    chartab_initialized = TRUE;
    return OK;
}

/*
 * Translate any special characters in buf[bufsize] in-place.
 * The result is a string with only printable characters, but if there is not
 * enough room, not all characters will be translated.
 */
    void
trans_characters(buf, bufsize)
    char_u	*buf;
    int		bufsize;
{
    int		len;		/* length of string needing translation */
    int		room;		/* room in buffer after string */
    char_u	*trs;		/* translated character */
    int		trs_len;	/* length of trs[] */

    len = (int)STRLEN(buf);
    room = bufsize - len;
    while (*buf != 0)
    {
# ifdef FEAT_MBYTE
	/* Assume a multi-byte character doesn't need translation. */
	if (has_mbyte && (trs_len = (*mb_ptr2len)(buf)) > 1)
	    len -= trs_len;
	else
# endif
	{
	    trs = transchar_byte(*buf);
	    trs_len = (int)STRLEN(trs);
	    if (trs_len > 1)
	    {
		room -= trs_len - 1;
		if (room <= 0)
		    return;
		mch_memmove(buf + trs_len, buf + 1, (size_t)len);
	    }
	    mch_memmove(buf, trs, (size_t)trs_len);
	    --len;
	}
	buf += trs_len;
    }
}

#if defined(FEAT_EVAL) || defined(FEAT_TITLE) || defined(FEAT_INS_EXPAND) \
	|| defined(PROTO)
/*
 * Translate a string into allocated memory, replacing special chars with
 * printable chars.  Returns NULL when out of memory.
 */
    char_u *
transstr(s)
    char_u	*s;
{
    char_u	*res;
    char_u	*p;
#ifdef FEAT_MBYTE
    int		l, len, c;
    char_u	hexbuf[11];
#endif

#ifdef FEAT_MBYTE
    if (has_mbyte)
    {
	/* Compute the length of the result, taking account of unprintable
	 * multi-byte characters. */
	len = 0;
	p = s;
	while (*p != NUL)
	{
	    if ((l = (*mb_ptr2len)(p)) > 1)
	    {
		c = (*mb_ptr2char)(p);
		p += l;
		if (vim_isprintc(c))
		    len += l;
		else
		{
		    transchar_hex(hexbuf, c);
		    len += (int)STRLEN(hexbuf);
		}
	    }
	    else
	    {
		l = byte2cells(*p++);
		if (l > 0)
		    len += l;
		else
		    len += 4;	/* illegal byte sequence */
	    }
	}
	res = alloc((unsigned)(len + 1));
    }
    else
#endif
	res = alloc((unsigned)(vim_strsize(s) + 1));
    if (res != NULL)
    {
	*res = NUL;
	p = s;
	while (*p != NUL)
	{
#ifdef FEAT_MBYTE
	    if (has_mbyte && (l = (*mb_ptr2len)(p)) > 1)
	    {
		c = (*mb_ptr2char)(p);
		if (vim_isprintc(c))
		    STRNCAT(res, p, l);	/* append printable multi-byte char */
		else
		    transchar_hex(res + STRLEN(res), c);
		p += l;
	    }
	    else
#endif
		STRCAT(res, transchar_byte(*p++));
	}
    }
    return res;
}
#endif

#if defined(FEAT_SYN_HL) || defined(FEAT_INS_EXPAND) || defined(PROTO)
/*
 * Convert the string "str[orglen]" to do ignore-case comparing.  Uses the
 * current locale.
 * When "buf" is NULL returns an allocated string (NULL for out-of-memory).
 * Otherwise puts the result in "buf[buflen]".
 */
    char_u *
str_foldcase(str, orglen, buf, buflen)
    char_u	*str;
    int		orglen;
    char_u	*buf;
    int		buflen;
{
    garray_T	ga;
    int		i;
    int		len = orglen;

#define GA_CHAR(i)  ((char_u *)ga.ga_data)[i]
#define GA_PTR(i)   ((char_u *)ga.ga_data + i)
#define STR_CHAR(i)  (buf == NULL ? GA_CHAR(i) : buf[i])
#define STR_PTR(i)   (buf == NULL ? GA_PTR(i) : buf + i)

    /* Copy "str" into "buf" or allocated memory, unmodified. */
    if (buf == NULL)
    {
	ga_init2(&ga, 1, 10);
	if (ga_grow(&ga, len + 1) == FAIL)
	    return NULL;
	mch_memmove(ga.ga_data, str, (size_t)len);
	ga.ga_len = len;
    }
    else
    {
	if (len >= buflen)	    /* Ugly! */
	    len = buflen - 1;
	mch_memmove(buf, str, (size_t)len);
    }
    if (buf == NULL)
	GA_CHAR(len) = NUL;
    else
	buf[len] = NUL;

    /* Make each character lower case. */
    i = 0;
    while (STR_CHAR(i) != NUL)
    {
#ifdef FEAT_MBYTE
	if (enc_utf8 || (has_mbyte && MB_BYTE2LEN(STR_CHAR(i)) > 1))
	{
	    if (enc_utf8)
	    {
		int	c = utf_ptr2char(STR_PTR(i));
		int	ol = utf_ptr2len(STR_PTR(i));
		int	lc = utf_tolower(c);

		/* Only replace the character when it is not an invalid
		 * sequence (ASCII character or more than one byte) and
		 * utf_tolower() doesn't return the original character. */
		if ((c < 0x80 || ol > 1) && c != lc)
		{
		    int	    nl = utf_char2len(lc);

		    /* If the byte length changes need to shift the following
		     * characters forward or backward. */
		    if (ol != nl)
		    {
			if (nl > ol)
			{
			    if (buf == NULL ? ga_grow(&ga, nl - ol + 1) == FAIL
						    : len + nl - ol >= buflen)
			    {
				/* out of memory, keep old char */
				lc = c;
				nl = ol;
			    }
			}
			if (ol != nl)
			{
			    if (buf == NULL)
			    {
				STRMOVE(GA_PTR(i) + nl, GA_PTR(i) + ol);
				ga.ga_len += nl - ol;
			    }
			    else
			    {
				STRMOVE(buf + i + nl, buf + i + ol);
				len += nl - ol;
			    }
			}
		    }
		    (void)utf_char2bytes(lc, STR_PTR(i));
		}
	    }
	    /* skip to next multi-byte char */
	    i += (*mb_ptr2len)(STR_PTR(i));
	}
	else
#endif
	{
	    if (buf == NULL)
		GA_CHAR(i) = TOLOWER_LOC(GA_CHAR(i));
	    else
		buf[i] = TOLOWER_LOC(buf[i]);
	    ++i;
	}
    }

    if (buf == NULL)
	return (char_u *)ga.ga_data;
    return buf;
}
#endif

/*
 * Catch 22: chartab[] can't be initialized before the options are
 * initialized, and initializing options may cause transchar() to be called!
 * When chartab_initialized == FALSE don't use chartab[].
 * Does NOT work for multi-byte characters, c must be <= 255.
 * Also doesn't work for the first byte of a multi-byte, "c" must be a
 * character!
 */
static char_u	transchar_buf[7];

    char_u *
transchar(c)
    int		c;
{
    int			i;

    i = 0;
    if (IS_SPECIAL(c))	    /* special key code, display as ~@ char */
    {
	transchar_buf[0] = '~';
	transchar_buf[1] = '@';
	i = 2;
	c = K_SECOND(c);
    }

    if ((!chartab_initialized && (
#ifdef EBCDIC
		    (c >= 64 && c < 255)
#else
		    (c >= ' ' && c <= '~')
#endif
#ifdef FEAT_FKMAP
			|| F_ischar(c)
#endif
		)) || (c < 256 && vim_isprintc_strict(c)))
    {
	/* printable character */
	transchar_buf[i] = c;
	transchar_buf[i + 1] = NUL;
    }
    else
	transchar_nonprint(transchar_buf + i, c);
    return transchar_buf;
}

#if defined(FEAT_MBYTE) || defined(PROTO)
/*
 * Like transchar(), but called with a byte instead of a character.  Checks
 * for an illegal UTF-8 byte.
 */
    char_u *
transchar_byte(c)
    int		c;
{
    if (enc_utf8 && c >= 0x80)
    {
	transchar_nonprint(transchar_buf, c);
	return transchar_buf;
    }
    return transchar(c);
}
#endif

/*
 * Convert non-printable character to two or more printable characters in
 * "buf[]".  "buf" needs to be able to hold five bytes.
 * Does NOT work for multi-byte characters, c must be <= 255.
 */
    void
transchar_nonprint(buf, c)
    char_u	*buf;
    int		c;
{
    if (c == NL)
	c = NUL;		/* we use newline in place of a NUL */
    else if (c == CAR && get_fileformat(curbuf) == EOL_MAC)
	c = NL;			/* we use CR in place of  NL in this case */

    if (dy_flags & DY_UHEX)		/* 'display' has "uhex" */
	transchar_hex(buf, c);

#ifdef EBCDIC
    /* For EBCDIC only the characters 0-63 and 255 are not printable */
    else if (CtrlChar(c) != 0 || c == DEL)
#else
    else if (c <= 0x7f)				/* 0x00 - 0x1f and 0x7f */
#endif
    {
	buf[0] = '^';
#ifdef EBCDIC
	if (c == DEL)
	    buf[1] = '?';		/* DEL displayed as ^? */
	else
	    buf[1] = CtrlChar(c);
#else
	buf[1] = c ^ 0x40;		/* DEL displayed as ^? */
#endif

	buf[2] = NUL;
    }
#ifdef FEAT_MBYTE
    else if (enc_utf8 && c >= 0x80)
    {
	transchar_hex(buf, c);
    }
#endif
#ifndef EBCDIC
    else if (c >= ' ' + 0x80 && c <= '~' + 0x80)    /* 0xa0 - 0xfe */
    {
	buf[0] = '|';
	buf[1] = c - 0x80;
	buf[2] = NUL;
    }
#else
    else if (c < 64)
    {
	buf[0] = '~';
	buf[1] = MetaChar(c);
	buf[2] = NUL;
    }
#endif
    else					    /* 0x80 - 0x9f and 0xff */
    {
	/*
	 * TODO: EBCDIC I don't know what to do with this chars, so I display
	 * them as '~?' for now
	 */
	buf[0] = '~';
#ifdef EBCDIC
	buf[1] = '?';			/* 0xff displayed as ~? */
#else
	buf[1] = (c - 0x80) ^ 0x40;	/* 0xff displayed as ~? */
#endif
	buf[2] = NUL;
    }
}

    void
transchar_hex(buf, c)
    char_u	*buf;
    int		c;
{
    int		i = 0;

    buf[0] = '<';
#ifdef FEAT_MBYTE
    if (c > 255)
    {
	buf[++i] = nr2hex((unsigned)c >> 12);
	buf[++i] = nr2hex((unsigned)c >> 8);
    }
#endif
    buf[++i] = nr2hex((unsigned)c >> 4);
    buf[++i] = nr2hex((unsigned)c);
    buf[++i] = '>';
    buf[++i] = NUL;
}

/*
 * Convert the lower 4 bits of byte "c" to its hex character.
 * Lower case letters are used to avoid the confusion of <F1> being 0xf1 or
 * function key 1.
 */
    static unsigned
nr2hex(c)
    unsigned	c;
{
    if ((c & 0xf) <= 9)
	return (c & 0xf) + '0';
    return (c & 0xf) - 10 + 'a';
}

/*
 * Return number of display cells occupied by byte "b".
 * Caller must make sure 0 <= b <= 255.
 * For multi-byte mode "b" must be the first byte of a character.
 * A TAB is counted as two cells: "^I".
 * For UTF-8 mode this will return 0 for bytes >= 0x80, because the number of
 * cells depends on further bytes.
 */
    int
byte2cells(b)
    int		b;
{
#ifdef FEAT_MBYTE
    if (enc_utf8 && b >= 0x80)
	return 0;
#endif
    return (chartab[b] & CT_CELL_MASK);
}

/*
 * Return number of display cells occupied by character "c".
 * "c" can be a special key (negative number) in which case 3 or 4 is returned.
 * A TAB is counted as two cells: "^I" or four: "<09>".
 */
    int
char2cells(c)
    int		c;
{
    if (IS_SPECIAL(c))
	return char2cells(K_SECOND(c)) + 2;
#ifdef FEAT_MBYTE
    if (c >= 0x80)
    {
	/* UTF-8: above 0x80 need to check the value */
	if (enc_utf8)
	    return utf_char2cells(c);
	/* DBCS: double-byte means double-width, except for euc-jp with first
	 * byte 0x8e */
	if (enc_dbcs != 0 && c >= 0x100)
	{
	    if (enc_dbcs == DBCS_JPNU && ((unsigned)c >> 8) == 0x8e)
		return 1;
	    return 2;
	}
    }
#endif
    return (chartab[c & 0xff] & CT_CELL_MASK);
}

/*
 * Return number of display cells occupied by character at "*p".
 * A TAB is counted as two cells: "^I" or four: "<09>".
 */
    int
ptr2cells(p)
    char_u	*p;
{
#ifdef FEAT_MBYTE
    /* For UTF-8 we need to look at more bytes if the first byte is >= 0x80. */
    if (enc_utf8 && *p >= 0x80)
	return utf_ptr2cells(p);
    /* For DBCS we can tell the cell count from the first byte. */
#endif
    return (chartab[*p] & CT_CELL_MASK);
}

/*
 * Return the number of characters string "s" will take on the screen,
 * counting TABs as two characters: "^I".
 */
    int
vim_strsize(s)
    char_u	*s;
{
    return vim_strnsize(s, (int)MAXCOL);
}

/*
 * Return the number of characters string "s[len]" will take on the screen,
 * counting TABs as two characters: "^I".
 */
    int
vim_strnsize(s, len)
    char_u	*s;
    int		len;
{
    int		size = 0;

    while (*s != NUL && --len >= 0)
    {
#ifdef FEAT_MBYTE
	if (has_mbyte)
	{
	    int	    l = (*mb_ptr2len)(s);

	    size += ptr2cells(s);
	    s += l;
	    len -= l - 1;
	}
	else
#endif
	    size += byte2cells(*s++);
    }
    return size;
}

/*
 * Return the number of characters 'c' will take on the screen, taking
 * into account the size of a tab.
 * Use a define to make it fast, this is used very often!!!
 * Also see getvcol() below.
 */

#define RET_WIN_BUF_CHARTABSIZE(wp, buf, p, col) \
    if (*(p) == TAB && (!(wp)->w_p_list || lcs_tab1)) \
    { \
	int ts; \
	ts = (buf)->b_p_ts; \
	return (int)(ts - (col % ts)); \
    } \
    else \
	return ptr2cells(p);

#if defined(FEAT_VREPLACE) || defined(FEAT_EX_EXTRA) || defined(FEAT_GUI) \
	|| defined(FEAT_VIRTUALEDIT) || defined(PROTO)
    int
chartabsize(p, col)
    char_u	*p;
    colnr_T	col;
{
    RET_WIN_BUF_CHARTABSIZE(curwin, curbuf, p, col)
}
#endif

#ifdef FEAT_LINEBREAK
    static int
win_chartabsize(wp, p, col)
    win_T	*wp;
    char_u	*p;
    colnr_T	col;
{
    RET_WIN_BUF_CHARTABSIZE(wp, wp->w_buffer, p, col)
}
#endif

/*
 * Return the number of characters the string 's' will take on the screen,
 * taking into account the size of a tab.
 */
    int
linetabsize(s)
    char_u	*s;
{
    return linetabsize_col(0, s);
}

/*
 * Like linetabsize(), but starting at column "startcol".
 */
    int
linetabsize_col(startcol, s)
    int		startcol;
    char_u	*s;
{
    colnr_T	col = startcol;

    while (*s != NUL)
	col += lbr_chartabsize_adv(&s, col);
    return (int)col;
}

/*
 * Like linetabsize(), but for a given window instead of the current one.
 */
    int
win_linetabsize(wp, p, len)
    win_T	*wp;
    char_u	*p;
    colnr_T	len;
{
    colnr_T	col = 0;
    char_u	*s;

    for (s = p; *s != NUL && (len == MAXCOL || s < p + len); mb_ptr_adv(s))
	col += win_lbr_chartabsize(wp, s, col, NULL);
    return (int)col;
}

/*
 * Return TRUE if 'c' is a normal identifier character:
 * Letters and characters from the 'isident' option.
 */
    int
vim_isIDc(c)
    int c;
{
    return (c > 0 && c < 0x100 && (chartab[c] & CT_ID_CHAR));
}

/*
 * return TRUE if 'c' is a keyword character: Letters and characters from
 * 'iskeyword' option for current buffer.
 * For multi-byte characters mb_get_class() is used (builtin rules).
 */
    int
vim_iswordc(c)
    int c;
{
#ifdef FEAT_MBYTE
    if (c >= 0x100)
    {
	if (enc_dbcs != 0)
	    return dbcs_class((unsigned)c >> 8, (unsigned)(c & 0xff)) >= 2;
	if (enc_utf8)
	    return utf_class(c) >= 2;
    }
#endif
    return (c > 0 && c < 0x100 && GET_CHARTAB(curbuf, c) != 0);
}

/*
 * Just like vim_iswordc() but uses a pointer to the (multi-byte) character.
 */
    int
vim_iswordp(p)
    char_u *p;
{
#ifdef FEAT_MBYTE
    if (has_mbyte && MB_BYTE2LEN(*p) > 1)
	return mb_get_class(p) >= 2;
#endif
    return GET_CHARTAB(curbuf, *p) != 0;
}

#if defined(FEAT_SYN_HL) || defined(PROTO)
    int
vim_iswordc_buf(p, buf)
    char_u	*p;
    buf_T	*buf;
{
# ifdef FEAT_MBYTE
    if (has_mbyte && MB_BYTE2LEN(*p) > 1)
	return mb_get_class(p) >= 2;
# endif
    return (GET_CHARTAB(buf, *p) != 0);
}
#endif

/*
 * return TRUE if 'c' is a valid file-name character
 * Assume characters above 0x100 are valid (multi-byte).
 */
    int
vim_isfilec(c)
    int	c;
{
    return (c >= 0x100 || (c > 0 && (chartab[c] & CT_FNAME_CHAR)));
}

/*
 * return TRUE if 'c' is a valid file-name character or a wildcard character
 * Assume characters above 0x100 are valid (multi-byte).
 * Explicitly interpret ']' as a wildcard character as mch_has_wildcard("]")
 * returns false.
 */
    int
vim_isfilec_or_wc(c)
    int c;
{
    char_u buf[2];

    buf[0] = (char_u)c;
    buf[1] = NUL;
    return vim_isfilec(c) || c == ']' || mch_has_wildcard(buf);
}

/*
 * return TRUE if 'c' is a printable character
 * Assume characters above 0x100 are printable (multi-byte), except for
 * Unicode.
 */
    int
vim_isprintc(c)
    int c;
{
#ifdef FEAT_MBYTE
    if (enc_utf8 && c >= 0x100)
	return utf_printable(c);
#endif
    return (c >= 0x100 || (c > 0 && (chartab[c] & CT_PRINT_CHAR)));
}

/*
 * Strict version of vim_isprintc(c), don't return TRUE if "c" is the head
 * byte of a double-byte character.
 */
    int
vim_isprintc_strict(c)
    int	c;
{
#ifdef FEAT_MBYTE
    if (enc_dbcs != 0 && c < 0x100 && MB_BYTE2LEN(c) > 1)
	return FALSE;
    if (enc_utf8 && c >= 0x100)
	return utf_printable(c);
#endif
    return (c >= 0x100 || (c > 0 && (chartab[c] & CT_PRINT_CHAR)));
}

/*
 * like chartabsize(), but also check for line breaks on the screen
 */
    int
lbr_chartabsize(s, col)
    unsigned char	*s;
    colnr_T		col;
{
#ifdef FEAT_LINEBREAK
    if (!curwin->w_p_lbr && *p_sbr == NUL)
    {
#endif
#ifdef FEAT_MBYTE
	if (curwin->w_p_wrap)
	    return win_nolbr_chartabsize(curwin, s, col, NULL);
#endif
	RET_WIN_BUF_CHARTABSIZE(curwin, curbuf, s, col)
#ifdef FEAT_LINEBREAK
    }
    return win_lbr_chartabsize(curwin, s, col, NULL);
#endif
}

/*
 * Call lbr_chartabsize() and advance the pointer.
 */
    int
lbr_chartabsize_adv(s, col)
    char_u	**s;
    colnr_T	col;
{
    int		retval;

    retval = lbr_chartabsize(*s, col);
    mb_ptr_adv(*s);
    return retval;
}

/*
 * This function is used very often, keep it fast!!!!
 *
 * If "headp" not NULL, set *headp to the size of what we for 'showbreak'
 * string at start of line.  Warning: *headp is only set if it's a non-zero
 * value, init to 0 before calling.
 */
    int
win_lbr_chartabsize(wp, s, col, headp)
    win_T	*wp;
    char_u	*s;
    colnr_T	col;
    int		*headp UNUSED;
{
#ifdef FEAT_LINEBREAK
    int		c;
    int		size;
    colnr_T	col2;
    colnr_T	colmax;
    int		added;
# ifdef FEAT_MBYTE
    int		mb_added = 0;
# else
#  define mb_added 0
# endif
    int		numberextra;
    char_u	*ps;
    int		tab_corr = (*s == TAB);
    int		n;

    /*
     * No 'linebreak' and 'showbreak': return quickly.
     */
    if (!wp->w_p_lbr && *p_sbr == NUL)
#endif
    {
#ifdef FEAT_MBYTE
	if (wp->w_p_wrap)
	    return win_nolbr_chartabsize(wp, s, col, headp);
#endif
	RET_WIN_BUF_CHARTABSIZE(wp, wp->w_buffer, s, col)
    }

#ifdef FEAT_LINEBREAK
    /*
     * First get normal size, without 'linebreak'
     */
    size = win_chartabsize(wp, s, col);
    c = *s;

    /*
     * If 'linebreak' set check at a blank before a non-blank if the line
     * needs a break here
     */
    if (wp->w_p_lbr
	    && vim_isbreak(c)
	    && !vim_isbreak(s[1])
	    && !wp->w_p_list
	    && wp->w_p_wrap
# ifdef FEAT_VERTSPLIT
	    && wp->w_width != 0
# endif
       )
    {
	/*
	 * Count all characters from first non-blank after a blank up to next
	 * non-blank after a blank.
	 */
	numberextra = win_col_off(wp);
	col2 = col;
	colmax = (colnr_T)(W_WIDTH(wp) - numberextra);
	if (col >= colmax)
	{
	    n = colmax + win_col_off2(wp);
	    if (n > 0)
		colmax += (((col - colmax) / n) + 1) * n;
	}

	for (;;)
	{
	    ps = s;
	    mb_ptr_adv(s);
	    c = *s;
	    if (!(c != NUL
		    && (vim_isbreak(c)
			|| (!vim_isbreak(c)
			    && (col2 == col || !vim_isbreak(*ps))))))
		break;

	    col2 += win_chartabsize(wp, s, col2);
	    if (col2 >= colmax)		/* doesn't fit */
	    {
		size = colmax - col;
		tab_corr = FALSE;
		break;
	    }
	}
    }
# ifdef FEAT_MBYTE
    else if (has_mbyte && size == 2 && MB_BYTE2LEN(*s) > 1
				    && wp->w_p_wrap && in_win_border(wp, col))
    {
	++size;		/* Count the ">" in the last column. */
	mb_added = 1;
    }
# endif

    /*
     * May have to add something for 'showbreak' string at start of line
     * Set *headp to the size of what we add.
     */
    added = 0;
    if (*p_sbr != NUL && wp->w_p_wrap && col != 0)
    {
	numberextra = win_col_off(wp);
	col += numberextra + mb_added;
	if (col >= (colnr_T)W_WIDTH(wp))
	{
	    col -= W_WIDTH(wp);
	    numberextra = W_WIDTH(wp) - (numberextra - win_col_off2(wp));
	    if (numberextra > 0)
		col = col % numberextra;
	}
	if (col == 0 || col + size > (colnr_T)W_WIDTH(wp))
	{
	    added = vim_strsize(p_sbr);
	    if (tab_corr)
		size += (added / wp->w_buffer->b_p_ts) * wp->w_buffer->b_p_ts;
	    else
		size += added;
	    if (col != 0)
		added = 0;
	}
    }
    if (headp != NULL)
	*headp = added + mb_added;
    return size;
#endif
}

#if defined(FEAT_MBYTE) || defined(PROTO)
/*
 * Like win_lbr_chartabsize(), except that we know 'linebreak' is off and
 * 'wrap' is on.  This means we need to check for a double-byte character that
 * doesn't fit at the end of the screen line.
 */
    static int
win_nolbr_chartabsize(wp, s, col, headp)
    win_T	*wp;
    char_u	*s;
    colnr_T	col;
    int		*headp;
{
    int		n;

    if (*s == TAB && (!wp->w_p_list || lcs_tab1))
    {
	n = wp->w_buffer->b_p_ts;
	return (int)(n - (col % n));
    }
    n = ptr2cells(s);
    /* Add one cell for a double-width character in the last column of the
     * window, displayed with a ">". */
    if (n == 2 && MB_BYTE2LEN(*s) > 1 && in_win_border(wp, col))
    {
	if (headp != NULL)
	    *headp = 1;
	return 3;
    }
    return n;
}

/*
 * Return TRUE if virtual column "vcol" is in the rightmost column of window
 * "wp".
 */
    int
in_win_border(wp, vcol)
    win_T	*wp;
    colnr_T	vcol;
{
    int		width1;		/* width of first line (after line number) */
    int		width2;		/* width of further lines */

#ifdef FEAT_VERTSPLIT
    if (wp->w_width == 0)	/* there is no border */
	return FALSE;
#endif
    width1 = W_WIDTH(wp) - win_col_off(wp);
    if ((int)vcol < width1 - 1)
	return FALSE;
    if ((int)vcol == width1 - 1)
	return TRUE;
    width2 = width1 + win_col_off2(wp);
    if (width2 <= 0)
	return FALSE;
    return ((vcol - width1) % width2 == width2 - 1);
}
#endif /* FEAT_MBYTE */

/*
 * Get virtual column number of pos.
 *  start: on the first position of this character (TAB, ctrl)
 * cursor: where the cursor is on this character (first char, except for TAB)
 *    end: on the last position of this character (TAB, ctrl)
 *
 * This is used very often, keep it fast!
 */
    void
getvcol(wp, pos, start, cursor, end)
    win_T	*wp;
    pos_T	*pos;
    colnr_T	*start;
    colnr_T	*cursor;
    colnr_T	*end;
{
    colnr_T	vcol;
    char_u	*ptr;		/* points to current char */
    char_u	*posptr;	/* points to char at pos->col */
    int		incr;
    int		head;
    int		ts = wp->w_buffer->b_p_ts;
    int		c;

    vcol = 0;
    ptr = ml_get_buf(wp->w_buffer, pos->lnum, FALSE);
    if (pos->col == MAXCOL)
	posptr = NULL;  /* continue until the NUL */
    else
	posptr = ptr + pos->col;

    /*
     * This function is used very often, do some speed optimizations.
     * When 'list', 'linebreak' and 'showbreak' are not set use a simple loop.
     * Also use this when 'list' is set but tabs take their normal size.
     */
    if ((!wp->w_p_list || lcs_tab1 != NUL)
#ifdef FEAT_LINEBREAK
	    && !wp->w_p_lbr && *p_sbr == NUL
#endif
       )
    {
#ifndef FEAT_MBYTE
	head = 0;
#endif
	for (;;)
	{
#ifdef FEAT_MBYTE
	    head = 0;
#endif
	    c = *ptr;
	    /* make sure we don't go past the end of the line */
	    if (c == NUL)
	    {
		incr = 1;	/* NUL at end of line only takes one column */
		break;
	    }
	    /* A tab gets expanded, depending on the current column */
	    if (c == TAB)
		incr = ts - (vcol % ts);
	    else
	    {
#ifdef FEAT_MBYTE
		if (has_mbyte)
		{
		    /* For utf-8, if the byte is >= 0x80, need to look at
		     * further bytes to find the cell width. */
		    if (enc_utf8 && c >= 0x80)
			incr = utf_ptr2cells(ptr);
		    else
			incr = CHARSIZE(c);

		    /* If a double-cell char doesn't fit at the end of a line
		     * it wraps to the next line, it's like this char is three
		     * cells wide. */
		    if (incr == 2 && wp->w_p_wrap && MB_BYTE2LEN(*ptr) > 1
			    && in_win_border(wp, vcol))
		    {
			++incr;
			head = 1;
		    }
		}
		else
#endif
		    incr = CHARSIZE(c);
	    }

	    if (posptr != NULL && ptr >= posptr) /* character at pos->col */
		break;

	    vcol += incr;
	    mb_ptr_adv(ptr);
	}
    }
    else
    {
	for (;;)
	{
	    /* A tab gets expanded, depending on the current column */
	    head = 0;
	    incr = win_lbr_chartabsize(wp, ptr, vcol, &head);
	    /* make sure we don't go past the end of the line */
	    if (*ptr == NUL)
	    {
		incr = 1;	/* NUL at end of line only takes one column */
		break;
	    }

	    if (posptr != NULL && ptr >= posptr) /* character at pos->col */
		break;

	    vcol += incr;
	    mb_ptr_adv(ptr);
	}
    }
    if (start != NULL)
	*start = vcol + head;
    if (end != NULL)
	*end = vcol + incr - 1;
    if (cursor != NULL)
    {
	if (*ptr == TAB
		&& (State & NORMAL)
		&& !wp->w_p_list
		&& !virtual_active()
#ifdef FEAT_VISUAL
		&& !(VIsual_active
				   && (*p_sel == 'e' || ltoreq(*pos, VIsual)))
#endif
		)
	    *cursor = vcol + incr - 1;	    /* cursor at end */
	else
	    *cursor = vcol + head;	    /* cursor at start */
    }
}

/*
 * Get virtual cursor column in the current window, pretending 'list' is off.
 */
    colnr_T
getvcol_nolist(posp)
    pos_T	*posp;
{
    int		list_save = curwin->w_p_list;
    colnr_T	vcol;

    curwin->w_p_list = FALSE;
    getvcol(curwin, posp, NULL, &vcol, NULL);
    curwin->w_p_list = list_save;
    return vcol;
}

#if defined(FEAT_VIRTUALEDIT) || defined(PROTO)
/*
 * Get virtual column in virtual mode.
 */
    void
getvvcol(wp, pos, start, cursor, end)
    win_T	*wp;
    pos_T	*pos;
    colnr_T	*start;
    colnr_T	*cursor;
    colnr_T	*end;
{
    colnr_T	col;
    colnr_T	coladd;
    colnr_T	endadd;
# ifdef FEAT_MBYTE
    char_u	*ptr;
# endif

    if (virtual_active())
    {
	/* For virtual mode, only want one value */
	getvcol(wp, pos, &col, NULL, NULL);

	coladd = pos->coladd;
	endadd = 0;
# ifdef FEAT_MBYTE
	/* Cannot put the cursor on part of a wide character. */
	ptr = ml_get_buf(wp->w_buffer, pos->lnum, FALSE);
	if (pos->col < (colnr_T)STRLEN(ptr))
	{
	    int c = (*mb_ptr2char)(ptr + pos->col);

	    if (c != TAB && vim_isprintc(c))
	    {
		endadd = (colnr_T)(char2cells(c) - 1);
		if (coladd > endadd)	/* past end of line */
		    endadd = 0;
		else
		    coladd = 0;
	    }
	}
# endif
	col += coladd;
	if (start != NULL)
	    *start = col;
	if (cursor != NULL)
	    *cursor = col;
	if (end != NULL)
	    *end = col + endadd;
    }
    else
	getvcol(wp, pos, start, cursor, end);
}
#endif

#if defined(FEAT_VISUAL) || defined(PROTO)
/*
 * Get the leftmost and rightmost virtual column of pos1 and pos2.
 * Used for Visual block mode.
 */
    void
getvcols(wp, pos1, pos2, left, right)
    win_T	*wp;
    pos_T	*pos1, *pos2;
    colnr_T	*left, *right;
{
    colnr_T	from1, from2, to1, to2;

    if (ltp(pos1, pos2))
    {
	getvvcol(wp, pos1, &from1, NULL, &to1);
	getvvcol(wp, pos2, &from2, NULL, &to2);
    }
    else
    {
	getvvcol(wp, pos2, &from1, NULL, &to1);
	getvvcol(wp, pos1, &from2, NULL, &to2);
    }
    if (from2 < from1)
	*left = from2;
    else
	*left = from1;
    if (to2 > to1)
    {
	if (*p_sel == 'e' && from2 - 1 >= to1)
	    *right = from2 - 1;
	else
	    *right = to2;
    }
    else
	*right = to1;
}
#endif

/*
 * skipwhite: skip over ' ' and '\t'.
 */
    char_u *
skipwhite(q)
    char_u	*q;
{
    char_u	*p = q;

    while (vim_iswhite(*p)) /* skip to next non-white */
	++p;
    return p;
}

/*
 * skip over digits
 */
    char_u *
skipdigits(q)
    char_u	*q;
{
    char_u	*p = q;

    while (VIM_ISDIGIT(*p))	/* skip to next non-digit */
	++p;
    return p;
}

#if defined(FEAT_SYN_HL) || defined(FEAT_SPELL) || defined(PROTO)
/*
 * skip over digits and hex characters
 */
    char_u *
skiphex(q)
    char_u	*q;
{
    char_u	*p = q;

    while (vim_isxdigit(*p))	/* skip to next non-digit */
	++p;
    return p;
}
#endif

#if defined(FEAT_EX_EXTRA) || defined(PROTO)
/*
 * skip to digit (or NUL after the string)
 */
    char_u *
skiptodigit(q)
    char_u	*q;
{
    char_u	*p = q;

    while (*p != NUL && !VIM_ISDIGIT(*p))	/* skip to next digit */
	++p;
    return p;
}

/*
 * skip to hex character (or NUL after the string)
 */
    char_u *
skiptohex(q)
    char_u	*q;
{
    char_u	*p = q;

    while (*p != NUL && !vim_isxdigit(*p))	/* skip to next digit */
	++p;
    return p;
}
#endif

/*
 * Variant of isdigit() that can handle characters > 0x100.
 * We don't use isdigit() here, because on some systems it also considers
 * superscript 1 to be a digit.
 * Use the VIM_ISDIGIT() macro for simple arguments.
 */
    int
vim_isdigit(c)
    int		c;
{
    return (c >= '0' && c <= '9');
}

/*
 * Variant of isxdigit() that can handle characters > 0x100.
 * We don't use isxdigit() here, because on some systems it also considers
 * superscript 1 to be a digit.
 */
    int
vim_isxdigit(c)
    int		c;
{
    return (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'f')
	|| (c >= 'A' && c <= 'F');
}

#if defined(FEAT_MBYTE) || defined(PROTO)
/*
 * Vim's own character class functions.  These exist because many library
 * islower()/toupper() etc. do not work properly: they crash when used with
 * invalid values or can't handle latin1 when the locale is C.
 * Speed is most important here.
 */
#define LATIN1LOWER 'l'
#define LATIN1UPPER 'U'

/*                                                                 !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]%_'abcdefghijklmnopqrstuvwxyz{|}~                                  ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏĞÑÒÓÔÕÖ×ØÙÚÛÜİŞßàáâãäåæçèéêëìíîïğñòóôõö÷øùúûüışÿ */
static char_u latin1flags[257] = "                                                                 UUUUUUUUUUUUUUUUUUUUUUUUUU      llllllllllllllllllllllllll                                                                     UUUUUUUUUUUUUUUUUUUUUUU UUUUUUUllllllllllllllllllllllll llllllll";
static char_u latin1upper[257] = "                                 !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`ABCDEFGHIJKLMNOPQRSTUVWXYZ{|}~€‚ƒ„…†‡ˆ‰Š‹Œ‘’“”•–—˜™š›œŸ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏĞÑÒÓÔÕÖ×ØÙÚÛÜİŞßÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏĞÑÒÓÔÕÖ÷ØÙÚÛÜİŞÿ";
static char_u latin1lower[257] = "                                 !\"#$%&'()*+,-./0123456789:;<=>?@abcdefghijklmnopqrstuvwxyz[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~€‚ƒ„…†‡ˆ‰Š‹Œ‘’“”•–—˜™š›œŸ ¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿àáâãäåæçèéêëìíîïğñòóôõö×øùúûüışßàáâãäåæçèéêëìíîïğñòóôõö÷øùúûüışÿ";

    int
vim_islower(c)
    int	    c;
{
    if (c <= '@')
	return FALSE;
    if (c >= 0x80)
    {
	if (enc_utf8)
	    return utf_islower(c);
	if (c >= 0x100)
	{
#ifdef HAVE_ISWLOWER
	    if (has_mbyte)
		return iswlower(c);
#endif
	    /* islower() can't handle these chars and may crash */
	    return FALSE;
	}
	if (enc_latin1like)
	    return (latin1flags[c] & LATIN1LOWER) == LATIN1LOWER;
    }
    return islower(c);
}

    int
vim_isupper(c)
    int	    c;
{
    if (c <= '@')
	return FALSE;
    if (c >= 0x80)
    {
	if (enc_utf8)
	    return utf_isupper(c);
	if (c >= 0x100)
	{
#ifdef HAVE_ISWUPPER
	    if (has_mbyte)
		return iswupper(c);
#endif
	    /* islower() can't handle these chars and may crash */
	    return FALSE;
	}
	if (enc_latin1like)
	    return (latin1flags[c] & LATIN1UPPER) == LATIN1UPPER;
    }
    return isupper(c);
}

    int
vim_toupper(c)
    int	    c;
{
    if (c <= '@')
	return c;
    if (c >= 0x80)
    {
	if (enc_utf8)
	    return utf_toupper(c);
	if (c >= 0x100)
	{
#ifdef HAVE_TOWUPPER
	    if (has_mbyte)
		return towupper(c);
#endif
	    /* toupper() can't handle these chars and may crash */
	    return c;
	}
	if (enc_latin1like)
	    return latin1upper[c];
    }
    return TOUPPER_LOC(c);
}

    int
vim_tolower(c)
    int	    c;
{
    if (c <= '@')
	return c;
    if (c >= 0x80)
    {
	if (enc_utf8)
	    return utf_tolower(c);
	if (c >= 0x100)
	{
#ifdef HAVE_TOWLOWER
	    if (has_mbyte)
		return towlower(c);
#endif
	    /* tolower() can't handle these chars and may crash */
	    return c;
	}
	if (enc_latin1like)
	    return latin1lower[c];
    }
    return TOLOWER_LOC(c);
}
#endif

/*
 * skiptowhite: skip over text until ' ' or '\t' or NUL.
 */
    char_u *
skiptowhite(p)
    char_u	*p;
{
    while (*p != ' ' && *p != '\t' && *p != NUL)
	++p;
    return p;
}

#if defined(FEAT_LISTCMDS) || defined(FEAT_SIGNS) || defined(FEAT_SNIFF) \
	|| defined(PROTO)
/*
 * skiptowhite_esc: Like skiptowhite(), but also skip escaped chars
 */
    char_u *
skiptowhite_esc(p)
    char_u	*p;
{
    while (*p != ' ' && *p != '\t' && *p != NUL)
    {
	if ((*p == '\\' || *p == Ctrl_V) && *(p + 1) != NUL)
	    ++p;
	++p;
    }
    return p;
}
#endif

/*
 * Getdigits: Get a number from a string and skip over it.
 * Note: the argument is a pointer to a char_u pointer!
 */
    long
getdigits(pp)
    char_u **pp;
{
    char_u	*p;
    long	retval;

    p = *pp;
    retval = atol((char *)p);
    if (*p == '-')		/* skip negative sign */
	++p;
    p = skipdigits(p);		/* skip to next non-digit */
    *pp = p;
    return retval;
}

/*
 * Return TRUE if "lbuf" is empty or only contains blanks.
 */
    int
vim_isblankline(lbuf)
    char_u	*lbuf;
{
    char_u	*p;

    p = skipwhite(lbuf);
    return (*p == NUL || *p == '\r' || *p == '\n');
}

/*
 * Convert a string into a long and/or unsigned long, taking care of
 * hexadecimal and octal numbers.  Accepts a '-' sign.
 * If "hexp" is not NULL, returns a flag to indicate the type of the number:
 *  0	    decimal
 *  '0'	    octal
 *  'X'	    hex
 *  'x'	    hex
 * If "len" is not NULL, the length of the number in characters is returned.
 * If "nptr" is not NULL, the signed result is returned in it.
 * If "unptr" is not NULL, the unsigned result is returned in it.
 * If "dooct" is non-zero recognize octal numbers, when > 1 always assume
 * octal number.
 * If "dohex" is non-zero recognize hex numbers, when > 1 always assume
 * hex number.
 */
    void
vim_str2nr(start, hexp, len, dooct, dohex, nptr, unptr)
    char_u		*start;
    int			*hexp;	    /* return: type of number 0 = decimal, 'x'
				       or 'X' is hex, '0' = octal */
    int			*len;	    /* return: detected length of number */
    int			dooct;	    /* recognize octal number */
    int			dohex;	    /* recognize hex number */
    long		*nptr;	    /* return: signed result */
    unsigned long	*unptr;	    /* return: unsigned result */
{
    char_u	    *ptr = start;
    int		    hex = 0;		/* default is decimal */
    int		    negative = FALSE;
    unsigned long   un = 0;
    int		    n;

    if (ptr[0] == '-')
    {
	negative = TRUE;
	++ptr;
    }

    /* Recognize hex and octal. */
    if (ptr[0] == '0' && ptr[1] != '8' && ptr[1] != '9')
    {
	hex = ptr[1];
	if (dohex && (hex == 'X' || hex == 'x') && vim_isxdigit(ptr[2]))
	    ptr += 2;			/* hexadecimal */
	else
	{
	    hex = 0;			/* default is decimal */
	    if (dooct)
	    {
		/* Don't interpret "0", "08" or "0129" as octal. */
		for (n = 1; VIM_ISDIGIT(ptr[n]); ++n)
		{
		    if (ptr[n] > '7')
		    {
			hex = 0;	/* can't be octal */
			break;
		    }
		    if (ptr[n] > '0')
			hex = '0';	/* assume octal */
		}
	    }
	}
    }

    /*
     * Do the string-to-numeric conversion "manually" to avoid sscanf quirks.
     */
    if (hex == '0' || dooct > 1)
    {
	/* octal */
	while ('0' <= *ptr && *ptr <= '7')
	{
	    un = 8 * un + (unsigned long)(*ptr - '0');
	    ++ptr;
	}
    }
    else if (hex != 0 || dohex > 1)
    {
	/* hex */
	while (vim_isxdigit(*ptr))
	{
	    un = 16 * un + (unsigned long)hex2nr(*ptr);
	    ++ptr;
	}
    }
    else
    {
	/* decimal */
	while (VIM_ISDIGIT(*ptr))
	{
	    un = 10 * un + (unsigned long)(*ptr - '0');
	    ++ptr;
	}
    }

    if (hexp != NULL)
	*hexp = hex;
    if (len != NULL)
	*len = (int)(ptr - start);
    if (nptr != NULL)
    {
	if (negative)   /* account for leading '-' for decimal numbers */
	    *nptr = -(long)un;
	else
	    *nptr = (long)un;
    }
    if (unptr != NULL)
	*unptr = un;
}

/*
 * Return the value of a single hex character.
 * Only valid when the argument is '0' - '9', 'A' - 'F' or 'a' - 'f'.
 */
    int
hex2nr(c)
    int		c;
{
    if (c >= 'a' && c <= 'f')
	return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
	return c - 'A' + 10;
    return c - '0';
}

#if defined(FEAT_TERMRESPONSE) \
	|| (defined(FEAT_GUI_GTK) && defined(FEAT_WINDOWS)) || defined(PROTO)
/*
 * Convert two hex characters to a byte.
 * Return -1 if one of the characters is not hex.
 */
    int
hexhex2nr(p)
    char_u	*p;
{
    if (!vim_isxdigit(p[0]) || !vim_isxdigit(p[1]))
	return -1;
    return (hex2nr(p[0]) << 4) + hex2nr(p[1]);
}
#endif

/*
 * Return TRUE if "str" starts with a backslash that should be removed.
 * For MS-DOS, WIN32 and OS/2 this is only done when the character after the
 * backslash is not a normal file name character.
 * '$' is a valid file name character, we don't remove the backslash before
 * it.  This means it is not possible to use an environment variable after a
 * backslash.  "C:\$VIM\doc" is taken literally, only "$VIM\doc" works.
 * Although "\ name" is valid, the backslash in "Program\ files" must be
 * removed.  Assume a file name doesn't start with a space.
 * For multi-byte names, never remove a backslash before a non-ascii
 * character, assume that all multi-byte characters are valid file name
 * characters.
 */
    int
rem_backslash(str)
    char_u  *str;
{
#ifdef BACKSLASH_IN_FILENAME
    return (str[0] == '\\'
# ifdef FEAT_MBYTE
	    && str[1] < 0x80
# endif
	    && (str[1] == ' '
		|| (str[1] != NUL
		    && str[1] != '*'
		    && str[1] != '?'
		    && !vim_isfilec(str[1]))));
#else
    return (str[0] == '\\' && str[1] != NUL);
#endif
}

/*
 * Halve the number of backslashes in a file name argument.
 * For MS-DOS we only do this if the character after the backslash
 * is not a normal file character.
 */
    void
backslash_halve(p)
    char_u	*p;
{
    for ( ; *p; ++p)
	if (rem_backslash(p))
	    STRMOVE(p, p + 1);
}

/*
 * backslash_halve() plus save the result in allocated memory.
 */
    char_u *
backslash_halve_save(p)
    char_u	*p;
{
    char_u	*res;

    res = vim_strsave(p);
    if (res == NULL)
	return p;
    backslash_halve(res);
    return res;
}

#if (defined(EBCDIC) && defined(FEAT_POSTSCRIPT)) || defined(PROTO)
/*
 * Table for EBCDIC to ASCII conversion unashamedly taken from xxd.c!
 * The first 64 entries have been added to map control characters defined in
 * ascii.h
 */
static char_u ebcdic2ascii_tab[256] =
{
    0000, 0001, 0002, 0003, 0004, 0011, 0006, 0177,
    0010, 0011, 0012, 0013, 0014, 0015, 0016, 0017,
    0020, 0021, 0022, 0023, 0024, 0012, 0010, 0027,
    0030, 0031, 0032, 0033, 0033, 0035, 0036, 0037,
    0040, 0041, 0042, 0043, 0044, 0045, 0046, 0047,
    0050, 0051, 0052, 0053, 0054, 0055, 0056, 0057,
    0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067,
    0070, 0071, 0072, 0073, 0074, 0075, 0076, 0077,
    0040, 0240, 0241, 0242, 0243, 0244, 0245, 0246,
    0247, 0250, 0325, 0056, 0074, 0050, 0053, 0174,
    0046, 0251, 0252, 0253, 0254, 0255, 0256, 0257,
    0260, 0261, 0041, 0044, 0052, 0051, 0073, 0176,
    0055, 0057, 0262, 0263, 0264, 0265, 0266, 0267,
    0270, 0271, 0313, 0054, 0045, 0137, 0076, 0077,
    0272, 0273, 0274, 0275, 0276, 0277, 0300, 0301,
    0302, 0140, 0072, 0043, 0100, 0047, 0075, 0042,
    0303, 0141, 0142, 0143, 0144, 0145, 0146, 0147,
    0150, 0151, 0304, 0305, 0306, 0307, 0310, 0311,
    0312, 0152, 0153, 0154, 0155, 0156, 0157, 0160,
    0161, 0162, 0136, 0314, 0315, 0316, 0317, 0320,
    0321, 0345, 0163, 0164, 0165, 0166, 0167, 0170,
    0171, 0172, 0322, 0323, 0324, 0133, 0326, 0327,
    0330, 0331, 0332, 0333, 0334, 0335, 0336, 0337,
    0340, 0341, 0342, 0343, 0344, 0135, 0346, 0347,
    0173, 0101, 0102, 0103, 0104, 0105, 0106, 0107,
    0110, 0111, 0350, 0351, 0352, 0353, 0354, 0355,
    0175, 0112, 0113, 0114, 0115, 0116, 0117, 0120,
    0121, 0122, 0356, 0357, 0360, 0361, 0362, 0363,
    0134, 0237, 0123, 0124, 0125, 0126, 0127, 0130,
    0131, 0132, 0364, 0365, 0366, 0367, 0370, 0371,
    0060, 0061, 0062, 0063, 0064, 0065, 0066, 0067,
    0070, 0071, 0372, 0373, 0374, 0375, 0376, 0377
};

/*
 * Convert a buffer worth of characters from EBCDIC to ASCII.  Only useful if
 * wanting 7-bit ASCII characters out the other end.
 */
    void
ebcdic2ascii(buffer, len)
    char_u	*buffer;
    int		len;
{
    int		i;

    for (i = 0; i < len; i++)
	buffer[i] = ebcdic2ascii_tab[buffer[i]];
}
#endif
