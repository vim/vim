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
 * The spell checking mechanism uses a tree (aka trie).  Each node in the tree
 * has a list of bytes that can appear (siblings).  For each byte there is a
 * pointer to the node with the byte that follows in the word (child).
 * A NUL byte is used where the word may end.
 *
 * There are two trees: one with case-folded words and one with words in
 * original case.  The second one is only used for keep-case words and is
 * usually small.
 *
 * Thanks to Olaf Seibert for providing an example implementation of this tree
 * and the compression mechanism.
 *
 * Matching involves checking the caps type: Onecap ALLCAP KeepCap.
 *
 * Why doesn't Vim use aspell/ispell/myspell/etc.?
 * See ":help develop-spell".
 */

/*
 * Vim spell file format:  <HEADER> <SUGGEST> <LWORDTREE> <KWORDTREE>
 *
 * <HEADER>: <fileID> <regioncnt> <regionname> ...
 *		 <charflagslen> <charflags> <fcharslen> <fchars>
 *
 * <fileID>     10 bytes    "VIMspell05"
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
 * <SUGGEST> : <suggestlen> <more> ...
 *
 * <suggestlen> 4 bytes	    Length of <SUGGEST> in bytes, excluding
 *			    <suggestlen>.  MSB first.
 * <more>		    To be defined.
 *
 *
 * <LWORDTREE>: <wordtree>
 *
 * <wordtree>: <nodecount> <nodedata> ...
 *
 * <nodecount>	4 bytes	    Number of nodes following.  MSB first.
 *
 * <nodedata>: <siblingcount> <sibling> ...
 *
 * <siblingcount> 1 byte    Number of siblings in this node.  The siblings
 *			    follow in sorted order.
 *
 * <sibling>: <byte> [<nodeidx> <xbyte> | <flags> [<region>]]
 *
 * <byte>	1 byte	    Byte value of the sibling.  Special cases:
 *			    BY_NOFLAGS: End of word without flags and for all
 *					regions.
 *			    BY_FLAGS: End of word, <flags> follow.
 *			    BY_INDEX: Child of sibling is shared, <nodeidx>
 *					and <xbyte> follow.
 *
 * <nodeidx>	3 bytes	    Index of child for this sibling, MSB first.
 *
 * <xbyte>	1 byte	    byte value of the sibling.
 *
 * <flags>	1 byte	    bitmask of:
 *			    WF_ALLCAP	word must have only capitals
 *			    WF_ONECAP   first char of word must be capital
 *			    WF_RARE	rare word
 *			    WF_REGION	<region> follows
 *
 * <region>	1 byte	    Bitmask for regions in which word is valid.  When
 *			    omitted it's valid in all regions.
 *			    Lowest bit is for region 1.
 *
 * <KWORDTREE>: <wordtree>
 *
 *
 * All text characters are in 'encoding', but stored as single bytes.
 * The region name is ASCII.
 */

#if defined(MSDOS) || defined(WIN16) || defined(WIN32) || defined(_WIN64)
# include <io.h>	/* for lseek(), must be before vim.h */
#endif

#include "vim.h"

#if defined(FEAT_SYN_HL) || defined(PROTO)

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#define MAXWLEN 250		/* assume max. word len is this many bytes */

/* Flags used for a word. */
#define WF_REGION   0x01	/* region byte follows */
#define WF_ONECAP   0x02	/* word with one capital (or all capitals) */
#define WF_ALLCAP   0x04	/* word must be all capitals */
#define WF_RARE	    0x08	/* rare word */

#define WF_KEEPCAP  0x100	/* keep-case word */

#define BY_NOFLAGS  0		/* end of word without flags or region */
#define BY_FLAGS    1		/* end of word, flag byte follows */
#define BY_INDEX    2		/* child is shared, index follows */
#define BY_SPECIAL  BY_INDEX	/* hightest special byte value */

/* Info from "REP" entries in ".aff" file used in af_rep.
 * TODO: This is not used yet.  Either use it or remove it. */
typedef struct repentry_S
{
    char_u	*re_from;
    char_u	*re_to;
} repentry_T;

/*
 * Structure used to store words and other info for one language, loaded from
 * a .spl file.
 * The main access is through the tree in "sl_fbyts/sl_fidxs", storing the
 * case-folded words.  "sl_kbyts/sl_kidxs" is for keep-case words.
 *
 * The "byts" array stores the possible bytes in each tree node, preceded by
 * the number of possible bytes, sorted on byte value:
 *	<len> <byte1> <byte2> ...
 * The "idxs" array stores the index of the child node corresponding to the
 * byte in "byts".
 * Exception: when the byte is zero, the word may end here and "idxs" holds
 * the flags and region for the word.  There may be several zeros in sequence
 * for alternative flag/region combinations.
 */
typedef struct slang_S slang_T;
struct slang_S
{
    slang_T	*sl_next;	/* next language */
    char_u	*sl_name;	/* language name "en", "en.rare", "nl", etc. */
    char_u	*sl_fbyts;	/* case-folded word bytes */
    int		*sl_fidxs;	/* case-folded word indexes */
    char_u	*sl_kbyts;	/* keep-case word bytes */
    int		*sl_kidxs;	/* keep-case word indexes */
    char_u	*sl_try;	/* "TRY" from .aff file  TODO: not used */
    garray_T	sl_rep;		/* list of repentry_T entries from REP lines
				 * TODO not used */
    char_u	sl_regions[17];	/* table with up to 8 region names plus NUL */
    int		sl_error;	/* error while loading */
};

/* First language that is loaded, start of the linked list of loaded
 * languages. */
static slang_T *first_lang = NULL;

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

#define VIMSPELLMAGIC "VIMspell05"  /* string at start of Vim spell file */
#define VIMSPELLMAGICL 10

/*
 * Structure to store info for word matching.
 */
typedef struct matchinf_S
{
    langp_T	*mi_lp;			/* info for language and region */
    slang_T	*mi_slang;		/* info for the language */

    /* pointers to original text to be checked */
    char_u	*mi_word;		/* start of word being checked */
    char_u	*mi_end;		/* end of matching word */
    char_u	*mi_fend;		/* next char to be added to mi_fword */
    char_u	*mi_cend;		/* char after what was used for
					   mi_capflags */

    /* case-folded text */
    char_u	mi_fword[MAXWLEN + 1];	/* mi_word case-folded */
    int		mi_fwordlen;		/* nr of valid bytes in mi_fword */

    /* others */
    int		mi_result;		/* result so far: SP_BAD, SP_OK, etc. */
    int		mi_capflags;		/* WF_ONECAP WF_ALLCAP WF_KEEPCAP */
} matchinf_T;

static slang_T *slang_alloc __ARGS((char_u *lang));
static void slang_free __ARGS((slang_T *lp));
static void find_word __ARGS((matchinf_T *mip, int keepcap));
static slang_T *spell_load_lang __ARGS((char_u *lang));
static void spell_load_file __ARGS((char_u *fname, void *cookie));
static int read_tree __ARGS((FILE *fd, char_u *byts, int *idxs, int maxidx, int startidx));
static int find_region __ARGS((char_u *rp, char_u *region));
static int captype __ARGS((char_u *word, char_u *end));

/*
 * Main spell-checking function.
 * "ptr" points to a character that could be the start of a word.
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
    matchinf_T	mi;		/* Most things are put in "mi" so that it can
				   be passed to functions quickly. */

    /* Find the end of the word. */
    mi.mi_word = ptr;
    mi.mi_end = ptr;

    /* A word starting with a number is always OK.  Also skip hexadecimal
     * numbers 0xFF99 and 0X99FF. */
    if (*ptr >= '0' && *ptr <= '9')
    {
	if (*ptr == '0' && (ptr[1] == 'x' || ptr[2] == 'X'))
	    mi.mi_end = skiphex(ptr);
	else
	    mi.mi_end = skipdigits(ptr);
    }
    else
    {
	mi.mi_fend = ptr;
	if (spell_iswordc(mi.mi_fend))
	{
	    /* Make case-folded copy of the characters until the next non-word
	     * character. */
	    do
	    {
		mb_ptr_adv(mi.mi_fend);
	    } while (*mi.mi_fend != NUL && spell_iswordc(mi.mi_fend));

	    (void)spell_casefold(ptr, (int)(mi.mi_fend - ptr), mi.mi_fword,
								 MAXWLEN + 1);
	    mi.mi_fwordlen = STRLEN(mi.mi_fword);

	    /* Check the caps type of the word. */
	    mi.mi_capflags = captype(ptr, mi.mi_fend);

	    /* We always use the characters up to the next non-word character,
	     * also for bad words. */
	    mi.mi_end = mi.mi_fend;
	}
	else
	{
	    /* No word characters.  Don't case-fold anything, we may quickly
	     * find out this is not a word (but it could be!). */
	    mi.mi_fwordlen = 0;
	    mi.mi_capflags = 0;
	}

	mi.mi_cend = mi.mi_fend;

	/* The word is bad unless we recognize it. */
	mi.mi_result = SP_BAD;

	/*
	 * Loop over the languages specified in 'spelllang'.
	 * We check them all, because a matching word may be longer than an
	 * already found matching word.
	 */
	for (mi.mi_lp = LANGP_ENTRY(wp->w_buffer->b_langp, 0);
				       mi.mi_lp->lp_slang != NULL; ++mi.mi_lp)
	{
	    /* Check for a matching word in case-folded words. */
	    find_word(&mi, FALSE);

	    /* Try keep-case words. */
	    find_word(&mi, TRUE);
	}

	if (mi.mi_result != SP_OK)
	{
	    /* When we are at a non-word character there is no error, just
	     * skip over the character (try looking for a word after it). */
	    if (!spell_iswordc(ptr))
	    {
#ifdef FEAT_MBYTE
		if (has_mbyte)
		    return mb_ptr2len_check(ptr);
#endif
		return 1;
	    }

	    if (mi.mi_result == SP_BAD)
		*attrp = highlight_attr[HLF_SPB];
	    else if (mi.mi_result == SP_RARE)
		*attrp = highlight_attr[HLF_SPR];
	    else
		*attrp = highlight_attr[HLF_SPL];
	}
    }

    return (int)(mi.mi_end - ptr);
}

/*
 * Check if the word at "mip->mi_word" is in the tree.
 * When "keepcap" is TRUE check in keep-case word tree.
 *
 * For a match mip->mi_result is updated.
 */
    static void
find_word(mip, keepcap)
    matchinf_T	*mip;
    int		keepcap;
{
    int		arridx = 0;
    int		endlen[MAXWLEN];    /* length at possible word endings */
    int		endidx[MAXWLEN];    /* possible word endings */
    int		endidxcnt = 0;
    int		len;
    int		wlen = 0;
    int		flen;
    int		c;
    char_u	*ptr;
    unsigned	lo, hi, m;
#ifdef FEAT_MBYTE
    char_u	*s;
    char_u	*p;
#endif
    int		res;
    int		valid;
    slang_T	*slang = mip->mi_lp->lp_slang;
    unsigned	flags;
    char_u	*byts;
    int		*idxs;

    if (keepcap)
    {
	/* Check for word with matching case in keep-case tree. */
	ptr = mip->mi_word;
	flen = 9999;		    /* no case folding, always enough bytes */
	byts = slang->sl_kbyts;
	idxs = slang->sl_kidxs;
    }
    else
    {
	/* Check for case-folded in case-folded tree. */
	ptr = mip->mi_fword;
	flen = mip->mi_fwordlen;    /* available case-folded bytes */
	byts = slang->sl_fbyts;
	idxs = slang->sl_fidxs;
    }

    if (byts == NULL)
	return;			/* array is empty */

    /*
     * Repeat advancing in the tree until there is a byte that doesn't match,
     * we reach the end of the tree or we reach the end of the line.
     */
    for (;;)
    {
	if (flen == 0 && *mip->mi_fend != NUL)
	{
	    /* Need to fold at least one more character.  Do until next
	     * non-word character for efficiency. */
	    do
	    {
#ifdef FEAT_MBYTE
		if (has_mbyte)
		    flen += mb_ptr2len_check(mip->mi_fend + flen);
		else
#endif
		    ++flen;
	    } while (spell_iswordc(mip->mi_fend + flen));

	    (void)spell_casefold(mip->mi_fend, flen,
				     mip->mi_fword + mip->mi_fwordlen,
				     MAXWLEN - mip->mi_fwordlen);
	    mip->mi_fend += flen;
	    flen = STRLEN(mip->mi_fword + mip->mi_fwordlen);
	    mip->mi_fwordlen += flen;
	}

	len = byts[arridx++];

	/* If the first possible byte is a zero the word could end here.
	 * Remember this index, we first check for the longest word. */
	if (byts[arridx] == 0)
	{
	    endlen[endidxcnt] = wlen;
	    endidx[endidxcnt++] = arridx++;
	    --len;

	    /* Skip over the zeros, there can be several flag/region
	     * combinations. */
	    while (len > 0 && byts[arridx] == 0)
	    {
		++arridx;
		--len;
	    }
	    if (len == 0)
		break;	    /* no children, word must end here */
	}

	/* Stop looking at end of the line. */
	if (ptr[wlen] == NUL)
	    break;

	/* Perform a binary search in the list of accepted bytes. */
	c = ptr[wlen];
	lo = arridx;
	hi = arridx + len - 1;
	while (lo < hi)
	{
	    m = (lo + hi) / 2;
	    if (byts[m] > c)
		hi = m - 1;
	    else if (byts[m] < c)
		lo = m + 1;
	    else
	    {
		lo = hi = m;
		break;
	    }
	}

	/* Stop if there is no matching byte. */
	if (hi < lo || byts[lo] != c)
	    break;

	/* Continue at the child (if there is one). */
	arridx = idxs[lo];
	++wlen;
	--flen;
    }

    /*
     * Verify that one of the possible endings is valid.  Try the longest
     * first.
     */
    while (endidxcnt > 0)
    {
	--endidxcnt;
	arridx = endidx[endidxcnt];
	wlen = endlen[endidxcnt];

#ifdef FEAT_MBYTE
	if ((*mb_head_off)(ptr, ptr + wlen) > 0)
	    continue;	    /* not at first byte of character */
#endif
	if (spell_iswordc(ptr + wlen))
	    continue;	    /* next char is a word character */

#ifdef FEAT_MBYTE
	if (!keepcap && has_mbyte)
	{
	    /* Compute byte length in original word, length may change
	     * when folding case. */
	    p = mip->mi_word;
	    for (s = ptr; s < ptr + wlen; mb_ptr_adv(s))
		mb_ptr_adv(p);
	    wlen = p - mip->mi_word;
	}
#endif

	/* Check flags and region.  Repeat this if there are more
	 * flags/region alternatives until there is a match. */
	res = SP_BAD;
	for (len = byts[arridx - 1]; len > 0 && byts[arridx] == 0; --len)
	{
	    flags = idxs[arridx];
	    if (keepcap)
	    {
		/* For "keepcap" tree the case is always right. */
		valid = TRUE;
	    }
	    else
	    {
		/* Check that the word is in the required case. */
		if (mip->mi_cend != mip->mi_word + wlen)
		{
		    /* mi_capflags was set for a different word
		     * length, need to do it again. */
		    mip->mi_cend = mip->mi_word + wlen;
		    mip->mi_capflags = captype(mip->mi_word,
							mip->mi_cend);
		}

		valid = (mip->mi_capflags == WF_ALLCAP
			|| ((flags & WF_ALLCAP) == 0
			    && ((flags & WF_ONECAP) == 0
				|| mip->mi_capflags == WF_ONECAP)));
	    }

	    if (valid && res != SP_OK)
	    {
		if (flags & WF_REGION)
		{
		    /* Check region. */
		    if ((mip->mi_lp->lp_region & (flags >> 8)) != 0)
			res = SP_OK;
		    else
			res = SP_LOCAL;
		}
		else if (flags & WF_RARE)
		    res = SP_RARE;
		else
		    res = SP_OK;
	    }

	    if (res == SP_OK)
		break;
	    ++arridx;
	}

	if (valid)
	{
	    /* Valid word!  Always use the longest match. */
	    if (mip->mi_end < mip->mi_word + wlen)
		mip->mi_end = mip->mi_word + wlen;
	    if (mip->mi_result != SP_OK)
		mip->mi_result = res;
	    break;
	}
    }
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

	while (*p != NUL)
	{
	    /* When searching backward don't search after the cursor. */
	    if (dir == BACKWARD
		    && lnum == curwin->w_cursor.lnum
		    && (colnr_T)(p - line) >= curwin->w_cursor.col)
		break;

	    /* start of word */
	    len = spell_check(curwin, p, &attr);

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

	    /* advance to character after the word */
	    p += len;
	    if (*p == NUL)
		break;
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
	vim_snprintf((char *)fname_enc, sizeof(fname_enc),
						  "spell/%s.%s.spl", lang, p);

	r = do_in_runtimepath(fname_enc, TRUE, spell_load_file, lp);
	if (r == FAIL && !lp->sl_error)
	{
	    /* Try loading the ASCII version. */
	    vim_snprintf((char *)fname_enc, sizeof(fname_enc),
						  "spell/%s.ascii.spl", lang);

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

    lp = (slang_T *)alloc_clear(sizeof(slang_T));
    if (lp != NULL)
    {
	lp->sl_name = vim_strsave(lang);
	ga_init2(&lp->sl_rep, sizeof(repentry_T), 4);
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
    vim_free(lp->sl_name);
    vim_free(lp->sl_fbyts);
    vim_free(lp->sl_kbyts);
    vim_free(lp->sl_fidxs);
    vim_free(lp->sl_kidxs);
    ga_clear(&lp->sl_rep);
    vim_free(lp->sl_try);
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
    char_u	*p;
    int		i;
    int		len;
    int		round;
    char_u	*save_sourcing_name = sourcing_name;
    linenr_T	save_sourcing_lnum = sourcing_lnum;
    int		cnt, ccnt;
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
	p = alloc((unsigned)cnt);
	if (p == NULL)
	    goto endFAIL;
	for (i = 0; i < cnt; ++i)
	    p[i] = getc(fd);				/* <charflags> */

	ccnt = (getc(fd) << 8) + getc(fd);		/* <fcharslen> */
	if (ccnt <= 0)
	{
	    vim_free(p);
	    goto formerr;
	}
	fol = alloc((unsigned)ccnt + 1);
	if (fol == NULL)
	{
	    vim_free(p);
	    goto endFAIL;
	}
	for (i = 0; i < ccnt; ++i)
	    fol[i] = getc(fd);				/* <fchars> */
	fol[i] = NUL;

	/* Set the word-char flags and fill spell_isupper() table. */
	i = set_spell_charflags(p, cnt, fol);
	vim_free(p);
	vim_free(fol);
	if (i == FAIL)
	    goto formerr;
    }
    else
    {
	/* When <charflagslen> is zero then <fcharlen> must also be zero. */
	cnt = (getc(fd) << 8) + getc(fd);
	if (cnt != 0)
	    goto formerr;
    }

    /* <SUGGEST> : <suggestlen> <more> ... */
    /* TODO, just skip this for now */
    i = (getc(fd) << 24) + (getc(fd) << 16) + (getc(fd) << 8) + getc(fd);
    while (i-- > 0)
	if (getc(fd) == EOF)				/* <suggestlen> */
	    goto truncerr;

    /* round 1: <LWORDTREE>
     * round 2: <KWORDTREE> */
    for (round = 1; round <= 2; ++round)
    {
	/* The tree size was computed when writing the file, so that we can
	 * allocate it as one long block. <nodecount> */
	len = (getc(fd) << 24) + (getc(fd) << 16) + (getc(fd) << 8) + getc(fd);
	if (len < 0)
	    goto truncerr;
	if (len > 0)
	{
	    /* Allocate the byte array. */
	    p = lalloc((long_u)len, TRUE);
	    if (p == NULL)
		goto endFAIL;
	    if (round == 1)
		lp->sl_fbyts = p;
	    else
		lp->sl_kbyts = p;

	    /* Allocate the index array. */
	    p = lalloc_clear((long_u)(len * sizeof(int)), TRUE);
	    if (p == NULL)
		goto endFAIL;
	    if (round == 1)
		lp->sl_fidxs = (int *)p;
	    else
		lp->sl_kidxs = (int *)p;


	    /* Read the tree and store it in the array. */
	    i = read_tree(fd,
			round == 1 ? lp->sl_fbyts : lp->sl_kbyts,
			round == 1 ? lp->sl_fidxs : lp->sl_kidxs,
			len, 0);
	    if (i == -1)
		goto truncerr;
	    if (i < 0)
		goto formerr;
	}
    }

    goto endOK;

endFAIL:
    lp->sl_error = TRUE;

endOK:
    if (fd != NULL)
	fclose(fd);
    sourcing_name = save_sourcing_name;
    sourcing_lnum = save_sourcing_lnum;
}

/*
 * Read one row of siblings from the spell file and store it in the byte array
 * "byts" and index array "idxs".  Recursively read the children.
 *
 * NOTE: The code here must match put_tree().
 *
 * Returns the index follosing the siblings.
 * Returns -1 if the file is shorter than expected.
 * Returns -2 if there is a format error.
 */
    static int
read_tree(fd, byts, idxs, maxidx, startidx)
    FILE	*fd;
    char_u	*byts;
    int		*idxs;
    int		maxidx;		    /* size of arrays */
    int		startidx;	    /* current index in "byts" and "idxs" */
{
    int		len;
    int		i;
    int		n;
    int		idx = startidx;
    int		c;
#define SHARED_MASK	0x8000000

    len = getc(fd);					/* <siblingcount> */
    if (len <= 0)
	return -1;

    if (startidx + len >= maxidx)
	return -2;
    byts[idx++] = len;

    /* Read the byte values, flag/region bytes and shared indexes. */
    for (i = 1; i <= len; ++i)
    {
	c = getc(fd);					/* <byte> */
	if (c < 0)
	    return -1;
	if (c <= BY_SPECIAL)
	{
	    if (c == BY_NOFLAGS)
	    {
		/* No flags, all regions. */
		idxs[idx] = 0;
		c = 0;
	    }
	    else if (c == BY_FLAGS)
	    {
		/* Read flags and option region. */
		c = getc(fd);				/* <flags> */
		if (c & WF_REGION)
		    c = (getc(fd) << 8) + c;		/* <region> */
		idxs[idx] = c;
		c = 0;
	    }
	    else /* c == BY_INDEX */
	    {
							/* <nodeidx> */
		n = (getc(fd) << 16) + (getc(fd) << 8) + getc(fd);
		if (n < 0 || n >= maxidx)
		    return -2;
		idxs[idx] = n + SHARED_MASK;
		c = getc(fd);				/* <xbyte> */
	    }
	}
	byts[idx++] = c;
    }

    /* Recursively read the children for non-shared siblings.
     * Skip the end-of-word ones (zero byte value) and the shared ones (and
     * remove SHARED_MASK) */
    for (i = 1; i <= len; ++i)
	if (byts[startidx + i] != 0)
	{
	    if (idxs[startidx + i] & SHARED_MASK)
		idxs[startidx + i] &= ~SHARED_MASK;
	    else
	    {
		idxs[startidx + i] = idx;
		idx = read_tree(fd, byts, idxs, maxidx, idx);
		if (idx < 0)
		    break;
	    }
	}

    return idx;
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
 * Word		WF_ONECAP
 * W WORD	WF_ALLCAP
 * WoRd	wOrd	WF_KEEPCAP
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
		    return WF_KEEPCAP;
		allcap = FALSE;
	    }
	    else if (!allcap)
		/* UlU -> KEEPCAP */
		return WF_KEEPCAP;
	    past_second = TRUE;
	}

    if (allcap)
	return WF_ALLCAP;
    if (firstcap)
	return WF_ONECAP;
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


#if defined(FEAT_MBYTE) || defined(PROTO)
/*
 * Functions for ":mkspell".
 * Only possible with the multi-byte feature.
 */

#define MAXLINELEN  500		/* Maximum length in bytes of a line in a .aff
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
/* Affix entry from ".aff" file.  Used for prefixes and suffixes. */
struct affentry_S
{
    affentry_T	*ae_next;	/* next affix with same name/number */
    char_u	*ae_chop;	/* text to chop off basic word (can be NULL) */
    char_u	*ae_add;	/* text to add to basic word (can be NULL) */
    char_u	*ae_cond;	/* condition (NULL for ".") */
    regprog_T	*ae_prog;	/* regexp program for ae_cond or NULL */
};

/* Affix header from ".aff" file.  Used for af_pref and af_suff. */
typedef struct affheader_S
{
    char_u	ah_key[2];	/* key for hashtable == name of affix entry */
    int		ah_combine;	/* suffix may combine with prefix */
    affentry_T	*ah_first;	/* first affix entry */
} affheader_T;

#define HI2AH(hi)   ((affheader_T *)(hi)->hi_key)

/*
 * Structure that is used to store the items in the word tree.  This avoids
 * the need to keep track of each allocated thing, it's freed all at once
 * after ":mkspell" is done.
 */
#define  SBLOCKSIZE 16000	/* size of sb_data */
typedef struct sblock_S sblock_T;
struct sblock_S
{
    sblock_T	*sb_next;	/* next block in list */
    int		sb_used;	/* nr of bytes already in use */
    char_u	sb_data[1];	/* data, actually longer */
};

/*
 * A node in the tree.
 */
typedef struct wordnode_S wordnode_T;
struct wordnode_S
{
    char_u	wn_hashkey[6];	/* room for the hash key */
    wordnode_T	*wn_next;	/* next node with same hash key */
    wordnode_T	*wn_child;	/* child (next byte in word) */
    wordnode_T  *wn_sibling;	/* next sibling (alternate byte in word,
				   always sorted) */
    wordnode_T	*wn_wnode;	/* parent node that will write this node */
    int		wn_index;	/* index in written nodes (valid after first
				   round) */
    char_u	wn_byte;	/* Byte for this node. NUL for word end */
    char_u	wn_flags;	/* when wn_byte is NUL: WF_ flags */
    char_u	wn_region;	/* when wn_byte is NUL: region mask */
};

#define HI2WN(hi)    (wordnode_T *)((hi)->hi_key)

/*
 * Info used while reading the spell files.
 */
typedef struct spellinfo_S
{
    wordnode_T	*si_foldroot;	/* tree with case-folded words */
    wordnode_T	*si_keeproot;	/* tree with keep-case words */
    sblock_T	*si_blocks;	/* memory blocks used */
    int		si_ascii;	/* handling only ASCII words */
    int		si_region;	/* region mask */
    vimconv_T	si_conv;	/* for conversion to 'encoding' */
} spellinfo_T;

static afffile_T *spell_read_aff __ARGS((char_u *fname, spellinfo_T *spin));
static int has_non_ascii __ARGS((char_u *s));
static void spell_free_aff __ARGS((afffile_T *aff));
static int spell_read_dic __ARGS((char_u *fname, spellinfo_T *spin, afffile_T *affile));
static int store_aff_word __ARGS((char_u *word, spellinfo_T *spin, char_u *afflist, hashtab_T *ht, hashtab_T *xht, int comb));
static int spell_read_wordfile __ARGS((char_u *fname, spellinfo_T *spin));
static void *getroom __ARGS((sblock_T **blp, size_t len));
static char_u *getroom_save __ARGS((sblock_T **blp, char_u *s));
static void free_blocks __ARGS((sblock_T *bl));
static wordnode_T *wordtree_alloc __ARGS((sblock_T **blp));
static int store_word __ARGS((char_u *word, spellinfo_T *spin));
static int tree_add_word __ARGS((char_u *word, wordnode_T *tree, int flags, int region, sblock_T **blp));
static void wordtree_compress __ARGS((wordnode_T *root));
static int node_compress __ARGS((wordnode_T *node, hashtab_T *ht, int *tot));
static int node_equal __ARGS((wordnode_T *n1, wordnode_T *n2));
static void write_vim_spell __ARGS((char_u *fname, spellinfo_T *spin, int regcount, char_u *regchars));
static int put_tree __ARGS((FILE *fd, wordnode_T *node, int index, int regionmask));

/*
 * Read an affix ".aff" file.
 * Returns an afffile_T, NULL for failure.
 */
    static afffile_T *
spell_read_aff(fname, spin)
    char_u	*fname;
    spellinfo_T	*spin;
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

    /*
     * Open the file.
     */
    fd = fopen((char *)fname, "r");
    if (fd == NULL)
    {
	EMSG2(_(e_notopen), fname);
	return NULL;
    }

    smsg((char_u *)_("Reading affix file %s..."), fname);
    out_flush();

    /*
     * Allocate and init the afffile_T structure.
     */
    aff = (afffile_T *)getroom(&spin->si_blocks, sizeof(afffile_T));
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
	if (spin->si_conv.vc_type != CONV_NONE)
	{
	    pc = string_convert(&spin->si_conv, rline, NULL);
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
	    if (itemcnt == 6)	    /* too many items */
		break;
	    items[itemcnt++] = p;
	    while (*p > ' ')	    /* skip until white space or CR/NL */
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
		/* Setup for conversion from "ENC" to 'encoding'. */
		aff->af_enc = enc_canonize(items[1]);
		if (aff->af_enc != NULL && !spin->si_ascii
			&& convert_setup(&spin->si_conv, aff->af_enc,
							       p_enc) == FAIL)
		    smsg((char_u *)_("Conversion in %s not supported: from %s to %s"),
					       fname, aff->af_enc, p_enc);
	    }
	    else if (STRCMP(items[0], "TRY") == 0 && itemcnt == 2
						       && aff->af_try == NULL)
	    {
		aff->af_try = getroom_save(&spin->si_blocks, items[1]);
	    }
	    else if ((STRCMP(items[0], "PFX") == 0
					      || STRCMP(items[0], "SFX") == 0)
		    && aff_todo == 0
		    && itemcnt == 4)
	    {
		/* New affix letter. */
		cur_aff = (affheader_T *)getroom(&spin->si_blocks,
							 sizeof(affheader_T));
		if (cur_aff == NULL)
		    break;
		cur_aff->ah_key[0] = *items[1];
		cur_aff->ah_key[1] = NUL;
		if (items[1][1] != NUL)
		    smsg((char_u *)_("Affix name too long in %s line %d: %s"),
						       fname, lnum, items[1]);
		if (*items[2] == 'Y')
		    cur_aff->ah_combine = TRUE;
		else if (*items[2] != 'N')
		    smsg((char_u *)_("Expected Y or N in %s line %d: %s"),
						       fname, lnum, items[2]);
		if (*items[0] == 'P')
		    tp = &aff->af_pref;
		else
		    tp = &aff->af_suff;
		aff_todo = atoi((char *)items[3]);
		if (!HASHITEM_EMPTY(hash_find(tp, cur_aff->ah_key)))
		{
		    smsg((char_u *)_("Duplicate affix in %s line %d: %s"),
						       fname, lnum, items[1]);
		    aff_todo = 0;
		}
		else
		    hash_add(tp, cur_aff->ah_key);
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
		aff_entry = (affentry_T *)getroom(&spin->si_blocks,
							  sizeof(affentry_T));
		if (aff_entry == NULL)
		    break;

		if (STRCMP(items[2], "0") != 0)
		    aff_entry->ae_chop = getroom_save(&spin->si_blocks,
								    items[2]);
		if (STRCMP(items[3], "0") != 0)
		    aff_entry->ae_add = getroom_save(&spin->si_blocks,
								    items[3]);

		/* Don't use an affix entry with non-ASCII characters when
		 * "spin->si_ascii" is TRUE. */
		if (!spin->si_ascii || !(has_non_ascii(aff_entry->ae_chop)
					  || has_non_ascii(aff_entry->ae_add)))
		{
		    aff_entry->ae_next = cur_aff->ah_first;
		    cur_aff->ah_first = aff_entry;

		    if (STRCMP(items[4], ".") != 0)
		    {
			char_u	buf[MAXLINELEN];

			aff_entry->ae_cond = getroom_save(&spin->si_blocks,
								    items[4]);
			if (*items[0] == 'P')
			    sprintf((char *)buf, "^%s", items[4]);
			else
			    sprintf((char *)buf, "%s$", items[4]);
			aff_entry->ae_prog = vim_regcomp(buf,
							RE_MAGIC + RE_STRING);
		    }
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
		rp->re_from = getroom_save(&spin->si_blocks, items[1]);
		rp->re_to = getroom_save(&spin->si_blocks, items[2]);
		++aff->af_rep.ga_len;
	    }
	    else
		smsg((char_u *)_("Unrecognized item in %s line %d: %s"),
						       fname, lnum, items[0]);
	}
    }

    if (fol != NULL || low != NULL || upp != NULL)
    {
	/* Don't write a word table for an ASCII file, so that we don't check
	 * for conflicts with a word table that matches 'encoding'. */
	if (!spin->si_ascii)
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
    affheader_T	*ah;
    affentry_T	*ae;

    vim_free(aff->af_enc);

    /* All this trouble to foree the "ae_prog" items... */
    for (ht = &aff->af_pref; ; ht = &aff->af_suff)
    {
	todo = ht->ht_used;
	for (hi = ht->ht_array; todo > 0; ++hi)
	{
	    if (!HASHITEM_EMPTY(hi))
	    {
		--todo;
		ah = HI2AH(hi);
		for (ae = ah->ah_first; ae != NULL; ae = ae->ae_next)
		    vim_free(ae->ae_prog);
	    }
	}
	if (ht == &aff->af_suff)
	    break;
    }

    hash_clear(&aff->af_pref);
    hash_clear(&aff->af_suff);
    ga_clear(&aff->af_rep);
}

/*
 * Read dictionary file "fname".
 * Returns OK or FAIL;
 */
    static int
spell_read_dic(fname, spin, affile)
    char_u	*fname;
    spellinfo_T	*spin;
    afffile_T	*affile;
{
    hashtab_T	ht;
    char_u	line[MAXLINELEN];
    char_u	*afflist;
    char_u	*dw;
    char_u	*pc;
    char_u	*w;
    int		l;
    hash_T	hash;
    hashitem_T	*hi;
    FILE	*fd;
    int		lnum = 1;
    int		non_ascii = 0;
    int		retval = OK;
    char_u	message[MAXLINELEN + MAXWLEN];

    /*
     * Open the file.
     */
    fd = fopen((char *)fname, "r");
    if (fd == NULL)
    {
	EMSG2(_(e_notopen), fname);
	return FAIL;
    }

    /* The hashtable is only used to detect duplicated words. */
    hash_init(&ht);

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

	/* Remove CR, LF and white space from the end.  White space halfway
	 * the word is kept to allow e.g., "et al.". */
	l = STRLEN(line);
	while (l > 0 && line[l - 1] <= ' ')
	    --l;
	if (l == 0)
	    continue;	/* empty line */
	line[l] = NUL;

	/* This takes time, print a message now and then. */
	if ((lnum & 0x3ff) == 0)
	{
	    vim_snprintf((char *)message, sizeof(message),
					      _("line %6d - %s"), lnum, line);
	    msg_start();
	    msg_outtrans_attr(message, 0);
	    msg_clr_eos();
	    msg_didout = FALSE;
	    msg_col = 0;
	    out_flush();
	}

	/* Find the optional affix names. */
	afflist = vim_strchr(line, '/');
	if (afflist != NULL)
	    *afflist++ = NUL;

	/* Skip non-ASCII words when "spin->si_ascii" is TRUE. */
	if (spin->si_ascii && has_non_ascii(line))
	{
	    ++non_ascii;
	    continue;
	}

	/* Convert from "SET" to 'encoding' when needed. */
	if (spin->si_conv.vc_type != CONV_NONE)
	{
	    pc = string_convert(&spin->si_conv, line, NULL);
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

	/* Store the word in the hashtable to be able to find duplicates. */
	dw = (char_u *)getroom_save(&spin->si_blocks, w);
	if (dw == NULL)
	    retval = FAIL;
	vim_free(pc);
	if (retval == FAIL)
	    break;

	hash = hash_hash(dw);
	hi = hash_lookup(&ht, dw, hash);
	if (!HASHITEM_EMPTY(hi))
	    smsg((char_u *)_("Duplicate word in %s line %d: %s"),
							   fname, lnum, line);
	else
	    hash_add_item(&ht, hi, dw, hash);

	/* Add the word to the word tree(s). */
	if (store_word(dw, spin) == FAIL)
	    retval = FAIL;

	if (afflist != NULL)
	{
	    /* Find all matching suffixes and add the resulting words.
	     * Additionally do matching prefixes that combine. */
	    if (store_aff_word(dw, spin, afflist,
			   &affile->af_suff, &affile->af_pref, FALSE) == FAIL)
		retval = FAIL;

	    /* Find all matching prefixes and add the resulting words. */
	    if (store_aff_word(dw, spin, afflist,
				       &affile->af_pref, NULL, FALSE) == FAIL)
		retval = FAIL;
	}
    }

    if (spin->si_ascii && non_ascii > 0)
	smsg((char_u *)_("Ignored %d words with non-ASCII characters"),
								   non_ascii);
    hash_clear(&ht);

    fclose(fd);
    return retval;
}

/*
 * Apply affixes to a word and store the resulting words.
 * "ht" is the hashtable with affentry_T that need to be applied, either
 * prefixes or suffixes.
 * "xht", when not NULL, is the prefix hashtable, to be used additionally on
 * the resulting words for combining affixes.
 *
 * Returns FAIL when out of memory.
 */
    static int
store_aff_word(word, spin, afflist, ht, xht, comb)
    char_u	*word;		/* basic word start */
    spellinfo_T	*spin;		/* spell info */
    char_u	*afflist;	/* list of names of supported affixes */
    hashtab_T	*ht;
    hashtab_T	*xht;
    int		comb;		/* only use affixes that combine */
{
    int		todo;
    hashitem_T	*hi;
    affheader_T	*ah;
    affentry_T	*ae;
    regmatch_T	regmatch;
    char_u	newword[MAXWLEN];
    int		retval = OK;
    int		i;
    char_u	*p;

    todo = ht->ht_used;
    for (hi = ht->ht_array; todo > 0 && retval == OK; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    --todo;
	    ah = HI2AH(hi);

	    /* Check that the affix combines, if required, and that the word
	     * supports this affix. */
	    if ((!comb || ah->ah_combine)
				  && vim_strchr(afflist, *ah->ah_key) != NULL)
	    {
		/* Loop over all affix entries with this name. */
		for (ae = ah->ah_first; ae != NULL; ae = ae->ae_next)
		{
		    /* Check the condition.  It's not logical to match case
		     * here, but it is required for compatibility with
		     * Myspell. */
		    regmatch.regprog = ae->ae_prog;
		    regmatch.rm_ic = FALSE;
		    if (ae->ae_prog == NULL
				  || vim_regexec(&regmatch, word, (colnr_T)0))
		    {
			/* Match.  Remove the chop and add the affix. */
			if (xht == NULL)
			{
			    /* prefix: chop/add at the start of the word */
			    if (ae->ae_add == NULL)
				*newword = NUL;
			    else
				STRCPY(newword, ae->ae_add);
			    p = word;
			    if (ae->ae_chop != NULL)
				/* Skip chop string. */
				for (i = mb_charlen(ae->ae_chop); i > 0; --i)
				    mb_ptr_adv(p);
			    STRCAT(newword, p);
			}
			else
			{
			    /* suffix: chop/add at the end of the word */
			    STRCPY(newword, word);
			    if (ae->ae_chop != NULL)
			    {
				/* Remove chop string. */
				p = newword + STRLEN(newword);
				for (i = mb_charlen(ae->ae_chop); i > 0; --i)
				    mb_ptr_back(newword, p);
				*p = NUL;
			    }
			    if (ae->ae_add != NULL)
				STRCAT(newword, ae->ae_add);
			}

			/* Store the modified word. */
			if (store_word(newword, spin) == FAIL)
			    retval = FAIL;

			/* When added a suffix and combining is allowed also
			 * try adding prefixes additionally. */
			if (xht != NULL && ah->ah_combine)
			    if (store_aff_word(newword, spin, afflist,
						     xht, NULL, TRUE) == FAIL)
				retval = FAIL;
		    }
		}
	    }
	}
    }

    return retval;
}

/*
 * Read a file with a list of words.
 */
    static int
spell_read_wordfile(fname, spin)
    char_u	*fname;
    spellinfo_T	*spin;
{
    FILE	*fd;
    long	lnum = 0;
    char_u	rline[MAXLINELEN];
    char_u	*line;
    char_u	*pc = NULL;
    int		l;
    int		retval = OK;
    int		did_word = FALSE;
    int		non_ascii = 0;
    char_u	*enc;

    /*
     * Open the file.
     */
    fd = fopen((char *)fname, "r");
    if (fd == NULL)
    {
	EMSG2(_(e_notopen), fname);
	return FAIL;
    }

    smsg((char_u *)_("Reading word file %s..."), fname);
    out_flush();

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

	/* Remove CR, LF and white space from the end. */
	l = STRLEN(rline);
	while (l > 0 && rline[l - 1] <= ' ')
	    --l;
	if (l == 0)
	    continue;	/* empty or blank line */
	rline[l] = NUL;

	/* Convert from "=encoding={encoding}" to 'encoding' when needed. */
	vim_free(pc);
	if (spin->si_conv.vc_type != CONV_NONE)
	{
	    pc = string_convert(&spin->si_conv, rline, NULL);
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

	if (*line == '=')
	{
	    if (STRNCMP(line + 1, "encoding=", 9) == 0)
	    {
		if (spin->si_conv.vc_type != CONV_NONE)
		    smsg((char_u *)_("Duplicate =encoding= line ignored in %s line %d: %s"),
						       fname, lnum, line);
		else if (did_word)
		    smsg((char_u *)_("=encoding= line after word ignored in %s line %d: %s"),
						       fname, lnum, line);
		else
		{
		    /* Setup for conversion to 'encoding'. */
		    enc = enc_canonize(line + 10);
		    if (enc != NULL && !spin->si_ascii
			    && convert_setup(&spin->si_conv, enc,
							       p_enc) == FAIL)
			smsg((char_u *)_("Conversion in %s not supported: from %s to %s"),
						   fname, line + 10, p_enc);
		    vim_free(enc);
		}
	    }
	    else
		smsg((char_u *)_("= line ignored in %s line %d: %s"),
							   fname, lnum, line);
	    continue;
	}

	/* Skip non-ASCII words when "spin->si_ascii" is TRUE. */
	if (spin->si_ascii && has_non_ascii(line))
	{
	    ++non_ascii;
	    continue;
	}

	/* Normal word: store it. */
	if (store_word(line, spin) == FAIL)
	{
	    retval = FAIL;
	    break;
	}
	did_word = TRUE;
    }

    vim_free(pc);
    fclose(fd);

    if (spin->si_ascii && non_ascii > 0)
	smsg((char_u *)_("Ignored %d words with non-ASCII characters"),
								   non_ascii);
    return retval;
}

/*
 * Get part of an sblock_T, "len" bytes long.
 * This avoids calling free() for every little struct we use.
 * The memory is cleared to all zeros.
 * Returns NULL when out of memory.
 */
    static void *
getroom(blp, len)
    sblock_T	**blp;
    size_t	len;	    /* length needed */
{
    char_u	*p;
    sblock_T	*bl = *blp;

    if (bl == NULL || bl->sb_used + len > SBLOCKSIZE)
    {
	/* Allocate a block of memory. This is not freed until much later. */
	bl = (sblock_T *)alloc_clear((unsigned)(sizeof(sblock_T) + SBLOCKSIZE));
	if (bl == NULL)
	    return NULL;
	bl->sb_next = *blp;
	*blp = bl;
	bl->sb_used = 0;
    }

    p = bl->sb_data + bl->sb_used;
    bl->sb_used += len;

    return p;
}

/*
 * Make a copy of a string into memory allocated with getroom().
 */
    static char_u *
getroom_save(blp, s)
    sblock_T	**blp;
    char_u	*s;
{
    char_u	*sc;

    sc = (char_u *)getroom(blp, STRLEN(s) + 1);
    if (sc != NULL)
	STRCPY(sc, s);
    return sc;
}


/*
 * Free the list of allocated sblock_T.
 */
    static void
free_blocks(bl)
    sblock_T	*bl;
{
    sblock_T	*next;

    while (bl != NULL)
    {
	next = bl->sb_next;
	vim_free(bl);
	bl = next;
    }
}

/*
 * Allocate the root of a word tree.
 */
    static wordnode_T *
wordtree_alloc(blp)
    sblock_T	**blp;
{
    return (wordnode_T *)getroom(blp, sizeof(wordnode_T));
}

/*
 * Store a word in the tree(s).
 * Always store it in the case-folded tree.
 * For a keep-case word also store it in the keep-case tree.
 */
    static int
store_word(word, spin)
    char_u	*word;
    spellinfo_T	*spin;
{
    int		len = STRLEN(word);
    int		ct = captype(word, word + len);
    char_u	foldword[MAXWLEN];
    int		res;

    (void)spell_casefold(word, len, foldword, MAXWLEN);
    res = tree_add_word(foldword, spin->si_foldroot, ct, spin->si_region,
							    &spin->si_blocks);
    if (res == OK && ct == WF_KEEPCAP)
	res = tree_add_word(word, spin->si_keeproot, ct, spin->si_region,
							    &spin->si_blocks);
    return res;
}

/*
 * Add word "word" to a word tree at "root".
 * Returns FAIL when out of memory.
 */
    static int
tree_add_word(word, root, flags, region, blp)
    char_u	*word;
    wordnode_T	*root;
    int		flags;
    int		region;
    sblock_T	**blp;
{
    wordnode_T	*node = root;
    wordnode_T	*np;
    wordnode_T	**prev = NULL;
    int		i;

    /* Add each byte of the word to the tree, including the NUL at the end. */
    for (i = 0; ; ++i)
    {
	/* Look for the sibling that has the same character.  They are sorted
	 * on byte value, thus stop searching when a sibling is found with a
	 * higher byte value.  For zero bytes (end of word) check that the
	 * flags are equal, there is a separate zero byte for each flag value.
	 */
	while (node != NULL && (node->wn_byte < word[i]
			  || (node->wn_byte == 0 && node->wn_flags != flags)))
	{
	    prev = &node->wn_sibling;
	    node = *prev;
	}
	if (node == NULL || node->wn_byte != word[i])
	{
	    /* Allocate a new node. */
	    np = (wordnode_T *)getroom(blp, sizeof(wordnode_T));
	    if (np == NULL)
		return FAIL;
	    np->wn_byte = word[i];
	    *prev = np;
	    np->wn_sibling = node;
	    node = np;
	}

	if (word[i] == NUL)
	{
	    node->wn_flags = flags;
	    node->wn_region |= region;
	    break;
	}
	prev = &node->wn_child;
	node = *prev;
    }

    return OK;
}

/*
 * Compress a tree: find tails that are identical and can be shared.
 */
    static void
wordtree_compress(root)
    wordnode_T	    *root;
{
    hashtab_T	    ht;
    int		    n;
    int		    tot = 0;

    if (root != NULL)
    {
	hash_init(&ht);
	n = node_compress(root, &ht, &tot);
	smsg((char_u *)_("Compressed %d of %d nodes; %d%% remaining"),
					       n, tot, (tot - n) * 100 / tot);
	hash_clear(&ht);
    }
}

/*
 * Compress a node, its siblings and its children, depth first.
 * Returns the number of compressed nodes.
 */
    static int
node_compress(node, ht, tot)
    wordnode_T	*node;
    hashtab_T	*ht;
    int		*tot;	    /* total count of nodes before compressing,
			       incremented while going through the tree */
{
    wordnode_T	*np;
    wordnode_T	*tp;
    wordnode_T	*child;
    hash_T	hash;
    hashitem_T	*hi;
    int		len = 0;
    unsigned	nr, n;
    int		compressed = 0;

    /*
     * Go through the list of siblings.  Compress each child and then try
     * finding an identical child to replace it.
     * Note that with "child" we mean not just the node that is pointed to,
     * but the whole list of siblings, of which the node is the first.
     */
    for (np = node; np != NULL; np = np->wn_sibling)
    {
	++len;
	if ((child = np->wn_child) != NULL)
	{
	    /* Compress the child.  This fills wn_hashkey. */
	    compressed += node_compress(child, ht, tot);

	    /* Try to find an identical child. */
	    hash = hash_hash(child->wn_hashkey);
	    hi = hash_lookup(ht, child->wn_hashkey, hash);
	    tp = NULL;
	    if (!HASHITEM_EMPTY(hi))
	    {
		/* There are children with an identical hash value.  Now check
		 * if there is one that is really identical. */
		for (tp = HI2WN(hi); tp != NULL; tp = tp->wn_next)
		    if (node_equal(child, tp))
		    {
			/* Found one!  Now use that child in place of the
			 * current one.  This means the current child is
			 * dropped from the tree. */
			np->wn_child = tp;
			++compressed;
			break;
		    }
		if (tp == NULL)
		{
		    /* No other child with this hash value equals the child of
		     * the node, add it to the linked list after the first
		     * item. */
		    tp = HI2WN(hi);
		    child->wn_next = tp->wn_next;
		    tp->wn_next = child;
		}
	    }
	    else
		/* No other child has this hash value, add it to the
		 * hashtable. */
		hash_add_item(ht, hi, child->wn_hashkey, hash);
	}
    }
    *tot += len;

    /*
     * Make a hash key for the node and its siblings, so that we can quickly
     * find a lookalike node.  This must be done after compressing the sibling
     * list, otherwise the hash key would become invalid by the compression.
     */
    node->wn_hashkey[0] = len;
    nr = 0;
    for (np = node; np != NULL; np = np->wn_sibling)
    {
	if (np->wn_byte == NUL)
	    /* end node: only use wn_flags and wn_region */
	    n = np->wn_flags + (np->wn_region << 8);
	else
	    /* byte node: use the byte value and the child pointer */
	    n = np->wn_byte + ((long_u)np->wn_child << 8);
	nr = nr * 101 + n;
    }

    /* Avoid NUL bytes, it terminates the hash key. */
    n = nr & 0xff;
    node->wn_hashkey[1] = n == 0 ? 1 : n;
    n = (nr >> 8) & 0xff;
    node->wn_hashkey[2] = n == 0 ? 1 : n;
    n = (nr >> 16) & 0xff;
    node->wn_hashkey[3] = n == 0 ? 1 : n;
    n = (nr >> 24) & 0xff;
    node->wn_hashkey[4] = n == 0 ? 1 : n;
    node->wn_hashkey[5] = NUL;

    return compressed;
}

/*
 * Return TRUE when two nodes have identical siblings and children.
 */
    static int
node_equal(n1, n2)
    wordnode_T	*n1;
    wordnode_T	*n2;
{
    wordnode_T	*p1;
    wordnode_T	*p2;

    for (p1 = n1, p2 = n2; p1 != NULL && p2 != NULL;
				     p1 = p1->wn_sibling, p2 = p2->wn_sibling)
	if (p1->wn_byte != p2->wn_byte
		|| (p1->wn_byte == NUL
		    ? (p1->wn_flags != p2->wn_flags
					    || p1->wn_region != p2->wn_region)
		    : (p1->wn_child != p2->wn_child)))
	    break;

    return p1 == NULL && p2 == NULL;
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
 * Write the Vim spell file "fname".
 */
    static void
write_vim_spell(fname, spin, regcount, regchars)
    char_u	*fname;
    spellinfo_T	*spin;
    int		regcount;	/* number of regions */
    char_u	*regchars;	/* region names */
{
    FILE	*fd;
    int		regionmask;
    int		round;
    wordnode_T	*tree;
    int		nodecount;

    fd = fopen((char *)fname, "w");
    if (fd == NULL)
    {
	EMSG2(_(e_notopen), fname);
	return;
    }

    /* <HEADER>: <fileID> <regioncnt> <regionname> ...
     *		 <charflagslen> <charflags> <fcharslen> <fchars> */

							    /* <fileID> */
    if (fwrite(VIMSPELLMAGIC, VIMSPELLMAGICL, (size_t)1, fd) != 1)
	EMSG(_(e_write));

    /* write the region names if there is more than one */
    if (regcount > 1)
    {
	putc(regcount, fd);	    /* <regioncnt> <regionname> ... */
	fwrite(regchars, (size_t)(regcount * 2), (size_t)1, fd);
	regionmask = (1 << regcount) - 1;
    }
    else
    {
	putc(0, fd);
	regionmask = 0;
    }

    /* Write the table with character flags and table for case folding.
     * <charflagslen> <charflags>  <fcharlen> <fchars>
     * Skip this for ASCII, the table may conflict with the one used for
     * 'encoding'. */
    if (spin->si_ascii)
    {
	putc(0, fd);
	putc(0, fd);
	putc(0, fd);
    }
    else
	write_spell_chartab(fd);


    /* <SUGGEST> : <suggestlen> <more> ...
     *  TODO.  Only write a zero length for now. */
    put_bytes(fd, 0L, 4);			/* <suggestlen> */

    /*
     * <LWORDTREE>  <KWORDTREE>
     */
    for (round = 1; round <= 2; ++round)
    {
	tree = (round == 1) ? spin->si_foldroot : spin->si_keeproot;

	/* Count the number of nodes.  Needed to be able to allocate the
	 * memory when reading the nodes.  Also fills in the index for shared
	 * nodes. */
	nodecount = put_tree(NULL, tree, 0, regionmask);

	/* number of nodes in 4 bytes */
	put_bytes(fd, (long_u)nodecount, 4);	/* <nodecount> */

	/* Write the nodes. */
	(void)put_tree(fd, tree, 0, regionmask);
    }

    fclose(fd);
}

/*
 * Dump a word tree at node "node".
 *
 * This first writes the list of possible bytes (siblings).  Then for each
 * byte recursively write the children.
 *
 * NOTE: The code here must match the code in read_tree(), since assumptions
 * are made about the indexes (so that we don't have to write them in the
 * file).
 *
 * Returns the number of nodes used.
 */
    static int
put_tree(fd, node, index, regionmask)
    FILE	*fd;	    /* NULL when only counting */
    wordnode_T	*node;
    int		index;
    int		regionmask;
{
    int		newindex = index;
    int		siblingcount = 0;
    wordnode_T	*np;
    int		flags;

    /* If "node" is zero the tree is empty. */
    if (node == NULL)
	return 0;

    /* Store the index where this node is written. */
    node->wn_index = index;

    /* Count the number of siblings. */
    for (np = node; np != NULL; np = np->wn_sibling)
	++siblingcount;

    /* Write the sibling count. */
    if (fd != NULL)
	putc(siblingcount, fd);				/* <siblingcount> */

    /* Write each sibling byte and optionally extra info. */
    for (np = node; np != NULL; np = np->wn_sibling)
    {
	if (np->wn_byte == 0)
	{
	    if (fd != NULL)
	    {
		/* For a NUL byte (end of word) instead of the byte itself
		 * we write the flag/region items. */
		flags = np->wn_flags;
		if (regionmask != 0 && np->wn_region != regionmask)
		    flags |= WF_REGION;
		if (flags == 0)
		{
		    /* word without flags or region */
		    putc(BY_NOFLAGS, fd);		/* <byte> */
		}
		else
		{
		    putc(BY_FLAGS, fd);		/* <byte> */
		    putc(flags, fd);		/* <flags> */
		    if (flags & WF_REGION)
			putc(np->wn_region, fd);	/* <regionmask> */
		}
	    }
	}
	else
	{
	    if (np->wn_child->wn_index != 0 && np->wn_child->wn_wnode != node)
	    {
		/* The child is written elsewhere, write the reference. */
		if (fd != NULL)
		{
		    putc(BY_INDEX, fd);			/* <byte> */
							/* <nodeidx> */
		    put_bytes(fd, (long_u)np->wn_child->wn_index, 3);
		}
	    }
	    else if (np->wn_child->wn_wnode == NULL)
		/* We will write the child below and give it an index. */
		np->wn_child->wn_wnode = node;

	    if (fd != NULL)
		if (putc(np->wn_byte, fd) == EOF) /* <byte> or <xbyte> */
		{
		    EMSG(_(e_write));
		    return 0;
		}
	}
    }

    /* Space used in the array when reading: one for each sibling and one for
     * the count. */
    newindex += siblingcount + 1;

    /* Recursively dump the children of each sibling. */
    for (np = node; np != NULL; np = np->wn_sibling)
	if (np->wn_byte != 0 && np->wn_child->wn_wnode == node)
	    newindex = put_tree(fd, np->wn_child, newindex, regionmask);

    return newindex;
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
    int		i;
    int		len;
    char_u	region_name[16];
    struct stat	st;
    char_u	*arg = eap->arg;
    int		error = FALSE;
    spellinfo_T spin;

    vim_memset(&spin, 0, sizeof(spin));

    if (STRNCMP(arg, "-ascii", 6) == 0)
    {
	spin.si_ascii = TRUE;
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
	vim_snprintf((char *)wfname, sizeof(wfname), "%s.%s.spl", fnames[0],
				   spin.si_ascii ? (char_u *)"ascii" : p_enc);
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

	spin.si_foldroot = wordtree_alloc(&spin.si_blocks);
	spin.si_keeproot = wordtree_alloc(&spin.si_blocks);
	if (spin.si_foldroot == NULL || spin.si_keeproot == NULL)
	{
	    error = TRUE;
	    goto theend;
	}

	/*
	 * Read all the .aff and .dic files.
	 * Text is converted to 'encoding'.
	 * Words are stored in the case-folded and keep-case trees.
	 */
	for (i = 1; i < fcount && !error; ++i)
	{
	    spin.si_conv.vc_type = CONV_NONE;
	    spin.si_region = 1 << (i - 1);

	    vim_snprintf((char *)fname, sizeof(fname), "%s.aff", fnames[i]);
	    if (mch_stat((char *)fname, &st) >= 0)
	    {
		/* Read the .aff file.  Will init "spin->si_conv" based on the
		 * "SET" line. */
		afile[i - 1] = spell_read_aff(fname, &spin);
		if (afile[i - 1] == NULL)
		    error = TRUE;
		else
		{
		    /* Read the .dic file and store the words in the trees. */
		    vim_snprintf((char *)fname, sizeof(fname), "%s.dic",
								   fnames[i]);
		    if (spell_read_dic(fname, &spin, afile[i - 1]) == FAIL)
			error = TRUE;
		}
	    }
	    else
	    {
		/* No .aff file, try reading the file as a word list.  Store
		 * the words in the trees. */
		if (spell_read_wordfile(fnames[i], &spin) == FAIL)
		    error = TRUE;
	    }

	    /* Free any conversion stuff. */
	    convert_setup(&spin.si_conv, NULL, NULL);
	}

	if (!error)
	{
	    /*
	     * Remove the dummy NUL from the start of the tree root.
	     */
	    spin.si_foldroot = spin.si_foldroot->wn_sibling;
	    spin.si_keeproot = spin.si_keeproot->wn_sibling;

	    /*
	     * Combine tails in the tree.
	     */
	    MSG(_("Compressing word tree..."));
	    out_flush();
	    wordtree_compress(spin.si_foldroot);
	    wordtree_compress(spin.si_keeproot);
	}

	if (!error)
	{
	    /*
	     * Write the info in the spell file.
	     */
	    smsg((char_u *)_("Writing spell file %s..."), wfname);
	    out_flush();
	    write_vim_spell(wfname, &spin, fcount - 1, region_name);
	    MSG(_("Done!"));
	    out_flush();
	}

	/* Free the allocated memory. */
	free_blocks(spin.si_blocks);

	/* Free the .aff file structures. */
	for (i = 1; i < fcount; ++i)
	    if (afile[i - 1] != NULL)
		spell_free_aff(afile[i - 1]);
    }

theend:
    FreeWild(fcount, fnames);
}

#endif  /* FEAT_MBYTE */


#endif  /* FEAT_SYN_HL */
