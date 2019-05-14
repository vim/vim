/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * misc1.c: functions that didn't seem to fit elsewhere
 */

#include "vim.h"
#include "version.h"

#if defined(FEAT_CMDL_COMPL) && defined(MSWIN)
# include <lm.h>
#endif

static char_u *vim_version_dir(char_u *vimdir);
static char_u *remove_tail(char_u *p, char_u *pend, char_u *name);

#define URL_SLASH	1		/* path_is_url() has found "://" */
#define URL_BACKSLASH	2		/* path_is_url() has found ":\\" */

/* All user names (for ~user completion as done by shell). */
#if defined(FEAT_CMDL_COMPL) || defined(PROTO)
static garray_T	ga_users;
#endif

/*
 * Count the size (in window cells) of the indent in the current line.
 */
    int
get_indent(void)
{
#ifdef FEAT_VARTABS
    return get_indent_str_vtab(ml_get_curline(), (int)curbuf->b_p_ts,
						 curbuf->b_p_vts_array, FALSE);
#else
    return get_indent_str(ml_get_curline(), (int)curbuf->b_p_ts, FALSE);
#endif
}

/*
 * Count the size (in window cells) of the indent in line "lnum".
 */
    int
get_indent_lnum(linenr_T lnum)
{
#ifdef FEAT_VARTABS
    return get_indent_str_vtab(ml_get(lnum), (int)curbuf->b_p_ts,
						 curbuf->b_p_vts_array, FALSE);
#else
    return get_indent_str(ml_get(lnum), (int)curbuf->b_p_ts, FALSE);
#endif
}

#if defined(FEAT_FOLDING) || defined(PROTO)
/*
 * Count the size (in window cells) of the indent in line "lnum" of buffer
 * "buf".
 */
    int
get_indent_buf(buf_T *buf, linenr_T lnum)
{
#ifdef FEAT_VARTABS
    return get_indent_str_vtab(ml_get_buf(buf, lnum, FALSE),
			       (int)curbuf->b_p_ts, buf->b_p_vts_array, FALSE);
#else
    return get_indent_str(ml_get_buf(buf, lnum, FALSE), (int)buf->b_p_ts, FALSE);
#endif
}
#endif

/*
 * count the size (in window cells) of the indent in line "ptr", with
 * 'tabstop' at "ts"
 */
    int
get_indent_str(
    char_u	*ptr,
    int		ts,
    int		list) /* if TRUE, count only screen size for tabs */
{
    int		count = 0;

    for ( ; *ptr; ++ptr)
    {
	if (*ptr == TAB)
	{
	    if (!list || lcs_tab1)    /* count a tab for what it is worth */
		count += ts - (count % ts);
	    else
		/* In list mode, when tab is not set, count screen char width
		 * for Tab, displays: ^I */
		count += ptr2cells(ptr);
	}
	else if (*ptr == ' ')
	    ++count;		/* count a space for one */
	else
	    break;
    }
    return count;
}

#ifdef FEAT_VARTABS
/*
 * Count the size (in window cells) of the indent in line "ptr", using
 * variable tabstops.
 * if "list" is TRUE, count only screen size for tabs.
 */
    int
get_indent_str_vtab(char_u *ptr, int ts, int *vts, int list)
{
    int		count = 0;

    for ( ; *ptr; ++ptr)
    {
	if (*ptr == TAB)    /* count a tab for what it is worth */
	{
	    if (!list || lcs_tab1)
		count += tabstop_padding(count, ts, vts);
	    else
		/* In list mode, when tab is not set, count screen char width
		 * for Tab, displays: ^I */
		count += ptr2cells(ptr);
	}
	else if (*ptr == ' ')
	    ++count;		/* count a space for one */
	else
	    break;
    }
    return count;
}
#endif

/*
 * Set the indent of the current line.
 * Leaves the cursor on the first non-blank in the line.
 * Caller must take care of undo.
 * "flags":
 *	SIN_CHANGED:	call changed_bytes() if the line was changed.
 *	SIN_INSERT:	insert the indent in front of the line.
 *	SIN_UNDO:	save line for undo before changing it.
 * Returns TRUE if the line was changed.
 */
    int
set_indent(
    int		size,		    /* measured in spaces */
    int		flags)
{
    char_u	*p;
    char_u	*newline;
    char_u	*oldline;
    char_u	*s;
    int		todo;
    int		ind_len;	    /* measured in characters */
    int		line_len;
    int		doit = FALSE;
    int		ind_done = 0;	    /* measured in spaces */
#ifdef FEAT_VARTABS
    int		ind_col = 0;
#endif
    int		tab_pad;
    int		retval = FALSE;
    int		orig_char_len = -1; /* number of initial whitespace chars when
				       'et' and 'pi' are both set */

    /*
     * First check if there is anything to do and compute the number of
     * characters needed for the indent.
     */
    todo = size;
    ind_len = 0;
    p = oldline = ml_get_curline();

    /* Calculate the buffer size for the new indent, and check to see if it
     * isn't already set */

    /* if 'expandtab' isn't set: use TABs; if both 'expandtab' and
     * 'preserveindent' are set count the number of characters at the
     * beginning of the line to be copied */
    if (!curbuf->b_p_et || (!(flags & SIN_INSERT) && curbuf->b_p_pi))
    {
	/* If 'preserveindent' is set then reuse as much as possible of
	 * the existing indent structure for the new indent */
	if (!(flags & SIN_INSERT) && curbuf->b_p_pi)
	{
	    ind_done = 0;

	    /* count as many characters as we can use */
	    while (todo > 0 && VIM_ISWHITE(*p))
	    {
		if (*p == TAB)
		{
#ifdef FEAT_VARTABS
		    tab_pad = tabstop_padding(ind_done, curbuf->b_p_ts,
							curbuf->b_p_vts_array);
#else
		    tab_pad = (int)curbuf->b_p_ts
					   - (ind_done % (int)curbuf->b_p_ts);
#endif
		    /* stop if this tab will overshoot the target */
		    if (todo < tab_pad)
			break;
		    todo -= tab_pad;
		    ++ind_len;
		    ind_done += tab_pad;
		}
		else
		{
		    --todo;
		    ++ind_len;
		    ++ind_done;
		}
		++p;
	    }

#ifdef FEAT_VARTABS
	    /* These diverge from this point. */
	    ind_col = ind_done;
#endif
	    /* Set initial number of whitespace chars to copy if we are
	     * preserving indent but expandtab is set */
	    if (curbuf->b_p_et)
		orig_char_len = ind_len;

	    /* Fill to next tabstop with a tab, if possible */
#ifdef FEAT_VARTABS
	    tab_pad = tabstop_padding(ind_done, curbuf->b_p_ts,
						curbuf->b_p_vts_array);
#else
	    tab_pad = (int)curbuf->b_p_ts - (ind_done % (int)curbuf->b_p_ts);
#endif
	    if (todo >= tab_pad && orig_char_len == -1)
	    {
		doit = TRUE;
		todo -= tab_pad;
		++ind_len;
		/* ind_done += tab_pad; */
#ifdef FEAT_VARTABS
		ind_col += tab_pad;
#endif
	    }
	}

	/* count tabs required for indent */
#ifdef FEAT_VARTABS
	for (;;)
	{
	    tab_pad = tabstop_padding(ind_col, curbuf->b_p_ts,
							curbuf->b_p_vts_array);
	    if (todo < tab_pad)
		break;
	    if (*p != TAB)
		doit = TRUE;
	    else
		++p;
	    todo -= tab_pad;
	    ++ind_len;
	    ind_col += tab_pad;
	}
#else
	while (todo >= (int)curbuf->b_p_ts)
	{
	    if (*p != TAB)
		doit = TRUE;
	    else
		++p;
	    todo -= (int)curbuf->b_p_ts;
	    ++ind_len;
	    /* ind_done += (int)curbuf->b_p_ts; */
	}
#endif
    }
    /* count spaces required for indent */
    while (todo > 0)
    {
	if (*p != ' ')
	    doit = TRUE;
	else
	    ++p;
	--todo;
	++ind_len;
	/* ++ind_done; */
    }

    /* Return if the indent is OK already. */
    if (!doit && !VIM_ISWHITE(*p) && !(flags & SIN_INSERT))
	return FALSE;

    /* Allocate memory for the new line. */
    if (flags & SIN_INSERT)
	p = oldline;
    else
	p = skipwhite(p);
    line_len = (int)STRLEN(p) + 1;

    /* If 'preserveindent' and 'expandtab' are both set keep the original
     * characters and allocate accordingly.  We will fill the rest with spaces
     * after the if (!curbuf->b_p_et) below. */
    if (orig_char_len != -1)
    {
	newline = alloc(orig_char_len + size - ind_done + line_len);
	if (newline == NULL)
	    return FALSE;
	todo = size - ind_done;
	ind_len = orig_char_len + todo;    /* Set total length of indent in
					    * characters, which may have been
					    * undercounted until now  */
	p = oldline;
	s = newline;
	while (orig_char_len > 0)
	{
	    *s++ = *p++;
	    orig_char_len--;
	}

	/* Skip over any additional white space (useful when newindent is less
	 * than old) */
	while (VIM_ISWHITE(*p))
	    ++p;

    }
    else
    {
	todo = size;
	newline = alloc(ind_len + line_len);
	if (newline == NULL)
	    return FALSE;
	s = newline;
    }

    /* Put the characters in the new line. */
    /* if 'expandtab' isn't set: use TABs */
    if (!curbuf->b_p_et)
    {
	/* If 'preserveindent' is set then reuse as much as possible of
	 * the existing indent structure for the new indent */
	if (!(flags & SIN_INSERT) && curbuf->b_p_pi)
	{
	    p = oldline;
	    ind_done = 0;

	    while (todo > 0 && VIM_ISWHITE(*p))
	    {
		if (*p == TAB)
		{
#ifdef FEAT_VARTABS
		    tab_pad = tabstop_padding(ind_done, curbuf->b_p_ts,
							curbuf->b_p_vts_array);
#else
		    tab_pad = (int)curbuf->b_p_ts
					   - (ind_done % (int)curbuf->b_p_ts);
#endif
		    /* stop if this tab will overshoot the target */
		    if (todo < tab_pad)
			break;
		    todo -= tab_pad;
		    ind_done += tab_pad;
		}
		else
		{
		    --todo;
		    ++ind_done;
		}
		*s++ = *p++;
	    }

	    /* Fill to next tabstop with a tab, if possible */
#ifdef FEAT_VARTABS
	    tab_pad = tabstop_padding(ind_done, curbuf->b_p_ts,
						curbuf->b_p_vts_array);
#else
	    tab_pad = (int)curbuf->b_p_ts - (ind_done % (int)curbuf->b_p_ts);
#endif
	    if (todo >= tab_pad)
	    {
		*s++ = TAB;
		todo -= tab_pad;
#ifdef FEAT_VARTABS
		ind_done += tab_pad;
#endif
	    }

	    p = skipwhite(p);
	}

#ifdef FEAT_VARTABS
	for (;;)
	{
	    tab_pad = tabstop_padding(ind_done, curbuf->b_p_ts,
							curbuf->b_p_vts_array);
	    if (todo < tab_pad)
		break;
	    *s++ = TAB;
	    todo -= tab_pad;
	    ind_done += tab_pad;
	}
#else
	while (todo >= (int)curbuf->b_p_ts)
	{
	    *s++ = TAB;
	    todo -= (int)curbuf->b_p_ts;
	}
#endif
    }
    while (todo > 0)
    {
	*s++ = ' ';
	--todo;
    }
    mch_memmove(s, p, (size_t)line_len);

    // Replace the line (unless undo fails).
    if (!(flags & SIN_UNDO) || u_savesub(curwin->w_cursor.lnum) == OK)
    {
	ml_replace(curwin->w_cursor.lnum, newline, FALSE);
	if (flags & SIN_CHANGED)
	    changed_bytes(curwin->w_cursor.lnum, 0);

	// Correct saved cursor position if it is in this line.
	if (saved_cursor.lnum == curwin->w_cursor.lnum)
	{
	    if (saved_cursor.col >= (colnr_T)(p - oldline))
		// cursor was after the indent, adjust for the number of
		// bytes added/removed
		saved_cursor.col += ind_len - (colnr_T)(p - oldline);
	    else if (saved_cursor.col >= (colnr_T)(s - newline))
		// cursor was in the indent, and is now after it, put it back
		// at the start of the indent (replacing spaces with TAB)
		saved_cursor.col = (colnr_T)(s - newline);
	}
#ifdef FEAT_TEXT_PROP
	adjust_prop_columns(curwin->w_cursor.lnum, (colnr_T)(p - oldline),
					     ind_len - (colnr_T)(p - oldline));
#endif
	retval = TRUE;
    }
    else
	vim_free(newline);

    curwin->w_cursor.col = ind_len;
    return retval;
}

/*
 * Return the indent of the current line after a number.  Return -1 if no
 * number was found.  Used for 'n' in 'formatoptions': numbered list.
 * Since a pattern is used it can actually handle more than numbers.
 */
    int
get_number_indent(linenr_T lnum)
{
    colnr_T	col;
    pos_T	pos;

    regmatch_T	regmatch;
    int		lead_len = 0;	/* length of comment leader */

    if (lnum > curbuf->b_ml.ml_line_count)
	return -1;
    pos.lnum = 0;

#ifdef FEAT_COMMENTS
    /* In format_lines() (i.e. not insert mode), fo+=q is needed too...  */
    if ((State & INSERT) || has_format_option(FO_Q_COMS))
	lead_len = get_leader_len(ml_get(lnum), NULL, FALSE, TRUE);
#endif
    regmatch.regprog = vim_regcomp(curbuf->b_p_flp, RE_MAGIC);
    if (regmatch.regprog != NULL)
    {
	regmatch.rm_ic = FALSE;

	/* vim_regexec() expects a pointer to a line.  This lets us
	 * start matching for the flp beyond any comment leader...  */
	if (vim_regexec(&regmatch, ml_get(lnum) + lead_len, (colnr_T)0))
	{
	    pos.lnum = lnum;
	    pos.col = (colnr_T)(*regmatch.endp - ml_get(lnum));
	    pos.coladd = 0;
	}
	vim_regfree(regmatch.regprog);
    }

    if (pos.lnum == 0 || *ml_get_pos(&pos) == NUL)
	return -1;
    getvcol(curwin, &pos, &col, NULL, NULL);
    return (int)col;
}

#if defined(FEAT_LINEBREAK) || defined(PROTO)
/*
 * Return appropriate space number for breakindent, taking influencing
 * parameters into account. Window must be specified, since it is not
 * necessarily always the current one.
 */
    int
get_breakindent_win(
    win_T	*wp,
    char_u	*line) /* start of the line */
{
    static int	    prev_indent = 0;  /* cached indent value */
    static long	    prev_ts     = 0L; /* cached tabstop value */
    static char_u   *prev_line = NULL; /* cached pointer to line */
    static varnumber_T prev_tick = 0;   /* changedtick of cached value */
#ifdef FEAT_VARTABS
    static int      *prev_vts = NULL;    /* cached vartabs values */
#endif
    int		    bri = 0;
    /* window width minus window margin space, i.e. what rests for text */
    const int	    eff_wwidth = wp->w_width
			    - ((wp->w_p_nu || wp->w_p_rnu)
				&& (vim_strchr(p_cpo, CPO_NUMCOL) == NULL)
						? number_width(wp) + 1 : 0);

    /* used cached indent, unless pointer or 'tabstop' changed */
    if (prev_line != line || prev_ts != wp->w_buffer->b_p_ts
	    || prev_tick != CHANGEDTICK(wp->w_buffer)
#ifdef FEAT_VARTABS
	    || prev_vts != wp->w_buffer->b_p_vts_array
#endif
	)
    {
	prev_line = line;
	prev_ts = wp->w_buffer->b_p_ts;
	prev_tick = CHANGEDTICK(wp->w_buffer);
#ifdef FEAT_VARTABS
	prev_vts = wp->w_buffer->b_p_vts_array;
	prev_indent = get_indent_str_vtab(line,
				     (int)wp->w_buffer->b_p_ts,
				    wp->w_buffer->b_p_vts_array, wp->w_p_list);
#else
	prev_indent = get_indent_str(line,
				     (int)wp->w_buffer->b_p_ts, wp->w_p_list);
#endif
    }
    bri = prev_indent + wp->w_p_brishift;

    /* indent minus the length of the showbreak string */
    if (wp->w_p_brisbr)
	bri -= vim_strsize(p_sbr);

    /* Add offset for number column, if 'n' is in 'cpoptions' */
    bri += win_col_off2(wp);

    /* never indent past left window margin */
    if (bri < 0)
	bri = 0;
    /* always leave at least bri_min characters on the left,
     * if text width is sufficient */
    else if (bri > eff_wwidth - wp->w_p_brimin)
	bri = (eff_wwidth - wp->w_p_brimin < 0)
			    ? 0 : eff_wwidth - wp->w_p_brimin;

    return bri;
}
#endif

#if defined(FEAT_COMMENTS) || defined(PROTO)
/*
 * get_leader_len() returns the length in bytes of the prefix of the given
 * string which introduces a comment.  If this string is not a comment then
 * 0 is returned.
 * When "flags" is not NULL, it is set to point to the flags of the recognized
 * comment leader.
 * "backward" must be true for the "O" command.
 * If "include_space" is set, include trailing whitespace while calculating the
 * length.
 */
    int
get_leader_len(
    char_u	*line,
    char_u	**flags,
    int		backward,
    int		include_space)
{
    int		i, j;
    int		result;
    int		got_com = FALSE;
    int		found_one;
    char_u	part_buf[COM_MAX_LEN];	/* buffer for one option part */
    char_u	*string;		/* pointer to comment string */
    char_u	*list;
    int		middle_match_len = 0;
    char_u	*prev_list;
    char_u	*saved_flags = NULL;

    result = i = 0;
    while (VIM_ISWHITE(line[i]))    /* leading white space is ignored */
	++i;

    /*
     * Repeat to match several nested comment strings.
     */
    while (line[i] != NUL)
    {
	/*
	 * scan through the 'comments' option for a match
	 */
	found_one = FALSE;
	for (list = curbuf->b_p_com; *list; )
	{
	    /* Get one option part into part_buf[].  Advance "list" to next
	     * one.  Put "string" at start of string.  */
	    if (!got_com && flags != NULL)
		*flags = list;	    /* remember where flags started */
	    prev_list = list;
	    (void)copy_option_part(&list, part_buf, COM_MAX_LEN, ",");
	    string = vim_strchr(part_buf, ':');
	    if (string == NULL)	    /* missing ':', ignore this part */
		continue;
	    *string++ = NUL;	    /* isolate flags from string */

	    /* If we found a middle match previously, use that match when this
	     * is not a middle or end. */
	    if (middle_match_len != 0
		    && vim_strchr(part_buf, COM_MIDDLE) == NULL
		    && vim_strchr(part_buf, COM_END) == NULL)
		break;

	    /* When we already found a nested comment, only accept further
	     * nested comments. */
	    if (got_com && vim_strchr(part_buf, COM_NEST) == NULL)
		continue;

	    /* When 'O' flag present and using "O" command skip this one. */
	    if (backward && vim_strchr(part_buf, COM_NOBACK) != NULL)
		continue;

	    /* Line contents and string must match.
	     * When string starts with white space, must have some white space
	     * (but the amount does not need to match, there might be a mix of
	     * TABs and spaces). */
	    if (VIM_ISWHITE(string[0]))
	    {
		if (i == 0 || !VIM_ISWHITE(line[i - 1]))
		    continue;  /* missing white space */
		while (VIM_ISWHITE(string[0]))
		    ++string;
	    }
	    for (j = 0; string[j] != NUL && string[j] == line[i + j]; ++j)
		;
	    if (string[j] != NUL)
		continue;  /* string doesn't match */

	    /* When 'b' flag used, there must be white space or an
	     * end-of-line after the string in the line. */
	    if (vim_strchr(part_buf, COM_BLANK) != NULL
			   && !VIM_ISWHITE(line[i + j]) && line[i + j] != NUL)
		continue;

	    /* We have found a match, stop searching unless this is a middle
	     * comment. The middle comment can be a substring of the end
	     * comment in which case it's better to return the length of the
	     * end comment and its flags.  Thus we keep searching with middle
	     * and end matches and use an end match if it matches better. */
	    if (vim_strchr(part_buf, COM_MIDDLE) != NULL)
	    {
		if (middle_match_len == 0)
		{
		    middle_match_len = j;
		    saved_flags = prev_list;
		}
		continue;
	    }
	    if (middle_match_len != 0 && j > middle_match_len)
		/* Use this match instead of the middle match, since it's a
		 * longer thus better match. */
		middle_match_len = 0;

	    if (middle_match_len == 0)
		i += j;
	    found_one = TRUE;
	    break;
	}

	if (middle_match_len != 0)
	{
	    /* Use the previously found middle match after failing to find a
	     * match with an end. */
	    if (!got_com && flags != NULL)
		*flags = saved_flags;
	    i += middle_match_len;
	    found_one = TRUE;
	}

	/* No match found, stop scanning. */
	if (!found_one)
	    break;

	result = i;

	/* Include any trailing white space. */
	while (VIM_ISWHITE(line[i]))
	    ++i;

	if (include_space)
	    result = i;

	/* If this comment doesn't nest, stop here. */
	got_com = TRUE;
	if (vim_strchr(part_buf, COM_NEST) == NULL)
	    break;
    }
    return result;
}

/*
 * Return the offset at which the last comment in line starts. If there is no
 * comment in the whole line, -1 is returned.
 *
 * When "flags" is not null, it is set to point to the flags describing the
 * recognized comment leader.
 */
    int
get_last_leader_offset(char_u *line, char_u **flags)
{
    int		result = -1;
    int		i, j;
    int		lower_check_bound = 0;
    char_u	*string;
    char_u	*com_leader;
    char_u	*com_flags;
    char_u	*list;
    int		found_one;
    char_u	part_buf[COM_MAX_LEN];	/* buffer for one option part */

    /*
     * Repeat to match several nested comment strings.
     */
    i = (int)STRLEN(line);
    while (--i >= lower_check_bound)
    {
	/*
	 * scan through the 'comments' option for a match
	 */
	found_one = FALSE;
	for (list = curbuf->b_p_com; *list; )
	{
	    char_u *flags_save = list;

	    /*
	     * Get one option part into part_buf[].  Advance list to next one.
	     * put string at start of string.
	     */
	    (void)copy_option_part(&list, part_buf, COM_MAX_LEN, ",");
	    string = vim_strchr(part_buf, ':');
	    if (string == NULL)	/* If everything is fine, this cannot actually
				 * happen. */
		continue;
	    *string++ = NUL;	/* Isolate flags from string. */
	    com_leader = string;

	    /*
	     * Line contents and string must match.
	     * When string starts with white space, must have some white space
	     * (but the amount does not need to match, there might be a mix of
	     * TABs and spaces).
	     */
	    if (VIM_ISWHITE(string[0]))
	    {
		if (i == 0 || !VIM_ISWHITE(line[i - 1]))
		    continue;
		while (VIM_ISWHITE(*string))
		    ++string;
	    }
	    for (j = 0; string[j] != NUL && string[j] == line[i + j]; ++j)
		/* do nothing */;
	    if (string[j] != NUL)
		continue;

	    /*
	     * When 'b' flag used, there must be white space or an
	     * end-of-line after the string in the line.
	     */
	    if (vim_strchr(part_buf, COM_BLANK) != NULL
		    && !VIM_ISWHITE(line[i + j]) && line[i + j] != NUL)
		continue;

	    if (vim_strchr(part_buf, COM_MIDDLE) != NULL)
	    {
		// For a middlepart comment, only consider it to match if
		// everything before the current position in the line is
		// whitespace.  Otherwise we would think we are inside a
		// comment if the middle part appears somewhere in the middle
		// of the line.  E.g. for C the "*" appears often.
		for (j = 0; VIM_ISWHITE(line[j]) && j <= i; j++)
		    ;
		if (j < i)
		    continue;
	    }

	    /*
	     * We have found a match, stop searching.
	     */
	    found_one = TRUE;

	    if (flags)
		*flags = flags_save;
	    com_flags = flags_save;

	    break;
	}

	if (found_one)
	{
	    char_u  part_buf2[COM_MAX_LEN];	/* buffer for one option part */
	    int     len1, len2, off;

	    result = i;
	    /*
	     * If this comment nests, continue searching.
	     */
	    if (vim_strchr(part_buf, COM_NEST) != NULL)
		continue;

	    lower_check_bound = i;

	    /* Let's verify whether the comment leader found is a substring
	     * of other comment leaders. If it is, let's adjust the
	     * lower_check_bound so that we make sure that we have determined
	     * the comment leader correctly.
	     */

	    while (VIM_ISWHITE(*com_leader))
		++com_leader;
	    len1 = (int)STRLEN(com_leader);

	    for (list = curbuf->b_p_com; *list; )
	    {
		char_u *flags_save = list;

		(void)copy_option_part(&list, part_buf2, COM_MAX_LEN, ",");
		if (flags_save == com_flags)
		    continue;
		string = vim_strchr(part_buf2, ':');
		++string;
		while (VIM_ISWHITE(*string))
		    ++string;
		len2 = (int)STRLEN(string);
		if (len2 == 0)
		    continue;

		/* Now we have to verify whether string ends with a substring
		 * beginning the com_leader. */
		for (off = (len2 > i ? i : len2); off > 0 && off + len1 > len2;)
		{
		    --off;
		    if (!STRNCMP(string + off, com_leader, len2 - off))
		    {
			if (i - off < lower_check_bound)
			    lower_check_bound = i - off;
		    }
		}
	    }
	}
    }
    return result;
}
#endif

/*
 * Return the number of window lines occupied by buffer line "lnum".
 */
    int
plines(linenr_T lnum)
{
    return plines_win(curwin, lnum, TRUE);
}

    int
plines_win(
    win_T	*wp,
    linenr_T	lnum,
    int		winheight)	/* when TRUE limit to window height */
{
#if defined(FEAT_DIFF) || defined(PROTO)
    /* Check for filler lines above this buffer line.  When folded the result
     * is one line anyway. */
    return plines_win_nofill(wp, lnum, winheight) + diff_check_fill(wp, lnum);
}

    int
plines_nofill(linenr_T lnum)
{
    return plines_win_nofill(curwin, lnum, TRUE);
}

    int
plines_win_nofill(
    win_T	*wp,
    linenr_T	lnum,
    int		winheight)	/* when TRUE limit to window height */
{
#endif
    int		lines;

    if (!wp->w_p_wrap)
	return 1;

    if (wp->w_width == 0)
	return 1;

#ifdef FEAT_FOLDING
    /* A folded lines is handled just like an empty line. */
    /* NOTE: Caller must handle lines that are MAYBE folded. */
    if (lineFolded(wp, lnum) == TRUE)
	return 1;
#endif

    lines = plines_win_nofold(wp, lnum);
    if (winheight > 0 && lines > wp->w_height)
	return (int)wp->w_height;
    return lines;
}

/*
 * Return number of window lines physical line "lnum" will occupy in window
 * "wp".  Does not care about folding, 'wrap' or 'diff'.
 */
    int
plines_win_nofold(win_T *wp, linenr_T lnum)
{
    char_u	*s;
    long	col;
    int		width;

    s = ml_get_buf(wp->w_buffer, lnum, FALSE);
    if (*s == NUL)		/* empty line */
	return 1;
    col = win_linetabsize(wp, s, (colnr_T)MAXCOL);

    /*
     * If list mode is on, then the '$' at the end of the line may take up one
     * extra column.
     */
    if (wp->w_p_list && lcs_eol != NUL)
	col += 1;

    /*
     * Add column offset for 'number', 'relativenumber' and 'foldcolumn'.
     */
    width = wp->w_width - win_col_off(wp);
    if (width <= 0)
	return 32000;
    if (col <= width)
	return 1;
    col -= width;
    width += win_col_off2(wp);
    return (col + (width - 1)) / width + 1;
}

/*
 * Like plines_win(), but only reports the number of physical screen lines
 * used from the start of the line to the given column number.
 */
    int
plines_win_col(win_T *wp, linenr_T lnum, long column)
{
    long	col;
    char_u	*s;
    int		lines = 0;
    int		width;
    char_u	*line;

#ifdef FEAT_DIFF
    /* Check for filler lines above this buffer line.  When folded the result
     * is one line anyway. */
    lines = diff_check_fill(wp, lnum);
#endif

    if (!wp->w_p_wrap)
	return lines + 1;

    if (wp->w_width == 0)
	return lines + 1;

    line = s = ml_get_buf(wp->w_buffer, lnum, FALSE);

    col = 0;
    while (*s != NUL && --column >= 0)
    {
	col += win_lbr_chartabsize(wp, line, s, (colnr_T)col, NULL);
	MB_PTR_ADV(s);
    }

    /*
     * If *s is a TAB, and the TAB is not displayed as ^I, and we're not in
     * INSERT mode, then col must be adjusted so that it represents the last
     * screen position of the TAB.  This only fixes an error when the TAB wraps
     * from one screen line to the next (when 'columns' is not a multiple of
     * 'ts') -- webb.
     */
    if (*s == TAB && (State & NORMAL) && (!wp->w_p_list || lcs_tab1))
	col += win_lbr_chartabsize(wp, line, s, (colnr_T)col, NULL) - 1;

    /*
     * Add column offset for 'number', 'relativenumber', 'foldcolumn', etc.
     */
    width = wp->w_width - win_col_off(wp);
    if (width <= 0)
	return 9999;

    lines += 1;
    if (col > width)
	lines += (col - width) / (width + win_col_off2(wp)) + 1;
    return lines;
}

    int
plines_m_win(win_T *wp, linenr_T first, linenr_T last)
{
    int		count = 0;

    while (first <= last)
    {
#ifdef FEAT_FOLDING
	int	x;

	/* Check if there are any really folded lines, but also included lines
	 * that are maybe folded. */
	x = foldedCount(wp, first, NULL);
	if (x > 0)
	{
	    ++count;	    /* count 1 for "+-- folded" line */
	    first += x;
	}
	else
#endif
	{
#ifdef FEAT_DIFF
	    if (first == wp->w_topline)
		count += plines_win_nofill(wp, first, TRUE) + wp->w_topfill;
	    else
#endif
		count += plines_win(wp, first, TRUE);
	    ++first;
	}
    }
    return (count);
}

    int
gchar_pos(pos_T *pos)
{
    char_u	*ptr;

    /* When searching columns is sometimes put at the end of a line. */
    if (pos->col == MAXCOL)
	return NUL;
    ptr = ml_get_pos(pos);
    if (has_mbyte)
	return (*mb_ptr2char)(ptr);
    return (int)*ptr;
}

    int
gchar_cursor(void)
{
    if (has_mbyte)
	return (*mb_ptr2char)(ml_get_cursor());
    return (int)*ml_get_cursor();
}

/*
 * Write a character at the current cursor position.
 * It is directly written into the block.
 */
    void
pchar_cursor(int c)
{
    *(ml_get_buf(curbuf, curwin->w_cursor.lnum, TRUE)
						  + curwin->w_cursor.col) = c;
}

/*
 * When extra == 0: Return TRUE if the cursor is before or on the first
 *		    non-blank in the line.
 * When extra == 1: Return TRUE if the cursor is before the first non-blank in
 *		    the line.
 */
    int
inindent(int extra)
{
    char_u	*ptr;
    colnr_T	col;

    for (col = 0, ptr = ml_get_curline(); VIM_ISWHITE(*ptr); ++col)
	++ptr;
    if (col >= curwin->w_cursor.col + extra)
	return TRUE;
    else
	return FALSE;
}

/*
 * Skip to next part of an option argument: Skip space and comma.
 */
    char_u *
skip_to_option_part(char_u *p)
{
    if (*p == ',')
	++p;
    while (*p == ' ')
	++p;
    return p;
}

/*
 * check_status: called when the status bars for the buffer 'buf'
 *		 need to be updated
 */
    void
check_status(buf_T *buf)
{
    win_T	*wp;

    FOR_ALL_WINDOWS(wp)
	if (wp->w_buffer == buf && wp->w_status_height)
	{
	    wp->w_redr_status = TRUE;
	    if (must_redraw < VALID)
		must_redraw = VALID;
	}
}

/*
 * Ask for a reply from the user, a 'y' or a 'n'.
 * No other characters are accepted, the message is repeated until a valid
 * reply is entered or CTRL-C is hit.
 * If direct is TRUE, don't use vgetc() but ui_inchar(), don't get characters
 * from any buffers but directly from the user.
 *
 * return the 'y' or 'n'
 */
    int
ask_yesno(char_u *str, int direct)
{
    int	    r = ' ';
    int	    save_State = State;

    if (exiting)		/* put terminal in raw mode for this question */
	settmode(TMODE_RAW);
    ++no_wait_return;
#ifdef USE_ON_FLY_SCROLL
    dont_scroll = TRUE;		/* disallow scrolling here */
#endif
    State = CONFIRM;		/* mouse behaves like with :confirm */
#ifdef FEAT_MOUSE
    setmouse();			/* disables mouse for xterm */
#endif
    ++no_mapping;
    ++allow_keys;		/* no mapping here, but recognize keys */

    while (r != 'y' && r != 'n')
    {
	/* same highlighting as for wait_return */
	smsg_attr(HL_ATTR(HLF_R), "%s (y/n)?", str);
	if (direct)
	    r = get_keystroke();
	else
	    r = plain_vgetc();
	if (r == Ctrl_C || r == ESC)
	    r = 'n';
	msg_putchar(r);	    /* show what you typed */
	out_flush();
    }
    --no_wait_return;
    State = save_State;
#ifdef FEAT_MOUSE
    setmouse();
#endif
    --no_mapping;
    --allow_keys;

    return r;
}

#if defined(FEAT_MOUSE) || defined(PROTO)
/*
 * Return TRUE if "c" is a mouse key.
 */
    int
is_mouse_key(int c)
{
    return c == K_LEFTMOUSE
	|| c == K_LEFTMOUSE_NM
	|| c == K_LEFTDRAG
	|| c == K_LEFTRELEASE
	|| c == K_LEFTRELEASE_NM
	|| c == K_MOUSEMOVE
	|| c == K_MIDDLEMOUSE
	|| c == K_MIDDLEDRAG
	|| c == K_MIDDLERELEASE
	|| c == K_RIGHTMOUSE
	|| c == K_RIGHTDRAG
	|| c == K_RIGHTRELEASE
	|| c == K_MOUSEDOWN
	|| c == K_MOUSEUP
	|| c == K_MOUSELEFT
	|| c == K_MOUSERIGHT
	|| c == K_X1MOUSE
	|| c == K_X1DRAG
	|| c == K_X1RELEASE
	|| c == K_X2MOUSE
	|| c == K_X2DRAG
	|| c == K_X2RELEASE;
}
#endif

/*
 * Get a key stroke directly from the user.
 * Ignores mouse clicks and scrollbar events, except a click for the left
 * button (used at the more prompt).
 * Doesn't use vgetc(), because it syncs undo and eats mapped characters.
 * Disadvantage: typeahead is ignored.
 * Translates the interrupt character for unix to ESC.
 */
    int
get_keystroke(void)
{
    char_u	*buf = NULL;
    int		buflen = 150;
    int		maxlen;
    int		len = 0;
    int		n;
    int		save_mapped_ctrl_c = mapped_ctrl_c;
    int		waited = 0;

    mapped_ctrl_c = FALSE;	/* mappings are not used here */
    for (;;)
    {
	cursor_on();
	out_flush();

	/* Leave some room for check_termcode() to insert a key code into (max
	 * 5 chars plus NUL).  And fix_input_buffer() can triple the number of
	 * bytes. */
	maxlen = (buflen - 6 - len) / 3;
	if (buf == NULL)
	    buf = alloc(buflen);
	else if (maxlen < 10)
	{
	    char_u  *t_buf = buf;

	    /* Need some more space. This might happen when receiving a long
	     * escape sequence. */
	    buflen += 100;
	    buf = vim_realloc(buf, buflen);
	    if (buf == NULL)
		vim_free(t_buf);
	    maxlen = (buflen - 6 - len) / 3;
	}
	if (buf == NULL)
	{
	    do_outofmem_msg((long_u)buflen);
	    return ESC;  /* panic! */
	}

	/* First time: blocking wait.  Second time: wait up to 100ms for a
	 * terminal code to complete. */
	n = ui_inchar(buf + len, maxlen, len == 0 ? -1L : 100L, 0);
	if (n > 0)
	{
	    /* Replace zero and CSI by a special key code. */
	    n = fix_input_buffer(buf + len, n);
	    len += n;
	    waited = 0;
	}
	else if (len > 0)
	    ++waited;	    /* keep track of the waiting time */

	/* Incomplete termcode and not timed out yet: get more characters */
	if ((n = check_termcode(1, buf, buflen, &len)) < 0
	       && (!p_ttimeout || waited * 100L < (p_ttm < 0 ? p_tm : p_ttm)))
	    continue;

	if (n == KEYLEN_REMOVED)  /* key code removed */
	{
	    if (must_redraw != 0 && !need_wait_return && (State & CMDLINE) == 0)
	    {
		/* Redrawing was postponed, do it now. */
		update_screen(0);
		setcursor(); /* put cursor back where it belongs */
	    }
	    continue;
	}
	if (n > 0)		/* found a termcode: adjust length */
	    len = n;
	if (len == 0)		/* nothing typed yet */
	    continue;

	/* Handle modifier and/or special key code. */
	n = buf[0];
	if (n == K_SPECIAL)
	{
	    n = TO_SPECIAL(buf[1], buf[2]);
	    if (buf[1] == KS_MODIFIER
		    || n == K_IGNORE
#ifdef FEAT_MOUSE
		    || (is_mouse_key(n) && n != K_LEFTMOUSE)
#endif
#ifdef FEAT_GUI
		    || n == K_VER_SCROLLBAR
		    || n == K_HOR_SCROLLBAR
#endif
	       )
	    {
		if (buf[1] == KS_MODIFIER)
		    mod_mask = buf[2];
		len -= 3;
		if (len > 0)
		    mch_memmove(buf, buf + 3, (size_t)len);
		continue;
	    }
	    break;
	}
	if (has_mbyte)
	{
	    if (MB_BYTE2LEN(n) > len)
		continue;	/* more bytes to get */
	    buf[len >= buflen ? buflen - 1 : len] = NUL;
	    n = (*mb_ptr2char)(buf);
	}
#ifdef UNIX
	if (n == intr_char)
	    n = ESC;
#endif
	break;
    }
    vim_free(buf);

    mapped_ctrl_c = save_mapped_ctrl_c;
    return n;
}

/*
 * Get a number from the user.
 * When "mouse_used" is not NULL allow using the mouse.
 */
    int
get_number(
    int	    colon,			/* allow colon to abort */
    int	    *mouse_used)
{
    int	n = 0;
    int	c;
    int typed = 0;

    if (mouse_used != NULL)
	*mouse_used = FALSE;

    /* When not printing messages, the user won't know what to type, return a
     * zero (as if CR was hit). */
    if (msg_silent != 0)
	return 0;

#ifdef USE_ON_FLY_SCROLL
    dont_scroll = TRUE;		/* disallow scrolling here */
#endif
    ++no_mapping;
    ++allow_keys;		/* no mapping here, but recognize keys */
    for (;;)
    {
	windgoto(msg_row, msg_col);
	c = safe_vgetc();
	if (VIM_ISDIGIT(c))
	{
	    n = n * 10 + c - '0';
	    msg_putchar(c);
	    ++typed;
	}
	else if (c == K_DEL || c == K_KDEL || c == K_BS || c == Ctrl_H)
	{
	    if (typed > 0)
	    {
		msg_puts("\b \b");
		--typed;
	    }
	    n /= 10;
	}
#ifdef FEAT_MOUSE
	else if (mouse_used != NULL && c == K_LEFTMOUSE)
	{
	    *mouse_used = TRUE;
	    n = mouse_row + 1;
	    break;
	}
#endif
	else if (n == 0 && c == ':' && colon)
	{
	    stuffcharReadbuff(':');
	    if (!exmode_active)
		cmdline_row = msg_row;
	    skip_redraw = TRUE;	    /* skip redraw once */
	    do_redraw = FALSE;
	    break;
	}
	else if (c == CAR || c == NL || c == Ctrl_C || c == ESC)
	    break;
    }
    --no_mapping;
    --allow_keys;
    return n;
}

/*
 * Ask the user to enter a number.
 * When "mouse_used" is not NULL allow using the mouse and in that case return
 * the line number.
 */
    int
prompt_for_number(int *mouse_used)
{
    int		i;
    int		save_cmdline_row;
    int		save_State;

    /* When using ":silent" assume that <CR> was entered. */
    if (mouse_used != NULL)
	msg_puts(_("Type number and <Enter> or click with mouse (empty cancels): "));
    else
	msg_puts(_("Type number and <Enter> (empty cancels): "));

    // Set the state such that text can be selected/copied/pasted and we still
    // get mouse events. redraw_after_callback() will not redraw if cmdline_row
    // is zero.
    save_cmdline_row = cmdline_row;
    cmdline_row = 0;
    save_State = State;
    State = CMDLINE;
#ifdef FEAT_MOUSE
    // May show different mouse shape.
    setmouse();
#endif

    i = get_number(TRUE, mouse_used);
    if (KeyTyped)
    {
	/* don't call wait_return() now */
	/* msg_putchar('\n'); */
	cmdline_row = msg_row - 1;
	need_wait_return = FALSE;
	msg_didany = FALSE;
	msg_didout = FALSE;
    }
    else
	cmdline_row = save_cmdline_row;
    State = save_State;
#ifdef FEAT_MOUSE
    // May need to restore mouse shape.
    setmouse();
#endif

    return i;
}

    void
msgmore(long n)
{
    long pn;

    if (global_busy	    /* no messages now, wait until global is finished */
	    || !messaging())  /* 'lazyredraw' set, don't do messages now */
	return;

    /* We don't want to overwrite another important message, but do overwrite
     * a previous "more lines" or "fewer lines" message, so that "5dd" and
     * then "put" reports the last action. */
    if (keep_msg != NULL && !keep_msg_more)
	return;

    if (n > 0)
	pn = n;
    else
	pn = -n;

    if (pn > p_report)
    {
	if (n > 0)
	    vim_snprintf(msg_buf, MSG_BUF_LEN,
		    NGETTEXT("%ld more line", "%ld more lines", pn), pn);
	else
	    vim_snprintf(msg_buf, MSG_BUF_LEN,
		    NGETTEXT("%ld line less", "%ld fewer lines", pn), pn);
	if (got_int)
	    vim_strcat((char_u *)msg_buf, (char_u *)_(" (Interrupted)"),
								  MSG_BUF_LEN);
	if (msg(msg_buf))
	{
	    set_keep_msg((char_u *)msg_buf, 0);
	    keep_msg_more = TRUE;
	}
    }
}

/*
 * flush map and typeahead buffers and give a warning for an error
 */
    void
beep_flush(void)
{
    if (emsg_silent == 0)
    {
	flush_buffers(FLUSH_MINIMAL);
	vim_beep(BO_ERROR);
    }
}

/*
 * Give a warning for an error.
 */
    void
vim_beep(
    unsigned val) /* one of the BO_ values, e.g., BO_OPER */
{
#ifdef FEAT_EVAL
    called_vim_beep = TRUE;
#endif

    if (emsg_silent == 0)
    {
	if (!((bo_flags & val) || (bo_flags & BO_ALL)))
	{
#ifdef ELAPSED_FUNC
	    static int		did_init = FALSE;
	    static elapsed_T	start_tv;

	    /* Only beep once per half a second, otherwise a sequence of beeps
	     * would freeze Vim. */
	    if (!did_init || ELAPSED_FUNC(start_tv) > 500)
	    {
		did_init = TRUE;
		ELAPSED_INIT(start_tv);
#endif
		if (p_vb
#ifdef FEAT_GUI
			/* While the GUI is starting up the termcap is set for
			 * the GUI but the output still goes to a terminal. */
			&& !(gui.in_use && gui.starting)
#endif
			)
		{
		    out_str_cf(T_VB);
#ifdef FEAT_VTP
		    /* No restore color information, refresh the screen. */
		    if (has_vtp_working() != 0
# ifdef FEAT_TERMGUICOLORS
			    && (p_tgc || (!p_tgc && t_colors >= 256))
# endif
			)
		    {
			redraw_later(CLEAR);
			update_screen(0);
			redrawcmd();
		    }
#endif
		}
		else
		    out_char(BELL);
#ifdef ELAPSED_FUNC
	    }
#endif
	}

	/* When 'debug' contains "beep" produce a message.  If we are sourcing
	 * a script or executing a function give the user a hint where the beep
	 * comes from. */
	if (vim_strchr(p_debug, 'e') != NULL)
	{
	    msg_source(HL_ATTR(HLF_W));
	    msg_attr(_("Beep!"), HL_ATTR(HLF_W));
	}
    }
}

/*
 * To get the "real" home directory:
 * - get value of $HOME
 * For Unix:
 *  - go to that directory
 *  - do mch_dirname() to get the real name of that directory.
 *  This also works with mounts and links.
 *  Don't do this for MS-DOS, it will change the "current dir" for a drive.
 * For Windows:
 *  This code is duplicated in init_homedir() in dosinst.c.  Keep in sync!
 */
static char_u	*homedir = NULL;

    void
init_homedir(void)
{
    char_u  *var;

    /* In case we are called a second time (when 'encoding' changes). */
    VIM_CLEAR(homedir);

#ifdef VMS
    var = mch_getenv((char_u *)"SYS$LOGIN");
#else
    var = mch_getenv((char_u *)"HOME");
#endif

#ifdef MSWIN
    /*
     * Typically, $HOME is not defined on Windows, unless the user has
     * specifically defined it for Vim's sake.  However, on Windows NT
     * platforms, $HOMEDRIVE and $HOMEPATH are automatically defined for
     * each user.  Try constructing $HOME from these.
     */
    if (var == NULL || *var == NUL)
    {
	char_u *homedrive, *homepath;

	homedrive = mch_getenv((char_u *)"HOMEDRIVE");
	homepath = mch_getenv((char_u *)"HOMEPATH");
	if (homepath == NULL || *homepath == NUL)
	    homepath = (char_u *)"\\";
	if (homedrive != NULL
			   && STRLEN(homedrive) + STRLEN(homepath) < MAXPATHL)
	{
	    sprintf((char *)NameBuff, "%s%s", homedrive, homepath);
	    if (NameBuff[0] != NUL)
		var = NameBuff;
	}
    }

    if (var == NULL)
	var = mch_getenv((char_u *)"USERPROFILE");

    /*
     * Weird but true: $HOME may contain an indirect reference to another
     * variable, esp. "%USERPROFILE%".  Happens when $USERPROFILE isn't set
     * when $HOME is being set.
     */
    if (var != NULL && *var == '%')
    {
	char_u	*p;
	char_u	*exp;

	p = vim_strchr(var + 1, '%');
	if (p != NULL)
	{
	    vim_strncpy(NameBuff, var + 1, p - (var + 1));
	    exp = mch_getenv(NameBuff);
	    if (exp != NULL && *exp != NUL
					&& STRLEN(exp) + STRLEN(p) < MAXPATHL)
	    {
		vim_snprintf((char *)NameBuff, MAXPATHL, "%s%s", exp, p + 1);
		var = NameBuff;
	    }
	}
    }

    if (var != NULL && *var == NUL)	/* empty is same as not set */
	var = NULL;

    if (enc_utf8 && var != NULL)
    {
	int	len;
	char_u  *pp = NULL;

	/* Convert from active codepage to UTF-8.  Other conversions are
	 * not done, because they would fail for non-ASCII characters. */
	acp_to_enc(var, (int)STRLEN(var), &pp, &len);
	if (pp != NULL)
	{
	    homedir = pp;
	    return;
	}
    }

    /*
     * Default home dir is C:/
     * Best assumption we can make in such a situation.
     */
    if (var == NULL)
	var = (char_u *)"C:/";
#endif

    if (var != NULL)
    {
#ifdef UNIX
	/*
	 * Change to the directory and get the actual path.  This resolves
	 * links.  Don't do it when we can't return.
	 */
	if (mch_dirname(NameBuff, MAXPATHL) == OK
					  && mch_chdir((char *)NameBuff) == 0)
	{
	    if (!mch_chdir((char *)var) && mch_dirname(IObuff, IOSIZE) == OK)
		var = IObuff;
	    if (mch_chdir((char *)NameBuff) != 0)
		emsg(_(e_prev_dir));
	}
#endif
	homedir = vim_strsave(var);
    }
}

#if defined(EXITFREE) || defined(PROTO)
    void
free_homedir(void)
{
    vim_free(homedir);
}

# ifdef FEAT_CMDL_COMPL
    void
free_users(void)
{
    ga_clear_strings(&ga_users);
}
# endif
#endif

/*
 * Call expand_env() and store the result in an allocated string.
 * This is not very memory efficient, this expects the result to be freed
 * again soon.
 */
    char_u *
expand_env_save(char_u *src)
{
    return expand_env_save_opt(src, FALSE);
}

/*
 * Idem, but when "one" is TRUE handle the string as one file name, only
 * expand "~" at the start.
 */
    char_u *
expand_env_save_opt(char_u *src, int one)
{
    char_u	*p;

    p = alloc(MAXPATHL);
    if (p != NULL)
	expand_env_esc(src, p, MAXPATHL, FALSE, one, NULL);
    return p;
}

/*
 * Expand environment variable with path name.
 * "~/" is also expanded, using $HOME.	For Unix "~user/" is expanded.
 * Skips over "\ ", "\~" and "\$" (not for Win32 though).
 * If anything fails no expansion is done and dst equals src.
 */
    void
expand_env(
    char_u	*src,		/* input string e.g. "$HOME/vim.hlp" */
    char_u	*dst,		/* where to put the result */
    int		dstlen)		/* maximum length of the result */
{
    expand_env_esc(src, dst, dstlen, FALSE, FALSE, NULL);
}

    void
expand_env_esc(
    char_u	*srcp,		/* input string e.g. "$HOME/vim.hlp" */
    char_u	*dst,		/* where to put the result */
    int		dstlen,		/* maximum length of the result */
    int		esc,		/* escape spaces in expanded variables */
    int		one,		/* "srcp" is one file name */
    char_u	*startstr)	/* start again after this (can be NULL) */
{
    char_u	*src;
    char_u	*tail;
    int		c;
    char_u	*var;
    int		copy_char;
    int		mustfree;	/* var was allocated, need to free it later */
    int		at_start = TRUE; /* at start of a name */
    int		startstr_len = 0;

    if (startstr != NULL)
	startstr_len = (int)STRLEN(startstr);

    src = skipwhite(srcp);
    --dstlen;		    /* leave one char space for "\," */
    while (*src && dstlen > 0)
    {
#ifdef FEAT_EVAL
	/* Skip over `=expr`. */
	if (src[0] == '`' && src[1] == '=')
	{
	    size_t len;

	    var = src;
	    src += 2;
	    (void)skip_expr(&src);
	    if (*src == '`')
		++src;
	    len = src - var;
	    if (len > (size_t)dstlen)
		len = dstlen;
	    vim_strncpy(dst, var, len);
	    dst += len;
	    dstlen -= (int)len;
	    continue;
	}
#endif
	copy_char = TRUE;
	if ((*src == '$'
#ifdef VMS
		    && at_start
#endif
	   )
#if defined(MSWIN)
		|| *src == '%'
#endif
		|| (*src == '~' && at_start))
	{
	    mustfree = FALSE;

	    /*
	     * The variable name is copied into dst temporarily, because it may
	     * be a string in read-only memory and a NUL needs to be appended.
	     */
	    if (*src != '~')				/* environment var */
	    {
		tail = src + 1;
		var = dst;
		c = dstlen - 1;

#ifdef UNIX
		/* Unix has ${var-name} type environment vars */
		if (*tail == '{' && !vim_isIDc('{'))
		{
		    tail++;	/* ignore '{' */
		    while (c-- > 0 && *tail && *tail != '}')
			*var++ = *tail++;
		}
		else
#endif
		{
		    while (c-- > 0 && *tail != NUL && ((vim_isIDc(*tail))
#if defined(MSWIN)
			    || (*src == '%' && *tail != '%')
#endif
			    ))
			*var++ = *tail++;
		}

#if defined(MSWIN) || defined(UNIX)
# ifdef UNIX
		if (src[1] == '{' && *tail != '}')
# else
		if (*src == '%' && *tail != '%')
# endif
		    var = NULL;
		else
		{
# ifdef UNIX
		    if (src[1] == '{')
# else
		    if (*src == '%')
#endif
			++tail;
#endif
		    *var = NUL;
		    var = vim_getenv(dst, &mustfree);
#if defined(MSWIN) || defined(UNIX)
		}
#endif
	    }
							/* home directory */
	    else if (  src[1] == NUL
		    || vim_ispathsep(src[1])
		    || vim_strchr((char_u *)" ,\t\n", src[1]) != NULL)
	    {
		var = homedir;
		tail = src + 1;
	    }
	    else					/* user directory */
	    {
#if defined(UNIX) || (defined(VMS) && defined(USER_HOME))
		/*
		 * Copy ~user to dst[], so we can put a NUL after it.
		 */
		tail = src;
		var = dst;
		c = dstlen - 1;
		while (	   c-- > 0
			&& *tail
			&& vim_isfilec(*tail)
			&& !vim_ispathsep(*tail))
		    *var++ = *tail++;
		*var = NUL;
# ifdef UNIX
		/*
		 * If the system supports getpwnam(), use it.
		 * Otherwise, or if getpwnam() fails, the shell is used to
		 * expand ~user.  This is slower and may fail if the shell
		 * does not support ~user (old versions of /bin/sh).
		 */
#  if defined(HAVE_GETPWNAM) && defined(HAVE_PWD_H)
		{
		    /* Note: memory allocated by getpwnam() is never freed.
		     * Calling endpwent() apparently doesn't help. */
		    struct passwd *pw = (*dst == NUL)
					? NULL : getpwnam((char *)dst + 1);

		    var = (pw == NULL) ? NULL : (char_u *)pw->pw_dir;
		}
		if (var == NULL)
#  endif
		{
		    expand_T	xpc;

		    ExpandInit(&xpc);
		    xpc.xp_context = EXPAND_FILES;
		    var = ExpandOne(&xpc, dst, NULL,
				WILD_ADD_SLASH|WILD_SILENT, WILD_EXPAND_FREE);
		    mustfree = TRUE;
		}

# else	/* !UNIX, thus VMS */
		/*
		 * USER_HOME is a comma-separated list of
		 * directories to search for the user account in.
		 */
		{
		    char_u	test[MAXPATHL], paths[MAXPATHL];
		    char_u	*path, *next_path, *ptr;
		    stat_T	st;

		    STRCPY(paths, USER_HOME);
		    next_path = paths;
		    while (*next_path)
		    {
			for (path = next_path; *next_path && *next_path != ',';
				next_path++);
			if (*next_path)
			    *next_path++ = NUL;
			STRCPY(test, path);
			STRCAT(test, "/");
			STRCAT(test, dst + 1);
			if (mch_stat(test, &st) == 0)
			{
			    var = alloc(STRLEN(test) + 1);
			    STRCPY(var, test);
			    mustfree = TRUE;
			    break;
			}
		    }
		}
# endif /* UNIX */
#else
		/* cannot expand user's home directory, so don't try */
		var = NULL;
		tail = (char_u *)"";	/* for gcc */
#endif /* UNIX || VMS */
	    }

#ifdef BACKSLASH_IN_FILENAME
	    /* If 'shellslash' is set change backslashes to forward slashes.
	     * Can't use slash_adjust(), p_ssl may be set temporarily. */
	    if (p_ssl && var != NULL && vim_strchr(var, '\\') != NULL)
	    {
		char_u	*p = vim_strsave(var);

		if (p != NULL)
		{
		    if (mustfree)
			vim_free(var);
		    var = p;
		    mustfree = TRUE;
		    forward_slash(var);
		}
	    }
#endif

	    /* If "var" contains white space, escape it with a backslash.
	     * Required for ":e ~/tt" when $HOME includes a space. */
	    if (esc && var != NULL && vim_strpbrk(var, (char_u *)" \t") != NULL)
	    {
		char_u	*p = vim_strsave_escaped(var, (char_u *)" \t");

		if (p != NULL)
		{
		    if (mustfree)
			vim_free(var);
		    var = p;
		    mustfree = TRUE;
		}
	    }

	    if (var != NULL && *var != NUL
		    && (STRLEN(var) + STRLEN(tail) + 1 < (unsigned)dstlen))
	    {
		STRCPY(dst, var);
		dstlen -= (int)STRLEN(var);
		c = (int)STRLEN(var);
		/* if var[] ends in a path separator and tail[] starts
		 * with it, skip a character */
		if (*var != NUL && after_pathsep(dst, dst + c)
#if defined(BACKSLASH_IN_FILENAME) || defined(AMIGA)
			&& dst[-1] != ':'
#endif
			&& vim_ispathsep(*tail))
		    ++tail;
		dst += c;
		src = tail;
		copy_char = FALSE;
	    }
	    if (mustfree)
		vim_free(var);
	}

	if (copy_char)	    /* copy at least one char */
	{
	    /*
	     * Recognize the start of a new name, for '~'.
	     * Don't do this when "one" is TRUE, to avoid expanding "~" in
	     * ":edit foo ~ foo".
	     */
	    at_start = FALSE;
	    if (src[0] == '\\' && src[1] != NUL)
	    {
		*dst++ = *src++;
		--dstlen;
	    }
	    else if ((src[0] == ' ' || src[0] == ',') && !one)
		at_start = TRUE;
	    if (dstlen > 0)
	    {
		*dst++ = *src++;
		--dstlen;

		if (startstr != NULL && src - startstr_len >= srcp
			&& STRNCMP(src - startstr_len, startstr,
							    startstr_len) == 0)
		    at_start = TRUE;
	    }
	}

    }
    *dst = NUL;
}

/*
 * Vim's version of getenv().
 * Special handling of $HOME, $VIM and $VIMRUNTIME.
 * Also does ACP to 'enc' conversion for Win32.
 * "mustfree" is set to TRUE when returned is allocated, it must be
 * initialized to FALSE by the caller.
 */
    char_u *
vim_getenv(char_u *name, int *mustfree)
{
    char_u	*p = NULL;
    char_u	*pend;
    int		vimruntime;
#ifdef MSWIN
    WCHAR	*wn, *wp;

    // use "C:/" when $HOME is not set
    if (STRCMP(name, "HOME") == 0)
	return homedir;

    // Use Wide function
    wn = enc_to_utf16(name, NULL);
    if (wn == NULL)
	return NULL;

    wp = _wgetenv(wn);
    vim_free(wn);

    if (wp != NULL && *wp == NUL)   // empty is the same as not set
	wp = NULL;

    if (wp != NULL)
    {
	p = utf16_to_enc(wp, NULL);
	if (p == NULL)
	    return NULL;

	*mustfree = TRUE;
	return p;
    }
#else
    p = mch_getenv(name);
    if (p != NULL && *p == NUL)	    // empty is the same as not set
	p = NULL;

    if (p != NULL)
	return p;
#endif

    // handling $VIMRUNTIME and $VIM is below, bail out if it's another name.
    vimruntime = (STRCMP(name, "VIMRUNTIME") == 0);
    if (!vimruntime && STRCMP(name, "VIM") != 0)
	return NULL;

    /*
     * When expanding $VIMRUNTIME fails, try using $VIM/vim<version> or $VIM.
     * Don't do this when default_vimruntime_dir is non-empty.
     */
    if (vimruntime
#ifdef HAVE_PATHDEF
	    && *default_vimruntime_dir == NUL
#endif
       )
    {
#ifdef MSWIN
	// Use Wide function
	wp = _wgetenv(L"VIM");
	if (wp != NULL && *wp == NUL)	    // empty is the same as not set
	    wp = NULL;
	if (wp != NULL)
	{
	    char_u *q = utf16_to_enc(wp, NULL);
	    if (q != NULL)
	    {
		p = vim_version_dir(q);
		*mustfree = TRUE;
		if (p == NULL)
		    p = q;
	    }
	}
#else
	p = mch_getenv((char_u *)"VIM");
	if (p != NULL && *p == NUL)	    // empty is the same as not set
	    p = NULL;
	if (p != NULL)
	{
	    p = vim_version_dir(p);
	    if (p != NULL)
		*mustfree = TRUE;
	    else
		p = mch_getenv((char_u *)"VIM");
	}
#endif
    }

    /*
     * When expanding $VIM or $VIMRUNTIME fails, try using:
     * - the directory name from 'helpfile' (unless it contains '$')
     * - the executable name from argv[0]
     */
    if (p == NULL)
    {
	if (p_hf != NULL && vim_strchr(p_hf, '$') == NULL)
	    p = p_hf;
#ifdef USE_EXE_NAME
	/*
	 * Use the name of the executable, obtained from argv[0].
	 */
	else
	    p = exe_name;
#endif
	if (p != NULL)
	{
	    /* remove the file name */
	    pend = gettail(p);

	    /* remove "doc/" from 'helpfile', if present */
	    if (p == p_hf)
		pend = remove_tail(p, pend, (char_u *)"doc");

#ifdef USE_EXE_NAME
# ifdef MACOS_X
	    /* remove "MacOS" from exe_name and add "Resources/vim" */
	    if (p == exe_name)
	    {
		char_u	*pend1;
		char_u	*pnew;

		pend1 = remove_tail(p, pend, (char_u *)"MacOS");
		if (pend1 != pend)
		{
		    pnew = alloc((unsigned)(pend1 - p) + 15);
		    if (pnew != NULL)
		    {
			STRNCPY(pnew, p, (pend1 - p));
			STRCPY(pnew + (pend1 - p), "Resources/vim");
			p = pnew;
			pend = p + STRLEN(p);
		    }
		}
	    }
# endif
	    /* remove "src/" from exe_name, if present */
	    if (p == exe_name)
		pend = remove_tail(p, pend, (char_u *)"src");
#endif

	    /* for $VIM, remove "runtime/" or "vim54/", if present */
	    if (!vimruntime)
	    {
		pend = remove_tail(p, pend, (char_u *)RUNTIME_DIRNAME);
		pend = remove_tail(p, pend, (char_u *)VIM_VERSION_NODOT);
	    }

	    /* remove trailing path separator */
	    if (pend > p && after_pathsep(p, pend))
		--pend;

#ifdef MACOS_X
	    if (p == exe_name || p == p_hf)
#endif
		/* check that the result is a directory name */
		p = vim_strnsave(p, (int)(pend - p));

	    if (p != NULL && !mch_isdir(p))
		VIM_CLEAR(p);
	    else
	    {
#ifdef USE_EXE_NAME
		/* may add "/vim54" or "/runtime" if it exists */
		if (vimruntime && (pend = vim_version_dir(p)) != NULL)
		{
		    vim_free(p);
		    p = pend;
		}
#endif
		*mustfree = TRUE;
	    }
	}
    }

#ifdef HAVE_PATHDEF
    /* When there is a pathdef.c file we can use default_vim_dir and
     * default_vimruntime_dir */
    if (p == NULL)
    {
	/* Only use default_vimruntime_dir when it is not empty */
	if (vimruntime && *default_vimruntime_dir != NUL)
	{
	    p = default_vimruntime_dir;
	    *mustfree = FALSE;
	}
	else if (*default_vim_dir != NUL)
	{
	    if (vimruntime && (p = vim_version_dir(default_vim_dir)) != NULL)
		*mustfree = TRUE;
	    else
	    {
		p = default_vim_dir;
		*mustfree = FALSE;
	    }
	}
    }
#endif

    /*
     * Set the environment variable, so that the new value can be found fast
     * next time, and others can also use it (e.g. Perl).
     */
    if (p != NULL)
    {
	if (vimruntime)
	{
	    vim_setenv((char_u *)"VIMRUNTIME", p);
	    didset_vimruntime = TRUE;
	}
	else
	{
	    vim_setenv((char_u *)"VIM", p);
	    didset_vim = TRUE;
	}
    }
    return p;
}

/*
 * Check if the directory "vimdir/<version>" or "vimdir/runtime" exists.
 * Return NULL if not, return its name in allocated memory otherwise.
 */
    static char_u *
vim_version_dir(char_u *vimdir)
{
    char_u	*p;

    if (vimdir == NULL || *vimdir == NUL)
	return NULL;
    p = concat_fnames(vimdir, (char_u *)VIM_VERSION_NODOT, TRUE);
    if (p != NULL && mch_isdir(p))
	return p;
    vim_free(p);
    p = concat_fnames(vimdir, (char_u *)RUNTIME_DIRNAME, TRUE);
    if (p != NULL && mch_isdir(p))
	return p;
    vim_free(p);
    return NULL;
}

/*
 * If the string between "p" and "pend" ends in "name/", return "pend" minus
 * the length of "name/".  Otherwise return "pend".
 */
    static char_u *
remove_tail(char_u *p, char_u *pend, char_u *name)
{
    int		len = (int)STRLEN(name) + 1;
    char_u	*newend = pend - len;

    if (newend >= p
	    && fnamencmp(newend, name, len - 1) == 0
	    && (newend == p || after_pathsep(p, newend)))
	return newend;
    return pend;
}

#if defined(FEAT_EVAL) || defined(PROTO)
    void
vim_unsetenv(char_u *var)
{
#ifdef HAVE_UNSETENV
    unsetenv((char *)var);
#else
    vim_setenv(var, (char_u *)"");
#endif
}
#endif


/*
 * Our portable version of setenv.
 */
    void
vim_setenv(char_u *name, char_u *val)
{
#ifdef HAVE_SETENV
    mch_setenv((char *)name, (char *)val, 1);
#else
    char_u	*envbuf;

    /*
     * Putenv does not copy the string, it has to remain
     * valid.  The allocated memory will never be freed.
     */
    envbuf = alloc((unsigned)(STRLEN(name) + STRLEN(val) + 2));
    if (envbuf != NULL)
    {
	sprintf((char *)envbuf, "%s=%s", name, val);
	putenv((char *)envbuf);
    }
#endif
#ifdef FEAT_GETTEXT
    /*
     * When setting $VIMRUNTIME adjust the directory to find message
     * translations to $VIMRUNTIME/lang.
     */
    if (*val != NUL && STRICMP(name, "VIMRUNTIME") == 0)
    {
	char_u	*buf = concat_str(val, (char_u *)"/lang");

	if (buf != NULL)
	{
	    bindtextdomain(VIMPACKAGE, (char *)buf);
	    vim_free(buf);
	}
    }
#endif
}

#if defined(FEAT_CMDL_COMPL) || defined(PROTO)
/*
 * Function given to ExpandGeneric() to obtain an environment variable name.
 */
    char_u *
get_env_name(
    expand_T	*xp UNUSED,
    int		idx)
{
# if defined(AMIGA)
    /*
     * No environ[] on the Amiga.
     */
    return NULL;
# else
# ifndef __WIN32__
    /* Borland C++ 5.2 has this in a header file. */
    extern char		**environ;
# endif
# define ENVNAMELEN 100
    static char_u	name[ENVNAMELEN];
    char_u		*str;
    int			n;

    str = (char_u *)environ[idx];
    if (str == NULL)
	return NULL;

    for (n = 0; n < ENVNAMELEN - 1; ++n)
    {
	if (str[n] == '=' || str[n] == NUL)
	    break;
	name[n] = str[n];
    }
    name[n] = NUL;
    return name;
# endif
}

/*
 * Add a user name to the list of users in ga_users.
 * Do nothing if user name is NULL or empty.
 */
    static void
add_user(char_u *user, int need_copy)
{
    char_u	*user_copy = (user != NULL && need_copy)
						    ? vim_strsave(user) : user;

    if (user_copy == NULL || *user_copy == NUL || ga_grow(&ga_users, 1) == FAIL)
    {
	if (need_copy)
	    vim_free(user);
	return;
    }
    ((char_u **)(ga_users.ga_data))[ga_users.ga_len++] = user_copy;
}

/*
 * Find all user names for user completion.
 * Done only once and then cached.
 */
    static void
init_users(void)
{
    static int	lazy_init_done = FALSE;

    if (lazy_init_done)
	return;

    lazy_init_done = TRUE;
    ga_init2(&ga_users, sizeof(char_u *), 20);

# if defined(HAVE_GETPWENT) && defined(HAVE_PWD_H)
    {
	struct passwd*	pw;

	setpwent();
	while ((pw = getpwent()) != NULL)
	    add_user((char_u *)pw->pw_name, TRUE);
	endpwent();
    }
# elif defined(MSWIN)
    {
	DWORD		nusers = 0, ntotal = 0, i;
	PUSER_INFO_0	uinfo;

	if (NetUserEnum(NULL, 0, 0, (LPBYTE *) &uinfo, MAX_PREFERRED_LENGTH,
				       &nusers, &ntotal, NULL) == NERR_Success)
	{
	    for (i = 0; i < nusers; i++)
		add_user(utf16_to_enc(uinfo[i].usri0_name, NULL), FALSE);

	    NetApiBufferFree(uinfo);
	}
    }
# endif
# if defined(HAVE_GETPWNAM)
    {
	char_u	*user_env = mch_getenv((char_u *)"USER");

	// The $USER environment variable may be a valid remote user name (NIS,
	// LDAP) not already listed by getpwent(), as getpwent() only lists
	// local user names.  If $USER is not already listed, check whether it
	// is a valid remote user name using getpwnam() and if it is, add it to
	// the list of user names.

	if (user_env != NULL && *user_env != NUL)
	{
	    int	i;

	    for (i = 0; i < ga_users.ga_len; i++)
	    {
		char_u	*local_user = ((char_u **)ga_users.ga_data)[i];

		if (STRCMP(local_user, user_env) == 0)
		    break;
	    }

	    if (i == ga_users.ga_len)
	    {
		struct passwd	*pw = getpwnam((char *)user_env);

		if (pw != NULL)
		    add_user((char_u *)pw->pw_name, TRUE);
	    }
	}
    }
# endif
}

/*
 * Function given to ExpandGeneric() to obtain an user names.
 */
    char_u*
get_users(expand_T *xp UNUSED, int idx)
{
    init_users();
    if (idx < ga_users.ga_len)
	return ((char_u **)ga_users.ga_data)[idx];
    return NULL;
}

/*
 * Check whether name matches a user name. Return:
 * 0 if name does not match any user name.
 * 1 if name partially matches the beginning of a user name.
 * 2 is name fully matches a user name.
 */
    int
match_user(char_u *name)
{
    int i;
    int n = (int)STRLEN(name);
    int result = 0;

    init_users();
    for (i = 0; i < ga_users.ga_len; i++)
    {
	if (STRCMP(((char_u **)ga_users.ga_data)[i], name) == 0)
	    return 2; /* full match */
	if (STRNCMP(((char_u **)ga_users.ga_data)[i], name, n) == 0)
	    result = 1; /* partial match */
    }
    return result;
}
#endif

/*
 * Replace home directory by "~" in each space or comma separated file name in
 * 'src'.
 * If anything fails (except when out of space) dst equals src.
 */
    void
home_replace(
    buf_T	*buf,	/* when not NULL, check for help files */
    char_u	*src,	/* input file name */
    char_u	*dst,	/* where to put the result */
    int		dstlen,	/* maximum length of the result */
    int		one)	/* if TRUE, only replace one file name, include
			   spaces and commas in the file name. */
{
    size_t	dirlen = 0, envlen = 0;
    size_t	len;
    char_u	*homedir_env, *homedir_env_orig;
    char_u	*p;

    if (src == NULL)
    {
	*dst = NUL;
	return;
    }

    /*
     * If the file is a help file, remove the path completely.
     */
    if (buf != NULL && buf->b_help)
    {
	vim_snprintf((char *)dst, dstlen, "%s", gettail(src));
	return;
    }

    /*
     * We check both the value of the $HOME environment variable and the
     * "real" home directory.
     */
    if (homedir != NULL)
	dirlen = STRLEN(homedir);

#ifdef VMS
    homedir_env_orig = homedir_env = mch_getenv((char_u *)"SYS$LOGIN");
#else
    homedir_env_orig = homedir_env = mch_getenv((char_u *)"HOME");
#endif
#ifdef MSWIN
    if (homedir_env == NULL)
	homedir_env_orig = homedir_env = mch_getenv((char_u *)"USERPROFILE");
#endif
    /* Empty is the same as not set. */
    if (homedir_env != NULL && *homedir_env == NUL)
	homedir_env = NULL;

#if defined(FEAT_MODIFY_FNAME) || defined(FEAT_EVAL)
    if (homedir_env != NULL && *homedir_env == '~')
    {
	int	usedlen = 0;
	int	flen;
	char_u	*fbuf = NULL;

	flen = (int)STRLEN(homedir_env);
	(void)modify_fname((char_u *)":p", FALSE, &usedlen,
						  &homedir_env, &fbuf, &flen);
	flen = (int)STRLEN(homedir_env);
	if (flen > 0 && vim_ispathsep(homedir_env[flen - 1]))
	    /* Remove the trailing / that is added to a directory. */
	    homedir_env[flen - 1] = NUL;
    }
#endif

    if (homedir_env != NULL)
	envlen = STRLEN(homedir_env);

    if (!one)
	src = skipwhite(src);
    while (*src && dstlen > 0)
    {
	/*
	 * Here we are at the beginning of a file name.
	 * First, check to see if the beginning of the file name matches
	 * $HOME or the "real" home directory. Check that there is a '/'
	 * after the match (so that if e.g. the file is "/home/pieter/bla",
	 * and the home directory is "/home/piet", the file does not end up
	 * as "~er/bla" (which would seem to indicate the file "bla" in user
	 * er's home directory)).
	 */
	p = homedir;
	len = dirlen;
	for (;;)
	{
	    if (   len
		&& fnamencmp(src, p, len) == 0
		&& (vim_ispathsep(src[len])
		    || (!one && (src[len] == ',' || src[len] == ' '))
		    || src[len] == NUL))
	    {
		src += len;
		if (--dstlen > 0)
		    *dst++ = '~';

		/*
		 * If it's just the home directory, add  "/".
		 */
		if (!vim_ispathsep(src[0]) && --dstlen > 0)
		    *dst++ = '/';
		break;
	    }
	    if (p == homedir_env)
		break;
	    p = homedir_env;
	    len = envlen;
	}

	/* if (!one) skip to separator: space or comma */
	while (*src && (one || (*src != ',' && *src != ' ')) && --dstlen > 0)
	    *dst++ = *src++;
	/* skip separator */
	while ((*src == ' ' || *src == ',') && --dstlen > 0)
	    *dst++ = *src++;
    }
    /* if (dstlen == 0) out of space, what to do??? */

    *dst = NUL;

    if (homedir_env != homedir_env_orig)
	vim_free(homedir_env);
}

/*
 * Like home_replace, store the replaced string in allocated memory.
 * When something fails, NULL is returned.
 */
    char_u  *
home_replace_save(
    buf_T	*buf,	/* when not NULL, check for help files */
    char_u	*src)	/* input file name */
{
    char_u	*dst;
    unsigned	len;

    len = 3;			/* space for "~/" and trailing NUL */
    if (src != NULL)		/* just in case */
	len += (unsigned)STRLEN(src);
    dst = alloc(len);
    if (dst != NULL)
	home_replace(buf, src, dst, len, TRUE);
    return dst;
}

/*
 * Compare two file names and return:
 * FPC_SAME   if they both exist and are the same file.
 * FPC_SAMEX  if they both don't exist and have the same file name.
 * FPC_DIFF   if they both exist and are different files.
 * FPC_NOTX   if they both don't exist.
 * FPC_DIFFX  if one of them doesn't exist.
 * For the first name environment variables are expanded
 */
    int
fullpathcmp(
    char_u *s1,
    char_u *s2,
    int	    checkname)		/* when both don't exist, check file names */
{
#ifdef UNIX
    char_u	    exp1[MAXPATHL];
    char_u	    full1[MAXPATHL];
    char_u	    full2[MAXPATHL];
    stat_T	    st1, st2;
    int		    r1, r2;

    expand_env(s1, exp1, MAXPATHL);
    r1 = mch_stat((char *)exp1, &st1);
    r2 = mch_stat((char *)s2, &st2);
    if (r1 != 0 && r2 != 0)
    {
	/* if mch_stat() doesn't work, may compare the names */
	if (checkname)
	{
	    if (fnamecmp(exp1, s2) == 0)
		return FPC_SAMEX;
	    r1 = vim_FullName(exp1, full1, MAXPATHL, FALSE);
	    r2 = vim_FullName(s2, full2, MAXPATHL, FALSE);
	    if (r1 == OK && r2 == OK && fnamecmp(full1, full2) == 0)
		return FPC_SAMEX;
	}
	return FPC_NOTX;
    }
    if (r1 != 0 || r2 != 0)
	return FPC_DIFFX;
    if (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino)
	return FPC_SAME;
    return FPC_DIFF;
#else
    char_u  *exp1;		/* expanded s1 */
    char_u  *full1;		/* full path of s1 */
    char_u  *full2;		/* full path of s2 */
    int	    retval = FPC_DIFF;
    int	    r1, r2;

    /* allocate one buffer to store three paths (alloc()/free() is slow!) */
    if ((exp1 = alloc(MAXPATHL * 3)) != NULL)
    {
	full1 = exp1 + MAXPATHL;
	full2 = full1 + MAXPATHL;

	expand_env(s1, exp1, MAXPATHL);
	r1 = vim_FullName(exp1, full1, MAXPATHL, FALSE);
	r2 = vim_FullName(s2, full2, MAXPATHL, FALSE);

	/* If vim_FullName() fails, the file probably doesn't exist. */
	if (r1 != OK && r2 != OK)
	{
	    if (checkname && fnamecmp(exp1, s2) == 0)
		retval = FPC_SAMEX;
	    else
		retval = FPC_NOTX;
	}
	else if (r1 != OK || r2 != OK)
	    retval = FPC_DIFFX;
	else if (fnamecmp(full1, full2))
	    retval = FPC_DIFF;
	else
	    retval = FPC_SAME;
	vim_free(exp1);
    }
    return retval;
#endif
}

/*
 * Get the tail of a path: the file name.
 * When the path ends in a path separator the tail is the NUL after it.
 * Fail safe: never returns NULL.
 */
    char_u *
gettail(char_u *fname)
{
    char_u  *p1, *p2;

    if (fname == NULL)
	return (char_u *)"";
    for (p1 = p2 = get_past_head(fname); *p2; )	/* find last part of path */
    {
	if (vim_ispathsep_nocolon(*p2))
	    p1 = p2 + 1;
	MB_PTR_ADV(p2);
    }
    return p1;
}

/*
 * Get pointer to tail of "fname", including path separators.  Putting a NUL
 * here leaves the directory name.  Takes care of "c:/" and "//".
 * Always returns a valid pointer.
 */
    char_u *
gettail_sep(char_u *fname)
{
    char_u	*p;
    char_u	*t;

    p = get_past_head(fname);	/* don't remove the '/' from "c:/file" */
    t = gettail(fname);
    while (t > p && after_pathsep(fname, t))
	--t;
#ifdef VMS
    /* path separator is part of the path */
    ++t;
#endif
    return t;
}

/*
 * get the next path component (just after the next path separator).
 */
    char_u *
getnextcomp(char_u *fname)
{
    while (*fname && !vim_ispathsep(*fname))
	MB_PTR_ADV(fname);
    if (*fname)
	++fname;
    return fname;
}

/*
 * Get a pointer to one character past the head of a path name.
 * Unix: after "/"; DOS: after "c:\"; Amiga: after "disk:/"; Mac: no head.
 * If there is no head, path is returned.
 */
    char_u *
get_past_head(char_u *path)
{
    char_u  *retval;

#if defined(MSWIN)
    /* may skip "c:" */
    if (isalpha(path[0]) && path[1] == ':')
	retval = path + 2;
    else
	retval = path;
#else
# if defined(AMIGA)
    /* may skip "label:" */
    retval = vim_strchr(path, ':');
    if (retval == NULL)
	retval = path;
# else	/* Unix */
    retval = path;
# endif
#endif

    while (vim_ispathsep(*retval))
	++retval;

    return retval;
}

/*
 * Return TRUE if 'c' is a path separator.
 * Note that for MS-Windows this includes the colon.
 */
    int
vim_ispathsep(int c)
{
#ifdef UNIX
    return (c == '/');	    /* UNIX has ':' inside file names */
#else
# ifdef BACKSLASH_IN_FILENAME
    return (c == ':' || c == '/' || c == '\\');
# else
#  ifdef VMS
    /* server"user passwd"::device:[full.path.name]fname.extension;version" */
    return (c == ':' || c == '[' || c == ']' || c == '/'
	    || c == '<' || c == '>' || c == '"' );
#  else
    return (c == ':' || c == '/');
#  endif /* VMS */
# endif
#endif
}

/*
 * Like vim_ispathsep(c), but exclude the colon for MS-Windows.
 */
    int
vim_ispathsep_nocolon(int c)
{
    return vim_ispathsep(c)
#ifdef BACKSLASH_IN_FILENAME
	&& c != ':'
#endif
	;
}

/*
 * Shorten the path of a file from "~/foo/../.bar/fname" to "~/f/../.b/fname"
 * It's done in-place.
 */
    void
shorten_dir(char_u *str)
{
    char_u	*tail, *s, *d;
    int		skip = FALSE;

    tail = gettail(str);
    d = str;
    for (s = str; ; ++s)
    {
	if (s >= tail)		    /* copy the whole tail */
	{
	    *d++ = *s;
	    if (*s == NUL)
		break;
	}
	else if (vim_ispathsep(*s))	    /* copy '/' and next char */
	{
	    *d++ = *s;
	    skip = FALSE;
	}
	else if (!skip)
	{
	    *d++ = *s;		    /* copy next char */
	    if (*s != '~' && *s != '.') /* and leading "~" and "." */
		skip = TRUE;
	    if (has_mbyte)
	    {
		int l = mb_ptr2len(s);

		while (--l > 0)
		    *d++ = *++s;
	    }
	}
    }
}

/*
 * Return TRUE if the directory of "fname" exists, FALSE otherwise.
 * Also returns TRUE if there is no directory name.
 * "fname" must be writable!.
 */
    int
dir_of_file_exists(char_u *fname)
{
    char_u	*p;
    int		c;
    int		retval;

    p = gettail_sep(fname);
    if (p == fname)
	return TRUE;
    c = *p;
    *p = NUL;
    retval = mch_isdir(fname);
    *p = c;
    return retval;
}

/*
 * Versions of fnamecmp() and fnamencmp() that handle '/' and '\' equally
 * and deal with 'fileignorecase'.
 */
    int
vim_fnamecmp(char_u *x, char_u *y)
{
#ifdef BACKSLASH_IN_FILENAME
    return vim_fnamencmp(x, y, MAXPATHL);
#else
    if (p_fic)
	return MB_STRICMP(x, y);
    return STRCMP(x, y);
#endif
}

    int
vim_fnamencmp(char_u *x, char_u *y, size_t len)
{
#ifdef BACKSLASH_IN_FILENAME
    char_u	*px = x;
    char_u	*py = y;
    int		cx = NUL;
    int		cy = NUL;

    while (len > 0)
    {
	cx = PTR2CHAR(px);
	cy = PTR2CHAR(py);
	if (cx == NUL || cy == NUL
	    || ((p_fic ? MB_TOLOWER(cx) != MB_TOLOWER(cy) : cx != cy)
		&& !(cx == '/' && cy == '\\')
		&& !(cx == '\\' && cy == '/')))
	    break;
	len -= MB_PTR2LEN(px);
	px += MB_PTR2LEN(px);
	py += MB_PTR2LEN(py);
    }
    if (len == 0)
	return 0;
    return (cx - cy);
#else
    if (p_fic)
	return MB_STRNICMP(x, y, len);
    return STRNCMP(x, y, len);
#endif
}

/*
 * Concatenate file names fname1 and fname2 into allocated memory.
 * Only add a '/' or '\\' when 'sep' is TRUE and it is necessary.
 */
    char_u  *
concat_fnames(char_u *fname1, char_u *fname2, int sep)
{
    char_u  *dest;

    dest = alloc((unsigned)(STRLEN(fname1) + STRLEN(fname2) + 3));
    if (dest != NULL)
    {
	STRCPY(dest, fname1);
	if (sep)
	    add_pathsep(dest);
	STRCAT(dest, fname2);
    }
    return dest;
}

/*
 * Concatenate two strings and return the result in allocated memory.
 * Returns NULL when out of memory.
 */
    char_u  *
concat_str(char_u *str1, char_u *str2)
{
    char_u  *dest;
    size_t  l = STRLEN(str1);

    dest = alloc((unsigned)(l + STRLEN(str2) + 1L));
    if (dest != NULL)
    {
	STRCPY(dest, str1);
	STRCPY(dest + l, str2);
    }
    return dest;
}

/*
 * Add a path separator to a file name, unless it already ends in a path
 * separator.
 */
    void
add_pathsep(char_u *p)
{
    if (*p != NUL && !after_pathsep(p, p + STRLEN(p)))
	STRCAT(p, PATHSEPSTR);
}

/*
 * FullName_save - Make an allocated copy of a full file name.
 * Returns NULL when out of memory.
 */
    char_u  *
FullName_save(
    char_u	*fname,
    int		force)		/* force expansion, even when it already looks
				 * like a full path name */
{
    char_u	*buf;
    char_u	*new_fname = NULL;

    if (fname == NULL)
	return NULL;

    buf = alloc((unsigned)MAXPATHL);
    if (buf != NULL)
    {
	if (vim_FullName(fname, buf, MAXPATHL, force) != FAIL)
	    new_fname = vim_strsave(buf);
	else
	    new_fname = vim_strsave(fname);
	vim_free(buf);
    }
    return new_fname;
}

    void
prepare_to_exit(void)
{
#if defined(SIGHUP) && defined(SIG_IGN)
    /* Ignore SIGHUP, because a dropped connection causes a read error, which
     * makes Vim exit and then handling SIGHUP causes various reentrance
     * problems. */
    signal(SIGHUP, SIG_IGN);
#endif

#ifdef FEAT_GUI
    if (gui.in_use)
    {
	gui.dying = TRUE;
	out_trash();	/* trash any pending output */
    }
    else
#endif
    {
	windgoto((int)Rows - 1, 0);

	/*
	 * Switch terminal mode back now, so messages end up on the "normal"
	 * screen (if there are two screens).
	 */
	settmode(TMODE_COOK);
	stoptermcap();
	out_flush();
    }
}

/*
 * Preserve files and exit.
 * When called IObuff must contain a message.
 * NOTE: This may be called from deathtrap() in a signal handler, avoid unsafe
 * functions, such as allocating memory.
 */
    void
preserve_exit(void)
{
    buf_T	*buf;

    prepare_to_exit();

    /* Setting this will prevent free() calls.  That avoids calling free()
     * recursively when free() was invoked with a bad pointer. */
    really_exiting = TRUE;

    out_str(IObuff);
    screen_start();		    /* don't know where cursor is now */
    out_flush();

    ml_close_notmod();		    /* close all not-modified buffers */

    FOR_ALL_BUFFERS(buf)
    {
	if (buf->b_ml.ml_mfp != NULL && buf->b_ml.ml_mfp->mf_fname != NULL)
	{
	    OUT_STR("Vim: preserving files...\n");
	    screen_start();	    /* don't know where cursor is now */
	    out_flush();
	    ml_sync_all(FALSE, FALSE);	/* preserve all swap files */
	    break;
	}
    }

    ml_close_all(FALSE);	    /* close all memfiles, without deleting */

    OUT_STR("Vim: Finished.\n");

    getout(1);
}

/*
 * return TRUE if "fname" exists.
 */
    int
vim_fexists(char_u *fname)
{
    stat_T st;

    if (mch_stat((char *)fname, &st))
	return FALSE;
    return TRUE;
}

/*
 * Check for CTRL-C pressed, but only once in a while.
 * Should be used instead of ui_breakcheck() for functions that check for
 * each line in the file.  Calling ui_breakcheck() each time takes too much
 * time, because it can be a system call.
 */

#ifndef BREAKCHECK_SKIP
# ifdef FEAT_GUI		    /* assume the GUI only runs on fast computers */
#  define BREAKCHECK_SKIP 200
# else
#  define BREAKCHECK_SKIP 32
# endif
#endif

static int	breakcheck_count = 0;

    void
line_breakcheck(void)
{
    if (++breakcheck_count >= BREAKCHECK_SKIP)
    {
	breakcheck_count = 0;
	ui_breakcheck();
    }
}

/*
 * Like line_breakcheck() but check 10 times less often.
 */
    void
fast_breakcheck(void)
{
    if (++breakcheck_count >= BREAKCHECK_SKIP * 10)
    {
	breakcheck_count = 0;
	ui_breakcheck();
    }
}

/*
 * Invoke expand_wildcards() for one pattern.
 * Expand items like "%:h" before the expansion.
 * Returns OK or FAIL.
 */
    int
expand_wildcards_eval(
    char_u	 **pat,		/* pointer to input pattern */
    int		  *num_file,	/* resulting number of files */
    char_u	***file,	/* array of resulting files */
    int		   flags)	/* EW_DIR, etc. */
{
    int		ret = FAIL;
    char_u	*eval_pat = NULL;
    char_u	*exp_pat = *pat;
    char      *ignored_msg;
    int		usedlen;

    if (*exp_pat == '%' || *exp_pat == '#' || *exp_pat == '<')
    {
	++emsg_off;
	eval_pat = eval_vars(exp_pat, exp_pat, &usedlen,
						    NULL, &ignored_msg, NULL);
	--emsg_off;
	if (eval_pat != NULL)
	    exp_pat = concat_str(eval_pat, exp_pat + usedlen);
    }

    if (exp_pat != NULL)
	ret = expand_wildcards(1, &exp_pat, num_file, file, flags);

    if (eval_pat != NULL)
    {
	vim_free(exp_pat);
	vim_free(eval_pat);
    }

    return ret;
}

/*
 * Expand wildcards.  Calls gen_expand_wildcards() and removes files matching
 * 'wildignore'.
 * Returns OK or FAIL.  When FAIL then "num_files" won't be set.
 */
    int
expand_wildcards(
    int		   num_pat,	/* number of input patterns */
    char_u	 **pat,		/* array of input patterns */
    int		  *num_files,	/* resulting number of files */
    char_u	***files,	/* array of resulting files */
    int		   flags)	/* EW_DIR, etc. */
{
    int		retval;
    int		i, j;
    char_u	*p;
    int		non_suf_match;	/* number without matching suffix */

    retval = gen_expand_wildcards(num_pat, pat, num_files, files, flags);

    /* When keeping all matches, return here */
    if ((flags & EW_KEEPALL) || retval == FAIL)
	return retval;

#ifdef FEAT_WILDIGN
    /*
     * Remove names that match 'wildignore'.
     */
    if (*p_wig)
    {
	char_u	*ffname;

	/* check all files in (*files)[] */
	for (i = 0; i < *num_files; ++i)
	{
	    ffname = FullName_save((*files)[i], FALSE);
	    if (ffname == NULL)		/* out of memory */
		break;
# ifdef VMS
	    vms_remove_version(ffname);
# endif
	    if (match_file_list(p_wig, (*files)[i], ffname))
	    {
		/* remove this matching file from the list */
		vim_free((*files)[i]);
		for (j = i; j + 1 < *num_files; ++j)
		    (*files)[j] = (*files)[j + 1];
		--*num_files;
		--i;
	    }
	    vim_free(ffname);
	}

	/* If the number of matches is now zero, we fail. */
	if (*num_files == 0)
	{
	    VIM_CLEAR(*files);
	    return FAIL;
	}
    }
#endif

    /*
     * Move the names where 'suffixes' match to the end.
     */
    if (*num_files > 1)
    {
	non_suf_match = 0;
	for (i = 0; i < *num_files; ++i)
	{
	    if (!match_suffix((*files)[i]))
	    {
		/*
		 * Move the name without matching suffix to the front
		 * of the list.
		 */
		p = (*files)[i];
		for (j = i; j > non_suf_match; --j)
		    (*files)[j] = (*files)[j - 1];
		(*files)[non_suf_match++] = p;
	    }
	}
    }

    return retval;
}

/*
 * Return TRUE if "fname" matches with an entry in 'suffixes'.
 */
    int
match_suffix(char_u *fname)
{
    int		fnamelen, setsuflen;
    char_u	*setsuf;
#define MAXSUFLEN 30	    /* maximum length of a file suffix */
    char_u	suf_buf[MAXSUFLEN];

    fnamelen = (int)STRLEN(fname);
    setsuflen = 0;
    for (setsuf = p_su; *setsuf; )
    {
	setsuflen = copy_option_part(&setsuf, suf_buf, MAXSUFLEN, ".,");
	if (setsuflen == 0)
	{
	    char_u *tail = gettail(fname);

	    /* empty entry: match name without a '.' */
	    if (vim_strchr(tail, '.') == NULL)
	    {
		setsuflen = 1;
		break;
	    }
	}
	else
	{
	    if (fnamelen >= setsuflen
		    && fnamencmp(suf_buf, fname + fnamelen - setsuflen,
						  (size_t)setsuflen) == 0)
		break;
	    setsuflen = 0;
	}
    }
    return (setsuflen != 0);
}

#if !defined(NO_EXPANDPATH) || defined(PROTO)

# ifdef VIM_BACKTICK
static int vim_backtick(char_u *p);
static int expand_backtick(garray_T *gap, char_u *pat, int flags);
# endif

# if defined(MSWIN)
/*
 * File name expansion code for MS-DOS, Win16 and Win32.  It's here because
 * it's shared between these systems.
 */

/*
 * comparison function for qsort in dos_expandpath()
 */
    static int
pstrcmp(const void *a, const void *b)
{
    return (pathcmp(*(char **)a, *(char **)b, -1));
}

/*
 * Recursively expand one path component into all matching files and/or
 * directories.  Adds matches to "gap".  Handles "*", "?", "[a-z]", "**", etc.
 * Return the number of matches found.
 * "path" has backslashes before chars that are not to be expanded, starting
 * at "path[wildoff]".
 * Return the number of matches found.
 * NOTE: much of this is identical to unix_expandpath(), keep in sync!
 */
    static int
dos_expandpath(
    garray_T	*gap,
    char_u	*path,
    int		wildoff,
    int		flags,		/* EW_* flags */
    int		didstar)	/* expanded "**" once already */
{
    char_u	*buf;
    char_u	*path_end;
    char_u	*p, *s, *e;
    int		start_len = gap->ga_len;
    char_u	*pat;
    regmatch_T	regmatch;
    int		starts_with_dot;
    int		matches;
    int		len;
    int		starstar = FALSE;
    static int	stardepth = 0;	    // depth for "**" expansion
    HANDLE		hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW    wfb;
    WCHAR		*wn = NULL;	// UCS-2 name, NULL when not used.
    char_u		*matchname;
    int			ok;

    /* Expanding "**" may take a long time, check for CTRL-C. */
    if (stardepth > 0)
    {
	ui_breakcheck();
	if (got_int)
	    return 0;
    }

    // Make room for file name.  When doing encoding conversion the actual
    // length may be quite a bit longer, thus use the maximum possible length.
    buf = alloc((int)MAXPATHL);
    if (buf == NULL)
	return 0;

    /*
     * Find the first part in the path name that contains a wildcard or a ~1.
     * Copy it into buf, including the preceding characters.
     */
    p = buf;
    s = buf;
    e = NULL;
    path_end = path;
    while (*path_end != NUL)
    {
	/* May ignore a wildcard that has a backslash before it; it will
	 * be removed by rem_backslash() or file_pat_to_reg_pat() below. */
	if (path_end >= path + wildoff && rem_backslash(path_end))
	    *p++ = *path_end++;
	else if (*path_end == '\\' || *path_end == ':' || *path_end == '/')
	{
	    if (e != NULL)
		break;
	    s = p + 1;
	}
	else if (path_end >= path + wildoff
			 && vim_strchr((char_u *)"*?[~", *path_end) != NULL)
	    e = p;
	if (has_mbyte)
	{
	    len = (*mb_ptr2len)(path_end);
	    STRNCPY(p, path_end, len);
	    p += len;
	    path_end += len;
	}
	else
	    *p++ = *path_end++;
    }
    e = p;
    *e = NUL;

    /* now we have one wildcard component between s and e */
    /* Remove backslashes between "wildoff" and the start of the wildcard
     * component. */
    for (p = buf + wildoff; p < s; ++p)
	if (rem_backslash(p))
	{
	    STRMOVE(p, p + 1);
	    --e;
	    --s;
	}

    /* Check for "**" between "s" and "e". */
    for (p = s; p < e; ++p)
	if (p[0] == '*' && p[1] == '*')
	    starstar = TRUE;

    starts_with_dot = *s == '.';
    pat = file_pat_to_reg_pat(s, e, NULL, FALSE);
    if (pat == NULL)
    {
	vim_free(buf);
	return 0;
    }

    /* compile the regexp into a program */
    if (flags & (EW_NOERROR | EW_NOTWILD))
	++emsg_silent;
    regmatch.rm_ic = TRUE;		/* Always ignore case */
    regmatch.regprog = vim_regcomp(pat, RE_MAGIC);
    if (flags & (EW_NOERROR | EW_NOTWILD))
	--emsg_silent;
    vim_free(pat);

    if (regmatch.regprog == NULL && (flags & EW_NOTWILD) == 0)
    {
	vim_free(buf);
	return 0;
    }

    /* remember the pattern or file name being looked for */
    matchname = vim_strsave(s);

    /* If "**" is by itself, this is the first time we encounter it and more
     * is following then find matches without any directory. */
    if (!didstar && stardepth < 100 && starstar && e - s == 2
							  && *path_end == '/')
    {
	STRCPY(s, path_end + 1);
	++stardepth;
	(void)dos_expandpath(gap, buf, (int)(s - buf), flags, TRUE);
	--stardepth;
    }

    /* Scan all files in the directory with "dir/ *.*" */
    STRCPY(s, "*.*");
    wn = enc_to_utf16(buf, NULL);
    if (wn != NULL)
	hFind = FindFirstFileW(wn, &wfb);
    ok = (hFind != INVALID_HANDLE_VALUE);

    while (ok)
    {
	p = utf16_to_enc(wfb.cFileName, NULL);   // p is allocated here
	if (p == NULL)
	    break;  // out of memory

	// Ignore entries starting with a dot, unless when asked for.  Accept
	// all entries found with "matchname".
	if ((p[0] != '.' || starts_with_dot
			 || ((flags & EW_DODOT)
			     && p[1] != NUL && (p[1] != '.' || p[2] != NUL)))
		&& (matchname == NULL
		  || (regmatch.regprog != NULL
				     && vim_regexec(&regmatch, p, (colnr_T)0))
		  || ((flags & EW_NOTWILD)
		     && fnamencmp(path + (s - buf), p, e - s) == 0)))
	{
	    STRCPY(s, p);
	    len = (int)STRLEN(buf);

	    if (starstar && stardepth < 100)
	    {
		/* For "**" in the pattern first go deeper in the tree to
		 * find matches. */
		STRCPY(buf + len, "/**");
		STRCPY(buf + len + 3, path_end);
		++stardepth;
		(void)dos_expandpath(gap, buf, len + 1, flags, TRUE);
		--stardepth;
	    }

	    STRCPY(buf + len, path_end);
	    if (mch_has_exp_wildcard(path_end))
	    {
		/* need to expand another component of the path */
		/* remove backslashes for the remaining components only */
		(void)dos_expandpath(gap, buf, len + 1, flags, FALSE);
	    }
	    else
	    {
		/* no more wildcards, check if there is a match */
		/* remove backslashes for the remaining components only */
		if (*path_end != 0)
		    backslash_halve(buf + len + 1);
		if (mch_getperm(buf) >= 0)	/* add existing file */
		    addfile(gap, buf, flags);
	    }
	}

	vim_free(p);
	ok = FindNextFileW(hFind, &wfb);

	/* If no more matches and no match was used, try expanding the name
	 * itself.  Finds the long name of a short filename. */
	if (!ok && matchname != NULL && gap->ga_len == start_len)
	{
	    STRCPY(s, matchname);
	    FindClose(hFind);
	    vim_free(wn);
	    wn = enc_to_utf16(buf, NULL);
	    if (wn != NULL)
		hFind = FindFirstFileW(wn, &wfb);
	    else
		hFind =	INVALID_HANDLE_VALUE;
	    ok = (hFind != INVALID_HANDLE_VALUE);
	    VIM_CLEAR(matchname);
	}
    }

    FindClose(hFind);
    vim_free(wn);
    vim_free(buf);
    vim_regfree(regmatch.regprog);
    vim_free(matchname);

    matches = gap->ga_len - start_len;
    if (matches > 0)
	qsort(((char_u **)gap->ga_data) + start_len, (size_t)matches,
						   sizeof(char_u *), pstrcmp);
    return matches;
}

    int
mch_expandpath(
    garray_T	*gap,
    char_u	*path,
    int		flags)		/* EW_* flags */
{
    return dos_expandpath(gap, path, 0, flags, FALSE);
}
# endif // MSWIN

#if (defined(UNIX) && !defined(VMS)) || defined(USE_UNIXFILENAME) \
	|| defined(PROTO)
/*
 * Unix style wildcard expansion code.
 * It's here because it's used both for Unix and Mac.
 */
    static int
pstrcmp(const void *a, const void *b)
{
    return (pathcmp(*(char **)a, *(char **)b, -1));
}

/*
 * Recursively expand one path component into all matching files and/or
 * directories.  Adds matches to "gap".  Handles "*", "?", "[a-z]", "**", etc.
 * "path" has backslashes before chars that are not to be expanded, starting
 * at "path + wildoff".
 * Return the number of matches found.
 * NOTE: much of this is identical to dos_expandpath(), keep in sync!
 */
    int
unix_expandpath(
    garray_T	*gap,
    char_u	*path,
    int		wildoff,
    int		flags,		/* EW_* flags */
    int		didstar)	/* expanded "**" once already */
{
    char_u	*buf;
    char_u	*path_end;
    char_u	*p, *s, *e;
    int		start_len = gap->ga_len;
    char_u	*pat;
    regmatch_T	regmatch;
    int		starts_with_dot;
    int		matches;
    int		len;
    int		starstar = FALSE;
    static int	stardepth = 0;	    /* depth for "**" expansion */

    DIR		*dirp;
    struct dirent *dp;

    /* Expanding "**" may take a long time, check for CTRL-C. */
    if (stardepth > 0)
    {
	ui_breakcheck();
	if (got_int)
	    return 0;
    }

    /* make room for file name */
    buf = alloc((int)STRLEN(path) + BASENAMELEN + 5);
    if (buf == NULL)
	return 0;

    /*
     * Find the first part in the path name that contains a wildcard.
     * When EW_ICASE is set every letter is considered to be a wildcard.
     * Copy it into "buf", including the preceding characters.
     */
    p = buf;
    s = buf;
    e = NULL;
    path_end = path;
    while (*path_end != NUL)
    {
	/* May ignore a wildcard that has a backslash before it; it will
	 * be removed by rem_backslash() or file_pat_to_reg_pat() below. */
	if (path_end >= path + wildoff && rem_backslash(path_end))
	    *p++ = *path_end++;
	else if (*path_end == '/')
	{
	    if (e != NULL)
		break;
	    s = p + 1;
	}
	else if (path_end >= path + wildoff
			 && (vim_strchr((char_u *)"*?[{~$", *path_end) != NULL
			     || (!p_fic && (flags & EW_ICASE)
					     && isalpha(PTR2CHAR(path_end)))))
	    e = p;
	if (has_mbyte)
	{
	    len = (*mb_ptr2len)(path_end);
	    STRNCPY(p, path_end, len);
	    p += len;
	    path_end += len;
	}
	else
	    *p++ = *path_end++;
    }
    e = p;
    *e = NUL;

    /* Now we have one wildcard component between "s" and "e". */
    /* Remove backslashes between "wildoff" and the start of the wildcard
     * component. */
    for (p = buf + wildoff; p < s; ++p)
	if (rem_backslash(p))
	{
	    STRMOVE(p, p + 1);
	    --e;
	    --s;
	}

    /* Check for "**" between "s" and "e". */
    for (p = s; p < e; ++p)
	if (p[0] == '*' && p[1] == '*')
	    starstar = TRUE;

    /* convert the file pattern to a regexp pattern */
    starts_with_dot = *s == '.';
    pat = file_pat_to_reg_pat(s, e, NULL, FALSE);
    if (pat == NULL)
    {
	vim_free(buf);
	return 0;
    }

    /* compile the regexp into a program */
    if (flags & EW_ICASE)
	regmatch.rm_ic = TRUE;		/* 'wildignorecase' set */
    else
	regmatch.rm_ic = p_fic;	/* ignore case when 'fileignorecase' is set */
    if (flags & (EW_NOERROR | EW_NOTWILD))
	++emsg_silent;
    regmatch.regprog = vim_regcomp(pat, RE_MAGIC);
    if (flags & (EW_NOERROR | EW_NOTWILD))
	--emsg_silent;
    vim_free(pat);

    if (regmatch.regprog == NULL && (flags & EW_NOTWILD) == 0)
    {
	vim_free(buf);
	return 0;
    }

    /* If "**" is by itself, this is the first time we encounter it and more
     * is following then find matches without any directory. */
    if (!didstar && stardepth < 100 && starstar && e - s == 2
							  && *path_end == '/')
    {
	STRCPY(s, path_end + 1);
	++stardepth;
	(void)unix_expandpath(gap, buf, (int)(s - buf), flags, TRUE);
	--stardepth;
    }

    /* open the directory for scanning */
    *s = NUL;
    dirp = opendir(*buf == NUL ? "." : (char *)buf);

    /* Find all matching entries */
    if (dirp != NULL)
    {
	for (;;)
	{
	    dp = readdir(dirp);
	    if (dp == NULL)
		break;
	    if ((dp->d_name[0] != '.' || starts_with_dot
			|| ((flags & EW_DODOT)
			    && dp->d_name[1] != NUL
			    && (dp->d_name[1] != '.' || dp->d_name[2] != NUL)))
		 && ((regmatch.regprog != NULL && vim_regexec(&regmatch,
					     (char_u *)dp->d_name, (colnr_T)0))
		   || ((flags & EW_NOTWILD)
		     && fnamencmp(path + (s - buf), dp->d_name, e - s) == 0)))
	    {
		STRCPY(s, dp->d_name);
		len = STRLEN(buf);

		if (starstar && stardepth < 100)
		{
		    /* For "**" in the pattern first go deeper in the tree to
		     * find matches. */
		    STRCPY(buf + len, "/**");
		    STRCPY(buf + len + 3, path_end);
		    ++stardepth;
		    (void)unix_expandpath(gap, buf, len + 1, flags, TRUE);
		    --stardepth;
		}

		STRCPY(buf + len, path_end);
		if (mch_has_exp_wildcard(path_end)) /* handle more wildcards */
		{
		    /* need to expand another component of the path */
		    /* remove backslashes for the remaining components only */
		    (void)unix_expandpath(gap, buf, len + 1, flags, FALSE);
		}
		else
		{
		    stat_T  sb;

		    /* no more wildcards, check if there is a match */
		    /* remove backslashes for the remaining components only */
		    if (*path_end != NUL)
			backslash_halve(buf + len + 1);
		    /* add existing file or symbolic link */
		    if ((flags & EW_ALLLINKS) ? mch_lstat((char *)buf, &sb) >= 0
						      : mch_getperm(buf) >= 0)
		    {
#ifdef MACOS_CONVERT
			size_t precomp_len = STRLEN(buf)+1;
			char_u *precomp_buf =
			    mac_precompose_path(buf, precomp_len, &precomp_len);

			if (precomp_buf)
			{
			    mch_memmove(buf, precomp_buf, precomp_len);
			    vim_free(precomp_buf);
			}
#endif
			addfile(gap, buf, flags);
		    }
		}
	    }
	}

	closedir(dirp);
    }

    vim_free(buf);
    vim_regfree(regmatch.regprog);

    matches = gap->ga_len - start_len;
    if (matches > 0)
	qsort(((char_u **)gap->ga_data) + start_len, matches,
						   sizeof(char_u *), pstrcmp);
    return matches;
}
#endif

#if defined(FEAT_SEARCHPATH) || defined(FEAT_CMDL_COMPL) || defined(PROTO)
/*
 * Sort "gap" and remove duplicate entries.  "gap" is expected to contain a
 * list of file names in allocated memory.
 */
    void
remove_duplicates(garray_T *gap)
{
    int	    i;
    int	    j;
    char_u  **fnames = (char_u **)gap->ga_data;

    sort_strings(fnames, gap->ga_len);
    for (i = gap->ga_len - 1; i > 0; --i)
	if (fnamecmp(fnames[i - 1], fnames[i]) == 0)
	{
	    vim_free(fnames[i]);
	    for (j = i + 1; j < gap->ga_len; ++j)
		fnames[j - 1] = fnames[j];
	    --gap->ga_len;
	}
}
#endif

/*
 * Return TRUE if "p" contains what looks like an environment variable.
 * Allowing for escaping.
 */
    static int
has_env_var(char_u *p)
{
    for ( ; *p; MB_PTR_ADV(p))
    {
	if (*p == '\\' && p[1] != NUL)
	    ++p;
	else if (vim_strchr((char_u *)
#if defined(MSWIN)
				    "$%"
#else
				    "$"
#endif
					, *p) != NULL)
	    return TRUE;
    }
    return FALSE;
}

#ifdef SPECIAL_WILDCHAR
/*
 * Return TRUE if "p" contains a special wildcard character, one that Vim
 * cannot expand, requires using a shell.
 */
    static int
has_special_wildchar(char_u *p)
{
    for ( ; *p; MB_PTR_ADV(p))
    {
	// Disallow line break characters.
	if (*p == '\r' || *p == '\n')
	    break;
	// Allow for escaping.
	if (*p == '\\' && p[1] != NUL && p[1] != '\r' && p[1] != '\n')
	    ++p;
	else if (vim_strchr((char_u *)SPECIAL_WILDCHAR, *p) != NULL)
	{
	    // A { must be followed by a matching }.
	    if (*p == '{' && vim_strchr(p, '}') == NULL)
		continue;
	    // A quote and backtick must be followed by another one.
	    if ((*p == '`' || *p == '\'') && vim_strchr(p, *p) == NULL)
		continue;
	    return TRUE;
	}
    }
    return FALSE;
}
#endif

/*
 * Generic wildcard expansion code.
 *
 * Characters in "pat" that should not be expanded must be preceded with a
 * backslash. E.g., "/path\ with\ spaces/my\*star*"
 *
 * Return FAIL when no single file was found.  In this case "num_file" is not
 * set, and "file" may contain an error message.
 * Return OK when some files found.  "num_file" is set to the number of
 * matches, "file" to the array of matches.  Call FreeWild() later.
 */
    int
gen_expand_wildcards(
    int		num_pat,	/* number of input patterns */
    char_u	**pat,		/* array of input patterns */
    int		*num_file,	/* resulting number of files */
    char_u	***file,	/* array of resulting files */
    int		flags)		/* EW_* flags */
{
    int			i;
    garray_T		ga;
    char_u		*p;
    static int		recursive = FALSE;
    int			add_pat;
    int			retval = OK;
#if defined(FEAT_SEARCHPATH)
    int			did_expand_in_path = FALSE;
#endif

    /*
     * expand_env() is called to expand things like "~user".  If this fails,
     * it calls ExpandOne(), which brings us back here.  In this case, always
     * call the machine specific expansion function, if possible.  Otherwise,
     * return FAIL.
     */
    if (recursive)
#ifdef SPECIAL_WILDCHAR
	return mch_expand_wildcards(num_pat, pat, num_file, file, flags);
#else
	return FAIL;
#endif

#ifdef SPECIAL_WILDCHAR
    /*
     * If there are any special wildcard characters which we cannot handle
     * here, call machine specific function for all the expansion.  This
     * avoids starting the shell for each argument separately.
     * For `=expr` do use the internal function.
     */
    for (i = 0; i < num_pat; i++)
    {
	if (has_special_wildchar(pat[i])
# ifdef VIM_BACKTICK
		&& !(vim_backtick(pat[i]) && pat[i][1] == '=')
# endif
	   )
	    return mch_expand_wildcards(num_pat, pat, num_file, file, flags);
    }
#endif

    recursive = TRUE;

    /*
     * The matching file names are stored in a growarray.  Init it empty.
     */
    ga_init2(&ga, (int)sizeof(char_u *), 30);

    for (i = 0; i < num_pat; ++i)
    {
	add_pat = -1;
	p = pat[i];

#ifdef VIM_BACKTICK
	if (vim_backtick(p))
	{
	    add_pat = expand_backtick(&ga, p, flags);
	    if (add_pat == -1)
		retval = FAIL;
	}
	else
#endif
	{
	    /*
	     * First expand environment variables, "~/" and "~user/".
	     */
	    if (has_env_var(p) || *p == '~')
	    {
		p = expand_env_save_opt(p, TRUE);
		if (p == NULL)
		    p = pat[i];
#ifdef UNIX
		/*
		 * On Unix, if expand_env() can't expand an environment
		 * variable, use the shell to do that.  Discard previously
		 * found file names and start all over again.
		 */
		else if (has_env_var(p) || *p == '~')
		{
		    vim_free(p);
		    ga_clear_strings(&ga);
		    i = mch_expand_wildcards(num_pat, pat, num_file, file,
							 flags|EW_KEEPDOLLAR);
		    recursive = FALSE;
		    return i;
		}
#endif
	    }

	    /*
	     * If there are wildcards: Expand file names and add each match to
	     * the list.  If there is no match, and EW_NOTFOUND is given, add
	     * the pattern.
	     * If there are no wildcards: Add the file name if it exists or
	     * when EW_NOTFOUND is given.
	     */
	    if (mch_has_exp_wildcard(p))
	    {
#if defined(FEAT_SEARCHPATH)
		if ((flags & EW_PATH)
			&& !mch_isFullName(p)
			&& !(p[0] == '.'
			    && (vim_ispathsep(p[1])
				|| (p[1] == '.' && vim_ispathsep(p[2]))))
		   )
		{
		    /* :find completion where 'path' is used.
		     * Recursiveness is OK here. */
		    recursive = FALSE;
		    add_pat = expand_in_path(&ga, p, flags);
		    recursive = TRUE;
		    did_expand_in_path = TRUE;
		}
		else
#endif
		    add_pat = mch_expandpath(&ga, p, flags);
	    }
	}

	if (add_pat == -1 || (add_pat == 0 && (flags & EW_NOTFOUND)))
	{
	    char_u	*t = backslash_halve_save(p);

	    /* When EW_NOTFOUND is used, always add files and dirs.  Makes
	     * "vim c:/" work. */
	    if (flags & EW_NOTFOUND)
		addfile(&ga, t, flags | EW_DIR | EW_FILE);
	    else
		addfile(&ga, t, flags);
	    vim_free(t);
	}

#if defined(FEAT_SEARCHPATH)
	if (did_expand_in_path && ga.ga_len > 0 && (flags & EW_PATH))
	    uniquefy_paths(&ga, p);
#endif
	if (p != pat[i])
	    vim_free(p);
    }

    *num_file = ga.ga_len;
    *file = (ga.ga_data != NULL) ? (char_u **)ga.ga_data : (char_u **)"";

    recursive = FALSE;

    return ((flags & EW_EMPTYOK) || ga.ga_data != NULL) ? retval : FAIL;
}

# ifdef VIM_BACKTICK

/*
 * Return TRUE if we can expand this backtick thing here.
 */
    static int
vim_backtick(char_u *p)
{
    return (*p == '`' && *(p + 1) != NUL && *(p + STRLEN(p) - 1) == '`');
}

/*
 * Expand an item in `backticks` by executing it as a command.
 * Currently only works when pat[] starts and ends with a `.
 * Returns number of file names found, -1 if an error is encountered.
 */
    static int
expand_backtick(
    garray_T	*gap,
    char_u	*pat,
    int		flags)	/* EW_* flags */
{
    char_u	*p;
    char_u	*cmd;
    char_u	*buffer;
    int		cnt = 0;
    int		i;

    /* Create the command: lop off the backticks. */
    cmd = vim_strnsave(pat + 1, (int)STRLEN(pat) - 2);
    if (cmd == NULL)
	return -1;

#ifdef FEAT_EVAL
    if (*cmd == '=')	    /* `={expr}`: Expand expression */
	buffer = eval_to_string(cmd + 1, &p, TRUE);
    else
#endif
	buffer = get_cmd_output(cmd, NULL,
				(flags & EW_SILENT) ? SHELL_SILENT : 0, NULL);
    vim_free(cmd);
    if (buffer == NULL)
	return -1;

    cmd = buffer;
    while (*cmd != NUL)
    {
	cmd = skipwhite(cmd);		/* skip over white space */
	p = cmd;
	while (*p != NUL && *p != '\r' && *p != '\n') /* skip over entry */
	    ++p;
	/* add an entry if it is not empty */
	if (p > cmd)
	{
	    i = *p;
	    *p = NUL;
	    addfile(gap, cmd, flags);
	    *p = i;
	    ++cnt;
	}
	cmd = p;
	while (*cmd != NUL && (*cmd == '\r' || *cmd == '\n'))
	    ++cmd;
    }

    vim_free(buffer);
    return cnt;
}
# endif /* VIM_BACKTICK */

/*
 * Add a file to a file list.  Accepted flags:
 * EW_DIR	add directories
 * EW_FILE	add files
 * EW_EXEC	add executable files
 * EW_NOTFOUND	add even when it doesn't exist
 * EW_ADDSLASH	add slash after directory name
 * EW_ALLLINKS	add symlink also when the referred file does not exist
 */
    void
addfile(
    garray_T	*gap,
    char_u	*f,	/* filename */
    int		flags)
{
    char_u	*p;
    int		isdir;
    stat_T	sb;

    /* if the file/dir/link doesn't exist, may not add it */
    if (!(flags & EW_NOTFOUND) && ((flags & EW_ALLLINKS)
			? mch_lstat((char *)f, &sb) < 0 : mch_getperm(f) < 0))
	return;

#ifdef FNAME_ILLEGAL
    /* if the file/dir contains illegal characters, don't add it */
    if (vim_strpbrk(f, (char_u *)FNAME_ILLEGAL) != NULL)
	return;
#endif

    isdir = mch_isdir(f);
    if ((isdir && !(flags & EW_DIR)) || (!isdir && !(flags & EW_FILE)))
	return;

    /* If the file isn't executable, may not add it.  Do accept directories.
     * When invoked from expand_shellcmd() do not use $PATH. */
    if (!isdir && (flags & EW_EXEC)
			     && !mch_can_exe(f, NULL, !(flags & EW_SHELLCMD)))
	return;

    /* Make room for another item in the file list. */
    if (ga_grow(gap, 1) == FAIL)
	return;

    p = alloc((unsigned)(STRLEN(f) + 1 + isdir));
    if (p == NULL)
	return;

    STRCPY(p, f);
#ifdef BACKSLASH_IN_FILENAME
    slash_adjust(p);
#endif
    /*
     * Append a slash or backslash after directory names if none is present.
     */
#ifndef DONT_ADD_PATHSEP_TO_DIR
    if (isdir && (flags & EW_ADDSLASH))
	add_pathsep(p);
#endif
    ((char_u **)gap->ga_data)[gap->ga_len++] = p;
}
#endif /* !NO_EXPANDPATH */

#if defined(VIM_BACKTICK) || defined(FEAT_EVAL) || defined(PROTO)

#ifndef SEEK_SET
# define SEEK_SET 0
#endif
#ifndef SEEK_END
# define SEEK_END 2
#endif

/*
 * Get the stdout of an external command.
 * If "ret_len" is NULL replace NUL characters with NL.  When "ret_len" is not
 * NULL store the length there.
 * Returns an allocated string, or NULL for error.
 */
    char_u *
get_cmd_output(
    char_u	*cmd,
    char_u	*infile,	/* optional input file name */
    int		flags,		/* can be SHELL_SILENT */
    int		*ret_len)
{
    char_u	*tempname;
    char_u	*command;
    char_u	*buffer = NULL;
    int		len;
    int		i = 0;
    FILE	*fd;

    if (check_restricted() || check_secure())
	return NULL;

    /* get a name for the temp file */
    if ((tempname = vim_tempname('o', FALSE)) == NULL)
    {
	emsg(_(e_notmp));
	return NULL;
    }

    /* Add the redirection stuff */
    command = make_filter_cmd(cmd, infile, tempname);
    if (command == NULL)
	goto done;

    /*
     * Call the shell to execute the command (errors are ignored).
     * Don't check timestamps here.
     */
    ++no_check_timestamps;
    call_shell(command, SHELL_DOOUT | SHELL_EXPAND | flags);
    --no_check_timestamps;

    vim_free(command);

    /*
     * read the names from the file into memory
     */
# ifdef VMS
    /* created temporary file is not always readable as binary */
    fd = mch_fopen((char *)tempname, "r");
# else
    fd = mch_fopen((char *)tempname, READBIN);
# endif

    if (fd == NULL)
    {
	semsg(_(e_notopen), tempname);
	goto done;
    }

    fseek(fd, 0L, SEEK_END);
    len = ftell(fd);		    /* get size of temp file */
    fseek(fd, 0L, SEEK_SET);

    buffer = alloc(len + 1);
    if (buffer != NULL)
	i = (int)fread((char *)buffer, (size_t)1, (size_t)len, fd);
    fclose(fd);
    mch_remove(tempname);
    if (buffer == NULL)
	goto done;
#ifdef VMS
    len = i;	/* VMS doesn't give us what we asked for... */
#endif
    if (i != len)
    {
	semsg(_(e_notread), tempname);
	VIM_CLEAR(buffer);
    }
    else if (ret_len == NULL)
    {
	/* Change NUL into SOH, otherwise the string is truncated. */
	for (i = 0; i < len; ++i)
	    if (buffer[i] == NUL)
		buffer[i] = 1;

	buffer[len] = NUL;	/* make sure the buffer is terminated */
    }
    else
	*ret_len = len;

done:
    vim_free(tempname);
    return buffer;
}
#endif

/*
 * Free the list of files returned by expand_wildcards() or other expansion
 * functions.
 */
    void
FreeWild(int count, char_u **files)
{
    if (count <= 0 || files == NULL)
	return;
    while (count--)
	vim_free(files[count]);
    vim_free(files);
}

/*
 * Return TRUE when need to go to Insert mode because of 'insertmode'.
 * Don't do this when still processing a command or a mapping.
 * Don't do this when inside a ":normal" command.
 */
    int
goto_im(void)
{
    return (p_im && stuff_empty() && typebuf_typed());
}

/*
 * Returns the isolated name of the shell in allocated memory:
 * - Skip beyond any path.  E.g., "/usr/bin/csh -f" -> "csh -f".
 * - Remove any argument.  E.g., "csh -f" -> "csh".
 * But don't allow a space in the path, so that this works:
 *   "/usr/bin/csh --rcfile ~/.cshrc"
 * But don't do that for Windows, it's common to have a space in the path.
 */
    char_u *
get_isolated_shell_name(void)
{
    char_u *p;

#ifdef MSWIN
    p = gettail(p_sh);
    p = vim_strnsave(p, (int)(skiptowhite(p) - p));
#else
    p = skiptowhite(p_sh);
    if (*p == NUL)
    {
	/* No white space, use the tail. */
	p = vim_strsave(gettail(p_sh));
    }
    else
    {
	char_u  *p1, *p2;

	/* Find the last path separator before the space. */
	p1 = p_sh;
	for (p2 = p_sh; p2 < p; MB_PTR_ADV(p2))
	    if (vim_ispathsep(*p2))
		p1 = p2 + 1;
	p = vim_strnsave(p1, (int)(p - p1));
    }
#endif
    return p;
}

/*
 * Check if the "://" of a URL is at the pointer, return URL_SLASH.
 * Also check for ":\\", which MS Internet Explorer accepts, return
 * URL_BACKSLASH.
 */
    int
path_is_url(char_u *p)
{
    if (STRNCMP(p, "://", (size_t)3) == 0)
	return URL_SLASH;
    else if (STRNCMP(p, ":\\\\", (size_t)3) == 0)
	return URL_BACKSLASH;
    return 0;
}

/*
 * Check if "fname" starts with "name://".  Return URL_SLASH if it does.
 * Return URL_BACKSLASH for "name:\\".
 * Return zero otherwise.
 */
    int
path_with_url(char_u *fname)
{
    char_u *p;

    for (p = fname; isalpha(*p); ++p)
	;
    return path_is_url(p);
}

/*
 * Return TRUE if "name" is a full (absolute) path name or URL.
 */
    int
vim_isAbsName(char_u *name)
{
    return (path_with_url(name) != 0 || mch_isFullName(name));
}

/*
 * Get absolute file name into buffer "buf[len]".
 *
 * return FAIL for failure, OK otherwise
 */
    int
vim_FullName(
    char_u	*fname,
    char_u	*buf,
    int		len,
    int		force)	    /* force expansion even when already absolute */
{
    int		retval = OK;
    int		url;

    *buf = NUL;
    if (fname == NULL)
	return FAIL;

    url = path_with_url(fname);
    if (!url)
	retval = mch_FullName(fname, buf, len, force);
    if (url || retval == FAIL)
    {
	/* something failed; use the file name (truncate when too long) */
	vim_strncpy(buf, fname, len - 1);
    }
#if defined(MSWIN)
    slash_adjust(buf);
#endif
    return retval;
}
