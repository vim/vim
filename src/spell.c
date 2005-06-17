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
 *
 * A NUL byte is used where the word may end.  The bytes are sorted, so that
 * binary searching can be used and the NUL bytes are at the start.  The
 * number of possible bytes is stored before the list of bytes.
 *
 * The tree uses two arrays: "byts" stores the characters, "idxs" stores
 * either the next index or flags.  The tree starts at index 0.  For example,
 * to lookup "vi" this sequence is followed:
 *	i = 0
 *	len = byts[i]
 *	n = where "v" appears in byts[i + 1] to byts[i + len]
 *	i = idxs[n]
 *	len = byts[i]
 *	n = where "i" appears in byts[i + 1] to byts[i + len]
 *	i = idxs[n]
 *	len = byts[i]
 *	find that byts[i + 1] is 0, idxs[i + 1] has flags for "vi".
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
 * Use this to let the score depend in how much a suggestion sounds like the
 * bad word.  It's quite slow and only occasionally makes the sorting better.
#define SOUNDFOLD_SCORE
 */

/*
 * Use this to adjust the score after finding suggestions, based on the
 * suggested word sounding like the bad word.  This is much faster than doing
 * it for every possible suggestion.
 * Disadvantage: When "the" is typed as "hte" it sounds different and goes
 * down in the list.
#define RESCORE(word_score, sound_score) ((2 * word_score + sound_score) / 3)
 */

/*
 * Vim spell file format:  <HEADER> <SUGGEST> <LWORDTREE> <KWORDTREE>
 *
 * <HEADER>: <fileID> <regioncnt> <regionname> ...
 *		 <charflagslen> <charflags> <fcharslen> <fchars>
 *
 * <fileID>     10 bytes    "VIMspell06"
 * <regioncnt>  1 byte	    number of regions following (8 supported)
 * <regionname>	2 bytes     Region name: ca, au, etc.  Lower case.
 *			    First <regionname> is region 1.
 *
 * <charflagslen> 1 byte    Number of bytes in <charflags> (should be 128).
 * <charflags>  N bytes     List of flags (first one is for character 128):
 *			    0x01  word character	CF_WORD
 *			    0x02  upper-case character	CF_UPPER
 * <fcharslen>  2 bytes     Number of bytes in <fchars>.
 * <fchars>     N bytes	    Folded characters, first one is for character 128.
 *
 *
 * <SUGGEST> : <repcount> <rep> ...
 *	       <salflags> <salcount> <sal> ...
 *	       <maplen> <mapstr>
 *
 * <repcount>	2 bytes	    number of <rep> items, MSB first.
 *
 * <rep> : <repfromlen> <repfrom> <reptolen> <repto>
 *
 * <repfromlen>	1 byte	    length of <repfrom>
 *
 * <repfrom>	N bytes	    "from" part of replacement
 *
 * <reptolen>	1 byte	    length of <repto>
 *
 * <repto>	N bytes	    "to" part of replacement
 *
 * <salflags>	1 byte	    flags for soundsalike conversion:
 *			    SAL_F0LLOWUP
 *			    SAL_COLLAPSE
 *			    SAL_REM_ACCENTS
 *
 * <sal> : <salfromlen> <salfrom> <saltolen> <salto>
 *
 * <salfromlen>	1 byte	    length of <salfrom>
 *
 * <salfrom>	N bytes	    "from" part of soundsalike
 *
 * <saltolen>	1 byte	    length of <salto>
 *
 * <salto>	N bytes	    "to" part of soundsalike
 *
 * <maplen>	2 bytes	    length of <mapstr>, MSB first
 *
 * <mapstr>	N bytes	    String with sequences of similar characters,
 *			    separated by slashes.
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
 * All text characters are in 'encoding', but stored as single bytes.
 */

#if defined(MSDOS) || defined(WIN16) || defined(WIN32) || defined(_WIN64)
# include <io.h>	/* for lseek(), must be before vim.h */
#endif

#include "vim.h"

#if defined(FEAT_SYN_HL) || defined(PROTO)

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif

#define MAXWLEN 250		/* Assume max. word len is this many bytes.
				   Some places assume a word length fits in a
				   byte, thus it can't be above 255. */

/* Type used for indexes in the word tree need to be at least 3 bytes.  If int
 * is 8 bytes we could use something smaller, but what? */
#if SIZEOF_INT > 2
typedef int idx_T;
#else
typedef long idx_T;
#endif

/* Flags used for a word.  Only the lowest byte can be used, the region byte
 * comes above it. */
#define WF_REGION   0x01	/* region byte follows */
#define WF_ONECAP   0x02	/* word with one capital (or all capitals) */
#define WF_ALLCAP   0x04	/* word must be all capitals */
#define WF_RARE	    0x08	/* rare word */
#define WF_BANNED   0x10	/* bad word */
#define WF_KEEPCAP  0x80	/* keep-case word */

#define WF_CAPMASK (WF_ONECAP | WF_ALLCAP | WF_KEEPCAP)

#define BY_NOFLAGS  0		/* end of word without flags or region */
#define BY_FLAGS    1		/* end of word, flag byte follows */
#define BY_INDEX    2		/* child is shared, index follows */
#define BY_SPECIAL  BY_INDEX	/* hightest special byte value */

/* Info from "REP" and "SAL" entries in ".aff" file used in si_rep, sl_rep,
 * si_sal and sl_sal.
 * One replacement: from "ft_from" to "ft_to". */
typedef struct fromto_S
{
    char_u	*ft_from;
    char_u	*ft_to;
} fromto_T;

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
    char_u	*sl_fname;	/* name of .spl file */
    int		sl_add;		/* TRUE if it's a .add file. */
    char_u	*sl_fbyts;	/* case-folded word bytes */
    idx_T	*sl_fidxs;	/* case-folded word indexes */
    char_u	*sl_kbyts;	/* keep-case word bytes */
    idx_T	*sl_kidxs;	/* keep-case word indexes */
    char_u	sl_regions[17];	/* table with up to 8 region names plus NUL */

    garray_T	sl_rep;		/* list of fromto_T entries from REP lines */
    short	sl_rep_first[256];  /* indexes where byte first appears, -1 if
				       there is none */
    garray_T	sl_sal;		/* list of fromto_T entries from SAL lines */
    short	sl_sal_first[256];  /* indexes where byte first appears, -1 if
				       there is none */
    int		sl_followup;	/* SAL followup */
    int		sl_collapse;	/* SAL collapse_result */
    int		sl_rem_accents;	/* SAL remove_accents */
    int		sl_has_map;	/* TRUE if there is a MAP line */
#ifdef FEAT_MBYTE
    hashtab_T	sl_map_hash;	/* MAP for multi-byte chars */
    int		sl_map_array[256]; /* MAP for first 256 chars */
#else
    char_u	sl_map_array[256]; /* MAP for first 256 chars */
#endif
};

/* First language that is loaded, start of the linked list of loaded
 * languages. */
static slang_T *first_lang = NULL;

/* Flags used in .spl file for soundsalike flags. */
#define SAL_F0LLOWUP		1
#define SAL_COLLAPSE		2
#define SAL_REM_ACCENTS		4

/*
 * Structure used in "b_langp", filled from 'spelllang'.
 */
typedef struct langp_S
{
    slang_T	*lp_slang;	/* info for this language (NULL for last one) */
    int		lp_region;	/* bitmask for region or REGION_ALL */
} langp_T;

#define LANGP_ENTRY(ga, i)	(((langp_T *)(ga).ga_data) + (i))

#define REGION_ALL 0xff		/* word valid in all regions */

/* Result values.  Lower number is accepted over higher one. */
#define SP_BANNED	-1
#define SP_OK		0
#define SP_RARE		1
#define SP_LOCAL	2
#define SP_BAD		3

#define VIMSPELLMAGIC "VIMspell06"  /* string at start of Vim spell file */
#define VIMSPELLMAGICL 10

/*
 * Information used when looking for suggestions.
 */
typedef struct suginfo_S
{
    garray_T	su_ga;		    /* suggestions, contains "suggest_T" */
    int		su_maxscore;	    /* maximum score for adding to su_ga */
    int		su_icase;	    /* accept words with wrong case */
    int		su_icase_add;	    /* add matches while ignoring case */
    char_u	*su_badptr;	    /* start of bad word in line */
    int		su_badlen;	    /* length of detected bad word in line */
    char_u	su_badword[MAXWLEN]; /* bad word truncated at su_badlen */
    char_u	su_fbadword[MAXWLEN]; /* su_badword case-folded */
    hashtab_T	su_banned;	    /* table with banned words */
#ifdef SOUNDFOLD_SCORE
    slang_T	*su_slang;	    /* currently used slang_T */
    char_u	su_salword[MAXWLEN]; /* soundfolded badword */
#endif
} suginfo_T;

/* One word suggestion.  Used in "si_ga". */
typedef struct suggest_S
{
    char_u	*st_word;	/* suggested word, allocated string */
    int		st_orglen;	/* length of replaced text */
    int		st_score;	/* lower is better */
#ifdef RESCORE
    int		st_had_bonus;	/* bonus already included in score */
#endif
} suggest_T;

#define SUG(sup, i) (((suggest_T *)(sup)->su_ga.ga_data)[i])

/* Number of suggestions displayed. */
#define SUG_PROMPT_COUNT    ((int)Rows - 2)

/* Number of suggestions kept when cleaning up.  When rescore_suggestions() is
 * called the score may change, thus we need to keep more than what is
 * displayed. */
#define SUG_CLEAN_COUNT	    (SUG_PROMPT_COUNT < 25 ? 25 : SUG_PROMPT_COUNT)

/* Threshold for sorting and cleaning up suggestions.  Don't want to keep lots
 * of suggestions that are not going to be displayed. */
#define SUG_MAX_COUNT	    (SUG_PROMPT_COUNT + 50)

/* score for various changes */
#define SCORE_SPLIT	99	/* split bad word */
#define SCORE_ICASE	52	/* slightly different case */
#define SCORE_ALLCAP	120	/* need all-cap case */
#define SCORE_REGION	70	/* word is for different region */
#define SCORE_RARE	180	/* rare word */

/* score for edit distance */
#define SCORE_SWAP	90	/* swap two characters */
#define SCORE_SWAP3	110	/* swap two characters in three */
#define SCORE_REP	87	/* REP replacement */
#define SCORE_SUBST	93	/* substitute a character */
#define SCORE_SIMILAR	33	/* substitute a similar character */
#define SCORE_DEL	94	/* delete a character */
#define SCORE_INS	96	/* insert a character */

#define SCORE_MAXINIT	350	/* Initial maximum score: higher == slower.
				 * 350 allows for about three changes. */
#define SCORE_MAXMAX	999999	/* accept any score */

/*
 * Structure to store info for word matching.
 */
typedef struct matchinf_S
{
    langp_T	*mi_lp;			/* info for language and region */

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

/*
 * The tables used for recognizing word characters according to spelling.
 * These are only used for the first 256 characters of 'encoding'.
 */
typedef struct spelltab_S
{
    char_u  st_isw[256];	/* flags: is word char */
    char_u  st_isu[256];	/* flags: is uppercase char */
    char_u  st_fold[256];	/* chars: folded case */
    char_u  st_upper[256];	/* chars: upper case */
} spelltab_T;

static spelltab_T   spelltab;
static int	    did_set_spelltab;

#define CF_WORD		0x01
#define CF_UPPER	0x02

static void clear_spell_chartab __ARGS((spelltab_T *sp));
static int set_spell_finish __ARGS((spelltab_T	*new_st));

/*
 * Return TRUE if "p" points to a word character or "c" is a word character
 * for spelling.
 * Checking for a word character is done very often, avoid the function call
 * overhead.
 */
#ifdef FEAT_MBYTE
# define SPELL_ISWORDP(p) ((has_mbyte && MB_BYTE2LEN(*(p)) > 1) \
		? (mb_get_class(p) >= 2) : spelltab.st_isw[*(p)])
#else
# define SPELL_ISWORDP(p) (spelltab.st_isw[*(p)])
#endif

/*
 * For finding suggestion: At each node in the tree these states are tried:
 */
typedef enum
{
    STATE_START = 0,	/* At start of node, check if word may end or
			 * split word. */
    STATE_SPLITUNDO,	/* Undo word split. */
    STATE_ENDNUL,	/* Past NUL bytes at start of the node. */
    STATE_PLAIN,	/* Use each byte of the node. */
    STATE_DEL,		/* Delete a byte from the bad word. */
    STATE_INS,		/* Insert a byte in the bad word. */
    STATE_SWAP,		/* Swap two bytes. */
    STATE_UNSWAP,	/* Undo swap two bytes. */
    STATE_SWAP3,	/* Swap two bytes over three. */
    STATE_UNSWAP3,	/* Undo Swap two bytes over three. */
    STATE_ROT3L,	/* Rotate three bytes left */
    STATE_UNROT3L,	/* Undo rotate three bytes left */
    STATE_ROT3R,	/* Rotate three bytes right */
    STATE_UNROT3R,	/* Undo rotate three bytes right */
    STATE_REP_INI,	/* Prepare for using REP items. */
    STATE_REP,		/* Use matching REP items from the .aff file. */
    STATE_REP_UNDO,	/* Undo a REP item replacement. */
    STATE_FINAL		/* End of this node. */
} state_T;

/*
 * Struct to keep the state at each level in spell_try_change().
 */
typedef struct trystate_S
{
    state_T	ts_state;	/* state at this level, STATE_ */
    int		ts_score;	/* score */
    short	ts_curi;	/* index in list of child nodes */
    char_u	ts_fidx;	/* index in fword[], case-folded bad word */
    char_u	ts_fidxtry;	/* ts_fidx at which bytes may be changed */
    char_u	ts_twordlen;	/* valid length of tword[] */
#ifdef FEAT_MBYTE
    char_u	ts_tcharlen;	/* number of bytes in tword character */
    char_u	ts_tcharidx;	/* current byte index in tword character */
    char_u	ts_isdiff;	/* DIFF_ values */
    char_u	ts_fcharstart;	/* index in fword where badword char started */
#endif
    idx_T	ts_arridx;	/* index in tree array, start of node */
    char_u	ts_save_prewordlen; /* saved "prewordlen" */
    char_u	ts_save_splitoff;   /* su_splitoff saved here */
    char_u	ts_save_badflags;   /* badflags saved here */
} trystate_T;

/* values for ts_isdiff */
#define DIFF_NONE	0	/* no different byte (yet) */
#define DIFF_YES	1	/* different byte found */
#define DIFF_INSERT	2	/* inserting character */

static slang_T *slang_alloc __ARGS((char_u *lang));
static void slang_free __ARGS((slang_T *lp));
static void slang_clear __ARGS((slang_T *lp));
static void find_word __ARGS((matchinf_T *mip, int keepcap));
static int spell_valid_case __ARGS((int origflags, int treeflags));
static void spell_load_lang __ARGS((char_u *lang));
static char_u *spell_enc __ARGS((void));
static void spell_load_cb __ARGS((char_u *fname, void *cookie));
static slang_T *spell_load_file __ARGS((char_u *fname, char_u *lang, slang_T *old_lp, int silent));
static idx_T read_tree __ARGS((FILE *fd, char_u *byts, idx_T *idxs, int maxidx, int startidx));
static int find_region __ARGS((char_u *rp, char_u *region));
static int captype __ARGS((char_u *word, char_u *end));
static void spell_reload_one __ARGS((char_u *fname, int added_word));
static int set_spell_charflags __ARGS((char_u *flags, int cnt, char_u *upp));
static int set_spell_chartab __ARGS((char_u *fol, char_u *low, char_u *upp));
static void write_spell_chartab __ARGS((FILE *fd));
static int spell_casefold __ARGS((char_u *p, int len, char_u *buf, int buflen));
static void onecap_copy __ARGS((char_u *word, char_u *wcopy, int upper));
static void spell_try_change __ARGS((suginfo_T *su));
static int try_deeper __ARGS((suginfo_T *su, trystate_T *stack, int depth, int score_add));
static void find_keepcap_word __ARGS((slang_T *slang, char_u *fword, char_u *kword));
static void spell_try_soundalike __ARGS((suginfo_T *su));
static void make_case_word __ARGS((char_u *fword, char_u *cword, int flags));
static void set_map_str __ARGS((slang_T *lp, char_u *map));
static int similar_chars __ARGS((slang_T *slang, int c1, int c2));
#ifdef RESCORE
static void add_suggestion __ARGS((suginfo_T *su, char_u *goodword, int use_score, int had_bonus));
#else
static void add_suggestion __ARGS((suginfo_T *su, char_u *goodword, int use_score));
#endif
static void add_banned __ARGS((suginfo_T *su, char_u *word));
static int was_banned __ARGS((suginfo_T *su, char_u *word));
static void free_banned __ARGS((suginfo_T *su));
#ifdef RESCORE
static void rescore_suggestions __ARGS((suginfo_T *su));
#endif
static void cleanup_suggestions __ARGS((suginfo_T *su, int keep));
static void spell_soundfold __ARGS((slang_T *slang, char_u *inword, char_u *res));
#if defined(RESCORE) || defined(SOUNDFOLD_SCORE)
static int spell_sound_score __ARGS((slang_T *slang, char_u *goodword, char_u	*badsound));
#endif
static int spell_edit_score __ARGS((char_u *badword, char_u *goodword));

/*
 * Use our own character-case definitions, because the current locale may
 * differ from what the .spl file uses.
 * These must not be called with negative number!
 */
#ifndef FEAT_MBYTE
/* Non-multi-byte implementation. */
# define SPELL_TOFOLD(c) ((c) < 256 ? spelltab.st_fold[c] : (c))
# define SPELL_TOUPPER(c) ((c) < 256 ? spelltab.st_upper[c] : (c))
# define SPELL_ISUPPER(c) ((c) < 256 ? spelltab.st_isu[c] : FALSE)
#else
/* Multi-byte implementation.  For Unicode we can call utf_*(), but don't do
 * that for ASCII, because we don't want to use 'casemap' here.  Otherwise use
 * the "w" library function for characters above 255 if available. */
# ifdef HAVE_TOWLOWER
#  define SPELL_TOFOLD(c) (enc_utf8 && (c) >= 128 ? utf_fold(c) \
	    : (c) < 256 ? spelltab.st_fold[c] : towlower(c))
# else
#  define SPELL_TOFOLD(c) (enc_utf8 && (c) >= 128 ? utf_fold(c) \
	    : (c) < 256 ? spelltab.st_fold[c] : (c))
# endif

# ifdef HAVE_TOWUPPER
#  define SPELL_TOUPPER(c) (enc_utf8 && (c) >= 128 ? utf_toupper(c) \
	    : (c) < 256 ? spelltab.st_upper[c] : towupper(c))
# else
#  define SPELL_TOUPPER(c) (enc_utf8 && (c) >= 128 ? utf_toupper(c) \
	    : (c) < 256 ? spelltab.st_upper[c] : (c))
# endif

# ifdef HAVE_ISWUPPER
#  define SPELL_ISUPPER(c) (enc_utf8 && (c) >= 128 ? utf_isupper(c) \
	    : (c) < 256 ? spelltab.st_isu[c] : iswupper(c))
# else
#  define SPELL_ISUPPER(c) (enc_utf8 && (c) >= 128 ? utf_isupper(c) \
	    : (c) < 256 ? spelltab.st_isu[c] : (c))
# endif
#endif


static char *e_format = N_("E759: Format error in spell file");

/*
 * Main spell-checking function.
 * "ptr" points to a character that could be the start of a word.
 * "*attrp" is set to the attributes for a badly spelled word.  For a non-word
 * or when it's OK it remains unchanged.
 * This must only be called when 'spelllang' is not empty.
 *
 * "sug" is normally NULL.  When looking for suggestions it points to
 * suginfo_T.  It's passed as a void pointer to keep the struct local.
 *
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

    /* A word never starts at a space or a control character.  Return quickly
     * then, skipping over the character. */
    if (*ptr <= ' ')
	return 1;

    /* A word starting with a number is always OK.  Also skip hexadecimal
     * numbers 0xFF99 and 0X99FF. */
    if (*ptr >= '0' && *ptr <= '9')
    {
	if (*ptr == '0' && (ptr[1] == 'x' || ptr[1] == 'X'))
	    mi.mi_end = skiphex(ptr + 2);
	else
	    mi.mi_end = skipdigits(ptr);
    }
    else
    {
	/* Find the end of the word. */
	mi.mi_word = ptr;
	mi.mi_fend = ptr;

	if (SPELL_ISWORDP(mi.mi_fend))
	{
	    /* Make case-folded copy of the characters until the next non-word
	     * character. */
	    do
	    {
		mb_ptr_adv(mi.mi_fend);
	    } while (*mi.mi_fend != NUL && SPELL_ISWORDP(mi.mi_fend));
	}

	/* We always use the characters up to the next non-word character,
	 * also for bad words. */
	mi.mi_end = mi.mi_fend;

	/* Check caps type later. */
	mi.mi_capflags = 0;
	mi.mi_cend = NULL;

	/* Include one non-word character so that we can check for the
	 * word end. */
	if (*mi.mi_fend != NUL)
	    mb_ptr_adv(mi.mi_fend);

	(void)spell_casefold(ptr, (int)(mi.mi_fend - ptr), mi.mi_fword,
								 MAXWLEN + 1);
	mi.mi_fwordlen = STRLEN(mi.mi_fword);

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

	    /* Check for a matching word in keep-case words. */
	    find_word(&mi, TRUE);
	}

	if (mi.mi_result != SP_OK)
	{
	    /* When we are at a non-word character there is no error, just
	     * skip over the character (try looking for a word after it). */
	    if (!SPELL_ISWORDP(ptr))
	    {
#ifdef FEAT_MBYTE
		if (has_mbyte)
		    return mb_ptr2len_check(ptr);
#endif
		return 1;
	    }

	    if (mi.mi_result == SP_BAD || mi.mi_result == SP_BANNED)
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
    idx_T	arridx = 0;
    int		endlen[MAXWLEN];    /* length at possible word endings */
    idx_T	endidx[MAXWLEN];    /* possible word endings */
    int		endidxcnt = 0;
    int		len;
    int		wlen = 0;
    int		flen;
    int		c;
    char_u	*ptr;
    idx_T	lo, hi, m;
#ifdef FEAT_MBYTE
    char_u	*s;
#endif
    char_u	*p;
    int		res = SP_BAD;
    int		valid;
    slang_T	*slang = mip->mi_lp->lp_slang;
    unsigned	flags;
    char_u	*byts;
    idx_T	*idxs;

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
     * Repeat advancing in the tree until:
     * - there is a byte that doesn't match,
     * - we reach the end of the tree,
     * - or we reach the end of the line.
     */
    for (;;)
    {
	if (flen == 0 && *mip->mi_fend != NUL)
	{
	    /* Need to fold at least one more character.  Do until next
	     * non-word character for efficiency. */
	    p = mip->mi_fend;
	    do
	    {
		mb_ptr_adv(mip->mi_fend);
	    } while (*mip->mi_fend != NUL && SPELL_ISWORDP(mip->mi_fend));

	    /* Include the non-word character so that we can check for the
	     * word end. */
	    if (*mip->mi_fend != NUL)
		mb_ptr_adv(mip->mi_fend);

	    (void)spell_casefold(p, (int)(mip->mi_fend - p),
				     mip->mi_fword + mip->mi_fwordlen,
				     MAXWLEN - mip->mi_fwordlen);
	    flen = STRLEN(mip->mi_fword + mip->mi_fwordlen);
	    mip->mi_fwordlen += flen;
	}

	len = byts[arridx++];

	/* If the first possible byte is a zero the word could end here.
	 * Remember this index, we first check for the longest word. */
	if (byts[arridx] == 0)
	{
	    if (endidxcnt == MAXWLEN)
	    {
		/* Must be a corrupted spell file. */
		EMSG(_(e_format));
		return;
	    }
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
	if (SPELL_ISWORDP(ptr + wlen))
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
		    /* mi_capflags was set for a different word length, need
		     * to do it again. */
		    mip->mi_cend = mip->mi_word + wlen;
		    mip->mi_capflags = captype(mip->mi_word, mip->mi_cend);
		}

		valid = spell_valid_case(mip->mi_capflags, flags);
	    }

	    if (valid)
	    {
		if (flags & WF_BANNED)
		    res = SP_BANNED;
		else if (flags & WF_REGION)
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

		/* Always use the longest match and the best result. */
		if (mip->mi_result > res)
		{
		    mip->mi_result = res;
		    mip->mi_end = mip->mi_word + wlen;
		}
		else if (mip->mi_result == res
					 && mip->mi_end < mip->mi_word + wlen)
		    mip->mi_end = mip->mi_word + wlen;

		if (res == SP_OK)
		    break;
	    }
	    else
		res = SP_BAD;

	    ++arridx;
	}

	if (res == SP_OK)
	    break;
    }
}

/*
 * Check case flags for a word.  Return TRUE if the word has the requested
 * case.
 */
    static int
spell_valid_case(origflags, treeflags)
    int	    origflags;	    /* flags for the checked word. */
    int	    treeflags;	    /* flags for the word in the spell tree */
{
    return (origflags == WF_ALLCAP
	    || ((treeflags & (WF_ALLCAP | WF_KEEPCAP)) == 0
		&& ((treeflags & WF_ONECAP) == 0 || origflags == WF_ONECAP)));
}


/*
 * Move to next spell error.
 * "curline" is TRUE for "z?": find word under/after cursor in the same line.
 * Return OK if found, FAIL otherwise.
 */
    int
spell_move_to(dir, allwords, curline)
    int		dir;		/* FORWARD or BACKWARD */
    int		allwords;	/* TRUE for "[s" and "]s" */
    int		curline;
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

    if (!curwin->w_p_spell || *curbuf->b_p_spl == NUL)
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
		if (allwords || attr == highlight_attr[HLF_SPB])
		{
		    /* When searching forward only accept a bad word after
		     * the cursor. */
		    if (dir == BACKWARD
			    || lnum > curwin->w_cursor.lnum
			    || (lnum == curwin->w_cursor.lnum
				&& (colnr_T)(curline ? p - line + len
						     : p - line)
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

	if (curline)
	    return FAIL;	/* only check cursor line */

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
 * Load word list(s) for "lang" from Vim spell file(s).
 * "lang" must be the language without the region: e.g., "en".
 */
    static void
spell_load_lang(lang)
    char_u	*lang;
{
    char_u	fname_enc[85];
    int		r;
    char_u	langcp[MAXWLEN + 1];

    /* Copy the language name to pass it to spell_load_cb() as a cookie.
     * It's truncated when an error is detected. */
    STRCPY(langcp, lang);

    /*
     * Find the first spell file for "lang" in 'runtimepath' and load it.
     */
    vim_snprintf((char *)fname_enc, sizeof(fname_enc) - 5,
					"spell/%s.%s.spl", lang, spell_enc());
    r = do_in_runtimepath(fname_enc, FALSE, spell_load_cb, &langcp);

    if (r == FAIL && *langcp != NUL)
    {
	/* Try loading the ASCII version. */
	vim_snprintf((char *)fname_enc, sizeof(fname_enc) - 5,
						  "spell/%s.ascii.spl", lang);
	r = do_in_runtimepath(fname_enc, FALSE, spell_load_cb, &langcp);
    }

    if (r == FAIL)
	smsg((char_u *)_("Warning: Cannot find word list \"%s\""),
							       fname_enc + 6);
    else if (*langcp != NUL)
    {
	/* Load all the additions. */
	STRCPY(fname_enc + STRLEN(fname_enc) - 3, "add.spl");
	do_in_runtimepath(fname_enc, TRUE, spell_load_cb, &langcp);
    }
}

/*
 * Return the encoding used for spell checking: Use 'encoding', except that we
 * use "latin1" for "latin9".  And limit to 60 characters (just in case).
 */
    static char_u *
spell_enc()
{

#ifdef FEAT_MBYTE
    if (STRLEN(p_enc) < 60 && STRCMP(p_enc, "iso-8859-15") != 0)
	return p_enc;
#endif
    return (char_u *)"latin1";
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
	ga_init2(&lp->sl_rep, sizeof(fromto_T), 10);
	ga_init2(&lp->sl_sal, sizeof(fromto_T), 10);
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
    vim_free(lp->sl_fname);
    slang_clear(lp);
    vim_free(lp);
}

/*
 * Clear an slang_T so that the file can be reloaded.
 */
    static void
slang_clear(lp)
    slang_T	*lp;
{
    garray_T	    *gap;
    fromto_T	    *ftp;
    int		    round;

    vim_free(lp->sl_fbyts);
    lp->sl_fbyts = NULL;
    vim_free(lp->sl_kbyts);
    lp->sl_kbyts = NULL;
    vim_free(lp->sl_fidxs);
    lp->sl_fidxs = NULL;
    vim_free(lp->sl_kidxs);
    lp->sl_kidxs = NULL;

    for (round = 1; round <= 2; ++round)
    {
	gap = round == 1 ? &lp->sl_rep : &lp->sl_sal;
	while (gap->ga_len > 0)
	{
	    ftp = &((fromto_T *)gap->ga_data)[--gap->ga_len];
	    vim_free(ftp->ft_from);
	    vim_free(ftp->ft_to);
	}
	ga_clear(gap);
    }

#ifdef FEAT_MBYTE
    {
	int	    todo = lp->sl_map_hash.ht_used;
	hashitem_T  *hi;

	for (hi = lp->sl_map_hash.ht_array; todo > 0; ++hi)
	    if (!HASHITEM_EMPTY(hi))
	    {
		--todo;
		vim_free(hi->hi_key);
	    }
    }
    hash_clear(&lp->sl_map_hash);
#endif
}

/*
 * Load one spell file and store the info into a slang_T.
 * Invoked through do_in_runtimepath().
 */
    static void
spell_load_cb(fname, cookie)
    char_u	*fname;
    void	*cookie;	    /* points to the language name */
{
    (void)spell_load_file(fname, (char_u *)cookie, NULL, FALSE);
}

/*
 * Load one spell file and store the info into a slang_T.
 *
 * This is invoked in two ways:
 * - From spell_load_cb() to load a spell file for the first time.  "lang" is
 *   the language name, "old_lp" is NULL.  Will allocate an slang_T.
 * - To reload a spell file that was changed.  "lang" is NULL and "old_lp"
 *   points to the existing slang_T.
 * Returns the slang_T the spell file was loaded into.  NULL for error.
 */
    static slang_T *
spell_load_file(fname, lang, old_lp, silent)
    char_u	*fname;
    char_u	*lang;
    slang_T	*old_lp;
    int		silent;		/* no error if file doesn't exist */
{
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
    slang_T	*lp = NULL;
    garray_T	*gap;
    fromto_T	*ftp;
    int		rr;
    short	*first;
    idx_T	idx;

    fd = mch_fopen((char *)fname, "r");
    if (fd == NULL)
    {
	if (!silent)
	    EMSG2(_(e_notopen), fname);
	else if (p_verbose > 2)
	{
	    verbose_enter();
	    smsg((char_u *)e_notopen, fname);
	    verbose_leave();
	}
	goto endFAIL;
    }
    if (p_verbose > 2)
    {
	verbose_enter();
	smsg((char_u *)_("Reading spell file \"%s\""), fname);
	verbose_leave();
    }

    if (old_lp == NULL)
    {
	lp = slang_alloc(lang);
	if (lp == NULL)
	    goto endFAIL;

	/* Remember the file name, used to reload the file when it's updated. */
	lp->sl_fname = vim_strsave(fname);
	if (lp->sl_fname == NULL)
	    goto endFAIL;

	/* Check for .add.spl. */
	lp->sl_add = strstr((char *)gettail(fname), ".add.") != NULL;
    }
    else
	lp = old_lp;

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
	EMSG(_(e_format));
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

	/* Set the word-char flags and fill SPELL_ISUPPER() table. */
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

    /* <SUGGEST> : <repcount> <rep> ...
     *             <salflags> <salcount> <sal> ...
     *             <maplen> <mapstr> */
    for (round = 1; round <= 2; ++round)
    {
	if (round == 1)
	{
	    gap = &lp->sl_rep;
	    first = lp->sl_rep_first;
	}
	else
	{
	    gap = &lp->sl_sal;
	    first = lp->sl_sal_first;

	    i = getc(fd);				/* <salflags> */
	    if (i & SAL_F0LLOWUP)
		lp->sl_followup = TRUE;
	    if (i & SAL_COLLAPSE)
		lp->sl_collapse = TRUE;
	    if (i & SAL_REM_ACCENTS)
		lp->sl_rem_accents = TRUE;
	}

	cnt = (getc(fd) << 8) + getc(fd);	/* <repcount> or <salcount> */
	if (cnt < 0)
	    goto formerr;

	if (ga_grow(gap, cnt) == FAIL)
	    goto endFAIL;
	for (; gap->ga_len < cnt; ++gap->ga_len)
	{
	    /* <rep> : <repfromlen> <repfrom> <reptolen> <repto> */
	    /* <sal> : <salfromlen> <salfrom> <saltolen> <salto> */
	    ftp = &((fromto_T *)gap->ga_data)[gap->ga_len];
	    for (rr = 1; rr <= 2; ++rr)
	    {
		ccnt = getc(fd);
		if (ccnt < 0)
		{
		    if (rr == 2)
			vim_free(ftp->ft_from);
		    goto formerr;
		}
		if ((p = alloc(ccnt + 1)) == NULL)
		{
		    if (rr == 2)
			vim_free(ftp->ft_from);
		    goto endFAIL;
		}
		for (i = 0; i < ccnt; ++i)
		    p[i] = getc(fd);	/* <repfrom> or <salfrom> */
		p[i] = NUL;
		if (rr == 1)
		    ftp->ft_from = p;
		else
		    ftp->ft_to = p;
	    }
	}

	/* Fill the first-index table. */
	for (i = 0; i < 256; ++i)
	    first[i] = -1;
	for (i = 0; i < gap->ga_len; ++i)
	{
	    ftp = &((fromto_T *)gap->ga_data)[i];
	    if (first[*ftp->ft_from] == -1)
		first[*ftp->ft_from] = i;
	}
    }

    cnt = (getc(fd) << 8) + getc(fd);		/* <maplen> */
    if (cnt < 0)
	goto formerr;
    p = alloc(cnt + 1);
    if (p == NULL)
	goto endFAIL;
    for (i = 0; i < cnt; ++i)
	p[i] = getc(fd);			/* <mapstr> */
    p[i] = NUL;
    set_map_str(lp, p);
    vim_free(p);


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
		lp->sl_fidxs = (idx_T *)p;
	    else
		lp->sl_kidxs = (idx_T *)p;


	    /* Read the tree and store it in the array. */
	    idx = read_tree(fd,
			round == 1 ? lp->sl_fbyts : lp->sl_kbyts,
			round == 1 ? lp->sl_fidxs : lp->sl_kidxs,
			len, 0);
	    if (idx == -1)
		goto truncerr;
	    if (idx < 0)
		goto formerr;
	}
    }

    /* For a new file link it in the list of spell files. */
    if (old_lp == NULL)
    {
	lp->sl_next = first_lang;
	first_lang = lp;
    }

    goto endOK;

endFAIL:
    if (lang != NULL)
	/* truncating the name signals the error to spell_load_lang() */
	*lang = NUL;
    if (lp != NULL && old_lp == NULL)
    {
	slang_free(lp);
	lp = NULL;
    }

endOK:
    if (fd != NULL)
	fclose(fd);
    sourcing_name = save_sourcing_name;
    sourcing_lnum = save_sourcing_lnum;

    return lp;
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
    static idx_T
read_tree(fd, byts, idxs, maxidx, startidx)
    FILE	*fd;
    char_u	*byts;
    idx_T	*idxs;
    int		maxidx;		    /* size of arrays */
    idx_T	startidx;	    /* current index in "byts" and "idxs" */
{
    int		len;
    int		i;
    int		n;
    idx_T	idx = startidx;
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
    char_u	spf_name[MAXPATHL];
    int		did_spf = FALSE;

    ga_init2(&ga, sizeof(langp_T), 2);

    /* Get the name of the .spl file associated with 'spellfile'. */
    if (*buf->b_p_spf == NUL)
	did_spf = TRUE;
    else
	vim_snprintf((char *)spf_name, sizeof(spf_name), "%s.spl",
								buf->b_p_spf);

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

	/* Check if we loaded this language before. */
	for (lp = first_lang; lp != NULL; lp = lp->sl_next)
	    if (STRNICMP(lp->sl_name, lang, 2) == 0)
		break;

	if (lp == NULL)
	{
	    /* Not found, load the language. */
	    vim_strncpy(lbuf, lang, e - lang);
	    if (region != NULL)
		mch_memmove(lbuf + 2, lbuf + 5, e - lang - 4);
	    spell_load_lang(lbuf);
	}

	/*
	 * Loop over the languages, there can be several files for each.
	 */
	for (lp = first_lang; lp != NULL; lp = lp->sl_next)
	    if (STRNICMP(lp->sl_name, lang, 2) == 0)
	    {
		region_mask = REGION_ALL;
		if (region != NULL)
		{
		    /* find region in sl_regions */
		    c = find_region(lp->sl_regions, region);
		    if (c == REGION_ALL)
		    {
			if (!lp->sl_add)
			{
			    c = *e;
			    *e = NUL;
			    smsg((char_u *)_("Warning: region %s not supported"),
									lang);
			    *e = c;
			}
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

		/* Check if this is the 'spellfile' spell file. */
		if (fullpathcmp(spf_name, lp->sl_fname, FALSE) == FPC_SAME)
		    did_spf = TRUE;
	    }

	if (*e == ',')
	    ++e;
    }

    /*
     * Make sure the 'spellfile' file is loaded.  It may be in 'runtimepath',
     * then it's probably loaded above already.  Otherwise load it here.
     */
    if (!did_spf)
    {
	for (lp = first_lang; lp != NULL; lp = lp->sl_next)
	    if (fullpathcmp(spf_name, lp->sl_fname, FALSE) == FPC_SAME)
		break;
	if (lp == NULL)
	{
	    vim_strncpy(lbuf, gettail(spf_name), 2);
	    lp = spell_load_file(spf_name, lbuf, NULL, TRUE);
	}
	if (lp != NULL && ga_grow(&ga, 1) == OK)
	{
	    LANGP_ENTRY(ga, ga.ga_len)->lp_slang = lp;
	    LANGP_ENTRY(ga, ga.ga_len)->lp_region = REGION_ALL;
	    ++ga.ga_len;
	}
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
 * Return case type of word:
 * w word	0
 * Word		WF_ONECAP
 * W WORD	WF_ALLCAP
 * WoRd	wOrd	WF_KEEPCAP
 */
    static int
captype(word, end)
    char_u	*word;
    char_u	*end;	    /* When NULL use up to NUL byte. */
{
    char_u	*p;
    int		c;
    int		firstcap;
    int		allcap;
    int		past_second = FALSE;	/* past second word char */

    /* find first letter */
    for (p = word; !SPELL_ISWORDP(p); mb_ptr_adv(p))
	if (end == NULL ? *p == NUL : p >= end)
	    return 0;	    /* only non-word characters, illegal word */
#ifdef FEAT_MBYTE
    if (has_mbyte)
	c = mb_ptr2char_adv(&p);
    else
#endif
	c = *p++;
    firstcap = allcap = SPELL_ISUPPER(c);

    /*
     * Need to check all letters to find a word with mixed upper/lower.
     * But a word with an upper char only at start is a ONECAP.
     */
    for ( ; end == NULL ? *p != NUL : p < end; mb_ptr_adv(p))
	if (SPELL_ISWORDP(p))
	{
#ifdef FEAT_MBYTE
	    c = mb_ptr2char(p);
#else
	    c = *p;
#endif
	    if (!SPELL_ISUPPER(c))
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
 * Used after 'encoding' is set and when ":mkspell" was used.
 */
    void
spell_reload()
{
    buf_T	*buf;
    slang_T	*lp;
    win_T	*wp;

    /* Initialize the table for SPELL_ISWORDP(). */
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

	/* Only load the wordlists when 'spelllang' is set and there is a
	 * window for this buffer in which 'spell' is set. */
	if (*buf->b_p_spl != NUL)
	{
	    FOR_ALL_WINDOWS(wp)
		if (wp->w_buffer == buf && wp->w_p_spell)
		{
		    (void)did_set_spelllang(buf);
# ifdef FEAT_WINDOWS
		    break;
# endif
		}
	}
    }
}
# endif

/*
 * Reload the spell file "fname" if it's loaded.
 */
    static void
spell_reload_one(fname, added_word)
    char_u	*fname;
    int		added_word;	/* invoked through "zg" */
{
    slang_T	*lp;
    int		didit = FALSE;

    for (lp = first_lang; lp != NULL; lp = lp->sl_next)
	if (fullpathcmp(fname, lp->sl_fname, FALSE) == FPC_SAME)
	{
	    slang_clear(lp);
	    (void)spell_load_file(fname, NULL, lp, FALSE);
	    redraw_all_later(NOT_VALID);
	    didit = TRUE;
	}

    /* When "zg" was used and the file wasn't loaded yet, should redo
     * 'spelllang' to get it loaded. */
    if (added_word && !didit)
	did_set_spelllang(curbuf);
}


/*
 * Functions for ":mkspell".
 */

#define MAXLINELEN  500		/* Maximum length in bytes of a line in a .aff
				   and .dic file. */
/*
 * Main structure to store the contents of a ".aff" file.
 */
typedef struct afffile_S
{
    char_u	*af_enc;	/* "SET", normalized, alloc'ed string or NULL */
    int		af_rar;		/* RAR ID for rare word */
    int		af_kep;		/* KEP ID for keep-case word */
    hashtab_T	af_pref;	/* hashtable for prefixes, affheader_T */
    hashtab_T	af_suff;	/* hashtable for suffixes, affheader_T */
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
    long	si_foldwcount;	/* nr of words in si_foldroot */
    wordnode_T	*si_keeproot;	/* tree with keep-case words */
    long	si_keepwcount;	/* nr of words in si_keeproot */
    sblock_T	*si_blocks;	/* memory blocks used */
    int		si_ascii;	/* handling only ASCII words */
    int		si_add;		/* addition file */
    int		si_region;	/* region mask */
    vimconv_T	si_conv;	/* for conversion to 'encoding' */
    int		si_memtot;	/* runtime memory used */
    int		si_verbose;	/* verbose messages */
    int		si_region_count; /* number of regions supported (1 when there
				    are no regions) */
    char_u	si_region_name[16]; /* region names (if count > 1) */

    garray_T	si_rep;		/* list of fromto_T entries from REP lines */
    garray_T	si_sal;		/* list of fromto_T entries from SAL lines */
    int		si_followup;	/* soundsalike: ? */
    int		si_collapse;	/* soundsalike: ? */
    int		si_rem_accents;	/* soundsalike: remove accents */
    garray_T	si_map;		/* MAP info concatenated */
} spellinfo_T;

static afffile_T *spell_read_aff __ARGS((char_u *fname, spellinfo_T *spin));
static void add_fromto __ARGS((spellinfo_T *spin, garray_T *gap, char_u	*from, char_u *to));
static int sal_to_bool __ARGS((char_u *s));
static int has_non_ascii __ARGS((char_u *s));
static void spell_free_aff __ARGS((afffile_T *aff));
static int spell_read_dic __ARGS((char_u *fname, spellinfo_T *spin, afffile_T *affile));
static int store_aff_word __ARGS((char_u *word, spellinfo_T *spin, char_u *afflist, hashtab_T *ht, hashtab_T *xht, int comb, int flags));
static int spell_read_wordfile __ARGS((char_u *fname, spellinfo_T *spin));
static void *getroom __ARGS((sblock_T **blp, size_t len));
static char_u *getroom_save __ARGS((sblock_T **blp, char_u *s));
static void free_blocks __ARGS((sblock_T *bl));
static wordnode_T *wordtree_alloc __ARGS((sblock_T **blp));
static int store_word __ARGS((char_u *word, spellinfo_T *spin, int flags, int region));
static int tree_add_word __ARGS((char_u *word, wordnode_T *tree, int flags, int region, sblock_T **blp));
static void wordtree_compress __ARGS((wordnode_T *root, spellinfo_T *spin));
static int node_compress __ARGS((wordnode_T *node, hashtab_T *ht, int *tot));
static int node_equal __ARGS((wordnode_T *n1, wordnode_T *n2));
static void write_vim_spell __ARGS((char_u *fname, spellinfo_T *spin));
static int put_tree __ARGS((FILE *fd, wordnode_T *node, int index, int regionmask));
static void mkspell __ARGS((int fcount, char_u **fnames, int ascii, int overwrite, int added_word));
static void init_spellfile __ARGS((void));

/*
 * Read the affix file "fname".
 * Returns an afffile_T, NULL for complete failure.
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
#define MAXITEMCNT  7
    char_u	*(items[MAXITEMCNT]);
    int		itemcnt;
    char_u	*p;
    int		lnum = 0;
    affheader_T	*cur_aff = NULL;
    int		aff_todo = 0;
    hashtab_T	*tp;
    char_u	*low = NULL;
    char_u	*fol = NULL;
    char_u	*upp = NULL;
    static char *e_affname = N_("Affix name too long in %s line %d: %s");
    int		do_rep;
    int		do_sal;
    int		do_map;
    int		found_map = FALSE;
    hashitem_T	*hi;

    /*
     * Open the file.
     */
    fd = mch_fopen((char *)fname, "r");
    if (fd == NULL)
    {
	EMSG2(_(e_notopen), fname);
	return NULL;
    }

    if (spin->si_verbose || p_verbose > 2)
    {
	if (!spin->si_verbose)
	    verbose_enter();
	smsg((char_u *)_("Reading affix file %s..."), fname);
	out_flush();
	if (!spin->si_verbose)
	    verbose_leave();
    }

    /* Only do REP lines when not done in another .aff file already. */
    do_rep = spin->si_rep.ga_len == 0;

    /* Only do SAL lines when not done in another .aff file already. */
    do_sal = spin->si_sal.ga_len == 0;

    /* Only do MAP lines when not done in another .aff file already. */
    do_map = spin->si_map.ga_len == 0;

    /*
     * Allocate and init the afffile_T structure.
     */
    aff = (afffile_T *)getroom(&spin->si_blocks, sizeof(afffile_T));
    if (aff == NULL)
	return NULL;
    hash_init(&aff->af_pref);
    hash_init(&aff->af_suff);

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
#ifdef FEAT_MBYTE
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
#endif
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
	    if (itemcnt == MAXITEMCNT)	    /* too many items */
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
#ifdef FEAT_MBYTE
		/* Setup for conversion from "ENC" to 'encoding'. */
		aff->af_enc = enc_canonize(items[1]);
		if (aff->af_enc != NULL && !spin->si_ascii
			&& convert_setup(&spin->si_conv, aff->af_enc,
							       p_enc) == FAIL)
		    smsg((char_u *)_("Conversion in %s not supported: from %s to %s"),
					       fname, aff->af_enc, p_enc);
#else
		    smsg((char_u *)_("Conversion in %s not supported"), fname);
#endif
	    }
	    else if (STRCMP(items[0], "NOSPLITSUGS") == 0 && itemcnt == 1)
	    {
		/* ignored, we always split */
	    }
	    else if (STRCMP(items[0], "TRY") == 0 && itemcnt == 2)
	    {
		/* ignored, we look in the tree for what chars may appear */
	    }
	    else if (STRCMP(items[0], "RAR") == 0 && itemcnt == 2
						       && aff->af_rar == 0)
	    {
		aff->af_rar = items[1][0];
		if (items[1][1] != NUL)
		    smsg((char_u *)_(e_affname), fname, lnum, items[1]);
	    }
	    else if (STRCMP(items[0], "KEP") == 0 && itemcnt == 2
						       && aff->af_kep == 0)
	    {
		aff->af_kep = items[1][0];
		if (items[1][1] != NUL)
		    smsg((char_u *)_(e_affname), fname, lnum, items[1]);
	    }
	    else if ((STRCMP(items[0], "PFX") == 0
					      || STRCMP(items[0], "SFX") == 0)
		    && aff_todo == 0
		    && itemcnt >= 4)
	    {
		/* Myspell allows extra text after the item, but that might
		 * mean mistakes go unnoticed.  Require a comment-starter. */
		if (itemcnt > 4 && *items[4] != '#')
		    smsg((char_u *)_("Trailing text in %s line %d: %s"),
						       fname, lnum, items[4]);

		/* New affix letter. */
		cur_aff = (affheader_T *)getroom(&spin->si_blocks,
							 sizeof(affheader_T));
		if (cur_aff == NULL)
		    break;
		cur_aff->ah_key[0] = *items[1];
		cur_aff->ah_key[1] = NUL;
		if (items[1][1] != NUL)
		    smsg((char_u *)_(e_affname), fname, lnum, items[1]);
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
		hi = hash_find(tp, cur_aff->ah_key);
		if (!HASHITEM_EMPTY(hi))
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
		    && itemcnt >= 5)
	    {
		affentry_T	*aff_entry;

		/* Myspell allows extra text after the item, but that might
		 * mean mistakes go unnoticed.  Require a comment-starter. */
		if (itemcnt > 5 && *items[5] != '#')
		    smsg((char_u *)_("Trailing text in %s line %d: %s"),
						       fname, lnum, items[5]);

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
	    {
		/* Ignore REP count */;
		if (!isdigit(*items[1]))
		    smsg((char_u *)_("Expected REP count in %s line %d"),
								 fname, lnum);
	    }
	    else if (STRCMP(items[0], "REP") == 0 && itemcnt == 3)
	    {
		/* REP item */
		if (do_rep)
		    add_fromto(spin, &spin->si_rep, items[1], items[2]);
	    }
	    else if (STRCMP(items[0], "MAP") == 0 && itemcnt == 2)
	    {
		/* MAP item or count */
		if (!found_map)
		{
		    /* First line contains the count. */
		    found_map = TRUE;
		    if (!isdigit(*items[1]))
			smsg((char_u *)_("Expected MAP count in %s line %d"),
								 fname, lnum);
		}
		else if (do_map)
		{
		    /* We simply concatenate all the MAP strings, separated by
		     * slashes. */
		    ga_concat(&spin->si_map, items[1]);
		    ga_append(&spin->si_map, '/');
		}
	    }
	    else if (STRCMP(items[0], "SAL") == 0 && itemcnt == 3)
	    {
		if (do_sal)
		{
		    /* SAL item (sounds-a-like)
		     * Either one of the known keys or a from-to pair. */
		    if (STRCMP(items[1], "followup") == 0)
			spin->si_followup = sal_to_bool(items[2]);
		    else if (STRCMP(items[1], "collapse_result") == 0)
			spin->si_collapse = sal_to_bool(items[2]);
		    else if (STRCMP(items[1], "remove_accents") == 0)
			spin->si_rem_accents = sal_to_bool(items[2]);
		    else
			/* when "to" is "_" it means empty */
			add_fromto(spin, &spin->si_sal, items[1],
				     STRCMP(items[2], "_") == 0 ? (char_u *)""
								: items[2]);
		}
	    }
	    else
		smsg((char_u *)_("Unrecognized item in %s line %d: %s"),
						       fname, lnum, items[0]);
	}
    }

    if (fol != NULL || low != NULL || upp != NULL)
    {
	/*
	 * Don't write a word table for an ASCII file, so that we don't check
	 * for conflicts with a word table that matches 'encoding'.
	 * Don't write one for utf-8 either, we use utf_*() and
	 * mb_get_class(), the list of chars in the file will be incomplete.
	 */
	if (!spin->si_ascii
#ifdef FEAT_MBYTE
		&& !enc_utf8
#endif
		)
	{
	    if (fol == NULL || low == NULL || upp == NULL)
		smsg((char_u *)_("Missing FOL/LOW/UPP line in %s"), fname);
	    else
		(void)set_spell_chartab(fol, low, upp);
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
 * Add a from-to item to "gap".  Used for REP and SAL items.
 * They are stored case-folded.
 */
    static void
add_fromto(spin, gap, from, to)
    spellinfo_T	*spin;
    garray_T	*gap;
    char_u	*from;
    char_u	*to;
{
    fromto_T	*ftp;
    char_u	word[MAXWLEN];

    if (ga_grow(gap, 1) == OK)
    {
	ftp = ((fromto_T *)gap->ga_data) + gap->ga_len;
	(void)spell_casefold(from, STRLEN(from), word, MAXWLEN);
	ftp->ft_from = getroom_save(&spin->si_blocks, word);
	(void)spell_casefold(to, STRLEN(to), word, MAXWLEN);
	ftp->ft_to = getroom_save(&spin->si_blocks, word);
	++gap->ga_len;
    }
}

/*
 * Convert a boolean argument in a SAL line to TRUE or FALSE;
 */
    static int
sal_to_bool(s)
    char_u	*s;
{
    return STRCMP(s, "1") == 0 || STRCMP(s, "true") == 0;
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
    int		flags;

    /*
     * Open the file.
     */
    fd = mch_fopen((char *)fname, "r");
    if (fd == NULL)
    {
	EMSG2(_(e_notopen), fname);
	return FAIL;
    }

    /* The hashtable is only used to detect duplicated words. */
    hash_init(&ht);

    spin->si_foldwcount = 0;
    spin->si_keepwcount = 0;

    if (spin->si_verbose || p_verbose > 2)
    {
	if (!spin->si_verbose)
	    verbose_enter();
	smsg((char_u *)_("Reading dictionary file %s..."), fname);
	out_flush();
	if (!spin->si_verbose)
	    verbose_leave();
    }

    /* Read and ignore the first line: word count. */
    (void)vim_fgets(line, MAXLINELEN, fd);
    if (!vim_isdigit(*skipwhite(line)))
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
	if (spin->si_verbose && (lnum & 0x3ff) == 0)
	{
	    vim_snprintf((char *)message, sizeof(message),
		    _("line %6d, word %6d - %s"),
		       lnum, spin->si_foldwcount + spin->si_keepwcount, line);
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

#ifdef FEAT_MBYTE
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
#endif
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

	flags = 0;
	if (afflist != NULL)
	{
	    /* Check for affix name that stands for keep-case word and stands
	     * for rare word (if defined). */
	    if (affile->af_kep != NUL
		    && vim_strchr(afflist, affile->af_kep) != NULL)
		flags |= WF_KEEPCAP;
	    if (affile->af_rar != NUL
		    && vim_strchr(afflist, affile->af_rar) != NULL)
		flags |= WF_RARE;
	}

	/* Add the word to the word tree(s). */
	if (store_word(dw, spin, flags, spin->si_region) == FAIL)
	    retval = FAIL;

	if (afflist != NULL)
	{
	    /* Find all matching suffixes and add the resulting words.
	     * Additionally do matching prefixes that combine. */
	    if (store_aff_word(dw, spin, afflist,
			   &affile->af_suff, &affile->af_pref,
							FALSE, flags) == FAIL)
		retval = FAIL;

	    /* Find all matching prefixes and add the resulting words. */
	    if (store_aff_word(dw, spin, afflist,
				&affile->af_pref, NULL, FALSE, flags) == FAIL)
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
store_aff_word(word, spin, afflist, ht, xht, comb, flags)
    char_u	*word;		/* basic word start */
    spellinfo_T	*spin;		/* spell info */
    char_u	*afflist;	/* list of names of supported affixes */
    hashtab_T	*ht;
    hashtab_T	*xht;
    int		comb;		/* only use affixes that combine */
    int		flags;		/* flags for the word */
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
			    {
				/* Skip chop string. */
#ifdef FEAT_MBYTE
				if (has_mbyte)
				{
				    i = mb_charlen(ae->ae_chop);
				    for ( ; i > 0; --i)
					mb_ptr_adv(p);
				}
				else
#endif
				    p += STRLEN(ae->ae_chop);
			    }
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
#ifdef FEAT_MBYTE
				if (has_mbyte)
				    i = mb_charlen(ae->ae_chop);
				else
#endif
				    i = STRLEN(ae->ae_chop);
				for ( ; i > 0; --i)
				    mb_ptr_back(newword, p);
				*p = NUL;
			    }
			    if (ae->ae_add != NULL)
				STRCAT(newword, ae->ae_add);
			}

			/* Store the modified word. */
			if (store_word(newword, spin,
					      flags, spin->si_region) == FAIL)
			    retval = FAIL;

			/* When added a suffix and combining is allowed also
			 * try adding prefixes additionally. */
			if (xht != NULL && ah->ah_combine)
			    if (store_aff_word(newword, spin, afflist,
					      xht, NULL, TRUE, flags) == FAIL)
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
    int		flags;
    int		regionmask;

    /*
     * Open the file.
     */
    fd = mch_fopen((char *)fname, "r");
    if (fd == NULL)
    {
	EMSG2(_(e_notopen), fname);
	return FAIL;
    }

    if (spin->si_verbose || p_verbose > 2)
    {
	if (!spin->si_verbose)
	    verbose_enter();
	smsg((char_u *)_("Reading word file %s..."), fname);
	out_flush();
	if (!spin->si_verbose)
	    verbose_leave();
    }

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
#ifdef FEAT_MBYTE
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
#endif
	{
	    pc = NULL;
	    line = rline;
	}

	flags = 0;
	regionmask = spin->si_region;

	if (*line == '/')
	{
	    ++line;

	    if (STRNCMP(line, "encoding=", 9) == 0)
	    {
		if (spin->si_conv.vc_type != CONV_NONE)
		    smsg((char_u *)_("Duplicate /encoding= line ignored in %s line %d: %s"),
						       fname, lnum, line - 1);
		else if (did_word)
		    smsg((char_u *)_("/encoding= line after word ignored in %s line %d: %s"),
						       fname, lnum, line - 1);
		else
		{
#ifdef FEAT_MBYTE
		    char_u	*enc;

		    /* Setup for conversion to 'encoding'. */
		    line += 10;
		    enc = enc_canonize(line);
		    if (enc != NULL && !spin->si_ascii
			    && convert_setup(&spin->si_conv, enc,
							       p_enc) == FAIL)
			smsg((char_u *)_("Conversion in %s not supported: from %s to %s"),
							  fname, line, p_enc);
		    vim_free(enc);
#else
		    smsg((char_u *)_("Conversion in %s not supported"), fname);
#endif
		}
		continue;
	    }

	    if (STRNCMP(line, "regions=", 8) == 0)
	    {
		if (spin->si_region_count > 1)
		    smsg((char_u *)_("Duplicate /regions= line ignored in %s line %d: %s"),
						       fname, lnum, line);
		else
		{
		    line += 8;
		    if (STRLEN(line) > 16)
			smsg((char_u *)_("Too many regions in %s line %d: %s"),
						       fname, lnum, line);
		    else
		    {
			spin->si_region_count = STRLEN(line) / 2;
			STRCPY(spin->si_region_name, line);
		    }
		}
		continue;
	    }

	    if (*line == '=')
	    {
		/* keep-case word */
		flags |= WF_KEEPCAP;
		++line;
	    }

	    if (*line == '!')
	    {
		/* Bad, bad, wicked word. */
		flags |= WF_BANNED;
		++line;
	    }
	    else if (*line == '?')
	    {
		/* Rare word. */
		flags |= WF_RARE;
		++line;
	    }

	    if (VIM_ISDIGIT(*line))
	    {
		/* region number(s) */
		regionmask = 0;
		while (VIM_ISDIGIT(*line))
		{
		    l = *line - '0';
		    if (l > spin->si_region_count)
		    {
			smsg((char_u *)_("Invalid region nr in %s line %d: %s"),
							   fname, lnum, line);
			break;
		    }
		    regionmask |= 1 << (l - 1);
		    ++line;
		}
		flags |= WF_REGION;
	    }

	    if (flags == 0)
	    {
		smsg((char_u *)_("/ line ignored in %s line %d: %s"),
							   fname, lnum, line);
		continue;
	    }
	}

	/* Skip non-ASCII words when "spin->si_ascii" is TRUE. */
	if (spin->si_ascii && has_non_ascii(line))
	{
	    ++non_ascii;
	    continue;
	}

	/* Normal word: store it. */
	if (store_word(line, spin, flags, regionmask) == FAIL)
	{
	    retval = FAIL;
	    break;
	}
	did_word = TRUE;
    }

    vim_free(pc);
    fclose(fd);

    if (spin->si_ascii && non_ascii > 0 && (spin->si_verbose || p_verbose > 2))
    {
	if (p_verbose > 2)
	    verbose_enter();
	smsg((char_u *)_("Ignored %d words with non-ASCII characters"),
								   non_ascii);
	if (p_verbose > 2)
	    verbose_leave();
    }
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
 * Always store it in the case-folded tree.  A keep-case word can also be used
 * with all caps.
 * For a keep-case word also store it in the keep-case tree.
 */
    static int
store_word(word, spin, flags, region)
    char_u	*word;
    spellinfo_T	*spin;
    int		flags;		/* extra flags, WF_BANNED */
    int		region;		/* supported region(s) */
{
    int		len = STRLEN(word);
    int		ct = captype(word, word + len);
    char_u	foldword[MAXWLEN];
    int		res;

    (void)spell_casefold(word, len, foldword, MAXWLEN);
    res = tree_add_word(foldword, spin->si_foldroot, ct | flags,
						region, &spin->si_blocks);
    ++spin->si_foldwcount;

    if (res == OK && (ct == WF_KEEPCAP || flags & WF_KEEPCAP))
    {
	res = tree_add_word(word, spin->si_keeproot, flags,
						    region, &spin->si_blocks);
	++spin->si_keepwcount;
    }
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
		 || (node->wn_byte == 0 && node->wn_flags != (flags & 0xff))))
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
wordtree_compress(root, spin)
    wordnode_T	    *root;
    spellinfo_T	    *spin;
{
    hashtab_T	    ht;
    int		    n;
    int		    tot = 0;

    if (root != NULL)
    {
	hash_init(&ht);
	n = node_compress(root, &ht, &tot);
	if (spin->si_verbose || p_verbose > 2)
	{
	    if (!spin->si_verbose)
		verbose_enter();
	    smsg((char_u *)_("Compressed %d of %d nodes; %d%% remaining"),
					       n, tot, (tot - n) * 100 / tot);
	    if (p_verbose > 2)
		verbose_leave();
	}
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

static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
rep_compare __ARGS((const void *s1, const void *s2));

/*
 * Function given to qsort() to sort the REP items on "from" string.
 */
    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
rep_compare(s1, s2)
    const void	*s1;
    const void	*s2;
{
    fromto_T	*p1 = (fromto_T *)s1;
    fromto_T	*p2 = (fromto_T *)s2;

    return STRCMP(p1->ft_from, p2->ft_from);
}

/*
 * Write the Vim spell file "fname".
 */
    static void
write_vim_spell(fname, spin)
    char_u	*fname;
    spellinfo_T	*spin;
{
    FILE	*fd;
    int		regionmask;
    int		round;
    wordnode_T	*tree;
    int		nodecount;
    int		i;
    int		l;
    garray_T	*gap;
    fromto_T	*ftp;
    char_u	*p;
    int		rr;

    fd = mch_fopen((char *)fname, "w");
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
    if (spin->si_region_count > 1)
    {
	putc(spin->si_region_count, fd);   /* <regioncnt> <regionname> ... */
	fwrite(spin->si_region_name, (size_t)(spin->si_region_count * 2),
							       (size_t)1, fd);
	regionmask = (1 << spin->si_region_count) - 1;
    }
    else
    {
	putc(0, fd);
	regionmask = 0;
    }

    /*
     * Write the table with character flags and table for case folding.
     * <charflagslen> <charflags>  <fcharlen> <fchars>
     * Skip this for ASCII, the table may conflict with the one used for
     * 'encoding'.
     * Also skip this for an .add.spl file, the main spell file must contain
     * the table (avoids that it conflicts).  File is shorter too.
     */
    if (spin->si_ascii || spin->si_add)
    {
	putc(0, fd);
	putc(0, fd);
	putc(0, fd);
    }
    else
	write_spell_chartab(fd);

    /* Sort the REP items. */
    qsort(spin->si_rep.ga_data, (size_t)spin->si_rep.ga_len,
					       sizeof(fromto_T), rep_compare);

    /* <SUGGEST> : <repcount> <rep> ...
     *             <salflags> <salcount> <sal> ...
     *             <maplen> <mapstr> */
    for (round = 1; round <= 2; ++round)
    {
	if (round == 1)
	    gap = &spin->si_rep;
	else
	{
	    gap = &spin->si_sal;

	    i = 0;
	    if (spin->si_followup)
		i |= SAL_F0LLOWUP;
	    if (spin->si_collapse)
		i |= SAL_COLLAPSE;
	    if (spin->si_rem_accents)
		i |= SAL_REM_ACCENTS;
	    putc(i, fd);			/* <salflags> */
	}

	put_bytes(fd, (long_u)gap->ga_len, 2);	/* <repcount> or <salcount> */
	for (i = 0; i < gap->ga_len; ++i)
	{
	    /* <rep> : <repfromlen> <repfrom> <reptolen> <repto> */
	    /* <sal> : <salfromlen> <salfrom> <saltolen> <salto> */
	    ftp = &((fromto_T *)gap->ga_data)[i];
	    for (rr = 1; rr <= 2; ++rr)
	    {
		p = rr == 1 ? ftp->ft_from : ftp->ft_to;
		l = STRLEN(p);
		putc(l, fd);
		fwrite(p, l, (size_t)1, fd);
	    }
	}
    }

    put_bytes(fd, (long_u)spin->si_map.ga_len, 2);	/* <maplen> */
    if (spin->si_map.ga_len > 0)			/* <mapstr> */
	fwrite(spin->si_map.ga_data, (size_t)spin->si_map.ga_len,
							       (size_t)1, fd);

    /*
     * <LWORDTREE>  <KWORDTREE>
     */
    spin->si_memtot = 0;
    for (round = 1; round <= 2; ++round)
    {
	tree = (round == 1) ? spin->si_foldroot : spin->si_keeproot;

	/* Count the number of nodes.  Needed to be able to allocate the
	 * memory when reading the nodes.  Also fills in the index for shared
	 * nodes. */
	nodecount = put_tree(NULL, tree, 0, regionmask);

	/* number of nodes in 4 bytes */
	put_bytes(fd, (long_u)nodecount, 4);	/* <nodecount> */
	spin->si_memtot += nodecount + nodecount * sizeof(int);

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
 * ":mkspell [-ascii] outfile  infile ..."
 * ":mkspell [-ascii] addfile"
 */
    void
ex_mkspell(eap)
    exarg_T *eap;
{
    int		fcount;
    char_u	**fnames;
    char_u	*arg = eap->arg;
    int		ascii = FALSE;

    if (STRNCMP(arg, "-ascii", 6) == 0)
    {
	ascii = TRUE;
	arg = skipwhite(arg + 6);
    }

    /* Expand all the remaining arguments (e.g., $VIMRUNTIME). */
    if (get_arglist_exp(arg, &fcount, &fnames) == OK)
    {
	mkspell(fcount, fnames, ascii, eap->forceit, FALSE);
	FreeWild(fcount, fnames);
    }
}

/*
 * Create a Vim spell file from one or more word lists.
 * "fnames[0]" is the output file name.
 * "fnames[fcount - 1]" is the last input file name.
 * Exception: when "fnames[0]" ends in ".add" it's used as the input file name
 * and ".spl" is appended to make the output file name.
 */
    static void
mkspell(fcount, fnames, ascii, overwrite, added_word)
    int		fcount;
    char_u	**fnames;
    int		ascii;		    /* -ascii argument given */
    int		overwrite;	    /* overwrite existing output file */
    int		added_word;	    /* invoked through "zg" */
{
    char_u	fname[MAXPATHL];
    char_u	wfname[MAXPATHL];
    char_u	**innames;
    int		incount;
    afffile_T	*(afile[8]);
    int		i;
    int		len;
    struct stat	st;
    int		error = FALSE;
    spellinfo_T spin;

    vim_memset(&spin, 0, sizeof(spin));
    spin.si_verbose = !added_word;
    spin.si_ascii = ascii;
    spin.si_followup = TRUE;
    spin.si_rem_accents = TRUE;
    ga_init2(&spin.si_rep, (int)sizeof(fromto_T), 20);
    ga_init2(&spin.si_sal, (int)sizeof(fromto_T), 20);
    ga_init2(&spin.si_map, (int)sizeof(char_u), 100);

    /* default: fnames[0] is output file, following are input files */
    innames = &fnames[1];
    incount = fcount - 1;

    if (fcount >= 1)
    {
	len = STRLEN(fnames[0]);
	if (fcount == 1 && len > 4 && STRCMP(fnames[0] + len - 4, ".add") == 0)
	{
	    /* For ":mkspell path/en.latin1.add" output file is
	     * "path/en.latin1.add.spl". */
	    innames = &fnames[0];
	    incount = 1;
	    vim_snprintf((char *)wfname, sizeof(wfname), "%s.spl", fnames[0]);
	}
	else if (len > 4 && STRCMP(fnames[0] + len - 4, ".spl") == 0)
	{
	    /* Name ends in ".spl", use as the file name. */
	    vim_strncpy(wfname, fnames[0], sizeof(wfname) - 1);
	}
	else
	    /* Name should be language, make the file name from it. */
	    vim_snprintf((char *)wfname, sizeof(wfname), "%s.%s.spl", fnames[0],
			     spin.si_ascii ? (char_u *)"ascii" : spell_enc());

	/* Check for .ascii.spl. */
	if (strstr((char *)gettail(wfname), ".ascii.") != NULL)
	    spin.si_ascii = TRUE;

	/* Check for .add.spl. */
	if (strstr((char *)gettail(wfname), ".add.") != NULL)
	    spin.si_add = TRUE;
    }

    if (incount <= 0)
	EMSG(_(e_invarg));	/* need at least output and input names */
    else if (incount > 8)
	EMSG(_("E754: Only up to 8 regions supported"));
    else
    {
	/* Check for overwriting before doing things that may take a lot of
	 * time. */
	if (!overwrite && mch_stat((char *)wfname, &st) >= 0)
	{
	    EMSG(_(e_exists));
	    return;
	}
	if (mch_isdir(wfname))
	{
	    EMSG2(_(e_isadir2), wfname);
	    return;
	}

	/*
	 * Init the aff and dic pointers.
	 * Get the region names if there are more than 2 arguments.
	 */
	for (i = 0; i < incount; ++i)
	{
	    afile[i] = NULL;

	    if (incount > 1)
	    {
		len = STRLEN(innames[i]);
		if (STRLEN(gettail(innames[i])) < 5
						|| innames[i][len - 3] != '_')
		{
		    EMSG2(_("E755: Invalid region in %s"), innames[i]);
		    return;
		}
		spin.si_region_name[i * 2] = TOLOWER_ASC(innames[i][len - 2]);
		spin.si_region_name[i * 2 + 1] =
					     TOLOWER_ASC(innames[i][len - 1]);
	    }
	}
	spin.si_region_count = incount;

	if (!spin.si_add)
	    /* Clear the char type tables, don't want to use any of the
	     * currently used spell properties. */
	    init_spell_chartab();

	spin.si_foldroot = wordtree_alloc(&spin.si_blocks);
	spin.si_keeproot = wordtree_alloc(&spin.si_blocks);
	if (spin.si_foldroot == NULL || spin.si_keeproot == NULL)
	{
	    error = TRUE;
	    return;
	}

	/*
	 * Read all the .aff and .dic files.
	 * Text is converted to 'encoding'.
	 * Words are stored in the case-folded and keep-case trees.
	 */
	for (i = 0; i < incount && !error; ++i)
	{
	    spin.si_conv.vc_type = CONV_NONE;
	    spin.si_region = 1 << i;

	    vim_snprintf((char *)fname, sizeof(fname), "%s.aff", innames[i]);
	    if (mch_stat((char *)fname, &st) >= 0)
	    {
		/* Read the .aff file.  Will init "spin->si_conv" based on the
		 * "SET" line. */
		afile[i] = spell_read_aff(fname, &spin);
		if (afile[i] == NULL)
		    error = TRUE;
		else
		{
		    /* Read the .dic file and store the words in the trees. */
		    vim_snprintf((char *)fname, sizeof(fname), "%s.dic",
								  innames[i]);
		    if (spell_read_dic(fname, &spin, afile[i]) == FAIL)
			error = TRUE;
		}
	    }
	    else
	    {
		/* No .aff file, try reading the file as a word list.  Store
		 * the words in the trees. */
		if (spell_read_wordfile(innames[i], &spin) == FAIL)
		    error = TRUE;
	    }

#ifdef FEAT_MBYTE
	    /* Free any conversion stuff. */
	    convert_setup(&spin.si_conv, NULL, NULL);
#endif
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
	    if (!added_word || p_verbose > 2)
	    {
		if (added_word)
		    verbose_enter();
		MSG(_("Compressing word tree..."));
		out_flush();
		if (added_word)
		    verbose_leave();
	    }
	    wordtree_compress(spin.si_foldroot, &spin);
	    wordtree_compress(spin.si_keeproot, &spin);
	}

	if (!error)
	{
	    /*
	     * Write the info in the spell file.
	     */
	    if (!added_word || p_verbose > 2)
	    {
		if (added_word)
		    verbose_enter();
		smsg((char_u *)_("Writing spell file %s..."), wfname);
		out_flush();
		if (added_word)
		    verbose_leave();
	    }

	    write_vim_spell(wfname, &spin);

	    if (!added_word || p_verbose > 2)
	    {
		if (added_word)
		    verbose_enter();
		MSG(_("Done!"));
		smsg((char_u *)_("Estimated runtime memory use: %d bytes"),
							      spin.si_memtot);
		out_flush();
		if (added_word)
		    verbose_leave();
	    }

	    /* If the file is loaded need to reload it. */
	    spell_reload_one(wfname, added_word);
	}

	/* Free the allocated memory. */
	free_blocks(spin.si_blocks);
	ga_clear(&spin.si_rep);
	ga_clear(&spin.si_sal);
	ga_clear(&spin.si_map);

	/* Free the .aff file structures. */
	for (i = 0; i < incount; ++i)
	    if (afile[i] != NULL)
		spell_free_aff(afile[i]);
    }
}


/*
 * ":spellgood  {word}"
 * ":spellwrong  {word}"
 */
    void
ex_spell(eap)
    exarg_T *eap;
{
    spell_add_word(eap->arg, STRLEN(eap->arg), eap->cmdidx == CMD_spellwrong);
}

/*
 * Add "word[len]" to 'spellfile' as a good or bad word.
 */
    void
spell_add_word(word, len, bad)
    char_u	*word;
    int		len;
    int		bad;
{
    FILE	*fd;
    buf_T	*buf;

    if (*curbuf->b_p_spf == NUL)
	init_spellfile();
    if (*curbuf->b_p_spf == NUL)
	EMSG(_("E764: 'spellfile' is not set"));
    else
    {
	/* Check that the user isn't editing the .add file somewhere. */
	buf = buflist_findname_exp(curbuf->b_p_spf);
	if (buf != NULL && buf->b_ml.ml_mfp == NULL)
	    buf = NULL;
	if (buf != NULL && bufIsChanged(buf))
	    EMSG(_(e_bufloaded));
	else
	{
	    fd = mch_fopen((char *)curbuf->b_p_spf, "a");
	    if (fd == NULL)
		EMSG2(_(e_notopen), curbuf->b_p_spf);
	    else
	    {
		if (bad)
		    fprintf(fd, "/!%.*s\n", len, word);
		else
		    fprintf(fd, "%.*s\n", len, word);
		fclose(fd);

		/* Update the .add.spl file. */
		mkspell(1, &curbuf->b_p_spf, FALSE, TRUE, TRUE);

		/* If the .add file is edited somewhere, reload it. */
		if (buf != NULL)
		    buf_reload(buf);

		redraw_all_later(NOT_VALID);
	    }
	}
    }
}

/*
 * Initialize 'spellfile' for the current buffer.
 */
    static void
init_spellfile()
{
    char_u	buf[MAXPATHL];
    int		l;
    slang_T	*sl;
    char_u	*rtp;

    if (*curbuf->b_p_spl != NUL && curbuf->b_langp.ga_len > 0)
    {
	/* Loop over all entries in 'runtimepath'. */
	rtp = p_rtp;
	while (*rtp != NUL)
	{
	    /* Copy the path from 'runtimepath' to buf[]. */
	    copy_option_part(&rtp, buf, MAXPATHL, ",");
	    if (filewritable(buf) == 2)
	    {
		/* Use the first language name from 'spelllang' and the
		 * encoding used in the first loaded .spl file. */
		sl = LANGP_ENTRY(curbuf->b_langp, 0)->lp_slang;
		l = STRLEN(buf);
		vim_snprintf((char *)buf + l, MAXPATHL - l,
			"/spell/%.*s.%s.add",
			2, curbuf->b_p_spl,
			strstr((char *)gettail(sl->sl_fname), ".ascii.") != NULL
					   ? (char_u *)"ascii" : spell_enc());
		set_option_value((char_u *)"spellfile", 0L, buf, OPT_LOCAL);
		break;
	    }
	}
    }
}


/*
 * Init the chartab used for spelling for ASCII.
 * EBCDIC is not supported!
 */
    static void
clear_spell_chartab(sp)
    spelltab_T	*sp;
{
    int		i;

    /* Init everything to FALSE. */
    vim_memset(sp->st_isw, FALSE, sizeof(sp->st_isw));
    vim_memset(sp->st_isu, FALSE, sizeof(sp->st_isu));
    for (i = 0; i < 256; ++i)
    {
	sp->st_fold[i] = i;
	sp->st_upper[i] = i;
    }

    /* We include digits.  A word shouldn't start with a digit, but handling
     * that is done separately. */
    for (i = '0'; i <= '9'; ++i)
	sp->st_isw[i] = TRUE;
    for (i = 'A'; i <= 'Z'; ++i)
    {
	sp->st_isw[i] = TRUE;
	sp->st_isu[i] = TRUE;
	sp->st_fold[i] = i + 0x20;
    }
    for (i = 'a'; i <= 'z'; ++i)
    {
	sp->st_isw[i] = TRUE;
	sp->st_upper[i] = i - 0x20;
    }
}

/*
 * Init the chartab used for spelling.  Only depends on 'encoding'.
 * Called once while starting up and when 'encoding' changes.
 * The default is to use isalpha(), but the spell file should define the word
 * characters to make it possible that 'encoding' differs from the current
 * locale.
 */
    void
init_spell_chartab()
{
    int	    i;

    did_set_spelltab = FALSE;
    clear_spell_chartab(&spelltab);

#ifdef FEAT_MBYTE
    if (enc_dbcs)
    {
	/* DBCS: assume double-wide characters are word characters. */
	for (i = 128; i <= 255; ++i)
	    if (MB_BYTE2LEN(i) == 2)
		spelltab.st_isw[i] = TRUE;
    }
    else if (enc_utf8)
    {
	for (i = 128; i < 256; ++i)
	{
	    spelltab.st_isu[i] = utf_isupper(i);
	    spelltab.st_isw[i] = spelltab.st_isu[i] || utf_islower(i);
	    spelltab.st_fold[i] = utf_fold(i);
	    spelltab.st_upper[i] = utf_toupper(i);
	}
    }
    else
#endif
    {
	/* Rough guess: use locale-dependent library functions. */
	for (i = 128; i < 256; ++i)
	{
	    if (MB_ISUPPER(i))
	    {
		spelltab.st_isw[i] = TRUE;
		spelltab.st_isu[i] = TRUE;
		spelltab.st_fold[i] = MB_TOLOWER(i);
	    }
	    else if (MB_ISLOWER(i))
	    {
		spelltab.st_isw[i] = TRUE;
		spelltab.st_upper[i] = MB_TOUPPER(i);
	    }
	}
    }
}

static char *e_affform = N_("E761: Format error in affix file FOL, LOW or UPP");
static char *e_affrange = N_("E762: Character in FOL, LOW or UPP is out of range");

/*
 * Set the spell character tables from strings in the affix file.
 */
    static int
set_spell_chartab(fol, low, upp)
    char_u	*fol;
    char_u	*low;
    char_u	*upp;
{
    /* We build the new tables here first, so that we can compare with the
     * previous one. */
    spelltab_T	new_st;
    char_u	*pf = fol, *pl = low, *pu = upp;
    int		f, l, u;

    clear_spell_chartab(&new_st);

    while (*pf != NUL)
    {
	if (*pl == NUL || *pu == NUL)
	{
	    EMSG(_(e_affform));
	    return FAIL;
	}
#ifdef FEAT_MBYTE
	f = mb_ptr2char_adv(&pf);
	l = mb_ptr2char_adv(&pl);
	u = mb_ptr2char_adv(&pu);
#else
	f = *pf++;
	l = *pl++;
	u = *pu++;
#endif
	/* Every character that appears is a word character. */
	if (f < 256)
	    new_st.st_isw[f] = TRUE;
	if (l < 256)
	    new_st.st_isw[l] = TRUE;
	if (u < 256)
	    new_st.st_isw[u] = TRUE;

	/* if "LOW" and "FOL" are not the same the "LOW" char needs
	 * case-folding */
	if (l < 256 && l != f)
	{
	    if (f >= 256)
	    {
		EMSG(_(e_affrange));
		return FAIL;
	    }
	    new_st.st_fold[l] = f;
	}

	/* if "UPP" and "FOL" are not the same the "UPP" char needs
	 * case-folding, it's upper case and the "UPP" is the upper case of
	 * "FOL" . */
	if (u < 256 && u != f)
	{
	    if (f >= 256)
	    {
		EMSG(_(e_affrange));
		return FAIL;
	    }
	    new_st.st_fold[u] = f;
	    new_st.st_isu[u] = TRUE;
	    new_st.st_upper[f] = u;
	}
    }

    if (*pl != NUL || *pu != NUL)
    {
	EMSG(_(e_affform));
	return FAIL;
    }

    return set_spell_finish(&new_st);
}

/*
 * Set the spell character tables from strings in the .spl file.
 */
    static int
set_spell_charflags(flags, cnt, upp)
    char_u	*flags;
    int		cnt;
    char_u	*upp;
{
    /* We build the new tables here first, so that we can compare with the
     * previous one. */
    spelltab_T	new_st;
    int		i;
    char_u	*p = upp;
    int		c;

    clear_spell_chartab(&new_st);

    for (i = 0; i < cnt; ++i)
    {
	new_st.st_isw[i + 128] = (flags[i] & CF_WORD) != 0;
	new_st.st_isu[i + 128] = (flags[i] & CF_UPPER) != 0;

	if (*p == NUL)
	    return FAIL;
#ifdef FEAT_MBYTE
	c = mb_ptr2char_adv(&p);
#else
	c = *p++;
#endif
	new_st.st_fold[i + 128] = c;
	if (i + 128 != c && new_st.st_isu[i + 128] && c < 256)
	    new_st.st_upper[c] = i + 128;
    }

    return set_spell_finish(&new_st);
}

    static int
set_spell_finish(new_st)
    spelltab_T	*new_st;
{
    int		i;

    if (did_set_spelltab)
    {
	/* check that it's the same table */
	for (i = 0; i < 256; ++i)
	{
	    if (spelltab.st_isw[i] != new_st->st_isw[i]
		    || spelltab.st_isu[i] != new_st->st_isu[i]
		    || spelltab.st_fold[i] != new_st->st_fold[i]
		    || spelltab.st_upper[i] != new_st->st_upper[i])
	    {
		EMSG(_("E763: Word characters differ between spell files"));
		return FAIL;
	    }
	}
    }
    else
    {
	/* copy the new spelltab into the one being used */
	spelltab = *new_st;
	did_set_spelltab = TRUE;
    }

    return OK;
}

/*
 * Write the current tables into the .spl file.
 * This makes sure the same characters are recognized as word characters when
 * generating an when using a spell file.
 */
    static void
write_spell_chartab(fd)
    FILE	*fd;
{
    char_u	charbuf[256 * 4];
    int		len = 0;
    int		flags;
    int		i;

    fputc(128, fd);				    /* <charflagslen> */
    for (i = 128; i < 256; ++i)
    {
	flags = 0;
	if (spelltab.st_isw[i])
	    flags |= CF_WORD;
	if (spelltab.st_isu[i])
	    flags |= CF_UPPER;
	fputc(flags, fd);			    /* <charflags> */

#ifdef FEAT_MBYTE
	if (has_mbyte)
	    len += mb_char2bytes(spelltab.st_fold[i], charbuf + len);
	else
#endif
	    charbuf[len++] = spelltab.st_fold[i];
    }

    put_bytes(fd, (long_u)len, 2);		    /* <fcharlen> */
    fwrite(charbuf, (size_t)len, (size_t)1, fd);    /* <fchars> */
}

/*
 * Case-fold "str[len]" into "buf[buflen]".  The result is NUL terminated.
 * Uses the character definitions from the .spl file.
 * When using a multi-byte 'encoding' the length may change!
 * Returns FAIL when something wrong.
 */
    static int
spell_casefold(str, len, buf, buflen)
    char_u	*str;
    int		len;
    char_u	*buf;
    int		buflen;
{
    int		i;

    if (len >= buflen)
    {
	buf[0] = NUL;
	return FAIL;		/* result will not fit */
    }

#ifdef FEAT_MBYTE
    if (has_mbyte)
    {
	int	outi = 0;
	char_u	*p;
	int	c;

	/* Fold one character at a time. */
	for (p = str; p < str + len; )
	{
	    if (outi + MB_MAXBYTES > buflen)
	    {
		buf[outi] = NUL;
		return FAIL;
	    }
	    c = mb_ptr2char_adv(&p);
	    outi += mb_char2bytes(SPELL_TOFOLD(c), buf + outi);
	}
	buf[outi] = NUL;
    }
    else
#endif
    {
	/* Be quick for non-multibyte encodings. */
	for (i = 0; i < len; ++i)
	    buf[i] = spelltab.st_fold[str[i]];
	buf[i] = NUL;
    }

    return OK;
}

/*
 * "z?": Find badly spelled word under or after the cursor.
 * Give suggestions for the properly spelled word.
 * This is based on the mechanisms of Aspell, but completely reimplemented.
 */
    void
spell_suggest()
{
    char_u	*line;
    pos_T	prev_cursor = curwin->w_cursor;
    int		attr;
    char_u	wcopy[MAXWLEN + 2];
    char_u	*p;
    int		i;
    int		c;
    suginfo_T	sug;
    suggest_T	*stp;

    /*
     * Find the start of the badly spelled word.
     */
    if (spell_move_to(FORWARD, TRUE, TRUE) == FAIL)
    {
	beep_flush();
	return;
    }

    /*
     * Set the info in "sug".
     */
    vim_memset(&sug, 0, sizeof(sug));
    ga_init2(&sug.su_ga, (int)sizeof(suggest_T), 10);
    hash_init(&sug.su_banned);
    line = ml_get_curline();
    sug.su_badptr = line + curwin->w_cursor.col;
    sug.su_badlen = spell_check(curwin, sug.su_badptr, &attr);
    if (sug.su_badlen >= MAXWLEN)
	sug.su_badlen = MAXWLEN - 1;	/* just in case */
    vim_strncpy(sug.su_badword, sug.su_badptr, sug.su_badlen);
    (void)spell_casefold(sug.su_badptr, sug.su_badlen,
						    sug.su_fbadword, MAXWLEN);

    /* Ban the bad word itself.  It may appear in another region. */
    add_banned(&sug, sug.su_badword);

    /*
     * 1. Try inserting/deleting/swapping/changing a letter, use REP entries
     *    from the .aff file and inserting a space (split the word).
     *
     * Set a maximum score to limit the combination of operations that is
     * tried.
     */
    sug.su_maxscore = SCORE_MAXINIT;
    spell_try_change(&sug);

    /*
     * 2. Try finding sound-a-like words.
     *
     * Only do this when we don't have a lot of suggestions yet, because it's
     * very slow and often doesn't find new suggestions.
     */
    if (sug.su_ga.ga_len < SUG_CLEAN_COUNT)
    {
	/* Allow a higher score now. */
	sug.su_maxscore = SCORE_MAXMAX;
	spell_try_soundalike(&sug);
    }

    /* When CTRL-C was hit while searching do show the results. */
    ui_breakcheck();
    if (got_int)
    {
	(void)vgetc();
	got_int = FALSE;
    }

    if (sug.su_ga.ga_len == 0)
	MSG(_("Sorry, no suggestions"));
    else
    {
#ifdef RESCORE
	/* Do slow but more accurate computation of the word score. */
	rescore_suggestions(&sug);
#endif

	/* Sort the suggestions and truncate at SUG_PROMPT_COUNT. */
	cleanup_suggestions(&sug, SUG_PROMPT_COUNT);

	/* List the suggestions. */
	msg_start();
	vim_snprintf((char *)IObuff, IOSIZE, _("Change \"%.*s\" to:"),
						sug.su_badlen, sug.su_badptr);
	msg_puts(IObuff);
	msg_clr_eos();
	msg_putchar('\n');
	msg_scroll = TRUE;
	for (i = 0; i < sug.su_ga.ga_len; ++i)
	{
	    stp = &SUG(&sug, i);

	    /* The suggested word may replace only part of the bad word, add
	     * the not replaced part. */
	    STRCPY(wcopy, stp->st_word);
	    if (sug.su_badlen > stp->st_orglen)
		vim_strncpy(wcopy + STRLEN(wcopy),
					       sug.su_badptr + stp->st_orglen,
					      sug.su_badlen - stp->st_orglen);
	    if (p_verbose > 0)
		vim_snprintf((char *)IObuff, IOSIZE, _("%2d \"%s\"  (%d)"),
						 i + 1, wcopy, stp->st_score);
	    else
		vim_snprintf((char *)IObuff, IOSIZE, _("%2d \"%s\""),
								i + 1, wcopy);
	    msg_puts(IObuff);
	    lines_left = 3;		/* avoid more prompt */
	    msg_putchar('\n');
	}

	/* Ask for choice. */
	i = prompt_for_number();
	if (i > 0 && i <= sug.su_ga.ga_len && u_save_cursor())
	{
	    /* Replace the word. */
	    stp = &SUG(&sug, i - 1);
	    p = alloc(STRLEN(line) - stp->st_orglen + STRLEN(stp->st_word) + 1);
	    if (p != NULL)
	    {
		c = sug.su_badptr - line;
		mch_memmove(p, line, c);
		STRCPY(p + c, stp->st_word);
		STRCAT(p, sug.su_badptr + stp->st_orglen);
		ml_replace(curwin->w_cursor.lnum, p, FALSE);
		curwin->w_cursor.col = c;
		changed_bytes(curwin->w_cursor.lnum, c);
	    }
	}
	else
	    curwin->w_cursor = prev_cursor;
    }

    /* Free the suggestions. */
    for (i = 0; i < sug.su_ga.ga_len; ++i)
	vim_free(SUG(&sug, i).st_word);
    ga_clear(&sug.su_ga);

    /* Free the banned words. */
    free_banned(&sug);
}

/*
 * Make a copy of "word", with the first letter upper or lower cased, to
 * "wcopy[MAXWLEN]".  "word" must not be empty.
 * The result is NUL terminated.
 */
    static void
onecap_copy(word, wcopy, upper)
    char_u	*word;
    char_u	*wcopy;
    int		upper;	    /* TRUE: first letter made upper case */
{
    char_u	*p;
    int		c;
    int		l;

    p = word;
#ifdef FEAT_MBYTE
    if (has_mbyte)
	c = mb_ptr2char_adv(&p);
    else
#endif
	c = *p++;
    if (upper)
	c = SPELL_TOUPPER(c);
    else
	c = SPELL_TOFOLD(c);
#ifdef FEAT_MBYTE
    if (has_mbyte)
	l = mb_char2bytes(c, wcopy);
    else
#endif
    {
	l = 1;
	wcopy[0] = c;
    }
    vim_strncpy(wcopy + l, p, MAXWLEN - l);
}

/*
 * Make a copy of "word" with all the letters upper cased into
 * "wcopy[MAXWLEN]".  The result is NUL terminated.
 */
    static void
allcap_copy(word, wcopy)
    char_u	*word;
    char_u	*wcopy;
{
    char_u	*s;
    char_u	*d;
    int		c;

    d = wcopy;
    for (s = word; *s != NUL; )
    {
#ifdef FEAT_MBYTE
	if (has_mbyte)
	    c = mb_ptr2char_adv(&s);
	else
#endif
	    c = *s++;
	c = SPELL_TOUPPER(c);

#ifdef FEAT_MBYTE
	if (has_mbyte)
	{
	    if (d - wcopy >= MAXWLEN - MB_MAXBYTES)
		break;
	    d += mb_char2bytes(c, d);
	}
	else
#endif
	{
	    if (d - wcopy >= MAXWLEN - 1)
		break;
	    *d++ = c;
	}
    }
    *d = NUL;
}

/*
 * Try finding suggestions by adding/removing/swapping letters.
 *
 * This uses a state machine.  At each node in the tree we try various
 * operations.  When trying if an operation work "depth" is increased and the
 * stack[] is used to store info.  This allows combinations, thus insert one
 * character, replace one and delete another.  The number of changes is
 * limited by su->su_maxscore, checked in try_deeper().
 */
    static void
spell_try_change(su)
    suginfo_T	*su;
{
    char_u	fword[MAXWLEN];	    /* copy of the bad word, case-folded */
    char_u	tword[MAXWLEN];	    /* good word collected so far */
    trystate_T	stack[MAXWLEN];
    char_u	preword[MAXWLEN * 3]; /* word found with proper case (appended
				       * to for word split) */
    char_u	prewordlen = 0;	    /* length of word in "preword" */
    int		splitoff = 0;	    /* index in tword after last split */
    trystate_T	*sp;
    int		newscore;
    langp_T	*lp;
    char_u	*byts;
    idx_T	*idxs;
    int		depth;
    int		c, c2, c3;
    int		n = 0;
    int		flags;
    int		badflags;
    garray_T	*gap;
    idx_T	arridx;
    int		len;
    char_u	*p;
    fromto_T	*ftp;
    int		fl = 0, tl;

    /* get caps flags for bad word */
    badflags = captype(su->su_badptr, su->su_badptr + su->su_badlen);

    /* We make a copy of the case-folded bad word, so that we can modify it
     * to find matches (esp. REP items). */
    STRCPY(fword, su->su_fbadword);


    for (lp = LANGP_ENTRY(curwin->w_buffer->b_langp, 0);
						   lp->lp_slang != NULL; ++lp)
    {
#ifdef SOUNDFOLD_SCORE
	su->su_slang = lp->lp_slang;
	if (lp->lp_slang->sl_sal.ga_len > 0)
	    /* soundfold the bad word */
	    spell_soundfold(lp->lp_slang, su->su_fbadword, su->su_salword);
#endif

	/*
	 * Go through the whole case-fold tree, try changes at each node.
	 * "tword[]" contains the word collected from nodes in the tree.
	 * "fword[]" the word we are trying to match with (initially the bad
	 * word).
	 */
	byts = lp->lp_slang->sl_fbyts;
	idxs = lp->lp_slang->sl_fidxs;

	depth = 0;
	stack[0].ts_state = STATE_START;
	stack[0].ts_score = 0;
	stack[0].ts_curi = 1;
	stack[0].ts_fidx = 0;
	stack[0].ts_fidxtry = 0;
	stack[0].ts_twordlen = 0;
	stack[0].ts_arridx = 0;
#ifdef FEAT_MBYTE
	stack[0].ts_tcharlen = 0;
#endif

	/*
	 * Loop to find all suggestions.  At each round we either:
	 * - For the current state try one operation, advance "ts_curi",
	 *   increase "depth".
	 * - When a state is done go to the next, set "ts_state".
	 * - When all states are tried decrease "depth".
	 */
	while (depth >= 0 && !got_int)
	{
	    sp = &stack[depth];
	    switch (sp->ts_state)
	    {
	    case STATE_START:
		/*
		 * Start of node: Deal with NUL bytes, which means
		 * tword[] may end here.
		 */
		arridx = sp->ts_arridx;	    /* current node in the tree */
		len = byts[arridx];	    /* bytes in this node */
		arridx += sp->ts_curi;	    /* index of current byte */

		if (sp->ts_curi > len || (c = byts[arridx]) != 0)
		{
		    /* Past bytes in node and/or past NUL bytes. */
		    sp->ts_state = STATE_ENDNUL;
		    break;
		}

		/*
		 * End of word in tree.
		 */
		++sp->ts_curi;		/* eat one NUL byte */

		flags = (int)idxs[arridx];

		/*
		 * Form the word with proper case in preword.
		 * If there is a word from a previous split, append.
		 */
		tword[sp->ts_twordlen] = NUL;
		if (flags & WF_KEEPCAP)
		    /* Must find the word in the keep-case tree. */
		    find_keepcap_word(lp->lp_slang, tword + splitoff,
							preword + prewordlen);
		else
		    /* Include badflags: if the badword is onecap or allcap
		     * use that for the goodword too. */
		    make_case_word(tword + splitoff,
				      preword + prewordlen, flags | badflags);

		/* Don't use a banned word.  It may appear again as a good
		 * word, thus remember it. */
		if (flags & WF_BANNED)
		{
		    add_banned(su, preword + prewordlen);
		    break;
		}
		if (was_banned(su, preword + prewordlen))
		    break;

		newscore = 0;
		if ((flags & WF_REGION)
			     && (((unsigned)flags >> 8) & lp->lp_region) == 0)
		    newscore += SCORE_REGION;
		if (flags & WF_RARE)
		    newscore += SCORE_RARE;

		if (!spell_valid_case(badflags,
					 captype(preword + prewordlen, NULL)))
		    newscore += SCORE_ICASE;

		if (fword[sp->ts_fidx] == 0)
		{
		    /* The badword also ends: add suggestions, */
		    add_suggestion(su, preword, sp->ts_score + newscore
#ifdef RESCORE
			    , FALSE
#endif
			    );
		}
		else if (sp->ts_fidx >= sp->ts_fidxtry
#ifdef FEAT_MBYTE
			/* Don't split halfway a character. */
			&& (!has_mbyte || sp->ts_tcharlen == 0)
#endif
			)
		{
		    /* The word in the tree ends but the badword
		     * continues: try inserting a space and check that a valid
		     * words starts at fword[sp->ts_fidx]. */
		    if (try_deeper(su, stack, depth, newscore + SCORE_SPLIT))
		    {
			/* Save things to be restored at STATE_SPLITUNDO. */
			sp->ts_save_prewordlen = prewordlen;
			sp->ts_save_badflags = badflags;
			sp->ts_save_splitoff = splitoff;

			/* Append a space to preword. */
			STRCAT(preword, " ");
			prewordlen = STRLEN(preword);
			splitoff = sp->ts_twordlen;
#ifdef FEAT_MBYTE
			if (has_mbyte)
			{
			    int		i = 0;

			    /* Case-folding may change the number of bytes:
			     * Count nr of chars in fword[sp->ts_fidx] and
			     * advance that many chars in su->su_badptr. */
			    for (p = fword; p < fword + sp->ts_fidx;
								mb_ptr_adv(p))
				++i;
			    for (p = su->su_badptr; i > 0; mb_ptr_adv(p))
				--i;
			}
			else
#endif
			    p = su->su_badptr + sp->ts_fidx;
			badflags = captype(p, su->su_badptr + su->su_badlen);

			sp->ts_state = STATE_SPLITUNDO;
			++depth;
			/* Restart at top of the tree. */
			stack[depth].ts_arridx = 0;
		    }
		}
		break;

	    case STATE_SPLITUNDO:
		/* Fixup the changes done for word split. */
		badflags = sp->ts_save_badflags;
		splitoff = sp->ts_save_splitoff;
		prewordlen =  sp->ts_save_prewordlen;

		/* Continue looking for NUL bytes. */
		sp->ts_state = STATE_START;
		break;

	    case STATE_ENDNUL:
		/* Past the NUL bytes in the node. */
		if (fword[sp->ts_fidx] == 0)
		{
		    /* The badword ends, can't use the bytes in this node. */
		    sp->ts_state = STATE_DEL;
		    break;
		}
		sp->ts_state = STATE_PLAIN;
		/*FALLTHROUGH*/

	    case STATE_PLAIN:
		/*
		 * Go over all possible bytes at this node, add each to
		 * tword[] and use child node.  "ts_curi" is the index.
		 */
		arridx = sp->ts_arridx;
		if (sp->ts_curi > byts[arridx])
		{
		    /* Done all bytes at this node, do next state.  When still
		     * at already changed bytes skip the other tricks. */
		    if (sp->ts_fidx >= sp->ts_fidxtry)
			sp->ts_state = STATE_DEL;
		    else
			sp->ts_state = STATE_FINAL;
		}
		else
		{
		    arridx += sp->ts_curi++;
		    c = byts[arridx];

		    /* Normal byte, go one level deeper.  If it's not equal to
		     * the byte in the bad word adjust the score.  But don't
		     * even try when the byte was already changed. */
		    if (c == fword[sp->ts_fidx]
#ifdef FEAT_MBYTE
			    || (sp->ts_tcharlen > 0
						&& sp->ts_isdiff != DIFF_NONE)
#endif
			    )
			newscore = 0;
		    else
			newscore = SCORE_SUBST;
		    if ((newscore == 0 || sp->ts_fidx >= sp->ts_fidxtry)
				    && try_deeper(su, stack, depth, newscore))
		    {
			++depth;
			sp = &stack[depth];
			++sp->ts_fidx;
			tword[sp->ts_twordlen++] = c;
			sp->ts_arridx = idxs[arridx];
#ifdef FEAT_MBYTE
			if (newscore == SCORE_SUBST)
			    sp->ts_isdiff = DIFF_YES;
			if (has_mbyte)
			{
			    /* Multi-byte characters are a bit complicated to
			     * handle: They differ when any of the bytes
			     * differ and then their length may also differ. */
			    if (sp->ts_tcharlen == 0)
			    {
				/* First byte. */
				sp->ts_tcharidx = 0;
				sp->ts_tcharlen = MB_BYTE2LEN(c);
				sp->ts_fcharstart = sp->ts_fidx - 1;
				sp->ts_isdiff = (newscore != 0)
						       ? DIFF_YES : DIFF_NONE;
			    }
			    else if (sp->ts_isdiff == DIFF_INSERT)
				/* When inserting trail bytes don't advance in
				 * the bad word. */
				--sp->ts_fidx;
			    if (++sp->ts_tcharidx == sp->ts_tcharlen)
			    {
				/* Last byte of character. */
				if (sp->ts_isdiff == DIFF_YES)
				{
				    /* Correct ts_fidx for the byte length of
				     * the character (we didn't check that
				     * before). */
				    sp->ts_fidx = sp->ts_fcharstart
						+ MB_BYTE2LEN(
						    fword[sp->ts_fcharstart]);

				    /* For a similar character adjust score
				     * from SCORE_SUBST to SCORE_SIMILAR. */
				    if (lp->lp_slang->sl_has_map
					    && similar_chars(lp->lp_slang,
						mb_ptr2char(tword
						    + sp->ts_twordlen
							   - sp->ts_tcharlen),
						mb_ptr2char(fword
							+ sp->ts_fcharstart)))
					sp->ts_score -=
						  SCORE_SUBST - SCORE_SIMILAR;
				}

				/* Starting a new char, reset the length. */
				sp->ts_tcharlen = 0;
			    }
			}
			else
#endif
			{
			    /* If we found a similar char adjust the score.
			     * We do this after calling try_deeper() because
			     * it's slow. */
			    if (newscore != 0
				    && lp->lp_slang->sl_has_map
				    && similar_chars(lp->lp_slang,
						   c, fword[sp->ts_fidx - 1]))
				sp->ts_score -= SCORE_SUBST - SCORE_SIMILAR;
			}
		    }
		}
		break;

	    case STATE_DEL:
#ifdef FEAT_MBYTE
		/* When past the first byte of a multi-byte char don't try
		 * delete/insert/swap a character. */
		if (has_mbyte && sp->ts_tcharlen > 0)
		{
		    sp->ts_state = STATE_FINAL;
		    break;
		}
#endif
		/*
		 * Try skipping one character in the bad word (delete it).
		 */
		sp->ts_state = STATE_INS;
		sp->ts_curi = 1;
		if (fword[sp->ts_fidx] != NUL
			&& try_deeper(su, stack, depth, SCORE_DEL))
		{
		    ++depth;
#ifdef FEAT_MBYTE
		    if (has_mbyte)
			stack[depth].ts_fidx += MB_BYTE2LEN(fword[sp->ts_fidx]);
		    else
#endif
			++stack[depth].ts_fidx;
		    break;
		}
		/*FALLTHROUGH*/

	    case STATE_INS:
		/* Insert one byte.  Do this for each possible byte at this
		 * node. */
		n = sp->ts_arridx;
		if (sp->ts_curi > byts[n])
		{
		    /* Done all bytes at this node, do next state. */
		    sp->ts_state = STATE_SWAP;
		}
		else
		{
		    /* Do one more byte at this node.  Skip NUL bytes. */
		    n += sp->ts_curi++;
		    c = byts[n];
		    if (c != 0 && try_deeper(su, stack, depth, SCORE_INS))
		    {
			++depth;
			sp = &stack[depth];
			tword[sp->ts_twordlen++] = c;
			sp->ts_arridx = idxs[n];
#ifdef FEAT_MBYTE
			if (has_mbyte)
			{
			    fl = MB_BYTE2LEN(c);
			    if (fl > 1)
			    {
				/* There are following bytes for the same
				 * character.  We must find all bytes before
				 * trying delete/insert/swap/etc. */
				sp->ts_tcharlen = fl;
				sp->ts_tcharidx = 1;
				sp->ts_isdiff = DIFF_INSERT;
			    }
			}
#endif
		    }
		}
		break;

	    case STATE_SWAP:
		/*
		 * Swap two bytes in the bad word: "12" -> "21".
		 * We change "fword" here, it's changed back afterwards.
		 */
		p = fword + sp->ts_fidx;
		c = *p;
		if (c == NUL)
		{
		    /* End of word, can't swap or replace. */
		    sp->ts_state = STATE_FINAL;
		    break;
		}
#ifdef FEAT_MBYTE
		if (has_mbyte)
		{
		    n = mb_ptr2len_check(p);
		    c = mb_ptr2char(p);
		    c2 = mb_ptr2char(p + n);
		}
		else
#endif
		    c2 = p[1];
		if (c == c2)
		{
		    /* Characters are identical, swap won't do anything. */
		    sp->ts_state = STATE_SWAP3;
		    break;
		}
		if (c2 != NUL && try_deeper(su, stack, depth, SCORE_SWAP))
		{
		    sp->ts_state = STATE_UNSWAP;
		    ++depth;
#ifdef FEAT_MBYTE
		    if (has_mbyte)
		    {
			fl = mb_char2len(c2);
			mch_memmove(p, p + n, fl);
			mb_char2bytes(c, p + fl);
			stack[depth].ts_fidxtry = sp->ts_fidx + n + fl;
		    }
		    else
#endif
		    {
			p[0] = c2;
			p[1] = c;
			stack[depth].ts_fidxtry = sp->ts_fidx + 2;
		    }
		}
		else
		    /* If this swap doesn't work then SWAP3 won't either. */
		    sp->ts_state = STATE_REP_INI;
		break;

	    case STATE_UNSWAP:
		/* Undo the STATE_SWAP swap: "21" -> "12". */
		p = fword + sp->ts_fidx;
#ifdef FEAT_MBYTE
		if (has_mbyte)
		{
		    n = MB_BYTE2LEN(*p);
		    c = mb_ptr2char(p + n);
		    mch_memmove(p + MB_BYTE2LEN(p[n]), p, n);
		    mb_char2bytes(c, p);
		}
		else
#endif
		{
		    c = *p;
		    *p = p[1];
		    p[1] = c;
		}
		/*FALLTHROUGH*/

	    case STATE_SWAP3:
		/* Swap two bytes, skipping one: "123" -> "321".  We change
		 * "fword" here, it's changed back afterwards. */
		p = fword + sp->ts_fidx;
#ifdef FEAT_MBYTE
		if (has_mbyte)
		{
		    n = mb_ptr2len_check(p);
		    c = mb_ptr2char(p);
		    fl = mb_ptr2len_check(p + n);
		    c2 = mb_ptr2char(p + n);
		    c3 = mb_ptr2char(p + n + fl);
		}
		else
#endif
		{
		    c = *p;
		    c2 = p[1];
		    c3 = p[2];
		}

		/* When characters are identical: "121" then SWAP3 result is
		 * identical, ROT3L result is same as SWAP: "211", ROT3L
		 * result is same as SWAP on next char: "112".  Thus skip all
		 * swapping.  Also skip when c3 is NUL.  */
		if (c == c3 || c3 == NUL)
		{
		    sp->ts_state = STATE_REP_INI;
		    break;
		}
		if (try_deeper(su, stack, depth, SCORE_SWAP3))
		{
		    sp->ts_state = STATE_UNSWAP3;
		    ++depth;
#ifdef FEAT_MBYTE
		    if (has_mbyte)
		    {
			tl = mb_char2len(c3);
			mch_memmove(p, p + n + fl, tl);
			mb_char2bytes(c2, p + tl);
			mb_char2bytes(c, p + fl + tl);
			stack[depth].ts_fidxtry = sp->ts_fidx + n + fl + tl;
		    }
		    else
#endif
		    {
			p[0] = p[2];
			p[2] = c;
			stack[depth].ts_fidxtry = sp->ts_fidx + 3;
		    }
		}
		else
		    sp->ts_state = STATE_REP_INI;
		break;

	    case STATE_UNSWAP3:
		/* Undo STATE_SWAP3: "321" -> "123" */
		p = fword + sp->ts_fidx;
#ifdef FEAT_MBYTE
		if (has_mbyte)
		{
		    n = MB_BYTE2LEN(*p);
		    c2 = mb_ptr2char(p + n);
		    fl = MB_BYTE2LEN(p[n]);
		    c = mb_ptr2char(p + n + fl);
		    tl = MB_BYTE2LEN(p[n + fl]);
		    mch_memmove(p + fl + tl, p, n);
		    mb_char2bytes(c, p);
		    mb_char2bytes(c2, p + tl);
		}
		else
#endif
		{
		    c = *p;
		    *p = p[2];
		    p[2] = c;
		}
		/*FALLTHROUGH*/

	    case STATE_ROT3L:
		/* Rotate three characters left: "123" -> "231".  We change
		 * "fword" here, it's changed back afterwards. */
		if (try_deeper(su, stack, depth, SCORE_SWAP3))
		{
		    sp->ts_state = STATE_UNROT3L;
		    ++depth;
		    p = fword + sp->ts_fidx;
#ifdef FEAT_MBYTE
		    if (has_mbyte)
		    {
			n = mb_ptr2len_check(p);
			c = mb_ptr2char(p);
			fl = mb_ptr2len_check(p + n);
			fl += mb_ptr2len_check(p + n + fl);
			mch_memmove(p, p + n, fl);
			mb_char2bytes(c, p + fl);
			stack[depth].ts_fidxtry = sp->ts_fidx + n + fl;
		    }
		    else
#endif
		    {
			c = *p;
			*p = p[1];
			p[1] = p[2];
			p[2] = c;
			stack[depth].ts_fidxtry = sp->ts_fidx + 3;
		    }
		}
		else
		    sp->ts_state = STATE_REP_INI;
		break;

	    case STATE_UNROT3L:
		/* Undo STATE_ROT3L: "231" -> "123" */
		p = fword + sp->ts_fidx;
#ifdef FEAT_MBYTE
		if (has_mbyte)
		{
		    n = MB_BYTE2LEN(*p);
		    n += MB_BYTE2LEN(p[n]);
		    c = mb_ptr2char(p + n);
		    tl = MB_BYTE2LEN(p[n]);
		    mch_memmove(p + tl, p, n);
		    mb_char2bytes(c, p);
		}
		else
#endif
		{
		    c = p[2];
		    p[2] = p[1];
		    p[1] = *p;
		    *p = c;
		}
		/*FALLTHROUGH*/

	    case STATE_ROT3R:
		/* Rotate three bytes right: "123" -> "312".  We change
		 * "fword" here, it's changed back afterwards. */
		if (try_deeper(su, stack, depth, SCORE_SWAP3))
		{
		    sp->ts_state = STATE_UNROT3R;
		    ++depth;
		    p = fword + sp->ts_fidx;
#ifdef FEAT_MBYTE
		    if (has_mbyte)
		    {
			n = mb_ptr2len_check(p);
			n += mb_ptr2len_check(p + n);
			c = mb_ptr2char(p + n);
			tl = mb_ptr2len_check(p + n);
			mch_memmove(p + tl, p, n);
			mb_char2bytes(c, p);
			stack[depth].ts_fidxtry = sp->ts_fidx + n + tl;
		    }
		    else
#endif
		    {
			c = p[2];
			p[2] = p[1];
			p[1] = *p;
			*p = c;
			stack[depth].ts_fidxtry = sp->ts_fidx + 3;
		    }
		}
		else
		    sp->ts_state = STATE_REP_INI;
		break;

	    case STATE_UNROT3R:
		/* Undo STATE_ROT3R: "312" -> "123" */
		p = fword + sp->ts_fidx;
#ifdef FEAT_MBYTE
		if (has_mbyte)
		{
		    c = mb_ptr2char(p);
		    tl = MB_BYTE2LEN(*p);
		    n = MB_BYTE2LEN(p[tl]);
		    n += MB_BYTE2LEN(p[tl + n]);
		    mch_memmove(p, p + tl, n);
		    mb_char2bytes(c, p + n);
		}
		else
#endif
		{
		    c = *p;
		    *p = p[1];
		    p[1] = p[2];
		    p[2] = c;
		}
		/*FALLTHROUGH*/

	    case STATE_REP_INI:
		/* Check if matching with REP items from the .aff file would
		 * work.  Quickly skip if there are no REP items or the score
		 * is going to be too high anyway. */
		gap = &lp->lp_slang->sl_rep;
		if (gap->ga_len == 0
			       || sp->ts_score + SCORE_REP >= su->su_maxscore)
		{
		    sp->ts_state = STATE_FINAL;
		    break;
		}

		/* Use the first byte to quickly find the first entry that
		 * may match.  If the index is -1 there is none. */
		sp->ts_curi = lp->lp_slang->sl_rep_first[fword[sp->ts_fidx]];
		if (sp->ts_curi < 0)
		{
		    sp->ts_state = STATE_FINAL;
		    break;
		}

		sp->ts_state = STATE_REP;
		/*FALLTHROUGH*/

	    case STATE_REP:
		/* Try matching with REP items from the .aff file.  For each
		 * match replace the characters and check if the resulting
		 * word is valid. */
		p = fword + sp->ts_fidx;

		gap = &lp->lp_slang->sl_rep;
		while (sp->ts_curi < gap->ga_len)
		{
		    ftp = (fromto_T *)gap->ga_data + sp->ts_curi++;
		    if (*ftp->ft_from != *p)
		    {
			/* past possible matching entries */
			sp->ts_curi = gap->ga_len;
			break;
		    }
		    if (STRNCMP(ftp->ft_from, p, STRLEN(ftp->ft_from)) == 0
			    && try_deeper(su, stack, depth, SCORE_REP))
		    {
			/* Need to undo this afterwards. */
			sp->ts_state = STATE_REP_UNDO;

			/* Change the "from" to the "to" string. */
			++depth;
			fl = STRLEN(ftp->ft_from);
			tl = STRLEN(ftp->ft_to);
			if (fl != tl)
			    mch_memmove(p + tl, p + fl, STRLEN(p + fl) + 1);
			mch_memmove(p, ftp->ft_to, tl);
			stack[depth].ts_fidxtry = sp->ts_fidx + tl;
#ifdef FEAT_MBYTE
			stack[depth].ts_tcharlen = 0;
#endif
			break;
		    }
		}

		if (sp->ts_curi >= gap->ga_len)
		    /* No (more) matches. */
		    sp->ts_state = STATE_FINAL;

		break;

	    case STATE_REP_UNDO:
		/* Undo a REP replacement and continue with the next one. */
		ftp = (fromto_T *)lp->lp_slang->sl_rep.ga_data
							    + sp->ts_curi - 1;
		fl = STRLEN(ftp->ft_from);
		tl = STRLEN(ftp->ft_to);
		p = fword + sp->ts_fidx;
		if (fl != tl)
		    mch_memmove(p + fl, p + tl, STRLEN(p + tl) + 1);
		mch_memmove(p, ftp->ft_from, fl);
		sp->ts_state = STATE_REP;
		break;

	    default:
		/* Did all possible states at this level, go up one level. */
		--depth;
	    }

	    line_breakcheck();
	}
    }
}

/*
 * Try going one level deeper in the tree.
 */
    static int
try_deeper(su, stack, depth, score_add)
    suginfo_T	*su;
    trystate_T	*stack;
    int		depth;
    int		score_add;
{
    int		newscore;

    /* Refuse to go deeper if the scrore is getting too big. */
    newscore = stack[depth].ts_score + score_add;
    if (newscore >= su->su_maxscore)
	return FALSE;

    stack[depth + 1] = stack[depth];
    stack[depth + 1].ts_state = STATE_START;
    stack[depth + 1].ts_score = newscore;
    stack[depth + 1].ts_curi = 1;	/* start just after length byte */
    return TRUE;
}

/*
 * "fword" is a good word with case folded.  Find the matching keep-case
 * words and put it in "kword".
 * Theoretically there could be several keep-case words that result in the
 * same case-folded word, but we only find one...
 */
    static void
find_keepcap_word(slang, fword, kword)
    slang_T	*slang;
    char_u	*fword;
    char_u	*kword;
{
    char_u	uword[MAXWLEN];		/* "fword" in upper-case */
    int		depth;
    idx_T	tryidx;

    /* The following arrays are used at each depth in the tree. */
    idx_T	arridx[MAXWLEN];
    int		round[MAXWLEN];
    int		fwordidx[MAXWLEN];
    int		uwordidx[MAXWLEN];
    int		kwordlen[MAXWLEN];

    int		flen, ulen;
    int		l;
    int		len;
    int		c;
    idx_T	lo, hi, m;
    char_u	*p;
    char_u	*byts = slang->sl_kbyts;    /* array with bytes of the words */
    idx_T	*idxs = slang->sl_kidxs;    /* array with indexes */

    if (byts == NULL)
    {
	/* array is empty: "cannot happen" */
	*kword = NUL;
	return;
    }

    /* Make an all-cap version of "fword". */
    allcap_copy(fword, uword);

    /*
     * Each character needs to be tried both case-folded and upper-case.
     * All this gets very complicated if we keep in mind that changing case
     * may change the byte length of a multi-byte character...
     */
    depth = 0;
    arridx[0] = 0;
    round[0] = 0;
    fwordidx[0] = 0;
    uwordidx[0] = 0;
    kwordlen[0] = 0;
    while (depth >= 0)
    {
	if (fword[fwordidx[depth]] == NUL)
	{
	    /* We are at the end of "fword".  If the tree allows a word to end
	     * here we have found a match. */
	    if (byts[arridx[depth] + 1] == 0)
	    {
		kword[kwordlen[depth]] = NUL;
		return;
	    }

	    /* kword is getting too long, continue one level up */
	    --depth;
	}
	else if (++round[depth] > 2)
	{
	    /* tried both fold-case and upper-case character, continue one
	     * level up */
	    --depth;
	}
	else
	{
	    /*
	     * round[depth] == 1: Try using the folded-case character.
	     * round[depth] == 2: Try using the upper-case character.
	     */
#ifdef FEAT_MBYTE
	    if (has_mbyte)
	    {
		flen = mb_ptr2len_check(fword + fwordidx[depth]);
		ulen = mb_ptr2len_check(uword + uwordidx[depth]);
	    }
	    else
#endif
		ulen = flen = 1;
	    if (round[depth] == 1)
	    {
		p = fword + fwordidx[depth];
		l = flen;
	    }
	    else
	    {
		p = uword + uwordidx[depth];
		l = ulen;
	    }

	    for (tryidx = arridx[depth]; l > 0; --l)
	    {
		/* Perform a binary search in the list of accepted bytes. */
		len = byts[tryidx++];
		c = *p++;
		lo = tryidx;
		hi = tryidx + len - 1;
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
		tryidx = idxs[lo];
	    }

	    if (l == 0)
	    {
		/*
		 * Found the matching char.  Copy it to "kword" and go a
		 * level deeper.
		 */
		if (round[depth] == 1)
		{
		    STRNCPY(kword + kwordlen[depth], fword + fwordidx[depth],
									flen);
		    kwordlen[depth + 1] = kwordlen[depth] + flen;
		}
		else
		{
		    STRNCPY(kword + kwordlen[depth], uword + uwordidx[depth],
									ulen);
		    kwordlen[depth + 1] = kwordlen[depth] + ulen;
		}
		fwordidx[depth + 1] = fwordidx[depth] + flen;
		uwordidx[depth + 1] = uwordidx[depth] + ulen;

		++depth;
		arridx[depth] = tryidx;
		round[depth] = 0;
	    }
	}
    }

    /* Didn't find it: "cannot happen". */
    *kword = NUL;
}

/*
 * Find suggestions by comparing the word in a sound-a-like form.
 */
    static void
spell_try_soundalike(su)
    suginfo_T	*su;
{
    char_u	salword[MAXWLEN];
    char_u	tword[MAXWLEN];
    char_u	tfword[MAXWLEN];
    char_u	tsalword[MAXWLEN];
    idx_T	arridx[MAXWLEN];
    int		curi[MAXWLEN];
    langp_T	*lp;
    char_u	*byts;
    idx_T	*idxs;
    int		depth;
    int		c;
    idx_T	n;
    int		round;
    int		flags;
    int		score, sound_score;
    char_u	*bp, *sp;

    for (lp = LANGP_ENTRY(curwin->w_buffer->b_langp, 0);
						   lp->lp_slang != NULL; ++lp)
    {
	if (lp->lp_slang->sl_sal.ga_len > 0)
	{
	    /* soundfold the bad word */
	    spell_soundfold(lp->lp_slang, su->su_fbadword, salword);

	    /*
	     * Go through the whole tree, soundfold each word and compare.
	     * round 1: use the case-folded tree.
	     * round 2: use the keep-case tree.
	     */
	    for (round = 1; round <= 2; ++round)
	    {
		if (round == 1)
		{
		    byts = lp->lp_slang->sl_fbyts;
		    idxs = lp->lp_slang->sl_fidxs;
		}
		else
		{
		    byts = lp->lp_slang->sl_kbyts;
		    idxs = lp->lp_slang->sl_kidxs;
		}

		depth = 0;
		arridx[0] = 0;
		curi[0] = 1;
		while (depth >= 0 && !got_int)
		{
		    if (curi[depth] > byts[arridx[depth]])
			/* Done all bytes at this node, go up one level. */
			--depth;
		    else
		    {
			/* Do one more byte at this node. */
			n = arridx[depth] + curi[depth];
			++curi[depth];
			c = byts[n];
			if (c == 0)
			{
			    /* End of word, deal with the word. */
			    flags = (int)idxs[n];
			    if (round == 2 || (flags & WF_KEEPCAP) == 0)
			    {
				tword[depth] = NUL;
				if (round == 1)
				    spell_soundfold(lp->lp_slang,
							     tword, tsalword);
				else
				{
				    /* In keep-case tree need to case-fold the
				     * word. */
				    (void)spell_casefold(tword, depth,
							     tfword, MAXWLEN);
				    spell_soundfold(lp->lp_slang,
							    tfword, tsalword);
				}

				/*
				 * Accept the word if the sound-folded words
				 * are (almost) equal.
				 */
				for (bp = salword, sp = tsalword; *bp == *sp;
								   ++bp, ++sp)
				    if (*bp == NUL)
					break;

				if (*bp == *sp)
				    /* equal */
				    sound_score = 0;
				else if (*bp != NUL && bp[1] != NUL
					&& *bp == sp[1] && bp[1] == *sp
					       && STRCMP(bp + 2, sp + 2) == 0)
				    /* swap two bytes */
				    sound_score = SCORE_SWAP;
				else if (STRCMP(bp + 1, sp) == 0)
				    /* delete byte */
				    sound_score = SCORE_DEL;
				else if (STRCMP(bp, sp + 1) == 0)
				    /* insert byte */
				    sound_score = SCORE_INS;
				else if (STRCMP(bp + 1, sp + 1) == 0)
				    /* skip one byte */
				    sound_score = SCORE_SUBST;
				else
				    /* not equal or similar */
				    sound_score = SCORE_MAXMAX;

				if (sound_score < SCORE_MAXMAX)
				{
				    char_u	cword[MAXWLEN];
				    char_u	*p;

				    if (round == 1 && flags != 0)
				    {
					/* Need to fix case according to
					 * "flags". */
					make_case_word(tword, cword, flags);
					p = cword;
				    }
				    else
					p = tword;

				    /* Compute the score. */
				    score = spell_edit_score(su->su_badword, p);
#ifdef RESCORE
				    /* give a bonus for the good word sounding
				     * the same as the bad word */
				    add_suggestion(su, tword,
						 RESCORE(score, sound_score),
									TRUE);
#else
				    add_suggestion(su, tword,
							 score + sound_score);
#endif
				}
			    }

			    /* Skip over other NUL bytes. */
			    while (byts[n + 1] == 0)
			    {
				++n;
				++curi[depth];
			    }
			}
			else
			{
			    /* Normal char, go one level deeper. */
			    tword[depth++] = c;
			    arridx[depth] = idxs[n];
			    curi[depth] = 1;
			}
		    }

		    line_breakcheck();
		}
	    }
	}
    }
}

/*
 * Copy "fword" to "cword", fixing case according to "flags".
 */
    static void
make_case_word(fword, cword, flags)
    char_u	*fword;
    char_u	*cword;
    int		flags;
{
    if (flags & WF_ALLCAP)
	/* Make it all upper-case */
	allcap_copy(fword, cword);
    else if (flags & WF_ONECAP)
	/* Make the first letter upper-case */
	onecap_copy(fword, cword, TRUE);
    else
	/* Use goodword as-is. */
	STRCPY(cword, fword);
}

/*
 * Use map string "map" for languages "lp".
 */
    static void
set_map_str(lp, map)
    slang_T	*lp;
    char_u	*map;
{
    char_u	*p;
    int		headc = 0;
    int		c;
    int		i;

    if (*map == NUL)
    {
	lp->sl_has_map = FALSE;
	return;
    }
    lp->sl_has_map = TRUE;

    /* Init the array and hash table empty. */
    for (i = 0; i < 256; ++i)
	lp->sl_map_array[i] = 0;
#ifdef FEAT_MBYTE
    hash_init(&lp->sl_map_hash);
#endif

    /*
     * The similar characters are stored separated with slashes:
     * "aaa/bbb/ccc/".  Fill sl_map_array[c] with the character before c and
     * before the same slash.  For characters above 255 sl_map_hash is used.
     */
    for (p = map; *p != NUL; )
    {
#ifdef FEAT_MBYTE
	c = mb_ptr2char_adv(&p);
#else
	c = *p++;
#endif
	if (c == '/')
	    headc = 0;
	else
	{
	    if (headc == 0)
		 headc = c;

#ifdef FEAT_MBYTE
	    /* Characters above 255 don't fit in sl_map_array[], put them in
	     * the hash table.  Each entry is the char, a NUL the headchar and
	     * a NUL. */
	    if (c >= 256)
	    {
		int	    cl = mb_char2len(c);
		int	    headcl = mb_char2len(headc);
		char_u	    *b;
		hash_T	    hash;
		hashitem_T  *hi;

		b = alloc((unsigned)(cl + headcl + 2));
		if (b == NULL)
		    return;
		mb_char2bytes(c, b);
		b[cl] = NUL;
		mb_char2bytes(headc, b + cl + 1);
		b[cl + 1 + headcl] = NUL;
		hash = hash_hash(b);
		hi = hash_lookup(&lp->sl_map_hash, b, hash);
		if (HASHITEM_EMPTY(hi))
		    hash_add_item(&lp->sl_map_hash, hi, b, hash);
		else
		{
		    /* This should have been checked when generating the .spl
		     * file. */
		    EMSG(_("E999: duplicate char in MAP entry"));
		    vim_free(b);
		}
	    }
	    else
#endif
		lp->sl_map_array[c] = headc;
	}
    }
}

/*
 * Return TRUE if "c1" and "c2" are similar characters according to the MAP
 * lines in the .aff file.
 */
    static int
similar_chars(slang, c1, c2)
    slang_T	*slang;
    int		c1;
    int		c2;
{
    int		m1, m2;
#ifdef FEAT_MBYTE
    char_u	buf[MB_MAXBYTES];
    hashitem_T  *hi;

    if (c1 >= 256)
    {
	buf[mb_char2bytes(c1, buf)] = 0;
	hi = hash_find(&slang->sl_map_hash, buf);
	if (HASHITEM_EMPTY(hi))
	    m1 = 0;
	else
	    m1 = mb_ptr2char(hi->hi_key + STRLEN(hi->hi_key) + 1);
    }
    else
#endif
	m1 = slang->sl_map_array[c1];
    if (m1 == 0)
	return FALSE;


#ifdef FEAT_MBYTE
    if (c2 >= 256)
    {
	buf[mb_char2bytes(c2, buf)] = 0;
	hi = hash_find(&slang->sl_map_hash, buf);
	if (HASHITEM_EMPTY(hi))
	    m2 = 0;
	else
	    m2 = mb_ptr2char(hi->hi_key + STRLEN(hi->hi_key) + 1);
    }
    else
#endif
	m2 = slang->sl_map_array[c2];

    return m1 == m2;
}

/*
 * Add a suggestion to the list of suggestions.
 * Do not add a duplicate suggestion or suggestions with a bad score.
 * When "use_score" is not zero it's used, otherwise the score is computed
 * with spell_edit_score().
 */
    static void
add_suggestion(su, goodword, score
#ifdef RESCORE
	    , had_bonus
#endif
	    )
    suginfo_T	*su;
    char_u	*goodword;
    int		score;
#ifdef RESCORE
    int		had_bonus;	/* set st_had_bonus */
#endif
{
    suggest_T   *stp;
    int		i;
#ifdef SOUNDFOLD_SCORE
    char_u	fword[MAXWLEN];
    char_u	salword[MAXWLEN];
#endif

    /* Check that the word wasn't banned. */
    if (was_banned(su, goodword))
	return;

    if (score <= su->su_maxscore)
    {
#ifdef SOUNDFOLD_SCORE
	/* Add to the score when the word sounds differently.
	 * This is slow... */
	if (su->su_slang->sl_sal.ga_len > 0)
	    score += spell_sound_score(su->su_slang, fword, su->su_salword);
#endif

	/* Check if the word is already there. */
	stp = &SUG(su, 0);
	for (i = su->su_ga.ga_len - 1; i >= 0; --i)
	    if (STRCMP(stp[i].st_word, goodword) == 0)
	    {
		/* Found it.  Remember the lowest score. */
		if (stp[i].st_score > score)
		{
		    stp[i].st_score = score;
#ifdef RESCORE
		    stp[i].st_had_bonus = had_bonus;
#endif
		}
		break;
	    }

	if (i < 0 && ga_grow(&su->su_ga, 1) == OK)
	{
	    /* Add a suggestion. */
	    stp = &SUG(su, su->su_ga.ga_len);
	    stp->st_word = vim_strsave(goodword);
	    if (stp->st_word != NULL)
	    {
		stp->st_score = score;
#ifdef RESCORE
		stp->st_had_bonus = had_bonus;
#endif
		stp->st_orglen = su->su_badlen;
		++su->su_ga.ga_len;

		/* If we have too many suggestions now, sort the list and keep
		 * the best suggestions. */
		if (su->su_ga.ga_len > SUG_MAX_COUNT)
		    cleanup_suggestions(su, SUG_CLEAN_COUNT);
	    }
	}
    }
}

/*
 * Add a word to be banned.
 */
    static void
add_banned(su, word)
    suginfo_T	*su;
    char_u	*word;
{
    char_u	*s = vim_strsave(word);
    hash_T	hash;
    hashitem_T	*hi;

    if (s != NULL)
    {
	hash = hash_hash(s);
	hi = hash_lookup(&su->su_banned, s, hash);
	if (HASHITEM_EMPTY(hi))
	    hash_add_item(&su->su_banned, hi, s, hash);
    }
}

/*
 * Return TRUE if a word appears in the list of banned words.
 */
    static int
was_banned(su, word)
    suginfo_T	*su;
    char_u	*word;
{
    hashitem_T	*hi = hash_find(&su->su_banned, word);

    return !HASHITEM_EMPTY(hi);
}

/*
 * Free the banned words in "su".
 */
    static void
free_banned(su)
    suginfo_T	*su;
{
    int		todo;
    hashitem_T	*hi;

    todo = su->su_banned.ht_used;
    for (hi = su->su_banned.ht_array; todo > 0; ++hi)
    {
	if (!HASHITEM_EMPTY(hi))
	{
	    vim_free(hi->hi_key);
	    --todo;
	}
    }
    hash_clear(&su->su_banned);
}

#ifdef RESCORE
/*
 * Recompute the score if sound-folding is possible.  This is slow,
 * thus only done for the final results.
 */
    static void
rescore_suggestions(su)
    suginfo_T	*su;
{
    langp_T	*lp;
    suggest_T	*stp;
    char_u	sal_badword[MAXWLEN];
    int		score;
    int		i;

    for (lp = LANGP_ENTRY(curwin->w_buffer->b_langp, 0);
						   lp->lp_slang != NULL; ++lp)
    {
	if (lp->lp_slang->sl_sal.ga_len > 0)
	{
	    /* soundfold the bad word */
	    spell_soundfold(lp->lp_slang, su->su_fbadword, sal_badword);

	    for (i = 0; i < su->su_ga.ga_len; ++i)
	    {
		stp = &SUG(su, i);
		if (!stp->st_had_bonus)
		{
		    score = spell_sound_score(lp->lp_slang, stp->st_word,
								 sal_badword);
		    stp->st_score = RESCORE(stp->st_score, score);
		}
	    }
	    break;
	}
    }
}
#endif

static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
sug_compare __ARGS((const void *s1, const void *s2));

/*
 * Function given to qsort() to sort the suggestions on st_score.
 */
    static int
#ifdef __BORLANDC__
_RTLENTRYF
#endif
sug_compare(s1, s2)
    const void	*s1;
    const void	*s2;
{
    suggest_T	*p1 = (suggest_T *)s1;
    suggest_T	*p2 = (suggest_T *)s2;

    return p1->st_score - p2->st_score;
}

/*
 * Cleanup the suggestions:
 * - Sort on score.
 * - Remove words that won't be displayed.
 */
    static void
cleanup_suggestions(su, keep)
    suginfo_T	*su;
    int		keep;		/* nr of suggestions to keep */
{
    suggest_T   *stp = &SUG(su, 0);
    int		i;

    /* Sort the list. */
    qsort(su->su_ga.ga_data, (size_t)su->su_ga.ga_len,
					      sizeof(suggest_T), sug_compare);

    /* Truncate the list to the number of suggestions that will be displayed. */
    if (su->su_ga.ga_len > keep)
    {
	for (i = keep; i < su->su_ga.ga_len; ++i)
	    vim_free(stp[i].st_word);
	su->su_ga.ga_len = keep;
	su->su_maxscore = stp[keep - 1].st_score;
    }
}

/*
 * Turn "inword" into its sound-a-like equivalent in "res[MAXWLEN]".
 */
    static void
spell_soundfold(slang, inword, res)
    slang_T	*slang;
    char_u	*inword;
    char_u	*res;
{
    fromto_T	*ftp;
    char_u	word[MAXWLEN];
#ifdef FEAT_MBYTE
    int		l;
    int		found_mbyte = FALSE;
#endif
    char_u	*s;
    char_u	*t;
    int		i, j, z;
    int		n, k = 0;
    int		z0;
    int		k0;
    int		n0;
    int		c;
    int		pri;
    int		p0 = -333;
    int		c0;

    /* Remove accents, if wanted.  We actually remove all non-word characters.
     * But keep white space. */
    if (slang->sl_rem_accents)
    {
	t = word;
	for (s = inword; *s != NUL; )
	{
	    if (vim_iswhite(*s))
		*t++ = *s++;
#ifdef FEAT_MBYTE
	    else if (has_mbyte)
	    {
		l = mb_ptr2len_check(s);
		if (SPELL_ISWORDP(s))
		{
		    mch_memmove(t, s, l);
		    t += l;
		    if (l > 1)
			found_mbyte = TRUE;
		}
		s += l;
	    }
#endif
	    else
	    {
		if (SPELL_ISWORDP(s))
		    *t++ = *s;
		++s;
	    }
	}
	*t = NUL;
    }
    else
    {
#ifdef FEAT_MBYTE
	if (has_mbyte)
	    for (s = inword; *s != NUL; s += l)
		if ((l = mb_ptr2len_check(s)) > 1)
		{
		    found_mbyte = TRUE;
		    break;
		}
#endif
	STRCPY(word, inword);
    }

#ifdef FEAT_MBYTE
    /* If there are multi-byte characters in the word return it as-is, because
     * the following won't work. */
    if (found_mbyte)
    {
	STRCPY(res, word);
	return;
    }
#endif

    ftp = (fromto_T *)slang->sl_sal.ga_data;

    /*
     * This comes from Aspell phonet.cpp.  Converted from C++ to C.
     * Changed to keep spaces.
     * TODO: support for multi-byte chars.
     */
    i = j = z = 0;
    while ((c = word[i]) != NUL)
    {
	n = slang->sl_sal_first[c];
	z0 = 0;

	if (n >= 0)
	{
	    /* check all rules for the same letter */
	    while (ftp[n].ft_from[0] == c)
	    {
		/* check whole string */
		k = 1;   /* number of found letters */
		pri = 5;   /* default priority */
		s = ftp[n].ft_from;
		s++;     /* important for (see below)  "*(s-1)" */

		/* Skip over normal letters that match with the word. */
		while (*s != NUL && word[i + k] == *s
			&& !vim_isdigit(*s) && strchr("(-<^$", *s) == NULL)
		{
		    k++;
		    s++;
		}

		if (*s == '(')
		{
		    /* check alternate letters in "(..)" */
		    for (t = s + 1; *t != ')' && *t != NUL; ++t)
			if (*t == word[i + k])
			{
			    /* match */
			    ++k;
			    for (s = t + 1; *s != NUL; ++s)
				if (*s == ')')
				{
				    ++s;
				    break;
				}
			    break;
			}
		}

		p0 = *s;
		k0 = k;
		while (*s == '-' && k > 1)
		{
		    k--;
		    s++;
		}
		if (*s == '<')
		    s++;
		if (vim_isdigit(*s))
		{
		    /* determine priority */
		    pri = *s - '0';
		    s++;
		}
		if (*s == '^' && *(s + 1) == '^')
		    s++;

		if (*s == NUL
			|| (*s == '^'
			    && (i == 0 || !(word[i - 1] == ' '
					      || SPELL_ISWORDP(word + i - 1)))
			    && (*(s + 1) != '$'
				|| (!SPELL_ISWORDP(word + i + k0))))
			|| (*s == '$' && i > 0
			    && SPELL_ISWORDP(word + i - 1)
			    && (!SPELL_ISWORDP(word + i + k0))))
		{
		    /* search for followup rules, if:    */
		    /* followup and k > 1  and  NO '-' in searchstring */
		    c0 = word[i + k - 1];
		    n0 = slang->sl_sal_first[c0];

		    if (slang->sl_followup && k > 1 && n0 >= 0
			    && p0 != '-' && word[i + k] != NUL)
		    {
			/* test follow-up rule for "word[i + k]" */
			while (ftp[n0].ft_from[0] == c0)
			{

			    /* check whole string */
			    k0 = k;
			    p0 = 5;
			    s = ftp[n0].ft_from;
			    s++;
			    while (*s != NUL && word[i+k0] == *s
				    && !vim_isdigit(*s)
						&& strchr("(-<^$",*s) == NULL)
			    {
				k0++;
				s++;
			    }
			    if (*s == '(')
			    {
				/* check alternate letters in "(..)" */
				for (t = s + 1; *t != ')' && *t != NUL; ++t)
				    if (*t == word[i + k0])
				    {
					/* match */
					++k0;
					for (s = t + 1; *s != NUL; ++s)
					    if (*s == ')')
					    {
						++s;
						break;
					    }
					break;
				    }
			    }
			    while (*s == '-')
			    {
				/* "k0" gets NOT reduced  */
				/* because "if (k0 == k)" */
				s++;
			    }
			    if (*s == '<')
				s++;
			    if (vim_isdigit(*s))
			    {
				p0 = *s - '0';
				s++;
			    }

			    if (*s == NUL
				    /* *s == '^' cuts */
				    || (*s == '$'
					    && !SPELL_ISWORDP(word + i + k0)))
			    {
				if (k0 == k)
				{
				    /* this is just a piece of the string */
				    ++n0;
				    continue;
				}

				if (p0 < pri)
				{
				    /* priority too low */
				    ++n0;
				    continue;
				}
				/* rule fits; stop search */
				break;
			    }
			    ++n0;
			}

			if (p0 >= pri && ftp[n0].ft_from[0] == c0)
			{
			    ++n;
			    continue;
			}
		    }

		    /* replace string */
		    s = ftp[n].ft_to;
		    p0 = (ftp[n].ft_from[0] != NUL
			    && vim_strchr(ftp[n].ft_from + 1,
							'<') != NULL) ? 1 : 0;
		    if (p0 == 1 && z == 0)
		    {
			/* rule with '<' is used */
			if (j > 0 && *s != NUL
				&& (res[j - 1] == c || res[j - 1] == *s))
			    j--;
			z0 = 1;
			z = 1;
			k0 = 0;
			while (*s != NUL && word[i+k0] != NUL)
			{
			    word[i + k0] = *s;
			    k0++;
			    s++;
			}
			if (k > k0)
			    mch_memmove(word + i + k0, word + i + k,
						    STRLEN(word + i + k) + 1);

			/* new "actual letter" */
			c = word[i];
		    }
		    else
		    {
			/* no '<' rule used */
			i += k - 1;
			z = 0;
			while (*s != NUL && s[1] != NUL && j < MAXWLEN)
			{
			    if (j == 0 || res[j - 1] != *s)
			    {
				res[j] = *s;
				j++;
			    }
			    s++;
			}
			/* new "actual letter" */
			c = *s;
			if (ftp[n].ft_from[0] != NUL
					 && strstr((char *)ftp[n].ft_from + 1,
								"^^") != NULL)
			{
			    if (c != NUL)
			    {
				res[j] = c;
				j++;
			    }
			    mch_memmove(word, word + i + 1,
						    STRLEN(word + i + 1) + 1);
			    i = 0;
			    z0 = 1;
			}
		    }
		    break;
		}
		++n;
	    }
	}
	else if (vim_iswhite(c))
	{
	    c = ' ';
	    k = 1;
	}

	if (z0 == 0)
	{
	    if (k && !p0 && j < MAXWLEN && c != NUL
		    && (!slang->sl_collapse || j == 0 || res[j - 1] != c))
	    {
		/* condense only double letters */
		res[j] = c;
		j++;
	    }

	    i++;
	    z = 0;
	    k = 0;
	}
    }

    res[j] = NUL;
}

#if defined(RESCORE) || defined(SOUNDFOLD_SCORE)
/*
 * Return the score for how much words sound different.
 */
    static int
spell_sound_score(slang, goodword, badsound)
    slang_T	*slang;
    char_u	*goodword;	/* good word */
    char_u	*badsound;	/* sound-folded bad word */
{
    char_u	fword[MAXWLEN];
    char_u	goodsound[MAXWLEN];
    int		score;

    /* Case-fold the word, needed for sound folding. */
    (void)spell_casefold(goodword, STRLEN(goodword), fword, MAXWLEN);

    /* sound-fold the good word */
    spell_soundfold(slang, fword, goodsound);

    /* compute the edit distance-score of the sounds */
    score = spell_edit_score(badsound, goodsound);

    /* Correction: adding/inserting "*" at the start (word starts with vowel)
     * shouldn't be counted so much, vowels halfway the word aren't counted at
     * all. */
    if (*badsound != *goodsound && (*badsound == '*' || *goodsound == '*'))
	score -= SCORE_DEL / 2;

    return score;
}
#endif

/*
 * Compute the "edit distance" to turn "badword" into "goodword".  The less
 * deletes/inserts/swaps are required the lower the score.
 *
 * The algorithm comes from Aspell editdist.cpp, edit_distance().
 * It has been converted from C++ to C and modified to support multi-byte
 * characters.
 */
    static int
spell_edit_score(badword, goodword)
    char_u	*badword;
    char_u	*goodword;
{
    int		*cnt;
    int		badlen, goodlen;
    int		j, i;
    int		t;
    int		bc, gc;
    int		pbc, pgc;
#ifdef FEAT_MBYTE
    char_u	*p;
    int		wbadword[MAXWLEN];
    int		wgoodword[MAXWLEN];

    if (has_mbyte)
    {
	/* Get the characters from the multi-byte strings and put them in an
	 * int array for easy access. */
	for (p = badword, badlen = 0; *p != NUL; )
	    wbadword[badlen++] = mb_ptr2char_adv(&p);
	++badlen;
	for (p = goodword, goodlen = 0; *p != NUL; )
	    wgoodword[goodlen++] = mb_ptr2char_adv(&p);
	++goodlen;
    }
    else
#endif
    {
	badlen = STRLEN(badword) + 1;
	goodlen = STRLEN(goodword) + 1;
    }

    /* We use "cnt" as an array: CNT(badword_idx, goodword_idx). */
#define CNT(a, b)   cnt[(a) + (b) * (badlen + 1)]
    cnt = (int *)lalloc((long_u)(sizeof(int) * (badlen + 1) * (goodlen + 1)),
									TRUE);
    if (cnt == NULL)
	return 0;	/* out of memory */

    CNT(0, 0) = 0;
    for (j = 1; j <= goodlen; ++j)
	CNT(0, j) = CNT(0, j - 1) + SCORE_DEL;

    for (i = 1; i <= badlen; ++i)
    {
	CNT(i, 0) = CNT(i - 1, 0) + SCORE_INS;
	for (j = 1; j <= goodlen; ++j)
	{
#ifdef FEAT_MBYTE
	    if (has_mbyte)
	    {
		bc = wbadword[i - 1];
		gc = wgoodword[j - 1];
	    }
	    else
#endif
	    {
		bc = badword[i - 1];
		gc = goodword[j - 1];
	    }
	    if (bc == gc)
		CNT(i, j) = CNT(i - 1, j - 1);
	    else
	    {
		/* Use a better score when there is only a case difference. */
		if (SPELL_TOFOLD(bc) == SPELL_TOFOLD(gc))
		    CNT(i, j) = SCORE_ICASE + CNT(i - 1, j - 1);
		else
		    CNT(i, j) = SCORE_SUBST + CNT(i - 1, j - 1);

		if (i > 1 && j > 1)
		{
#ifdef FEAT_MBYTE
		    if (has_mbyte)
		    {
			pbc = wbadword[i - 2];
			pgc = wgoodword[j - 2];
		    }
		    else
#endif
		    {
			pbc = badword[i - 2];
			pgc = goodword[j - 2];
		    }
		    if (bc == pgc && pbc == gc)
		    {
			t = SCORE_SWAP + CNT(i - 2, j - 2);
			if (t < CNT(i, j))
			    CNT(i, j) = t;
		    }
		}
		t = SCORE_DEL + CNT(i - 1, j);
		if (t < CNT(i, j))
		    CNT(i, j) = t;
		t = SCORE_INS + CNT(i, j - 1);
		if (t < CNT(i, j))
		    CNT(i, j) = t;
	    }
	}
    }
    return CNT(badlen - 1, goodlen - 1);
}

#endif  /* FEAT_SYN_HL */
