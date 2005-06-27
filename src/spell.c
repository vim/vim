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
 * There are two word trees: one with case-folded words and one with words in
 * original case.  The second one is only used for keep-case words and is
 * usually small.
 *
 * There is one additional tree for when prefixes are not applied when
 * generating the .spl file.  This tree stores all the possible prefixes, as
 * if they were words.  At each word (prefix) end the prefix nr is stored, the
 * following word must support this prefix nr.  And the condition nr is
 * stored, used to lookup the condition that the word must match with.
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
 * Use this to adjust the score after finding suggestions, based on the
 * suggested word sounding like the bad word.  This is much faster than doing
 * it for every possible suggestion.
 * Disadvantage: When "the" is typed as "hte" it sounds different and goes
 * down in the list.
 * Used when 'spellsuggest' is set to "best".
 */
#define RESCORE(word_score, sound_score) ((3 * word_score + sound_score) / 4)

/*
 * The double scoring mechanism is based on the principle that there are two
 * kinds of spelling mistakes:
 * 1. You know how to spell the word, but mistype something.  This results in
 *    a small editing distance (character swapped/omitted/inserted) and
 *    possibly a word that sounds completely different.
 * 2. You don't know how to spell the word and type something that sounds
 *    right.  The edit distance can be big but the word is similar after
 *    sound-folding.
 * Since scores for these two mistakes will be very different we use a list
 * for each.
 * The sound-folding is slow, only do double scoring when 'spellsuggest' is
 * "double".
 */

/*
 * Vim spell file format: <HEADER>
 *			  <SUGGEST>
 *			  <LWORDTREE>
 *			  <KWORDTREE>
 *			  <PREFIXTREE>
 *
 * <HEADER>: <fileID>
 *		<regioncnt> <regionname> ...
 *		<charflagslen> <charflags>
 *		<fcharslen> <fchars>
 *		<midwordlen> <midword>
 *		<prefcondcnt> <prefcond> ...
 *
 * <fileID>     10 bytes    "VIMspell08"
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
 * <midwordlen> 2 bytes     Number of bytes in <midword>.
 * <midword>    N bytes	    Characters that are word characters only when used
 *			    in the middle of a word.
 *
 * <prefcondcnt> 2 bytes    Number of <prefcond> items following.
 *
 * <prefcond> : <condlen> <condstr>
 *
 * <condlen>	1 byte	    Length of <condstr>.
 *
 * <condstr>	N bytes	    Condition for the prefix.
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
 * <KWORDTREE>: <wordtree>
 *
 * <PREFIXTREE>: <wordtree>
 *
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
 * <sibling>: <byte> [ <nodeidx> <xbyte>
 *		      | <flags> [<region>] [<prefixID>]
 *		      | <prefixID> <prefcondnr> ]
 *
 * <byte>	1 byte	    Byte value of the sibling.  Special cases:
 *			    BY_NOFLAGS: End of word without flags and for all
 *					regions.
 *					For PREFIXTREE <prefixID> and
 *					<prefcondnr> follow.
 *			    BY_FLAGS: End of word, <flags> follow.
 *					For PREFIXTREE <prefixID> and
 *					<prefcondnr> follow for rare prefix.
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
 *			    WF_PFX	<prefixID> follows
 *
 * <region>	1 byte	    Bitmask for regions in which word is valid.  When
 *			    omitted it's valid in all regions.
 *			    Lowest bit is for region 1.
 *
 * <prefixID>	1 byte	    ID of prefix that can be used with this word.  For
 *			    PREFIXTREE used for the required prefix ID.
 *
 * <prefcondnr>	2 bytes	    Prefix condition number, index in <prefcond> list
 *			    from HEADER.
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
#define WF_PFX	    0x20	/* prefix ID list follows */
#define WF_KEEPCAP  0x80	/* keep-case word */

#define WF_CAPMASK (WF_ONECAP | WF_ALLCAP | WF_KEEPCAP)

#define WF_RAREPFX  0x1000000	/* in sl_pidxs: flag for rare postponed
				   prefix; must be above prefixID (one byte)
				   and prefcondnr (two bytes) */

#define BY_NOFLAGS  0		/* end of word without flags or region */
#define BY_FLAGS    1		/* end of word, flag byte follows */
#define BY_INDEX    2		/* child is shared, index follows */
#define BY_SPECIAL  BY_INDEX	/* hightest special byte value */

/* Info from "REP" and "SAL" entries in ".aff" file used in si_rep, sl_rep,
 * and si_sal.  Not for sl_sal!
 * One replacement: from "ft_from" to "ft_to". */
typedef struct fromto_S
{
    char_u	*ft_from;
    char_u	*ft_to;
} fromto_T;

/* Info from "SAL" entries in ".aff" file used in sl_sal.
 * The info is split for quick processing by spell_soundfold().
 * Note that "sm_oneof" and "sm_rules" point into sm_lead. */
typedef struct salitem_S
{
    char_u	*sm_lead;	/* leading letters */
    int		sm_leadlen;	/* length of "sm_lead" */
    char_u	*sm_oneoff;	/* letters from () or NULL */
    char_u	*sm_rules;	/* rules like ^, $, priority */
    char_u	*sm_to;		/* replacement. */
} salitem_T;

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
    char_u	*sl_pbyts;	/* prefix tree word bytes */
    idx_T	*sl_pidxs;	/* prefix tree word indexes */

    char_u	sl_regions[17];	/* table with up to 8 region names plus NUL */

    int		sl_prefixcnt;	/* number of items in "sl_prefprog" */
    regprog_T	**sl_prefprog;	/* table with regprogs for prefixes */

    garray_T	sl_rep;		/* list of fromto_T entries from REP lines */
    short	sl_rep_first[256];  /* indexes where byte first appears, -1 if
				       there is none */
    garray_T	sl_sal;		/* list of salitem_T entries from SAL lines */
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

#define VIMSPELLMAGIC "VIMspell08"  /* string at start of Vim spell file */
#define VIMSPELLMAGICL 10

/*
 * Information used when looking for suggestions.
 */
typedef struct suginfo_S
{
    garray_T	su_ga;		    /* suggestions, contains "suggest_T" */
    int		su_maxcount;	    /* max. number of suggestions displayed */
    int		su_maxscore;	    /* maximum score for adding to su_ga */
    garray_T	su_sga;		    /* like su_ga, sound-folded scoring */
    char_u	*su_badptr;	    /* start of bad word in line */
    int		su_badlen;	    /* length of detected bad word in line */
    int		su_badflags;	    /* caps flags for bad word */
    char_u	su_badword[MAXWLEN]; /* bad word truncated at su_badlen */
    char_u	su_fbadword[MAXWLEN]; /* su_badword case-folded */
    hashtab_T	su_banned;	    /* table with banned words */
} suginfo_T;

/* One word suggestion.  Used in "si_ga". */
typedef struct suggest_S
{
    char_u	*st_word;	/* suggested word, allocated string */
    int		st_orglen;	/* length of replaced text */
    int		st_score;	/* lower is better */
    int		st_altscore;	/* used when st_score compares equal */
    int		st_salscore;	/* st_score is for soundalike */
    int		st_had_bonus;	/* bonus already included in score */
} suggest_T;

#define SUG(ga, i) (((suggest_T *)(ga).ga_data)[i])

/* Number of suggestions kept when cleaning up.  When rescore_suggestions() is
 * called the score may change, thus we need to keep more than what is
 * displayed. */
#define SUG_CLEAN_COUNT(su)    ((su)->su_maxcount < 50 ? 50 : (su)->su_maxcount)

/* Threshold for sorting and cleaning up suggestions.  Don't want to keep lots
 * of suggestions that are not going to be displayed. */
#define SUG_MAX_COUNT(su)    ((su)->su_maxcount + 50)

/* score for various changes */
#define SCORE_SPLIT	149	/* split bad word */
#define SCORE_ICASE	52	/* slightly different case */
#define SCORE_REGION	70	/* word is for different region */
#define SCORE_RARE	180	/* rare word */
#define SCORE_SWAP	90	/* swap two characters */
#define SCORE_SWAP3	110	/* swap two characters in three */
#define SCORE_REP	87	/* REP replacement */
#define SCORE_SUBST	93	/* substitute a character */
#define SCORE_SIMILAR	33	/* substitute a similar character */
#define SCORE_DEL	94	/* delete a character */
#define SCORE_DELDUP	64	/* delete a duplicated character */
#define SCORE_INS	96	/* insert a character */
#define SCORE_INSDUP	66	/* insert a duplicate character */
#define SCORE_NONWORD	103	/* change non-word to word char */

#define SCORE_MAXINIT	350	/* Initial maximum score: higher == slower.
				 * 350 allows for about three changes. */

#define SCORE_BIG	SCORE_INS * 3	/* big difference */
#define SCORE_MAXMAX	999999	/* accept any score */

/*
 * Structure to store info for word matching.
 */
typedef struct matchinf_S
{
    langp_T	*mi_lp;			/* info for language and region */

    /* pointers to original text to be checked */
    char_u	*mi_word;		/* start of word being checked */
    char_u	*mi_end;		/* end of matching word so far */
    char_u	*mi_fend;		/* next char to be added to mi_fword */
    char_u	*mi_cend;		/* char after what was used for
					   mi_capflags */

    /* case-folded text */
    char_u	mi_fword[MAXWLEN + 1];	/* mi_word case-folded */
    int		mi_fwordlen;		/* nr of valid bytes in mi_fword */

    /* for when checking word after a prefix */
    int		mi_prefarridx;		/* index in sl_pidxs with list of
					   prefixID/condition */
    int		mi_prefcnt;		/* number of entries at mi_prefarridx */
    int		mi_prefixlen;		/* byte length of prefix */

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
static char_u	    spell_ismw[256];		/* flags: is midword char */
#ifdef FEAT_MBYTE
static char_u	    *spell_ismw_mb = NULL;	/* multi-byte midword chars */
#endif

#define CF_WORD		0x01
#define CF_UPPER	0x02

static void clear_spell_chartab __ARGS((spelltab_T *sp));
static int set_spell_finish __ARGS((spelltab_T	*new_st));
static int spell_iswordp __ARGS((char_u *p));
static void write_spell_prefcond __ARGS((FILE *fd, garray_T *gap));

/*
 * Return TRUE if "p" points to a word character.  Like spell_iswordp() but
 * without the special handling of a single quote.
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
 * For finding suggestions: At each node in the tree these states are tried:
 */
typedef enum
{
    STATE_START = 0,	/* At start of node check for NUL bytes (goodword
			 * ends); if badword ends there is a match, otherwise
			 * try splitting word. */
    STATE_SPLITUNDO,	/* Undo splitting. */
    STATE_ENDNUL,	/* Past NUL bytes at start of the node. */
    STATE_PLAIN,	/* Use each byte of the node. */
    STATE_DEL,		/* Delete a byte from the bad word. */
    STATE_INS,		/* Insert a byte in the bad word. */
    STATE_SWAP,		/* Swap two bytes. */
    STATE_UNSWAP,	/* Undo swap two characters. */
    STATE_SWAP3,	/* Swap two characters over three. */
    STATE_UNSWAP3,	/* Undo Swap two characters over three. */
    STATE_UNROT3L,	/* Undo rotate three characters left */
    STATE_UNROT3R,	/* Undo rotate three characters right */
    STATE_REP_INI,	/* Prepare for using REP items. */
    STATE_REP,		/* Use matching REP items from the .aff file. */
    STATE_REP_UNDO,	/* Undo a REP item replacement. */
    STATE_FINAL		/* End of this node. */
} state_T;

/*
 * Struct to keep the state at each level in suggest_try_change().
 */
typedef struct trystate_S
{
    state_T	ts_state;	/* state at this level, STATE_ */
    int		ts_score;	/* score */
    idx_T	ts_arridx;	/* index in tree array, start of node */
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
    char_u	ts_save_prewordlen; /* saved "prewordlen" */
    char_u	ts_save_splitoff;   /* su_splitoff saved here */
    char_u	ts_save_badflags;   /* su_badflags saved here */
} trystate_T;

/* values for ts_isdiff */
#define DIFF_NONE	0	/* no different byte (yet) */
#define DIFF_YES	1	/* different byte found */
#define DIFF_INSERT	2	/* inserting character */

/* mode values for find_word */
#define FIND_FOLDWORD	0	/* find word case-folded */
#define FIND_KEEPWORD	1	/* find keep-case word */
#define FIND_PREFIX	2	/* find word after prefix */

static slang_T *slang_alloc __ARGS((char_u *lang));
static void slang_free __ARGS((slang_T *lp));
static void slang_clear __ARGS((slang_T *lp));
static void find_word __ARGS((matchinf_T *mip, int mode));
static int valid_word_prefix __ARGS((int totprefcnt, int arridx, int prefid, char_u *word, slang_T *slang));
static void find_prefix __ARGS((matchinf_T *mip));
static int fold_more __ARGS((matchinf_T *mip));
static int spell_valid_case __ARGS((int origflags, int treeflags));
static int no_spell_checking __ARGS((void));
static void spell_load_lang __ARGS((char_u *lang));
static char_u *spell_enc __ARGS((void));
static void spell_load_cb __ARGS((char_u *fname, void *cookie));
static slang_T *spell_load_file __ARGS((char_u *fname, char_u *lang, slang_T *old_lp, int silent));
static idx_T read_tree __ARGS((FILE *fd, char_u *byts, idx_T *idxs, int maxidx, int startidx, int prefixtree, int maxprefcondnr));
static int find_region __ARGS((char_u *rp, char_u *region));
static int captype __ARGS((char_u *word, char_u *end));
static void spell_reload_one __ARGS((char_u *fname, int added_word));
static int set_spell_charflags __ARGS((char_u *flags, int cnt, char_u *upp));
static int set_spell_chartab __ARGS((char_u *fol, char_u *low, char_u *upp));
static void write_spell_chartab __ARGS((FILE *fd));
static int spell_casefold __ARGS((char_u *p, int len, char_u *buf, int buflen));
static void spell_find_suggest __ARGS((char_u *badptr, suginfo_T *su, int maxcount, int banbadword));
static void spell_find_cleanup __ARGS((suginfo_T *su));
static void onecap_copy __ARGS((char_u *word, char_u *wcopy, int upper));
static void allcap_copy __ARGS((char_u *word, char_u *wcopy));
static void suggest_try_special __ARGS((suginfo_T *su));
static void suggest_try_change __ARGS((suginfo_T *su));
static int try_deeper __ARGS((suginfo_T *su, trystate_T *stack, int depth, int score_add));
static void find_keepcap_word __ARGS((slang_T *slang, char_u *fword, char_u *kword));
static void score_comp_sal __ARGS((suginfo_T *su));
static void score_combine __ARGS((suginfo_T *su));
static int stp_sal_score __ARGS((suggest_T *stp, suginfo_T *su, slang_T *slang, char_u *badsound));
static void suggest_try_soundalike __ARGS((suginfo_T *su));
static void make_case_word __ARGS((char_u *fword, char_u *cword, int flags));
static void set_map_str __ARGS((slang_T *lp, char_u *map));
static int similar_chars __ARGS((slang_T *slang, int c1, int c2));
static void add_suggestion __ARGS((suginfo_T *su, garray_T *gap, char_u *goodword, int badlen, int score, int altscore, int had_bonus));
static void add_banned __ARGS((suginfo_T *su, char_u *word));
static int was_banned __ARGS((suginfo_T *su, char_u *word));
static void free_banned __ARGS((suginfo_T *su));
static void rescore_suggestions __ARGS((suginfo_T *su));
static int cleanup_suggestions __ARGS((garray_T *gap, int maxscore, int keep));
static void spell_soundfold __ARGS((slang_T *slang, char_u *inword, char_u *res));
static int soundalike_score __ARGS((char_u *goodsound, char_u *badsound));
static int spell_edit_score __ARGS((char_u *badword, char_u *goodword));
static void dump_word __ARGS((char_u *word, int round, int flags, linenr_T lnum));
static linenr_T apply_prefixes __ARGS((slang_T *slang, char_u *word, int round, int flags, linenr_T startlnum));

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
    int		nrlen = 0;	/* found a number first */

    /* A word never starts at a space or a control character.  Return quickly
     * then, skipping over the character. */
    if (*ptr <= ' ')
	return 1;

    /* A number is always OK.  Also skip hexadecimal numbers 0xFF99 and
     * 0X99FF.  But when a word character follows do check spelling to find
     * "3GPP". */
    if (*ptr >= '0' && *ptr <= '9')
    {
	if (*ptr == '0' && (ptr[1] == 'x' || ptr[1] == 'X'))
	    mi.mi_end = skiphex(ptr + 2);
	else
	{
	    mi.mi_end = skipdigits(ptr);
	    nrlen = mi.mi_end - ptr;
	}
	if (!spell_iswordp(mi.mi_end))
	    return (int)(mi.mi_end - ptr);

	/* Try including the digits in the word. */
	mi.mi_fend = ptr + nrlen;
    }
    else
	mi.mi_fend = ptr;

    /* Find the normal end of the word (until the next non-word character). */
    mi.mi_word = ptr;
    if (spell_iswordp(mi.mi_fend))
    {
	do
	{
	    mb_ptr_adv(mi.mi_fend);
	} while (*mi.mi_fend != NUL && spell_iswordp(mi.mi_fend));
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
	find_word(&mi, FIND_FOLDWORD);

	/* Check for a matching word in keep-case words. */
	find_word(&mi, FIND_KEEPWORD);

	/* Check for matching prefixes. */
	find_prefix(&mi);
    }

    if (mi.mi_result != SP_OK)
    {
	/* If we found a number skip over it.  Allows for "42nd".  Do flag
	 * rare and local words, e.g., "3GPP". */
	if (nrlen > 0)
	{
	    if (mi.mi_result == SP_BAD || mi.mi_result == SP_BANNED)
		return nrlen;
	}

	/* When we are at a non-word character there is no error, just
	 * skip over the character (try looking for a word after it). */
	else if (!SPELL_ISWORDP(ptr))
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

    return (int)(mi.mi_end - ptr);
}

/*
 * Check if the word at "mip->mi_word" is in the tree.
 * When "mode" is FIND_FOLDWORD check in fold-case word tree.
 * When "mode" is FIND_KEEPWORD check in keep-case word tree.
 * When "mode" is FIND_PREFIX check for word after prefix in fold-case word
 * tree.
 *
 * For a match mip->mi_result is updated.
 */
    static void
find_word(mip, mode)
    matchinf_T	*mip;
    int		mode;
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
    char_u	*p;
#endif
    int		res = SP_BAD;
    slang_T	*slang = mip->mi_lp->lp_slang;
    unsigned	flags;
    char_u	*byts;
    idx_T	*idxs;
    int		prefid;

    if (mode == FIND_KEEPWORD)
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

	if (mode == FIND_PREFIX)
	{
	    /* Skip over the prefix. */
	    wlen = mip->mi_prefixlen;
	    flen -= mip->mi_prefixlen;
	}
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
	if (flen <= 0 && *mip->mi_fend != NUL)
	    flen = fold_more(mip);

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
	if (c == TAB)	    /* <Tab> is handled like <Space> */
	    c = ' ';
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

	/* One space in the good word may stand for several spaces in the
	 * checked word. */
	if (c == ' ')
	{
	    for (;;)
	    {
		if (flen <= 0 && *mip->mi_fend != NUL)
		    flen = fold_more(mip);
		if (ptr[wlen] != ' ' && ptr[wlen] != TAB)
		    break;
		++wlen;
		--flen;
	    }
	}
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
	if (spell_iswordp(ptr + wlen))
	    continue;	    /* next char is a word character */

#ifdef FEAT_MBYTE
	if (mode != FIND_KEEPWORD && has_mbyte)
	{
	    /* Compute byte length in original word, length may change
	     * when folding case.  This can be slow, take a shortcut when the
	     * case-folded word is equal to the keep-case word. */
	    p = mip->mi_word;
	    if (STRNCMP(ptr, p, wlen) != 0)
	    {
		for (s = ptr; s < ptr + wlen; mb_ptr_adv(s))
		    mb_ptr_adv(p);
		wlen = p - mip->mi_word;
	    }
	}
#endif

	/* Check flags and region.  For FIND_PREFIX check the condition and
	 * prefix ID.
	 * Repeat this if there are more flags/region alternatives until there
	 * is a match. */
	res = SP_BAD;
	for (len = byts[arridx - 1]; len > 0 && byts[arridx] == 0;
							      --len, ++arridx)
	{
	    flags = idxs[arridx];

	    /* For the fold-case tree check that the case of the checked word
	     * matches with what the word in the tree requires.
	     * For keep-case tree the case is always right.  For prefixes we
	     * don't bother to check. */
	    if (mode == FIND_FOLDWORD)
	    {
		if (mip->mi_cend != mip->mi_word + wlen)
		{
		    /* mi_capflags was set for a different word length, need
		     * to do it again. */
		    mip->mi_cend = mip->mi_word + wlen;
		    mip->mi_capflags = captype(mip->mi_word, mip->mi_cend);
		}

		if (mip->mi_capflags == WF_KEEPCAP
				|| !spell_valid_case(mip->mi_capflags, flags))
		    continue;
	    }

	    /* When mode is FIND_PREFIX the word must support the prefix:
	     * check the prefix ID and the condition.  Do that for the list at
	     * mip->mi_prefarridx that find_prefix() filled. */
	    if (mode == FIND_PREFIX)
	    {
		/* The prefix ID is stored two bytes above the flags. */
		prefid = (unsigned)flags >> 16;
		c = valid_word_prefix(mip->mi_prefcnt, mip->mi_prefarridx,
				   prefid, mip->mi_fword + mip->mi_prefixlen,
								       slang);
		if (c == 0)
		    continue;

		/* Use the WF_RARE flag for a rare prefix. */
		if (c & WF_RAREPFX)
		    flags |= WF_RARE;
	    }

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
	    else if (mip->mi_result == res && mip->mi_end < mip->mi_word + wlen)
		mip->mi_end = mip->mi_word + wlen;

	    if (res == SP_OK)
		break;
	}

	if (res == SP_OK)
	    break;
    }
}

/*
 * Return non-zero if the prefix indicated by "mip->mi_prefarridx" matches
 * with the prefix ID "prefid" for the word "word".
 * The WF_RAREPFX flag is included in the return value for a rare prefix.
 */
    static int
valid_word_prefix(totprefcnt, arridx, prefid, word, slang)
    int		totprefcnt;	/* nr of prefix IDs */
    int		arridx;		/* idx in sl_pidxs[] */
    int		prefid;
    char_u	*word;
    slang_T	*slang;
{
    int		prefcnt;
    int		pidx;
    regprog_T	*rp;
    regmatch_T	regmatch;

    for (prefcnt = totprefcnt - 1; prefcnt >= 0; --prefcnt)
    {
	pidx = slang->sl_pidxs[arridx + prefcnt];

	/* Check the prefix ID. */
	if (prefid != (pidx & 0xff))
	    continue;

	/* Check the condition, if there is one.  The condition index is
	 * stored in the two bytes above the prefix ID byte.  */
	rp = slang->sl_prefprog[((unsigned)pidx >> 8) & 0xffff];
	if (rp != NULL)
	{
	    regmatch.regprog = rp;
	    regmatch.rm_ic = FALSE;
	    if (!vim_regexec(&regmatch, word, 0))
		continue;
	}

	/* It's a match!  Return the WF_RAREPFX flag. */
	return pidx;
    }
    return 0;
}

/*
 * Check if the word at "mip->mi_word" has a matching prefix.
 * If it does, then check the following word.
 *
 * For a match mip->mi_result is updated.
 */
    static void
find_prefix(mip)
    matchinf_T	*mip;
{
    idx_T	arridx = 0;
    int		len;
    int		wlen = 0;
    int		flen;
    int		c;
    char_u	*ptr;
    idx_T	lo, hi, m;
    slang_T	*slang = mip->mi_lp->lp_slang;
    char_u	*byts;
    idx_T	*idxs;

    /* We use the case-folded word here, since prefixes are always
     * case-folded. */
    ptr = mip->mi_fword;
    flen = mip->mi_fwordlen;    /* available case-folded bytes */
    byts = slang->sl_pbyts;
    idxs = slang->sl_pidxs;

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
	    flen = fold_more(mip);

	len = byts[arridx++];

	/* If the first possible byte is a zero the prefix could end here.
	 * Check if the following word matches and supports the prefix. */
	if (byts[arridx] == 0)
	{
	    /* There can be several prefixes with different conditions.  We
	     * try them all, since we don't know which one will give the
	     * longest match.  The word is the same each time, pass the list
	     * of possible prefixes to find_word(). */
	    mip->mi_prefarridx = arridx;
	    mip->mi_prefcnt = len;
	    while (len > 0 && byts[arridx] == 0)
	    {
		++arridx;
		--len;
	    }
	    mip->mi_prefcnt -= len;

	    /* Find the word that comes after the prefix. */
	    mip->mi_prefixlen = wlen;
	    find_word(mip, FIND_PREFIX);


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
}

/*
 * Need to fold at least one more character.  Do until next non-word character
 * for efficiency.
 * Return the length of the folded chars in bytes.
 */
    static int
fold_more(mip)
    matchinf_T	*mip;
{
    int		flen;
    char_u	*p;

    p = mip->mi_fend;
    do
    {
	mb_ptr_adv(mip->mi_fend);
    } while (*mip->mi_fend != NUL && spell_iswordp(mip->mi_fend));

    /* Include the non-word character so that we can check for the
     * word end. */
    if (*mip->mi_fend != NUL)
	mb_ptr_adv(mip->mi_fend);

    (void)spell_casefold(p, (int)(mip->mi_fend - p),
			     mip->mi_fword + mip->mi_fwordlen,
			     MAXWLEN - mip->mi_fwordlen);
    flen = STRLEN(mip->mi_fword + mip->mi_fwordlen);
    mip->mi_fwordlen += flen;
    return flen;
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
 * Return TRUE if spell checking is not enabled.
 */
    static int
no_spell_checking()
{
    if (!curwin->w_p_spell || *curbuf->b_p_spl == NUL)
    {
	EMSG(_("E756: Spell checking is not enabled"));
	return TRUE;
    }
    return FALSE;
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
    char_u	*endp;
    int		attr;
    int		len;
    int		has_syntax = syntax_present(curbuf);
    int		col;
    int		can_spell;
    char_u	*buf = NULL;
    int		buflen = 0;
    int		skip = 0;

    if (no_spell_checking())
	return FAIL;

    /*
     * Start looking for bad word at the start of the line, because we can't
     * start halfway a word, we don't know where the it starts or ends.
     *
     * When searching backwards, we continue in the line to find the last
     * bad word (in the cursor line: before the cursor).
     *
     * We concatenate the start of the next line, so that wrapped words work
     * (e.g. "et<line-break>cetera").  Doesn't work when searching backwards
     * though...
     */
    lnum = curwin->w_cursor.lnum;
    found_pos.lnum = 0;

    while (!got_int)
    {
	line = ml_get(lnum);

	len = STRLEN(line);
	if (buflen < len + MAXWLEN + 2)
	{
	    vim_free(buf);
	    buflen = len + MAXWLEN + 2;
	    buf = alloc(buflen);
	    if (buf == NULL)
		break;
	}

	/* Copy the line into "buf" and append the start of the next line if
	 * possible. */
	STRCPY(buf, line);
	if (lnum < curbuf->b_ml.ml_line_count)
	    spell_cat_line(buf + STRLEN(buf), ml_get(lnum + 1), MAXWLEN);

	p = buf + skip;
	endp = buf + len;
	while (p < endp)
	{
	    /* When searching backward don't search after the cursor. */
	    if (dir == BACKWARD
		    && lnum == curwin->w_cursor.lnum
		    && (colnr_T)(p - buf) >= curwin->w_cursor.col)
		break;

	    /* start of word */
	    attr = 0;
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
				&& (colnr_T)(curline ? p - buf + len
						     : p - buf)
						  > curwin->w_cursor.col))
		    {
			if (has_syntax)
			{
			    col = p - buf;
			    (void)syn_get_id(lnum, (colnr_T)col,
						       FALSE, &can_spell);
			}
			else
			    can_spell = TRUE;

			if (can_spell)
			{
			    found_pos.lnum = lnum;
			    found_pos.col = p - buf;
#ifdef FEAT_VIRTUALEDIT
			    found_pos.coladd = 0;
#endif
			    if (dir == FORWARD)
			    {
				/* No need to search further. */
				curwin->w_cursor = found_pos;
				vim_free(buf);
				return OK;
			    }
			}
		    }
		}
	    }

	    /* advance to character after the word */
	    p += len;
	}

	if (curline)
	    break;	/* only check cursor line */

	/* Advance to next line. */
	if (dir == BACKWARD)
	{
	    if (found_pos.lnum != 0)
	    {
		/* Use the last match in the line. */
		curwin->w_cursor = found_pos;
		vim_free(buf);
		return OK;
	    }
	    if (lnum == 1)
		break;
	    --lnum;
	}
	else
	{
	    if (lnum == curbuf->b_ml.ml_line_count)
		break;
	    ++lnum;

	    /* Skip the characters at the start of the next line that were
	     * included in a match crossing line boundaries. */
	    if (attr == 0)
		skip = p - endp;
	    else
		skip = 0;
	}

	line_breakcheck();
    }

    vim_free(buf);
    return FAIL;
}

/*
 * For spell checking: concatenate the start of the following line "line" into
 * "buf", blanking-out special characters.  Copy less then "maxlen" bytes.
 */
    void
spell_cat_line(buf, line, maxlen)
    char_u	*buf;
    char_u	*line;
    int		maxlen;
{
    char_u	*p;
    int		n;

    p = skipwhite(line);
    while (vim_strchr((char_u *)"*#/\"\t", *p) != NULL)
	p = skipwhite(p + 1);

    if (*p != NUL)
    {
	*buf = ' ';
	vim_strncpy(buf + 1, line, maxlen - 1);
	n = p - line;
	if (n >= maxlen)
	    n = maxlen - 1;
	vim_memset(buf + 1, ' ', n);
    }
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
	ga_init2(&lp->sl_sal, sizeof(salitem_T), 10);
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
    garray_T	*gap;
    fromto_T	*ftp;
    salitem_T	*smp;
    int		i;

    vim_free(lp->sl_fbyts);
    lp->sl_fbyts = NULL;
    vim_free(lp->sl_kbyts);
    lp->sl_kbyts = NULL;
    vim_free(lp->sl_pbyts);
    lp->sl_pbyts = NULL;

    vim_free(lp->sl_fidxs);
    lp->sl_fidxs = NULL;
    vim_free(lp->sl_kidxs);
    lp->sl_kidxs = NULL;
    vim_free(lp->sl_pidxs);
    lp->sl_pidxs = NULL;

    gap = &lp->sl_rep;
    while (gap->ga_len > 0)
    {
	ftp = &((fromto_T *)gap->ga_data)[--gap->ga_len];
	vim_free(ftp->ft_from);
	vim_free(ftp->ft_to);
    }
    ga_clear(gap);

    gap = &lp->sl_sal;
    while (gap->ga_len > 0)
    {
	smp = &((salitem_T *)gap->ga_data)[--gap->ga_len];
	vim_free(smp->sm_lead);
	vim_free(smp->sm_to);
    }
    ga_clear(gap);

    for (i = 0; i < lp->sl_prefixcnt; ++i)
	vim_free(lp->sl_prefprog[i]);
    vim_free(lp->sl_prefprog);

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
    char_u	*bp;
    idx_T	*ip;
    int		i;
    int		n;
    int		len;
    int		round;
    char_u	*save_sourcing_name = sourcing_name;
    linenr_T	save_sourcing_lnum = sourcing_lnum;
    int		cnt, ccnt;
    char_u	*fol;
    slang_T	*lp = NULL;
    garray_T	*gap;
    fromto_T	*ftp;
    salitem_T	*smp;
    int		rr;
    short	*first;
    idx_T	idx;
    int		c = 0;

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

    /* <HEADER>: <fileID>
     *		<regioncnt> <regionname> ...
     *		<charflagslen> <charflags>
     *		<fcharslen> <fchars>
     *		<midwordlen> <midword>
     *		<prefcondcnt> <prefcond> ...
     */
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
#if 0	/* tolerate the differences */
	if (i == FAIL)
	    goto formerr;
#endif
    }
    else
    {
	/* When <charflagslen> is zero then <fcharlen> must also be zero. */
	cnt = (getc(fd) << 8) + getc(fd);
	if (cnt != 0)
	    goto formerr;
    }

    /* <midwordlen> <midword> */
    cnt = (getc(fd) << 8) + getc(fd);
    if (cnt < 0)
	goto truncerr;
    if (cnt > 0)
    {
	for (i = 0; i < cnt; ++i)
	    if (i < MAXWLEN)	    /* truncate at reasonable length */
		buf[i] = getc(fd);
	if (i < MAXWLEN)
	    buf[i] = NUL;
	else
	    buf[MAXWLEN] = NUL;

	/* The midword characters add up to any midword characters from other
	 * .spel files. */
	for (p = buf; *p != NUL; )
#ifdef FEAT_MBYTE
	    if (has_mbyte)
	    {
		c = mb_ptr2char(p);
		i = mb_ptr2len_check(p);
		if (c < 256)
		    spell_ismw[c] = TRUE;
		else if (spell_ismw_mb == NULL)
		    /* First multi-byte char in "spell_ismw_mb". */
		    spell_ismw_mb = vim_strnsave(p, i);
		else
		{
		    /* Append multi-byte chars to "spell_ismw_mb". */
		    n = STRLEN(spell_ismw_mb);
		    bp = vim_strnsave(spell_ismw_mb, n + i);
		    if (bp != NULL)
		    {
			vim_free(spell_ismw_mb);
			spell_ismw_mb = bp;
			vim_strncpy(bp + n, p, i);
		    }
		}
		p += i;
	    }
	    else
#endif
		spell_ismw[*p++] = TRUE;
    }

    /* <prefcondcnt> <prefcond> ... */
    cnt = (getc(fd) << 8) + getc(fd);			/* <prefcondcnt> */
    if (cnt > 0)
    {
	lp->sl_prefprog = (regprog_T **)alloc_clear(
					 (unsigned)sizeof(regprog_T *) * cnt);
	if (lp->sl_prefprog == NULL)
	    goto endFAIL;
	lp->sl_prefixcnt = cnt;

	for (i = 0; i < cnt; ++i)
	{
	    /* <prefcond> : <condlen> <condstr> */
	    n = getc(fd);				/* <condlen> */
	    if (n < 0)
		goto formerr;
	    /* When <condlen> is zero we have an empty condition.  Otherwise
	     * compile the regexp program used to check for the condition. */
	    if (n > 0)
	    {
		buf[0] = '^';	    /* always match at one position only */
		p = buf + 1;
		while (n-- > 0)
		    *p++ = getc(fd);			/* <condstr> */
		*p = NUL;
		lp->sl_prefprog[i] = vim_regcomp(buf, RE_MAGIC + RE_STRING);
	    }
	}
    }


    /* <SUGGEST> : <repcount> <rep> ...
     *             <salflags> <salcount> <sal> ...
     *             <maplen> <mapstr> */

    cnt = (getc(fd) << 8) + getc(fd);			/* <repcount> */
    if (cnt < 0)
	goto formerr;

    gap = &lp->sl_rep;
    if (ga_grow(gap, cnt) == FAIL)
	goto endFAIL;

    /* <rep> : <repfromlen> <repfrom> <reptolen> <repto> */
    for (; gap->ga_len < cnt; ++gap->ga_len)
    {
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
		p[i] = getc(fd);		/* <repfrom> or <repto> */
	    p[i] = NUL;
	    if (rr == 1)
		ftp->ft_from = p;
	    else
		ftp->ft_to = p;
	}
    }

    /* Fill the first-index table. */
    first = lp->sl_rep_first;
    for (i = 0; i < 256; ++i)
	first[i] = -1;
    for (i = 0; i < gap->ga_len; ++i)
    {
	ftp = &((fromto_T *)gap->ga_data)[i];
	if (first[*ftp->ft_from] == -1)
	    first[*ftp->ft_from] = i;
    }

    i = getc(fd);				/* <salflags> */
    if (i & SAL_F0LLOWUP)
	lp->sl_followup = TRUE;
    if (i & SAL_COLLAPSE)
	lp->sl_collapse = TRUE;
    if (i & SAL_REM_ACCENTS)
	lp->sl_rem_accents = TRUE;

    cnt = (getc(fd) << 8) + getc(fd);		/* <salcount> */
    if (cnt < 0)
	goto formerr;

    gap = &lp->sl_sal;
    if (ga_grow(gap, cnt) == FAIL)
	goto endFAIL;

    /* <sal> : <salfromlen> <salfrom> <saltolen> <salto> */
    for (; gap->ga_len < cnt; ++gap->ga_len)
    {
	smp = &((salitem_T *)gap->ga_data)[gap->ga_len];
	ccnt = getc(fd);			/* <salfromlen> */
	if (ccnt < 0)
	    goto formerr;
	if ((p = alloc(ccnt + 2)) == NULL)
	    goto endFAIL;
	smp->sm_lead = p;

	/* Read up to the first special char into sm_lead. */
	for (i = 0; i < ccnt; ++i)
	{
	    c = getc(fd);			/* <salfrom> */
	    if (vim_strchr((char_u *)"0123456789(-<^$", c) != NULL)
		break;
	    *p++ = c;
	}
	smp->sm_leadlen = p - smp->sm_lead;
	*p++ = NUL;

	/* Put optional chars in sm_oneoff, if any. */
	if (c == '(')
	{
	    smp->sm_oneoff = p;
	    for (++i; i < ccnt; ++i)
	    {
		c = getc(fd);			/* <salfrom> */
		if (c == ')')
		    break;
		*p++ = c;
	    }
	    *p++ = NUL;
	    if (++i < ccnt)
		c = getc(fd);
	}
	else
	    smp->sm_oneoff = NULL;

	/* Any following chars go in sm_rules. */
	smp->sm_rules = p;
	if (i < ccnt)
	    *p++ = c;
	for (++i; i < ccnt; ++i)
	    *p++ = getc(fd);			/* <salfrom> */
	*p++ = NUL;

	ccnt = getc(fd);			/* <saltolen> */
	if (ccnt < 0)
	{
	    vim_free(smp->sm_lead);
	    goto formerr;
	}
	if ((p = alloc(ccnt + 1)) == NULL)
	{
	    vim_free(smp->sm_lead);
	    goto endFAIL;
	}
	smp->sm_to = p;

	for (i = 0; i < ccnt; ++i)
	    *p++ = getc(fd);			/* <salto> */
	*p++ = NUL;
    }

    /* Fill the first-index table. */
    first = lp->sl_sal_first;
    for (i = 0; i < 256; ++i)
	first[i] = -1;
    for (i = 0; i < gap->ga_len; ++i)
    {
	smp = &((salitem_T *)gap->ga_data)[i];
	if (first[*smp->sm_lead] == -1)
	    first[*smp->sm_lead] = i;
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
     * round 2: <KWORDTREE>
     * round 3: <PREFIXTREE> */
    for (round = 1; round <= 3; ++round)
    {
	/* The tree size was computed when writing the file, so that we can
	 * allocate it as one long block. <nodecount> */
	len = (getc(fd) << 24) + (getc(fd) << 16) + (getc(fd) << 8) + getc(fd);
	if (len < 0)
	    goto truncerr;
	if (len > 0)
	{
	    /* Allocate the byte array. */
	    bp = lalloc((long_u)len, TRUE);
	    if (bp == NULL)
		goto endFAIL;
	    if (round == 1)
		lp->sl_fbyts = bp;
	    else if (round == 2)
		lp->sl_kbyts = bp;
	    else
		lp->sl_pbyts = bp;

	    /* Allocate the index array. */
	    ip = (idx_T *)lalloc_clear((long_u)(len * sizeof(int)), TRUE);
	    if (ip == NULL)
		goto endFAIL;
	    if (round == 1)
		lp->sl_fidxs = ip;
	    else if (round == 2)
		lp->sl_kidxs = ip;
	    else
		lp->sl_pidxs = ip;

	    /* Read the tree and store it in the array. */
	    idx = read_tree(fd, bp, ip, len, 0, round == 3, lp->sl_prefixcnt);
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
 * NOTE: The code here must match put_node().
 *
 * Returns the index follosing the siblings.
 * Returns -1 if the file is shorter than expected.
 * Returns -2 if there is a format error.
 */
    static idx_T
read_tree(fd, byts, idxs, maxidx, startidx, prefixtree, maxprefcondnr)
    FILE	*fd;
    char_u	*byts;
    idx_T	*idxs;
    int		maxidx;		    /* size of arrays */
    idx_T	startidx;	    /* current index in "byts" and "idxs" */
    int		prefixtree;	    /* TRUE for reading PREFIXTREE */
    int		maxprefcondnr;	    /* maximum for <prefcondnr> */
{
    int		len;
    int		i;
    int		n;
    idx_T	idx = startidx;
    int		c;
    int		c2;
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
	    if (c == BY_NOFLAGS && !prefixtree)
	    {
		/* No flags, all regions. */
		idxs[idx] = 0;
		c = 0;
	    }
	    else if (c == BY_FLAGS || c == BY_NOFLAGS)
	    {
		if (prefixtree)
		{
		    /* Read the prefix ID and the condition nr.  In idxs[]
		     * store the prefix ID in the low byte, the condition
		     * index shifted up 8 bits. */
		    c2 = getc(fd);			/* <prefixID> */
		    n = (getc(fd) << 8) + getc(fd);	/* <prefcondnr> */
		    if (n >= maxprefcondnr)
			return -2;
		    c2 += (n << 8);
		    if (c == BY_NOFLAGS)
			c = c2;
		    else
			c = c2 | WF_RAREPFX;
		}
		else
		{
		    /* Read flags and optional region and prefix ID.  In
		     * idxs[] the flags go in the low byte, region above that
		     * and prefix ID above the region. */
		    c = getc(fd);			/* <flags> */
		    if (c & WF_REGION)
			c = (getc(fd) << 8) + c;	/* <region> */
		    if (c & WF_PFX)
			c = (getc(fd) << 16) + c;	/* <prefixID> */
		}

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
		idx = read_tree(fd, byts, idxs, maxidx, idx,
						     prefixtree, maxprefcondnr);
		if (idx < 0)
		    break;
	    }
	}

    return idx;
}

/*
 * Parse 'spelllang' and set buf->b_langp accordingly.
 * Returns NULL if it's OK, an error message otherwise.
 */
    char_u *
did_set_spelllang(buf)
    buf_T	*buf;
{
    garray_T	ga;
    char_u	*splp;
    char_u	*region;
    int		filename;
    int		region_mask;
    slang_T	*lp;
    int		c;
    char_u	lang[MAXWLEN + 1];
    char_u	spf_name[MAXPATHL];
    int		load_spf;
    int		len;
    char_u	*p;

    ga_init2(&ga, sizeof(langp_T), 2);

    /* Make the name of the .spl file associated with 'spellfile'. */
    if (*buf->b_p_spf == NUL)
	load_spf = FALSE;
    else
    {
	vim_snprintf((char *)spf_name, sizeof(spf_name), "%s.spl",
								buf->b_p_spf);
	load_spf = TRUE;
    }

    /* loop over comma separated language names. */
    for (splp = buf->b_p_spl; *splp != NUL; )
    {
	/* Get one language name. */
	copy_option_part(&splp, lang, MAXWLEN, ",");

	region = NULL;
	len = STRLEN(lang);

	/* If the name ends in ".spl" use it as the name of the spell file.
	 * If there is a region name let "region" point to it and remove it
	 * from the name. */
	if (len > 4 && fnamecmp(lang + len - 4, ".spl") == 0)
	{
	    filename = TRUE;

	    /* Check if we loaded this language before. */
	    for (lp = first_lang; lp != NULL; lp = lp->sl_next)
		if (fullpathcmp(lang, lp->sl_fname, FALSE) == FPC_SAME)
		    break;
	}
	else
	{
	    filename = FALSE;
	    if (len > 3 && lang[len - 3] == '_')
	    {
		region = lang + len - 2;
		len -= 3;
		lang[len] = NUL;
	    }

	    /* Check if we loaded this language before. */
	    for (lp = first_lang; lp != NULL; lp = lp->sl_next)
		if (STRICMP(lang, lp->sl_name) == 0)
		    break;
	}

	/* If not found try loading the language now. */
	if (lp == NULL)
	{
	    if (filename)
		(void)spell_load_file(lang, lang, NULL, FALSE);
	    else
		spell_load_lang(lang);
	}

	/*
	 * Loop over the languages, there can be several files for "lang".
	 */
	for (lp = first_lang; lp != NULL; lp = lp->sl_next)
	    if (filename ? fullpathcmp(lang, lp->sl_fname, FALSE) == FPC_SAME
			 : STRICMP(lang, lp->sl_name) == 0)
	    {
		region_mask = REGION_ALL;
		if (!filename && region != NULL)
		{
		    /* find region in sl_regions */
		    c = find_region(lp->sl_regions, region);
		    if (c == REGION_ALL)
		    {
			if (!lp->sl_add)
			    smsg((char_u *)
				    _("Warning: region %s not supported"),
								      region);
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

		/* Check if this is the spell file related to 'spellfile'. */
		if (load_spf && fullpathcmp(spf_name, lp->sl_fname, FALSE)
								  == FPC_SAME)
		    load_spf = FALSE;
	    }
    }

    /*
     * Make sure the 'spellfile' file is loaded.  It may be in 'runtimepath',
     * then it's probably loaded above already.  Otherwise load it here.
     */
    if (load_spf)
    {
	/* Check if it was loaded already. */
	for (lp = first_lang; lp != NULL; lp = lp->sl_next)
	    if (fullpathcmp(spf_name, lp->sl_fname, FALSE) == FPC_SAME)
		break;
	if (lp == NULL)
	{
	    /* Not loaded, try loading it now.  The language name includes the
	     * region name, the region is ignored otherwise. */
	    vim_strncpy(lang, gettail(buf->b_p_spf), MAXWLEN);
	    p = vim_strchr(lang, '.');
	    if (p != NULL)
		*p = NUL;	/* truncate at ".encoding.add" */
	    lp = spell_load_file(spf_name, lang, NULL, TRUE);
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
    for (p = word; !spell_iswordp(p); mb_ptr_adv(p))
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
	if (spell_iswordp(p))
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

# if defined(FEAT_MBYTE) || defined(EXITFREE) || defined(PROTO)
/*
 * Free all languages.
 */
    void
spell_free_all()
{
    slang_T	*lp;
    buf_T	*buf;

    /* Go through all buffers and handle 'spelllang'. */
    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	ga_clear(&buf->b_langp);

    while (first_lang != NULL)
    {
	lp = first_lang;
	first_lang = lp->sl_next;
	slang_free(lp);
    }

    init_spell_chartab();
}
# endif

# if defined(FEAT_MBYTE) || defined(PROTO)
/*
 * Clear all spelling tables and reload them.
 * Used after 'encoding' is set and when ":mkspell" was used.
 */
    void
spell_reload()
{
    buf_T	*buf;
    win_T	*wp;

    /* Initialize the table for spell_iswordp(). */
    init_spell_chartab();

    /* Unload all allocated memory. */
    spell_free_all();

    /* Go through all buffers and handle 'spelllang'. */
    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
    {
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
    int		af_bad;		/* BAD ID for banned word */
    int		af_pfxpostpone;	/* postpone prefixes without chop string */
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
    int		ae_rare;	/* rare affix */
};

/* Affix header from ".aff" file.  Used for af_pref and af_suff. */
typedef struct affheader_S
{
    char_u	ah_key[2];	/* key for hashtable == name of affix entry */
    int		ah_newID;	/* prefix ID after renumbering */
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
    union   /* shared to save space */
    {
	char_u	hashkey[6];	/* room for the hash key */
	int	index;		/* index in written nodes (valid after first
				   round) */
    } wn_u1;
    union   /* shared to save space */
    {
	wordnode_T *next;	/* next node with same hash key */
	wordnode_T *wnode;	/* parent node that will write this node */
    } wn_u2;
    wordnode_T	*wn_child;	/* child (next byte in word) */
    wordnode_T  *wn_sibling;	/* next sibling (alternate byte in word,
				   always sorted) */
    char_u	wn_byte;	/* Byte for this node. NUL for word end */
    char_u	wn_flags;	/* when wn_byte is NUL: WF_ flags */
    short	wn_region;	/* when wn_byte is NUL: region mask; for
				   PREFIXTREE it's the prefcondnr */
    char_u	wn_prefixID;	/* supported/required prefix ID or 0 */
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
    wordnode_T	*si_prefroot;	/* tree with postponed prefixes */
    sblock_T	*si_blocks;	/* memory blocks used */
    int		si_ascii;	/* handling only ASCII words */
    int		si_add;		/* addition file */
    int		si_clear_chartab;   /* when TRUE clear char tables */
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
    char_u	*si_midword;	/* MIDWORD chars, alloc'ed string or NULL  */
    garray_T	si_prefcond;	/* table with conditions for postponed
				 * prefixes, each stored as a string */
    int		si_newID;	/* current value for ah_newID */
} spellinfo_T;

static afffile_T *spell_read_aff __ARGS((char_u *fname, spellinfo_T *spin));
static int str_equal __ARGS((char_u *s1, char_u	*s2));
static void add_fromto __ARGS((spellinfo_T *spin, garray_T *gap, char_u	*from, char_u *to));
static int sal_to_bool __ARGS((char_u *s));
static int has_non_ascii __ARGS((char_u *s));
static void spell_free_aff __ARGS((afffile_T *aff));
static int spell_read_dic __ARGS((char_u *fname, spellinfo_T *spin, afffile_T *affile));
static char_u *get_pfxlist __ARGS((afffile_T *affile, char_u *afflist, sblock_T	**blp));
static int store_aff_word __ARGS((char_u *word, spellinfo_T *spin, char_u *afflist, afffile_T *affile, hashtab_T *ht, hashtab_T *xht, int comb, int flags, char_u *pfxlist));
static int spell_read_wordfile __ARGS((char_u *fname, spellinfo_T *spin));
static void *getroom __ARGS((sblock_T **blp, size_t len));
static char_u *getroom_save __ARGS((sblock_T **blp, char_u *s));
static void free_blocks __ARGS((sblock_T *bl));
static wordnode_T *wordtree_alloc __ARGS((sblock_T **blp));
static int store_word __ARGS((char_u *word, spellinfo_T *spin, int flags, int region, char_u *pfxlist));
static int tree_add_word __ARGS((char_u *word, wordnode_T *tree, int flags, int region, int prefixID, sblock_T **blp));
static void wordtree_compress __ARGS((wordnode_T *root, spellinfo_T *spin));
static int node_compress __ARGS((wordnode_T *node, hashtab_T *ht, int *tot));
static int node_equal __ARGS((wordnode_T *n1, wordnode_T *n2));
static void write_vim_spell __ARGS((char_u *fname, spellinfo_T *spin));
static void clear_node __ARGS((wordnode_T *node));
static int put_node __ARGS((FILE *fd, wordnode_T *node, int index, int regionmask, int prefixtree));
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
    int		do_midword;
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
	smsg((char_u *)_("Reading affix file %s ..."), fname);
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

    /* Only do MIDWORD line when not done in another .aff file already */
    do_midword = spin->si_midword == NULL;

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
	    else if (STRCMP(items[0], "MIDWORD") == 0 && itemcnt == 2)
	    {
		if (do_midword)
		    spin->si_midword = vim_strsave(items[1]);
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
	    else if (STRCMP(items[0], "BAD") == 0 && itemcnt == 2
						       && aff->af_bad == 0)
	    {
		aff->af_bad = items[1][0];
		if (items[1][1] != NUL)
		    smsg((char_u *)_(e_affname), fname, lnum, items[1]);
	    }
	    else if (STRCMP(items[0], "PFXPOSTPONE") == 0 && itemcnt == 1)
	    {
		aff->af_pfxpostpone = TRUE;
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
		cur_aff->ah_key[0] = *items[1];	/* TODO: multi-byte? */
		cur_aff->ah_key[1] = NUL;
		if (items[1][1] != NUL)
		    smsg((char_u *)_(e_affname), fname, lnum, items[1]);
		if (*items[2] == 'Y')
		    cur_aff->ah_combine = TRUE;
		else if (*items[2] != 'N')
		    smsg((char_u *)_("Expected Y or N in %s line %d: %s"),
						       fname, lnum, items[2]);

		if (*items[0] == 'P')
		{
		    tp = &aff->af_pref;
		    /* Use a new number in the .spl file later, to be able to
		     * handle multiple .aff files. */
		    if (aff->af_pfxpostpone)
			cur_aff->ah_newID = ++spin->si_newID;
		}
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
		int		rare = FALSE;
		int		lasti = 5;

		/* Check for "rare" after the other info. */
		if (itemcnt > 5 && STRICMP(items[5], "rare") == 0)
		{
		    rare = TRUE;
		    lasti = 6;
		}

		/* Myspell allows extra text after the item, but that might
		 * mean mistakes go unnoticed.  Require a comment-starter. */
		if (itemcnt > lasti && *items[lasti] != '#')
		    smsg((char_u *)_("Trailing text in %s line %d: %s"),
						   fname, lnum, items[lasti]);

		/* New item for an affix letter. */
		--aff_todo;
		aff_entry = (affentry_T *)getroom(&spin->si_blocks,
							  sizeof(affentry_T));
		if (aff_entry == NULL)
		    break;
		aff_entry->ae_rare = rare;

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

		    /* For postponed prefixes we need an entry in si_prefcond
		     * for the condition.  Use an existing one if possible. */
		    if (*items[0] == 'P' && aff->af_pfxpostpone
						&& aff_entry->ae_chop == NULL)
		    {
			int	idx;
			char_u	**pp;

			for (idx = spin->si_prefcond.ga_len - 1; idx >= 0;
									--idx)
			{
			    p = ((char_u **)spin->si_prefcond.ga_data)[idx];
			    if (str_equal(p, aff_entry->ae_cond))
				break;
			}
			if (idx < 0 && ga_grow(&spin->si_prefcond, 1) == OK)
			{
			    /* Not found, add a new condition. */
			    idx = spin->si_prefcond.ga_len++;
			    pp = ((char_u **)spin->si_prefcond.ga_data) + idx;
			    if (aff_entry->ae_cond == NULL)
				*pp = NULL;
			    else
				*pp = getroom_save(&spin->si_blocks,
							  aff_entry->ae_cond);
			}

			/* Add the prefix to the prefix tree. */
			if (aff_entry->ae_add == NULL)
			    p = (char_u *)"";
			else
			    p = aff_entry->ae_add;
			tree_add_word(p, spin->si_prefroot, rare ? -2 : -1,
				    idx, cur_aff->ah_newID, &spin->si_blocks);
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
		    int		c;

		    /* Check that every character appears only once. */
		    for (p = items[1]; *p != NUL; )
		    {
#ifdef FEAT_MBYTE
			c = mb_ptr2char_adv(&p);
#else
			c = *p++;
#endif
			if ((spin->si_map.ga_len > 0
				    && vim_strchr(spin->si_map.ga_data, c)
								      != NULL)
				|| vim_strchr(p, c) != NULL)
			    smsg((char_u *)_("Duplicate character in MAP in %s line %d"),
								 fname, lnum);
		    }

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
	if (spin->si_clear_chartab)
	{
	    /* Clear the char type tables, don't want to use any of the
	     * currently used spell properties. */
	    init_spell_chartab();
	    spin->si_clear_chartab = FALSE;
	}

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
 * Return TRUE if strings "s1" and "s2" are equal.  Also consider both being
 * NULL as equal.
 */
    static int
str_equal(s1, s2)
    char_u	*s1;
    char_u	*s2;
{
    if (s1 == NULL || s2 == NULL)
	return s1 == s2;
    return STRCMP(s1, s2) == 0;
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

    /* All this trouble to free the "ae_prog" items... */
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
    char_u	*pfxlist;
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
	smsg((char_u *)_("Reading dictionary file %s ..."), fname);
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
	if (line[0] == '#')
	    continue;	/* comment line */

	/* Remove CR, LF and white space from the end.  White space halfway
	 * the word is kept to allow e.g., "et al.". */
	l = STRLEN(line);
	while (l > 0 && line[l - 1] <= ' ')
	    --l;
	if (l == 0)
	    continue;	/* empty line */
	line[l] = NUL;

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

	/* This takes time, print a message now and then. */
	if (spin->si_verbose && (lnum & 0x3ff) == 0)
	{
	    vim_snprintf((char *)message, sizeof(message),
		    _("line %6d, word %6d - %s"),
		       lnum, spin->si_foldwcount + spin->si_keepwcount, w);
	    msg_start();
	    msg_puts_long_attr(message, 0);
	    msg_clr_eos();
	    msg_didout = FALSE;
	    msg_col = 0;
	    out_flush();
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
							      fname, lnum, w);
	else
	    hash_add_item(&ht, hi, dw, hash);

	flags = 0;
	pfxlist = NULL;
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
	    if (affile->af_bad != NUL
		    && vim_strchr(afflist, affile->af_bad) != NULL)
		flags |= WF_BANNED;

	    if (affile->af_pfxpostpone)
		/* Need to store the list of prefix IDs with the word. */
		pfxlist = get_pfxlist(affile, afflist, &spin->si_blocks);
	}

	/* Add the word to the word tree(s). */
	if (store_word(dw, spin, flags, spin->si_region, pfxlist) == FAIL)
	    retval = FAIL;

	if (afflist != NULL)
	{
	    /* Find all matching suffixes and add the resulting words.
	     * Additionally do matching prefixes that combine. */
	    if (store_aff_word(dw, spin, afflist, affile,
			   &affile->af_suff, &affile->af_pref,
					       FALSE, flags, pfxlist) == FAIL)
		retval = FAIL;

	    /* Find all matching prefixes and add the resulting words. */
	    if (store_aff_word(dw, spin, afflist, affile,
			  &affile->af_pref, NULL,
					       FALSE, flags, pfxlist) == FAIL)
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
 * Get the list of prefix IDs from the affix list "afflist".
 * Used for PFXPOSTPONE.
 * Returns a string allocated with getroom(). NULL when there are no prefixes
 * or when out of memory.
 */
    static char_u *
get_pfxlist(affile, afflist, blp)
    afffile_T	*affile;
    char_u	*afflist;
    sblock_T	**blp;
{
    char_u	*p;
    int		cnt;
    int		round;
    char_u	*res = NULL;
    char_u	key[2];
    hashitem_T	*hi;

    key[1] = NUL;

    /* round 1: count the number of prefix IDs.
     * round 2: move prefix IDs to "res" */
    for (round = 1; round <= 2; ++round)
    {
	cnt = 0;
	for (p = afflist; *p != NUL; ++p)
	{
	    key[0] = *p;
	    hi = hash_find(&affile->af_pref, key);
	    if (!HASHITEM_EMPTY(hi))
	    {
		/* This is a prefix ID, use the new number. */
		if (round == 2)
		    res[cnt] = HI2AH(hi)->ah_newID;
		++cnt;
	    }
	}
	if (round == 1 && cnt > 0)
	    res = getroom(blp, cnt + 1);
	if (res == NULL)
	    break;
    }

    if (res != NULL)
	res[cnt] = NUL;
    return res;
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
store_aff_word(word, spin, afflist, affile, ht, xht, comb, flags, pfxlist)
    char_u	*word;		/* basic word start */
    spellinfo_T	*spin;		/* spell info */
    char_u	*afflist;	/* list of names of supported affixes */
    afffile_T	*affile;
    hashtab_T	*ht;
    hashtab_T	*xht;
    int		comb;		/* only use affixes that combine */
    int		flags;		/* flags for the word */
    char_u	*pfxlist;	/* list of prefix IDs */
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
    int		use_flags;

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
		     * Myspell.
		     * For prefixes, when "PFXPOSTPONE" was used, only do
		     * prefixes with a chop string. */
		    regmatch.regprog = ae->ae_prog;
		    regmatch.rm_ic = FALSE;
		    if ((xht != NULL || !affile->af_pfxpostpone
				|| ae->ae_chop != NULL)
			    && (ae->ae_prog == NULL
				|| vim_regexec(&regmatch, word, (colnr_T)0)))
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

			/* Obey the "rare" flag of the affix. */
			if (ae->ae_rare)
			    use_flags = flags | WF_RARE;
			else
			    use_flags = flags;

			/* Store the modified word. */
			if (store_word(newword, spin, use_flags,
					    spin->si_region, pfxlist) == FAIL)
			    retval = FAIL;

			/* When added a suffix and combining is allowed also
			 * try adding prefixes additionally. */
			if (xht != NULL && ah->ah_combine)
			    if (store_aff_word(newword, spin, afflist, affile,
					  xht, NULL, TRUE, use_flags, pfxlist)
								      == FAIL)
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
	smsg((char_u *)_("Reading word file %s ..."), fname);
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
	if (store_word(line, spin, flags, regionmask, NULL) == FAIL)
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
 * When "pfxlist" is not NULL store the word for each prefix ID.
 */
    static int
store_word(word, spin, flags, region, pfxlist)
    char_u	*word;
    spellinfo_T	*spin;
    int		flags;		/* extra flags, WF_BANNED */
    int		region;		/* supported region(s) */
    char_u	*pfxlist;	/* list of prefix IDs or NULL */
{
    int		len = STRLEN(word);
    int		ct = captype(word, word + len);
    char_u	foldword[MAXWLEN];
    int		res = OK;
    char_u	*p;

    (void)spell_casefold(word, len, foldword, MAXWLEN);
    for (p = pfxlist; res == OK; ++p)
    {
	res = tree_add_word(foldword, spin->si_foldroot, ct | flags,
				region, p == NULL ? 0 : *p, &spin->si_blocks);
	if (p == NULL || *p == NUL)
	    break;
    }
    ++spin->si_foldwcount;

    if (res == OK && (ct == WF_KEEPCAP || flags & WF_KEEPCAP))
    {
	for (p = pfxlist; res == OK; ++p)
	{
	    res = tree_add_word(word, spin->si_keeproot, flags,
				region, p == NULL ? 0 : *p, &spin->si_blocks);
	    if (p == NULL || *p == NUL)
		break;
	}
	++spin->si_keepwcount;
    }
    return res;
}

/*
 * Add word "word" to a word tree at "root".
 * When "flags" < 0 we are adding to the prefix tree where flags is used for
 * "rare" and "region" is the condition nr.
 * Returns FAIL when out of memory.
 */
    static int
tree_add_word(word, root, flags, region, prefixID, blp)
    char_u	*word;
    wordnode_T	*root;
    int		flags;
    int		region;
    int		prefixID;
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
	 * higher byte value.  For zero bytes (end of word) the sorting is
	 * done on flags and then on prefixID
	 */
	while (node != NULL
		&& (node->wn_byte < word[i]
		    || (node->wn_byte == NUL
			&& (flags < 0
			    ? node->wn_prefixID < prefixID
			    : node->wn_flags < (flags & 0xff)
				|| (node->wn_flags == (flags & 0xff)
				    && node->wn_prefixID < prefixID)))))
	{
	    prev = &node->wn_sibling;
	    node = *prev;
	}
	if (node == NULL
		|| node->wn_byte != word[i]
		|| (word[i] == NUL
		    && (flags < 0
			|| node->wn_flags != (flags & 0xff)
			|| node->wn_prefixID != prefixID)))
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
	    node->wn_prefixID = prefixID;
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
	    /* Compress the child.  This fills hashkey. */
	    compressed += node_compress(child, ht, tot);

	    /* Try to find an identical child. */
	    hash = hash_hash(child->wn_u1.hashkey);
	    hi = hash_lookup(ht, child->wn_u1.hashkey, hash);
	    tp = NULL;
	    if (!HASHITEM_EMPTY(hi))
	    {
		/* There are children with an identical hash value.  Now check
		 * if there is one that is really identical. */
		for (tp = HI2WN(hi); tp != NULL; tp = tp->wn_u2.next)
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
		    child->wn_u2.next = tp->wn_u2.next;
		    tp->wn_u2.next = child;
		}
	    }
	    else
		/* No other child has this hash value, add it to the
		 * hashtable. */
		hash_add_item(ht, hi, child->wn_u1.hashkey, hash);
	}
    }
    *tot += len;

    /*
     * Make a hash key for the node and its siblings, so that we can quickly
     * find a lookalike node.  This must be done after compressing the sibling
     * list, otherwise the hash key would become invalid by the compression.
     */
    node->wn_u1.hashkey[0] = len;
    nr = 0;
    for (np = node; np != NULL; np = np->wn_sibling)
    {
	if (np->wn_byte == NUL)
	    /* end node: use wn_flags, wn_region and wn_prefixID */
	    n = np->wn_flags + (np->wn_region << 8) + (np->wn_prefixID << 16);
	else
	    /* byte node: use the byte value and the child pointer */
	    n = np->wn_byte + ((long_u)np->wn_child << 8);
	nr = nr * 101 + n;
    }

    /* Avoid NUL bytes, it terminates the hash key. */
    n = nr & 0xff;
    node->wn_u1.hashkey[1] = n == 0 ? 1 : n;
    n = (nr >> 8) & 0xff;
    node->wn_u1.hashkey[2] = n == 0 ? 1 : n;
    n = (nr >> 16) & 0xff;
    node->wn_u1.hashkey[3] = n == 0 ? 1 : n;
    n = (nr >> 24) & 0xff;
    node->wn_u1.hashkey[4] = n == 0 ? 1 : n;
    node->wn_u1.hashkey[5] = NUL;

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
			|| p1->wn_region != p2->wn_region
			|| p1->wn_prefixID != p2->wn_prefixID)
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
     *		 <charflagslen> <charflags>
     *		 <fcharslen> <fchars>
     *		 <midwordlen> <midword>
     *		 <prefcondcnt> <prefcond> ... */

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


    if (spin->si_midword == NULL)
	put_bytes(fd, 0L, 2);			/* <midwordlen> */
    else
    {
	i = STRLEN(spin->si_midword);
	put_bytes(fd, (long_u)i, 2);		/* <midwordlen> */
	fwrite(spin->si_midword, (size_t)i, (size_t)1, fd); /* <midword> */
    }


    /* Write the prefix conditions. */
    write_spell_prefcond(fd, &spin->si_prefcond);

    /* <SUGGEST> : <repcount> <rep> ...
     *             <salflags> <salcount> <sal> ...
     *             <maplen> <mapstr> */

    /* Sort the REP items. */
    qsort(spin->si_rep.ga_data, (size_t)spin->si_rep.ga_len,
					       sizeof(fromto_T), rep_compare);

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
     * <LWORDTREE>  <KWORDTREE>  <PREFIXTREE>
     */
    spin->si_memtot = 0;
    for (round = 1; round <= 3; ++round)
    {
	if (round == 1)
	    tree = spin->si_foldroot;
	else if (round == 2)
	    tree = spin->si_keeproot;
	else
	    tree = spin->si_prefroot;

	/* Clear the index and wnode fields in the tree. */
	clear_node(tree);

	/* Count the number of nodes.  Needed to be able to allocate the
	 * memory when reading the nodes.  Also fills in index for shared
	 * nodes. */
	nodecount = put_node(NULL, tree, 0, regionmask, round == 3);

	/* number of nodes in 4 bytes */
	put_bytes(fd, (long_u)nodecount, 4);	/* <nodecount> */
	spin->si_memtot += nodecount + nodecount * sizeof(int);

	/* Write the nodes. */
	(void)put_node(fd, tree, 0, regionmask, round == 3);
    }

    fclose(fd);
}

/*
 * Clear the index and wnode fields of "node", it siblings and its
 * children.  This is needed because they are a union with other items to save
 * space.
 */
    static void
clear_node(node)
    wordnode_T	*node;
{
    wordnode_T	*np;

    if (node != NULL)
	for (np = node; np != NULL; np = np->wn_sibling)
	{
	    np->wn_u1.index = 0;
	    np->wn_u2.wnode = NULL;

	    if (np->wn_byte != NUL)
		clear_node(np->wn_child);
	}
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
put_node(fd, node, index, regionmask, prefixtree)
    FILE	*fd;		/* NULL when only counting */
    wordnode_T	*node;
    int		index;
    int		regionmask;
    int		prefixtree;	/* TRUE for PREFIXTREE */
{
    int		newindex = index;
    int		siblingcount = 0;
    wordnode_T	*np;
    int		flags;

    /* If "node" is zero the tree is empty. */
    if (node == NULL)
	return 0;

    /* Store the index where this node is written. */
    node->wn_u1.index = index;

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
		/* For a NUL byte (end of word) write the flags etc. */
		if (prefixtree)
		{
		    /* In PREFIXTREE write the required prefixID and the
		     * associated condition nr (stored in wn_region). */
		    if (np->wn_flags == (char_u)-2)
			putc(BY_FLAGS, fd);		/* <byte> rare */
		    else
			putc(BY_NOFLAGS, fd);		/* <byte> */
		    putc(np->wn_prefixID, fd);		/* <prefixID> */
		    put_bytes(fd, (long_u)np->wn_region, 2); /* <prefcondnr> */
		}
		else
		{
		    /* For word trees we write the flag/region items. */
		    flags = np->wn_flags;
		    if (regionmask != 0 && np->wn_region != regionmask)
			flags |= WF_REGION;
		    if (np->wn_prefixID != 0)
			flags |= WF_PFX;
		    if (flags == 0)
		    {
			/* word without flags or region */
			putc(BY_NOFLAGS, fd);			/* <byte> */
		    }
		    else
		    {
			putc(BY_FLAGS, fd);			/* <byte> */
			putc(flags, fd);			/* <flags> */
			if (flags & WF_REGION)
			    putc(np->wn_region, fd);		/* <region> */
			if (flags & WF_PFX)
			    putc(np->wn_prefixID, fd);		/* <prefixID> */
		    }
		}
	    }
	}
	else
	{
	    if (np->wn_child->wn_u1.index != 0
					 && np->wn_child->wn_u2.wnode != node)
	    {
		/* The child is written elsewhere, write the reference. */
		if (fd != NULL)
		{
		    putc(BY_INDEX, fd);			/* <byte> */
							/* <nodeidx> */
		    put_bytes(fd, (long_u)np->wn_child->wn_u1.index, 3);
		}
	    }
	    else if (np->wn_child->wn_u2.wnode == NULL)
		/* We will write the child below and give it an index. */
		np->wn_child->wn_u2.wnode = node;

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
	if (np->wn_byte != 0 && np->wn_child->wn_u2.wnode == node)
	    newindex = put_node(fd, np->wn_child, newindex, regionmask,
								  prefixtree);

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
    ga_init2(&spin.si_prefcond, (int)sizeof(char_u *), 50);

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
	else if (fcount == 1)
	{
	    /* For ":mkspell path/vim" output file is "path/vim.latin1.spl". */
	    innames = &fnames[0];
	    incount = 1;
	    vim_snprintf((char *)wfname, sizeof(wfname), "%s.%s.spl", fnames[0],
			     spin.si_ascii ? (char_u *)"ascii" : spell_enc());
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
    else if (vim_strchr(gettail(wfname), '_') != NULL)
	EMSG(_("E751: Output file name must not have region name"));
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

	spin.si_foldroot = wordtree_alloc(&spin.si_blocks);
	spin.si_keeproot = wordtree_alloc(&spin.si_blocks);
	spin.si_prefroot = wordtree_alloc(&spin.si_blocks);
	if (spin.si_foldroot == NULL
		|| spin.si_keeproot == NULL
		|| spin.si_prefroot == NULL)
	{
	    error = TRUE;
	    return;
	}

	/* When not producing a .add.spl file clear the character table when
	 * we encounter one in the .aff file.  This means we dump the current
	 * one in the .spl file if the .aff file doesn't define one.  That's
	 * better than guessing the contents, the table will match a
	 * previously loaded spell file. */
	if (!spin.si_add)
	    spin.si_clear_chartab = TRUE;

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
	    spin.si_prefroot = spin.si_prefroot->wn_sibling;

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
	    wordtree_compress(spin.si_prefroot, &spin);
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
		smsg((char_u *)_("Writing spell file %s ..."), wfname);
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
	ga_clear(&spin.si_rep);
	ga_clear(&spin.si_sal);
	ga_clear(&spin.si_map);
	ga_clear(&spin.si_prefcond);
	vim_free(spin.si_midword);

	/* Free the .aff file structures. */
	for (i = 0; i < incount; ++i)
	    if (afile[i] != NULL)
		spell_free_aff(afile[i]);

	/* Free all the bits and pieces at once. */
	free_blocks(spin.si_blocks);
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
    int		new_spf = FALSE;
    struct stat	st;

    /* If 'spellfile' isn't set figure out a good default value. */
    if (*curbuf->b_p_spf == NUL)
    {
	init_spellfile();
	new_spf = TRUE;
    }

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
	    if (fd == NULL && new_spf)
	    {
		/* We just initialized the 'spellfile' option and can't open
		 * the file.  We may need to create the "spell" directory
		 * first.  We already checked the runtime directory is
		 * writable in init_spellfile(). */
		STRCPY(NameBuff, curbuf->b_p_spf);
		*gettail_sep(NameBuff) = NUL;
		if (mch_stat((char *)NameBuff, &st) < 0)
		{
		    /* The directory doesn't exist.  Try creating it and
		     * opening the file again. */
		    vim_mkdir(NameBuff, 0755);
		    fd = mch_fopen((char *)curbuf->b_p_spf, "a");
		}
	    }

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
    char_u	*lend;

    if (*curbuf->b_p_spl != NUL && curbuf->b_langp.ga_len > 0)
    {
	/* Find the end of the language name.  Exclude the region. */
	for (lend = curbuf->b_p_spl; *lend != NUL
			&& vim_strchr((char_u *)",._", *lend) == NULL; ++lend)
	    ;

	/* Loop over all entries in 'runtimepath'.  Use the first one where we
	 * are allowed to write. */
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
			(int)(lend - curbuf->b_p_spl), curbuf->b_p_spl,
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
    vim_memset(spell_ismw, FALSE, sizeof(spell_ismw));
#ifdef FEAT_MBYTE
    vim_free(spell_ismw_mb);
    spell_ismw_mb = NULL;

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
 * Return TRUE if "p" points to a word character.
 * As a special case we see "midword" characters as word character when it is
 * followed by a word character.  This finds they'there but not 'they there'.
 * Thus this only works properly when past the first character of the word.
 */
    static int
spell_iswordp(p)
    char_u	*p;
{
#ifdef FEAT_MBYTE
    char_u	*s;
    int		l;
    int		c;

    if (has_mbyte)
    {
	l = MB_BYTE2LEN(*p);
	s = p;
	if (l == 1)
	{
	    /* be quick for ASCII */
	    if (spell_ismw[*p])
	    {
		s = p + 1;		/* skip a mid-word character */
		l = MB_BYTE2LEN(*s);
	    }
	}
	else
	{
	    c = mb_ptr2char(p);
	    if (c < 256 ? spell_ismw[c] : (spell_ismw_mb != NULL
				     && vim_strchr(spell_ismw_mb, c) != NULL))
	    {
		s = p + l;
		l = MB_BYTE2LEN(*s);
	    }
	}

	if (l > 1)
	    return mb_get_class(s) >= 2;
	return spelltab.st_isw[*s];
    }
#endif

    return spelltab.st_isw[spell_ismw[*p] ? p[1] : p[0]];
}

/*
 * Write the table with prefix conditions to the .spl file.
 */
    static void
write_spell_prefcond(fd, gap)
    FILE	*fd;
    garray_T	*gap;
{
    int		i;
    char_u	*p;
    int		len;

    put_bytes(fd, (long_u)gap->ga_len, 2);	    /* <prefcondcnt> */

    for (i = 0; i < gap->ga_len; ++i)
    {
	/* <prefcond> : <condlen> <condstr> */
	p = ((char_u **)gap->ga_data)[i];
	if (p == NULL)
	    fputc(0, fd);
	else
	{
	    len = STRLEN(p);
	    fputc(len, fd);
	    fwrite(p, (size_t)len, (size_t)1, fd);
	}
    }
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
 */
    void
spell_suggest()
{
    char_u	*line;
    pos_T	prev_cursor = curwin->w_cursor;
    char_u	wcopy[MAXWLEN + 2];
    char_u	*p;
    int		i;
    int		c;
    suginfo_T	sug;
    suggest_T	*stp;

    /* Find the start of the badly spelled word. */
    if (spell_move_to(FORWARD, TRUE, TRUE) == FAIL
	    || curwin->w_cursor.col > prev_cursor.col)
    {
	if (!curwin->w_p_spell || *curbuf->b_p_spl == NUL)
	    return;

	/* No bad word or it starts after the cursor: use the word under the
	 * cursor. */
	curwin->w_cursor = prev_cursor;
	line = ml_get_curline();
	p = line + curwin->w_cursor.col;
	/* Backup to before start of word. */
	while (p > line && SPELL_ISWORDP(p))
	    mb_ptr_back(line, p);
	/* Forward to start of word. */
	while (!SPELL_ISWORDP(p))
	    mb_ptr_adv(p);

	if (!SPELL_ISWORDP(p))		/* No word found. */
	{
	    beep_flush();
	    return;
	}
	curwin->w_cursor.col = p - line;
    }

    /* Get the word and its length. */
    line = ml_get_curline();

    /* Get the list of suggestions */
    spell_find_suggest(line + curwin->w_cursor.col, &sug, (int)Rows - 2, TRUE);

    if (sug.su_ga.ga_len == 0)
	MSG(_("Sorry, no suggestions"));
    else
    {
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
	    stp = &SUG(sug.su_ga, i);

	    /* The suggested word may replace only part of the bad word, add
	     * the not replaced part. */
	    STRCPY(wcopy, stp->st_word);
	    if (sug.su_badlen > stp->st_orglen)
		vim_strncpy(wcopy + STRLEN(wcopy),
					       sug.su_badptr + stp->st_orglen,
					      sug.su_badlen - stp->st_orglen);
	    vim_snprintf((char *)IObuff, IOSIZE, _("%2d \"%s\""), i + 1, wcopy);
	    msg_puts(IObuff);

	    /* The word may replace more than "su_badlen". */
	    if (sug.su_badlen < stp->st_orglen)
	    {
		vim_snprintf((char *)IObuff, IOSIZE, _(" < \"%.*s\""),
					       stp->st_orglen, sug.su_badptr);
		msg_puts(IObuff);
	    }

	    if (p_verbose > 0)
	    {
		/* Add the score. */
		if (sps_flags & (SPS_DOUBLE | SPS_BEST))
		    vim_snprintf((char *)IObuff, IOSIZE, _(" (%s%d - %d)"),
			stp->st_salscore ? "s " : "",
			stp->st_score, stp->st_altscore);
		else
		    vim_snprintf((char *)IObuff, IOSIZE, _(" (%d)"),
			    stp->st_score);
		msg_advance(30);
		msg_puts(IObuff);
	    }
	    lines_left = 3;		/* avoid more prompt */
	    msg_putchar('\n');
	}

	/* Ask for choice. */
	i = prompt_for_number();
	if (i > 0 && i <= sug.su_ga.ga_len && u_save_cursor() == OK)
	{
	    /* Replace the word. */
	    stp = &SUG(sug.su_ga, i - 1);
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

		/* For redo we use a change-word command. */
		ResetRedobuff();
		AppendToRedobuff((char_u *)"ciw");
		AppendToRedobuff(stp->st_word);
		AppendCharToRedobuff(ESC);
	    }
	}
	else
	    curwin->w_cursor = prev_cursor;
    }

    spell_find_cleanup(&sug);
}

/*
 * Find spell suggestions for "word".  Return them in the growarray "*gap" as
 * a list of allocated strings.
 */
    void
spell_suggest_list(gap, word, maxcount)
    garray_T	*gap;
    char_u	*word;
    int		maxcount;	/* maximum nr of suggestions */
{
    suginfo_T	sug;
    int		i;
    suggest_T	*stp;
    char_u	*wcopy;

    spell_find_suggest(word, &sug, maxcount, FALSE);

    /* Make room in "gap". */
    ga_init2(gap, sizeof(char_u *), sug.su_ga.ga_len + 1);
    if (ga_grow(gap, sug.su_ga.ga_len) == FAIL)
	return;

    for (i = 0; i < sug.su_ga.ga_len; ++i)
    {
	stp = &SUG(sug.su_ga, i);

	/* The suggested word may replace only part of "word", add the not
	 * replaced part. */
	wcopy = alloc(STRLEN(stp->st_word)
				+ STRLEN(sug.su_badptr + stp->st_orglen) + 1);
	if (wcopy == NULL)
	    break;
	STRCPY(wcopy, stp->st_word);
	STRCAT(wcopy, sug.su_badptr + stp->st_orglen);
	((char_u **)gap->ga_data)[gap->ga_len++] = wcopy;
    }

    spell_find_cleanup(&sug);
}

/*
 * Find spell suggestions for the word at the start of "badptr".
 * Return the suggestions in "su->su_ga".
 * The maximum number of suggestions is "maxcount".
 * Note: does use info for the current window.
 * This is based on the mechanisms of Aspell, but completely reimplemented.
 */
    static void
spell_find_suggest(badptr, su, maxcount, banbadword)
    char_u	*badptr;
    suginfo_T	*su;
    int		maxcount;
    int		banbadword;	/* don't include badword in suggestions */
{
    int		attr;

    /*
     * Set the info in "*su".
     */
    vim_memset(su, 0, sizeof(suginfo_T));
    ga_init2(&su->su_ga, (int)sizeof(suggest_T), 10);
    ga_init2(&su->su_sga, (int)sizeof(suggest_T), 10);
    if (*badptr == NUL)
	return;
    hash_init(&su->su_banned);

    su->su_badptr = badptr;
    su->su_badlen = spell_check(curwin, su->su_badptr, &attr);
    su->su_maxcount = maxcount;

    if (su->su_badlen >= MAXWLEN)
	su->su_badlen = MAXWLEN - 1;	/* just in case */
    vim_strncpy(su->su_badword, su->su_badptr, su->su_badlen);
    (void)spell_casefold(su->su_badptr, su->su_badlen,
						    su->su_fbadword, MAXWLEN);
    /* get caps flags for bad word */
    su->su_badflags = captype(su->su_badptr, su->su_badptr + su->su_badlen);

    /* Ban the bad word itself.  It may appear in another region. */
    if (banbadword)
	add_banned(su, su->su_badword);

    /*
     * 1. Try special cases, such as repeating a word: "the the" -> "the".
     *
     * Set a maximum score to limit the combination of operations that is
     * tried.
     */
    su->su_maxscore = SCORE_MAXINIT;
    suggest_try_special(su);

    /*
     * 2. Try inserting/deleting/swapping/changing a letter, use REP entries
     *    from the .aff file and inserting a space (split the word).
     */
    suggest_try_change(su);

    /* For the resulting top-scorers compute the sound-a-like score. */
    if (sps_flags & SPS_DOUBLE)
	score_comp_sal(su);

    /*
     * 3. Try finding sound-a-like words.
     *
     * Only do this when we don't have a lot of suggestions yet, because it's
     * very slow and often doesn't find new suggestions.
     */
    if ((sps_flags & SPS_DOUBLE)
	    || (!(sps_flags & SPS_FAST)
				    && su->su_ga.ga_len < SUG_CLEAN_COUNT(su)))
    {
	/* Allow a higher score now. */
	su->su_maxscore = SCORE_MAXMAX;
	suggest_try_soundalike(su);
    }

    /* When CTRL-C was hit while searching do show the results. */
    ui_breakcheck();
    if (got_int)
    {
	(void)vgetc();
	got_int = FALSE;
    }

    if (sps_flags & SPS_DOUBLE)
    {
	/* Combine the two list of suggestions. */
	score_combine(su);
    }
    else if (su->su_ga.ga_len != 0)
    {
	if (sps_flags & SPS_BEST)
	    /* Adjust the word score for how it sounds like. */
	    rescore_suggestions(su);

	/* Sort the suggestions and truncate at "maxcount". */
	(void)cleanup_suggestions(&su->su_ga, su->su_maxscore, maxcount);
    }
}

/*
 * Free the info put in "*su" by spell_find_suggest().
 */
    static void
spell_find_cleanup(su)
    suginfo_T	*su;
{
    int		i;

    /* Free the suggestions. */
    for (i = 0; i < su->su_ga.ga_len; ++i)
	vim_free(SUG(su->su_ga, i).st_word);
    ga_clear(&su->su_ga);
    for (i = 0; i < su->su_sga.ga_len; ++i)
	vim_free(SUG(su->su_sga, i).st_word);
    ga_clear(&su->su_sga);

    /* Free the banned words. */
    free_banned(su);
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
 * Try finding suggestions by recognizing specific situations.
 */
    static void
suggest_try_special(su)
    suginfo_T	*su;
{
    char_u	*p;
    int		len;
    int		c;
    char_u	word[MAXWLEN];

    /*
     * Recognize a word that is repeated: "the the".
     */
    p = skiptowhite(su->su_fbadword);
    len = p - su->su_fbadword;
    p = skipwhite(p);
    if (STRLEN(p) == len && STRNCMP(su->su_fbadword, p, len) == 0)
    {
	/* Include badflags: if the badword is onecap or allcap
	 * use that for the goodword too: "The the" -> "The". */
	c = su->su_fbadword[len];
	su->su_fbadword[len] = NUL;
	make_case_word(su->su_fbadword, word, su->su_badflags);
	su->su_fbadword[len] = c;
	add_suggestion(su, &su->su_ga, word, su->su_badlen, SCORE_DEL, 0, TRUE);
    }
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
suggest_try_change(su)
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
    garray_T	*gap;
    idx_T	arridx;
    int		len;
    char_u	*p;
    fromto_T	*ftp;
    int		fl = 0, tl;
    int		repextra = 0;	    /* extra bytes in fword[] from REP item */

    /* We make a copy of the case-folded bad word, so that we can modify it
     * to find matches (esp. REP items).  Append some more text, changing
     * chars after the bad word may help. */
    STRCPY(fword, su->su_fbadword);
    n = STRLEN(fword);
    p = su->su_badptr + su->su_badlen;
    (void)spell_casefold(p, STRLEN(p), fword + n, MAXWLEN - n);

    for (lp = LANGP_ENTRY(curwin->w_buffer->b_langp, 0);
						   lp->lp_slang != NULL; ++lp)
    {
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

		if (sp->ts_curi > len || byts[arridx] != 0)
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
		{
		    /* Include badflags: if the badword is onecap or allcap
		     * use that for the goodword too.  But if the badword is
		     * allcap and it's only one char long use onecap. */
		    c = su->su_badflags;
		    if ((c & WF_ALLCAP)
#ifdef FEAT_MBYTE
			    && su->su_badlen == mb_ptr2len_check(su->su_badptr)
#else
			    && su->su_badlen == 1
#endif
			    )
			c = WF_ONECAP;
		    make_case_word(tword + splitoff,
					     preword + prewordlen, flags | c);
		}

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

		if (!spell_valid_case(su->su_badflags,
					 captype(preword + prewordlen, NULL)))
		    newscore += SCORE_ICASE;

		if ((fword[sp->ts_fidx] == NUL
				       || !spell_iswordp(fword + sp->ts_fidx))
			&& sp->ts_fidx >= sp->ts_fidxtry)
		{
		    /* The badword also ends: add suggestions.  Give a penalty
		     * when changing non-word char to word char, e.g., "thes,"
		     * -> "these". */
		    p = fword + sp->ts_fidx;
#ifdef FEAT_MBYTE
		    if (has_mbyte)
			mb_ptr_back(fword, p);
		    else
#endif
			--p;
		    if (!spell_iswordp(p))
		    {
			p = preword + STRLEN(preword);
#ifdef FEAT_MBYTE
			if (has_mbyte)
			    mb_ptr_back(preword, p);
			else
#endif
			    --p;
			if (spell_iswordp(p))
			    newscore += SCORE_NONWORD;
		    }

		    add_suggestion(su, &su->su_ga, preword,
			    sp->ts_fidx - repextra,
					   sp->ts_score + newscore, 0, FALSE);
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
			sp->ts_save_badflags = su->su_badflags;
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
			su->su_badflags = captype(p, su->su_badptr
							     + su->su_badlen);

			sp->ts_state = STATE_SPLITUNDO;
			++depth;
			/* Restart at top of the tree. */
			stack[depth].ts_arridx = 0;
		    }
		}
		break;

	    case STATE_SPLITUNDO:
		/* Undo the changes done for word split. */
		su->su_badflags = sp->ts_save_badflags;
		splitoff = sp->ts_save_splitoff;
		prewordlen =  sp->ts_save_prewordlen;

		/* Continue looking for NUL bytes. */
		sp->ts_state = STATE_START;
		break;

	    case STATE_ENDNUL:
		/* Past the NUL bytes in the node. */
		if (fword[sp->ts_fidx] == NUL)
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
				else if (sp->ts_isdiff == DIFF_INSERT
					&& sp->ts_twordlen > sp->ts_tcharlen)
				{
				    /* If the previous character was the same,
				     * thus doubling a character, give a bonus
				     * to the score. */
				    p = tword + sp->ts_twordlen
							    - sp->ts_tcharlen;
				    c = mb_ptr2char(p);
				    mb_ptr_back(tword, p);
				    if (c == mb_ptr2char(p))
					sp->ts_score -= SCORE_INS
							       - SCORE_INSDUP;
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

		    /* Advance over the character in fword[]. Give a bonus to
		     * the score if the same character is following "nn" ->
		     * "n". */
#ifdef FEAT_MBYTE
		    if (has_mbyte)
		    {
			c = mb_ptr2char(fword + sp->ts_fidx);
			stack[depth].ts_fidx += MB_BYTE2LEN(fword[sp->ts_fidx]);
			if (c == mb_ptr2char(fword + stack[depth].ts_fidx))
			    stack[depth].ts_score -= SCORE_DEL - SCORE_DELDUP;
		    }
		    else
#endif
		    {
			++stack[depth].ts_fidx;
			if (fword[sp->ts_fidx] == fword[sp->ts_fidx + 1])
			    stack[depth].ts_score -= SCORE_DEL - SCORE_DELDUP;
		    }
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
			else
			    fl = 1;
			if (fl == 1)
#endif
			{
			    /* If the previous character was the same, thus
			     * doubling a character, give a bonus to the
			     * score. */
			    if (sp->ts_twordlen >= 2
					   && tword[sp->ts_twordlen - 2] == c)
				sp->ts_score -= SCORE_INS - SCORE_INSDUP;
			}
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
		/* Undo ROT3L: "231" -> "123" */
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
		/* Undo ROT3R: "312" -> "123" */
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
			{
			    mch_memmove(p + tl, p + fl, STRLEN(p + fl) + 1);
			    repextra += tl - fl;
			}
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
		{
		    mch_memmove(p + fl, p + tl, STRLEN(p + tl) + 1);
		    repextra -= tl - fl;
		}
		mch_memmove(p, ftp->ft_from, fl);
		sp->ts_state = STATE_REP;
		break;

	    default:
		/* Did all possible states at this level, go up one level. */
		--depth;

		/* Don't check for CTRL-C too often, it takes time. */
		line_breakcheck();
	    }
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
 * Compute the sound-a-like score for suggestions in su->su_ga and add them to
 * su->su_sga.
 */
    static void
score_comp_sal(su)
    suginfo_T	*su;
{
    langp_T	*lp;
    char_u	badsound[MAXWLEN];
    int		i;
    suggest_T   *stp;
    suggest_T   *sstp;
    int		score;

    if (ga_grow(&su->su_sga, su->su_ga.ga_len) == FAIL)
	return;

    /*	Use the sound-folding of the first language that supports it. */
    for (lp = LANGP_ENTRY(curwin->w_buffer->b_langp, 0);
						   lp->lp_slang != NULL; ++lp)
	if (lp->lp_slang->sl_sal.ga_len > 0)
	{
	    /* soundfold the bad word */
	    spell_soundfold(lp->lp_slang, su->su_fbadword, badsound);

	    for (i = 0; i < su->su_ga.ga_len; ++i)
	    {
		stp = &SUG(su->su_ga, i);

		/* Case-fold the suggested word, sound-fold it and compute the
		 * sound-a-like score. */
		score = stp_sal_score(stp, su, lp->lp_slang, badsound);
		if (score < SCORE_MAXMAX)
		{
		    /* Add the suggestion. */
		    sstp = &SUG(su->su_sga, su->su_sga.ga_len);
		    sstp->st_word = vim_strsave(stp->st_word);
		    if (sstp->st_word != NULL)
		    {
			sstp->st_score = score;
			sstp->st_altscore = 0;
			sstp->st_orglen = stp->st_orglen;
			++su->su_sga.ga_len;
		    }
		}
	    }
	    break;
	}
}

/*
 * Combine the list of suggestions in su->su_ga and su->su_sga.
 * They are intwined.
 */
    static void
score_combine(su)
    suginfo_T	*su;
{
    int		i;
    int		j;
    garray_T	ga;
    garray_T	*gap;
    langp_T	*lp;
    suggest_T	*stp;
    char_u	*p;
    char_u	badsound[MAXWLEN];
    int		round;

    /* Add the alternate score to su_ga. */
    for (lp = LANGP_ENTRY(curwin->w_buffer->b_langp, 0);
						   lp->lp_slang != NULL; ++lp)
    {
	if (lp->lp_slang->sl_sal.ga_len > 0)
	{
	    /* soundfold the bad word */
	    spell_soundfold(lp->lp_slang, su->su_fbadword, badsound);

	    for (i = 0; i < su->su_ga.ga_len; ++i)
	    {
		stp = &SUG(su->su_ga, i);
		stp->st_altscore = stp_sal_score(stp, su, lp->lp_slang,
								    badsound);
		if (stp->st_altscore == SCORE_MAXMAX)
		    stp->st_score = (stp->st_score * 3 + SCORE_BIG) / 4;
		else
		    stp->st_score = (stp->st_score * 3
						  + stp->st_altscore) / 4;
		stp->st_salscore = FALSE;
	    }
	    break;
	}
    }

    /* Add the alternate score to su_sga. */
    for (i = 0; i < su->su_sga.ga_len; ++i)
    {
	stp = &SUG(su->su_sga, i);
	stp->st_altscore = spell_edit_score(su->su_badword, stp->st_word);
	if (stp->st_score == SCORE_MAXMAX)
	    stp->st_score = (SCORE_BIG * 7 + stp->st_altscore) / 8;
	else
	    stp->st_score = (stp->st_score * 7 + stp->st_altscore) / 8;
	stp->st_salscore = TRUE;
    }

    /* Sort the suggestions and truncate at "maxcount" for both lists. */
    (void)cleanup_suggestions(&su->su_ga, su->su_maxscore, su->su_maxcount);
    (void)cleanup_suggestions(&su->su_sga, su->su_maxscore, su->su_maxcount);

    ga_init2(&ga, (int)sizeof(suginfo_T), 1);
    if (ga_grow(&ga, su->su_ga.ga_len + su->su_sga.ga_len) == FAIL)
	return;

    stp = &SUG(ga, 0);
    for (i = 0; i < su->su_ga.ga_len || i < su->su_sga.ga_len; ++i)
    {
	/* round 1: get a suggestion from su_ga
	 * round 2: get a suggestion from su_sga */
	for (round = 1; round <= 2; ++round)
	{
	    gap = round == 1 ? &su->su_ga : &su->su_sga;
	    if (i < gap->ga_len)
	    {
		/* Don't add a word if it's already there. */
		p = SUG(*gap, i).st_word;
		for (j = 0; j < ga.ga_len; ++j)
		    if (STRCMP(stp[j].st_word, p) == 0)
			break;
		if (j == ga.ga_len)
		    stp[ga.ga_len++] = SUG(*gap, i);
		else
		    vim_free(p);
	    }
	}
    }

    ga_clear(&su->su_ga);
    ga_clear(&su->su_sga);

    /* Truncate the list to the number of suggestions that will be displayed. */
    if (ga.ga_len > su->su_maxcount)
    {
	for (i = su->su_maxcount; i < ga.ga_len; ++i)
	    vim_free(stp[i].st_word);
	ga.ga_len = su->su_maxcount;
    }

    su->su_ga = ga;
}

/*
 * For the goodword in "stp" compute the soundalike score compared to the
 * badword.
 */
    static int
stp_sal_score(stp, su, slang, badsound)
    suggest_T	*stp;
    suginfo_T	*su;
    slang_T	*slang;
    char_u	*badsound;	/* sound-folded badword */
{
    char_u	*p;
    char_u	badsound2[MAXWLEN];
    char_u	fword[MAXWLEN];
    char_u	goodsound[MAXWLEN];

    if (stp->st_orglen <= su->su_badlen)
	p = badsound;
    else
    {
	/* soundfold the bad word with more characters following */
	(void)spell_casefold(su->su_badptr, stp->st_orglen, fword, MAXWLEN);

	/* When joining two words the sound often changes a lot.  E.g., "t he"
	 * sounds like "t h" while "the" sounds like "@".  Avoid that by
	 * removing the space.  Don't do it when the good word also contains a
	 * space. */
	if (vim_iswhite(su->su_badptr[su->su_badlen])
					 && *skiptowhite(stp->st_word) == NUL)
	    for (p = fword; *(p = skiptowhite(p)) != NUL; )
		mch_memmove(p, p + 1, STRLEN(p));

	spell_soundfold(slang, fword, badsound2);
	p = badsound2;
    }

    /* Case-fold the word, sound-fold the word and compute the score for the
     * difference. */
    (void)spell_casefold(stp->st_word, STRLEN(stp->st_word), fword, MAXWLEN);
    spell_soundfold(slang, fword, goodsound);

    return soundalike_score(goodsound, p);
}

/*
 * Find suggestions by comparing the word in a sound-a-like form.
 */
    static void
suggest_try_soundalike(su)
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
    int		sound_score;

    /* Do this for all languages that support sound folding. */
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
		    {
			/* Done all bytes at this node, go up one level. */
			--depth;
			line_breakcheck();
		    }
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

				/* Compute the edit distance between the
				 * sound-a-like words. */
				sound_score = soundalike_score(salword,
								    tsalword);
				if (sound_score < SCORE_MAXMAX)
				{
				    char_u	cword[MAXWLEN];
				    char_u	*p;
				    int		score;

				    if (round == 1 && (flags & WF_CAPMASK) != 0)
				    {
					/* Need to fix case according to
					 * "flags". */
					make_case_word(tword, cword, flags);
					p = cword;
				    }
				    else
					p = tword;

				    if (sps_flags & SPS_DOUBLE)
					add_suggestion(su, &su->su_sga, p,
						su->su_badlen,
						       sound_score, 0, FALSE);
				    else
				    {
					/* Compute the score. */
					score = spell_edit_score(
							   su->su_badword, p);
					if (sps_flags & SPS_BEST)
					    /* give a bonus for the good word
					     * sounding the same as the bad
					     * word */
					    add_suggestion(su, &su->su_ga, p,
						    su->su_badlen,
						  RESCORE(score, sound_score),
							   sound_score, TRUE);
					else
					    add_suggestion(su, &su->su_ga, p,
						    su->su_badlen,
					       score + sound_score, 0, FALSE);
				    }
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
add_suggestion(su, gap, goodword, badlen, score, altscore, had_bonus)
    suginfo_T	*su;
    garray_T	*gap;
    char_u	*goodword;
    int		badlen;		/* length of bad word used */
    int		score;
    int		altscore;
    int		had_bonus;	/* value for st_had_bonus */
{
    suggest_T   *stp;
    int		i;
    char_u	*p = NULL;
    int		c = 0;

    /* Check that the word wasn't banned. */
    if (was_banned(su, goodword))
	return;

    /* If past "su_badlen" and the rest is identical stop at "su_badlen".
     * Remove the common part from "goodword". */
    i = badlen - su->su_badlen;
    if (i > 0)
    {
	/* This assumes there was no case folding or it didn't change the
	 * length... */
	p = goodword + STRLEN(goodword) - i;
	if (p > goodword && STRNICMP(su->su_badptr + su->su_badlen, p, i) == 0)
	{
	    badlen = su->su_badlen;
	    c = *p;
	    *p = NUL;
	}
	else
	    p = NULL;
    }

    if (score <= su->su_maxscore)
    {
	/* Check if the word is already there.  Also check the length that is
	 * being replaced "thes," -> "these" is a different suggestion from
	 * "thes" -> "these". */
	stp = &SUG(*gap, 0);
	for (i = gap->ga_len - 1; i >= 0; --i)
	    if (STRCMP(stp[i].st_word, goodword) == 0
						&& stp[i].st_orglen == badlen)
	    {
		/* Found it.  Remember the lowest score. */
		if (stp[i].st_score > score)
		{
		    stp[i].st_score = score;
		    stp[i].st_had_bonus = had_bonus;
		}
		break;
	    }

	if (i < 0 && ga_grow(gap, 1) == OK)
	{
	    /* Add a suggestion. */
	    stp = &SUG(*gap, gap->ga_len);
	    stp->st_word = vim_strsave(goodword);
	    if (stp->st_word != NULL)
	    {
		stp->st_score = score;
		stp->st_altscore = altscore;
		stp->st_had_bonus = had_bonus;
		stp->st_orglen = badlen;
		++gap->ga_len;

		/* If we have too many suggestions now, sort the list and keep
		 * the best suggestions. */
		if (gap->ga_len > SUG_MAX_COUNT(su))
		    su->su_maxscore = cleanup_suggestions(gap, su->su_maxscore,
							 SUG_CLEAN_COUNT(su));
	    }
	}
    }

    if (p != NULL)
	*p = c;		/* restore "goodword" */
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
	else
	    vim_free(s);
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
		stp = &SUG(su->su_ga, i);
		if (!stp->st_had_bonus)
		{
		    stp->st_altscore = stp_sal_score(stp, su,
						   lp->lp_slang, sal_badword);
		    if (stp->st_altscore == SCORE_MAXMAX)
			stp->st_altscore = SCORE_BIG;
		    stp->st_score = RESCORE(stp->st_score, stp->st_altscore);
		}
	    }
	    break;
	}
    }
}

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
    int		n = p1->st_score - p2->st_score;

    if (n == 0)
	return p1->st_altscore - p2->st_altscore;
    return n;
}

/*
 * Cleanup the suggestions:
 * - Sort on score.
 * - Remove words that won't be displayed.
 * Returns the maximum score in the list or "maxscore" unmodified.
 */
    static int
cleanup_suggestions(gap, maxscore, keep)
    garray_T	*gap;
    int		maxscore;
    int		keep;		/* nr of suggestions to keep */
{
    suggest_T   *stp = &SUG(*gap, 0);
    int		i;

    /* Sort the list. */
    qsort(gap->ga_data, (size_t)gap->ga_len, sizeof(suggest_T), sug_compare);

    /* Truncate the list to the number of suggestions that will be displayed. */
    if (gap->ga_len > keep)
    {
	for (i = keep; i < gap->ga_len; ++i)
	    vim_free(stp[i].st_word);
	gap->ga_len = keep;
	return stp[keep - 1].st_score;
    }
    return maxscore;
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
    salitem_T	*smp;
    char_u	word[MAXWLEN];
#ifdef FEAT_MBYTE
    int		l;
    int		found_mbyte = FALSE;
#endif
    char_u	*s;
    char_u	*t;
    char_u	*pf;
    int		i, j, z;
    int		reslen;
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
	    {
		*t++ = ' ';
		s = skipwhite(s);
	    }
#ifdef FEAT_MBYTE
	    else if (has_mbyte)
	    {
		l = mb_ptr2len_check(s);
		if (spell_iswordp(s))
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
		if (spell_iswordp(s))
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

    smp = (salitem_T *)slang->sl_sal.ga_data;

    /*
     * This comes from Aspell phonet.cpp.  Converted from C++ to C.
     * Changed to keep spaces.
     * TODO: support for multi-byte chars.
     */
    i = reslen = z = 0;
    while ((c = word[i]) != NUL)
    {
	/* Start with the first rule that has the character in the word. */
	n = slang->sl_sal_first[c];
	z0 = 0;

	if (n >= 0)
	{
	    /* check all rules for the same letter */
	    for (; (s = smp[n].sm_lead)[0] == c; ++n)
	    {
		/* Quickly skip entries that don't match the word.  Most
		 * entries are less then three chars, optimize for that. */
		k = smp[n].sm_leadlen;
		if (k > 1)
		{
		    if (word[i + 1] != s[1])
			continue;
		    if (k > 2)
		    {
			for (j = 2; j < k; ++j)
			    if (word[i + j] != s[j])
				break;
			if (j < k)
			    continue;
		    }
		}

		if ((pf = smp[n].sm_oneoff) != NULL)
		{
		    /* Check for match with one of the chars in "sm_oneoff". */
		    while (*pf != NUL && *pf != word[i + k])
			++pf;
		    if (*pf == NUL)
			continue;
		    ++k;
		}
		s = smp[n].sm_rules;
		pri = 5;    /* default priority */

		p0 = *s;
		k0 = k;
		while (*s == '-' && k > 1)
		{
		    k--;
		    s++;
		}
		if (*s == '<')
		    s++;
		if (VIM_ISDIGIT(*s))
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
					      || spell_iswordp(word + i - 1)))
			    && (*(s + 1) != '$'
				|| (!spell_iswordp(word + i + k0))))
			|| (*s == '$' && i > 0
			    && spell_iswordp(word + i - 1)
			    && (!spell_iswordp(word + i + k0))))
		{
		    /* search for followup rules, if:    */
		    /* followup and k > 1  and  NO '-' in searchstring */
		    c0 = word[i + k - 1];
		    n0 = slang->sl_sal_first[c0];

		    if (slang->sl_followup && k > 1 && n0 >= 0
					   && p0 != '-' && word[i + k] != NUL)
		    {
			/* test follow-up rule for "word[i + k]" */
			for ( ; (s = smp[n0].sm_lead)[0] == c0; ++n0)
			{
			    /* Quickly skip entries that don't match the word.
			     * */
			    k0 = smp[n0].sm_leadlen;
			    if (k0 > 1)
			    {
				if (word[i + k] != s[1])
				    continue;
				if (k0 > 2)
				{
				    pf = word + i + k + 1;
				    for (j = 2; j < k0; ++j)
					if (*pf++ != s[j])
					    break;
				    if (j < k0)
					continue;
				}
			    }
			    k0 += k - 1;

			    if ((pf = smp[n0].sm_oneoff) != NULL)
			    {
				/* Check for match with one of the chars in
				 * "sm_oneoff". */
				while (*pf != NUL && *pf != word[i + k0])
				    ++pf;
				if (*pf == NUL)
				    continue;
				++k0;
			    }

			    p0 = 5;
			    s = smp[n0].sm_rules;
			    while (*s == '-')
			    {
				/* "k0" gets NOT reduced because
				 * "if (k0 == k)" */
				s++;
			    }
			    if (*s == '<')
				s++;
			    if (VIM_ISDIGIT(*s))
			    {
				p0 = *s - '0';
				s++;
			    }

			    if (*s == NUL
				    /* *s == '^' cuts */
				    || (*s == '$'
					    && !spell_iswordp(word + i + k0)))
			    {
				if (k0 == k)
				    /* this is just a piece of the string */
				    continue;

				if (p0 < pri)
				    /* priority too low */
				    continue;
				/* rule fits; stop search */
				break;
			    }
			}

			if (p0 >= pri && smp[n0].sm_lead[0] == c0)
			    continue;
		    }

		    /* replace string */
		    s = smp[n].sm_to;
		    pf = smp[n].sm_rules;
		    p0 = (vim_strchr(pf, '<') != NULL) ? 1 : 0;
		    if (p0 == 1 && z == 0)
		    {
			/* rule with '<' is used */
			if (reslen > 0 && *s != NUL && (res[reslen - 1] == c
						    || res[reslen - 1] == *s))
			    reslen--;
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
			while (*s != NUL && s[1] != NUL && reslen < MAXWLEN)
			{
			    if (reslen == 0 || res[reslen - 1] != *s)
			    {
				res[reslen] = *s;
				reslen++;
			    }
			    s++;
			}
			/* new "actual letter" */
			c = *s;
			if (strstr((char *)pf, "^^") != NULL)
			{
			    if (c != NUL)
			    {
				res[reslen] = c;
				reslen++;
			    }
			    mch_memmove(word, word + i + 1,
						    STRLEN(word + i + 1) + 1);
			    i = 0;
			    z0 = 1;
			}
		    }
		    break;
		}
	    }
	}
	else if (vim_iswhite(c))
	{
	    c = ' ';
	    k = 1;
	}

	if (z0 == 0)
	{
	    if (k && !p0 && reslen < MAXWLEN && c != NUL
		    && (!slang->sl_collapse || reslen == 0
						     || res[reslen - 1] != c))
	    {
		/* condense only double letters */
		res[reslen] = c;
		reslen++;
	    }

	    i++;
	    z = 0;
	    k = 0;
	}
    }

    res[reslen] = NUL;
}

/*
 * Compute a score for two sound-a-like words.
 * This permits up to two inserts/deletes/swaps/etc. to keep things fast.
 * Instead of a generic loop we write out the code.  That keeps it fast by
 * avoiding checks that will not be possible.
 */
    static int
soundalike_score(goodstart, badstart)
    char_u	*goodstart;	/* sound-folded good word */
    char_u	*badstart;	/* sound-folded bad word */
{
    char_u	*goodsound = goodstart;
    char_u	*badsound = badstart;
    int		goodlen;
    int		badlen;
    int		n;
    char_u	*pl, *ps;
    char_u	*pl2, *ps2;
    int		score = 0;

    /* adding/inserting "*" at the start (word starts with vowel) shouldn't be
     * counted so much, vowels halfway the word aren't counted at all. */
    if ((*badsound == '*' || *goodsound == '*') && *badsound != *goodsound)
    {
	score = SCORE_DEL / 2;
	if (*badsound == '*')
	    ++badsound;
	else
	    ++goodsound;
    }

    goodlen = STRLEN(goodsound);
    badlen = STRLEN(badsound);

    /* Return quickly if the lenghts are too different to be fixed by two
     * changes. */
    n = goodlen - badlen;
    if (n < -2 || n > 2)
	return SCORE_MAXMAX;

    if (n > 0)
    {
	pl = goodsound;	    /* goodsound is longest */
	ps = badsound;
    }
    else
    {
	pl = badsound;	    /* badsound is longest */
	ps = goodsound;
    }

    /* Skip over the identical part. */
    while (*pl == *ps && *pl != NUL)
    {
	++pl;
	++ps;
    }

    switch (n)
    {
	case -2:
	case 2:
	    /*
	     * Must delete two characters from "pl".
	     */
	    ++pl;	/* first delete */
	    while (*pl == *ps)
	    {
		++pl;
		++ps;
	    }
	    /* strings must be equal after second delete */
	    if (STRCMP(pl + 1, ps) == 0)
		return score + SCORE_DEL * 2;

	    /* Failed to compare. */
	    break;

	case -1:
	case 1:
	    /*
	     * Minimal one delete from "pl" required.
	     */

	    /* 1: delete */
	    pl2 = pl + 1;
	    ps2 = ps;
	    while (*pl2 == *ps2)
	    {
		if (*pl2 == NUL)	/* reached the end */
		    return score + SCORE_DEL;
		++pl2;
		++ps2;
	    }

	    /* 2: delete then swap, then rest must be equal */
	    if (pl2[0] == ps2[1] && pl2[1] == ps2[0]
					     && STRCMP(pl2 + 2, ps2 + 2) == 0)
		return score + SCORE_DEL + SCORE_SWAP;

	    /* 3: delete then substitute, then the rest must be equal */
	    if (STRCMP(pl2 + 1, ps2 + 1) == 0)
		return score + SCORE_DEL + SCORE_SUBST;

	    /* 4: first swap then delete */
	    if (pl[0] == ps[1] && pl[1] == ps[0])
	    {
		pl2 = pl + 2;	    /* swap, skip two chars */
		ps2 = ps + 2;
		while (*pl2 == *ps2)
		{
		    ++pl2;
		    ++ps2;
		}
		/* delete a char and then strings must be equal */
		if (STRCMP(pl2 + 1, ps2) == 0)
		    return score + SCORE_SWAP + SCORE_DEL;
	    }

	    /* 5: first substitute then delete */
	    pl2 = pl + 1;	    /* substitute, skip one char */
	    ps2 = ps + 1;
	    while (*pl2 == *ps2)
	    {
		++pl2;
		++ps2;
	    }
	    /* delete a char and then strings must be equal */
	    if (STRCMP(pl2 + 1, ps2) == 0)
		return score + SCORE_SUBST + SCORE_DEL;

	    /* Failed to compare. */
	    break;

	case 0:
	    /*
	     * Lenghts are equal, thus changes must result in same length: An
	     * insert is only possible in combination with a delete.
	     * 1: check if for identical strings
	     */
	    if (*pl == NUL)
		return score;

	    /* 2: swap */
	    if (pl[0] == ps[1] && pl[1] == ps[0])
	    {
		pl2 = pl + 2;	    /* swap, skip two chars */
		ps2 = ps + 2;
		while (*pl2 == *ps2)
		{
		    if (*pl2 == NUL)	/* reached the end */
			return score + SCORE_SWAP;
		    ++pl2;
		    ++ps2;
		}
		/* 3: swap and swap again */
		if (pl2[0] == ps2[1] && pl2[1] == ps2[0]
					     && STRCMP(pl2 + 2, ps2 + 2) == 0)
		    return score + SCORE_SWAP + SCORE_SWAP;

		/* 4: swap and substitute */
		if (STRCMP(pl2 + 1, ps2 + 1) == 0)
		    return score + SCORE_SWAP + SCORE_SUBST;
	    }

	    /* 5: substitute */
	    pl2 = pl + 1;
	    ps2 = ps + 1;
	    while (*pl2 == *ps2)
	    {
		if (*pl2 == NUL)	/* reached the end */
		    return score + SCORE_SUBST;
		++pl2;
		++ps2;
	    }

	    /* 6: substitute and swap */
	    if (pl2[0] == ps2[1] && pl2[1] == ps2[0]
					     && STRCMP(pl2 + 2, ps2 + 2) == 0)
		return score + SCORE_SUBST + SCORE_SWAP;

	    /* 7: substitute and substitute */
	    if (STRCMP(pl2 + 1, ps2 + 1) == 0)
		return score + SCORE_SUBST + SCORE_SUBST;

	    /* 8: insert then delete */
	    pl2 = pl;
	    ps2 = ps + 1;
	    while (*pl2 == *ps2)
	    {
		++pl2;
		++ps2;
	    }
	    if (STRCMP(pl2 + 1, ps2) == 0)
		return score + SCORE_INS + SCORE_DEL;

	    /* 9: delete then insert */
	    pl2 = pl + 1;
	    ps2 = ps;
	    while (*pl2 == *ps2)
	    {
		++pl2;
		++ps2;
	    }
	    if (STRCMP(pl2, ps2 + 1) == 0)
		return score + SCORE_INS + SCORE_DEL;

	    /* Failed to compare. */
	    break;
    }

    return SCORE_MAXMAX;
}

/*
 * Compute the "edit distance" to turn "badword" into "goodword".  The less
 * deletes/inserts/substitutes/swaps are required the lower the score.
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

    i = CNT(badlen - 1, goodlen - 1);
    vim_free(cnt);
    return i;
}

/*
 * ":spelldump"
 */
/*ARGSUSED*/
    void
ex_spelldump(eap)
    exarg_T *eap;
{
    buf_T	*buf = curbuf;
    langp_T	*lp;
    slang_T	*slang;
    idx_T	arridx[MAXWLEN];
    int		curi[MAXWLEN];
    char_u	word[MAXWLEN];
    int		c;
    char_u	*byts;
    idx_T	*idxs;
    linenr_T	lnum = 0;
    int		round;
    int		depth;
    int		n;
    int		flags;

    if (no_spell_checking())
	return;

    /* Create a new empty buffer by splitting the window. */
    do_cmdline_cmd((char_u *)"new");
    if (!bufempty() || !buf_valid(buf))
	return;

    for (lp = LANGP_ENTRY(buf->b_langp, 0); lp->lp_slang != NULL; ++lp)
    {
	slang = lp->lp_slang;

	vim_snprintf((char *)IObuff, IOSIZE, "# file: %s", slang->sl_fname);
	ml_append(lnum++, IObuff, (colnr_T)0, FALSE);

	/* round 1: case-folded tree
	 * round 2: keep-case tree */
	for (round = 1; round <= 2; ++round)
	{
	    if (round == 1)
	    {
		byts = slang->sl_fbyts;
		idxs = slang->sl_fidxs;
	    }
	    else
	    {
		byts = slang->sl_kbyts;
		idxs = slang->sl_kidxs;
	    }
	    if (byts == NULL)
		continue;		/* array is empty */

	    depth = 0;
	    arridx[0] = 0;
	    curi[0] = 1;
	    while (depth >= 0 && !got_int)
	    {
		if (curi[depth] > byts[arridx[depth]])
		{
		    /* Done all bytes at this node, go up one level. */
		    --depth;
		    line_breakcheck();
		}
		else
		{
		    /* Do one more byte at this node. */
		    n = arridx[depth] + curi[depth];
		    ++curi[depth];
		    c = byts[n];
		    if (c == 0)
		    {
			/* End of word, deal with the word.
			 * Don't use keep-case words in the fold-case tree,
			 * they will appear in the keep-case tree.
			 * Only use the word when the region matches. */
			flags = (int)idxs[n];
			if ((round == 2 || (flags & WF_KEEPCAP) == 0)
				&& ((flags & WF_REGION) == 0
					|| (((unsigned)flags >> 8)
						       & lp->lp_region) != 0))
			{
			    word[depth] = NUL;

			    /* Dump the basic word if there is no prefix or
			     * when it's the first one. */
			    c = (unsigned)flags >> 16;
			    if (c == 0 || curi[depth] == 2)
				dump_word(word, round, flags, lnum++);

			    /* Apply the prefix, if there is one. */
			    if (c != 0)
				lnum = apply_prefixes(slang, word, round,
								 flags, lnum);
			}
		    }
		    else
		    {
			/* Normal char, go one level deeper. */
			word[depth++] = c;
			arridx[depth] = idxs[n];
			curi[depth] = 1;
		    }
		}
	    }
	}
    }

    /* Delete the empty line that we started with. */
    if (curbuf->b_ml.ml_line_count > 1)
	ml_delete(curbuf->b_ml.ml_line_count, FALSE);

    redraw_later(NOT_VALID);
}

/*
 * Dump one word: apply case modifications and append a line to the buffer.
 */
    static void
dump_word(word, round, flags, lnum)
    char_u	*word;
    int		round;
    int		flags;
    linenr_T	lnum;
{
    int		keepcap = FALSE;
    char_u	*p;
    char_u	cword[MAXWLEN];
    char_u	badword[MAXWLEN + 3];

    if (round == 1 && (flags & WF_CAPMASK) != 0)
    {
	/* Need to fix case according to "flags". */
	make_case_word(word, cword, flags);
	p = cword;
    }
    else
    {
	p = word;
	if (round == 2 && (captype(word, NULL) & WF_KEEPCAP) == 0)
	    keepcap = TRUE;
    }

    /* Bad word is preceded by "/!" and some other
     * flags. */
    if ((flags & (WF_BANNED | WF_RARE)) || keepcap)
    {
	STRCPY(badword, "/");
	if (keepcap)
	    STRCAT(badword, "=");
	if (flags & WF_BANNED)
	    STRCAT(badword, "!");
	else if (flags & WF_RARE)
	    STRCAT(badword, "?");
	STRCAT(badword, p);
	p = badword;
    }

    ml_append(lnum, p, (colnr_T)0, FALSE);
}

/*
 * Find matching prefixes for "word".  Prepend each to "word" and append
 * a line to the buffer.
 * Return the updated line number.
 */
    static linenr_T
apply_prefixes(slang, word, round, flags, startlnum)
    slang_T	*slang;
    char_u	*word;	    /* case-folded word */
    int		round;
    int		flags;	    /* flags with prefix ID */
    linenr_T	startlnum;
{
    idx_T	arridx[MAXWLEN];
    int		curi[MAXWLEN];
    char_u	prefix[MAXWLEN];
    int		c;
    char_u	*byts;
    idx_T	*idxs;
    linenr_T	lnum = startlnum;
    int		depth;
    int		n;
    int		len;
    int		prefid = (unsigned)flags >> 16;
    int		i;

    byts = slang->sl_pbyts;
    idxs = slang->sl_pidxs;
    if (byts != NULL)		/* array not is empty */
    {
	/*
	 * Loop over all prefixes, building them byte-by-byte in prefix[].
	 * When at the end of a prefix check that it supports "prefid".
	 */
	depth = 0;
	arridx[0] = 0;
	curi[0] = 1;
	while (depth >= 0 && !got_int)
	{
	    len = arridx[depth];
	    if (curi[depth] > byts[len])
	    {
		/* Done all bytes at this node, go up one level. */
		--depth;
		line_breakcheck();
	    }
	    else
	    {
		/* Do one more byte at this node. */
		n = len + curi[depth];
		++curi[depth];
		c = byts[n];
		if (c == 0)
		{
		    /* End of prefix, find out how many IDs there are. */
		    for (i = 1; i < len; ++i)
			if (byts[n + i] != 0)
			    break;
		    curi[depth] += i - 1;

		    i = valid_word_prefix(i, n, prefid, word, slang);
		    if (i != 0)
		    {
			vim_strncpy(prefix + depth, word, MAXWLEN - depth);
			dump_word(prefix, round,
				(i & WF_RAREPFX) ? (flags | WF_RARE)
							     : flags, lnum++);
		    }
		}
		else
		{
		    /* Normal char, go one level deeper. */
		    prefix[depth++] = c;
		    arridx[depth] = idxs[n];
		    curi[depth] = 1;
		}
	    }
	}
    }

    return lnum;
}

#endif  /* FEAT_SYN_HL */
