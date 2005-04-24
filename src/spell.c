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
 * The basic spell checking mechanism is:
 * 1. Isolate a word, up to the next non-word character.
 * 2. Find the word in the hashtable of basic words.
 * 3. If not found, look in the hashtable with "prewords".  These are prefixes
 *    with a non-word character following a word character, e.g., "de-".
 * 4. If still not found, for each matching a prefix try if the word matches
 *    without the prefix (and with the "chop" string added back).
 * 5. If still still not found, for each matching suffix try if the word
 *    matches without the suffix (and with the "chop" string added back).
 *
 * Matching involves checking the caps type: Onecap ALLCAP KeepCap.
 * After finding a matching word check for a leadstring (non-word characters
 * before the word) and addstring (more text following, starting with a
 * non-word character).
 *
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
 * Structure that is used to store the structures and strings from the
 * language file.  This avoids the need to allocate space for each individual
 * word.  It's allocated in big chunks for speed.  It's freed all at once when
 * 'encoding' changes.
 */
#define  SBLOCKSIZE 4096	/* default size of sb_data */
typedef struct sblock_S sblock_T;
struct sblock_S
{
    sblock_T	*sb_next;	/* next block in list */
    char_u	sb_data[1];	/* data, actually longer */
};

/* Info from "REP" entries in ".aff" file used in af_rep.
 * TODO: This is not used yet.  Either use it or remove it. */
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
    char_u	ai_flags;	/* AFF_ flags */
    char_u	ai_choplen;	/* length of chop string in bytes */
    char_u	ai_addlen;	/* length of ai_add in bytes */
    char_u	ai_leadlen;	/* for AFF_PREWORD: length of lead string */
    char_u	ai_taillen;	/* for AFF_PREWORD: length of tail string */
    char_u	ai_add[1];	/* Text added to basic word. This stores:
				 * 0: word for AFF_PREWORD or whole addition
				 * ai_addlen + 1: chop string
				 * + ai_choplen + 1: lead string for AFF_PREWORD
				 * + ai_leadlen + 1: trail string f. AFF_PREWORD
				 */
};

/* Get affitem_T pointer from hashitem that uses ai_add */
static affitem_T dumai;
#define HI2AI(hi)	((affitem_T *)((hi)->hi_key - (dumai.ai_add - (char_u *)&dumai)))

/* ai_flags: Affix item flags */
#define AFF_COMBINE	0x01	/* prefix combines with suffix */
#define AFF_PREWORD	0x02	/* prefix includes word */

/*
 * Structure used to store words and other info for one language, loaded from
 * a .spl file.
 * The main access is through hashtable "sl_word", using the case-folded
 * word as the key.  This finds a linked list of fword_T.
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
    hashtab_T	sl_prewords;	/* prefixes that include a word */
    int		sl_suffcnt;	/* number of suffix NRs */
    garray_T	sl_sufftab;	/* list of hashtables to lookup suffixes */
    affitem_T	*sl_suffzero;	/* list of suffixes with zero add length */
    char_u	*sl_try;	/* "TRY" from .aff file  TODO: not used */
    garray_T	sl_rep;		/* list of repentry_T entries from REP lines
				 * TODO not used */
    char_u	sl_regions[17];	/* table with up to 8 region names plus NUL */
    sblock_T	*sl_block;	/* list with allocated memory blocks */
    int		sl_error;	/* error while loading */
};

/* First language that is loaded, start of the linked list of loaded
 * languages. */
static slang_T *first_lang = NULL;

/*
 * Structure to store an addition to a basic word.
 * There are many of these, keep it small!
 */
typedef struct addword_S addword_T;
struct addword_S
{
    addword_T	*aw_next;	/* next addition */
    char_u	aw_flags;	/* ADD_ flags */
    char_u	aw_region;	/* region for word with this addition */
    char_u	aw_leadlen;	/* byte length of lead in aw_word */
    char_u	aw_wordlen;	/* byte length of first word in aw_word */
    char_u	aw_saveb;	/* saved byte where aw_word[] is truncated at
				   end of hashtable key; NUL when not using
				   hashtable */
    char_u	aw_word[1];	/* text, actually longer: case-folded addition
				   plus, with ADD_KEEPCAP: keep-case addition */
};

/* Get addword_T pointer from hashitem that uses aw_word */
static addword_T dumaw;
#define HI2ADDWORD(hi)	((addword_T *)((hi)->hi_key - (dumaw.aw_word - (char_u *)&dumaw)))

/*
 * Structure to store a basic word.
 * There are many of these, keep it small!
 * The list of prefix and suffix NRs is stored after "fw_word" to avoid the
 * need for two extra pointers.
 */
typedef struct fword_S fword_T;
struct fword_S
{
    fword_T	*fw_next;	/* same basic word with different caps and/or
				 * affixes */
    addword_T	*fw_adds;	/* first addword_T entry */
    short_u	fw_flags;	/* BWF_ flags */
    char_u	fw_region;	/* region bits */
    char_u	fw_prefixcnt;	/* number of prefix NRs */
    char_u	fw_suffixcnt;	/* number of suffix NRs */
    char_u	fw_word[1];	/* actually longer:
				 * 0:  case folded word or keep-case word when
				 *     (flags & BWF_KEEPCAP)
				 * + word length + 1: list of prefix NRs
				 * + fw_prefixcnt [* 2]: list of suffix NRs
				 */
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
#define BWF_ADDS_M	0x1000	    /* there are more than 255 additions */

#define BWF_ADDHASH	0x8000	    /* Internal: use hashtab for additions */

#define NOWC_KEY (char_u *)"x"	    /* hashtab key used for additions without
				       any word character */

/* flags used for addition in the spell file */
#define ADD_REGION	0x02	    /* region byte follows */
#define ADD_ONECAP	0x04	    /* first letter must be capital */
#define ADD_LEADLEN	0x10	    /* there is a leadlen byte */
#define ADD_COPYLEN	0x20	    /* there is a copylen byte */
#define ADD_ALLCAP	0x40	    /* all letters must be capital (not used
				       for single-letter words) */
#define ADD_KEEPCAP	0x80	    /* fixed case */

/* Translate ADD_ flags to BWF_ flags.
 * (Needed to keep ADD_ flags in one byte.) */
#define ADD2BWF(x)	(((x) & 0x0f) | (((x) & 0xf0) << 4))

#define VIMSPELLMAGIC "VIMspell04"  /* string at start of Vim spell file */
#define VIMSPELLMAGICL 10

/*
 * Structure to store info for word matching.
 */
typedef struct matchinf_S
{
    langp_T	*mi_lp;			/* info for language and region */
    slang_T	*mi_slang;		/* info for the language */

    /* pointers to original text to be checked */
    char_u	*mi_line;		/* start of line containing word */
    char_u	*mi_word;		/* start of word being checked */
    char_u	*mi_end;		/* first non-word char after mi_word */
    char_u	*mi_wend;		/* end of matching word (is mi_end
					 * or further) */
    char_u	*mi_fend;		/* next char to be added to mi_fword */

    /* case-folded text */
    char_u	mi_fword[MAXWLEN + 1];	/* mi_word case-folded */
    int		mi_fendlen;		/* byte length of first word in
					   mi_fword */
    int		mi_faddlen;		/* byte length of text in mi_fword
					   after first word */
    char_u	*mi_cword;		/* word to check, points in mi_fword */
    char_u	*mi_awend;		/* after next word, to check for
					   addition (NULL when not done yet) */
    int		mi_did_awend;		/* did compute mi_awend */

    /* others */
    int		mi_result;		/* result so far: SP_BAD, SP_OK, etc. */
    int		mi_capflags;		/* BWF_ONECAP BWF_ALLCAP BWF_KEEPCAP */
} matchinf_T;

static int word_match __ARGS((matchinf_T *mip));
static int check_adds __ARGS((matchinf_T *mip, fword_T *fw, int req_pref, int req_suf));
static void fill_awend __ARGS((matchinf_T *mip));
static void fold_addchars __ARGS((matchinf_T *mip, int addlen));
static int supports_affix __ARGS((int cnt, char_u *afflist, int afflistlen, int nr));
static int prefix_match __ARGS((matchinf_T *mip));
static int noprefix_match __ARGS((matchinf_T *mip, char_u *pword, char_u *cstart, affitem_T *ai));
static int suffix_match __ARGS((matchinf_T *mip));
static int match_caps __ARGS((int flags, char_u *caseword, matchinf_T *mip, char_u *cword, char_u *end));
static slang_T *slang_alloc __ARGS((char_u *lang));
static void slang_free __ARGS((slang_T *lp));
static slang_T *spell_load_lang __ARGS((char_u *lang));
static void spell_load_file __ARGS((char_u *fname, void *cookie));
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

    /* Make case-folded copy of the word. */
    (void)spell_casefold(ptr, mi.mi_end - ptr, mi.mi_fword, MAXWLEN + 1);
    mi.mi_cword = mi.mi_fword;
    mi.mi_fendlen = STRLEN(mi.mi_fword);
    mi.mi_faddlen = 0;
    mi.mi_fend = mi.mi_end;

    /* Check the caps type of the word. */
    mi.mi_capflags = captype(ptr, mi.mi_end);

    /* The word is bad unless we recognize it. */
    mi.mi_result = SP_BAD;
    mi.mi_wend = mi.mi_end;

    mi.mi_awend = NULL;
    mi.mi_did_awend = FALSE;
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
 * Check if the word "mip->mi_word" matches.
 * "mip->mi_fword" is the same word case-folded;
 *
 * This checks the word as a whole and for prefixes that include a word.
 *
 * Note that when called mi_fword only contains the word up to mip->mi_end,
 * but when checking additions it gets longer.
 */
    static int
word_match(mip)
    matchinf_T *mip;
{
    hash_T	fhash = hash_hash(mip->mi_fword);
    hashitem_T	*hi;
    fword_T	*fw;
    int		valid = FALSE;
    char_u	*p;
    char_u	pword[MAXWLEN + 1];
    int		charlen;
    int		capflags_save;
    affitem_T	*ai;
    char_u	*cstart;
    int		addlen;
    int		n;
    char_u	*save_end;
    int		cc;

    hi = hash_lookup(&mip->mi_slang->sl_words, mip->mi_fword, fhash);
    if (!HASHITEM_EMPTY(hi))
    {
	/*
	 * Find a basic word for which the case of "mi_word" is correct.
	 * If it is, check additions and use the longest one.
	 */
	for (fw = HI2FWORD(hi); fw != NULL; fw = fw->fw_next)
	    if (match_caps(fw->fw_flags, fw->fw_word, mip,
						   mip->mi_word, mip->mi_end))
		valid |= check_adds(mip, fw, -1, -1);
    }

    /*
     * Try finding a matching preword for "mip->mi_word".  These are
     * prefixes that have a non-word character after a word character:
     * "d'", "de-", "'s-", "l'de-".  But not "'s".
     * Also need to do this when a matching word was already found, because we
     * might find a longer match this way (French: "qu" and "qu'a-t-elle").
     * The check above may have added characters to mi_fword, thus we need to
     * truncate it after the basic word for the hash lookup.
     */
    cc = mip->mi_fword[mip->mi_fendlen];
    mip->mi_fword[mip->mi_fendlen] = NUL;
    hi = hash_lookup(&mip->mi_slang->sl_prewords, mip->mi_fword, fhash);
    mip->mi_fword[mip->mi_fendlen] = cc;
    if (!HASHITEM_EMPTY(hi))
    {
	capflags_save = mip->mi_capflags;

	/* Go through the list of matching prewords. */
	for (ai = HI2AI(hi); ai != NULL; ai = ai->ai_next)
	{
	    /* Check that the lead string matches before the word. */
	    p = ai->ai_add + ai->ai_addlen + ai->ai_choplen + 2;
	    if (ai->ai_leadlen > 0)
	    {
		if (mip->mi_word - mip->mi_line < ai->ai_leadlen
			|| STRNCMP(mip->mi_word - ai->ai_leadlen, p,
						     ai->ai_leadlen) != 0)
		    continue;
		p += ai->ai_leadlen + 1;	/* advance "p" to tail */
	    }
	    else
		++p;			/* advance "p" to tail */

	    /* Check that the tail string matches after the word.  Need
	     * to fold case first.  */
	    if (ai->ai_taillen > 0)
	    {
		if (ai->ai_taillen >= mip->mi_faddlen)
		{
		    fold_addchars(mip, ai->ai_taillen);
		    if (ai->ai_taillen > mip->mi_faddlen)
			continue;	/* not enough chars, can't match */
		}
		if (STRNCMP(mip->mi_fword + mip->mi_fendlen,
						  p, ai->ai_taillen) != 0)
		    continue;
	    }

	    /*
	     * This preword matches.  Remove the preword and check that
	     * the resulting word exits.
	     */

	    /* Find the place in the original word where the tail ends,
	     * needed for case checks. */
#ifdef FEAT_MBYTE
	    charlen = mb_charlen(p);
#else
	    charlen = ai->ai_taillen;
#endif
	    cstart = mip->mi_end;
	    for (n = 0; n < charlen; ++n)
		mb_ptr_adv(cstart);

	    /* The new word starts with the chop. Then add up to the next
	     * non-word char. */
	    mch_memmove(pword, ai->ai_add + ai->ai_addlen + 1,
							  ai->ai_choplen);
	    p = mip->mi_fword + mip->mi_fendlen + ai->ai_taillen;
	    addlen = ai->ai_taillen;
	    while (spell_iswordc(p))
	    {
		++charlen;
#ifdef FEAT_MBYTE
		addlen += (*mb_ptr2len_check)(p);
#else
		++addlen;
#endif
		mb_ptr_adv(p);
		if (addlen >= mip->mi_faddlen)
		{
		    /* Get more folded characters in mip->mi_fword. */
		    fold_addchars(mip, addlen);
		    if (addlen >= mip->mi_faddlen)
			break;	/* not enough chars, can't match */
		}
	    }
	    mch_memmove(pword + ai->ai_choplen,
		    mip->mi_fword + mip->mi_fendlen + ai->ai_taillen,
						 addlen - ai->ai_taillen);
	    pword[ai->ai_choplen + addlen - ai->ai_taillen] = NUL;

	    /* Need to set mi_end to find additions.  Also set mi_fendlen
	     * and mi_faddlen. */
	    save_end = mip->mi_end;
	    while (--charlen >= 0)
		mb_ptr_adv(mip->mi_end);
	    mip->mi_fendlen += addlen;
	    mip->mi_faddlen -= addlen;

	    /* Find the word "pword", caseword "cstart". */
	    n = noprefix_match(mip, pword, cstart, ai);
	    mip->mi_end = save_end;
	    mip->mi_fendlen -= addlen;
	    mip->mi_faddlen += addlen;
	    if (n)
		valid = TRUE;

	    /* If we found a valid word, we still need to try other
	     * suffixes, because it may have an addition that's longer. */
	}

	mip->mi_capflags = capflags_save;
    }

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
    addword_T	*naw = NULL;
    char_u	*p;
    int		addlen;
    int		cc;
    hashitem_T	*hi;
    char_u	*cp = NULL;
    int		n;

    /* Check if required prefixes and suffixes are supported.  These are on
     * the basic word, not on each addition. */
    if (req_pref >= 0 || req_suf >= 0)
    {
	/* Prefix NRs are stored just after the word in fw_word. */
	cp = fw->fw_word + STRLEN(fw->fw_word) + 1;
	if (req_pref >= 0 && !supports_affix(mip->mi_slang->sl_prefcnt,
					      cp, fw->fw_prefixcnt, req_pref))
	    return FALSE;
	if (req_suf >= 0)
	{
	    /* Suffix NRs are stored just after the Prefix NRs. */
	    if (fw->fw_prefixcnt > 0)
	    {
		if (mip->mi_slang->sl_prefcnt > 256)
		    cp += fw->fw_prefixcnt * 2;
		else
		    cp += fw->fw_prefixcnt;
	    }
	    if (!supports_affix(mip->mi_slang->sl_suffcnt,
					       cp, fw->fw_suffixcnt, req_suf))
		return FALSE;
	}
    }

    /* A word may be valid without an addition. */
    if (fw->fw_flags & BWF_VALID)
    {
	valid = TRUE;
	if (mip->mi_result != SP_OK)
	{
	    if ((fw->fw_region & mip->mi_lp->lp_region) == 0)
		mip->mi_result = SP_LOCAL;
	    else
		mip->mi_result = SP_OK;
	}
	/* Set word end, required when matching a word after a preword. */
	if (mip->mi_wend < mip->mi_end)
	    mip->mi_wend = mip->mi_end;
    }

    /*
     * Check additions, both before and after the word.
     * This may make the word longer, thus we also need to check
     * when we already found a matching word.
     * When the BWF_ADDHASH flag is present then fw_adds points to a hashtable
     * for quick lookup.  Otherwise it points to the list of all possible
     * additions.
     */
    if (fw->fw_flags & BWF_ADDHASH)
    {
	/* Locate the text up to the next end-of-word. */
	if (!mip->mi_did_awend)
	    fill_awend(mip);
	if (mip->mi_awend == NULL)
	    return valid;	    /* there is no next word */

	cc = *mip->mi_awend;
	*mip->mi_awend = NUL;
	hi = hash_find((hashtab_T *)fw->fw_adds,
					     mip->mi_fword + mip->mi_fendlen);
	*mip->mi_awend = cc;
	if (HASHITEM_EMPTY(hi))
	    return valid;		/* no matching addition */
	aw = HI2ADDWORD(hi);

	/* Also check additions without word characters.  If they are there,
	 * skip the first dummy entry. */
	hi = hash_find((hashtab_T *)fw->fw_adds, NOWC_KEY);
	if (!HASHITEM_EMPTY(hi))
	    naw = HI2ADDWORD(hi)->aw_next;
    }
    else
	aw = fw->fw_adds;

    for ( ; ; aw = aw->aw_next)
    {
	if (aw == NULL)
	{
	    /* At end of list: may also try additions without word chars. */
	    if (naw == NULL)
		break;
	    aw = naw;
	    naw = NULL;
	}

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
	    if (addlen >= mip->mi_faddlen)
		fold_addchars(mip, addlen);

	    /* Put back the saved char, if needed. */
	    if (aw->aw_saveb != NUL)
	    {
		cp = p + STRLEN(p);
		*cp = aw->aw_saveb;
	    }
	    n = STRNCMP(mip->mi_fword + mip->mi_fendlen, p, addlen);
	    if (aw->aw_saveb != NUL)
		*cp = NUL;

	    if (n != 0 || (mip->mi_fword[mip->mi_fendlen + addlen] != NUL
		  && spell_iswordc(mip->mi_fword + mip->mi_fendlen + addlen)))
		continue;

	    /* Compute the length in the original word, before case folding. */
#ifdef FEAT_MBYTE
	    if (has_mbyte)
	    {
		int	l;

		p = mip->mi_end;
		for (l = 0; l < addlen; l += (*mb_ptr2len_check)(mip->mi_fword
						       + mip->mi_fendlen + l))
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
 * Locate the text up to the next end-of-word after mip->mi_end.
 */
    static void
fill_awend(mip)
    matchinf_T	*mip;
{
    char_u	*p = mip->mi_end;
    int		addlen = 0;
    int		find_word = TRUE;

    mip->mi_did_awend = TRUE;
    if (mip->mi_faddlen == 0)
	fold_addchars(mip, 0);	    /* need to fold first char */

    /* 1: find_word == TRUE: skip over non-word characters after mi_end.
     * 2: find_word == FALSE: skip over following word characters. */
    for (p = mip->mi_fword + mip->mi_fendlen; *p != NUL; mb_ptr_adv(p))
    {
	if (spell_iswordc(p) == find_word)
	{
	    if (!find_word)
		break;		    /* done */
	    find_word = !find_word;
	}
#ifdef FEAT_MBYTE
	addlen += (*mb_ptr2len_check)(p);
#else
	++addlen;
#endif
	if (addlen >= mip->mi_faddlen)
	    fold_addchars(mip, addlen);	    /* need to fold more chars */
    }

    /* If there are extra chars store the result. */
    if (addlen != 0)
	mip->mi_awend = p;
}

/*
 * Fold enough characters of the checked text to be able to compare with an
 * addition of length "addlen" plus one character (to be able to check the
 * next character to be a non-word char).
 * When there are not enough characters (end of line) mip->mi_faddlen will be
 * smaller than "addlen".
 */
    static void
fold_addchars(mip, addlen)
    matchinf_T	*mip;
    int		addlen;
{
    int		l;
    char_u	*p = mip->mi_fword + mip->mi_fendlen;

    while (mip->mi_faddlen <= addlen)
    {
	if (*mip->mi_fend == NUL)	/* end of the line */
	{
	    p[mip->mi_faddlen] = NUL;
	    break;
	}
#ifdef FEAT_MBYTE
	if (has_mbyte)
	    l = (*mb_ptr2len_check)(mip->mi_fend);
	else
#endif
	    l = 1;
	(void)spell_casefold(mip->mi_fend, l, p + mip->mi_faddlen,
				 MAXWLEN - mip->mi_fendlen - mip->mi_faddlen);
	mip->mi_fend += l;
	mip->mi_faddlen += STRLEN(p + mip->mi_faddlen);
    }
}

/*
 * Return TRUE if affix "nr" appears in affix list "afflist[afflistlen]".
 */
    static int
supports_affix(cnt, afflist, afflistlen, nr)
    int		cnt;		/* total affix NR count */
    char_u	*afflist;
    int		afflistlen;	/* affix count in "afflist" */
    int		nr;
{
    char_u	*pc = afflist;
    int		i;
    int		nr_msb, nr_lsb;

    if (cnt <= 256)
    {
	/* one byte affix numbers */
	for (i = afflistlen; --i >= 0; )
	    if (*pc++ == nr)
		return TRUE;
    }
    else
    {
	/* two byte affix numbers, MSB first */
	nr_msb = (unsigned)nr >> 8;
	nr_lsb = nr & 0xff;
	for (i = afflistlen; --i >= 0; )
	{
	    if (*pc++ == nr_msb && *pc == nr_lsb)
		return TRUE;
	    ++pc;
	}
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
    hashtab_T	*ht;
    hashitem_T	*hi;
    int		found_valid = FALSE;
    int		cstart_charlen = 0;
    char_u	*cstart = mip->mi_word;
    int		capflags_save = mip->mi_capflags;

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
		len += (*mb_ptr2len_check)(mip->mi_cword + len);
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
	    /* Create the basic word from the chop string and the word after
	     * the matching add string. */
	    mch_memmove(pword, ai->ai_add + ai->ai_addlen + 1, ai->ai_choplen);
	    mch_memmove(pword + ai->ai_choplen, mip->mi_cword + ai->ai_addlen,
					     mip->mi_fendlen - ai->ai_addlen);
	    pword[mip->mi_fendlen - ai->ai_addlen] = NUL;

	    /* Adjust the word start for case checks, we only check the
	     * part after the prefix. */
	    while (cstart_charlen < charlen)
	    {
		mb_ptr_adv(cstart);
		++cstart_charlen;
	    }

	    /* Find the word "pword", caseword "cstart". */
	    found_valid |= noprefix_match(mip, pword, cstart, ai);

	    if (found_valid && mip->mi_result == SP_OK)
	    {
		/* Found a valid word, no need to try other suffixes. */
		mip->mi_capflags = capflags_save;
		return TRUE;
	    }
	}
    }

    mip->mi_capflags = capflags_save;
    return FALSE;
}

/*
 * Check for matching word after removing a prefix.
 * Return TRUE if found.
 */
    static int
noprefix_match(mip, pword, cstart, ai)
    matchinf_T	*mip;
    char_u	*pword;	    /* case-folded word */
    char_u	*cstart;    /* original word after removed prefix */
    affitem_T	*ai;	    /* the prefix item */
{
    hashitem_T	*hi;
    fword_T	*fw;
    int		found_valid = FALSE;
    char_u	*word;
    int		i;
    int		fendlen;

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
	    /* Found a valid word, no need to try other suffixes. */
	    return TRUE;
    }

    /* No matching basic word without prefix.  When combining is
     * allowed try with suffixes. */
    if (ai->ai_flags & AFF_COMBINE)
    {
	/* Pass the word with prefix removed to suffix_match(). */
	mip->mi_cword = pword;
	word = mip->mi_word;
	mip->mi_word = cstart;
	fendlen = mip->mi_fendlen;
	mip->mi_fendlen = STRLEN(pword);
	i = suffix_match(mip);
	mip->mi_cword = mip->mi_fword;
	mip->mi_word = word;
	mip->mi_fendlen = fendlen;
	if (i)
	    return TRUE;
    }

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
    char_u	*endw = mip->mi_cword + mip->mi_fendlen;
    int		endw_c = *endw;
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
    sufp = endw;
    *endw = NUL;	/* truncate after possible suffix */

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
	    *endw = endw_c;

	    for ( ; ai != NULL; ai = ai->ai_next)
	    {
		/* Found a matching suffix.  Create the basic word by removing
		 * the suffix and adding the chop string. */
		if (ai->ai_choplen == 0)
		    pword[tlen] = NUL;
		else
		    mch_memmove(pword + tlen, ai->ai_add + ai->ai_addlen + 1,
							  ai->ai_choplen + 1);

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

	    *endw = NUL;	/* truncate after possible suffix */
	}
    }

    *endw = endw_c;
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
	/* If "end" is past "mip->mi_end" we need to adjust the caps type for
	 * characters after the basic word. */
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
		if (spell_isupper(c))
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
    linenr_T	lnum;
    pos_T	found_pos;
    char_u	*line;
    char_u	*p;
    int		wc;
    int		nwc;
    int		attr = 0;
    int		len;
    int		has_syntax = syntax_present(curbuf);
    int		col;
    int		can_spell;

    if (!curwin->w_p_spell || *curwin->w_buffer->b_p_spl == NUL)
    {
	EMSG(_("E756: Spell checking not enabled"));
	return FAIL;
    }

    /*
     * Start looking for bad word at the start of the line, because we can't
     * start halfway a word, we don't know where it starts or ends.
     *
     * When searching backwards, we continue in the line to find the last
     * bad word (in the cursor line: before the cursor).
     */
    lnum = curwin->w_cursor.lnum;
    found_pos.lnum = 0;

    while (!got_int)
    {
	line = ml_get(lnum);
	p = line;
	wc = FALSE;

	while (*p != NUL)
	{
	    nwc = spell_iswordc(p);
	    if (!wc && nwc)
	    {
		/* When searching backward don't search after the cursor. */
		if (dir == BACKWARD
			&& lnum == curwin->w_cursor.lnum
			&& (colnr_T)(p - line) >= curwin->w_cursor.col)
		    break;

		/* start of word */
		len = spell_check(curwin, line, p, &attr);

		if (attr != 0)
		{
		    /* We found a bad word.  Check the attribute. */
		    /* TODO: check for syntax @Spell cluster. */
		    if (allwords || attr == highlight_attr[HLF_SPB])
		    {
			/* When searching forward only accept a bad word after
			 * the cursor. */
			if (dir == BACKWARD
				|| lnum > curwin->w_cursor.lnum
				|| (lnum == curwin->w_cursor.lnum
				    && (colnr_T)(p - line)
						      > curwin->w_cursor.col))
			{
			    if (has_syntax)
			    {
				col = p - line;
				(void)syn_get_id(lnum, (colnr_T)col,
							   FALSE, &can_spell);

				/* have to get the line again, a multi-line
				 * regexp may make it invalid */
				line = ml_get(lnum);
				p = line + col;
			    }
			    else
				can_spell = TRUE;

			    if (can_spell)
			    {
				found_pos.lnum = lnum;
				found_pos.col = p - line;
#ifdef FEAT_VIRTUALEDIT
				found_pos.coladd = 0;
#endif
				if (dir == FORWARD)
				{
				    /* No need to search further. */
				    curwin->w_cursor = found_pos;
				    return OK;
				}
			    }
			}
		    }
		    attr = 0;
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
	if (dir == BACKWARD)
	{
	    if (found_pos.lnum != 0)
	    {
		/* Use the last match in the line. */
		curwin->w_cursor = found_pos;
		return OK;
	    }
	    if (lnum == 1)
		return FAIL;
	    --lnum;
	}
	else
	{
	    if (lnum == curbuf->b_ml.ml_line_count)
		return FAIL;
	    ++lnum;
	}

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
	if (r == FAIL && !lp->sl_error)
	{
	    /* Try loading the ASCII version. */
	    sprintf((char *)fname_enc, "spell/%s.ascii.spl", lang);

	    r = do_in_runtimepath(fname_enc, TRUE, spell_load_file, lp);
	}
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
	hash_init(&lp->sl_prewords);
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
    fword_T	*fw;
    int		todo;
    hashitem_T	*hi;

    vim_free(lp->sl_name);

    /* The words themselves are in memory blocks referenced by "sl_block".
     * Only the hashtables for additions need to be cleared. */
    todo = lp->sl_words.ht_used;
    for (hi = lp->sl_words.ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    fw = HI2FWORD(hi);
	    if (fw->fw_flags & BWF_ADDHASH)
		hash_clear((hashtab_T *)fw->fw_adds);
	}
    }
    hash_clear(&lp->sl_words);

    for (i = 0; i < lp->sl_preftab.ga_len; ++i)
	hash_clear(((hashtab_T *)lp->sl_preftab.ga_data) + i);
    ga_clear(&lp->sl_preftab);
    hash_clear(&lp->sl_prewords);
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
    char_u	affixbuf[256 * 2 * 2]; /* max 2 * 256 affix nrs of 2 bytes */
    char_u	*p;
    int		itm;
    int		i;
    int		affcount;
    int		affnr;
    int		affflags;
    int		affitemcnt;
    int		prefixcnt, suffixcnt;
    int		bl_used = SBLOCKSIZE;
    int		widx;
    int		prefm = 0;  /* 1 if <= 256 prefixes, sizeof(short_u) otherw. */
    int		suffm = 0;  /* 1 if <= 256 suffixes, sizeof(short_u) otherw. */
    int		wlen;
    int		flags;
    affitem_T	*ai, *ai2, **aip;
    int		round;
    char_u	*save_sourcing_name = sourcing_name;
    linenr_T	save_sourcing_lnum = sourcing_lnum;
    int		cnt, ccnt;
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
    addword_T	*aw, *naw;
    int		flen;
    int		xlen;
    char_u	*fol;

    fd = fopen((char *)fname, "r");
    if (fd == NULL)
    {
	EMSG2(_(e_notopen), fname);
	goto endFAIL;
    }

    /* Set sourcing_name, so that error messages mention the file name. */
    sourcing_name = fname;
    sourcing_lnum = 0;

    /* <HEADER>: <fileID> <regioncnt> <regionname> ...
     *		 <charflagslen> <charflags>  <fcharslen> <fchars> */
    for (i = 0; i < VIMSPELLMAGICL; ++i)
	buf[i] = getc(fd);				/* <fileID> */
    if (STRNCMP(buf, VIMSPELLMAGIC, VIMSPELLMAGICL) != 0)
    {
	EMSG(_("E757: Wrong file ID in spell file"));
	goto endFAIL;
    }

    cnt = getc(fd);					/* <regioncnt> */
    if (cnt < 0)
    {
truncerr:
	EMSG(_("E758: Truncated spell file"));
	goto endFAIL;
    }
    if (cnt > 8)
    {
formerr:
	EMSG(_("E759: Format error in spell file"));
	goto endFAIL;
    }
    for (i = 0; i < cnt; ++i)
    {
	lp->sl_regions[i * 2] = getc(fd);		/* <regionname> */
	lp->sl_regions[i * 2 + 1] = getc(fd);
    }
    lp->sl_regions[cnt * 2] = NUL;

    cnt = getc(fd);					/* <charflagslen> */
    if (cnt > 0)
    {
	p = (char_u *)getroom(lp, &bl_used, cnt);
	if (p == NULL)
	    goto endFAIL;
	for (i = 0; i < cnt; ++i)
	    p[i] = getc(fd);				/* <charflags> */

	ccnt = (getc(fd) << 8) + getc(fd);		/* <fcharslen> */
	if (ccnt <= 0)
	    goto formerr;
	fol = (char_u *)getroom(lp, &bl_used, ccnt + 1);
	if (fol == NULL)
	    goto endFAIL;
	for (i = 0; i < ccnt; ++i)
	    fol[i] = getc(fd);				/* <fchars> */
	fol[i] = NUL;

	/* Set the word-char flags and fill spell_isupper() table. */
	if (set_spell_charflags(p, cnt, fol) == FAIL)
	    goto formerr;
    }
    else
    {
	/* When <charflagslen> is zero then <fcharlen> must also be zero. */
	cnt = (getc(fd) << 8) + getc(fd);
	if (cnt != 0)
	    goto formerr;
    }

    /* round 1: <PREFIXLIST>: <affcount> <affix> ...
     * round 2: <SUFFIXLIST>: <affcount> <affix> ...  */
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
	    prefm = affcount > 256 ? 2 : 1;
	}
	else
	{
	    gap = &lp->sl_sufftab;
	    aip = &lp->sl_suffzero;
	    lp->sl_suffcnt = affcount;
	    suffm = affcount > 256 ? 2 : 1;
	}

	/*
	 * For each affix NR there can be several affixes.
	 */
	for (affnr = 0; affnr < affcount; ++affnr)
	{
	    /* <affix>: <affitemcnt> <affitem> ... */
	    affitemcnt = (getc(fd) << 8) + getc(fd);	/* <affitemcnt> */
	    if (affitemcnt < 0)
		goto truncerr;
	    for (itm = 0; itm < affitemcnt; ++itm)
	    {
		/* <affitem>: <affflags> <affchoplen> <affchop>
		 *				    <affaddlen> <affadd> */
		affflags = getc(fd);			/* <affflags> */
		choplen = getc(fd);			/* <affchoplen> */
		if (choplen < 0)
		    goto truncerr;
		if (choplen >= MAXWLEN)
		    goto formerr;
		for (i = 0; i < choplen; ++i)		/* <affchop> */
		    buf[i] = getc(fd);
		buf[i] = NUL;
		addlen = getc(fd);			/* <affaddlen> */
		if (addlen < 0)
		    goto truncerr;
		if (affflags & AFF_PREWORD)
		    xlen = addlen + 2;	/* space for lead and trail string */
		else
		    xlen = 0;

		/* Get room to store the affitem_T, chop and add strings. */
		ai = (affitem_T *)getroom(lp, &bl_used,
			     sizeof(affitem_T) + addlen + choplen + 1 + xlen);
		if (ai == NULL)
		    goto endFAIL;

		ai->ai_nr = affnr;
		ai->ai_flags = affflags;
		ai->ai_choplen = choplen;
		ai->ai_addlen = addlen;

		/* Chop string is at ai_add[ai_addlen + 1]. */
		p = ai->ai_add + addlen + 1;
		STRCPY(p, buf);

		p = ai->ai_add;
		for (i = 0; i < addlen; ++i)		/* <affadd> */
		    p[i] = getc(fd);
		p[i] = NUL;

		if (affflags & AFF_PREWORD)
		{
		    int	    l, leadoff, trailoff;

		    /*
		     * A preword is a prefix that's recognized as a word: it
		     * contains a word characters folled by a non-word
		     * character.
		     * <affadd> is the whole prefix.  Separate lead and trail
		     * string, put the word itself at ai_add, so that it can
		     * be used as hashtable key.
		     */
		    /* lead string: up to first word char */
		    while (*p != NUL && !spell_iswordc(p))
			mb_ptr_adv(p);
		    ai->ai_leadlen = p - ai->ai_add;
		    leadoff = addlen + choplen + 2;
		    mch_memmove(ai->ai_add + leadoff, ai->ai_add,
							      ai->ai_leadlen);
		    ai->ai_add[leadoff + ai->ai_leadlen] = NUL;

		    /* trail string: after last word char */
		    while (*p != NUL && spell_iswordc(p))
			mb_ptr_adv(p);
		    trailoff = leadoff + ai->ai_leadlen + 1;
		    STRCPY(ai->ai_add + trailoff, p);
		    ai->ai_taillen = STRLEN(p);

		    /* word itself */
		    l = (p - ai->ai_add) - ai->ai_leadlen;
		    mch_memmove(ai->ai_add, ai->ai_add + ai->ai_leadlen, l);
		    ai->ai_add[l] = NUL;
		    hash = hash_hash(ai->ai_add);
		    hi = hash_lookup(&lp->sl_prewords, ai->ai_add, hash);
		    if (HASHITEM_EMPTY(hi))
		    {
			/* First preword with this word, add to hashtable. */
			hash_add_item(&lp->sl_prewords, hi, ai->ai_add, hash);
			ai->ai_next = NULL;
		    }
		    else
		    {
			/* There already is a preword with this word, link in
			 * the list.  */
			ai2 = HI2AI(hi);
			ai->ai_next = ai2->ai_next;
			ai2->ai_next = ai;
		    }
		}
		else
		{
		    /*
		     * Add the affix to a hashtable.  Which one depends on the
		     * length of the added string in characters.
		     */
#ifdef FEAT_MBYTE
		    /* Change "addlen" from length in bytes to length in
		     * chars. */
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
				goto endFAIL;

			    /* Re-allocating ga_data means that an ht_array
			     * pointing to ht_smallarray becomes invalid.  We
			     * can recognize this: ht_mask is at its init
			     * value. */
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
			    /* First affix with this "ai_add", add to
			     * hashtable. */
			    hash_add_item(ht, hi, p, hash);
			    ai->ai_next = NULL;
			}
			else
			{
			    /* There already is an affix with this "ai_add",
			     * link in the list.  */
			    ai2 = HI2AI(hi);
			    ai->ai_next = ai2->ai_next;
			    ai2->ai_next = ai;
			}
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
	if (wlen < 0)
	{
	    if (widx >= wordcount)	/* normal way to end the file */
		break;
	    goto truncerr;
	}

	/* Read further word bytes until one below 0x20, that one must be the
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
	     * basic word without KEEPCAP! */
	    wlen = getc(fd);
	    if (wlen < 0)
		goto truncerr;
	    if (wlen >= MAXWLEN)
		goto formerr;
	    for (i = 0; i < wlen; ++i)
		cbuf[i] = getc(fd);
	    cbuf[i] = NUL;
	}

	/* Optional prefixes */
	p = affixbuf;
	if (flags & BWF_PREFIX)
	{
	    cnt = getc(fd);				/* <affixcnt> */
	    if (cnt < 0)
		goto truncerr;
	    prefixcnt = cnt;
	    for (i = cnt * prefm; --i >= 0; )		/* <affixNR> */
		*p++ = getc(fd);
	}
	else
	    prefixcnt = 0;

	/* Optional suffixes */
	if (flags & BWF_SUFFIX)
	{
	    cnt = getc(fd);				/* <affixcnt> */
	    if (cnt < 0)
		goto truncerr;
	    suffixcnt = cnt;
	    for (i = cnt * suffm; --i >= 0; )		/* <affixNR> */
		*p++ = getc(fd);
	}
	else
	    suffixcnt = 0;

	/* Find room to store the word in an fword_T. */
	fw = (fword_T *)getroom(lp, &bl_used, (int)sizeof(fword_T) + wlen
							    + (p - affixbuf));
	if (fw == NULL)
	    goto endFAIL;
	mch_memmove(fw->fw_word, (flags & BWF_KEEPCAP) ? cbuf : buf, wlen + 1);

	/* Put the affix NRs just after the word, if any. */
	if (p > affixbuf)
	    mch_memmove(fw->fw_word + wlen + 1, affixbuf, p - affixbuf);

	fw->fw_flags = flags;
	fw->fw_prefixcnt = prefixcnt;
	fw->fw_suffixcnt = suffixcnt;

	/* We store the word in the hashtable case-folded.  For a KEEPCAP word
	 * the entry must already exist, because fw_word can't be used as the
	 * key, it differs from "buf"! */
	hash = hash_hash(buf);
	hi = hash_lookup(&lp->sl_words, buf, hash);
	if (HASHITEM_EMPTY(hi))
	{
	    if (hash_add_item(&lp->sl_words, hi, fw->fw_word, hash) == FAIL)
		goto endFAIL;
	    fw->fw_next = NULL;
	}
	else
	{
	    /* Already have this basic word in the hashtable, this one will
	     * have different case flags and/or affixes. */
	    fw2 = HI2FWORD(hi);
	    fw->fw_next = fw2->fw_next;
	    fw2->fw_next = fw;
	    --widx;		/* don't count this one as a basic word */
	}

	if (flags & BWF_REGION)
	    fw->fw_region = getc(fd);			/* <region> */
	else
	    fw->fw_region = REGION_ALL;

	fw->fw_adds = NULL;
	if (flags & BWF_ADDS)
	{
	    if (flags & BWF_ADDS_M)
		adds = (getc(fd) << 8) + getc(fd);	/* <addcnt> */
	    else
		adds = getc(fd);			/* <addcnt> */
	    if (adds < 0)
		goto formerr;

	    if (adds > 30)
	    {
		/* Use a hashtable to lookup the part until the next word end.
		 * Thus for "de-bur-die" "de" is the basic word, "-bur" is key
		 * in the addition hashtable, "-bur<NUL>die" the whole
		 * addition and "aw_saveb" is '-'.
		 * This uses more memory and involves some overhead, thus only
		 * do it when there are many additions (e.g., for French). */
		ht = (hashtab_T *)getroom(lp, &bl_used, sizeof(hashtab_T));
		if (ht == NULL)
		    goto endFAIL;
		hash_init(ht);
		fw->fw_adds = (addword_T *)ht;
		fw->fw_flags |= BWF_ADDHASH;

		/* Preset the size of the hashtable. It's never unlocked. */
		hash_lock_size(ht, adds + 1);
	    }
	    else
		ht = NULL;

	    /*
	     * Note: uses cbuf[] to copy bytes from previous addition.
	     */
	    while (--adds >= 0)
	    {
		/* <add>: <addflags> <addlen> [<leadlen>] [<copylen>]
		 *				[<addstring>] [<region>] */
		flags = getc(fd);			/* <addflags> */
		addlen = getc(fd);			/* <addlen> */
		if (addlen < 0)
		    goto truncerr;
		if (addlen >= MAXWLEN)
		    goto formerr;

		if (flags & ADD_LEADLEN)
		{
		    leadlen = getc(fd);			/* <leadlen> */
		    if (leadlen > addlen)
			goto formerr;
		}
		else
		    leadlen = 0;

		if (addlen > 0)
		{
		    if (flags & ADD_COPYLEN)
			i = getc(fd);			/* <copylen> */
		    else
			i = 0;
		    for ( ; i < addlen; ++i)		/* <addstring> */
			cbuf[i] = getc(fd);
		    cbuf[i] = NUL;
		}

		if (flags & ADD_KEEPCAP)
		{
		    /* <addstring> is in original case, need to get
		     * case-folded word too. */
		    (void)spell_casefold(cbuf, addlen, fbuf, MAXWLEN);
		    flen = addlen - leadlen + 1;
		    addlen = STRLEN(fbuf);
		}
		else
		    flen = 0;

		aw = (addword_T *)getroom(lp, &bl_used,
					   sizeof(addword_T) + addlen + flen);
		if (aw == NULL)
		    goto endFAIL;

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
		aw->aw_leadlen = leadlen;

		if (flags & ADD_REGION)
		    aw->aw_region = getc(fd);		/* <region> */
		else
		    aw->aw_region = REGION_ALL;

		if (ht == NULL)
		{
		    /* Using simple linked list, put it in front. */
		    aw->aw_next = fw->fw_adds;
		    fw->fw_adds = aw;
		    aw->aw_saveb = NUL;
		}
		else
		{
		    /* Put addition in hashtable.  For key we use the part up
		     * to the next end-of-word. */
		    if (leadlen == 0)
		    {
			p = aw->aw_word;
			while (*p != NUL && !spell_iswordc(p))
			    mb_ptr_adv(p);
		    }

		    if (leadlen != 0 || *p == NUL)
		    {
			/* Only non-word characters in addition, add it to the
			 * list with the special key NOWC_KEY.  Also do this
			 * when there is a leadstring, it would get too
			 * complicated. */
			hash = hash_hash(NOWC_KEY);
			hi = hash_lookup(ht, NOWC_KEY, hash);
			if (HASHITEM_EMPTY(hi))
			{
			    /* we use a dummy item as the list header */
			    naw = (addword_T *)getroom(lp, &bl_used,
					sizeof(addword_T) + STRLEN(NOWC_KEY));
			    if (naw == NULL)
				goto endFAIL;
			    STRCPY(naw->aw_word, NOWC_KEY);
			    hash_add_item(ht, hi, naw->aw_word, hash);
			    naw->aw_next = aw;
			    aw->aw_next = NULL;
			}
			else
			{
			    naw = HI2ADDWORD(hi);
			    aw->aw_next = naw->aw_next;
			    naw->aw_next = aw;
			}
			aw->aw_saveb = NUL;
		    }
		    else
		    {
			/* Truncate at next non-word character, store that
			 * byte in "aw_saveb". */
			while (*p != NUL && spell_iswordc(p))
			    mb_ptr_adv(p);
			aw->aw_saveb = *p;
			*p = NUL;
			hash = hash_hash(aw->aw_word);
			hi = hash_lookup(ht, aw->aw_word, hash);
			if (HASHITEM_EMPTY(hi))
			{
			    hash_add_item(ht, hi, aw->aw_word, hash);
			    aw->aw_next = NULL;
			}
			else
			{
			    naw = HI2ADDWORD(hi);
			    aw->aw_next = naw->aw_next;
			    naw->aw_next = aw;
			}
		    }
		}
	    }
	}
    }
    goto endOK;

endFAIL:
    lp->sl_error = TRUE;

endOK:
    if (fd != NULL)
	fclose(fd);
    hash_unlock(&lp->sl_words);
    sourcing_name = save_sourcing_name;
    sourcing_lnum = save_sourcing_lnum;
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
	region = NULL;
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
    firstcap = allcap = spell_isupper(c);

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
	    if (!spell_isupper(c))
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
    char_u	*ae_add_nw;	/* For a suffix: first non-word char in
				 * "ae_add"; for a prefix with only non-word
				 * chars: equal to "ae_add", for a prefix with
				 * word and non-word chars: first non-word
				 * char after word char.  NULL otherwise. */
    char_u	*ae_add_pw;	/* For a prefix with both word and non-word
			         * chars: first word char.  NULL otherwise. */
    char_u	ae_preword;	/* TRUE for a prefix with one word */
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
    char_u	*bw_caseword;	/* keep-case word or NULL */
    char_u	*bw_leadstring;	/* must come before bw_word or NULL */
    char_u	*bw_addstring;	/* must come after bw_word or NULL */
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

/* info for writing the spell file */
typedef struct winfo_S
{
    FILE	*wif_fd;
    basicword_T	*wif_prevbw;	/* last written basic word */
    int		wif_regionmask;	/* regions supported */
    int		wif_prefm;	/* 1 or 2 bytes used for prefix NR */
    int		wif_suffm;	/* 1 or 2 bytes used for suffix NR */
    long	wif_wcount;	/* written word count */
    long	wif_acount;	/* written addition count */
    long	wif_addmax;	/* max number of additions on one word */
    char_u	*wif_addmaxw;	/* word with max additions */
} winfo_T;


static afffile_T *spell_read_aff __ARGS((char_u *fname, vimconv_T *conv, int ascii));
static void spell_free_aff __ARGS((afffile_T *aff));
static int has_non_ascii __ARGS((char_u *s));
static int spell_read_dic __ARGS((hashtab_T *ht, char_u *fname, vimconv_T *conv, int ascii));
static int get_new_aff __ARGS((hashtab_T *oldaff, garray_T *gap, int prefix));
static void spell_free_dic __ARGS((hashtab_T *dic));
static int same_affentries __ARGS((affheader_T *ah1, affheader_T *ah2));
static void add_affhash __ARGS((hashtab_T *ht, char_u *key, int newnr));
static void clear_affhash __ARGS((hashtab_T *ht));
static void trans_affixes __ARGS((dicword_T *dw, basicword_T *bw, afffile_T *oldaff, hashtab_T *newwords));
static int build_wordlist __ARGS((hashtab_T *newwords, hashtab_T *oldwords, afffile_T *oldaff, int regionmask));
static basicword_T *get_basicword __ARGS((char_u *word, int asize));
static void combine_regions __ARGS((hashtab_T *newwords));
static int same_affixes __ARGS((basicword_T *bw, basicword_T *nbw));
static int expand_affixes __ARGS((hashtab_T *newwords, garray_T *prefgap, garray_T *suffgap));
static int expand_one_aff __ARGS((basicword_T *bw, garray_T *add_words, affentry_T *pae, affentry_T *sae));
static int add_to_wordlist __ARGS((hashtab_T *newwords, basicword_T *bw));
static void write_affix __ARGS((FILE *fd, affheader_T *ah));
static void write_affixlist __ARGS((FILE *fd, garray_T *aff, int bytes));
static void write_vim_spell __ARGS((char_u *fname, garray_T *prefga, garray_T *suffga, hashtab_T *newwords, int regcount, char_u *regchars, int ascii));
static void write_bword __ARGS((winfo_T *wif, basicword_T *bw, int lowcap));
static void free_wordtable __ARGS((hashtab_T *ht));
static void free_basicword __ARGS((basicword_T *bw));
static void free_affixentries __ARGS((affentry_T *first));
static void free_affix_entry __ARGS((affentry_T *ap));

/*
 * Read an affix ".aff" file.
 * Returns an afffile_T, NULL for failure.
 */
    static afffile_T *
spell_read_aff(fname, conv, ascii)
    char_u	*fname;
    vimconv_T	*conv;		/* info for encoding conversion */
    int		ascii;		/* Only accept ASCII characters */
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
    char_u	*low = NULL;
    char_u	*fol = NULL;
    char_u	*upp = NULL;

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
    while (!vim_fgets(rline, MAXLINELEN, fd) && !got_int)
    {
	line_breakcheck();
	++lnum;

	/* Skip comment lines. */
	if (*rline == '#')
	    continue;

	/* Convert from "SET" to 'encoding' when needed. */
	vim_free(pc);
	if (conv->vc_type != CONV_NONE)
	{
	    pc = string_convert(conv, rline, NULL);
	    if (pc == NULL)
	    {
		smsg((char_u *)_("Conversion failure for word in %s line %d: %s"),
							   fname, lnum, rline);
		continue;
	    }
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
		    if (aff->af_enc != NULL && !ascii
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

		if (ascii && (has_non_ascii(aff_entry->ae_chop)
					  || has_non_ascii(aff_entry->ae_add)))
		{
		    /* Don't use an affix entry with non-ASCII characters when
		     * "ascii" is TRUE. */
		    free_affix_entry(aff_entry);
		}
		else
		{
		    aff_entry->ae_next = cur_aff->ah_first;
		    cur_aff->ah_first = aff_entry;
		}
	    }
	    else if (STRCMP(items[0], "FOL") == 0 && itemcnt == 2)
	    {
		if (fol != NULL)
		    smsg((char_u *)_("Duplicate FOL in %s line %d"),
								 fname, lnum);
		else
		    fol = vim_strsave(items[1]);
	    }
	    else if (STRCMP(items[0], "LOW") == 0 && itemcnt == 2)
	    {
		if (low != NULL)
		    smsg((char_u *)_("Duplicate LOW in %s line %d"),
								 fname, lnum);
		else
		    low = vim_strsave(items[1]);
	    }
	    else if (STRCMP(items[0], "UPP") == 0 && itemcnt == 2)
	    {
		if (upp != NULL)
		    smsg((char_u *)_("Duplicate UPP in %s line %d"),
								 fname, lnum);
		else
		    upp = vim_strsave(items[1]);
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

    if (fol != NULL || low != NULL || upp != NULL)
    {
	/* Don't write a word table for an ASCII file, so that we don't check
	 * for conflicts with a word table that matches 'encoding'. */
	if (!ascii)
	{
	    if (fol == NULL || low == NULL || upp == NULL)
		smsg((char_u *)_("Missing FOL/LOW/UPP line in %s"), fname);
	    else
		set_spell_chartab(fol, low, upp);
	}

	vim_free(fol);
	vim_free(low);
	vim_free(upp);
    }

    vim_free(pc);
    fclose(fd);
    return aff;
}

/*
 * Return TRUE if string "s" contains a non-ASCII character (128 or higher).
 * When "s" is NULL FALSE is returned.
 */
    static int
has_non_ascii(s)
    char_u	*s;
{
    char_u	*p;

    if (s != NULL)
	for (p = s; *p != NUL; ++p)
	    if (*p >= 128)
		return TRUE;
    return FALSE;
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
spell_read_dic(ht, fname, conv, ascii)
    hashtab_T	*ht;
    char_u	*fname;
    vimconv_T	*conv;		/* info for encoding conversion */
    int		ascii;		/* only accept ASCII words */
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
    while (!vim_fgets(line, MAXLINELEN, fd) && !got_int)
    {
	line_breakcheck();
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

	/* Skip non-ASCII words when "ascii" is TRUE. */
	if (ascii && has_non_ascii(line))
	    continue;

	/* Convert from "SET" to 'encoding' when needed. */
	if (conv->vc_type != CONV_NONE)
	{
	    pc = string_convert(conv, line, NULL);
	    if (pc == NULL)
	    {
		smsg((char_u *)_("Conversion failure for word in %s line %d: %s"),
						       fname, lnum, line);
		continue;
	    }
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
	{
	    vim_free(pc);
	    break;
	}
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
get_new_aff(oldaff, gap, prefix)
    hashtab_T	*oldaff;	/* hashtable with affheader_T */
    garray_T	*gap;		/* table with new affixes */
    int		prefix;		/* TRUE when doing prefixes, FALSE for
				   suffixes */
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
    int		preword;

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
		preword = FALSE;
		oldae->ae_add_nw = NULL;
		oldae->ae_add_pw = NULL;
		if (oldae->ae_add != NULL)
		{
		    /* Check for non-word characters in the affix.  If there
		     * is one a suffix will be turned into an addition, a
		     * prefix may be turned into a leadstring.
		     * This is stored with the old affix, that is where
		     * trans_affixes() will check. */
		    for (p = oldae->ae_add; *p != NUL; mb_ptr_adv(p))
			if (!spell_iswordc(p))
			{
			    oldae->ae_add_nw = p;
			    break;
			}

		    if (prefix && oldae->ae_add_nw != NULL)
		    {
			/* If a prefix has non-word characters special
			 * treatment is necessary:
			 * - If it has only non-word characters it becomes a
			 *   leadstring.
			 * - If it has a sequence of word characters followed
			 *   by a non-word char it becomes a "preword": "d'",
			 *   "de-", "d'ai", etc.
			 * - if it has another mix of word and non-word
			 *   characters the part before the last word char
			 *   becomes a leadstring: "'d", etc.
			 */
			for (p = oldae->ae_add; *p != NUL; mb_ptr_adv(p))
			    if (spell_iswordc(p))
			    {
				oldae->ae_add_pw = p;
				break;
			    }
			if (oldae->ae_add_pw != NULL)
			{
			    /* Mixed prefix, set ae_add_nw to first non-word
			     * char after ae_add_pw (if there is one). */
			    oldae->ae_add_nw = NULL;
			    for ( ; *p != NUL; mb_ptr_adv(p))
				if (!spell_iswordc(p))
				{
				    oldae->ae_add_nw = p;
				    break;
				}
			    if (oldae->ae_add_nw != NULL)
			    {
				preword = TRUE;
				oldae->ae_add_pw = NULL;
				oldae->ae_add_nw = NULL;
			    }
			}
		    }
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
		    newae->ae_preword = preword;

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
		    if (newah->ah_combine == gapah->ah_combine
					     && same_affentries(newah, gapah))
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
    garray_T	*gap, *agap;
    hashitem_T	*aff_hi;
    affheader_T	*ah;
    affentry_T	*ae;
    regmatch_T	regmatch;
    int		i;
    basicword_T *nbw;
    int		alen;
    garray_T	suffixga;	/* list of words with non-word suffixes */
    garray_T	prefixga;	/* list of words with non-word prefixes */
    char_u	nword[MAXWLEN];
    int		flags;
    int		n;

    ga_init2(&suffixga, (int)sizeof(basicword_T *), 5);
    ga_init2(&prefixga, (int)sizeof(basicword_T *), 5);

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
	    /* Setup for regexp matching.  Note that we don't ignore case.
	     * This is weird, because the rules in an .aff file don't care
	     * about case, but it's necessary for compatibility with Myspell.
	     */
	    regmatch.regprog = ae->ae_prog;
	    regmatch.rm_ic = FALSE;
	    if (ae->ae_prog == NULL
			   || vim_regexec(&regmatch, dw->dw_word, (colnr_T)0))
	    {
		if ((ae->ae_add_nw != NULL || ae->ae_add_pw != NULL)
			&& (gap != &bw->bw_suffix || bw->bw_addstring == NULL))
		{
		    /*
		     * Affix has a non-word character and isn't prepended to
		     * leader or appended to addition.  Need to use another
		     * word with a leadstring and/or addstring.
		     */
		    if (gap == &bw->bw_suffix || ae->ae_add_nw == NULL)
		    {
			/* Suffix or prefix with only non-word chars.
			 * Build the new basic word in "nword": Remove chop
			 * string and append/prepend addition. */
			if (gap == &bw->bw_suffix)
			{
			    /* suffix goes at the end of the word */
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
			    agap = &suffixga;
			}
			else
			{
			    /* prefix goes before the word */
			    STRCPY(nword, ae->ae_add);
			    p = dw->dw_word;
			    if (ae->ae_chop != NULL)
				/* Skip chop string. */
				for (i = mb_charlen(ae->ae_chop); i > 0; --i)
				    mb_ptr_adv( p);
			    STRCAT(nword, p);
			    agap = &prefixga;
			}

			/* Create a basicword_T from the word. */
			nbw = get_basicword(nword, 1);
			if (nbw != NULL)
			{
			    nbw->bw_region = bw->bw_region;
			    nbw->bw_flags |= bw->bw_flags
				   & ~(BWF_ONECAP | BWF_ALLCAP | BWF_KEEPCAP);

			    if (STRCMP(bw->bw_word, nbw->bw_word) != 0)
				/* Basic word differs, add new word entry. */
				(void)add_to_wordlist(newwords, nbw);
			    else
			    {
				/* Basic word is the same, link "nbw" after
				 * "bw". */
				nbw->bw_next = bw->bw_next;
				bw->bw_next = nbw;
			    }

			    /* Remember this word, we need to set bw_prefix
			     * or bw_suffix later. */
			    if (ga_grow(agap, 1) == OK)
				((basicword_T **)agap->ga_data)
						       [agap->ga_len++] = nbw;
			}
		    }
		    else
		    {
			/* Prefix with both non-word and word characters: Turn
			 * prefix into basic word, original word becomes an
			 * addstring. */

			/* Fold-case the word characters in the prefix into
			 * nword[]. */
			alen = 0;
			for (p = ae->ae_add_pw; p < ae->ae_add_nw; p += n)
			{
#ifdef FEAT_MBYTE
			    n = (*mb_ptr2len_check)(p);
#else
			    n = 1;
#endif
			    (void)spell_casefold(p, n, nword + alen,
							      MAXWLEN - alen);
			    alen += STRLEN(nword + alen);
			}

			/* Allocate a new word entry. */
			nbw = (basicword_T *)alloc((unsigned)(
					     sizeof(basicword_T) + alen + 1));
			if (nbw != NULL)
			{
			    *nbw = *bw;
			    ga_init2(&nbw->bw_prefix, sizeof(short_u), 1);
			    ga_init2(&nbw->bw_suffix, sizeof(short_u), 1);

			    mch_memmove(nbw->bw_word, nword, alen);
			    nbw->bw_word[alen] = NUL;

			    /* Use the cap type of the prefix. */
			    alen = ae->ae_add_nw - ae->ae_add_pw;
			    mch_memmove(nword, ae->ae_add_pw, alen);
			    nword[alen] = NUL;
			    flags = captype(nword, nword + STRLEN(nword));
			    if (flags & BWF_KEEPCAP)
				nbw->bw_caseword = vim_strsave(nword);
			    else
				nbw->bw_caseword = NULL;
			    nbw->bw_flags &= ~(BWF_ONECAP | BWF_ALLCAP
							       | BWF_KEEPCAP);
			    nbw->bw_flags |= flags;

			    /* The addstring is the prefix after the word
			     * characters, the original word excluding "chop",
			     * plus any addition. */
			    STRCPY(nword, ae->ae_add_nw);
			    p = bw->bw_word;
			    if (ae->ae_chop != NULL)
				p += STRLEN(ae->ae_chop);
			    STRCAT(nword, p);
			    if (bw->bw_addstring != NULL)
				STRCAT(nword, bw->bw_addstring);
			    nbw->bw_addstring = vim_strsave(nword);

			    if (ae->ae_add_pw > ae->ae_add)
				nbw->bw_leadstring = vim_strnsave(ae->ae_add,
						  ae->ae_add_pw - ae->ae_add);
			    else
				nbw->bw_leadstring = NULL;

			    (void)add_to_wordlist(newwords, nbw);

			    /* Remember this word, we need to set bw_suffix
			     * and bw_suffix later. */
			    if (ga_grow(&prefixga, 1) == OK)
				((basicword_T **)prefixga.ga_data)
						    [prefixga.ga_len++] = nbw;
			}
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
     */
    for (i = 0; i < suffixga.ga_len; ++i)
    {
	nbw = ((basicword_T **)suffixga.ga_data)[i];
	if (ga_grow(&nbw->bw_prefix, bw->bw_prefix.ga_len) == OK)
	{
	    mch_memmove(nbw->bw_prefix.ga_data, bw->bw_prefix.ga_data,
				      bw->bw_prefix.ga_len * sizeof(short_u));
	    nbw->bw_prefix.ga_len = bw->bw_prefix.ga_len;
	}
    }

    /*
     * For the words that we added for prefixes with non-word characters: Use
     * the suffix list of the main word.
     */
    for (i = 0; i < prefixga.ga_len; ++i)
    {
	nbw = ((basicword_T **)prefixga.ga_data)[i];
	if (ga_grow(&nbw->bw_suffix, bw->bw_suffix.ga_len) == OK)
	{
	    mch_memmove(nbw->bw_suffix.ga_data, bw->bw_suffix.ga_data,
				      bw->bw_suffix.ga_len * sizeof(short_u));
	    nbw->bw_suffix.ga_len = bw->bw_suffix.ga_len;
	}
    }

    ga_clear(&suffixga);
    ga_clear(&prefixga);
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
    char_u	message[MAXLINELEN + MAXWLEN];

    todo = oldwords->ht_used;
    for (old_hi = oldwords->ht_array; todo > 0; ++old_hi)
    {
	if (!HASHITEM_EMPTY(old_hi))
	{
	    --todo;
	    dw = HI2DW(old_hi);

	    /* This takes time, print a message now and then. */
	    if ((todo & 0x3ff) == 0 || todo == (int)oldwords->ht_used - 1)
	    {
		sprintf((char *)message, _("%6d todo - %s"),
							   todo, dw->dw_word);
		msg_start();
		msg_outtrans_attr(message, 0);
		msg_clr_eos();
		msg_didout = FALSE;
		msg_col = 0;
		out_flush();
		ui_breakcheck();
		if (got_int)
		    break;
	    }

	    bw = get_basicword(dw->dw_word, 10);
	    if (bw == NULL)
		break;
	    bw->bw_region = regionmask;

	    (void)add_to_wordlist(newwords, bw);

	    /* Deal with any affix names on the old word, translate them
	     * into affix numbers. */
	    if (dw->dw_affnm != NULL)
		trans_affixes(dw, bw, oldaff, newwords);
	}
    }
    if (todo > 0)
	return FAIL;
    return OK;
}

/*
 * Get a basicword_T from a word in original case.
 * Caller must set bw_region.
 * Returns NULL when something fails.
 */
    static basicword_T *
get_basicword(word, asize)
    char_u	*word;
    int		asize;	    /* growsize for affix garray */
{
    int		dwlen;
    char_u	foldword[MAXLINELEN];
    int		flags;
    int		clen;
    int		leadlen;
    char_u	*p;
    char_u	leadstring[MAXLINELEN];
    int		addlen;
    char_u	addstring[MAXLINELEN];
    char_u	*cp = NULL;
    int		l;
    basicword_T *bw;

    /* The basic words are always stored with folded case. */
    dwlen = STRLEN(word);
    (void)spell_casefold(word, dwlen, foldword, MAXLINELEN);
    flags = captype(word, word + dwlen);

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
	    for (cp = word; l < clen; mb_ptr_adv(cp))
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
	return NULL;

    STRCPY(bw->bw_word, foldword);

    if (leadlen > 0)
	bw->bw_leadstring = vim_strsave(leadstring);
    else
	bw->bw_leadstring = NULL;
    if (addlen > 0)
	bw->bw_addstring = vim_strsave(addstring);
    else
	bw->bw_addstring = NULL;

    if (flags & BWF_KEEPCAP)
    {
	if (addlen == 0)
	    /* use the whole word */
	    bw->bw_caseword = vim_strsave(word + leadlen);
	else
	    /* use only up to the addition */
	    bw->bw_caseword = vim_strnsave(word + leadlen,
							 cp - word - leadlen);
    }

    bw->bw_flags = flags;
    ga_init2(&bw->bw_prefix, sizeof(short_u), asize);
    ga_init2(&bw->bw_suffix, sizeof(short_u), asize);

    return bw;
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
				|| bw->bw_caseword == NULL
				|| nbw->bw_caseword == NULL
				|| STRCMP(bw->bw_caseword,
						      nbw->bw_caseword) == 0)
			    && (bw->bw_leadstring == NULL
				|| STRCMP(bw->bw_leadstring,
						    nbw->bw_leadstring) == 0)
			    && (bw->bw_addstring == NULL
				|| STRCMP(bw->bw_addstring,
						     nbw->bw_addstring) == 0)
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
 * For each basic word with additions turn the suffixes into other additions
 * and/or new basic words.  For each basic word with a leadstring turn the
 * prefixes into other leadstrings and/or new basic words.
 * The result is that no affixes apply to the additions or leadstring of a
 * word.
 * This is also needed when a word with an addition has a prefix and the word
 * with prefix also exists.  E.g., "blurp's/D" (D is prefix "de") and
 * "deblurp".  "deblurp" would match and no prefix would be tried.
 *
 * Returns FAIL when out of memory.
 */
    static int
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
    char_u	message[MAXLINELEN + MAXWLEN];
    int		retval = OK;

    ga_init2(&add_words, sizeof(basicword_T *), 10);

    todo = newwords->ht_used;
    for (hi = newwords->ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;

	    /* This takes time, print a message now and then. */
	    if ((todo & 0x3ff) == 0 || todo == (int)newwords->ht_used - 1)
	    {
		sprintf((char *)message, _("%6d todo - %s"),
						    todo, HI2BW(hi)->bw_word);
		msg_start();
		msg_outtrans_attr(message, 0);
		msg_clr_eos();
		msg_didout = FALSE;
		msg_col = 0;
		out_flush();
		ui_breakcheck();
		if (got_int)
		    break;
	    }

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
			  /* Skip prewords, they don't need to be expanded. */
			  if (pae == NULL || !pae->ae_preword)
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
				    if (expand_one_aff(bw, &add_words,
							    pae, sae) == FAIL)
				    {
					retval = FAIL;
					goto theend;
				    }

				    /* Advance to next suffix entry, if there
				     * is one. */
				    if (sae != NULL)
					sae = sae->ae_next;
				} while (sae != NULL);
			    }
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
    {
	retval = add_to_wordlist(newwords,
				     ((basicword_T **)add_words.ga_data)[pi]);
	if (retval == FAIL)
	    break;
    }

theend:
    ga_clear(&add_words);
    return retval;
}

/*
 * Add one word to "add_words" for basic word "bw" with additions, adding
 * prefix "pae" and suffix "sae".  Either "pae" or "sae" can be NULL.
 * Don't do this when not necessary:
 * - no leadstring and adding prefix doesn't result in existing word.
 * Returns FAIL when out of memory.
 */
    static int
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
    if ((bw->bw_flags & BWF_KEEPCAP) && bw->bw_caseword != NULL)
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
    if (nbw == NULL)
	return FAIL;

    /* Add the new word to the list of words to be added later. */
    if (ga_grow(add_words, 1) == FAIL)
    {
	vim_free(nbw);
	return FAIL;
    }
    ((basicword_T **)add_words->ga_data)[add_words->ga_len++] = nbw;

    /* Copy the (modified) basic word, flags and region. */
    STRCPY(nbw->bw_word, word);
    nbw->bw_flags = bw->bw_flags;
    nbw->bw_region = bw->bw_region;

    /* Set the (modified) caseword. */
    if (bw->bw_flags & BWF_KEEPCAP)
	nbw->bw_caseword = vim_strsave(caseword);
    else
	nbw->bw_caseword = NULL;

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
    else if (bw->bw_prefix.ga_len > 0)
    {
	/* There is no leadstring, copy the list of possible prefixes. */
	ga_init2(&nbw->bw_prefix, sizeof(short_u), 1);
	if (ga_grow(&nbw->bw_prefix, bw->bw_prefix.ga_len) == OK)
	{
	    mch_memmove(nbw->bw_prefix.ga_data, bw->bw_prefix.ga_data,
				  bw->bw_prefix.ga_len * sizeof(short_u));
	    nbw->bw_prefix.ga_len = bw->bw_prefix.ga_len;
	}
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

    return OK;
}

/*
 * Add basicword_T "*bw" to wordlist "newwords".
 */
    static int
add_to_wordlist(newwords, bw)
    hashtab_T	*newwords;
    basicword_T	*bw;
{
    hashitem_T	*hi;
    basicword_T *bw2;
    int		retval = OK;

    hi = hash_find(newwords, bw->bw_word);
    if (HASHITEM_EMPTY(hi))
    {
	/* New entry, add to hashlist. */
	retval = hash_add(newwords, bw->bw_word);
	bw->bw_next = NULL;
    }
    else
    {
	/* Existing entry, append to list of basic words. */
	bw2 = HI2BW(hi);
	bw->bw_next = bw2->bw_next;
	bw2->bw_next = bw;
    }
    return retval;
}

/*
 * Write a number to file "fd", MSB first, in "len" bytes.
 */
    void
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
 * Write affix info.  <affitemcnt> <affitem> ...
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
    int		flags;

    /* Count the number of entries. */
    for (ae = ah->ah_first; ae != NULL; ae = ae->ae_next)
	++i;
    put_bytes(fd, (long_u)i, 2);		/* <affitemcnt> */

    /* <affitem>: <affflags> <affchoplen> <affchop> <affaddlen> <affadd> */
    for (ae = ah->ah_first; ae != NULL; ae = ae->ae_next)
    {
	flags = ah->ah_combine ? AFF_COMBINE : 0;
	if (ae->ae_preword)
	    flags |= AFF_PREWORD;
	fputc(flags, fd);			/* <affflags> */

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
 *		 <charflagslen> <charflags> <fcharslen> <fchars>
 *
 * <fileID>     10 bytes    "VIMspell04"
 * <regioncnt>  1 byte	    number of regions following (8 supported)
 * <regionname>	2 bytes     Region name: ca, au, etc.
 *			    First <regionname> is region 1.
 *
 * <charflagslen> 1 byte    Number of bytes in <charflags> (should be 128).
 * <charflags>  N bytes     List of flags (first one is for character 128):
 *			    0x01  word character
 *			    0x01  upper-case character
 * <fcharslen>  2 bytes     Number of bytes in <fchars>.
 * <fchars>     N bytes	    Folded characters, first one is for character 128.
 *
 *
 * <PREFIXLIST>: <affcount> <affix> ...
 * <SUFFIXLIST>: <affcount> <affix> ...
 *		list of possible affixes: prefixes and suffixes.
 *
 * <affcount>	2 bytes	    Number of affixes (MSB comes first).
 *                          When more than 256 an affixNR is 2 bytes.
 *                          This is separate for prefixes and suffixes!
 *                          First affixNR is 0.
 *
 * <affix>: <affitemcnt> <affitem> ...
 *
 * <affitemcnt> 2 bytes	    Number of affixes with this affixNR (MSB first).
 *
 * <affitem>: <affflags> <affchoplen> <affchop> <affaddlen> <affadd>
 *
 * <affflags>	1 byte	    0x01: prefix combines with suffix, AFF_COMBINE
 *			    0x02: prefix includes word, AFF_PREWORD
 *			    0x04-0x80: unset
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
 * <flags>	1 byte	    0x01: word is valid without addition, BWF_VALID
 *			    0x02: has region byte, BWF_REGION
 *			    0x04: first letter must be upper-case, BWF_ONECAP
 *			    0x08: has suffixes, <affixcnt> and <affixNR> follow
 *					BWF_SUFFIX
 *			    0x10: more flags, <flags2> follows next, BWF_SECOND
 *			    0x20-0x80: can't be used, unset
 * <flags2>	1 byte	    0x01: has additions, <addcnt> and <add> follow,
 *					BWF_ADDS
 *			    0x02: has prefixes, <affixcnt> and <affixNR> follow
 *					BWF_PREFIX
 *			    0x04: all letters must be upper-case, BWF_ALLCAP
 *			    0x08: case must match, BWF_KEEPCAP
 *			    0x10: has more than 255 additions, <addcnt> is two
 *				  bytes, BWF_ADDS_M
 *			    0x10-0x80: unset
 * <caselen>	1 byte	    Length of <caseword>.
 * <caseword>	N bytes	    Word with matching case.
 * <affixcnt>	1 byte	    Number of affix NRs following.
 * <affixNR>	1 or 2 byte Number of possible affix for this word.
 *			    When using 2 bytes MSB comes first.
 * <region>	1 byte	    Bitmask for regions in which word is valid.  When
 *			    omitted it's valid in all regions.
 *			    Lowest bit is for region 1.
 * <addcnt>	1 or 2 byte Number of <add> items following.
 *
 * <add>: <addflags> <addlen> [<leadlen>] [<copylen>] [<addstring>] [<region>]
 *
 * <addflags>	1 byte	    0x01: unset
 *			    0x02: has region byte, ADD_REGION
 *			    0x04: first letter must be upper-case, ADD_ONECAP
 *			    0x08: unset
 *			    0x10: has a <leadlen>, ADD_LEADLEN
 *			    0x20: has a <copylen>, ADD_COPYLEN
 *			    0x40: all letters must be upper-case, ADD_ALLCAP
 *			    0x80: fixed case, <addstring> is the whole word
 *				  with matching case, ADD_KEEPCAP.
 * <addlen>	1 byte	    Length of <addstring> in bytes.
 * <leadlen>	1 byte	    Number of bytes at start of <addstring> that must
 *			    come before the start of the basic word.
 * <copylen>	1 byte	    Number of bytes copied from previous <addstring>.
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
write_vim_spell(fname, prefga, suffga, newwords, regcount, regchars, ascii)
    char_u	*fname;
    garray_T	*prefga;	/* prefixes, affheader_T entries */
    garray_T	*suffga;	/* suffixes, affheader_T entries */
    hashtab_T	*newwords;	/* basic words, basicword_T entries */
    int		regcount;	/* number of regions */
    char_u	*regchars;	/* region names */
    int		ascii;		/* TRUE for ascii spell file */
{
    winfo_T	wif;
    garray_T	*gap;
    hashitem_T	*hi;
    char_u	**wtab;
    int		todo;
    int		flags, aflags;
    basicword_T	*bw, *bwf, *bw2 = NULL;
    int		i;
    int		round;
    garray_T	bwga;

    vim_memset(&wif, 0, sizeof(winfo_T));

    wif.wif_fd = fopen((char *)fname, "w");
    if (wif.wif_fd == NULL)
    {
	EMSG2(_(e_notopen), fname);
	return;
    }

    /* <HEADER>: <fileID> <regioncnt> <regionname> ...
     *		 <charflagslen> <charflags> <fcharslen> <fchars> */
    fwrite(VIMSPELLMAGIC, VIMSPELLMAGICL, (size_t)1, wif.wif_fd); /* <fileID> */

    /* write the region names if there is more than one */
    if (regcount > 1)
    {
	putc(regcount, wif.wif_fd);	    /* <regioncnt> <regionname> ... */
	fwrite(regchars, (size_t)(regcount * 2), (size_t)1, wif.wif_fd);
	wif.wif_regionmask = (1 << regcount) - 1;
    }
    else
    {
	putc(0, wif.wif_fd);
	wif.wif_regionmask = 0;
    }

    /* Write the table with character flags and table for case folding.
     * <charflagslen> <charflags>  <fcharlen> <fchars>
     * Skip this for ASCII, the table may conflict with the one used for
     * 'encoding'. */
    if (ascii)
    {
	putc(0, wif.wif_fd);
	putc(0, wif.wif_fd);
	putc(0, wif.wif_fd);
    }
    else
	write_spell_chartab(wif.wif_fd);

    /* <PREFIXLIST>: <affcount> <affix> ...
     * <SUFFIXLIST>: <affcount> <affix> ... */
    for (round = 1; round <= 2; ++round)
    {
	gap = round == 1 ? prefga : suffga;
	put_bytes(wif.wif_fd, (long_u)gap->ga_len, 2);	    /* <affcount> */

	for (i = 0; i < gap->ga_len; ++i)
	    write_affix(wif.wif_fd, (affheader_T *)gap->ga_data + i);
    }

    /* Number of bytes used for affix NR depends on affix count. */
    wif.wif_prefm = (prefga->ga_len > 256) ? 2 : 1;
    wif.wif_suffm = (suffga->ga_len > 256) ? 2 : 1;

    /* <SUGGEST> : <suggestlen> <more> ...
     *  TODO.  Only write a zero length for now. */
    put_bytes(wif.wif_fd, 0L, 4);			    /* <suggestlen> */

    /*
     * <WORDLIST>: <wordcount> <worditem> ...
     */

    /* number of basic words in 4 bytes */
    put_bytes(wif.wif_fd, newwords->ht_used, 4);	    /* <wordcount> */

    /*
     * Sort the word list, so that we can copy as many bytes as possible from
     * the previous word.
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
	for (todo = 0; (long_u)todo < newwords->ht_used; ++todo)
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
				|| bw->bw_caseword == NULL
				|| bw2->bw_caseword == NULL
				|| STRCMP(bw->bw_caseword,
						       bw2->bw_caseword) == 0)
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
		write_bword(&wif, bw, TRUE);
	    write_bword(&wif, bw, FALSE);

	    /* Write other basic words, with different caps. */
	    for (i = 0; i < bwga.ga_len; ++i)
	    {
		bw2 = ((basicword_T **)bwga.ga_data)[i];
		if (bw2 != bw)
		    write_bword(&wif, bw2, FALSE);
	    }
	}

	ga_clear(&bwga);
	vim_free(wtab);
    }

    fclose(wif.wif_fd);

    /* Print a few statistics. */
    if (wif.wif_addmaxw == NULL)
	wif.wif_addmaxw = (char_u *)"";
    smsg((char_u *)_("Maximum number of adds on a word: %ld (%s)"),
					     wif.wif_addmax, wif.wif_addmaxw);
    smsg((char_u *)_("Average number of adds on a word: %f"),
			       (float)wif.wif_acount / (float)wif.wif_wcount);
}

/*
 * Compare two basic words for their <addstring>.
 */
static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
bw_compare __ARGS((const void *s1, const void *s2));

    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
bw_compare(s1, s2)
    const void	*s1;
    const void	*s2;
{
    basicword_T *bw1 = *(basicword_T **)s1;
    basicword_T *bw2 = *(basicword_T **)s2;
    int		i = 0;

    /* compare the leadstrings */
    if (bw1->bw_leadstring == NULL)
    {
	if (bw2->bw_leadstring != NULL)
	    return 1;
    }
    else if (bw2->bw_leadstring == NULL)
	return -1;
    else
	i = STRCMP(bw1->bw_leadstring, bw2->bw_leadstring);

    if (i == 0)
    {
	/* leadstrings are identical, compare the addstrings */
	if (bw1->bw_addstring == NULL)
	{
	    if (bw2->bw_addstring != NULL)
		return 1;
	}
	else if (bw2->bw_addstring == NULL)
	    return -1;
	else
	    i = STRCMP(bw1->bw_addstring, bw2->bw_addstring);
    }
    return i;
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
write_bword(wif, bwf, lowcap)
    winfo_T	*wif;		/* info for writing */
    basicword_T	*bwf;
    int		lowcap;		/* write KEEPKAP word as not-valid */
{
    FILE	*fd = wif->wif_fd;
    int		flags;
    int		aflags;
    int		len;
    int		leadlen, addlen;
    int		copylen;
    int		clen;
    int		adds = 0;
    int		i;
    int		idx;
    basicword_T *bw, *bw2;
    basicword_T **wtab;
    int		count;
    int		l;

    /* Check how many bytes can be copied from the previous word. */
    len = STRLEN(bwf->bw_word);
    if (wif->wif_prevbw == NULL)
	clen = 0;
    else
	for (clen = 0; clen < len
		&& wif->wif_prevbw->bw_word[clen] == bwf->bw_word[clen]; ++clen)
	    ;
    putc(clen, fd);				/* <nr> */
    wif->wif_prevbw = bwf;
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

	/* Flags: add the region byte if the word isn't valid in all
	 * regions. */
	if (wif->wif_regionmask != 0 && (bw->bw_region & wif->wif_regionmask)
						       != wif->wif_regionmask)
	    flags |= BWF_REGION;
    }
    /* Add the prefix/suffix list if there are prefixes/suffixes. */
    if (bw->bw_leadstring == NULL && bw->bw_prefix.ga_len > 0)
	flags |= BWF_PREFIX;
    if (bw->bw_addstring == NULL && bw->bw_suffix.ga_len > 0)
	flags |= BWF_SUFFIX;

    /* Flags: may have additions. */
    if (adds > 0)
    {
	flags |= BWF_ADDS;
	if (adds >= 256)
	    flags |= BWF_ADDS_M;
    }

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

    if ((flags & BWF_KEEPCAP) && bw->bw_caseword != NULL)
    {
	len = STRLEN(bw->bw_caseword);
	putc(len, fd);			/* <caselen> */
	for (i = 0; i < len; ++i)
	    putc(bw->bw_caseword[i], fd);	/* <caseword> */
    }

    /* write prefix and suffix lists: <affixcnt> <affixNR> ... */
    if (flags & BWF_PREFIX)
	write_affixlist(fd, &bw->bw_prefix, wif->wif_prefm);
    if (flags & BWF_SUFFIX)
	write_affixlist(fd, &bw->bw_suffix, wif->wif_suffm);

    if (flags & BWF_REGION)
	putc(bw->bw_region, fd);		/* <region> */

    ++wif->wif_wcount;

    /*
     * Additions.
     */
    if (adds > 0)
    {
	if (adds >= 256)
	    put_bytes(fd, (long_u)adds, 2);	/* 2 byte <addcnt> */
	else
	    putc(adds, fd);			/* 1 byte <addcnt> */

	/* statistics */
	wif->wif_acount += adds;
	if (wif->wif_addmax < adds)
	{
	    wif->wif_addmax = adds;
	    wif->wif_addmaxw = bw->bw_word;
	}

	/*
	 * Sort the list of additions, so that we can copy as many bytes as
	 * possible from the previous addstring.
	 */

	/* Make a table with pointers to each basic word that has additions. */
	wtab = (basicword_T **)alloc((unsigned)(sizeof(basicword_T *) * adds));
	if (wtab == NULL)
	    return;
	count = 0;
	for (bw = bwf; bw != NULL; bw = bw->bw_cnext)
	    if (bw->bw_leadstring != NULL || bw->bw_addstring != NULL)
		wtab[count++] = bw;

	/* Sort. */
	qsort((void *)wtab, (size_t)count, sizeof(basicword_T *), bw_compare);

	/* Now write each basic word to the spell file.  Copy bytes from the
	 * previous leadstring/addstring if possible. */
	bw2 = NULL;
	for (idx = 0; idx < count; ++idx)
	{
	    bw = wtab[idx];

	    /* <add>: <addflags> <addlen> [<leadlen>] [<copylen>]
	     *				[<addstring>] [<region>] */
	    copylen = 0;
	    if (bw->bw_leadstring == NULL)
		leadlen = 0;
	    else
	    {
		leadlen = STRLEN(bw->bw_leadstring);
		if (bw2 != NULL && bw2->bw_leadstring != NULL)
		    for ( ; copylen < leadlen; ++copylen)
			if (bw->bw_leadstring[copylen]
					   != bw2->bw_leadstring[copylen])
			    break;
	    }
	    if (bw->bw_addstring == NULL)
		addlen = 0;
	    else
	    {
		addlen = STRLEN(bw->bw_addstring);
		if (bw2 != NULL && copylen == leadlen
					     && bw2->bw_addstring != NULL)
		{
		    for (i = 0; i < addlen; ++i)
			if (bw->bw_addstring[i] != bw2->bw_addstring[i])
			    break;
		    copylen += i;
		}
	    }

	    aflags = 0;
	    /* Only copy bytes when it's more than one, the length itself
	     * takes an extra byte. */
	    if (copylen > 1)
		aflags |= ADD_COPYLEN;
	    else
		copylen = 0;

	    if (bw->bw_flags & BWF_ONECAP)
		aflags |= ADD_ONECAP;
	    if (bw->bw_flags & BWF_ALLCAP)
		aflags |= ADD_ALLCAP;
	    if (bw->bw_flags & BWF_KEEPCAP)
		aflags |= ADD_KEEPCAP;
	    if (wif->wif_regionmask != 0 && (bw->bw_region
			    & wif->wif_regionmask) != wif->wif_regionmask)
		aflags |= ADD_REGION;
	    if (leadlen > 0)
		aflags |= ADD_LEADLEN;
	    putc(aflags, fd);		    /* <addflags> */

	    putc(leadlen + addlen, fd);			/* <addlen> */
	    if (aflags & ADD_LEADLEN)
		putc(leadlen, fd);			/* <leadlen> */
	    if (aflags & ADD_COPYLEN)
		putc(copylen, fd);			/* <copylen> */

							/* <addstring> */
	    if (leadlen > copylen && bw->bw_leadstring != NULL)
		fwrite(bw->bw_leadstring + copylen,
				  (size_t)(leadlen - copylen), (size_t)1, fd);
	    if (leadlen + addlen > copylen && bw->bw_addstring != NULL)
	    {
		if (copylen >= leadlen)
		    l = copylen - leadlen;
		else
		    l = 0;
		fwrite(bw->bw_addstring + l,
					 (size_t)(addlen - l), (size_t)1, fd);
	    }

	    if (aflags & ADD_REGION)
		putc(bw->bw_region, fd);		/* <region> */

	    bw2 = bw;
	}

	vim_free(wtab);
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
    int		ascii = FALSE;
    char_u	*arg = eap->arg;
    int		error = FALSE;

    if (STRNCMP(arg, "-ascii", 6) == 0)
    {
	ascii = TRUE;
	arg = skipwhite(arg + 6);
    }

    /* Expand all the remaining arguments (e.g., $VIMRUNTIME). */
    if (get_arglist_exp(arg, &fcount, &fnames) == FAIL)
	return;
    if (fcount < 2)
	EMSG(_(e_invarg));	/* need at least output and input names */
    else if (fcount > 9)
	EMSG(_("E754: Only up to 8 regions supported"));
    else
    {
	/* Check for overwriting before doing things that may take a lot of
	 * time. */
	sprintf((char *)wfname, "%s.%s.spl", fnames[0],
					   ascii ? (char_u *)"ascii" : p_enc);
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

	/* Clear the char type tables, don't want to use any of the currently
	 * used spell properties. */
	init_spell_chartab();

	/*
	 * Read all the .aff and .dic files.
	 * Text is converted to 'encoding'.
	 */
	for (i = 1; i < fcount; ++i)
	{
	    /* Read the .aff file.  Will init "conv" based on the "SET" line. */
	    conv.vc_type = CONV_NONE;
	    sprintf((char *)fname, "%s.aff", fnames[i]);
	    if ((afile[i - 1] = spell_read_aff(fname, &conv, ascii)) == NULL)
		break;

	    /* Read the .dic file. */
	    sprintf((char *)fname, "%s.dic", fnames[i]);
	    if (spell_read_dic(&dfile[i - 1], fname, &conv, ascii) == FAIL)
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
					   : &afile[i - 1]->af_suff,
					   gap, round == 1);
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
	    error = expand_affixes(&newwords, &prefga, &suffga) == FAIL;

	    if (!error)
	    {
		/* Write the info in the spell file. */
		smsg((char_u *)_("Writing spell file %s..."), wfname);
		out_flush();
		write_vim_spell(wfname, &prefga, &suffga, &newwords,
					      fcount - 1, region_name, ascii);
		MSG(_("Done!"));
		out_flush();
	    }

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
 * Free a list of affentry_T and what they contain.
 */
    static void
free_affixentries(first)
    affentry_T	*first;
{
    affentry_T	*ap, *an;

    for (ap = first; ap != NULL; ap = an)
    {
	an = ap->ae_next;
	free_affix_entry(ap);
    }
}

/*
 * Free one affentry_T and what it contains.
 */
    static void
free_affix_entry(ap)
    affentry_T *ap;
{
    vim_free(ap->ae_chop);
    vim_free(ap->ae_add);
    vim_free(ap->ae_cond);
    vim_free(ap->ae_prog);
    vim_free(ap);
}

#endif  /* FEAT_MBYTE */

#endif  /* FEAT_SYN_HL */
