/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved    by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * arabic.c: functions for Arabic language
 *
 * Author: Nadim Shaikli & Isam Bayazidi
 */

#include "vim.h"

#if defined(FEAT_ARABIC) || defined(PROTO)

static int  A_firstc_laa(int c1, int c);
static int  A_is_harakat(int c);
static int  A_is_iso(int c);
static int  A_is_formb(int c);
static int  A_is_ok(int c);
static int  A_is_valid(int c);
static int  A_is_special(int c);


/*
 * Returns True if c is an ISO-8859-6 shaped ARABIC letter (user entered)
 */
    static int
A_is_a(int cur_c)
{
    switch (cur_c)
    {
	case a_HAMZA:
	case a_ALEF_MADDA:
	case a_ALEF_HAMZA_ABOVE:
	case a_WAW_HAMZA:
	case a_ALEF_HAMZA_BELOW:
	case a_YEH_HAMZA:
	case a_ALEF:
	case a_BEH:
	case a_TEH_MARBUTA:
	case a_TEH:
	case a_THEH:
	case a_JEEM:
	case a_HAH:
	case a_KHAH:
	case a_DAL:
	case a_THAL:
	case a_REH:
	case a_ZAIN:
	case a_SEEN:
	case a_SHEEN:
	case a_SAD:
	case a_DAD:
	case a_TAH:
	case a_ZAH:
	case a_AIN:
	case a_GHAIN:
	case a_TATWEEL:
	case a_FEH:
	case a_QAF:
	case a_KAF:
	case a_LAM:
	case a_MEEM:
	case a_NOON:
	case a_HEH:
	case a_WAW:
	case a_ALEF_MAKSURA:
	case a_YEH:
	    return TRUE;
    }

    return FALSE;
}


/*
 * Returns True if c is an Isolated Form-B ARABIC letter
 */
    static int
A_is_s(int cur_c)
{
    switch (cur_c)
    {
	case a_s_HAMZA:
	case a_s_ALEF_MADDA:
	case a_s_ALEF_HAMZA_ABOVE:
	case a_s_WAW_HAMZA:
	case a_s_ALEF_HAMZA_BELOW:
	case a_s_YEH_HAMZA:
	case a_s_ALEF:
	case a_s_BEH:
	case a_s_TEH_MARBUTA:
	case a_s_TEH:
	case a_s_THEH:
	case a_s_JEEM:
	case a_s_HAH:
	case a_s_KHAH:
	case a_s_DAL:
	case a_s_THAL:
	case a_s_REH:
	case a_s_ZAIN:
	case a_s_SEEN:
	case a_s_SHEEN:
	case a_s_SAD:
	case a_s_DAD:
	case a_s_TAH:
	case a_s_ZAH:
	case a_s_AIN:
	case a_s_GHAIN:
	case a_s_FEH:
	case a_s_QAF:
	case a_s_KAF:
	case a_s_LAM:
	case a_s_MEEM:
	case a_s_NOON:
	case a_s_HEH:
	case a_s_WAW:
	case a_s_ALEF_MAKSURA:
	case a_s_YEH:
	    return TRUE;
    }

    return FALSE;
}


/*
 * Returns True if c is a Final shape of an ARABIC letter
 */
    static int
A_is_f(int cur_c)
{
    switch (cur_c)
    {
	case a_f_ALEF_MADDA:
	case a_f_ALEF_HAMZA_ABOVE:
	case a_f_WAW_HAMZA:
	case a_f_ALEF_HAMZA_BELOW:
	case a_f_YEH_HAMZA:
	case a_f_ALEF:
	case a_f_BEH:
	case a_f_TEH_MARBUTA:
	case a_f_TEH:
	case a_f_THEH:
	case a_f_JEEM:
	case a_f_HAH:
	case a_f_KHAH:
	case a_f_DAL:
	case a_f_THAL:
	case a_f_REH:
	case a_f_ZAIN:
	case a_f_SEEN:
	case a_f_SHEEN:
	case a_f_SAD:
	case a_f_DAD:
	case a_f_TAH:
	case a_f_ZAH:
	case a_f_AIN:
	case a_f_GHAIN:
	case a_f_FEH:
	case a_f_QAF:
	case a_f_KAF:
	case a_f_LAM:
	case a_f_MEEM:
	case a_f_NOON:
	case a_f_HEH:
	case a_f_WAW:
	case a_f_ALEF_MAKSURA:
	case a_f_YEH:
	case a_f_LAM_ALEF_MADDA_ABOVE:
	case a_f_LAM_ALEF_HAMZA_ABOVE:
	case a_f_LAM_ALEF_HAMZA_BELOW:
	case a_f_LAM_ALEF:
	    return TRUE;
    }
    return FALSE;
}


/*
 * Change shape - from ISO-8859-6/Isolated to Form-B Isolated
 */
    static int
chg_c_a2s(int cur_c)
{
    switch (cur_c)
    {
	case a_HAMZA: return a_s_HAMZA;
	case a_ALEF_MADDA: return a_s_ALEF_MADDA;
	case a_ALEF_HAMZA_ABOVE: return a_s_ALEF_HAMZA_ABOVE;
	case a_WAW_HAMZA: return a_s_WAW_HAMZA;
	case a_ALEF_HAMZA_BELOW: return a_s_ALEF_HAMZA_BELOW;
	case a_YEH_HAMZA: return a_s_YEH_HAMZA;
	case a_ALEF: return a_s_ALEF;
	case a_TEH_MARBUTA: return a_s_TEH_MARBUTA;
	case a_DAL: return a_s_DAL;
	case a_THAL: return a_s_THAL;
	case a_REH: return a_s_REH;
	case a_ZAIN: return a_s_ZAIN;
	case a_TATWEEL: return cur_c;	/* exceptions */
	case a_WAW: return a_s_WAW;
	case a_ALEF_MAKSURA: return a_s_ALEF_MAKSURA;
	case a_BEH: return a_s_BEH;
	case a_TEH: return a_s_TEH;
	case a_THEH: return a_s_THEH;
	case a_JEEM: return a_s_JEEM;
	case a_HAH: return a_s_HAH;
	case a_KHAH: return a_s_KHAH;
	case a_SEEN: return a_s_SEEN;
	case a_SHEEN: return a_s_SHEEN;
	case a_SAD: return a_s_SAD;
	case a_DAD: return a_s_DAD;
	case a_TAH: return a_s_TAH;
	case a_ZAH: return a_s_ZAH;
	case a_AIN: return a_s_AIN;
	case a_GHAIN: return a_s_GHAIN;
	case a_FEH: return a_s_FEH;
	case a_QAF: return a_s_QAF;
	case a_KAF: return a_s_KAF;
	case a_LAM: return a_s_LAM;
	case a_MEEM: return a_s_MEEM;
	case a_NOON: return a_s_NOON;
	case a_HEH: return a_s_HEH;
	case a_YEH: return a_s_YEH;
    }
    return 0;
}


/*
 * Change shape - from ISO-8859-6/Isolated to Initial
 */
    static int
chg_c_a2i(int cur_c)
{
    switch (cur_c)
    {
	case a_YEH_HAMZA: return a_i_YEH_HAMZA;
	case a_HAMZA:			/* exceptions */
	    return a_s_HAMZA;
	case a_ALEF_MADDA:		/* exceptions */
	    return a_s_ALEF_MADDA;
	case a_ALEF_HAMZA_ABOVE:	/* exceptions */
	    return a_s_ALEF_HAMZA_ABOVE;
	case a_WAW_HAMZA:		/* exceptions */
	    return a_s_WAW_HAMZA;
	case a_ALEF_HAMZA_BELOW:	/* exceptions */
	    return a_s_ALEF_HAMZA_BELOW;
	case a_ALEF:			/* exceptions */
	    return a_s_ALEF;
	case a_TEH_MARBUTA:		/* exceptions */
	    return a_s_TEH_MARBUTA;
	case a_DAL:			/* exceptions */
	    return a_s_DAL;
	case a_THAL:			/* exceptions */
	    return a_s_THAL;
	case a_REH:			/* exceptions */
	    return a_s_REH;
	case a_ZAIN:			/* exceptions */
	    return a_s_ZAIN;
	case a_TATWEEL:			/* exceptions */
	    return cur_c;
	case a_WAW:			/* exceptions */
	    return a_s_WAW;
	case a_ALEF_MAKSURA:		/* exceptions */
	    return a_s_ALEF_MAKSURA;
	case a_BEH: return a_i_BEH;
	case a_TEH: return a_i_TEH;
	case a_THEH: return a_i_THEH;
	case a_JEEM: return a_i_JEEM;
	case a_HAH: return a_i_HAH;
	case a_KHAH: return a_i_KHAH;
	case a_SEEN: return a_i_SEEN;
	case a_SHEEN: return a_i_SHEEN;
	case a_SAD: return a_i_SAD;
	case a_DAD: return a_i_DAD;
	case a_TAH: return a_i_TAH;
	case a_ZAH: return a_i_ZAH;
	case a_AIN: return a_i_AIN;
	case a_GHAIN: return a_i_GHAIN;
	case a_FEH: return a_i_FEH;
	case a_QAF: return a_i_QAF;
	case a_KAF: return a_i_KAF;
	case a_LAM: return a_i_LAM;
	case a_MEEM: return a_i_MEEM;
	case a_NOON: return a_i_NOON;
	case a_HEH: return a_i_HEH;
	case a_YEH: return a_i_YEH;
    }
    return 0;
}


/*
 * Change shape - from ISO-8859-6/Isolated to Medial
 */
    static int
chg_c_a2m(int cur_c)
{
    switch (cur_c)
    {
	case a_HAMZA: return a_s_HAMZA;	/* exception */
	case a_ALEF_MADDA: return a_f_ALEF_MADDA;	/* exception */
	case a_ALEF_HAMZA_ABOVE: return a_f_ALEF_HAMZA_ABOVE;	/* exception */
	case a_WAW_HAMZA: return a_f_WAW_HAMZA;	/* exception */
	case a_ALEF_HAMZA_BELOW: return a_f_ALEF_HAMZA_BELOW;	/* exception */
	case a_YEH_HAMZA: return a_m_YEH_HAMZA;
	case a_ALEF: return a_f_ALEF;	/* exception */
	case a_BEH: return a_m_BEH;
	case a_TEH_MARBUTA: return a_f_TEH_MARBUTA;	/* exception */
	case a_TEH: return a_m_TEH;
	case a_THEH: return a_m_THEH;
	case a_JEEM: return a_m_JEEM;
	case a_HAH: return a_m_HAH;
	case a_KHAH: return a_m_KHAH;
	case a_DAL: return a_f_DAL;	/* exception */
	case a_THAL: return a_f_THAL;	/* exception */
	case a_REH: return a_f_REH;	/* exception */
	case a_ZAIN: return a_f_ZAIN;	/* exception */
	case a_SEEN: return a_m_SEEN;
	case a_SHEEN: return a_m_SHEEN;
	case a_SAD: return a_m_SAD;
	case a_DAD: return a_m_DAD;
	case a_TAH: return a_m_TAH;
	case a_ZAH: return a_m_ZAH;
	case a_AIN: return a_m_AIN;
	case a_GHAIN: return a_m_GHAIN;
	case a_TATWEEL: return cur_c;	/* exception */
	case a_FEH: return a_m_FEH;
	case a_QAF: return a_m_QAF;
	case a_KAF: return a_m_KAF;
	case a_LAM: return a_m_LAM;
	case a_MEEM: return a_m_MEEM;
	case a_NOON: return a_m_NOON;
	case a_HEH: return a_m_HEH;
	case a_WAW: return a_f_WAW;	/* exception */
	case a_ALEF_MAKSURA: return a_f_ALEF_MAKSURA;	/* exception */
	case a_YEH: return a_m_YEH;
    }
    return 0;
}


/*
 * Change shape - from ISO-8859-6/Isolated to final
 */
    static int
chg_c_a2f(int cur_c)
{
    /* NOTE: these encodings need to be accounted for
     * a_f_ALEF_MADDA;
     * a_f_ALEF_HAMZA_ABOVE;
     * a_f_ALEF_HAMZA_BELOW;
     * a_f_LAM_ALEF_MADDA_ABOVE;
     * a_f_LAM_ALEF_HAMZA_ABOVE;
     * a_f_LAM_ALEF_HAMZA_BELOW;
     */
    switch (cur_c)
    {
	case a_HAMZA: return a_s_HAMZA;	/* exception */
	case a_ALEF_MADDA: return a_f_ALEF_MADDA;
	case a_ALEF_HAMZA_ABOVE: return a_f_ALEF_HAMZA_ABOVE;
	case a_WAW_HAMZA: return a_f_WAW_HAMZA;
	case a_ALEF_HAMZA_BELOW: return a_f_ALEF_HAMZA_BELOW;
	case a_YEH_HAMZA: return a_f_YEH_HAMZA;
	case a_ALEF: return a_f_ALEF;
	case a_BEH: return a_f_BEH;
	case a_TEH_MARBUTA: return a_f_TEH_MARBUTA;
	case a_TEH: return a_f_TEH;
	case a_THEH: return a_f_THEH;
	case a_JEEM: return a_f_JEEM;
	case a_HAH: return a_f_HAH;
	case a_KHAH: return a_f_KHAH;
	case a_DAL: return a_f_DAL;
	case a_THAL: return a_f_THAL;
	case a_REH: return a_f_REH;
	case a_ZAIN: return a_f_ZAIN;
	case a_SEEN: return a_f_SEEN;
	case a_SHEEN: return a_f_SHEEN;
	case a_SAD: return a_f_SAD;
	case a_DAD: return a_f_DAD;
	case a_TAH: return a_f_TAH;
	case a_ZAH: return a_f_ZAH;
	case a_AIN: return a_f_AIN;
	case a_GHAIN: return a_f_GHAIN;
	case a_TATWEEL:	return cur_c;	/* exception */
	case a_FEH: return a_f_FEH;
	case a_QAF: return a_f_QAF;
	case a_KAF: return a_f_KAF;
	case a_LAM: return a_f_LAM;
	case a_MEEM: return a_f_MEEM;
	case a_NOON: return a_f_NOON;
	case a_HEH: return a_f_HEH;
	case a_WAW: return a_f_WAW;
	case a_ALEF_MAKSURA: return a_f_ALEF_MAKSURA;
	case a_YEH: return a_f_YEH;
    }
    return 0;
}


/*
 * Change shape - from Initial to Medial
 * This code is unreachable, because for the relevant characters ARABIC_CHAR()
 * is FALSE;
 */
#if 0
    static int
chg_c_i2m(int cur_c)
{
    switch (cur_c)
    {
	case a_i_YEH_HAMZA: return a_m_YEH_HAMZA;
	case a_i_BEH: return a_m_BEH;
	case a_i_TEH: return a_m_TEH;
	case a_i_THEH: return a_m_THEH;
	case a_i_JEEM: return a_m_JEEM;
	case a_i_HAH: return a_m_HAH;
	case a_i_KHAH: return a_m_KHAH;
	case a_i_SEEN: return a_m_SEEN;
	case a_i_SHEEN: return a_m_SHEEN;
	case a_i_SAD: return a_m_SAD;
	case a_i_DAD: return a_m_DAD;
	case a_i_TAH: return a_m_TAH;
	case a_i_ZAH: return a_m_ZAH;
	case a_i_AIN: return a_m_AIN;
	case a_i_GHAIN: return a_m_GHAIN;
	case a_i_FEH: return a_m_FEH;
	case a_i_QAF: return a_m_QAF;
	case a_i_KAF: return a_m_KAF;
	case a_i_LAM: return a_m_LAM;
	case a_i_MEEM: return a_m_MEEM;
	case a_i_NOON: return a_m_NOON;
	case a_i_HEH: return a_m_HEH;
	case a_i_YEH: return a_m_YEH;
    }
    return 0;
}
#endif


/*
 * Change shape - from Final to Medial
 */
    static int
chg_c_f2m(int cur_c)
{
    switch (cur_c)
    {
	/* NOTE: these encodings are multi-positional, no ?
	 * case a_f_ALEF_MADDA:
	 * case a_f_ALEF_HAMZA_ABOVE:
	 * case a_f_ALEF_HAMZA_BELOW:
	 */
	case a_f_YEH_HAMZA: return a_m_YEH_HAMZA;
	case a_f_WAW_HAMZA:		/* exceptions */
	case a_f_ALEF:
	case a_f_TEH_MARBUTA:
	case a_f_DAL:
	case a_f_THAL:
	case a_f_REH:
	case a_f_ZAIN:
	case a_f_WAW:
	case a_f_ALEF_MAKSURA:
		return cur_c;
	case a_f_BEH: return a_m_BEH;
	case a_f_TEH: return a_m_TEH;
	case a_f_THEH: return a_m_THEH;
	case a_f_JEEM: return a_m_JEEM;
	case a_f_HAH: return a_m_HAH;
	case a_f_KHAH: return a_m_KHAH;
	case a_f_SEEN: return a_m_SEEN;
	case a_f_SHEEN: return a_m_SHEEN;
	case a_f_SAD: return a_m_SAD;
	case a_f_DAD: return a_m_DAD;
	case a_f_TAH: return a_m_TAH;
	case a_f_ZAH: return a_m_ZAH;
	case a_f_AIN: return a_m_AIN;
	case a_f_GHAIN: return a_m_GHAIN;
	case a_f_FEH: return a_m_FEH;
	case a_f_QAF: return a_m_QAF;
	case a_f_KAF: return a_m_KAF;
	case a_f_LAM: return a_m_LAM;
	case a_f_MEEM: return a_m_MEEM;
	case a_f_NOON: return a_m_NOON;
	case a_f_HEH: return a_m_HEH;
	case a_f_YEH: return a_m_YEH;

	/* NOTE: these encodings are multi-positional, no ?
	 * case a_f_LAM_ALEF_MADDA_ABOVE:
	 * case a_f_LAM_ALEF_HAMZA_ABOVE:
	 * case a_f_LAM_ALEF_HAMZA_BELOW:
	 * case a_f_LAM_ALEF:
	 */
    }
    return 0;
}


/*
 * Change shape - from Combination (2 char) to an Isolated
 */
    static int
chg_c_laa2i(int hid_c)
{
    switch (hid_c)
    {
	case a_ALEF_MADDA: return a_s_LAM_ALEF_MADDA_ABOVE;
	case a_ALEF_HAMZA_ABOVE: return a_s_LAM_ALEF_HAMZA_ABOVE;
	case a_ALEF_HAMZA_BELOW: return a_s_LAM_ALEF_HAMZA_BELOW;
	case a_ALEF: return a_s_LAM_ALEF;
    }
    return 0;
}


/*
 * Change shape - from Combination-Isolated to Final
 */
    static int
chg_c_laa2f(int hid_c)
{
    switch (hid_c)
    {
	case a_ALEF_MADDA: return a_f_LAM_ALEF_MADDA_ABOVE;
	case a_ALEF_HAMZA_ABOVE: return a_f_LAM_ALEF_HAMZA_ABOVE;
	case a_ALEF_HAMZA_BELOW: return a_f_LAM_ALEF_HAMZA_BELOW;
	case a_ALEF: return a_f_LAM_ALEF;
    }
    return 0;
}

/*
 * Do "half-shaping" on character "c".  Return zero if no shaping.
 */
    static int
half_shape(int c)
{
    if (A_is_a(c))
	return chg_c_a2i(c);
    if (A_is_valid(c) && A_is_f(c))
	return chg_c_f2m(c);
    return 0;
}

/*
 * Do Arabic shaping on character "c".  Returns the shaped character.
 * out:    "ccp" points to the first byte of the character to be shaped.
 * in/out: "c1p" points to the first composing char for "c".
 * in:     "prev_c"  is the previous character (not shaped)
 * in:     "prev_c1" is the first composing char for the previous char
 *		     (not shaped)
 * in:     "next_c"  is the next character (not shaped).
 */
    int
arabic_shape(
    int		c,
    int		*ccp,
    int		*c1p,
    int		prev_c,
    int		prev_c1,
    int		next_c)
{
    int		curr_c;
    int		shape_c;
    int		curr_laa;
    int		prev_laa;

    /* Deal only with Arabic character, pass back all others */
    if (!A_is_ok(c))
	return c;

    /* half-shape current and previous character */
    shape_c = half_shape(prev_c);

    /* Save away current character */
    curr_c = c;

    curr_laa = A_firstc_laa(c, *c1p);
    prev_laa = A_firstc_laa(prev_c, prev_c1);

    if (curr_laa)
    {
	if (A_is_valid(prev_c) && !A_is_f(shape_c)
					 && !A_is_s(shape_c) && !prev_laa)
	    curr_c = chg_c_laa2f(curr_laa);
	else
	    curr_c = chg_c_laa2i(curr_laa);

	/* Remove the composing character */
	*c1p = 0;
    }
    else if (!A_is_valid(prev_c) && A_is_valid(next_c))
	curr_c = chg_c_a2i(c);
    else if (!shape_c || A_is_f(shape_c) || A_is_s(shape_c) || prev_laa)
	curr_c = A_is_valid(next_c) ? chg_c_a2i(c) : chg_c_a2s(c);
    else if (A_is_valid(next_c))
#if 0
	curr_c = A_is_iso(c) ? chg_c_a2m(c) : chg_c_i2m(c);
#else
	curr_c = A_is_iso(c) ? chg_c_a2m(c) : 0;
#endif
    else if (A_is_valid(prev_c))
	curr_c = chg_c_a2f(c);
    else
	curr_c = chg_c_a2s(c);

    /* Sanity check -- curr_c should, in the future, never be 0.
     * We should, in the future, insert a fatal error here. */
    if (curr_c == NUL)
	curr_c = c;

    if (curr_c != c && ccp != NULL)
    {
	char_u buf[MB_MAXBYTES + 1];

	/* Update the first byte of the character. */
	(*mb_char2bytes)(curr_c, buf);
	*ccp = buf[0];
    }

    /* Return the shaped character */
    return curr_c;
}


/*
 * A_firstc_laa returns first character of LAA combination if it exists
 */
    static int
A_firstc_laa(
    int c,	/* base character */
    int c1)	/* first composing character */
{
    if (c1 != NUL && c == a_LAM && !A_is_harakat(c1))
	return c1;
    return 0;
}


/*
 * A_is_harakat returns TRUE if 'c' is an Arabic Harakat character
 *		(harakat/tanween)
 */
    static int
A_is_harakat(int c)
{
    return (c >= a_FATHATAN && c <= a_SUKUN);
}


/*
 * A_is_iso returns TRUE if 'c' is an Arabic ISO-8859-6 character
 *		(alphabet/number/punctuation)
 */
    static int
A_is_iso(int c)
{
    return ((c >= a_HAMZA && c <= a_GHAIN)
	    || (c >= a_TATWEEL && c <= a_HAMZA_BELOW)
	    || c == a_MINI_ALEF);
}


/*
 * A_is_formb returns TRUE if 'c' is an Arabic 10646-1 FormB character
 *		(alphabet/number/punctuation)
 */
    static int
A_is_formb(int c)
{
    return ((c >= a_s_FATHATAN && c <= a_s_DAMMATAN)
	    || c == a_s_KASRATAN
	    || (c >= a_s_FATHA && c <= a_f_LAM_ALEF)
	    || c == a_BYTE_ORDER_MARK);
}


/*
 * A_is_ok returns TRUE if 'c' is an Arabic 10646 (8859-6 or Form-B)
 */
    static int
A_is_ok(int c)
{
    return (A_is_iso(c) || A_is_formb(c));
}


/*
 * A_is_valid returns TRUE if 'c' is an Arabic 10646 (8859-6 or Form-B)
 *		with some exceptions/exclusions
 */
    static int
A_is_valid(int c)
{
    return (A_is_ok(c) && !A_is_special(c));
}


/*
 * A_is_special returns TRUE if 'c' is not a special Arabic character.
 *		Specials don't adhere to most of the rules.
 */
    static int
A_is_special(int c)
{
    return (c == a_HAMZA || c == a_s_HAMZA);
}

#endif /* FEAT_ARABIC */
