/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * spell.c: code for spell checking
 *
 * Terminology:
 * "dword" is a dictionary word, made out of letters and digits.
 * "nword" is a word with a character that's not a letter or digit.
 * "word"  is either a "dword" or an "nword".
 */

/*
 * Why doesn't Vim use aspell/ispell/myspell/etc.?
 * See ":help develop-spell".
 */

#if defined(MSDOS) || defined(WIN16) || defined(WIN32) || defined(_WIN64)
# include <io.h>	/* for lseek(), must be before vim.h */
#endif

#include "vim.h"

#if defined(FEAT_SYN_HL) || defined(PROTO)

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#define MAXWLEN 100		/* assume max. word len is this many bytes */

/*
 * Structure that is used to store the text from the language file.  This
 * avoids the need to allocate space for each individual word.  It's allocated
 * in big chunks for speed.
 */
#define  SBLOCKSIZE 4096	/* default size of sb_data */
typedef struct sblock_S sblock_T;
struct sblock_S
{
    sblock_T	*sb_next;	/* next block in list */
    char_u	sb_data[1];	/* data, actually longer */
};

/* Info from "REP" entries in ".aff" file used in af_rep. */
typedef struct repentry_S
{
    char_u	*re_from;
    char_u	*re_to;
} repentry_T;

/*
 * Structure to store affix info.
 */
typedef struct affitem_S affitem_T;
struct affitem_S
{
    affitem_T	*ai_next;	/* next affix with same ai_add[] or NULL */
    short_u	ai_nr;		/* affix number */
    char_u	ai_combine;	/* prefix combines with suffix */
    char_u	ai_choplen;	/* length of ai_chop in bytes */
    char_u	ai_addlen;	/* length of ai_add in bytes */
    char_u	*ai_chop;	/* text chopped off basic word (can be NULL) */
    char_u	ai_add[1];	/* text added to basic word (actually longer) */
};

/* Get affitem_T pointer from hashitem that uses ai_add */
static affitem_T dumai;
#define HI2AI(hi)	((affitem_T *)((hi)->hi_key - (dumai.ai_add - (char_u *)&dumai)))

/*
 * Structure used to store words and other info for one language.
 */
typedef struct slang_S slang_T;
struct slang_S
{
    slang_T	*sl_next;	/* next language */
    char_u	*sl_name;	/* language name "en", "en.rare", "nl", etc. */
    hashtab_T	sl_words;	/* main word table, fword_T */
    int		sl_prefcnt;	/* number of prefix NRs */
    garray_T	sl_preftab;	/* list of hashtables to lookup prefixes */
    affitem_T	*sl_prefzero;	/* list of prefixes with zero add length */
    int		sl_suffcnt;	/* number of suffix NRs */
    garray_T	sl_sufftab;	/* list of hashtables to lookup suffixes */
    affitem_T	*sl_suffzero;	/* list of suffixes with zero add length */
    char_u	*sl_try;	/* "TRY" from .aff file */
    garray_T	sl_rep;		/* list of repentry_T entries from REP lines */
    char_u	sl_regions[17];	/* table with up to 8 region names plus NUL */
    sblock_T	*sl_block;	/* list with allocated memory blocks */
    int		sl_error;	/* error while loading */
};

static slang_T *first_lang = NULL;

/*
 * Structure to store an addition to a basic word.
 */
typedef struct addword_S addword_T;
struct addword_S
{
    addword_T	*aw_next;	/* next addition */
    char_u	aw_flags;	/* ADD_ flags */
    char_u	aw_leadlen;	/* length of lead in bytes */
    char_u	aw_wordlen;	/* length of aw_word in bytes */
    char_u	aw_region;	/* region for word with this addition */
    char_u	aw_word[1];	/* text, actually longer: case-folded addition
				   plus, with ADD_KEEPCAP: keep-case addition */
};

/*
 * Structure to store a basic word.
 */
typedef struct fword_S fword_T;
struct fword_S
{
    fword_T	*fw_next;	/* same basic word with different caps */
    char_u	fw_region;	/* region bits */
    char_u	fw_prefixcnt;	/* number of prefix numbers */
    char_u	fw_suffixcnt;	/* number of suffix numbers */
    short_u	fw_flags;	/* BWF_ flags */
    void	*fw_prefix;	/* table with prefix numbers */
    void	*fw_suffix;	/* table with suffix numbers */
    addword_T	*fw_adds;	/* first addword_T entry */
    char_u	fw_word[1];	/* actually longer: case folded word, or
				   keep-case word when (flags & BWF_KEEPCAP) */
};

/* Get fword_T pointer from hashitem that uses fw_word */
static fword_T dumfw;
#define HI2FWORD(hi)	((fword_T *)((hi)->hi_key - (dumfw.fw_word - (char_u *)&dumfw)))

#define REGION_ALL 0xff


/*
 * Structure used in "b_langp", filled from 'spelllang'.
 */
typedef struct langp_S
{
    slang_T	*lp_slang;	/* info for this language (NULL for last one) */
    int		lp_region;	/* bitmask for region or REGION_ALL */
} langp_T;

#define LANGP_ENTRY(ga, i)	(((langp_T *)(ga).ga_data) + (i))

#define SP_OK		0
#define SP_BAD		1
#define SP_RARE		2
#define SP_LOCAL	3

/* flags used for basic words in the spell file */
#define BWF_VALID	0x01	    /* word is valid without additions */
#define BWF_REGION	0x02	    /* region byte follows */
#define BWF_ONECAP	0x04	    /* first letter must be capital */
#define BWF_SUFFIX	0x08	    /* has suffix NR list */
#define BWF_SECOND	0x10	    /* second flags byte follows */

#define BWF_ADDS	0x0100	    /* there are additions */
#define BWF_PREFIX	0x0200	    /* has prefix NR list */
#define BWF_ALLCAP	0x0400	    /* all letters must be capital (not used
				       for single-letter words) */
#define BWF_KEEPCAP	0x0800	    /* Keep case as-is */

/* flags used for addition in the spell file */
#define ADD_REGION	0x02	    /* region byte follows */
#define ADD_ONECAP	0x04	    /* first letter must be capital */
#define ADD_ALLCAP	0x40	    /* all letters must be capital (not used
				       for single-letter words) */
#define ADD_KEEPCAP	0x80	    /* fixed case */

/* Translate ADD_ flags to BWF_ flags.
 * (Needed to keep ADD_ flags in one byte.) */
#define ADD2BWF(x)	(((x) & 0x0f) | (((x) & 0xf0) << 4))

#define VIMSPELLMAGIC "VIMspell01"  /* string at start of Vim spell file */
#define VIMSPELLMAGICL 10

/*
 * Structure to store info for word matching.
 */
typedef struct matchinf_S
{
    langp_T	*mi_lp;			/* info for language and region */
    slang_T	*mi_slang;		/* info for the language */
    char_u	*mi_line;		/* start of line containing word */
    char_u	*mi_word;		/* start of word being checked */
    char_u	*mi_end;		/* first non-word char after mi_word */
    char_u	*mi_wend;		/* end of matching word (is "mi_end"
					 * or further) */
    char_u	*mi_cword;		/* word to check, can be "mi_fword" */
    char_u	mi_fword[MAXWLEN + 1];	/* "mi_word" to "mi_end" case-folded */
    int		mi_faddlen;		/* length of valid bytes in "mi_fadd" */
    char_u	*mi_faddp;		/* next char to be added to "mi_fadd" */
    char_u	mi_fadd[MAXWLEN + 1];	/* "mi_end" and further case-folded */
    int		mi_result;		/* result so far: SP_BAD, SP_OK, etc. */
    int		mi_capflags;		/* BWF_ONECAP BWF_ALLCAP BWF_KEEPCAP */
} matchinf_T;

static int word_match __ARGS((matchinf_T *mip));
static int check_adds __ARGS((matchinf_T *mip, fword_T *fw, int req_pref, int req_suf));
static int supports_afffix __ARGS((int cnt, void *afffix, int afffixcnt, int nr));
static int prefix_match __ARGS((matchinf_T *mip));
static int suffix_match __ARGS((matchinf_T *mip));
static int match_caps __ARGS((int flags, char_u *caseword, matchinf_T *mip, char_u *cword, char_u *end));
static slang_T *slang_alloc __ARGS((char_u *lang));
static void slang_free __ARGS((slang_T *lp));
static slang_T *spell_load_lang __ARGS((char_u *lang));
static void spell_load_file __ARGS((char_u *fname, void *cookie));
static int spell_load_affixes __ARGS((FILE *fd, slang_T *lp, int *bl_usedp, int affm, void **affp));
static void *getroom __ARGS((slang_T *lp, int *bl_used, int len));
static int find_region __ARGS((char_u *rp, char_u *region));
static int captype __ARGS((char_u *word, char_u *end));

/*
 * Main spell-checking function.
 * "ptr" points to the start of a word.
 * "*attrp" is set to the attributes for a badly spelled word.  For a non-word
 * or when it's OK it remains unchanged.
 * This must only be called when 'spelllang' is not empty.
 * Returns the length of the word in bytes, also when it's OK, so that the
 * caller can skip over the word.
 */
    int
spell_check(wp, line, ptr, attrp)
    win_T	*wp;		/* current window */
    char_u	*line;		/* start of line where "ptr" points into */
    char_u	*ptr;
    int		*attrp;
{
    matchinf_T	mi;		/* Most things are put in "mi" so that it can
				   be passed to functions quickly. */

    /* Find the end of the word.  We already know that *ptr is a word char. */
    mi.mi_word = ptr;
    mi.mi_end = ptr;
    do
    {
	mb_ptr_adv(mi.mi_end);
    } while (*mi.mi_end != NUL && spell_iswordc(mi.mi_end));

    /* A word starting with a number is always OK. */
    if (*ptr >= '0' && *ptr <= '9')
	return (int)(mi.mi_end - ptr);

    /* Make case-folded copy of the Word.  Compute its hash value. */
    (void)str_foldcase(ptr, mi.mi_end - ptr, mi.mi_fword, MAXWLEN + 1);
    mi.mi_cword = mi.mi_fword;

    /* The word is bad unless we find it in the dictionary. */
    mi.mi_result = SP_BAD;
    mi.mi_wend = mi.mi_end;
    mi.mi_faddp = mi.mi_end;
    mi.mi_faddlen = 0;
    mi.mi_capflags = captype(ptr, mi.mi_end);
    mi.mi_line = line;

    /*
     * Loop over the languages specified in 'spelllang'.
     * We check them all, because a matching word may have additions that are
     * longer than an already found matching word.
     */
    for (mi.mi_lp = LANGP_ENTRY(wp->w_buffer->b_langp, 0);
				       mi.mi_lp->lp_slang != NULL; ++mi.mi_lp)
    {
	/*
	 * Check for a matching word.
	 * If not found or wrong region try removing prefixes (and then
	 * suffixes).
	 * If still not found or wrong region try removing suffixes.
	 */
	mi.mi_slang = mi.mi_lp->lp_slang;
	if (!word_match(&mi) || mi.mi_result != SP_OK)
	    if (!prefix_match(&mi) || mi.mi_result != SP_OK)
		suffix_match(&mi);
    }

    if (mi.mi_result != SP_OK)
    {
	if (mi.mi_result == SP_BAD)
	    *attrp = highlight_attr[HLF_SPB];
	else if (mi.mi_result == SP_RARE)
	    *attrp = highlight_attr[HLF_SPR];
	else
	    *attrp = highlight_attr[HLF_SPL];
    }

    return (int)(mi.mi_wend - ptr);
}

/*
 * Check if the word "mip->mi_cword" matches.
 */
    static int
word_match(mip)
    matchinf_T *mip;
{
    hash_T	fhash = hash_hash(mip->mi_cword);
    hashitem_T	*hi;
    fword_T	*fw;
    int		valid = FALSE;

    hi = hash_lookup(&mip->mi_slang->sl_words, mip->mi_cword, fhash);
    if (HASHITEM_EMPTY(hi))
	return FALSE;

    /*
     * Find a basic word for which the case of word "cword" is correct.
     * If it is, check additions and use the longest one.
     */
    for (fw = HI2FWORD(hi); fw != NULL; fw = fw->fw_next)
	if (match_caps(fw->fw_flags, fw->fw_word, mip,
						   mip->mi_word, mip->mi_end))
	    valid |= check_adds(mip, fw, -1, -1);

    return valid;
}

/*
 * Check a matching basic word for additions.
 * Return TRUE if we have a valid match.
 */
    static int
check_adds(mip, fw, req_pref, req_suf)
    matchinf_T	*mip;
    fword_T	*fw;
    int		req_pref;	/* required prefix nr, -1 if none */
    int		req_suf;	/* required suffix nr, -1 if none */
{
    int		valid = FALSE;
    addword_T	*aw;
    char_u	*p;
    int		addlen;
    int		fl;

    /* A word may be valid without additions. */
    if ((fw->fw_flags & BWF_VALID)
	    && (req_pref < 0 || supports_afffix(mip->mi_slang->sl_prefcnt,
				   fw->fw_prefix, fw->fw_prefixcnt, req_pref))
	    && (req_suf < 0 || supports_afffix(mip->mi_slang->sl_suffcnt,
				   fw->fw_suffix, fw->fw_suffixcnt, req_suf)))
    {
	valid = TRUE;
	if (mip->mi_result != SP_OK)
	{
	    if ((fw->fw_region & mip->mi_lp->lp_region) == 0)
		mip->mi_result = SP_LOCAL;
	    else
		mip->mi_result = SP_OK;
	}
    }

    /*
     * Check additions, both before and after the word.
     * This may make the word longer, thus we also need to check
     * when we already found a matching word.
     */
    for (aw = fw->fw_adds; aw != NULL; aw = aw->aw_next)
    {
	if (aw->aw_leadlen > 0)
	{
	    /* There is a leader, verify that it matches. */
	    if (aw->aw_leadlen > mip->mi_word - mip->mi_line
		    || STRNCMP(mip->mi_word - aw->aw_leadlen,
					    aw->aw_word, aw->aw_leadlen) != 0)
		continue;
	    if (mip->mi_word - aw->aw_leadlen > mip->mi_line)
	    {
		/* There must not be a word character just before the
		 * leader. */
		p = mip->mi_word - aw->aw_leadlen;
		mb_ptr_back(mip->mi_line, p);
		if (spell_iswordc(p))
		    continue;
	    }
	    /* Leader matches.  Addition is rest of "aw_word". */
	    p = aw->aw_word + aw->aw_leadlen;
	}
	else
	    /* No leader, use whole of "aw_word" for addition. */
	    p = aw->aw_word;

	addlen = aw->aw_wordlen - aw->aw_leadlen;
	if (addlen > 0)
	{
	    /* Check for matching addition and no word character after it.
	     * First make sure we have enough case-folded chars to compare
	     * with. */
	    while (mip->mi_faddlen <= addlen)
	    {
		if (*mip->mi_faddp == NUL)
		{
		    mip->mi_fadd[mip->mi_faddlen] = NUL;
		    break;
		}
#ifdef FEAT_MBYTE
		fl = (*mb_ptr2len_check)(mip->mi_faddp);
#else
		fl = 1;
#endif
		(void)str_foldcase(mip->mi_faddp, fl,
				   mip->mi_fadd + mip->mi_faddlen,
						   MAXWLEN - mip->mi_faddlen);
		mip->mi_faddp += fl;
		mip->mi_faddlen += STRLEN(mip->mi_fadd + mip->mi_faddlen);
	    }

	    if (STRNCMP(mip->mi_fadd, p, addlen) != 0
		    || (mip->mi_fadd[addlen] != NUL
				     && spell_iswordc(mip->mi_fadd + addlen)))
		continue;

	    /* Compute the length in the original word, before case folding. */
#ifdef FEAT_MBYTE
	    if (has_mbyte)
	    {
		int	l;

		p = mip->mi_end;
		for (l = 0; l < addlen;
				   l += (*mb_ptr2len_check)(mip->mi_fadd + l))
		    mb_ptr_adv(p);
		addlen = p - mip->mi_end;
	    }
#endif

	    /* Check case of the addition. */
	    if (!match_caps(ADD2BWF(aw->aw_flags),
			  aw->aw_word + aw->aw_wordlen + 1, mip,
					   mip->mi_end, mip->mi_end + addlen))
		continue;
	}

	/* Match!  Use the new length if it's longer. */
	if (mip->mi_wend < mip->mi_end + addlen)
	    mip->mi_wend = mip->mi_end + addlen;

	valid = TRUE;
	if (mip->mi_result != SP_OK)
	{
	    if ((aw->aw_region & mip->mi_lp->lp_region) == 0)
		mip->mi_result = SP_LOCAL;
	    else
		mip->mi_result = SP_OK;
	}
    }

    return valid;
}

/*
 * Return TRUE if word "fw" supports afffix "nr".
 */
    static int
supports_afffix(cnt, afffix, afffixcnt, nr)
    int		cnt;
    void	*afffix;
    int		afffixcnt;
    int		nr;
{
    char_u	*pc;
    short_u	*ps;
    int		i;

    if (cnt <= 256)
    {
	/* char_u affix numbers */
	pc = afffix;
	for (i = afffixcnt; --i >= 0; )
	    if (*pc++ == nr)
		return TRUE;
    }
    else
    {
	/* short_u affix numbers */
	ps = afffix;
	for (i = afffixcnt; --i >= 0; )
	    if (*ps++ == nr)
		return TRUE;
    }
    return FALSE;
}

/*
 * Try finding a match for "mip->mi_cword" by removing prefixes.
 */
    static int
prefix_match(mip)
    matchinf_T	*mip;
{
    int		len = 0;
    int		charlen = 0;
    int		cc;
    affitem_T	*ai;
    char_u	pword[MAXWLEN + 1];
    fword_T	*fw;
    hashtab_T	*ht;
    hashitem_T	*hi;
    int		i;
    int		found_valid = FALSE;
    int		cstart_charlen = 0;
    char_u	*cstart = mip->mi_word;
    int		capflags_save = mip->mi_capflags;
    char_u	*p;

    /*
     * Check for prefixes with different character lengths.
     * Start with zero length (only chop off).
     */
    for (charlen = 0; charlen <= mip->mi_slang->sl_preftab.ga_len; ++charlen)
    {
	if (charlen > 0)
	{
#ifdef FEAT_MBYTE
	    if (has_mbyte)
		len += mb_ptr2len_check(mip->mi_cword + len);
	    else
#endif
		len += 1;
	}
	if (mip->mi_cword[len] == NUL)	/* end of word, no prefix possible */
	    break;

	if (charlen == 0)
	    ai = mip->mi_slang->sl_prefzero;
	else
	{
	    /* Get pointer to hashtab for prefix of this many chars. */
	    ht = ((hashtab_T *)mip->mi_slang->sl_preftab.ga_data) + charlen - 1;
	    if (ht->ht_used == 0)
		continue;

	    cc = mip->mi_cword[len];
	    mip->mi_cword[len] = NUL;
	    hi = hash_find(ht, mip->mi_cword);
	    mip->mi_cword[len] = cc;

	    if (HASHITEM_EMPTY(hi))
		ai = NULL;
	    else
		ai = HI2AI(hi);
	}

	/* Loop over all matching prefixes. */
	for ( ; ai != NULL; ai = ai->ai_next)
	{
	    /* Create the basic word by removing the prefix and adding the
	     * chop string. */
	    mch_memmove(pword, ai->ai_chop, ai->ai_choplen);
	    STRCPY(pword + ai->ai_choplen, mip->mi_cword + ai->ai_addlen);

	    /* Adjust the word start for case checks, we only check the
	     * part after the prefix. */
	    while (cstart_charlen < charlen)
	    {
		mb_ptr_adv(cstart);
		++cstart_charlen;
	    }

	    /* Removing the prefix may change the caps, e.g. for
	     * "deAlf" removing "de" makes it ONECAP. */
	    mip->mi_capflags = captype(cstart, mip->mi_end);

	    /* Find the basic word. */
	    hi = hash_find(&mip->mi_slang->sl_words, pword);
	    if (!HASHITEM_EMPTY(hi))
	    {
		/* Check if the word supports this prefix. */
		for (fw = HI2FWORD(hi); fw != NULL; fw = fw->fw_next)
		    if (match_caps(fw->fw_flags, fw->fw_word, mip,
							 cstart, mip->mi_end))
			found_valid |= check_adds(mip, fw, ai->ai_nr, -1);

		if (found_valid && mip->mi_result == SP_OK)
		{
		    /* Found a valid word, no need to try other suffixes. */
		    mip->mi_capflags = capflags_save;
		    return TRUE;
		}
	    }

	    /* No matching basic word without prefix.  When combining is
	     * allowed try with suffixes. */
	    if (ai->ai_combine)
	    {
		/* Pass the word with prefix removed to suffix_match(). */
		mip->mi_cword = pword;
		p = mip->mi_word;
		mip->mi_word = cstart;
		i = suffix_match(mip);
		mip->mi_cword = mip->mi_fword;
		mip->mi_word = p;
		if (i)
		{
		    mip->mi_capflags = capflags_save;
		    return TRUE;
		}
	    }
	}
    }

    mip->mi_capflags = capflags_save;
    return FALSE;
}

/*
 * Try finding a match for "mip->mi_cword" by removing suffixes.
 */
    static int
suffix_match(mip)
    matchinf_T	*mip;
{
    char_u	*sufp;
    int		charlen;
    affitem_T	*ai;
    char_u	pword[MAXWLEN + 1];
    fword_T	*fw;
    hashtab_T	*ht;
    hashitem_T	*hi;
    int		tlen;
    int		cend_charlen = 0;
    char_u	*cend = mip->mi_end;
    int		found_valid = FALSE;
    int		capflags_save = mip->mi_capflags;

    /*
     * Try suffixes of different length, starting with an empty suffix (chop
     * only, thus adds something).
     * Stop checking if there are no suffixes with so many characters.
     */
    sufp = mip->mi_cword + STRLEN(mip->mi_cword);
    for (charlen = 0; charlen <= mip->mi_slang->sl_sufftab.ga_len; ++charlen)
    {
	/* Move the pointer to the possible suffix back one character, unless
	 * doing the first round (empty suffix). */
	if (charlen > 0)
	{
	    mb_ptr_back(mip->mi_cword, sufp);
	    if (sufp <= mip->mi_cword)	/* start of word, no suffix possible */
		break;
	}

	if (charlen == 0)
	    ai = mip->mi_slang->sl_suffzero;
	else
	{
	    /* Get pointer to hashtab for suffix of this many chars. */
	    ht = ((hashtab_T *)mip->mi_slang->sl_sufftab.ga_data) + charlen - 1;
	    if (ht->ht_used == 0)
		continue;

	    hi = hash_find(ht, sufp);
	    if (HASHITEM_EMPTY(hi))
		ai = NULL;
	    else
		ai = HI2AI(hi);
	}

	if (ai != NULL)
	{
	    /* Found a list of matching suffixes.  Now check that there is one
	     * we can use. */
	    tlen = sufp - mip->mi_cword;    /* length of word without suffix */
	    mch_memmove(pword, mip->mi_cword, tlen);

	    for ( ; ai != NULL; ai = ai->ai_next)
	    {
		/* Found a matching suffix.  Create the basic word by removing
		 * the suffix and adding the chop string. */
		if (ai->ai_choplen == 0)
		    pword[tlen] = NUL;
		else
		    mch_memmove(pword + tlen, ai->ai_chop, ai->ai_choplen + 1);

		/* Find the basic word. */
		hi = hash_find(&mip->mi_slang->sl_words, pword);
		if (!HASHITEM_EMPTY(hi))
		{
		    /* Adjust the end for case checks, we only check the part
		     * before the suffix. */
		    while (cend_charlen < charlen)
		    {
			mb_ptr_back(mip->mi_word, cend);
			++cend_charlen;
		    }

		    /* Removing the suffix may change the caps, e.g. for
		     * "UFOs" removing 's' makes it ALLCAP. */
		    mip->mi_capflags = captype(mip->mi_word, cend);

		    /* Check if the word supports this suffix. */
		    for (fw = HI2FWORD(hi); fw != NULL; fw = fw->fw_next)
			if (match_caps(fw->fw_flags, fw->fw_word, mip,
							  mip->mi_word, cend))
			    found_valid |= check_adds(mip, fw, -1, ai->ai_nr);

		    if (found_valid && mip->mi_result == SP_OK)
		    {
			/* Found a valid word, no need to try other suffixes. */
			mip->mi_capflags = capflags_save;
			return TRUE;
		    }
		}
	    }
	}
    }

    mip->mi_capflags = capflags_save;
    return FALSE;
}

/*
 * Return TRUE if case of "cword" meets the requirements of case flags
 * "flags".
 */
    static int
match_caps(flags, caseword, mip, cword, end)
    int		flags;	    /* flags required by basic word or addition */
    char_u	*caseword;  /* word with case as required */
    matchinf_T	*mip;
    char_u	*cword;	    /* word to compare against "caseword" */
    char_u	*end;	    /* end of "cword" */
{
    char_u	*p;
    int		c;
    int		len;
    int		capflags = mip->mi_capflags;	    /* flags of checked word */
    int		past_second;

    if ((capflags & BWF_KEEPCAP) == 0 && end > mip->mi_end)
    {
	/* If "end" is past "mip->mi_end" we need to check the characters
	 * after the basic word. */
#ifdef FEAT_MBYTE
	past_second = (mip->mi_word + (*mb_ptr2len_check)(mip->mi_word)
							       < mip->mi_end);
#else
	past_second = mip->mi_word + 1 < mip->mi_end;
#endif
	for (p = mip->mi_end; p < end; )
	{
	    if (!spell_iswordc(p))
		mb_ptr_adv(p);
	    else
	    {
#ifdef FEAT_MBYTE
		if (has_mbyte)
		    c = mb_ptr2char_adv(&p);
		else
#endif
		    c = *p++;
		if (MB_ISUPPER(c))
		{
		    if (capflags == 0 || (capflags & BWF_ONECAP))
		    {
			capflags = BWF_KEEPCAP;	/* lU or UlU */
			break;
		    }
		}
		else
		{
		    if (capflags & BWF_ALLCAP)
		    {
			if (past_second)
			{
			    capflags = BWF_KEEPCAP;	/* UUl */
			    break;
			}
			capflags = BWF_ONECAP;		/* Uu */
		    }
		}
		past_second = TRUE;
	    }
	}
    }

    if (capflags == BWF_ALLCAP)
	return TRUE;		/* All caps is always OK. */

    if (flags & BWF_KEEPCAP)
    {
	len = STRLEN(caseword);
	return (len == end - cword && STRNCMP(caseword, cword, len) == 0);
    }

    if (flags & BWF_ALLCAP)
	return FALSE;		/* need ALLCAP, already checked above */

    if (flags & BWF_ONECAP)
	return capflags == BWF_ONECAP;

    return capflags != BWF_KEEPCAP;	/* no case check, only KEEPCAP is bad */
}

/*
 * Move to next spell error.
 * Return OK if found, FAIL otherwise.
 */
    int
spell_move_to(dir, allwords)
    int		dir;		/* FORWARD or BACKWARD */
    int		allwords;	/* TRUE for "[s" and "]s" */
{
    pos_T	pos;
    char_u	*line;
    char_u	*p;
    int		wc;
    int		nwc;
    int		attr = 0;
    int		len;

    if (!curwin->w_p_spell || *curwin->w_buffer->b_p_spl == NUL)
    {
	EMSG(_("E756: Spell checking not enabled"));
	return FAIL;
    }

    /* TODO: moving backwards */

    /* Start looking for bad word at the start of the line, because we can't
     * start halfway a word and know where it ends. */
    pos = curwin->w_cursor;
    pos.col = 0;
    wc = FALSE;

    while (!got_int)
    {
	line = ml_get(pos.lnum);
	p = line + pos.col;
	while (*p != NUL)
	{
	    nwc = spell_iswordc(p);
	    if (!wc && nwc)
	    {
		/* start of word */
		/* TODO: check for bad word attr */
		len = spell_check(curwin, line, p, &attr);
		if (attr != 0)
		{
		    if (curwin->w_cursor.lnum < pos.lnum
			    || (curwin->w_cursor.lnum == pos.lnum
				&& curwin->w_cursor.col < (colnr_T)(p - line)))
		    {
			curwin->w_cursor.lnum = pos.lnum;
			curwin->w_cursor.col = p - line;
			return OK;
		    }
		    attr = 0;	/* bad word is before or at cursor */
		}
		p += len;
		if (*p == NUL)
		    break;
		nwc = FALSE;
	    }

	    /* advance to next character */
	    mb_ptr_adv(p);
	    wc = nwc;
	}

	/* Advance to next line. */
	if (pos.lnum == curbuf->b_ml.ml_line_count)
	    return FAIL;
	++pos.lnum;
	pos.col = 0;
	wc = FALSE;

	line_breakcheck();
    }

    return FAIL;	/* interrupted */
}

/*
 * Load word list for "lang" from a Vim spell file.
 * "lang" must be the language without the region: "en" or "en-rare".
 */
    static slang_T *
spell_load_lang(lang)
    char_u	*lang;
{
    slang_T	*lp;
    char_u	fname_enc[80];
    char_u	*p;
    int		r;

    lp = slang_alloc(lang);
    if (lp != NULL)
    {
	/* Find all spell files for "lang" in 'runtimepath' and load them.
	 * Use 'encoding', except that we use "latin1" for "latin9". */
#ifdef FEAT_MBYTE
	if (STRLEN(p_enc) < 60 && STRCMP(p_enc, "iso-8859-15") != 0)
	    p = p_enc;
	else
#endif
	    p = (char_u *)"latin1";
	sprintf((char *)fname_enc, "spell/%s.%s.spl", lang, p);

	r = do_in_runtimepath(fname_enc, TRUE, spell_load_file, lp);
	if (r == FAIL || lp->sl_error)
	{
	    slang_free(lp);
	    lp = NULL;
	    if (r == FAIL)
		smsg((char_u *)_("Warning: Cannot find word list \"%s\""),
							       fname_enc + 6);
	}
	else
	{
	    lp->sl_next = first_lang;
	    first_lang = lp;
	}
    }

    return lp;
}

/*
 * Allocate a new slang_T.
 * Caller must fill "sl_next".
 */
    static slang_T *
slang_alloc(lang)
    char_u	*lang;
{
    slang_T *lp;

    lp = (slang_T *)alloc(sizeof(slang_T));
    if (lp != NULL)
    {
	lp->sl_name = vim_strsave(lang);
	hash_init(&lp->sl_words);
	ga_init2(&lp->sl_preftab, sizeof(hashtab_T), 4);
	ga_init2(&lp->sl_sufftab, sizeof(hashtab_T), 4);
	lp->sl_prefzero = NULL;
	lp->sl_suffzero = NULL;
	lp->sl_try = NULL;
	ga_init2(&lp->sl_rep, sizeof(repentry_T), 4);
	lp->sl_regions[0] = NUL;
	lp->sl_block = NULL;
	lp->sl_error = FALSE;
    }
    return lp;
}

/*
 * Free the contents of an slang_T and the structure itself.
 */
    static void
slang_free(lp)
    slang_T	*lp;
{
    sblock_T	*sp;
    int		i;

    vim_free(lp->sl_name);
    hash_clear(&lp->sl_words);
    for (i = 0; i < lp->sl_preftab.ga_len; ++i)
	hash_clear(((hashtab_T *)lp->sl_preftab.ga_data) + i);
    ga_clear(&lp->sl_preftab);
    for (i = 0; i < lp->sl_sufftab.ga_len; ++i)
	hash_clear(((hashtab_T *)lp->sl_sufftab.ga_data) + i);
    ga_clear(&lp->sl_sufftab);
    ga_clear(&lp->sl_rep);
    vim_free(lp->sl_try);
    while (lp->sl_block != NULL)
    {
	sp = lp->sl_block;
	lp->sl_block = sp->sb_next;
	vim_free(sp);
    }
    vim_free(lp);
}

/*
 * Load one spell file into an slang_T.
 * Invoked through do_in_runtimepath().
 */
    static void
spell_load_file(fname, cookie)
    char_u	*fname;
    void	*cookie;	    /* points to the slang_T to be filled */
{
    slang_T	*lp = cookie;
    FILE	*fd;
    char_u	buf[MAXWLEN + 1];
    char_u	cbuf[MAXWLEN + 1];
    char_u	fbuf[MAXWLEN + 1];
    char_u	*p;
    int		itm;
    int		i;
    int		affcount;
    int		affnr;
    int		affflags;
    int		affitemcnt;
    int		bl_used = SBLOCKSIZE;
    int		widx;
    int		prefm;	    /* 1 if <= 256 prefixes, sizeof(short_u) otherw. */
    int		suffm;	    /* 1 if <= 256 suffixes, sizeof(short_u) otherw. */
    int		wlen;
    int		flags;
    affitem_T	*ai, *ai2, **aip;
    int		round;
    char_u	*save_sourcing_name = sourcing_name;
    linenr_T	save_sourcing_lnum = sourcing_lnum;
    int		cnt;
    int		choplen;
    int		addlen;
    int		leadlen;
    int		wordcount;
    fword_T	*fw, *fw2;
    garray_T	*gap;
    hashtab_T	*ht;
    hashitem_T	*hi;
    hash_T	hash;
    int		adds;
    addword_T	*aw;
    int		flen;

    fd = fopen((char *)fname, "r");
    if (fd == NULL)
    {
	EMSG2(_(e_notopen), fname);
	goto errorend;
    }

    /* Set sourcing_name, so that error messages mention the file name. */
    sourcing_name = fname;
    sourcing_lnum = 0;

    /* <HEADER>: <fileID> <regioncnt> <regionname> ... */
    for (i = 0; i < VIMSPELLMAGICL; ++i)
	buf[i] = getc(fd);				/* <fileID> */
    if (STRNCMP(buf, VIMSPELLMAGIC, VIMSPELLMAGICL) != 0)
    {
	EMSG(_("E757: Wrong file ID in spell file"));
	goto errorend;
    }

    cnt = getc(fd);					/* <regioncnt> */
    if (cnt == EOF)
    {
truncerr:
	EMSG(_("E758: Truncated spell file"));
	goto errorend;
    }
    if (cnt > 8)
    {
formerr:
	EMSG(_("E759: Format error in spell file"));
	goto errorend;
    }
    for (i = 0; i < cnt; ++i)
    {
	lp->sl_regions[i * 2] = getc(fd);		/* <regionname> */
	lp->sl_regions[i * 2 + 1] = getc(fd);
    }
    lp->sl_regions[cnt * 2] = NUL;

    /* round 1: <PREFIXLIST>: <affcount> <afftotcnt> <affix> ...
     * round 2: <SUFFIXLIST>: <affcount> <afftotcnt> <affix> ...  */
    for (round = 1; round <= 2; ++round)
    {
	affcount = (getc(fd) << 8) + getc(fd);		/* <affcount> */
	if (affcount < 0)
	    goto truncerr;
	if (round == 1)
	{
	    gap = &lp->sl_preftab;
	    aip = &lp->sl_prefzero;
	    lp->sl_prefcnt = affcount;
	    prefm = affcount > 256 ? sizeof(short_u) : 1;
	}
	else
	{
	    gap = &lp->sl_sufftab;
	    aip = &lp->sl_suffzero;
	    lp->sl_suffcnt = affcount;
	    suffm = affcount > 256 ? sizeof(short_u) : 1;
	}

	i = (getc(fd) << 8) + getc(fd);		/* <afftotcnt> */
	/* afftotcnt is not used */

	/*
	 * For each affix NR there can be several affixes.
	 */
	for (affnr = 0; affnr < affcount; ++affnr)
	{
	    /* <affix>: <affflags> <affitemcnt> <affitem> ... */
	    affflags = getc(fd);			/* <affflags> */
	    if (affflags == EOF)
		goto truncerr;
	    affitemcnt = (getc(fd) << 8) + getc(fd);	/* <affitemcnt> */
	    if (affitemcnt < 0)
		goto truncerr;
	    for (itm = 0; itm < affitemcnt; ++itm)
	    {
		/* <affitem>: <affchoplen> <affchop> <affaddlen> <affadd> */
		choplen = getc(fd);			/* <affchoplen> */
		if (choplen == EOF)
		    goto truncerr;
		if (choplen >= MAXWLEN)
		    goto formerr;
		for (i = 0; i < choplen; ++i)		/* <affchop> */
		    buf[i] = getc(fd);
		buf[i] = NUL;
		addlen = getc(fd);			/* <affaddlen> */
		if (addlen == EOF)
		    goto truncerr;
		/* Get room to store the affitem_T, chop and add strings. */
		p = (char_u *)getroom(lp, &bl_used,
				    sizeof(affitem_T) + choplen + addlen + 1);
		if (p == NULL)
		    goto errorend;

		ai = (affitem_T *)p;
		ai->ai_nr = affnr;
		ai->ai_combine = affflags;
		ai->ai_choplen = choplen;
		ai->ai_addlen = addlen;

		p += sizeof(affitem_T) + addlen;
		ai->ai_chop = p;
		STRCPY(p, buf);

		p = ai->ai_add;
		for (i = 0; i < addlen; ++i)		/* <affadd> */
		    p[i] = getc(fd);
		p[i] = NUL;

		/*
		 * Add the affix to a hashtable.  Which one depends on the
		 * length of the added string in characters.
		 */
#ifdef FEAT_MBYTE
		/* Change "addlen" from length in bytes to length in chars. */
		if (has_mbyte)
		    addlen = mb_charlen(p);
#endif
		if (addlen == 0)
		{
		    /* Link in list of zero length affixes. */
		    ai->ai_next = *aip;
		    *aip = ai;
		}
		else
		{
		    if (gap->ga_len < addlen)
		    {
			/* Longer affix, need more hashtables. */
			if (ga_grow(gap, addlen - gap->ga_len) == FAIL)
			    goto errorend;

			/* Re-allocating ga_data means that an ht_array
			 * pointing to ht_smallarray becomes invalid.  We can
			 * recognize this: ht_mask is at its init value. */
			for (i = 0; i < gap->ga_len; ++i)
			{
			    ht = ((hashtab_T *)gap->ga_data) + i;
			    if (ht->ht_mask == HT_INIT_SIZE - 1)
				ht->ht_array = ht->ht_smallarray;
			}

			/* Init the newly used hashtable(s). */
			while (gap->ga_len < addlen)
			{
			    hash_init(((hashtab_T *)gap->ga_data)
							       + gap->ga_len);
			    ++gap->ga_len;
			}
		    }
		    ht = ((hashtab_T *)gap->ga_data) + addlen - 1;
		    hash = hash_hash(p);
		    hi = hash_lookup(ht, p, hash);
		    if (HASHITEM_EMPTY(hi))
		    {
			/* First affix with this "ai_add", add to hashtable. */
			hash_add_item(ht, hi, p, hash);
			ai->ai_next = NULL;
		    }
		    else
		    {
			/* There already is an affix with this "ai_add", link
			 * in the list.  */
			ai2 = HI2AI(hi);
			ai->ai_next = ai2->ai_next;
			ai2->ai_next = ai;
		    }
		}
	    }
	}
    }

    /* <SUGGEST> : <suggestlen> <more> ... */
    /* TODO, just skip this for now */
    i = (getc(fd) << 24) + (getc(fd) << 16) + (getc(fd) << 8) + getc(fd);
    while (i-- > 0)
	if (getc(fd) == EOF)				/* <suggestlen> */
	    goto truncerr;

    /* <WORDLIST>: <wordcount> <worditem> ... */	/* <wordcount> */
    wordcount = (getc(fd) << 24) + (getc(fd) << 16) + (getc(fd) << 8)
								   + getc(fd);
    if (wordcount < 0)
	goto truncerr;

    /* Init hashtable for this number of words, so that it doesn't need to
     * reallocate the table halfway. */
    hash_lock_size(&lp->sl_words, wordcount);

    for (widx = 0; ; ++widx)
    {
	/* <worditem>: <nr> <string> <flags> [<flags2>]
	 *			  [<caselen> <caseword>]
	 *			  [<affixcnt> <affixNR> ...]    (prefixes)
	 *			  [<affixcnt> <affixNR> ...]	(suffixes)
	 *			  [<region>]
	 *			  [<addcnt> <add> ...]
	 */
	/* Use <nr> bytes from the previous word. */
	wlen = getc(fd);				/* <nr> */
	if (wlen == EOF)
	{
	    if (widx >= wordcount)	/* normal way to end the file */
		break;
	    goto truncerr;
	}

	/* Read further word bytes until one below 0x20, that must be the
	 * flags.  Keep this fast! */
	for (;;)
	{
	    if ((buf[wlen] = getc(fd)) < 0x20)		/* <string> */
		break;
	    if (++wlen == MAXWLEN)
		goto formerr;
	}
	flags = buf[wlen];				/* <flags> */
	buf[wlen] = NUL;

	/* Get more flags if they're there. */
	if (flags & BWF_SECOND)
	    flags += getc(fd) << 8;			/* <flags2> */

	if (flags & BWF_KEEPCAP)
	{
	    /* Read <caselen> and <caseword> first, its length may differ from
	     * the case-folded word.  Note: this should only happen after the
	     * basic word! */
	    wlen = getc(fd);
	    if (wlen == EOF)
		goto truncerr;
	    for (i = 0; i < wlen; ++i)
		cbuf[i] = getc(fd);
	    cbuf[i] = NUL;
	}

	/* Find room to store the word in a fword_T. */
	fw = (fword_T *)getroom(lp, &bl_used, (int)sizeof(fword_T) + wlen);
	if (fw == NULL)
	    goto errorend;
	mch_memmove(fw->fw_word, (flags & BWF_KEEPCAP) ? cbuf : buf, wlen + 1);
	fw->fw_flags = flags;

	hash = hash_hash(buf);
	hi = hash_lookup(&lp->sl_words, buf, hash);
	if (HASHITEM_EMPTY(hi))
	{
	    if (hash_add_item(&lp->sl_words, hi, fw->fw_word, hash) == FAIL)
		goto errorend;
	    fw->fw_next = NULL;
	}
	else
	{
	    /* Already have this basic word in the hashtable, this one will
	     * have different case flags. */
	    fw2 = HI2FWORD(hi);
	    fw->fw_next = fw2->fw_next;
	    fw2->fw_next = fw;
	    --widx;			/* don't count this one */
	}

	/* Optional prefixes and suffixes. */
	if (flags & BWF_PREFIX)
	    fw->fw_prefixcnt = spell_load_affixes(fd, lp, &bl_used,
						       prefm, &fw->fw_prefix);
	else
	    fw->fw_prefixcnt = 0;
	if (flags & BWF_SUFFIX)
	    fw->fw_suffixcnt = spell_load_affixes(fd, lp, &bl_used,
						       suffm, &fw->fw_suffix);
	else
	    fw->fw_suffixcnt = 0;

	if (flags & BWF_REGION)
	    fw->fw_region = getc(fd);			/* <region> */
	else
	    fw->fw_region = REGION_ALL;

	fw->fw_adds = NULL;
	if (flags & BWF_ADDS)
	{
	    adds = (getc(fd) << 8) + getc(fd);		/* <addcnt> */

	    while (--adds >= 0)
	    {
		/* <add>: <addflags> <addlen> [<leadlen> <addstring>]
		 *			[<region>] */
		flags = getc(fd);			/* <addflags> */
		addlen = getc(fd);			/* <addlen> */
		if (addlen == EOF)
		    goto truncerr;
		if (addlen >= MAXWLEN)
		    goto formerr;

		if (addlen > 0)
		{
		    leadlen = getc(fd);			/* <leadlen> */
		    for (i = 0; i < addlen; ++i)	/* <addstring> */
			cbuf[i] = getc(fd);
		    cbuf[i] = NUL;
		}
		else
		    leadlen = 0;

		if (flags & ADD_KEEPCAP)
		{
		    /* <addstring> is in original case, need to get
		     * case-folded word too. */
		    (void)str_foldcase(cbuf, addlen, fbuf, MAXWLEN);
		    flen = addlen - leadlen + 1;
		    addlen = STRLEN(fbuf);
		}
		else
		    flen = 0;

		aw = (addword_T *)getroom(lp, &bl_used,
					   sizeof(addword_T) + addlen + flen);
		if (aw == NULL)
		    goto errorend;
		aw->aw_next = fw->fw_adds;
		fw->fw_adds = aw;
		aw->aw_leadlen = leadlen;

		if (flags & ADD_KEEPCAP)
		{
		    /* Put the addition in original case after the case-folded
		     * string. */
		    STRCPY(aw->aw_word, fbuf);
		    STRCPY(aw->aw_word + addlen + 1, cbuf + leadlen);
		}
		else
		    STRCPY(aw->aw_word, cbuf);

		aw->aw_flags = flags;
		aw->aw_wordlen = addlen;

		if (flags & ADD_REGION)
		    aw->aw_region = getc(fd);		/* <region> */
		else
		    aw->aw_region = REGION_ALL;
	    }
	}
    }
    goto end_OK;

errorend:
    lp->sl_error = TRUE;
end_OK:
    if (fd != NULL)
	fclose(fd);
    hash_unlock(&lp->sl_words);
    sourcing_name = save_sourcing_name;
    sourcing_lnum = save_sourcing_lnum;
}

/*
 * Read a list of affixes from the spell file.
 */
    static int
spell_load_affixes(fd, lp, bl_usedp, affm, affp)
    FILE	*fd;
    slang_T	*lp;
    int		*bl_usedp;
    int		affm;
    void	**affp;
{
    int		cnt;
    int		i, n;
    char_u	*p;

    cnt = getc(fd);				/* <affixcnt> */
    if (cnt == EOF)
	return 0;

    /* Get room to store the affixNR list, either as char_u (1
     * byte) or short_u (2 bytes). */
    p = (char_u *)getroom(lp, bl_usedp, cnt * affm);
    if (p == NULL)
	return 0;
    *affp = p;
    for (n = 0; n < cnt; ++n)
    {
	i = getc(fd);			/* <affixNR> */
	if (affm > 1)
	{
	    i = (i << 8) + getc(fd);
	    *(short_u *)p = i;
	    p += sizeof(short_u);
	}
	else
	{
	    *(char_u *)p = i;
	    ++p;
	}
    }
    return cnt;
}

/*
 * Get part of an sblock_T, at least "len" bytes long.
 * Returns NULL when out of memory.
 */
    static void *
getroom(lp, bl_used, len)
    slang_T	*lp;	    /* lp->sl_block is current block or NULL */
    int		*bl_used;    /* used up from current block */
    int		len;	    /* length needed */
{
    char_u	*p;
    sblock_T	*bl = lp->sl_block;

    if (bl == NULL || *bl_used + len > SBLOCKSIZE)
    {
	/* Allocate a block of memory. This is not freed until spell_reload()
	 * is called. */
	bl = (sblock_T *)alloc((unsigned)(sizeof(sblock_T) + SBLOCKSIZE));
	if (bl == NULL)
	    return NULL;
	bl->sb_next = lp->sl_block;
	lp->sl_block = bl;
	*bl_used = 0;
    }

    p = bl->sb_data + *bl_used;
    *bl_used += len;

    return p;
}

/*
 * Parse 'spelllang' and set buf->b_langp accordingly.
 * Returns an error message or NULL.
 */
    char_u *
did_set_spelllang(buf)
    buf_T	*buf;
{
    garray_T	ga;
    char_u	*lang;
    char_u	*e;
    char_u	*region;
    int		region_mask;
    slang_T	*lp;
    int		c;
    char_u	lbuf[MAXWLEN + 1];

    ga_init2(&ga, sizeof(langp_T), 2);

    /* loop over comma separated languages. */
    for (lang = buf->b_p_spl; *lang != NUL; lang = e)
    {
	e = vim_strchr(lang, ',');
	if (e == NULL)
	    e = lang + STRLEN(lang);
	if (e > lang + 2)
	{
	    if (e - lang >= MAXWLEN)
	    {
		ga_clear(&ga);
		return e_invarg;
	    }
	    if (lang[2] == '_')
		region = lang + 3;
	}
	else
	    region = NULL;

	for (lp = first_lang; lp != NULL; lp = lp->sl_next)
	    if (STRNICMP(lp->sl_name, lang, 2) == 0)
		break;

	if (lp == NULL)
	{
	    /* Not found, load the language. */
	    STRNCPY(lbuf, lang, e - lang);
	    lbuf[e - lang] = NUL;
	    if (region != NULL)
		mch_memmove(lbuf + 2, lbuf + 5, e - lang - 4);
	    lp = spell_load_lang(lbuf);
	}

	if (lp != NULL)
	{
	    if (region == NULL)
		region_mask = REGION_ALL;
	    else
	    {
		/* find region in sl_regions */
		c = find_region(lp->sl_regions, region);
		if (c == REGION_ALL)
		{
		    c = *e;
		    *e = NUL;
		    smsg((char_u *)_("Warning: region %s not supported"), lang);
		    *e = c;
		    region_mask = REGION_ALL;
		}
		else
		    region_mask = 1 << c;
	    }

	    if (ga_grow(&ga, 1) == FAIL)
	    {
		ga_clear(&ga);
		return e_outofmem;
	    }
	    LANGP_ENTRY(ga, ga.ga_len)->lp_slang = lp;
	    LANGP_ENTRY(ga, ga.ga_len)->lp_region = region_mask;
	    ++ga.ga_len;
	}

	if (*e == ',')
	    ++e;
    }

    /* Add a NULL entry to mark the end of the list. */
    if (ga_grow(&ga, 1) == FAIL)
    {
	ga_clear(&ga);
	return e_outofmem;
    }
    LANGP_ENTRY(ga, ga.ga_len)->lp_slang = NULL;
    ++ga.ga_len;

    /* Everything is fine, store the new b_langp value. */
    ga_clear(&buf->b_langp);
    buf->b_langp = ga;

    return NULL;
}

/*
 * Find the region "region[2]" in "rp" (points to "sl_regions").
 * Each region is simply stored as the two characters of it's name.
 * Returns the index if found, REGION_ALL if not found.
 */
    static int
find_region(rp, region)
    char_u	*rp;
    char_u	*region;
{
    int		i;

    for (i = 0; ; i += 2)
    {
	if (rp[i] == NUL)
	    return REGION_ALL;
	if (rp[i] == region[0] && rp[i + 1] == region[1])
	    break;
    }
    return i / 2;
}

/*
 * Return type of word:
 * w word	0
 * Word		BWF_ONECAP
 * W WORD	BWF_ALLCAP
 * WoRd	wOrd	BWF_KEEPCAP
 */
    static int
captype(word, end)
    char_u	*word;
    char_u	*end;
{
    char_u	*p;
    int		c;
    int		firstcap;
    int		allcap;
    int		past_second = FALSE;	/* past second word char */

    /* find first letter */
    for (p = word; !spell_iswordc(p); mb_ptr_adv(p))
	if (p >= end)
	    return 0;	    /* only non-word characters, illegal word */
#ifdef FEAT_MBYTE
    c = mb_ptr2char_adv(&p);
#else
    c = *p++;
#endif
    firstcap = allcap = MB_ISUPPER(c);

    /*
     * Need to check all letters to find a word with mixed upper/lower.
     * But a word with an upper char only at start is a ONECAP.
     */
    for ( ; p < end; mb_ptr_adv(p))
	if (spell_iswordc(p))
	{
#ifdef FEAT_MBYTE
	    c = mb_ptr2char(p);
#else
	    c = *p;
#endif
	    if (!MB_ISUPPER(c))
	    {
		/* UUl -> KEEPCAP */
		if (past_second && allcap)
		    return BWF_KEEPCAP;
		allcap = FALSE;
	    }
	    else if (!allcap)
		/* UlU -> KEEPCAP */
		return BWF_KEEPCAP;
	    past_second = TRUE;
	}

    if (allcap)
	return BWF_ALLCAP;
    if (firstcap)
	return BWF_ONECAP;
    return 0;
}

# if defined(FEAT_MBYTE) || defined(PROTO)
/*
 * Clear all spelling tables and reload them.
 * Used after 'encoding' is set.
 */
    void
spell_reload()
{
    buf_T	*buf;
    slang_T	*lp;

    /* Initialize the table for spell_iswordc(). */
    init_spell_chartab();

    /* Unload all allocated memory. */
    while (first_lang != NULL)
    {
	lp = first_lang;
	first_lang = lp->sl_next;
	slang_free(lp);
    }

    /* Go through all buffers and handle 'spelllang'. */
    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
    {
	ga_clear(&buf->b_langp);
	if (*buf->b_p_spl != NUL)
	    did_set_spelllang(buf);
    }
}
# endif

/*
 * Recognizing words uses a two-step mechanism:
 * 1. Locate a basic word, made out of word characters only and separated by
 *    non-word characters.
 * 2. When a basic word is found, check if (possibly required) additions
 *    before and after the word are present.
 *
 * Both mechanisms use affixes (prefixes and suffixes) to reduce the number of
 * words.  When no matching word was found in the hashtable the start of the
 * word is checked for matching prefixes and the end of the word for matching
 * suffixes.  All matching affixes are removed and then the resulting word is
 * searched for.  If found it is checked if it supports the used affix.
 */


#if defined(FEAT_MBYTE) || defined(PROTO)
/*
 * Functions for ":mkspell".
 * Only possible with the multi-byte feature.
 */

#define MAXLINELEN  300		/* Maximum length in bytes of a line in a .aff
				   and .dic file. */
/*
 * Main structure to store the contents of a ".aff" file.
 */
typedef struct afffile_S
{
    char_u	*af_enc;	/* "SET", normalized, alloc'ed string or NULL */
    char_u	*af_try;	/* "TRY" line in "af_enc" encoding */
    hashtab_T	af_pref;	/* hashtable for prefixes, affheader_T */
    hashtab_T	af_suff;	/* hashtable for suffixes, affheader_T */
    garray_T	af_rep;		/* list of repentry_T entries from REP lines */
} afffile_T;

typedef struct affentry_S affentry_T;

/* Affix header from ".aff" file.  Used for af_pref and af_suff. */
typedef struct affheader_S
{
    char_u	ah_key[2];	/* key for hashtable == name of affix entry */
    int		ah_combine;
    affentry_T	*ah_first;	/* first affix entry */
    short_u	ah_affnr;	/* used in get_new_aff() */
} affheader_T;

#define HI2AH(hi)   ((affheader_T *)(hi)->hi_key)

/* Affix entry from ".aff" file.  Used for prefixes and suffixes. */
struct affentry_S
{
    affentry_T	*ae_next;	/* next affix with same name/number */
    char_u	*ae_chop;	/* text to chop off basic word (can be NULL) */
    char_u	*ae_add;	/* text to add to basic word (can be NULL) */
    char_u	*ae_add_nw;	/* first non-word character in "ae_add" */
    char_u	*ae_cond;	/* condition (NULL for ".") */
    regprog_T	*ae_prog;	/* regexp program for ae_cond or NULL */
    short_u	ae_affnr;	/* for old affix: new affix number */
};

/*
 * Structure to store a word from a ".dic" file.
 */
typedef struct dicword_S
{
    char_u	*dw_affnm;	/* original affix names */
    char_u	dw_word[1];	/* actually longer: the word in 'encoding' */
} dicword_T;

static dicword_T dumdw;
#define HI2DW(hi)	((dicword_T *)((hi)->hi_key - (dumdw.dw_word - (char_u *)&dumdw)))

/*
 * Structure to store a basic word for the spell file.
 * This is used for ":mkspell", not for spell checking.
 */
typedef struct basicword_S basicword_T;
struct basicword_S
{
    basicword_T	*bw_next;	/* next word with same basic word */
    basicword_T	*bw_cnext;	/* next word with same caps */
    int		bw_flags;	/* BWF_ flags */
    garray_T	bw_prefix;	/* table with prefix numbers */
    garray_T	bw_suffix;	/* table with suffix numbers */
    int		bw_region;	/* region bits */
    char_u	*bw_caseword;	/* keep-case word */
    char_u	*bw_leadstring;	/* must come before bw_word */
    char_u	*bw_addstring;	/* must come after bw_word */
    char_u	bw_word[1];	/* actually longer: word case folded */
};

static basicword_T dumbw;
#define KEY2BW(p)	((basicword_T *)((p) - (dumbw.bw_word - (char_u *)&dumbw)))
#define HI2BW(hi)	KEY2BW((hi)->hi_key)

/* Store the affix number related with a certain string. */
typedef struct affhash_S
{
    short_u	as_nr;		/* the affix nr */
    char_u	as_word[1];	/* actually longer */
} affhash_T;

static affhash_T dumas;
#define HI2AS(hi)	((affhash_T *)((hi)->hi_key - (dumas.as_word - (char_u *)&dumas)))


static afffile_T *spell_read_aff __ARGS((char_u *fname, vimconv_T *conv));
static void spell_free_aff __ARGS((afffile_T *aff));
static int spell_read_dic __ARGS((hashtab_T *ht, char_u *fname, vimconv_T *conv));
static int get_new_aff __ARGS((hashtab_T *oldaff, garray_T *gap));
static void spell_free_dic __ARGS((hashtab_T *dic));
static int same_affentries __ARGS((affheader_T *ah1, affheader_T *ah2));
static void add_affhash __ARGS((hashtab_T *ht, char_u *key, int newnr));
static void clear_affhash __ARGS((hashtab_T *ht));
static void trans_affixes __ARGS((dicword_T *dw, basicword_T *bw, afffile_T *oldaff, hashtab_T *newwords));
static int build_wordlist __ARGS((hashtab_T *newwords, hashtab_T *oldwords, afffile_T *oldaff, int regionmask));
static void combine_regions __ARGS((hashtab_T *newwords));
static int same_affixes __ARGS((basicword_T *bw, basicword_T *nbw));
static void expand_affixes __ARGS((hashtab_T *newwords, garray_T *prefgap, garray_T *suffgap));
static void expand_one_aff __ARGS((basicword_T *bw, garray_T *add_words, affentry_T *pae, affentry_T *sae));
static void add_to_wordlist __ARGS((hashtab_T *newwords, basicword_T *bw));
static void put_bytes __ARGS((FILE *fd, long_u nr, int len));
static void write_affix __ARGS((FILE *fd, affheader_T *ah));
static void write_affixlist __ARGS((FILE *fd, garray_T *aff, int bytes));
static void write_vim_spell __ARGS((char_u *fname, garray_T *prefga, garray_T *suffga, hashtab_T *newwords, int regcount, char_u *regchars));
static void write_bword __ARGS((FILE *fd, basicword_T *bw, int lowcap, basicword_T **prevbw, int regionmask, int prefm, int suffm));
static void free_wordtable __ARGS((hashtab_T *ht));
static void free_basicword __ARGS((basicword_T *bw));
static void free_affixentries __ARGS((affentry_T *first));

/*
 * Read an affix ".aff" file.
 * Returns an afffile_T, NULL for failure.
 */
    static afffile_T *
spell_read_aff(fname, conv)
    char_u	*fname;
    vimconv_T	*conv;		/* info for encoding conversion */
{
    FILE	*fd;
    afffile_T	*aff;
    char_u	rline[MAXLINELEN];
    char_u	*line;
    char_u	*pc = NULL;
    char_u	*(items[6]);
    int		itemcnt;
    char_u	*p;
    int		lnum = 0;
    affheader_T	*cur_aff = NULL;
    int		aff_todo = 0;
    hashtab_T	*tp;

    fd = fopen((char *)fname, "r");
    if (fd == NULL)
    {
	EMSG2(_(e_notopen), fname);
	return NULL;
    }

    smsg((char_u *)_("Reading affix file %s..."), fname);
    out_flush();

    aff = (afffile_T *)alloc_clear((unsigned)sizeof(afffile_T));
    if (aff == NULL)
	return NULL;
    hash_init(&aff->af_pref);
    hash_init(&aff->af_suff);
    ga_init2(&aff->af_rep, (int)sizeof(repentry_T), 20);

    /*
     * Read all the lines in the file one by one.
     */
    while (!vim_fgets(rline, MAXLINELEN, fd))
    {
	++lnum;

	/* Skip comment lines. */
	if (*rline == '#')
	    continue;

	/* Convert from "SET" to 'encoding' when needed. */
	vim_free(pc);
	if (conv->vc_type != CONV_NONE)
	{
	    pc = string_convert(conv, rline, NULL);
	    line = pc;
	}
	else
	{
	    pc = NULL;
	    line = rline;
	}

	/* Split the line up in white separated items.  Put a NUL after each
	 * item. */
	itemcnt = 0;
	for (p = line; ; )
	{
	    while (*p != NUL && *p <= ' ')  /* skip white space and CR/NL */
		++p;
	    if (*p == NUL)
		break;
	    items[itemcnt++] = p;
	    while (*p > ' ')  /* skip until white space or CR/NL */
		++p;
	    if (*p == NUL)
		break;
	    *p++ = NUL;
	}

	/* Handle non-empty lines. */
	if (itemcnt > 0)
	{
	    if (STRCMP(items[0], "SET") == 0 && itemcnt == 2
						       && aff->af_enc == NULL)
	    {
		if (aff->af_enc != NULL)
		    smsg((char_u *)_("Duplicate SET line ignored in %s line %d: %s"),
						       fname, lnum, line);
		else
		{
		    /* Setup for conversion from "ENC" to 'encoding'. */
		    aff->af_enc = enc_canonize(items[1]);
		    if (aff->af_enc != NULL
			    && convert_setup(conv, aff->af_enc, p_enc) == FAIL)
			smsg((char_u *)_("Conversion in %s not supported: from %s to %s"),
						   fname, aff->af_enc, p_enc);
		}
	    }
	    else if (STRCMP(items[0], "TRY") == 0 && itemcnt == 2
						       && aff->af_try == NULL)
		aff->af_try = vim_strsave(items[1]);
	    else if ((STRCMP(items[0], "PFX") == 0
					      || STRCMP(items[0], "SFX") == 0)
		    && aff_todo == 0
		    && itemcnt == 4)
	    {
		/* New affix letter. */
		cur_aff = (affheader_T *)alloc((unsigned)sizeof(affheader_T));
		if (cur_aff == NULL)
		    break;
		cur_aff->ah_key[0] = *items[1];
		cur_aff->ah_key[1] = NUL;
		if (items[1][1] != NUL)
		    smsg((char_u *)_("Affix name too long in %s line %d: %s"),
						       fname, lnum, items[1]);
		if (*items[2] == 'Y')
		    cur_aff->ah_combine = TRUE;
		else if (*items[2] == 'N')
		    cur_aff->ah_combine = FALSE;
		else if (p_verbose > 0)
		    smsg((char_u *)_("Expected Y or N in %s line %d: %s"),
						       fname, lnum, items[2]);
		cur_aff->ah_first = NULL;
		if (*items[0] == 'P')
		    tp = &aff->af_pref;
		else
		    tp = &aff->af_suff;
		if (!HASHITEM_EMPTY(hash_find(tp, cur_aff->ah_key)))
		    smsg((char_u *)_("Duplicate affix in %s line %d: %s"),
						       fname, lnum, items[1]);
		else
		    hash_add(tp, cur_aff->ah_key);

		aff_todo = atoi((char *)items[3]);
	    }
	    else if ((STRCMP(items[0], "PFX") == 0
					      || STRCMP(items[0], "SFX") == 0)
		    && aff_todo > 0
		    && STRCMP(cur_aff->ah_key, items[1]) == 0
		    && itemcnt == 5)
	    {
		affentry_T	*aff_entry;

		/* New item for an affix letter. */
		--aff_todo;
		aff_entry = (affentry_T *)alloc_clear(
						(unsigned)sizeof(affentry_T));
		if (aff_entry == NULL)
		    break;
		aff_entry->ae_next = cur_aff->ah_first;
		cur_aff->ah_first = aff_entry;
		if (STRCMP(items[2], "0") != 0)
		    aff_entry->ae_chop = vim_strsave(items[2]);
		if (STRCMP(items[3], "0") != 0)
		    aff_entry->ae_add = vim_strsave(items[3]);
		if (STRCMP(items[4], ".") != 0)
		{
		    char_u	buf[MAXLINELEN];

		    aff_entry->ae_cond = vim_strsave(items[4]);
		    if (*items[0] == 'P')
			sprintf((char *)buf, "^%s", items[4]);
		    else
			sprintf((char *)buf, "%s$", items[4]);
		    aff_entry->ae_prog = vim_regcomp(buf, RE_MAGIC + RE_STRING);
		}
	    }
	    else if (STRCMP(items[0], "REP") == 0 && itemcnt == 2)
		/* Ignore REP count */;
	    else if (STRCMP(items[0], "REP") == 0 && itemcnt == 3)
	    {
		repentry_T  *rp;

		/* REP item */
		if (ga_grow(&aff->af_rep, 1) == FAIL)
		    break;
		rp = ((repentry_T *)aff->af_rep.ga_data) + aff->af_rep.ga_len;
		rp->re_from = vim_strsave(items[1]);
		rp->re_to = vim_strsave(items[2]);
		++aff->af_rep.ga_len;
	    }
	    else if (p_verbose > 0)
		smsg((char_u *)_("Unrecognized item in %s line %d: %s"),
						       fname, lnum, items[0]);
	}

    }

    vim_free(pc);
    fclose(fd);
    return aff;
}

/*
 * Free the structure filled by spell_read_aff().
 */
    static void
spell_free_aff(aff)
    afffile_T	*aff;
{
    hashtab_T	*ht;
    hashitem_T	*hi;
    int		todo;
    int		i;
    repentry_T  *rp;
    affheader_T	*ah;

    vim_free(aff->af_enc);
    vim_free(aff->af_try);

    for (ht = &aff->af_pref; ; ht = &aff->af_suff)
    {
	todo = ht->ht_used;
	for (hi = ht->ht_array; todo > 0; ++hi)
	{
	    if (!HASHITEM_EMPTY(hi))
	    {
		--todo;
		ah = HI2AH(hi);
		free_affixentries(ah->ah_first);
		vim_free(ah);
	    }
	}
	if (ht == &aff->af_suff)
	    break;
    }
    hash_clear(&aff->af_pref);
    hash_clear(&aff->af_suff);

    for (i = 0; i < aff->af_rep.ga_len; ++i)
    {
	rp = ((repentry_T *)aff->af_rep.ga_data) + i;
	vim_free(rp->re_from);
	vim_free(rp->re_to);
    }
    ga_clear(&aff->af_rep);

    vim_free(aff);
}

/*
 * Read a dictionary ".dic" file.
 * Returns OK or FAIL;
 * Each entry in the hashtab_T is a dicword_T.
 */
    static int
spell_read_dic(ht, fname, conv)
    hashtab_T	*ht;
    char_u	*fname;
    vimconv_T	*conv;		/* info for encoding conversion */
{
    char_u	line[MAXLINELEN];
    char_u	*p;
    dicword_T	*dw;
    char_u	*pc;
    char_u	*w;
    int		l;
    hash_T	hash;
    hashitem_T	*hi;
    FILE	*fd;
    int		lnum = 1;

    fd = fopen((char *)fname, "r");
    if (fd == NULL)
    {
	EMSG2(_(e_notopen), fname);
	return FAIL;
    }

    smsg((char_u *)_("Reading dictionary file %s..."), fname);
    out_flush();

    /* Read and ignore the first line: word count. */
    (void)vim_fgets(line, MAXLINELEN, fd);
    if (!isdigit(*skipwhite(line)))
	EMSG2(_("E760: No word count in %s"), fname);

    /*
     * Read all the lines in the file one by one.
     * The words are converted to 'encoding' here, before being added to
     * the hashtable.
     */
    while (!vim_fgets(line, MAXLINELEN, fd))
    {
	++lnum;

	/* Remove CR, LF and white space from end. */
	l = STRLEN(line);
	while (l > 0 && line[l - 1] <= ' ')
	    --l;
	if (l == 0)
	    continue;	/* empty line */
	line[l] = NUL;

	/* Find the optional affix names. */
	p = vim_strchr(line, '/');
	if (p != NULL)
	    *p++ = NUL;

	/* Convert from "SET" to 'encoding' when needed. */
	if (conv->vc_type != CONV_NONE)
	{
	    pc = string_convert(conv, line, NULL);
	    w = pc;
	}
	else
	{
	    pc = NULL;
	    w = line;
	}

	dw = (dicword_T *)alloc_clear((unsigned)sizeof(dicword_T)
							     + STRLEN(w));
	if (dw == NULL)
	    break;
	STRCPY(dw->dw_word, w);
	vim_free(pc);

	hash = hash_hash(dw->dw_word);
	hi = hash_lookup(ht, dw->dw_word, hash);
	if (!HASHITEM_EMPTY(hi))
	    smsg((char_u *)_("Duplicate word in %s line %d: %s"),
						       fname, lnum, line);
	else
	    hash_add_item(ht, hi, dw->dw_word, hash);

	if (p != NULL)
	    dw->dw_affnm = vim_strsave(p);
    }

    fclose(fd);
    return OK;
}

/*
 * Free the structure filled by spell_read_dic().
 */
    static void
spell_free_dic(dic)
    hashtab_T	*dic;
{
    int		todo;
    dicword_T	*dw;
    hashitem_T	*hi;

    todo = dic->ht_used;
    for (hi = dic->ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    dw = HI2DW(hi);
	    vim_free(dw->dw_affnm);
	    vim_free(dw);
	}
    }
    hash_clear(dic);
}

/*
 * Take the affixes read by spell_read_aff() and add them to the new list.
 * Attempts to re-use the same number for identical affixes (ignoring the
 * condition, since we remove that).  That is especially important when using
 * multiple regions.
 * Returns OK or FAIL;
 */
    static int
get_new_aff(oldaff, gap)
    hashtab_T	*oldaff;	/* hashtable with affheader_T */
    garray_T	*gap;		/* table with new affixes */
{
    int		oldtodo;
    affheader_T	*oldah, *newah, *gapah;
    affentry_T	*oldae, *newae;
    hashitem_T	*oldhi;
    hashitem_T	*hi;
    hashtab_T	condht;		/* conditions already found */
    char_u	condkey[MAXLINELEN];
    int		newnr;
    int		gapnr;
    int		retval = OK;
    char_u	*p;
    garray_T	tga;

    /*
     * Loop over all the old affix names.
     */
    oldtodo = oldaff->ht_used;
    for (oldhi = oldaff->ht_array; oldtodo > 0 && retval == OK; ++oldhi)
    {
	if (!HASHITEM_EMPTY(oldhi))
	{
	    --oldtodo;
	    oldah = (affheader_T *)oldhi->hi_key;

	    /* Put entries with the same condition under the same new affix
	     * nr in "tga".  Use hashtable "condht" to find them. */
	    ga_init2(&tga, sizeof(affheader_T), 10);
	    hash_init(&condht);

	    /*
	     * Loop over all affixes with the same name.
	     * The affixes with the same condition will get the same number,
	     * since they can be used with the same words.
	     * 1. build the lists of new affentry_T, with the headers in "tga".
	     * 2. Check if some of the lists already exist in "gap", re-use
	     *    their number.
	     * 3. Assign the new numbers to the old affixes.
	     */

	    /* 1. build the lists of new affentry_T. */
	    for (oldae = oldah->ah_first; oldae != NULL && retval == OK;
						       oldae = oldae->ae_next)
	    {
		oldae->ae_add_nw = NULL;
		if (oldae->ae_add != NULL)
		{
		    /* Check for non-word characters in the suffix.  If there
		     * is one this affix will be turned into an addition.
		     * This is stored with the old affix, that is where
		     * trans_affixes() will check. */
		    for (p = oldae->ae_add; *p != NUL; mb_ptr_adv(p))
			if (!spell_iswordc(p))
			    break;
		    if (*p != NUL)
			oldae->ae_add_nw = p;
		}

		if (oldae->ae_cond == NULL)
		    /* hashtable requires a non-empty key */
		    STRCPY(condkey, "---");
		else
		    STRCPY(condkey, oldae->ae_cond);

		/* Look for an existing list with this name and condition. */
		hi = hash_find(&condht, condkey);
		if (!HASHITEM_EMPTY(hi))
		    /* Match with existing affix, use that one. */
		    newnr = HI2AS(hi)->as_nr;
		else
		{
		    /* Add a new affix number. */
		    newnr = tga.ga_len;
		    if (ga_grow(&tga, 1) == FAIL)
			retval = FAIL;
		    else
		    {
			newah = ((affheader_T *)tga.ga_data) + newnr;
			newah->ah_combine = oldah->ah_combine;
			newah->ah_first = NULL;
			++tga.ga_len;

			/* Add the new list to the condht hashtable. */
			add_affhash(&condht, condkey, newnr);
		    }
		}

		/* Add the new affentry_T to the list. */
		newah = ((affheader_T *)tga.ga_data) + newnr;
		newae = (affentry_T *)alloc_clear((unsigned)sizeof(affentry_T));
		if (newae == NULL)
		    retval = FAIL;
		else
		{
		    newae->ae_next = newah->ah_first;
		    newah->ah_first = newae;
		    if (oldae->ae_chop == NULL)
			newae->ae_chop = NULL;
		    else
			newae->ae_chop = vim_strsave(oldae->ae_chop);
		    if (oldae->ae_add == NULL)
			newae->ae_add = NULL;
		    else
			newae->ae_add = vim_strsave(oldae->ae_add);

		    /* The condition is not copied, since the new affix is
		     * only used for words where the condition matches. */
		}
	    }

	    /* 2. Check if some of the lists already exist, re-use their
	     *    number.  Otherwise add the list to "gap". */
	    for (newnr = 0; newnr < tga.ga_len; ++newnr)
	    {
		newah = ((affheader_T *)tga.ga_data) + newnr;
		for (gapnr = 0; gapnr < gap->ga_len; ++gapnr)
		{
		    gapah = ((affheader_T *)gap->ga_data) + gapnr;
		    if (same_affentries(newah, gapah))
			/* Found an existing affheader_T entry with same
			 * affentry_T list, use its number. */
			break;
		}

		newah->ah_affnr = gapnr;
		if (gapnr == gap->ga_len)
		{
		    /* This is a new affentry_T list, add it. */
		    if (ga_grow(gap, 1) == FAIL)
			retval = FAIL;
		    else
		    {
			*(((affheader_T *)gap->ga_data) + gap->ga_len) = *newah;
			++gap->ga_len;
		    }
		}
		else
		{
		    /* free unused affentry_T list */
		    free_affixentries(newah->ah_first);
		}
	    }

	    /* 3. Assign the new affix numbers to the old affixes. */
	    for (oldae = oldah->ah_first; oldae != NULL && retval == OK;
						       oldae = oldae->ae_next)
	    {
		if (oldae->ae_cond == NULL)
		    /* hashtable requires a non-empty key */
		    STRCPY(condkey, "---");
		else
		    STRCPY(condkey, oldae->ae_cond);

		/* Look for an existing affix with this name and condition. */
		hi = hash_find(&condht, condkey);
		if (!HASHITEM_EMPTY(hi))
		    /* Match with existing affix, use that one. */
		    newnr = HI2AS(hi)->as_nr;
		else
		{
		    EMSG(_(e_internal));
		    retval = FAIL;
		}
		newah = ((affheader_T *)tga.ga_data) + newnr;
		oldae->ae_affnr = newah->ah_affnr;
	    }

	    ga_clear(&tga);
	    clear_affhash(&condht);
	}
    }

    return retval;
}

/*
 * Return TRUE if the affentry_T lists for "ah1" and "ah2" contain the same
 * items, ignoring the order.
 * Only compares the chop and add strings, not the condition.
 */
    static int
same_affentries(ah1, ah2)
    affheader_T	*ah1;
    affheader_T	*ah2;
{
    affentry_T	*ae1, *ae2;

    /* Check the length of the lists first. */
    ae2 = ah2->ah_first;
    for (ae1 = ah1->ah_first; ae1 != NULL; ae1 = ae1->ae_next)
    {
	if (ae2 == NULL)
	    return FALSE;	/* "ah1" list is longer */
	ae2 = ae2->ae_next;
    }
    if (ae2 != NULL)
	return FALSE;		/* "ah2" list is longer */

    /* Check that each entry in "ah1" appears in "ah2". */
    for (ae1 = ah1->ah_first; ae1 != NULL; ae1 = ae1->ae_next)
    {
	for (ae2 = ah2->ah_first; ae2 != NULL; ae2 = ae2->ae_next)
	{
	    if ((ae1->ae_chop == NULL) == (ae2->ae_chop == NULL)
		&& (ae1->ae_add == NULL) == (ae2->ae_add == NULL)
		&& (ae1->ae_chop == NULL
				   || STRCMP(ae1->ae_chop, ae2->ae_chop) == 0)
		&& (ae1->ae_add == NULL
				    || STRCMP(ae1->ae_add, ae2->ae_add) == 0))
		break;
	}
	if (ae2 == NULL)
	    return FALSE;
    }

    return TRUE;
}

/*
 * Add a chop/add or cond hashtable entry.
 */
    static void
add_affhash(ht, key, newnr)
    hashtab_T	*ht;
    char_u	*key;
    int		newnr;
{
    affhash_T	*as;

    as = (affhash_T *)alloc((unsigned)sizeof(affhash_T) + STRLEN(key));
    if (as != NULL)
    {
	as->as_nr = newnr;
	STRCPY(as->as_word, key);
	hash_add(ht, as->as_word);
    }
}

/*
 * Clear the chop/add hashtable used to detect identical affixes.
 */
    static void
clear_affhash(ht)
    hashtab_T	*ht;
{
    int		todo;
    hashitem_T	*hi;

    todo = ht->ht_used;
    for (hi = ht->ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    vim_free(HI2AS(hi));
	}
    }
    hash_clear(ht);
}

/*
 * Translate list of affix names for an old word to affix numbers in a new
 * basic word.
 * This checks if the conditions match with the old word.  The result is that
 * the new affix does not need to store the condition.
 */
    static void
trans_affixes(dw, bw, oldaff, newwords)
    dicword_T	*dw;		/* old word */
    basicword_T *bw;		/* basic word */
    afffile_T	*oldaff;	/* affixes for "oldwords" */
    hashtab_T	*newwords;	/* table with words */
{
    char_u	key[2];
    char_u	*p;
    char_u	*affnm;
    garray_T	*gap;
    hashitem_T	*aff_hi;
    affheader_T	*ah;
    affentry_T	*ae;
    regmatch_T	regmatch;
    int		i;
    basicword_T *nbw;
    int		alen;
    int		wlen;
    garray_T	fixga;
    char_u	nword[MAXWLEN];
    int		flags;
    int		n;

    ga_init2(&fixga, (int)sizeof(basicword_T *), 5);

    /* Loop over all the affix names of the old word. */
    key[1] = NUL;
    for (affnm = dw->dw_affnm; *affnm != NUL; ++affnm)
    {
	key[0] = *affnm;
	aff_hi = hash_find(&oldaff->af_pref, key);
	if (!HASHITEM_EMPTY(aff_hi))
	    gap = &bw->bw_prefix;	/* found a prefix */
	else
	{
	    gap = &bw->bw_suffix;	/* must be a suffix */
	    aff_hi = hash_find(&oldaff->af_suff, key);
	    if (HASHITEM_EMPTY(aff_hi))
	    {
		smsg((char_u *)_("No affix entry '%s' for word %s"),
							    key, dw->dw_word);
		continue;
	    }
	}

	/* Loop over all the affix entries for this affix name. */
	ah = HI2AH(aff_hi);
	for (ae = ah->ah_first; ae != NULL; ae = ae->ae_next)
	{
	    regmatch.regprog = ae->ae_prog;
	    regmatch.rm_ic = FALSE;	/* TODO: Should this be TRUE??? */
	    if (ae->ae_prog == NULL
			   || vim_regexec(&regmatch, dw->dw_word, (colnr_T)0))
	    {
		if (ae->ae_add_nw != NULL && (gap == &bw->bw_suffix
			    ? bw->bw_addstring : bw->bw_leadstring) == NULL)
		{
		    /* Affix has a non-word character and isn't prepended to
		     * leader or appended to addition.  Need to use another
		     * word with an addition.  It's a copy of the basicword_T
		     * "bw". */
		    if (gap == &bw->bw_suffix)
		    {
			alen = ae->ae_add_nw - ae->ae_add;
			nbw = (basicword_T *)alloc((unsigned)(
				    sizeof(basicword_T) + STRLEN(bw->bw_word)
								 + alen + 1));
			if (nbw != NULL)
			{
			    *nbw = *bw;
			    ga_init2(&nbw->bw_prefix, sizeof(short_u), 1);
			    ga_init2(&nbw->bw_suffix, sizeof(short_u), 1);

			    /* Adding the suffix may change the caps. */
			    STRCPY(nword, dw->dw_word);
			    if (ae->ae_chop != NULL)
			    {
				/* Remove chop string. */
				p = nword + STRLEN(nword);
				for (i = mb_charlen(ae->ae_chop); i > 0; --i)
				    mb_ptr_back(nword, p);
				*p = NUL;
			    }
			    STRCAT(nword, ae->ae_add);
			    flags = captype(nword, nword + STRLEN(nword));
			    if (flags & BWF_KEEPCAP)
			    {
				nword[STRLEN(dw->dw_word) + alen] = NUL;
				nbw->bw_caseword = vim_strsave(nword);
			    }
			    nbw->bw_flags &= ~(BWF_ONECAP | BWF_ALLCAP
							       | BWF_KEEPCAP);
			    nbw->bw_flags |= flags;

			    if (bw->bw_leadstring != NULL)
				nbw->bw_leadstring =
					       vim_strsave(bw->bw_leadstring);
			    nbw->bw_addstring = vim_strsave(ae->ae_add_nw);

			    STRCPY(nbw->bw_word, bw->bw_word);
			    if (alen > 0 || ae->ae_chop != NULL)
			    {
				/* Suffix starts with word character.  Append
				 * it to the word.  Add new word entry. */
				wlen = STRLEN(nbw->bw_word);
				if (ae->ae_chop != NULL)
				    wlen -= STRLEN(ae->ae_chop);
				mch_memmove(nbw->bw_word + wlen, ae->ae_add,
									alen);
				nbw->bw_word[wlen + alen] = NUL;
				add_to_wordlist(newwords, nbw);
			    }
			    else
				/* Basic word is the same, link "nbw" after
				 * "bw". */
				bw->bw_next = nbw;

			    /* Remember this word, we need to set bw_prefix
			     * and bw_suffix later. */
			    if (ga_grow(&fixga, 1) == OK)
				((basicword_T **)fixga.ga_data)[fixga.ga_len++]
									= nbw;
			}
		    }
		    else
		    {
			/* TODO: prefix with non-word char */
		    }
		}
		else
		{
		    /* Affix applies to this word, add the related affix
		     * number.  But only if it's not there yet.  And keep the
		     * list sorted, so that we can compare it later. */
		    for (i = 0; i < gap->ga_len; ++i)
		    {
			n = ((short_u *)gap->ga_data)[i];
			if (n >= ae->ae_affnr)
			{
			    if (n == ae->ae_affnr)
				i = -1;
			    break;
			}
		    }
		    if (i >= 0 && ga_grow(gap, 1) == OK)
		    {
			if (i < gap->ga_len)
			    mch_memmove(((short_u *)gap->ga_data) + i + 1,
					((short_u *)gap->ga_data) + i,
					 sizeof(short_u) * (gap->ga_len - i));
			((short_u *)gap->ga_data)[i] = ae->ae_affnr;
			++gap->ga_len;
		    }
		}
	    }
	}
    }

    /*
     * For the words that we added for suffixes with non-word characters: Use
     * the prefix list of the main word.
     * TODO: do the same for prefixes.
     */
    for (i = 0; i < fixga.ga_len; ++i)
    {
	nbw = ((basicword_T **)fixga.ga_data)[i];
	if (ga_grow(&nbw->bw_prefix, bw->bw_prefix.ga_len) == OK)
	{
	    mch_memmove(nbw->bw_prefix.ga_data, bw->bw_prefix.ga_data,
				      bw->bw_prefix.ga_len * sizeof(short_u));
	    nbw->bw_prefix.ga_len = bw->bw_prefix.ga_len;
	}
    }

    ga_clear(&fixga);
}

/*
 * Go over all words in "oldwords" and change the old affix names to the new
 * affix numbers, check the conditions, fold case, extract the basic word and
 * additions.
 */
    static int
build_wordlist(newwords, oldwords, oldaff, regionmask)
    hashtab_T	*newwords;	/* basicword_T entries */
    hashtab_T	*oldwords;	/* dicword_T entries */
    afffile_T	*oldaff;	/* affixes for "oldwords" */
    int		regionmask;	/* value for bw_region */
{
    int		todo;
    hashitem_T	*old_hi;
    dicword_T	*dw;
    basicword_T *bw;
    char_u	foldword[MAXLINELEN];
    int		leadlen;
    char_u	leadstring[MAXLINELEN];
    int		addlen;
    char_u	addstring[MAXLINELEN];
    int		dwlen;
    char_u	*p;
    int		clen;
    int		flags;
    char_u	*cp;
    int		l;

    todo = oldwords->ht_used;
    for (old_hi = oldwords->ht_array; todo > 0; ++old_hi)
    {
	if (!HASHITEM_EMPTY(old_hi))
	{
	    --todo;
	    dw = HI2DW(old_hi);

	    /* This takes time, print a message now and then. */
	    if ((todo & 0x3ff) == 0 || todo == oldwords->ht_used - 1)
	    {
		if (todo != oldwords->ht_used - 1)
		{
		    msg_didout = FALSE;
		    msg_col = 0;
		}
		smsg((char_u *)_("%6d todo - %s"), todo, dw->dw_word);
		out_flush();
		ui_breakcheck();
		if (got_int)
		    break;
	    }

	    /* The basic words are always stored with folded case. */
	    dwlen = STRLEN(dw->dw_word);
	    (void)str_foldcase(dw->dw_word, dwlen, foldword, MAXLINELEN);
	    flags = captype(dw->dw_word, dw->dw_word + dwlen);

	    /* Check for non-word characters before the word. */
	    clen = 0;
	    leadlen = 0;
	    if (!spell_iswordc(foldword))
	    {
		p = foldword;
		for (;;)
		{
		    mb_ptr_adv(p);
		    ++clen;
		    if (*p == NUL)	/* Only non-word chars (bad word!) */
		    {
			if (p_verbose > 0)
			    smsg((char_u *)_("Warning: word without word characters: \"%s\""),
								    foldword);
			break;
		    }
		    if (spell_iswordc(p))
		    {
			/* Move the leader to "leadstring" and remove it from
			 * "foldword". */
			leadlen = p - foldword;
			mch_memmove(leadstring, foldword, leadlen);
			leadstring[leadlen] = NUL;
			mch_memmove(foldword, p, STRLEN(p) + 1);
			break;
		    }
		}
	    }

	    /* Check for non-word characters after word characters. */
	    addlen = 0;
	    for (p = foldword; spell_iswordc(p); mb_ptr_adv(p))
	    {
		if (*p == NUL)
		    break;
		++clen;
	    }
	    if (*p != NUL)
	    {
		/* Move the addition to "addstring" and truncate "foldword". */
		if (flags & BWF_KEEPCAP)
		{
		    /* Preserve caps, need to skip the right number of
		     * characters in the original word (case folding may
		     * change the byte count). */
		    l = 0;
		    for (cp = dw->dw_word; l < clen; mb_ptr_adv(cp))
			++l;
		    addlen = STRLEN(cp);
		    mch_memmove(addstring, cp, addlen + 1);
		}
		else
		{
		    addlen = STRLEN(p);
		    mch_memmove(addstring, p, addlen + 1);
		}
		*p = NUL;
	    }

	    bw = (basicword_T *)alloc_clear((unsigned)sizeof(basicword_T)
							  + STRLEN(foldword));
	    if (bw == NULL)
		break;
	    STRCPY(bw->bw_word, foldword);
	    bw->bw_region = regionmask;

	    if (leadlen > 0)
		bw->bw_leadstring = vim_strsave(leadstring);
	    else
		bw->bw_leadstring = NULL;
	    if (addlen > 0)
		bw->bw_addstring = vim_strsave(addstring);
	    else
		bw->bw_addstring = NULL;

	    add_to_wordlist(newwords, bw);

	    if (flags & BWF_KEEPCAP)
	    {
		if (addlen == 0)
		    /* use the whole word */
		    bw->bw_caseword = vim_strsave(dw->dw_word + leadlen);
		else
		    /* use only up to the addition */
		    bw->bw_caseword = vim_strnsave(dw->dw_word + leadlen,
						  cp - dw->dw_word - leadlen);
		if (bw->bw_caseword == NULL)	/* out of memory */
		    flags &= ~BWF_KEEPCAP;
	    }
	    bw->bw_flags = flags;

	    /* Deal with any affix names on the old word, translate them
	     * into affix numbers. */
	    ga_init2(&bw->bw_prefix, sizeof(short_u), 10);
	    ga_init2(&bw->bw_suffix, sizeof(short_u), 10);
	    if (dw->dw_affnm != NULL)
		trans_affixes(dw, bw, oldaff, newwords);
	}
    }
    if (todo > 0)
	return FAIL;
    return OK;
}

/*
 * Go through the list of words and combine the ones that are identical except
 * for the region.
 */
    static void
combine_regions(newwords)
    hashtab_T	*newwords;
{
    int		todo;
    hashitem_T	*hi;
    basicword_T *bw, *nbw, *pbw;

    /* Loop over all basic words in the words table. */
    todo = newwords->ht_used;
    for (hi = newwords->ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;

	    /* Loop over the list of words for this basic word.  Compare with
	     * each following word in the same list. */
	    for (bw = HI2BW(hi); bw != NULL; bw = bw->bw_next)
	    {
		pbw = bw;
		for (nbw = pbw->bw_next; nbw != NULL; nbw = pbw->bw_next)
		{
		    if (bw->bw_flags == nbw->bw_flags
			    && (bw->bw_leadstring == NULL)
					       == (nbw->bw_leadstring == NULL)
			    && (bw->bw_addstring == NULL)
						== (nbw->bw_addstring == NULL)
			    && ((bw->bw_flags & BWF_KEEPCAP) == 0
				|| (STRCMP(bw->bw_caseword,
						      nbw->bw_caseword) == 0))
			    && (bw->bw_leadstring == NULL
				|| (STRCMP(bw->bw_leadstring,
						    nbw->bw_leadstring) == 0))
			    && (bw->bw_addstring == NULL
				|| (STRCMP(bw->bw_addstring,
						     nbw->bw_addstring) == 0))
			    && same_affixes(bw, nbw)
			    )
		    {
			/* Match, combine regions and delete "nbw". */
			pbw->bw_next = nbw->bw_next;
			bw->bw_region |= nbw->bw_region;
			free_basicword(nbw);
		    }
		    else
			/* No match, continue with next one. */
			pbw = nbw;
		}
	    }
	}
    }
}

/*
 * Return TRUE when the prefixes and suffixes for "bw" and "nbw" are equal.
 */
    static int
same_affixes(bw, nbw)
    basicword_T	*bw;
    basicword_T	*nbw;
{
    return (bw->bw_prefix.ga_len == nbw->bw_prefix.ga_len
	    && bw->bw_suffix.ga_len == nbw->bw_suffix.ga_len
	    && (bw->bw_prefix.ga_len == 0
		|| vim_memcmp(bw->bw_prefix.ga_data,
		    nbw->bw_prefix.ga_data,
		    bw->bw_prefix.ga_len * sizeof(short_u)) == 0)
	    && (bw->bw_suffix.ga_len == 0
		|| vim_memcmp(bw->bw_suffix.ga_data,
		    nbw->bw_suffix.ga_data,
		    bw->bw_suffix.ga_len * sizeof(short_u)) == 0));
}

/*
 * For each basic word with additions turn the affixes into other additions
 * and/or new basic words.  The result is that no affixes apply to a word with
 * additions.
 */
    static void
expand_affixes(newwords, prefgap, suffgap)
    hashtab_T	*newwords;
    garray_T	*prefgap;
    garray_T	*suffgap;
{
    int		todo;
    hashitem_T	*hi;
    basicword_T *bw;
    int		pi, si;
    affentry_T	*pae, *sae;
    garray_T	add_words;
    int		n;

    ga_init2(&add_words, sizeof(basicword_T *), 10);

    todo = newwords->ht_used;
    for (hi = newwords->ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    for (bw = HI2BW(hi); bw != NULL; bw = bw->bw_next)
	    {
		/*
		 * Need to fix affixes if there is a leader or addition and
		 * there are prefixes or suffixes.
		 */
		if ((bw->bw_leadstring != NULL || bw->bw_addstring != NULL)
			&& (bw->bw_prefix.ga_len != 0
						|| bw->bw_suffix.ga_len != 0))
		{
		    /* Loop over all prefix numbers, but first without a
		     * prefix. */
		    for (pi = -1; pi < bw->bw_prefix.ga_len; ++pi)
		    {
			pae = NULL;
			if (pi >= 0)
			{
			    n = ((short_u *)bw->bw_prefix.ga_data)[pi];
			    pae = ((affheader_T *)prefgap->ga_data + n)
								   ->ah_first;
			}

			/* Loop over all entries for prefix "pi".  Do it once
			 * when there is no prefix (pi == -1). */
			do
			{
			    /* Loop over all suffix numbers.  Do without a
			     * suffix first when there is a prefix. */
			    for (si = (pi == -1 ? 0 : -1);
					      si < bw->bw_suffix.ga_len; ++si)
			    {
				sae = NULL;
				if (si >= 0)
				{
				    n = ((short_u *)bw->bw_suffix.ga_data)[si];
				    sae = ((affheader_T *)suffgap->ga_data + n)
								   ->ah_first;
				}

				/* Loop over all entries for suffix "si".  Do
				 * it once when there is no suffix (si == -1).
				 */
				do
				{
				    /* Expand the word for this combination of
				     * prefixes and affixes. */
				    expand_one_aff(bw, &add_words, pae, sae);

				    /* Advance to next suffix entry, if there
				     * is one. */
				    if (sae != NULL)
					sae = sae->ae_next;
				} while (sae != NULL);
			    }

			    /* Advance to next prefix entry, if there is one. */
			    if (pae != NULL)
				pae = pae->ae_next;
			} while (pae != NULL);
		    }
		}
	    }
	}
    }

    /*
     * Add the new words afterwards, can't change "newwords" while going over
     * all its items.
     */
    for (pi = 0; pi < add_words.ga_len; ++pi)
	add_to_wordlist(newwords, ((basicword_T **)add_words.ga_data)[pi]);

    ga_clear(&add_words);
}

/*
 * Add one word to "add_words" for basic word "bw" with additions, adding
 * prefix "pae" and suffix "sae".  Either "pae" or "sae" can be NULL.
 */
    static void
expand_one_aff(bw, add_words, pae, sae)
    basicword_T	    *bw;
    garray_T	    *add_words;
    affentry_T	    *pae;
    affentry_T	    *sae;
{
    char_u	word[MAXWLEN + 1];
    char_u	caseword[MAXWLEN + 1];
    int		l = 0;
    int		choplen = 0;
    int		ll;
    basicword_T	*nbw;

    /* Prepend prefix to the basic word if there is a prefix and there is no
     * leadstring. */
    if (pae != NULL && bw->bw_leadstring == NULL)
    {
	if (pae->ae_add != NULL)
	{
	    l = STRLEN(pae->ae_add);
	    mch_memmove(word, pae->ae_add, l);
	}
	if (pae->ae_chop != NULL)
	    choplen = STRLEN(pae->ae_chop);
    }

    /* Copy the body of the word. */
    STRCPY(word + l, bw->bw_word + choplen);

    /* Do the same for bw_caseword, if it's there. */
    if (bw->bw_flags & BWF_KEEPCAP)
    {
	if (l > 0)
	    mch_memmove(caseword, pae->ae_add, l);
	STRCPY(caseword + l, bw->bw_caseword + choplen);
    }

    /* Append suffix to the basic word if there is a suffix and there is no
     * addstring. */
    if (sae != 0 && bw->bw_addstring == NULL)
    {
	l = STRLEN(word);
	if (sae->ae_chop != NULL)
	    l -= STRLEN(sae->ae_chop);
	if (sae->ae_add == NULL)
	    word[l] = NUL;
	else
	    STRCPY(word + l, sae->ae_add);

	if (bw->bw_flags & BWF_KEEPCAP)
	{
	    /* Do the same for the caseword. */
	    l = STRLEN(caseword);
	    if (sae->ae_chop != NULL)
		l -= STRLEN(sae->ae_chop);
	    if (sae->ae_add == NULL)
		caseword[l] = NUL;
	    else
		STRCPY(caseword + l, sae->ae_add);
	}
    }

    nbw = (basicword_T *)alloc_clear((unsigned)
					  sizeof(basicword_T) + STRLEN(word));
    if (nbw != NULL)
    {
	/* Add the new word to the list of words to be added later. */
	if (ga_grow(add_words, 1) == FAIL)
	{
	    vim_free(nbw);
	    return;
	}
	((basicword_T **)add_words->ga_data)[add_words->ga_len++] = nbw;

	/* Copy the (modified) basic word, flags and region. */
	STRCPY(nbw->bw_word, word);
	nbw->bw_flags = bw->bw_flags;
	nbw->bw_region = bw->bw_region;

	/* Set the (modified) caseword. */
	if (bw->bw_flags & BWF_KEEPCAP)
	    if ((nbw->bw_caseword = vim_strsave(caseword)) == NULL)
		nbw->bw_flags &= ~BWF_KEEPCAP;

	if (bw->bw_leadstring != NULL)
	{
	    if (pae != NULL)
	    {
		/* Prepend prefix to leadstring. */
		ll = STRLEN(bw->bw_leadstring);
		l = choplen = 0;
		if (pae->ae_add != NULL)
		    l = STRLEN(pae->ae_add);
		if (pae->ae_chop != NULL)
		{
		    choplen = STRLEN(pae->ae_chop);
		    if (choplen > ll)	    /* TODO: error? */
			choplen = ll;
		}
		nbw->bw_leadstring = alloc((unsigned)(ll + l - choplen + 1));
		if (nbw->bw_leadstring != NULL)
		{
		    if (l > 0)
			mch_memmove(nbw->bw_leadstring, pae->ae_add, l);
		    STRCPY(nbw->bw_leadstring + l, bw->bw_leadstring + choplen);
		}
	    }
	    else
		nbw->bw_leadstring = vim_strsave(bw->bw_leadstring);
	}

	if (bw->bw_addstring != NULL)
	{
	    if (sae != NULL)
	    {
		/* Append suffix to addstring. */
		l = STRLEN(bw->bw_addstring);
		if (sae->ae_chop != NULL)
		{
		    l -= STRLEN(sae->ae_chop);
		    if (l < 0)	    /* TODO: error? */
			l = 0;
		}
		if (sae->ae_add == NULL)
		    ll = 0;
		else
		    ll = STRLEN(sae->ae_add);
		nbw->bw_addstring = alloc((unsigned)(ll + l - choplen + 1));
		if (nbw->bw_addstring != NULL)
		{
		    STRCPY(nbw->bw_addstring, bw->bw_addstring);
		    if (sae->ae_add == NULL)
			nbw->bw_addstring[l] = NUL;
		    else
			STRCPY(nbw->bw_addstring + l, sae->ae_add);
		}
	    }
	    else
		nbw->bw_addstring = vim_strsave(bw->bw_addstring);
	}
    }
}

/*
 * Add basicword_T "*bw" to wordlist "newwords".
 */
    static void
add_to_wordlist(newwords, bw)
    hashtab_T	*newwords;
    basicword_T	*bw;
{
    hashitem_T	*hi;
    basicword_T *bw2;

    hi = hash_find(newwords, bw->bw_word);
    if (HASHITEM_EMPTY(hi))
    {
	/* New entry, add to hashlist. */
	hash_add(newwords, bw->bw_word);
	bw->bw_next = NULL;
    }
    else
    {
	/* Existing entry, append to list of basic words. */
	bw2 = HI2BW(hi);
	bw->bw_next = bw2->bw_next;
	bw2->bw_next = bw;
    }
}

/*
 * Write a number to file "fd", MSB first, in "len" bytes.
 */
    static void
put_bytes(fd, nr, len)
    FILE    *fd;
    long_u  nr;
    int	    len;
{
    int	    i;

    for (i = len - 1; i >= 0; --i)
	putc((int)(nr >> (i * 8)), fd);
}

/*
 * Write affix info.  <affflags> <affitemcnt> <affitem> ...
 */
    static void
write_affix(fd, ah)
    FILE	*fd;
    affheader_T	*ah;
{
    int		i = 0;
    affentry_T	*ae;
    char_u	*p;
    int		round;

    fputc(ah->ah_combine ? 1 : 0, fd);		/* <affflags> */

    /* Count the number of entries. */
    for (ae = ah->ah_first; ae != NULL; ae = ae->ae_next)
	++i;
    put_bytes(fd, (long_u)i, 2);		/* <affitemcnt> */

    for (ae = ah->ah_first; ae != NULL; ae = ae->ae_next)
	for (round = 1; round <= 2; ++round)
	{
	    p = round == 1 ? ae->ae_chop : ae->ae_add;
	    if (p == NULL)
		putc(0, fd);		/* <affchoplen> / <affaddlen> */
	    else
	    {
		putc(STRLEN(p), fd);	/* <affchoplen> / <affaddlen> */
					/* <affchop> / <affadd> */
		fwrite(p, STRLEN(p), (size_t)1, fd);
	    }
	}
}

/*
 * Write list of affix NRs: <affixcnt> <affixNR> ...
 */
    static void
write_affixlist(fd, aff, bytes)
    FILE	*fd;
    garray_T	*aff;
    int		bytes;
{
    int		i;

    if (aff->ga_len > 0)
    {
	putc(aff->ga_len, fd);	    /* <affixcnt> */
	for (i = 0; i < aff->ga_len; ++i)
	    put_bytes(fd, (long_u )((short_u *)aff->ga_data)[i], bytes);
    }
}

/*
 * Vim spell file format:  <HEADER> <PREFIXLIST> <SUFFIXLIST>
 *						    <SUGGEST> <WORDLIST>
 *
 * <HEADER>: <fileID> <regioncnt> <regionname> ...
 *
 * <fileID>     10 bytes    "VIMspell01"
 * <regioncnt>  1 byte	    number of regions following (8 supported)
 * <regionname>	2 bytes     Region name: ca, au, etc.
 *			    First <regionname> is region 1.
 *
 *
 * <PREFIXLIST>: <affcount> <afftotcnt> <affix> ...
 * <SUFFIXLIST>: <affcount> <afftotcnt> <affix> ...
 *		list of possible affixes: prefixes and suffixes.
 *
 * <affcount>	2 bytes	    Number of affixes (MSB comes first).
 *                          When more than 256 an affixNR is 2 bytes.
 *                          This is separate for prefixes and suffixes!
 *                          First affixNR is 0.
 * <afftotcnt>	2 bytes	    Total number of affix items (MSB comes first).
 *
 * <affix>: <affflags> <affitemcnt> <affitem> ...
 *
 * <affflags>	1 byte	    0x01: prefix combines with suffix.
 *			    0x02-0x80: unset
 * <affitemcnt> 2 bytes	    Number of affixes with this affixNR (MSB first).
 *
 * <affitem>: <affchoplen> <affchop> <affaddlen> <affadd>
 *
 * <affchoplen> 1 byte	    Length of <affchop> in bytes.
 * <affchop>    N bytes     To be removed from basic word.
 * <affaddlen>  1 byte	    Length of <affadd> in bytes.
 * <affadd>     N bytes     To be added to basic word.
 *
 *
 * <SUGGEST> : <suggestlen> <more> ...
 *
 * <suggestlen> 4 bytes	    Length of <SUGGEST> in bytes, excluding
 *			    <suggestlen>.  MSB first.
 * <more>		    To be defined.
 *
 *
 * <WORDLIST>: <wordcount> <worditem> ...
 *
 * <wordcount>	4 bytes	    Number of <worditem> following.  MSB first.
 *
 * <worditem>: <nr> <string> <flags> [<flags2>]
 *			  [<caselen> <caseword>]
 *			  [<affixcnt> <affixNR> ...]    (prefixes)
 *			  [<affixcnt> <affixNR> ...]	(suffixes)
 *			  [<region>]
 *			  [<addcnt> <add> ...]
 *
 * <nr>	i	1 byte	    Number of bytes copied from previous word.
 * <string>	N bytes	    Additional bytes for word, up to byte smaller than
 *			    0x20 (space).
 *			    Must only contain case-folded word characters.
 * <flags>	1 byte	    0x01: word is valid without addition
 *			    0x02: has region byte
 *			    0x04: first letter must be upper-case
 *			    0x08: has suffixes, <affixcnt> and <affixNR> follow
 *			    0x10: more flags, <flags2> follows next
 *			    0x20-0x80: can't be used, unset
 * <flags2>	1 byte	    0x01: has additions, <addcnt> and <add> follow
 *			    0x02: has prefixes, <affixcnt> and <affixNR> follow
 *			    0x04: all letters must be upper-case
 *			    0x08: case must match
 *			    0x10-0x80: unset
 * <caselen>	1 byte	    Length of <caseword>.
 * <caseword>	N bytes	    Word with matching case.
 * <affixcnt>	1 byte	    Number of affix NRs following.
 * <affixNR>	1 or 2 byte Number of possible affix for this word.
 *			    When using 2 bytes MSB comes first.
 * <region>	1 byte	    Bitmask for regions in which word is valid.  When
 *			    omitted it's valid in all regions.
 *			    Lowest bit is for region 1.
 * <addcnt>	2 bytes	    Number of <add> items following.
 *
 * <add>: <addflags> <addlen> [<leadlen> <addstring>] [<region>]
 *
 * <addflags>	1 byte	    0x01: fixed case, <addstring> is the whole word
 *				  with matching case.
 *			    0x02: first letter must be upper-case
 *			    0x04: all letters must be upper-case
 *			    0x08: has region byte
 *			    0x10-0x80: unset
 * <addlen>	1 byte	    Length of <addstring> in bytes.
 * <leadlen>	1 byte	    Number of bytes at start of <addstring> that must
 *			    come before the start of the basic word.
 * <addstring>	N bytes	    Word characters, before/in/after the word.
 *
 * All text characters are in 'encoding': <affchop>, <affadd>, <string>,
 * <caseword>> and <addstring>.
 * All other fields are ASCII: <regionname>
 * <string> is always case-folded.
 */

/*
 * Write the Vim spell file "fname".
 */
    static void
write_vim_spell(fname, prefga, suffga, newwords, regcount, regchars)
    char_u	*fname;
    garray_T	*prefga;	/* prefixes, affheader_T entries */
    garray_T	*suffga;	/* suffixes, affheader_T entries */
    hashtab_T	*newwords;	/* basic words, basicword_T entries */
    int		regcount;	/* number of regions */
    char_u	*regchars;	/* region names */
{
    FILE	*fd;
    garray_T	*gap;
    hashitem_T	*hi;
    char_u	**wtab;
    int		todo;
    int		flags, aflags;
    basicword_T	*bw, *bwf, *bw2, *prevbw = NULL;
    int		regionmask;	/* mask for all relevant region bits */
    int		i;
    int		cnt;
    affentry_T	*ae;
    int		round;
    int		prefm, suffm;
    garray_T	bwga;

    fd = fopen((char *)fname, "w");
    if (fd == NULL)
    {
	EMSG2(_(e_notopen), fname);
	return;
    }

    fwrite(VIMSPELLMAGIC, VIMSPELLMAGICL, (size_t)1, fd);

    /* write the region names if there is more than one */
    if (regcount > 1)
    {
	putc(regcount, fd);
	fwrite(regchars, (size_t)(regcount * 2), (size_t)1, fd);
	regionmask = (1 << regcount) - 1;
    }
    else
    {
	putc(0, fd);
	regionmask = 0;
    }

    /* Write the prefix and suffix lists. */
    for (round = 1; round <= 2; ++round)
    {
	gap = round == 1 ? prefga : suffga;
	put_bytes(fd, (long_u)gap->ga_len, 2);	    /* <affcount> */

	/* Count the total number of affix items. */
	cnt = 0;
	for (i = 0; i < gap->ga_len; ++i)
	    for (ae = ((affheader_T *)gap->ga_data + i)->ah_first;
						 ae != NULL; ae = ae->ae_next)
		++cnt;
	put_bytes(fd, (long_u)cnt, 2);		    /* <afftotcnt> */

	for (i = 0; i < gap->ga_len; ++i)
	    write_affix(fd, (affheader_T *)gap->ga_data + i);
    }

    /* Number of bytes used for affix NR depends on affix count. */
    prefm = (prefga->ga_len > 256) ? 2 : 1;
    suffm = (suffga->ga_len > 256) ? 2 : 1;

    /* Write the suggest info. TODO */
    put_bytes(fd, 0L, 4);

    /*
     * Write the word list.  <wordcount> <worditem> ...
     */
    /* number of basic words in 4 bytes */
    put_bytes(fd, newwords->ht_used, 4);	    /* <wordcount> */

    /*
     * Sort the word list, so that we can reuse as many bytes as possible.
     */
    wtab = (char_u **)alloc((unsigned)(sizeof(char_u *) * newwords->ht_used));
    if (wtab != NULL)
    {
	/* Make a table with pointers to each word. */
	todo = newwords->ht_used;
	for (hi = newwords->ht_array; todo > 0; ++hi)
	    if (!HASHITEM_EMPTY(hi))
		wtab[--todo] = hi->hi_key;

	/* Sort. */
	sort_strings(wtab, (int)newwords->ht_used);

	/* Now write each basic word to the spell file. */
	ga_init2(&bwga, sizeof(basicword_T *), 10);
	for (todo = 0; todo < newwords->ht_used; ++todo)
	{
	    bwf = KEY2BW(wtab[todo]);

	    /*
	     * Reorder the list of basicword_T words: make a list for words
	     * with the same case-folded word.  Put them together for same
	     * caps (ONECAP, ALLCAP and various KEEPCAP words) and same
	     * affixes.  Each list will then be put as a basic word with
	     * additions.
	     * This won't take much space, since the basic word is the same
	     * every time, only its length is written.
	     */
	    bwga.ga_len = 0;
	    for (bw = bwf; bw != NULL; bw = bw->bw_next)
	    {
		flags = bw->bw_flags & (BWF_ONECAP | BWF_KEEPCAP | BWF_ALLCAP);

		/* Go through the lists we found so far.  Break when the case
		 * matches. */
		for (i = 0; i < bwga.ga_len; ++i)
		{
		    bw2 = ((basicword_T **)bwga.ga_data)[i];
		    aflags = bw2->bw_flags & (BWF_ONECAP | BWF_KEEPCAP
								| BWF_ALLCAP);
		    if (flags == aflags
			    && ((flags & BWF_KEEPCAP) == 0
				|| (STRCMP(bw->bw_caseword,
						     bw2->bw_caseword) == 0))
			    && same_affixes(bw, bw2))
			break;
		}
		if (i == bwga.ga_len)
		{
		    /* No word with similar caps, make a new list. */
		    if (ga_grow(&bwga, 1) == FAIL)
			break;
		    ((basicword_T **)bwga.ga_data)[i] = bw;
		    bw->bw_cnext = NULL;
		    ++bwga.ga_len;
		}
		else
		{
		    /* Add to list of words with similar caps. */
		    bw->bw_cnext = bw2->bw_cnext;
		    bw2->bw_cnext = bw;
		}
	    }

	    /* Prefer the word with no caps to use as the first basic word.
	     * At least one without KEEPCAP. */
	    bw = NULL;
	    for (i = 0; i < bwga.ga_len; ++i)
	    {
		bw2 = ((basicword_T **)bwga.ga_data)[i];
		if (bw == NULL
			|| (bw2->bw_flags & (BWF_ONECAP | BWF_KEEPCAP
							  | BWF_ALLCAP)) == 0
			|| (bw->bw_flags & BWF_KEEPCAP))
		    bw = bw2;
	    }

	    /* Write first basic word.  If it's KEEPCAP then we need a word
	     * without VALID flag first (makes it easier to read the list back
	     * in). */
	    if (bw->bw_flags & BWF_KEEPCAP)
		write_bword(fd, bw, TRUE, &prevbw, regionmask, prefm, suffm);
	    write_bword(fd, bw, FALSE, &prevbw, regionmask, prefm, suffm);

	    /* Write other basic words, with different caps. */
	    for (i = 0; i < bwga.ga_len; ++i)
	    {
		bw2 = ((basicword_T **)bwga.ga_data)[i];
		if (bw2 != bw)
		    write_bword(fd, bw2, FALSE, &prevbw, regionmask,
								prefm, suffm);
	    }
	}

	ga_clear(&bwga);
    }

    fclose(fd);
}

/*
 * Write basic word, followed by any additions.
 *
 * <worditem>: <nr> <string> <flags> [<flags2>]
 *			  [<caselen> <caseword>]
 *			  [<affixcnt> <affixNR> ...]    (prefixes)
 *			  [<affixcnt> <affixNR> ...]	(suffixes)
 *			  [<region>]
 *			  [<addcnt> <add> ...]
 */
    static void
write_bword(fd, bwf, lowcap, prevbw, regionmask, prefm, suffm)
    FILE	*fd;
    basicword_T	*bwf;
    int		lowcap;		/* write KEEPKAP word as not-valid */
    basicword_T **prevbw;	/* last written basic word */
    int		regionmask;	/* mask that includes all possible regions */
    int		prefm;
    int		suffm;
{
    int		flags;
    int		aflags;
    int		len;
    int		leadlen, addlen;
    int		clen;
    int		adds = 0;
    int		i;
    basicword_T *bw, *bw2;

    /* Check how many bytes can be copied from the previous word. */
    len = STRLEN(bwf->bw_word);
    if (*prevbw == NULL)
	clen = 0;
    else
	for (clen = 0; clen < len
		&& (*prevbw)->bw_word[clen] == bwf->bw_word[clen]; ++clen)
	    ;
    putc(clen, fd);				/* <nr> */
    *prevbw = bwf;
						/* <string> */
    if (len > clen)
	fwrite(bwf->bw_word + clen, (size_t)(len - clen), (size_t)1, fd);

    /* Try to find a word without additions to use first. */
    bw = bwf;
    for (bw2 = bwf; bw2 != NULL; bw2 = bw2->bw_cnext)
    {
	if (bw2->bw_addstring != NULL || bw2->bw_leadstring != NULL)
	    ++adds;
	else
	    bw = bw2;
    }

    /* Flags: If there is no leadstring and no addstring the basic word is
     * valid, may have prefixes, suffixes and region. */
    flags = bw->bw_flags;
    if (bw->bw_addstring == NULL && bw->bw_leadstring == NULL)
    {
	flags |= BWF_VALID;

	/* Add the prefix/suffix list if there are prefixes/suffixes. */
	if (bw->bw_prefix.ga_len > 0)
	    flags |= BWF_PREFIX;
	if (bw->bw_suffix.ga_len > 0)
	    flags |= BWF_SUFFIX;

	/* Flags: add the region byte if the word isn't valid in all
	 * regions. */
	if (regionmask != 0 && (bw->bw_region & regionmask) != regionmask)
	    flags |= BWF_REGION;
    }

    /* Flags: may have additions. */
    if (adds > 0)
	flags |= BWF_ADDS;

    /* The dummy word before a KEEPCAP word doesn't have any flags, they are
     * in the actual word that follows. */
    if (lowcap)
	flags = 0;

    /* Flags: when the upper byte is not used we only write one flags
     * byte, if it's used then set an extra flag in the first byte and
     * also write the second byte. */
    if ((flags & 0xff00) == 0)
	putc(flags, fd);			/* <flags> */
    else
    {
	putc(flags | BWF_SECOND, fd);		/* <flags> */
	putc((int)((unsigned)flags >> 8), fd);	/* <flags2> */
    }

    /* First dummy word doesn't need anything but flags. */
    if (lowcap)
	return;

    if (flags & BWF_KEEPCAP)
    {
	len = STRLEN(bw->bw_caseword);
	putc(len, fd);			/* <caselen> */
	for (i = 0; i < len; ++i)
	    putc(bw->bw_caseword[i], fd);	/* <caseword> */
    }

    /* write prefix and suffix lists: <affixcnt> <affixNR> ... */
    if (flags & BWF_PREFIX)
	write_affixlist(fd, &bw->bw_prefix, prefm);
    if (flags & BWF_SUFFIX)
	write_affixlist(fd, &bw->bw_suffix, suffm);

    if (flags & BWF_REGION)
	putc(bw->bw_region, fd);		/* <region> */

    /*
     * Additions.
     */
    if (adds > 0)
    {
	put_bytes(fd, (long_u)adds, 2);		/* <addcnt> */

	for (bw = bwf; bw != NULL; bw = bw->bw_cnext)
	    if (bw->bw_leadstring != NULL || bw->bw_addstring != NULL)
	    {
		/* <add>: <addflags> <addlen> [<leadlen> <addstring>]
		 *	  [<region>] */
		aflags = 0;
		if (bw->bw_flags & BWF_ONECAP)
		    aflags |= ADD_ONECAP;
		if (bw->bw_flags & BWF_ALLCAP)
		    aflags |= ADD_ALLCAP;
		if (bw->bw_flags & BWF_KEEPCAP)
		    aflags |= ADD_KEEPCAP;
		if (regionmask != 0
				&& (bw->bw_region & regionmask) != regionmask)
		    aflags |= ADD_REGION;
		putc(aflags, fd);		    /* <addflags> */

		if (bw->bw_leadstring == NULL)
		    leadlen = 0;
		else
		    leadlen = STRLEN(bw->bw_leadstring);
		if (bw->bw_addstring == NULL)
		    addlen = 0;
		else
		    addlen = STRLEN(bw->bw_addstring);
		putc(leadlen + addlen, fd);		    /* <addlen> */
		putc(leadlen, fd);			    /* <leadlen> */
							    /* <addstring> */
		if (bw->bw_leadstring != NULL)
		    fwrite(bw->bw_leadstring, (size_t)leadlen, (size_t)1, fd);
		if (bw->bw_addstring != NULL)
		    fwrite(bw->bw_addstring, (size_t)addlen, (size_t)1, fd);

		if (aflags & ADD_REGION)
		    putc(bw->bw_region, fd);		/* <region> */
	    }
    }
}


/*
 * ":mkspell  outfile  infile ..."
 */
    void
ex_mkspell(eap)
    exarg_T *eap;
{
    int		fcount;
    char_u	**fnames;
    char_u	fname[MAXPATHL];
    char_u	wfname[MAXPATHL];
    afffile_T	*(afile[8]);
    hashtab_T	dfile[8];
    int		i;
    int		len;
    char_u	region_name[16];
    struct stat	st;
    int		round;
    vimconv_T	conv;

    /* Expand all the arguments (e.g., $VIMRUNTIME). */
    if (get_arglist_exp(eap->arg, &fcount, &fnames) == FAIL)
	return;
    if (fcount < 2)
	EMSG(_(e_invarg));	/* need at least output and input names */
    else if (fcount > 9)
	EMSG(_("E754: Only up to 8 regions supported"));
    else
    {
	/* Check for overwriting before doing things that may take a lot of
	 * time. */
	sprintf((char *)wfname, "%s.%s.spl", fnames[0], p_enc);
	if (!eap->forceit && mch_stat((char *)wfname, &st) >= 0)
	{
	    EMSG(_(e_exists));
	    goto theend;
	}
	if (mch_isdir(fnames[0]))
	{
	    EMSG2(_(e_isadir2), fnames[0]);
	    goto theend;
	}

	/*
	 * Init the aff and dic pointers.
	 * Get the region names if there are more than 2 arguments.
	 */
	for (i = 1; i < fcount; ++i)
	{
	    afile[i - 1] = NULL;
	    hash_init(&dfile[i - 1]);
	    if (fcount > 2)
	    {
		len = STRLEN(fnames[i]);
		if (STRLEN(gettail(fnames[i])) < 5 || fnames[i][len - 3] != '_')
		{
		    EMSG2(_("E755: Invalid region in %s"), fnames[i]);
		    goto theend;
		}
		else
		{
		    region_name[(i - 1) * 2] = TOLOWER_ASC(fnames[i][len - 2]);
		    region_name[(i - 1) * 2 + 1] =
					      TOLOWER_ASC(fnames[i][len - 1]);
		}
	    }
	}

	/*
	 * Read all the .aff and .dic files.
	 * Text is converted to 'encoding'.
	 */
	for (i = 1; i < fcount; ++i)
	{
	    /* Read the .aff file.  Will init "conv" based on the "SET" line. */
	    conv.vc_type = CONV_NONE;
	    sprintf((char *)fname, "%s.aff", fnames[i]);
	    if ((afile[i - 1] = spell_read_aff(fname, &conv)) == NULL)
		break;

	    /* Read the .dic file. */
	    sprintf((char *)fname, "%s.dic", fnames[i]);
	    if (spell_read_dic(&dfile[i - 1], fname, &conv) == FAIL)
		break;

	    /* Free any conversion stuff. */
	    convert_setup(&conv, NULL, NULL);
	}

	/* Process the data when all the files could be read. */
	if (i == fcount)
	{
	    garray_T	prefga;
	    garray_T	suffga;
	    garray_T	*gap;
	    hashtab_T	newwords;

	    /*
	     * Combine all the affixes into one new affix list.  This is done
	     * for prefixes and suffixes separately.
	     * We need to do this for each region, try to re-use the same
	     * affixes.
	     * Since we number the new affix entries, a growarray will do.  In
	     * the affheader_T the ah_key is unused.
	     */
	    MSG(_("Combining affixes..."));
	    out_flush();
	    for (round = 1; round <= 2; ++round)
	    {
		gap = round == 1 ? &prefga : &suffga;
		ga_init2(gap, sizeof(affheader_T), 50);
		for (i = 1; i < fcount; ++i)
		    get_new_aff(round == 1 ? &afile[i - 1]->af_pref
					   : &afile[i - 1]->af_suff, gap);
	    }

	    /*
	     * Go over all words and:
	     * - change the old affix names to the new affix numbers
	     * - check the conditions
	     * - fold case
	     * - extract the basic word and additions.
	     * Do this for each region.
	     */
	    MSG(_("Building word list..."));
	    out_flush();
	    hash_init(&newwords);

	    for (i = 1; i < fcount; ++i)
		build_wordlist(&newwords, &dfile[i - 1], afile[i - 1],
								1 << (i - 1));

	    if (fcount > 2)
	    {
		/* Combine words for the different regions into one. */
		MSG(_("Combining regions..."));
		out_flush();
		combine_regions(&newwords);
	    }

	    /*
	     * Affixes on a word with additions are clumsy, would require
	     * inefficient searching.  Turn the affixes into additions and/or
	     * the expanded word.
	     */
	    MSG(_("Processing words..."));
	    out_flush();
	    expand_affixes(&newwords, &prefga, &suffga);

	    /* Write the info in the spell file. */
	    smsg((char_u *)_("Writing spell file %s..."), wfname);
	    out_flush();
	    write_vim_spell(wfname, &prefga, &suffga, &newwords,
						     fcount - 1, region_name);
	    MSG(_("Done!"));
	    out_flush();

	    /* Free the allocated stuff. */
	    free_wordtable(&newwords);
	    for (round = 1; round <= 2; ++round)
	    {
		gap = round == 1 ? &prefga: &suffga;
		for (i = 0; i < gap->ga_len; ++i)
		    free_affixentries(((affheader_T *)gap->ga_data + i)
								  ->ah_first);
		ga_clear(gap);
	    }
	}

	/* Free the .aff and .dic file structures. */
	for (i = 1; i < fcount; ++i)
	{
	    if (afile[i - 1] != NULL)
		spell_free_aff(afile[i - 1]);
	    spell_free_dic(&dfile[i - 1]);
	}
    }

theend:
    FreeWild(fcount, fnames);
}

    static void
free_wordtable(ht)
    hashtab_T	*ht;
{
    int		todo;
    basicword_T	*bw, *nbw;
    hashitem_T	*hi;

    todo = ht->ht_used;
    for (hi = ht->ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    for (bw = HI2BW(hi); bw != NULL; bw = nbw)
	    {
		nbw = bw->bw_next;
		free_basicword(bw);
	    }
	}
    }
}

/*
 * Free a basicword_T and what it contains.
 */
    static void
free_basicword(bw)
    basicword_T	*bw;
{
    ga_clear(&bw->bw_prefix);
    ga_clear(&bw->bw_suffix);
    vim_free(bw->bw_caseword);
    vim_free(bw->bw_leadstring);
    vim_free(bw->bw_addstring);
    vim_free(bw);
}

/*
 * Free a list of affentry_T.
 */
    static void
free_affixentries(first)
    affentry_T	*first;
{
    affentry_T	*ap, *an;

    for (ap = first; ap != NULL; ap = an)
    {
	an = ap->ae_next;
	vim_free(ap->ae_chop);
	vim_free(ap->ae_add);
	vim_free(ap->ae_cond);
	vim_free(ap->ae_prog);
	vim_free(ap);
    }
}

#endif  /* FEAT_MBYTE */

#endif  /* FEAT_SYN_HL */

#if 0  /* old spell code with words in .spl file */
/*
 * Structure that is used to store the text from the language file.  This
 * avoids the need to allocate space for each individual word.  It's allocated
 * in big chunks for speed.
 */
#define  SBLOCKSIZE 4096	/* default size of sb_data */
typedef struct sblock_S sblock_T;
struct sblock_S
{
    sblock_T	*sb_next;	/* next block in list */
    char_u	sb_data[1];	/* data, actually longer */
};

/* Structure to store words and additions.  Used twice : once for case-folded
 * and once for keep-case words. */
typedef struct winfo_S
{
    hashtab_T	wi_ht;		/* hashtable with all words, both dword_T and
				   nword_T (check flags for DW_NWORD) */
    garray_T	wi_add;		/* table with pointers to additions in a
				   dword_T */
    int		wi_addlen;	/* longest addition length */
} winfo_T;

/*
 * Structure used to store words and other info for one language.
 */
typedef struct slang_S slang_T;
struct slang_S
{
    slang_T	*sl_next;	/* next language */
    char_u	sl_name[2];	/* language name "en", "nl", etc. */
    winfo_T	sl_fwords;	/* case-folded words and additions */
    winfo_T	sl_kwords;	/* keep-case words and additions */
    char_u	sl_regions[17];	/* table with up to 8 region names plus NUL */
    sblock_T	*sl_block;	/* list with allocated memory blocks */
};

static slang_T *first_lang = NULL;

/* Entry for dword in "sl_ht".  Also used for part of an nword, starting with
 * the first non-word character.  And used for additions in wi_add. */
typedef struct dword_S
{
    char_u	dw_region;	/* one bit per region where it's valid */
    char_u	dw_flags;	/* DW_ flags */
    char_u	dw_word[1];	/* actually longer, NUL terminated */
} dword_T;

#define REGION_ALL 0xff

#define HI2DWORD(hi) (dword_T *)(hi->hi_key - 2)

/* Entry for a nword in "sl_ht".  Note that the last three items must be
 * identical to dword_T, so that they can be in the same hashtable. */
typedef struct nword_S
{
    garray_T	nw_ga;		/* table with pointers to dword_T for part
				   starting with non-word character */
    int		nw_maxlen;	/* longest nword length (after the dword) */
    char_u	nw_region;	/* one bit per region where it's valid */
    char_u	nw_flags;	/* DW_ flags */
    char_u	nw_word[1];	/* actually longer, NUL terminated */
} nword_T;

/* Get nword_T pointer from hashitem that uses nw_word */
static nword_T dumnw;
#define HI2NWORD(hi)	((nword_T *)((hi)->hi_key - (dumnw.nw_word - (char_u *)&dumnw)))

#define DW_CAP	    0x01	/* word must start with capital */
#define DW_RARE	    0x02	/* rare word */
#define DW_NWORD    0x04	/* this is an nword_T */
#define DW_DWORD    0x08	/* (also) use as dword without nword */

/*
 * Structure used in "b_langp", filled from 'spelllang'.
 */
typedef struct langp_S
{
    slang_T	*lp_slang;	/* info for this language (NULL for last one) */
    int		lp_region;	/* bitmask for region or REGION_ALL */
} langp_T;

#define LANGP_ENTRY(ga, i)	(((langp_T *)(ga).ga_data) + (i))
#define DWORD_ENTRY(gap, i)	*(((dword_T **)(gap)->ga_data) + i)

#define SP_OK		0
#define SP_BAD		1
#define SP_RARE		2
#define SP_LOCAL	3

static char *e_invchar2 = N_("E753: Invalid character in \"%s\"");

static slang_T *spell_load_lang __ARGS((char_u *lang));
static void spell_load_file __ARGS((char_u *fname));
static int find_region __ARGS((char_u *rp, char_u *region));

/*
 * Main spell-checking function.
 * "ptr" points to the start of a word.
 * "*attrp" is set to the attributes for a badly spelled word.  For a non-word
 * or when it's OK it remains unchanged.
 * This must only be called when 'spelllang' is not empty.
 * Returns the length of the word in bytes, also when it's OK, so that the
 * caller can skip over the word.
 */
    int
spell_check(wp, ptr, attrp)
    win_T	*wp;		/* current window */
    char_u	*ptr;
    int		*attrp;
{
    char_u	*e;		/* end of word */
    char_u	*ne;		/* new end of word */
    char_u	*me;		/* max. end of match */
    langp_T	*lp;
    int		result;
    int		len = 0;
    hashitem_T	*hi;
    int		round;
    char_u	kword[MAXWLEN + 1];	/* word copy */
    char_u	fword[MAXWLEN + 1];	/* word with case folded */
    char_u	match[MAXWLEN + 1];	/* fword with additional chars */
    char_u	kwordclen[MAXWLEN + 1];	/* len of orig chars after kword[] */
    char_u	fwordclen[MAXWLEN + 1]; /* len of chars after fword[] */
    char_u	*clen;
    int		cidx = 0;		/* char index in xwordclen[] */
    hash_T	fhash;			/* hash for fword */
    hash_T	khash;			/* hash for kword */
    int		match_len = 0;		/* length of match[] */
    int		fmatch_len = 0;		/* length of nword match in chars */
    garray_T	*gap;
    int		l, t;
    char_u	*p, *tp;
    int		n;
    dword_T	*dw;
    dword_T	*tdw;
    winfo_T	*wi;
    nword_T	*nw;
    int		w_isupper;

    /* Find the end of the word.  We already know that *ptr is a word char. */
    e = ptr;
    do
    {
	mb_ptr_adv(e);
	++len;
    } while (*e != NUL && spell_iswordc(e));

    /* A word starting with a number is always OK. */
    if (*ptr >= '0' && *ptr <= '9')
	return (int)(e - ptr);

#ifdef FEAT_MBYTE
    w_isupper = MB_ISUPPER(mb_ptr2char(ptr));
#else
    w_isupper = MB_ISUPPER(*ptr);
#endif

    /* Make a copy of the word so that it can be NUL terminated.
     * Compute hash value. */
    mch_memmove(kword, ptr, e - ptr);
    kword[e - ptr] = NUL;
    khash = hash_hash(kword);

    /* Make case-folded copy of the Word.  Compute its hash value. */
    (void)str_foldcase(ptr, e - ptr, fword, MAXWLEN + 1);
    fhash = hash_hash(fword);

    /* Further case-folded characters to check for an nword match go in
     * match[]. */
    me = e;

    /* "ne" is the end for the longest match */
    ne = e;

    /* The word is bad unless we find it in the dictionary. */
    result = SP_BAD;

    /*
     * Loop over the languages specified in 'spelllang'.
     * We check them all, because a matching nword may be longer than an
     * already found dword or nword.
     */
    for (lp = LANGP_ENTRY(wp->w_buffer->b_langp, 0); lp->lp_slang != NULL; ++lp)
    {
	/*
	 * Check for a matching word in the hashtable.
	 * Check both the keep-case word and the fold-case word.
	 */
	for (round = 0; round <= 1; ++round)
	{
	    if (round == 0)
	    {
		wi = &lp->lp_slang->sl_kwords;
		hi = hash_lookup(&wi->wi_ht, kword, khash);
	    }
	    else
	    {
		wi = &lp->lp_slang->sl_fwords;
		hi = hash_lookup(&wi->wi_ht, fword, fhash);
	    }
	    if (!HASHITEM_EMPTY(hi))
	    {
		/*
		 * If this is an nword entry, check for match with remainder.
		 */
		dw = HI2DWORD(hi);
		if (dw->dw_flags & DW_NWORD)
		{
		    /* If the word is not defined as a dword we must find an
		     * nword. */
		    if ((dw->dw_flags & DW_DWORD) == 0)
			dw = NULL;

		    /* Fold more characters when needed for the nword.  Need
		     * to do one extra to check for a non-word character after
		     * the nword.  Also keep the byte-size of each character,
		     * both before and after folding case. */
		    nw = HI2NWORD(hi);
		    while ((round == 0
				? me - e <= nw->nw_maxlen
				: match_len <= nw->nw_maxlen)
			    && *me != NUL)
		    {
#ifdef FEAT_MBYTE
			l = mb_ptr2len_check(me);
#else
			l = 1;
#endif
			(void)str_foldcase(me, l, match + match_len,
						     MAXWLEN - match_len + 1);
			me += l;
			kwordclen[cidx] = l;
			fwordclen[cidx] = STRLEN(match + match_len);
			match_len += fwordclen[cidx];
			++cidx;
		    }

		    if (round == 0)
		    {
			clen = kwordclen;
			tp = e;
		    }
		    else
		    {
			clen = fwordclen;
			tp = match;
		    }

		    /* Match with each item.  The longest match wins:
		     * "you've" is longer than "you". */
		    gap = &nw->nw_ga;
		    for (t = 0; t < gap->ga_len; ++t)
		    {
			/* Skip entries with wrong case for first char.
			 * Continue if it's a rare word without a captial. */
			tdw = DWORD_ENTRY(gap, t);
			if ((tdw->dw_flags & (DW_CAP | DW_RARE)) == DW_CAP
								&& !w_isupper)
			    continue;

			p = tdw->dw_word;
			l = 0;
			for (n = 0; p[n] != 0; n += clen[l++])
			    if (vim_memcmp(p + n, tp + n, clen[l]) != 0)
				break;

			/* Use a match if it's longer than previous matches
			 * and the next character is not a word character. */
			if (p[n] == 0 && l > fmatch_len && (tp[n] == 0
						   || !spell_iswordc(tp + n)))
			{
			    dw = tdw;
			    fmatch_len = l;
			    if (round == 0)
				ne = tp + n;
			    else
			    {
				/* Need to use the length of the original
				 * chars, not the fold-case ones. */
				ne = e;
				for (l = 0; l < fmatch_len; ++l)
				    ne += kwordclen[l];
			    }
			    if ((lp->lp_region & tdw->dw_region) == 0)
				result = SP_LOCAL;
			    else if ((tdw->dw_flags & DW_CAP) && !w_isupper)
				result = SP_RARE;
			    else
				result = SP_OK;
			}
		    }

		}

		if (dw != NULL)
		{
		    if (dw->dw_flags & DW_CAP)
		    {
			/* Need to check first letter is uppercase.  If it is,
			 * check region.  If it isn't it may be a rare word.
			 * */
			if (w_isupper)
			{
			    if ((dw->dw_region & lp->lp_region) == 0)
				result = SP_LOCAL;
			    else
				result = SP_OK;
			}
			else if (dw->dw_flags & DW_RARE)
			    result = SP_RARE;
		    }
		    else
		    {
			if ((dw->dw_region & lp->lp_region) == 0)
			    result = SP_LOCAL;
			else if (dw->dw_flags & DW_RARE)
			    result = SP_RARE;
			else
			    result = SP_OK;
		    }
		}
	    }
	}

	/*
	 * Check for an addition.
	 * Only after a dword, not after an nword.
	 * Check both the keep-case word and the fold-case word.
	 */
	if (fmatch_len == 0)
	    for (round = 0; round <= 1; ++round)
	    {
		if (round == 0)
		    wi = &lp->lp_slang->sl_kwords;
		else
		    wi = &lp->lp_slang->sl_fwords;
		gap = &wi->wi_add;
		if (gap->ga_len == 0)   /* no additions, skip quickly */
		    continue;

		/* Fold characters when needed for the addition.  Need to do one
		 * extra to check for a word character after the addition. */
		while ((round == 0
			    ? me - e <= wi->wi_addlen
			    : match_len <= wi->wi_addlen)
			&& *me != NUL)
		{
#ifdef FEAT_MBYTE
		    l = mb_ptr2len_check(me);
#else
		    l = 1;
#endif
		    (void)str_foldcase(me, l, match + match_len,
							 MAXWLEN - match_len + 1);
		    me += l;
		    kwordclen[cidx] = l;
		    fwordclen[cidx] = STRLEN(match + match_len);
		    match_len += fwordclen[cidx];
		    ++cidx;
		}

		if (round == 0)
		{
		    clen = kwordclen;
		    tp = e;
		}
		else
		{
		    clen = fwordclen;
		    tp = match;
		}

		/* Addition lookup.  Uses a linear search, there should be
		 * very few.  If there is a match adjust "ne" to the end.
		 * This doesn't change whether a word was good or bad, only
		 * the length. */
		for (t = 0; t < gap->ga_len; ++t)
		{
		    tdw = DWORD_ENTRY(gap, t);
		    p = tdw->dw_word;
		    l = 0;
		    for (n = 0; p[n] != 0; n += clen[l++])
			if (vim_memcmp(p + n, tp + n, clen[l]) != 0)
			    break;

		    /* Use a match if it's longer than previous matches
		     * and the next character is not a word character. */
		    if (p[n] == 0 && l > fmatch_len
				    && (tp[n] == 0 || !spell_iswordc(tp + n)))
		    {
			fmatch_len = l;
			if (round == 0)
			    ne = tp + n;
			else
			{
			    /* Need to use the length of the original
			     * chars, not the fold-case ones. */
			    ne = e;
			    for (l = 0; l < fmatch_len; ++l)
				ne += kwordclen[l];
			}
		    }
		}
	    }
    }

    if (result != SP_OK)
    {
	if (result == SP_BAD)
	    *attrp = highlight_attr[HLF_SPB];
	else if (result == SP_RARE)
	    *attrp = highlight_attr[HLF_SPR];
	else
	    *attrp = highlight_attr[HLF_SPL];
    }

    return (int)(ne - ptr);
}

static slang_T	    *load_lp;	/* passed from spell_load_lang() to
				   spell_load_file() */

/*
 * Load language "lang[2]".
 */
    static slang_T *
spell_load_lang(lang)
    char_u	*lang;
{
    slang_T	*lp;
    char_u	fname_enc[80];
    char_u	fname_ascii[20];
    char_u	*p;
    int		r;

    lp = (slang_T *)alloc(sizeof(slang_T));
    if (lp != NULL)
    {
	lp->sl_name[0] = lang[0];
	lp->sl_name[1] = lang[1];
	hash_init(&lp->sl_fwords.wi_ht);
	ga_init2(&lp->sl_fwords.wi_add, sizeof(dword_T *), 4);
	lp->sl_fwords.wi_addlen = 0;
	hash_init(&lp->sl_kwords.wi_ht);
	ga_init2(&lp->sl_kwords.wi_add, sizeof(dword_T *), 4);
	lp->sl_kwords.wi_addlen = 0;
	lp->sl_regions[0] = NUL;
	lp->sl_block = NULL;

	/* Find all spell files for "lang" in 'runtimepath' and load them.
	 * Use 'encoding', except that we use "latin1" for "latin9". */
#ifdef FEAT_MBYTE
	if (STRLEN(p_enc) < 60 && STRCMP(p_enc, "iso-8859-15") != 0)
	    p = p_enc;
	else
#endif
	    p = (char_u *)"latin1";
	load_lp = lp;
	sprintf((char *)fname_enc, "spell/%c%c.%s.spl", lang[0], lang[1], p);
	r = do_in_runtimepath(fname_enc, TRUE, spell_load_file);
	if (r == FAIL)
	{
	    /* Try again to find an ASCII spell file. */
	    sprintf((char *)fname_ascii, "spell/%c%c.spl", lang[0], lang[1]);
	    r = do_in_runtimepath(fname_ascii, TRUE, spell_load_file);
	}

	if (r == FAIL)
	{
	    vim_free(lp);
	    lp = NULL;
	    smsg((char_u *)_("Warning: Cannot find dictionary \"%s\""),
							       fname_enc + 6);
	}
	else
	{
	    lp->sl_next = first_lang;
	    first_lang = lp;
	}
    }

    return lp;
}

/*
 * Load one spell file into "load_lp".
 * Invoked through do_in_runtimepath().
 */
    static void
spell_load_file(fname)
    char_u	*fname;
{
    int		fd;
    size_t	len;
    int		l;
    char_u	*p = NULL, *np;
    sblock_T	*bl = NULL;
    int		bl_used = 0;
    size_t	rest = 0;
    char_u	*rbuf;		/* read buffer */
    char_u	*rbuf_end;	/* past last valid char in "rbuf" */
    hash_T	hash;
    hashitem_T	*hi;
    int		c;
    int		cc;
    int		region = REGION_ALL;
    int		wlen;
    winfo_T	*wi;
    dword_T	*dw, *edw = NULL;
    nword_T	*nw = NULL;
    int		flags;
    char_u	*save_sourcing_name = sourcing_name;
    linenr_T	save_sourcing_lnum = sourcing_lnum;

    rbuf = alloc((unsigned)(SBLOCKSIZE + MAXWLEN + 1));
    if (rbuf == NULL)
	return;

    fd = mch_open((char *)fname, O_RDONLY | O_EXTRA, 0);
    if (fd < 0)
    {
	EMSG2(_(e_notopen), fname);
	goto theend;
    }

    sourcing_name = fname;
    sourcing_lnum = 0;

    /* Get the length of the whole file. */
    len = lseek(fd, (off_t)0, SEEK_END);
    lseek(fd, (off_t)0, SEEK_SET);

    /*
     * Read the file one block at a time.
     * "rest" is the length of an incomplete line at the previous block.
     * "p" points to the remainder.
     */
    while (len > 0)
    {
	/* Read a block from the file.  Prepend the remainder of the previous
	 * block, if any. */
	if (rest > 0)
	{
	    if (rest > MAXWLEN)	    /* truncate long line (should be comment) */
		rest = MAXWLEN;
	    mch_memmove(rbuf, p, rest);
	    --sourcing_lnum;
	}
	if (len > SBLOCKSIZE)
	    l = SBLOCKSIZE;
	else
	    l = len;
	len -= l;
	if (read(fd, rbuf + rest, l) != l)
	{
	    EMSG2(_(e_notread), fname);
	    break;
	}
	rbuf_end = rbuf + l + rest;
	rest = 0;

	/* Deal with each line that was read until we finish the block. */
	for (p = rbuf; p < rbuf_end; p = np)
	{
	    ++sourcing_lnum;

	    /* "np" points to the first char after the line (CR, NL or white
	     * space). */
	    for (np = p; np < rbuf_end && *np >= ' '; mb_ptr_adv(np))
		;
	    if (np >= rbuf_end)
	    {
		/* Incomplete line or end of file. */
		rest = np - p;
		if (len == 0)
		    EMSG(_("E751: Truncated spell file"));
		break;
	    }
	    *np = NUL;	    /* terminate the line with a NUL */

	    if (*p == '-')
	    {
		/*
		 * Region marker: ---, -xx, -xx-yy, etc.
		 */
		++p;
		if (*p == '-')
		{
		    if (p[1] != '-' || p[2] != NUL)
		    {
			EMSG2(_(e_invchar2), p - 1);
			len = 0;
			break;
		    }
		    region = REGION_ALL;
		}
		else
		{
		    char_u	*rp = load_lp->sl_regions;
		    int		r;

		    /* Start of a region.  The region may be repeated:
		     * "-ca-uk".  Fill "region" with the bit mask for the
		     * ones we find. */
		    region = 0;
		    for (;;)
		    {
			r = find_region(rp, p);
			if (r == REGION_ALL)
			{
			    /* new region, add it to sl_regions[] */
			    r = STRLEN(rp);
			    if (r >= 16)
			    {
				EMSG2(_("E752: Too many regions: %s"), p);
				len = 0;
				break;
			    }
			    else
			    {
				rp[r] = p[0];
				rp[r + 1] = p[1];
				rp[r + 2] = NUL;
				r = 1 << (r / 2);
			    }
			}
			else
			    r = 1 << r;

			region |= r;
			if (p[2] != '-')
			{
			    if (p[2] > ' ')
			    {
				EMSG2(_(e_invchar2), p - 1);
				len = 0;
			    }
			    break;
			}
			p += 3;
		    }
		}
	    }
	    else if (*p != '#' && *p != NUL)
	    {
		/*
		 * Not an empty line or comment.
		 */
		if (*p == '!')
		{
		    wi = &load_lp->sl_kwords;	    /* keep case */
		    ++p;
		}
		else
		    wi = &load_lp->sl_fwords;	    /* fold case */

		flags = 0;
		c = *p;
		if (c == '>')		/* rare word */
		{
		    flags = DW_RARE;
		    ++p;
		}
		else if (*p == '+')	/* addition */
		    ++p;

		if (c != '+' && !spell_iswordc(p))
		{
		    EMSG2(_(e_invchar2), p);
		    len = 0;
		    break;
		}

		/* Make sure there is room for the word.  Folding case may
		 * double the size. */
		wlen = np - p;
		if (bl == NULL || bl_used + sizeof(dword_T) + wlen
#ifdef FEAT_MBYTE
					    * (has_mbyte ? 2 : 1)
#endif
							    >= SBLOCKSIZE)
		{
		    /* Allocate a block of memory to store the dword_T in.
		     * This is not freed until spell_reload() is called. */
		    bl = (sblock_T *)alloc((unsigned)(sizeof(sblock_T)
							   + SBLOCKSIZE));
		    if (bl == NULL)
		    {
			len = 0;
			break;
		    }
		    bl->sb_next = load_lp->sl_block;
		    load_lp->sl_block = bl;
		    bl_used = 0;
		}
		dw = (dword_T *)(bl->sb_data + bl_used);

		/* For fold-case words fold the case and check for start
		 * with uppercase letter. */
		if (wi == &load_lp->sl_fwords)
		{
#ifdef FEAT_MBYTE
		    if (MB_ISUPPER(mb_ptr2char(p)))
#else
		    if (MB_ISUPPER(*p))
#endif
			flags |= DW_CAP;

		    /* Fold case. */
		    (void)str_foldcase(p, np - p, dw->dw_word, wlen
#ifdef FEAT_MBYTE
						     * (has_mbyte ? 2 : 1)
#endif
								     + 1);
#ifdef FEAT_MBYTE
		    /* case folding may change length of word */
		    wlen = STRLEN(dw->dw_word);
#endif
		}
		else
		{
		    /* Keep case: copy the word as-is. */
		    mch_memmove(dw->dw_word, p, wlen + 1);
		}

		if (c == '+')
		{
		    garray_T    *gap = &wi->wi_add;

		    /* Addition.  TODO: search for matching entry? */
		    if (wi->wi_addlen < wlen)
			wi->wi_addlen = wlen;
		    if (ga_grow(gap, 1) == FAIL)
		    {
			len = 0;
			break;
		    }
		    *(((dword_T **)gap->ga_data) + gap->ga_len) = dw;
		    ++gap->ga_len;
		    dw->dw_region = region;
		    dw->dw_flags = flags;
		    bl_used += sizeof(dword_T) + wlen;
		}
		else
		{
		    /*
		     * Check for a non-word character.  If found it's
		     * going to be an nword.
		     * For an nword we split in two: the leading dword and
		     * the remainder.  The dword goes in the hashtable
		     * with an nword_T, the remainder is put in the
		     * dword_T (starting with the first non-word
		     * character).
		     */
		    cc = NUL;
		    for (p = dw->dw_word; *p != NUL; mb_ptr_adv(p))
			if (!spell_iswordc(p))
			{
			    cc = *p;
			    *p = NUL;
			    break;
			}

		    /* check if we already have this dword */
		    hash = hash_hash(dw->dw_word);
		    hi = hash_lookup(&wi->wi_ht, dw->dw_word, hash);
		    if (!HASHITEM_EMPTY(hi))
		    {
			/* Existing entry. */
			edw = HI2DWORD(hi);
			if ((edw->dw_flags & (DW_CAP | DW_RARE))
				   == (dw->dw_flags & (DW_CAP | DW_RARE)))
			{
			    if (p_verbose > 0)
				smsg((char_u *)_("Warning: duplicate word \"%s\" in %s"),
						      dw->dw_word, fname);
			}
		    }

		    if (cc != NUL) /* nword */
		    {
			if (HASHITEM_EMPTY(hi)
				       || (edw->dw_flags & DW_NWORD) == 0)
			{
			    sblock_T *sb;

			    /* Need to allocate a new nword_T.  Put it in an
			     * sblock_T, so that we can free it later. */
			    sb = (sblock_T *)alloc(
				    (unsigned)(sizeof(sblock_T)
					       + sizeof(nword_T) + wlen));
			    if (sb == NULL)
			    {
				len = 0;
				break;
			    }
			    sb->sb_next = load_lp->sl_block;
			    load_lp->sl_block = sb;
			    nw = (nword_T *)sb->sb_data;

			    ga_init2(&nw->nw_ga, sizeof(dword_T *), 4);
			    nw->nw_maxlen = 0;
			    STRCPY(nw->nw_word, dw->dw_word);
			    if (!HASHITEM_EMPTY(hi))
			    {
				/* Note: the nw_region and nw_flags is for
				 * the dword that matches with the start
				 * of this nword, not for the nword
				 * itself! */
				nw->nw_region = edw->dw_region;
				nw->nw_flags = edw->dw_flags | DW_NWORD;

				/* Remove the dword item so that we can
				 * add it as an nword. */
				hash_remove(&wi->wi_ht, hi);
				hi = hash_lookup(&wi->wi_ht,
						       nw->nw_word, hash);
			    }
			    else
			    {
				nw->nw_region = 0;
				nw->nw_flags = DW_NWORD;
			    }
			}
			else
			    nw = HI2NWORD(hi);
		    }

		    if (HASHITEM_EMPTY(hi))
		    {
			/* Add new dword or nword entry. */
			hash_add_item(&wi->wi_ht, hi, cc == NUL
				       ? dw->dw_word : nw->nw_word, hash);
			if (cc == NUL)
			{
			    /* New dword: init the values and count the
			     * used space.  */
			    dw->dw_flags = DW_DWORD | flags;
			    dw->dw_region = region;
			    bl_used += sizeof(dword_T) + wlen;
			}
		    }
		    else if (cc == NUL)
		    {
			/* existing dword: add the region and flags */
			dw = edw;
			dw->dw_region |= region;
			dw->dw_flags |= DW_DWORD | flags;
		    }

		    if (cc != NUL)
		    {
			/* Use the dword for the non-word character and
			 * following characters. */
			dw->dw_region = region;
			dw->dw_flags = flags;
			STRCPY(dw->dw_word + 1, p + 1);
			dw->dw_word[0] = cc;
			l = wlen - (p - dw->dw_word);
			bl_used += sizeof(dword_T) + l;
			if (nw->nw_maxlen < l)
			    nw->nw_maxlen = l;

			/* Add the dword to the growarray in the nword. */
			if (ga_grow(&nw->nw_ga, 1) == FAIL)
			{
			    len = 0;
			    break;
			}
			*((dword_T **)nw->nw_ga.ga_data + nw->nw_ga.ga_len)
								     = dw;
			++nw->nw_ga.ga_len;
		    }
		}
	    }

	    /* Skip over CR and NL characters and trailing white space. */
	    while (np < rbuf_end && *np <= ' ')
		++np;
	}
    }

    close(fd);
theend:
    sourcing_name = save_sourcing_name;
    sourcing_lnum = save_sourcing_lnum;
    vim_free(rbuf);
}


#endif
