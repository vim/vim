/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */
/*
 * os_mac_conv.c: Code specifically for Mac string conversions.
 *
 * This code has been put in a separate file to avoid the conflicts that are
 * caused by including both the X11 and Carbon header files.
 */

#define NO_X11_INCLUDES
#include "vim.h"

extern char_u *mac_string_convert __ARGS((char_u *ptr, int len, int *lenp, int fail_on_error, int from, int to, int *unconvlenp));
extern int macroman2enc __ARGS((char_u *ptr, long *sizep, long real_size));
extern int enc2macroman __ARGS((char_u *from, size_t fromlen, char_u *to, int *tolenp, int maxtolen, char_u *rest, int *restlenp));

/*
 * A Mac version of string_convert_ext() for special cases.
 */
    char_u *
mac_string_convert(ptr, len, lenp, fail_on_error, from_enc, to_enc, unconvlenp)
    char_u		*ptr;
    int			len;
    int			*lenp;
    int			fail_on_error;
    int			from_enc;
    int			to_enc;
    int			*unconvlenp;
{
    char_u		*retval, *d;
    CFStringRef		cfstr;
    int			buflen, in, out, l, i;
    CFStringEncoding	from;
    CFStringEncoding	to;

    switch (from_enc)
    {
	case 'l':   from = kCFStringEncodingISOLatin1; break;
	case 'm':   from = kCFStringEncodingMacRoman; break;
	case 'u':   from = kCFStringEncodingUTF8; break;
	default:    return NULL;
    }
    switch (to_enc)
    {
	case 'l':   to = kCFStringEncodingISOLatin1; break;
	case 'm':   to = kCFStringEncodingMacRoman; break;
	case 'u':   to = kCFStringEncodingUTF8; break;
	default:    return NULL;
    }

    if (unconvlenp != NULL)
	*unconvlenp = 0;
    cfstr = CFStringCreateWithBytes(NULL, ptr, len, from, 0);

    /* When conversion failed, try excluding bytes from the end, helps when
     * there is an incomplete byte sequence.  Only do up to 6 bytes to avoid
     * looping a long time when there really is something unconvertable. */
    while (cfstr == NULL && unconvlenp != NULL && len > 1 && *unconvlenp < 6)
    {
	--len;
	++*unconvlenp;
	cfstr = CFStringCreateWithBytes(NULL, ptr, len, from, 0);
    }
    if (cfstr == NULL)
	return NULL;
    if (to == kCFStringEncodingUTF8)
	buflen = len * 6 + 1;
    else
	buflen = len + 1;
    retval = alloc(buflen);
    if (retval == NULL)
    {
	CFRelease(cfstr);
	return NULL;
    }
    if (!CFStringGetCString(cfstr, retval, buflen, to))
    {
	CFRelease(cfstr);
	if (fail_on_error)
	{
	    vim_free(retval);
	    return NULL;
	}

	/* conversion failed for the whole string, but maybe it will work
	 * for each character */
	for (d = retval, in = 0, out = 0; in < len && out < buflen - 1;)
	{
	    if (from == kCFStringEncodingUTF8)
		l = utf_ptr2len_check(ptr + in);
	    else
		l = 1;
	    cfstr = CFStringCreateWithBytes(NULL, ptr + in, l, from, 0);
	    if (cfstr == NULL)
	    {
		*d++ = '?';
		out++;
	    }
	    else
	    {
		if (!CFStringGetCString(cfstr, d, buflen - out, to))
		{
		    *d++ = '?';
		    out++;
		}
		else
		{
		    i = strlen(d);
		    d += i;
		    out += i;
		}
		CFRelease(cfstr);
	    }
	    in += l;
	}
	*d = NUL;
	if (lenp != NULL)
	    *lenp = out;
	return retval;
    }
    CFRelease(cfstr);
    if (lenp != NULL)
	*lenp = strlen(retval);
    return retval;
}

/*
 * Conversion from Apple MacRoman char encoding to UTF-8 or latin1, using
 * standard Carbon framework.
 * Input: "ptr[*sizep]".
 * "real_size" is the size of the buffer that "ptr" points to.
 * output is in-place, "sizep" is adjusted.
 * Returns OK or FAIL.
 */
    int
macroman2enc(ptr, sizep, real_size)
    char_u	*ptr;
    long	*sizep;
    long	real_size;
{
    CFStringRef		cfstr;
    CFRange		r;
    CFIndex		len = *sizep;

    /* MacRoman is an 8-bit encoding, no need to move bytes to
     * conv_rest[]. */
    cfstr = CFStringCreateWithBytes(NULL, ptr, len,
						kCFStringEncodingMacRoman, 0);
    /*
     * If there is a conversion error, try using another
     * conversion.
     */
    if (cfstr == NULL)
	return FAIL;

    r.location = 0;
    r.length = CFStringGetLength(cfstr);
    if (r.length != CFStringGetBytes(cfstr, r,
	    (enc_utf8) ? kCFStringEncodingUTF8 : kCFStringEncodingISOLatin1,
	    0, /* no lossy conversion */
	    0, /* not external representation */
	    ptr + *sizep, real_size - *sizep, &len))
    {
	CFRelease(cfstr);
	return FAIL;
    }
    CFRelease(cfstr);
    mch_memmove(ptr, ptr + *sizep, len);
    *sizep = len;

    return OK;
}

/*
 * Conversion from UTF-8 or latin1 to MacRoman.
 * Input: "from[fromlen]"
 * Output: "to[maxtolen]" length in "*tolenp"
 * Unconverted rest in rest[*restlenp].
 * Returns OK or FAIL.
 */
    int
enc2macroman(from, fromlen, to, tolenp, maxtolen, rest, restlenp)
    char_u	*from;
    size_t	fromlen;
    char_u	*to;
    int		*tolenp;
    int		maxtolen;
    char_u	*rest;
    int		*restlenp;
{
    CFStringRef	cfstr;
    CFRange	r;
    CFIndex	l;

    *restlenp = 0;
    cfstr = CFStringCreateWithBytes(NULL, from, fromlen,
	    (enc_utf8) ? kCFStringEncodingUTF8 : kCFStringEncodingISOLatin1,
	    0);
    while (cfstr == NULL && *restlenp < 3 && fromlen > 1)
    {
	rest[*restlenp++] = from[--fromlen];
	cfstr = CFStringCreateWithBytes(NULL, from, fromlen,
		(enc_utf8) ? kCFStringEncodingUTF8 : kCFStringEncodingISOLatin1,
		0);
    }
    if (cfstr == NULL)
	return FAIL;

    r.location = 0;
    r.length = CFStringGetLength(cfstr);
    if (r.length != CFStringGetBytes(cfstr, r,
		kCFStringEncodingMacRoman,
		0, /* no lossy conversion */
		0, /* not external representation (since vim
		    * handles this internally */
		to, maxtolen, &l))
    {
	CFRelease(cfstr);
	return FAIL;
    }
    CFRelease(cfstr);
    *tolenp = l;
    return OK;
}
