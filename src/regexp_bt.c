/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * Backtracking regular expression implementation.
 *
 * This file is included in "regexp.c".
 */

static long	bt_regexec_both(char_u *line, colnr_T col, proftime_T *tm, int *timed_out);
static long	regtry(bt_regprog_T *prog, colnr_T col, proftime_T *tm, int *timed_out);
#ifdef BT_REGEXP_DUMP
static void	regdump(char_u *, bt_regprog_T *);
#endif

/*
 * bt_regcomp() - compile a regular expression into internal code for the
 * traditional back track matcher.
 * Returns the program in allocated space.  Returns NULL for an error.
 *
 * We can't allocate space until we know how big the compiled form will be,
 * but we can't compile it (and thus know how big it is) until we've got a
 * place to put the code.  So we cheat:  we compile it twice, once with code
 * generation turned off and size counting turned on, and once "for real".
 * This also means that we don't allocate space until we are sure that the
 * thing really will compile successfully, and we never have to move the
 * code and thus invalidate pointers into it.  (Note that it has to be in
 * one piece because vim_free() must be able to free it all.)
 *
 * Whether upper/lower case is to be ignored is decided when executing the
 * program, it does not matter here.
 *
 * Beware that the optimization-preparation code in here knows about some
 * of the structure of the compiled regexp.
 * "re_flags": RE_MAGIC and/or RE_STRING.
 */
    static regprog_T *
bt_regcomp(char_u *expr, int re_flags)
{
    bt_regprog_T    *r;
    char_u	*scan;
    char_u	*longest;
    int		len;
    int		flags;

    if (expr == NULL)
	EMSG_RET_NULL(_(e_null));

    init_class_tab();

    // First pass: determine size, legality.
    regcomp_start(expr, re_flags);
    regcode = JUST_CALC_SIZE;
    regc(REGMAGIC);
    if (reg(REG_NOPAREN, &flags) == NULL)
	return NULL;

    // Allocate space.
    r = alloc(offsetof(bt_regprog_T, program) + regsize);
    if (r == NULL)
	return NULL;
    r->re_in_use = FALSE;

    // Second pass: emit code.
    regcomp_start(expr, re_flags);
    regcode = r->program;
    regc(REGMAGIC);
    if (reg(REG_NOPAREN, &flags) == NULL || reg_toolong)
    {
	vim_free(r);
	if (reg_toolong)
	    EMSG_RET_NULL(_("E339: Pattern too long"));
	return NULL;
    }

    // Dig out information for optimizations.
    r->regstart = NUL;		// Worst-case defaults.
    r->reganch = 0;
    r->regmust = NULL;
    r->regmlen = 0;
    r->regflags = regflags;
    if (flags & HASNL)
	r->regflags |= RF_HASNL;
    if (flags & HASLOOKBH)
	r->regflags |= RF_LOOKBH;
#ifdef FEAT_SYN_HL
    // Remember whether this pattern has any \z specials in it.
    r->reghasz = re_has_z;
#endif
    scan = r->program + 1;	// First BRANCH.
    if (OP(regnext(scan)) == END)   // Only one top-level choice.
    {
	scan = OPERAND(scan);

	// Starting-point info.
	if (OP(scan) == BOL || OP(scan) == RE_BOF)
	{
	    r->reganch++;
	    scan = regnext(scan);
	}

	if (OP(scan) == EXACTLY)
	{
	    if (has_mbyte)
		r->regstart = (*mb_ptr2char)(OPERAND(scan));
	    else
		r->regstart = *OPERAND(scan);
	}
	else if ((OP(scan) == BOW
		    || OP(scan) == EOW
		    || OP(scan) == NOTHING
		    || OP(scan) == MOPEN + 0 || OP(scan) == NOPEN
		    || OP(scan) == MCLOSE + 0 || OP(scan) == NCLOSE)
		 && OP(regnext(scan)) == EXACTLY)
	{
	    if (has_mbyte)
		r->regstart = (*mb_ptr2char)(OPERAND(regnext(scan)));
	    else
		r->regstart = *OPERAND(regnext(scan));
	}

	// If there's something expensive in the r.e., find the longest
	// literal string that must appear and make it the regmust.  Resolve
	// ties in favor of later strings, since the regstart check works
	// with the beginning of the r.e. and avoiding duplication
	// strengthens checking.  Not a strong reason, but sufficient in the
	// absence of others.

	// When the r.e. starts with BOW, it is faster to look for a regmust
	// first. Used a lot for "#" and "*" commands. (Added by mool).
	if ((flags & SPSTART || OP(scan) == BOW || OP(scan) == EOW)
							  && !(flags & HASNL))
	{
	    longest = NULL;
	    len = 0;
	    for (; scan != NULL; scan = regnext(scan))
		if (OP(scan) == EXACTLY && STRLEN(OPERAND(scan)) >= (size_t)len)
		{
		    longest = OPERAND(scan);
		    len = (int)STRLEN(OPERAND(scan));
		}
	    r->regmust = longest;
	    r->regmlen = len;
	}
    }
#ifdef BT_REGEXP_DUMP
    regdump(expr, r);
#endif
    r->engine = &bt_regengine;
    return (regprog_T *)r;
}

/*
 * Free a compiled regexp program, returned by bt_regcomp().
 */
    static void
bt_regfree(regprog_T *prog)
{
    vim_free(prog);
}

/*
 * Match a regexp against a string.
 * "rmp->regprog" is a compiled regexp as returned by vim_regcomp().
 * Uses curbuf for line count and 'iskeyword'.
 * if "line_lbr" is TRUE  consider a "\n" in "line" to be a line break.
 *
 * Returns 0 for failure, number of lines contained in the match otherwise.
 */
    static int
bt_regexec_nl(
    regmatch_T	*rmp,
    char_u	*line,	// string to match against
    colnr_T	col,	// column to start looking for match
    int		line_lbr)
{
    rex.reg_match = rmp;
    rex.reg_mmatch = NULL;
    rex.reg_maxline = 0;
    rex.reg_line_lbr = line_lbr;
    rex.reg_buf = curbuf;
    rex.reg_win = NULL;
    rex.reg_ic = rmp->rm_ic;
    rex.reg_icombine = FALSE;
    rex.reg_maxcol = 0;

    return bt_regexec_both(line, col, NULL, NULL);
}

/*
 * Match a regexp against multiple lines.
 * "rmp->regprog" is a compiled regexp as returned by vim_regcomp().
 * Uses curbuf for line count and 'iskeyword'.
 *
 * Return zero if there is no match.  Return number of lines contained in the
 * match otherwise.
 */
    static long
bt_regexec_multi(
    regmmatch_T	*rmp,
    win_T	*win,		// window in which to search or NULL
    buf_T	*buf,		// buffer in which to search
    linenr_T	lnum,		// nr of line to start looking for match
    colnr_T	col,		// column to start looking for match
    proftime_T	*tm,		// timeout limit or NULL
    int		*timed_out)	// flag set on timeout or NULL
{
    rex.reg_match = NULL;
    rex.reg_mmatch = rmp;
    rex.reg_buf = buf;
    rex.reg_win = win;
    rex.reg_firstlnum = lnum;
    rex.reg_maxline = rex.reg_buf->b_ml.ml_line_count - lnum;
    rex.reg_line_lbr = FALSE;
    rex.reg_ic = rmp->rmm_ic;
    rex.reg_icombine = FALSE;
    rex.reg_maxcol = rmp->rmm_maxcol;

    return bt_regexec_both(NULL, col, tm, timed_out);
}

/*
 * Match a regexp against a string ("line" points to the string) or multiple
 * lines ("line" is NULL, use reg_getline()).
 * Returns 0 for failure, number of lines contained in the match otherwise.
 */
    static long
bt_regexec_both(
    char_u	*line,
    colnr_T	col,		// column to start looking for match
    proftime_T	*tm,		// timeout limit or NULL
    int		*timed_out)	// flag set on timeout or NULL
{
    bt_regprog_T    *prog;
    char_u	    *s;
    long	    retval = 0L;

    // Create "regstack" and "backpos" if they are not allocated yet.
    // We allocate *_INITIAL amount of bytes first and then set the grow size
    // to much bigger value to avoid many malloc calls in case of deep regular
    // expressions.
    if (regstack.ga_data == NULL)
    {
	// Use an item size of 1 byte, since we push different things
	// onto the regstack.
	ga_init2(&regstack, 1, REGSTACK_INITIAL);
	(void)ga_grow(&regstack, REGSTACK_INITIAL);
	regstack.ga_growsize = REGSTACK_INITIAL * 8;
    }

    if (backpos.ga_data == NULL)
    {
	ga_init2(&backpos, sizeof(backpos_T), BACKPOS_INITIAL);
	(void)ga_grow(&backpos, BACKPOS_INITIAL);
	backpos.ga_growsize = BACKPOS_INITIAL * 8;
    }

    if (REG_MULTI)
    {
	prog = (bt_regprog_T *)rex.reg_mmatch->regprog;
	line = reg_getline((linenr_T)0);
	rex.reg_startpos = rex.reg_mmatch->startpos;
	rex.reg_endpos = rex.reg_mmatch->endpos;
    }
    else
    {
	prog = (bt_regprog_T *)rex.reg_match->regprog;
	rex.reg_startp = rex.reg_match->startp;
	rex.reg_endp = rex.reg_match->endp;
    }

    // Be paranoid...
    if (prog == NULL || line == NULL)
    {
	emsg(_(e_null));
	goto theend;
    }

    // Check validity of program.
    if (prog_magic_wrong())
	goto theend;

    // If the start column is past the maximum column: no need to try.
    if (rex.reg_maxcol > 0 && col >= rex.reg_maxcol)
	goto theend;

    // If pattern contains "\c" or "\C": overrule value of rex.reg_ic
    if (prog->regflags & RF_ICASE)
	rex.reg_ic = TRUE;
    else if (prog->regflags & RF_NOICASE)
	rex.reg_ic = FALSE;

    // If pattern contains "\Z" overrule value of rex.reg_icombine
    if (prog->regflags & RF_ICOMBINE)
	rex.reg_icombine = TRUE;

    // If there is a "must appear" string, look for it.
    if (prog->regmust != NULL)
    {
	int c;

	if (has_mbyte)
	    c = (*mb_ptr2char)(prog->regmust);
	else
	    c = *prog->regmust;
	s = line + col;

	// This is used very often, esp. for ":global".  Use three versions of
	// the loop to avoid overhead of conditions.
	if (!rex.reg_ic && !has_mbyte)
	    while ((s = vim_strbyte(s, c)) != NULL)
	    {
		if (cstrncmp(s, prog->regmust, &prog->regmlen) == 0)
		    break;		// Found it.
		++s;
	    }
	else if (!rex.reg_ic || (!enc_utf8 && mb_char2len(c) > 1))
	    while ((s = vim_strchr(s, c)) != NULL)
	    {
		if (cstrncmp(s, prog->regmust, &prog->regmlen) == 0)
		    break;		// Found it.
		MB_PTR_ADV(s);
	    }
	else
	    while ((s = cstrchr(s, c)) != NULL)
	    {
		if (cstrncmp(s, prog->regmust, &prog->regmlen) == 0)
		    break;		// Found it.
		MB_PTR_ADV(s);
	    }
	if (s == NULL)		// Not present.
	    goto theend;
    }

    rex.line = line;
    rex.lnum = 0;
    reg_toolong = FALSE;

    // Simplest case: Anchored match need be tried only once.
    if (prog->reganch)
    {
	int	c;

	if (has_mbyte)
	    c = (*mb_ptr2char)(rex.line + col);
	else
	    c = rex.line[col];
	if (prog->regstart == NUL
		|| prog->regstart == c
		|| (rex.reg_ic
		    && (((enc_utf8 && utf_fold(prog->regstart) == utf_fold(c)))
			|| (c < 255 && prog->regstart < 255 &&
			    MB_TOLOWER(prog->regstart) == MB_TOLOWER(c)))))
	    retval = regtry(prog, col, tm, timed_out);
	else
	    retval = 0;
    }
    else
    {
#ifdef FEAT_RELTIME
	int tm_count = 0;
#endif
	// Messy cases:  unanchored match.
	while (!got_int)
	{
	    if (prog->regstart != NUL)
	    {
		// Skip until the char we know it must start with.
		// Used often, do some work to avoid call overhead.
		if (!rex.reg_ic && !has_mbyte)
		    s = vim_strbyte(rex.line + col, prog->regstart);
		else
		    s = cstrchr(rex.line + col, prog->regstart);
		if (s == NULL)
		{
		    retval = 0;
		    break;
		}
		col = (int)(s - rex.line);
	    }

	    // Check for maximum column to try.
	    if (rex.reg_maxcol > 0 && col >= rex.reg_maxcol)
	    {
		retval = 0;
		break;
	    }

	    retval = regtry(prog, col, tm, timed_out);
	    if (retval > 0)
		break;

	    // if not currently on the first line, get it again
	    if (rex.lnum != 0)
	    {
		rex.lnum = 0;
		rex.line = reg_getline((linenr_T)0);
	    }
	    if (rex.line[col] == NUL)
		break;
	    if (has_mbyte)
		col += (*mb_ptr2len)(rex.line + col);
	    else
		++col;
#ifdef FEAT_RELTIME
	    // Check for timeout once in a twenty times to avoid overhead.
	    if (tm != NULL && ++tm_count == 20)
	    {
		tm_count = 0;
		if (profile_passed_limit(tm))
		{
		    if (timed_out != NULL)
			*timed_out = TRUE;
		    break;
		}
	    }
#endif
	}
    }

theend:
    // Free "reg_tofree" when it's a bit big.
    // Free regstack and backpos if they are bigger than their initial size.
    if (reg_tofreelen > 400)
	VIM_CLEAR(reg_tofree);
    if (regstack.ga_maxlen > REGSTACK_INITIAL)
	ga_clear(&regstack);
    if (backpos.ga_maxlen > BACKPOS_INITIAL)
	ga_clear(&backpos);

    return retval;
}

/*
 * regtry - try match of "prog" with at rex.line["col"].
 * Returns 0 for failure, number of lines contained in the match otherwise.
 */
    static long
regtry(
    bt_regprog_T	*prog,
    colnr_T		col,
    proftime_T		*tm,		// timeout limit or NULL
    int			*timed_out)	// flag set on timeout or NULL
{
    rex.input = rex.line + col;
    rex.need_clear_subexpr = TRUE;
#ifdef FEAT_SYN_HL
    // Clear the external match subpointers if necessary.
    rex.need_clear_zsubexpr = (prog->reghasz == REX_SET);
#endif

    if (regmatch(prog->program + 1, tm, timed_out) == 0)
	return 0;

    cleanup_subexpr();
    if (REG_MULTI)
    {
	if (rex.reg_startpos[0].lnum < 0)
	{
	    rex.reg_startpos[0].lnum = 0;
	    rex.reg_startpos[0].col = col;
	}
	if (rex.reg_endpos[0].lnum < 0)
	{
	    rex.reg_endpos[0].lnum = rex.lnum;
	    rex.reg_endpos[0].col = (int)(rex.input - rex.line);
	}
	else
	    // Use line number of "\ze".
	    rex.lnum = rex.reg_endpos[0].lnum;
    }
    else
    {
	if (rex.reg_startp[0] == NULL)
	    rex.reg_startp[0] = rex.line + col;
	if (rex.reg_endp[0] == NULL)
	    rex.reg_endp[0] = rex.input;
    }
#ifdef FEAT_SYN_HL
    // Package any found \z(...\) matches for export. Default is none.
    unref_extmatch(re_extmatch_out);
    re_extmatch_out = NULL;

    if (prog->reghasz == REX_SET)
    {
	int		i;

	cleanup_zsubexpr();
	re_extmatch_out = make_extmatch();
	for (i = 0; i < NSUBEXP; i++)
	{
	    if (REG_MULTI)
	    {
		// Only accept single line matches.
		if (reg_startzpos[i].lnum >= 0
			&& reg_endzpos[i].lnum == reg_startzpos[i].lnum
			&& reg_endzpos[i].col >= reg_startzpos[i].col)
		    re_extmatch_out->matches[i] =
			vim_strnsave(reg_getline(reg_startzpos[i].lnum)
						       + reg_startzpos[i].col,
				   reg_endzpos[i].col - reg_startzpos[i].col);
	    }
	    else
	    {
		if (reg_startzp[i] != NULL && reg_endzp[i] != NULL)
		    re_extmatch_out->matches[i] =
			    vim_strnsave(reg_startzp[i],
					(int)(reg_endzp[i] - reg_startzp[i]));
	    }
	}
    }
#endif
    return 1 + rex.lnum;
}

#ifdef BT_REGEXP_DUMP

/*
 * regdump - dump a regexp onto stdout in vaguely comprehensible form
 */
    static void
regdump(char_u *pattern, bt_regprog_T *r)
{
    char_u  *s;
    int	    op = EXACTLY;	// Arbitrary non-END op.
    char_u  *next;
    char_u  *end = NULL;
    FILE    *f;

#ifdef BT_REGEXP_LOG
    f = fopen("bt_regexp_log.log", "a");
#else
    f = stdout;
#endif
    if (f == NULL)
	return;
    fprintf(f, "-------------------------------------\n\r\nregcomp(%s):\r\n", pattern);

    s = r->program + 1;
    // Loop until we find the END that isn't before a referred next (an END
    // can also appear in a NOMATCH operand).
    while (op != END || s <= end)
    {
	op = OP(s);
	fprintf(f, "%2d%s", (int)(s - r->program), regprop(s)); // Where, what.
	next = regnext(s);
	if (next == NULL)	// Next ptr.
	    fprintf(f, "(0)");
	else
	    fprintf(f, "(%d)", (int)((s - r->program) + (next - s)));
	if (end < next)
	    end = next;
	if (op == BRACE_LIMITS)
	{
	    // Two ints
	    fprintf(f, " minval %ld, maxval %ld", OPERAND_MIN(s), OPERAND_MAX(s));
	    s += 8;
	}
	else if (op == BEHIND || op == NOBEHIND)
	{
	    // one int
	    fprintf(f, " count %ld", OPERAND_MIN(s));
	    s += 4;
	}
	else if (op == RE_LNUM || op == RE_COL || op == RE_VCOL)
	{
	    // one int plus comparator
	    fprintf(f, " count %ld", OPERAND_MIN(s));
	    s += 5;
	}
	s += 3;
	if (op == ANYOF || op == ANYOF + ADD_NL
		|| op == ANYBUT || op == ANYBUT + ADD_NL
		|| op == EXACTLY)
	{
	    // Literal string, where present.
	    fprintf(f, "\nxxxxxxxxx\n");
	    while (*s != NUL)
		fprintf(f, "%c", *s++);
	    fprintf(f, "\nxxxxxxxxx\n");
	    s++;
	}
	fprintf(f, "\r\n");
    }

    // Header fields of interest.
    if (r->regstart != NUL)
	fprintf(f, "start `%s' 0x%x; ", r->regstart < 256
		? (char *)transchar(r->regstart)
		: "multibyte", r->regstart);
    if (r->reganch)
	fprintf(f, "anchored; ");
    if (r->regmust != NULL)
	fprintf(f, "must have \"%s\"", r->regmust);
    fprintf(f, "\r\n");

#ifdef BT_REGEXP_LOG
    fclose(f);
#endif
}
#endif	    // BT_REGEXP_DUMP
