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
 * avoids the need to allocate each individual word and copying it.  It's
 * allocated in big chunks for speed.
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
    char_u	dw_flags;	/* WF_ flags */
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
    char_u	nw_flags;	/* WF_ flags */
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
    dword_T	*dw, *edw;
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

    ga_init2(&ga, sizeof(langp_T), 2);

    /* loop over comma separated languages. */
    for (lang = buf->b_p_spl; *lang != NUL; lang = e)
    {
	e = vim_strchr(lang, ',');
	if (e == NULL)
	    e = lang + STRLEN(lang);
	if (e > lang + 2)
	{
	    if (lang[2] != '_' || e - lang != 5)
	    {
		ga_clear(&ga);
		return e_invarg;
	    }
	    region = lang + 3;
	}
	else
	    region = NULL;

	for (lp = first_lang; lp != NULL; lp = lp->sl_next)
	    if (STRNICMP(lp->sl_name, lang, 2) == 0)
		break;

	if (lp == NULL)
	    /* Not found, load the language. */
	    lp = spell_load_lang(lang);

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
		    c = lang[5];
		    lang[5] = NUL;
		    smsg((char_u *)_("Warning: region %s not supported"), lang);
		    lang[5] = c;
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
    sblock_T	*sp;

    /* Initialize the table for spell_iswordc(). */
    init_spell_chartab();

    /* Unload all allocated memory. */
    while (first_lang != NULL)
    {
	lp = first_lang;
	first_lang = lp->sl_next;

	hash_clear(&lp->sl_fwords.wi_ht);
	ga_clear(&lp->sl_fwords.wi_add);
	hash_clear(&lp->sl_kwords.wi_ht);
	ga_clear(&lp->sl_kwords.wi_add);
	while (lp->sl_block != NULL)
	{
	    sp = lp->sl_block;
	    lp->sl_block = sp->sb_next;
	    vim_free(sp);
	}
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

#endif  /* FEAT_SYN_HL */
