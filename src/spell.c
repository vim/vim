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
 */

#if defined(MSDOS) || defined(WIN16) || defined(WIN32) || defined(_WIN64)
# include <io.h>	/* for lseek(), must be before vim.h */
#endif

#include "vim.h"

#if defined(FEAT_SYN_HL) || defined(PROTO)

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

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

/*
 * Structure used to store words and other info for one language.
 */
typedef struct slang_S slang_T;

struct slang_S
{
    slang_T	*sl_next;	/* next language */
    char_u	sl_name[2];	/* language name "en", "nl", etc. */
    hashtab_T	sl_ht;		/* hashtable with all words */
    garray_T	sl_match;	/* table with pointers to matches */
    garray_T	sl_add;		/* table with pointers to additions */
    char_u	sl_regions[13];	/* table with up to 6 region names */
    sblock_T	*sl_block;	/* list with allocated memory blocks */
};

static slang_T *first_lang = NULL;

/*
 * Structure used in "b_langp", filled from 'spelllang'.
 */
typedef struct langp_S
{
    slang_T	*lp_slang;	/* info for this language (NULL for last one) */
    int		lp_region;	/* bitmask for region or REGION_ALL */
} langp_T;

#define LANGP_ENTRY(ga, i)	(((langp_T *)(ga).ga_data) + (i))
#define MATCH_ENTRY(gap, i)	*(((char_u **)(gap)->ga_data) + i)

/*
 * The byte before a word in the hashtable indicates the type of word.
 * Also used for the byte just before a match.
 * The top two bits are used to indicate rare and case-sensitive words.
 * The lower bits are used to indicate the region in which the word is valid.
 * Words valid in all regions use REGION_ALL.
 */
#define REGION_MASK	0x3f
#define REGION_ALL	0x3f
#define CASE_MASK	0x40
#define RARE_MASK	0x80

#define SP_OK		0
#define SP_BAD		1
#define SP_RARE		2
#define SP_LOCAL	3

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
    char_u	*e;
    langp_T	*lp;
    int		result;
    int		len = 0;
    hash_T	hash;
    hashitem_T	*hi;
    int		c;
#define MAXWLEN 80	/* assume max. word len is 80 */
    char_u	word[MAXWLEN + 1];
    garray_T	*gap;
    int		l, h, t;
    char_u	*p;
    int		n;

    /* Find the end of the word.  We already know that *ptr is a word char. */
    e = ptr;
    do
    {
	mb_ptr_adv(e);
	++len;
    } while (*e != NUL && vim_iswordc_buf(e, wp->w_buffer));

    /* The word is bad unless we find it in the dictionary. */
    result = SP_BAD;

    /* Words are always stored with folded case. */
    (void)str_foldcase(ptr, e - ptr, word, MAXWLEN + 1);
    hash = hash_hash(word);

    /*
     * Loop over the languages specified in 'spelllang'.
     * We check them all, because a match may find a longer word.
     */
    for (lp = LANGP_ENTRY(wp->w_buffer->b_langp, 0); lp->lp_slang != NULL;
								     ++lp)
    {
	/* Check words when it wasn't recognized as a good word yet. */
	if (result != SP_OK)
	{
	    /* Word lookup.  Using a hash table is fast. */
	    hi = hash_lookup(&lp->lp_slang->sl_ht, word, hash);
	    if (!HASHITEM_EMPTY(hi))
	    {
		/* The character before the key indicates the type of word. */
		c = hi->hi_key[-1];
		if ((c & CASE_MASK) != 0)
		{
		    /* Need to check first letter is uppercase.  If it is,
		     * check region.  If it isn't it may be a rare word. */
		    if (
#ifdef FEAT_MBYTE
			    MB_ISUPPER(mb_ptr2char(ptr))
#else
			    MB_ISUPPER(*ptr)
#endif
			    )
		    {
			if ((c & lp->lp_region) == 0)
			    result = SP_LOCAL;
			else
			    result = SP_OK;
		    }
		    else if (c & RARE_MASK)
			result = SP_RARE;
		}
		else
		{
		    if ((c & lp->lp_region) == 0)
			result = SP_LOCAL;
		    else if (c & RARE_MASK)
			result = SP_RARE;
		    else
			result = SP_OK;
		}
	    }
	}

	/* Match lookup.  Uses a binary search.  If there is a match adjust
	 * "e" to the end.  This is also done when a word matched, because
	 * "you've" is longer than "you". */
	gap = &lp->lp_slang->sl_match;
	l = 0;			/* low index */
	h = gap->ga_len - 1;	/* high index */
	/* keep searching, the match must be between "l" and "h" (inclusive) */
	while (h >= l)
	{
	    t = (h + l) / 2;
	    p = MATCH_ENTRY(gap, t) + 1;
	    for (n = 0; p[n] != 0 && p[n] == ptr[n]; ++n)
		;
	    if (p[n] == 0)
	    {
		if ((ptr[n] == 0 || !vim_iswordc_buf(ptr + n, wp->w_buffer)))
		{
		    /* match! */
		    e = ptr + n;
		    if (result != SP_OK)
		    {
			if ((lp->lp_region & p[-1]) == 0)
			    result = SP_LOCAL;
			else
			    result = SP_OK;
		    }
		    break;
		}
		/* match is too short, next item is new low index */
		l = t + 1;
	    }
	    else if (p[n] < ptr[n])
		/* match is before word, next item is new low index */
		l = t + 1;
	    else
		/* match is after word, previous item is new high index */
		h = t - 1;
	}

	/* Addition lookup.  Uses a linear search, there should be very few.
	 * If there is a match adjust "e" to the end.  This doesn't change
	 * whether a word was good or bad, only the length. */
	gap = &lp->lp_slang->sl_add;
	for (t = 0; t < gap->ga_len; ++t)
	{
	    p = MATCH_ENTRY(gap, t) + 1;
	    for (n = 0; p[n] != 0 && p[n] == e[n]; ++n)
		;
	    if (p[n] == 0
		      && (e[n] == 0 || !vim_iswordc_buf(e + n, wp->w_buffer)))
	    {
		/* match */
		e += n;
		break;
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

    return (int)(e - ptr);
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

    lp = (slang_T *)alloc(sizeof(slang_T));
    if (lp != NULL)
    {
	lp->sl_name[0] = lang[0];
	lp->sl_name[1] = lang[1];
	hash_init(&lp->sl_ht);
	ga_init2(&lp->sl_match, sizeof(char_u *), 20);
	ga_init2(&lp->sl_add, sizeof(char_u *), 4);
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
	if (do_in_runtimepath(fname_enc, TRUE, spell_load_file) == FAIL)
	{
	    /* Try again to find an ASCII spell file. */
	    sprintf((char *)fname_ascii, "spell/%c%c.spl", lang[0], lang[1]);
	    if (do_in_runtimepath(fname_ascii, TRUE, spell_load_file) == FAIL)
	    {
		vim_free(lp);
		lp = NULL;
		smsg((char_u *)_("Warning: Cannot find dictionary \"%s\""),
							       fname_enc + 6);
	    }
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
    size_t	l;
    size_t	rest = 0;
    char_u	*p = NULL, *np;
    sblock_T	*bl;
    hash_T	hash;
    hashitem_T	*hi;
    int		c;
    int		region = REGION_ALL;
    char_u	word[MAXWLEN + 1];
    int		n;

    fd = mch_open((char *)fname, O_RDONLY | O_EXTRA, 0);
    if (fd < 0)
    {
	EMSG2(_(e_notopen), fname);
	return;
    }

    /* Get the length of the whole file. */
    len = lseek(fd, (off_t)0, SEEK_END);
    lseek(fd, (off_t)0, SEEK_SET);

    /* Loop, reading the file one block at a time.
     * "rest" is the length of an incomplete line at the previous block.
     * "p" points to the remainder. */
    while (len > 0)
    {
	/* Allocate a block of memory to store the info in.  This is not freed
	 * until spell_reload() is called. */
	if (len > SBLOCKSIZE)
	    l = SBLOCKSIZE;
	else
	    l = len;
	len -= l;
	bl = (sblock_T *)alloc((unsigned)(sizeof(sblock_T) - 1 + l + rest));
	if (bl == NULL)
	    break;
	bl->sb_next = load_lp->sl_block;
	load_lp->sl_block = bl;

	/* Read a block from the file.  Prepend the remainder of the previous
	 * block. */
	if (rest > 0)
	    mch_memmove(bl->sb_data, p, rest);
	if (read(fd, bl->sb_data + rest, l) != l)
	{
	    EMSG2(_(e_notread), fname);
	    break;
	}
	l += rest;
	rest = 0;

	/* Deal with each line that was read until we finish the block. */
	for (p = bl->sb_data; l > 0; p = np)
	{
	    /* "np" points to the char after the line (CR or NL). */
	    for (np = p; l > 0 && *np >= ' '; ++np)
		--l;
	    if (l == 0)
	    {
		/* Incomplete line (or end of file). */
		rest = np - p;
		if (len == 0)
		    EMSG2(_("E751: Truncated spell file: %s"), fname);
		break;
	    }
	    *np = NUL;	    /* terminate the line with a NUL */

	    /* Skip comment and empty lines. */
	    c = *p;
	    if (c != '#' && np > p)
	    {
		if (c == '=' || c == '+')
		{
		    garray_T *gap;

		    /* Match or Add item. */
		    if (c == '=')
			gap = &load_lp->sl_match;
		    else
			gap = &load_lp->sl_add;

		    if (ga_grow(gap, 1) == OK)
		    {
			for (n = 0; n < gap->ga_len; ++n)
			    if ((c = STRCMP(p + 1,
						MATCH_ENTRY(gap, n) + 1)) < 0)
				break;
			if (c == 0)
			{
			    if (p_verbose > 0)
				smsg((char_u *)_("Warning: duplicate match \"%s\" in %s"),
								p + 1, fname);
			}
			else
			{
			    mch_memmove((char_u **)gap->ga_data + n + 1,
				    (char_u **)gap->ga_data + n,
				    (gap->ga_len - n) * sizeof(char_u *));
			    *(((char_u **)gap->ga_data) + n) = p;
			    *p = region;
			    ++gap->ga_len;
			}
		    }
		}
		else if (c == '-')
		{
		    /* region item */
		    ++p;
		    if (*p == '-')
			/* end of a region */
			region = REGION_ALL;
		    else
		    {
			char_u	*rp = load_lp->sl_regions;
			int	r;

			/* The region may be repeated: "-ca-uk".  Fill
			 * "region" with the bit mask for the ones we find. */
			region = 0;
			for (;;)
			{
			    /* start of a region */
			    r = find_region(rp, p);
			    if (r == REGION_ALL)
			    {
				/* new region, add it */
				r = STRLEN(rp);
				if (r >= 12)
				{
				    EMSG2(_("E752: Too many regions in %s"),
								       fname);
				    r = REGION_ALL;
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
				if (p[2] != NUL)
				    EMSG2(_("E753: Invalid character in \"%s\""),
								       p - 1);
				break;
			    }
			    p += 3;
			}
		    }
		}
		else
		{
		    /* add the word */
		    if (c == '>')
			c = region | RARE_MASK;
		    else
		    {
			if (c != ' ')
			    EMSG2(_("E753: Invalid character in \"%s\""), p);
			c = region;
		    }
#ifdef FEAT_MBYTE
		    if (MB_ISUPPER(mb_ptr2char(p + 1)))
#else
		    if (MB_ISUPPER(p[1]))
#endif
			c |= CASE_MASK;
		    *p++ = c;
		    (void)str_foldcase(p, np - p, word, MAXWLEN + 1);
		    n = STRLEN(word);
		    if (n > np - p)
		    {
			sblock_T	*s;

			/* Folding case made word longer!  We need to allocate
			 * memory for it. */
			s = (sblock_T *)alloc((unsigned)sizeof(sblock_T)
								     + n + 1);
			if (s != NULL)
			{
			    s->sb_next = load_lp->sl_block;
			    load_lp->sl_block = s;
			    s->sb_data[0] = p[-1];
			    p = s->sb_data + 1;
			}
		    }
		    mch_memmove(p, word, n + 1);

		    hash = hash_hash(p);
		    hi = hash_lookup(&load_lp->sl_ht, p, hash);
		    if (!HASHITEM_EMPTY(hi))
		    {
			c = hi->hi_key[-1];
			if ((c & (CASE_MASK | RARE_MASK))
					 == (p[-1] & (CASE_MASK | RARE_MASK)))
			{
			    if (p_verbose > 0)
				smsg((char_u *)_("Warning: duplicate word \"%s\" in %s"),
								    p, fname);
			}
			else
			    hi->hi_key[-1] |= (p[-1] & (CASE_MASK | RARE_MASK));
		    }
		    else
			hash_add_item(&load_lp->sl_ht, hi, p, hash);
		}
	    }

	    while (l > 0 && *np < ' ')
	    {
		++np;
		--l;
	    }
	}
    }

    close(fd);
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

    /* Unload all allocated memory. */
    while (first_lang != NULL)
    {
	lp = first_lang;
	first_lang = lp->sl_next;

	hash_clear(&lp->sl_ht);
	ga_clear(&lp->sl_match);
	ga_clear(&lp->sl_add);
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
