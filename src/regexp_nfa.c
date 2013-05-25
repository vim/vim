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

/* Upper limit allowed for {m,n} repetitions handled by NFA */
#define	    NFA_BRACES_MAXLIMIT		    10
/* For allocating space for the postfix representation */
#define	    NFA_POSTFIX_MULTIPLIER	    (NFA_BRACES_MAXLIMIT + 2)*2

enum
{
    NFA_SPLIT = -1024,
    NFA_MATCH,
    NFA_SKIP_CHAR,		    /* matches a 0-length char */
    NFA_END_NEG_RANGE,		    /* Used when expanding [^ab] */

    NFA_CONCAT,
    NFA_OR,
    NFA_STAR,
    NFA_PLUS,
    NFA_QUEST,
    NFA_QUEST_NONGREEDY,	    /* Non-greedy version of \? */
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
    NFA_END_INVISIBLE,
    NFA_COMPOSING,		    /* Next nodes in NFA are part of the
				       composing multibyte char */
    NFA_END_COMPOSING,		    /* End of a composing char in the NFA */

    /* The following are used only in the postfix form, not in the NFA */
    NFA_PREV_ATOM_NO_WIDTH,	    /* Used for \@= */
    NFA_PREV_ATOM_NO_WIDTH_NEG,	    /* Used for \@! */
    NFA_PREV_ATOM_JUST_BEFORE,	    /* Used for \@<= */
    NFA_PREV_ATOM_JUST_BEFORE_NEG,  /* Used for \@<! */
    NFA_PREV_ATOM_LIKE_PATTERN,	    /* Used for \@> */

    NFA_MOPEN,
    NFA_MCLOSE = NFA_MOPEN + NSUBEXP,

    /* NFA_FIRST_NL */
    NFA_ANY = NFA_MCLOSE + NSUBEXP, /*	Match any one character. */
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

/*
 * NFA errors can be of 3 types:
 * *** NFA runtime errors, when something unknown goes wrong. The NFA fails
 *     silently and revert the to backtracking engine.
 *     syntax_error = FALSE;
 * *** Regexp syntax errors, when the input regexp is not syntactically correct.
 *     The NFA engine displays an error message, and nothing else happens.
 *     syntax_error = TRUE
 * *** Unsupported features, when the input regexp uses an operator that is not
 *     implemented in the NFA. The NFA engine fails silently, and reverts to the
 *     old backtracking engine.
 *     syntax_error = FALSE
 * "The NFA fails" means that "compiling the regexp with the NFA fails":
 * nfa_regcomp() returns FAIL.
 */
static int syntax_error = FALSE;

/* NFA regexp \ze operator encountered. */
static int nfa_has_zend = FALSE;

static int *post_start;  /* holds the postfix form of r.e. */
static int *post_end;
static int *post_ptr;

static int nstate;	/* Number of states in the NFA. */
static int istate;	/* Index in the state vector, used in new_state() */
static int nstate_max;	/* Upper bound of estimated number of states. */


static int nfa_regcomp_start __ARGS((char_u*expr, int re_flags));
static int nfa_recognize_char_class __ARGS((char_u *start, char_u *end, int extra_newl));
static int nfa_emit_equi_class __ARGS((int c, int neg));
static void nfa_inc __ARGS((char_u **p));
static void nfa_dec __ARGS((char_u **p));
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
static nfa_state_T *new_state __ARGS((int c, nfa_state_T *out, nfa_state_T *out1));
static nfa_state_T *post2nfa __ARGS((int *postfix, int *end, int nfa_calc_size));
static int check_char_class __ARGS((int class, int c));
static void st_error __ARGS((int *postfix, int *end, int *p));
static void nfa_save_listids __ARGS((nfa_state_T *start, int *list));
static void nfa_restore_listids __ARGS((nfa_state_T *start, int *list));
static void nfa_set_null_listids __ARGS((nfa_state_T *start));
static void nfa_set_neg_listids __ARGS((nfa_state_T *start));
static long nfa_regtry __ARGS((nfa_state_T *start, colnr_T col));
static long nfa_regexec_both __ARGS((char_u *line, colnr_T col));
static regprog_T *nfa_regcomp __ARGS((char_u *expr, int re_flags));
static int nfa_regexec __ARGS((regmatch_T *rmp, char_u *line, colnr_T col));
static long nfa_regexec_multi __ARGS((regmmatch_T *rmp, win_T *win, buf_T *buf, linenr_T lnum, colnr_T col, proftime_T *tm));

/* helper functions used when doing re2post() ... regatom() parsing */
#define EMIT(c)	do {				\
		    if (post_ptr >= post_end)	\
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

    nstate = 0;
    istate = 0;
    /* A reasonable estimation for size */
    nstate_max = (int)(STRLEN(expr) + 1) * NFA_POSTFIX_MULTIPLIER;

    /* Some items blow up in size, such as [A-z].  Add more space for that.
     * TODO: some patterns may still fail. */
    nstate_max += 1000;

    /* Size for postfix representation of expr. */
    postfix_size = sizeof(*post_start) * nstate_max;

    post_start = (int *)lalloc(postfix_size, TRUE);
    if (post_start == NULL)
	return FAIL;
    vim_memset(post_start, 0, postfix_size);
    post_ptr = post_start;
    post_end = post_start + nstate_max;
    nfa_has_zend = FALSE;

    regcomp_start(expr, re_flags);

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
    int		i;
    /* Each of these variables takes up a char in "config[]",
     * in the order they are here. */
    int		not = FALSE, af = FALSE, AF = FALSE, az = FALSE, AZ = FALSE,
		o7 = FALSE, o9 = FALSE, underscore = FALSE, newl = FALSE;
    char_u	*p;
#define NCONFIGS 16
    int		classid[NCONFIGS] = {
	NFA_DIGIT, NFA_NDIGIT, NFA_HEX, NFA_NHEX,
	NFA_OCTAL, NFA_NOCTAL, NFA_WORD, NFA_NWORD,
	NFA_HEAD, NFA_NHEAD, NFA_ALPHA, NFA_NALPHA,
	NFA_LOWER, NFA_NLOWER, NFA_UPPER, NFA_NUPPER
    };
    char_u	myconfig[10];
    char_u	config[NCONFIGS][9] = {
	"000000100",	/* digit */
	"100000100",	/* non digit */
	"011000100",	/* hex-digit */
	"111000100",	/* non hex-digit */
	"000001000",	/* octal-digit */
	"100001000",	/* [^0-7] */
	"000110110",	/* [0-9A-Za-z_]	*/
	"100110110",	/* [^0-9A-Za-z_] */
	"000110010",	/* head of word */
	"100110010",	/* not head of word */
	"000110000",	/* alphabetic char a-z */
	"100110000",	/* non alphabetic char */
	"000100000",	/* lowercase letter */
	"100100000",	/* non lowercase */
	"000010000",	/* uppercase */
	"100010000"	/* non uppercase */
    };

    if (extra_newl == TRUE)
	newl = TRUE;

    if (*end != ']')
	return FAIL;
    p = start;
    if (*p == '^')
    {
	not = TRUE;
	p ++;
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
			o9 = TRUE;
			break;
		    }
		    else
		    if (*(p + 2) == '7')
		    {
			o7 = TRUE;
			break;
		    }
		case 'a':
		    if (*(p + 2) == 'z')
		    {
			az = TRUE;
			break;
		    }
		    else
		    if (*(p + 2) == 'f')
		    {
			af = TRUE;
			break;
		    }
		case 'A':
		    if (*(p + 2) == 'Z')
		    {
			AZ = TRUE;
			break;
		    }
		    else
		    if (*(p + 2) == 'F')
		    {
			AF = TRUE;
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
	    underscore = TRUE;
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

    /* build the config that represents the ranges we gathered */
    STRCPY(myconfig, "000000000");
    if (not == TRUE)
	myconfig[0] = '1';
    if (af == TRUE)
	myconfig[1] = '1';
    if (AF == TRUE)
	myconfig[2] = '1';
    if (az == TRUE)
	myconfig[3] = '1';
    if (AZ == TRUE)
	myconfig[4] = '1';
    if (o7 == TRUE)
	myconfig[5] = '1';
    if (o9 == TRUE)
	myconfig[6] = '1';
    if (underscore == TRUE)
	myconfig[7] = '1';
    if (newl == TRUE)
    {
	myconfig[8] = '1';
	extra_newl = ADD_NL;
    }
    /* try to recognize character classes */
    for (i = 0; i < NCONFIGS; i++)
	if (STRNCMP(myconfig, config[i], 8) == 0)
	    return classid[i] + extra_newl;

    /* fallthrough => no success so far */
    return FAIL;

#undef NCONFIGS
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
 * Increments the pointer "p" by one (multi-byte) character.
 */
    static void
nfa_inc(p)
    char_u **p;
{
#ifdef FEAT_MBYTE
    if (has_mbyte)
	mb_ptr2char_adv(p);
    else
#endif
	*p = *p + 1;
}

/*
 * Decrements the pointer "p" by one (multi-byte) character.
 */
    static void
nfa_dec(p)
    char_u **p;
{
#ifdef FEAT_MBYTE
    char_u *p2, *oldp;

    if (has_mbyte)
    {
	oldp = *p;
	/* Try to find the multibyte char that advances to the current
	 * position. */
	do
	{
	    *p = *p - 1;
	    p2 = *p;
	    mb_ptr2char_adv(&p2);
	} while (p2 != oldp);
    }
#else
    *p = *p - 1;
#endif
}

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
    int		i;
#endif
    int		extra = 0;
    int		first;
    int		emit_range;
    int		negated;
    int		result;
    int		startc = -1;
    int		endc = -1;
    int		oldstartc = -1;
    int		cpo_lit;	/* 'cpoptions' contains 'l' flag */
    int		cpo_bsl;	/* 'cpoptions' contains '\' flag */
    int		glue;		/* ID that will "glue" nodes together */

    cpo_lit = vim_strchr(p_cpo, CPO_LITERAL) != NULL;
    cpo_bsl = vim_strchr(p_cpo, CPO_BACKSL) != NULL;

    c = getchr();
    switch (c)
    {
	case NUL:
	    syntax_error = TRUE;
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
		return FAIL;	    /* runtime error */
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
	    syntax_error = TRUE;
	    EMSGN(_(e_misplaced), no_Magic(c));
	    return FAIL;

	case Magic('='):
	case Magic('?'):
	case Magic('+'):
	case Magic('@'):
	case Magic('*'):
	case Magic('{'):
	    /* these should follow an atom, not form an atom */
	    syntax_error = TRUE;
	    EMSGN(_(e_misplaced), no_Magic(c));
	    return FAIL;

	case Magic('~'):		/* previous substitute pattern */
	    /* Not supported yet */
	    return FAIL;

	case Magic('1'):
	case Magic('2'):
	case Magic('3'):
	case Magic('4'):
	case Magic('5'):
	case Magic('6'):
	case Magic('7'):
	case Magic('8'):
	case Magic('9'):
	    /* not supported yet */
	    return FAIL;

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
		    /* TODO: Currently \ze does not work properly. */
		    return FAIL;
		    /* break; */
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '(':
		    /* \z1...\z9 and \z( not yet supported */
		    return FAIL;
		default:
		    syntax_error = TRUE;
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
			int i;

			switch (c)
			{
			    case 'd': i = getdecchrs(); break;
			    case 'o': i = getoctchrs(); break;
			    case 'x': i = gethexchrs(2); break;
			    case 'u': i = gethexchrs(4); break;
			    case 'U': i = gethexchrs(8); break;
			    default:  i = -1; break;
			}

			if (i < 0)
			    EMSG2_RET_FAIL(
			       _("E678: Invalid character after %s%%[dxouU]"),
				    reg_magic == MAGIC_ALL);
			/* TODO: what if a composing character follows? */
			EMIT(i);
		    }
		    break;

		/* Catch \%^ and \%$ regardless of where they appear in the
		 * pattern -- regardless of whether or not it makes sense. */
		case '^':
		    EMIT(NFA_BOF);
		    /* Not yet supported */
		    return FAIL;
		    break;

		case '$':
		    EMIT(NFA_EOF);
		    /* Not yet supported */
		    return FAIL;
		    break;

		case '#':
		    /* not supported yet */
		    return FAIL;
		    break;

		case 'V':
		    /* not supported yet */
		    return FAIL;
		    break;

		case '[':
		    /* \%[abc] not supported yet */
		    return FAIL;

		default:
		    /* not supported yet */
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
		    nfa_inc(&regparse);
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
		    nfa_inc(&regparse);
		}
		if (*regparse == '-')
		{
		    startc = '-';
		    EMIT(startc);
		    TRY_NEG();
		    EMIT_GLUE();
		    nfa_inc(&regparse);
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
		    /* Try a range like 'a-x' or '\t-z' */
		    if (*regparse == '-')
		    {
			emit_range = TRUE;
			startc = oldstartc;
			nfa_inc(&regparse);
			continue;	    /* reading the end of the range */
		    }

		    /* Now handle simple and escaped characters.
		     * Only "\]", "\^", "\]" and "\\" are special in Vi.  Vim
		     * accepts "\t", "\e", etc., but only when the 'l' flag in
		     * 'cpoptions' is not included.
		     * Posix doesn't recognize backslash at all.
		     */
		    if (*regparse == '\\'
			    && !cpo_bsl
			    && regparse + 1 <= endp
			    && (vim_strchr(REGEXP_INRANGE, regparse[1]) != NULL
				|| (!cpo_lit
				    && vim_strchr(REGEXP_ABBR, regparse[1])
								      != NULL)
			    )
			)
		    {
			nfa_inc(&regparse);

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
				nfa_dec(&regparse);
			    }
			    else
			    {
				/* \r,\t,\e,\b */
				startc = backslash_trans(*regparse);
			    }
		    }

		    /* Normal printable char */
		    if (startc == -1)
#ifdef FEAT_MBYTE
			startc = (*mb_ptr2char)(regparse);
#else
		    startc = *regparse;
#endif

		    /* Previous char was '-', so this char is end of range. */
		    if (emit_range)
		    {
			endc = startc; startc = oldstartc;
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
			    emit_range = FALSE;
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
			    emit_range = FALSE;
			}
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

		    nfa_inc(&regparse);
		} /* while (p < endp) */

		nfa_dec(&regparse);
		if (*regparse == '-')	    /* if last, '-' is just a char */
		{
		    EMIT('-');
		    TRY_NEG();
		    EMIT_GLUE();
		}
		nfa_inc(&regparse);

		if (extra == ADD_NL)	    /* \_[] also matches \n */
		{
		    EMIT(reg_string ? NL : NFA_NEWL);
		    TRY_NEG();
		    EMIT_GLUE();
		}

		/* skip the trailing ] */
		regparse = endp;
		nfa_inc(&regparse);
		if (negated == TRUE)
		{
		    /* Mark end of negated char range */
		    EMIT(NFA_END_NEG_RANGE);
		    EMIT(NFA_CONCAT);
		}
		return OK;
	    } /* if exists closing ] */

	    if (reg_strict)
	    {
		syntax_error = TRUE;
		EMSG_RET_FAIL(_(e_missingbracket));
	    }
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
		    /* A base character plus composing characters, or just one
		     * or more composing characters.
		     * This requires creating a separate atom as if enclosing
		     * the characters in (), where NFA_COMPOSING is the ( and
		     * NFA_END_COMPOSING is the ). Note that right now we are
		     * building the postfix form, not the NFA itself;
		     * a composing char could be: a, b, c, NFA_COMPOSING
		     * where 'b' and 'c' are chars with codes > 256. */
		    i = 0;
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
    char_u	*old_regparse, *new_regparse;
    int		c2;
    int		*old_post_ptr, *my_post_start;
    int		old_regnpar;
    int		quest;

    /* Save the current position in the regexp, so that we can use it if
     * <atom>{m,n} is next. */
    old_regparse = regparse;
    /* Save current number of open parenthesis, so we can use it if
     * <atom>{m,n} is next */
    old_regnpar = regnpar;
    /* store current pos in the postfix form, for \{m,n} involving 0s */
    my_post_start = post_ptr;

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
	     * In order to be consistent with the old engine, we disable
	     * NFA_PLUS, and replace <atom>+ with <atom><atom>*
	     */
	    /*	EMIT(NFA_PLUS);	 */
	    regnpar = old_regnpar;
	    regparse = old_regparse;
	    curchr = -1;
	    if (nfa_regatom() == FAIL)
		return FAIL;
	    EMIT(NFA_STAR);
	    EMIT(NFA_CONCAT);
	    skipchr();		/* skip the \+	*/
	    break;

	case Magic('@'):
	    op = no_Magic(getchr());
	    switch(op)
	    {
		case '=':
		    EMIT(NFA_PREV_ATOM_NO_WIDTH);
		    break;
		case '!':
		case '<':
		case '>':
		    /* Not supported yet */
		    return FAIL;
		default:
		    syntax_error = TRUE;
		    EMSGN(_("E869: (NFA) Unknown operator '\\@%c'"), op);
		    return FAIL;
	    }
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
	    {
		syntax_error = TRUE;
		EMSG_RET_FAIL(_("E870: (NFA regexp) Error reading repetition limits"));
	    }
	    /*  <atom>{0,inf}, <atom>{0,} and <atom>{}  are equivalent to
	     *  <atom>*  */
	    if (minval == 0 && maxval == MAX_LIMIT && greedy)
	    {
		EMIT(NFA_STAR);
		break;
	    }

	    if (maxval > NFA_BRACES_MAXLIMIT)
	    {
		/* This would yield a huge automaton and use too much memory.
		 * Revert to old engine */
		return FAIL;
	    }

	    /* Special case: x{0} or x{-0} */
	    if (maxval == 0)
	    {
		/* Ignore result of previous call to nfa_regatom() */
		post_ptr = my_post_start;
		/* NFA_SKIP_CHAR has 0-length and works everywhere */
		EMIT(NFA_SKIP_CHAR);
		return OK;
	    }

	    /* Ignore previous call to nfa_regatom() */
	    post_ptr = my_post_start;
	    /* Save pos after the repeated atom and the \{} */
	    new_regparse = regparse;

	    quest = (greedy == TRUE? NFA_QUEST : NFA_QUEST_NONGREEDY);
	    for (i = 0; i < maxval; i++)
	    {
		/* Goto beginning of the repeated atom */
		regparse = old_regparse;
		curchr = -1;
		/* Restore count of parenthesis */
		regnpar = old_regnpar;
		old_post_ptr = post_ptr;
		if (nfa_regatom() == FAIL)
		    return FAIL;
		/* after "minval" times, atoms are optional */
		if (i + 1 > minval)
		    EMIT(quest);
		if (old_post_ptr != my_post_start)
		    EMIT(NFA_CONCAT);
	    }

	    /* Go to just after the repeated atom and the \{} */
	    regparse = new_regparse;
	    curchr = -1;

	    break;


	default:
	    break;
    }	/* end switch */

    if (re_multi_type(peekchr()) != NOT_MULTI)
    {
	/* Can't have a multi follow a multi. */
	syntax_error = TRUE;
	EMSG_RET_FAIL(_("E871: (NFA regexp) Can't have a multi follow a multi !"));
    }

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
    int		*old_post_ptr;

    old_post_ptr = post_ptr;

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
	old_post_ptr = post_ptr;
	if (nfa_regconcat() == FAIL)
	    return FAIL;
	/* if concat is empty, skip a input char. But do emit a node */
	if (old_post_ptr == post_ptr)
	    EMIT(NFA_SKIP_CHAR);
	EMIT(NFA_CONCAT);
	ch = peekchr();
    }

    /* Even if a branch is empty, emit one node for it */
    if (old_post_ptr == post_ptr)
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

#ifdef FEAT_SYN_HL
#endif
    if (paren == REG_PAREN)
    {
	if (regnpar >= NSUBEXP) /* Too many `(' */
	{
	    syntax_error = TRUE;
	    EMSG_RET_FAIL(_("E872: (NFA regexp) Too many '('"));
	}
	parno = regnpar++;
    }

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
	syntax_error = TRUE;
	if (paren == REG_NPAREN)
	    EMSG2_RET_FAIL(_(e_unmatchedpp), reg_magic == MAGIC_ALL);
	else
	    EMSG2_RET_FAIL(_(e_unmatchedp), reg_magic == MAGIC_ALL);
    }
    else if (paren == REG_NOPAREN && peekchr() != NUL)
    {
	syntax_error = TRUE;
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

    return OK;
}

typedef struct
{
    char_u	*start[NSUBEXP];
    char_u	*end[NSUBEXP];
    lpos_T	startpos[NSUBEXP];
    lpos_T	endpos[NSUBEXP];
} regsub_T;

static int nfa_regmatch __ARGS((nfa_state_T *start, regsub_T *submatch, regsub_T *m));

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

	case NFA_PREV_ATOM_NO_WIDTH:
			    STRCPY(code, "NFA_PREV_ATOM_NO_WIDTH"); break;
	case NFA_NOPEN:		    STRCPY(code, "NFA_MOPEN_INVISIBLE"); break;
	case NFA_NCLOSE:	    STRCPY(code, "NFA_MCLOSE_INVISIBLE"); break;
	case NFA_START_INVISIBLE:   STRCPY(code, "NFA_START_INVISIBLE"); break;
	case NFA_END_INVISIBLE:	    STRCPY(code, "NFA_END_INVISIBLE"); break;

	case NFA_COMPOSING:	    STRCPY(code, "NFA_COMPOSING"); break;
	case NFA_END_COMPOSING:	    STRCPY(code, "NFA_END_COMPOSING"); break;

	case NFA_MOPEN + 0:
	case NFA_MOPEN + 1:
	case NFA_MOPEN + 2:
	case NFA_MOPEN + 3:
	case NFA_MOPEN + 4:
	case NFA_MOPEN + 5:
	case NFA_MOPEN + 6:
	case NFA_MOPEN + 7:
	case NFA_MOPEN + 8:
	case NFA_MOPEN + 9:
	    STRCPY(code, "NFA_MOPEN(x)");
	    code[10] = c - NFA_MOPEN + '0';
	    break;
	case NFA_MCLOSE + 0:
	case NFA_MCLOSE + 1:
	case NFA_MCLOSE + 2:
	case NFA_MCLOSE + 3:
	case NFA_MCLOSE + 4:
	case NFA_MCLOSE + 5:
	case NFA_MCLOSE + 6:
	case NFA_MCLOSE + 7:
	case NFA_MCLOSE + 8:
	case NFA_MCLOSE + 9:
	    STRCPY(code, "NFA_MCLOSE(x)");
	    code[11] = c - NFA_MCLOSE + '0';
	    break;
	case NFA_EOL:		STRCPY(code, "NFA_EOL "); break;
	case NFA_BOL:		STRCPY(code, "NFA_BOL "); break;
	case NFA_EOW:		STRCPY(code, "NFA_EOW "); break;
	case NFA_BOW:		STRCPY(code, "NFA_BOW "); break;
	case NFA_STAR:		STRCPY(code, "NFA_STAR "); break;
	case NFA_PLUS:		STRCPY(code, "NFA_PLUS "); break;
	case NFA_NOT:		STRCPY(code, "NFA_NOT "); break;
	case NFA_SKIP_CHAR:	STRCPY(code, "NFA_SKIP_CHAR"); break;
	case NFA_OR:		STRCPY(code, "NFA_OR"); break;
	case NFA_QUEST:		STRCPY(code, "NFA_QUEST"); break;
	case NFA_QUEST_NONGREEDY: STRCPY(code, "NFA_QUEST_NON_GREEDY"); break;
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
new_state(c, out, out1)
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
    s->lastlist = 0;
    s->visits = 0;
    s->negated = FALSE;

    return s;
}

/*
 * A partially built NFA without the matching state filled in.
 * Frag_T.start points at the start state.
 * Frag_T.out is a list of places that need to be set to the
 * next state for this fragment.
 */
typedef union Ptrlist Ptrlist;
struct Frag
{
    nfa_state_T   *start;
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
 * Since the out pointers in the list are always
 * uninitialized, we use the pointers themselves
 * as storage for the Ptrlists.
 */
union Ptrlist
{
    Ptrlist	*next;
    nfa_state_T	*s;
};

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
	stack = (Frag_T *) lalloc((nstate + 1) * sizeof(Frag_T), TRUE);
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
	    s = new_state(NFA_SPLIT, e1.start, e2.start);
	    if (s == NULL)
		goto theend;
	    PUSH(frag(s, append(e1.out, e2.out)));
	    break;

	case NFA_STAR:
	    /* Zero or more */
	    if (nfa_calc_size == TRUE)
	    {
		nstate++;
		break;
	    }
	    e = POP();
	    s = new_state(NFA_SPLIT, e.start, NULL);
	    if (s == NULL)
		goto theend;
	    patch(e.out, s);
	    PUSH(frag(s, list1(&s->out1)));
	    break;

	case NFA_QUEST:
	    /* one or zero atoms=> greedy match */
	    if (nfa_calc_size == TRUE)
	    {
		nstate++;
		break;
	    }
	    e = POP();
	    s = new_state(NFA_SPLIT, e.start, NULL);
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
	    s = new_state(NFA_SPLIT, NULL, e.start);
	    if (s == NULL)
		goto theend;
	    PUSH(frag(s, append(e.out, list1(&s->out))));
	    break;

	case NFA_PLUS:
	    /* One or more */
	    if (nfa_calc_size == TRUE)
	    {
		nstate++;
		break;
	    }
	    e = POP();
	    s = new_state(NFA_SPLIT, e.start, NULL);
	    if (s == NULL)
		goto theend;
	    patch(e.out, s);
	    PUSH(frag(e.start, list1(&s->out1)));
	    break;

	case NFA_SKIP_CHAR:
	    /* Symbol of 0-length, Used in a repetition
	     * with max/min count of 0 */
	    if (nfa_calc_size == TRUE)
	    {
		nstate++;
		break;
	    }
	    s = new_state(NFA_SKIP_CHAR, NULL, NULL);
	    if (s == NULL)
		goto theend;
	    PUSH(frag(s, list1(&s->out)));
	    break;

	case NFA_PREV_ATOM_NO_WIDTH:
	    /* The \@= operator: match the preceding atom with 0 width.
	     * Surrounds the preceding atom with START_INVISIBLE and
	     * END_INVISIBLE, similarly to MOPEN.
	     */
	    /* TODO: Maybe this drops the speed? */
	    goto theend;

	    if (nfa_calc_size == TRUE)
	    {
		nstate += 2;
		break;
	    }
	    e = POP();
	    s1 = new_state(NFA_END_INVISIBLE, NULL, NULL);
	    if (s1 == NULL)
		goto theend;
	    patch(e.out, s1);

	    s = new_state(NFA_START_INVISIBLE, e.start, s1);
	    if (s == NULL)
		goto theend;
	    PUSH(frag(s, list1(&s1->out)));
	    break;

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

	case NFA_MOPEN + 0:	/* Submatch */
	case NFA_MOPEN + 1:
	case NFA_MOPEN + 2:
	case NFA_MOPEN + 3:
	case NFA_MOPEN + 4:
	case NFA_MOPEN + 5:
	case NFA_MOPEN + 6:
	case NFA_MOPEN + 7:
	case NFA_MOPEN + 8:
	case NFA_MOPEN + 9:
	case NFA_NOPEN:		/* \%( "Invisible Submatch" */
	    if (nfa_calc_size == TRUE)
	    {
		nstate += 2;
		break;
	    }

	    mopen = *p;
	    switch (*p)
	    {
		case NFA_NOPEN:
		    mclose = NFA_NCLOSE;
		    break;
#ifdef FEAT_MBYTE
		case NFA_COMPOSING:
		    mclose = NFA_END_COMPOSING;
		    break;
#endif
		default:
		    /* NFA_MOPEN(0) ... NFA_MOPEN(9) */
		    mclose = *p + NSUBEXP;
		    break;
	    }

	    /* Allow "NFA_MOPEN" as a valid postfix representation for
	     * the empty regexp "". In this case, the NFA will be
	     * NFA_MOPEN -> NFA_MCLOSE. Note that this also allows
	     * empty groups of parenthesis, and empty mbyte chars */
	    if (stackp == stack)
	    {
		s = new_state(mopen, NULL, NULL);
		if (s == NULL)
		    goto theend;
		s1 = new_state(mclose, NULL, NULL);
		if (s1 == NULL)
		    goto theend;
		patch(list1(&s->out), s1);
		PUSH(frag(s, list1(&s1->out)));
		break;
	    }

	    /* At least one node was emitted before NFA_MOPEN, so
	     * at least one node will be between NFA_MOPEN and NFA_MCLOSE */
	    e = POP();
	    s = new_state(mopen, e.start, NULL);   /* `(' */
	    if (s == NULL)
		goto theend;

	    s1 = new_state(mclose, NULL, NULL);   /* `)' */
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

	case NFA_ZSTART:
	case NFA_ZEND:
	default:
	    /* Operands */
	    if (nfa_calc_size == TRUE)
	    {
		nstate++;
		break;
	    }
	    s = new_state(*p, NULL, NULL);
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

/* nfa_thread_T contains runtime information of a NFA state */
typedef struct
{
    nfa_state_T	*state;
    regsub_T	sub;		/* Submatch info. TODO: expensive! */
} nfa_thread_T;


typedef struct
{
    nfa_thread_T    *t;
    int		    n;
} nfa_list_T;

static void addstate __ARGS((nfa_list_T *l, nfa_state_T *state, regsub_T *m, int off, int lid, int *match));

static void addstate_here __ARGS((nfa_list_T *l, nfa_state_T *state, regsub_T *m, int lid, int *match, int *ip));

    static void
addstate(l, state, m, off, lid, match)
    nfa_list_T		*l;	/* runtime state list */
    nfa_state_T		*state;	/* state to update */
    regsub_T		*m;	/* pointers to subexpressions */
    int			off;	/* byte offset, when -1 go to next line */
    int			lid;
    int			*match;	/* found match? */
{
    regsub_T		save;
    int			subidx = 0;
    nfa_thread_T	*lastthread;

    if (l == NULL || state == NULL)
	return;

    switch (state->c)
    {
	case NFA_SPLIT:
	case NFA_NOT:
	case NFA_NOPEN:
	case NFA_NCLOSE:
	case NFA_MCLOSE:
	case NFA_MCLOSE + 1:
	case NFA_MCLOSE + 2:
	case NFA_MCLOSE + 3:
	case NFA_MCLOSE + 4:
	case NFA_MCLOSE + 5:
	case NFA_MCLOSE + 6:
	case NFA_MCLOSE + 7:
	case NFA_MCLOSE + 8:
	case NFA_MCLOSE + 9:
	    /* Do not remember these nodes in list "thislist" or "nextlist" */
	    break;

	default:
	    if (state->lastlist == lid)
	    {
		if (++state->visits > 2)
		    return;
	    }
	    else
	    {
		/* add the state to the list */
		state->lastlist = lid;
		lastthread = &l->t[l->n++];
		lastthread->state = state;
		lastthread->sub = *m; /* TODO: expensive! */
	    }
    }

#ifdef ENABLE_LOG
    nfa_set_code(state->c);
    fprintf(log_fd, "> Adding state %d to list. Character %s, code %d\n",
	abs(state->id), code, state->c);
#endif
    switch (state->c)
    {
	case NFA_MATCH:
	    *match = TRUE;
	    break;

	case NFA_SPLIT:
	    addstate(l, state->out, m, off, lid, match);
	    addstate(l, state->out1, m, off, lid, match);
	    break;

	case NFA_SKIP_CHAR:
	    addstate(l, state->out, m, off, lid, match);
	    break;

#if 0
	case NFA_END_NEG_RANGE:
	    /* Nothing to handle here. nfa_regmatch() will take care of it */
	    break;

	case NFA_NOT:
	    EMSG(_("E999: (NFA regexp internal error) Should not process NOT node !"));
#ifdef ENABLE_LOG
	fprintf(f, "\n\n>>> E999: Added state NFA_NOT to a list ... Something went wrong ! Why wasn't it processed already? \n\n");
#endif
	    break;

	case NFA_COMPOSING:
	    /* nfa_regmatch() will match all the bytes of this composing char. */
	    break;
#endif

	case NFA_NOPEN:
	case NFA_NCLOSE:
	    addstate(l, state->out, m, off, lid, match);
	    break;

	/* If this state is reached, then a recursive call of nfa_regmatch()
	 * succeeded. the next call saves the found submatches in the
	 * first state after the "invisible" branch. */
#if 0
	case NFA_END_INVISIBLE:
	    break;
#endif

	case NFA_MOPEN + 0:
	case NFA_MOPEN + 1:
	case NFA_MOPEN + 2:
	case NFA_MOPEN + 3:
	case NFA_MOPEN + 4:
	case NFA_MOPEN + 5:
	case NFA_MOPEN + 6:
	case NFA_MOPEN + 7:
	case NFA_MOPEN + 8:
	case NFA_MOPEN + 9:
	case NFA_ZSTART:
	    subidx = state->c - NFA_MOPEN;
	    if (state->c == NFA_ZSTART)
		subidx = 0;

	    if (REG_MULTI)
	    {
		save.startpos[subidx] = m->startpos[subidx];
		save.endpos[subidx] = m->endpos[subidx];
		if (off == -1)
		{
		    m->startpos[subidx].lnum = reglnum + 1;
		    m->startpos[subidx].col = 0;
		}
		else
		{
		    m->startpos[subidx].lnum = reglnum;
		    m->startpos[subidx].col =
					  (colnr_T)(reginput - regline + off);
		}
	    }
	    else
	    {
		save.start[subidx] = m->start[subidx];
		save.end[subidx] = m->end[subidx];
		m->start[subidx] = reginput + off;
	    }

	    addstate(l, state->out, m, off, lid, match);

	    if (REG_MULTI)
	    {
		m->startpos[subidx] = save.startpos[subidx];
		m->endpos[subidx] = save.endpos[subidx];
	    }
	    else
	    {
		m->start[subidx] = save.start[subidx];
		m->end[subidx] = save.end[subidx];
	    }
	    break;

	case NFA_MCLOSE + 0:
	    if (nfa_has_zend == TRUE)
	    {
		addstate(l, state->out, m, off, lid, match);
		break;
	    }
	case NFA_MCLOSE + 1:
	case NFA_MCLOSE + 2:
	case NFA_MCLOSE + 3:
	case NFA_MCLOSE + 4:
	case NFA_MCLOSE + 5:
	case NFA_MCLOSE + 6:
	case NFA_MCLOSE + 7:
	case NFA_MCLOSE + 8:
	case NFA_MCLOSE + 9:
	case NFA_ZEND:
	    subidx = state->c - NFA_MCLOSE;
	    if (state->c == NFA_ZEND)
		subidx = 0;

	    if (REG_MULTI)
	    {
		save.startpos[subidx] = m->startpos[subidx];
		save.endpos[subidx] = m->endpos[subidx];
		if (off == -1)
		{
		    m->endpos[subidx].lnum = reglnum + 1;
		    m->endpos[subidx].col = 0;
		}
		else
		{
		    m->endpos[subidx].lnum = reglnum;
		    m->endpos[subidx].col = (colnr_T)(reginput - regline + off);
		}
	    }
	    else
	    {
		save.start[subidx] = m->start[subidx];
		save.end[subidx] = m->end[subidx];
		m->end[subidx] = reginput + off;
	    }

	    addstate(l, state->out, m, off, lid, match);

	    if (REG_MULTI)
	    {
		m->startpos[subidx] = save.startpos[subidx];
		m->endpos[subidx] = save.endpos[subidx];
	    }
	    else
	    {
		m->start[subidx] = save.start[subidx];
		m->end[subidx] = save.end[subidx];
	    }
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
addstate_here(l, state, m, lid, matchp, ip)
    nfa_list_T		*l;	/* runtime state list */
    nfa_state_T		*state;	/* state to update */
    regsub_T		*m;	/* pointers to subexpressions */
    int			lid;
    int			*matchp;	/* found match? */
    int			*ip;
{
    int tlen = l->n;
    int count;
    int i = *ip;

    /* first add the state(s) at the end, so that we know how many there are */
    addstate(l, state, m, 0, lid, matchp);

    /* when "*ip" was at the end of the list, nothing to do */
    if (i + 1 == tlen)
	return;

    /* re-order to put the new state at the current position */
    count = l->n - tlen;
    if (count > 1)
    {
	/* make space for new states, then move them from the
	 * end to the current position */
	mch_memmove(&(l->t[i + count]),
		&(l->t[i + 1]),
		sizeof(nfa_thread_T) * (l->n - i - 1));
	mch_memmove(&(l->t[i]),
		&(l->t[l->n - 1]),
		sizeof(nfa_thread_T) * count);
    }
    else
    {
	/* overwrite the current state */
	l->t[i] = l->t[l->n - 1];
    }
    --l->n;
    *ip = i - 1;
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

/*
 * Set all NFA nodes' list ID equal to -1.
 */
    static void
nfa_set_neg_listids(start)
    nfa_state_T	    *start;
{
    if (start == NULL)
	return;
    if (start->lastlist >= 0)
    {
	start->lastlist = -1;
	nfa_set_neg_listids(start->out);
	nfa_set_neg_listids(start->out1);
    }
}

/*
 * Set all NFA nodes' list ID equal to 0.
 */
    static void
nfa_set_null_listids(start)
    nfa_state_T	    *start;
{
    if (start == NULL)
	return;
    if (start->lastlist == -1)
    {
	start->lastlist = 0;
	nfa_set_null_listids(start->out);
	nfa_set_null_listids(start->out1);
    }
}

/*
 * Save list IDs for all NFA states in "list".
 */
    static void
nfa_save_listids(start, list)
    nfa_state_T	    *start;
    int		    *list;
{
    if (start == NULL)
	return;
    if (start->lastlist != -1)
    {
	list[abs(start->id)] = start->lastlist;
	start->lastlist = -1;
	nfa_save_listids(start->out, list);
	nfa_save_listids(start->out1, list);
    }
}

/*
 * Restore list IDs from "list" to all NFA states.
 */
    static void
nfa_restore_listids(start, list)
    nfa_state_T	    *start;
    int		    *list;
{
    if (start == NULL)
	return;
    if (start->lastlist == -1)
    {
	start->lastlist = list[abs(start->id)];
	nfa_restore_listids(start->out, list);
	nfa_restore_listids(start->out1, list);
    }
}

/*
 * Main matching routine.
 *
 * Run NFA to determine whether it matches reginput.
 *
 * Return TRUE if there is a match, FALSE otherwise.
 * Note: Caller must ensure that: start != NULL.
 */
    static int
nfa_regmatch(start, submatch, m)
    nfa_state_T		*start;
    regsub_T		*submatch;
    regsub_T		*m;
{
    int		c;
    int		n;
    int		i = 0;
    int		result;
    int		size = 0;
    int		match = FALSE;
    int		flag = 0;
    int		old_reglnum = -1;
    int		go_to_nextline = FALSE;
    nfa_thread_T *t;
    char_u	*old_reginput = NULL;
    char_u	*old_regline = NULL;
    nfa_list_T	list[3];
    nfa_list_T	*listtbl[2][2];
    nfa_list_T	*ll;
    int		listid = 1;
    nfa_list_T	*thislist;
    nfa_list_T	*nextlist;
    nfa_list_T	*neglist;
    int		*listids = NULL;
    int		j = 0;
#ifdef NFA_REGEXP_DEBUG_LOG
    FILE	*debug = fopen(NFA_REGEXP_DEBUG_LOG, "a");

    if (debug == NULL)
    {
	EMSG2(_("(NFA) COULD NOT OPEN %s !"), NFA_REGEXP_DEBUG_LOG);
	return FALSE;
    }
#endif

    /* Allocate memory for the lists of nodes */
    size = (nstate + 1) * sizeof(nfa_thread_T);
    list[0].t = (nfa_thread_T *)lalloc(size, TRUE);
    list[1].t = (nfa_thread_T *)lalloc(size, TRUE);
    list[2].t = (nfa_thread_T *)lalloc(size, TRUE);
    if (list[0].t == NULL || list[1].t == NULL || list[2].t == NULL)
	goto theend;
    vim_memset(list[0].t, 0, size);
    vim_memset(list[1].t, 0, size);
    vim_memset(list[2].t, 0, size);

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
    addstate(thislist, start, m, 0, listid, &match);

    /* There are two cases when the NFA advances: 1. input char matches the
     * NFA node and 2. input char does not match the NFA node, but the next
     * node is NFA_NOT. The following macro calls addstate() according to
     * these rules. It is used A LOT, so use the "listtbl" table for speed */
    listtbl[0][0] = NULL;
    listtbl[0][1] = neglist;
    listtbl[1][0] = nextlist;
    listtbl[1][1] = NULL;
#define	ADD_POS_NEG_STATE(node)						    \
    ll = listtbl[result ? 1 : 0][node->negated];			    \
    if (ll != NULL)							    \
	addstate(ll, node->out , &t->sub, n, listid + 1, &match);


    /*
     * Run for each character.
     */
    for (;;)
    {
#ifdef FEAT_MBYTE
	if (has_mbyte)
	{
	    c = (*mb_ptr2char)(reginput);
	    n = (*mb_ptr2len)(reginput);
	}
	else
#endif
	{
	    c = *reginput;
	    n = 1;
	}
	if (c == NUL)
	{
	    n = 0;
	    go_to_nextline = FALSE;
	}

	/* swap lists */
	thislist = &list[flag];
	nextlist = &list[flag ^= 1];
	nextlist->n = 0;	    /* `clear' nextlist */
	listtbl[1][0] = nextlist;
	++listid;

#ifdef ENABLE_LOG
	fprintf(log_fd, "------------------------------------------\n");
	fprintf(log_fd, ">>> Reginput is \"%s\"\n", reginput);
	fprintf(log_fd, ">>> Advanced one character ... Current char is %c (code %d) \n", c, (int)c);
	fprintf(log_fd, ">>> Thislist has %d states available: ", thislist->n);
	for (i = 0; i < thislist->n; i++)
	    fprintf(log_fd, "%d  ", abs(thislist->t[i].state->id));
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
	for (i = 0; i < thislist->n || neglist->n > 0; ++i)
	{
	    if (neglist->n > 0)
	    {
		t = &neglist->t[0];
		neglist->n--;
		i--;
	    }
	    else
		t = &thislist->t[i];

#ifdef NFA_REGEXP_DEBUG_LOG
	    nfa_set_code(t->state->c);
	    fprintf(debug, "%s, ", code);
#endif
#ifdef ENABLE_LOG
	    nfa_set_code(t->state->c);
	    fprintf(log_fd, "(%d) %s, code %d ... \n", abs(t->state->id),
						      code, (int)t->state->c);
#endif

	    /*
	     * Handle the possible codes of the current state.
	     * The most important is NFA_MATCH.
	     */
	    switch (t->state->c)
	    {
	    case NFA_MATCH:
		match = TRUE;
		*submatch = t->sub;
#ifdef ENABLE_LOG
		for (j = 0; j < 4; j++)
		    if (REG_MULTI)
			fprintf(log_fd, "\n *** group %d, start: c=%d, l=%d, end: c=%d, l=%d",
				j,
				t->sub.startpos[j].col,
				(int)t->sub.startpos[j].lnum,
				t->sub.endpos[j].col,
				(int)t->sub.endpos[j].lnum);
		    else
			fprintf(log_fd, "\n *** group %d, start: \"%s\", end: \"%s\"",
				j,
				(char *)t->sub.start[j],
				(char *)t->sub.end[j]);
		fprintf(log_fd, "\n");
#endif
		/* Found the left-most longest match, do not look at any other
		 * states at this position. */
		goto nextchar;

	    case NFA_END_INVISIBLE:
		/* This is only encountered after a NFA_START_INVISIBLE node.
		 * They surround a zero-width group, used with "\@=" and "\&".
		 * If we got here, it means that the current "invisible" group
		 * finished successfully, so return control to the parent
		 * nfa_regmatch().  Submatches are stored in *m, and used in
		 * the parent call. */
		if (start->c == NFA_MOPEN + 0)
		    addstate_here(thislist, t->state->out, &t->sub, listid,
								  &match, &i);
		else
		{
		    *m = t->sub;
		    match = TRUE;
		}
		break;

	    case NFA_START_INVISIBLE:
		/* Save global variables, and call nfa_regmatch() to check if
		 * the current concat matches at this position. The concat
		 * ends with the node NFA_END_INVISIBLE */
		old_reginput = reginput;
		old_regline = regline;
		old_reglnum = reglnum;
		if (listids == NULL)
		{
		    listids = (int *) lalloc(sizeof(int) * nstate, TRUE);
		    if (listids == NULL)
		    {
			EMSG(_("E878: (NFA) Could not allocate memory for branch traversal!"));
			return 0;
		    }
		}
#ifdef ENABLE_LOG
		if (log_fd != stderr)
		    fclose(log_fd);
		log_fd = NULL;
#endif
		/* Have to clear the listid field of the NFA nodes, so that
		 * nfa_regmatch() and addstate() can run properly after
		 * recursion. */
		nfa_save_listids(start, listids);
		nfa_set_null_listids(start);
		result = nfa_regmatch(t->state->out, submatch, m);
		nfa_set_neg_listids(start);
		nfa_restore_listids(start, listids);

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
		if (result == TRUE)
		{
		    /* Restore position in input text */
		    reginput = old_reginput;
		    regline = old_regline;
		    reglnum = old_reglnum;
		    /* Copy submatch info from the recursive call */
		    if (REG_MULTI)
			for (j = 1; j < NSUBEXP; j++)
			{
			    t->sub.startpos[j] = m->startpos[j];
			    t->sub.endpos[j] = m->endpos[j];
			}
		    else
			for (j = 1; j < NSUBEXP; j++)
			{
			    t->sub.start[j] = m->start[j];
			    t->sub.end[j] = m->end[j];
			}
		    /* t->state->out1 is the corresponding END_INVISIBLE node */
		    addstate_here(thislist, t->state->out1->out, &t->sub,
							  listid, &match, &i);
		}
		else
		{
		    /* continue with next input char */
		    reginput = old_reginput;
		}
		break;

	    case NFA_BOL:
		if (reginput == regline)
		    addstate_here(thislist, t->state->out, &t->sub, listid,
								  &match, &i);
		break;

	    case NFA_EOL:
		if (c == NUL)
		    addstate_here(thislist, t->state->out, &t->sub, listid,
								  &match, &i);
		break;

	    case NFA_BOW:
	    {
		int bow = TRUE;

		if (c == NUL)
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
		else if (!vim_iswordc_buf(c, reg_buf)
			   || (reginput > regline
				   && vim_iswordc_buf(reginput[-1], reg_buf)))
		    bow = FALSE;
		if (bow)
		    addstate_here(thislist, t->state->out, &t->sub, listid,
								  &match, &i);
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
			|| (reginput[0] != NUL && vim_iswordc_buf(c, reg_buf)))
		    eow = FALSE;
		if (eow)
		    addstate_here(thislist, t->state->out, &t->sub, listid,
								  &match, &i);
		break;
	    }

#ifdef FEAT_MBYTE
	    case NFA_COMPOSING:
	    {
		int	    mc = c;
		int	    len = 0;
		nfa_state_T *end;
		nfa_state_T *sta;

		result = OK;
		sta = t->state->out;
		len = 0;
		if (utf_iscomposing(sta->c))
		{
		    /* Only match composing character(s), ignore base
		     * character.  Used for ".{composing}" and "{composing}"
		     * (no preceding character). */
		    len += mb_char2len(c);
		}
		if (ireg_icombine)
		{
		    /* If \Z was present, then ignore composing characters.
		     * When ignoring the base character this always matches. */
		    /* TODO: How about negated? */
		    if (len == 0 && sta->c != c)
			result = FAIL;
		    len = n;
		    while (sta->c != NFA_END_COMPOSING)
			sta = sta->out;
		}
		else
		    while (sta->c != NFA_END_COMPOSING && len < n)
		    {
			if (len > 0)
			    mc = mb_ptr2char(reginput + len);
			if (mc != sta->c)
			    break;
			len += mb_char2len(mc);
			sta = sta->out;
		    }

		/* if input char length doesn't match regexp char length */
		if (len < n || sta->c != NFA_END_COMPOSING)
		    result = FAIL;
		end = t->state->out1;	    /* NFA_END_COMPOSING */
		ADD_POS_NEG_STATE(end);
		break;
	    }
#endif

	    case NFA_NEWL:
		if (!reg_line_lbr && REG_MULTI
					&& c == NUL && reglnum <= reg_maxline)
		{
		    go_to_nextline = TRUE;
		    /* Pass -1 for the offset, which means taking the position
		     * at the start of the next line. */
		    addstate(nextlist, t->state->out, &t->sub, -1,
							  listid + 1, &match);
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
		result = check_char_class(t->state->c, c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_END_NEG_RANGE:
		/* This follows a series of negated nodes, like:
		 * CHAR(x), NFA_NOT, CHAR(y), NFA_NOT etc. */
		if (c > 0)
		    addstate(nextlist, t->state->out, &t->sub, n, listid + 1,
								    &match);
		break;

	    case NFA_ANY:
		/* Any char except '\0', (end of input) does not match. */
		if (c > 0)
		    addstate(nextlist, t->state->out, &t->sub, n, listid + 1,
								    &match);
		break;

	    /*
	     * Character classes like \a for alpha, \d for digit etc.
	     */
	    case NFA_IDENT:	/*  \i	*/
		result = vim_isIDc(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_SIDENT:	/*  \I	*/
		result = !VIM_ISDIGIT(c) && vim_isIDc(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_KWORD:	/*  \k	*/
		result = vim_iswordp_buf(reginput, reg_buf);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_SKWORD:	/*  \K	*/
		result = !VIM_ISDIGIT(c) && vim_iswordp_buf(reginput, reg_buf);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_FNAME:	/*  \f	*/
		result = vim_isfilec(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_SFNAME:	/*  \F	*/
		result = !VIM_ISDIGIT(c) && vim_isfilec(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_PRINT:	/*  \p	*/
		result = ptr2cells(reginput) == 1;
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_SPRINT:	/*  \P	*/
		result = !VIM_ISDIGIT(c) && ptr2cells(reginput) == 1;
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_WHITE:	/*  \s	*/
		result = vim_iswhite(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NWHITE:	/*  \S	*/
		result = c != NUL && !vim_iswhite(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_DIGIT:	/*  \d	*/
		result = ri_digit(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NDIGIT:	/*  \D	*/
		result = c != NUL && !ri_digit(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_HEX:	/*  \x	*/
		result = ri_hex(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NHEX:	/*  \X	*/
		result = c != NUL && !ri_hex(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_OCTAL:	/*  \o	*/
		result = ri_octal(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NOCTAL:	/*  \O	*/
		result = c != NUL && !ri_octal(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_WORD:	/*  \w	*/
		result = ri_word(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NWORD:	/*  \W	*/
		result = c != NUL && !ri_word(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_HEAD:	/*  \h	*/
		result = ri_head(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NHEAD:	/*  \H	*/
		result = c != NUL && !ri_head(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_ALPHA:	/*  \a	*/
		result = ri_alpha(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NALPHA:	/*  \A	*/
		result = c != NUL && !ri_alpha(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_LOWER:	/*  \l	*/
		result = ri_lower(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NLOWER:	/*  \L	*/
		result = c != NUL && !ri_lower(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_UPPER:	/*  \u	*/
		result = ri_upper(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_NUPPER:	/* \U	*/
		result = c != NUL && !ri_upper(c);
		ADD_POS_NEG_STATE(t->state);
		break;

	    case NFA_MOPEN + 0:
	    case NFA_MOPEN + 1:
	    case NFA_MOPEN + 2:
	    case NFA_MOPEN + 3:
	    case NFA_MOPEN + 4:
	    case NFA_MOPEN + 5:
	    case NFA_MOPEN + 6:
	    case NFA_MOPEN + 7:
	    case NFA_MOPEN + 8:
	    case NFA_MOPEN + 9:
		/* handled below */
		break;

	    case NFA_SKIP_CHAR:
	    case NFA_ZSTART:
		/* TODO: should not happen? */
		break;

	    default:	/* regular character */
		/* TODO: put this in #ifdef later */
		if (t->state->c < -256)
		    EMSGN("INTERNAL: Negative state char: %ld", t->state->c);
		result = (no_Magic(t->state->c) == c);

		if (!result)
		    result = ireg_ic == TRUE
				&& MB_TOLOWER(t->state->c) == MB_TOLOWER(c);
#ifdef FEAT_MBYTE
		/* If there is a composing character which is not being
		 * ignored there can be no match. Match with composing
		 * character uses NFA_COMPOSING above. */
		if (result && enc_utf8 && !ireg_icombine
						      && n != utf_char2len(c))
		    result = FALSE;
#endif
		ADD_POS_NEG_STATE(t->state);
		break;
	    }

	} /* for (thislist = thislist; thislist->state; thislist++) */

	/* The first found match is the leftmost one, but there may be a
	 * longer one. Keep running the NFA, but don't start from the
	 * beginning. Also, do not add the start state in recursive calls of
	 * nfa_regmatch(), because recursive calls should only start in the
	 * first position. */
	if (match == FALSE && start->c == NFA_MOPEN + 0)
	{
#ifdef ENABLE_LOG
	    fprintf(log_fd, "(---) STARTSTATE\n");
#endif
	    addstate(nextlist, start, m, n, listid + 1, &match);
	}

#ifdef ENABLE_LOG
	fprintf(log_fd, ">>> Thislist had %d states available: ", thislist->n);
	for (i = 0; i< thislist->n; i++)
	    fprintf(log_fd, "%d  ", abs(thislist->t[i].state->id));
	fprintf(log_fd, "\n");
#endif

nextchar:
	/* Advance to the next character, or advance to the next line, or
	 * finish. */
	if (n != 0)
	    reginput += n;
	else if (go_to_nextline)
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
    list[0].t = list[1].t = list[2].t = NULL;
    if (listids != NULL)
	vim_free(listids);
#undef ADD_POS_NEG_STATE
#ifdef NFA_REGEXP_DEBUG_LOG
    fclose(debug);
#endif

    return match;
}

/*
 * Try match of "prog" with at regline["col"].
 * Returns 0 for failure, number of lines contained in the match otherwise.
 */
    static long
nfa_regtry(start, col)
    nfa_state_T	*start;
    colnr_T	col;
{
    int		i;
    regsub_T	sub, m;
#ifdef ENABLE_LOG
    FILE	*f;
#endif

    reginput = regline + col;
    need_clear_subexpr = TRUE;

#ifdef ENABLE_LOG
    f = fopen(NFA_REGEXP_RUN_LOG, "a");
    if (f != NULL)
    {
	fprintf(f, "\n\n\n\n\n\n\t\t=======================================================\n");
	fprintf(f, "		=======================================================\n");
#ifdef DEBUG
	fprintf(f, "\tRegexp is \"%s\"\n", nfa_regengine.expr);
#endif
	fprintf(f, "\tInput text is \"%s\" \n", reginput);
	fprintf(f, "		=======================================================\n\n\n\n\n\n\n");
	nfa_print_state(f, start);
	fprintf(f, "\n\n");
	fclose(f);
    }
    else
	EMSG(_("Could not open temporary log file for writing "));
#endif

    if (REG_MULTI)
    {
	/* Use 0xff to set lnum to -1 */
	vim_memset(sub.startpos, 0xff, sizeof(lpos_T) * NSUBEXP);
	vim_memset(sub.endpos, 0xff, sizeof(lpos_T) * NSUBEXP);
	vim_memset(m.startpos, 0xff, sizeof(lpos_T) * NSUBEXP);
	vim_memset(m.endpos, 0xff, sizeof(lpos_T) * NSUBEXP);
    }
    else
    {
	vim_memset(sub.start, 0, sizeof(char_u *) * NSUBEXP);
	vim_memset(sub.end, 0, sizeof(char_u *) * NSUBEXP);
	vim_memset(m.start, 0, sizeof(char_u *) * NSUBEXP);
	vim_memset(m.end, 0, sizeof(char_u *) * NSUBEXP);
    }

    if (nfa_regmatch(start, &sub, &m) == FALSE)
	return 0;

    cleanup_subexpr();
    if (REG_MULTI)
    {
	for (i = 0; i < NSUBEXP; i++)
	{
	    reg_startpos[i] = sub.startpos[i];
	    reg_endpos[i] = sub.endpos[i];
	}

	if (reg_startpos[0].lnum < 0)
	{
	    reg_startpos[0].lnum = 0;
	    reg_startpos[0].col = col;
	}
	if (reg_endpos[0].lnum < 0)
	{
	    reg_endpos[0].lnum = reglnum;
	    reg_endpos[0].col = (int)(reginput - regline);
	}
	else
	    /* Use line number of "\ze". */
	    reglnum = reg_endpos[0].lnum;
    }
    else
    {
	for (i = 0; i < NSUBEXP; i++)
	{
	    reg_startp[i] = sub.start[i];
	    reg_endp[i] = sub.end[i];
	}

	if (reg_startp[0] == NULL)
	    reg_startp[0] = regline + col;
	if (reg_endp[0] == NULL)
	    reg_endp[0] = reginput;
    }

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

    nstate = prog->nstate;

    for (i = 0; i < nstate; ++i)
    {
	prog->state[i].id = i;
	prog->state[i].lastlist = 0;
	prog->state[i].visits = 0;
    }

    retval = nfa_regtry(prog->start, col);

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
	goto fail;	    /* Cascaded (syntax?) error */

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
#ifdef ENABLE_LOG
    nfa_postfix_dump(expr, OK);
    nfa_dump(prog);
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
