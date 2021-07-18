/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * strings.c: string manipulation functions
 */

#include "vim.h"

/*
 * Copy "string" into newly allocated memory.
 */
    char_u *
vim_strsave(char_u *string)
{
    char_u	*p;
    size_t	len;

    len = STRLEN(string) + 1;
    p = alloc(len);
    if (p != NULL)
	mch_memmove(p, string, len);
    return p;
}

/*
 * Copy up to "len" bytes of "string" into newly allocated memory and
 * terminate with a NUL.
 * The allocated memory always has size "len + 1", also when "string" is
 * shorter.
 */
    char_u *
vim_strnsave(char_u *string, size_t len)
{
    char_u	*p;

    p = alloc(len + 1);
    if (p != NULL)
    {
	STRNCPY(p, string, len);
	p[len] = NUL;
    }
    return p;
}

/*
 * Same as vim_strsave(), but any characters found in esc_chars are preceded
 * by a backslash.
 */
    char_u *
vim_strsave_escaped(char_u *string, char_u *esc_chars)
{
    return vim_strsave_escaped_ext(string, esc_chars, '\\', FALSE);
}

/*
 * Same as vim_strsave_escaped(), but when "bsl" is TRUE also escape
 * characters where rem_backslash() would remove the backslash.
 * Escape the characters with "cc".
 */
    char_u *
vim_strsave_escaped_ext(
    char_u	*string,
    char_u	*esc_chars,
    int		cc,
    int		bsl)
{
    char_u	*p;
    char_u	*p2;
    char_u	*escaped_string;
    unsigned	length;
    int		l;

    /*
     * First count the number of backslashes required.
     * Then allocate the memory and insert them.
     */
    length = 1;				// count the trailing NUL
    for (p = string; *p; p++)
    {
	if (has_mbyte && (l = (*mb_ptr2len)(p)) > 1)
	{
	    length += l;		// count a multibyte char
	    p += l - 1;
	    continue;
	}
	if (vim_strchr(esc_chars, *p) != NULL || (bsl && rem_backslash(p)))
	    ++length;			// count a backslash
	++length;			// count an ordinary char
    }
    escaped_string = alloc(length);
    if (escaped_string != NULL)
    {
	p2 = escaped_string;
	for (p = string; *p; p++)
	{
	    if (has_mbyte && (l = (*mb_ptr2len)(p)) > 1)
	    {
		mch_memmove(p2, p, (size_t)l);
		p2 += l;
		p += l - 1;		// skip multibyte char
		continue;
	    }
	    if (vim_strchr(esc_chars, *p) != NULL || (bsl && rem_backslash(p)))
		*p2++ = cc;
	    *p2++ = *p;
	}
	*p2 = NUL;
    }
    return escaped_string;
}

/*
 * Return TRUE when 'shell' has "csh" in the tail.
 */
    int
csh_like_shell(void)
{
    return (strstr((char *)gettail(p_sh), "csh") != NULL);
}

/*
 * Escape "string" for use as a shell argument with system().
 * This uses single quotes, except when we know we need to use double quotes
 * (MS-DOS and MS-Windows not using PowerShell and without 'shellslash' set).
 * PowerShell also uses a novel escaping for enclosed single quotes - double
 * them up.
 * Escape a newline, depending on the 'shell' option.
 * When "do_special" is TRUE also replace "!", "%", "#" and things starting
 * with "<" like "<cfile>".
 * When "do_newline" is FALSE do not escape newline unless it is csh shell.
 * Returns the result in allocated memory, NULL if we have run out.
 */
    char_u *
vim_strsave_shellescape(char_u *string, int do_special, int do_newline)
{
    unsigned	length;
    char_u	*p;
    char_u	*d;
    char_u	*escaped_string;
    int		l;
    int		csh_like;
    char_u	*shname;
    int		powershell;
# ifdef MSWIN
    int		double_quotes;
# endif

    // Only csh and similar shells expand '!' within single quotes.  For sh and
    // the like we must not put a backslash before it, it will be taken
    // literally.  If do_special is set the '!' will be escaped twice.
    // Csh also needs to have "\n" escaped twice when do_special is set.
    csh_like = csh_like_shell();

    // PowerShell uses it's own version for quoting single quotes
    shname = gettail(p_sh);
    powershell = strstr((char *)shname, "pwsh") != NULL;
# ifdef MSWIN
    powershell = powershell || strstr((char *)shname, "powershell") != NULL;
    // PowerShell only accepts single quotes so override shellslash.
    double_quotes = !powershell && !p_ssl;
# endif

    // First count the number of extra bytes required.
    length = (unsigned)STRLEN(string) + 3;  // two quotes and a trailing NUL
    for (p = string; *p != NUL; MB_PTR_ADV(p))
    {
# ifdef MSWIN
	if (double_quotes)
	{
	    if (*p == '"')
		++length;		// " -> ""
	}
	else
# endif
	if (*p == '\'')
	{
	    if (powershell)
		length +=2;		// ' => ''
	    else
		length += 3;		// ' => '\''
	}
	if ((*p == '\n' && (csh_like || do_newline))
		|| (*p == '!' && (csh_like || do_special)))
	{
	    ++length;			// insert backslash
	    if (csh_like && do_special)
		++length;		// insert backslash
	}
	if (do_special && find_cmdline_var(p, &l) >= 0)
	{
	    ++length;			// insert backslash
	    p += l - 1;
	}
    }

    // Allocate memory for the result and fill it.
    escaped_string = alloc(length);
    if (escaped_string != NULL)
    {
	d = escaped_string;

	// add opening quote
# ifdef MSWIN
	if (double_quotes)
	    *d++ = '"';
	else
# endif
	    *d++ = '\'';

	for (p = string; *p != NUL; )
	{
# ifdef MSWIN
	    if (double_quotes)
	    {
		if (*p == '"')
		{
		    *d++ = '"';
		    *d++ = '"';
		    ++p;
		    continue;
		}
	    }
	    else
# endif
	    if (*p == '\'')
	    {
		if (powershell)
		{
		    *d++ = '\'';
		    *d++ = '\'';
		}
		else
		{
		    *d++ = '\'';
		    *d++ = '\\';
		    *d++ = '\'';
		    *d++ = '\'';
		}
		++p;
		continue;
	    }
	    if ((*p == '\n' && (csh_like || do_newline))
		    || (*p == '!' && (csh_like || do_special)))
	    {
		*d++ = '\\';
		if (csh_like && do_special)
		    *d++ = '\\';
		*d++ = *p++;
		continue;
	    }
	    if (do_special && find_cmdline_var(p, &l) >= 0)
	    {
		*d++ = '\\';		// insert backslash
		while (--l >= 0)	// copy the var
		    *d++ = *p++;
		continue;
	    }

	    MB_COPY_CHAR(p, d);
	}

	// add terminating quote and finish with a NUL
# ifdef MSWIN
	if (double_quotes)
	    *d++ = '"';
	else
# endif
	    *d++ = '\'';
	*d = NUL;
    }

    return escaped_string;
}

/*
 * Like vim_strsave(), but make all characters uppercase.
 * This uses ASCII lower-to-upper case translation, language independent.
 */
    char_u *
vim_strsave_up(char_u *string)
{
    char_u *p1;

    p1 = vim_strsave(string);
    vim_strup(p1);
    return p1;
}

/*
 * Like vim_strnsave(), but make all characters uppercase.
 * This uses ASCII lower-to-upper case translation, language independent.
 */
    char_u *
vim_strnsave_up(char_u *string, size_t len)
{
    char_u *p1;

    p1 = vim_strnsave(string, len);
    vim_strup(p1);
    return p1;
}

/*
 * ASCII lower-to-upper case translation, language independent.
 */
    void
vim_strup(
    char_u	*p)
{
    char_u  *p2;
    int	    c;

    if (p != NULL)
    {
	p2 = p;
	while ((c = *p2) != NUL)
#ifdef EBCDIC
	    *p2++ = isalpha(c) ? toupper(c) : c;
#else
	    *p2++ = (c < 'a' || c > 'z') ? c : (c - 0x20);
#endif
    }
}

#if defined(FEAT_EVAL) || defined(FEAT_SPELL) || defined(PROTO)
/*
 * Make string "s" all upper-case and return it in allocated memory.
 * Handles multi-byte characters as well as possible.
 * Returns NULL when out of memory.
 */
    static char_u *
strup_save(char_u *orig)
{
    char_u	*p;
    char_u	*res;

    res = p = vim_strsave(orig);

    if (res != NULL)
	while (*p != NUL)
	{
	    int		l;

	    if (enc_utf8)
	    {
		int	c, uc;
		int	newl;
		char_u	*s;

		c = utf_ptr2char(p);
		l = utf_ptr2len(p);
		if (c == 0)
		{
		    // overlong sequence, use only the first byte
		    c = *p;
		    l = 1;
		}
		uc = utf_toupper(c);

		// Reallocate string when byte count changes.  This is rare,
		// thus it's OK to do another malloc()/free().
		newl = utf_char2len(uc);
		if (newl != l)
		{
		    s = alloc(STRLEN(res) + 1 + newl - l);
		    if (s == NULL)
		    {
			vim_free(res);
			return NULL;
		    }
		    mch_memmove(s, res, p - res);
		    STRCPY(s + (p - res) + newl, p + l);
		    p = s + (p - res);
		    vim_free(res);
		    res = s;
		}

		utf_char2bytes(uc, p);
		p += newl;
	    }
	    else if (has_mbyte && (l = (*mb_ptr2len)(p)) > 1)
		p += l;		// skip multi-byte character
	    else
	    {
		*p = TOUPPER_LOC(*p); // note that toupper() can be a macro
		p++;
	    }
	}

    return res;
}

/*
 * Make string "s" all lower-case and return it in allocated memory.
 * Handles multi-byte characters as well as possible.
 * Returns NULL when out of memory.
 */
    char_u *
strlow_save(char_u *orig)
{
    char_u	*p;
    char_u	*res;

    res = p = vim_strsave(orig);

    if (res != NULL)
	while (*p != NUL)
	{
	    int		l;

	    if (enc_utf8)
	    {
		int	c, lc;
		int	newl;
		char_u	*s;

		c = utf_ptr2char(p);
		l = utf_ptr2len(p);
		if (c == 0)
		{
		    // overlong sequence, use only the first byte
		    c = *p;
		    l = 1;
		}
		lc = utf_tolower(c);

		// Reallocate string when byte count changes.  This is rare,
		// thus it's OK to do another malloc()/free().
		newl = utf_char2len(lc);
		if (newl != l)
		{
		    s = alloc(STRLEN(res) + 1 + newl - l);
		    if (s == NULL)
		    {
			vim_free(res);
			return NULL;
		    }
		    mch_memmove(s, res, p - res);
		    STRCPY(s + (p - res) + newl, p + l);
		    p = s + (p - res);
		    vim_free(res);
		    res = s;
		}

		utf_char2bytes(lc, p);
		p += newl;
	    }
	    else if (has_mbyte && (l = (*mb_ptr2len)(p)) > 1)
		p += l;		// skip multi-byte character
	    else
	    {
		*p = TOLOWER_LOC(*p); // note that tolower() can be a macro
		p++;
	    }
	}

    return res;
}
#endif

/*
 * delete spaces at the end of a string
 */
    void
del_trailing_spaces(char_u *ptr)
{
    char_u	*q;

    q = ptr + STRLEN(ptr);
    while (--q > ptr && VIM_ISWHITE(q[0]) && q[-1] != '\\' && q[-1] != Ctrl_V)
	*q = NUL;
}

/*
 * Like strncpy(), but always terminate the result with one NUL.
 * "to" must be "len + 1" long!
 */
    void
vim_strncpy(char_u *to, char_u *from, size_t len)
{
    STRNCPY(to, from, len);
    to[len] = NUL;
}

/*
 * Like strcat(), but make sure the result fits in "tosize" bytes and is
 * always NUL terminated. "from" and "to" may overlap.
 */
    void
vim_strcat(char_u *to, char_u *from, size_t tosize)
{
    size_t tolen = STRLEN(to);
    size_t fromlen = STRLEN(from);

    if (tolen + fromlen + 1 > tosize)
    {
	mch_memmove(to + tolen, from, tosize - tolen - 1);
	to[tosize - 1] = NUL;
    }
    else
	mch_memmove(to + tolen, from, fromlen + 1);
}

#if (!defined(HAVE_STRCASECMP) && !defined(HAVE_STRICMP)) || defined(PROTO)
/*
 * Compare two strings, ignoring case, using current locale.
 * Doesn't work for multi-byte characters.
 * return 0 for match, < 0 for smaller, > 0 for bigger
 */
    int
vim_stricmp(char *s1, char *s2)
{
    int		i;

    for (;;)
    {
	i = (int)TOLOWER_LOC(*s1) - (int)TOLOWER_LOC(*s2);
	if (i != 0)
	    return i;			    // this character different
	if (*s1 == NUL)
	    break;			    // strings match until NUL
	++s1;
	++s2;
    }
    return 0;				    // strings match
}
#endif

#if (!defined(HAVE_STRNCASECMP) && !defined(HAVE_STRNICMP)) || defined(PROTO)
/*
 * Compare two strings, for length "len", ignoring case, using current locale.
 * Doesn't work for multi-byte characters.
 * return 0 for match, < 0 for smaller, > 0 for bigger
 */
    int
vim_strnicmp(char *s1, char *s2, size_t len)
{
    int		i;

    while (len > 0)
    {
	i = (int)TOLOWER_LOC(*s1) - (int)TOLOWER_LOC(*s2);
	if (i != 0)
	    return i;			    // this character different
	if (*s1 == NUL)
	    break;			    // strings match until NUL
	++s1;
	++s2;
	--len;
    }
    return 0;				    // strings match
}
#endif

/*
 * Search for first occurrence of "c" in "string".
 * Version of strchr() that handles unsigned char strings with characters from
 * 128 to 255 correctly.  It also doesn't return a pointer to the NUL at the
 * end of the string.
 */
    char_u  *
vim_strchr(char_u *string, int c)
{
    char_u	*p;
    int		b;

    p = string;
    if (enc_utf8 && c >= 0x80)
    {
	while (*p != NUL)
	{
	    int l = utfc_ptr2len(p);

	    // Avoid matching an illegal byte here.
	    if (utf_ptr2char(p) == c && l > 1)
		return p;
	    p += l;
	}
	return NULL;
    }
    if (enc_dbcs != 0 && c > 255)
    {
	int	n2 = c & 0xff;

	c = ((unsigned)c >> 8) & 0xff;
	while ((b = *p) != NUL)
	{
	    if (b == c && p[1] == n2)
		return p;
	    p += (*mb_ptr2len)(p);
	}
	return NULL;
    }
    if (has_mbyte)
    {
	while ((b = *p) != NUL)
	{
	    if (b == c)
		return p;
	    p += (*mb_ptr2len)(p);
	}
	return NULL;
    }
    while ((b = *p) != NUL)
    {
	if (b == c)
	    return p;
	++p;
    }
    return NULL;
}

/*
 * Version of strchr() that only works for bytes and handles unsigned char
 * strings with characters above 128 correctly. It also doesn't return a
 * pointer to the NUL at the end of the string.
 */
    char_u  *
vim_strbyte(char_u *string, int c)
{
    char_u	*p = string;

    while (*p != NUL)
    {
	if (*p == c)
	    return p;
	++p;
    }
    return NULL;
}

/*
 * Search for last occurrence of "c" in "string".
 * Version of strrchr() that handles unsigned char strings with characters from
 * 128 to 255 correctly.  It also doesn't return a pointer to the NUL at the
 * end of the string.
 * Return NULL if not found.
 * Does not handle multi-byte char for "c"!
 */
    char_u  *
vim_strrchr(char_u *string, int c)
{
    char_u	*retval = NULL;
    char_u	*p = string;

    while (*p)
    {
	if (*p == c)
	    retval = p;
	MB_PTR_ADV(p);
    }
    return retval;
}

/*
 * Vim's version of strpbrk(), in case it's missing.
 * Don't generate a prototype for this, causes problems when it's not used.
 */
#ifndef PROTO
# ifndef HAVE_STRPBRK
#  ifdef vim_strpbrk
#   undef vim_strpbrk
#  endif
    char_u *
vim_strpbrk(char_u *s, char_u *charset)
{
    while (*s)
    {
	if (vim_strchr(charset, *s) != NULL)
	    return s;
	MB_PTR_ADV(s);
    }
    return NULL;
}
# endif
#endif

/*
 * Sort an array of strings.
 */
static int sort_compare(const void *s1, const void *s2);

    static int
sort_compare(const void *s1, const void *s2)
{
    return STRCMP(*(char **)s1, *(char **)s2);
}

    void
sort_strings(
    char_u	**files,
    int		count)
{
    qsort((void *)files, (size_t)count, sizeof(char_u *), sort_compare);
}

#if defined(FEAT_QUICKFIX) || defined(FEAT_SPELL) || defined(PROTO)
/*
 * Return TRUE if string "s" contains a non-ASCII character (128 or higher).
 * When "s" is NULL FALSE is returned.
 */
    int
has_non_ascii(char_u *s)
{
    char_u	*p;

    if (s != NULL)
	for (p = s; *p != NUL; ++p)
	    if (*p >= 128)
		return TRUE;
    return FALSE;
}
#endif

/*
 * Concatenate two strings and return the result in allocated memory.
 * Returns NULL when out of memory.
 */
    char_u  *
concat_str(char_u *str1, char_u *str2)
{
    char_u  *dest;
    size_t  l = str1 == NULL ? 0 : STRLEN(str1);

    dest = alloc(l + (str2 == NULL ? 0 : STRLEN(str2)) + 1L);
    if (dest != NULL)
    {
	if (str1 == NULL)
	    *dest = NUL;
	else
	    STRCPY(dest, str1);
	if (str2 != NULL)
	    STRCPY(dest + l, str2);
    }
    return dest;
}

#if defined(FEAT_EVAL) || defined(PROTO)

/*
 * Return string "str" in ' quotes, doubling ' characters.
 * If "str" is NULL an empty string is assumed.
 * If "function" is TRUE make it function('string').
 */
    char_u *
string_quote(char_u *str, int function)
{
    unsigned	len;
    char_u	*p, *r, *s;

    len = (function ? 13 : 3);
    if (str != NULL)
    {
	len += (unsigned)STRLEN(str);
	for (p = str; *p != NUL; MB_PTR_ADV(p))
	    if (*p == '\'')
		++len;
    }
    s = r = alloc(len);
    if (r != NULL)
    {
	if (function)
	{
	    STRCPY(r, "function('");
	    r += 10;
	}
	else
	    *r++ = '\'';
	if (str != NULL)
	    for (p = str; *p != NUL; )
	    {
		if (*p == '\'')
		    *r++ = '\'';
		MB_COPY_CHAR(p, r);
	    }
	*r++ = '\'';
	if (function)
	    *r++ = ')';
	*r++ = NUL;
    }
    return s;
}

    static void
byteidx(typval_T *argvars, typval_T *rettv, int comp UNUSED)
{
    char_u	*t;
    char_u	*str;
    varnumber_T	idx;

    rettv->vval.v_number = -1;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_number_arg(argvars, 1) == FAIL))
	return;

    str = tv_get_string_chk(&argvars[0]);
    idx = tv_get_number_chk(&argvars[1], NULL);
    if (str == NULL || idx < 0)
	return;

    t = str;
    for ( ; idx > 0; idx--)
    {
	if (*t == NUL)		// EOL reached
	    return;
	if (enc_utf8 && comp)
	    t += utf_ptr2len(t);
	else
	    t += (*mb_ptr2len)(t);
    }
    rettv->vval.v_number = (varnumber_T)(t - str);
}

/*
 * "byteidx()" function
 */
    void
f_byteidx(typval_T *argvars, typval_T *rettv)
{
    byteidx(argvars, rettv, FALSE);
}

/*
 * "byteidxcomp()" function
 */
    void
f_byteidxcomp(typval_T *argvars, typval_T *rettv)
{
    byteidx(argvars, rettv, TRUE);
}

/*
 * "charidx()" function
 */
    void
f_charidx(typval_T *argvars, typval_T *rettv)
{
    char_u	*str;
    varnumber_T	idx;
    varnumber_T	countcc = FALSE;
    char_u	*p;
    int		len;
    int		(*ptr2len)(char_u *);

    rettv->vval.v_number = -1;

    if (argvars[0].v_type != VAR_STRING || argvars[1].v_type != VAR_NUMBER
	    || (argvars[2].v_type != VAR_UNKNOWN
					   && argvars[2].v_type != VAR_NUMBER
					   && argvars[2].v_type != VAR_BOOL))
    {
	emsg(_(e_invarg));
	return;
    }

    str = tv_get_string_chk(&argvars[0]);
    idx = tv_get_number_chk(&argvars[1], NULL);
    if (str == NULL || idx < 0)
	return;

    if (argvars[2].v_type != VAR_UNKNOWN)
	countcc = tv_get_bool(&argvars[2]);
    if (countcc < 0 || countcc > 1)
    {
	semsg(_(e_using_number_as_bool_nr), countcc);
	return;
    }

    if (enc_utf8 && countcc)
	ptr2len = utf_ptr2len;
    else
	ptr2len = mb_ptr2len;

    for (p = str, len = 0; p <= str + idx; len++)
    {
	if (*p == NUL)
	    return;
	p += ptr2len(p);
    }

    rettv->vval.v_number = len > 0 ? len - 1 : 0;
}

/*
 * "str2list()" function
 */
    void
f_str2list(typval_T *argvars, typval_T *rettv)
{
    char_u	*p;
    int		utf8 = FALSE;

    if (rettv_list_alloc(rettv) == FAIL)
	return;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_opt_bool_arg(argvars, 1) == FAIL))
	return;

    if (argvars[1].v_type != VAR_UNKNOWN)
	utf8 = (int)tv_get_bool_chk(&argvars[1], NULL);

    p = tv_get_string(&argvars[0]);

    if (has_mbyte || utf8)
    {
	int (*ptr2len)(char_u *);
	int (*ptr2char)(char_u *);

	if (utf8 || enc_utf8)
	{
	    ptr2len = utf_ptr2len;
	    ptr2char = utf_ptr2char;
	}
	else
	{
	    ptr2len = mb_ptr2len;
	    ptr2char = mb_ptr2char;
	}

	for ( ; *p != NUL; p += (*ptr2len)(p))
	    list_append_number(rettv->vval.v_list, (*ptr2char)(p));
    }
    else
	for ( ; *p != NUL; ++p)
	    list_append_number(rettv->vval.v_list, *p);
}

/*
 * "str2nr()" function
 */
    void
f_str2nr(typval_T *argvars, typval_T *rettv)
{
    int		base = 10;
    char_u	*p;
    varnumber_T	n;
    int		what = 0;
    int		isneg;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	base = (int)tv_get_number(&argvars[1]);
	if (base != 2 && base != 8 && base != 10 && base != 16)
	{
	    emsg(_(e_invarg));
	    return;
	}
	if (argvars[2].v_type != VAR_UNKNOWN && tv_get_bool(&argvars[2]))
	    what |= STR2NR_QUOTE;
    }

    p = skipwhite(tv_get_string_strict(&argvars[0]));
    isneg = (*p == '-');
    if (*p == '+' || *p == '-')
	p = skipwhite(p + 1);
    switch (base)
    {
	case 2: what |= STR2NR_BIN + STR2NR_FORCE; break;
	case 8: what |= STR2NR_OCT + STR2NR_OOCT + STR2NR_FORCE; break;
	case 16: what |= STR2NR_HEX + STR2NR_FORCE; break;
    }
    vim_str2nr(p, NULL, NULL, what, &n, NULL, 0, FALSE);
    // Text after the number is silently ignored.
    if (isneg)
	rettv->vval.v_number = -n;
    else
	rettv->vval.v_number = n;

}

/*
 * "strgetchar()" function
 */
    void
f_strgetchar(typval_T *argvars, typval_T *rettv)
{
    char_u	*str;
    int		len;
    int		error = FALSE;
    int		charidx;
    int		byteidx = 0;

    rettv->vval.v_number = -1;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_number_arg(argvars, 1) == FAIL))
	return;

    str = tv_get_string_chk(&argvars[0]);
    if (str == NULL)
	return;
    len = (int)STRLEN(str);
    charidx = (int)tv_get_number_chk(&argvars[1], &error);
    if (error)
	return;

    while (charidx >= 0 && byteidx < len)
    {
	if (charidx == 0)
	{
	    rettv->vval.v_number = mb_ptr2char(str + byteidx);
	    break;
	}
	--charidx;
	byteidx += MB_CPTR2LEN(str + byteidx);
    }
}

/*
 * "stridx()" function
 */
    void
f_stridx(typval_T *argvars, typval_T *rettv)
{
    char_u	buf[NUMBUFLEN];
    char_u	*needle;
    char_u	*haystack;
    char_u	*save_haystack;
    char_u	*pos;
    int		start_idx;

    needle = tv_get_string_chk(&argvars[1]);
    save_haystack = haystack = tv_get_string_buf_chk(&argvars[0], buf);
    rettv->vval.v_number = -1;
    if (needle == NULL || haystack == NULL)
	return;		// type error; errmsg already given

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	int	    error = FALSE;

	start_idx = (int)tv_get_number_chk(&argvars[2], &error);
	if (error || start_idx >= (int)STRLEN(haystack))
	    return;
	if (start_idx >= 0)
	    haystack += start_idx;
    }

    pos	= (char_u *)strstr((char *)haystack, (char *)needle);
    if (pos != NULL)
	rettv->vval.v_number = (varnumber_T)(pos - save_haystack);
}

/*
 * "string()" function
 */
    void
f_string(typval_T *argvars, typval_T *rettv)
{
    char_u	*tofree;
    char_u	numbuf[NUMBUFLEN];

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = tv2string(&argvars[0], &tofree, numbuf,
								get_copyID());
    // Make a copy if we have a value but it's not in allocated memory.
    if (rettv->vval.v_string != NULL && tofree == NULL)
	rettv->vval.v_string = vim_strsave(rettv->vval.v_string);
}

/*
 * "strlen()" function
 */
    void
f_strlen(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = (varnumber_T)(STRLEN(
					      tv_get_string(&argvars[0])));
}

    static void
strchar_common(typval_T *argvars, typval_T *rettv, int skipcc)
{
    char_u		*s = tv_get_string(&argvars[0]);
    varnumber_T		len = 0;
    int			(*func_mb_ptr2char_adv)(char_u **pp);

    func_mb_ptr2char_adv = skipcc ? mb_ptr2char_adv : mb_cptr2char_adv;
    while (*s != NUL)
    {
	func_mb_ptr2char_adv(&s);
	++len;
    }
    rettv->vval.v_number = len;
}

/*
 * "strcharlen()" function
 */
    void
f_strcharlen(typval_T *argvars, typval_T *rettv)
{
    strchar_common(argvars, rettv, TRUE);
}

/*
 * "strchars()" function
 */
    void
f_strchars(typval_T *argvars, typval_T *rettv)
{
    varnumber_T		skipcc = FALSE;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_opt_bool_arg(argvars, 1) == FAIL))
	return;

    if (argvars[1].v_type != VAR_UNKNOWN)
	skipcc = tv_get_bool(&argvars[1]);
    if (skipcc < 0 || skipcc > 1)
	semsg(_(e_using_number_as_bool_nr), skipcc);
    else
	strchar_common(argvars, rettv, skipcc);
}

/*
 * "strdisplaywidth()" function
 */
    void
f_strdisplaywidth(typval_T *argvars, typval_T *rettv)
{
    char_u	*s;
    int		col = 0;

    rettv->vval.v_number = -1;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_opt_number_arg(argvars, 1) == FAIL))
	return;

    s = tv_get_string(&argvars[0]);
    if (argvars[1].v_type != VAR_UNKNOWN)
	col = (int)tv_get_number(&argvars[1]);

    rettv->vval.v_number = (varnumber_T)(linetabsize_col(col, s) - col);
}

/*
 * "strwidth()" function
 */
    void
f_strwidth(typval_T *argvars, typval_T *rettv)
{
    char_u	*s = tv_get_string_strict(&argvars[0]);

    rettv->vval.v_number = (varnumber_T)(mb_string2cells(s, -1));
}

/*
 * "strcharpart()" function
 */
    void
f_strcharpart(typval_T *argvars, typval_T *rettv)
{
    char_u	*p;
    int		nchar;
    int		nbyte = 0;
    int		charlen;
    int		skipcc = FALSE;
    int		len = 0;
    int		slen;
    int		error = FALSE;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_number_arg(argvars, 1) == FAIL
		|| check_for_opt_number_arg(argvars, 2) == FAIL
		|| (argvars[2].v_type != VAR_UNKNOWN
		    && check_for_opt_bool_arg(argvars, 3) == FAIL)))
	return;

    p = tv_get_string(&argvars[0]);
    slen = (int)STRLEN(p);

    nchar = (int)tv_get_number_chk(&argvars[1], &error);
    if (!error)
    {
	if (argvars[2].v_type != VAR_UNKNOWN
					   && argvars[3].v_type != VAR_UNKNOWN)
	{
	    skipcc = tv_get_bool(&argvars[3]);
	    if (skipcc < 0 || skipcc > 1)
	    {
		semsg(_(e_using_number_as_bool_nr), skipcc);
		return;
	    }
	}

	if (nchar > 0)
	    while (nchar > 0 && nbyte < slen)
	    {
		if (skipcc)
		    nbyte += mb_ptr2len(p + nbyte);
		else
		    nbyte += MB_CPTR2LEN(p + nbyte);
		--nchar;
	    }
	else
	    nbyte = nchar;
	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    charlen = (int)tv_get_number(&argvars[2]);
	    while (charlen > 0 && nbyte + len < slen)
	    {
		int off = nbyte + len;

		if (off < 0)
		    len += 1;
		else
		{
		    if (skipcc)
			len += mb_ptr2len(p + off);
		    else
			len += MB_CPTR2LEN(p + off);
		}
		--charlen;
	    }
	}
	else
	    len = slen - nbyte;    // default: all bytes that are available.
    }

    /*
     * Only return the overlap between the specified part and the actual
     * string.
     */
    if (nbyte < 0)
    {
	len += nbyte;
	nbyte = 0;
    }
    else if (nbyte > slen)
	nbyte = slen;
    if (len < 0)
	len = 0;
    else if (nbyte + len > slen)
	len = slen - nbyte;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_strnsave(p + nbyte, len);
}

/*
 * "strpart()" function
 */
    void
f_strpart(typval_T *argvars, typval_T *rettv)
{
    char_u	*p;
    int		n;
    int		len;
    int		slen;
    int		error = FALSE;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_number_arg(argvars, 1) == FAIL
		|| check_for_opt_number_arg(argvars, 2) == FAIL
		|| (argvars[2].v_type != VAR_UNKNOWN
		    && check_for_opt_bool_arg(argvars, 3) == FAIL)))
	return;

    p = tv_get_string(&argvars[0]);
    slen = (int)STRLEN(p);

    n = (int)tv_get_number_chk(&argvars[1], &error);
    if (error)
	len = 0;
    else if (argvars[2].v_type != VAR_UNKNOWN)
	len = (int)tv_get_number(&argvars[2]);
    else
	len = slen - n;	    // default len: all bytes that are available.

    // Only return the overlap between the specified part and the actual
    // string.
    if (n < 0)
    {
	len += n;
	n = 0;
    }
    else if (n > slen)
	n = slen;
    if (len < 0)
	len = 0;
    else if (n + len > slen)
	len = slen - n;

    if (argvars[2].v_type != VAR_UNKNOWN && argvars[3].v_type != VAR_UNKNOWN)
    {
	int off;

	// length in characters
	for (off = n; off < slen && len > 0; --len)
	    off += mb_ptr2len(p + off);
	len = off - n;
    }

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_strnsave(p + n, len);
}

/*
 * "strridx()" function
 */
    void
f_strridx(typval_T *argvars, typval_T *rettv)
{
    char_u	buf[NUMBUFLEN];
    char_u	*needle;
    char_u	*haystack;
    char_u	*rest;
    char_u	*lastmatch = NULL;
    int		haystack_len, end_idx;

    needle = tv_get_string_chk(&argvars[1]);
    haystack = tv_get_string_buf_chk(&argvars[0], buf);

    rettv->vval.v_number = -1;
    if (needle == NULL || haystack == NULL)
	return;		// type error; errmsg already given

    haystack_len = (int)STRLEN(haystack);
    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	// Third argument: upper limit for index
	end_idx = (int)tv_get_number_chk(&argvars[2], NULL);
	if (end_idx < 0)
	    return;	// can never find a match
    }
    else
	end_idx = haystack_len;

    if (*needle == NUL)
    {
	// Empty string matches past the end.
	lastmatch = haystack + end_idx;
    }
    else
    {
	for (rest = haystack; *rest != '\0'; ++rest)
	{
	    rest = (char_u *)strstr((char *)rest, (char *)needle);
	    if (rest == NULL || rest > haystack + end_idx)
		break;
	    lastmatch = rest;
	}
    }

    if (lastmatch == NULL)
	rettv->vval.v_number = -1;
    else
	rettv->vval.v_number = (varnumber_T)(lastmatch - haystack);
}

/*
 * "strtrans()" function
 */
    void
f_strtrans(typval_T *argvars, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = transstr(tv_get_string(&argvars[0]));
}

/*
 * "tolower(string)" function
 */
    void
f_tolower(typval_T *argvars, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = strlow_save(tv_get_string(&argvars[0]));
}

/*
 * "toupper(string)" function
 */
    void
f_toupper(typval_T *argvars, typval_T *rettv)
{
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = strup_save(tv_get_string(&argvars[0]));
}

/*
 * "tr(string, fromstr, tostr)" function
 */
    void
f_tr(typval_T *argvars, typval_T *rettv)
{
    char_u	*in_str;
    char_u	*fromstr;
    char_u	*tostr;
    char_u	*p;
    int		inlen;
    int		fromlen;
    int		tolen;
    int		idx;
    char_u	*cpstr;
    int		cplen;
    int		first = TRUE;
    char_u	buf[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    garray_T	ga;

    in_str = tv_get_string(&argvars[0]);
    fromstr = tv_get_string_buf_chk(&argvars[1], buf);
    tostr = tv_get_string_buf_chk(&argvars[2], buf2);

    // Default return value: empty string.
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
    if (fromstr == NULL || tostr == NULL)
	    return;		// type error; errmsg already given
    ga_init2(&ga, (int)sizeof(char), 80);

    if (!has_mbyte)
	// not multi-byte: fromstr and tostr must be the same length
	if (STRLEN(fromstr) != STRLEN(tostr))
	{
error:
	    semsg(_(e_invarg2), fromstr);
	    ga_clear(&ga);
	    return;
	}

    // fromstr and tostr have to contain the same number of chars
    while (*in_str != NUL)
    {
	if (has_mbyte)
	{
	    inlen = (*mb_ptr2len)(in_str);
	    cpstr = in_str;
	    cplen = inlen;
	    idx = 0;
	    for (p = fromstr; *p != NUL; p += fromlen)
	    {
		fromlen = (*mb_ptr2len)(p);
		if (fromlen == inlen && STRNCMP(in_str, p, inlen) == 0)
		{
		    for (p = tostr; *p != NUL; p += tolen)
		    {
			tolen = (*mb_ptr2len)(p);
			if (idx-- == 0)
			{
			    cplen = tolen;
			    cpstr = p;
			    break;
			}
		    }
		    if (*p == NUL)	// tostr is shorter than fromstr
			goto error;
		    break;
		}
		++idx;
	    }

	    if (first && cpstr == in_str)
	    {
		// Check that fromstr and tostr have the same number of
		// (multi-byte) characters.  Done only once when a character
		// of in_str doesn't appear in fromstr.
		first = FALSE;
		for (p = tostr; *p != NUL; p += tolen)
		{
		    tolen = (*mb_ptr2len)(p);
		    --idx;
		}
		if (idx != 0)
		    goto error;
	    }

	    (void)ga_grow(&ga, cplen);
	    mch_memmove((char *)ga.ga_data + ga.ga_len, cpstr, (size_t)cplen);
	    ga.ga_len += cplen;

	    in_str += inlen;
	}
	else
	{
	    // When not using multi-byte chars we can do it faster.
	    p = vim_strchr(fromstr, *in_str);
	    if (p != NULL)
		ga_append(&ga, tostr[p - fromstr]);
	    else
		ga_append(&ga, *in_str);
	    ++in_str;
	}
    }

    // add a terminating NUL
    (void)ga_grow(&ga, 1);
    ga_append(&ga, NUL);

    rettv->vval.v_string = ga.ga_data;
}

/*
 * "trim({expr})" function
 */
    void
f_trim(typval_T *argvars, typval_T *rettv)
{
    char_u	buf1[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    char_u	*head = tv_get_string_buf_chk(&argvars[0], buf1);
    char_u	*mask = NULL;
    char_u	*tail;
    char_u	*prev;
    char_u	*p;
    int		c1;
    int		dir = 0;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;
    if (head == NULL)
	return;

    if (argvars[1].v_type != VAR_UNKNOWN && argvars[1].v_type != VAR_STRING)
    {
	semsg(_(e_invarg2), tv_get_string(&argvars[1]));
	return;
    }

    if (argvars[1].v_type == VAR_STRING)
    {
	mask = tv_get_string_buf_chk(&argvars[1], buf2);

	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    int	error = 0;

	    // leading or trailing characters to trim
	    dir = (int)tv_get_number_chk(&argvars[2], &error);
	    if (error)
		return;
	    if (dir < 0 || dir > 2)
	    {
		semsg(_(e_invarg2), tv_get_string(&argvars[2]));
		return;
	    }
	}
    }

    if (dir == 0 || dir == 1)
    {
	// Trim leading characters
	while (*head != NUL)
	{
	    c1 = PTR2CHAR(head);
	    if (mask == NULL)
	    {
		if (c1 > ' ' && c1 != 0xa0)
		    break;
	    }
	    else
	    {
		for (p = mask; *p != NUL; MB_PTR_ADV(p))
		    if (c1 == PTR2CHAR(p))
			break;
		if (*p == NUL)
		    break;
	    }
	    MB_PTR_ADV(head);
	}
    }

    tail = head + STRLEN(head);
    if (dir == 0 || dir == 2)
    {
	// Trim trailing characters
	for (; tail > head; tail = prev)
	{
	    prev = tail;
	    MB_PTR_BACK(head, prev);
	    c1 = PTR2CHAR(prev);
	    if (mask == NULL)
	    {
		if (c1 > ' ' && c1 != 0xa0)
		    break;
	    }
	    else
	    {
		for (p = mask; *p != NUL; MB_PTR_ADV(p))
		    if (c1 == PTR2CHAR(p))
			break;
		if (*p == NUL)
		    break;
	    }
	}
    }
    rettv->vval.v_string = vim_strnsave(head, tail - head);
}

#endif
