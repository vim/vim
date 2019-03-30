/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * ops.c: implementation of various operators: op_shift, op_delete, op_tilde,
 *	  op_change, op_yank, do_put, do_join
 */

#include "vim.h"

/*
 * Number of registers.
 *	0 = unnamed register, for normal yanks and puts
 *   1..9 = registers '1' to '9', for deletes
 * 10..35 = registers 'a' to 'z'
 *     36 = delete register '-'
 *     37 = Selection register '*'. Only if FEAT_CLIPBOARD defined
 *     38 = Clipboard register '+'. Only if FEAT_CLIPBOARD and FEAT_X11 defined
 */
/*
 * Symbolic names for some registers.
 */
#define DELETION_REGISTER	36
#ifdef FEAT_CLIPBOARD
# define STAR_REGISTER		37
#  ifdef FEAT_X11
#   define PLUS_REGISTER	38
#  else
#   define PLUS_REGISTER	STAR_REGISTER	    /* there is only one */
#  endif
#endif
#ifdef FEAT_DND
# define TILDE_REGISTER		(PLUS_REGISTER + 1)
#endif

#ifdef FEAT_CLIPBOARD
# ifdef FEAT_DND
#  define NUM_REGISTERS		(TILDE_REGISTER + 1)
# else
#  define NUM_REGISTERS		(PLUS_REGISTER + 1)
# endif
#else
# define NUM_REGISTERS		37
#endif

/*
 * Each yank register has an array of pointers to lines.
 */
typedef struct
{
    char_u	**y_array;	/* pointer to array of line pointers */
    linenr_T	y_size;		/* number of lines in y_array */
    char_u	y_type;		/* MLINE, MCHAR or MBLOCK */
    colnr_T	y_width;	/* only set if y_type == MBLOCK */
#ifdef FEAT_VIMINFO
    time_t	y_time_set;
#endif
} yankreg_T;

static yankreg_T	y_regs[NUM_REGISTERS];

static yankreg_T	*y_current;	    /* ptr to current yankreg */
static int		y_append;	    /* TRUE when appending */
static yankreg_T	*y_previous = NULL; /* ptr to last written yankreg */

/*
 * structure used by block_prep, op_delete and op_yank for blockwise operators
 * also op_change, op_shift, op_insert, op_replace - AKelly
 */
struct block_def
{
    int		startspaces;	/* 'extra' cols before first char */
    int		endspaces;	/* 'extra' cols after last char */
    int		textlen;	/* chars in block */
    char_u	*textstart;	/* pointer to 1st char (partially) in block */
    colnr_T	textcol;	/* index of chars (partially) in block */
    colnr_T	start_vcol;	/* start col of 1st char wholly inside block */
    colnr_T	end_vcol;	/* start col of 1st char wholly after block */
    int		is_short;	/* TRUE if line is too short to fit in block */
    int		is_MAX;		/* TRUE if curswant==MAXCOL when starting */
    int		is_oneChar;	/* TRUE if block within one character */
    int		pre_whitesp;	/* screen cols of ws before block */
    int		pre_whitesp_c;	/* chars of ws before block */
    colnr_T	end_char_vcols;	/* number of vcols of post-block char */
    colnr_T	start_char_vcols; /* number of vcols of pre-block char */
};

static void shift_block(oparg_T *oap, int amount);
static int	stuff_yank(int, char_u *);
static void	put_reedit_in_typebuf(int silent);
static int	put_in_typebuf(char_u *s, int esc, int colon,
								 int silent);
static void	stuffescaped(char_u *arg, int literally);
static void	mb_adjust_opend(oparg_T *oap);
static void	free_yank_all(void);
static int	yank_copy_line(struct block_def *bd, long y_idx);
#ifdef FEAT_CLIPBOARD
static void	copy_yank_reg(yankreg_T *reg);
static void	may_set_selection(void);
#endif
static void	dis_msg(char_u *p, int skip_esc);
static void	block_prep(oparg_T *oap, struct block_def *, linenr_T, int);
static int	do_addsub(int op_type, pos_T *pos, int length, linenr_T Prenum1);
#if defined(FEAT_CLIPBOARD) || defined(FEAT_EVAL)
static void	str_to_reg(yankreg_T *y_ptr, int yank_type, char_u *str, long len, long blocklen, int str_list);
#endif
static int	ends_in_white(linenr_T lnum);
#ifdef FEAT_COMMENTS
static int	fmt_check_par(linenr_T, int *, char_u **, int do_comments);
#else
static int	fmt_check_par(linenr_T);
#endif

// Flags for third item in "opchars".
#define OPF_LINES  1	// operator always works on lines
#define OPF_CHANGE 2	// operator changes text

/*
 * The names of operators.
 * IMPORTANT: Index must correspond with defines in vim.h!!!
 * The third field holds OPF_ flags.
 */
static char opchars[][3] =
{
    {NUL, NUL, 0},			// OP_NOP
    {'d', NUL, OPF_CHANGE},		// OP_DELETE
    {'y', NUL, 0},			// OP_YANK
    {'c', NUL, OPF_CHANGE},		// OP_CHANGE
    {'<', NUL, OPF_LINES | OPF_CHANGE},	// OP_LSHIFT
    {'>', NUL, OPF_LINES | OPF_CHANGE},	// OP_RSHIFT
    {'!', NUL, OPF_LINES | OPF_CHANGE},	// OP_FILTER
    {'g', '~', OPF_CHANGE},		// OP_TILDE
    {'=', NUL, OPF_LINES | OPF_CHANGE},	// OP_INDENT
    {'g', 'q', OPF_LINES | OPF_CHANGE},	// OP_FORMAT
    {':', NUL, OPF_LINES},		// OP_COLON
    {'g', 'U', OPF_CHANGE},		// OP_UPPER
    {'g', 'u', OPF_CHANGE},		// OP_LOWER
    {'J', NUL, OPF_LINES | OPF_CHANGE},	// DO_JOIN
    {'g', 'J', OPF_LINES | OPF_CHANGE},	// DO_JOIN_NS
    {'g', '?', OPF_CHANGE},		// OP_ROT13
    {'r', NUL, OPF_CHANGE},		// OP_REPLACE
    {'I', NUL, OPF_CHANGE},		// OP_INSERT
    {'A', NUL, OPF_CHANGE},		// OP_APPEND
    {'z', 'f', OPF_LINES},		// OP_FOLD
    {'z', 'o', OPF_LINES},		// OP_FOLDOPEN
    {'z', 'O', OPF_LINES},		// OP_FOLDOPENREC
    {'z', 'c', OPF_LINES},		// OP_FOLDCLOSE
    {'z', 'C', OPF_LINES},		// OP_FOLDCLOSEREC
    {'z', 'd', OPF_LINES},		// OP_FOLDDEL
    {'z', 'D', OPF_LINES},		// OP_FOLDDELREC
    {'g', 'w', OPF_LINES | OPF_CHANGE},	// OP_FORMAT2
    {'g', '@', OPF_CHANGE},		// OP_FUNCTION
    {Ctrl_A, NUL, OPF_CHANGE},		// OP_NR_ADD
    {Ctrl_X, NUL, OPF_CHANGE},		// OP_NR_SUB
};

/*
 * Translate a command name into an operator type.
 * Must only be called with a valid operator name!
 */
    int
get_op_type(int char1, int char2)
{
    int		i;

    if (char1 == 'r')		/* ignore second character */
	return OP_REPLACE;
    if (char1 == '~')		/* when tilde is an operator */
	return OP_TILDE;
    if (char1 == 'g' && char2 == Ctrl_A)	/* add */
	return OP_NR_ADD;
    if (char1 == 'g' && char2 == Ctrl_X)	/* subtract */
	return OP_NR_SUB;
    for (i = 0; ; ++i)
    {
	if (opchars[i][0] == char1 && opchars[i][1] == char2)
	    break;
	if (i == (int)(sizeof(opchars) / sizeof(char [3]) - 1))
	{
	    internal_error("get_op_type()");
	    break;
	}
    }
    return i;
}

/*
 * Return TRUE if operator "op" always works on whole lines.
 */
    int
op_on_lines(int op)
{
    return opchars[op][2] & OPF_LINES;
}

#if defined(FEAT_JOB_CHANNEL) || defined(PROTO)
/*
 * Return TRUE if operator "op" changes text.
 */
    int
op_is_change(int op)
{
    return opchars[op][2] & OPF_CHANGE;
}
#endif

/*
 * Get first operator command character.
 * Returns 'g' or 'z' if there is another command character.
 */
    int
get_op_char(int optype)
{
    return opchars[optype][0];
}

/*
 * Get second operator command character.
 */
    int
get_extra_op_char(int optype)
{
    return opchars[optype][1];
}

/*
 * op_shift - handle a shift operation
 */
    void
op_shift(oparg_T *oap, int curs_top, int amount)
{
    long	    i;
    int		    first_char;
    int		    block_col = 0;

    if (u_save((linenr_T)(oap->start.lnum - 1),
				       (linenr_T)(oap->end.lnum + 1)) == FAIL)
	return;

    if (oap->block_mode)
	block_col = curwin->w_cursor.col;

    for (i = oap->line_count; --i >= 0; )
    {
	first_char = *ml_get_curline();
	if (first_char == NUL)				/* empty line */
	    curwin->w_cursor.col = 0;
	else if (oap->block_mode)
	    shift_block(oap, amount);
	else
	    /* Move the line right if it doesn't start with '#', 'smartindent'
	     * isn't set or 'cindent' isn't set or '#' isn't in 'cino'. */
#if defined(FEAT_SMARTINDENT) || defined(FEAT_CINDENT)
	    if (first_char != '#' || !preprocs_left())
#endif
	    shift_line(oap->op_type == OP_LSHIFT, p_sr, amount, FALSE);
	++curwin->w_cursor.lnum;
    }

    changed_lines(oap->start.lnum, 0, oap->end.lnum + 1, 0L);
    if (oap->block_mode)
    {
	curwin->w_cursor.lnum = oap->start.lnum;
	curwin->w_cursor.col = block_col;
    }
    else if (curs_top)	    /* put cursor on first line, for ">>" */
    {
	curwin->w_cursor.lnum = oap->start.lnum;
	beginline(BL_SOL | BL_FIX);   /* shift_line() may have set cursor.col */
    }
    else
	--curwin->w_cursor.lnum;	/* put cursor on last line, for ":>" */

#ifdef FEAT_FOLDING
    /* The cursor line is not in a closed fold */
    foldOpenCursor();
#endif


    if (oap->line_count > p_report)
    {
	char	    *op;
	char	    *msg_line_single;
	char	    *msg_line_plural;

	if (oap->op_type == OP_RSHIFT)
	    op = ">";
	else
	    op = "<";
	msg_line_single = NGETTEXT("%ld line %sed %d time",
					     "%ld line %sed %d times", amount);
	msg_line_plural = NGETTEXT("%ld lines %sed %d time",
					    "%ld lines %sed %d times", amount);
	vim_snprintf((char *)IObuff, IOSIZE,
		NGETTEXT(msg_line_single, msg_line_plural, oap->line_count),
		oap->line_count, op, amount);
	msg((char *)IObuff);
    }

    /*
     * Set "'[" and "']" marks.
     */
    curbuf->b_op_start = oap->start;
    curbuf->b_op_end.lnum = oap->end.lnum;
    curbuf->b_op_end.col = (colnr_T)STRLEN(ml_get(oap->end.lnum));
    if (curbuf->b_op_end.col > 0)
	--curbuf->b_op_end.col;
}

/*
 * Shift the current line one shiftwidth left (if left != 0) or right
 * leaves cursor on first blank in the line.
 */
    void
shift_line(
    int	left,
    int	round,
    int	amount,
    int call_changed_bytes)	/* call changed_bytes() */
{
    int		count;
    int		i, j;
    int		p_sw = (int)get_sw_value_indent(curbuf);

    count = get_indent();	/* get current indent */

    if (round)			/* round off indent */
    {
	i = count / p_sw;	/* number of p_sw rounded down */
	j = count % p_sw;	/* extra spaces */
	if (j && left)		/* first remove extra spaces */
	    --amount;
	if (left)
	{
	    i -= amount;
	    if (i < 0)
		i = 0;
	}
	else
	    i += amount;
	count = i * p_sw;
    }
    else		/* original vi indent */
    {
	if (left)
	{
	    count -= p_sw * amount;
	    if (count < 0)
		count = 0;
	}
	else
	    count += p_sw * amount;
    }

    /* Set new indent */
    if (State & VREPLACE_FLAG)
	change_indent(INDENT_SET, count, FALSE, NUL, call_changed_bytes);
    else
	(void)set_indent(count, call_changed_bytes ? SIN_CHANGED : 0);
}

/*
 * Shift one line of the current block one shiftwidth right or left.
 * Leaves cursor on first character in block.
 */
    static void
shift_block(oparg_T *oap, int amount)
{
    int			left = (oap->op_type == OP_LSHIFT);
    int			oldstate = State;
    int			total;
    char_u		*newp, *oldp;
    int			oldcol = curwin->w_cursor.col;
    int			p_sw = (int)get_sw_value_indent(curbuf);
#ifdef FEAT_VARTABS
    int			*p_vts = curbuf->b_p_vts_array;
#endif
    int			p_ts = (int)curbuf->b_p_ts;
    struct block_def	bd;
    int			incr;
    colnr_T		ws_vcol;
    int			i = 0, j = 0;
    int			len;
#ifdef FEAT_RIGHTLEFT
    int			old_p_ri = p_ri;

    p_ri = 0;			/* don't want revins in indent */
#endif

    State = INSERT;		/* don't want REPLACE for State */
    block_prep(oap, &bd, curwin->w_cursor.lnum, TRUE);
    if (bd.is_short)
	return;

    /* total is number of screen columns to be inserted/removed */
    total = (int)((unsigned)amount * (unsigned)p_sw);
    if ((total / p_sw) != amount)
	return; /* multiplication overflow */

    oldp = ml_get_curline();

    if (!left)
    {
	/*
	 *  1. Get start vcol
	 *  2. Total ws vcols
	 *  3. Divvy into TABs & spp
	 *  4. Construct new string
	 */
	total += bd.pre_whitesp; /* all virtual WS upto & incl a split TAB */
	ws_vcol = bd.start_vcol - bd.pre_whitesp;
	if (bd.startspaces)
	{
	    if (has_mbyte)
	    {
		if ((*mb_ptr2len)(bd.textstart) == 1)
		    ++bd.textstart;
		else
		{
		    ws_vcol = 0;
		    bd.startspaces = 0;
		}
	    }
	    else
		++bd.textstart;
	}
	for ( ; VIM_ISWHITE(*bd.textstart); )
	{
	    /* TODO: is passing bd.textstart for start of the line OK? */
	    incr = lbr_chartabsize_adv(bd.textstart, &bd.textstart,
						    (colnr_T)(bd.start_vcol));
	    total += incr;
	    bd.start_vcol += incr;
	}
	/* OK, now total=all the VWS reqd, and textstart points at the 1st
	 * non-ws char in the block. */
#ifdef FEAT_VARTABS
	if (!curbuf->b_p_et)
	    tabstop_fromto(ws_vcol, ws_vcol + total, p_ts, p_vts, &i, &j);
	else
	    j = total;
#else
	if (!curbuf->b_p_et)
	    i = ((ws_vcol % p_ts) + total) / p_ts; /* number of tabs */
	if (i)
	    j = ((ws_vcol % p_ts) + total) % p_ts; /* number of spp */
	else
	    j = total;
#endif
	/* if we're splitting a TAB, allow for it */
	bd.textcol -= bd.pre_whitesp_c - (bd.startspaces != 0);
	len = (int)STRLEN(bd.textstart) + 1;
	newp = alloc_check((unsigned)(bd.textcol + i + j + len));
	if (newp == NULL)
	    return;
	vim_memset(newp, NUL, (size_t)(bd.textcol + i + j + len));
	mch_memmove(newp, oldp, (size_t)bd.textcol);
	vim_memset(newp + bd.textcol, TAB, (size_t)i);
	vim_memset(newp + bd.textcol + i, ' ', (size_t)j);
	/* the end */
	mch_memmove(newp + bd.textcol + i + j, bd.textstart, (size_t)len);
    }
    else /* left */
    {
	colnr_T	    destination_col;	/* column to which text in block will
					   be shifted */
	char_u	    *verbatim_copy_end;	/* end of the part of the line which is
					   copied verbatim */
	colnr_T	    verbatim_copy_width;/* the (displayed) width of this part
					   of line */
	unsigned    fill;		/* nr of spaces that replace a TAB */
	unsigned    new_line_len;	/* the length of the line after the
					   block shift */
	size_t	    block_space_width;
	size_t	    shift_amount;
	char_u	    *non_white = bd.textstart;
	colnr_T	    non_white_col;

	/*
	 * Firstly, let's find the first non-whitespace character that is
	 * displayed after the block's start column and the character's column
	 * number. Also, let's calculate the width of all the whitespace
	 * characters that are displayed in the block and precede the searched
	 * non-whitespace character.
	 */

	/* If "bd.startspaces" is set, "bd.textstart" points to the character,
	 * the part of which is displayed at the block's beginning. Let's start
	 * searching from the next character. */
	if (bd.startspaces)
	    MB_PTR_ADV(non_white);

	/* The character's column is in "bd.start_vcol".  */
	non_white_col = bd.start_vcol;

	while (VIM_ISWHITE(*non_white))
	{
	    incr = lbr_chartabsize_adv(bd.textstart, &non_white, non_white_col);
	    non_white_col += incr;
	}

	block_space_width = non_white_col - oap->start_vcol;
	/* We will shift by "total" or "block_space_width", whichever is less.
	 */
	shift_amount = (block_space_width < (size_t)total
					 ? block_space_width : (size_t)total);

	/* The column to which we will shift the text.  */
	destination_col = (colnr_T)(non_white_col - shift_amount);

	/* Now let's find out how much of the beginning of the line we can
	 * reuse without modification.  */
	verbatim_copy_end = bd.textstart;
	verbatim_copy_width = bd.start_vcol;

	/* If "bd.startspaces" is set, "bd.textstart" points to the character
	 * preceding the block. We have to subtract its width to obtain its
	 * column number.  */
	if (bd.startspaces)
	    verbatim_copy_width -= bd.start_char_vcols;
	while (verbatim_copy_width < destination_col)
	{
	    char_u *line = verbatim_copy_end;

	    /* TODO: is passing verbatim_copy_end for start of the line OK? */
	    incr = lbr_chartabsize(line, verbatim_copy_end,
							 verbatim_copy_width);
	    if (verbatim_copy_width + incr > destination_col)
		break;
	    verbatim_copy_width += incr;
	    MB_PTR_ADV(verbatim_copy_end);
	}

	/* If "destination_col" is different from the width of the initial
	 * part of the line that will be copied, it means we encountered a tab
	 * character, which we will have to partly replace with spaces.  */
	fill = destination_col - verbatim_copy_width;

	/* The replacement line will consist of:
	 * - the beginning of the original line up to "verbatim_copy_end",
	 * - "fill" number of spaces,
	 * - the rest of the line, pointed to by non_white.  */
	new_line_len = (unsigned)(verbatim_copy_end - oldp)
		       + fill
		       + (unsigned)STRLEN(non_white) + 1;

	newp = alloc_check(new_line_len);
	if (newp == NULL)
	    return;
	mch_memmove(newp, oldp, (size_t)(verbatim_copy_end - oldp));
	vim_memset(newp + (verbatim_copy_end - oldp), ' ', (size_t)fill);
	STRMOVE(newp + (verbatim_copy_end - oldp) + fill, non_white);
    }
    /* replace the line */
    ml_replace(curwin->w_cursor.lnum, newp, FALSE);
    changed_bytes(curwin->w_cursor.lnum, (colnr_T)bd.textcol);
    State = oldstate;
    curwin->w_cursor.col = oldcol;
#ifdef FEAT_RIGHTLEFT
    p_ri = old_p_ri;
#endif
}

/*
 * Insert string "s" (b_insert ? before : after) block :AKelly
 * Caller must prepare for undo.
 */
    static void
block_insert(
    oparg_T		*oap,
    char_u		*s,
    int			b_insert,
    struct block_def	*bdp)
{
    int		p_ts;
    int		count = 0;	/* extra spaces to replace a cut TAB */
    int		spaces = 0;	/* non-zero if cutting a TAB */
    colnr_T	offset;		/* pointer along new line */
    unsigned	s_len;		/* STRLEN(s) */
    char_u	*newp, *oldp;	/* new, old lines */
    linenr_T	lnum;		/* loop var */
    int		oldstate = State;

    State = INSERT;		/* don't want REPLACE for State */
    s_len = (unsigned)STRLEN(s);

    for (lnum = oap->start.lnum + 1; lnum <= oap->end.lnum; lnum++)
    {
	block_prep(oap, bdp, lnum, TRUE);
	if (bdp->is_short && b_insert)
	    continue;	/* OP_INSERT, line ends before block start */

	oldp = ml_get(lnum);

	if (b_insert)
	{
	    p_ts = bdp->start_char_vcols;
	    spaces = bdp->startspaces;
	    if (spaces != 0)
		count = p_ts - 1; /* we're cutting a TAB */
	    offset = bdp->textcol;
	}
	else /* append */
	{
	    p_ts = bdp->end_char_vcols;
	    if (!bdp->is_short) /* spaces = padding after block */
	    {
		spaces = (bdp->endspaces ? p_ts - bdp->endspaces : 0);
		if (spaces != 0)
		    count = p_ts - 1; /* we're cutting a TAB */
		offset = bdp->textcol + bdp->textlen - (spaces != 0);
	    }
	    else /* spaces = padding to block edge */
	    {
		/* if $ used, just append to EOL (ie spaces==0) */
		if (!bdp->is_MAX)
		    spaces = (oap->end_vcol - bdp->end_vcol) + 1;
		count = spaces;
		offset = bdp->textcol + bdp->textlen;
	    }
	}

	if (has_mbyte && spaces > 0)
	{
	    int off;

	    /* Avoid starting halfway a multi-byte character. */
	    if (b_insert)
	    {
		off = (*mb_head_off)(oldp, oldp + offset + spaces);
	    }
	    else
	    {
		off = (*mb_off_next)(oldp, oldp + offset);
		offset += off;
	    }
	    spaces -= off;
	    count -= off;
	}

	newp = alloc_check((unsigned)(STRLEN(oldp)) + s_len + count + 1);
	if (newp == NULL)
	    continue;

	/* copy up to shifted part */
	mch_memmove(newp, oldp, (size_t)(offset));
	oldp += offset;

	/* insert pre-padding */
	vim_memset(newp + offset, ' ', (size_t)spaces);

	/* copy the new text */
	mch_memmove(newp + offset + spaces, s, (size_t)s_len);
	offset += s_len;

	if (spaces && !bdp->is_short)
	{
	    /* insert post-padding */
	    vim_memset(newp + offset + spaces, ' ', (size_t)(p_ts - spaces));
	    /* We're splitting a TAB, don't copy it. */
	    oldp++;
	    /* We allowed for that TAB, remember this now */
	    count++;
	}

	if (spaces > 0)
	    offset += count;
	STRMOVE(newp + offset, oldp);

	ml_replace(lnum, newp, FALSE);

	if (lnum == oap->end.lnum)
	{
	    /* Set "']" mark to the end of the block instead of the end of
	     * the insert in the first line.  */
	    curbuf->b_op_end.lnum = oap->end.lnum;
	    curbuf->b_op_end.col = offset;
	}
    } /* for all lnum */

    changed_lines(oap->start.lnum + 1, 0, oap->end.lnum + 1, 0L);

    State = oldstate;
}

#if defined(FEAT_LISP) || defined(FEAT_CINDENT) || defined(PROTO)
/*
 * op_reindent - handle reindenting a block of lines.
 */
    void
op_reindent(oparg_T *oap, int (*how)(void))
{
    long	i;
    char_u	*l;
    int		amount;
    linenr_T	first_changed = 0;
    linenr_T	last_changed = 0;
    linenr_T	start_lnum = curwin->w_cursor.lnum;

    /* Don't even try when 'modifiable' is off. */
    if (!curbuf->b_p_ma)
    {
	emsg(_(e_modifiable));
	return;
    }

    for (i = oap->line_count; --i >= 0 && !got_int; )
    {
	/* it's a slow thing to do, so give feedback so there's no worry that
	 * the computer's just hung. */

	if (i > 1
		&& (i % 50 == 0 || i == oap->line_count - 1)
		&& oap->line_count > p_report)
	    smsg(_("%ld lines to indent... "), i);

	/*
	 * Be vi-compatible: For lisp indenting the first line is not
	 * indented, unless there is only one line.
	 */
#ifdef FEAT_LISP
	if (i != oap->line_count - 1 || oap->line_count == 1
						    || how != get_lisp_indent)
#endif
	{
	    l = skipwhite(ml_get_curline());
	    if (*l == NUL)		    /* empty or blank line */
		amount = 0;
	    else
		amount = how();		    /* get the indent for this line */

	    if (amount >= 0 && set_indent(amount, SIN_UNDO))
	    {
		/* did change the indent, call changed_lines() later */
		if (first_changed == 0)
		    first_changed = curwin->w_cursor.lnum;
		last_changed = curwin->w_cursor.lnum;
	    }
	}
	++curwin->w_cursor.lnum;
	curwin->w_cursor.col = 0;  /* make sure it's valid */
    }

    /* put cursor on first non-blank of indented line */
    curwin->w_cursor.lnum = start_lnum;
    beginline(BL_SOL | BL_FIX);

    /* Mark changed lines so that they will be redrawn.  When Visual
     * highlighting was present, need to continue until the last line.  When
     * there is no change still need to remove the Visual highlighting. */
    if (last_changed != 0)
	changed_lines(first_changed, 0,
		oap->is_VIsual ? start_lnum + oap->line_count :
		last_changed + 1, 0L);
    else if (oap->is_VIsual)
	redraw_curbuf_later(INVERTED);

    if (oap->line_count > p_report)
    {
	i = oap->line_count - (i + 1);
	smsg(NGETTEXT("%ld line indented ",
						 "%ld lines indented ", i), i);
    }
    /* set '[ and '] marks */
    curbuf->b_op_start = oap->start;
    curbuf->b_op_end = oap->end;
}
#endif /* defined(FEAT_LISP) || defined(FEAT_CINDENT) */

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Keep the last expression line here, for repeating.
 */
static char_u	*expr_line = NULL;

/*
 * Get an expression for the "\"=expr1" or "CTRL-R =expr1"
 * Returns '=' when OK, NUL otherwise.
 */
    int
get_expr_register(void)
{
    char_u	*new_line;

    new_line = getcmdline('=', 0L, 0);
    if (new_line == NULL)
	return NUL;
    if (*new_line == NUL)	/* use previous line */
	vim_free(new_line);
    else
	set_expr_line(new_line);
    return '=';
}

/*
 * Set the expression for the '=' register.
 * Argument must be an allocated string.
 */
    void
set_expr_line(char_u *new_line)
{
    vim_free(expr_line);
    expr_line = new_line;
}

/*
 * Get the result of the '=' register expression.
 * Returns a pointer to allocated memory, or NULL for failure.
 */
    char_u *
get_expr_line(void)
{
    char_u	*expr_copy;
    char_u	*rv;
    static int	nested = 0;

    if (expr_line == NULL)
	return NULL;

    /* Make a copy of the expression, because evaluating it may cause it to be
     * changed. */
    expr_copy = vim_strsave(expr_line);
    if (expr_copy == NULL)
	return NULL;

    /* When we are invoked recursively limit the evaluation to 10 levels.
     * Then return the string as-is. */
    if (nested >= 10)
	return expr_copy;

    ++nested;
    rv = eval_to_string(expr_copy, NULL, TRUE);
    --nested;
    vim_free(expr_copy);
    return rv;
}

/*
 * Get the '=' register expression itself, without evaluating it.
 */
    char_u *
get_expr_line_src(void)
{
    if (expr_line == NULL)
	return NULL;
    return vim_strsave(expr_line);
}
#endif /* FEAT_EVAL */

/*
 * Check if 'regname' is a valid name of a yank register.
 * Note: There is no check for 0 (default register), caller should do this
 */
    int
valid_yank_reg(
    int	    regname,
    int	    writing)	    /* if TRUE check for writable registers */
{
    if (       (regname > 0 && ASCII_ISALNUM(regname))
	    || (!writing && vim_strchr((char_u *)
#ifdef FEAT_EVAL
				    "/.%:="
#else
				    "/.%:"
#endif
					, regname) != NULL)
	    || regname == '#'
	    || regname == '"'
	    || regname == '-'
	    || regname == '_'
#ifdef FEAT_CLIPBOARD
	    || regname == '*'
	    || regname == '+'
#endif
#ifdef FEAT_DND
	    || (!writing && regname == '~')
#endif
							)
	return TRUE;
    return FALSE;
}

/*
 * Set y_current and y_append, according to the value of "regname".
 * Cannot handle the '_' register.
 * Must only be called with a valid register name!
 *
 * If regname is 0 and writing, use register 0
 * If regname is 0 and reading, use previous register
 *
 * Return TRUE when the register should be inserted literally (selection or
 * clipboard).
 */
    int
get_yank_register(int regname, int writing)
{
    int	    i;
    int	    ret = FALSE;

    y_append = FALSE;
    if ((regname == 0 || regname == '"') && !writing && y_previous != NULL)
    {
	y_current = y_previous;
	return ret;
    }
    i = regname;
    if (VIM_ISDIGIT(i))
	i -= '0';
    else if (ASCII_ISLOWER(i))
	i = CharOrdLow(i) + 10;
    else if (ASCII_ISUPPER(i))
    {
	i = CharOrdUp(i) + 10;
	y_append = TRUE;
    }
    else if (regname == '-')
	i = DELETION_REGISTER;
#ifdef FEAT_CLIPBOARD
    /* When selection is not available, use register 0 instead of '*' */
    else if (clip_star.available && regname == '*')
    {
	i = STAR_REGISTER;
	ret = TRUE;
    }
    /* When clipboard is not available, use register 0 instead of '+' */
    else if (clip_plus.available && regname == '+')
    {
	i = PLUS_REGISTER;
	ret = TRUE;
    }
#endif
#ifdef FEAT_DND
    else if (!writing && regname == '~')
	i = TILDE_REGISTER;
#endif
    else		/* not 0-9, a-z, A-Z or '-': use register 0 */
	i = 0;
    y_current = &(y_regs[i]);
    if (writing)	/* remember the register we write into for do_put() */
	y_previous = y_current;
    return ret;
}

#if defined(FEAT_CLIPBOARD) || defined(PROTO)
/*
 * When "regname" is a clipboard register, obtain the selection.  If it's not
 * available return zero, otherwise return "regname".
 */
    int
may_get_selection(int regname)
{
    if (regname == '*')
    {
	if (!clip_star.available)
	    regname = 0;
	else
	    clip_get_selection(&clip_star);
    }
    else if (regname == '+')
    {
	if (!clip_plus.available)
	    regname = 0;
	else
	    clip_get_selection(&clip_plus);
    }
    return regname;
}
#endif

/*
 * Obtain the contents of a "normal" register. The register is made empty.
 * The returned pointer has allocated memory, use put_register() later.
 */
    void *
get_register(
    int		name,
    int		copy)	/* make a copy, if FALSE make register empty. */
{
    yankreg_T	*reg;
    int		i;

#ifdef FEAT_CLIPBOARD
    /* When Visual area changed, may have to update selection.  Obtain the
     * selection too. */
    if (name == '*' && clip_star.available)
    {
	if (clip_isautosel_star())
	    clip_update_selection(&clip_star);
	may_get_selection(name);
    }
    if (name == '+' && clip_plus.available)
    {
	if (clip_isautosel_plus())
	    clip_update_selection(&clip_plus);
	may_get_selection(name);
    }
#endif

    get_yank_register(name, 0);
    reg = (yankreg_T *)alloc((unsigned)sizeof(yankreg_T));
    if (reg != NULL)
    {
	*reg = *y_current;
	if (copy)
	{
	    /* If we run out of memory some or all of the lines are empty. */
	    if (reg->y_size == 0)
		reg->y_array = NULL;
	    else
		reg->y_array = (char_u **)alloc((unsigned)(sizeof(char_u *)
							      * reg->y_size));
	    if (reg->y_array != NULL)
	    {
		for (i = 0; i < reg->y_size; ++i)
		    reg->y_array[i] = vim_strsave(y_current->y_array[i]);
	    }
	}
	else
	    y_current->y_array = NULL;
    }
    return (void *)reg;
}

/*
 * Put "reg" into register "name".  Free any previous contents and "reg".
 */
    void
put_register(int name, void *reg)
{
    get_yank_register(name, 0);
    free_yank_all();
    *y_current = *(yankreg_T *)reg;
    vim_free(reg);

#ifdef FEAT_CLIPBOARD
    /* Send text written to clipboard register to the clipboard. */
    may_set_selection();
#endif
}

#if (defined(FEAT_CLIPBOARD) && defined(FEAT_X11) && defined(USE_SYSTEM)) \
	|| defined(PROTO)
    void
free_register(void *reg)
{
    yankreg_T tmp;

    tmp = *y_current;
    *y_current = *(yankreg_T *)reg;
    free_yank_all();
    vim_free(reg);
    *y_current = tmp;
}
#endif

#if defined(FEAT_MOUSE) || defined(PROTO)
/*
 * return TRUE if the current yank register has type MLINE
 */
    int
yank_register_mline(int regname)
{
    if (regname != 0 && !valid_yank_reg(regname, FALSE))
	return FALSE;
    if (regname == '_')		/* black hole is always empty */
	return FALSE;
    get_yank_register(regname, FALSE);
    return (y_current->y_type == MLINE);
}
#endif

/*
 * Start or stop recording into a yank register.
 *
 * Return FAIL for failure, OK otherwise.
 */
    int
do_record(int c)
{
    char_u	    *p;
    static int	    regname;
    yankreg_T	    *old_y_previous, *old_y_current;
    int		    retval;

    if (reg_recording == 0)	    /* start recording */
    {
	/* registers 0-9, a-z and " are allowed */
	if (c < 0 || (!ASCII_ISALNUM(c) && c != '"'))
	    retval = FAIL;
	else
	{
	    reg_recording = c;
	    showmode();
	    regname = c;
	    retval = OK;
	}
    }
    else			    /* stop recording */
    {
	/*
	 * Get the recorded key hits.  K_SPECIAL and CSI will be escaped, this
	 * needs to be removed again to put it in a register.  exec_reg then
	 * adds the escaping back later.
	 */
	reg_recording = 0;
	msg("");
	p = get_recorded();
	if (p == NULL)
	    retval = FAIL;
	else
	{
	    /* Remove escaping for CSI and K_SPECIAL in multi-byte chars. */
	    vim_unescape_csi(p);

	    /*
	     * We don't want to change the default register here, so save and
	     * restore the current register name.
	     */
	    old_y_previous = y_previous;
	    old_y_current = y_current;

	    retval = stuff_yank(regname, p);

	    y_previous = old_y_previous;
	    y_current = old_y_current;
	}
    }
    return retval;
}

/*
 * Stuff string "p" into yank register "regname" as a single line (append if
 * uppercase).	"p" must have been alloced.
 *
 * return FAIL for failure, OK otherwise
 */
    static int
stuff_yank(int regname, char_u *p)
{
    char_u	*lp;
    char_u	**pp;

    /* check for read-only register */
    if (regname != 0 && !valid_yank_reg(regname, TRUE))
    {
	vim_free(p);
	return FAIL;
    }
    if (regname == '_')		    /* black hole: don't do anything */
    {
	vim_free(p);
	return OK;
    }
    get_yank_register(regname, TRUE);
    if (y_append && y_current->y_array != NULL)
    {
	pp = &(y_current->y_array[y_current->y_size - 1]);
	lp = lalloc((long_u)(STRLEN(*pp) + STRLEN(p) + 1), TRUE);
	if (lp == NULL)
	{
	    vim_free(p);
	    return FAIL;
	}
	STRCPY(lp, *pp);
	STRCAT(lp, p);
	vim_free(p);
	vim_free(*pp);
	*pp = lp;
    }
    else
    {
	free_yank_all();
	if ((y_current->y_array =
			(char_u **)alloc((unsigned)sizeof(char_u *))) == NULL)
	{
	    vim_free(p);
	    return FAIL;
	}
	y_current->y_array[0] = p;
	y_current->y_size = 1;
	y_current->y_type = MCHAR;  /* used to be MLINE, why? */
#ifdef FEAT_VIMINFO
	y_current->y_time_set = vim_time();
#endif
    }
    return OK;
}

static int execreg_lastc = NUL;

/*
 * Execute a yank register: copy it into the stuff buffer.
 *
 * Return FAIL for failure, OK otherwise.
 */
    int
do_execreg(
    int	    regname,
    int	    colon,		/* insert ':' before each line */
    int	    addcr,		/* always add '\n' to end of line */
    int	    silent)		/* set "silent" flag in typeahead buffer */
{
    long	i;
    char_u	*p;
    int		retval = OK;
    int		remap;

    if (regname == '@')			/* repeat previous one */
    {
	if (execreg_lastc == NUL)
	{
	    emsg(_("E748: No previously used register"));
	    return FAIL;
	}
	regname = execreg_lastc;
    }
					/* check for valid regname */
    if (regname == '%' || regname == '#' || !valid_yank_reg(regname, FALSE))
    {
	emsg_invreg(regname);
	return FAIL;
    }
    execreg_lastc = regname;

#ifdef FEAT_CLIPBOARD
    regname = may_get_selection(regname);
#endif

    if (regname == '_')			/* black hole: don't stuff anything */
	return OK;

#ifdef FEAT_CMDHIST
    if (regname == ':')			/* use last command line */
    {
	if (last_cmdline == NULL)
	{
	    emsg(_(e_nolastcmd));
	    return FAIL;
	}
	// don't keep the cmdline containing @:
	VIM_CLEAR(new_last_cmdline);
	// Escape all control characters with a CTRL-V
	p = vim_strsave_escaped_ext(last_cmdline,
		    (char_u *)"\001\002\003\004\005\006\007"
			  "\010\011\012\013\014\015\016\017"
			  "\020\021\022\023\024\025\026\027"
			  "\030\031\032\033\034\035\036\037",
		    Ctrl_V, FALSE);
	if (p != NULL)
	{
	    /* When in Visual mode "'<,'>" will be prepended to the command.
	     * Remove it when it's already there. */
	    if (VIsual_active && STRNCMP(p, "'<,'>", 5) == 0)
		retval = put_in_typebuf(p + 5, TRUE, TRUE, silent);
	    else
		retval = put_in_typebuf(p, TRUE, TRUE, silent);
	}
	vim_free(p);
    }
#endif
#ifdef FEAT_EVAL
    else if (regname == '=')
    {
	p = get_expr_line();
	if (p == NULL)
	    return FAIL;
	retval = put_in_typebuf(p, TRUE, colon, silent);
	vim_free(p);
    }
#endif
    else if (regname == '.')		/* use last inserted text */
    {
	p = get_last_insert_save();
	if (p == NULL)
	{
	    emsg(_(e_noinstext));
	    return FAIL;
	}
	retval = put_in_typebuf(p, FALSE, colon, silent);
	vim_free(p);
    }
    else
    {
	get_yank_register(regname, FALSE);
	if (y_current->y_array == NULL)
	    return FAIL;

	/* Disallow remaping for ":@r". */
	remap = colon ? REMAP_NONE : REMAP_YES;

	/*
	 * Insert lines into typeahead buffer, from last one to first one.
	 */
	put_reedit_in_typebuf(silent);
	for (i = y_current->y_size; --i >= 0; )
	{
	    char_u *escaped;

	    /* insert NL between lines and after last line if type is MLINE */
	    if (y_current->y_type == MLINE || i < y_current->y_size - 1
								     || addcr)
	    {
		if (ins_typebuf((char_u *)"\n", remap, 0, TRUE, silent) == FAIL)
		    return FAIL;
	    }
	    escaped = vim_strsave_escape_csi(y_current->y_array[i]);
	    if (escaped == NULL)
		return FAIL;
	    retval = ins_typebuf(escaped, remap, 0, TRUE, silent);
	    vim_free(escaped);
	    if (retval == FAIL)
		return FAIL;
	    if (colon && ins_typebuf((char_u *)":", remap, 0, TRUE, silent)
								      == FAIL)
		return FAIL;
	}
	reg_executing = regname == 0 ? '"' : regname; // disable "q" command
    }
    return retval;
}

/*
 * If "restart_edit" is not zero, put it in the typeahead buffer, so that it's
 * used only after other typeahead has been processed.
 */
    static void
put_reedit_in_typebuf(int silent)
{
    char_u	buf[3];

    if (restart_edit != NUL)
    {
	if (restart_edit == 'V')
	{
	    buf[0] = 'g';
	    buf[1] = 'R';
	    buf[2] = NUL;
	}
	else
	{
	    buf[0] = restart_edit == 'I' ? 'i' : restart_edit;
	    buf[1] = NUL;
	}
	if (ins_typebuf(buf, REMAP_NONE, 0, TRUE, silent) == OK)
	    restart_edit = NUL;
    }
}

/*
 * Insert register contents "s" into the typeahead buffer, so that it will be
 * executed again.
 * When "esc" is TRUE it is to be taken literally: Escape CSI characters and
 * no remapping.
 */
    static int
put_in_typebuf(
    char_u	*s,
    int		esc,
    int		colon,	    /* add ':' before the line */
    int		silent)
{
    int		retval = OK;

    put_reedit_in_typebuf(silent);
    if (colon)
	retval = ins_typebuf((char_u *)"\n", REMAP_NONE, 0, TRUE, silent);
    if (retval == OK)
    {
	char_u	*p;

	if (esc)
	    p = vim_strsave_escape_csi(s);
	else
	    p = s;
	if (p == NULL)
	    retval = FAIL;
	else
	    retval = ins_typebuf(p, esc ? REMAP_NONE : REMAP_YES,
							     0, TRUE, silent);
	if (esc)
	    vim_free(p);
    }
    if (colon && retval == OK)
	retval = ins_typebuf((char_u *)":", REMAP_NONE, 0, TRUE, silent);
    return retval;
}

/*
 * Insert a yank register: copy it into the Read buffer.
 * Used by CTRL-R command and middle mouse button in insert mode.
 *
 * return FAIL for failure, OK otherwise
 */
    int
insert_reg(
    int		regname,
    int		literally_arg)	/* insert literally, not as if typed */
{
    long	i;
    int		retval = OK;
    char_u	*arg;
    int		allocated;
    int		literally = literally_arg;

    /*
     * It is possible to get into an endless loop by having CTRL-R a in
     * register a and then, in insert mode, doing CTRL-R a.
     * If you hit CTRL-C, the loop will be broken here.
     */
    ui_breakcheck();
    if (got_int)
	return FAIL;

    /* check for valid regname */
    if (regname != NUL && !valid_yank_reg(regname, FALSE))
	return FAIL;

#ifdef FEAT_CLIPBOARD
    regname = may_get_selection(regname);
#endif

    if (regname == '.')			/* insert last inserted text */
	retval = stuff_inserted(NUL, 1L, TRUE);
    else if (get_spec_reg(regname, &arg, &allocated, TRUE))
    {
	if (arg == NULL)
	    return FAIL;
	stuffescaped(arg, literally);
	if (allocated)
	    vim_free(arg);
    }
    else				/* name or number register */
    {
	if (get_yank_register(regname, FALSE))
	    literally = TRUE;
	if (y_current->y_array == NULL)
	    retval = FAIL;
	else
	{
	    for (i = 0; i < y_current->y_size; ++i)
	    {
		stuffescaped(y_current->y_array[i], literally);
		/*
		 * Insert a newline between lines and after last line if
		 * y_type is MLINE.
		 */
		if (y_current->y_type == MLINE || i < y_current->y_size - 1)
		    stuffcharReadbuff('\n');
	    }
	}
    }

    return retval;
}

/*
 * Stuff a string into the typeahead buffer, such that edit() will insert it
 * literally ("literally" TRUE) or interpret is as typed characters.
 */
    static void
stuffescaped(char_u *arg, int literally)
{
    int		c;
    char_u	*start;

    while (*arg != NUL)
    {
	/* Stuff a sequence of normal ASCII characters, that's fast.  Also
	 * stuff K_SPECIAL to get the effect of a special key when "literally"
	 * is TRUE. */
	start = arg;
	while ((*arg >= ' '
#ifndef EBCDIC
		    && *arg < DEL /* EBCDIC: chars above space are normal */
#endif
		    )
		|| (*arg == K_SPECIAL && !literally))
	    ++arg;
	if (arg > start)
	    stuffReadbuffLen(start, (long)(arg - start));

	/* stuff a single special character */
	if (*arg != NUL)
	{
	    if (has_mbyte)
		c = mb_cptr2char_adv(&arg);
	    else
		c = *arg++;
	    if (literally && ((c < ' ' && c != TAB) || c == DEL))
		stuffcharReadbuff(Ctrl_V);
	    stuffcharReadbuff(c);
	}
    }
}

/*
 * If "regname" is a special register, return TRUE and store a pointer to its
 * value in "argp".
 */
    int
get_spec_reg(
    int		regname,
    char_u	**argp,
    int		*allocated,	/* return: TRUE when value was allocated */
    int		errmsg)		/* give error message when failing */
{
    int		cnt;

    *argp = NULL;
    *allocated = FALSE;
    switch (regname)
    {
	case '%':		/* file name */
	    if (errmsg)
		check_fname();	/* will give emsg if not set */
	    *argp = curbuf->b_fname;
	    return TRUE;

	case '#':		/* alternate file name */
	    *argp = getaltfname(errmsg);	/* may give emsg if not set */
	    return TRUE;

#ifdef FEAT_EVAL
	case '=':		/* result of expression */
	    *argp = get_expr_line();
	    *allocated = TRUE;
	    return TRUE;
#endif

	case ':':		/* last command line */
	    if (last_cmdline == NULL && errmsg)
		emsg(_(e_nolastcmd));
	    *argp = last_cmdline;
	    return TRUE;

	case '/':		/* last search-pattern */
	    if (last_search_pat() == NULL && errmsg)
		emsg(_(e_noprevre));
	    *argp = last_search_pat();
	    return TRUE;

	case '.':		/* last inserted text */
	    *argp = get_last_insert_save();
	    *allocated = TRUE;
	    if (*argp == NULL && errmsg)
		emsg(_(e_noinstext));
	    return TRUE;

#ifdef FEAT_SEARCHPATH
	case Ctrl_F:		/* Filename under cursor */
	case Ctrl_P:		/* Path under cursor, expand via "path" */
	    if (!errmsg)
		return FALSE;
	    *argp = file_name_at_cursor(FNAME_MESS | FNAME_HYP
			    | (regname == Ctrl_P ? FNAME_EXP : 0), 1L, NULL);
	    *allocated = TRUE;
	    return TRUE;
#endif

	case Ctrl_W:		/* word under cursor */
	case Ctrl_A:		/* WORD (mnemonic All) under cursor */
	    if (!errmsg)
		return FALSE;
	    cnt = find_ident_under_cursor(argp, regname == Ctrl_W
				   ?  (FIND_IDENT|FIND_STRING) : FIND_STRING);
	    *argp = cnt ? vim_strnsave(*argp, cnt) : NULL;
	    *allocated = TRUE;
	    return TRUE;

	case Ctrl_L:		/* Line under cursor */
	    if (!errmsg)
		return FALSE;

	    *argp = ml_get_buf(curwin->w_buffer,
			curwin->w_cursor.lnum, FALSE);
	    return TRUE;

	case '_':		/* black hole: always empty */
	    *argp = (char_u *)"";
	    return TRUE;
    }

    return FALSE;
}

/*
 * Paste a yank register into the command line.
 * Only for non-special registers.
 * Used by CTRL-R command in command-line mode
 * insert_reg() can't be used here, because special characters from the
 * register contents will be interpreted as commands.
 *
 * return FAIL for failure, OK otherwise
 */
    int
cmdline_paste_reg(
    int regname,
    int literally_arg,	/* Insert text literally instead of "as typed" */
    int remcr)		/* don't add CR characters */
{
    long	i;
    int		literally = literally_arg;

    if (get_yank_register(regname, FALSE))
	literally = TRUE;
    if (y_current->y_array == NULL)
	return FAIL;

    for (i = 0; i < y_current->y_size; ++i)
    {
	cmdline_paste_str(y_current->y_array[i], literally);

	/* Insert ^M between lines and after last line if type is MLINE.
	 * Don't do this when "remcr" is TRUE. */
	if ((y_current->y_type == MLINE || i < y_current->y_size - 1) && !remcr)
	    cmdline_paste_str((char_u *)"\r", literally);

	/* Check for CTRL-C, in case someone tries to paste a few thousand
	 * lines and gets bored. */
	ui_breakcheck();
	if (got_int)
	    return FAIL;
    }
    return OK;
}

#if defined(FEAT_CLIPBOARD) || defined(PROTO)
/*
 * Adjust the register name pointed to with "rp" for the clipboard being
 * used always and the clipboard being available.
 */
    void
adjust_clip_reg(int *rp)
{
    /* If no reg. specified, and "unnamed" or "unnamedplus" is in 'clipboard',
     * use '*' or '+' reg, respectively. "unnamedplus" prevails. */
    if (*rp == 0 && (clip_unnamed != 0 || clip_unnamed_saved != 0))
    {
	if (clip_unnamed != 0)
	    *rp = ((clip_unnamed & CLIP_UNNAMED_PLUS) && clip_plus.available)
								  ? '+' : '*';
	else
	    *rp = ((clip_unnamed_saved & CLIP_UNNAMED_PLUS) && clip_plus.available)
								  ? '+' : '*';
    }
    if (!clip_star.available && *rp == '*')
	*rp = 0;
    if (!clip_plus.available && *rp == '+')
	*rp = 0;
}
#endif

/*
 * Shift the delete registers: "9 is cleared, "8 becomes "9, etc.
 */
    void
shift_delete_registers()
{
    int		n;

    y_current = &y_regs[9];
    free_yank_all();			/* free register nine */
    for (n = 9; n > 1; --n)
	y_regs[n] = y_regs[n - 1];
    y_current = &y_regs[1];
    if (!y_append)
	y_previous = y_current;
    y_regs[1].y_array = NULL;		/* set register one to empty */
}

#if defined(FEAT_EVAL)
    static void
yank_do_autocmd(oparg_T *oap, yankreg_T *reg)
{
    static int	recursive = FALSE;
    dict_T	*v_event;
    list_T	*list;
    int		n;
    char_u	buf[NUMBUFLEN + 2];
    long	reglen = 0;

    if (recursive)
	return;

    v_event = get_vim_var_dict(VV_EVENT);

    list = list_alloc();
    if (list == NULL)
	return;
    for (n = 0; n < reg->y_size; n++)
	list_append_string(list, reg->y_array[n], -1);
    list->lv_lock = VAR_FIXED;
    dict_add_list(v_event, "regcontents", list);

    buf[0] = (char_u)oap->regname;
    buf[1] = NUL;
    dict_add_string(v_event, "regname", buf);

    buf[0] = get_op_char(oap->op_type);
    buf[1] = get_extra_op_char(oap->op_type);
    buf[2] = NUL;
    dict_add_string(v_event, "operator", buf);

    buf[0] = NUL;
    buf[1] = NUL;
    switch (get_reg_type(oap->regname, &reglen))
    {
	case MLINE: buf[0] = 'V'; break;
	case MCHAR: buf[0] = 'v'; break;
	case MBLOCK:
		vim_snprintf((char *)buf, sizeof(buf), "%c%ld", Ctrl_V,
			     reglen + 1);
		break;
    }
    dict_add_string(v_event, "regtype", buf);

    /* Lock the dictionary and its keys */
    dict_set_items_ro(v_event);

    recursive = TRUE;
    textlock++;
    apply_autocmds(EVENT_TEXTYANKPOST, NULL, NULL, FALSE, curbuf);
    textlock--;
    recursive = FALSE;

    /* Empty the dictionary, v:event is still valid */
    dict_free_contents(v_event);
    hash_init(&v_event->dv_hashtab);
}
#endif

/*
 * Handle a delete operation.
 *
 * Return FAIL if undo failed, OK otherwise.
 */
    int
op_delete(oparg_T *oap)
{
    int			n;
    linenr_T		lnum;
    char_u		*ptr;
    char_u		*newp, *oldp;
    struct block_def	bd;
    linenr_T		old_lcount = curbuf->b_ml.ml_line_count;
    int			did_yank = FALSE;

    if (curbuf->b_ml.ml_flags & ML_EMPTY)	    /* nothing to do */
	return OK;

    /* Nothing to delete, return here.	Do prepare undo, for op_change(). */
    if (oap->empty)
	return u_save_cursor();

    if (!curbuf->b_p_ma)
    {
	emsg(_(e_modifiable));
	return FAIL;
    }

#ifdef FEAT_CLIPBOARD
    adjust_clip_reg(&oap->regname);
#endif

    if (has_mbyte)
	mb_adjust_opend(oap);

    /*
     * Imitate the strange Vi behaviour: If the delete spans more than one
     * line and motion_type == MCHAR and the result is a blank line, make the
     * delete linewise.  Don't do this for the change command or Visual mode.
     */
    if (       oap->motion_type == MCHAR
	    && !oap->is_VIsual
	    && !oap->block_mode
	    && oap->line_count > 1
	    && oap->motion_force == NUL
	    && oap->op_type == OP_DELETE)
    {
	ptr = ml_get(oap->end.lnum) + oap->end.col;
	if (*ptr != NUL)
	    ptr += oap->inclusive;
	ptr = skipwhite(ptr);
	if (*ptr == NUL && inindent(0))
	    oap->motion_type = MLINE;
    }

    /*
     * Check for trying to delete (e.g. "D") in an empty line.
     * Note: For the change operator it is ok.
     */
    if (       oap->motion_type == MCHAR
	    && oap->line_count == 1
	    && oap->op_type == OP_DELETE
	    && *ml_get(oap->start.lnum) == NUL)
    {
	/*
	 * It's an error to operate on an empty region, when 'E' included in
	 * 'cpoptions' (Vi compatible).
	 */
	if (virtual_op)
	    /* Virtual editing: Nothing gets deleted, but we set the '[ and ']
	     * marks as if it happened. */
	    goto setmarks;
	if (vim_strchr(p_cpo, CPO_EMPTYREGION) != NULL)
	    beep_flush();
	return OK;
    }

    /*
     * Do a yank of whatever we're about to delete.
     * If a yank register was specified, put the deleted text into that
     * register.  For the black hole register '_' don't yank anything.
     */
    if (oap->regname != '_')
    {
	if (oap->regname != 0)
	{
	    /* check for read-only register */
	    if (!valid_yank_reg(oap->regname, TRUE))
	    {
		beep_flush();
		return OK;
	    }
	    get_yank_register(oap->regname, TRUE); /* yank into specif'd reg. */
	    if (op_yank(oap, TRUE, FALSE) == OK)   /* yank without message */
		did_yank = TRUE;
	}

	/*
	 * Put deleted text into register 1 and shift number registers if the
	 * delete contains a line break, or when using a specific operator (Vi
	 * compatible)
	 * Use the register name from before adjust_clip_reg() may have
	 * changed it.
	 */
	if (oap->motion_type == MLINE || oap->line_count > 1
							   || oap->use_reg_one)
	{
	    shift_delete_registers();
	    if (op_yank(oap, TRUE, FALSE) == OK)
		did_yank = TRUE;
	}

	/* Yank into small delete register when no named register specified
	 * and the delete is within one line. */
	if ((
#ifdef FEAT_CLIPBOARD
	    ((clip_unnamed & CLIP_UNNAMED) && oap->regname == '*') ||
	    ((clip_unnamed & CLIP_UNNAMED_PLUS) && oap->regname == '+') ||
#endif
	    oap->regname == 0) && oap->motion_type != MLINE
						      && oap->line_count == 1)
	{
	    oap->regname = '-';
	    get_yank_register(oap->regname, TRUE);
	    if (op_yank(oap, TRUE, FALSE) == OK)
		did_yank = TRUE;
	    oap->regname = 0;
	}

	/*
	 * If there's too much stuff to fit in the yank register, then get a
	 * confirmation before doing the delete. This is crude, but simple.
	 * And it avoids doing a delete of something we can't put back if we
	 * want.
	 */
	if (!did_yank)
	{
	    int msg_silent_save = msg_silent;

	    msg_silent = 0;	/* must display the prompt */
	    n = ask_yesno((char_u *)_("cannot yank; delete anyway"), TRUE);
	    msg_silent = msg_silent_save;
	    if (n != 'y')
	    {
		emsg(_(e_abort));
		return FAIL;
	    }
	}

#if defined(FEAT_EVAL)
	if (did_yank && has_textyankpost())
	    yank_do_autocmd(oap, y_current);
#endif
    }

    /*
     * block mode delete
     */
    if (oap->block_mode)
    {
	if (u_save((linenr_T)(oap->start.lnum - 1),
			       (linenr_T)(oap->end.lnum + 1)) == FAIL)
	    return FAIL;

	for (lnum = curwin->w_cursor.lnum; lnum <= oap->end.lnum; ++lnum)
	{
	    block_prep(oap, &bd, lnum, TRUE);
	    if (bd.textlen == 0)	/* nothing to delete */
		continue;

	    /* Adjust cursor position for tab replaced by spaces and 'lbr'. */
	    if (lnum == curwin->w_cursor.lnum)
	    {
		curwin->w_cursor.col = bd.textcol + bd.startspaces;
		curwin->w_cursor.coladd = 0;
	    }

	    /* n == number of chars deleted
	     * If we delete a TAB, it may be replaced by several characters.
	     * Thus the number of characters may increase!
	     */
	    n = bd.textlen - bd.startspaces - bd.endspaces;
	    oldp = ml_get(lnum);
	    newp = alloc_check((unsigned)STRLEN(oldp) + 1 - n);
	    if (newp == NULL)
		continue;
	    /* copy up to deleted part */
	    mch_memmove(newp, oldp, (size_t)bd.textcol);
	    /* insert spaces */
	    vim_memset(newp + bd.textcol, ' ',
				     (size_t)(bd.startspaces + bd.endspaces));
	    /* copy the part after the deleted part */
	    oldp += bd.textcol + bd.textlen;
	    STRMOVE(newp + bd.textcol + bd.startspaces + bd.endspaces, oldp);
	    /* replace the line */
	    ml_replace(lnum, newp, FALSE);
	}

	check_cursor_col();
	changed_lines(curwin->w_cursor.lnum, curwin->w_cursor.col,
						       oap->end.lnum + 1, 0L);
	oap->line_count = 0;	    /* no lines deleted */
    }
    else if (oap->motion_type == MLINE)
    {
	if (oap->op_type == OP_CHANGE)
	{
	    /* Delete the lines except the first one.  Temporarily move the
	     * cursor to the next line.  Save the current line number, if the
	     * last line is deleted it may be changed.
	     */
	    if (oap->line_count > 1)
	    {
		lnum = curwin->w_cursor.lnum;
		++curwin->w_cursor.lnum;
		del_lines((long)(oap->line_count - 1), TRUE);
		curwin->w_cursor.lnum = lnum;
	    }
	    if (u_save_cursor() == FAIL)
		return FAIL;
	    if (curbuf->b_p_ai)		    /* don't delete indent */
	    {
		beginline(BL_WHITE);	    /* cursor on first non-white */
		did_ai = TRUE;		    /* delete the indent when ESC hit */
		ai_col = curwin->w_cursor.col;
	    }
	    else
		beginline(0);		    /* cursor in column 0 */
	    truncate_line(FALSE);   /* delete the rest of the line */
				    /* leave cursor past last char in line */
	    if (oap->line_count > 1)
		u_clearline();	    /* "U" command not possible after "2cc" */
	}
	else
	{
	    del_lines(oap->line_count, TRUE);
	    beginline(BL_WHITE | BL_FIX);
	    u_clearline();	/* "U" command not possible after "dd" */
	}
    }
    else
    {
	if (virtual_op)
	{
	    int		endcol = 0;

	    /* For virtualedit: break the tabs that are partly included. */
	    if (gchar_pos(&oap->start) == '\t')
	    {
		if (u_save_cursor() == FAIL)	/* save first line for undo */
		    return FAIL;
		if (oap->line_count == 1)
		    endcol = getviscol2(oap->end.col, oap->end.coladd);
		coladvance_force(getviscol2(oap->start.col, oap->start.coladd));
		oap->start = curwin->w_cursor;
		if (oap->line_count == 1)
		{
		    coladvance(endcol);
		    oap->end.col = curwin->w_cursor.col;
		    oap->end.coladd = curwin->w_cursor.coladd;
		    curwin->w_cursor = oap->start;
		}
	    }

	    /* Break a tab only when it's included in the area. */
	    if (gchar_pos(&oap->end) == '\t'
				     && (int)oap->end.coladd < oap->inclusive)
	    {
		/* save last line for undo */
		if (u_save((linenr_T)(oap->end.lnum - 1),
				       (linenr_T)(oap->end.lnum + 1)) == FAIL)
		    return FAIL;
		curwin->w_cursor = oap->end;
		coladvance_force(getviscol2(oap->end.col, oap->end.coladd));
		oap->end = curwin->w_cursor;
		curwin->w_cursor = oap->start;
	    }
	}

	if (oap->line_count == 1)	/* delete characters within one line */
	{
	    if (u_save_cursor() == FAIL)	/* save line for undo */
		return FAIL;

	    /* if 'cpoptions' contains '$', display '$' at end of change */
	    if (       vim_strchr(p_cpo, CPO_DOLLAR) != NULL
		    && oap->op_type == OP_CHANGE
		    && oap->end.lnum == curwin->w_cursor.lnum
		    && !oap->is_VIsual)
		display_dollar(oap->end.col - !oap->inclusive);

	    n = oap->end.col - oap->start.col + 1 - !oap->inclusive;

	    if (virtual_op)
	    {
		/* fix up things for virtualedit-delete:
		 * break the tabs which are going to get in our way
		 */
		char_u		*curline = ml_get_curline();
		int		len = (int)STRLEN(curline);

		if (oap->end.coladd != 0
			&& (int)oap->end.col >= len - 1
			&& !(oap->start.coladd && (int)oap->end.col >= len - 1))
		    n++;
		/* Delete at least one char (e.g, when on a control char). */
		if (n == 0 && oap->start.coladd != oap->end.coladd)
		    n = 1;

		/* When deleted a char in the line, reset coladd. */
		if (gchar_cursor() != NUL)
		    curwin->w_cursor.coladd = 0;
	    }
	    (void)del_bytes((long)n, !virtual_op,
			    oap->op_type == OP_DELETE && !oap->is_VIsual);
	}
	else				/* delete characters between lines */
	{
	    pos_T   curpos;

	    /* save deleted and changed lines for undo */
	    if (u_save((linenr_T)(curwin->w_cursor.lnum - 1),
		 (linenr_T)(curwin->w_cursor.lnum + oap->line_count)) == FAIL)
		return FAIL;

	    truncate_line(TRUE);	/* delete from cursor to end of line */

	    curpos = curwin->w_cursor;	/* remember curwin->w_cursor */
	    ++curwin->w_cursor.lnum;
	    del_lines((long)(oap->line_count - 2), FALSE);

	    /* delete from start of line until op_end */
	    n = (oap->end.col + 1 - !oap->inclusive);
	    curwin->w_cursor.col = 0;
	    (void)del_bytes((long)n, !virtual_op,
			    oap->op_type == OP_DELETE && !oap->is_VIsual);
	    curwin->w_cursor = curpos;	/* restore curwin->w_cursor */
	    (void)do_join(2, FALSE, FALSE, FALSE, FALSE);
	}
    }

    msgmore(curbuf->b_ml.ml_line_count - old_lcount);

setmarks:
    if (oap->block_mode)
    {
	curbuf->b_op_end.lnum = oap->end.lnum;
	curbuf->b_op_end.col = oap->start.col;
    }
    else
	curbuf->b_op_end = oap->start;
    curbuf->b_op_start = oap->start;

    return OK;
}

/*
 * Adjust end of operating area for ending on a multi-byte character.
 * Used for deletion.
 */
    static void
mb_adjust_opend(oparg_T *oap)
{
    char_u	*p;

    if (oap->inclusive)
    {
	p = ml_get(oap->end.lnum);
	oap->end.col += mb_tail_off(p, p + oap->end.col);
    }
}

/*
 * Replace the character under the cursor with "c".
 * This takes care of multi-byte characters.
 */
    static void
replace_character(int c)
{
    int n = State;

    State = REPLACE;
    ins_char(c);
    State = n;
    /* Backup to the replaced character. */
    dec_cursor();
}

/*
 * Replace a whole area with one character.
 */
    int
op_replace(oparg_T *oap, int c)
{
    int			n, numc;
    int			num_chars;
    char_u		*newp, *oldp;
    size_t		oldlen;
    struct block_def	bd;
    char_u		*after_p = NULL;
    int			had_ctrl_v_cr = FALSE;

    if ((curbuf->b_ml.ml_flags & ML_EMPTY ) || oap->empty)
	return OK;	    /* nothing to do */

    if (c == REPLACE_CR_NCHAR)
    {
	had_ctrl_v_cr = TRUE;
	c = CAR;
    }
    else if (c == REPLACE_NL_NCHAR)
    {
	had_ctrl_v_cr = TRUE;
	c = NL;
    }

    if (has_mbyte)
	mb_adjust_opend(oap);

    if (u_save((linenr_T)(oap->start.lnum - 1),
				       (linenr_T)(oap->end.lnum + 1)) == FAIL)
	return FAIL;

    /*
     * block mode replace
     */
    if (oap->block_mode)
    {
	bd.is_MAX = (curwin->w_curswant == MAXCOL);
	for ( ; curwin->w_cursor.lnum <= oap->end.lnum; ++curwin->w_cursor.lnum)
	{
	    curwin->w_cursor.col = 0;  /* make sure cursor position is valid */
	    block_prep(oap, &bd, curwin->w_cursor.lnum, TRUE);
	    if (bd.textlen == 0 && (!virtual_op || bd.is_MAX))
		continue;	    /* nothing to replace */

	    /* n == number of extra chars required
	     * If we split a TAB, it may be replaced by several characters.
	     * Thus the number of characters may increase!
	     */
	    /* If the range starts in virtual space, count the initial
	     * coladd offset as part of "startspaces" */
	    if (virtual_op && bd.is_short && *bd.textstart == NUL)
	    {
		pos_T vpos;

		vpos.lnum = curwin->w_cursor.lnum;
		getvpos(&vpos, oap->start_vcol);
		bd.startspaces += vpos.coladd;
		n = bd.startspaces;
	    }
	    else
		/* allow for pre spaces */
		n = (bd.startspaces ? bd.start_char_vcols - 1 : 0);

	    /* allow for post spp */
	    n += (bd.endspaces
		    && !bd.is_oneChar
		    && bd.end_char_vcols > 0) ? bd.end_char_vcols - 1 : 0;
	    /* Figure out how many characters to replace. */
	    numc = oap->end_vcol - oap->start_vcol + 1;
	    if (bd.is_short && (!virtual_op || bd.is_MAX))
		numc -= (oap->end_vcol - bd.end_vcol) + 1;

	    /* A double-wide character can be replaced only up to half the
	     * times. */
	    if ((*mb_char2cells)(c) > 1)
	    {
		if ((numc & 1) && !bd.is_short)
		{
		    ++bd.endspaces;
		    ++n;
		}
		numc = numc / 2;
	    }

	    /* Compute bytes needed, move character count to num_chars. */
	    num_chars = numc;
	    numc *= (*mb_char2len)(c);
	    /* oldlen includes textlen, so don't double count */
	    n += numc - bd.textlen;

	    oldp = ml_get_curline();
	    oldlen = STRLEN(oldp);
	    newp = alloc_check((unsigned)oldlen + 1 + n);
	    if (newp == NULL)
		continue;
	    vim_memset(newp, NUL, (size_t)(oldlen + 1 + n));
	    /* copy up to deleted part */
	    mch_memmove(newp, oldp, (size_t)bd.textcol);
	    oldp += bd.textcol + bd.textlen;
	    /* insert pre-spaces */
	    vim_memset(newp + bd.textcol, ' ', (size_t)bd.startspaces);
	    /* insert replacement chars CHECK FOR ALLOCATED SPACE */
	    /* REPLACE_CR_NCHAR/REPLACE_NL_NCHAR is used for entering CR
	     * literally. */
	    if (had_ctrl_v_cr || (c != '\r' && c != '\n'))
	    {
		if (has_mbyte)
		{
		    n = (int)STRLEN(newp);
		    while (--num_chars >= 0)
			n += (*mb_char2bytes)(c, newp + n);
		}
		else
		    vim_memset(newp + STRLEN(newp), c, (size_t)numc);
		if (!bd.is_short)
		{
		    /* insert post-spaces */
		    vim_memset(newp + STRLEN(newp), ' ', (size_t)bd.endspaces);
		    /* copy the part after the changed part */
		    STRMOVE(newp + STRLEN(newp), oldp);
		}
	    }
	    else
	    {
		/* Replacing with \r or \n means splitting the line. */
		after_p = alloc_check(
				   (unsigned)(oldlen + 1 + n - STRLEN(newp)));
		if (after_p != NULL)
		    STRMOVE(after_p, oldp);
	    }
	    /* replace the line */
	    ml_replace(curwin->w_cursor.lnum, newp, FALSE);
	    if (after_p != NULL)
	    {
		ml_append(curwin->w_cursor.lnum++, after_p, 0, FALSE);
		appended_lines_mark(curwin->w_cursor.lnum, 1L);
		oap->end.lnum++;
		vim_free(after_p);
	    }
	}
    }
    else
    {
	/*
	 * MCHAR and MLINE motion replace.
	 */
	if (oap->motion_type == MLINE)
	{
	    oap->start.col = 0;
	    curwin->w_cursor.col = 0;
	    oap->end.col = (colnr_T)STRLEN(ml_get(oap->end.lnum));
	    if (oap->end.col)
		--oap->end.col;
	}
	else if (!oap->inclusive)
	    dec(&(oap->end));

	while (LTOREQ_POS(curwin->w_cursor, oap->end))
	{
	    n = gchar_cursor();
	    if (n != NUL)
	    {
		if ((*mb_char2len)(c) > 1 || (*mb_char2len)(n) > 1)
		{
		    /* This is slow, but it handles replacing a single-byte
		     * with a multi-byte and the other way around. */
		    if (curwin->w_cursor.lnum == oap->end.lnum)
			oap->end.col += (*mb_char2len)(c) - (*mb_char2len)(n);
		    replace_character(c);
		}
		else
		{
		    if (n == TAB)
		    {
			int end_vcol = 0;

			if (curwin->w_cursor.lnum == oap->end.lnum)
			{
			    /* oap->end has to be recalculated when
			     * the tab breaks */
			    end_vcol = getviscol2(oap->end.col,
							     oap->end.coladd);
			}
			coladvance_force(getviscol());
			if (curwin->w_cursor.lnum == oap->end.lnum)
			    getvpos(&oap->end, end_vcol);
		    }
		    PBYTE(curwin->w_cursor, c);
		}
	    }
	    else if (virtual_op && curwin->w_cursor.lnum == oap->end.lnum)
	    {
		int virtcols = oap->end.coladd;

		if (curwin->w_cursor.lnum == oap->start.lnum
			&& oap->start.col == oap->end.col && oap->start.coladd)
		    virtcols -= oap->start.coladd;

		/* oap->end has been trimmed so it's effectively inclusive;
		 * as a result an extra +1 must be counted so we don't
		 * trample the NUL byte. */
		coladvance_force(getviscol2(oap->end.col, oap->end.coladd) + 1);
		curwin->w_cursor.col -= (virtcols + 1);
		for (; virtcols >= 0; virtcols--)
		{
                   if ((*mb_char2len)(c) > 1)
		       replace_character(c);
                   else
			PBYTE(curwin->w_cursor, c);
		   if (inc(&curwin->w_cursor) == -1)
		       break;
		}
	    }

	    /* Advance to next character, stop at the end of the file. */
	    if (inc_cursor() == -1)
		break;
	}
    }

    curwin->w_cursor = oap->start;
    check_cursor();
    changed_lines(oap->start.lnum, oap->start.col, oap->end.lnum + 1, 0L);

    /* Set "'[" and "']" marks. */
    curbuf->b_op_start = oap->start;
    curbuf->b_op_end = oap->end;

    return OK;
}

static int swapchars(int op_type, pos_T *pos, int length);

/*
 * Handle the (non-standard vi) tilde operator.  Also for "gu", "gU" and "g?".
 */
    void
op_tilde(oparg_T *oap)
{
    pos_T		pos;
    struct block_def	bd;
    int			did_change = FALSE;

    if (u_save((linenr_T)(oap->start.lnum - 1),
				       (linenr_T)(oap->end.lnum + 1)) == FAIL)
	return;

    pos = oap->start;
    if (oap->block_mode)		    /* Visual block mode */
    {
	for (; pos.lnum <= oap->end.lnum; ++pos.lnum)
	{
	    int one_change;

	    block_prep(oap, &bd, pos.lnum, FALSE);
	    pos.col = bd.textcol;
	    one_change = swapchars(oap->op_type, &pos, bd.textlen);
	    did_change |= one_change;

#ifdef FEAT_NETBEANS_INTG
	    if (netbeans_active() && one_change)
	    {
		char_u *ptr = ml_get_buf(curbuf, pos.lnum, FALSE);

		netbeans_removed(curbuf, pos.lnum, bd.textcol,
							    (long)bd.textlen);
		netbeans_inserted(curbuf, pos.lnum, bd.textcol,
						&ptr[bd.textcol], bd.textlen);
	    }
#endif
	}
	if (did_change)
	    changed_lines(oap->start.lnum, 0, oap->end.lnum + 1, 0L);
    }
    else				    /* not block mode */
    {
	if (oap->motion_type == MLINE)
	{
	    oap->start.col = 0;
	    pos.col = 0;
	    oap->end.col = (colnr_T)STRLEN(ml_get(oap->end.lnum));
	    if (oap->end.col)
		--oap->end.col;
	}
	else if (!oap->inclusive)
	    dec(&(oap->end));

	if (pos.lnum == oap->end.lnum)
	    did_change = swapchars(oap->op_type, &pos,
						  oap->end.col - pos.col + 1);
	else
	    for (;;)
	    {
		did_change |= swapchars(oap->op_type, &pos,
				pos.lnum == oap->end.lnum ? oap->end.col + 1:
					   (int)STRLEN(ml_get_pos(&pos)));
		if (LTOREQ_POS(oap->end, pos) || inc(&pos) == -1)
		    break;
	    }
	if (did_change)
	{
	    changed_lines(oap->start.lnum, oap->start.col, oap->end.lnum + 1,
									  0L);
#ifdef FEAT_NETBEANS_INTG
	    if (netbeans_active() && did_change)
	    {
		char_u *ptr;
		int count;

		pos = oap->start;
		while (pos.lnum < oap->end.lnum)
		{
		    ptr = ml_get_buf(curbuf, pos.lnum, FALSE);
		    count = (int)STRLEN(ptr) - pos.col;
		    netbeans_removed(curbuf, pos.lnum, pos.col, (long)count);
		    netbeans_inserted(curbuf, pos.lnum, pos.col,
							&ptr[pos.col], count);
		    pos.col = 0;
		    pos.lnum++;
		}
		ptr = ml_get_buf(curbuf, pos.lnum, FALSE);
		count = oap->end.col - pos.col + 1;
		netbeans_removed(curbuf, pos.lnum, pos.col, (long)count);
		netbeans_inserted(curbuf, pos.lnum, pos.col,
							&ptr[pos.col], count);
	    }
#endif
	}
    }

    if (!did_change && oap->is_VIsual)
	/* No change: need to remove the Visual selection */
	redraw_curbuf_later(INVERTED);

    /*
     * Set '[ and '] marks.
     */
    curbuf->b_op_start = oap->start;
    curbuf->b_op_end = oap->end;

    if (oap->line_count > p_report)
	smsg(NGETTEXT("%ld line changed", "%ld lines changed",
					    oap->line_count), oap->line_count);
}

/*
 * Invoke swapchar() on "length" bytes at position "pos".
 * "pos" is advanced to just after the changed characters.
 * "length" is rounded up to include the whole last multi-byte character.
 * Also works correctly when the number of bytes changes.
 * Returns TRUE if some character was changed.
 */
    static int
swapchars(int op_type, pos_T *pos, int length)
{
    int todo;
    int	did_change = 0;

    for (todo = length; todo > 0; --todo)
    {
	if (has_mbyte)
	{
	    int len = (*mb_ptr2len)(ml_get_pos(pos));

	    /* we're counting bytes, not characters */
	    if (len > 0)
		todo -= len - 1;
	}
	did_change |= swapchar(op_type, pos);
	if (inc(pos) == -1)    /* at end of file */
	    break;
    }
    return did_change;
}

/*
 * If op_type == OP_UPPER: make uppercase,
 * if op_type == OP_LOWER: make lowercase,
 * if op_type == OP_ROT13: do rot13 encoding,
 * else swap case of character at 'pos'
 * returns TRUE when something actually changed.
 */
    int
swapchar(int op_type, pos_T *pos)
{
    int	    c;
    int	    nc;

    c = gchar_pos(pos);

    /* Only do rot13 encoding for ASCII characters. */
    if (c >= 0x80 && op_type == OP_ROT13)
	return FALSE;

    if (op_type == OP_UPPER && c == 0xdf
		      && (enc_latin1like || STRCMP(p_enc, "iso-8859-2") == 0))
    {
	pos_T   sp = curwin->w_cursor;

	/* Special handling of German sharp s: change to "SS". */
	curwin->w_cursor = *pos;
	del_char(FALSE);
	ins_char('S');
	ins_char('S');
	curwin->w_cursor = sp;
	inc(pos);
    }

    if (enc_dbcs != 0 && c >= 0x100)	/* No lower/uppercase letter */
	return FALSE;
    nc = c;
    if (MB_ISLOWER(c))
    {
	if (op_type == OP_ROT13)
	    nc = ROT13(c, 'a');
	else if (op_type != OP_LOWER)
	    nc = MB_TOUPPER(c);
    }
    else if (MB_ISUPPER(c))
    {
	if (op_type == OP_ROT13)
	    nc = ROT13(c, 'A');
	else if (op_type != OP_UPPER)
	    nc = MB_TOLOWER(c);
    }
    if (nc != c)
    {
	if (enc_utf8 && (c >= 0x80 || nc >= 0x80))
	{
	    pos_T   sp = curwin->w_cursor;

	    curwin->w_cursor = *pos;
	    /* don't use del_char(), it also removes composing chars */
	    del_bytes(utf_ptr2len(ml_get_cursor()), FALSE, FALSE);
	    ins_char(nc);
	    curwin->w_cursor = sp;
	}
	else
	    PBYTE(*pos, nc);
	return TRUE;
    }
    return FALSE;
}

/*
 * op_insert - Insert and append operators for Visual mode.
 */
    void
op_insert(oparg_T *oap, long count1)
{
    long		ins_len, pre_textlen = 0;
    char_u		*firstline, *ins_text;
    colnr_T		ind_pre = 0, ind_post;
    struct block_def	bd;
    int			i;
    pos_T		t1;

    /* edit() changes this - record it for OP_APPEND */
    bd.is_MAX = (curwin->w_curswant == MAXCOL);

    /* vis block is still marked. Get rid of it now. */
    curwin->w_cursor.lnum = oap->start.lnum;
    update_screen(INVERTED);

    if (oap->block_mode)
    {
	/* When 'virtualedit' is used, need to insert the extra spaces before
	 * doing block_prep().  When only "block" is used, virtual edit is
	 * already disabled, but still need it when calling
	 * coladvance_force(). */
	if (curwin->w_cursor.coladd > 0)
	{
	    int		old_ve_flags = ve_flags;

	    ve_flags = VE_ALL;
	    if (u_save_cursor() == FAIL)
		return;
	    coladvance_force(oap->op_type == OP_APPEND
					   ? oap->end_vcol + 1 : getviscol());
	    if (oap->op_type == OP_APPEND)
		--curwin->w_cursor.col;
	    ve_flags = old_ve_flags;
	}
	/* Get the info about the block before entering the text */
	block_prep(oap, &bd, oap->start.lnum, TRUE);
	/* Get indent information */
	ind_pre = (colnr_T)getwhitecols_curline();
	firstline = ml_get(oap->start.lnum) + bd.textcol;

	if (oap->op_type == OP_APPEND)
	    firstline += bd.textlen;
	pre_textlen = (long)STRLEN(firstline);
    }

    if (oap->op_type == OP_APPEND)
    {
	if (oap->block_mode && curwin->w_cursor.coladd == 0)
	{
	    /* Move the cursor to the character right of the block. */
	    curwin->w_set_curswant = TRUE;
	    while (*ml_get_cursor() != NUL
		    && (curwin->w_cursor.col < bd.textcol + bd.textlen))
		++curwin->w_cursor.col;
	    if (bd.is_short && !bd.is_MAX)
	    {
		/* First line was too short, make it longer and adjust the
		 * values in "bd". */
		if (u_save_cursor() == FAIL)
		    return;
		for (i = 0; i < bd.endspaces; ++i)
		    ins_char(' ');
		bd.textlen += bd.endspaces;
	    }
	}
	else
	{
	    curwin->w_cursor = oap->end;
	    check_cursor_col();

	    /* Works just like an 'i'nsert on the next character. */
	    if (!LINEEMPTY(curwin->w_cursor.lnum)
		    && oap->start_vcol != oap->end_vcol)
		inc_cursor();
	}
    }

    t1 = oap->start;
    (void)edit(NUL, FALSE, (linenr_T)count1);

    /* When a tab was inserted, and the characters in front of the tab
     * have been converted to a tab as well, the column of the cursor
     * might have actually been reduced, so need to adjust here. */
    if (t1.lnum == curbuf->b_op_start_orig.lnum
	    && LT_POS(curbuf->b_op_start_orig, t1))
	oap->start = curbuf->b_op_start_orig;

    /* If user has moved off this line, we don't know what to do, so do
     * nothing.
     * Also don't repeat the insert when Insert mode ended with CTRL-C. */
    if (curwin->w_cursor.lnum != oap->start.lnum || got_int)
	return;

    if (oap->block_mode)
    {
	struct block_def	bd2;
	int			did_indent = FALSE;
	size_t			len;
	int			add;

	/* If indent kicked in, the firstline might have changed
	 * but only do that, if the indent actually increased. */
	ind_post = (colnr_T)getwhitecols_curline();
	if (curbuf->b_op_start.col > ind_pre && ind_post > ind_pre)
	{
	    bd.textcol += ind_post - ind_pre;
	    bd.start_vcol += ind_post - ind_pre;
	    did_indent = TRUE;
	}

	/* The user may have moved the cursor before inserting something, try
	 * to adjust the block for that.  But only do it, if the difference
	 * does not come from indent kicking in. */
	if (oap->start.lnum == curbuf->b_op_start_orig.lnum
						  && !bd.is_MAX && !did_indent)
	{
	    if (oap->op_type == OP_INSERT
		    && oap->start.col + oap->start.coladd
			!= curbuf->b_op_start_orig.col
					      + curbuf->b_op_start_orig.coladd)
	    {
		int t = getviscol2(curbuf->b_op_start_orig.col,
					      curbuf->b_op_start_orig.coladd);
		oap->start.col = curbuf->b_op_start_orig.col;
		pre_textlen -= t - oap->start_vcol;
		oap->start_vcol = t;
	    }
	    else if (oap->op_type == OP_APPEND
		      && oap->end.col + oap->end.coladd
			>= curbuf->b_op_start_orig.col
					      + curbuf->b_op_start_orig.coladd)
	    {
		int t = getviscol2(curbuf->b_op_start_orig.col,
					      curbuf->b_op_start_orig.coladd);
		oap->start.col = curbuf->b_op_start_orig.col;
		/* reset pre_textlen to the value of OP_INSERT */
		pre_textlen += bd.textlen;
		pre_textlen -= t - oap->start_vcol;
		oap->start_vcol = t;
		oap->op_type = OP_INSERT;
	    }
	}

	/*
	 * Spaces and tabs in the indent may have changed to other spaces and
	 * tabs.  Get the starting column again and correct the length.
	 * Don't do this when "$" used, end-of-line will have changed.
	 */
	block_prep(oap, &bd2, oap->start.lnum, TRUE);
	if (!bd.is_MAX || bd2.textlen < bd.textlen)
	{
	    if (oap->op_type == OP_APPEND)
	    {
		pre_textlen += bd2.textlen - bd.textlen;
		if (bd2.endspaces)
		    --bd2.textlen;
	    }
	    bd.textcol = bd2.textcol;
	    bd.textlen = bd2.textlen;
	}

	/*
	 * Subsequent calls to ml_get() flush the firstline data - take a
	 * copy of the required string.
	 */
	firstline = ml_get(oap->start.lnum);
	len = STRLEN(firstline);
	add = bd.textcol;
	if (oap->op_type == OP_APPEND)
	    add += bd.textlen;
	if ((size_t)add > len)
	    firstline += len;  // short line, point to the NUL
	else
	    firstline += add;
	if (pre_textlen >= 0
		     && (ins_len = (long)STRLEN(firstline) - pre_textlen) > 0)
	{
	    ins_text = vim_strnsave(firstline, (int)ins_len);
	    if (ins_text != NULL)
	    {
		/* block handled here */
		if (u_save(oap->start.lnum,
					 (linenr_T)(oap->end.lnum + 1)) == OK)
		    block_insert(oap, ins_text, (oap->op_type == OP_INSERT),
									 &bd);

		curwin->w_cursor.col = oap->start.col;
		check_cursor();
		vim_free(ins_text);
	    }
	}
    }
}

/*
 * op_change - handle a change operation
 *
 * return TRUE if edit() returns because of a CTRL-O command
 */
    int
op_change(oparg_T *oap)
{
    colnr_T		l;
    int			retval;
    long		offset;
    linenr_T		linenr;
    long		ins_len;
    long		pre_textlen = 0;
    long		pre_indent = 0;
    char_u		*firstline;
    char_u		*ins_text, *newp, *oldp;
    struct block_def	bd;

    l = oap->start.col;
    if (oap->motion_type == MLINE)
    {
	l = 0;
#ifdef FEAT_SMARTINDENT
	if (!p_paste && curbuf->b_p_si
# ifdef FEAT_CINDENT
		&& !curbuf->b_p_cin
# endif
		)
	    can_si = TRUE;	/* It's like opening a new line, do si */
#endif
    }

    /* First delete the text in the region.  In an empty buffer only need to
     * save for undo */
    if (curbuf->b_ml.ml_flags & ML_EMPTY)
    {
	if (u_save_cursor() == FAIL)
	    return FALSE;
    }
    else if (op_delete(oap) == FAIL)
	return FALSE;

    if ((l > curwin->w_cursor.col) && !LINEEMPTY(curwin->w_cursor.lnum)
							 && !virtual_op)
	inc_cursor();

    /* check for still on same line (<CR> in inserted text meaningless) */
    /* skip blank lines too */
    if (oap->block_mode)
    {
	/* Add spaces before getting the current line length. */
	if (virtual_op && (curwin->w_cursor.coladd > 0
						    || gchar_cursor() == NUL))
	    coladvance_force(getviscol());
	firstline = ml_get(oap->start.lnum);
	pre_textlen = (long)STRLEN(firstline);
	pre_indent = (long)getwhitecols(firstline);
	bd.textcol = curwin->w_cursor.col;
    }

#if defined(FEAT_LISP) || defined(FEAT_CINDENT)
    if (oap->motion_type == MLINE)
	fix_indent();
#endif

    retval = edit(NUL, FALSE, (linenr_T)1);

    /*
     * In Visual block mode, handle copying the new text to all lines of the
     * block.
     * Don't repeat the insert when Insert mode ended with CTRL-C.
     */
    if (oap->block_mode && oap->start.lnum != oap->end.lnum && !got_int)
    {
	/* Auto-indenting may have changed the indent.  If the cursor was past
	 * the indent, exclude that indent change from the inserted text. */
	firstline = ml_get(oap->start.lnum);
	if (bd.textcol > (colnr_T)pre_indent)
	{
	    long new_indent = (long)getwhitecols(firstline);

	    pre_textlen += new_indent - pre_indent;
	    bd.textcol += new_indent - pre_indent;
	}

	ins_len = (long)STRLEN(firstline) - pre_textlen;
	if (ins_len > 0)
	{
	    /* Subsequent calls to ml_get() flush the firstline data - take a
	     * copy of the inserted text.  */
	    if ((ins_text = alloc_check((unsigned)(ins_len + 1))) != NULL)
	    {
		vim_strncpy(ins_text, firstline + bd.textcol, (size_t)ins_len);
		for (linenr = oap->start.lnum + 1; linenr <= oap->end.lnum;
								     linenr++)
		{
		    block_prep(oap, &bd, linenr, TRUE);
		    if (!bd.is_short || virtual_op)
		    {
			pos_T vpos;

			/* If the block starts in virtual space, count the
			 * initial coladd offset as part of "startspaces" */
			if (bd.is_short)
			{
			    vpos.lnum = linenr;
			    (void)getvpos(&vpos, oap->start_vcol);
			}
			else
			    vpos.coladd = 0;
			oldp = ml_get(linenr);
			newp = alloc_check((unsigned)(STRLEN(oldp)
						 + vpos.coladd + ins_len + 1));
			if (newp == NULL)
			    continue;
			/* copy up to block start */
			mch_memmove(newp, oldp, (size_t)bd.textcol);
			offset = bd.textcol;
			vim_memset(newp + offset, ' ', (size_t)vpos.coladd);
			offset += vpos.coladd;
			mch_memmove(newp + offset, ins_text, (size_t)ins_len);
			offset += ins_len;
			oldp += bd.textcol;
			STRMOVE(newp + offset, oldp);
			ml_replace(linenr, newp, FALSE);
		    }
		}
		check_cursor();

		changed_lines(oap->start.lnum + 1, 0, oap->end.lnum + 1, 0L);
	    }
	    vim_free(ins_text);
	}
    }

    return retval;
}

/*
 * set all the yank registers to empty (called from main())
 */
    void
init_yank(void)
{
    int		i;

    for (i = 0; i < NUM_REGISTERS; ++i)
	y_regs[i].y_array = NULL;
}

#if defined(EXITFREE) || defined(PROTO)
    void
clear_registers(void)
{
    int		i;

    for (i = 0; i < NUM_REGISTERS; ++i)
    {
	y_current = &y_regs[i];
	if (y_current->y_array != NULL)
	    free_yank_all();
    }
}
#endif

/*
 * Free "n" lines from the current yank register.
 * Called for normal freeing and in case of error.
 */
    static void
free_yank(long n)
{
    if (y_current->y_array != NULL)
    {
	long	    i;

	for (i = n; --i >= 0; )
	{
#ifdef AMIGA	    /* only for very slow machines */
	    if ((i & 1023) == 1023)  /* this may take a while */
	    {
		/*
		 * This message should never cause a hit-return message.
		 * Overwrite this message with any next message.
		 */
		++no_wait_return;
		smsg(_("freeing %ld lines"), i + 1);
		--no_wait_return;
		msg_didout = FALSE;
		msg_col = 0;
	    }
#endif
	    vim_free(y_current->y_array[i]);
	}
	VIM_CLEAR(y_current->y_array);
#ifdef AMIGA
	if (n >= 1000)
	    msg("");
#endif
    }
}

    static void
free_yank_all(void)
{
    free_yank(y_current->y_size);
}

/*
 * Yank the text between "oap->start" and "oap->end" into a yank register.
 * If we are to append (uppercase register), we first yank into a new yank
 * register and then concatenate the old and the new one (so we keep the old
 * one in case of out-of-memory).
 *
 * Return FAIL for failure, OK otherwise.
 */
    int
op_yank(oparg_T *oap, int deleting, int mess)
{
    long		y_idx;		/* index in y_array[] */
    yankreg_T		*curr;		/* copy of y_current */
    yankreg_T		newreg;		/* new yank register when appending */
    char_u		**new_ptr;
    linenr_T		lnum;		/* current line number */
    long		j;
    int			yanktype = oap->motion_type;
    long		yanklines = oap->line_count;
    linenr_T		yankendlnum = oap->end.lnum;
    char_u		*p;
    char_u		*pnew;
    struct block_def	bd;
#if defined(FEAT_CLIPBOARD) && defined(FEAT_X11)
    int			did_star = FALSE;
#endif

				    /* check for read-only register */
    if (oap->regname != 0 && !valid_yank_reg(oap->regname, TRUE))
    {
	beep_flush();
	return FAIL;
    }
    if (oap->regname == '_')	    /* black hole: nothing to do */
	return OK;

#ifdef FEAT_CLIPBOARD
    if (!clip_star.available && oap->regname == '*')
	oap->regname = 0;
    else if (!clip_plus.available && oap->regname == '+')
	oap->regname = 0;
#endif

    if (!deleting)		    /* op_delete() already set y_current */
	get_yank_register(oap->regname, TRUE);

    curr = y_current;
				    /* append to existing contents */
    if (y_append && y_current->y_array != NULL)
	y_current = &newreg;
    else
	free_yank_all();	    /* free previously yanked lines */

    /*
     * If the cursor was in column 1 before and after the movement, and the
     * operator is not inclusive, the yank is always linewise.
     */
    if (       oap->motion_type == MCHAR
	    && oap->start.col == 0
	    && !oap->inclusive
	    && (!oap->is_VIsual || *p_sel == 'o')
	    && !oap->block_mode
	    && oap->end.col == 0
	    && yanklines > 1)
    {
	yanktype = MLINE;
	--yankendlnum;
	--yanklines;
    }

    y_current->y_size = yanklines;
    y_current->y_type = yanktype;   /* set the yank register type */
    y_current->y_width = 0;
    y_current->y_array = (char_u **)lalloc_clear((long_u)(sizeof(char_u *) *
							    yanklines), TRUE);
    if (y_current->y_array == NULL)
    {
	y_current = curr;
	return FAIL;
    }
#ifdef FEAT_VIMINFO
    y_current->y_time_set = vim_time();
#endif

    y_idx = 0;
    lnum = oap->start.lnum;

    if (oap->block_mode)
    {
	/* Visual block mode */
	y_current->y_type = MBLOCK;	    /* set the yank register type */
	y_current->y_width = oap->end_vcol - oap->start_vcol;

	if (curwin->w_curswant == MAXCOL && y_current->y_width > 0)
	    y_current->y_width--;
    }

    for ( ; lnum <= yankendlnum; lnum++, y_idx++)
    {
	switch (y_current->y_type)
	{
	    case MBLOCK:
		block_prep(oap, &bd, lnum, FALSE);
		if (yank_copy_line(&bd, y_idx) == FAIL)
		    goto fail;
		break;

	    case MLINE:
		if ((y_current->y_array[y_idx] =
			    vim_strsave(ml_get(lnum))) == NULL)
		    goto fail;
		break;

	    case MCHAR:
		{
		    colnr_T startcol = 0, endcol = MAXCOL;
		    int is_oneChar = FALSE;
		    colnr_T cs, ce;

		    p = ml_get(lnum);
		    bd.startspaces = 0;
		    bd.endspaces = 0;

		    if (lnum == oap->start.lnum)
		    {
			startcol = oap->start.col;
			if (virtual_op)
			{
			    getvcol(curwin, &oap->start, &cs, NULL, &ce);
			    if (ce != cs && oap->start.coladd > 0)
			    {
				/* Part of a tab selected -- but don't
				 * double-count it. */
				bd.startspaces = (ce - cs + 1)
							  - oap->start.coladd;
				startcol++;
			    }
			}
		    }

		    if (lnum == oap->end.lnum)
		    {
			endcol = oap->end.col;
			if (virtual_op)
			{
			    getvcol(curwin, &oap->end, &cs, NULL, &ce);
			    if (p[endcol] == NUL || (cs + oap->end.coladd < ce
					/* Don't add space for double-wide
					 * char; endcol will be on last byte
					 * of multi-byte char. */
					&& (*mb_head_off)(p, p + endcol) == 0))
			    {
				if (oap->start.lnum == oap->end.lnum
					    && oap->start.col == oap->end.col)
				{
				    /* Special case: inside a single char */
				    is_oneChar = TRUE;
				    bd.startspaces = oap->end.coladd
					 - oap->start.coladd + oap->inclusive;
				    endcol = startcol;
				}
				else
				{
				    bd.endspaces = oap->end.coladd
							     + oap->inclusive;
				    endcol -= oap->inclusive;
				}
			    }
			}
		    }
		    if (endcol == MAXCOL)
			endcol = (colnr_T)STRLEN(p);
		    if (startcol > endcol || is_oneChar)
			bd.textlen = 0;
		    else
			bd.textlen = endcol - startcol + oap->inclusive;
		    bd.textstart = p + startcol;
		    if (yank_copy_line(&bd, y_idx) == FAIL)
			goto fail;
		    break;
		}
		/* NOTREACHED */
	}
    }

    if (curr != y_current)	/* append the new block to the old block */
    {
	new_ptr = (char_u **)lalloc((long_u)(sizeof(char_u *) *
				   (curr->y_size + y_current->y_size)), TRUE);
	if (new_ptr == NULL)
	    goto fail;
	for (j = 0; j < curr->y_size; ++j)
	    new_ptr[j] = curr->y_array[j];
	vim_free(curr->y_array);
	curr->y_array = new_ptr;
#ifdef FEAT_VIMINFO
	curr->y_time_set = vim_time();
#endif

	if (yanktype == MLINE)	/* MLINE overrides MCHAR and MBLOCK */
	    curr->y_type = MLINE;

	/* Concatenate the last line of the old block with the first line of
	 * the new block, unless being Vi compatible. */
	if (curr->y_type == MCHAR && vim_strchr(p_cpo, CPO_REGAPPEND) == NULL)
	{
	    pnew = lalloc((long_u)(STRLEN(curr->y_array[curr->y_size - 1])
			      + STRLEN(y_current->y_array[0]) + 1), TRUE);
	    if (pnew == NULL)
	    {
		y_idx = y_current->y_size - 1;
		goto fail;
	    }
	    STRCPY(pnew, curr->y_array[--j]);
	    STRCAT(pnew, y_current->y_array[0]);
	    vim_free(curr->y_array[j]);
	    vim_free(y_current->y_array[0]);
	    curr->y_array[j++] = pnew;
	    y_idx = 1;
	}
	else
	    y_idx = 0;
	while (y_idx < y_current->y_size)
	    curr->y_array[j++] = y_current->y_array[y_idx++];
	curr->y_size = j;
	vim_free(y_current->y_array);
	y_current = curr;
    }
    if (curwin->w_p_rnu)
	redraw_later(SOME_VALID);	/* cursor moved to start */
    if (mess)			/* Display message about yank? */
    {
	if (yanktype == MCHAR
		&& !oap->block_mode
		&& yanklines == 1)
	    yanklines = 0;
	/* Some versions of Vi use ">=" here, some don't...  */
	if (yanklines > p_report)
	{
	    char namebuf[100];

	    if (oap->regname == NUL)
		*namebuf = NUL;
	    else
		vim_snprintf(namebuf, sizeof(namebuf),
						_(" into \"%c"), oap->regname);

	    /* redisplay now, so message is not deleted */
	    update_topline_redraw();
	    if (oap->block_mode)
	    {
		smsg(NGETTEXT("block of %ld line yanked%s",
				     "block of %ld lines yanked%s", yanklines),
			yanklines, namebuf);
	    }
	    else
	    {
		smsg(NGETTEXT("%ld line yanked%s",
					      "%ld lines yanked%s", yanklines),
			yanklines, namebuf);
	    }
	}
    }

    /*
     * Set "'[" and "']" marks.
     */
    curbuf->b_op_start = oap->start;
    curbuf->b_op_end = oap->end;
    if (yanktype == MLINE && !oap->block_mode)
    {
	curbuf->b_op_start.col = 0;
	curbuf->b_op_end.col = MAXCOL;
    }

#ifdef FEAT_CLIPBOARD
    /*
     * If we were yanking to the '*' register, send result to clipboard.
     * If no register was specified, and "unnamed" in 'clipboard', make a copy
     * to the '*' register.
     */
    if (clip_star.available
	    && (curr == &(y_regs[STAR_REGISTER])
		|| (!deleting && oap->regname == 0
		   && ((clip_unnamed | clip_unnamed_saved) & CLIP_UNNAMED))))
    {
	if (curr != &(y_regs[STAR_REGISTER]))
	    /* Copy the text from register 0 to the clipboard register. */
	    copy_yank_reg(&(y_regs[STAR_REGISTER]));

	clip_own_selection(&clip_star);
	clip_gen_set_selection(&clip_star);
# ifdef FEAT_X11
	did_star = TRUE;
# endif
    }

# ifdef FEAT_X11
    /*
     * If we were yanking to the '+' register, send result to selection.
     * Also copy to the '*' register, in case auto-select is off.
     */
    if (clip_plus.available
	    && (curr == &(y_regs[PLUS_REGISTER])
		|| (!deleting && oap->regname == 0
		  && ((clip_unnamed | clip_unnamed_saved) &
		      CLIP_UNNAMED_PLUS))))
    {
	if (curr != &(y_regs[PLUS_REGISTER]))
	    /* Copy the text from register 0 to the clipboard register. */
	    copy_yank_reg(&(y_regs[PLUS_REGISTER]));

	clip_own_selection(&clip_plus);
	clip_gen_set_selection(&clip_plus);
	if (!clip_isautosel_star() && !clip_isautosel_plus()
		&& !did_star && curr == &(y_regs[PLUS_REGISTER]))
	{
	    copy_yank_reg(&(y_regs[STAR_REGISTER]));
	    clip_own_selection(&clip_star);
	    clip_gen_set_selection(&clip_star);
	}
    }
# endif
#endif

#if defined(FEAT_EVAL)
    if (!deleting && has_textyankpost())
	yank_do_autocmd(oap, y_current);
#endif

    return OK;

fail:		/* free the allocated lines */
    free_yank(y_idx + 1);
    y_current = curr;
    return FAIL;
}

    static int
yank_copy_line(struct block_def *bd, long y_idx)
{
    char_u	*pnew;

    if ((pnew = alloc(bd->startspaces + bd->endspaces + bd->textlen + 1))
								      == NULL)
	return FAIL;
    y_current->y_array[y_idx] = pnew;
    vim_memset(pnew, ' ', (size_t)bd->startspaces);
    pnew += bd->startspaces;
    mch_memmove(pnew, bd->textstart, (size_t)bd->textlen);
    pnew += bd->textlen;
    vim_memset(pnew, ' ', (size_t)bd->endspaces);
    pnew += bd->endspaces;
    *pnew = NUL;
    return OK;
}

#ifdef FEAT_CLIPBOARD
/*
 * Make a copy of the y_current register to register "reg".
 */
    static void
copy_yank_reg(yankreg_T *reg)
{
    yankreg_T	*curr = y_current;
    long	j;

    y_current = reg;
    free_yank_all();
    *y_current = *curr;
    y_current->y_array = (char_u **)lalloc_clear(
			(long_u)(sizeof(char_u *) * y_current->y_size), TRUE);
    if (y_current->y_array == NULL)
	y_current->y_size = 0;
    else
	for (j = 0; j < y_current->y_size; ++j)
	    if ((y_current->y_array[j] = vim_strsave(curr->y_array[j])) == NULL)
	    {
		free_yank(j);
		y_current->y_size = 0;
		break;
	    }
    y_current = curr;
}
#endif

/*
 * Put contents of register "regname" into the text.
 * Caller must check "regname" to be valid!
 * "flags": PUT_FIXINDENT	make indent look nice
 *	    PUT_CURSEND		leave cursor after end of new text
 *	    PUT_LINE		force linewise put (":put")
 */
    void
do_put(
    int		regname,
    int		dir,		/* BACKWARD for 'P', FORWARD for 'p' */
    long	count,
    int		flags)
{
    char_u	*ptr;
    char_u	*newp, *oldp;
    int		yanklen;
    int		totlen = 0;		/* init for gcc */
    linenr_T	lnum;
    colnr_T	col;
    long	i;			/* index in y_array[] */
    int		y_type;
    long	y_size;
    int		oldlen;
    long	y_width = 0;
    colnr_T	vcol;
    int		delcount;
    int		incr = 0;
    long	j;
    struct block_def bd;
    char_u	**y_array = NULL;
    long	nr_lines = 0;
    pos_T	new_cursor;
    int		indent;
    int		orig_indent = 0;	/* init for gcc */
    int		indent_diff = 0;	/* init for gcc */
    int		first_indent = TRUE;
    int		lendiff = 0;
    pos_T	old_pos;
    char_u	*insert_string = NULL;
    int		allocated = FALSE;
    long	cnt;

#ifdef FEAT_CLIPBOARD
    /* Adjust register name for "unnamed" in 'clipboard'. */
    adjust_clip_reg(&regname);
    (void)may_get_selection(regname);
#endif

    if (flags & PUT_FIXINDENT)
	orig_indent = get_indent();

    curbuf->b_op_start = curwin->w_cursor;	/* default for '[ mark */
    curbuf->b_op_end = curwin->w_cursor;	/* default for '] mark */

    /*
     * Using inserted text works differently, because the register includes
     * special characters (newlines, etc.).
     */
    if (regname == '.')
    {
	if (VIsual_active)
	    stuffcharReadbuff(VIsual_mode);
	(void)stuff_inserted((dir == FORWARD ? (count == -1 ? 'o' : 'a') :
				    (count == -1 ? 'O' : 'i')), count, FALSE);
	/* Putting the text is done later, so can't really move the cursor to
	 * the next character.  Use "l" to simulate it. */
	if ((flags & PUT_CURSEND) && gchar_cursor() != NUL)
	    stuffcharReadbuff('l');
	return;
    }

    /*
     * For special registers '%' (file name), '#' (alternate file name) and
     * ':' (last command line), etc. we have to create a fake yank register.
     */
    if (get_spec_reg(regname, &insert_string, &allocated, TRUE))
    {
	if (insert_string == NULL)
	    return;
    }

    /* Autocommands may be executed when saving lines for undo.  This might
     * make "y_array" invalid, so we start undo now to avoid that. */
    if (u_save(curwin->w_cursor.lnum, curwin->w_cursor.lnum + 1) == FAIL)
	goto end;

    if (insert_string != NULL)
    {
	y_type = MCHAR;
#ifdef FEAT_EVAL
	if (regname == '=')
	{
	    /* For the = register we need to split the string at NL
	     * characters.
	     * Loop twice: count the number of lines and save them. */
	    for (;;)
	    {
		y_size = 0;
		ptr = insert_string;
		while (ptr != NULL)
		{
		    if (y_array != NULL)
			y_array[y_size] = ptr;
		    ++y_size;
		    ptr = vim_strchr(ptr, '\n');
		    if (ptr != NULL)
		    {
			if (y_array != NULL)
			    *ptr = NUL;
			++ptr;
			/* A trailing '\n' makes the register linewise. */
			if (*ptr == NUL)
			{
			    y_type = MLINE;
			    break;
			}
		    }
		}
		if (y_array != NULL)
		    break;
		y_array = (char_u **)alloc((unsigned)
						 (y_size * sizeof(char_u *)));
		if (y_array == NULL)
		    goto end;
	    }
	}
	else
#endif
	{
	    y_size = 1;		/* use fake one-line yank register */
	    y_array = &insert_string;
	}
    }
    else
    {
	get_yank_register(regname, FALSE);

	y_type = y_current->y_type;
	y_width = y_current->y_width;
	y_size = y_current->y_size;
	y_array = y_current->y_array;
    }

    if (y_type == MLINE)
    {
	if (flags & PUT_LINE_SPLIT)
	{
	    char_u *p;

	    /* "p" or "P" in Visual mode: split the lines to put the text in
	     * between. */
	    if (u_save_cursor() == FAIL)
		goto end;
	    p = ml_get_cursor();
	    if (dir == FORWARD && *p != NUL)
		MB_PTR_ADV(p);
	    ptr = vim_strsave(p);
	    if (ptr == NULL)
		goto end;
	    ml_append(curwin->w_cursor.lnum, ptr, (colnr_T)0, FALSE);
	    vim_free(ptr);

	    oldp = ml_get_curline();
	    p = oldp + curwin->w_cursor.col;
	    if (dir == FORWARD && *p != NUL)
		MB_PTR_ADV(p);
	    ptr = vim_strnsave(oldp, p - oldp);
	    if (ptr == NULL)
		goto end;
	    ml_replace(curwin->w_cursor.lnum, ptr, FALSE);
	    ++nr_lines;
	    dir = FORWARD;
	}
	if (flags & PUT_LINE_FORWARD)
	{
	    /* Must be "p" for a Visual block, put lines below the block. */
	    curwin->w_cursor = curbuf->b_visual.vi_end;
	    dir = FORWARD;
	}
	curbuf->b_op_start = curwin->w_cursor;	/* default for '[ mark */
	curbuf->b_op_end = curwin->w_cursor;	/* default for '] mark */
    }

    if (flags & PUT_LINE)	/* :put command or "p" in Visual line mode. */
	y_type = MLINE;

    if (y_size == 0 || y_array == NULL)
    {
	semsg(_("E353: Nothing in register %s"),
		  regname == 0 ? (char_u *)"\"" : transchar(regname));
	goto end;
    }

    if (y_type == MBLOCK)
    {
	lnum = curwin->w_cursor.lnum + y_size + 1;
	if (lnum > curbuf->b_ml.ml_line_count)
	    lnum = curbuf->b_ml.ml_line_count + 1;
	if (u_save(curwin->w_cursor.lnum - 1, lnum) == FAIL)
	    goto end;
    }
    else if (y_type == MLINE)
    {
	lnum = curwin->w_cursor.lnum;
#ifdef FEAT_FOLDING
	/* Correct line number for closed fold.  Don't move the cursor yet,
	 * u_save() uses it. */
	if (dir == BACKWARD)
	    (void)hasFolding(lnum, &lnum, NULL);
	else
	    (void)hasFolding(lnum, NULL, &lnum);
#endif
	if (dir == FORWARD)
	    ++lnum;
	/* In an empty buffer the empty line is going to be replaced, include
	 * it in the saved lines. */
	if ((BUFEMPTY() ? u_save(0, 2) : u_save(lnum - 1, lnum)) == FAIL)
	    goto end;
#ifdef FEAT_FOLDING
	if (dir == FORWARD)
	    curwin->w_cursor.lnum = lnum - 1;
	else
	    curwin->w_cursor.lnum = lnum;
	curbuf->b_op_start = curwin->w_cursor;	/* for mark_adjust() */
#endif
    }
    else if (u_save_cursor() == FAIL)
	goto end;

    yanklen = (int)STRLEN(y_array[0]);

    if (ve_flags == VE_ALL && y_type == MCHAR)
    {
	if (gchar_cursor() == TAB)
	{
	    /* Don't need to insert spaces when "p" on the last position of a
	     * tab or "P" on the first position. */
#ifdef FEAT_VARTABS
	    int viscol = getviscol();
	    if (dir == FORWARD
		    ? tabstop_padding(viscol, curbuf->b_p_ts,
						    curbuf->b_p_vts_array) != 1
		    : curwin->w_cursor.coladd > 0)
		coladvance_force(viscol);
#else
	    if (dir == FORWARD
		    ? (int)curwin->w_cursor.coladd < curbuf->b_p_ts - 1
						: curwin->w_cursor.coladd > 0)
		coladvance_force(getviscol());
#endif
	    else
		curwin->w_cursor.coladd = 0;
	}
	else if (curwin->w_cursor.coladd > 0 || gchar_cursor() == NUL)
	    coladvance_force(getviscol() + (dir == FORWARD));
    }

    lnum = curwin->w_cursor.lnum;
    col = curwin->w_cursor.col;

    /*
     * Block mode
     */
    if (y_type == MBLOCK)
    {
	int	c = gchar_cursor();
	colnr_T	endcol2 = 0;

	if (dir == FORWARD && c != NUL)
	{
	    if (ve_flags == VE_ALL)
		getvcol(curwin, &curwin->w_cursor, &col, NULL, &endcol2);
	    else
		getvcol(curwin, &curwin->w_cursor, NULL, NULL, &col);

	    if (has_mbyte)
		/* move to start of next multi-byte character */
		curwin->w_cursor.col += (*mb_ptr2len)(ml_get_cursor());
	    else
	    if (c != TAB || ve_flags != VE_ALL)
		++curwin->w_cursor.col;
	    ++col;
	}
	else
	    getvcol(curwin, &curwin->w_cursor, &col, NULL, &endcol2);

	col += curwin->w_cursor.coladd;
	if (ve_flags == VE_ALL
		&& (curwin->w_cursor.coladd > 0
		    || endcol2 == curwin->w_cursor.col))
	{
	    if (dir == FORWARD && c == NUL)
		++col;
	    if (dir != FORWARD && c != NUL)
		++curwin->w_cursor.col;
	    if (c == TAB)
	    {
		if (dir == BACKWARD && curwin->w_cursor.col)
		    curwin->w_cursor.col--;
		if (dir == FORWARD && col - 1 == endcol2)
		    curwin->w_cursor.col++;
	    }
	}
	curwin->w_cursor.coladd = 0;
	bd.textcol = 0;
	for (i = 0; i < y_size; ++i)
	{
	    int spaces;
	    char shortline;

	    bd.startspaces = 0;
	    bd.endspaces = 0;
	    vcol = 0;
	    delcount = 0;

	    /* add a new line */
	    if (curwin->w_cursor.lnum > curbuf->b_ml.ml_line_count)
	    {
		if (ml_append(curbuf->b_ml.ml_line_count, (char_u *)"",
						   (colnr_T)1, FALSE) == FAIL)
		    break;
		++nr_lines;
	    }
	    /* get the old line and advance to the position to insert at */
	    oldp = ml_get_curline();
	    oldlen = (int)STRLEN(oldp);
	    for (ptr = oldp; vcol < col && *ptr; )
	    {
		/* Count a tab for what it's worth (if list mode not on) */
		incr = lbr_chartabsize_adv(oldp, &ptr, (colnr_T)vcol);
		vcol += incr;
	    }
	    bd.textcol = (colnr_T)(ptr - oldp);

	    shortline = (vcol < col) || (vcol == col && !*ptr) ;

	    if (vcol < col) /* line too short, padd with spaces */
		bd.startspaces = col - vcol;
	    else if (vcol > col)
	    {
		bd.endspaces = vcol - col;
		bd.startspaces = incr - bd.endspaces;
		--bd.textcol;
		delcount = 1;
		if (has_mbyte)
		    bd.textcol -= (*mb_head_off)(oldp, oldp + bd.textcol);
		if (oldp[bd.textcol] != TAB)
		{
		    /* Only a Tab can be split into spaces.  Other
		     * characters will have to be moved to after the
		     * block, causing misalignment. */
		    delcount = 0;
		    bd.endspaces = 0;
		}
	    }

	    yanklen = (int)STRLEN(y_array[i]);

	    /* calculate number of spaces required to fill right side of block*/
	    spaces = y_width + 1;
	    for (j = 0; j < yanklen; j++)
		spaces -= lbr_chartabsize(NULL, &y_array[i][j], 0);
	    if (spaces < 0)
		spaces = 0;

	    /* insert the new text */
	    totlen = count * (yanklen + spaces) + bd.startspaces + bd.endspaces;
	    newp = alloc_check((unsigned)totlen + oldlen + 1);
	    if (newp == NULL)
		break;
	    /* copy part up to cursor to new line */
	    ptr = newp;
	    mch_memmove(ptr, oldp, (size_t)bd.textcol);
	    ptr += bd.textcol;
	    /* may insert some spaces before the new text */
	    vim_memset(ptr, ' ', (size_t)bd.startspaces);
	    ptr += bd.startspaces;
	    /* insert the new text */
	    for (j = 0; j < count; ++j)
	    {
		mch_memmove(ptr, y_array[i], (size_t)yanklen);
		ptr += yanklen;

		/* insert block's trailing spaces only if there's text behind */
		if ((j < count - 1 || !shortline) && spaces)
		{
		    vim_memset(ptr, ' ', (size_t)spaces);
		    ptr += spaces;
		}
	    }
	    /* may insert some spaces after the new text */
	    vim_memset(ptr, ' ', (size_t)bd.endspaces);
	    ptr += bd.endspaces;
	    /* move the text after the cursor to the end of the line. */
	    mch_memmove(ptr, oldp + bd.textcol + delcount,
				(size_t)(oldlen - bd.textcol - delcount + 1));
	    ml_replace(curwin->w_cursor.lnum, newp, FALSE);

	    ++curwin->w_cursor.lnum;
	    if (i == 0)
		curwin->w_cursor.col += bd.startspaces;
	}

	changed_lines(lnum, 0, curwin->w_cursor.lnum, nr_lines);

	/* Set '[ mark. */
	curbuf->b_op_start = curwin->w_cursor;
	curbuf->b_op_start.lnum = lnum;

	/* adjust '] mark */
	curbuf->b_op_end.lnum = curwin->w_cursor.lnum - 1;
	curbuf->b_op_end.col = bd.textcol + totlen - 1;
	curbuf->b_op_end.coladd = 0;
	if (flags & PUT_CURSEND)
	{
	    colnr_T len;

	    curwin->w_cursor = curbuf->b_op_end;
	    curwin->w_cursor.col++;

	    /* in Insert mode we might be after the NUL, correct for that */
	    len = (colnr_T)STRLEN(ml_get_curline());
	    if (curwin->w_cursor.col > len)
		curwin->w_cursor.col = len;
	}
	else
	    curwin->w_cursor.lnum = lnum;
    }
    else
    {
	/*
	 * Character or Line mode
	 */
	if (y_type == MCHAR)
	{
	    /* if type is MCHAR, FORWARD is the same as BACKWARD on the next
	     * char */
	    if (dir == FORWARD && gchar_cursor() != NUL)
	    {
		if (has_mbyte)
		{
		    int bytelen = (*mb_ptr2len)(ml_get_cursor());

		    /* put it on the next of the multi-byte character. */
		    col += bytelen;
		    if (yanklen)
		    {
			curwin->w_cursor.col += bytelen;
			curbuf->b_op_end.col += bytelen;
		    }
		}
		else
		{
		    ++col;
		    if (yanklen)
		    {
			++curwin->w_cursor.col;
			++curbuf->b_op_end.col;
		    }
		}
	    }
	    curbuf->b_op_start = curwin->w_cursor;
	}
	/*
	 * Line mode: BACKWARD is the same as FORWARD on the previous line
	 */
	else if (dir == BACKWARD)
	    --lnum;
	new_cursor = curwin->w_cursor;

	/*
	 * simple case: insert into current line
	 */
	if (y_type == MCHAR && y_size == 1)
	{
	    linenr_T end_lnum = 0; /* init for gcc */

	    if (VIsual_active)
	    {
		end_lnum = curbuf->b_visual.vi_end.lnum;
		if (end_lnum < curbuf->b_visual.vi_start.lnum)
		    end_lnum = curbuf->b_visual.vi_start.lnum;
	    }

	    do {
		totlen = count * yanklen;
		if (totlen > 0)
		{
		    oldp = ml_get(lnum);
		    if (VIsual_active && col > (int)STRLEN(oldp))
		    {
			lnum++;
			continue;
		    }
		    newp = alloc_check((unsigned)(STRLEN(oldp) + totlen + 1));
		    if (newp == NULL)
			goto end;	/* alloc() gave an error message */
		    mch_memmove(newp, oldp, (size_t)col);
		    ptr = newp + col;
		    for (i = 0; i < count; ++i)
		    {
			mch_memmove(ptr, y_array[0], (size_t)yanklen);
			ptr += yanklen;
		    }
		    STRMOVE(ptr, oldp + col);
		    ml_replace(lnum, newp, FALSE);
		    /* Place cursor on last putted char. */
		    if (lnum == curwin->w_cursor.lnum)
		    {
			/* make sure curwin->w_virtcol is updated */
			changed_cline_bef_curs();
			curwin->w_cursor.col += (colnr_T)(totlen - 1);
		    }
		}
		if (VIsual_active)
		    lnum++;
	    } while (VIsual_active && lnum <= end_lnum);

	    if (VIsual_active) /* reset lnum to the last visual line */
		lnum--;

	    curbuf->b_op_end = curwin->w_cursor;
	    /* For "CTRL-O p" in Insert mode, put cursor after last char */
	    if (totlen && (restart_edit != 0 || (flags & PUT_CURSEND)))
		++curwin->w_cursor.col;
	    changed_bytes(lnum, col);
	}
	else
	{
	    /*
	     * Insert at least one line.  When y_type is MCHAR, break the first
	     * line in two.
	     */
	    for (cnt = 1; cnt <= count; ++cnt)
	    {
		i = 0;
		if (y_type == MCHAR)
		{
		    /*
		     * Split the current line in two at the insert position.
		     * First insert y_array[size - 1] in front of second line.
		     * Then append y_array[0] to first line.
		     */
		    lnum = new_cursor.lnum;
		    ptr = ml_get(lnum) + col;
		    totlen = (int)STRLEN(y_array[y_size - 1]);
		    newp = alloc_check((unsigned)(STRLEN(ptr) + totlen + 1));
		    if (newp == NULL)
			goto error;
		    STRCPY(newp, y_array[y_size - 1]);
		    STRCAT(newp, ptr);
		    /* insert second line */
		    ml_append(lnum, newp, (colnr_T)0, FALSE);
		    vim_free(newp);

		    oldp = ml_get(lnum);
		    newp = alloc_check((unsigned)(col + yanklen + 1));
		    if (newp == NULL)
			goto error;
					    /* copy first part of line */
		    mch_memmove(newp, oldp, (size_t)col);
					    /* append to first line */
		    mch_memmove(newp + col, y_array[0], (size_t)(yanklen + 1));
		    ml_replace(lnum, newp, FALSE);

		    curwin->w_cursor.lnum = lnum;
		    i = 1;
		}

		for (; i < y_size; ++i)
		{
		    if ((y_type != MCHAR || i < y_size - 1)
			    && ml_append(lnum, y_array[i], (colnr_T)0, FALSE)
								      == FAIL)
			    goto error;
		    lnum++;
		    ++nr_lines;
		    if (flags & PUT_FIXINDENT)
		    {
			old_pos = curwin->w_cursor;
			curwin->w_cursor.lnum = lnum;
			ptr = ml_get(lnum);
			if (cnt == count && i == y_size - 1)
			    lendiff = (int)STRLEN(ptr);
#if defined(FEAT_SMARTINDENT) || defined(FEAT_CINDENT)
			if (*ptr == '#' && preprocs_left())
			    indent = 0;     /* Leave # lines at start */
			else
#endif
			     if (*ptr == NUL)
			    indent = 0;     /* Ignore empty lines */
			else if (first_indent)
			{
			    indent_diff = orig_indent - get_indent();
			    indent = orig_indent;
			    first_indent = FALSE;
			}
			else if ((indent = get_indent() + indent_diff) < 0)
			    indent = 0;
			(void)set_indent(indent, 0);
			curwin->w_cursor = old_pos;
			/* remember how many chars were removed */
			if (cnt == count && i == y_size - 1)
			    lendiff -= (int)STRLEN(ml_get(lnum));
		    }
		}
	    }

error:
	    /* Adjust marks. */
	    if (y_type == MLINE)
	    {
		curbuf->b_op_start.col = 0;
		if (dir == FORWARD)
		    curbuf->b_op_start.lnum++;
	    }
	    /* Skip mark_adjust when adding lines after the last one, there
	     * can't be marks there. But still needed in diff mode. */
	    if (curbuf->b_op_start.lnum + (y_type == MCHAR) - 1 + nr_lines
						 < curbuf->b_ml.ml_line_count
#ifdef FEAT_DIFF
						 || curwin->w_p_diff
#endif
						 )
		mark_adjust(curbuf->b_op_start.lnum + (y_type == MCHAR),
					     (linenr_T)MAXLNUM, nr_lines, 0L);

	    /* note changed text for displaying and folding */
	    if (y_type == MCHAR)
		changed_lines(curwin->w_cursor.lnum, col,
					 curwin->w_cursor.lnum + 1, nr_lines);
	    else
		changed_lines(curbuf->b_op_start.lnum, 0,
					   curbuf->b_op_start.lnum, nr_lines);

	    /* put '] mark at last inserted character */
	    curbuf->b_op_end.lnum = lnum;
	    /* correct length for change in indent */
	    col = (colnr_T)STRLEN(y_array[y_size - 1]) - lendiff;
	    if (col > 1)
		curbuf->b_op_end.col = col - 1;
	    else
		curbuf->b_op_end.col = 0;

	    if (flags & PUT_CURSLINE)
	    {
		/* ":put": put cursor on last inserted line */
		curwin->w_cursor.lnum = lnum;
		beginline(BL_WHITE | BL_FIX);
	    }
	    else if (flags & PUT_CURSEND)
	    {
		/* put cursor after inserted text */
		if (y_type == MLINE)
		{
		    if (lnum >= curbuf->b_ml.ml_line_count)
			curwin->w_cursor.lnum = curbuf->b_ml.ml_line_count;
		    else
			curwin->w_cursor.lnum = lnum + 1;
		    curwin->w_cursor.col = 0;
		}
		else
		{
		    curwin->w_cursor.lnum = lnum;
		    curwin->w_cursor.col = col;
		}
	    }
	    else if (y_type == MLINE)
	    {
		/* put cursor on first non-blank in first inserted line */
		curwin->w_cursor.col = 0;
		if (dir == FORWARD)
		    ++curwin->w_cursor.lnum;
		beginline(BL_WHITE | BL_FIX);
	    }
	    else	/* put cursor on first inserted character */
		curwin->w_cursor = new_cursor;
	}
    }

    msgmore(nr_lines);
    curwin->w_set_curswant = TRUE;

end:
    if (allocated)
	vim_free(insert_string);
    if (regname == '=')
	vim_free(y_array);

    VIsual_active = FALSE;

    /* If the cursor is past the end of the line put it at the end. */
    adjust_cursor_eol();
}

/*
 * When the cursor is on the NUL past the end of the line and it should not be
 * there move it left.
 */
    void
adjust_cursor_eol(void)
{
    if (curwin->w_cursor.col > 0
	    && gchar_cursor() == NUL
	    && (ve_flags & VE_ONEMORE) == 0
	    && !(restart_edit || (State & INSERT)))
    {
	/* Put the cursor on the last character in the line. */
	dec_cursor();

	if (ve_flags == VE_ALL)
	{
	    colnr_T	    scol, ecol;

	    /* Coladd is set to the width of the last character. */
	    getvcol(curwin, &curwin->w_cursor, &scol, NULL, &ecol);
	    curwin->w_cursor.coladd = ecol - scol + 1;
	}
    }
}

#if defined(FEAT_SMARTINDENT) || defined(FEAT_CINDENT) || defined(PROTO)
/*
 * Return TRUE if lines starting with '#' should be left aligned.
 */
    int
preprocs_left(void)
{
    return
# ifdef FEAT_SMARTINDENT
#  ifdef FEAT_CINDENT
	(curbuf->b_p_si && !curbuf->b_p_cin) ||
#  else
	curbuf->b_p_si
#  endif
# endif
# ifdef FEAT_CINDENT
	(curbuf->b_p_cin && in_cinkeys('#', ' ', TRUE)
					   && curbuf->b_ind_hash_comment == 0)
# endif
	;
}
#endif

/*
 * Return the character name of the register with the given number.
 */
    int
get_register_name(int num)
{
    if (num == -1)
	return '"';
    else if (num < 10)
	return num + '0';
    else if (num == DELETION_REGISTER)
	return '-';
#ifdef FEAT_CLIPBOARD
    else if (num == STAR_REGISTER)
	return '*';
    else if (num == PLUS_REGISTER)
	return '+';
#endif
    else
    {
#ifdef EBCDIC
	int i;

	/* EBCDIC is really braindead ... */
	i = 'a' + (num - 10);
	if (i > 'i')
	    i += 7;
	if (i > 'r')
	    i += 8;
	return i;
#else
	return num + 'a' - 10;
#endif
    }
}

/*
 * ":dis" and ":registers": Display the contents of the yank registers.
 */
    void
ex_display(exarg_T *eap)
{
    int		i, n;
    long	j;
    char_u	*p;
    yankreg_T	*yb;
    int		name;
    int		attr;
    char_u	*arg = eap->arg;
    int		clen;

    if (arg != NULL && *arg == NUL)
	arg = NULL;
    attr = HL_ATTR(HLF_8);

    /* Highlight title */
    msg_puts_title(_("\n--- Registers ---"));
    for (i = -1; i < NUM_REGISTERS && !got_int; ++i)
    {
	name = get_register_name(i);
	if (arg != NULL && vim_strchr(arg, name) == NULL
#ifdef ONE_CLIPBOARD
	    /* Star register and plus register contain the same thing. */
		&& (name != '*' || vim_strchr(arg, '+') == NULL)
#endif
		)
	    continue;	    /* did not ask for this register */

#ifdef FEAT_CLIPBOARD
	/* Adjust register name for "unnamed" in 'clipboard'.
	 * When it's a clipboard register, fill it with the current contents
	 * of the clipboard.  */
	adjust_clip_reg(&name);
	(void)may_get_selection(name);
#endif

	if (i == -1)
	{
	    if (y_previous != NULL)
		yb = y_previous;
	    else
		yb = &(y_regs[0]);
	}
	else
	    yb = &(y_regs[i]);

#ifdef FEAT_EVAL
	if (name == MB_TOLOWER(redir_reg)
		|| (redir_reg == '"' && yb == y_previous))
	    continue;	    /* do not list register being written to, the
			     * pointer can be freed */
#endif

	if (yb->y_array != NULL)
	{
	    msg_putchar('\n');
	    msg_putchar('"');
	    msg_putchar(name);
	    msg_puts("   ");

	    n = (int)Columns - 6;
	    for (j = 0; j < yb->y_size && n > 1; ++j)
	    {
		if (j)
		{
		    msg_puts_attr("^J", attr);
		    n -= 2;
		}
		for (p = yb->y_array[j]; *p && (n -= ptr2cells(p)) >= 0; ++p)
		{
		    clen = (*mb_ptr2len)(p);
		    msg_outtrans_len(p, clen);
		    p += clen - 1;
		}
	    }
	    if (n > 1 && yb->y_type == MLINE)
		msg_puts_attr("^J", attr);
	    out_flush();		    /* show one line at a time */
	}
	ui_breakcheck();
    }

    /*
     * display last inserted text
     */
    if ((p = get_last_insert()) != NULL
		 && (arg == NULL || vim_strchr(arg, '.') != NULL) && !got_int)
    {
	msg_puts("\n\".   ");
	dis_msg(p, TRUE);
    }

    /*
     * display last command line
     */
    if (last_cmdline != NULL && (arg == NULL || vim_strchr(arg, ':') != NULL)
								  && !got_int)
    {
	msg_puts("\n\":   ");
	dis_msg(last_cmdline, FALSE);
    }

    /*
     * display current file name
     */
    if (curbuf->b_fname != NULL
	    && (arg == NULL || vim_strchr(arg, '%') != NULL) && !got_int)
    {
	msg_puts("\n\"%   ");
	dis_msg(curbuf->b_fname, FALSE);
    }

    /*
     * display alternate file name
     */
    if ((arg == NULL || vim_strchr(arg, '%') != NULL) && !got_int)
    {
	char_u	    *fname;
	linenr_T    dummy;

	if (buflist_name_nr(0, &fname, &dummy) != FAIL)
	{
	    msg_puts("\n\"#   ");
	    dis_msg(fname, FALSE);
	}
    }

    /*
     * display last search pattern
     */
    if (last_search_pat() != NULL
		 && (arg == NULL || vim_strchr(arg, '/') != NULL) && !got_int)
    {
	msg_puts("\n\"/   ");
	dis_msg(last_search_pat(), FALSE);
    }

#ifdef FEAT_EVAL
    /*
     * display last used expression
     */
    if (expr_line != NULL && (arg == NULL || vim_strchr(arg, '=') != NULL)
								  && !got_int)
    {
	msg_puts("\n\"=   ");
	dis_msg(expr_line, FALSE);
    }
#endif
}

/*
 * display a string for do_dis()
 * truncate at end of screen line
 */
    static void
dis_msg(
    char_u	*p,
    int		skip_esc)	    /* if TRUE, ignore trailing ESC */
{
    int		n;
    int		l;

    n = (int)Columns - 6;
    while (*p != NUL
	    && !(*p == ESC && skip_esc && *(p + 1) == NUL)
	    && (n -= ptr2cells(p)) >= 0)
    {
	if (has_mbyte && (l = (*mb_ptr2len)(p)) > 1)
	{
	    msg_outtrans_len(p, l);
	    p += l;
	}
	else
	    msg_outtrans_len(p++, 1);
    }
    ui_breakcheck();
}

#if defined(FEAT_COMMENTS) || defined(PROTO)
/*
 * If "process" is TRUE and the line begins with a comment leader (possibly
 * after some white space), return a pointer to the text after it. Put a boolean
 * value indicating whether the line ends with an unclosed comment in
 * "is_comment".
 * line - line to be processed,
 * process - if FALSE, will only check whether the line ends with an unclosed
 *	     comment,
 * include_space - whether to also skip space following the comment leader,
 * is_comment - will indicate whether the current line ends with an unclosed
 *		comment.
 */
    char_u *
skip_comment(
    char_u   *line,
    int      process,
    int	     include_space,
    int      *is_comment)
{
    char_u *comment_flags = NULL;
    int    lead_len;
    int    leader_offset = get_last_leader_offset(line, &comment_flags);

    *is_comment = FALSE;
    if (leader_offset != -1)
    {
	/* Let's check whether the line ends with an unclosed comment.
	 * If the last comment leader has COM_END in flags, there's no comment.
	 */
	while (*comment_flags)
	{
	    if (*comment_flags == COM_END
		    || *comment_flags == ':')
		break;
	    ++comment_flags;
	}
	if (*comment_flags != COM_END)
	    *is_comment = TRUE;
    }

    if (process == FALSE)
	return line;

    lead_len = get_leader_len(line, &comment_flags, FALSE, include_space);

    if (lead_len == 0)
	return line;

    /* Find:
     * - COM_END,
     * - colon,
     * whichever comes first.
     */
    while (*comment_flags)
    {
	if (*comment_flags == COM_END
		|| *comment_flags == ':')
	    break;
	++comment_flags;
    }

    /* If we found a colon, it means that we are not processing a line
     * starting with a closing part of a three-part comment. That's good,
     * because we don't want to remove those as this would be annoying.
     */
    if (*comment_flags == ':' || *comment_flags == NUL)
	line += lead_len;

    return line;
}
#endif

/*
 * Join 'count' lines (minimal 2) at cursor position.
 * When "save_undo" is TRUE save lines for undo first.
 * Set "use_formatoptions" to FALSE when e.g. processing backspace and comment
 * leaders should not be removed.
 * When setmark is TRUE, sets the '[ and '] mark, else, the caller is expected
 * to set those marks.
 *
 * return FAIL for failure, OK otherwise
 */
    int
do_join(
    long    count,
    int	    insert_space,
    int	    save_undo,
    int	    use_formatoptions UNUSED,
    int	    setmark)
{
    char_u	*curr = NULL;
    char_u      *curr_start = NULL;
    char_u	*cend;
    char_u	*newp;
    char_u	*spaces;	/* number of spaces inserted before a line */
    int		endcurr1 = NUL;
    int		endcurr2 = NUL;
    int		currsize = 0;	/* size of the current line */
    int		sumsize = 0;	/* size of the long new line */
    linenr_T	t;
    colnr_T	col = 0;
    int		ret = OK;
#if defined(FEAT_COMMENTS) || defined(PROTO)
    int		*comments = NULL;
    int		remove_comments = (use_formatoptions == TRUE)
				  && has_format_option(FO_REMOVE_COMS);
    int		prev_was_comment;
#endif


    if (save_undo && u_save((linenr_T)(curwin->w_cursor.lnum - 1),
			    (linenr_T)(curwin->w_cursor.lnum + count)) == FAIL)
	return FAIL;

    /* Allocate an array to store the number of spaces inserted before each
     * line.  We will use it to pre-compute the length of the new line and the
     * proper placement of each original line in the new one. */
    spaces = lalloc_clear((long_u)count, TRUE);
    if (spaces == NULL)
	return FAIL;
#if defined(FEAT_COMMENTS) || defined(PROTO)
    if (remove_comments)
    {
	comments = (int *)lalloc_clear((long_u)count * sizeof(int), TRUE);
	if (comments == NULL)
	{
	    vim_free(spaces);
	    return FAIL;
	}
    }
#endif

    /*
     * Don't move anything, just compute the final line length
     * and setup the array of space strings lengths
     */
    for (t = 0; t < count; ++t)
    {
	curr = curr_start = ml_get((linenr_T)(curwin->w_cursor.lnum + t));
	if (t == 0 && setmark)
	{
	    /* Set the '[ mark. */
	    curwin->w_buffer->b_op_start.lnum = curwin->w_cursor.lnum;
	    curwin->w_buffer->b_op_start.col  = (colnr_T)STRLEN(curr);
	}
#if defined(FEAT_COMMENTS) || defined(PROTO)
	if (remove_comments)
	{
	    /* We don't want to remove the comment leader if the
	     * previous line is not a comment. */
	    if (t > 0 && prev_was_comment)
	    {

		char_u *new_curr = skip_comment(curr, TRUE, insert_space,
							   &prev_was_comment);
		comments[t] = (int)(new_curr - curr);
		curr = new_curr;
	    }
	    else
		curr = skip_comment(curr, FALSE, insert_space,
							   &prev_was_comment);
	}
#endif

	if (insert_space && t > 0)
	{
	    curr = skipwhite(curr);
	    if (*curr != ')' && currsize != 0 && endcurr1 != TAB
		    && (!has_format_option(FO_MBYTE_JOIN)
			|| (mb_ptr2char(curr) < 0x100 && endcurr1 < 0x100))
		    && (!has_format_option(FO_MBYTE_JOIN2)
			|| mb_ptr2char(curr) < 0x100 || endcurr1 < 0x100)
	       )
	    {
		/* don't add a space if the line is ending in a space */
		if (endcurr1 == ' ')
		    endcurr1 = endcurr2;
		else
		    ++spaces[t];
		/* extra space when 'joinspaces' set and line ends in '.' */
		if (       p_js
			&& (endcurr1 == '.'
			    || (vim_strchr(p_cpo, CPO_JOINSP) == NULL
				&& (endcurr1 == '?' || endcurr1 == '!'))))
		    ++spaces[t];
	    }
	}
	currsize = (int)STRLEN(curr);
	sumsize += currsize + spaces[t];
	endcurr1 = endcurr2 = NUL;
	if (insert_space && currsize > 0)
	{
	    if (has_mbyte)
	    {
		cend = curr + currsize;
		MB_PTR_BACK(curr, cend);
		endcurr1 = (*mb_ptr2char)(cend);
		if (cend > curr)
		{
		    MB_PTR_BACK(curr, cend);
		    endcurr2 = (*mb_ptr2char)(cend);
		}
	    }
	    else
	    {
		endcurr1 = *(curr + currsize - 1);
		if (currsize > 1)
		    endcurr2 = *(curr + currsize - 2);
	    }
	}
	line_breakcheck();
	if (got_int)
	{
	    ret = FAIL;
	    goto theend;
	}
    }

    /* store the column position before last line */
    col = sumsize - currsize - spaces[count - 1];

    /* allocate the space for the new line */
    newp = alloc_check((unsigned)(sumsize + 1));
    cend = newp + sumsize;
    *cend = 0;

    /*
     * Move affected lines to the new long one.
     *
     * Move marks from each deleted line to the joined line, adjusting the
     * column.  This is not Vi compatible, but Vi deletes the marks, thus that
     * should not really be a problem.
     */
    for (t = count - 1; ; --t)
    {
	int spaces_removed;

	cend -= currsize;
	mch_memmove(cend, curr, (size_t)currsize);
	if (spaces[t] > 0)
	{
	    cend -= spaces[t];
	    vim_memset(cend, ' ', (size_t)(spaces[t]));
	}

	// If deleting more spaces than adding, the cursor moves no more than
	// what is added if it is inside these spaces.
	spaces_removed = (curr - curr_start) - spaces[t];

	mark_col_adjust(curwin->w_cursor.lnum + t, (colnr_T)0, (linenr_T)-t,
			 (long)(cend - newp - spaces_removed), spaces_removed);
	if (t == 0)
	    break;
	curr = curr_start = ml_get((linenr_T)(curwin->w_cursor.lnum + t - 1));
#if defined(FEAT_COMMENTS) || defined(PROTO)
	if (remove_comments)
	    curr += comments[t - 1];
#endif
	if (insert_space && t > 1)
	    curr = skipwhite(curr);
	currsize = (int)STRLEN(curr);
    }
    ml_replace(curwin->w_cursor.lnum, newp, FALSE);

    if (setmark)
    {
	/* Set the '] mark. */
	curwin->w_buffer->b_op_end.lnum = curwin->w_cursor.lnum;
	curwin->w_buffer->b_op_end.col  = (colnr_T)STRLEN(newp);
    }

    /* Only report the change in the first line here, del_lines() will report
     * the deleted line. */
    changed_lines(curwin->w_cursor.lnum, currsize,
					       curwin->w_cursor.lnum + 1, 0L);

    /*
     * Delete following lines. To do this we move the cursor there
     * briefly, and then move it back. After del_lines() the cursor may
     * have moved up (last line deleted), so the current lnum is kept in t.
     */
    t = curwin->w_cursor.lnum;
    ++curwin->w_cursor.lnum;
    del_lines(count - 1, FALSE);
    curwin->w_cursor.lnum = t;

    /*
     * Set the cursor column:
     * Vi compatible: use the column of the first join
     * vim:	      use the column of the last join
     */
    curwin->w_cursor.col =
		    (vim_strchr(p_cpo, CPO_JOINCOL) != NULL ? currsize : col);
    check_cursor_col();

    curwin->w_cursor.coladd = 0;
    curwin->w_set_curswant = TRUE;

theend:
    vim_free(spaces);
#if defined(FEAT_COMMENTS) || defined(PROTO)
    if (remove_comments)
	vim_free(comments);
#endif
    return ret;
}

#ifdef FEAT_COMMENTS
/*
 * Return TRUE if the two comment leaders given are the same.  "lnum" is
 * the first line.  White-space is ignored.  Note that the whole of
 * 'leader1' must match 'leader2_len' characters from 'leader2' -- webb
 */
    static int
same_leader(
    linenr_T lnum,
    int	    leader1_len,
    char_u  *leader1_flags,
    int	    leader2_len,
    char_u  *leader2_flags)
{
    int	    idx1 = 0, idx2 = 0;
    char_u  *p;
    char_u  *line1;
    char_u  *line2;

    if (leader1_len == 0)
	return (leader2_len == 0);

    /*
     * If first leader has 'f' flag, the lines can be joined only if the
     * second line does not have a leader.
     * If first leader has 'e' flag, the lines can never be joined.
     * If fist leader has 's' flag, the lines can only be joined if there is
     * some text after it and the second line has the 'm' flag.
     */
    if (leader1_flags != NULL)
    {
	for (p = leader1_flags; *p && *p != ':'; ++p)
	{
	    if (*p == COM_FIRST)
		return (leader2_len == 0);
	    if (*p == COM_END)
		return FALSE;
	    if (*p == COM_START)
	    {
		if (*(ml_get(lnum) + leader1_len) == NUL)
		    return FALSE;
		if (leader2_flags == NULL || leader2_len == 0)
		    return FALSE;
		for (p = leader2_flags; *p && *p != ':'; ++p)
		    if (*p == COM_MIDDLE)
			return TRUE;
		return FALSE;
	    }
	}
    }

    /*
     * Get current line and next line, compare the leaders.
     * The first line has to be saved, only one line can be locked at a time.
     */
    line1 = vim_strsave(ml_get(lnum));
    if (line1 != NULL)
    {
	for (idx1 = 0; VIM_ISWHITE(line1[idx1]); ++idx1)
	    ;
	line2 = ml_get(lnum + 1);
	for (idx2 = 0; idx2 < leader2_len; ++idx2)
	{
	    if (!VIM_ISWHITE(line2[idx2]))
	    {
		if (line1[idx1++] != line2[idx2])
		    break;
	    }
	    else
		while (VIM_ISWHITE(line1[idx1]))
		    ++idx1;
	}
	vim_free(line1);
    }
    return (idx2 == leader2_len && idx1 == leader1_len);
}
#endif

/*
 * Implementation of the format operator 'gq'.
 */
    void
op_format(
    oparg_T	*oap,
    int		keep_cursor)		/* keep cursor on same text char */
{
    long	old_line_count = curbuf->b_ml.ml_line_count;

    /* Place the cursor where the "gq" or "gw" command was given, so that "u"
     * can put it back there. */
    curwin->w_cursor = oap->cursor_start;

    if (u_save((linenr_T)(oap->start.lnum - 1),
				       (linenr_T)(oap->end.lnum + 1)) == FAIL)
	return;
    curwin->w_cursor = oap->start;

    if (oap->is_VIsual)
	/* When there is no change: need to remove the Visual selection */
	redraw_curbuf_later(INVERTED);

    /* Set '[ mark at the start of the formatted area */
    curbuf->b_op_start = oap->start;

    /* For "gw" remember the cursor position and put it back below (adjusted
     * for joined and split lines). */
    if (keep_cursor)
	saved_cursor = oap->cursor_start;

    format_lines(oap->line_count, keep_cursor);

    /*
     * Leave the cursor at the first non-blank of the last formatted line.
     * If the cursor was moved one line back (e.g. with "Q}") go to the next
     * line, so "." will do the next lines.
     */
    if (oap->end_adjusted && curwin->w_cursor.lnum < curbuf->b_ml.ml_line_count)
	++curwin->w_cursor.lnum;
    beginline(BL_WHITE | BL_FIX);
    old_line_count = curbuf->b_ml.ml_line_count - old_line_count;
    msgmore(old_line_count);

    /* put '] mark on the end of the formatted area */
    curbuf->b_op_end = curwin->w_cursor;

    if (keep_cursor)
    {
	curwin->w_cursor = saved_cursor;
	saved_cursor.lnum = 0;
    }

    if (oap->is_VIsual)
    {
	win_T	*wp;

	FOR_ALL_WINDOWS(wp)
	{
	    if (wp->w_old_cursor_lnum != 0)
	    {
		/* When lines have been inserted or deleted, adjust the end of
		 * the Visual area to be redrawn. */
		if (wp->w_old_cursor_lnum > wp->w_old_visual_lnum)
		    wp->w_old_cursor_lnum += old_line_count;
		else
		    wp->w_old_visual_lnum += old_line_count;
	    }
	}
    }
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Implementation of the format operator 'gq' for when using 'formatexpr'.
 */
    void
op_formatexpr(oparg_T *oap)
{
    if (oap->is_VIsual)
	/* When there is no change: need to remove the Visual selection */
	redraw_curbuf_later(INVERTED);

    if (fex_format(oap->start.lnum, oap->line_count, NUL) != 0)
	/* As documented: when 'formatexpr' returns non-zero fall back to
	 * internal formatting. */
	op_format(oap, FALSE);
}

    int
fex_format(
    linenr_T	lnum,
    long	count,
    int		c)	/* character to be inserted */
{
    int		use_sandbox = was_set_insecurely((char_u *)"formatexpr",
								   OPT_LOCAL);
    int		r;
    char_u	*fex;

    /*
     * Set v:lnum to the first line number and v:count to the number of lines.
     * Set v:char to the character to be inserted (can be NUL).
     */
    set_vim_var_nr(VV_LNUM, lnum);
    set_vim_var_nr(VV_COUNT, count);
    set_vim_var_char(c);

    /* Make a copy, the option could be changed while calling it. */
    fex = vim_strsave(curbuf->b_p_fex);
    if (fex == NULL)
	return 0;

    /*
     * Evaluate the function.
     */
    if (use_sandbox)
	++sandbox;
    r = (int)eval_to_number(fex);
    if (use_sandbox)
	--sandbox;

    set_vim_var_string(VV_CHAR, NULL, -1);
    vim_free(fex);

    return r;
}
#endif

/*
 * Format "line_count" lines, starting at the cursor position.
 * When "line_count" is negative, format until the end of the paragraph.
 * Lines after the cursor line are saved for undo, caller must have saved the
 * first line.
 */
    void
format_lines(
    linenr_T	line_count,
    int		avoid_fex)		/* don't use 'formatexpr' */
{
    int		max_len;
    int		is_not_par;		/* current line not part of parag. */
    int		next_is_not_par;	/* next line not part of paragraph */
    int		is_end_par;		/* at end of paragraph */
    int		prev_is_end_par = FALSE;/* prev. line not part of parag. */
    int		next_is_start_par = FALSE;
#ifdef FEAT_COMMENTS
    int		leader_len = 0;		/* leader len of current line */
    int		next_leader_len;	/* leader len of next line */
    char_u	*leader_flags = NULL;	/* flags for leader of current line */
    char_u	*next_leader_flags;	/* flags for leader of next line */
    int		do_comments;		/* format comments */
    int		do_comments_list = 0;	/* format comments with 'n' or '2' */
#endif
    int		advance = TRUE;
    int		second_indent = -1;	/* indent for second line (comment
					 * aware) */
    int		do_second_indent;
    int		do_number_indent;
    int		do_trail_white;
    int		first_par_line = TRUE;
    int		smd_save;
    long	count;
    int		need_set_indent = TRUE;	/* set indent of next paragraph */
    int		force_format = FALSE;
    int		old_State = State;

    /* length of a line to force formatting: 3 * 'tw' */
    max_len = comp_textwidth(TRUE) * 3;

    /* check for 'q', '2' and '1' in 'formatoptions' */
#ifdef FEAT_COMMENTS
    do_comments = has_format_option(FO_Q_COMS);
#endif
    do_second_indent = has_format_option(FO_Q_SECOND);
    do_number_indent = has_format_option(FO_Q_NUMBER);
    do_trail_white = has_format_option(FO_WHITE_PAR);

    /*
     * Get info about the previous and current line.
     */
    if (curwin->w_cursor.lnum > 1)
	is_not_par = fmt_check_par(curwin->w_cursor.lnum - 1
#ifdef FEAT_COMMENTS
				, &leader_len, &leader_flags, do_comments
#endif
				);
    else
	is_not_par = TRUE;
    next_is_not_par = fmt_check_par(curwin->w_cursor.lnum
#ifdef FEAT_COMMENTS
			   , &next_leader_len, &next_leader_flags, do_comments
#endif
				);
    is_end_par = (is_not_par || next_is_not_par);
    if (!is_end_par && do_trail_white)
	is_end_par = !ends_in_white(curwin->w_cursor.lnum - 1);

    curwin->w_cursor.lnum--;
    for (count = line_count; count != 0 && !got_int; --count)
    {
	/*
	 * Advance to next paragraph.
	 */
	if (advance)
	{
	    curwin->w_cursor.lnum++;
	    prev_is_end_par = is_end_par;
	    is_not_par = next_is_not_par;
#ifdef FEAT_COMMENTS
	    leader_len = next_leader_len;
	    leader_flags = next_leader_flags;
#endif
	}

	/*
	 * The last line to be formatted.
	 */
	if (count == 1 || curwin->w_cursor.lnum == curbuf->b_ml.ml_line_count)
	{
	    next_is_not_par = TRUE;
#ifdef FEAT_COMMENTS
	    next_leader_len = 0;
	    next_leader_flags = NULL;
#endif
	}
	else
	{
	    next_is_not_par = fmt_check_par(curwin->w_cursor.lnum + 1
#ifdef FEAT_COMMENTS
			   , &next_leader_len, &next_leader_flags, do_comments
#endif
					);
	    if (do_number_indent)
		next_is_start_par =
			   (get_number_indent(curwin->w_cursor.lnum + 1) > 0);
	}
	advance = TRUE;
	is_end_par = (is_not_par || next_is_not_par || next_is_start_par);
	if (!is_end_par && do_trail_white)
	    is_end_par = !ends_in_white(curwin->w_cursor.lnum);

	/*
	 * Skip lines that are not in a paragraph.
	 */
	if (is_not_par)
	{
	    if (line_count < 0)
		break;
	}
	else
	{
	    /*
	     * For the first line of a paragraph, check indent of second line.
	     * Don't do this for comments and empty lines.
	     */
	    if (first_par_line
		    && (do_second_indent || do_number_indent)
		    && prev_is_end_par
		    && curwin->w_cursor.lnum < curbuf->b_ml.ml_line_count)
	    {
		if (do_second_indent && !LINEEMPTY(curwin->w_cursor.lnum + 1))
		{
#ifdef FEAT_COMMENTS
		    if (leader_len == 0 && next_leader_len == 0)
		    {
			/* no comment found */
#endif
			second_indent =
				   get_indent_lnum(curwin->w_cursor.lnum + 1);
#ifdef FEAT_COMMENTS
		    }
		    else
		    {
			second_indent = next_leader_len;
			do_comments_list = 1;
		    }
#endif
		}
		else if (do_number_indent)
		{
#ifdef FEAT_COMMENTS
		    if (leader_len == 0 && next_leader_len == 0)
		    {
			/* no comment found */
#endif
			second_indent =
				     get_number_indent(curwin->w_cursor.lnum);
#ifdef FEAT_COMMENTS
		    }
		    else
		    {
			/* get_number_indent() is now "comment aware"... */
			second_indent =
				     get_number_indent(curwin->w_cursor.lnum);
			do_comments_list = 1;
		    }
#endif
		}
	    }

	    /*
	     * When the comment leader changes, it's the end of the paragraph.
	     */
	    if (curwin->w_cursor.lnum >= curbuf->b_ml.ml_line_count
#ifdef FEAT_COMMENTS
		    || !same_leader(curwin->w_cursor.lnum,
					leader_len, leader_flags,
					  next_leader_len, next_leader_flags)
#endif
		    )
		is_end_par = TRUE;

	    /*
	     * If we have got to the end of a paragraph, or the line is
	     * getting long, format it.
	     */
	    if (is_end_par || force_format)
	    {
		if (need_set_indent)
		    /* replace indent in first line with minimal number of
		     * tabs and spaces, according to current options */
		    (void)set_indent(get_indent(), SIN_CHANGED);

		/* put cursor on last non-space */
		State = NORMAL;	/* don't go past end-of-line */
		coladvance((colnr_T)MAXCOL);
		while (curwin->w_cursor.col && vim_isspace(gchar_cursor()))
		    dec_cursor();

		/* do the formatting, without 'showmode' */
		State = INSERT;	/* for open_line() */
		smd_save = p_smd;
		p_smd = FALSE;
		insertchar(NUL, INSCHAR_FORMAT
#ifdef FEAT_COMMENTS
			+ (do_comments ? INSCHAR_DO_COM : 0)
			+ (do_comments && do_comments_list
						       ? INSCHAR_COM_LIST : 0)
#endif
			+ (avoid_fex ? INSCHAR_NO_FEX : 0), second_indent);
		State = old_State;
		p_smd = smd_save;
		second_indent = -1;
		/* at end of par.: need to set indent of next par. */
		need_set_indent = is_end_par;
		if (is_end_par)
		{
		    /* When called with a negative line count, break at the
		     * end of the paragraph. */
		    if (line_count < 0)
			break;
		    first_par_line = TRUE;
		}
		force_format = FALSE;
	    }

	    /*
	     * When still in same paragraph, join the lines together.  But
	     * first delete the leader from the second line.
	     */
	    if (!is_end_par)
	    {
		advance = FALSE;
		curwin->w_cursor.lnum++;
		curwin->w_cursor.col = 0;
		if (line_count < 0 && u_save_cursor() == FAIL)
		    break;
#ifdef FEAT_COMMENTS
		if (next_leader_len > 0)
		{
		    (void)del_bytes((long)next_leader_len, FALSE, FALSE);
		    mark_col_adjust(curwin->w_cursor.lnum, (colnr_T)0, 0L,
						      (long)-next_leader_len, 0);
		} else
#endif
		    if (second_indent > 0)  /* the "leader" for FO_Q_SECOND */
		{
		    int indent = getwhitecols_curline();

		    if (indent > 0)
		    {
			(void)del_bytes(indent, FALSE, FALSE);
			mark_col_adjust(curwin->w_cursor.lnum,
					       (colnr_T)0, 0L, (long)-indent, 0);
		    }
		}
		curwin->w_cursor.lnum--;
		if (do_join(2, TRUE, FALSE, FALSE, FALSE) == FAIL)
		{
		    beep_flush();
		    break;
		}
		first_par_line = FALSE;
		/* If the line is getting long, format it next time */
		if (STRLEN(ml_get_curline()) > (size_t)max_len)
		    force_format = TRUE;
		else
		    force_format = FALSE;
	    }
	}
	line_breakcheck();
    }
}

/*
 * Return TRUE if line "lnum" ends in a white character.
 */
    static int
ends_in_white(linenr_T lnum)
{
    char_u	*s = ml_get(lnum);
    size_t	l;

    if (*s == NUL)
	return FALSE;
    /* Don't use STRLEN() inside VIM_ISWHITE(), SAS/C complains: "macro
     * invocation may call function multiple times". */
    l = STRLEN(s) - 1;
    return VIM_ISWHITE(s[l]);
}

/*
 * Blank lines, and lines containing only the comment leader, are left
 * untouched by the formatting.  The function returns TRUE in this
 * case.  It also returns TRUE when a line starts with the end of a comment
 * ('e' in comment flags), so that this line is skipped, and not joined to the
 * previous line.  A new paragraph starts after a blank line, or when the
 * comment leader changes -- webb.
 */
#ifdef FEAT_COMMENTS
    static int
fmt_check_par(
    linenr_T	lnum,
    int		*leader_len,
    char_u	**leader_flags,
    int		do_comments)
{
    char_u	*flags = NULL;	    /* init for GCC */
    char_u	*ptr;

    ptr = ml_get(lnum);
    if (do_comments)
	*leader_len = get_leader_len(ptr, leader_flags, FALSE, TRUE);
    else
	*leader_len = 0;

    if (*leader_len > 0)
    {
	/*
	 * Search for 'e' flag in comment leader flags.
	 */
	flags = *leader_flags;
	while (*flags && *flags != ':' && *flags != COM_END)
	    ++flags;
    }

    return (*skipwhite(ptr + *leader_len) == NUL
	    || (*leader_len > 0 && *flags == COM_END)
	    || startPS(lnum, NUL, FALSE));
}
#else
    static int
fmt_check_par(linenr_T lnum)
{
    return (*skipwhite(ml_get(lnum)) == NUL || startPS(lnum, NUL, FALSE));
}
#endif

/*
 * Return TRUE when a paragraph starts in line "lnum".  Return FALSE when the
 * previous line is in the same paragraph.  Used for auto-formatting.
 */
    int
paragraph_start(linenr_T lnum)
{
    char_u	*p;
#ifdef FEAT_COMMENTS
    int		leader_len = 0;		/* leader len of current line */
    char_u	*leader_flags = NULL;	/* flags for leader of current line */
    int		next_leader_len;	/* leader len of next line */
    char_u	*next_leader_flags;	/* flags for leader of next line */
    int		do_comments;		/* format comments */
#endif

    if (lnum <= 1)
	return TRUE;		/* start of the file */

    p = ml_get(lnum - 1);
    if (*p == NUL)
	return TRUE;		/* after empty line */

#ifdef FEAT_COMMENTS
    do_comments = has_format_option(FO_Q_COMS);
#endif
    if (fmt_check_par(lnum - 1
#ifdef FEAT_COMMENTS
				, &leader_len, &leader_flags, do_comments
#endif
		))
	return TRUE;		/* after non-paragraph line */

    if (fmt_check_par(lnum
#ifdef FEAT_COMMENTS
			   , &next_leader_len, &next_leader_flags, do_comments
#endif
		))
	return TRUE;		/* "lnum" is not a paragraph line */

    if (has_format_option(FO_WHITE_PAR) && !ends_in_white(lnum - 1))
	return TRUE;		/* missing trailing space in previous line. */

    if (has_format_option(FO_Q_NUMBER) && (get_number_indent(lnum) > 0))
	return TRUE;		/* numbered item starts in "lnum". */

#ifdef FEAT_COMMENTS
    if (!same_leader(lnum - 1, leader_len, leader_flags,
					  next_leader_len, next_leader_flags))
	return TRUE;		/* change of comment leader. */
#endif

    return FALSE;
}

/*
 * prepare a few things for block mode yank/delete/tilde
 *
 * for delete:
 * - textlen includes the first/last char to be (partly) deleted
 * - start/endspaces is the number of columns that are taken by the
 *   first/last deleted char minus the number of columns that have to be
 *   deleted.
 * for yank and tilde:
 * - textlen includes the first/last char to be wholly yanked
 * - start/endspaces is the number of columns of the first/last yanked char
 *   that are to be yanked.
 */
    static void
block_prep(
    oparg_T		*oap,
    struct block_def	*bdp,
    linenr_T		lnum,
    int			is_del)
{
    int		incr = 0;
    char_u	*pend;
    char_u	*pstart;
    char_u	*line;
    char_u	*prev_pstart;
    char_u	*prev_pend;

    bdp->startspaces = 0;
    bdp->endspaces = 0;
    bdp->textlen = 0;
    bdp->start_vcol = 0;
    bdp->end_vcol = 0;
    bdp->is_short = FALSE;
    bdp->is_oneChar = FALSE;
    bdp->pre_whitesp = 0;
    bdp->pre_whitesp_c = 0;
    bdp->end_char_vcols = 0;
    bdp->start_char_vcols = 0;

    line = ml_get(lnum);
    pstart = line;
    prev_pstart = line;
    while (bdp->start_vcol < oap->start_vcol && *pstart)
    {
	/* Count a tab for what it's worth (if list mode not on) */
	incr = lbr_chartabsize(line, pstart, (colnr_T)bdp->start_vcol);
	bdp->start_vcol += incr;
	if (VIM_ISWHITE(*pstart))
	{
	    bdp->pre_whitesp += incr;
	    bdp->pre_whitesp_c++;
	}
	else
	{
	    bdp->pre_whitesp = 0;
	    bdp->pre_whitesp_c = 0;
	}
	prev_pstart = pstart;
	MB_PTR_ADV(pstart);
    }
    bdp->start_char_vcols = incr;
    if (bdp->start_vcol < oap->start_vcol)	/* line too short */
    {
	bdp->end_vcol = bdp->start_vcol;
	bdp->is_short = TRUE;
	if (!is_del || oap->op_type == OP_APPEND)
	    bdp->endspaces = oap->end_vcol - oap->start_vcol + 1;
    }
    else
    {
	/* notice: this converts partly selected Multibyte characters to
	 * spaces, too. */
	bdp->startspaces = bdp->start_vcol - oap->start_vcol;
	if (is_del && bdp->startspaces)
	    bdp->startspaces = bdp->start_char_vcols - bdp->startspaces;
	pend = pstart;
	bdp->end_vcol = bdp->start_vcol;
	if (bdp->end_vcol > oap->end_vcol)	/* it's all in one character */
	{
	    bdp->is_oneChar = TRUE;
	    if (oap->op_type == OP_INSERT)
		bdp->endspaces = bdp->start_char_vcols - bdp->startspaces;
	    else if (oap->op_type == OP_APPEND)
	    {
		bdp->startspaces += oap->end_vcol - oap->start_vcol + 1;
		bdp->endspaces = bdp->start_char_vcols - bdp->startspaces;
	    }
	    else
	    {
		bdp->startspaces = oap->end_vcol - oap->start_vcol + 1;
		if (is_del && oap->op_type != OP_LSHIFT)
		{
		    /* just putting the sum of those two into
		     * bdp->startspaces doesn't work for Visual replace,
		     * so we have to split the tab in two */
		    bdp->startspaces = bdp->start_char_vcols
					- (bdp->start_vcol - oap->start_vcol);
		    bdp->endspaces = bdp->end_vcol - oap->end_vcol - 1;
		}
	    }
	}
	else
	{
	    prev_pend = pend;
	    while (bdp->end_vcol <= oap->end_vcol && *pend != NUL)
	    {
		/* Count a tab for what it's worth (if list mode not on) */
		prev_pend = pend;
		incr = lbr_chartabsize_adv(line, &pend, (colnr_T)bdp->end_vcol);
		bdp->end_vcol += incr;
	    }
	    if (bdp->end_vcol <= oap->end_vcol
		    && (!is_del
			|| oap->op_type == OP_APPEND
			|| oap->op_type == OP_REPLACE)) /* line too short */
	    {
		bdp->is_short = TRUE;
		/* Alternative: include spaces to fill up the block.
		 * Disadvantage: can lead to trailing spaces when the line is
		 * short where the text is put */
		/* if (!is_del || oap->op_type == OP_APPEND) */
		if (oap->op_type == OP_APPEND || virtual_op)
		    bdp->endspaces = oap->end_vcol - bdp->end_vcol
							     + oap->inclusive;
		else
		    bdp->endspaces = 0; /* replace doesn't add characters */
	    }
	    else if (bdp->end_vcol > oap->end_vcol)
	    {
		bdp->endspaces = bdp->end_vcol - oap->end_vcol - 1;
		if (!is_del && bdp->endspaces)
		{
		    bdp->endspaces = incr - bdp->endspaces;
		    if (pend != pstart)
			pend = prev_pend;
		}
	    }
	}
	bdp->end_char_vcols = incr;
	if (is_del && bdp->startspaces)
	    pstart = prev_pstart;
	bdp->textlen = (int)(pend - pstart);
    }
    bdp->textcol = (colnr_T) (pstart - line);
    bdp->textstart = pstart;
}

/*
 * Handle the add/subtract operator.
 */
    void
op_addsub(
    oparg_T	*oap,
    linenr_T	Prenum1,	    /* Amount of add/subtract */
    int		g_cmd)		    /* was g<c-a>/g<c-x> */
{
    pos_T		pos;
    struct block_def	bd;
    int			change_cnt = 0;
    linenr_T		amount = Prenum1;

   // do_addsub() might trigger re-evaluation of 'foldexpr' halfway, when the
   // buffer is not completely updated yet. Postpone updating folds until before
   // the call to changed_lines().
#ifdef FEAT_FOLDING
   disable_fold_update++;
#endif

    if (!VIsual_active)
    {
	pos = curwin->w_cursor;
	if (u_save_cursor() == FAIL)
	{
#ifdef FEAT_FOLDING
	    disable_fold_update--;
#endif
	    return;
	}
	change_cnt = do_addsub(oap->op_type, &pos, 0, amount);
#ifdef FEAT_FOLDING
	disable_fold_update--;
#endif
	if (change_cnt)
	    changed_lines(pos.lnum, 0, pos.lnum + 1, 0L);
    }
    else
    {
	int	one_change;
	int	length;
	pos_T	startpos;

	if (u_save((linenr_T)(oap->start.lnum - 1),
					(linenr_T)(oap->end.lnum + 1)) == FAIL)
	{
#ifdef FEAT_FOLDING
	    disable_fold_update--;
#endif
	    return;
	}

	pos = oap->start;
	for (; pos.lnum <= oap->end.lnum; ++pos.lnum)
	{
	    if (oap->block_mode)		    /* Visual block mode */
	    {
		block_prep(oap, &bd, pos.lnum, FALSE);
		pos.col = bd.textcol;
		length = bd.textlen;
	    }
	    else if (oap->motion_type == MLINE)
	    {
		curwin->w_cursor.col = 0;
		pos.col = 0;
		length = (colnr_T)STRLEN(ml_get(pos.lnum));
	    }
	    else /* oap->motion_type == MCHAR */
	    {
		if (pos.lnum == oap->start.lnum && !oap->inclusive)
		    dec(&(oap->end));
		length = (colnr_T)STRLEN(ml_get(pos.lnum));
		pos.col = 0;
		if (pos.lnum == oap->start.lnum)
		{
		    pos.col += oap->start.col;
		    length -= oap->start.col;
		}
		if (pos.lnum == oap->end.lnum)
		{
		    length = (int)STRLEN(ml_get(oap->end.lnum));
		    if (oap->end.col >= length)
			oap->end.col = length - 1;
		    length = oap->end.col - pos.col + 1;
		}
	    }
	    one_change = do_addsub(oap->op_type, &pos, length, amount);
	    if (one_change)
	    {
		/* Remember the start position of the first change. */
		if (change_cnt == 0)
		    startpos = curbuf->b_op_start;
		++change_cnt;
	    }

#ifdef FEAT_NETBEANS_INTG
	    if (netbeans_active() && one_change)
	    {
		char_u *ptr = ml_get_buf(curbuf, pos.lnum, FALSE);

		netbeans_removed(curbuf, pos.lnum, pos.col, (long)length);
		netbeans_inserted(curbuf, pos.lnum, pos.col,
						&ptr[pos.col], length);
	    }
#endif
	    if (g_cmd && one_change)
		amount += Prenum1;
	}

#ifdef FEAT_FOLDING
	disable_fold_update--;
#endif
	if (change_cnt)
	    changed_lines(oap->start.lnum, 0, oap->end.lnum + 1, 0L);

	if (!change_cnt && oap->is_VIsual)
	    /* No change: need to remove the Visual selection */
	    redraw_curbuf_later(INVERTED);

	/* Set '[ mark if something changed. Keep the last end
	 * position from do_addsub(). */
	if (change_cnt > 0)
	    curbuf->b_op_start = startpos;

	if (change_cnt > p_report)
	    smsg(NGETTEXT("%ld line changed", "%ld lines changed",
						      change_cnt), change_cnt);
    }
}

/*
 * Add or subtract 'Prenum1' from a number in a line
 * op_type is OP_NR_ADD or OP_NR_SUB
 *
 * Returns TRUE if some character was changed.
 */
    static int
do_addsub(
    int		op_type,
    pos_T	*pos,
    int		length,
    linenr_T	Prenum1)
{
    int		col;
    char_u	*buf1;
    char_u	buf2[NUMBUFLEN];
    int		pre;		/* 'X'/'x': hex; '0': octal; 'B'/'b': bin */
    static int	hexupper = FALSE;	/* 0xABC */
    uvarnumber_T	n;
    uvarnumber_T	oldn;
    char_u	*ptr;
    int		c;
    int		todel;
    int		dohex;
    int		dooct;
    int		dobin;
    int		doalp;
    int		firstdigit;
    int		subtract;
    int		negative = FALSE;
    int		was_positive = TRUE;
    int		visual = VIsual_active;
    int		did_change = FALSE;
    pos_T	save_cursor = curwin->w_cursor;
    int		maxlen = 0;
    pos_T	startpos;
    pos_T	endpos;

    dohex = (vim_strchr(curbuf->b_p_nf, 'x') != NULL);	/* "heX" */
    dooct = (vim_strchr(curbuf->b_p_nf, 'o') != NULL);	/* "Octal" */
    dobin = (vim_strchr(curbuf->b_p_nf, 'b') != NULL);	/* "Bin" */
    doalp = (vim_strchr(curbuf->b_p_nf, 'p') != NULL);	/* "alPha" */

    curwin->w_cursor = *pos;
    ptr = ml_get(pos->lnum);
    col = pos->col;

    if (*ptr == NUL)
	goto theend;

    /*
     * First check if we are on a hexadecimal number, after the "0x".
     */
    if (!VIsual_active)
    {
	if (dobin)
	    while (col > 0 && vim_isbdigit(ptr[col]))
	    {
		--col;
		if (has_mbyte)
		    col -= (*mb_head_off)(ptr, ptr + col);
	    }

	if (dohex)
	    while (col > 0 && vim_isxdigit(ptr[col]))
	    {
		--col;
		if (has_mbyte)
		    col -= (*mb_head_off)(ptr, ptr + col);
	    }

	if (       dobin
		&& dohex
		&& ! ((col > 0
		    && (ptr[col] == 'X'
			|| ptr[col] == 'x')
		    && ptr[col - 1] == '0'
		    && (!has_mbyte ||
			!(*mb_head_off)(ptr, ptr + col - 1))
		    && vim_isxdigit(ptr[col + 1]))))
	{

	    /* In case of binary/hexadecimal pattern overlap match, rescan */

	    col = pos->col;

	    while (col > 0 && vim_isdigit(ptr[col]))
	    {
		col--;
		if (has_mbyte)
		    col -= (*mb_head_off)(ptr, ptr + col);
	    }
	}

	if ((       dohex
		&& col > 0
		&& (ptr[col] == 'X'
		    || ptr[col] == 'x')
		&& ptr[col - 1] == '0'
		&& (!has_mbyte ||
		    !(*mb_head_off)(ptr, ptr + col - 1))
		&& vim_isxdigit(ptr[col + 1])) ||
	    (       dobin
		&& col > 0
		&& (ptr[col] == 'B'
		    || ptr[col] == 'b')
		&& ptr[col - 1] == '0'
		&& (!has_mbyte ||
		    !(*mb_head_off)(ptr, ptr + col - 1))
		&& vim_isbdigit(ptr[col + 1])))
	{
	    /* Found hexadecimal or binary number, move to its start. */
	    --col;
	    if (has_mbyte)
		col -= (*mb_head_off)(ptr, ptr + col);
	}
	else
	{
	    /*
	     * Search forward and then backward to find the start of number.
	     */
	    col = pos->col;

	    while (ptr[col] != NUL
		    && !vim_isdigit(ptr[col])
		    && !(doalp && ASCII_ISALPHA(ptr[col])))
		col += MB_PTR2LEN(ptr + col);

	    while (col > 0
		    && vim_isdigit(ptr[col - 1])
		    && !(doalp && ASCII_ISALPHA(ptr[col])))
	    {
		--col;
		if (has_mbyte)
		    col -= (*mb_head_off)(ptr, ptr + col);
	    }
	}
    }

    if (visual)
    {
	while (ptr[col] != NUL && length > 0
		&& !vim_isdigit(ptr[col])
		&& !(doalp && ASCII_ISALPHA(ptr[col])))
	{
	    int mb_len = MB_PTR2LEN(ptr + col);

	    col += mb_len;
	    length -= mb_len;
	}

	if (length == 0)
	    goto theend;

	if (col > pos->col && ptr[col - 1] == '-'
		&& (!has_mbyte || !(*mb_head_off)(ptr, ptr + col - 1)))
	{
	    negative = TRUE;
	    was_positive = FALSE;
	}
    }

    /*
     * If a number was found, and saving for undo works, replace the number.
     */
    firstdigit = ptr[col];
    if (!VIM_ISDIGIT(firstdigit) && !(doalp && ASCII_ISALPHA(firstdigit)))
    {
	beep_flush();
	goto theend;
    }

    if (doalp && ASCII_ISALPHA(firstdigit))
    {
	/* decrement or increment alphabetic character */
	if (op_type == OP_NR_SUB)
	{
	    if (CharOrd(firstdigit) < Prenum1)
	    {
		if (isupper(firstdigit))
		    firstdigit = 'A';
		else
		    firstdigit = 'a';
	    }
	    else
#ifdef EBCDIC
		firstdigit = EBCDIC_CHAR_ADD(firstdigit, -Prenum1);
#else
		firstdigit -= Prenum1;
#endif
	}
	else
	{
	    if (26 - CharOrd(firstdigit) - 1 < Prenum1)
	    {
		if (isupper(firstdigit))
		    firstdigit = 'Z';
		else
		    firstdigit = 'z';
	    }
	    else
#ifdef EBCDIC
		firstdigit = EBCDIC_CHAR_ADD(firstdigit, Prenum1);
#else
		firstdigit += Prenum1;
#endif
	}
	curwin->w_cursor.col = col;
	if (!did_change)
	    startpos = curwin->w_cursor;
	did_change = TRUE;
	(void)del_char(FALSE);
	ins_char(firstdigit);
	endpos = curwin->w_cursor;
	curwin->w_cursor.col = col;
    }
    else
    {
	if (col > 0 && ptr[col - 1] == '-'
		&& (!has_mbyte ||
		    !(*mb_head_off)(ptr, ptr + col - 1))
		&& !visual)
	{
	    /* negative number */
	    --col;
	    negative = TRUE;
	}
	/* get the number value (unsigned) */
	if (visual && VIsual_mode != 'V')
	    maxlen = (curbuf->b_visual.vi_curswant == MAXCOL
		    ? (int)STRLEN(ptr) - col
		    : length);

	vim_str2nr(ptr + col, &pre, &length,
		0 + (dobin ? STR2NR_BIN : 0)
		    + (dooct ? STR2NR_OCT : 0)
		    + (dohex ? STR2NR_HEX : 0),
		NULL, &n, maxlen);

	/* ignore leading '-' for hex and octal and bin numbers */
	if (pre && negative)
	{
	    ++col;
	    --length;
	    negative = FALSE;
	}
	/* add or subtract */
	subtract = FALSE;
	if (op_type == OP_NR_SUB)
	    subtract ^= TRUE;
	if (negative)
	    subtract ^= TRUE;

	oldn = n;
	if (subtract)
	    n -= (uvarnumber_T)Prenum1;
	else
	    n += (uvarnumber_T)Prenum1;
	/* handle wraparound for decimal numbers */
	if (!pre)
	{
	    if (subtract)
	    {
		if (n > oldn)
		{
		    n = 1 + (n ^ (uvarnumber_T)-1);
		    negative ^= TRUE;
		}
	    }
	    else
	    {
		/* add */
		if (n < oldn)
		{
		    n = (n ^ (uvarnumber_T)-1);
		    negative ^= TRUE;
		}
	    }
	    if (n == 0)
		negative = FALSE;
	}

	if (visual && !was_positive && !negative && col > 0)
	{
	    /* need to remove the '-' */
	    col--;
	    length++;
	}

	/*
	 * Delete the old number.
	 */
	curwin->w_cursor.col = col;
	if (!did_change)
	    startpos = curwin->w_cursor;
	did_change = TRUE;
	todel = length;
	c = gchar_cursor();
	/*
	 * Don't include the '-' in the length, only the length of the
	 * part after it is kept the same.
	 */
	if (c == '-')
	    --length;
	while (todel-- > 0)
	{
	    if (c < 0x100 && isalpha(c))
	    {
		if (isupper(c))
		    hexupper = TRUE;
		else
		    hexupper = FALSE;
	    }
	    /* del_char() will mark line needing displaying */
	    (void)del_char(FALSE);
	    c = gchar_cursor();
	}

	/*
	 * Prepare the leading characters in buf1[].
	 * When there are many leading zeros it could be very long.
	 * Allocate a bit too much.
	 */
	buf1 = alloc((unsigned)length + NUMBUFLEN);
	if (buf1 == NULL)
	    goto theend;
	ptr = buf1;
	if (negative && (!visual || was_positive))
	    *ptr++ = '-';
	if (pre)
	{
	    *ptr++ = '0';
	    --length;
	}
	if (pre == 'b' || pre == 'B' ||
	    pre == 'x' || pre == 'X')
	{
	    *ptr++ = pre;
	    --length;
	}

	/*
	 * Put the number characters in buf2[].
	 */
	if (pre == 'b' || pre == 'B')
	{
	    int i;
	    int bit = 0;
	    int bits = sizeof(uvarnumber_T) * 8;

	    /* leading zeros */
	    for (bit = bits; bit > 0; bit--)
		if ((n >> (bit - 1)) & 0x1) break;

	    for (i = 0; bit > 0; bit--)
		buf2[i++] = ((n >> (bit - 1)) & 0x1) ? '1' : '0';

	    buf2[i] = '\0';
	}
	else if (pre == 0)
	    vim_snprintf((char *)buf2, NUMBUFLEN, "%llu",
							(long_long_u_T)n);
	else if (pre == '0')
	    vim_snprintf((char *)buf2, NUMBUFLEN, "%llo",
							(long_long_u_T)n);
	else if (pre && hexupper)
	    vim_snprintf((char *)buf2, NUMBUFLEN, "%llX",
							(long_long_u_T)n);
	else
	    vim_snprintf((char *)buf2, NUMBUFLEN, "%llx",
							(long_long_u_T)n);
	length -= (int)STRLEN(buf2);

	/*
	 * Adjust number of zeros to the new number of digits, so the
	 * total length of the number remains the same.
	 * Don't do this when
	 * the result may look like an octal number.
	 */
	if (firstdigit == '0' && !(dooct && pre == 0))
	    while (length-- > 0)
		*ptr++ = '0';
	*ptr = NUL;
	STRCAT(buf1, buf2);
	ins_str(buf1);		/* insert the new number */
	vim_free(buf1);
	endpos = curwin->w_cursor;
	if (did_change && curwin->w_cursor.col)
	    --curwin->w_cursor.col;
    }

    if (did_change)
    {
	/* set the '[ and '] marks */
	curbuf->b_op_start = startpos;
	curbuf->b_op_end = endpos;
	if (curbuf->b_op_end.col > 0)
	    --curbuf->b_op_end.col;
    }

theend:
    if (visual)
	curwin->w_cursor = save_cursor;
    else if (did_change)
	curwin->w_set_curswant = TRUE;

    return did_change;
}

#ifdef FEAT_VIMINFO

static yankreg_T *y_read_regs = NULL;

#define REG_PREVIOUS 1
#define REG_EXEC 2

/*
 * Prepare for reading viminfo registers when writing viminfo later.
 */
    void
prepare_viminfo_registers(void)
{
     y_read_regs = (yankreg_T *)alloc_clear(NUM_REGISTERS
						    * (int)sizeof(yankreg_T));
}

    void
finish_viminfo_registers(void)
{
    int		i;
    int		j;

    if (y_read_regs != NULL)
    {
	for (i = 0; i < NUM_REGISTERS; ++i)
	    if (y_read_regs[i].y_array != NULL)
	    {
		for (j = 0; j < y_read_regs[i].y_size; j++)
		    vim_free(y_read_regs[i].y_array[j]);
		vim_free(y_read_regs[i].y_array);
	    }
	VIM_CLEAR(y_read_regs);
    }
}

    int
read_viminfo_register(vir_T *virp, int force)
{
    int		eof;
    int		do_it = TRUE;
    int		size;
    int		limit;
    int		i;
    int		set_prev = FALSE;
    char_u	*str;
    char_u	**array = NULL;
    int		new_type = MCHAR; /* init to shut up compiler */
    colnr_T	new_width = 0; /* init to shut up compiler */

    /* We only get here (hopefully) if line[0] == '"' */
    str = virp->vir_line + 1;

    /* If the line starts with "" this is the y_previous register. */
    if (*str == '"')
    {
	set_prev = TRUE;
	str++;
    }

    if (!ASCII_ISALNUM(*str) && *str != '-')
    {
	if (viminfo_error("E577: ", _("Illegal register name"), virp->vir_line))
	    return TRUE;	/* too many errors, pretend end-of-file */
	do_it = FALSE;
    }
    get_yank_register(*str++, FALSE);
    if (!force && y_current->y_array != NULL)
	do_it = FALSE;

    if (*str == '@')
    {
	/* "x@: register x used for @@ */
	if (force || execreg_lastc == NUL)
	    execreg_lastc = str[-1];
    }

    size = 0;
    limit = 100;	/* Optimized for registers containing <= 100 lines */
    if (do_it)
    {
	/*
	 * Build the new register in array[].
	 * y_array is kept as-is until done.
	 * The "do_it" flag is reset when something is wrong, in which case
	 * array[] needs to be freed.
	 */
	if (set_prev)
	    y_previous = y_current;
	array = (char_u **)alloc((unsigned)(limit * sizeof(char_u *)));
	str = skipwhite(skiptowhite(str));
	if (STRNCMP(str, "CHAR", 4) == 0)
	    new_type = MCHAR;
	else if (STRNCMP(str, "BLOCK", 5) == 0)
	    new_type = MBLOCK;
	else
	    new_type = MLINE;
	/* get the block width; if it's missing we get a zero, which is OK */
	str = skipwhite(skiptowhite(str));
	new_width = getdigits(&str);
    }

    while (!(eof = viminfo_readline(virp))
		    && (virp->vir_line[0] == TAB || virp->vir_line[0] == '<'))
    {
	if (do_it)
	{
	    if (size == limit)
	    {
		char_u **new_array = (char_u **)
			      alloc((unsigned)(limit * 2 * sizeof(char_u *)));

		if (new_array == NULL)
		{
		    do_it = FALSE;
		    break;
		}
		for (i = 0; i < limit; i++)
		    new_array[i] = array[i];
		vim_free(array);
		array = new_array;
		limit *= 2;
	    }
	    str = viminfo_readstring(virp, 1, TRUE);
	    if (str != NULL)
		array[size++] = str;
	    else
		/* error, don't store the result */
		do_it = FALSE;
	}
    }

    if (do_it)
    {
	/* free y_array[] */
	for (i = 0; i < y_current->y_size; i++)
	    vim_free(y_current->y_array[i]);
	vim_free(y_current->y_array);

	y_current->y_type = new_type;
	y_current->y_width = new_width;
	y_current->y_size = size;
	y_current->y_time_set = 0;
	if (size == 0)
	{
	    y_current->y_array = NULL;
	}
	else
	{
	    /* Move the lines from array[] to y_array[]. */
	    y_current->y_array =
			(char_u **)alloc((unsigned)(size * sizeof(char_u *)));
	    for (i = 0; i < size; i++)
	    {
		if (y_current->y_array == NULL)
		    vim_free(array[i]);
		else
		    y_current->y_array[i] = array[i];
	    }
	}
    }
    else
    {
	/* Free array[] if it was filled. */
	for (i = 0; i < size; i++)
	    vim_free(array[i]);
    }
    vim_free(array);

    return eof;
}

/*
 * Accept a new style register line from the viminfo, store it when it's new.
 */
    void
handle_viminfo_register(garray_T *values, int force)
{
    bval_T	*vp = (bval_T *)values->ga_data;
    int		flags;
    int		name;
    int		type;
    int		linecount;
    int		width;
    time_t	timestamp;
    yankreg_T	*y_ptr;
    int		i;

    /* Check the format:
     * |{bartype},{flags},{name},{type},
     *      {linecount},{width},{timestamp},"line1","line2"
     */
    if (values->ga_len < 6
	    || vp[0].bv_type != BVAL_NR
	    || vp[1].bv_type != BVAL_NR
	    || vp[2].bv_type != BVAL_NR
	    || vp[3].bv_type != BVAL_NR
	    || vp[4].bv_type != BVAL_NR
	    || vp[5].bv_type != BVAL_NR)
	return;
    flags = vp[0].bv_nr;
    name = vp[1].bv_nr;
    if (name < 0 || name >= NUM_REGISTERS)
	return;
    type = vp[2].bv_nr;
    if (type != MCHAR && type != MLINE && type != MBLOCK)
	return;
    linecount = vp[3].bv_nr;
    if (values->ga_len < 6 + linecount)
	return;
    width = vp[4].bv_nr;
    if (width < 0)
	return;

    if (y_read_regs != NULL)
	/* Reading viminfo for merging and writing.  Store the register
	 * content, don't update the current registers. */
	y_ptr = &y_read_regs[name];
    else
	y_ptr = &y_regs[name];

    /* Do not overwrite unless forced or the timestamp is newer. */
    timestamp = (time_t)vp[5].bv_nr;
    if (y_ptr->y_array != NULL && !force
			 && (timestamp == 0 || y_ptr->y_time_set > timestamp))
	return;

    if (y_ptr->y_array != NULL)
	for (i = 0; i < y_ptr->y_size; i++)
	    vim_free(y_ptr->y_array[i]);
    vim_free(y_ptr->y_array);

    if (y_read_regs == NULL)
    {
	if (flags & REG_PREVIOUS)
	    y_previous = y_ptr;
	if ((flags & REG_EXEC) && (force || execreg_lastc == NUL))
	    execreg_lastc = get_register_name(name);
    }
    y_ptr->y_type = type;
    y_ptr->y_width = width;
    y_ptr->y_size = linecount;
    y_ptr->y_time_set = timestamp;
    if (linecount == 0)
	y_ptr->y_array = NULL;
    else
    {
	y_ptr->y_array =
		   (char_u **)alloc((unsigned)(linecount * sizeof(char_u *)));
	for (i = 0; i < linecount; i++)
	{
	    if (vp[i + 6].bv_allocated)
	    {
		y_ptr->y_array[i] = vp[i + 6].bv_string;
		vp[i + 6].bv_string = NULL;
	    }
	    else
		y_ptr->y_array[i] = vim_strsave(vp[i + 6].bv_string);
	}
    }
}

    void
write_viminfo_registers(FILE *fp)
{
    int		i, j;
    char_u	*type;
    char_u	c;
    int		num_lines;
    int		max_num_lines;
    int		max_kbyte;
    long	len;
    yankreg_T	*y_ptr;

    fputs(_("\n# Registers:\n"), fp);

    /* Get '<' value, use old '"' value if '<' is not found. */
    max_num_lines = get_viminfo_parameter('<');
    if (max_num_lines < 0)
	max_num_lines = get_viminfo_parameter('"');
    if (max_num_lines == 0)
	return;
    max_kbyte = get_viminfo_parameter('s');
    if (max_kbyte == 0)
	return;

    for (i = 0; i < NUM_REGISTERS; i++)
    {
#ifdef FEAT_CLIPBOARD
	/* Skip '*'/'+' register, we don't want them back next time */
	if (i == STAR_REGISTER || i == PLUS_REGISTER)
	    continue;
#endif
#ifdef FEAT_DND
	/* Neither do we want the '~' register */
	if (i == TILDE_REGISTER)
	    continue;
#endif
	/* When reading viminfo for merging and writing: Use the register from
	 * viminfo if it's newer. */
	if (y_read_regs != NULL
		&& y_read_regs[i].y_array != NULL
		&& (y_regs[i].y_array == NULL ||
			    y_read_regs[i].y_time_set > y_regs[i].y_time_set))
	    y_ptr = &y_read_regs[i];
	else if (y_regs[i].y_array == NULL)
	    continue;
	else
	    y_ptr = &y_regs[i];

	/* Skip empty registers. */
	num_lines = y_ptr->y_size;
	if (num_lines == 0
		|| (num_lines == 1 && y_ptr->y_type == MCHAR
					&& *y_ptr->y_array[0] == NUL))
	    continue;

	if (max_kbyte > 0)
	{
	    /* Skip register if there is more text than the maximum size. */
	    len = 0;
	    for (j = 0; j < num_lines; j++)
		len += (long)STRLEN(y_ptr->y_array[j]) + 1L;
	    if (len > (long)max_kbyte * 1024L)
		continue;
	}

	switch (y_ptr->y_type)
	{
	    case MLINE:
		type = (char_u *)"LINE";
		break;
	    case MCHAR:
		type = (char_u *)"CHAR";
		break;
	    case MBLOCK:
		type = (char_u *)"BLOCK";
		break;
	    default:
		semsg(_("E574: Unknown register type %d"), y_ptr->y_type);
		type = (char_u *)"LINE";
		break;
	}
	if (y_previous == &y_regs[i])
	    fprintf(fp, "\"");
	c = get_register_name(i);
	fprintf(fp, "\"%c", c);
	if (c == execreg_lastc)
	    fprintf(fp, "@");
	fprintf(fp, "\t%s\t%d\n", type, (int)y_ptr->y_width);

	/* If max_num_lines < 0, then we save ALL the lines in the register */
	if (max_num_lines > 0 && num_lines > max_num_lines)
	    num_lines = max_num_lines;
	for (j = 0; j < num_lines; j++)
	{
	    putc('\t', fp);
	    viminfo_writestring(fp, y_ptr->y_array[j]);
	}

	{
	    int	    flags = 0;
	    int	    remaining;

	    /* New style with a bar line. Format:
	     * |{bartype},{flags},{name},{type},
	     *      {linecount},{width},{timestamp},"line1","line2"
	     * flags: REG_PREVIOUS - register is y_previous
	     *	      REG_EXEC - used for @@
	     */
	    if (y_previous == &y_regs[i])
		flags |= REG_PREVIOUS;
	    if (c == execreg_lastc)
		flags |= REG_EXEC;
	    fprintf(fp, "|%d,%d,%d,%d,%d,%d,%ld", BARTYPE_REGISTER, flags,
		    i, y_ptr->y_type, num_lines, (int)y_ptr->y_width,
		    (long)y_ptr->y_time_set);
	    /* 11 chars for type/flags/name/type, 3 * 20 for numbers */
	    remaining = LSIZE - 71;
	    for (j = 0; j < num_lines; j++)
	    {
		putc(',', fp);
		--remaining;
		remaining = barline_writestring(fp, y_ptr->y_array[j],
								   remaining);
	    }
	    putc('\n', fp);
	}
    }
}
#endif /* FEAT_VIMINFO */

#if defined(FEAT_CLIPBOARD) || defined(PROTO)
/*
 * SELECTION / PRIMARY ('*')
 *
 * Text selection stuff that uses the GUI selection register '*'.  When using a
 * GUI this may be text from another window, otherwise it is the last text we
 * had highlighted with VIsual mode.  With mouse support, clicking the middle
 * button performs the paste, otherwise you will need to do <"*p>. "
 * If not under X, it is synonymous with the clipboard register '+'.
 *
 * X CLIPBOARD ('+')
 *
 * Text selection stuff that uses the GUI clipboard register '+'.
 * Under X, this matches the standard cut/paste buffer CLIPBOARD selection.
 * It will be used for unnamed cut/pasting is 'clipboard' contains "unnamed",
 * otherwise you will need to do <"+p>. "
 * If not under X, it is synonymous with the selection register '*'.
 */

/*
 * Routine to export any final X selection we had to the environment
 * so that the text is still available after Vim has exited. X selections
 * only exist while the owning application exists, so we write to the
 * permanent (while X runs) store CUT_BUFFER0.
 * Dump the CLIPBOARD selection if we own it (it's logically the more
 * 'permanent' of the two), otherwise the PRIMARY one.
 * For now, use a hard-coded sanity limit of 1Mb of data.
 */
#if (defined(FEAT_X11) && defined(FEAT_CLIPBOARD)) || defined(PROTO)
    void
x11_export_final_selection(void)
{
    Display	*dpy;
    char_u	*str = NULL;
    long_u	len = 0;
    int		motion_type = -1;

# ifdef FEAT_GUI
    if (gui.in_use)
	dpy = X_DISPLAY;
    else
# endif
# ifdef FEAT_XCLIPBOARD
	dpy = xterm_dpy;
# else
	return;
# endif

    /* Get selection to export */
    if (clip_plus.owned)
	motion_type = clip_convert_selection(&str, &len, &clip_plus);
    else if (clip_star.owned)
	motion_type = clip_convert_selection(&str, &len, &clip_star);

    /* Check it's OK */
    if (dpy != NULL && str != NULL && motion_type >= 0
					       && len < 1024*1024 && len > 0)
    {
	int ok = TRUE;

	/* The CUT_BUFFER0 is supposed to always contain latin1.  Convert from
	 * 'enc' when it is a multi-byte encoding.  When 'enc' is an 8-bit
	 * encoding conversion usually doesn't work, so keep the text as-is.
	 */
	if (has_mbyte)
	{
	    vimconv_T	vc;

	    vc.vc_type = CONV_NONE;
	    if (convert_setup(&vc, p_enc, (char_u *)"latin1") == OK)
	    {
		int	intlen = len;
		char_u	*conv_str;

		vc.vc_fail = TRUE;
		conv_str = string_convert(&vc, str, &intlen);
		len = intlen;
		if (conv_str != NULL)
		{
		    vim_free(str);
		    str = conv_str;
		}
		else
		{
		    ok = FALSE;
		}
		convert_setup(&vc, NULL, NULL);
	    }
	    else
	    {
		ok = FALSE;
	    }
	}

	/* Do not store the string if conversion failed.  Better to use any
	 * other selection than garbled text. */
	if (ok)
	{
	    XStoreBuffer(dpy, (char *)str, (int)len, 0);
	    XFlush(dpy);
	}
    }

    vim_free(str);
}
#endif

    void
clip_free_selection(VimClipboard *cbd)
{
    yankreg_T *y_ptr = y_current;

    if (cbd == &clip_plus)
	y_current = &y_regs[PLUS_REGISTER];
    else
	y_current = &y_regs[STAR_REGISTER];
    free_yank_all();
    y_current->y_size = 0;
    y_current = y_ptr;
}

/*
 * Get the selected text and put it in register '*' or '+'.
 */
    void
clip_get_selection(VimClipboard *cbd)
{
    yankreg_T	*old_y_previous, *old_y_current;
    pos_T	old_cursor;
    pos_T	old_visual;
    int		old_visual_mode;
    colnr_T	old_curswant;
    int		old_set_curswant;
    pos_T	old_op_start, old_op_end;
    oparg_T	oa;
    cmdarg_T	ca;

    if (cbd->owned)
    {
	if ((cbd == &clip_plus && y_regs[PLUS_REGISTER].y_array != NULL)
		|| (cbd == &clip_star && y_regs[STAR_REGISTER].y_array != NULL))
	    return;

	/* Get the text between clip_star.start & clip_star.end */
	old_y_previous = y_previous;
	old_y_current = y_current;
	old_cursor = curwin->w_cursor;
	old_curswant = curwin->w_curswant;
	old_set_curswant = curwin->w_set_curswant;
	old_op_start = curbuf->b_op_start;
	old_op_end = curbuf->b_op_end;
	old_visual = VIsual;
	old_visual_mode = VIsual_mode;
	clear_oparg(&oa);
	oa.regname = (cbd == &clip_plus ? '+' : '*');
	oa.op_type = OP_YANK;
	vim_memset(&ca, 0, sizeof(ca));
	ca.oap = &oa;
	ca.cmdchar = 'y';
	ca.count1 = 1;
	ca.retval = CA_NO_ADJ_OP_END;
	do_pending_operator(&ca, 0, TRUE);
	y_previous = old_y_previous;
	y_current = old_y_current;
	curwin->w_cursor = old_cursor;
	changed_cline_bef_curs();   /* need to update w_virtcol et al */
	curwin->w_curswant = old_curswant;
	curwin->w_set_curswant = old_set_curswant;
	curbuf->b_op_start = old_op_start;
	curbuf->b_op_end = old_op_end;
	VIsual = old_visual;
	VIsual_mode = old_visual_mode;
    }
    else if (!is_clipboard_needs_update())
    {
	clip_free_selection(cbd);

	/* Try to get selected text from another window */
	clip_gen_request_selection(cbd);
    }
}

/*
 * Convert from the GUI selection string into the '*'/'+' register.
 */
    void
clip_yank_selection(
    int		type,
    char_u	*str,
    long	len,
    VimClipboard *cbd)
{
    yankreg_T *y_ptr;

    if (cbd == &clip_plus)
	y_ptr = &y_regs[PLUS_REGISTER];
    else
	y_ptr = &y_regs[STAR_REGISTER];

    clip_free_selection(cbd);

    str_to_reg(y_ptr, type, str, len, 0L, FALSE);
}

/*
 * Convert the '*'/'+' register into a GUI selection string returned in *str
 * with length *len.
 * Returns the motion type, or -1 for failure.
 */
    int
clip_convert_selection(char_u **str, long_u *len, VimClipboard *cbd)
{
    char_u	*p;
    int		lnum;
    int		i, j;
    int_u	eolsize;
    yankreg_T	*y_ptr;

    if (cbd == &clip_plus)
	y_ptr = &y_regs[PLUS_REGISTER];
    else
	y_ptr = &y_regs[STAR_REGISTER];

#ifdef USE_CRNL
    eolsize = 2;
#else
    eolsize = 1;
#endif

    *str = NULL;
    *len = 0;
    if (y_ptr->y_array == NULL)
	return -1;

    for (i = 0; i < y_ptr->y_size; i++)
	*len += (long_u)STRLEN(y_ptr->y_array[i]) + eolsize;

    /*
     * Don't want newline character at end of last line if we're in MCHAR mode.
     */
    if (y_ptr->y_type == MCHAR && *len >= eolsize)
	*len -= eolsize;

    p = *str = lalloc(*len + 1, TRUE);	/* add one to avoid zero */
    if (p == NULL)
	return -1;
    lnum = 0;
    for (i = 0, j = 0; i < (int)*len; i++, j++)
    {
	if (y_ptr->y_array[lnum][j] == '\n')
	    p[i] = NUL;
	else if (y_ptr->y_array[lnum][j] == NUL)
	{
#ifdef USE_CRNL
	    p[i++] = '\r';
#endif
	    p[i] = '\n';
	    lnum++;
	    j = -1;
	}
	else
	    p[i] = y_ptr->y_array[lnum][j];
    }
    return y_ptr->y_type;
}


/*
 * If we have written to a clipboard register, send the text to the clipboard.
 */
    static void
may_set_selection(void)
{
    if (y_current == &(y_regs[STAR_REGISTER]) && clip_star.available)
    {
	clip_own_selection(&clip_star);
	clip_gen_set_selection(&clip_star);
    }
    else if (y_current == &(y_regs[PLUS_REGISTER]) && clip_plus.available)
    {
	clip_own_selection(&clip_plus);
	clip_gen_set_selection(&clip_plus);
    }
}

#endif /* FEAT_CLIPBOARD || PROTO */


#if defined(FEAT_DND) || defined(PROTO)
/*
 * Replace the contents of the '~' register with str.
 */
    void
dnd_yank_drag_data(char_u *str, long len)
{
    yankreg_T *curr;

    curr = y_current;
    y_current = &y_regs[TILDE_REGISTER];
    free_yank_all();
    str_to_reg(y_current, MCHAR, str, len, 0L, FALSE);
    y_current = curr;
}
#endif


#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Return the type of a register.
 * Used for getregtype()
 * Returns MAUTO for error.
 */
    char_u
get_reg_type(int regname, long *reglen)
{
    switch (regname)
    {
	case '%':		/* file name */
	case '#':		/* alternate file name */
	case '=':		/* expression */
	case ':':		/* last command line */
	case '/':		/* last search-pattern */
	case '.':		/* last inserted text */
#ifdef FEAT_SEARCHPATH
	case Ctrl_F:		/* Filename under cursor */
	case Ctrl_P:		/* Path under cursor, expand via "path" */
#endif
	case Ctrl_W:		/* word under cursor */
	case Ctrl_A:		/* WORD (mnemonic All) under cursor */
	case '_':		/* black hole: always empty */
	    return MCHAR;
    }

#ifdef FEAT_CLIPBOARD
    regname = may_get_selection(regname);
#endif

    if (regname != NUL && !valid_yank_reg(regname, FALSE))
	return MAUTO;

    get_yank_register(regname, FALSE);

    if (y_current->y_array != NULL)
    {
	if (reglen != NULL && y_current->y_type == MBLOCK)
	    *reglen = y_current->y_width;
	return y_current->y_type;
    }
    return MAUTO;
}

/*
 * When "flags" has GREG_LIST return a list with text "s".
 * Otherwise just return "s".
 */
    static char_u *
getreg_wrap_one_line(char_u *s, int flags)
{
    if (flags & GREG_LIST)
    {
	list_T *list = list_alloc();

	if (list != NULL)
	{
	    if (list_append_string(list, NULL, -1) == FAIL)
	    {
		list_free(list);
		return NULL;
	    }
	    list->lv_first->li_tv.vval.v_string = s;
	}
	return (char_u *)list;
    }
    return s;
}

/*
 * Return the contents of a register as a single allocated string.
 * Used for "@r" in expressions and for getreg().
 * Returns NULL for error.
 * Flags:
 *	GREG_NO_EXPR	Do not allow expression register
 *	GREG_EXPR_SRC	For the expression register: return expression itself,
 *			not the result of its evaluation.
 *	GREG_LIST	Return a list of lines in place of a single string.
 */
    char_u *
get_reg_contents(int regname, int flags)
{
    long	i;
    char_u	*retval;
    int		allocated;
    long	len;

    /* Don't allow using an expression register inside an expression */
    if (regname == '=')
    {
	if (flags & GREG_NO_EXPR)
	    return NULL;
	if (flags & GREG_EXPR_SRC)
	    return getreg_wrap_one_line(get_expr_line_src(), flags);
	return getreg_wrap_one_line(get_expr_line(), flags);
    }

    if (regname == '@')	    /* "@@" is used for unnamed register */
	regname = '"';

    /* check for valid regname */
    if (regname != NUL && !valid_yank_reg(regname, FALSE))
	return NULL;

#ifdef FEAT_CLIPBOARD
    regname = may_get_selection(regname);
#endif

    if (get_spec_reg(regname, &retval, &allocated, FALSE))
    {
	if (retval == NULL)
	    return NULL;
	if (allocated)
	    return getreg_wrap_one_line(retval, flags);
	return getreg_wrap_one_line(vim_strsave(retval), flags);
    }

    get_yank_register(regname, FALSE);
    if (y_current->y_array == NULL)
	return NULL;

    if (flags & GREG_LIST)
    {
	list_T	*list = list_alloc();
	int	error = FALSE;

	if (list == NULL)
	    return NULL;
	for (i = 0; i < y_current->y_size; ++i)
	    if (list_append_string(list, y_current->y_array[i], -1) == FAIL)
		error = TRUE;
	if (error)
	{
	    list_free(list);
	    return NULL;
	}
	return (char_u *)list;
    }

    /*
     * Compute length of resulting string.
     */
    len = 0;
    for (i = 0; i < y_current->y_size; ++i)
    {
	len += (long)STRLEN(y_current->y_array[i]);
	/*
	 * Insert a newline between lines and after last line if
	 * y_type is MLINE.
	 */
	if (y_current->y_type == MLINE || i < y_current->y_size - 1)
	    ++len;
    }

    retval = lalloc(len + 1, TRUE);

    /*
     * Copy the lines of the yank register into the string.
     */
    if (retval != NULL)
    {
	len = 0;
	for (i = 0; i < y_current->y_size; ++i)
	{
	    STRCPY(retval + len, y_current->y_array[i]);
	    len += (long)STRLEN(retval + len);

	    /*
	     * Insert a NL between lines and after the last line if y_type is
	     * MLINE.
	     */
	    if (y_current->y_type == MLINE || i < y_current->y_size - 1)
		retval[len++] = '\n';
	}
	retval[len] = NUL;
    }

    return retval;
}

    static int
init_write_reg(
    int		name,
    yankreg_T	**old_y_previous,
    yankreg_T	**old_y_current,
    int		must_append,
    int		*yank_type UNUSED)
{
    if (!valid_yank_reg(name, TRUE))	    /* check for valid reg name */
    {
	emsg_invreg(name);
	return FAIL;
    }

    /* Don't want to change the current (unnamed) register */
    *old_y_previous = y_previous;
    *old_y_current = y_current;

    get_yank_register(name, TRUE);
    if (!y_append && !must_append)
	free_yank_all();
    return OK;
}

    static void
finish_write_reg(
    int		name,
    yankreg_T	*old_y_previous,
    yankreg_T	*old_y_current)
{
# ifdef FEAT_CLIPBOARD
    /* Send text of clipboard register to the clipboard. */
    may_set_selection();
# endif

    /* ':let @" = "val"' should change the meaning of the "" register */
    if (name != '"')
	y_previous = old_y_previous;
    y_current = old_y_current;
}

/*
 * Store string "str" in register "name".
 * "maxlen" is the maximum number of bytes to use, -1 for all bytes.
 * If "must_append" is TRUE, always append to the register.  Otherwise append
 * if "name" is an uppercase letter.
 * Note: "maxlen" and "must_append" don't work for the "/" register.
 * Careful: 'str' is modified, you may have to use a copy!
 * If "str" ends in '\n' or '\r', use linewise, otherwise use characterwise.
 */
    void
write_reg_contents(
    int		name,
    char_u	*str,
    int		maxlen,
    int		must_append)
{
    write_reg_contents_ex(name, str, maxlen, must_append, MAUTO, 0L);
}

    void
write_reg_contents_lst(
    int		name,
    char_u	**strings,
    int		maxlen UNUSED,
    int		must_append,
    int		yank_type,
    long	block_len)
{
    yankreg_T  *old_y_previous, *old_y_current;

    if (name == '/'
#ifdef FEAT_EVAL
	    || name == '='
#endif
	    )
    {
	char_u	*s;

	if (strings[0] == NULL)
	    s = (char_u *)"";
	else if (strings[1] != NULL)
	{
	    emsg(_("E883: search pattern and expression register may not "
			"contain two or more lines"));
	    return;
	}
	else
	    s = strings[0];
	write_reg_contents_ex(name, s, -1, must_append, yank_type, block_len);
	return;
    }

    if (name == '_')	    /* black hole: nothing to do */
	return;

    if (init_write_reg(name, &old_y_previous, &old_y_current, must_append,
		&yank_type) == FAIL)
	return;

    str_to_reg(y_current, yank_type, (char_u *) strings, -1, block_len, TRUE);

    finish_write_reg(name, old_y_previous, old_y_current);
}

    void
write_reg_contents_ex(
    int		name,
    char_u	*str,
    int		maxlen,
    int		must_append,
    int		yank_type,
    long	block_len)
{
    yankreg_T	*old_y_previous, *old_y_current;
    long	len;

    if (maxlen >= 0)
	len = maxlen;
    else
	len = (long)STRLEN(str);

    /* Special case: '/' search pattern */
    if (name == '/')
    {
	set_last_search_pat(str, RE_SEARCH, TRUE, TRUE);
	return;
    }

    if (name == '#')
    {
	buf_T	*buf;

	if (VIM_ISDIGIT(*str))
	{
	    int	num = atoi((char *)str);

	    buf = buflist_findnr(num);
	    if (buf == NULL)
		semsg(_(e_nobufnr), (long)num);
	}
	else
	    buf = buflist_findnr(buflist_findpat(str, str + STRLEN(str),
							 TRUE, FALSE, FALSE));
	if (buf == NULL)
	    return;
	curwin->w_alt_fnum = buf->b_fnum;
	return;
    }

#ifdef FEAT_EVAL
    if (name == '=')
    {
	char_u	    *p, *s;

	p = vim_strnsave(str, (int)len);
	if (p == NULL)
	    return;
	if (must_append)
	{
	    s = concat_str(get_expr_line_src(), p);
	    vim_free(p);
	    p = s;
	}
	set_expr_line(p);
	return;
    }
#endif

    if (name == '_')	    /* black hole: nothing to do */
	return;

    if (init_write_reg(name, &old_y_previous, &old_y_current, must_append,
		&yank_type) == FAIL)
	return;

    str_to_reg(y_current, yank_type, str, len, block_len, FALSE);

    finish_write_reg(name, old_y_previous, old_y_current);
}
#endif	/* FEAT_EVAL */

#if defined(FEAT_CLIPBOARD) || defined(FEAT_EVAL)
/*
 * Put a string into a register.  When the register is not empty, the string
 * is appended.
 */
    static void
str_to_reg(
    yankreg_T	*y_ptr,		/* pointer to yank register */
    int		yank_type,	/* MCHAR, MLINE, MBLOCK, MAUTO */
    char_u	*str,		/* string to put in register */
    long	len,		/* length of string */
    long	blocklen,	/* width of Visual block */
    int		str_list)	/* TRUE if str is char_u ** */
{
    int		type;			/* MCHAR, MLINE or MBLOCK */
    int		lnum;
    long	start;
    long	i;
    int		extra;
    int		newlines;		/* number of lines added */
    int		extraline = 0;		/* extra line at the end */
    int		append = FALSE;		/* append to last line in register */
    char_u	*s;
    char_u	**ss;
    char_u	**pp;
    long	maxlen;

    if (y_ptr->y_array == NULL)		/* NULL means empty register */
	y_ptr->y_size = 0;

    if (yank_type == MAUTO)
	type = ((str_list || (len > 0 && (str[len - 1] == NL
					    || str[len - 1] == CAR)))
							     ? MLINE : MCHAR);
    else
	type = yank_type;

    /*
     * Count the number of lines within the string
     */
    newlines = 0;
    if (str_list)
    {
	for (ss = (char_u **) str; *ss != NULL; ++ss)
	    ++newlines;
    }
    else
    {
	for (i = 0; i < len; i++)
	    if (str[i] == '\n')
		++newlines;
	if (type == MCHAR || len == 0 || str[len - 1] != '\n')
	{
	    extraline = 1;
	    ++newlines;	/* count extra newline at the end */
	}
	if (y_ptr->y_size > 0 && y_ptr->y_type == MCHAR)
	{
	    append = TRUE;
	    --newlines;	/* uncount newline when appending first line */
	}
    }

    /* Without any lines make the register empty. */
    if (y_ptr->y_size + newlines == 0)
    {
	VIM_CLEAR(y_ptr->y_array);
	return;
    }

    /*
     * Allocate an array to hold the pointers to the new register lines.
     * If the register was not empty, move the existing lines to the new array.
     */
    pp = (char_u **)lalloc_clear((y_ptr->y_size + newlines)
						    * sizeof(char_u *), TRUE);
    if (pp == NULL)	/* out of memory */
	return;
    for (lnum = 0; lnum < y_ptr->y_size; ++lnum)
	pp[lnum] = y_ptr->y_array[lnum];
    vim_free(y_ptr->y_array);
    y_ptr->y_array = pp;
    maxlen = 0;

    /*
     * Find the end of each line and save it into the array.
     */
    if (str_list)
    {
	for (ss = (char_u **) str; *ss != NULL; ++ss, ++lnum)
	{
	    i = (long)STRLEN(*ss);
	    pp[lnum] = vim_strnsave(*ss, i);
	    if (i > maxlen)
		maxlen = i;
	}
    }
    else
    {
	for (start = 0; start < len + extraline; start += i + 1)
	{
	    for (i = start; i < len; ++i)	/* find the end of the line */
		if (str[i] == '\n')
		    break;
	    i -= start;			/* i is now length of line */
	    if (i > maxlen)
		maxlen = i;
	    if (append)
	    {
		--lnum;
		extra = (int)STRLEN(y_ptr->y_array[lnum]);
	    }
	    else
		extra = 0;
	    s = alloc((unsigned)(i + extra + 1));
	    if (s == NULL)
		break;
	    if (extra)
		mch_memmove(s, y_ptr->y_array[lnum], (size_t)extra);
	    if (append)
		vim_free(y_ptr->y_array[lnum]);
	    if (i)
		mch_memmove(s + extra, str + start, (size_t)i);
	    extra += i;
	    s[extra] = NUL;
	    y_ptr->y_array[lnum++] = s;
	    while (--extra >= 0)
	    {
		if (*s == NUL)
		    *s = '\n';	    /* replace NUL with newline */
		++s;
	    }
	    append = FALSE;		    /* only first line is appended */
	}
    }
    y_ptr->y_type = type;
    y_ptr->y_size = lnum;
    if (type == MBLOCK)
	y_ptr->y_width = (blocklen < 0 ? maxlen - 1 : blocklen);
    else
	y_ptr->y_width = 0;
#ifdef FEAT_VIMINFO
    y_ptr->y_time_set = vim_time();
#endif
}
#endif /* FEAT_CLIPBOARD || FEAT_EVAL || PROTO */

    void
clear_oparg(oparg_T *oap)
{
    vim_memset(oap, 0, sizeof(oparg_T));
}

/*
 *  Count the number of bytes, characters and "words" in a line.
 *
 *  "Words" are counted by looking for boundaries between non-space and
 *  space characters.  (it seems to produce results that match 'wc'.)
 *
 *  Return value is byte count; word count for the line is added to "*wc".
 *  Char count is added to "*cc".
 *
 *  The function will only examine the first "limit" characters in the
 *  line, stopping if it encounters an end-of-line (NUL byte).  In that
 *  case, eol_size will be added to the character count to account for
 *  the size of the EOL character.
 */
    static varnumber_T
line_count_info(
    char_u	*line,
    varnumber_T	*wc,
    varnumber_T	*cc,
    varnumber_T	limit,
    int		eol_size)
{
    varnumber_T	i;
    varnumber_T	words = 0;
    varnumber_T	chars = 0;
    int		is_word = 0;

    for (i = 0; i < limit && line[i] != NUL; )
    {
	if (is_word)
	{
	    if (vim_isspace(line[i]))
	    {
		words++;
		is_word = 0;
	    }
	}
	else if (!vim_isspace(line[i]))
	    is_word = 1;
	++chars;
	i += (*mb_ptr2len)(line + i);
    }

    if (is_word)
	words++;
    *wc += words;

    /* Add eol_size if the end of line was reached before hitting limit. */
    if (i < limit && line[i] == NUL)
    {
	i += eol_size;
	chars += eol_size;
    }
    *cc += chars;
    return i;
}

/*
 * Give some info about the position of the cursor (for "g CTRL-G").
 * In Visual mode, give some info about the selected region.  (In this case,
 * the *_count_cursor variables store running totals for the selection.)
 * When "dict" is not NULL store the info there instead of showing it.
 */
    void
cursor_pos_info(dict_T *dict)
{
    char_u	*p;
    char_u	buf1[50];
    char_u	buf2[40];
    linenr_T	lnum;
    varnumber_T	byte_count = 0;
    varnumber_T	bom_count  = 0;
    varnumber_T	byte_count_cursor = 0;
    varnumber_T	char_count = 0;
    varnumber_T	char_count_cursor = 0;
    varnumber_T	word_count = 0;
    varnumber_T	word_count_cursor = 0;
    int		eol_size;
    varnumber_T	last_check = 100000L;
    long	line_count_selected = 0;
    pos_T	min_pos, max_pos;
    oparg_T	oparg;
    struct block_def	bd;

    /*
     * Compute the length of the file in characters.
     */
    if (curbuf->b_ml.ml_flags & ML_EMPTY)
    {
	if (dict == NULL)
	{
	    msg(_(no_lines_msg));
	    return;
	}
    }
    else
    {
	if (get_fileformat(curbuf) == EOL_DOS)
	    eol_size = 2;
	else
	    eol_size = 1;

	if (VIsual_active)
	{
	    if (LT_POS(VIsual, curwin->w_cursor))
	    {
		min_pos = VIsual;
		max_pos = curwin->w_cursor;
	    }
	    else
	    {
		min_pos = curwin->w_cursor;
		max_pos = VIsual;
	    }
	    if (*p_sel == 'e' && max_pos.col > 0)
		--max_pos.col;

	    if (VIsual_mode == Ctrl_V)
	    {
#ifdef FEAT_LINEBREAK
		char_u * saved_sbr = p_sbr;

		/* Make 'sbr' empty for a moment to get the correct size. */
		p_sbr = empty_option;
#endif
		oparg.is_VIsual = 1;
		oparg.block_mode = TRUE;
		oparg.op_type = OP_NOP;
		getvcols(curwin, &min_pos, &max_pos,
					  &oparg.start_vcol, &oparg.end_vcol);
#ifdef FEAT_LINEBREAK
		p_sbr = saved_sbr;
#endif
		if (curwin->w_curswant == MAXCOL)
		    oparg.end_vcol = MAXCOL;
		/* Swap the start, end vcol if needed */
		if (oparg.end_vcol < oparg.start_vcol)
		{
		    oparg.end_vcol += oparg.start_vcol;
		    oparg.start_vcol = oparg.end_vcol - oparg.start_vcol;
		    oparg.end_vcol -= oparg.start_vcol;
		}
	    }
	    line_count_selected = max_pos.lnum - min_pos.lnum + 1;
	}

	for (lnum = 1; lnum <= curbuf->b_ml.ml_line_count; ++lnum)
	{
	    /* Check for a CTRL-C every 100000 characters. */
	    if (byte_count > last_check)
	    {
		ui_breakcheck();
		if (got_int)
		    return;
		last_check = byte_count + 100000L;
	    }

	    /* Do extra processing for VIsual mode. */
	    if (VIsual_active
		    && lnum >= min_pos.lnum && lnum <= max_pos.lnum)
	    {
		char_u	    *s = NULL;
		long	    len = 0L;

		switch (VIsual_mode)
		{
		    case Ctrl_V:
			virtual_op = virtual_active();
			block_prep(&oparg, &bd, lnum, 0);
			virtual_op = MAYBE;
			s = bd.textstart;
			len = (long)bd.textlen;
			break;
		    case 'V':
			s = ml_get(lnum);
			len = MAXCOL;
			break;
		    case 'v':
			{
			    colnr_T start_col = (lnum == min_pos.lnum)
							   ? min_pos.col : 0;
			    colnr_T end_col = (lnum == max_pos.lnum)
				      ? max_pos.col - start_col + 1 : MAXCOL;

			    s = ml_get(lnum) + start_col;
			    len = end_col;
			}
			break;
		}
		if (s != NULL)
		{
		    byte_count_cursor += line_count_info(s, &word_count_cursor,
					   &char_count_cursor, len, eol_size);
		    if (lnum == curbuf->b_ml.ml_line_count
			    && !curbuf->b_p_eol
			    && (curbuf->b_p_bin || !curbuf->b_p_fixeol)
			    && (long)STRLEN(s) < len)
			byte_count_cursor -= eol_size;
		}
	    }
	    else
	    {
		/* In non-visual mode, check for the line the cursor is on */
		if (lnum == curwin->w_cursor.lnum)
		{
		    word_count_cursor += word_count;
		    char_count_cursor += char_count;
		    byte_count_cursor = byte_count +
			line_count_info(ml_get(lnum),
				&word_count_cursor, &char_count_cursor,
				(varnumber_T)(curwin->w_cursor.col + 1),
				eol_size);
		}
	    }
	    /* Add to the running totals */
	    byte_count += line_count_info(ml_get(lnum), &word_count,
					 &char_count, (varnumber_T)MAXCOL,
					 eol_size);
	}

	/* Correction for when last line doesn't have an EOL. */
	if (!curbuf->b_p_eol && (curbuf->b_p_bin || !curbuf->b_p_fixeol))
	    byte_count -= eol_size;

	if (dict == NULL)
	{
	    if (VIsual_active)
	    {
		if (VIsual_mode == Ctrl_V && curwin->w_curswant < MAXCOL)
		{
		    getvcols(curwin, &min_pos, &max_pos, &min_pos.col,
								    &max_pos.col);
		    vim_snprintf((char *)buf1, sizeof(buf1), _("%ld Cols; "),
			    (long)(oparg.end_vcol - oparg.start_vcol + 1));
		}
		else
		    buf1[0] = NUL;

		if (char_count_cursor == byte_count_cursor
						    && char_count == byte_count)
		    vim_snprintf((char *)IObuff, IOSIZE,
			    _("Selected %s%ld of %ld Lines; %lld of %lld Words; %lld of %lld Bytes"),
			    buf1, line_count_selected,
			    (long)curbuf->b_ml.ml_line_count,
			    (long_long_T)word_count_cursor,
			    (long_long_T)word_count,
			    (long_long_T)byte_count_cursor,
			    (long_long_T)byte_count);
		else
		    vim_snprintf((char *)IObuff, IOSIZE,
			    _("Selected %s%ld of %ld Lines; %lld of %lld Words; %lld of %lld Chars; %lld of %lld Bytes"),
			    buf1, line_count_selected,
			    (long)curbuf->b_ml.ml_line_count,
			    (long_long_T)word_count_cursor,
			    (long_long_T)word_count,
			    (long_long_T)char_count_cursor,
			    (long_long_T)char_count,
			    (long_long_T)byte_count_cursor,
			    (long_long_T)byte_count);
	    }
	    else
	    {
		p = ml_get_curline();
		validate_virtcol();
		col_print(buf1, sizeof(buf1), (int)curwin->w_cursor.col + 1,
			(int)curwin->w_virtcol + 1);
		col_print(buf2, sizeof(buf2), (int)STRLEN(p),
				    linetabsize(p));

		if (char_count_cursor == byte_count_cursor
			&& char_count == byte_count)
		    vim_snprintf((char *)IObuff, IOSIZE,
			_("Col %s of %s; Line %ld of %ld; Word %lld of %lld; Byte %lld of %lld"),
			(char *)buf1, (char *)buf2,
			(long)curwin->w_cursor.lnum,
			(long)curbuf->b_ml.ml_line_count,
			(long_long_T)word_count_cursor, (long_long_T)word_count,
			(long_long_T)byte_count_cursor, (long_long_T)byte_count);
		else
		    vim_snprintf((char *)IObuff, IOSIZE,
			_("Col %s of %s; Line %ld of %ld; Word %lld of %lld; Char %lld of %lld; Byte %lld of %lld"),
			(char *)buf1, (char *)buf2,
			(long)curwin->w_cursor.lnum,
			(long)curbuf->b_ml.ml_line_count,
			(long_long_T)word_count_cursor, (long_long_T)word_count,
			(long_long_T)char_count_cursor, (long_long_T)char_count,
			(long_long_T)byte_count_cursor, (long_long_T)byte_count);
	    }
	}

	bom_count = bomb_size();
	if (bom_count > 0)
	    vim_snprintf((char *)IObuff + STRLEN(IObuff), IOSIZE,
				 _("(+%lld for BOM)"), (long_long_T)bom_count);
	if (dict == NULL)
	{
	    /* Don't shorten this message, the user asked for it. */
	    p = p_shm;
	    p_shm = (char_u *)"";
	    msg((char *)IObuff);
	    p_shm = p;
	}
    }
#if defined(FEAT_EVAL)
    if (dict != NULL)
    {
	dict_add_number(dict, "words", word_count);
	dict_add_number(dict, "chars", char_count);
	dict_add_number(dict, "bytes", byte_count + bom_count);
	dict_add_number(dict, VIsual_active ? "visual_bytes" : "cursor_bytes",
		byte_count_cursor);
	dict_add_number(dict, VIsual_active ? "visual_chars" : "cursor_chars",
		char_count_cursor);
	dict_add_number(dict, VIsual_active ? "visual_words" : "cursor_words",
		word_count_cursor);
    }
#endif
}
