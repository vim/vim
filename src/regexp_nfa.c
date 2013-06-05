/* vi:set ts=8 sts=4 sw=4:
 *
 * NFA regular expression implementation.
 *
 * This file is included in "regexp.c".
 */

/*
 * Logging of NFA engine.
 *
 * The NFA engine can write four log files:
 * - Error log: Contains NFA engine's fatal errors.
 * - Dump log: Contains compiled NFA state machine's information.
 * - Run log: Contains information of matching procedure.
 * - Debug log: Contains detailed information of matching procedure. Can be
 *   disabled by undefining NFA_REGEXP_DEBUG_LOG.
 * The first one can also be used without debug mode.
 * The last three are enabled when compiled as debug mode and individually
 * disabled by commenting them out.
 * The log files can get quite big!
 * Do disable all of this when compiling Vim for debugging, undefine DEBUG in
 * regexp.c
 */
#ifdef DEBUG
# define NFA_REGEXP_ERROR_LOG	"nfa_regexp_error.log"
# define ENABLE_LOG
# define NFA_REGEXP_DUMP_LOG	"nfa_regexp_dump.log"
# define NFA_REGEXP_RUN_LOG	"nfa_regexp_run.log"
# define NFA_REGEXP_DEBUG_LOG	"nfa_regexp_debug.log"
#endif

enum
{
    NFA_SPLIT = -1024,
    NFA_MATCH,
    NFA_SKIP_CHAR,		    /* matches a 0-length char */
    NFA_END_NEG_RANGE,		    /* Used when expanding [^ab] */

    NFA_CONCAT,
    NFA_OR,
    NFA_STAR,			    /* greedy * */
    NFA_STAR_NONGREEDY,		    /* non-greedy * */
    NFA_QUEST,			    /* greedy \? */
    NFA_QUEST_NONGREEDY,	    /* non-greedy \? */
    NFA_NOT,			    /* used for [^ab] negated char ranges */

    NFA_BOL,			    /* ^    Begin line */
    NFA_EOL,			    /* $    End line */
    NFA_BOW,			    /* \<   Begin word */
    NFA_EOW,			    /* \>   End word */
    NFA_BOF,			    /* \%^  Begin file */
    NFA_EOF,			    /* \%$  End file */
    NFA_NEWL,
    NFA_ZSTART,			    /* Used for \zs */
    NFA_ZEND,			    /* Used for \ze */
    NFA_NOPEN,			    /* Start of subexpression marked with \%( */
    NFA_NCLOSE,			    /* End of subexpr. marked with \%( ... \) */
    NFA_START_INVISIBLE,
    NFA_START_INVISIBLE_BEFORE,
    NFA_START_PATTERN,
    NFA_END_INVISIBLE,
    NFA_END_PATTERN,
    NFA_COMPOSING,		    /* Next nodes in NFA are part of the
				       composing multibyte char */
    NFA_END_COMPOSING,		    /* End of a composing char in the NFA */
    NFA_OPT_CHARS,		    /* \%[abc] */

    /* The following are used only in the postfix form, not in the NFA */
    NFA_PREV_ATOM_NO_WIDTH,	    /* Used for \@= */
    NFA_PREV_ATOM_NO_WIDTH_NEG,	    /* Used for \@! */
    NFA_PREV_ATOM_JUST_BEFORE,	    /* Used for \@<= */
    NFA_PREV_ATOM_JUST_BEFORE_NEG,  /* Used for \@<! */
    NFA_PREV_ATOM_LIKE_PATTERN,	    /* Used for \@> */

    NFA_BACKREF1,		    /* \1 */
    NFA_BACKREF2,		    /* \2 */
    NFA_BACKREF3,		    /* \3 */
    NFA_BACKREF4,		    /* \4 */
    NFA_BACKREF5,		    /* \5 */
    NFA_BACKREF6,		    /* \6 */
    NFA_BACKREF7,		    /* \7 */
    NFA_BACKREF8,		    /* \8 */
    NFA_BACKREF9,		    /* \9 */
#ifdef FEAT_SYN_HL
    NFA_ZREF1,			    /* \z1 */
    NFA_ZREF2,			    /* \z2 */
    NFA_ZREF3,			    /* \z3 */
    NFA_ZREF4,			    /* \z4 */
    NFA_ZREF5,			    /* \z5 */
    NFA_ZREF6,			    /* \z6 */
    NFA_ZREF7,			    /* \z7 */
    NFA_ZREF8,			    /* \z8 */
    NFA_ZREF9,			    /* \z9 */
#endif
    NFA_SKIP,			    /* Skip characters */

    NFA_MOPEN,
    NFA_MOPEN1,
    NFA_MOPEN2,
    NFA_MOPEN3,
    NFA_MOPEN4,
    NFA_MOPEN5,
    NFA_MOPEN6,
    NFA_MOPEN7,
    NFA_MOPEN8,
    NFA_MOPEN9,

    NFA_MCLOSE,
    NFA_MCLOSE1,
    NFA_MCLOSE2,
    NFA_MCLOSE3,
    NFA_MCLOSE4,
    NFA_MCLOSE5,
    NFA_MCLOSE6,
    NFA_MCLOSE7,
    NFA_MCLOSE8,
    NFA_MCLOSE9,

#ifdef FEAT_SYN_HL
    NFA_ZOPEN,
    NFA_ZOPEN1,
    NFA_ZOPEN2,
    NFA_ZOPEN3,
    NFA_ZOPEN4,
    NFA_ZOPEN5,
    NFA_ZOPEN6,
    NFA_ZOPEN7,
    NFA_ZOPEN8,
    NFA_ZOPEN9,

    NFA_ZCLOSE,
    NFA_ZCLOSE1,
    NFA_ZCLOSE2,
    NFA_ZCLOSE3,
    NFA_ZCLOSE4,
    NFA_ZCLOSE5,
    NFA_ZCLOSE6,
    NFA_ZCLOSE7,
    NFA_ZCLOSE8,
    NFA_ZCLOSE9,
#endif

    /* NFA_FIRST_NL */
    NFA_ANY,		/*	Match any one character. */
    NFA_ANYOF,		/*	Match any character in this string. */
    NFA_ANYBUT,		/*	Match any character not in this string. */
    NFA_IDENT,		/*	Match identifier char */
    NFA_SIDENT,		/*	Match identifier char but no digit */
    NFA_KWORD,		/*	Match keyword char */
    NFA_SKWORD,		/*	Match word char but no digit */
    NFA_FNAME,		/*	Match file name char */
    NFA_SFNAME,		/*	Match file name char but no digit */
    NFA_PRINT,		/*	Match printable char */
    NFA_SPRINT,		/*	Match printable char but no digit */
    NFA_WHITE,		/*	Match whitespace char */
    NFA_NWHITE,		/*	Match non-whitespace char */
    NFA_DIGIT,		/*	Match digit char */
    NFA_NDIGIT,		/*	Match non-digit char */
    NFA_HEX,		/*	Match hex char */
    NFA_NHEX,		/*	Match non-hex char */
    NFA_OCTAL,		/*	Match octal char */
    NFA_NOCTAL,		/*	Match non-octal char */
    NFA_WORD,		/*	Match word char */
    NFA_NWORD,		/*	Match non-word char */
    NFA_HEAD,		/*	Match head char */
    NFA_NHEAD,		/*	Match non-head char */
    NFA_ALPHA,		/*	Match alpha char */
    NFA_NALPHA,		/*	Match non-alpha char */
    NFA_LOWER,		/*	Match lowercase char */
    NFA_NLOWER,		/*	Match non-lowercase char */
    NFA_UPPER,		/*	Match uppercase char */
    NFA_NUPPER,		/*	Match non-uppercase char */

    NFA_CURSOR,		/*	Match cursor pos */
    NFA_LNUM,		/*	Match line number */
    NFA_LNUM_GT,	/*	Match > line number */
    NFA_LNUM_LT,	/*	Match < line number */
    NFA_COL,		/*	Match cursor column */
    NFA_COL_GT,		/*	Match > cursor column */
    NFA_COL_LT,		/*	Match < cursor column */
    NFA_VCOL,		/*	Match cursor virtual column */
    NFA_VCOL_GT,	/*	Match > cursor virtual column */
    NFA_VCOL_LT,	/*	Match < cursor virtual column */
    NFA_MARK,		/*	Match mark */
    NFA_MARK_GT,	/*	Match > mark */
    NFA_MARK_LT,	/*	Match < mark */
    NFA_VISUAL,		/*	Match Visual area */

    NFA_FIRST_NL = NFA_ANY + ADD_NL,
    NFA_LAST_NL = NFA_NUPPER + ADD_NL,

    /* Character classes [:alnum:] etc */
    NFA_CLASS_ALNUM,
    NFA_CLASS_ALPHA,
    NFA_CLASS_BLANK,
    NFA_CLASS_CNTRL,
    NFA_CLASS_DIGIT,
    NFA_CLASS_GRAPH,
    NFA_CLASS_LOWER,
    NFA_CLASS_PRINT,
    NFA_CLASS_PUNCT,
    NFA_CLASS_SPACE,
    NFA_CLASS_UPPER,
    NFA_CLASS_XDIGIT,
    NFA_CLASS_TAB,
    NFA_CLASS_RETURN,
    NFA_CLASS_BACKSPACE,
    NFA_CLASS_ESCAPE
};

/* Keep in sync with classchars. */
static int nfa_classcodes[] = {
    NFA_ANY, NFA_IDENT, NFA_SIDENT, NFA_KWORD,NFA_SKWORD,
    NFA_FNAME, NFA_SFNAME, NFA_PRINT, NFA_SPRINT,
    NFA_WHITE, NFA_NWHITE, NFA_DIGIT, NFA_NDIGIT,
    NFA_HEX, NFA_NHEX, NFA_OCTAL, NFA_NOCTAL,
    NFA_WORD, NFA_NWORD, NFA_HEAD, NFA_NHEAD,
    NFA_ALPHA, NFA_NALPHA, NFA_LOWER, NFA_NLOWER,
    NFA_UPPER, NFA_NUPPER
};

static char_u e_misplaced[] = N_("E866: (NFA regexp) Misplaced %c");

/* NFA regexp \ze operator encountered. */
static int nfa_has_zend;

/* NFA regexp \1 .. \9 encountered. */
static int nfa_has_backref;

#ifdef FEAT_SYN_HL
/* NFA regexp has \z( ), set zsubexpr. */
static int nfa_has_zsubexpr;
#endif

/* Number of sub expressions actually being used during execution. 1 if only
 * the whole match (subexpr 0) is used. */
static int nfa_nsubexpr;

static int *post_start;  /* holds the postfix form of r.e. */
static int *post_end;
static int *post_ptr;

static int nstate;	/* Number of states in the NFA. Also used when
			 * executing. */
static int istate;	/* Index in the state vector, used in alloc_state() */

/* If not NULL match must end at this position */
static save_se_T *nfa_endp = NULL;

/* listid is global, so that it increases on recursive calls to
 * nfa_regmatch(), which means we don't have to clear the lastlist field of
 * all the states. */
static int nfa_listid;
static int nfa_alt_listid;

/* 0 for first call to nfa_regmatch(), 1 for recursive call. */
static int nfa_ll_index = 0;

static int nfa_regcomp_start __ARGS((char_u*expr, int re_flags));
static int nfa_recognize_char_class __ARGS((char_u *start, char_u *end, int extra_newl));
static int nfa_emit_equi_class __ARGS((int c, int neg));
static int nfa_regatom __ARGS((void));
static int nfa_regpiece __ARGS((void));
static int nfa_regconcat __ARGS((void));
static int nfa_regbranch __ARGS((void));
static int nfa_reg __ARGS((int paren));
#ifdef DEBUG
static void nfa_set_code __ARGS((int c));
static void nfa_postfix_dump __ARGS((char_u *expr, int retval));
static void nfa_print_state __ARGS((FILE *debugf, nfa_state_T *state));
static void nfa_print_state2 __ARGS((FILE *debugf, nfa_state_T *state, garray_T *indent));
static void nfa_dump __ARGS((nfa_regprog_T *prog));
#endif
static int *re2post __ARGS((void));
static nfa_state_T *alloc_state __ARGS((int c, nfa_state_T *out, nfa_state_T *out1));
static nfa_state_T *post2nfa __ARGS((int *postfix, int *end, int nfa_calc_size));
static int check_char_class __ARGS((int class, int c));
static void st_error __ARGS((int *postfix, int *end, int *p));
static void nfa_save_listids __ARGS((nfa_regprog_T *prog, int *list));
static void nfa_restore_listids __ARGS((nfa_regprog_T *prog, int *list));
static int nfa_re_num_cmp __ARGS((long_u val, int op, long_u pos));
static long nfa_regtry __ARGS((nfa_regprog_T *prog, colnr_T col));
static long nfa_regexec_both __ARGS((char_u *line, colnr_T col));
static regprog_T *nfa_regcomp __ARGS((char_u *expr, int re_flags));
static int nfa_regexec __ARGS((regmatch_T *rmp, char_u *line, colnr_T col));
static long nfa_regexec_multi __ARGS((regmmatch_T *rmp, win_T *win, buf_T *buf, linenr_T lnum, colnr_T col, proftime_T *tm));

/* helper functions used when doing re2post() ... regatom() parsing */
#define EMIT(c)	do {				\
		    if (post_ptr >= post_end && realloc_post_list() == FAIL) \
			return FAIL;		\
		    *post_ptr++ = c;		\
		} while (0)

/*
 * Initialize internal variables before NFA compilation.
 * Return OK on success, FAIL otherwise.
 */
    static int
nfa_regcomp_start(expr, re_flags)
    char_u	*expr;
    int		re_flags;	    /* see vim_regcomp() */
{
    size_t	postfix_size;
    int		nstate_max;

    nstate = 0;
    istate = 0;
    /* A reasonable estimation for maximum size */
    nstate_max = (int)(STRLEN(expr) + 1) * 25;

    /* Some items blow up in size, such as [A-z].  Add more space for that.
     * When it is still not enough realloc_post_list() will be used. */
    nstate_max += 1000;

    /* Size for postfix representation of expr. */
    postfix_size = sizeof(int) * nstate_max;

    post_start = (int *)lalloc(postfix_size, TRUE);
    if (post_start == NULL)
	return FAIL;
    vim_memset(post_start, 0, postfix_size);
    post_ptr = post_start;
    post_end = post_start + nstate_max;
    nfa_has_zend = FALSE;
    nfa_has_backref = FALSE;

    /* shared with BT engine */
    regcomp_start(expr, re_flags);

    return OK;
}

/*
 * Allocate more space for post_start.  Called when
 * running above the estimated number of states.
 */
    static int
realloc_post_list()
{
    int   nstate_max = (int)(post_end - post_start);
    int   new_max = nstate_max + 1000;
    int   *new_start;
    int	  *old_start;

    new_start = (int *)lalloc(new_max * sizeof(int), TRUE);
    if (new_start == NULL)
	return FAIL;
    mch_memmove(new_start, post_start, nstate_max * sizeof(int));
    vim_memset(new_start + nstate_max, 0, 1000 * sizeof(int));
    old_start = post_start;
    post_start = new_start;
    post_ptr = new_start + (post_ptr - old_start);
    post_end = post_start + new_max;
    vim_free(old_start);
    return OK;
}

/*
 * Search between "start" and "end" and try to recognize a
 * character class in expanded form. For example [0-9].
 * On success, return the id the character class to be emitted.
 * On failure, return 0 (=FAIL)
 * Start points to the first char of the range, while end should point
 * to the closing brace.
 */
    static int
nfa_recognize_char_class(start, end, extra_newl)
    char_u  *start;
    char_u  *end;
    int	    extra_newl;
{
#   define CLASS_not		0x80
#   define CLASS_af		0x40
#   define CLASS_AF		0x20
#   define CLASS_az		0x10
#   define CLASS_AZ		0x08
#   define CLASS_o7		0x04
#   define CLASS_o9		0x02
#   define CLASS_underscore	0x01

    int		newl = FALSE;
    char_u	*p;
    int		config = 0;

    if (extra_newl == TRUE)
	newl = TRUE;

    if (*end != ']')
	return FAIL;
    p = start;
    if (*p == '^')
    {
	config |= CLASS_not;
	p++;
    }

    while (p < end)
    {
	if (p + 2 < end && *(p + 1) == '-')
	{
	    switch (*p)
	    {
		case '0':
		    if (*(p + 2) == '9')
		    {
			config |= CLASS_o9;
			break;
		    }
		    else
		    if (*(p + 2) == '7')
		    {
			config |= CLASS_o7;
			break;
		    }
		case 'a':
		    if (*(p + 2) == 'z')
		    {
			config |= CLASS_az;
			break;
		    }
		    else
		    if (*(p + 2) == 'f')
		    {
			config |= CLASS_af;
			break;
		    }
		case 'A':
		    if (*(p + 2) == 'Z')
		    {
			config |= CLASS_AZ;
			break;
		    }
		    else
		    if (*(p + 2) == 'F')
		    {
			config |= CLASS_AF;
			break;
		    }
		/* FALLTHROUGH */
		default:
		    return FAIL;
	    }
	    p += 3;
	}
	else if (p + 1 < end && *p == '\\' && *(p + 1) == 'n')
	{
	    newl = TRUE;
	    p += 2;
	}
	else if (*p == '_')
	{
	    config |= CLASS_underscore;
	    p ++;
	}
	else if (*p == '\n')
	{
	    newl = TRUE;
	    p ++;
	}
	else
	    return FAIL;
    } /* while (p < end) */

    if (p != end)
	return FAIL;

    if (newl == TRUE)
	extra_newl = ADD_NL;

    switch (config)
    {
	case CLASS_o9:
	    return extra_newl + NFA_DIGIT;
	case CLASS_not |  CLASS_o9:
	    return extra_newl + NFA_NDIGIT;
	case CLASS_af | CLASS_AF | CLASS_o9:
	    return extra_newl + NFA_HEX;
	case CLASS_not | CLASS_af | CLASS_AF | CLASS_o9:
	    return extra_newl + NFA_NHEX;
	case CLASS_o7:
	    return extra_newl + NFA_OCTAL;
	case CLASS_not | CLASS_o7:
	    return extra_newl + NFA_NOCTAL;
	case CLASS_az | CLASS_AZ | CLASS_o9 | CLASS_underscore:
	    return extra_newl + NFA_WORD;
	case CLASS_not | CLASS_az | CLASS_AZ | CLASS_o9 | CLASS_underscore:
	    return extra_newl + NFA_NWORD;
	case CLASS_az | CLASS_AZ | CLASS_underscore:
	    return extra_newl + NFA_HEAD;
	case CLASS_not | CLASS_az | CLASS_AZ | CLASS_underscore:
	    return extra_newl + NFA_NHEAD;
	case CLASS_az | CLASS_AZ:
	    return extra_newl + NFA_ALPHA;
	case CLASS_not | CLASS_az | CLASS_AZ:
	    return extra_newl + NFA_NALPHA;
	case CLASS_az:
	   return extra_newl + NFA_LOWER;
	case CLASS_not | CLASS_az:
	    return extra_newl + NFA_NLOWER;
	case CLASS_AZ:
	    return extra_newl + NFA_UPPER;
	case CLASS_not | CLASS_AZ:
	    return extra_newl + NFA_NUPPER;
    }
    return FAIL;
}

/*
 * Produce the bytes for equivalence class "c".
 * Currently only handles latin1, latin9 and utf-8.
 * Emits bytes in postfix notation: 'a,b,NFA_OR,c,NFA_OR' is
 * equivalent to 'a OR b OR c'
 *
 * NOTE! When changing this function, also update reg_equi_class()
 */
    static int
nfa_emit_equi_class(c, neg)
    int	    c;
    int	    neg;
{
    int	first = TRUE;
    int	glue = neg == TRUE ? NFA_CONCAT : NFA_OR;
#define EMIT2(c)		\
	EMIT(c);		\
	if (neg == TRUE) {	\
	    EMIT(NFA_NOT);	\
	}			\
	if (first == FALSE)	\
	    EMIT(glue);		\
	else			\
	    first = FALSE;	\

#ifdef FEAT_MBYTE
    if (enc_utf8 || STRCMP(p_enc, "latin1") == 0
					 || STRCMP(p_enc, "iso-8859-15") == 0)
#endif
    {
	switch (c)
	{
	    case 'A': case '\300': case '\301': case '\302':
	    case '\303': case '\304': case '\305':
		    EMIT2('A');	    EMIT2('\300');  EMIT2('\301');
		    EMIT2('\302');  EMIT2('\303');  EMIT2('\304');
		    EMIT2('\305');
		    return OK;

	    case 'C': case '\307':
		    EMIT2('C');	    EMIT2('\307');
		    return OK;

	    case 'E': case '\310': case '\311': case '\312': case '\313':
		    EMIT2('E');	    EMIT2('\310');  EMIT2('\311');
		    EMIT2('\312');  EMIT2('\313');
		    return OK;

	    case 'I': case '\314': case '\315': case '\316': case '\317':
		    EMIT2('I');	    EMIT2('\314');  EMIT2('\315');
		    EMIT2('\316');  EMIT2('\317');
		    return OK;

	    case 'N': case '\321':
		    EMIT2('N');	    EMIT2('\321');
		    return OK;

	    case 'O': case '\322': case '\323': case '\324': case '\325':
	    case '\326':
		    EMIT2('O');	    EMIT2('\322');  EMIT2('\323');
		    EMIT2('\324');  EMIT2('\325');  EMIT2('\326');
		    return OK;

	    case 'U': case '\331': case '\332': case '\333': case '\334':
		    EMIT2('U');	    EMIT2('\331');  EMIT2('\332');
		    EMIT2('\333');  EMIT2('\334');
		    return OK;

	    case 'Y': case '\335':
		    EMIT2('Y');	    EMIT2('\335');
		    return OK;

	    case 'a': case '\340': case '\341': case '\342':
	    case '\343': case '\344': case '\345':
		    EMIT2('a');	    EMIT2('\340');  EMIT2('\341');
		    EMIT2('\342');  EMIT2('\343');  EMIT2('\344');
		    EMIT2('\345');
		    return OK;

	    case 'c': case '\347':
		    EMIT2('c');	    EMIT2('\347');
		    return OK;

	    case 'e': case '\350': case '\351': case '\352': case '\353':
		    EMIT2('e');	    EMIT2('\350');  EMIT2('\351');
		    EMIT2('\352');  EMIT2('\353');
		    return OK;

	    case 'i': case '\354': case '\355': case '\356': case '\357':
		    EMIT2('i');	    EMIT2('\354');  EMIT2('\355');
		    EMIT2('\356');  EMIT2('\357');
		    return OK;

	    case 'n': case '\361':
		    EMIT2('n');	    EMIT2('\361');
		    return OK;

	    case 'o': case '\362': case '\363': case '\364': case '\365':
	    case '\366':
		    EMIT2('o');	    EMIT2('\362');  EMIT2('\363');
		    EMIT2('\364');  EMIT2('\365');  EMIT2('\366');
		    return OK;

	    case 'u': case '\371': case '\372': case '\373': case '\374':
		    EMIT2('u');	    EMIT2('\371');  EMIT2('\372');
		    EMIT2('\373');  EMIT2('\374');
		    return OK;

	    case 'y': case '\375': case '\377':
		    EMIT2('y');	    EMIT2('\375');  EMIT2('\377');
		    return OK;

	    default:
		    return FAIL;
	}
    }

    EMIT(c);
    return OK;
#undef EMIT2
}

/*
 * Code to parse regular expression.
 *
 * We try to reuse parsing functions in regexp.c to
 * minimize surprise and keep the syntax consistent.
 */

/*
 * Parse the lowest level.
 *
 * An atom can be one of a long list of items.  Many atoms match one character
 * in the text.  It is often an ordinary character or a character class.
 * Braces can be used to make a pattern into an atom.  The "\z(\)" construct
 * is only for syntax highlighting.
 *
 * atom    ::=     ordinary-atom
 *     or  \( pattern \)
 *     or  \%( pattern \)
 *     or  \z( pattern \)
 */
    static int
nfa_regatom()
{
    int		c;
    int		charclass;
    int		equiclass;
    int		collclass;
    int		got_coll_char;
    char_u	*p;
    char_u	*endp;
#ifdef FEAT_MBYTE
    char_u	*old_regparse = regparse;
#endif
    int		extra = 0;
    int		first;
    int		emit_range;
    int		negated;
    int		result;
    int		startc = -1;
    int		endc = -1;
    int		oldstartc = -1;
    int		glue;		/* ID that will "glue" nodes together */

    c = getchr();
    switch (c)
    {
	case NUL:
	    EMSG_RET_FAIL(_("E865: (NFA) Regexp end encountered prematurely"));

	case Magic('^'):
	    EMIT(NFA_BOL);
	    break;

	case Magic('$'):
	    EMIT(NFA_EOL);
#if defined(FEAT_SYN_HL) || defined(PROTO)
	    had_eol = TRUE;
#endif
	    break;

	case Magic('<'):
	    EMIT(NFA_BOW);
	    break;

	case Magic('>'):
	    EMIT(NFA_EOW);
	    break;

	case Magic('_'):
	    c = no_Magic(getchr());
	    if (c == '^')	/* "\_^" is start-of-line */
	    {
		EMIT(NFA_BOL);
		break;
	    }
	    if (c == '$')	/* "\_$" is end-of-line */
	    {
		EMIT(NFA_EOL);
#if defined(FEAT_SYN_HL) || defined(PROTO)
		had_eol = TRUE;
#endif
		break;
	    }

	    extra = ADD_NL;

	    /* "\_[" is collection plus newline */
	    if (c == '[')
		goto collection;

	/* "\_x" is character class plus newline */
	/*FALLTHROUGH*/

	/*
	 * Character classes.
	 */
	case Magic('.'):
	case Magic('i'):
	case Magic('I'):
	case Magic('k'):
	case Magic('K'):
	case Magic('f'):
	case Magic('F'):
	case Magic('p'):
	case Magic('P'):
	case Magic('s'):
	case Magic('S'):
	case Magic('d'):
	case Magic('D'):
	case Magic('x'):
	case Magic('X'):
	case Magic('o'):
	case Magic('O'):
	case Magic('w'):
	case Magic('W'):
	case Magic('h'):
	case Magic('H'):
	case Magic('a'):
	case Magic('A'):
	case Magic('l'):
	case Magic('L'):
	case Magic('u'):
	case Magic('U'):
	    p = vim_strchr(classchars, no_Magic(c));
	    if (p == NULL)
	    {
		EMSGN("INTERNAL: Unknown character class char: %ld", c);
		return FAIL;
	    }
#ifdef FEAT_MBYTE
	    /* When '.' is followed by a composing char ignore the dot, so that
	     * the composing char is matched here. */
	    if (enc_utf8 && c == Magic('.') && utf_iscomposing(peekchr()))
	    {
		old_regparse = regparse;
		c = getchr();
		goto nfa_do_multibyte;
	    }
#endif
	    EMIT(nfa_classcodes[p - classchars]);
	    if (extra == ADD_NL)
	    {
		EMIT(NFA_NEWL);
		EMIT(NFA_OR);
		regflags |= RF_HASNL;
	    }
	    break;

	case Magic('n'):
	    if (reg_string)
	    /* In a string "\n" matches a newline character. */
	    EMIT(NL);
	    else
	    {
		/* In buffer text "\n" matches the end of a line. */
		EMIT(NFA_NEWL);
		regflags |= RF_HASNL;
	    }
	    break;

	case Magic('('):
	    if (nfa_reg(REG_PAREN) == FAIL)
		return FAIL;	    /* cascaded error */
	    break;

	case Magic('|'):
	case Magic('&'):
	case Magic(')'):
	    EMSGN(_(e_misplaced), no_Magic(c));
	    return FAIL;

	case Magic('='):
	case Magic('?'):
	case Magic('+'):
	case Magic('@'):
	case Magic('*'):
	case Magic('{'):
	    /* these should follow an atom, not form an atom */
	    EMSGN(_(e_misplaced), no_Magic(c));
	    return FAIL;

	case Magic('~'):
	    {
		char_u	    *lp;

		/* Previous substitute pattern.
		 * Generated as "\%(pattern\)". */
		if (reg_prev_sub == NULL)
		{
		    EMSG(_(e_nopresub));
		    return FAIL;
		}
		for (lp = reg_prev_sub; *lp != NUL; mb_cptr_adv(lp))
		{
		    EMIT(PTR2CHAR(lp));
		    if (lp != reg_prev_sub)
			EMIT(NFA_CONCAT);
		}
		EMIT(NFA_NOPEN);
		break;
	    }

	case Magic('1'):
	case Magic('2'):
	case Magic('3'):
	case Magic('4'):
	case Magic('5'):
	case Magic('6'):
	case Magic('7'):
	case Magic('8'):
	case Magic('9'):
	    EMIT(NFA_BACKREF1 + (no_Magic(c) - '1'));
	    nfa_has_backref = TRUE;
	    break;

	case Magic('z'):
	    c = no_Magic(getchr());
	    switch (c)
	    {
		case 's':
		    EMIT(NFA_ZSTART);
		    break;
		case 'e':
		    EMIT(NFA_ZEND);
		    nfa_has_zend = TRUE;
		    break;
#ifdef FEAT_SYN_HL
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		    /* \z1...\z9 */
		    if (reg_do_extmatch != REX_USE)
			EMSG_RET_FAIL(_(e_z1_not_allowed));
		    EMIT(NFA_ZREF1 + (no_Magic(c) - '1'));
		    /* No need to set nfa_has_backref, the sub-matches don't
		     * change when \z1 .. \z9 matches or not. */
		    re_has_z = REX_USE;
		    break;
		case '(':
		    /* \z(  */
		    if (reg_do_extmatch != REX_SET)
			EMSG_RET_FAIL(_(e_z_not_allowed));
		    if (nfa_reg(REG_ZPAREN) == FAIL)
			return FAIL;	    /* cascaded error */
		    re_has_z = REX_SET;
		    break;
#endif
		default:
		    EMSGN(_("E867: (NFA) Unknown operator '\\z%c'"),
								 no_Magic(c));
		    return FAIL;
	    }
	    break;

	case Magic('%'):
	    c = no_Magic(getchr());
	    switch (c)
	    {
		/* () without a back reference */
		case '(':
		    if (nfa_reg(REG_NPAREN) == FAIL)
			return FAIL;
		    EMIT(NFA_NOPEN);
		    break;

		case 'd':   /* %d123 decimal */
		case 'o':   /* %o123 octal */
		case 'x':   /* %xab hex 2 */
		case 'u':   /* %uabcd hex 4 */
		case 'U':   /* %U1234abcd hex 8 */
		    {
			int nr;

			switch (c)
			{
			    case 'd': nr = getdecchrs(); break;
			    case 'o': nr = getoctchrs(); break;
			    case 'x': nr = gethexchrs(2); break;
			    case 'u': nr = gethexchrs(4); break;
			    case 'U': nr = gethexchrs(8); break;
			    default:  nr = -1; break;
			}

			if (nr < 0)
			    EMSG2_RET_FAIL(
			       _("E678: Invalid character after %s%%[dxouU]"),
				    reg_magic == MAGIC_ALL);
			/* TODO: what if a composing character follows? */
			EMIT(nr);
		    }
		    break;

		/* Catch \%^ and \%$ regardless of where they appear in the
		 * pattern -- regardless of whether or not it makes sense. */
		case '^':
		    EMIT(NFA_BOF);
		    break;

		case '$':
		    EMIT(NFA_EOF);
		    break;

		case '#':
		    EMIT(NFA_CURSOR);
		    break;

		case 'V':
		    EMIT(NFA_VISUAL);
		    break;

		case '[':
		    {
			int	    n;

			/* \%[abc] */
			for (n = 0; (c = getchr()) != ']'; ++n)
			{
			    if (c == NUL)
				EMSG2_RET_FAIL(_(e_missing_sb),
						      reg_magic == MAGIC_ALL);
			    EMIT(c);
			}
			if (n == 0)
			    EMSG2_RET_FAIL(_(e_empty_sb),
						      reg_magic == MAGIC_ALL);
			EMIT(NFA_OPT_CHARS);
			EMIT(n);
			break;
		    }

		default:
		    {
			int	n = 0;
			int	cmp = c;

			if (c == '<' || c == '>')
			    c = getchr();
			while (VIM_ISDIGIT(c))
			{
			    n = n * 10 + (c - '0');
			    c = getchr();
			}
			if (c == 'l' || c == 'c' || c == 'v')
			{
			    if (c == 'l')
				/* \%{n}l  \%{n}<l  \%{n}>l  */
				EMIT(cmp == '<' ? NFA_LNUM_LT :
				     cmp == '>' ? NFA_LNUM_GT : NFA_LNUM);
			    else if (c == 'c')
				/* \%{n}c  \%{n}<c  \%{n}>c  */
				EMIT(cmp == '<' ? NFA_COL_LT :
				     cmp == '>' ? NFA_COL_GT : NFA_COL);
			    else
				/* \%{n}v  \%{n}<v  \%{n}>v  */
				EMIT(cmp == '<' ? NFA_VCOL_LT :
				     cmp == '>' ? NFA_VCOL_GT : NFA_VCOL);
			    EMIT(n);
			    break;
			}
			else if (c == '\'' && n == 0)
			{
			    /* \%'m  \%<'m  \%>'m  */
			    EMIT(cmp == '<' ? NFA_MARK_LT :
				 cmp == '>' ? NFA_MARK_GT : NFA_MARK);
			    EMIT(getchr());
			    break;
			}
		    }
		    EMSGN(_("E867: (NFA) Unknown operator '\\%%%c'"),
								 no_Magic(c));
		    return FAIL;
	    }
	    break;

	case Magic('['):
collection:
	    /*
	     * Glue is emitted between several atoms from the [].
	     * It is either NFA_OR, or NFA_CONCAT.
	     *
	     * [abc] expands to 'a b NFA_OR c NFA_OR' (in postfix notation)
	     * [^abc] expands to 'a NFA_NOT b NFA_NOT NFA_CONCAT c NFA_NOT
	     *		NFA_CONCAT NFA_END_NEG_RANGE NFA_CONCAT' (in postfix
	     *		notation)
	     *
	     */


/* Emit negation atoms, if needed.
 * The CONCAT below merges the NOT with the previous node. */
#define TRY_NEG()		    \
	    if (negated == TRUE)    \
	    {			    \
		EMIT(NFA_NOT);	    \
	    }

/* Emit glue between important nodes : CONCAT or OR. */
#define EMIT_GLUE()		    \
	    if (first == FALSE)	    \
		EMIT(glue);	    \
	    else		    \
		first = FALSE;

	    p = regparse;
	    endp = skip_anyof(p);
	    if (*endp == ']')
	    {
		/*
		 * Try to reverse engineer character classes. For example,
		 * recognize that [0-9] stands for  \d and [A-Za-z_] with \h,
		 * and perform the necessary substitutions in the NFA.
		 */
		result = nfa_recognize_char_class(regparse, endp,
							    extra == ADD_NL);
		if (result != FAIL)
		{
		    if (result >= NFA_DIGIT && result <= NFA_NUPPER)
			EMIT(result);
		    else	/* must be char class + newline */
		    {
			EMIT(result - ADD_NL);
			EMIT(NFA_NEWL);
			EMIT(NFA_OR);
		    }
		    regparse = endp;
		    mb_ptr_adv(regparse);
		    return OK;
		}
		/*
		 * Failed to recognize a character class. Use the simple
		 * version that turns [abc] into 'a' OR 'b' OR 'c'
		 */
		startc = endc = oldstartc = -1;
		first = TRUE;	    /* Emitting first atom in this sequence? */
		negated = FALSE;
		glue = NFA_OR;
		if (*regparse == '^')			/* negated range */
		{
		    negated = TRUE;
		    glue = NFA_CONCAT;
		    mb_ptr_adv(regparse);
		}
		if (*regparse == '-')
		{
		    startc = '-';
		    EMIT(startc);
		    TRY_NEG();
		    EMIT_GLUE();
		    mb_ptr_adv(regparse);
		}
		/* Emit the OR branches for each character in the [] */
		emit_range = FALSE;
		while (regparse < endp)
		{
		    oldstartc = startc;
		    startc = -1;
		    got_coll_char = FALSE;
		    if (*regparse == '[')
		    {
			/* Check for [: :], [= =], [. .] */
			equiclass = collclass = 0;
			charclass = get_char_class(&regparse);
			if (charclass == CLASS_NONE)
			{
			    equiclass = get_equi_class(&regparse);
			    if (equiclass == 0)
				collclass = get_coll_element(&regparse);
			}

			/* Character class like [:alpha:]  */
			if (charclass != CLASS_NONE)
			{
			    switch (charclass)
			    {
				case CLASS_ALNUM:
				    EMIT(NFA_CLASS_ALNUM);
				    break;
				case CLASS_ALPHA:
				    EMIT(NFA_CLASS_ALPHA);
				    break;
				case CLASS_BLANK:
				    EMIT(NFA_CLASS_BLANK);
				    break;
				case CLASS_CNTRL:
				    EMIT(NFA_CLASS_CNTRL);
				    break;
				case CLASS_DIGIT:
				    EMIT(NFA_CLASS_DIGIT);
				    break;
				case CLASS_GRAPH:
				    EMIT(NFA_CLASS_GRAPH);
				    break;
				case CLASS_LOWER:
				    EMIT(NFA_CLASS_LOWER);
				    break;
				case CLASS_PRINT:
				    EMIT(NFA_CLASS_PRINT);
				    break;
				case CLASS_PUNCT:
				    EMIT(NFA_CLASS_PUNCT);
				    break;
				case CLASS_SPACE:
				    EMIT(NFA_CLASS_SPACE);
				    break;
				case CLASS_UPPER:
				    EMIT(NFA_CLASS_UPPER);
				    break;
				case CLASS_XDIGIT:
				    EMIT(NFA_CLASS_XDIGIT);
				    break;
				case CLASS_TAB:
				    EMIT(NFA_CLASS_TAB);
				    break;
				case CLASS_RETURN:
				    EMIT(NFA_CLASS_RETURN);
				    break;
				case CLASS_BACKSPACE:
				    EMIT(NFA_CLASS_BACKSPACE);
				    break;
				case CLASS_ESCAPE:
				    EMIT(NFA_CLASS_ESCAPE);
				    break;
			    }
			    TRY_NEG();
			    EMIT_GLUE();
			    continue;
			}
			/* Try equivalence class [=a=] and the like */
			if (equiclass != 0)
			{
			    result = nfa_emit_equi_class(equiclass, negated);
			    if (result == FAIL)
			    {
				/* should never happen */
				EMSG_RET_FAIL(_("E868: Error building NFA with equivalence class!"));
			    }
			    EMIT_GLUE();
			    continue;
			}
			/* Try collating class like [. .]  */
			if (collclass != 0)
			{
			    startc = collclass;	 /* allow [.a.]-x as a range */
			    /* Will emit the proper atom at the end of the
			     * while loop. */
			}
		    }
		    /* Try a range like 'a-x' or '\t-z'. Also allows '-' as a
		     * start character. */
		    if (*regparse == '-' && oldstartc != -1)
		    {
			emit_range = TRUE;
			startc = oldstartc;
			mb_ptr_adv(regparse);
			continue;	    /* reading the end of the range */
		    }

		    /* Now handle simple and escaped characters.
		     * Only "\]", "\^", "\]" and "\\" are special in Vi.  Vim
		     * accepts "\t", "\e", etc., but only when the 'l' flag in
		     * 'cpoptions' is not included.
		     * Posix doesn't recognize backslash at all.
		     */
		    if (*regparse == '\\'
			    && !reg_cpo_bsl
			    && regparse + 1 <= endp
			    && (vim_strchr(REGEXP_INRANGE, regparse[1]) != NULL
				|| (!reg_cpo_lit
				    && vim_strchr(REGEXP_ABBR, regparse[1])
								      != NULL)
			    )
			)
		    {
			mb_ptr_adv(regparse);

			if (*regparse == 'n')
			    startc = reg_string ? NL : NFA_NEWL;
			else
			    if  (*regparse == 'd'
				    || *regparse == 'o'
				    || *regparse == 'x'
				    || *regparse == 'u'
				    || *regparse == 'U'
				)
			    {
				/* TODO(RE) This needs more testing */
				startc = coll_get_char();
				got_coll_char = TRUE;
				mb_ptr_back(old_regparse, regparse);
			    }
			    else
			    {
				/* \r,\t,\e,\b */
				startc = backslash_trans(*regparse);
			    }
		    }

		    /* Normal printable char */
		    if (startc == -1)
			startc = PTR2CHAR(regparse);

		    /* Previous char was '-', so this char is end of range. */
		    if (emit_range)
		    {
			endc = startc;
			startc = oldstartc;
			if (startc > endc)
			    EMSG_RET_FAIL(_(e_invrange));
#ifdef FEAT_MBYTE
			if (has_mbyte && ((*mb_char2len)(startc) > 1
				    || (*mb_char2len)(endc) > 1))
			{
			    if (endc > startc + 256)
				EMSG_RET_FAIL(_(e_invrange));
			    /* Emit the range. "startc" was already emitted, so
			     * skip it. */
			    for (c = startc + 1; c <= endc; c++)
			    {
				EMIT(c);
				TRY_NEG();
				EMIT_GLUE();
			    }
			}
			else
#endif
			{
#ifdef EBCDIC
			    int alpha_only = FALSE;

			    /* for alphabetical range skip the gaps
			     * 'i'-'j', 'r'-'s', 'I'-'J' and 'R'-'S'. */
			    if (isalpha(startc) && isalpha(endc))
				alpha_only = TRUE;
#endif
			    /* Emit the range. "startc" was already emitted, so
			     * skip it. */
			    for (c = startc + 1; c <= endc; c++)
#ifdef EBCDIC
				if (!alpha_only || isalpha(startc))
#endif
				{
				    EMIT(c);
				    TRY_NEG();
				    EMIT_GLUE();
				}
			}
			emit_range = FALSE;
			startc = -1;
		    }
		    else
		    {
			/*
			 * This char (startc) is not part of a range. Just
			 * emit it.
			 *
			 * Normally, simply emit startc. But if we get char
			 * code=0 from a collating char, then replace it with
			 * 0x0a.
			 *
			 * This is needed to completely mimic the behaviour of
			 * the backtracking engine.
			 */
			if (got_coll_char == TRUE && startc == 0)
			    EMIT(0x0a);
			else
			    EMIT(startc);
			TRY_NEG();
			EMIT_GLUE();
		    }

		    mb_ptr_adv(regparse);
		} /* while (p < endp) */

		mb_ptr_back(old_regparse, regparse);
		if (*regparse == '-')	    /* if last, '-' is just a char */
		{
		    EMIT('-');
		    TRY_NEG();
		    EMIT_GLUE();
		}
		mb_ptr_adv(regparse);

		/* skip the trailing ] */
		regparse = endp;
		mb_ptr_adv(regparse);
		if (negated == TRUE)
		{
		    /* Mark end of negated char range */
		    EMIT(NFA_END_NEG_RANGE);
		    EMIT(NFA_CONCAT);
		}

		/* \_[] also matches \n but it's not negated */
		if (extra == ADD_NL)
		{
		    EMIT(reg_string ? NL : NFA_NEWL);
		    EMIT(NFA_OR);
		}

		return OK;
	    } /* if exists closing ] */

	    if (reg_strict)
		EMSG_RET_FAIL(_(e_missingbracket));
	    /* FALLTHROUGH */

	default:
	    {
#ifdef FEAT_MBYTE
		int	plen;

nfa_do_multibyte:
		/* plen is length of current char with composing chars */
		if (enc_utf8 && ((*mb_char2len)(c)
			    != (plen = (*mb_ptr2len)(old_regparse))
						       || utf_iscomposing(c)))
		{
		    int i = 0;

		    /* A base character plus composing characters, or just one
		     * or more composing characters.
		     * This requires creating a separate atom as if enclosing
		     * the characters in (), where NFA_COMPOSING is the ( and
		     * NFA_END_COMPOSING is the ). Note that right now we are
		     * building the postfix form, not the NFA itself;
		     * a composing char could be: a, b, c, NFA_COMPOSING
		     * where 'b' and 'c' are chars with codes > 256. */
		    for (;;)
		    {
			EMIT(c);
			if (i > 0)
			    EMIT(NFA_CONCAT);
			if ((i += utf_char2len(c)) >= plen)
			    break;
			c = utf_ptr2char(old_regparse + i);
		    }
		    EMIT(NFA_COMPOSING);
		    regparse = old_regparse + plen;
		}
		else
#endif
		{
		    c = no_Magic(c);
		    EMIT(c);
		}
		return OK;
	    }
    }

#undef TRY_NEG
#undef EMIT_GLUE

    return OK;
}

/*
 * Parse something followed by possible [*+=].
 *
 * A piece is an atom, possibly followed by a multi, an indication of how many
 * times the atom can be matched.  Example: "a*" matches any sequence of "a"
 * characters: "", "a", "aa", etc.
 *
 * piece   ::=	    atom
 *	or  atom  multi
 */
    static int
nfa_regpiece()
{
    int		i;
    int		op;
    int		ret;
    long	minval, maxval;
    int		greedy = TRUE;      /* Braces are prefixed with '-' ? */
    parse_state_T old_state;
    parse_state_T new_state;
    int		c2;
    int		old_post_pos;
    int		my_post_start;
    int		quest;

    /* Save the current parse state, so that we can use it if <atom>{m,n} is
     * next. */
    save_parse_state(&old_state);

    /* store current pos in the postfix form, for \{m,n} involving 0s */
    my_post_start = (int)(post_ptr - post_start);

    ret = nfa_regatom();
    if (ret == FAIL)
	return FAIL;	    /* cascaded error */

    op = peekchr();
    if (re_multi_type(op) == NOT_MULTI)
	return OK;

    skipchr();
    switch (op)
    {
	case Magic('*'):
	    EMIT(NFA_STAR);
	    break;

	case Magic('+'):
	    /*
	     * Trick: Normally, (a*)\+ would match the whole input "aaa".  The
	     * first and only submatch would be "aaa". But the backtracking
	     * engine interprets the plus as "try matching one more time", and
	     * a* matches a second time at the end of the input, the empty
	     * string.
	     * The submatch will the empty string.
	     *
	     * In order to be consistent with the old engine, we replace
	     * <atom>+ with <atom><atom>*
	     */
	    restore_parse_state(&old_state);
	    curchr = -1;
	    if (nfa_regatom() == FAIL)
		return FAIL;
	    EMIT(NFA_STAR);
	    EMIT(NFA_CONCAT);
	    skipchr();		/* skip the \+	*/
	    break;

	case Magic('@'):
	    c2 = getdecchrs();
	    op = no_Magic(getchr());
	    i = 0;
	    switch(op)
	    {
		case '=':
		    /* \@= */
		    i = NFA_PREV_ATOM_NO_WIDTH;
		    break;
		case '!':
		    /* \@! */
		    i = NFA_PREV_ATOM_NO_WIDTH_NEG;
		    break;
		case '<':
		    op = no_Magic(getchr());
		    if (op == '=')
			/* \@<= */
			i = NFA_PREV_ATOM_JUST_BEFORE;
		    else if (op == '!')
			/* \@<! */
			i = NFA_PREV_ATOM_JUST_BEFORE_NEG;
		    break;
		case '>':
		    /* \@>  */
		    i = NFA_PREV_ATOM_LIKE_PATTERN;
		    break;
	    }
	    if (i == 0)
	    {
		EMSGN(_("E869: (NFA) Unknown operator '\\@%c'"), op);
		return FAIL;
	    }
	    EMIT(i);
	    if (i == NFA_PREV_ATOM_JUST_BEFORE
					|| i == NFA_PREV_ATOM_JUST_BEFORE_NEG)
		EMIT(c2);
	    break;

	case Magic('?'):
	case Magic('='):
	    EMIT(NFA_QUEST);
	    break;

	case Magic('{'):
	    /* a{2,5} will expand to 'aaa?a?a?'
	     * a{-1,3} will expand to 'aa??a??', where ?? is the nongreedy
	     * version of '?'
	     * \v(ab){2,3} will expand to '(ab)(ab)(ab)?', where all the
	     * parenthesis have the same id
	     */

	    greedy = TRUE;
	    c2 = peekchr();
	    if (c2 == '-' || c2 == Magic('-'))
	    {
		skipchr();
		greedy = FALSE;
	    }
	    if (!read_limits(&minval, &maxval))
		EMSG_RET_FAIL(_("E870: (NFA regexp) Error reading repetition limits"));

	    /*  <atom>{0,inf}, <atom>{0,} and <atom>{}  are equivalent to
	     *  <atom>*  */
	    if (minval == 0 && maxval == MAX_LIMIT)
	    {
		if (greedy)
		    /* \{}, \{0,} */
		    EMIT(NFA_STAR);
		else
		    /* \{-}, \{-0,} */
		    EMIT(NFA_STAR_NONGREEDY);
		break;
	    }

	    /* Special case: x{0} or x{-0} */
	    if (maxval == 0)
	    {
		/* Ignore result of previous call to nfa_regatom() */
		post_ptr = post_start + my_post_start;
		/* NFA_SKIP_CHAR has 0-length and works everywhere */
		EMIT(NFA_SKIP_CHAR);
		return OK;
	    }

	    /* Ignore previous call to nfa_regatom() */
	    post_ptr = post_start + my_post_start;
	    /* Save parse state after the repeated atom and the \{} */
	    save_parse_state(&new_state);

	    quest = (greedy == TRUE? NFA_QUEST : NFA_QUEST_NONGREEDY);
	    for (i = 0; i < maxval; i++)
	    {
		/* Goto beginning of the repeated atom */
		restore_parse_state(&old_state);
		old_post_pos = (int)(post_ptr - post_start);
		if (nfa_regatom() == FAIL)
		    return FAIL;
		/* after "minval" times, atoms are optional */
		if (i + 1 > minval)
		{
		    if (maxval == MAX_LIMIT)
		    {
			if (greedy)
			    EMIT(NFA_STAR);
			else
			    EMIT(NFA_STAR_NONGREEDY);
		    }
		    else
			EMIT(quest);
		}
		if (old_post_pos != my_post_start)
		    EMIT(NFA_CONCAT);
		if (i + 1 > minval && maxval == MAX_LIMIT)
		    break;
	    }

	    /* Go to just after the repeated atom and the \{} */
	    restore_parse_state(&new_state);
	    curchr = -1;

	    break;


	default:
	    break;
    }	/* end switch */

    if (re_multi_type(peekchr()) != NOT_MULTI)
	/* Can't have a multi follow a multi. */
	EMSG_RET_FAIL(_("E871: (NFA regexp) Can't have a multi follow a multi !"));

    return OK;
}

/*
 * Parse one or more pieces, concatenated.  It matches a match for the
 * first piece, followed by a match for the second piece, etc.  Example:
 * "f[0-9]b", first matches "f", then a digit and then "b".
 *
 * concat  ::=	    piece
 *	or  piece piece
 *	or  piece piece piece
 *	etc.
 */
    static int
nfa_regconcat()
{
    int		cont = TRUE;
    int		first = TRUE;

    while (cont)
    {
	switch (peekchr())
	{
	    case NUL:
	    case Magic('|'):
	    case Magic('&'):
	    case Magic(')'):
		cont = FALSE;
		break;

	    case Magic('Z'):
#ifdef FEAT_MBYTE
		regflags |= RF_ICOMBINE;
#endif
		skipchr_keepstart();
		break;
	    case Magic('c'):
		regflags |= RF_ICASE;
		skipchr_keepstart();
		break;
	    case Magic('C'):
		regflags |= RF_NOICASE;
		skipchr_keepstart();
		break;
	    case Magic('v'):
		reg_magic = MAGIC_ALL;
		skipchr_keepstart();
		curchr = -1;
		break;
	    case Magic('m'):
		reg_magic = MAGIC_ON;
		skipchr_keepstart();
		curchr = -1;
		break;
	    case Magic('M'):
		reg_magic = MAGIC_OFF;
		skipchr_keepstart();
		curchr = -1;
		break;
	    case Magic('V'):
		reg_magic = MAGIC_NONE;
		skipchr_keepstart();
		curchr = -1;
		break;

	    default:
		if (nfa_regpiece() == FAIL)
		    return FAIL;
		if (first == FALSE)
		    EMIT(NFA_CONCAT);
		else
		    first = FALSE;
		break;
	}
    }

    return OK;
}

/*
 * Parse a branch, one or more concats, separated by "\&".  It matches the
 * last concat, but only if all the preceding concats also match at the same
 * position.  Examples:
 *      "foobeep\&..." matches "foo" in "foobeep".
 *      ".*Peter\&.*Bob" matches in a line containing both "Peter" and "Bob"
 *
 * branch ::=	    concat
 *		or  concat \& concat
 *		or  concat \& concat \& concat
 *		etc.
 */
    static int
nfa_regbranch()
{
    int		ch;
    int		old_post_pos;

    old_post_pos = (int)(post_ptr - post_start);

    /* First branch, possibly the only one */
    if (nfa_regconcat() == FAIL)
	return FAIL;

    ch = peekchr();
    /* Try next concats */
    while (ch == Magic('&'))
    {
	skipchr();
	EMIT(NFA_NOPEN);
	EMIT(NFA_PREV_ATOM_NO_WIDTH);
	old_post_pos = (int)(post_ptr - post_start);
	if (nfa_regconcat() == FAIL)
	    return FAIL;
	/* if concat is empty, skip a input char. But do emit a node */
	if (old_post_pos == (int)(post_ptr - post_start))
	    EMIT(NFA_SKIP_CHAR);
	EMIT(NFA_CONCAT);
	ch = peekchr();
    }

    /* Even if a branch is empty, emit one node for it */
    if (old_post_pos == (int)(post_ptr - post_start))
	EMIT(NFA_SKIP_CHAR);

    return OK;
}

/*
 *  Parse a pattern, one or more branches, separated by "\|".  It matches
 *  anything that matches one of the branches.  Example: "foo\|beep" matches
 *  "foo" and matches "beep".  If more than one branch matches, the first one
 *  is used.
 *
 *  pattern ::=	    branch
 *	or  branch \| branch
 *	or  branch \| branch \| branch
 *	etc.
 */
    static int
nfa_reg(paren)
    int		paren;	/* REG_NOPAREN, REG_PAREN, REG_NPAREN or REG_ZPAREN */
{
    int		parno = 0;

    if (paren == REG_PAREN)
    {
	if (regnpar >= NSUBEXP) /* Too many `(' */
	    EMSG_RET_FAIL(_("E872: (NFA regexp) Too many '('"));
	parno = regnpar++;
    }
#ifdef FEAT_SYN_HL
    else if (paren == REG_ZPAREN)
    {
	/* Make a ZOPEN node. */
	if (regnzpar >= NSUBEXP)
	    EMSG_RET_FAIL(_("E879: (NFA regexp) Too many \\z("));
	parno = regnzpar++;
    }
#endif

    if (nfa_regbranch() == FAIL)
	return FAIL;	    /* cascaded error */

    while (peekchr() == Magic('|'))
    {
	skipchr();
	if (nfa_regbranch() == FAIL)
	    return FAIL;    /* cascaded error */
	EMIT(NFA_OR);
    }

    /* Check for proper termination. */
    if (paren != REG_NOPAREN && getchr() != Magic(')'))
    {
	if (paren == REG_NPAREN)
	    EMSG2_RET_FAIL(_(e_unmatchedpp), reg_magic == MAGIC_ALL);
	else
	    EMSG2_RET_FAIL(_(e_unmatchedp), reg_magic == MAGIC_ALL);
    }
    else if (paren == REG_NOPAREN && peekchr() != NUL)
    {
	if (peekchr() == Magic(')'))
	    EMSG2_RET_FAIL(_(e_unmatchedpar), reg_magic == MAGIC_ALL);
	else
	    EMSG_RET_FAIL(_("E873: (NFA regexp) proper termination error"));
    }
    /*
     * Here we set the flag allowing back references to this set of
     * parentheses.
     */
    if (paren == REG_PAREN)
    {
	had_endbrace[parno] = TRUE;     /* have seen the close paren */
	EMIT(NFA_MOPEN + parno);
    }
#ifdef FEAT_SYN_HL
    else if (paren == REG_ZPAREN)
	EMIT(NFA_ZOPEN + parno);
#endif

    return OK;
}

#ifdef DEBUG
static char_u code[50];

    static void
nfa_set_code(c)
    int	    c;
{
    int	    addnl = FALSE;

    if (c >= NFA_FIRST_NL && c <= NFA_LAST_NL)
    {
	addnl = TRUE;
	c -= ADD_NL;
    }

    STRCPY(code, "");
    switch (c)
    {
	case NFA_MATCH:	    STRCPY(code, "NFA_MATCH "); break;
	case NFA_SPLIT:	    STRCPY(code, "NFA_SPLIT "); break;
	case NFA_CONCAT:    STRCPY(code, "NFA_CONCAT "); break;
	case NFA_NEWL:	    STRCPY(code, "NFA_NEWL "); break;
	case NFA_ZSTART:    STRCPY(code, "NFA_ZSTART"); break;
	case NFA_ZEND:	    STRCPY(code, "NFA_ZEND"); break;

	case NFA_BACKREF1:  STRCPY(code, "NFA_BACKREF1"); break;
	case NFA_BACKREF2:  STRCPY(code, "NFA_BACKREF2"); break;
	case NFA_BACKREF3:  STRCPY(code, "NFA_BACKREF3"); break;
	case NFA_BACKREF4:  STRCPY(code, "NFA_BACKREF4"); break;
	case NFA_BACKREF5:  STRCPY(code, "NFA_BACKREF5"); break;
	case NFA_BACKREF6:  STRCPY(code, "NFA_BACKREF6"); break;
	case NFA_BACKREF7:  STRCPY(code, "NFA_BACKREF7"); break;
	case NFA_BACKREF8:  STRCPY(code, "NFA_BACKREF8"); break;
	case NFA_BACKREF9:  STRCPY(code, "NFA_BACKREF9"); break;
#ifdef FEAT_SYN_HL
	case NFA_ZREF1:	    STRCPY(code, "NFA_ZREF1"); break;
	case NFA_ZREF2:	    STRCPY(code, "NFA_ZREF2"); break;
	case NFA_ZREF3:	    STRCPY(code, "NFA_ZREF3"); break;
	case NFA_ZREF4:	    STRCPY(code, "NFA_ZREF4"); break;
	case NFA_ZREF5:	    STRCPY(code, "NFA_ZREF5"); break;
	case NFA_ZREF6:	    STRCPY(code, "NFA_ZREF6"); break;
	case NFA_ZREF7:	    STRCPY(code, "NFA_ZREF7"); break;
	case NFA_ZREF8:	    STRCPY(code, "NFA_ZREF8"); break;
	case NFA_ZREF9:	    STRCPY(code, "NFA_ZREF9"); break;
#endif
	case NFA_SKIP:	    STRCPY(code, "NFA_SKIP"); break;

	case NFA_PREV_ATOM_NO_WIDTH:
			    STRCPY(code, "NFA_PREV_ATOM_NO_WIDTH"); break;
	case NFA_PREV_ATOM_NO_WIDTH_NEG:
			    STRCPY(code, "NFA_PREV_ATOM_NO_WIDTH_NEG"); break;
	case NFA_PREV_ATOM_JUST_BEFORE:
			    STRCPY(code, "NFA_PREV_ATOM_JUST_BEFORE"); break;
	case NFA_PREV_ATOM_JUST_BEFORE_NEG:
			 STRCPY(code, "NFA_PREV_ATOM_JUST_BEFORE_NEG"); break;
	case NFA_PREV_ATOM_LIKE_PATTERN:
			    STRCPY(code, "NFA_PREV_ATOM_LIKE_PATTERN"); break;

	case NFA_NOPEN:		    STRCPY(code, "NFA_NOPEN"); break;
	case NFA_NCLOSE:	    STRCPY(code, "NFA_NCLOSE"); break;
	case NFA_START_INVISIBLE:   STRCPY(code, "NFA_START_INVISIBLE"); break;
	case NFA_START_INVISIBLE_BEFORE:
			    STRCPY(code, "NFA_START_INVISIBLE_BEFORE"); break;
	case NFA_START_PATTERN:   STRCPY(code, "NFA_START_PATTERN"); break;
	case NFA_END_INVISIBLE:	    STRCPY(code, "NFA_END_INVISIBLE"); break;
	case NFA_END_PATTERN:	    STRCPY(code, "NFA_END_PATTERN"); break;

	case NFA_COMPOSING:	    STRCPY(code, "NFA_COMPOSING"); break;
	case NFA_END_COMPOSING:	    STRCPY(code, "NFA_END_COMPOSING"); break;
	case NFA_OPT_CHARS:	    STRCPY(code, "NFA_OPT_CHARS"); break;

	case NFA_MOPEN:
	case NFA_MOPEN1:
	case NFA_MOPEN2:
	case NFA_MOPEN3:
	case NFA_MOPEN4:
	case NFA_MOPEN5:
	case NFA_MOPEN6:
	case NFA_MOPEN7:
	case NFA_MOPEN8:
	case NFA_MOPEN9:
	    STRCPY(code, "NFA_MOPEN(x)");
	    code[10] = c - NFA_MOPEN + '0';
	    break;
	case NFA_MCLOSE:
	case NFA_MCLOSE1:
	case NFA_MCLOSE2:
	case NFA_MCLOSE3:
	case NFA_MCLOSE4:
	case NFA_MCLOSE5:
	case NFA_MCLOSE6:
	case NFA_MCLOSE7:
	case NFA_MCLOSE8:
	case NFA_MCLOSE9:
	    STRCPY(code, "NFA_MCLOSE(x)");
	    code[11] = c - NFA_MCLOSE + '0';
	    break;
#ifdef FEAT_SYN_HL
	case NFA_ZOPEN:
	case NFA_ZOPEN1:
	case NFA_ZOPEN2:
	case NFA_ZOPEN3:
	case NFA_ZOPEN4:
	case NFA_ZOPEN5:
	case NFA_ZOPEN6:
	case NFA_ZOPEN7:
	case NFA_ZOPEN8:
	case NFA_ZOPEN9:
	    STRCPY(code, "NFA_ZOPEN(x)");
	    code[10] = c - NFA_ZOPEN + '0';
	    break;
	case NFA_ZCLOSE:
	case NFA_ZCLOSE1:
	case NFA_ZCLOSE2:
	case NFA_ZCLOSE3:
	case NFA_ZCLOSE4:
	case NFA_ZCLOSE5:
	case NFA_ZCLOSE6:
	case NFA_ZCLOSE7:
	case NFA_ZCLOSE8:
	case NFA_ZCLOSE9:
	    STRCPY(code, "NFA_ZCLOSE(x)");
	    code[11] = c - NFA_ZCLOSE + '0';
	    break;
#endif
	case NFA_EOL:		STRCPY(code, "NFA_EOL "); break;
	case NFA_BOL:		STRCPY(code, "NFA_BOL "); break;
	case NFA_EOW:		STRCPY(code, "NFA_EOW "); break;
	case NFA_BOW:		STRCPY(code, "NFA_BOW "); break;
	case NFA_EOF:		STRCPY(code, "NFA_EOF "); break;
	case NFA_BOF:		STRCPY(code, "NFA_BOF "); break;
	case NFA_LNUM:		STRCPY(code, "NFA_LNUM "); break;
	case NFA_LNUM_GT:	STRCPY(code, "NFA_LNUM_GT "); break;
	case NFA_LNUM_LT:	STRCPY(code, "NFA_LNUM_LT "); break;
	case NFA_COL:		STRCPY(code, "NFA_COL "); break;
	case NFA_COL_GT:	STRCPY(code, "NFA_COL_GT "); break;
	case NFA_COL_LT:	STRCPY(code, "NFA_COL_LT "); break;
	case NFA_VCOL:		STRCPY(code, "NFA_VCOL "); break;
	case NFA_VCOL_GT:	STRCPY(code, "NFA_VCOL_GT "); break;
	case NFA_VCOL_LT:	STRCPY(code, "NFA_VCOL_LT "); break;
	case NFA_MARK:		STRCPY(code, "NFA_MARK "); break;
	case NFA_MARK_GT:	STRCPY(code, "NFA_MARK_GT "); break;
	case NFA_MARK_LT:	STRCPY(code, "NFA_MARK_LT "); break;
	case NFA_CURSOR:	STRCPY(code, "NFA_CURSOR "); break;
	case NFA_VISUAL:	STRCPY(code, "NFA_VISUAL "); break;

	case NFA_STAR:		STRCPY(code, "NFA_STAR "); break;
	case NFA_STAR_NONGREEDY: STRCPY(code, "NFA_STAR_NONGREEDY "); break;
	case NFA_QUEST:		STRCPY(code, "NFA_QUEST"); break;
	case NFA_QUEST_NONGREEDY: STRCPY(code, "NFA_QUEST_NON_GREEDY"); break;
	case NFA_NOT:		STRCPY(code, "NFA_NOT "); break;
	case NFA_SKIP_CHAR:	STRCPY(code, "NFA_SKIP_CHAR"); break;
	case NFA_OR:		STRCPY(code, "NFA_OR"); break;
	case NFA_END_NEG_RANGE:	STRCPY(code, "NFA_END_NEG_RANGE"); break;
	case NFA_CLASS_ALNUM:	STRCPY(code, "NFA_CLASS_ALNUM"); break;
	case NFA_CLASS_ALPHA:	STRCPY(code, "NFA_CLASS_ALPHA"); break;
	case NFA_CLASS_BLANK:	STRCPY(code, "NFA_CLASS_BLANK"); break;
	case NFA_CLASS_CNTRL:	STRCPY(code, "NFA_CLASS_CNTRL"); break;
	case NFA_CLASS_DIGIT:	STRCPY(code, "NFA_CLASS_DIGIT"); break;
	case NFA_CLASS_GRAPH:	STRCPY(code, "NFA_CLASS_GRAPH"); break;
	case NFA_CLASS_LOWER:	STRCPY(code, "NFA_CLASS_LOWER"); break;
	case NFA_CLASS_PRINT:	STRCPY(code, "NFA_CLASS_PRINT"); break;
	case NFA_CLASS_PUNCT:	STRCPY(code, "NFA_CLASS_PUNCT"); break;
	case NFA_CLASS_SPACE:	STRCPY(code, "NFA_CLASS_SPACE"); break;
	case NFA_CLASS_UPPER:	STRCPY(code, "NFA_CLASS_UPPER"); break;
	case NFA_CLASS_XDIGIT:	STRCPY(code, "NFA_CLASS_XDIGIT"); break;
	case NFA_CLASS_TAB:	STRCPY(code, "NFA_CLASS_TAB"); break;
	case NFA_CLASS_RETURN:	STRCPY(code, "NFA_CLASS_RETURN"); break;
	case NFA_CLASS_BACKSPACE:   STRCPY(code, "NFA_CLASS_BACKSPACE"); break;
	case NFA_CLASS_ESCAPE:	STRCPY(code, "NFA_CLASS_ESCAPE"); break;

	case NFA_ANY:	STRCPY(code, "NFA_ANY"); break;
	case NFA_IDENT:	STRCPY(code, "NFA_IDENT"); break;
	case NFA_SIDENT:STRCPY(code, "NFA_SIDENT"); break;
	case NFA_KWORD:	STRCPY(code, "NFA_KWORD"); break;
	case NFA_SKWORD:STRCPY(code, "NFA_SKWORD"); break;
	case NFA_FNAME:	STRCPY(code, "NFA_FNAME"); break;
	case NFA_SFNAME:STRCPY(code, "NFA_SFNAME"); break;
	case NFA_PRINT:	STRCPY(code, "NFA_PRINT"); break;
	case NFA_SPRINT:STRCPY(code, "NFA_SPRINT"); break;
	case NFA_WHITE:	STRCPY(code, "NFA_WHITE"); break;
	case NFA_NWHITE:STRCPY(code, "NFA_NWHITE"); break;
	case NFA_DIGIT:	STRCPY(code, "NFA_DIGIT"); break;
	case NFA_NDIGIT:STRCPY(code, "NFA_NDIGIT"); break;
	case NFA_HEX:	STRCPY(code, "NFA_HEX"); break;
	case NFA_NHEX:	STRCPY(code, "NFA_NHEX"); break;
	case NFA_OCTAL:	STRCPY(code, "NFA_OCTAL"); break;
	case NFA_NOCTAL:STRCPY(code, "NFA_NOCTAL"); break;
	case NFA_WORD:	STRCPY(code, "NFA_WORD"); break;
	case NFA_NWORD:	STRCPY(code, "NFA_NWORD"); break;
	case NFA_HEAD:	STRCPY(code, "NFA_HEAD"); break;
	case NFA_NHEAD:	STRCPY(code, "NFA_NHEAD"); break;
	case NFA_ALPHA:	STRCPY(code, "NFA_ALPHA"); break;
	case NFA_NALPHA:STRCPY(code, "NFA_NALPHA"); break;
	case NFA_LOWER:	STRCPY(code, "NFA_LOWER"); break;
	case NFA_NLOWER:STRCPY(code, "NFA_NLOWER"); break;
	case NFA_UPPER:	STRCPY(code, "NFA_UPPER"); break;
	case NFA_NUPPER:STRCPY(code, "NFA_NUPPER"); break;

	default:
	    STRCPY(code, "CHAR(x)");
	    code[5] = c;
    }

    if (addnl == TRUE)
	STRCAT(code, " + NEWLINE ");

}

#ifdef ENABLE_LOG
static FILE *log_fd;

/*
 * Print the postfix notation of the current regexp.
 */
    static void
nfa_postfix_dump(expr, retval)
    char_u  *expr;
    int	    retval;
{
    int *p;
    FILE *f;

    f = fopen(NFA_REGEXP_DUMP_LOG, "a");
    if (f != NULL)
    {
	fprintf(f, "\n-------------------------\n");
	if (retval == FAIL)
	    fprintf(f, ">>> NFA engine failed ... \n");
	else if (retval == OK)
	    fprintf(f, ">>> NFA engine succeeded !\n");
	fprintf(f, "Regexp: \"%s\"\nPostfix notation (char): \"", expr);
	for (p = post_start; *p && p < post_end; p++)
	{
	    nfa_set_code(*p);
	    fprintf(f, "%s, ", code);
	}
	fprintf(f, "\"\nPostfix notation (int): ");
	for (p = post_start; *p && p < post_end; p++)
		fprintf(f, "%d ", *p);
	fprintf(f, "\n\n");
	fclose(f);
    }
}

/*
 * Print the NFA starting with a root node "state".
 */
    static void
nfa_print_state(debugf, state)
    FILE *debugf;
    nfa_state_T *state;
{
    garray_T indent;

    ga_init2(&indent, 1, 64);
    ga_append(&indent, '\0');
    nfa_print_state2(debugf, state, &indent);
    ga_clear(&indent);
}

    static void
nfa_print_state2(debugf, state, indent)
    FILE *debugf;
    nfa_state_T *state;
    garray_T *indent;
{
    char_u  *p;

    if (state == NULL)
	return;

    fprintf(debugf, "(%2d)", abs(state->id));

    /* Output indent */
    p = (char_u *)indent->ga_data;
    if (indent->ga_len >= 3)
    {
	int	last = indent->ga_len - 3;
	char_u	save[2];

	STRNCPY(save, &p[last], 2);
	STRNCPY(&p[last], "+-", 2);
	fprintf(debugf, " %s", p);
	STRNCPY(&p[last], save, 2);
    }
    else
	fprintf(debugf, " %s", p);

    nfa_set_code(state->c);
    fprintf(debugf, "%s%s (%d) (id=%d)\n",
		 state->negated ? "NOT " : "", code, state->c, abs(state->id));
    if (state->id < 0)
	return;

    state->id = abs(state->id) * -1;

    /* grow indent for state->out */
    indent->ga_len -= 1;
    if (state->out1)
	ga_concat(indent, (char_u *)"| ");
    else
	ga_concat(indent, (char_u *)"  ");
    ga_append(indent, '\0');

    nfa_print_state2(debugf, state->out, indent);

    /* replace last part of indent for state->out1 */
    indent->ga_len -= 3;
    ga_concat(indent, (char_u *)"  ");
    ga_append(indent, '\0');

    nfa_print_state2(debugf, state->out1, indent);

    /* shrink indent */
    indent->ga_len -= 3;
    ga_append(indent, '\0');
}

/*
 * Print the NFA state machine.
 */
    static void
nfa_dump(prog)
    nfa_regprog_T *prog;
{
    FILE *debugf = fopen(NFA_REGEXP_DUMP_LOG, "a");

    if (debugf != NULL)
    {
	nfa_print_state(debugf, prog->start);
	fclose(debugf);
    }
}
#endif	    /* ENABLE_LOG */
#endif	    /* DEBUG */

/*
 * Parse r.e. @expr and convert it into postfix form.
 * Return the postfix string on success, NULL otherwise.
 */
    static int *
re2post()
{
    if (nfa_reg(REG_NOPAREN) == FAIL)
	return NULL;
    EMIT(NFA_MOPEN);
    return post_start;
}

/* NB. Some of the code below is inspired by Russ's. */

/*
 * Represents an NFA state plus zero or one or two arrows exiting.
 * if c == MATCH, no arrows out; matching state.
 * If c == SPLIT, unlabeled arrows to out and out1 (if != NULL).
 * If c < 256, labeled arrow with character c to out.
 */

static nfa_state_T	*state_ptr; /* points to nfa_prog->state */

/*
 * Allocate and initialize nfa_state_T.
 */
    static nfa_state_T *
alloc_state(c, out, out1)
    int		c;
    nfa_state_T	*out;
    nfa_state_T	*out1;
{
    nfa_state_T *s;

    if (istate >= nstate)
	return NULL;

    s = &state_ptr[istate++];

    s->c    = c;
    s->out  = out;
    s->out1 = out1;

    s->id   = istate;
    s->lastlist[0] = 0;
    s->lastlist[1] = 0;
    s->negated = FALSE;

    return s;
}

/*
 * A partially built NFA without the matching state filled in.
 * Frag_T.start points at the start state.
 * Frag_T.out is a list of places that need to be set to the
 * next state for this fragment.
 */

/* Since the out pointers in the list are always
 * uninitialized, we use the pointers themselves
 * as storage for the Ptrlists. */
typedef union Ptrlist Ptrlist;
union Ptrlist
{
    Ptrlist	*next;
    nfa_state_T	*s;
};

struct Frag
{
    nfa_state_T *start;
    Ptrlist	*out;
};
typedef struct Frag Frag_T;

static Frag_T frag __ARGS((nfa_state_T *start, Ptrlist *out));
static Ptrlist *list1 __ARGS((nfa_state_T **outp));
static void patch __ARGS((Ptrlist *l, nfa_state_T *s));
static Ptrlist *append __ARGS((Ptrlist *l1, Ptrlist *l2));
static void st_push __ARGS((Frag_T s, Frag_T **p, Frag_T *stack_end));
static Frag_T st_pop __ARGS((Frag_T **p, Frag_T *stack));

/*
 * Initialize a Frag_T struct and return it.
 */
    static Frag_T
frag(start, out)
    nfa_state_T	*start;
    Ptrlist	*out;
{
    Frag_T n;

    n.start = start;
    n.out = out;
    return n;
}

/*
 * Create singleton list containing just outp.
 */
    static Ptrlist *
list1(outp)
    nfa_state_T	**outp;
{
    Ptrlist *l;

    l = (Ptrlist *)outp;
    l->next = NULL;
    return l;
}

/*
 * Patch the list of states at out to point to start.
 */
    static void
patch(l, s)
    Ptrlist	*l;
    nfa_state_T	*s;
{
    Ptrlist *next;

    for (; l; l = next)
    {
	next = l->next;
	l->s = s;
    }
}


/*
 * Join the two lists l1 and l2, returning the combination.
 */
    static Ptrlist *
append(l1, l2)
    Ptrlist *l1;
    Ptrlist *l2;
{
    Ptrlist *oldl1;

    oldl1 = l1;
    while (l1->next)
	l1 = l1->next;
    l1->next = l2;
    return oldl1;
}

/*
 * Stack used for transforming postfix form into NFA.
 */
static Frag_T empty;

    static void
st_error(postfix, end, p)
    int *postfix UNUSED;
    int *end UNUSED;
    int *p UNUSED;
{
#ifdef NFA_REGEXP_ERROR_LOG
    FILE *df;
    int *p2;

    df = fopen(NFA_REGEXP_ERROR_LOG, "a");
    if (df)
    {
	fprintf(df, "Error popping the stack!\n");
#ifdef DEBUG
	fprintf(df, "Current regexp is \"%s\"\n", nfa_regengine.expr);
#endif
	fprintf(df, "Postfix form is: ");
#ifdef DEBUG
	for (p2 = postfix; p2 < end; p2++)
	{
	    nfa_set_code(*p2);
	    fprintf(df, "%s, ", code);
	}
	nfa_set_code(*p);
	fprintf(df, "\nCurrent position is: ");
	for (p2 = postfix; p2 <= p; p2 ++)
	{
	    nfa_set_code(*p2);
	    fprintf(df, "%s, ", code);
	}
#else
	for (p2 = postfix; p2 < end; p2++)
	{
	    fprintf(df, "%d, ", *p2);
	}
	fprintf(df, "\nCurrent position is: ");
	for (p2 = postfix; p2 <= p; p2 ++)
	{
	    fprintf(df, "%d, ", *p2);
	}
#endif
	fprintf(df, "\n--------------------------\n");
	fclose(df);
    }
#endif
    EMSG(_("E874: (NFA) Could not pop the stack !"));
}

/*
 * Push an item onto the stack.
 */
    static void
st_push(s, p, stack_end)
    Frag_T s;
    Frag_T **p;
    Frag_T *stack_end;
{
    Frag_T *stackp = *p;

    if (stackp >= stack_end)
	return;
    *stackp = s;
    *p = *p + 1;
}

/*
 * Pop an item from the stack.
 */
    static Frag_T
st_pop(p, stack)
    Frag_T **p;
    Frag_T *stack;
{
    Frag_T *stackp;

    *p = *p - 1;
    stackp = *p;
    if (stackp < stack)
	return empty;
    return **p;
}

/*
 * Convert a postfix form into its equivalent NFA.
 * Return the NFA start state on success, NULL otherwise.
 */
    static nfa_state_T *
post2nfa(postfix, end, nfa_calc_size)
    int		*postfix;
    int		*end;
    int		nfa_calc_size;
{
    int		*p;
    int		mopen;
    int		mclose;
    Frag_T	*stack = NULL;
    Frag_T	*stackp = NULL;
    Frag_T	*stack_end = NULL;
    Frag_T	e1;
    Frag_T	e2;
    Frag_T	e;
    nfa_state_T	*s;
    nfa_state_T	*s1;
    nfa_state_T	*matchstate;
    nfa_state_T	*ret = NULL;

    if (postfix == NULL)
	return NULL;

#define PUSH(s)	    st_push((s), &stackp, stack_end)
#define POP()	    st_pop(&stackp, stack);		\
		    if (stackp < stack)			\
		    {					\
			st_error(postfix, end, p);	\
			return NULL;			\
		    }

    if (nfa_calc_size == FALSE)
    {
	/* Allocate space for the stack. Max states on the stack : nstate */
	stack = (Frag_T *)lalloc((nstate + 1) * sizeof(Frag_T), TRUE);
	stackp = stack;
	stack_end = stack + (nstate + 1);
    }

    for (p = postfix; p < end; ++p)
    {
	switch (*p)
	{
	case NFA_CONCAT:
	    /* Catenation.
	     * Pay attention: this operator does not exist
	     * in the r.e. itself (it is implicit, really).
	     * It is added when r.e. is translated to postfix
	     * form in re2post().
	     *
	     * No new state added here. */
	    if (nfa_calc_size == TRUE)
	    {
		/* nstate += 0; */
		break;
	    }
	    e2 = POP();
	    e1 = POP();
	    patch(e1.out, e2.start);
	    PUSH(frag(e1.start, e2.out));
	    break;

	case NFA_NOT:
	    /* Negation of a character */
	    if (nfa_calc_size == TRUE)
	    {
		/* nstate += 0; */
		break;
	    }
	    e1 = POP();
	    e1.start->negated = TRUE;
#ifdef FEAT_MBYTE
	    if (e1.start->c == NFA_COMPOSING)
		e1.start->out1->negated = TRUE;
#endif
	    PUSH(e1);
	    break;

	case NFA_OR:
	    /* Alternation */
	    if (nfa_calc_size == TRUE)
	    {
		nstate++;
		break;
	    }
	    e2 = POP();
	    e1 = POP();
	    s = alloc_state(NFA_SPLIT, e1.start, e2.start);
	    if (s == NULL)
		goto theend;
	    PUSH(frag(s, append(e1.out, e2.out)));
	    break;

	case NFA_STAR:
	    /* Zero or more, prefer more */
	    if (nfa_calc_size == TRUE)
	    {
		nstate++;
		break;
	    }
	    e = POP();
	    s = alloc_state(NFA_SPLIT, e.start, NULL);
	    if (s == NULL)
		goto theend;
	    patch(e.out, s);
	    PUSH(frag(s, list1(&s->out1)));
	    break;

	case NFA_STAR_NONGREEDY:
	    /* Zero or more, prefer zero */
	    if (nfa_calc_size == TRUE)
	    {
		nstate++;
		break;
	    }
	    e = POP();
	    s = alloc_state(NFA_SPLIT, NULL, e.start);
	    if (s == NULL)
		goto theend;
	    patch(e.out, s);
	    PUSH(frag(s, list1(&s->out)));
	    break;

	case NFA_QUEST:
	    /* one or zero atoms=> greedy match */
	    if (nfa_calc_size == TRUE)
	    {
		nstate++;
		break;
	    }
	    e = POP();
	    s = alloc_state(NFA_SPLIT, e.start, NULL);
	    if (s == NULL)
		goto theend;
	    PUSH(frag(s, append(e.out, list1(&s->out1))));
	    break;

	case NFA_QUEST_NONGREEDY:
	    /* zero or one atoms => non-greedy match */
	    if (nfa_calc_size == TRUE)
	    {
		nstate++;
		break;
	    }
	    e = POP();
	    s = alloc_state(NFA_SPLIT, NULL, e.start);
	    if (s == NULL)
		goto theend;
	    PUSH(frag(s, append(e.out, list1(&s->out))));
	    break;

	case NFA_SKIP_CHAR:
	    /* Symbol of 0-length, Used in a repetition
	     * with max/min count of 0 */
	    if (nfa_calc_size == TRUE)
	    {
		nstate++;
		break;
	    }
	    s = alloc_state(NFA_SKIP_CHAR, NULL, NULL);
	    if (s == NULL)
		goto theend;
	    PUSH(frag(s, list1(&s->out)));
	    break;

	case NFA_OPT_CHARS:
	  {
	    int    n;

	    /* \%[abc] */
	    n = *++p; /* get number of characters */
	    if (nfa_calc_size == TRUE)
	    {
		nstate += n;
		break;
	    }
	    s = NULL; /* avoid compiler warning */
	    e1.out = NULL; /* stores list with out1's */
	    s1 = NULL; /* previous NFA_SPLIT to connect to */
	    while (n-- > 0)
	    {
		e = POP(); /* get character */
		s = alloc_state(NFA_SPLIT, e.start, NULL);
		if (s == NULL)
		    goto theend;
		if (e1.out == NULL)
		    e1 = e;
		patch(e.out, s1);
		append(e1.out, list1(&s->out1));
		s1 = s;
	    }
	    PUSH(frag(s, e1.out));
	    break;
	  }

	case NFA_PREV_ATOM_NO_WIDTH:
	case NFA_PREV_ATOM_NO_WIDTH_NEG:
	case NFA_PREV_ATOM_JUST_BEFORE:
	case NFA_PREV_ATOM_JUST_BEFORE_NEG:
	case NFA_PREV_ATOM_LIKE_PATTERN:
	  {
	    int neg = (*p == NFA_PREV_ATOM_NO_WIDTH_NEG
				      || *p == NFA_PREV_ATOM_JUST_BEFORE_NEG);
	    int before = (*p == NFA_PREV_ATOM_JUST_BEFORE
				      || *p == NFA_PREV_ATOM_JUST_BEFORE_NEG);
	    int pattern = (*p == NFA_PREV_ATOM_LIKE_PATTERN);
	    int start_state = NFA_START_INVISIBLE;
	    int end_state = NFA_END_INVISIBLE;
	    int n = 0;
	    nfa_state_T *zend;
	    nfa_state_T *skip;

	    if (before)
		start_state = NFA_START_INVISIBLE_BEFORE;
	    else if (pattern)
	    {
		start_state = NFA_START_PATTERN;
		end_state = NFA_END_PATTERN;
	    }

	    if (before)
		n = *++p; /* get the count */

	    /* The \@= operator: match the preceding atom with zero width.
	     * The \@! operator: no match for the preceding atom.
	     * The \@<= operator: match for the preceding atom.
	     * The \@<! operator: no match for the preceding atom.
	     * Surrounds the preceding atom with START_INVISIBLE and
	     * END_INVISIBLE, similarly to MOPEN. */

	    if (nfa_calc_size == TRUE)
	    {
		nstate += pattern ? 4 : 2;
		break;
	    }
	    e = POP();
	    s1 = alloc_state(end_state, NULL, NULL);
	    if (s1 == NULL)
		goto theend;

	    s = alloc_state(start_state, e.start, s1);
	    if (s == NULL)
		goto theend;
	    if (neg)
	    {
		s->negated = TRUE;
		s1->negated = TRUE;
	    }
	    if (before)
		s->val = n; /* store the count */
	    if (pattern)
	    {
		/* NFA_ZEND -> NFA_END_PATTERN -> NFA_SKIP -> what follows. */
		skip = alloc_state(NFA_SKIP, NULL, NULL);
		zend = alloc_state(NFA_ZEND, s1, NULL);
		s1->out= skip;
		patch(e.out, zend);
		PUSH(frag(s, list1(&skip->out)));
	    }
	    else
	    {
		patch(e.out, s1);
		PUSH(frag(s, list1(&s1->out)));
	    }
	    break;
	  }

#ifdef FEAT_MBYTE
	case NFA_COMPOSING:	/* char with composing char */
#if 0
	    /* TODO */
	    if (regflags & RF_ICOMBINE)
	    {
		/* use the base character only */
	    }
#endif
	    /* FALLTHROUGH */
#endif

	case NFA_MOPEN:	/* \( \) Submatch */
	case NFA_MOPEN1:
	case NFA_MOPEN2:
	case NFA_MOPEN3:
	case NFA_MOPEN4:
	case NFA_MOPEN5:
	case NFA_MOPEN6:
	case NFA_MOPEN7:
	case NFA_MOPEN8:
	case NFA_MOPEN9:
#ifdef FEAT_SYN_HL
	case NFA_ZOPEN:	/* \z( \) Submatch */
	case NFA_ZOPEN1:
	case NFA_ZOPEN2:
	case NFA_ZOPEN3:
	case NFA_ZOPEN4:
	case NFA_ZOPEN5:
	case NFA_ZOPEN6:
	case NFA_ZOPEN7:
	case NFA_ZOPEN8:
	case NFA_ZOPEN9:
#endif
	case NFA_NOPEN:	/* \%( \) "Invisible Submatch" */
	    if (nfa_calc_size == TRUE)
	    {
		nstate += 2;
		break;
	    }

	    mopen = *p;
	    switch (*p)
	    {
		case NFA_NOPEN: mclose = NFA_NCLOSE; break;
#ifdef FEAT_SYN_HL
		case NFA_ZOPEN: mclose = NFA_ZCLOSE; break;
		case NFA_ZOPEN1: mclose = NFA_ZCLOSE1; break;
		case NFA_ZOPEN2: mclose = NFA_ZCLOSE2; break;
		case NFA_ZOPEN3: mclose = NFA_ZCLOSE3; break;
		case NFA_ZOPEN4: mclose = NFA_ZCLOSE4; break;
		case NFA_ZOPEN5: mclose = NFA_ZCLOSE5; break;
		case NFA_ZOPEN6: mclose = NFA_ZCLOSE6; break;
		case NFA_ZOPEN7: mclose = NFA_ZCLOSE7; break;
		case NFA_ZOPEN8: mclose = NFA_ZCLOSE8; break;
		case NFA_ZOPEN9: mclose = NFA_ZCLOSE9; break;
#endif
#ifdef FEAT_MBYTE
		case NFA_COMPOSING: mclose = NFA_END_COMPOSING; break;
#endif
		default:
		    /* NFA_MOPEN, NFA_MOPEN1 .. NFA_MOPEN9 */
		    mclose = *p + NSUBEXP;
		    break;
	    }

	    /* Allow "NFA_MOPEN" as a valid postfix representation for
	     * the empty regexp "". In this case, the NFA will be
	     * NFA_MOPEN -> NFA_MCLOSE. Note that this also allows
	     * empty groups of parenthesis, and empty mbyte chars */
	    if (stackp == stack)
	    {
		s = alloc_state(mopen, NULL, NULL);
		if (s == NULL)
		    goto theend;
		s1 = alloc_state(mclose, NULL, NULL);
		if (s1 == NULL)
		    goto theend;
		patch(list1(&s->out), s1);
		PUSH(frag(s, list1(&s1->out)));
		break;
	    }

	    /* At least one node was emitted before NFA_MOPEN, so
	     * at least one node will be between NFA_MOPEN and NFA_MCLOSE */
	    e = POP();
	    s = alloc_state(mopen, e.start, NULL);   /* `(' */
	    if (s == NULL)
		goto theend;

	    s1 = alloc_state(mclose, NULL, NULL);   /* `)' */
	    if (s1 == NULL)
		goto theend;
	    patch(e.out, s1);

#ifdef FEAT_MBYTE
	    if (mopen == NFA_COMPOSING)
		/* COMPOSING->out1 = END_COMPOSING */
		patch(list1(&s->out1), s1);
#endif

	    PUSH(frag(s, list1(&s1->out)));
	    break;

	case NFA_BACKREF1:
	case NFA_BACKREF2:
	case NFA_BACKREF3:
	case NFA_BACKREF4:
	case NFA_BACKREF5:
	case NFA_BACKREF6:
	case NFA_BACKREF7:
	case NFA_BACKREF8:
	case NFA_BACKREF9:
#ifdef FEAT_SYN_HL
	case NFA_ZREF1:
	case NFA_ZREF2:
	case NFA_ZREF3:
	case NFA_ZREF4:
	case NFA_ZREF5:
	case NFA_ZREF6:
	case NFA_ZREF7:
	case NFA_ZREF8:
	case NFA_ZREF9:
#endif
	    if (nfa_calc_size == TRUE)
	    {
		nstate += 2;
		break;
	    }
	    s = alloc_state(*p, NULL, NULL);
	    if (s == NULL)
		goto theend;
	    s1 = alloc_state(NFA_SKIP, NULL, NULL);
	    if (s1 == NULL)
		goto theend;
	    patch(list1(&s->out), s1);
	    PUSH(frag(s, list1(&s1->out)));
	    break;

	case NFA_LNUM:
	case NFA_LNUM_GT:
	case NFA_LNUM_LT:
	case NFA_VCOL:
	case NFA_VCOL_GT:
	case NFA_VCOL_LT:
	case NFA_COL:
	case NFA_COL_GT:
	case NFA_COL_LT:
	case NFA_MARK:
	case NFA_MARK_GT:
	case NFA_MARK_LT:
	  {
	    int n = *++p; /* lnum, col or mark name */

	    if (nfa_calc_size == TRUE)
	    {
		nstate += 1;
		break;
	    }
	    s = alloc_state(p[-1], NULL, NULL);
	    if (s == NULL)
		goto theend;
	    s->val = n;
	    PUSH(frag(s, list1(&s->out)));
	    break;
	  }

	case NFA_ZSTART:
	case NFA_ZEND:
	default:
	    /* Operands */
	    if (nfa_calc_size == TRUE)
	    {
		nstate++;
		break;
	    }
	    s = alloc_state(*p, NULL, NULL);
	    if (s == NULL)
		goto theend;
	    PUSH(frag(s, list1(&s->out)));
	    break;

	} /* switch(*p) */

    } /* for(p = postfix; *p; ++p) */

    if (nfa_calc_size == TRUE)
    {
	nstate++;
	goto theend;	/* Return value when counting size is ignored anyway */
    }

    e = POP();
    if (stackp != stack)
	EMSG_RET_NULL(_("E875: (NFA regexp) (While converting from postfix to NFA), too many states left on stack"));

    if (istate >= nstate)
	EMSG_RET_NULL(_("E876: (NFA regexp) Not enough space to store the whole NFA "));

    matchstate = &state_ptr[istate++]; /* the match state */
    matchstate->c = NFA_MATCH;
    matchstate->out = matchstate->out1 = NULL;

    patch(e.out, matchstate);
    ret = e.start;

theend:
    vim_free(stack);
    return ret;

#undef POP1
#undef PUSH1
#undef POP2
#undef PUSH2
#undef POP
#undef PUSH
}

/****************************************************************
 * NFA execution code.
 ****************************************************************/

typedef struct
{
    int	    in_use; /* number of subexpr with useful info */

    /* When REG_MULTI is TRUE list.multi is used, otherwise list.line. */
    union
    {
	struct multipos
	{
	    lpos_T	start;
	    lpos_T	end;
	} multi[NSUBEXP];
	struct linepos
	{
	    char_u	*start;
	    char_u	*end;
	} line[NSUBEXP];
    } list;
} regsub_T;

typedef struct
{
    regsub_T	norm; /* \( .. \) matches */
#ifdef FEAT_SYN_HL
    regsub_T	synt; /* \z( .. \) matches */
#endif
} regsubs_T;

/* nfa_pim_T stores a Postponed Invisible Match. */
typedef struct nfa_pim_S nfa_pim_T;
struct nfa_pim_S
{
    nfa_state_T	*state;
    int		result;		/* NFA_PIM_TODO, NFA_PIM_[NO]MATCH */
    nfa_pim_T	*pim;		/* another PIM at the same position */
    regsubs_T	subs;		/* submatch info, only party used */
};

/* Values for done in nfa_pim_T. */
#define NFA_PIM_TODO    0
#define NFA_PIM_MATCH   1
#define NFA_PIM_NOMATCH -1


/* nfa_thread_T contains execution information of a NFA state */
typedef struct
{
    nfa_state_T	*state;
    int		count;
    nfa_pim_T	*pim;		/* if not NULL: postponed invisible match */
    regsubs_T	subs;		/* submatch info, only party used */
} nfa_thread_T;

/* nfa_list_T contains the alternative NFA execution states. */
typedef struct
{
    nfa_thread_T    *t;		/* allocated array of states */
    int		    n;		/* nr of states currently in "t" */
    int		    len;	/* max nr of states in "t" */
    int		    id;		/* ID of the list */
} nfa_list_T;

#ifdef ENABLE_LOG
static void log_subsexpr __ARGS((regsubs_T *subs));
static void log_subexpr __ARGS((regsub_T *sub));

    static void
log_subsexpr(subs)
    regsubs_T *subs;
{
    log_subexpr(&subs->norm);
# ifdef FEAT_SYN_HL
    log_subexpr(&subs->synt);
# endif
}

    static void
log_subexpr(sub)
    regsub_T *sub;
{
    int j;

    for (j = 0; j < sub->in_use; j++)
	if (REG_MULTI)
	    fprintf(log_fd, "*** group %d, start: c=%d, l=%d, end: c=%d, l=%d\n",
		    j,
		    sub->list.multi[j].start.col,
		    (int)sub->list.multi[j].start.lnum,
		    sub->list.multi[j].end.col,
		    (int)sub->list.multi[j].end.lnum);
	else
	{
	    char *s = (char *)sub->list.line[j].start;
	    char *e = (char *)sub->list.line[j].end;

	    fprintf(log_fd, "*** group %d, start: \"%s\", end: \"%s\"\n",
		    j,
		    s == NULL ? "NULL" : s,
		    e == NULL ? "NULL" : e);
	}
}
#endif

/* Used during execution: whether a match has been found. */
static int nfa_match;

static void clear_sub __ARGS((regsub_T *sub));
static void copy_sub __ARGS((regsub_T *to, regsub_T *from));
static void copy_sub_off __ARGS((regsub_T *to, regsub_T *from));
static int sub_equal __ARGS((regsub_T *sub1, regsub_T *sub2));
static void addstate __ARGS((nfa_list_T *l, nfa_state_T *state, regsubs_T *subs, int off));
static void addstate_here __ARGS((nfa_list_T *l, nfa_state_T *state, regsubs_T *subs, nfa_pim_T *pim, int *ip));

    static void
clear_sub(sub)
    regsub_T *sub;
{
    if (REG_MULTI)
	/* Use 0xff to set lnum to -1 */
	vim_memset(sub->list.multi, 0xff,
				      sizeof(struct multipos) * nfa_nsubexpr);
    else
	vim_memset(sub->list.line, 0, sizeof(struct linepos) * nfa_nsubexpr);
    sub->in_use = 0;
}

/*
 * Copy the submatches from "from" to "to".
 */
    static void
copy_sub(to, from)
    regsub_T	*to;
    regsub_T	*from;
{
    to->in_use = from->in_use;
    if (from->in_use > 0)
    {
	/* Copy the match start and end positions. */
	if (REG_MULTI)
	    mch_memmove(&to->list.multi[0],
			&from->list.multi[0],
			sizeof(struct multipos) * from->in_use);
	else
	    mch_memmove(&to->list.line[0],
			&from->list.line[0],
			sizeof(struct linepos) * from->in_use);
    }
}

/*
 * Like copy_sub() but exclude the main match.
 */
    static void
copy_sub_off(to, from)
    regsub_T	*to;
    regsub_T	*from;
{
    if (to->in_use < from->in_use)
	to->in_use = from->in_use;
    if (from->in_use > 1)
    {
	/* Copy the match start and end positions. */
	if (REG_MULTI)
	    mch_memmove(&to->list.multi[1],
			&from->list.multi[1],
			sizeof(struct multipos) * (from->in_use - 1));
	else
	    mch_memmove(&to->list.line[1],
			&from->list.line[1],
			sizeof(struct linepos) * (from->in_use - 1));
    }
}

/*
 * Return TRUE if "sub1" and "sub2" have the same positions.
 */
    static int
sub_equal(sub1, sub2)
    regsub_T	*sub1;
    regsub_T	*sub2;
{
    int		i;
    int		todo;
    linenr_T	s1, e1;
    linenr_T	s2, e2;
    char_u	*sp1, *ep1;
    char_u	*sp2, *ep2;

    todo = sub1->in_use > sub2->in_use ? sub1->in_use : sub2->in_use;
    if (REG_MULTI)
    {
	for (i = 0; i < todo; ++i)
	{
	    if (i < sub1->in_use)
	    {
		s1 = sub1->list.multi[i].start.lnum;
		e1 = sub1->list.multi[i].end.lnum;
	    }
	    else
	    {
		s1 = 0;
		e1 = 0;
	    }
	    if (i < sub2->in_use)
	    {
		s2 = sub2->list.multi[i].start.lnum;
		e2 = sub2->list.multi[i].end.lnum;
	    }
	    else
	    {
		s2 = 0;
		e2 = 0;
	    }
	    if (s1 != s2 || e1 != e2)
		return FALSE;
	    if (s1 != 0 && sub1->list.multi[i].start.col
					     != sub2->list.multi[i].start.col)
		return FALSE;
	    if (e1 != 0 && sub1->list.multi[i].end.col
					     != sub2->list.multi[i].end.col)
		return FALSE;
	}
    }
    else
    {
	for (i = 0; i < todo; ++i)
	{
	    if (i < sub1->in_use)
	    {
		sp1 = sub1->list.line[i].start;
		ep1 = sub1->list.line[i].end;
	    }
	    else
	    {
		sp1 = NULL;
		ep1 = NULL;
	    }
	    if (i < sub2->in_use)
	    {
		sp2 = sub2->list.line[i].start;
		ep2 = sub2->list.line[i].end;
	    }
	    else
	    {
		sp2 = NULL;
		ep2 = NULL;
	    }
	    if (sp1 != sp2 || ep1 != ep2)
		return FALSE;
	}
    }

    return TRUE;
}

#ifdef ENABLE_LOG
    static void
report_state(char *action, regsub_T *sub, nfa_state_T *state, int lid)
{
    int col;

    if (sub->in_use <= 0)
	col = -1;
    else if (REG_MULTI)
	col = sub->list.multi[0].start.col;
    else
	col = (int)(sub->list.line[0].start - regline);
    nfa_set_code(state->c);
    fprintf(log_fd, "> %s state %d to list %d. char %d: %s (start col %d)\n",
	    action, abs(state->id), lid, state->c, code, col);
}
#endif

    static void
addstate(l, state, subs, off)
    nfa_list_T		*l;	/* runtime state list */
    nfa_state_T		*state;	/* state to update */
    regsubs_T		*subs;	/* pointers to subexpressions */
    int			off;	/* byte offset, when -1 go to next line */
{
    int			subidx;
    nfa_thread_T	*thread;
    lpos_T		save_lpos;
    int			save_in_use;
    char_u		*save_ptr;
    int			i;
    regsub_T		*sub;
#ifdef ENABLE_LOG
    int			did_print = FALSE;
#endif

    if (l == NULL || state == NULL)
	return;

    switch (state->c)
    {
	case NFA_SPLIT:
	case NFA_NOT:
	case NFA_NOPEN:
	case NFA_SKIP_CHAR:
	case NFA_NCLOSE:
	case NFA_MCLOSE:
	case NFA_MCLOSE1:
	case NFA_MCLOSE2:
	case NFA_MCLOSE3:
	case NFA_MCLOSE4:
	case NFA_MCLOSE5:
	case NFA_MCLOSE6:
	case NFA_MCLOSE7:
	case NFA_MCLOSE8:
	case NFA_MCLOSE9:
#ifdef FEAT_SYN_HL
	case NFA_ZCLOSE:
	case NFA_ZCLOSE1:
	case NFA_ZCLOSE2:
	case NFA_ZCLOSE3:
	case NFA_ZCLOSE4:
	case NFA_ZCLOSE5:
	case NFA_ZCLOSE6:
	case NFA_ZCLOSE7:
	case NFA_ZCLOSE8:
	case NFA_ZCLOSE9:
#endif
	case NFA_ZEND:
	    /* These nodes are not added themselves but their "out" and/or
	     * "out1" may be added below.  */
	    break;

	case NFA_MOPEN:
	case NFA_MOPEN1:
	case NFA_MOPEN2:
	case NFA_MOPEN3:
	case NFA_MOPEN4:
	case NFA_MOPEN5:
	case NFA_MOPEN6:
	case NFA_MOPEN7:
	case NFA_MOPEN8:
	case NFA_MOPEN9:
#ifdef FEAT_SYN_HL
	case NFA_ZOPEN:
	case NFA_ZOPEN1:
	case NFA_ZOPEN2:
	case NFA_ZOPEN3:
	case NFA_ZOPEN4:
	case NFA_ZOPEN5:
	case NFA_ZOPEN6:
	case NFA_ZOPEN7:
	case NFA_ZOPEN8:
	case NFA_ZOPEN9:
#endif
	case NFA_ZSTART:
	    /* These nodes do not need to be added, but we need to bail out
	     * when it was tried to be added to this list before. */
	    if (state->lastlist[nfa_ll_index] == l->id)
		goto skip_add;
	    state->lastlist[nfa_ll_index] = l->id;
	    break;

	case NFA_BOL:
	case NFA_BOF:
	    /* "^" won't match past end-of-line, don't bother trying.
	     * Except when we are going to the next line for a look-behind
	     * match. */
	    if (reginput > regline
		    && (nfa_endp == NULL
			|| !REG_MULTI
			|| reglnum == nfa_endp->se_u.pos.lnum))
		goto skip_add;
	    /* FALLTHROUGH */

	default:
	    if (state->lastlist[nfa_ll_index] == l->id)
	    {
		/* This state is already in the list, don't add it again,
		 * unless it is an MOPEN that is used for a backreference. */
		if (!nfa_has_backref)
		{
skip_add:
#ifdef ENABLE_LOG
		    nfa_set_code(state->c);
		    fprintf(log_fd, "> Not adding state %d to list %d. char %d: %s\n",
			    abs(state->id), l->id, state->c, code);
#endif
		    return;
		}

		/* See if the same state is already in the list with the same
		 * positions. */
		for (i = 0; i < l->n; ++i)
		{
		    thread = &l->t[i];
		    if (thread->state->id == state->id
			    && sub_equal(&thread->subs.norm, &subs->norm)
#ifdef FEAT_SYN_HL
			    && (!nfa_has_zsubexpr ||
				   sub_equal(&thread->subs.synt, &subs->synt))
#endif
					  )
			goto skip_add;
		}
	    }

	    /* when there are backreferences or look-behind matches the number
	     * of states may be (a lot) bigger */
	    if (nfa_has_backref && l->n == l->len)
	    {
		int newlen = l->len * 3 / 2 + 50;

		l->t = vim_realloc(l->t, newlen * sizeof(nfa_thread_T));
		l->len = newlen;
	    }

	    /* add the state to the list */
	    state->lastlist[nfa_ll_index] = l->id;
	    thread = &l->t[l->n++];
	    thread->state = state;
	    thread->pim = NULL;
	    copy_sub(&thread->subs.norm, &subs->norm);
#ifdef FEAT_SYN_HL
	    if (nfa_has_zsubexpr)
		copy_sub(&thread->subs.synt, &subs->synt);
#endif
#ifdef ENABLE_LOG
	    report_state("Adding", &thread->subs.norm, state, l->id);
	    did_print = TRUE;
#endif
    }

#ifdef ENABLE_LOG
    if (!did_print)
	report_state("Processing", &subs->norm, state, l->id);
#endif
    switch (state->c)
    {
	case NFA_MATCH:
	    nfa_match = TRUE;
	    break;

	case NFA_SPLIT:
	    /* order matters here */
	    addstate(l, state->out, subs, off);
	    addstate(l, state->out1, subs, off);
	    break;

	case NFA_SKIP_CHAR:
	case NFA_NOPEN:
	case NFA_NCLOSE:
	    addstate(l, state->out, subs, off);
	    break;

	case NFA_MOPEN:
	case NFA_MOPEN1:
	case NFA_MOPEN2:
	case NFA_MOPEN3:
	case NFA_MOPEN4:
	case NFA_MOPEN5:
	case NFA_MOPEN6:
	case NFA_MOPEN7:
	case NFA_MOPEN8:
	case NFA_MOPEN9:
#ifdef FEAT_SYN_HL
	case NFA_ZOPEN:
	case NFA_ZOPEN1:
	case NFA_ZOPEN2:
	case NFA_ZOPEN3:
	case NFA_ZOPEN4:
	case NFA_ZOPEN5:
	case NFA_ZOPEN6:
	case NFA_ZOPEN7:
	case NFA_ZOPEN8:
	case NFA_ZOPEN9:
#endif
	case NFA_ZSTART:
	    if (state->c == NFA_ZSTART)
	    {
		subidx = 0;
		sub = &subs->norm;
	    }
#ifdef FEAT_SYN_HL
	    else if (state->c >= NFA_ZOPEN)
	    {
		subidx = state->c - NFA_ZOPEN;
		sub = &subs->synt;
	    }
#endif
	    else
	    {
		subidx = state->c - NFA_MOPEN;
		sub = &subs->norm;
	    }

	    /* Set the position (with "off") in the subexpression.  Save and
	     * restore it when it was in use.  Otherwise fill any gap. */
	    save_ptr = NULL;
	    if (REG_MULTI)
	    {
		if (subidx < sub->in_use)
		{
		    save_lpos = sub->list.multi[subidx].start;
		    save_in_use = -1;
		}
		else
		{
		    save_in_use = sub->in_use;
		    for (i = sub->in_use; i < subidx; ++i)
		    {
			sub->list.multi[i].start.lnum = -1;
			sub->list.multi[i].end.lnum = -1;
		    }
		    sub->in_use = subidx + 1;
		}
		if (off == -1)
		{
		    sub->list.multi[subidx].start.lnum = reglnum + 1;
		    sub->list.multi[subidx].start.col = 0;
		}
		else
		{
		    sub->list.multi[subidx].start.lnum = reglnum;
		    sub->list.multi[subidx].start.col =
					  (colnr_T)(reginput - regline + off);
		}
	    }
	    else
	    {
		if (subidx < sub->in_use)
		{
		    save_ptr = sub->list.line[subidx].start;
		    save_in_use = -1;
		}
		else
		{
		    save_in_use = sub->in_use;
		    for (i = sub->in_use; i < subidx; ++i)
		    {
			sub->list.line[i].start = NULL;
			sub->list.line[i].end = NULL;
		    }
		    sub->in_use = subidx + 1;
		}
		sub->list.line[subidx].start = reginput + off;
	    }

	    addstate(l, state->out, subs, off);

	    if (save_in_use == -1)
	    {
		if (REG_MULTI)
		    sub->list.multi[subidx].start = save_lpos;
		else
		    sub->list.line[subidx].start = save_ptr;
	    }
	    else
		sub->in_use = save_in_use;
	    break;

	case NFA_MCLOSE:
	    if (nfa_has_zend)
	    {
		/* Do not overwrite the position set by \ze. If no \ze
		 * encountered end will be set in nfa_regtry(). */
		addstate(l, state->out, subs, off);
		break;
	    }
	case NFA_MCLOSE1:
	case NFA_MCLOSE2:
	case NFA_MCLOSE3:
	case NFA_MCLOSE4:
	case NFA_MCLOSE5:
	case NFA_MCLOSE6:
	case NFA_MCLOSE7:
	case NFA_MCLOSE8:
	case NFA_MCLOSE9:
#ifdef FEAT_SYN_HL
	case NFA_ZCLOSE:
	case NFA_ZCLOSE1:
	case NFA_ZCLOSE2:
	case NFA_ZCLOSE3:
	case NFA_ZCLOSE4:
	case NFA_ZCLOSE5:
	case NFA_ZCLOSE6:
	case NFA_ZCLOSE7:
	case NFA_ZCLOSE8:
	case NFA_ZCLOSE9:
#endif
	case NFA_ZEND:
	    if (state->c == NFA_ZEND)
	    {
		subidx = 0;
		sub = &subs->norm;
	    }
#ifdef FEAT_SYN_HL
	    else if (state->c >= NFA_ZCLOSE)
	    {
		subidx = state->c - NFA_ZCLOSE;
		sub = &subs->synt;
	    }
#endif
	    else
	    {
		subidx = state->c - NFA_MCLOSE;
		sub = &subs->norm;
	    }

	    /* We don't fill in gaps here, there must have been an MOPEN that
	     * has done that. */
	    save_in_use = sub->in_use;
	    if (sub->in_use <= subidx)
		sub->in_use = subidx + 1;
	    if (REG_MULTI)
	    {
		save_lpos = sub->list.multi[subidx].end;
		if (off == -1)
		{
		    sub->list.multi[subidx].end.lnum = reglnum + 1;
		    sub->list.multi[subidx].end.col = 0;
		}
		else
		{
		    sub->list.multi[subidx].end.lnum = reglnum;
		    sub->list.multi[subidx].end.col =
					  (colnr_T)(reginput - regline + off);
		}
	    }
	    else
	    {
		save_ptr = sub->list.line[subidx].end;
		sub->list.line[subidx].end = reginput + off;
	    }

	    addstate(l, state->out, subs, off);

	    if (REG_MULTI)
		sub->list.multi[subidx].end = save_lpos;
	    else
		sub->list.line[subidx].end = save_ptr;
	    sub->in_use = save_in_use;
	    break;
    }
}

/*
 * Like addstate(), but the new state(s) are put at position "*ip".
 * Used for zero-width matches, next state to use is the added one.
 * This makes sure the order of states to be tried does not change, which
 * matters for alternatives.
 */
    static void
addstate_here(l, state, subs, pim, ip)
    nfa_list_T		*l;	/* runtime state list */
    nfa_state_T		*state;	/* state to update */
    regsubs_T		*subs;	/* pointers to subexpressions */
    nfa_pim_T		*pim;   /* postponed look-behind match */
    int			*ip;
{
    int tlen = l->n;
    int count;
    int listidx = *ip;
    int i;

    /* first add the state(s) at the end, so that we know how many there are */
    addstate(l, state, subs, 0);

    /* fill in the "pim" field in the new states */
    if (pim != NULL)
	for (i = tlen; i < l->n; ++i)
	    l->t[i].pim = pim;

    /* when "*ip" was at the end of the list, nothing to do */
    if (listidx + 1 == tlen)
	return;

    /* re-order to put the new state at the current position */
    count = l->n - tlen;
    if (count == 1)
    {
	/* overwrite the current state */
	l->t[listidx] = l->t[l->n - 1];
    }
    else if (count > 1)
    {
	/* make space for new states, then move them from the
	 * end to the current position */
	mch_memmove(&(l->t[listidx + count]),
		&(l->t[listidx + 1]),
		sizeof(nfa_thread_T) * (l->n - listidx - 1));
	mch_memmove(&(l->t[listidx]),
		&(l->t[l->n - 1]),
		sizeof(nfa_thread_T) * count);
    }
    --l->n;
    *ip = listidx - 1;
}

/*
 * Check character class "class" against current character c.
 */
    static int
check_char_class(class, c)
    int		class;
    int		c;
{
    switch (class)
    {
	case NFA_CLASS_ALNUM:
	    if (c >= 1 && c <= 255 && isalnum(c))
		return OK;
	    break;
	case NFA_CLASS_ALPHA:
	    if (c >= 1 && c <= 255 && isalpha(c))
		return OK;
	    break;
	case NFA_CLASS_BLANK:
	    if (c == ' ' || c == '\t')
		return OK;
	    break;
	case NFA_CLASS_CNTRL:
	    if (c >= 1 && c <= 255 && iscntrl(c))
		return OK;
	    break;
	case NFA_CLASS_DIGIT:
	    if (VIM_ISDIGIT(c))
		return OK;
	    break;
	case NFA_CLASS_GRAPH:
	    if (c >= 1 && c <= 255 && isgraph(c))
		return OK;
	    break;
	case NFA_CLASS_LOWER:
	    if (MB_ISLOWER(c))
		return OK;
	    break;
	case NFA_CLASS_PRINT:
	    if (vim_isprintc(c))
		return OK;
	    break;
	case NFA_CLASS_PUNCT:
	    if (c >= 1 && c <= 255 && ispunct(c))
		return OK;
	    break;
	case NFA_CLASS_SPACE:
	    if ((c >=9 && c <= 13) || (c == ' '))
		return OK;
	    break;
	case NFA_CLASS_UPPER:
	    if (MB_ISUPPER(c))
		return OK;
	    break;
	case NFA_CLASS_XDIGIT:
	    if (vim_isxdigit(c))
		return OK;
	    break;
	case NFA_CLASS_TAB:
	    if (c == '\t')
		return OK;
	    break;
	case NFA_CLASS_RETURN:
	    if (c == '\r')
		return OK;
	    break;
	case NFA_CLASS_BACKSPACE:
	    if (c == '\b')
		return OK;
	    break;
	case NFA_CLASS_ESCAPE:
	    if (c == '\033')
		return OK;
	    break;

	default:
	    /* should not be here :P */
	    EMSG_RET_FAIL(_("E877: (NFA regexp) Invalid character class "));
    }
    return FAIL;
}

static int match_backref __ARGS((regsub_T *sub, int subidx, int *bytelen));

/*
 * Check for a match with subexpression "subidx".
 * Return TRUE if it matches.
 */
    static int
match_backref(sub, subidx, bytelen)
    regsub_T	*sub;	    /* pointers to subexpressions */
    int		subidx;
    int		*bytelen;   /* out: length of match in bytes */
{
    int		len;

    if (sub->in_use <= subidx)
    {
retempty:
	/* backref was not set, match an empty string */
	*bytelen = 0;
	return TRUE;
    }

    if (REG_MULTI)
    {
	if (sub->list.multi[subidx].start.lnum < 0
				       || sub->list.multi[subidx].end.lnum < 0)
	    goto retempty;
	/* TODO: line breaks */
	len = sub->list.multi[subidx].end.col
					 - sub->list.multi[subidx].start.col;
	if (cstrncmp(regline + sub->list.multi[subidx].start.col,
							reginput, &len) == 0)
	{
	    *bytelen = len;
	    return TRUE;
	}
    }
    else
    {
	if (sub->list.line[subidx].start == NULL
					|| sub->list.line[subidx].end == NULL)
	    goto retempty;
	len = (int)(sub->list.line[subidx].end - sub->list.line[subidx].start);
	if (cstrncmp(sub->list.line[subidx].start, reginput, &len) == 0)
	{
	    *bytelen = len;
	    return TRUE;
	}
    }
    return FALSE;
}

#ifdef FEAT_SYN_HL

static int match_zref __ARGS((int subidx, int *bytelen));

/*
 * Check for a match with \z subexpression "subidx".
 * Return TRUE if it matches.
 */
    static int
match_zref(subidx, bytelen)
    int		subidx;
    int		*bytelen;   /* out: length of match in bytes */
{
    int		len;

    cleanup_zsubexpr();
    if (re_extmatch_in == NULL || re_extmatch_in->matches[subidx] == NULL)
    {
	/* backref was not set, match an empty string */
	*bytelen = 0;
	return TRUE;
    }

    len = (int)STRLEN(re_extmatch_in->matches[subidx]);
    if (cstrncmp(re_extmatch_in->matches[subidx], reginput, &len) == 0)
    {
	*bytelen = len;
	return TRUE;
    }
    return FALSE;
}
#endif

/*
 * Save list IDs for all NFA states of "prog" into "list".
 * Also reset the IDs to zero.
 * Only used for the recursive value lastlist[1].
 */
    static void
nfa_save_listids(prog, list)
    nfa_regprog_T   *prog;
    int		    *list;
{
    int		    i;
    nfa_state_T	    *p;

    /* Order in the list is reverse, it's a bit faster that way. */
    p = &prog->state[0];
    for (i = prog->nstate; --i >= 0; )
    {
	list[i] = p->lastlist[1];
	p->lastlist[1] = 0;
	++p;
    }
}

/*
 * Restore list IDs from "list" to all NFA states.
 */
    static void
nfa_restore_listids(prog, list)
    nfa_regprog_T   *prog;
    int		    *list;
{
    int		    i;
    nfa_state_T	    *p;

    p = &prog->state[0];
    for (i = prog->nstate; --i >= 0; )
    {
	p->lastlist[1] = list[i];
	++p;
    }
}

    static int
nfa_re_num_cmp(val, op, pos)
    long_u	val;
    int		op;
    long_u	pos;
{
    if (op == 1) return pos > val;
    if (op == 2) return pos < val;
    return val == pos;
}

static int recursive_regmatch __ARGS((nfa_state_T *state, nfa_regprog_T *prog, regsubs_T *submatch, regsubs_T *m, int **listids));
static int nfa_regmatch __ARGS((nfa_regprog_T *prog, nfa_state_T *start, regsubs_T *submatch, regsubs_T *m));

/*
 * Recursively call nfa_regmatch()
 */
    static int
recursive_regmatch(state, prog, submatch, m, listids)
    nfa_state_T	    *state;
    nfa_regprog_T   *prog;
    regsubs_T	    *submatch;
    regsubs_T	    *m;
    int		    **listids;
{
    char_u	*save_reginput = reginput;
    char_u	*save_regline = regline;
    int		save_reglnum = reglnum;
    int		save_nfa_match = nfa_match;
    int		save_nfa_listid = nfa_listid;
    save_se_T   *save_nfa_endp = nfa_endp;
    save_se_T   endpos;
    save_se_T   *endposp = NULL;
    int		result;
    int		need_restore = FALSE;

    if (state->c == NFA_START_INVISIBLE_BEFORE)
    {
	/* The recursive match must end at the current position. */
	endposp = &endpos;
	if (REG_MULTI)
	{
	    endpos.se_u.pos.col = (int)(reginput - regline);
	    endpos.se_u.pos.lnum = reglnum;
	}
	else
	    endpos.se_u.ptr = reginput;

	/* Go back the specified number of bytes, or as far as the
	 * start of the previous line, to try matching "\@<=" or
	 * not matching "\@<!".
	 * TODO: This is very inefficient! Would be better to
	 * first check for a match with what follows. */
	if (state->val <= 0)
	{
	    if (REG_MULTI)
	    {
		regline = reg_getline(--reglnum);
		if (regline == NULL)
		    /* can't go before the first line */
		    regline = reg_getline(++reglnum);
	    }
	    reginput = regline;
	}
	else
	{
	    if (REG_MULTI && (int)(reginput - regline) < state->val)
	    {
		/* Not enough bytes in this line, go to end of
		 * previous line. */
		regline = reg_getline(--reglnum);
		if (regline == NULL)
		{
		    /* can't go before the first line */
		    regline = reg_getline(++reglnum);
		    reginput = regline;
		}
		else
		    reginput = regline + STRLEN(regline);
	    }
	    if ((int)(reginput - regline) >= state->val)
	    {
		reginput -= state->val;
#ifdef FEAT_MBYTE
		if (has_mbyte)
		    reginput -= mb_head_off(regline, reginput);
#endif
	    }
	    else
		reginput = regline;
	}
    }

#ifdef ENABLE_LOG
    if (log_fd != stderr)
	fclose(log_fd);
    log_fd = NULL;
#endif
    /* Have to clear the lastlist field of the NFA nodes, so that
     * nfa_regmatch() and addstate() can run properly after recursion. */
    if (nfa_ll_index == 1)
    {
	/* Already calling nfa_regmatch() recursively.  Save the lastlist[1]
	 * values and clear them. */
	if (*listids == NULL)
	{
	    *listids = (int *)lalloc(sizeof(int) * nstate, TRUE);
	    if (*listids == NULL)
	    {
		EMSG(_("E878: (NFA) Could not allocate memory for branch traversal!"));
		return 0;
	    }
	}
	nfa_save_listids(prog, *listids);
	need_restore = TRUE;
	/* any value of nfa_listid will do */
    }
    else
    {
	/* First recursive nfa_regmatch() call, switch to the second lastlist
	 * entry.  Make sure nfa_listid is different from a previous recursive
	 * call, because some states may still have this ID. */
	++nfa_ll_index;
	if (nfa_listid <= nfa_alt_listid)
	    nfa_listid = nfa_alt_listid;
    }

    /* Call nfa_regmatch() to check if the current concat matches at this
     * position. The concat ends with the node NFA_END_INVISIBLE */
    nfa_endp = endposp;
    result = nfa_regmatch(prog, state->out, submatch, m);

    if (need_restore)
	nfa_restore_listids(prog, *listids);
    else
    {
	--nfa_ll_index;
	nfa_alt_listid = nfa_listid;
    }

    /* restore position in input text */
    reginput = save_reginput;
    regline = save_regline;
    reglnum = save_reglnum;
    nfa_match = save_nfa_match;
    nfa_endp = save_nfa_endp;
    nfa_listid = save_nfa_listid;

#ifdef ENABLE_LOG
    log_fd = fopen(NFA_REGEXP_RUN_LOG, "a");
    if (log_fd != NULL)
    {
	fprintf(log_fd, "****************************\n");
	fprintf(log_fd, "FINISHED RUNNING nfa_regmatch() recursively\n");
	fprintf(log_fd, "MATCH = %s\n", result == TRUE ? "OK" : "FALSE");
	fprintf(log_fd, "****************************\n");
    }
    else
    {
	EMSG(_("Could not open temporary log file for writing, displaying on stderr ... "));
	log_fd = stderr;
    }
#endif

    return result;
}

static int failure_chance __ARGS((nfa_state_T *state, int depth));

/*
 * Estimate the chance of a match with "state" failing.
 * NFA_ANY: 1
 * specific character: 99
 */
    static int
failure_chance(state, depth)
    nfa_state_T *state;
    int		depth;
{
    int c = state->c;
    int l, r;

    /* detect looping */
    if (depth > 4)
	return 1;

    switch (c)
    {
	case NFA_SPLIT:
	    if (state->out->c == NFA_SPLIT || state->out1->c == NFA_SPLIT)
		/* avoid recursive stuff */
		return 1;
	    /* two alternatives, use the lowest failure chance */
	    l = failure_chance(state->out, depth + 1);
	    r = failure_chance(state->out1, depth + 1);
	    return l < r ? l : r;

	case NFA_ANY:
	    /* matches anything, unlikely to fail */
	    return 1;
	case NFA_MATCH:
	    /* empty match works always */
	    return 0;

	case NFA_BOL:
	case NFA_EOL:
	case NFA_BOF:
	case NFA_EOF:
	case NFA_NEWL:
	    return 99;

	case NFA_BOW:
	case NFA_EOW:
	    return 90;

	case NFA_MOPEN:
	case NFA_MOPEN1:
	case NFA_MOPEN2:
	case NFA_MOPEN3:
	case NFA_MOPEN4:
	case NFA_MOPEN5:
	case NFA_MOPEN6:
	case NFA_MOPEN7:
	case NFA_MOPEN8:
	case NFA_MOPEN9:
#ifdef FEAT_SYN_HL
	case NFA_ZOPEN:
	case NFA_ZOPEN1:
	case NFA_ZOPEN2:
	case NFA_ZOPEN3:
	case NFA_ZOPEN4:
	case NFA_ZOPEN5:
	case NFA_ZOPEN6:
	case NFA_ZOPEN7:
	case NFA_ZOPEN8:
	case NFA_ZOPEN9:
	case NFA_ZCLOSE:
	case NFA_ZCLOSE1:
	case NFA_ZCLOSE2:
	case NFA_ZCLOSE3:
	case NFA_ZCLOSE4:
	case NFA_ZCLOSE5:
	case NFA_ZCLOSE6:
	case NFA_ZCLOSE7:
	case NFA_ZCLOSE8:
	case NFA_ZCLOSE9:
#endif
	case NFA_NOPEN:
	case NFA_MCLOSE:
	case NFA_MCLOSE1:
	case NFA_MCLOSE2:
	case NFA_MCLOSE3:
	case NFA_MCLOSE4:
	case NFA_MCLOSE5:
	case NFA_MCLOSE6:
	case NFA_MCLOSE7:
	case NFA_MCLOSE8:
	case NFA_MCLOSE9:
	case NFA_NCLOSE:
	    return failure_chance(state->out, depth + 1);

	case NFA_BACKREF1:
	case NFA_BACKREF2:
	case NFA_BACKREF3:
	case NFA_BACKREF4:
	case NFA_BACKREF5:
	case NFA_BACKREF6:
	case NFA_BACKREF7:
	case NFA_BACKREF8:
	case NFA_BACKREF9:
#ifdef FEAT_SYN_HL
	case NFA_ZREF1:
	case NFA_ZREF2:
	case NFA_ZREF3:
	case NFA_ZREF4:
	case NFA_ZREF5:
	case NFA_ZREF6:
	case NFA_ZREF7:
	case NFA_ZREF8:
	case NFA_ZREF9:
#endif
	    /* backreferences don't match in many places */
	    return 94;

	case NFA_LNUM_GT:
	case NFA_LNUM_LT:
	case NFA_COL_GT:
	case NFA_COL_LT:
	case NFA_VCOL_GT:
	case NFA_VCOL_LT:
	case NFA_MARK_GT:
	case NFA_MARK_LT:
	case NFA_VISUAL:
	    /* before/after positions don't match very often */
	    return 85;

	case NFA_LNUM:
	    return 90;

	case NFA_CURSOR:
	case NFA_COL:
	case NFA_VCOL:
	case NFA_MARK:
	    /* specific positions rarely match */
	    return 98;

	case NFA_COMPOSING:
	    return 95;

	default:
	    if (c > 0)
		/* character match fails often */
		return 95;
    }

    /* something else, includes character classes */
    return 50;
}

/*
 * Main matching routine.
 *
 * Run NFA to determine whether it matches reginput.
 *
 * When "nfa_endp" is not NULL it is a required end-of-match position.
 *
 * Return TRUE if there is a match, FALSE otherwise.
 * Note: Caller must ensure that: start != NULL.
 */
    static int
nfa_regmatch(prog, start, submatch, m)
    nfa_regprog_T	*prog;
    nfa_state_T		*start;
    regsubs_T		*submatch;
    regsubs_T		*m;
{
    int		result;
    int		size = 0;
    int		flag = 0;
    int		go_to_nextline = FALSE;
    nfa_thread_T *t;
    nfa_list_T	list[3];
    nfa_list_T	*listtbl[2][2];
    nfa_list_T	*ll;
    int		listidx;
    nfa_list_T	*thislist;
    nfa_list_T	*nextlist;
    nfa_list_T	*neglist;
    int		*listids = NULL;
    nfa_state_T *add_state;
    int		 add_count;
    int		 add_off;
    garray_T	pimlist;
#ifdef NFA_REGEXP_DEBUG_LOG
    FILE	*debug = fopen(NFA_REGEXP_DEBUG_LOG, "a");

    if (debug == NULL)
    {
	EMSG2(_("(NFA) COULD NOT OPEN %s !"), NFA_REGEXP_DEBUG_LOG);
	return FALSE;
    }
#endif
    nfa_match = FALSE;
    ga_init2(&pimlist, sizeof(nfa_pim_T), 5);

    /* Allocate memory for the lists of nodes. */
    size = (nstate + 1) * sizeof(nfa_thread_T);
    list[0].t = (nfa_thread_T *)lalloc_clear(size, TRUE);
    list[0].len = nstate + 1;
    list[1].t = (nfa_thread_T *)lalloc_clear(size, TRUE);
    list[1].len = nstate + 1;
    list[2].t = (nfa_thread_T *)lalloc_clear(size, TRUE);
    list[2].len = nstate + 1;
    if (list[0].t == NULL || list[1].t == NULL || list[2].t == NULL)
	goto theend;

#ifdef ENABLE_LOG
    log_fd = fopen(NFA_REGEXP_RUN_LOG, "a");
    if (log_fd != NULL)
    {
	fprintf(log_fd, "**********************************\n");
	nfa_set_code(start->c);
	fprintf(log_fd, " RUNNING nfa_regmatch() starting with state %d, code %s\n",
	abs(start->id), code);
	fprintf(log_fd, "**********************************\n");
    }
    else
    {
	EMSG(_("Could not open temporary log file for writing, displaying on stderr ... "));
	log_fd = stderr;
    }
#endif

    thislist = &list[0];
    thislist->n = 0;
    nextlist = &list[1];
    nextlist->n = 0;
    neglist = &list[2];
    neglist->n = 0;
#ifdef ENABLE_LOG
    fprintf(log_fd, "(---) STARTSTATE\n");
#endif
    thislist->id = nfa_listid + 1;
    addstate(thislist, start, m, 0);

    /* There are two cases when the NFA advances: 1. input char matches the
     * NFA node and 2. input char does not match the NFA node, but the next
     * node is NFA_NOT. The following macro calls addstate() according to
     * these rules. It is used A LOT, so use the "listtbl" table for speed */
    listtbl[0][0] = NULL;
    listtbl[0][1] = neglist;
    listtbl[1][0] = nextlist;
    listtbl[1][1] = NULL;
#define	ADD_POS_NEG_STATE(state)			\
    ll = listtbl[result ? 1 : 0][state->negated];	\
    if (ll != NULL) {					\
	add_state = state->out;				\
	add_off = clen;					\
    }

    /*
     * Run for each character.
     */
    for (;;)
    {
	int	curc;
	int	clen;

#ifdef FEAT_MBYTE
	if (has_mbyte)
	{
	    curc = (*mb_ptr2char)(reginput);
	    clen = (*mb_ptr2len)(reginput);
	}
	else
#endif
	{
	    curc = *reginput;
	    clen = 1;
	}
	if (curc == NUL)
	{
	    clen = 0;
	    go_to_nextline = FALSE;
	}

	/* swap lists */
	thislist = &list[flag];
	nextlist = &list[flag ^= 1];
	nextlist->n = 0;	    /* clear nextlist */
	listtbl[1][0] = nextlist;
	++nfa_listid;
	thislist->id = nfa_listid;
	nextlist->id = nfa_listid + 1;
	neglist->id = nfa_listid + 1;

	pimlist.ga_len = 0;

#ifdef ENABLE_LOG
	fprintf(log_fd, "------------------------------------------\n");
	fprintf(log_fd, ">>> Reginput is \"%s\"\n", reginput);
	fprintf(log_fd, ">>> Advanced one character ... Current char is %c (code %d) \n", curc, (int)curc);
	fprintf(log_fd, ">>> Thislist has %d states available: ", thislist->n);
	{
	    int i;

	    for (i = 0; i < thislist->n; i++)
		fprintf(log_fd, "%d  ", abs(thislist->t[i].state->id));
	}
	fprintf(log_fd, "\n");
#endif

#ifdef NFA_REGEXP_DEBUG_LOG
	fprintf(debug, "\n-------------------\n");
#endif
	/*
	 * If the state lists are empty we can stop.
	 */
	if (thislist->n == 0 && neglist->n == 0)
	    break;

	/* compute nextlist */
	for (listidx = 0; listidx < thislist->n || neglist->n > 0; ++listidx)
	{
	    if (neglist->n > 0)
	    {
		t = &neglist->t[0];
		neglist->n--;
		listidx--;
	    }
	    else
		t = &thislist->t[listidx];

#ifdef NFA_REGEXP_DEBUG_LOG
	    nfa_set_code(t->state->c);
	    fprintf(debug, "%s, ", code);
#endif
#ifdef ENABLE_LOG
	    {
		int col;

		if (t->subs.norm.in_use <= 0)
		    col = -1;
		else if (REG_MULTI)
		    col = t->subs.norm.list.multi[0].start.col;
		else
		    col = (int)(t->subs.norm.list.line[0].start - regline);
		nfa_set_code(t->state->c);
		fprintf(log_fd, "(%d) char %d %s (start col %d) ... \n",
			abs(t->state->id), (int)t->state->c, code, col);
	    }
#endif

	    /*
	     * Handle the possible codes of the current state.
	     * The most important is NFA_MATCH.
	     */
	    add_state = NULL;
	    add_count = 0;
	    switch (t->state->c)
	    {
	    case NFA_MATCH:
	      {
		nfa_match = TRUE;
		copy_sub(&submatch->norm, &t->subs.norm);
#ifdef FEAT_SYN_HL
		if (nfa_has_zsubexpr)
		    copy_sub(&submatch->synt, &t->subs.synt);
#endif
#ifdef ENABLE_LOG
		log_subsexpr(&t->subs);
#endif
		/* Found the left-most longest match, do not look at any other
		 * states at this position.  When the list of states is going
		 * to be empty quit without advancing, so that "reginput" is
		 * correct. */
		if (nextlist->n == 0 && neglist->n == 0)
		    clen = 0;
		goto nextchar;
	      }

	    case NFA_END_INVISIBLE:
	    case NFA_END_PATTERN:
		/*
		 * This is only encountered after a NFA_START_INVISIBLE or
		 * NFA_START_INVISIBLE_BEFORE node.
		 * They surround a zero-width group, used with "\@=", "\&",
		 * "\@!", "\@<=" and "\@<!".
		 * If we got here, it means that the current "invisible" group
		 * finished successfully, so return control to the parent
		 * nfa_regmatch().  For a look-behind match only when it ends
		 * in the position in "nfa_endp".
		 * Submatches are stored in *m, and used in the parent call.
		 */
#ifdef ENABLE_LOG
		if (nfa_endp != NULL)
		{
		    if (REG_MULTI)
			fprintf(log_fd, "Current lnum: %d, endp lnum: %d; current col: %d, endp col: %d\n",
				(int)reglnum,
				(int)nfa_endp->se_u.pos.lnum,
				(int)(reginput - regline),
				nfa_endp->se_u.pos.col);
		    else
			fprintf(log_fd, "Current col: %d, endp col: %d\n",
				(int)(reginput - regline),
				(int)(nfa_endp->se_u.ptr - reginput));
		}
#endif
		/* If "nfa_endp" is set it's only a match if it ends at
		 * "nfa_endp" */
		if (nfa_endp != NULL && (REG_MULTI
			? (reglnum != nfa_endp->se_u.pos.lnum
			    || (int)(reginput - regline)
						!= nfa_endp->se_u.pos.col)
			: reginput != nfa_endp->se_u.ptr))
		    break;

		/* do not set submatches for \@! */
		if (!t->state->negated)
		{
		    copy_sub(&m->norm, &t->subs.norm);
#ifdef FEAT_SYN_HL
		    if (nfa_has_zsubexpr)
			copy_sub(&m->synt, &t->subs.synt);
#endif
		}
#ifdef ENABLE_LOG
		fprintf(log_fd, "Match found:\n");
		log_subsexpr(m);
#endif
		nfa_match = TRUE;
		break;

	    case NFA_START_INVISIBLE:
	    case NFA_START_INVISIBLE_BEFORE:
		{
		    nfa_pim_T *pim;
		    int cout = t->state->out1->out->c;

		    /* Do it directly when what follows is possibly end of
		     * match (closing paren).
		     * Postpone when it is \@<= or \@<!, these are expensive.
		     * TODO: remove the check for t->pim and check multiple
		     * where it's used?
		     * Otherwise first do the one that has the highest chance
		     * of failing. */
		    if ((cout >= NFA_MCLOSE && cout <= NFA_MCLOSE9)
#ifdef FEAT_SYN_HL
			    || (cout >= NFA_ZCLOSE && cout <= NFA_ZCLOSE9)
#endif
			    || cout == NFA_NCLOSE
			    || t->pim != NULL
			    || (t->state->c != NFA_START_INVISIBLE_BEFORE
				&& failure_chance(t->state->out1->out, 0)
					  < failure_chance(t->state->out, 0)))
		    {
			/*
			 * First try matching the invisible match, then what
			 * follows.
			 */
			result = recursive_regmatch(t->state, prog,
						       submatch, m, &listids);

			/* for \@! it is a match when result is FALSE */
			if (result != t->state->negated)
			{
			    /* Copy submatch info from the recursive call */
			    copy_sub_off(&t->subs.norm, &m->norm);
#ifdef FEAT_SYN_HL
			    copy_sub_off(&t->subs.synt, &m->synt);
#endif

			    /* t->state->out1 is the corresponding
			     * END_INVISIBLE node; Add its out to the current
			     * list (zero-width match). */
			    addstate_here(thislist, t->state->out1->out,
						  &t->subs, t->pim, &listidx);
			}
		    }
		    else
		    {
			/*
			 * First try matching what follows at the current
			 * position.  Only if a match is found, addstate() is
			 * called, then verify the invisible match matches.
			 * Add a nfa_pim_T to the following states, it
			 * contains info about the invisible match.
			 */
			if (ga_grow(&pimlist, 1) == FAIL)
			    goto theend;
			pim = (nfa_pim_T *)pimlist.ga_data + pimlist.ga_len;
			++pimlist.ga_len;
			pim->state = t->state;
			pim->pim = NULL;
			pim->result = NFA_PIM_TODO;

			/* t->state->out1 is the corresponding END_INVISIBLE
			 * node; Add its out to the current list (zero-width
			 * match). */
			addstate_here(thislist, t->state->out1->out, &t->subs,
							       pim, &listidx);
		    }
		}
		break;

	    case NFA_START_PATTERN:
		/* First try matching the pattern. */
		result = recursive_regmatch(t->state, prog,
						       submatch, m, &listids);
		if (result)
		{
		    int bytelen;

#ifdef ENABLE_LOG
		    fprintf(log_fd, "NFA_START_PATTERN matches:\n");
		    log_subsexpr(m);
#endif
		    /* Copy submatch info from the recursive call */
		    copy_sub_off(&t->subs.norm, &m->norm);
#ifdef FEAT_SYN_HL
		    copy_sub_off(&t->subs.synt, &m->synt);
#endif
		    /* Now we need to skip over the matched text and then
		     * continue with what follows. */
		    if (REG_MULTI)
			/* TODO: multi-line match */
			bytelen = m->norm.list.multi[0].end.col
						  - (int)(reginput - regline);
		    else
			bytelen = (int)(m->norm.list.line[0].end - reginput);

#ifdef ENABLE_LOG
		    fprintf(log_fd, "NFA_START_PATTERN length: %d\n", bytelen);
#endif
		    if (bytelen == 0)
		    {
			/* empty match, output of corresponding
			 * NFA_END_PATTERN/NFA_SKIP to be used at current
			 * position */
			addstate_here(thislist, t->state->out1->out->out,
						  &t->subs, t->pim, &listidx);
		    }
		    else if (bytelen <= clen)
		    {
			/* match current character, output of corresponding
			 * NFA_END_PATTERN to be used at next position. */
			ll = nextlist;
			add_state = t->state->out1->out->out;
			add_off = clen;
		    }
		    else
		    {
			/* skip over the matched characters, set character
			 * count in NFA_SKIP */
			ll = nextlist;
			add_state = t->state->out1->out;
			add_off = bytelen;
			add_count = bytelen - clen;
		    }
		}
		break;

	    case NFA_BOL:
		if (reginput == regline)
		    addstate_here(thislist, t->state->out, &t->subs,
							    t->pim, &listidx);
		break;

	    case NFA_EOL:
		if (curc == NUL)
		    addstate_here(thislist, t->state->out, &t->subs,
							    t->pim, &listidx);
		break;

	    case NFA_BOW:
	    {
		int bow = TRUE;

		if (curc == NUL)
		    bow = FALSE;
#ifdef FEAT_MBYTE
		else if (has_mbyte)
		{
		    int this_class;

		    /* Get class of current and previous char (if it exists). */
		    this_class = mb_get_class_buf(reginput, reg_buf);
		    if (this_class <= 1)
			bow = FALSE;
		    else if (reg_prev_class() == this_class)
			bow = FALSE;
		}
#endif
		else if (!vim_iswordc_buf(curc, reg_buf)
			   || (reginput > regline
				   && vim_iswordc_buf(reginput[-1], reg_buf)))
		    bow = FALSE;
		if (bow)
		    addstate_here(thislist, t->state->out, &t->subs,
							    t->pim, &listidx);
		break;
	    }

	    case NFA_EOW:
	    {
		int eow = TRUE;

		if (reginput == regline)
		    eow = FALSE;
#ifdef FEAT_MBYTE
		else if (has_mbyte)
		{
		    int this_class, prev_class;

		    /* Get class of current and previous char (if it exists). */
		    this_class = mb_get_class_buf(reginput, reg_buf);
		    prev_class = reg_prev_class();
		    if (this_class == prev_class
					|| prev_class == 0 || prev_class == 1)
			eow = FALSE;
		}
#endif
		else if (!vim_iswordc_buf(reginput[-1], reg_buf)
			|| (reginput[0] != NUL
					   && vim_iswordc_buf(curc, reg_buf)))
		    eow = FALSE;
		if (eow)
		    addstate_here(thislist, t->state->out, &t->subs,
							    t->pim, &listidx);
		break;
	    }

	    case NFA_BOF:
		if (reglnum == 0 && reginput == regline
					&& (!REG_MULTI || reg_firstlnum == 1))
		    addstate_here(thislist, t->state->out, &t->subs,
							    t->pim, &listidx);
		break;

	    case NFA_EOF:
		if (reglnum == reg_maxline && curc == NUL)
		    addstate_here(thislist, t->state->out, &t->subs,
							    t->pim, &listidx);
		break;

#ifdef FEAT_MBYTE
	    case NFA_COMPOSING:
	    {
		int	    mc = curc;
		int	    len = 0;
		nfa_state_T *end;
		nfa_state_T *sta;
		int	    cchars[MAX_MCO];
		int	    ccount = 0;
		int	    j;

		sta = t->state->out;
		len = 0;
		if (utf_iscomposing(sta->c))
		{
		    /* Only match composing character(s), ignore base
		     * character.  Used for ".{composing}" and "{composing}"
		     * (no preceding character). */
		    len += mb_char2len(mc);
		}
		if (ireg_icombine && len == 0)
		{
		    /* If \Z was present, then ignore composing characters.
		     * When ignoring the base character this always matches. */
		    /* TODO: How about negated? */
		    if (len == 0 && sta->c != curc)
			result = FAIL;
		    else
			result = OK;
		    while (sta->c != NFA_END_COMPOSING)
			sta = sta->out;
		}

		/* Check base character matches first, unless ignored. */
		else if (len > 0 || mc == sta->c)
		{
		    if (len == 0)
		    {
			len += mb_char2len(mc);
			sta = sta->out;
		    }

		    /* We don't care about the order of composing characters.
		     * Get them into cchars[] first. */
		    while (len < clen)
		    {
			mc = mb_ptr2char(reginput + len);
			cchars[ccount++] = mc;
			len += mb_char2len(mc);
			if (ccount == MAX_MCO)
			    break;
		    }

		    /* Check that each composing char in the pattern matches a
		     * composing char in the text.  We do not check if all
		     * composing chars are matched. */
		    result = OK;
		    while (sta->c != NFA_END_COMPOSING)
		    {
			for (j = 0; j < ccount; ++j)
			    if (cchars[j] == sta->c)
				break;
			if (j == ccount)
			{
			    result = FAIL;
			    break;
			}
			sta = sta->out;
		    }
		}
		else
		    result = FAIL;

		end = t->state->out1;	    /* NFA_END_COMPOSING */
		ADD_POS_NEG_STATE(end);
		break;
	    }
#endif

	    case NFA_NEWL:
		if (curc == NUL && !reg_line_lbr && REG_MULTI
						    && reglnum <= reg_maxline)
		{
		    go_to_nextline = TRUE;
		    /* Pass -1 for the offset, which means taking the position
		     * at the start of the next line. */
		    ll = nextlist;
		    add_state = t->state->out;
		    add_off = -1;
		}
		else if (curc == '\n' && reg_line_lbr)
		{
		    /* match \n as if it is an ordinary character */
		    ll = nextlist;
		    add_state = t->state->out;
		    add_off = 1;
		}
		break;

	    case NFA_CLASS_ALNUM:
	    case NFA_CLASS_ALPHA:
	    case NFA_CLASS_BLANK:
	    case NFA_CLASS_CNTRL:
	    case NFA_CLASS_DIGIT:
	    case NFA_CLASS_GRAPH:
	    case NFA_CLASS_LOWER:
	    case NFA_CLASS_PRINT:
	    case NFA_CLASS_PUNCT:
	    case NFA_CLASS_SPACE:
	    case NFA_CLASS_UPPER:
	    case NFA_CLASS_XDIGIT:
	    case NFA_CLASS_TAB:
	    case NFA_CLASS_RETURN:
	    case NFA_CLASS_BACKSPACE:
	    case NFA_CLASS_ESCAPE:
		result = check_char_class(t->state->c, curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_END_NEG_RANGE:
		/* This follows a series of negated nodes, like:
		 * CHAR(x), NFA_NOT, CHAR(y), NFA_NOT etc. */
		if (curc > 0)
		{
		    ll = nextlist;
		    add_state = t->state->out;
		    add_off = clen;
		}
		break;

	    case NFA_ANY:
		/* Any char except '\0', (end of input) does not match. */
		if (curc > 0)
		{
		    ll = nextlist;
		    add_state = t->state->out;
		    add_off = clen;
		}
		break;

	    /*
	     * Character classes like \a for alpha, \d for digit etc.
	     */
	    case NFA_IDENT:	/*  \i	*/
		result = vim_isIDc(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_SIDENT:	/*  \I	*/
		result = !VIM_ISDIGIT(curc) && vim_isIDc(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_KWORD:	/*  \k	*/
		result = vim_iswordp_buf(reginput, reg_buf);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_SKWORD:	/*  \K	*/
		result = !VIM_ISDIGIT(curc)
					&& vim_iswordp_buf(reginput, reg_buf);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_FNAME:	/*  \f	*/
		result = vim_isfilec(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_SFNAME:	/*  \F	*/
		result = !VIM_ISDIGIT(curc) && vim_isfilec(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_PRINT:	/*  \p	*/
		result = ptr2cells(reginput) == 1;
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_SPRINT:	/*  \P	*/
		result = !VIM_ISDIGIT(curc) && ptr2cells(reginput) == 1;
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_WHITE:	/*  \s	*/
		result = vim_iswhite(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NWHITE:	/*  \S	*/
		result = curc != NUL && !vim_iswhite(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_DIGIT:	/*  \d	*/
		result = ri_digit(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NDIGIT:	/*  \D	*/
		result = curc != NUL && !ri_digit(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_HEX:	/*  \x	*/
		result = ri_hex(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NHEX:	/*  \X	*/
		result = curc != NUL && !ri_hex(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_OCTAL:	/*  \o	*/
		result = ri_octal(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NOCTAL:	/*  \O	*/
		result = curc != NUL && !ri_octal(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_WORD:	/*  \w	*/
		result = ri_word(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NWORD:	/*  \W	*/
		result = curc != NUL && !ri_word(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_HEAD:	/*  \h	*/
		result = ri_head(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NHEAD:	/*  \H	*/
		result = curc != NUL && !ri_head(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_ALPHA:	/*  \a	*/
		result = ri_alpha(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NALPHA:	/*  \A	*/
		result = curc != NUL && !ri_alpha(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_LOWER:	/*  \l	*/
		result = ri_lower(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NLOWER:	/*  \L	*/
		result = curc != NUL && !ri_lower(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_UPPER:	/*  \u	*/
		result = ri_upper(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NUPPER:	/* \U	*/
		result = curc != NUL && !ri_upper(curc);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_BACKREF1:
	    case NFA_BACKREF2:
	    case NFA_BACKREF3:
	    case NFA_BACKREF4:
	    case NFA_BACKREF5:
	    case NFA_BACKREF6:
	    case NFA_BACKREF7:
	    case NFA_BACKREF8:
	    case NFA_BACKREF9:
#ifdef FEAT_SYN_HL
	    case NFA_ZREF1:
	    case NFA_ZREF2:
	    case NFA_ZREF3:
	    case NFA_ZREF4:
	    case NFA_ZREF5:
	    case NFA_ZREF6:
	    case NFA_ZREF7:
	    case NFA_ZREF8:
	    case NFA_ZREF9:
#endif
		/* \1 .. \9  \z1 .. \z9 */
	      {
		int subidx;
		int bytelen;

		if (t->state->c <= NFA_BACKREF9)
		{
		    subidx = t->state->c - NFA_BACKREF1 + 1;
		    result = match_backref(&t->subs.norm, subidx, &bytelen);
		}
#ifdef FEAT_SYN_HL
		else
		{
		    subidx = t->state->c - NFA_ZREF1 + 1;
		    result = match_zref(subidx, &bytelen);
		}
#endif

		if (result)
		{
		    if (bytelen == 0)
		    {
			/* empty match always works, output of NFA_SKIP to be
			 * used next */
			addstate_here(thislist, t->state->out->out, &t->subs,
							    t->pim, &listidx);
		    }
		    else if (bytelen <= clen)
		    {
			/* match current character, jump ahead to out of
			 * NFA_SKIP */
			ll = nextlist;
			add_state = t->state->out->out;
			add_off = clen;
		    }
		    else
		    {
			/* skip over the matched characters, set character
			 * count in NFA_SKIP */
			ll = nextlist;
			add_state = t->state->out;
			add_off = bytelen;
			add_count = bytelen - clen;
		    }
		}
		break;
	      }
	    case NFA_SKIP:
	      /* character of previous matching \1 .. \9  or \@> */
	      if (t->count - clen <= 0)
	      {
		  /* end of match, go to what follows */
		  ll = nextlist;
		  add_state = t->state->out;
		  add_off = clen;
	      }
	      else
	      {
		  /* add state again with decremented count */
		  ll = nextlist;
		  add_state = t->state;
		  add_off = 0;
		  add_count = t->count - clen;
	      }
	      break;

	    case NFA_LNUM:
	    case NFA_LNUM_GT:
	    case NFA_LNUM_LT:
		result = (REG_MULTI &&
			nfa_re_num_cmp(t->state->val, t->state->c - NFA_LNUM,
			    (long_u)(reglnum + reg_firstlnum)));
		if (result)
		    addstate_here(thislist, t->state->out, &t->subs,
							    t->pim, &listidx);
		break;

	    case NFA_COL:
	    case NFA_COL_GT:
	    case NFA_COL_LT:
		result = nfa_re_num_cmp(t->state->val, t->state->c - NFA_COL,
			(long_u)(reginput - regline) + 1);
		if (result)
		    addstate_here(thislist, t->state->out, &t->subs,
							    t->pim, &listidx);
		break;

	    case NFA_VCOL:
	    case NFA_VCOL_GT:
	    case NFA_VCOL_LT:
		result = nfa_re_num_cmp(t->state->val, t->state->c - NFA_VCOL,
		    (long_u)win_linetabsize(
			    reg_win == NULL ? curwin : reg_win,
			    regline, (colnr_T)(reginput - regline)) + 1);
		if (result)
		    addstate_here(thislist, t->state->out, &t->subs,
							    t->pim, &listidx);
		break;

	    case NFA_MARK:
	    case NFA_MARK_GT:
	    case NFA_MARK_LT:
	      {
		pos_T	*pos = getmark_buf(reg_buf, t->state->val, FALSE);

		/* Compare the mark position to the match position. */
		result = (pos != NULL		     /* mark doesn't exist */
			&& pos->lnum > 0    /* mark isn't set in reg_buf */
			&& (pos->lnum == reglnum + reg_firstlnum
				? (pos->col == (colnr_T)(reginput - regline)
				    ? t->state->c == NFA_MARK
				    : (pos->col < (colnr_T)(reginput - regline)
					? t->state->c == NFA_MARK_GT
					: t->state->c == NFA_MARK_LT))
				: (pos->lnum < reglnum + reg_firstlnum
				    ? t->state->c == NFA_MARK_GT
				    : t->state->c == NFA_MARK_LT)));
		if (result)
		    addstate_here(thislist, t->state->out, &t->subs,
							    t->pim, &listidx);
		break;
	      }

	    case NFA_CURSOR:
		result = (reg_win != NULL
			&& (reglnum + reg_firstlnum == reg_win->w_cursor.lnum)
			&& ((colnr_T)(reginput - regline)
						   == reg_win->w_cursor.col));
		if (result)
		    addstate_here(thislist, t->state->out, &t->subs,
							    t->pim, &listidx);
		break;

	    case NFA_VISUAL:
#ifdef FEAT_VISUAL
		result = reg_match_visual();
		if (result)
		    addstate_here(thislist, t->state->out, &t->subs,
							    t->pim, &listidx);
#endif
		break;

	    default:	/* regular character */
	      {
		int c = t->state->c;

		/* TODO: put this in #ifdef later */
		if (c < -256)
		    EMSGN("INTERNAL: Negative state char: %ld", c);
		if (is_Magic(c))
		    c = un_Magic(c);
		result = (c == curc);

		if (!result && ireg_ic)
		    result = MB_TOLOWER(c) == MB_TOLOWER(curc);
#ifdef FEAT_MBYTE
		/* If there is a composing character which is not being
		 * ignored there can be no match. Match with composing
		 * character uses NFA_COMPOSING above. */
		if (result && enc_utf8 && !ireg_icombine
						&& clen != utf_char2len(curc))
		    result = FALSE;
#endif
		ADD_POS_NEG_STATE(t->state);
		break;
	      }

	    } /* switch (t->state->c) */

	    if (add_state != NULL)
	    {
		if (t->pim != NULL)
		{
		    /* postponed invisible match */
		    /* TODO: also do t->pim->pim recursively? */
		    if (t->pim->result == NFA_PIM_TODO)
		    {
#ifdef ENABLE_LOG
			fprintf(log_fd, "\n");
			fprintf(log_fd, "==================================\n");
			fprintf(log_fd, "Postponed recursive nfa_regmatch()\n");
			fprintf(log_fd, "\n");
#endif
			result = recursive_regmatch(t->pim->state,
						 prog, submatch, m, &listids);
			t->pim->result = result ? NFA_PIM_MATCH
							    : NFA_PIM_NOMATCH;
			/* for \@! it is a match when result is FALSE */
			if (result != t->pim->state->negated)
			{
			    /* Copy submatch info from the recursive call */
			    copy_sub_off(&t->pim->subs.norm, &m->norm);
#ifdef FEAT_SYN_HL
			    copy_sub_off(&t->pim->subs.synt, &m->synt);
#endif
			}
		    }
		    else
		    {
			result = (t->pim->result == NFA_PIM_MATCH);
#ifdef ENABLE_LOG
			fprintf(log_fd, "\n");
			fprintf(log_fd, "Using previous recursive nfa_regmatch() result, result == %d\n", t->pim->result);
			fprintf(log_fd, "MATCH = %s\n", result == TRUE ? "OK" : "FALSE");
			fprintf(log_fd, "\n");
#endif
		    }

		    /* for \@! it is a match when result is FALSE */
		    if (result != t->pim->state->negated)
		    {
			/* Copy submatch info from the recursive call */
			copy_sub_off(&t->subs.norm, &t->pim->subs.norm);
#ifdef FEAT_SYN_HL
			copy_sub_off(&t->subs.synt, &t->pim->subs.synt);
#endif
		    }
		    else
			/* look-behind match failed, don't add the state */
			continue;
		}

		addstate(ll, add_state, &t->subs, add_off);
		if (add_count > 0)
		    nextlist->t[ll->n - 1].count = add_count;
	    }

	} /* for (thislist = thislist; thislist->state; thislist++) */

	/* Look for the start of a match in the current position by adding the
	 * start state to the list of states.
	 * The first found match is the leftmost one, thus the order of states
	 * matters!
	 * Do not add the start state in recursive calls of nfa_regmatch(),
	 * because recursive calls should only start in the first position.
	 * Unless "nfa_endp" is not NULL, then we match the end position.
	 * Also don't start a match past the first line. */
	if (nfa_match == FALSE
		&& ((start->c == NFA_MOPEN
			&& reglnum == 0
			&& clen != 0
			&& (ireg_maxcol == 0
			    || (colnr_T)(reginput - regline) < ireg_maxcol))
		    || (nfa_endp != NULL
			&& (REG_MULTI
			    ? (reglnum < nfa_endp->se_u.pos.lnum
			       || (reglnum == nfa_endp->se_u.pos.lnum
			           && (int)(reginput - regline)
						    < nfa_endp->se_u.pos.col))
			    : reginput < nfa_endp->se_u.ptr))))
	{
#ifdef ENABLE_LOG
	    fprintf(log_fd, "(---) STARTSTATE\n");
#endif
	    addstate(nextlist, start, m, clen);
	}

#ifdef ENABLE_LOG
	fprintf(log_fd, ">>> Thislist had %d states available: ", thislist->n);
	{
	    int i;

	    for (i = 0; i < thislist->n; i++)
		fprintf(log_fd, "%d  ", abs(thislist->t[i].state->id));
	}
	fprintf(log_fd, "\n");
#endif

nextchar:
	/* Advance to the next character, or advance to the next line, or
	 * finish. */
	if (clen != 0)
	    reginput += clen;
	else if (go_to_nextline || (nfa_endp != NULL && REG_MULTI
					&& reglnum < nfa_endp->se_u.pos.lnum))
	    reg_nextline();
	else
	    break;
    }

#ifdef ENABLE_LOG
    if (log_fd != stderr)
	fclose(log_fd);
    log_fd = NULL;
#endif

theend:
    /* Free memory */
    vim_free(list[0].t);
    vim_free(list[1].t);
    vim_free(list[2].t);
    vim_free(listids);
    ga_clear(&pimlist);
#undef ADD_POS_NEG_STATE
#ifdef NFA_REGEXP_DEBUG_LOG
    fclose(debug);
#endif

    return nfa_match;
}

/*
 * Try match of "prog" with at regline["col"].
 * Returns 0 for failure, number of lines contained in the match otherwise.
 */
    static long
nfa_regtry(prog, col)
    nfa_regprog_T   *prog;
    colnr_T	    col;
{
    int		i;
    regsubs_T	subs, m;
    nfa_state_T	*start = prog->start;
#ifdef ENABLE_LOG
    FILE	*f;
#endif

    reginput = regline + col;
    need_clear_subexpr = TRUE;
#ifdef FEAT_SYN_HL
    /* Clear the external match subpointers if necessary. */
    if (prog->reghasz == REX_SET)
    {
	nfa_has_zsubexpr = TRUE;
	need_clear_zsubexpr = TRUE;
    }
    else
	nfa_has_zsubexpr = FALSE;
#endif

#ifdef ENABLE_LOG
    f = fopen(NFA_REGEXP_RUN_LOG, "a");
    if (f != NULL)
    {
	fprintf(f, "\n\n\t=======================================================\n");
#ifdef DEBUG
	fprintf(f, "\tRegexp is \"%s\"\n", nfa_regengine.expr);
#endif
	fprintf(f, "\tInput text is \"%s\" \n", reginput);
	fprintf(f, "\t=======================================================\n\n");
	nfa_print_state(f, start);
	fprintf(f, "\n\n");
	fclose(f);
    }
    else
	EMSG(_("Could not open temporary log file for writing "));
#endif

    clear_sub(&subs.norm);
    clear_sub(&m.norm);
#ifdef FEAT_SYN_HL
    clear_sub(&subs.synt);
    clear_sub(&m.synt);
#endif

    if (nfa_regmatch(prog, start, &subs, &m) == FALSE)
	return 0;

    cleanup_subexpr();
    if (REG_MULTI)
    {
	for (i = 0; i < subs.norm.in_use; i++)
	{
	    reg_startpos[i] = subs.norm.list.multi[i].start;
	    reg_endpos[i] = subs.norm.list.multi[i].end;
	}

	if (reg_startpos[0].lnum < 0)
	{
	    reg_startpos[0].lnum = 0;
	    reg_startpos[0].col = col;
	}
	if (reg_endpos[0].lnum < 0)
	{
	    /* pattern has a \ze but it didn't match, use current end */
	    reg_endpos[0].lnum = reglnum;
	    reg_endpos[0].col = (int)(reginput - regline);
	}
	else
	    /* Use line number of "\ze". */
	    reglnum = reg_endpos[0].lnum;
    }
    else
    {
	for (i = 0; i < subs.norm.in_use; i++)
	{
	    reg_startp[i] = subs.norm.list.line[i].start;
	    reg_endp[i] = subs.norm.list.line[i].end;
	}

	if (reg_startp[0] == NULL)
	    reg_startp[0] = regline + col;
	if (reg_endp[0] == NULL)
	    reg_endp[0] = reginput;
    }

#ifdef FEAT_SYN_HL
    /* Package any found \z(...\) matches for export. Default is none. */
    unref_extmatch(re_extmatch_out);
    re_extmatch_out = NULL;

    if (prog->reghasz == REX_SET)
    {
	cleanup_zsubexpr();
	re_extmatch_out = make_extmatch();
	for (i = 0; i < subs.synt.in_use; i++)
	{
	    if (REG_MULTI)
	    {
		struct multipos *mpos = &subs.synt.list.multi[i];

		/* Only accept single line matches. */
		if (mpos->start.lnum >= 0 && mpos->start.lnum == mpos->end.lnum)
		    re_extmatch_out->matches[i] =
			vim_strnsave(reg_getline(mpos->start.lnum)
							    + mpos->start.col,
					     mpos->end.col - mpos->start.col);
	    }
	    else
	    {
		struct linepos *lpos = &subs.synt.list.line[i];

		if (lpos->start != NULL && lpos->end != NULL)
		    re_extmatch_out->matches[i] =
			    vim_strnsave(lpos->start,
					      (int)(lpos->end - lpos->start));
	    }
	}
    }
#endif

    return 1 + reglnum;
}

/*
 * Match a regexp against a string ("line" points to the string) or multiple
 * lines ("line" is NULL, use reg_getline()).
 *
 * Returns 0 for failure, number of lines contained in the match otherwise.
 */
    static long
nfa_regexec_both(line, col)
    char_u	*line;
    colnr_T	col;		/* column to start looking for match */
{
    nfa_regprog_T   *prog;
    long	    retval = 0L;
    int		    i;

    if (REG_MULTI)
    {
	prog = (nfa_regprog_T *)reg_mmatch->regprog;
	line = reg_getline((linenr_T)0);    /* relative to the cursor */
	reg_startpos = reg_mmatch->startpos;
	reg_endpos = reg_mmatch->endpos;
    }
    else
    {
	prog = (nfa_regprog_T *)reg_match->regprog;
	reg_startp = reg_match->startp;
	reg_endp = reg_match->endp;
    }

    /* Be paranoid... */
    if (prog == NULL || line == NULL)
    {
	EMSG(_(e_null));
	goto theend;
    }

    /* If the start column is past the maximum column: no need to try. */
    if (ireg_maxcol > 0 && col >= ireg_maxcol)
	goto theend;

    /* If pattern contains "\c" or "\C": overrule value of ireg_ic */
    if (prog->regflags & RF_ICASE)
	ireg_ic = TRUE;
    else if (prog->regflags & RF_NOICASE)
	ireg_ic = FALSE;

#ifdef FEAT_MBYTE
    /* If pattern contains "\Z" overrule value of ireg_icombine */
    if (prog->regflags & RF_ICOMBINE)
	ireg_icombine = TRUE;
#endif

    regline = line;
    reglnum = 0;    /* relative to line */

    nfa_has_zend = prog->has_zend;
    nfa_has_backref = prog->has_backref;
    nfa_nsubexpr = prog->nsubexp;
    nfa_listid = 1;
    nfa_alt_listid = 2;
#ifdef DEBUG
    nfa_regengine.expr = prog->pattern;
#endif

    nstate = prog->nstate;
    for (i = 0; i < nstate; ++i)
    {
	prog->state[i].id = i;
	prog->state[i].lastlist[0] = 0;
	prog->state[i].lastlist[1] = 0;
    }

    retval = nfa_regtry(prog, col);

#ifdef DEBUG
    nfa_regengine.expr = NULL;
#endif

theend:
    return retval;
}

/*
 * Compile a regular expression into internal code for the NFA matcher.
 * Returns the program in allocated space.  Returns NULL for an error.
 */
    static regprog_T *
nfa_regcomp(expr, re_flags)
    char_u	*expr;
    int		re_flags;
{
    nfa_regprog_T	*prog = NULL;
    size_t		prog_size;
    int			*postfix;

    if (expr == NULL)
	return NULL;

#ifdef DEBUG
    nfa_regengine.expr = expr;
#endif

    init_class_tab();

    if (nfa_regcomp_start(expr, re_flags) == FAIL)
	return NULL;

    /* Build postfix form of the regexp. Needed to build the NFA
     * (and count its size). */
    postfix = re2post();
    if (postfix == NULL)
    {
	/* TODO: only give this error for debugging? */
	if (post_ptr >= post_end)
	    EMSGN("Internal error: estimated max number of states insufficient: %ld", post_end - post_start);
	goto fail;	    /* Cascaded (syntax?) error */
    }

    /*
     * In order to build the NFA, we parse the input regexp twice:
     * 1. first pass to count size (so we can allocate space)
     * 2. second to emit code
     */
#ifdef ENABLE_LOG
    {
	FILE *f = fopen(NFA_REGEXP_RUN_LOG, "a");

	if (f != NULL)
	{
	    fprintf(f, "\n*****************************\n\n\n\n\tCompiling regexp \"%s\" ... hold on !\n", expr);
	    fclose(f);
	}
    }
#endif

    /*
     * PASS 1
     * Count number of NFA states in "nstate". Do not build the NFA.
     */
    post2nfa(postfix, post_ptr, TRUE);

    /* Space for compiled regexp */
    prog_size = sizeof(nfa_regprog_T) + sizeof(nfa_state_T) * nstate;
    prog = (nfa_regprog_T *)lalloc(prog_size, TRUE);
    if (prog == NULL)
	goto fail;
    vim_memset(prog, 0, prog_size);
    state_ptr = prog->state;

    /*
     * PASS 2
     * Build the NFA
     */
    prog->start = post2nfa(postfix, post_ptr, FALSE);
    if (prog->start == NULL)
	goto fail;

    prog->regflags = regflags;
    prog->engine = &nfa_regengine;
    prog->nstate = nstate;
    prog->has_zend = nfa_has_zend;
    prog->has_backref = nfa_has_backref;
    prog->nsubexp = regnpar;
#ifdef ENABLE_LOG
    nfa_postfix_dump(expr, OK);
    nfa_dump(prog);
#endif
#ifdef FEAT_SYN_HL
    /* Remember whether this pattern has any \z specials in it. */
    prog->reghasz = re_has_z;
#endif
#ifdef DEBUG
    prog->pattern = vim_strsave(expr); /* memory will leak */
    nfa_regengine.expr = NULL;
#endif

out:
    vim_free(post_start);
    post_start = post_ptr = post_end = NULL;
    state_ptr = NULL;
    return (regprog_T *)prog;

fail:
    vim_free(prog);
    prog = NULL;
#ifdef ENABLE_LOG
    nfa_postfix_dump(expr, FAIL);
#endif
#ifdef DEBUG
    nfa_regengine.expr = NULL;
#endif
    goto out;
}


/*
 * Match a regexp against a string.
 * "rmp->regprog" is a compiled regexp as returned by nfa_regcomp().
 * Uses curbuf for line count and 'iskeyword'.
 *
 * Return TRUE if there is a match, FALSE if not.
 */
    static int
nfa_regexec(rmp, line, col)
    regmatch_T	*rmp;
    char_u	*line;	/* string to match against */
    colnr_T	col;	/* column to start looking for match */
{
    reg_match = rmp;
    reg_mmatch = NULL;
    reg_maxline = 0;
    reg_line_lbr = FALSE;
    reg_buf = curbuf;
    reg_win = NULL;
    ireg_ic = rmp->rm_ic;
#ifdef FEAT_MBYTE
    ireg_icombine = FALSE;
#endif
    ireg_maxcol = 0;
    return (nfa_regexec_both(line, col) != 0);
}

#if defined(FEAT_MODIFY_FNAME) || defined(FEAT_EVAL) \
	|| defined(FIND_REPLACE_DIALOG) || defined(PROTO)

static int  nfa_regexec_nl __ARGS((regmatch_T *rmp, char_u *line, colnr_T col));

/*
 * Like nfa_regexec(), but consider a "\n" in "line" to be a line break.
 */
    static int
nfa_regexec_nl(rmp, line, col)
    regmatch_T	*rmp;
    char_u	*line;	/* string to match against */
    colnr_T	col;	/* column to start looking for match */
{
    reg_match = rmp;
    reg_mmatch = NULL;
    reg_maxline = 0;
    reg_line_lbr = TRUE;
    reg_buf = curbuf;
    reg_win = NULL;
    ireg_ic = rmp->rm_ic;
#ifdef FEAT_MBYTE
    ireg_icombine = FALSE;
#endif
    ireg_maxcol = 0;
    return (nfa_regexec_both(line, col) != 0);
}
#endif


/*
 * Match a regexp against multiple lines.
 * "rmp->regprog" is a compiled regexp as returned by vim_regcomp().
 * Uses curbuf for line count and 'iskeyword'.
 *
 * Return zero if there is no match.  Return number of lines contained in the
 * match otherwise.
 *
 * Note: the body is the same as bt_regexec() except for nfa_regexec_both()
 *
 * ! Also NOTE : match may actually be in another line. e.g.:
 * when r.e. is \nc, cursor is at 'a' and the text buffer looks like
 *
 * +-------------------------+
 * |a                        |
 * |b                        |
 * |c                        |
 * |                         |
 * +-------------------------+
 *
 * then nfa_regexec_multi() returns 3. while the original
 * vim_regexec_multi() returns 0 and a second call at line 2 will return 2.
 *
 * FIXME if this behavior is not compatible.
 */
    static long
nfa_regexec_multi(rmp, win, buf, lnum, col, tm)
    regmmatch_T	*rmp;
    win_T	*win;		/* window in which to search or NULL */
    buf_T	*buf;		/* buffer in which to search */
    linenr_T	lnum;		/* nr of line to start looking for match */
    colnr_T	col;		/* column to start looking for match */
    proftime_T	*tm UNUSED;	/* timeout limit or NULL */
{
    reg_match = NULL;
    reg_mmatch = rmp;
    reg_buf = buf;
    reg_win = win;
    reg_firstlnum = lnum;
    reg_maxline = reg_buf->b_ml.ml_line_count - lnum;
    reg_line_lbr = FALSE;
    ireg_ic = rmp->rmm_ic;
#ifdef FEAT_MBYTE
    ireg_icombine = FALSE;
#endif
    ireg_maxcol = rmp->rmm_maxcol;

    return nfa_regexec_both(NULL, col);
}

#ifdef DEBUG
# undef ENABLE_LOG
#endif
