/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * strptime.c: portable strptime() fallback for platforms whose C runtime
 * does not provide one (currently the MSVC / MinGW CRT on Windows).
 *
 * Ported from NetBSD's lib/libc/time/strptime.c (rev 1.67, 2024-06-07).
 * The original BSD 2-clause licence follows below and must be preserved.
 * Windows-specific adjustments:
 *   - Locale-specific weekday / month / AM-PM tables are hard-coded in
 *     English; this matches what strftime() emits in the "C" locale on
 *     Windows and is sufficient for Vim's strptime() test suite.
 *   - The NetBSD-specific fromzone()/tzalloc() path is stubbed out because
 *     the Windows CRT does not ship the IANA tzfile loader.
 *   - The tm_gmtoff / tm_zone BSD extensions are not available in the
 *     Windows CRT's struct tm, so the %Z / %z conversion parses the input
 *     without storing the offset (matching NetBSD behavior when those
 *     fields are not compiled in).
 */

/*-
 * Copyright (c) 1997, 1998, 2005, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code was contributed to The NetBSD Foundation by Klaus Klein.
 * Heavily optimised by David Laight
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "vim.h"

#ifdef MSWIN

/* Constants lifted from NetBSD's <tzfile.h>. */
#define TM_YEAR_BASE	1900
#define TM_SUNDAY	0
#define TM_MONDAY	1
#define SECSPERHOUR	3600
#define SECSPERMIN	60
#define HOURSPERDAY	24

#define isleap(y)	(((y) % 4 == 0 && (y) % 100 != 0) || (y) % 400 == 0)
/* isleap_sum avoids integer overflow when adding the 1900 base. */
#define isleap_sum(a, b) \
			isleap(((unsigned)(a) + (unsigned)(b)) % 400)

#ifdef _MSC_VER
# define timezone   _timezone
# define tzname	    _tzname
#endif

/* Locale tables (English / "C" locale). */
static const char *const c_day[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};
static const char *const c_abday[] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const char *const c_mon[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};
static const char *const c_abmon[] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
static const char *const c_am_pm[] = { "AM", "PM" };
static const char c_d_t_fmt[] = "%a %b %e %H:%M:%S %Y";
static const char c_d_fmt[] = "%m/%d/%y";
static const char c_t_fmt[] = "%H:%M:%S";
static const char c_t_fmt_ampm[] = "%I:%M:%S %p";

static const unsigned char *conv_num(const unsigned char *, int *,
						unsigned int, unsigned int);
static const unsigned char *find_string(const unsigned char *, int *,
			const char * const *, const char * const *, int);

/*
 * We do not implement alternate representations. However, we always
 * check whether a given modifier is allowed for a certain conversion.
 */
#define ALT_E			0x01
#define ALT_O			0x02
#define LEGAL_ALT(x)		{ if (alt_format & ~(x)) return NULL; }

#define S_YEAR			(1 << 0)
#define S_MON			(1 << 1)
#define S_YDAY			(1 << 2)
#define S_MDAY			(1 << 3)
#define S_WDAY			(1 << 4)
#define S_HOUR			(1 << 5)

#define HAVE_MDAY(s)		(s & S_MDAY)
#define HAVE_MON(s)		(s & S_MON)
#define HAVE_WDAY(s)		(s & S_WDAY)
#define HAVE_YDAY(s)		(s & S_YDAY)
#define HAVE_YEAR(s)		(s & S_YEAR)
#define HAVE_HOUR(s)		(s & S_HOUR)

/* RFC-822/RFC-2822 */
static const char *const nast[5] = {
    "EST",    "CST",    "MST",    "PST",    "\0\0\0"
};
static const char *const nadt[5] = {
    "EDT",    "CDT",    "MDT",    "PDT",    "\0\0\0"
};

/*
 * Table to determine the ordinal date for the start of a month.
 * Ref: http://en.wikipedia.org/wiki/ISO_week_date
 */
static const int start_of_month[2][13] = {
    /* non-leap year */
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
    /* leap year */
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

/*
 * Calculate the week day of the first day of a year. Valid for
 * the Gregorian calendar, which began Sept 14, 1752 in the UK
 * and its colonies. Ref:
 * http://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week
 */
    static int
first_wday_of(int yr)
{
    return ((2 * (3 - (yr / 100) % 4)) + (yr % 100) + ((yr % 100) /  4) +
	    (isleap(yr) ? 6 : 0) + 1) % 7;
}

#define delim(p)	((p) == '\0' || isspace((unsigned char)(p)))

/*
 * Stub replacing NetBSD's fromzone(): the Windows CRT does not provide
 * tzalloc() / tzgetgmtoff() and the associated IANA tzfile loader, so
 * we simply decline to recognize arbitrary timezone names.  The %Z / %z
 * numeric and RFC-822/2822 forms are still handled by the caller.
 */
    static int
fromzone(const unsigned char **bp UNUSED, struct tm *tm UNUSED,
						    int mandatory UNUSED)
{
    return 0;
}

    char *
strptime(const char *buf, const char *fmt, struct tm *tm)
{
    unsigned char c;
    const unsigned char *bp, *ep, *zname;
    int alt_format, i, split_year = 0, neg = 0, state = 0,
	day_offset = -1, week_offset = 0, offs, mandatory;
    const char *new_fmt;

    bp = (const unsigned char *)buf;

    while (bp != NULL && (c = *fmt++) != '\0')
    {
	/* Clear `alternate' modifier prior to new conversion. */
	alt_format = 0;
	i = 0;

	/* Eat up white-space. */
	if (isspace(c))
	{
	    while (isspace(*bp))
		bp++;
	    continue;
	}

	if (c != '%')
	    goto literal;


again:	switch (c = *fmt++)
	{
	case '%':	/* "%%" is converted to "%". */
literal:
	    if (c != *bp++)
		return NULL;
	    LEGAL_ALT(0);
	    continue;

	/*
	 * "Alternative" modifiers. Just set the appropriate flag
	 * and start over again.
	 */
	case 'E':	/* "%E?" alternative conversion modifier. */
	    LEGAL_ALT(0);
	    alt_format |= ALT_E;
	    goto again;

	case 'O':	/* "%O?" alternative conversion modifier. */
	    LEGAL_ALT(0);
	    alt_format |= ALT_O;
	    goto again;

	/*
	 * "Complex" conversion rules, implemented through recursion.
	 */
	case 'c':	/* Date and time, using the locale's format. */
	    new_fmt = c_d_t_fmt;
	    state |= S_WDAY | S_MON | S_MDAY | S_YEAR;
	    goto recurse;

	case 'D':	/* The date as "%m/%d/%y". */
	    new_fmt = "%m/%d/%y";
	    LEGAL_ALT(0);
	    state |= S_MON | S_MDAY | S_YEAR;
	    goto recurse;

	case 'F':	/* The date as "%Y-%m-%d". */
	    new_fmt = "%Y-%m-%d";
	    LEGAL_ALT(0);
	    state |= S_MON | S_MDAY | S_YEAR;
	    goto recurse;

	case 'R':	/* The time as "%H:%M". */
	    new_fmt = "%H:%M";
	    LEGAL_ALT(0);
	    goto recurse;

	case 'r':	/* The time in 12-hour clock representation. */
	    new_fmt = c_t_fmt_ampm;
	    LEGAL_ALT(0);
	    goto recurse;

	case 'T':	/* The time as "%H:%M:%S". */
	    new_fmt = "%H:%M:%S";
	    LEGAL_ALT(0);
	    goto recurse;

	case 'X':	/* The time, using the locale's format. */
	    new_fmt = c_t_fmt;
	    goto recurse;

	case 'x':	/* The date, using the locale's format. */
	    new_fmt = c_d_fmt;
	    state |= S_MON | S_MDAY | S_YEAR;
	recurse:
	    bp = (const unsigned char *)strptime((const char *)bp,
							new_fmt, tm);
	    LEGAL_ALT(ALT_E);
	    continue;

	/*
	 * "Elementary" conversion rules.
	 */
	case 'A':	/* The day of week, using the locale's form. */
	case 'a':
	    bp = find_string(bp, &tm->tm_wday, c_day, c_abday, 7);
	    LEGAL_ALT(0);
	    state |= S_WDAY;
	    continue;

	case 'B':	/* The month, using the locale's form. */
	case 'b':
	case 'h':
	    bp = find_string(bp, &tm->tm_mon, c_mon, c_abmon, 12);
	    LEGAL_ALT(0);
	    state |= S_MON;
	    continue;

	case 'C':	/* The century number. */
	    i = 20;
	    bp = conv_num(bp, &i, 0, 99);

	    i = i * 100 - TM_YEAR_BASE;
	    if (split_year)
		i += tm->tm_year % 100;
	    split_year = 1;
	    tm->tm_year = i;
	    LEGAL_ALT(ALT_E);
	    state |= S_YEAR;
	    continue;

	case 'd':	/* The day of month. */
	case 'e':
	    bp = conv_num(bp, &tm->tm_mday, 1, 31);
	    LEGAL_ALT(ALT_O);
	    state |= S_MDAY;
	    continue;

	case 'k':	/* The hour (24-hour clock representation). */
	    LEGAL_ALT(0);
	    /* FALLTHROUGH */
	case 'H':
	    bp = conv_num(bp, &tm->tm_hour, 0, 23);
	    LEGAL_ALT(ALT_O);
	    state |= S_HOUR;
	    continue;

	case 'l':	/* The hour (12-hour clock representation). */
	    LEGAL_ALT(0);
	    /* FALLTHROUGH */
	case 'I':
	    bp = conv_num(bp, &tm->tm_hour, 1, 12);
	    if (tm->tm_hour == 12)
		tm->tm_hour = 0;
	    LEGAL_ALT(ALT_O);
	    state |= S_HOUR;
	    continue;

	case 'j':	/* The day of year. */
	    i = 1;
	    bp = conv_num(bp, &i, 1, 366);
	    tm->tm_yday = i - 1;
	    LEGAL_ALT(0);
	    state |= S_YDAY;
	    continue;

	case 'M':	/* The minute. */
	    bp = conv_num(bp, &tm->tm_min, 0, 59);
	    LEGAL_ALT(ALT_O);
	    continue;

	case 'm':	/* The month. */
	    i = 1;
	    bp = conv_num(bp, &i, 1, 12);
	    tm->tm_mon = i - 1;
	    LEGAL_ALT(ALT_O);
	    state |= S_MON;
	    continue;

	case 'p':	/* The locale's equivalent of AM/PM. */
	    bp = find_string(bp, &i, c_am_pm, NULL, 2);
	    if (HAVE_HOUR(state) && tm->tm_hour > 11)
		return NULL;
	    tm->tm_hour += i * 12;
	    LEGAL_ALT(0);
	    continue;

	case 'S':	/* The seconds. */
	    bp = conv_num(bp, &tm->tm_sec, 0, 61);
	    LEGAL_ALT(ALT_O);
	    continue;

	case 's':	/* seconds since the epoch */
	    {
		const time_t TIME_MAX = (time_t)((sizeof(time_t) == 8)
					? LLONG_MAX : INT_MAX);
		time_t sse, d;

		if (*bp < '0' || *bp > '9')
		{
		    bp = NULL;
		    continue;
		}

		sse = *bp++ - '0';
		while (*bp >= '0' && *bp <= '9')
		{
		    d = *bp++ - '0';
		    if (sse > TIME_MAX / 10)
		    {
			bp = NULL;
			break;
		    }
		    sse *= 10;
		    if (sse > TIME_MAX - d)
		    {
			bp = NULL;
			break;
		    }
		    sse += d;
		}
		if (bp == NULL)
		    continue;

		{
		    struct tm *lt = localtime(&sse);

		    if (lt == NULL)
			bp = NULL;
		    else
		    {
			*tm = *lt;
			state |= S_YDAY | S_WDAY |
				S_MON | S_MDAY | S_YEAR;
		    }
		}
		continue;
	    }

	case 'U':	/* The week of year, beginning on sunday. */
	case 'W':	/* The week of year, beginning on monday. */
	    /*
	     * This is bogus, as we can not assume any valid
	     * information present in the tm structure at this
	     * point to calculate a real value, so save the
	     * week for now in case it can be used later.
	     */
	    bp = conv_num(bp, &i, 0, 53);
	    LEGAL_ALT(ALT_O);
	    if (c == 'U')
		day_offset = TM_SUNDAY;
	    else
		day_offset = TM_MONDAY;
	    week_offset = i;
	    continue;

	case 'w':	/* The day of week, beginning on sunday. */
	    bp = conv_num(bp, &tm->tm_wday, 0, 6);
	    LEGAL_ALT(ALT_O);
	    state |= S_WDAY;
	    continue;

	case 'u':	/* The day of week, monday = 1. */
	    bp = conv_num(bp, &i, 1, 7);
	    tm->tm_wday = i % 7;
	    LEGAL_ALT(ALT_O);
	    state |= S_WDAY;
	    continue;

	case 'g':	/* ISO week year without century (parsed, ignored). */
	    bp = conv_num(bp, &i, 0, 99);
	    continue;

	case 'G':	/* ISO week year with century (parsed, ignored). */
	    do
		bp++;
	    while (isdigit(*bp));
	    continue;

	case 'V':	/* ISO 8601:1988 week number (parsed, ignored). */
	    bp = conv_num(bp, &i, 1, 53);
	    continue;

	case 'Y':	/* The year. */
	    i = TM_YEAR_BASE;	/* just for data sanity... */
	    bp = conv_num(bp, &i, 0, 9999);
	    tm->tm_year = i - TM_YEAR_BASE;
	    LEGAL_ALT(ALT_E);
	    state |= S_YEAR;
	    continue;

	case 'y':	/* The year within 100 years of the epoch. */
	    /* LEGAL_ALT(ALT_E | ALT_O); */
	    bp = conv_num(bp, &i, 0, 99);

	    if (split_year)
		/* preserve century */
		i += (tm->tm_year / 100) * 100;
	    else
	    {
		split_year = 1;
		if (i <= 68)
		    i = i + 2000 - TM_YEAR_BASE;
		else
		    i = i + 1900 - TM_YEAR_BASE;
	    }
	    tm->tm_year = i;
	    state |= S_YEAR;
	    continue;

	case 'Z':
	case 'z':
	    tzset();
	    mandatory = c == 'z';
	    /*
	     * We recognize all ISO 8601 formats:
	     * Z	= Zulu time/UTC
	     * [+-]hhmm
	     * [+-]hh:mm
	     * [+-]hh
	     * We recognize all RFC-822/RFC-2822 formats:
	     * UT|GMT
	     *          North American : UTC offsets
	     * E[DS]T = Eastern : -4 | -5
	     * C[DS]T = Central : -5 | -6
	     * M[DS]T = Mountain: -6 | -7
	     * P[DS]T = Pacific : -7 | -8
	     *          Nautical/Military
	     * [A-IL-M] = -1 ... -9 (J not used)
	     * [N-Y]  = +1 ... +12
	     * Note: J maybe used to denote non-nautical
	     *       local time
	     */
	    if (mandatory)
		while (isspace(*bp))
		    bp++;

	    zname = bp;
	    switch (*bp++)
	    {
	    case 'G':
		if (*bp++ != 'M')
		    goto namedzone;
		/*FALLTHROUGH*/
	    case 'U':
		if (*bp++ != 'T')
		    goto namedzone;
		else if (!delim(*bp) && *bp++ != 'C')
		    goto namedzone;
		/*FALLTHROUGH*/
	    case 'Z':
		if (!delim(*bp))
		    goto namedzone;
		tm->tm_isdst = 0;
		continue;
	    case '+':
		neg = 0;
		break;
	    case '-':
		neg = 1;
		break;
	    default:
namedzone:
		bp = zname;

		/* Nautical / Military style */
		if (delim(bp[1]) &&
			((*bp >= 'A' && *bp <= 'I') ||
			 (*bp >= 'L' && *bp <= 'Y')))
		{
		    bp++;
		    continue;
		}
		/* 'J' is local time */
		if (delim(bp[1]) && *bp == 'J')
		{
		    bp++;
		    continue;
		}

		/*
		 * From our 3 letter hard-coded table.
		 */
		if (delim(bp[0]) || delim(bp[1]) ||
			delim(bp[2]) || !delim(bp[3]))
		    goto loadzone;
		ep = find_string(bp, &i, nast, NULL, 4);
		if (ep != NULL)
		{
		    bp = ep;
		    continue;
		}
		ep = find_string(bp, &i, nadt, NULL, 4);
		if (ep != NULL)
		{
		    tm->tm_isdst = 1;
		    bp = ep;
		    continue;
		}
		/*
		 * Our current timezone
		 */
		ep = find_string(bp, &i,
			(const char *const *)tzname, NULL, 2);
		if (ep != NULL)
		{
		    tm->tm_isdst = i;
		    bp = ep;
		    continue;
		}
loadzone:
		/*
		 * The hard way, load the zone!
		 */
		if (fromzone(&bp, tm, mandatory))
		    continue;
		goto out;
	    }
	    offs = 0;
	    for (i = 0; i < 4; )
	    {
		if (isdigit(*bp))
		{
		    offs = offs * 10 + (*bp++ - '0');
		    i++;
		    continue;
		}
		if (i == 2 && *bp == ':')
		{
		    bp++;
		    continue;
		}
		break;
	    }
	    if (isdigit(*bp))
		goto out;
	    switch (i)
	    {
	    case 2:
		offs *= SECSPERHOUR;
		break;
	    case 4:
		i = offs % 100;
		offs /= 100;
		if (i >= SECSPERMIN)
		    goto out;
		/* Convert minutes into decimal */
		offs = offs * SECSPERHOUR + i * SECSPERMIN;
		break;
	    default:
out:
		if (mandatory)
		    return NULL;
		bp = zname;
		continue;
	    }
	    /* ISO 8601 & RFC 3339 limit to 23:59 max */
	    if (offs >= (HOURSPERDAY * SECSPERHOUR))
		goto out;
	    if (neg)
		offs = -offs;
	    tm->tm_isdst = 0;	/* XXX */
	    continue;

	/*
	 * Miscellaneous conversions.
	 */
	case 'n':	/* Any kind of white-space. */
	case 't':
	    while (isspace(*bp))
		bp++;
	    LEGAL_ALT(0);
	    continue;


	default:	/* Unknown/unsupported conversion. */
	    return NULL;
	}
    }

    if (!HAVE_YDAY(state) && HAVE_YEAR(state))
    {
	if (HAVE_MON(state) && HAVE_MDAY(state))
	{
	    /* calculate day of year (ordinal date) */
	    tm->tm_yday =  start_of_month[isleap_sum(tm->tm_year,
			    TM_YEAR_BASE)][tm->tm_mon] + (tm->tm_mday - 1);
	    state |= S_YDAY;
	}
	else if (day_offset != -1)
	{
	    /*
	     * Set the date to the first Sunday (or Monday)
	     * of the specified week of the year.
	     */
	    if (!HAVE_WDAY(state))
	    {
		tm->tm_wday = day_offset;
		state |= S_WDAY;
	    }
	    tm->tm_yday = (7 -
		    first_wday_of(tm->tm_year + TM_YEAR_BASE) +
		    day_offset) % 7 + (week_offset - 1) * 7 +
		    tm->tm_wday  - day_offset;
	    state |= S_YDAY;
	}
    }

    if (HAVE_YDAY(state) && HAVE_YEAR(state))
    {
	int leap;

	if (!HAVE_MON(state))
	{
	    /* calculate month of day of year */
	    i = 0;
	    leap = isleap_sum(tm->tm_year, TM_YEAR_BASE);
	    while (tm->tm_yday >= start_of_month[leap][i])
		i++;
	    if (i > 12)
	    {
		i = 1;
		tm->tm_yday -= start_of_month[leap][12];
		tm->tm_year++;
	    }
	    tm->tm_mon = i - 1;
	    state |= S_MON;
	}

	if (!HAVE_MDAY(state))
	{
	    /* calculate day of month */
	    leap = isleap_sum(tm->tm_year, TM_YEAR_BASE);
	    tm->tm_mday = tm->tm_yday -
		start_of_month[leap][tm->tm_mon] + 1;
	    state |= S_MDAY;
	}

	if (!HAVE_WDAY(state))
	{
	    /* calculate day of week */
	    i = 0;
	    week_offset = first_wday_of(tm->tm_year);
	    while (i++ <= tm->tm_yday)
	    {
		if (week_offset++ >= 6)
		    week_offset = 0;
	    }
	    tm->tm_wday = week_offset;
	    state |= S_WDAY;
	}
    }

    return (char *)bp;
}


    static const unsigned char *
conv_num(const unsigned char *buf, int *dest, unsigned int llim,
						    unsigned int ulim)
{
    unsigned int result = 0;
    unsigned char ch;

    /* The limit also determines the number of valid digits. */
    unsigned int rulim = ulim;

    ch = *buf;
    if (ch < '0' || ch > '9')
	return NULL;

    do
    {
	result *= 10;
	result += ch - '0';
	rulim /= 10;
	ch = *++buf;
    } while ((result * 10 <= ulim) && rulim && ch >= '0' && ch <= '9');

    if (result < llim || result > ulim)
	return NULL;

    *dest = result;
    return buf;
}

    static const unsigned char *
find_string(const unsigned char *bp, int *tgt, const char *const *n1,
					    const char *const *n2, int c)
{
    int		i;
    size_t	len;

    /* check full name - then abbreviated ones */
    for (; n1 != NULL; n1 = n2, n2 = NULL)
    {
	for (i = 0; i < c; i++, n1++)
	{
	    len = strlen(*n1);
	    if (STRNICMP(*n1, bp, len) == 0)
	    {
		*tgt = i;
		return bp + len;
	    }
	}
    }

    /* Nothing matched */
    return NULL;
}

#endif // MSWIN
