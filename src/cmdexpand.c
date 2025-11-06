/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * cmdexpand.c: functions for command-line completion
 */

#include "vim.h"

static int	cmd_showtail;	// Only show path tail in lists ?
static int	may_expand_pattern = FALSE;
static pos_T	pre_incsearch_pos; // Cursor position when incsearch started

static void	set_context_for_wildcard_arg(exarg_T *eap, char_u *arg, int usefilter, expand_T *xp, int *complp);
static int	ExpandFromContext(expand_T *xp, char_u *, char_u ***, int *, int);
static char_u	*showmatches_gettail(char_u *s);
static int	expand_showtail(expand_T *xp);
static int	expand_shellcmd(char_u *filepat, char_u ***matches, int *numMatches, int flagsarg);
#if defined(FEAT_EVAL)
static int	ExpandUserDefined(char_u *pat, expand_T *xp, regmatch_T *regmatch, char_u ***matches, int *numMatches);
static int	ExpandUserList(expand_T *xp, char_u ***matches, int *numMatches);
#endif
static int	expand_pattern_in_buf(char_u *pat, int dir, char_u ***matches, int *numMatches);

// "compl_match_array" points the currently displayed list of entries in the
// popup menu.  It is NULL when there is no popup menu.
static pumitem_T *compl_match_array = NULL;
static int compl_match_arraysize;
// First column in cmdline of the matched item for completion.
static int compl_startcol;
static int compl_selected;
// cmdline before expansion
static char_u *cmdline_orig = NULL;

#define SHOW_MATCH(m) (showtail ? showmatches_gettail(matches[m]) : matches[m])

/*
 * Returns TRUE if fuzzy completion is supported for a given cmdline completion
 * context.
 */
    static int
cmdline_fuzzy_completion_supported(expand_T *xp)
{
    switch (xp->xp_context)
    {
    case EXPAND_BOOL_SETTINGS:
    case EXPAND_COLORS:
    case EXPAND_COMPILER:
    case EXPAND_DIRECTORIES:
    case EXPAND_DIRS_IN_CDPATH:
    case EXPAND_FILES:
    case EXPAND_FILES_IN_PATH:
    case EXPAND_FILETYPE:
    case EXPAND_FILETYPECMD:
    case EXPAND_HELP:
    case EXPAND_KEYMAP:
    case EXPAND_OLD_SETTING:
    case EXPAND_STRING_SETTING:
    case EXPAND_SETTING_SUBTRACT:
    case EXPAND_OWNSYNTAX:
    case EXPAND_PACKADD:
    case EXPAND_RUNTIME:
    case EXPAND_SHELLCMD:
    case EXPAND_SHELLCMDLINE:
    case EXPAND_TAGS:
    case EXPAND_TAGS_LISTFILES:
	return FALSE;

    default:
	break;
    }

    return vim_strchr(p_wop, WOP_FUZZY) != NULL;
}

/*
 * Returns TRUE if fuzzy completion for cmdline completion is enabled and
 * 'fuzzystr' is not empty.  If search pattern is empty, then don't use fuzzy
 * matching.
 */
    int
cmdline_fuzzy_complete(char_u *fuzzystr)
{
    return vim_strchr(p_wop, WOP_FUZZY) != NULL && *fuzzystr != NUL;
}

/*
 * sort function for the completion matches.
 * <SNR> functions should be sorted to the end.
 */
    static int
sort_func_compare(const void *s1, const void *s2)
{
    char_u *p1 = *(char_u **)s1;
    char_u *p2 = *(char_u **)s2;

    if (*p1 != '<' && *p2 == '<') return -1;
    if (*p1 == '<' && *p2 != '<') return 1;
    return STRCMP(p1, p2);
}

/*
 * Escape special characters in the cmdline completion matches.
 */
    static void
wildescape(
    expand_T	*xp,
    char_u	*str,
    int		numfiles,
    char_u	**files)
{
    char_u	*p;
    int		vse_what = xp->xp_context == EXPAND_BUFFERS
						       ? VSE_BUFFER : VSE_NONE;

    if (xp->xp_context == EXPAND_FILES
	    || xp->xp_context == EXPAND_FILES_IN_PATH
	    || xp->xp_context == EXPAND_SHELLCMD
	    || xp->xp_context == EXPAND_BUFFERS
	    || xp->xp_context == EXPAND_DIRECTORIES
	    || xp->xp_context == EXPAND_DIRS_IN_CDPATH)
    {
	// Insert a backslash into a file name before a space, \, %, #
	// and wildmatch characters, except '~'.
	for (int i = 0; i < numfiles; ++i)
	{
	    // for ":set path=" we need to escape spaces twice
	    if (xp->xp_backslash & XP_BS_THREE)
	    {
		char *pat = (xp->xp_backslash & XP_BS_COMMA) ?  " ," : " ";
		p = vim_strsave_escaped(files[i], (char_u *)pat);
		if (p != NULL)
		{
		    vim_free(files[i]);
		    files[i] = p;
#if defined(BACKSLASH_IN_FILENAME)
		    p = vim_strsave_escaped(files[i], (char_u *)" ");
		    if (p != NULL)
		    {
			vim_free(files[i]);
			files[i] = p;
		    }
#endif
		}
	    }
	    else if (xp->xp_backslash & XP_BS_COMMA)
	    {
		if (vim_strchr(files[i], ',') != NULL)
		{
		    p = vim_strsave_escaped(files[i], (char_u *)",");
		    if (p != NULL)
		    {
			vim_free(files[i]);
			files[i] = p;
		    }
		}
	    }
#ifdef BACKSLASH_IN_FILENAME
	    p = vim_strsave_fnameescape(files[i], vse_what);
#else
	    p = vim_strsave_fnameescape(files[i],
		    xp->xp_shell ? VSE_SHELL : vse_what);
#endif
	    if (p != NULL)
	    {
		vim_free(files[i]);
		files[i] = p;
	    }

	    // If 'str' starts with "\~", replace "~" at start of
	    // files[i] with "\~".
	    if (str[0] == '\\' && str[1] == '~' && files[i][0] == '~')
		escape_fname(&files[i]);
	}
	xp->xp_backslash = XP_BS_NONE;

	// If the first file starts with a '+' escape it.  Otherwise it
	// could be seen as "+cmd".
	if (*files[0] == '+')
	    escape_fname(&files[0]);
    }
    else if (xp->xp_context == EXPAND_TAGS)
    {
	// Insert a backslash before characters in a tag name that
	// would terminate the ":tag" command.
	for (int i = 0; i < numfiles; ++i)
	{
	    p = vim_strsave_escaped(files[i], (char_u *)"\\|\"");
	    if (p != NULL)
	    {
		vim_free(files[i]);
		files[i] = p;
	    }
	}
    }
}

/*
 * Escape special characters in the cmdline completion matches.
 */
    static void
ExpandEscape(
    expand_T	*xp,
    char_u	*str,
    int		numfiles,
    char_u	**files,
    int		options)
{
    // May change home directory back to "~"
    if (options & WILD_HOME_REPLACE)
	tilde_replace(str, numfiles, files);

    if (options & WILD_ESCAPE)
	wildescape(xp, str, numfiles, files);
}

/*
 * Return FAIL if this is not an appropriate context in which to do
 * completion of anything, return OK if it is (even if there are no matches).
 * For the caller, this means that the character is just passed through like a
 * normal character (instead of being expanded).  This allows :s/^I^D etc.
 */
    int
nextwild(
    expand_T	*xp,
    int		type,
    int		options,	// extra options for ExpandOne()
    int		escape)		// if TRUE, escape the returned matches
{
    cmdline_info_T	*ccline = get_cmdline_info();
    int		i;
    char_u	*p;
    int		from_wildtrigger_func = options & WILD_FUNC_TRIGGER;
    int		wild_navigate = (type == WILD_NEXT || type == WILD_PREV
	    || type == WILD_PAGEUP || type == WILD_PAGEDOWN);

    if (xp->xp_numfiles == -1)
    {
	pre_incsearch_pos = xp->xp_pre_incsearch_pos;
#ifdef FEAT_EVAL
	if (ccline->input_fn && ccline->xp_context == EXPAND_COMMANDS)
	{
	    // Expand commands typed in input() function
	    set_cmd_context(xp, ccline->cmdbuff, ccline->cmdlen, ccline->cmdpos, FALSE);
	}
	else
#endif
	{
	    may_expand_pattern = options & WILD_MAY_EXPAND_PATTERN;
	    set_expand_context(xp);
	    may_expand_pattern = FALSE;
	}
	cmd_showtail = expand_showtail(xp);
    }

    if (xp->xp_context == EXPAND_UNSUCCESSFUL)
    {
	beep_flush();
	return OK;  // Something illegal on command line
    }
    if (xp->xp_context == EXPAND_NOTHING)
    {
	// Caller can use the character as a normal char instead
	return FAIL;
    }

    i = (int)(xp->xp_pattern - ccline->cmdbuff);
    xp->xp_pattern_len = ccline->cmdpos - i;

    // Skip showing matches if prefix is invalid during wildtrigger()
    if (from_wildtrigger_func && xp->xp_context == EXPAND_COMMANDS
	    && xp->xp_pattern_len == 0)
	return FAIL;

    // If cmd_silent is set then don't show the dots, because redrawcmd() below
    // won't remove them.
    if (!cmd_silent && !from_wildtrigger_func && !wild_navigate)
    {
	msg_puts("...");	    // show that we are busy
	out_flush();
    }

    if (wild_navigate)
    {
	// Get next/previous match for a previous expanded pattern.
	p = ExpandOne(xp, NULL, NULL, 0, type);
    }
    else
    {
	char_u	*tmp;

	if (cmdline_fuzzy_completion_supported(xp)
		|| xp->xp_context == EXPAND_PATTERN_IN_BUF)
	    // Don't modify the search string
	    tmp = vim_strnsave(xp->xp_pattern, xp->xp_pattern_len);
	else
	    tmp = addstar(xp->xp_pattern, xp->xp_pattern_len, xp->xp_context);

	// Translate string into pattern and expand it.
	if (tmp == NULL)
	    p = NULL;
	else
	{
	    int use_options = options |
		    WILD_HOME_REPLACE|WILD_ADD_SLASH|WILD_SILENT;
	    if (escape)
		use_options |= WILD_ESCAPE;
	    if (p_wic)
		use_options += WILD_ICASE;

	    p = ExpandOne(xp, tmp,
			 vim_strnsave(&ccline->cmdbuff[i], xp->xp_pattern_len),
							   use_options, type);
	    vim_free(tmp);
	    // longest match: make sure it is not shorter, happens with :help
	    if (p != NULL && type == WILD_LONGEST)
	    {
		int	j;

		for (j = 0; j < xp->xp_pattern_len; ++j)
		{
		    char_u  c = ccline->cmdbuff[i + j];
		    if (c == '*' || c == '?')
			break;
		}
		if ((int)STRLEN(p) < j)
		    VIM_CLEAR(p);
	    }
	}
    }

    // Save cmdline before inserting selected item
    if (!wild_navigate && ccline->cmdbuff != NULL)
    {
	vim_free(cmdline_orig);
	cmdline_orig = vim_strnsave(ccline->cmdbuff, ccline->cmdlen);
    }

    if (p != NULL && !got_int && !(options & WILD_NOSELECT))
    {
	size_t	plen = STRLEN(p);
	int	difflen;
	int	v = OK;

	difflen = (int)plen - xp->xp_pattern_len;
	if (ccline->cmdlen + difflen + 4 > ccline->cmdbufflen)
	{
	    v = realloc_cmdbuff(ccline->cmdlen + difflen + 4);
	    xp->xp_pattern = ccline->cmdbuff + i;
	}

	if (v == OK)
	{
	    mch_memmove(&ccline->cmdbuff[ccline->cmdpos + difflen],
		    &ccline->cmdbuff[ccline->cmdpos],
		    (size_t)(ccline->cmdlen - ccline->cmdpos + 1));
	    mch_memmove(&ccline->cmdbuff[i], p, plen);
	    ccline->cmdlen += difflen;
	    ccline->cmdpos += difflen;
	}
    }

    redrawcmd();
    cursorcmd();

    // When expanding a ":map" command and no matches are found, assume that
    // the key is supposed to be inserted literally
    if (xp->xp_context == EXPAND_MAPPINGS && p == NULL)
	return FAIL;

    if (xp->xp_numfiles <= 0 && p == NULL)
	beep_flush();
    else if (xp->xp_numfiles == 1 && !(options & WILD_NOSELECT)
	    && !wild_navigate)
	// free expanded pattern
	(void)ExpandOne(xp, NULL, NULL, 0, WILD_FREE);

    vim_free(p);

    return OK;
}

/*
 * Create completion popup menu with items from 'matches'.
 */
    static int
cmdline_pum_create(
	cmdline_info_T	*ccline,
	expand_T	*xp,
	char_u		**matches,
	int		numMatches,
	int		showtail)
{
    int	prefix_len;

    // Add all the completion matches
    compl_match_array = ALLOC_MULT(pumitem_T, numMatches);
    if (compl_match_array == NULL)
	return EXPAND_UNSUCCESSFUL;

    compl_match_arraysize = numMatches;
    for (int i = 0; i < numMatches; i++)
    {
	compl_match_array[i].pum_text = SHOW_MATCH(i);
	compl_match_array[i].pum_info = NULL;
	compl_match_array[i].pum_extra = NULL;
	compl_match_array[i].pum_kind = NULL;
	compl_match_array[i].pum_user_abbr_hlattr = -1;
	compl_match_array[i].pum_user_kind_hlattr = -1;
    }

    // Compute the popup menu starting column
    compl_startcol = ccline == NULL ? 0 : vim_strsize(ccline->cmdbuff) + 1;
    prefix_len = vim_strsize(xp->xp_pattern);
    if (showtail)
	prefix_len += vim_strsize(showmatches_gettail(matches[0]))
	    - vim_strsize(matches[0]);
    compl_startcol = cmdline_col_off + MAX(0, compl_startcol - prefix_len);

    return EXPAND_OK;
}

/*
 * Display the cmdline completion matches in a popup menu
 */
    void
cmdline_pum_display(void)
{
    pum_display(compl_match_array, compl_match_arraysize, compl_selected);
}

/*
 * Returns TRUE if the cmdline completion popup menu is being displayed.
 */
    int
cmdline_pum_active(void)
{
    return pum_visible() && compl_match_array != NULL;
}

/*
 * Remove the cmdline completion popup menu (if present), free the list of
 * items and refresh the screen.
 */
    void
cmdline_pum_remove(cmdline_info_T *cclp UNUSED, int defer_redraw)
{
    int	save_KeyTyped = KeyTyped;
#ifdef FEAT_EVAL
    int	save_RedrawingDisabled = RedrawingDisabled;
    if (cclp->input_fn)
	RedrawingDisabled = 0;
#endif

    pum_undisplay();
    VIM_CLEAR(compl_match_array);
    compl_match_arraysize = 0;
    if (!defer_redraw)
    {
	int save_p_lz = p_lz;
	p_lz = FALSE;  // avoid the popup menu hanging around
	update_screen(0);
	p_lz = save_p_lz;
    }
    else
	pum_call_update_screen();
    redrawcmd();

    // When a function is called (e.g. for 'foldtext') KeyTyped might be reset
    // as a side effect.
    KeyTyped = save_KeyTyped;
#ifdef FEAT_EVAL
    if (cclp->input_fn)
	RedrawingDisabled = save_RedrawingDisabled;
#endif
}

    void
cmdline_pum_cleanup(cmdline_info_T *cclp)
{
    cmdline_pum_remove(cclp, FALSE);
    wildmenu_cleanup(cclp);
}

/*
 * Returns the starting column number to use for the cmdline completion popup
 * menu.
 */
    int
cmdline_compl_startcol(void)
{
    return compl_startcol;
}

/*
 * Returns the current cmdline completion pattern.
 */
    char_u *
cmdline_compl_pattern(void)
{
    expand_T	*xp = get_cmdline_info()->xpc;

    return xp == NULL ? NULL : xp->xp_orig;
}

/*
 * Returns TRUE if fuzzy cmdline completion is active, FALSE otherwise.
 */
    int
cmdline_compl_is_fuzzy(void)
{
    expand_T	*xp = get_cmdline_info()->xpc;

    return xp != NULL && cmdline_fuzzy_completion_supported(xp);
}

/*
 * Return the number of characters that should be skipped in a status match.
 * These are backslashes used for escaping.  Do show backslashes in help tags
 * and in search pattern completion matches.
 */
    static int
skip_status_match_char(expand_T *xp, char_u *s)
{
    if ((rem_backslash(s) && xp->xp_context != EXPAND_HELP
		&& xp->xp_context != EXPAND_PATTERN_IN_BUF)
#ifdef FEAT_MENU
	    || ((xp->xp_context == EXPAND_MENUS
		    || xp->xp_context == EXPAND_MENUNAMES)
			  && (s[0] == '\t' || (s[0] == '\\' && s[1] != NUL)))
#endif
	   )
    {
#ifndef BACKSLASH_IN_FILENAME
	if (xp->xp_shell && csh_like_shell() && s[1] == '\\' && s[2] == '!')
	    return 2;
#endif
	return 1;
    }
    return 0;
}

/*
 * Get the length of an item as it will be shown in the status line.
 */
    static int
status_match_len(expand_T *xp, char_u *s)
{
    int	len = 0;

#ifdef FEAT_MENU
    int emenu = xp->xp_context == EXPAND_MENUS
	     || xp->xp_context == EXPAND_MENUNAMES;

    // Check for menu separators - replace with '|'.
    if (emenu && menu_is_separator(s))
	return 1;
#endif

    while (*s != NUL)
    {
	s += skip_status_match_char(xp, s);
	len += ptr2cells(s);
	MB_PTR_ADV(s);
    }

    return len;
}

/*
 * Show wildchar matches in the status line.
 * Show at least the "match" item.
 * We start at item 'first_match' in the list and show all matches that fit.
 *
 * If inversion is possible we use it. Else '=' characters are used.
 */
    static void
win_redr_status_matches(
    expand_T	*xp,
    int		num_matches,
    char_u	**matches,	// list of matches
    int		match,
    int		showtail)
{
    int		row;
    char_u	*buf;
    int		len;
    int		clen;		// length in screen cells
    int		fillchar;
    int		attr;
    int		i;
    int		highlight = TRUE;
    char_u	*selstart = NULL;
    int		selstart_col = 0;
    char_u	*selend = NULL;
    static int	first_match = 0;
    int		add_left = FALSE;
    char_u	*s;
#ifdef FEAT_MENU
    int		emenu;
#endif
    int		l;

    if (matches == NULL)	// interrupted completion?
	return;

    if (has_mbyte)
	buf = alloc(topframe->fr_width * MB_MAXBYTES + 1);
    else
	buf = alloc(topframe->fr_width + 1);
    if (buf == NULL)
	return;

    if (match == -1)	// don't show match but original text
    {
	match = 0;
	highlight = FALSE;
    }
    // count 1 for the ending ">"
    clen = status_match_len(xp, SHOW_MATCH(match)) + 3;
    if (match == 0)
	first_match = 0;
    else if (match < first_match)
    {
	// jumping left, as far as we can go
	first_match = match;
	add_left = TRUE;
    }
    else
    {
	// check if match fits on the screen
	for (i = first_match; i < match; ++i)
	    clen += status_match_len(xp, SHOW_MATCH(i)) + 2;
	if (first_match > 0)
	    clen += 2;
	// jumping right, put match at the left
	if (clen > topframe->fr_width)
	{
	    first_match = match;
	    // if showing the last match, we can add some on the left
	    clen = 2;
	    for (i = match; i < num_matches; ++i)
	    {
		clen += status_match_len(xp, SHOW_MATCH(i)) + 2;
		if (clen >= topframe->fr_width)
		    break;
	    }
	    if (i == num_matches)
		add_left = TRUE;
	}
    }
    if (add_left)
	while (first_match > 0)
	{
	    clen += status_match_len(xp, SHOW_MATCH(first_match - 1)) + 2;
	    if (clen >= topframe->fr_width)
		break;
	    --first_match;
	}

    fillchar = fillchar_status(&attr, curwin);

    if (first_match == 0)
    {
	*buf = NUL;
	len = 0;
    }
    else
    {
	STRCPY(buf, "< ");
	len = 2;
    }
    clen = len;

    i = first_match;
    while (clen + status_match_len(xp, SHOW_MATCH(i)) + 2 < topframe->fr_width)
    {
	if (i == match)
	{
	    selstart = buf + len;
	    selstart_col = clen;
	}

	s = SHOW_MATCH(i);
	// Check for menu separators - replace with '|'
#ifdef FEAT_MENU
	emenu = (xp->xp_context == EXPAND_MENUS
		|| xp->xp_context == EXPAND_MENUNAMES);
	if (emenu && menu_is_separator(s))
	{
	    STRCPY(buf + len, transchar('|'));
	    l = (int)STRLEN(buf + len);
	    len += l;
	    clen += l;
	}
	else
#endif
	for ( ; *s != NUL; ++s)
	{
	    s += skip_status_match_char(xp, s);
	    clen += ptr2cells(s);
	    if (has_mbyte && (l = (*mb_ptr2len)(s)) > 1)
	    {
		STRNCPY(buf + len, s, l);
		s += l - 1;
		len += l;
	    }
	    else
	    {
		STRCPY(buf + len, transchar_byte(*s));
		len += (int)STRLEN(buf + len);
	    }
	}
	if (i == match)
	    selend = buf + len;

	*(buf + len++) = ' ';
	*(buf + len++) = ' ';
	clen += 2;
	if (++i == num_matches)
		break;
    }

    if (i != num_matches)
    {
	*(buf + len++) = '>';
	++clen;
    }

    buf[len] = NUL;

    row = cmdline_row - 1;
    if (row >= 0)
    {
	if (wild_menu_showing == 0)
	{
	    if (msg_scrolled > 0)
	    {
		// Put the wildmenu just above the command line.  If there is
		// no room, scroll the screen one line up.
		if (cmdline_row == Rows - 1)
		{
		    screen_del_lines(0, 0, 1, (int)Rows, TRUE, 0, NULL);
		    ++msg_scrolled;
		}
		else
		{
		    ++cmdline_row;
		    ++row;
		}
		wild_menu_showing = WM_SCROLLED;
	    }
	    else
	    {
		// Create status line if needed by setting 'laststatus' to 2.
		// Set 'winminheight' to zero to avoid that the window is
		// resized.
		if (lastwin->w_status_height == 0)
		{
		    save_p_ls = p_ls;
		    save_p_wmh = p_wmh;
		    p_ls = 2;
		    p_wmh = 0;
		    last_status(FALSE);
		}
		wild_menu_showing = WM_SHOWN;
	    }
	}

	screen_puts(buf, row, firstwin->w_wincol, attr);
	if (selstart != NULL && highlight)
	{
	    *selend = NUL;
	    screen_puts(selstart, row, firstwin->w_wincol + selstart_col,
		    HL_ATTR(HLF_WM));
	}

	screen_fill(row, row + 1, firstwin->w_wincol + clen,
		firstwin->w_wincol + topframe->fr_width,
		fillchar, fillchar, attr);
    }

    win_redraw_last_status(topframe);
    vim_free(buf);
}

/*
 * Get the next or prev cmdline completion match. The index of the match is set
 * in "xp->xp_selected"
 */
    static char_u *
get_next_or_prev_match(int mode, expand_T *xp)
{
    int	    findex = xp->xp_selected;
    int	    ht;

    // When no matches found, return NULL
    if (xp->xp_numfiles <= 0)
	return NULL;

    if (mode == WILD_PREV)
    {
	// Select the last entry if at original text
	if (findex == -1)
	    findex = xp->xp_numfiles;
	// Otherwise select the previous entry
	--findex;
    }
    else if (mode == WILD_NEXT)
    {
	// Select the next entry
	++findex;
    }
    else   // WILD_PAGEDOWN or WILD_PAGEUP
    {
	// Get the height of popup menu (used for both PAGEUP and PAGEDOWN)
	ht = pum_get_height();
	if (ht > 3)
	    ht -= 2;

	if (mode == WILD_PAGEUP)
	{
	    if (findex == 0)
		// at the first entry, don't select any entries
		findex = -1;
	    else if (findex < 0)
		// no entry is selected. select the last entry
		findex = xp->xp_numfiles - 1;
	    else
		// go up by the pum height
		findex = MAX(findex - ht, 0);
	}
	else    // mode == WILD_PAGEDOWN
	{
	    if (findex >= xp->xp_numfiles - 1)
		// at the last entry, don't select any entries
		findex = -1;
	    else if (findex < 0)
		// no entry is selected, select the first entry
		findex = 0;
	    else
		// go down by the pum height
		findex = MIN(findex + ht, xp->xp_numfiles - 1);
	}
    }

    // Handle wrapping around
    if (findex < 0 || findex >= xp->xp_numfiles)
    {
	// If original text exists, return to it when wrapping around
	if (xp->xp_orig != NULL)
	    findex = -1;
	else
	    // Wrap around to opposite end
	    findex = (findex < 0) ? xp->xp_numfiles - 1 : 0;
    }

    // Display matches on screen
    if (p_wmnu)
    {
	if (compl_match_array)
	{
	    compl_selected = findex;
	    cmdline_pum_display();
	}
	else if (vim_strchr(p_wop, WOP_PUM) != NULL)
	{
	    if (cmdline_pum_create(get_cmdline_info(), xp, xp->xp_files,
			xp->xp_numfiles, cmd_showtail) == EXPAND_OK)
	    {
		compl_selected = findex;
		pum_clear();
		cmdline_pum_display();
	    }
	}
	else
	    win_redr_status_matches(xp, xp->xp_numfiles, xp->xp_files, findex,
		    cmd_showtail);
    }

    xp->xp_selected = findex;
    // Return the original text or the selected match
    return vim_strsave(findex == -1 ? xp->xp_orig : xp->xp_files[findex]);
}

/*
 * Start the command-line expansion and get the matches.
 */
    static char_u *
ExpandOne_start(int mode, expand_T *xp, char_u *str, int options)
{
    int		non_suf_match;		// number without matching suffix
    int		i;
    char_u	*ss = NULL;

    // Do the expansion.
    if (ExpandFromContext(xp, str, &xp->xp_files, &xp->xp_numfiles,
		options) == FAIL)
    {
#ifdef FNAME_ILLEGAL
	// Illegal file name has been silently skipped.  But when there
	// are wildcards, the real problem is that there was no match,
	// causing the pattern to be added, which has illegal characters.
	if (!(options & WILD_SILENT) && (options & WILD_LIST_NOTFOUND))
	    semsg(_(e_no_match_str_2), str);
#endif
    }
    else if (xp->xp_numfiles == 0)
    {
	if (!(options & WILD_SILENT))
	    semsg(_(e_no_match_str_2), str);
    }
    else
    {
	// Escape the matches for use on the command line.
	ExpandEscape(xp, str, xp->xp_numfiles, xp->xp_files, options);

	// Check for matching suffixes in file names.
	if (mode != WILD_ALL && mode != WILD_ALL_KEEP && mode != WILD_LONGEST)
	{
	    if (xp->xp_numfiles)
		non_suf_match = xp->xp_numfiles;
	    else
		non_suf_match = 1;
	    if ((xp->xp_context == EXPAND_FILES
			|| xp->xp_context == EXPAND_DIRECTORIES)
		    && xp->xp_numfiles > 1)
	    {
		// More than one match; check suffix.
		// The files will have been sorted on matching suffix in
		// expand_wildcards, only need to check the first two.
		non_suf_match = 0;
		for (i = 0; i < 2; ++i)
		    if (match_suffix(xp->xp_files[i]))
			++non_suf_match;
	    }
	    if (non_suf_match != 1)
	    {
		// Can we ever get here unless it's while expanding
		// interactively?  If not, we can get rid of this all
		// together. Don't really want to wait for this message
		// (and possibly have to hit return to continue!).
		if (!(options & WILD_SILENT))
		    emsg(_(e_too_many_file_names));
		else if (!(options & WILD_NO_BEEP))
		    beep_flush();
	    }
	    if (!(non_suf_match != 1 && mode == WILD_EXPAND_FREE))
		ss = vim_strsave(xp->xp_files[0]);
	}
    }

    return ss;
}

/*
 * Return the longest common part in the list of cmdline completion matches.
 */
    static char_u *
find_longest_match(expand_T *xp, int options)
{
    long_u	len;
    int		mb_len = 1;
    int		c0, ci;
    int		i;
    char_u	*ss;

    for (len = 0; xp->xp_files[0][len]; len += mb_len)
    {
	if (has_mbyte)
	{
	    mb_len = (*mb_ptr2len)(&xp->xp_files[0][len]);
	    c0 = (*mb_ptr2char)(&xp->xp_files[0][len]);
	}
	else
	    c0 = xp->xp_files[0][len];
	for (i = 1; i < xp->xp_numfiles; ++i)
	{
	    if (has_mbyte)
		ci = (*mb_ptr2char)(&xp->xp_files[i][len]);
	    else
		ci = xp->xp_files[i][len];
	    if (p_fic && (xp->xp_context == EXPAND_DIRECTORIES
			|| xp->xp_context == EXPAND_FILES
			|| xp->xp_context == EXPAND_SHELLCMD
			|| xp->xp_context == EXPAND_BUFFERS))
	    {
		if (MB_TOLOWER(c0) != MB_TOLOWER(ci))
		    break;
	    }
	    else if (c0 != ci)
		break;
	}
	if (i < xp->xp_numfiles)
	{
	    if (!(options & WILD_NO_BEEP))
		vim_beep(BO_WILD);
	    break;
	}
    }

    ss = alloc(len + 1);
    if (ss)
	vim_strncpy(ss, xp->xp_files[0], (size_t)len);

    return ss;
}

/*
 * Do wildcard expansion on the string "str".
 * Chars that should not be expanded must be preceded with a backslash.
 * Return a pointer to allocated memory containing the new string.
 * Return NULL for failure.
 *
 * "orig" is the originally expanded string, copied to allocated memory.  It
 * should either be kept in "xp->xp_orig" or freed.  When "mode" is WILD_NEXT
 * or WILD_PREV "orig" should be NULL.
 *
 * Results are cached in xp->xp_files and xp->xp_numfiles, except when "mode"
 * is WILD_EXPAND_FREE or WILD_ALL.
 *
 * mode = WILD_FREE:	    just free previously expanded matches
 * mode = WILD_EXPAND_FREE: normal expansion, do not keep matches
 * mode = WILD_EXPAND_KEEP: normal expansion, keep matches
 * mode = WILD_NEXT:	    use next match in multiple match, wrap to first
 * mode = WILD_PREV:	    use previous match in multiple match, wrap to first
 * mode = WILD_ALL:	    return all matches concatenated
 * mode = WILD_LONGEST:	    return longest matched part
 * mode = WILD_ALL_KEEP:    get all matches, keep matches
 * mode = WILD_APPLY:	    apply the item selected in the cmdline completion
 *			    popup menu and close the menu.
 * mode = WILD_CANCEL:	    cancel and close the cmdline completion popup and
 *			    use the original text.
 *
 * options = WILD_LIST_NOTFOUND:    list entries without a match
 * options = WILD_HOME_REPLACE:	    do home_replace() for buffer names
 * options = WILD_USE_NL:	    Use '\n' for WILD_ALL
 * options = WILD_NO_BEEP:	    Don't beep for multiple matches
 * options = WILD_ADD_SLASH:	    add a slash after directory names
 * options = WILD_KEEP_ALL:	    don't remove 'wildignore' entries
 * options = WILD_SILENT:	    don't print warning messages
 * options = WILD_ESCAPE:	    put backslash before special chars
 * options = WILD_ICASE:	    ignore case for files
 * options = WILD_ALLLINKS;	    keep broken links
 *
 * The variables xp->xp_context and xp->xp_backslash must have been set!
 */
    char_u *
ExpandOne(
    expand_T	*xp,
    char_u	*str,
    char_u	*orig,	    // allocated copy of original of expanded string
    int		options,
    int		mode)
{
    char_u	*ss = NULL;
    int		orig_saved = FALSE;

    // first handle the case of using an old match
    if (mode == WILD_NEXT || mode == WILD_PREV
	    || mode == WILD_PAGEUP || mode == WILD_PAGEDOWN)
	return get_next_or_prev_match(mode, xp);

    if (mode == WILD_CANCEL)
	ss = vim_strsave(xp->xp_orig ? xp->xp_orig : (char_u *)"");
    else if (mode == WILD_APPLY)
	ss = vim_strsave(xp->xp_selected == -1
			    ? (xp->xp_orig ? xp->xp_orig : (char_u *)"")
			    : xp->xp_files[xp->xp_selected]);

    // free old names
    if (xp->xp_numfiles != -1 && mode != WILD_ALL && mode != WILD_LONGEST)
    {
	FreeWild(xp->xp_numfiles, xp->xp_files);
	xp->xp_numfiles = -1;
	VIM_CLEAR(xp->xp_orig);

	// The entries from xp_files may be used in the PUM, remove it.
	if (compl_match_array != NULL)
	    cmdline_pum_remove(get_cmdline_info(), FALSE);
    }
    xp->xp_selected = (options & WILD_NOSELECT) ? -1 : 0;

    if (mode == WILD_FREE)	// only release file name
	return NULL;

    if (xp->xp_numfiles == -1 && mode != WILD_APPLY && mode != WILD_CANCEL)
    {
	vim_free(xp->xp_orig);
	xp->xp_orig = orig;
	orig_saved = TRUE;

	ss = ExpandOne_start(mode, xp, str, options);
    }

    // Find longest common part
    if (mode == WILD_LONGEST && xp->xp_numfiles > 0)
    {
	ss = find_longest_match(xp, options);
	xp->xp_selected = -1;			// next p_wc gets first one
    }

    // Concatenate all matching names.  Unless interrupted, this can be slow
    // and the result probably won't be used.
    if (mode == WILD_ALL && xp->xp_numfiles > 0 && !got_int)
    {
	size_t	ss_size = 0;
	char	*prefix = "";
	char	*suffix = (options & WILD_USE_NL) ? "\n" : " ";
	int	n = xp->xp_numfiles - 1;
	int	i;

	if (xp->xp_prefix == XP_PREFIX_NO)
	{
	    prefix = "no";
	    ss_size = STRLEN_LITERAL("no") * n;
	}
	else if (xp->xp_prefix == XP_PREFIX_INV)
	{
	    prefix = "inv";
	    ss_size = STRLEN_LITERAL("inv") * n;
	}

	for (i = 0; i < xp->xp_numfiles; ++i)
	    ss_size += STRLEN(xp->xp_files[i]) + 1;	// +1 for the suffix
	++ss_size;					// +1 for the NUL

	ss = alloc(ss_size);
	if (ss != NULL)
	{
	    size_t  ss_len = 0;

	    for (i = 0; i < xp->xp_numfiles; ++i)
	    {
		ss_len += vim_snprintf_safelen(
		    (char *)ss + ss_len,
		    ss_size - ss_len,
		    "%s%s%s",
		    (i > 0) ? prefix : "",
		    (char *)xp->xp_files[i],
		    (i < n) ? suffix : "");
	    }
	}
    }

    if (mode == WILD_EXPAND_FREE || mode == WILD_ALL)
	ExpandCleanup(xp);

    // Free "orig" if it wasn't stored in "xp->xp_orig".
    if (!orig_saved)
	vim_free(orig);

    return ss;
}

/*
 * Prepare an expand structure for use.
 */
    void
ExpandInit(expand_T *xp)
{
    CLEAR_POINTER(xp);
    xp->xp_backslash = XP_BS_NONE;
    xp->xp_prefix = XP_PREFIX_NONE;
    xp->xp_numfiles = -1;
}

/*
 * Cleanup an expand structure after use.
 */
    void
ExpandCleanup(expand_T *xp)
{
    if (xp->xp_numfiles >= 0)
    {
	FreeWild(xp->xp_numfiles, xp->xp_files);
	xp->xp_numfiles = -1;
    }
    VIM_CLEAR(xp->xp_orig);
}

    void
clear_cmdline_orig(void)
{
    VIM_CLEAR(cmdline_orig);
}

/*
 * Display one line of completion matches. Multiple matches are displayed in
 * each line (used by wildmode=list and CTRL-D)
 *   matches - list of completion match names
 *   numMatches - number of completion matches in "matches"
 *   lines - number of output lines
 *   linenr - line number of matches to display
 *   maxlen - maximum number of characters in each line
 *   showtail - display only the tail of the full path of a file name
 *   dir_attr - highlight attribute to use for directory names
 */
    static void
showmatches_oneline(
	expand_T	*xp,
	char_u		**matches,
	int		numMatches,
	int		lines,
	int		linenr,
	int		maxlen,
	int		showtail,
	int		dir_attr)
{
    int		i, j;
    int		isdir;
    int		lastlen;
    char_u	*p;

    lastlen = 999;
    for (j = linenr; j < numMatches; j += lines)
    {
	if (xp->xp_context == EXPAND_TAGS_LISTFILES)
	{
	    msg_outtrans_attr(matches[j], HL_ATTR(HLF_D));
	    p = matches[j] + STRLEN(matches[j]) + 1;
	    msg_advance(maxlen + 1);
	    msg_puts((char *)p);
	    msg_advance(maxlen + 3);
	    msg_outtrans_long_attr(p + 2, HL_ATTR(HLF_D));
	    break;
	}
	for (i = maxlen - lastlen; --i >= 0; )
	    msg_putchar(' ');
	if (xp->xp_context == EXPAND_FILES
		|| xp->xp_context == EXPAND_SHELLCMD
		|| xp->xp_context == EXPAND_BUFFERS)
	{
	    // highlight directories
	    if (xp->xp_numfiles != -1)
	    {
		char_u	*halved_slash;
		char_u	*exp_path;
		char_u	*path;

		// Expansion was done before and special characters
		// were escaped, need to halve backslashes.  Also
		// $HOME has been replaced with ~/.
		exp_path = expand_env_save_opt(matches[j], TRUE);
		path = exp_path != NULL ? exp_path : matches[j];
		halved_slash = backslash_halve_save(path);
		isdir = mch_isdir(halved_slash != NULL ? halved_slash
			: matches[j]);
		vim_free(exp_path);
		if (halved_slash != path)
		    vim_free(halved_slash);
	    }
	    else
		// Expansion was done here, file names are literal.
		isdir = mch_isdir(matches[j]);
	    if (showtail)
		p = SHOW_MATCH(j);
	    else
	    {
		home_replace(NULL, matches[j], NameBuff, MAXPATHL,
			TRUE);
		p = NameBuff;
	    }
	}
	else
	{
	    isdir = FALSE;
	    p = SHOW_MATCH(j);
	}
	lastlen = msg_outtrans_attr(p, isdir ? dir_attr : 0);
    }
    if (msg_col > 0)	// when not wrapped around
    {
	msg_clr_eos();
	msg_putchar('\n');
    }
    out_flush();		    // show one line at a time
}

/*
 * Display completion matches.
 * Returns EXPAND_NOTHING when the character that triggered expansion should be
 *   inserted as a normal character.
 */
    int
showmatches(expand_T *xp, int display_wildmenu, int display_list, int noselect)
{
    cmdline_info_T	*ccline = get_cmdline_info();
    int		numMatches;
    char_u	**matches;
    int		i;
    int		maxlen;
    int		lines;
    int		columns;
    int		attr;
    int		showtail;

    if (xp->xp_numfiles == -1)
    {
	int retval;
	set_expand_context(xp);
	retval = expand_cmdline(xp, ccline->cmdbuff, ccline->cmdpos,
		&numMatches, &matches);
	if (retval != EXPAND_OK)
	    return retval;
	showtail = expand_showtail(xp);
    }
    else
    {
	numMatches = xp->xp_numfiles;
	matches = xp->xp_files;
	showtail = cmd_showtail;
    }

    if (display_wildmenu && !display_list
	    && vim_strchr(p_wop, WOP_PUM) != NULL)
    {
	int retval = cmdline_pum_create(ccline, xp, matches, numMatches,
		showtail && !noselect);
	if (retval == EXPAND_OK)
	{
	    compl_selected = noselect ? -1 : 0;
	    pum_clear();
	    cmdline_pum_display();
	}
	return retval;
    }

    if (display_list)
    {
	msg_didany = FALSE;		// lines_left will be set
	msg_start();			// prepare for paging
	msg_putchar('\n');
	out_flush();
	cmdline_row = msg_row;
	msg_didany = FALSE;		// lines_left will be set again
	msg_start();			// prepare for paging
    }

    if (got_int)
	got_int = FALSE;  // only interrupt the completion, not the cmd line
    else if (display_wildmenu && !display_list)
	win_redr_status_matches(xp, numMatches, matches, noselect ? -1 : 0,
		showtail);  // display statusbar menu
    else if (display_list)
    {
	// find the length of the longest file name
	maxlen = 0;
	for (i = 0; i < numMatches; ++i)
	{
	    int	len;
	    if (!showtail && (xp->xp_context == EXPAND_FILES
			  || xp->xp_context == EXPAND_SHELLCMD
			  || xp->xp_context == EXPAND_BUFFERS))
	    {
		home_replace(NULL, matches[i], NameBuff, MAXPATHL, TRUE);
		len = vim_strsize(NameBuff);
	    }
	    else
		len = vim_strsize(SHOW_MATCH(i));
	    if (len > maxlen)
		maxlen = len;
	}

	if (xp->xp_context == EXPAND_TAGS_LISTFILES)
	    lines = numMatches;
	else
	{
	    // compute the number of columns and lines for the listing
	    maxlen += 2;    // two spaces between file names
	    columns = (cmdline_width + 2) / maxlen;
	    if (columns < 1)
		columns = 1;
	    lines = (numMatches + columns - 1) / columns;
	}

	attr = HL_ATTR(HLF_D);	// find out highlighting for directories

	if (xp->xp_context == EXPAND_TAGS_LISTFILES)
	{
	    msg_puts_attr(_("tagname"), HL_ATTR(HLF_T));
	    msg_clr_eos();
	    msg_advance(maxlen - 3);
	    msg_puts_attr(_(" kind file\n"), HL_ATTR(HLF_T));
	}

	// list the files line by line
	for (i = 0; i < lines; ++i)
	{
	    showmatches_oneline(xp, matches, numMatches, lines, i,
						maxlen, showtail, attr);
	    if (got_int)
	    {
		got_int = FALSE;
		break;
	    }
	}

	// we redraw the command below the lines that we have just listed
	// This is a bit tricky, but it saves a lot of screen updating.
	cmdline_row = msg_row;	// will put it back later
    }

    if (xp->xp_numfiles == -1)
	FreeWild(numMatches, matches);

    return EXPAND_OK;
}

/*
 * gettail() version for showmatches() and win_redr_status_matches():
 * Return the tail of file name path "s", ignoring a trailing "/".
 */
    static char_u *
showmatches_gettail(char_u *s)
{
    char_u	*p;
    char_u	*t = s;
    int		had_sep = FALSE;

    for (p = s; *p != NUL; )
    {
	if (vim_ispathsep(*p)
#ifdef BACKSLASH_IN_FILENAME
		&& !rem_backslash(p)
#endif
	   )
	    had_sep = TRUE;
	else if (had_sep)
	{
	    t = p;
	    had_sep = FALSE;
	}
	MB_PTR_ADV(p);
    }
    return t;
}

/*
 * Return TRUE if we only need to show the tail of completion matches.
 * When not completing file names or there is a wildcard in the path FALSE is
 * returned.
 */
    static int
expand_showtail(expand_T *xp)
{
    char_u	*s;
    char_u	*end;

    // When not completing file names a "/" may mean something different.
    if (xp->xp_context != EXPAND_FILES
	    && xp->xp_context != EXPAND_SHELLCMD
	    && xp->xp_context != EXPAND_DIRECTORIES)
	return FALSE;

    end = gettail(xp->xp_pattern);
    if (end == xp->xp_pattern)		// there is no path separator
	return FALSE;

    for (s = xp->xp_pattern; s < end; s++)
    {
	// Skip escaped wildcards.  Only when the backslash is not a path
	// separator, on DOS the '*' "path\*\file" must not be skipped.
	if (rem_backslash(s))
	    ++s;
	else if (vim_strchr((char_u *)"*?[", *s) != NULL)
	    return FALSE;
    }
    return TRUE;
}

/*
 * Prepare a string for expansion.
 * When expanding file names: The string will be used with expand_wildcards().
 * Copy "fname[len]" into allocated memory and add a '*' at the end.
 * When expanding other names: The string will be used with regcomp().  Copy
 * the name into allocated memory and prepend "^".
 */
    char_u *
addstar(
    char_u	*fname,
    int		len,
    int		context)	// EXPAND_FILES etc.
{
    char_u	*retval;
    int		i, j;
    int		new_len;
    char_u	*tail;
    int		ends_in_star;

    if (context != EXPAND_FILES
	    && context != EXPAND_FILES_IN_PATH
	    && context != EXPAND_SHELLCMD
	    && context != EXPAND_DIRECTORIES
	    && context != EXPAND_DIRS_IN_CDPATH)
    {
	// Matching will be done internally (on something other than files).
	// So we convert the file-matching-type wildcards into our kind for
	// use with vim_regcomp().  First work out how long it will be:

	// For help tags the translation is done in find_help_tags().
	// For a tag pattern starting with "/" no translation is needed.
	if (context == EXPAND_FINDFUNC
		|| context == EXPAND_HELP
		|| context == EXPAND_COLORS
		|| context == EXPAND_COMPILER
		|| context == EXPAND_OWNSYNTAX
		|| context == EXPAND_FILETYPE
		|| context == EXPAND_KEYMAP
		|| context == EXPAND_PACKADD
		|| context == EXPAND_RUNTIME
		|| ((context == EXPAND_TAGS_LISTFILES
			|| context == EXPAND_TAGS)
		    && fname[0] == '/'))
	    retval = vim_strnsave(fname, len);
	else
	{
	    new_len = len + 2;		// +2 for '^' at start, NUL at end
	    for (i = 0; i < len; i++)
	    {
		if (fname[i] == '*' || fname[i] == '~')
		    new_len++;		// '*' needs to be replaced by ".*"
					// '~' needs to be replaced by "\~"

		// Buffer names are like file names.  "." should be literal
		if (context == EXPAND_BUFFERS && fname[i] == '.')
		    new_len++;		// "." becomes "\."

		// Custom expansion takes care of special things, match
		// backslashes literally (perhaps also for other types?)
		if ((context == EXPAND_USER_DEFINED
			  || context == EXPAND_USER_LIST) && fname[i] == '\\')
		    new_len++;		// '\' becomes "\\"
	    }
	    retval = alloc(new_len);
	    if (retval != NULL)
	    {
		retval[0] = '^';
		j = 1;
		for (i = 0; i < len; i++, j++)
		{
		    // Skip backslash.  But why?  At least keep it for custom
		    // expansion.
		    if (context != EXPAND_USER_DEFINED
			    && context != EXPAND_USER_LIST
			    && fname[i] == '\\'
			    && ++i == len)
			break;

		    switch (fname[i])
		    {
			case '*':   retval[j++] = '.';
				    break;
			case '~':   retval[j++] = '\\';
				    break;
			case '?':   retval[j] = '.';
				    continue;
			case '.':   if (context == EXPAND_BUFFERS)
					retval[j++] = '\\';
				    break;
			case '\\':  if (context == EXPAND_USER_DEFINED
					    || context == EXPAND_USER_LIST)
					retval[j++] = '\\';
				    break;
		    }
		    retval[j] = fname[i];
		}
		retval[j] = NUL;
	    }
	}
    }
    else
    {
	retval = alloc(len + 4);
	if (retval != NULL)
	{
	    vim_strncpy(retval, fname, len);

	    // Don't add a star to *, ~, ~user, $var or `cmd`.
	    // * would become **, which walks the whole tree.
	    // ~ would be at the start of the file name, but not the tail.
	    // $ could be anywhere in the tail.
	    // ` could be anywhere in the file name.
	    // When the name ends in '$' don't add a star, remove the '$'.
	    tail = gettail(retval);
	    ends_in_star = (len > 0 && retval[len - 1] == '*');
#ifndef BACKSLASH_IN_FILENAME
	    for (i = len - 2; i >= 0; --i)
	    {
		if (retval[i] != '\\')
		    break;
		ends_in_star = !ends_in_star;
	    }
#endif
	    if ((*retval != '~' || tail != retval)
		    && !ends_in_star
		    && vim_strchr(tail, '$') == NULL
		    && vim_strchr(retval, '`') == NULL)
		retval[len++] = '*';
	    else if (len > 0 && retval[len - 1] == '$')
		--len;
	    retval[len] = NUL;
	}
    }
    return retval;
}

/*
 * Must parse the command line so far to work out what context we are in.
 * Completion can then be done based on that context.
 * This routine sets the variables:
 *  xp->xp_pattern	    The start of the pattern to be expanded within
 *				the command line (ends at the cursor).
 *  xp->xp_context	    The type of thing to expand.  Will be one of:
 *
 *  EXPAND_UNSUCCESSFUL	    Used sometimes when there is something illegal on
 *			    the command line, like an unknown command.	Caller
 *			    should beep.
 *  EXPAND_NOTHING	    Unrecognised context for completion, use char like
 *			    a normal char, rather than for completion.	eg
 *			    :s/^I/
 *  EXPAND_COMMANDS	    Cursor is still touching the command, so complete
 *			    it.
 *  EXPAND_BUFFERS	    Complete file names for :buf and :sbuf commands.
 *  EXPAND_FILES	    After command with EX_XFILE set, or after setting
 *			    with P_EXPAND set.	eg :e ^I, :w>>^I
 *  EXPAND_DIRECTORIES	    In some cases this is used instead of the latter
 *			    when we know only directories are of interest.
 *			    E.g.  :set dir=^I  and  :cd ^I
 *  EXPAND_SHELLCMD	    After ":!cmd", ":r !cmd"  or ":w !cmd".
 *  EXPAND_SETTINGS	    Complete variable names.  eg :set d^I
 *  EXPAND_BOOL_SETTINGS    Complete boolean variables only,  eg :set no^I
 *  EXPAND_TAGS		    Complete tags from the files in p_tags.  eg :ta a^I
 *  EXPAND_TAGS_LISTFILES   As above, but list filenames on ^D, after :tselect
 *  EXPAND_HELP		    Complete tags from the file 'helpfile'/tags
 *  EXPAND_EVENTS	    Complete event names
 *  EXPAND_SYNTAX	    Complete :syntax command arguments
 *  EXPAND_HIGHLIGHT	    Complete highlight (syntax) group names
 *  EXPAND_AUGROUP	    Complete autocommand group names
 *  EXPAND_USER_VARS	    Complete user defined variable names, eg :unlet a^I
 *  EXPAND_MAPPINGS	    Complete mapping and abbreviation names,
 *			      eg :unmap a^I , :cunab x^I
 *  EXPAND_FUNCTIONS	    Complete internal or user defined function names,
 *			      eg :call sub^I
 *  EXPAND_USER_FUNC	    Complete user defined function names, eg :delf F^I
 *  EXPAND_EXPRESSION	    Complete internal or user defined function/variable
 *			    names in expressions, eg :while s^I
 *  EXPAND_ENV_VARS	    Complete environment variable names
 *  EXPAND_USER		    Complete user names
 *  EXPAND_PATTERN_IN_BUF   Complete pattern in '/', '?', ':s', ':g', etc.
 */
    void
set_expand_context(expand_T *xp)
{
    cmdline_info_T  *ccline = get_cmdline_info();

    // Handle search commands: '/' or '?'
    if ((ccline->cmdfirstc == '/' || ccline->cmdfirstc == '?')
	    && may_expand_pattern)
    {
	xp->xp_context = EXPAND_PATTERN_IN_BUF;
	xp->xp_search_dir = (ccline->cmdfirstc == '/') ? FORWARD : BACKWARD;
	xp->xp_pattern = ccline->cmdbuff;
	xp->xp_pattern_len = ccline->cmdpos;
#ifdef FEAT_SEARCH_EXTRA
	search_first_line = 0; // Search entire buffer
#endif
	return;
    }

    // Only handle ':', '>', or '=' command-lines, or expression input
    if (ccline->cmdfirstc != ':'
#ifdef FEAT_EVAL
	    && ccline->cmdfirstc != '>' && ccline->cmdfirstc != '='
	    && !ccline->input_fn
#endif
       )
    {
	xp->xp_context = EXPAND_NOTHING;
	return;
    }

    // Fallback to command-line expansion
    set_cmd_context(xp, ccline->cmdbuff, ccline->cmdlen, ccline->cmdpos, TRUE);
}

/*
 * Sets the index of a built-in or user defined command 'cmd' in eap->cmdidx.
 * For user defined commands, the completion context is set in 'xp' and the
 * completion flags in 'complp'.
 *
 * Returns a pointer to the text after the command or NULL for failure.
 */
    static char_u *
set_cmd_index(char_u *cmd, exarg_T *eap, expand_T *xp, int *complp)
{
    char_u	*p = NULL;
    int		len = 0;
    int		fuzzy = cmdline_fuzzy_complete(cmd);

    // Isolate the command and search for it in the command table.
    // Exceptions:
    // - the 'k' command can directly be followed by any character, but do
    // accept "keepmarks", "keepalt" and "keepjumps". As fuzzy matching can
    // find matches anywhere in the command name, do this only for command
    // expansion based on regular expression and not for fuzzy matching.
    // - the 's' command can be followed directly by 'c', 'g', 'i', 'I' or 'r'
    if (!fuzzy && (*cmd == 'k' && cmd[1] != 'e'))
    {
	eap->cmdidx = CMD_k;
	p = cmd + 1;
    }
    else
    {
	p = cmd;
	while (ASCII_ISALPHA(*p) || *p == '*')    // Allow * wild card
	    ++p;
	// A user command may contain digits.
	// Include "9" for "vim9*" commands; "vim9cmd" and "vim9script".
	if (ASCII_ISUPPER(cmd[0]) || STRNCMP("vim9", cmd, 4) == 0)
	    while (ASCII_ISALNUM(*p) || *p == '*')
		++p;
	// for python 3.x: ":py3*" commands completion
	if (cmd[0] == 'p' && cmd[1] == 'y' && p == cmd + 2 && *p == '3')
	{
	    ++p;
	    while (ASCII_ISALPHA(*p) || *p == '*')
		++p;
	}
	// check for non-alpha command
	if (p == cmd && vim_strchr((char_u *)"@*!=><&~#", *p) != NULL)
	    ++p;
	len = (int)(p - cmd);

	if (len == 0)
	{
	    xp->xp_context = EXPAND_UNSUCCESSFUL;
	    return NULL;
	}

	eap->cmdidx = excmd_get_cmdidx(cmd, len);

	// User defined commands support alphanumeric characters.
	// Also when doing fuzzy expansion for non-shell commands, support
	// alphanumeric characters.
	if ((cmd[0] >= 'A' && cmd[0] <= 'Z')
		|| (fuzzy && eap->cmdidx != CMD_bang && *p != NUL))
	    while (ASCII_ISALNUM(*p) || *p == '*')	// Allow * wild card
		++p;
    }

    // If the cursor is touching the command, and it ends in an alphanumeric
    // character, complete the command name.
    if (*p == NUL && ASCII_ISALNUM(p[-1]))
	return NULL;

    if (eap->cmdidx == CMD_SIZE)
    {
	if (*cmd == 's' && vim_strchr((char_u *)"cgriI", cmd[1]) != NULL)
	{
	    eap->cmdidx = CMD_substitute;
	    p = cmd + 1;
	}
	else if (cmd[0] >= 'A' && cmd[0] <= 'Z')
	{
	    eap->cmd = cmd;
	    p = find_ucmd(eap, p, NULL, xp, complp);
	    if (p == NULL)
		eap->cmdidx = CMD_SIZE;	// ambiguous user command
	}
    }
    if (eap->cmdidx == CMD_SIZE)
    {
	// Not still touching the command and it was an illegal one
	xp->xp_context = EXPAND_UNSUCCESSFUL;
	return NULL;
    }

    return p;
}

/*
 * Set the completion context for a command argument with wild card characters.
 */
    static void
set_context_for_wildcard_arg(
	exarg_T		*eap,
	char_u		*arg,
	int		usefilter,
	expand_T	*xp,
	int		*complp)
{
    char_u	*p;
    int		c;
    int		in_quote = FALSE;
    char_u	*bow = NULL;	// Beginning of word
    int		len = 0;

    // Allow spaces within back-quotes to count as part of the argument
    // being expanded.
    xp->xp_pattern = skipwhite(arg);
    p = xp->xp_pattern;
    while (*p != NUL)
    {
	if (has_mbyte)
	    c = mb_ptr2char(p);
	else
	    c = *p;
	if (c == '\\' && p[1] != NUL)
	    ++p;
	else if (c == '`')
	{
	    if (!in_quote)
	    {
		xp->xp_pattern = p;
		bow = p + 1;
	    }
	    in_quote = !in_quote;
	}
	// An argument can contain just about everything, except
	// characters that end the command and white space.
	else if (c == '|' || c == '\n' || c == '"' || (VIM_ISWHITE(c)
#ifdef SPACE_IN_FILENAME
		    && (!(eap != NULL && (eap->argt & EX_NOSPC)) || usefilter)
#endif
		    ))
	{
	    len = 0;  // avoid getting stuck when space is in 'isfname'
	    while (*p != NUL)
	    {
		if (has_mbyte)
		    c = mb_ptr2char(p);
		else
		    c = *p;
		if (c == '`' || vim_isfilec_or_wc(c))
		    break;
		if (has_mbyte)
		    len = (*mb_ptr2len)(p);
		else
		    len = 1;
		MB_PTR_ADV(p);
	    }
	    if (in_quote)
		bow = p;
	    else
		xp->xp_pattern = p;
	    p -= len;
	}
	MB_PTR_ADV(p);
    }

    // If we are still inside the quotes, and we passed a space, just
    // expand from there.
    if (bow != NULL && in_quote)
	xp->xp_pattern = bow;
    xp->xp_context = EXPAND_FILES;

    // For a shell command more chars need to be escaped.
    if (usefilter
	    || (eap != NULL
		&& (eap->cmdidx == CMD_bang || eap->cmdidx == CMD_terminal))
	    || *complp == EXPAND_SHELLCMDLINE)
    {
#ifndef BACKSLASH_IN_FILENAME
	xp->xp_shell = TRUE;
#endif
	// When still after the command name expand executables.
	if (xp->xp_pattern == skipwhite(arg))
	    xp->xp_context = EXPAND_SHELLCMD;
    }

    // Check for environment variable.
    if (*xp->xp_pattern == '$')
    {
	for (p = xp->xp_pattern + 1; *p != NUL; ++p)
	    if (!vim_isIDc(*p))
		break;
	if (*p == NUL)
	{
	    xp->xp_context = EXPAND_ENV_VARS;
	    ++xp->xp_pattern;
	    // Avoid that the assignment uses EXPAND_FILES again.
	    if (*complp != EXPAND_USER_DEFINED && *complp != EXPAND_USER_LIST)
		*complp = EXPAND_ENV_VARS;
	}
    }
    // Check for user names.
    if (*xp->xp_pattern == '~')
    {
	for (p = xp->xp_pattern + 1; *p != NUL && *p != '/'; ++p)
	    ;
	// Complete ~user only if it partially matches a user name.
	// A full match ~user<Tab> will be replaced by user's home
	// directory i.e. something like ~user<Tab> -> /home/user/
	if (*p == NUL && p > xp->xp_pattern + 1
		&& match_user(xp->xp_pattern + 1) >= 1)
	{
	    xp->xp_context = EXPAND_USER;
	    ++xp->xp_pattern;
	}
    }
}

/*
 * Set the completion context for the "++opt=arg" argument.  Always returns
 * NULL.
 */
    static char_u *
set_context_in_argopt(expand_T *xp, char_u *arg)
{
    char_u	*p;

    p = vim_strchr(arg, '=');
    if (p == NULL)
	xp->xp_pattern = arg;
    else
	xp->xp_pattern = p + 1;

    xp->xp_context = EXPAND_ARGOPT;
    return NULL;
}

#ifdef FEAT_TERMINAL
/*
 * Set the completion context for :terminal's [options].  Always returns NULL.
 */
    static char_u *
set_context_in_terminalopt(expand_T *xp, char_u *arg)
{
    char_u	*p;

    p = vim_strchr(arg, '=');
    if (p == NULL)
	xp->xp_pattern = arg;
    else
	xp->xp_pattern = p + 1;

    xp->xp_context = EXPAND_TERMINALOPT;
    return NULL;
}
#endif

/*
 * Set the completion context for the :filter command. Returns a pointer to the
 * next command after the :filter command.
 */
    static char_u *
set_context_in_filter_cmd(expand_T *xp, char_u *arg)
{
    if (*arg != NUL)
	arg = skip_vimgrep_pat(arg, NULL, NULL);
    if (arg == NULL || *arg == NUL)
    {
	xp->xp_context = EXPAND_NOTHING;
	return NULL;
    }
    return skipwhite(arg);
}

#ifdef FEAT_SEARCH_EXTRA
/*
 * Set the completion context for the :match command. Returns a pointer to the
 * next command after the :match command.
 */
    static char_u *
set_context_in_match_cmd(expand_T *xp, char_u *arg)
{
    if (*arg == NUL || !ends_excmd(*arg))
    {
	// also complete "None"
	set_context_in_echohl_cmd(xp, arg);
	arg = skipwhite(skiptowhite(arg));
	if (*arg != NUL)
	{
	    xp->xp_context = EXPAND_NOTHING;
	    arg = skip_regexp(arg + 1, *arg, magic_isset());
	}
    }
    return find_nextcmd(arg);
}
#endif

/*
 * Returns a pointer to the next command after a :global or a :v command.
 * Returns NULL if there is no next command.
 */
    static char_u *
find_cmd_after_global_cmd(char_u *arg)
{
    int		delim;

    delim = *arg;	    // get the delimiter
    if (delim)
	++arg;		    // skip delimiter if there is one

    while (arg[0] != NUL && arg[0] != delim)
    {
	if (arg[0] == '\\' && arg[1] != NUL)
	    ++arg;
	++arg;
    }
    if (arg[0] != NUL)
	return arg + 1;

    return NULL;
}

/*
 * Returns a pointer to the next command after a :substitute or a :& command.
 * Returns NULL if there is no next command.
 */
    static char_u *
find_cmd_after_substitute_cmd(char_u *arg)
{
    int		delim;

    delim = *arg;
    if (delim)
    {
	// skip "from" part
	++arg;
	arg = skip_regexp(arg, delim, magic_isset());

	if (arg[0] != NUL && arg[0] == delim)
	{
	    // skip "to" part
	    ++arg;
	    while (arg[0] != NUL && arg[0] != delim)
	    {
		if (arg[0] == '\\' && arg[1] != NUL)
		    ++arg;
		++arg;
	    }
	    if (arg[0] != NUL)	// skip delimiter
		++arg;
	}
    }
    while (arg[0] && vim_strchr((char_u *)"|\"#", arg[0]) == NULL)
	++arg;
    if (arg[0] != NUL)
	return arg;

    return NULL;
}

/*
 * Returns a pointer to the next command after a :isearch/:dsearch/:ilist
 * :dlist/:ijump/:psearch/:djump/:isplit/:dsplit command.
 * Returns NULL if there is no next command.
 */
    static char_u *
find_cmd_after_isearch_cmd(expand_T *xp, char_u *arg)
{
    arg = skipwhite(skipdigits(arg));	    // skip count
    if (*arg != '/')
	return NULL;

    // Match regexp, not just whole words
    for (++arg; *arg && *arg != '/'; arg++)
	if (*arg == '\\' && arg[1] != NUL)
	    arg++;
    if (*arg)
    {
	arg = skipwhite(arg + 1);

	// Check for trailing illegal characters
	if (*arg == NUL || vim_strchr((char_u *)"|\"\n", *arg) == NULL)
	    xp->xp_context = EXPAND_NOTHING;
	else
	    return arg;
    }

    return NULL;
}

#ifdef FEAT_EVAL
/*
 * Set the completion context for the :unlet command. Always returns NULL.
 */
    static char_u *
set_context_in_unlet_cmd(expand_T *xp, char_u *arg)
{
    while ((xp->xp_pattern = vim_strchr(arg, ' ')) != NULL)
	arg = xp->xp_pattern + 1;

    xp->xp_context = EXPAND_USER_VARS;
    xp->xp_pattern = arg;

    if (*xp->xp_pattern == '$')
    {
	xp->xp_context = EXPAND_ENV_VARS;
	++xp->xp_pattern;
    }

    return NULL;
}
#endif

#if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
/*
 * Set the completion context for the :language command. Always returns NULL.
 */
    static char_u *
set_context_in_lang_cmd(expand_T *xp, char_u *arg)
{
    char_u	*p;

    p = skiptowhite(arg);
    if (*p == NUL)
    {
	xp->xp_context = EXPAND_LANGUAGE;
	xp->xp_pattern = arg;
    }
    else
    {
	if ( STRNCMP(arg, "messages", p - arg) == 0
		|| STRNCMP(arg, "ctype", p - arg) == 0
		|| STRNCMP(arg, "time", p - arg) == 0
		|| STRNCMP(arg, "collate", p - arg) == 0)
	{
	    xp->xp_context = EXPAND_LOCALES;
	    xp->xp_pattern = skipwhite(p);
	}
	else
	    xp->xp_context = EXPAND_NOTHING;
    }

    return NULL;
}
#endif

static enum
{
    EXP_FILETYPECMD_ALL,	// expand all :filetype values
    EXP_FILETYPECMD_PLUGIN,	// expand plugin on off
    EXP_FILETYPECMD_INDENT,	// expand indent on off
    EXP_FILETYPECMD_ONOFF,	// expand on off
} filetype_expand_what;

#define EXPAND_FILETYPECMD_PLUGIN 0x01
#define EXPAND_FILETYPECMD_INDENT 0x02
#define EXPAND_FILETYPECMD_ONOFF  0x04

#ifdef FEAT_EVAL
static enum
{
    EXP_BREAKPT_ADD,	// expand ":breakadd" sub-commands
    EXP_BREAKPT_DEL,	// expand ":breakdel" sub-commands
    EXP_PROFDEL		// expand ":profdel" sub-commands
} breakpt_expand_what;

/*
 * Set the completion context for the :breakadd command. Always returns NULL.
 */
    static char_u *
set_context_in_breakadd_cmd(expand_T *xp, char_u *arg, cmdidx_T cmdidx)
{
    char_u *p;
    char_u *subcmd_start;

    xp->xp_context = EXPAND_BREAKPOINT;
    xp->xp_pattern = arg;

    if (cmdidx == CMD_breakadd)
	breakpt_expand_what = EXP_BREAKPT_ADD;
    else if (cmdidx == CMD_breakdel)
	breakpt_expand_what = EXP_BREAKPT_DEL;
    else
	breakpt_expand_what = EXP_PROFDEL;

    p = skipwhite(arg);
    if (*p == NUL)
	return NULL;
    subcmd_start = p;

    if (STRNCMP("file ", p, 5) == 0 || STRNCMP("func ", p, 5) == 0)
    {
	// :breakadd file [lnum] <filename>
	// :breakadd func [lnum] <funcname>
	p += 4;
	p = skipwhite(p);

	// skip line number (if specified)
	if (VIM_ISDIGIT(*p))
	{
	    p = skipdigits(p);
	    if (*p != ' ')
	    {
		xp->xp_context = EXPAND_NOTHING;
		return NULL;
	    }
	    p = skipwhite(p);
	}
	if (STRNCMP("file", subcmd_start, 4) == 0)
	    xp->xp_context = EXPAND_FILES;
	else
	    xp->xp_context = EXPAND_USER_FUNC;
	xp->xp_pattern = p;
    }
    else if (STRNCMP("expr ", p, 5) == 0)
    {
	// :breakadd expr <expression>
	xp->xp_context = EXPAND_EXPRESSION;
	xp->xp_pattern = skipwhite(p + 5);
    }

    return NULL;
}

    static char_u *
set_context_in_scriptnames_cmd(expand_T *xp, char_u *arg)
{
    char_u *p;

    xp->xp_context = EXPAND_NOTHING;
    xp->xp_pattern = NULL;

    p = skipwhite(arg);
    if (VIM_ISDIGIT(*p))
	return NULL;

    xp->xp_context = EXPAND_SCRIPTNAMES;
    xp->xp_pattern = p;

    return NULL;
}
#endif

/*
 * Set the completion context for the :filetype command. Always returns NULL.
 */
    static char_u *
set_context_in_filetype_cmd(expand_T *xp, char_u *arg)
{
    char_u *p;
    int    val = 0;

    xp->xp_context = EXPAND_FILETYPECMD;
    xp->xp_pattern = arg;
    filetype_expand_what = EXP_FILETYPECMD_ALL;

    p = skipwhite(arg);
    if (*p == NUL)
	return NULL;

    for (;;)
    {
	if (STRNCMP(p, "plugin", 6) == 0)
	{
	    val |= EXPAND_FILETYPECMD_PLUGIN;
	    p = skipwhite(p + 6);
	    continue;
	}
	if (STRNCMP(p, "indent", 6) == 0)
	{
	    val |= EXPAND_FILETYPECMD_INDENT;
	    p = skipwhite(p + 6);
	    continue;
	}
	break;
    }

    if ((val & EXPAND_FILETYPECMD_PLUGIN) && (val & EXPAND_FILETYPECMD_INDENT))
	filetype_expand_what = EXP_FILETYPECMD_ONOFF;
    else if ((val & EXPAND_FILETYPECMD_PLUGIN))
	filetype_expand_what = EXP_FILETYPECMD_INDENT;
    else if ((val & EXPAND_FILETYPECMD_INDENT))
	filetype_expand_what = EXP_FILETYPECMD_PLUGIN;

    xp->xp_pattern = p;

    return NULL;
}

/*
 * Sets the completion context for commands that involve a search pattern
 * and a line range (e.g., :s, :g, :v).
 */
    static void
set_context_with_pattern(expand_T *xp)
{
    int		    skiplen = 0;
    cmdline_info_T  *ccline = get_cmdline_info();
#ifdef FEAT_SEARCH_EXTRA
    int		    dummy, patlen, retval;

    ++emsg_off;
    retval = parse_pattern_and_range(&pre_incsearch_pos, &dummy, &skiplen,
	    &patlen);
    --emsg_off;

    // Check if cursor is within search pattern
    if (!retval || ccline->cmdpos <= skiplen
	    || ccline->cmdpos > skiplen + patlen)
	return;
#endif

    xp->xp_pattern = ccline->cmdbuff + skiplen;
    xp->xp_pattern_len = ccline->cmdpos - skiplen;
    xp->xp_context = EXPAND_PATTERN_IN_BUF;
    xp->xp_search_dir = FORWARD;
}

/*
 * Set the completion context in 'xp' for command 'cmd' with index 'cmdidx'.
 * The argument to the command is 'arg' and the argument flags is 'argt'.
 * For user-defined commands and for environment variables, 'compl' has the
 * completion type.
 * Returns a pointer to the next command. Returns NULL if there is no next
 * command.
 */
    static char_u *
set_context_by_cmdname(
	char_u		*cmd,
	cmdidx_T	cmdidx,
	expand_T	*xp,
	char_u		*arg,
	long		argt,
	int		compl,
	int		forceit)
{
    char_u  *nextcmd;

    switch (cmdidx)
    {
	case CMD_find:
	case CMD_sfind:
	case CMD_tabfind:
	    if (xp->xp_context == EXPAND_FILES)
		xp->xp_context = *get_findfunc() != NUL ? EXPAND_FINDFUNC
							: EXPAND_FILES_IN_PATH;
	    break;
	case CMD_cd:
	case CMD_chdir:
	case CMD_tcd:
	case CMD_tchdir:
	case CMD_lcd:
	case CMD_lchdir:
	    if (xp->xp_context == EXPAND_FILES)
		xp->xp_context = EXPAND_DIRS_IN_CDPATH;
	    break;
	case CMD_help:
	    xp->xp_context = EXPAND_HELP;
	    xp->xp_pattern = arg;
	    break;

	// Command modifiers: return the argument.
	// Also for commands with an argument that is a command.
	case CMD_aboveleft:
	case CMD_argdo:
	case CMD_belowright:
	case CMD_botright:
	case CMD_browse:
	case CMD_bufdo:
	case CMD_cdo:
	case CMD_cfdo:
	case CMD_confirm:
	case CMD_debug:
	case CMD_folddoclosed:
	case CMD_folddoopen:
	case CMD_hide:
	case CMD_horizontal:
	case CMD_keepalt:
	case CMD_keepjumps:
	case CMD_keepmarks:
	case CMD_keeppatterns:
	case CMD_ldo:
	case CMD_leftabove:
	case CMD_lfdo:
	case CMD_lockmarks:
	case CMD_noautocmd:
	case CMD_noswapfile:
	case CMD_rightbelow:
	case CMD_sandbox:
	case CMD_silent:
	case CMD_tab:
	case CMD_tabdo:
	case CMD_topleft:
	case CMD_unsilent:
	case CMD_verbose:
	case CMD_vertical:
	case CMD_windo:
	case CMD_vim9cmd:
	case CMD_legacy:
	    return arg;

	case CMD_filter:
	    return set_context_in_filter_cmd(xp, arg);

#ifdef FEAT_SEARCH_EXTRA
	case CMD_match:
	    return set_context_in_match_cmd(xp, arg);
#endif

	// All completion for the +cmdline_compl feature goes here.

	case CMD_command:
	    return set_context_in_user_cmd(xp, arg);

	case CMD_delcommand:
	    xp->xp_context = EXPAND_USER_COMMANDS;
	    xp->xp_pattern = arg;
	    break;

	case CMD_global:
	case CMD_vglobal:
	    nextcmd = find_cmd_after_global_cmd(arg);
	    if (!nextcmd && may_expand_pattern)
		set_context_with_pattern(xp);
	    return nextcmd;

	case CMD_and:
	case CMD_substitute:
	    nextcmd = find_cmd_after_substitute_cmd(arg);
	    if (!nextcmd && may_expand_pattern)
		set_context_with_pattern(xp);
	    return nextcmd;

	case CMD_isearch:
	case CMD_dsearch:
	case CMD_ilist:
	case CMD_dlist:
	case CMD_ijump:
	case CMD_psearch:
	case CMD_djump:
	case CMD_isplit:
	case CMD_dsplit:
	    return find_cmd_after_isearch_cmd(xp, arg);
	case CMD_autocmd:
	    return set_context_in_autocmd(xp, arg, FALSE);
	case CMD_doautocmd:
	case CMD_doautoall:
	    return set_context_in_autocmd(xp, arg, TRUE);
	case CMD_set:
	    set_context_in_set_cmd(xp, arg, 0);
	    break;
	case CMD_setglobal:
	    set_context_in_set_cmd(xp, arg, OPT_GLOBAL);
	    break;
	case CMD_setlocal:
	    set_context_in_set_cmd(xp, arg, OPT_LOCAL);
	    break;
	case CMD_tag:
	case CMD_stag:
	case CMD_ptag:
	case CMD_ltag:
	case CMD_tselect:
	case CMD_stselect:
	case CMD_ptselect:
	case CMD_tjump:
	case CMD_stjump:
	case CMD_ptjump:
	    if (vim_strchr(p_wop, WOP_TAGFILE) != NULL)
		xp->xp_context = EXPAND_TAGS_LISTFILES;
	    else
		xp->xp_context = EXPAND_TAGS;
	    xp->xp_pattern = arg;
	    break;
	case CMD_augroup:
	    xp->xp_context = EXPAND_AUGROUP;
	    xp->xp_pattern = arg;
	    break;
#ifdef FEAT_SYN_HL
	case CMD_syntax:
	    set_context_in_syntax_cmd(xp, arg);
	    break;
#endif
#ifdef FEAT_EVAL
	case CMD_final:
	case CMD_const:
	case CMD_let:
	case CMD_var:
	case CMD_if:
	case CMD_elseif:
	case CMD_while:
	case CMD_for:
	case CMD_echo:
	case CMD_echon:
	case CMD_execute:
	case CMD_echomsg:
	case CMD_echoerr:
	case CMD_echoconsole:
	case CMD_echowindow:
	case CMD_call:
	case CMD_return:
	case CMD_cexpr:
	case CMD_caddexpr:
	case CMD_cgetexpr:
	case CMD_lexpr:
	case CMD_laddexpr:
	case CMD_lgetexpr:
	    set_context_for_expression(xp, arg, cmdidx);
	    break;

	case CMD_unlet:
	    return set_context_in_unlet_cmd(xp, arg);
	case CMD_function:
	case CMD_delfunction:
	    xp->xp_context = EXPAND_USER_FUNC;
	    xp->xp_pattern = arg;
	    break;
	case CMD_disassemble:
	    set_context_in_disassemble_cmd(xp, arg);
	    break;

	case CMD_echohl:
	    set_context_in_echohl_cmd(xp, arg);
	    break;
#endif
	case CMD_highlight:
	    set_context_in_highlight_cmd(xp, arg);
	    break;
#ifdef FEAT_CSCOPE
	case CMD_cscope:
	case CMD_lcscope:
	case CMD_scscope:
	    set_context_in_cscope_cmd(xp, arg, cmdidx);
	    break;
#endif
#ifdef FEAT_SIGNS
	case CMD_sign:
	    set_context_in_sign_cmd(xp, arg);
	    break;
#endif
	case CMD_bdelete:
	case CMD_bwipeout:
	case CMD_bunload:
	    while ((xp->xp_pattern = vim_strchr(arg, ' ')) != NULL)
		arg = xp->xp_pattern + 1;
	    // FALLTHROUGH
	case CMD_buffer:
	case CMD_sbuffer:
	case CMD_pbuffer:
	case CMD_checktime:
	    xp->xp_context = EXPAND_BUFFERS;
	    xp->xp_pattern = arg;
	    break;
#ifdef FEAT_DIFF
	case CMD_diffget:
	case CMD_diffput:
	    // If current buffer is in diff mode, complete buffer names
	    // which are in diff mode, and different than current buffer.
	    xp->xp_context = EXPAND_DIFF_BUFFERS;
	    xp->xp_pattern = arg;
	    break;
#endif
	case CMD_USER:
	case CMD_USER_BUF:
	    return set_context_in_user_cmdarg(cmd, arg, argt, compl, xp,
								forceit);

	case CMD_map:	    case CMD_noremap:
	case CMD_nmap:	    case CMD_nnoremap:
	case CMD_vmap:	    case CMD_vnoremap:
	case CMD_omap:	    case CMD_onoremap:
	case CMD_imap:	    case CMD_inoremap:
	case CMD_cmap:	    case CMD_cnoremap:
	case CMD_lmap:	    case CMD_lnoremap:
	case CMD_smap:	    case CMD_snoremap:
	case CMD_tmap:	    case CMD_tnoremap:
	case CMD_xmap:	    case CMD_xnoremap:
	    return set_context_in_map_cmd(xp, cmd, arg, forceit,
						     FALSE, FALSE, cmdidx);
	case CMD_unmap:
	case CMD_nunmap:
	case CMD_vunmap:
	case CMD_ounmap:
	case CMD_iunmap:
	case CMD_cunmap:
	case CMD_lunmap:
	case CMD_sunmap:
	case CMD_tunmap:
	case CMD_xunmap:
	    return set_context_in_map_cmd(xp, cmd, arg, forceit,
						      FALSE, TRUE, cmdidx);
	case CMD_mapclear:
	case CMD_nmapclear:
	case CMD_vmapclear:
	case CMD_omapclear:
	case CMD_imapclear:
	case CMD_cmapclear:
	case CMD_lmapclear:
	case CMD_smapclear:
	case CMD_tmapclear:
	case CMD_xmapclear:
	    xp->xp_context = EXPAND_MAPCLEAR;
	    xp->xp_pattern = arg;
	    break;

	case CMD_abbreviate:	case CMD_noreabbrev:
	case CMD_cabbrev:	case CMD_cnoreabbrev:
	case CMD_iabbrev:	case CMD_inoreabbrev:
	    return set_context_in_map_cmd(xp, cmd, arg, forceit,
						      TRUE, FALSE, cmdidx);
	case CMD_unabbreviate:
	case CMD_cunabbrev:
	case CMD_iunabbrev:
	    return set_context_in_map_cmd(xp, cmd, arg, forceit,
						       TRUE, TRUE, cmdidx);
#ifdef FEAT_MENU
	case CMD_menu:	    case CMD_noremenu:	    case CMD_unmenu:
	case CMD_amenu:	    case CMD_anoremenu:	    case CMD_aunmenu:
	case CMD_nmenu:	    case CMD_nnoremenu:	    case CMD_nunmenu:
	case CMD_vmenu:	    case CMD_vnoremenu:	    case CMD_vunmenu:
	case CMD_omenu:	    case CMD_onoremenu:	    case CMD_ounmenu:
	case CMD_imenu:	    case CMD_inoremenu:	    case CMD_iunmenu:
	case CMD_cmenu:	    case CMD_cnoremenu:	    case CMD_cunmenu:
	case CMD_tlmenu:    case CMD_tlnoremenu:    case CMD_tlunmenu:
	case CMD_tmenu:				    case CMD_tunmenu:
	case CMD_popup:	    case CMD_tearoff:	    case CMD_emenu:
	    return set_context_in_menu_cmd(xp, cmd, arg, forceit);
#endif

	case CMD_colorscheme:
	    xp->xp_context = EXPAND_COLORS;
	    xp->xp_pattern = arg;
	    break;

	case CMD_compiler:
	    xp->xp_context = EXPAND_COMPILER;
	    xp->xp_pattern = arg;
	    break;

	case CMD_ownsyntax:
	    xp->xp_context = EXPAND_OWNSYNTAX;
	    xp->xp_pattern = arg;
	    break;

	case CMD_setfiletype:
	    xp->xp_context = EXPAND_FILETYPE;
	    xp->xp_pattern = arg;
	    break;

	case CMD_packadd:
	    xp->xp_context = EXPAND_PACKADD;
	    xp->xp_pattern = arg;
	    break;

	case CMD_runtime:
	    set_context_in_runtime_cmd(xp, arg);
	    break;

#if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
	case CMD_language:
	    return set_context_in_lang_cmd(xp, arg);
#endif
#if defined(FEAT_PROFILE)
	case CMD_profile:
	    set_context_in_profile_cmd(xp, arg);
	    break;
#endif
	case CMD_behave:
	    xp->xp_context = EXPAND_BEHAVE;
	    xp->xp_pattern = arg;
	    break;

	case CMD_retab:
	    xp->xp_context = EXPAND_RETAB;
	    xp->xp_pattern = arg;
	    break;

	case CMD_messages:
	    xp->xp_context = EXPAND_MESSAGES;
	    xp->xp_pattern = arg;
	    break;

	case CMD_history:
	    xp->xp_context = EXPAND_HISTORY;
	    xp->xp_pattern = arg;
	    break;
#if defined(FEAT_PROFILE)
	case CMD_syntime:
	    xp->xp_context = EXPAND_SYNTIME;
	    xp->xp_pattern = arg;
	    break;
#endif

	case CMD_argdelete:
	    while ((xp->xp_pattern = vim_strchr(arg, ' ')) != NULL)
		arg = xp->xp_pattern + 1;
	    xp->xp_context = EXPAND_ARGLIST;
	    xp->xp_pattern = arg;
	    break;

#ifdef FEAT_EVAL
	case CMD_breakadd:
	case CMD_profdel:
	case CMD_breakdel:
	    return set_context_in_breakadd_cmd(xp, arg, cmdidx);

	case CMD_scriptnames:
	    return set_context_in_scriptnames_cmd(xp, arg);
#endif
	case CMD_filetype:
	    return set_context_in_filetype_cmd(xp, arg);

	default:
	    break;
    }
    return NULL;
}

/*
 * This is all pretty much copied from do_one_cmd(), with all the extra stuff
 * we don't need/want deleted.	Maybe this could be done better if we didn't
 * repeat all this stuff.  The only problem is that they may not stay
 * perfectly compatible with each other, but then the command line syntax
 * probably won't change that much -- webb.
 */
    static char_u *
set_one_cmd_context(
    expand_T	*xp,
    char_u	*buff)	    // buffer for command string
{
    char_u		*p;
    char_u		*cmd, *arg;
    int			len = 0;
    exarg_T		ea;
    int			compl = EXPAND_NOTHING;
    int			forceit = FALSE;
    int			usefilter = FALSE;  // filter instead of file name

    ExpandInit(xp);
    xp->xp_pattern = buff;
    xp->xp_line = buff;
    xp->xp_context = EXPAND_COMMANDS;	// Default until we get past command
    ea.argt = 0;

    // 1. skip comment lines and leading space, colons or bars
    for (cmd = buff; vim_strchr((char_u *)" \t:|", *cmd) != NULL; cmd++)
	;
    xp->xp_pattern = cmd;

    if (*cmd == NUL)
	return NULL;
    if (*cmd == '"')	    // ignore comment lines
    {
	xp->xp_context = EXPAND_NOTHING;
	return NULL;
    }

    // 3. Skip over the range to find the command.
    cmd = skip_range(cmd, TRUE, &xp->xp_context);
    xp->xp_pattern = cmd;
    if (*cmd == NUL)
	return NULL;
    if (*cmd == '"')
    {
	xp->xp_context = EXPAND_NOTHING;
	return NULL;
    }

    if (*cmd == '|' || *cmd == '\n')
	return cmd + 1;			// There's another command

    // Get the command index.
    p = set_cmd_index(cmd, &ea, xp, &compl);
    if (p == NULL)
	return NULL;

    xp->xp_context = EXPAND_NOTHING; // Default now that we're past command

    if (*p == '!')		    // forced commands
    {
	forceit = TRUE;
	++p;
    }

    // 6. parse arguments
    if (!IS_USER_CMDIDX(ea.cmdidx))
	ea.argt = excmd_get_argt(ea.cmdidx);

    arg = skipwhite(p);

    // Does command allow "++argopt" argument?
    if ((ea.argt & EX_ARGOPT) || ea.cmdidx == CMD_terminal)
    {
	while (*arg != NUL && STRNCMP(arg, "++", 2) == 0)
	{
	    p = arg + 2;
	    while (*p && !vim_isspace(*p))
		MB_PTR_ADV(p);

	    // Still touching the command after "++"?
	    if (*p == NUL)
	    {
		if (ea.argt & EX_ARGOPT)
		    return set_context_in_argopt(xp, arg + 2);
#ifdef FEAT_TERMINAL
		if (ea.cmdidx == CMD_terminal)
		    return set_context_in_terminalopt(xp, arg + 2);
#endif
	    }

	    arg = skipwhite(p);
	}
    }

    if (ea.cmdidx == CMD_write || ea.cmdidx == CMD_update)
    {
	if (*arg == '>')			// append
	{
	    if (*++arg == '>')
		++arg;
	    arg = skipwhite(arg);
	}
	else if (*arg == '!' && ea.cmdidx == CMD_write)	// :w !filter
	{
	    ++arg;
	    usefilter = TRUE;
	}
    }

    if (ea.cmdidx == CMD_read)
    {
	usefilter = forceit;			// :r! filter if forced
	if (*arg == '!')			// :r !filter
	{
	    ++arg;
	    usefilter = TRUE;
	}
    }

    if (ea.cmdidx == CMD_lshift || ea.cmdidx == CMD_rshift)
    {
	while (*arg == *cmd)	    // allow any number of '>' or '<'
	    ++arg;
	arg = skipwhite(arg);
    }

    // Does command allow "+command"?
    if ((ea.argt & EX_CMDARG) && !usefilter && *arg == '+')
    {
	// Check if we're in the +command
	p = arg + 1;
	arg = skip_cmd_arg(arg, FALSE);

	// Still touching the command after '+'?
	if (*arg == NUL)
	    return p;

	// Skip space(s) after +command to get to the real argument
	arg = skipwhite(arg);
    }


    // Check for '|' to separate commands and '"' to start comments.
    // Don't do this for ":read !cmd" and ":write !cmd".
    if ((ea.argt & EX_TRLBAR) && !usefilter)
    {
	p = arg;
	// ":redir @" is not the start of a comment
	if (ea.cmdidx == CMD_redir && p[0] == '@' && p[1] == '"')
	    p += 2;
	while (*p)
	{
	    if (*p == Ctrl_V)
	    {
		if (p[1] != NUL)
		    ++p;
	    }
	    else if ( (*p == '"' && !(ea.argt & EX_NOTRLCOM))
		    || *p == '|' || *p == '\n')
	    {
		if (*(p - 1) != '\\')
		{
		    if (*p == '|' || *p == '\n')
			return p + 1;
		    return NULL;    // It's a comment
		}
	    }
	    MB_PTR_ADV(p);
	}
    }

    if (!(ea.argt & EX_EXTRA) && *arg != NUL
				  && vim_strchr((char_u *)"|\"", *arg) == NULL)
	// no arguments allowed but there is something
	return NULL;

    // Find start of last argument (argument just before cursor):
    p = buff;
    xp->xp_pattern = p;
    len = (int)STRLEN(buff);
    while (*p && p < buff + len)
    {
	if (*p == ' ' || *p == TAB)
	{
	    // argument starts after a space
	    xp->xp_pattern = ++p;
	}
	else
	{
	    if (*p == '\\' && *(p + 1) != NUL)
		++p; // skip over escaped character
	    MB_PTR_ADV(p);
	}
    }

    if (ea.argt & EX_XFILE)
	set_context_for_wildcard_arg(&ea, arg, usefilter, xp, &compl);

    // 6. Switch on command name.
    return set_context_by_cmdname(cmd, ea.cmdidx, xp, arg, ea.argt, compl,
								forceit);
}

/*
 * Set the completion context in 'xp' for command 'str'
 */
    void
set_cmd_context(
    expand_T	*xp,
    char_u	*str,	    // start of command line
    int		len,	    // length of command line (excl. NUL)
    int		col,	    // position of cursor
    int		use_ccline UNUSED) // use ccline for info
{
#ifdef FEAT_EVAL
    cmdline_info_T	*ccline = get_cmdline_info();
    int			context;
#endif
    int		old_char = NUL;
    char_u	*nextcomm;

    // Avoid a UMR warning from Purify, only save the character if it has been
    // written before.
    if (col < len)
	old_char = str[col];
    str[col] = NUL;
    nextcomm = str;

#ifdef FEAT_EVAL
    if (use_ccline && ccline->cmdfirstc == '=')
    {
	// pass CMD_SIZE because there is no real command
	set_context_for_expression(xp, str, CMD_SIZE);
    }
    else if (use_ccline && ccline->input_fn)
    {
	xp->xp_context = ccline->xp_context;
	xp->xp_pattern = ccline->cmdbuff;
	xp->xp_arg = ccline->xp_arg;
	if (xp->xp_context == EXPAND_SHELLCMDLINE)
	{
	    context = xp->xp_context;
	    set_context_for_wildcard_arg(NULL, xp->xp_pattern, FALSE, xp,
								     &context);
	}
    }
    else
#endif
	while (nextcomm != NULL)
	    nextcomm = set_one_cmd_context(xp, nextcomm);

    // Store the string here so that call_user_expand_func() can get to them
    // easily.
    xp->xp_line = str;
    xp->xp_col = col;

    str[col] = old_char;
}

/*
 * Expand the command line "str" from context "xp".
 * "xp" must have been set by set_cmd_context().
 * xp->xp_pattern points into "str", to where the text that is to be expanded
 * starts.
 * Returns EXPAND_UNSUCCESSFUL when there is something illegal before the
 * cursor.
 * Returns EXPAND_NOTHING when there is nothing to expand, might insert the
 * key that triggered expansion literally.
 * Returns EXPAND_OK otherwise.
 */
    int
expand_cmdline(
    expand_T	*xp,
    char_u	*str,		// start of command line
    int		col,		// position of cursor
    int		*matchcount,	// return: nr of matches
    char_u	***matches)	// return: array of pointers to matches
{
    char_u	*file_str = NULL;
    int		options = WILD_ADD_SLASH|WILD_SILENT;

    if (xp->xp_context == EXPAND_UNSUCCESSFUL)
    {
	beep_flush();
	return EXPAND_UNSUCCESSFUL;  // Something illegal on command line
    }
    if (xp->xp_context == EXPAND_NOTHING)
    {
	// Caller can use the character as a normal char instead
	return EXPAND_NOTHING;
    }

    // add star to file name, or convert to regexp if not exp. files.
    xp->xp_pattern_len = (int)(str + col - xp->xp_pattern);
    if (cmdline_fuzzy_completion_supported(xp))
	// If fuzzy matching, don't modify the search string
	file_str = vim_strsave(xp->xp_pattern);
    else
	file_str = addstar(xp->xp_pattern, xp->xp_pattern_len, xp->xp_context);
    if (file_str == NULL)
	return EXPAND_UNSUCCESSFUL;

    if (p_wic)
	options += WILD_ICASE;

    // find all files that match the description
    if (ExpandFromContext(xp, file_str, matches, matchcount, options) == FAIL)
    {
	*matchcount = 0;
	*matches = NULL;
    }
    vim_free(file_str);

    return EXPAND_OK;
}

/*
 * Expand file or directory names.
 * Returns OK or FAIL.
 */
    static int
expand_files_and_dirs(
	expand_T	*xp,
	char_u		*pat,
	char_u		***matches,
	int		*numMatches,
	int		flags,
	int		options)
{
    int		free_pat = FALSE;
    int		ret = FAIL;

    // for ":set path=" and ":set tags=" halve backslashes for escaped
    // space
    if (xp->xp_backslash != XP_BS_NONE)
    {
	size_t	pat_len;
	char_u	*pat_end;
	char_u	*p;

	free_pat = TRUE;

	pat_len = STRLEN(pat);
	pat = vim_strnsave(pat, pat_len);
	if (pat == NULL)
	    return ret;

	pat_end = pat + pat_len;
	for (p = pat; *p != NUL; ++p)
	{
	    char_u  *from;

	    if (*p != '\\')
		continue;

	    if (xp->xp_backslash & XP_BS_THREE
		&& *(p + 1) == '\\'
		&& *(p + 2) == '\\'
		&& *(p + 3) == ' ')
	    {
		from = p + 3;
		mch_memmove(p, from,
		    (size_t)(pat_end - from) + 1);   // +1 for NUL
		pat_end -= 3;
	    }
	    else if (xp->xp_backslash & XP_BS_ONE
		&& *(p + 1) == ' ')
	    {
		from = p + 1;
		mch_memmove(p, from,
		    (size_t)(pat_end - from) + 1);   // +1 for NUL
		--pat_end;
	    }
	    else if (xp->xp_backslash & XP_BS_COMMA)
	    {
		if (*(p + 1) == '\\' && *(p + 2) == ',')
		{
		    from = p + 2;
		    mch_memmove(p, from,
			(size_t)(pat_end - from) + 1);   // +1 for NUL
		    pat_end -= 2;
		}
#ifdef BACKSLASH_IN_FILENAME
		else if (*(p + 1) == ',')
		{
		    from = p + 1;
		    mch_memmove(p, from,
			(size_t)(pat_end - from) + 1);   // +1 for NUL
		    --pat_end;
		}
#endif
	    }
	}
    }

    if (xp->xp_context == EXPAND_FINDFUNC)
    {
#ifdef FEAT_EVAL
	ret = expand_findfunc(pat, matches, numMatches);
#endif
    }
    else
    {
	if (xp->xp_context == EXPAND_FILES)
	    flags |= EW_FILE;
	else if (xp->xp_context == EXPAND_FILES_IN_PATH)
	    flags |= (EW_FILE | EW_PATH);
	else if (xp->xp_context == EXPAND_DIRS_IN_CDPATH)
	    flags = (flags | EW_DIR | EW_CDPATH) & ~EW_FILE;
	else
	    flags = (flags | EW_DIR) & ~EW_FILE;
	if (options & WILD_ICASE)
	    flags |= EW_ICASE;

	// Expand wildcards, supporting %:h and the like.
	ret = expand_wildcards_eval(&pat, numMatches, matches, flags);
    }
    if (free_pat)
	vim_free(pat);
#ifdef BACKSLASH_IN_FILENAME
    if (p_csl[0] != NUL && (options & WILD_IGNORE_COMPLETESLASH) == 0)
    {
	int j;

	for (j = 0; j < *numMatches; ++j)
	{
	    char_u	*ptr = (*matches)[j];

	    while (*ptr != NUL)
	    {
		if (p_csl[0] == 's' && *ptr == '\\')
		    *ptr = '/';
		else if (p_csl[0] == 'b' && *ptr == '/')
		    *ptr = '\\';
		ptr += (*mb_ptr2len)(ptr);
	    }
	}
    }
#endif
    return ret;
}

/*
 * Function given to ExpandGeneric() to obtain the possible arguments of the
 * ":behave {mswin,xterm}" command.
 */
    static char_u *
get_behave_arg(expand_T *xp UNUSED, int idx)
{
    if (idx == 0)
	return (char_u *)"mswin";
    if (idx == 1)
	return (char_u *)"xterm";
    return NULL;
}

/*
 * Function given to ExpandGeneric() to obtain the possible arguments of the
 * ":filetype {plugin,indent}" command.
 */
    static char_u *
get_filetypecmd_arg(expand_T *xp UNUSED, int idx)
{
    if (idx < 0)
	return NULL;

    if (filetype_expand_what == EXP_FILETYPECMD_ALL && idx < 4)
    {
	char	*opts_all[] = {"indent", "plugin", "on", "off"};
	return (char_u *)opts_all[idx];
    }
    if (filetype_expand_what == EXP_FILETYPECMD_PLUGIN && idx < 3)
    {
	char	*opts_plugin[] = {"plugin", "on", "off"};
	return (char_u *)opts_plugin[idx];
    }
    if (filetype_expand_what == EXP_FILETYPECMD_INDENT && idx < 3)
    {
	char	*opts_indent[] = {"indent", "on", "off"};
	return (char_u *)opts_indent[idx];
    }
    if (filetype_expand_what == EXP_FILETYPECMD_ONOFF && idx < 2)
    {
	char	*opts_onoff[] = {"on", "off"};
	return (char_u *)opts_onoff[idx];
    }
    return NULL;
}

#ifdef FEAT_EVAL
/*
 * Function given to ExpandGeneric() to obtain the possible arguments of the
 * ":breakadd {expr, file, func, here}" command.
 * ":breakdel {func, file, here}" command.
 */
    static char_u *
get_breakadd_arg(expand_T *xp UNUSED, int idx)
{
    if (idx >= 0 && idx <= 3)
    {
	char	*opts[] = {"expr", "file", "func", "here"};

	// breakadd {expr, file, func, here}
	if (breakpt_expand_what == EXP_BREAKPT_ADD)
	    return (char_u *)opts[idx];
	else if (breakpt_expand_what == EXP_BREAKPT_DEL)
	{
	    // breakdel {func, file, here}
	    if (idx <= 2)
		return (char_u *)opts[idx + 1];
	}
	else
	{
	    // profdel {func, file}
	    if (idx <= 1)
		return (char_u *)opts[idx + 1];
	}
    }
    return NULL;
}

/*
 * Function given to ExpandGeneric() to obtain the possible arguments for the
 * ":scriptnames" command.
 */
    static char_u *
get_scriptnames_arg(expand_T *xp UNUSED, int idx)
{
    scriptitem_T *si;

    if (!SCRIPT_ID_VALID(idx + 1))
	return NULL;

    si = SCRIPT_ITEM(idx + 1);
    home_replace(NULL, si->sn_name, NameBuff, MAXPATHL, TRUE);
    return NameBuff;
}
#endif

/*
 * Function given to ExpandGeneric() to obtain the possible arguments of the
 * ":retab {-indentonly}" option.
 */
    static char_u *
get_retab_arg(expand_T *xp UNUSED, int idx)
{
    if (idx == 0)
	return (char_u *)"-indentonly";
    return NULL;
}

/*
 * Function given to ExpandGeneric() to obtain the possible arguments of the
 * ":messages {clear}" command.
 */
    static char_u *
get_messages_arg(expand_T *xp UNUSED, int idx)
{
    if (idx == 0)
	return (char_u *)"clear";
    return NULL;
}

    static char_u *
get_mapclear_arg(expand_T *xp UNUSED, int idx)
{
    if (idx == 0)
	return (char_u *)"<buffer>";
    return NULL;
}

/*
 * Do the expansion based on xp->xp_context and 'rmp'.
 */
    static int
ExpandOther(
	char_u		*pat,
	expand_T	*xp,
	regmatch_T	*rmp,
	char_u		***matches,
	int		*numMatches)
{
    static struct expgen
    {
	int		context;
	char_u	*((*func)(expand_T *, int));
	int		ic;
	int		escaped;
    } tab[] =
    {
	{EXPAND_COMMANDS, get_command_name, FALSE, TRUE},
	{EXPAND_BEHAVE, get_behave_arg, TRUE, TRUE},
	{EXPAND_FILETYPECMD, get_filetypecmd_arg, TRUE, TRUE},
	{EXPAND_MAPCLEAR, get_mapclear_arg, TRUE, TRUE},
	{EXPAND_MESSAGES, get_messages_arg, TRUE, TRUE},
	{EXPAND_HISTORY, get_history_arg, TRUE, TRUE},
	{EXPAND_USER_COMMANDS, get_user_commands, FALSE, TRUE},
	{EXPAND_USER_ADDR_TYPE, get_user_cmd_addr_type, FALSE, TRUE},
	{EXPAND_USER_CMD_FLAGS, get_user_cmd_flags, FALSE, TRUE},
	{EXPAND_USER_NARGS, get_user_cmd_nargs, FALSE, TRUE},
	{EXPAND_USER_COMPLETE, get_user_cmd_complete, FALSE, TRUE},
#ifdef FEAT_EVAL
	{EXPAND_USER_VARS, get_user_var_name, FALSE, TRUE},
	{EXPAND_FUNCTIONS, get_function_name, FALSE, TRUE},
	{EXPAND_USER_FUNC, get_user_func_name, FALSE, TRUE},
	{EXPAND_DISASSEMBLE, get_disassemble_argument, FALSE, TRUE},
	{EXPAND_EXPRESSION, get_expr_name, FALSE, TRUE},
#endif
#ifdef FEAT_MENU
	{EXPAND_MENUS, get_menu_name, FALSE, TRUE},
	{EXPAND_MENUNAMES, get_menu_names, FALSE, TRUE},
#endif
#ifdef FEAT_SYN_HL
	{EXPAND_SYNTAX, get_syntax_name, TRUE, TRUE},
#endif
#ifdef FEAT_PROFILE
	{EXPAND_SYNTIME, get_syntime_arg, TRUE, TRUE},
#endif
	{EXPAND_HIGHLIGHT, get_highlight_name, TRUE, TRUE},
	{EXPAND_EVENTS, get_event_name, TRUE, FALSE},
	{EXPAND_AUGROUP, get_augroup_name, TRUE, FALSE},
#ifdef FEAT_CSCOPE
	{EXPAND_CSCOPE, get_cscope_name, TRUE, TRUE},
#endif
#ifdef FEAT_SIGNS
	{EXPAND_SIGN, get_sign_name, TRUE, TRUE},
#endif
#ifdef FEAT_PROFILE
	{EXPAND_PROFILE, get_profile_name, TRUE, TRUE},
#endif
#if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
	{EXPAND_LANGUAGE, get_lang_arg, TRUE, FALSE},
	{EXPAND_LOCALES, get_locales, TRUE, FALSE},
#endif
	{EXPAND_ENV_VARS, get_env_name, TRUE, TRUE},
	{EXPAND_USER, get_users, TRUE, FALSE},
	{EXPAND_ARGLIST, get_arglist_name, TRUE, FALSE},
#ifdef FEAT_EVAL
	{EXPAND_BREAKPOINT, get_breakadd_arg, TRUE, TRUE},
	{EXPAND_SCRIPTNAMES, get_scriptnames_arg, TRUE, FALSE},
#endif
	{EXPAND_RETAB, get_retab_arg, TRUE, TRUE},
    };
    int	i;
    int ret = FAIL;

    // Find a context in the table and call the ExpandGeneric() with the
    // right function to do the expansion.
    for (i = 0; i < (int)ARRAY_LENGTH(tab); ++i)
    {
	if (xp->xp_context == tab[i].context)
	{
	    if (tab[i].ic)
		rmp->rm_ic = TRUE;
	    ret = ExpandGeneric(pat, xp, rmp, matches, numMatches,
						tab[i].func, tab[i].escaped);
	    break;
	}
    }

    return ret;
}

/*
 * Map wild expand options to flags for expand_wildcards()
 */
    static int
map_wildopts_to_ewflags(int options)
{
    int		flags;

    flags = EW_DIR;	// include directories
    if (options & WILD_LIST_NOTFOUND)
	flags |= EW_NOTFOUND;
    if (options & WILD_ADD_SLASH)
	flags |= EW_ADDSLASH;
    if (options & WILD_KEEP_ALL)
	flags |= EW_KEEPALL;
    if (options & WILD_SILENT)
	flags |= EW_SILENT;
    if (options & WILD_NOERROR)
	flags |= EW_NOERROR;
    if (options & WILD_ALLLINKS)
	flags |= EW_ALLLINKS;

    return flags;
}

/*
 * Do the expansion based on xp->xp_context and "pat".
 */
    static int
ExpandFromContext(
    expand_T	*xp,
    char_u	*pat,
    char_u	***matches,
    int		*numMatches,
    int		options)  // WILD_ flags
{
    regmatch_T	regmatch;
    int		ret;
    int		flags;
    char_u	*tofree = NULL;
    int		fuzzy = cmdline_fuzzy_complete(pat)
				     && cmdline_fuzzy_completion_supported(xp);

    flags = map_wildopts_to_ewflags(options);

    if (xp->xp_context == EXPAND_FILES
	    || xp->xp_context == EXPAND_DIRECTORIES
	    || xp->xp_context == EXPAND_FILES_IN_PATH
	    || xp->xp_context == EXPAND_FINDFUNC
	    || xp->xp_context == EXPAND_DIRS_IN_CDPATH)
	return expand_files_and_dirs(xp, pat, matches, numMatches, flags,
								options);

    *matches = (char_u **)"";
    *numMatches = 0;
    if (xp->xp_context == EXPAND_HELP)
    {
	// With an empty argument we would get all the help tags, which is
	// very slow.  Get matches for "help" instead.
	if (find_help_tags(*pat == NUL ? (char_u *)"help" : pat,
					numMatches, matches, FALSE) == OK)
	{
#ifdef FEAT_MULTI_LANG
	    cleanup_help_tags(*numMatches, *matches);
#endif
	    return OK;
	}
	return FAIL;
    }

    if (xp->xp_context == EXPAND_SHELLCMD)
	return expand_shellcmd(pat, matches, numMatches, flags);
    if (xp->xp_context == EXPAND_OLD_SETTING)
	return ExpandOldSetting(numMatches, matches);
    if (xp->xp_context == EXPAND_BUFFERS)
	return ExpandBufnames(pat, numMatches, matches, options);
#ifdef FEAT_DIFF
    if (xp->xp_context == EXPAND_DIFF_BUFFERS)
	return ExpandBufnames(pat, numMatches, matches,
						options | BUF_DIFF_FILTER);
#endif
    if (xp->xp_context == EXPAND_TAGS
	    || xp->xp_context == EXPAND_TAGS_LISTFILES)
	return expand_tags(xp->xp_context == EXPAND_TAGS, pat, numMatches,
								matches);
    if (xp->xp_context == EXPAND_COLORS)
    {
	char *directories[] = {"colors", NULL};
	return ExpandRTDir(pat, DIP_START + DIP_OPT, numMatches, matches,
								directories);
    }
    if (xp->xp_context == EXPAND_COMPILER)
    {
	char *directories[] = {"compiler", NULL};
	return ExpandRTDir(pat, 0, numMatches, matches, directories);
    }
    if (xp->xp_context == EXPAND_OWNSYNTAX)
    {
	char *directories[] = {"syntax", NULL};
	return ExpandRTDir(pat, 0, numMatches, matches, directories);
    }
    if (xp->xp_context == EXPAND_FILETYPE)
    {
	char *directories[] = {"syntax", "indent", "ftplugin", NULL};
	return ExpandRTDir(pat, 0, numMatches, matches, directories);
    }
#ifdef FEAT_KEYMAP
    if (xp->xp_context == EXPAND_KEYMAP)
    {
	char *directories[] = {"keymap", NULL};
	return ExpandRTDir(pat, 0, numMatches, matches, directories);
    }
#endif
#if defined(FEAT_EVAL)
    if (xp->xp_context == EXPAND_USER_LIST)
	return ExpandUserList(xp, matches, numMatches);
#endif
    if (xp->xp_context == EXPAND_PACKADD)
	return ExpandPackAddDir(pat, numMatches, matches);
    if (xp->xp_context == EXPAND_RUNTIME)
	return expand_runtime_cmd(pat, numMatches, matches);
    if (xp->xp_context == EXPAND_PATTERN_IN_BUF)
	return expand_pattern_in_buf(pat, xp->xp_search_dir,
		matches, numMatches);

    // When expanding a function name starting with s:, match the <SNR>nr_
    // prefix.
    if ((xp->xp_context == EXPAND_USER_FUNC
				       || xp->xp_context == EXPAND_DISASSEMBLE)
	    && STRNCMP(pat, "^s:", 3) == 0)
    {
	int len = (int)STRLEN(pat) + 20;

	tofree = alloc(len);
	if (tofree == NULL)
	    return FAIL;
	vim_snprintf((char *)tofree, len, "^<SNR>\\d\\+_%s", pat + 3);
	pat = tofree;
    }

    if (!fuzzy)
    {
	regmatch.regprog = vim_regcomp(pat, magic_isset() ? RE_MAGIC : 0);
	if (regmatch.regprog == NULL)
	    return FAIL;

	// set ignore-case according to p_ic, p_scs and pat
	regmatch.rm_ic = ignorecase(pat);
    }

    if (xp->xp_context == EXPAND_SETTINGS
	    || xp->xp_context == EXPAND_BOOL_SETTINGS)
	ret = ExpandSettings(xp, &regmatch, pat, numMatches, matches, fuzzy);
    else if (xp->xp_context == EXPAND_STRING_SETTING)
	ret = ExpandStringSetting(xp, &regmatch, numMatches, matches);
    else if (xp->xp_context == EXPAND_SETTING_SUBTRACT)
	ret = ExpandSettingSubtract(xp, &regmatch, numMatches, matches);
    else if (xp->xp_context == EXPAND_MAPPINGS)
	ret = ExpandMappings(pat, &regmatch, numMatches, matches);
    else if (xp->xp_context == EXPAND_ARGOPT)
	ret = expand_argopt(pat, xp, &regmatch, matches, numMatches);
    else if (xp->xp_context == EXPAND_HIGHLIGHT_GROUP)
	ret = expand_highlight_group(pat, xp, &regmatch, matches, numMatches);
#if defined(FEAT_TERMINAL)
    else if (xp->xp_context == EXPAND_TERMINALOPT)
	ret = expand_terminal_opt(pat, xp, &regmatch, matches, numMatches);
#endif
#if defined(FEAT_EVAL)
    else if (xp->xp_context == EXPAND_USER_DEFINED)
	ret = ExpandUserDefined(pat, xp, &regmatch, matches, numMatches);
#endif
    else
	ret = ExpandOther(pat, xp, &regmatch, matches, numMatches);

    if (!fuzzy)
	vim_regfree(regmatch.regprog);
    vim_free(tofree);

    return ret;
}

    int
ExpandGeneric(
    char_u	*pat,
    expand_T	*xp,
    regmatch_T	*regmatch,
    char_u	***matches,
    int		*numMatches,
    char_u	*(*func)(expand_T *, int),
					  // returns a string from the list
    int		escaped)
{
    return ExpandGenericExt(
	pat, xp, regmatch, matches, numMatches, func, escaped, 0);
}

/*
 * Expand a list of names.
 *
 * Generic function for command line completion.  It calls a function to
 * obtain strings, one by one.	The strings are matched against a regexp
 * program.  Matching strings are copied into an array, which is returned.
 *
 * If 'fuzzy' is TRUE, then fuzzy matching is used. Otherwise, regex matching
 * is used.
 *
 * 'sortStartIdx' allows the caller to control sorting behavior. Items before
 * the index will not be sorted. Pass 0 to sort all, and -1 to prevent any
 * sorting.
 *
 * Returns OK when no problems encountered, FAIL for error (out of memory).
 */
    int
ExpandGenericExt(
    char_u	*pat,
    expand_T	*xp,
    regmatch_T	*regmatch,
    char_u	***matches,
    int		*numMatches,
    char_u	*(*func)(expand_T *, int),
					  // returns a string from the list
    int		escaped,
    int		sortStartIdx)
{
    int		i;
    garray_T	ga;
    char_u	*str;
    fuzmatch_str_T	*fuzmatch = NULL;
    int		score = 0;
    int		fuzzy;
    int		match;
    int		sort_matches = FALSE;
    int		funcsort = FALSE;
    int		sortStartMatchIdx = -1;

    fuzzy = cmdline_fuzzy_complete(pat);
    *matches = NULL;
    *numMatches = 0;

    if (!fuzzy)
	ga_init2(&ga, sizeof(char *), 30);
    else
	ga_init2(&ga, sizeof(fuzmatch_str_T), 30);

    for (i = 0; ; ++i)
    {
	str = (*func)(xp, i);
	if (str == NULL)	    // end of list
	    break;
	if (*str == NUL)	    // skip empty strings
	    continue;

	if (xp->xp_pattern[0] != NUL)
	{
	    if (!fuzzy)
		match = vim_regexec(regmatch, str, (colnr_T)0);
	    else
	    {
		score = fuzzy_match_str(str, pat);
		match = (score != FUZZY_SCORE_NONE);
	    }
	}
	else
	    match = TRUE;

	if (!match)
	    continue;

	if (escaped)
	    str = vim_strsave_escaped(str, (char_u *)" \t\\.");
	else
	    str = vim_strsave(str);
	if (str == NULL)
	{
	    if (!fuzzy)
	    {
		ga_clear_strings(&ga);
		return FAIL;
	    }
	    fuzmatch_str_free(ga.ga_data, ga.ga_len);
	    return FAIL;
	}

	if (ga_grow(&ga, 1) == FAIL)
	{
	    vim_free(str);
	    break;
	}

	if (fuzzy)
	{
	    fuzmatch = &((fuzmatch_str_T *)ga.ga_data)[ga.ga_len];
	    fuzmatch->idx = ga.ga_len;
	    fuzmatch->str = str;
	    fuzmatch->score = score;
	}
	else
	    ((char_u **)ga.ga_data)[ga.ga_len] = str;

#ifdef FEAT_MENU
	if (func == get_menu_names)
	{
	    // test for separator added by get_menu_names()
	    str += STRLEN(str) - 1;
	    if (*str == '\001')
		*str = '.';
	}
#endif

	if (sortStartIdx >= 0 && i >= sortStartIdx && sortStartMatchIdx == -1)
	{
	    // Found first item to start sorting from. This is usually 0.
	    sortStartMatchIdx = ga.ga_len;
	}

	++ga.ga_len;
    }

    if (ga.ga_len == 0)
	return OK;

    // sort the matches when using regular expression matching and sorting
    // applies to the completion context. Menus and scriptnames should be kept
    // in the specified order.
    if (!fuzzy && xp->xp_context != EXPAND_MENUNAMES
					&& xp->xp_context != EXPAND_STRING_SETTING
					&& xp->xp_context != EXPAND_MENUS
					&& xp->xp_context != EXPAND_SCRIPTNAMES
					&& xp->xp_context != EXPAND_ARGOPT
					&& xp->xp_context != EXPAND_TERMINALOPT)
	sort_matches = TRUE;

    // <SNR> functions should be sorted to the end.
    if (xp->xp_context == EXPAND_EXPRESSION
	    || xp->xp_context == EXPAND_FUNCTIONS
	    || xp->xp_context == EXPAND_USER_FUNC
	    || xp->xp_context == EXPAND_DISASSEMBLE)
	funcsort = TRUE;

    // Sort the matches.
    if (sort_matches && sortStartMatchIdx != -1)
    {
	if (funcsort)
	    // <SNR> functions should be sorted to the end.
	    qsort((void *)ga.ga_data, (size_t)ga.ga_len, sizeof(char_u *),
							   sort_func_compare);
	else
	    sort_strings((char_u **)ga.ga_data + sortStartMatchIdx, ga.ga_len - sortStartMatchIdx);
    }

    if (!fuzzy)
    {
	*matches = ga.ga_data;
	*numMatches = ga.ga_len;
    }
    else
    {
	if (fuzzymatches_to_strmatches(ga.ga_data, matches, ga.ga_len,
							funcsort) == FAIL)
	    return FAIL;
	*numMatches = ga.ga_len;
    }

#if defined(FEAT_SYN_HL)
    // Reset the variables used for special highlight names expansion, so that
    // they don't show up when getting normal highlight names by ID.
    reset_expand_highlight();
#endif

    return OK;
}

/*
 * Expand shell command matches in one directory of $PATH.
 */
    static void
expand_shellcmd_onedir(
	char_u		*pathed_pattern,    // fully pathed pattern
	size_t		pathlen,	    // length of the path portion of pathed_pattern
					    // (0 if no path).
	char_u		***matches,
	int		*numMatches,
	int		flags,
	hashtab_T	*ht,
	garray_T	*gap)
{
    // Expand matches in one directory of $PATH.
    if (expand_wildcards(1, &pathed_pattern, numMatches, matches, flags) != OK)
	return;

    if (ga_grow(gap, *numMatches) == FAIL)
    {
	FreeWild(*numMatches, *matches);
	return;
    }

    for (int i = 0; i < *numMatches; ++i)
    {
	char_u	*name = (*matches)[i];
	size_t	namelen = STRLEN(name);

	if (namelen > pathlen)
	{
	    hash_T	hash;
	    hashitem_T	*hi;

	    // Check if this name was already found.
	    hash = hash_hash(name + pathlen);
	    hi = hash_lookup(ht, name + pathlen, hash);
	    if (HASHITEM_EMPTY(hi))
	    {
		// Remove the path that was prepended.
		mch_memmove(name, name + pathlen,
		    (size_t)(namelen - pathlen) + 1);   // +1 for NUL
		((char_u **)gap->ga_data)[gap->ga_len++] = name;
		hash_add_item(ht, hi, name, hash);
		name = NULL;
	    }
	}
	vim_free(name);
    }
    vim_free(*matches);
}

/*
 * Complete a shell command.
 * Returns FAIL or OK;
 */
    static int
expand_shellcmd(
    char_u	*filepat,	// pattern to match with command names
    char_u	***matches,	// return: array with matches
    int		*numMatches,	// return: number of matches
    int		flagsarg)	// EW_ flags
{
    char_u	*pat;
    size_t	patlen;
    char_u	*path = NULL;
    int		mustfree = FALSE;
    garray_T    ga;
    char_u	*buf;
    char_u	*s, *e;
    int		flags = flagsarg;
    int		did_curdir = FALSE;
    hashtab_T	found_ht;

    buf = alloc(MAXPATHL);
    if (buf == NULL)
	return FAIL;

    // for ":set path=" and ":set tags=" halve backslashes for escaped space
    patlen = STRLEN(filepat);
    pat = vim_strnsave(filepat, patlen);
    if (pat == NULL)
    {
	vim_free(buf);
	return FAIL;
    }

    // Replace "\ " with " ".
    e = pat + patlen;
    for (s = pat; *s != NUL; ++s)
    {
	char_u  *p;

	if (*s != '\\')
	    continue;

	p = s + 1;
	if (*p == ' ')
	{
	    mch_memmove(s, p, (size_t)(e - p) + 1);     // +1 for NUL
	    --e;
	}
    }
    patlen = (size_t)(e - pat);

    flags |= EW_FILE | EW_EXEC | EW_SHELLCMD;

    if (pat[0] == '.' && (vim_ispathsep(pat[1])
			       || (pat[1] == '.' && vim_ispathsep(pat[2]))))
	path = (char_u *)".";
    else
    {
	// For an absolute name we don't use $PATH.
	if (!mch_isFullName(pat))
	    path = vim_getenv((char_u *)"PATH", &mustfree);
	if (path == NULL)
	    path = (char_u *)"";
    }

    // Go over all directories in $PATH.  Expand matches in that directory and
    // collect them in "ga".  When "." is not in $PATH also expand for the
    // current directory, to find "subdir/cmd".
    ga_init2(&ga, sizeof(char *), 10);
    hash_init(&found_ht);
    for (s = path; ; s = e)
    {
	size_t	pathlen;	// length of the path portion of buf
				// (including trailing slash).
	size_t	seplen;

	if (*s == NUL)
	{
	    if (did_curdir)
		break;

	    // Find directories in the current directory, path is empty.
	    did_curdir = TRUE;
	    flags |= EW_DIR;

	    e = s;
	    pathlen = 0;
	    seplen = 0;
	}
	else
	{
#if defined(MSWIN)
	    e = vim_strchr(s, ';');
#else
	    e = vim_strchr(s, ':');
#endif
	    if (e == NULL)
		e = s + STRLEN(s);

	    pathlen = (size_t)(e - s);
	    if (STRNCMP(s, ".", pathlen) == 0)
	    {
		did_curdir = TRUE;
		flags |= EW_DIR;
	    }
	    else
		// Do not match directories inside a $PATH item.
		flags &= ~EW_DIR;

	    seplen = !after_pathsep(s, e) ? STRLEN_LITERAL(PATHSEPSTR) : 0;
	}

	// Make sure that the pathed pattern (ie the path and pattern concatenated
	// together) will fit inside the buffer. If not skip it and move on to the
	// next path.
	if (pathlen + seplen + patlen + 1 <= MAXPATHL)
	{
	    if (pathlen > 0)
	    {
		vim_strncpy(buf, s, pathlen);
		if (seplen > 0)
		{
		    STRCPY(buf + pathlen, PATHSEPSTR);
		    pathlen += seplen;
		}
	    }
	    STRCPY(buf + pathlen, pat);

	    expand_shellcmd_onedir(buf, pathlen, matches, numMatches, flags,
				    &found_ht, &ga);
	}

	if (*e != NUL)
	    ++e;
    }
    *matches = ga.ga_data;
    *numMatches = ga.ga_len;

    vim_free(buf);
    vim_free(pat);
    if (mustfree)
	vim_free(path);
    hash_clear(&found_ht);
    return OK;
}

#if defined(FEAT_EVAL)
/*
 * Call "user_expand_func()" to invoke a user defined Vim script function and
 * return the result (either a string, a List or NULL).
 */
    static void *
call_user_expand_func(
    void	*(*user_expand_func)(char_u *, int, typval_T *),
    expand_T	*xp)
{
    cmdline_info_T	*ccline = get_cmdline_info();
    int		keep = 0;
    typval_T	args[4];
    sctx_T	save_current_sctx = current_sctx;
    char_u	*pat = NULL;
    void	*ret;

    if (xp->xp_arg == NULL || xp->xp_arg[0] == '\0' || xp->xp_line == NULL)
	return NULL;

    if (ccline->cmdbuff != NULL)
    {
	keep = ccline->cmdbuff[ccline->cmdlen];
	ccline->cmdbuff[ccline->cmdlen] = 0;
    }

    pat = vim_strnsave(xp->xp_pattern, xp->xp_pattern_len);

    args[0].v_type = VAR_STRING;
    args[0].vval.v_string = pat;
    args[1].v_type = VAR_STRING;
    args[1].vval.v_string = xp->xp_line;
    args[2].v_type = VAR_NUMBER;
    args[2].vval.v_number = xp->xp_col;
    args[3].v_type = VAR_UNKNOWN;

    current_sctx = xp->xp_script_ctx;

    ret = user_expand_func(xp->xp_arg, 3, args);

    current_sctx = save_current_sctx;
    if (ccline->cmdbuff != NULL)
	ccline->cmdbuff[ccline->cmdlen] = keep;

    vim_free(pat);
    return ret;
}

/*
 * Expand names with a function defined by the user (EXPAND_USER_DEFINED and
 * EXPAND_USER_LIST).
 */
    static int
ExpandUserDefined(
    char_u	*pat,
    expand_T	*xp,
    regmatch_T	*regmatch,
    char_u	***matches,
    int		*numMatches)
{
    char_u	*retstr;
    char_u	*s;
    char_u	*e;
    int		keep;
    garray_T	ga;
    int		fuzzy;
    int		match;
    int		score = 0;

    fuzzy = cmdline_fuzzy_complete(pat);
    *matches = NULL;
    *numMatches = 0;

    retstr = call_user_expand_func(call_func_retstr, xp);
    if (retstr == NULL)
	return FAIL;

    if (!fuzzy)
	ga_init2(&ga, sizeof(char *), 3);
    else
	ga_init2(&ga, sizeof(fuzmatch_str_T), 3);

    for (s = retstr; *s != NUL; s = e)
    {
	e = vim_strchr(s, '\n');
	if (e == NULL)
	    e = s + STRLEN(s);
	keep = *e;
	*e = NUL;

	if (xp->xp_pattern[0] != NUL)
	{
	    if (!fuzzy)
		match = vim_regexec(regmatch, s, (colnr_T)0);
	    else
	    {
		score = fuzzy_match_str(s, pat);
		match = (score != FUZZY_SCORE_NONE);
	    }
	}
	else
	    match = TRUE;		// match everything

	*e = keep;

	if (match)
	{
	    char_u  *p = vim_strnsave(s, (size_t)(e - s));
	    if (p == NULL)
		break;

	    if (ga_grow(&ga, 1) == FAIL)
	    {
		vim_free(p);
		break;
	    }

	    if (!fuzzy)
		((char_u **)ga.ga_data)[ga.ga_len] = p;
	    else
	    {
		fuzmatch_str_T  *fuzmatch =
				&((fuzmatch_str_T  *)ga.ga_data)[ga.ga_len];
		fuzmatch->idx = ga.ga_len;
		fuzmatch->str = p;
		fuzmatch->score = score;
	    }
	    ++ga.ga_len;
	}

	if (*e != NUL)
	    ++e;
    }
    vim_free(retstr);

    if (ga.ga_len == 0)
	return OK;

    if (!fuzzy)
    {
	*matches = ga.ga_data;
	*numMatches = ga.ga_len;
    }
    else
    {
	if (fuzzymatches_to_strmatches(ga.ga_data, matches, ga.ga_len,
								FALSE) == FAIL)
	    return FAIL;
	*numMatches = ga.ga_len;
    }
    return OK;
}

/*
 * Expand names with a list returned by a function defined by the user.
 */
    static int
ExpandUserList(
    expand_T	*xp,
    char_u	***matches,
    int		*numMatches)
{
    list_T      *retlist;
    listitem_T	*li;
    garray_T	ga;

    *matches = NULL;
    *numMatches = 0;
    retlist = call_user_expand_func(call_func_retlist, xp);
    if (retlist == NULL)
	return FAIL;

    ga_init2(&ga, sizeof(char *), 3);
    // Loop over the items in the list.
    FOR_ALL_LIST_ITEMS(retlist, li)
    {
	char_u	*p;

	if (li->li_tv.v_type != VAR_STRING || li->li_tv.vval.v_string == NULL)
	    continue;  // Skip non-string items and empty strings

	p = vim_strsave(li->li_tv.vval.v_string);
	if (p == NULL)
	    break;

	if (ga_grow(&ga, 1) == FAIL)
	{
	    vim_free(p);
	    break;
	}

	((char_u **)ga.ga_data)[ga.ga_len] = p;
	++ga.ga_len;
    }
    list_unref(retlist);

    *matches = ga.ga_data;
    *numMatches = ga.ga_len;
    return OK;
}
#endif

/*
 * Expand "file" for all comma-separated directories in "path".
 * Adds the matches to "ga".  Caller must init "ga".
 * If "dirs" is TRUE only expand directory names.
 */
    void
globpath(
    char_u	*path,
    char_u	*file,
    garray_T	*ga,
    int		expand_options,
    int		dirs)
{
    expand_T	xpc;
    char_u	*buf;
    size_t	pathlen;	    // length of the path portion of buf
				    // (including trailing slash)
    size_t	seplen;
    size_t	filelen;

    buf = alloc(MAXPATHL);
    if (buf == NULL)
	return;

    ExpandInit(&xpc);
    xpc.xp_context = dirs ? EXPAND_DIRECTORIES : EXPAND_FILES;

    filelen = STRLEN(file);

#if defined(MSWIN)
    // Using the platform's path separator (\) makes vim incorrectly
    // treat it as an escape character, use '/' instead.
    #define TMP_PATHSEPSTR "/"
#else
    #define TMP_PATHSEPSTR PATHSEPSTR
#endif

    // Loop over all entries in {path}.
    while (*path != NUL)
    {
	// Copy one item of the path to buf[] and concatenate the file name.
	pathlen = (size_t)copy_option_part(&path, buf, MAXPATHL, ",");
	seplen = (*buf != NUL && !after_pathsep(buf, buf + pathlen))
	    ? STRLEN_LITERAL(TMP_PATHSEPSTR)
	    : 0;

	if (pathlen + seplen + filelen + 1 <= MAXPATHL)
	{
	    int	    num_p;
	    char_u  **p;

	    if (seplen > 0)
	    {
		STRCPY(buf + pathlen, TMP_PATHSEPSTR);
		pathlen += seplen;
	    }
	    STRCPY(buf + pathlen, file);

	    if (ExpandFromContext(&xpc, buf, &p, &num_p,
			     WILD_SILENT|expand_options) != FAIL && num_p > 0)
	    {
		ExpandEscape(&xpc, buf, num_p, p, WILD_SILENT|expand_options);

		if (ga_grow(ga, num_p) == OK)
		{
		    // take over the pointers and put them in "ga"
		    for (int i = 0; i < num_p; ++i)
		    {
			((char_u **)ga->ga_data)[ga->ga_len] = p[i];
			++ga->ga_len;
		    }
		}
		vim_free(p);
	    }
	}
    }

    vim_free(buf);
}
#undef TMP_PATHSEPSTR

/*
 * Translate some keys pressed when 'wildmenu' is used.
 */
    int
wildmenu_translate_key(
	cmdline_info_T	*cclp,
	int		key,
	expand_T	*xp,
	int		did_wild_list)
{
    int c = key;

    if (cmdline_pum_active())
    {
	// When the popup menu is used for cmdline completion:
	//   Up	  : go to the previous item in the menu
	//   Down : go to the next item in the menu
	//   Left : go to the parent directory
	//   Right: list the files in the selected directory
	switch (c)
	{
	    case K_UP:	  c = K_LEFT; break;
	    case K_DOWN:  c = K_RIGHT; break;
	    case K_LEFT:  c = K_UP; break;
	    case K_RIGHT: c = K_DOWN; break;
	    default:	  break;
	}
    }

    if (cmdline_pum_active() || did_wild_list || wild_menu_showing)
    {
	if (c == K_LEFT)
	    c = Ctrl_P;
	else if (c == K_RIGHT)
	    c = Ctrl_N;
    }

    // Hitting CR after "emenu Name.": complete submenu
    if (xp->xp_context == EXPAND_MENUNAMES
	    && cclp->cmdpos > 1
	    && cclp->cmdbuff[cclp->cmdpos - 1] == '.'
	    && cclp->cmdbuff[cclp->cmdpos - 2] != '\\'
	    && (c == '\n' || c == '\r' || c == K_KENTER))
	c = K_DOWN;

    return c;
}

/*
 * Delete characters on the command line, from "from" to the current
 * position.
 */
    static void
cmdline_del(cmdline_info_T *cclp, int from)
{
    mch_memmove(cclp->cmdbuff + from, cclp->cmdbuff + cclp->cmdpos,
	    (size_t)(cclp->cmdlen - cclp->cmdpos + 1));
    cclp->cmdlen -= cclp->cmdpos - from;
    cclp->cmdpos = from;
}

/*
 * Handle a key pressed when the wild menu for the menu names
 * (EXPAND_MENUNAMES) is displayed.
 */
    static int
wildmenu_process_key_menunames(cmdline_info_T *cclp, int key, expand_T *xp)
{
    int		i;
    int		j;

    // Hitting <Down> after "emenu Name.": complete submenu
    if (key == K_DOWN && cclp->cmdpos > 0
	    && cclp->cmdbuff[cclp->cmdpos - 1] == '.')
    {
	key = p_wc;
	KeyTyped = TRUE;  // in case the key was mapped
    }
    else if (key == K_UP)
    {
	// Hitting <Up>: Remove one submenu name in front of the
	// cursor
	int found = FALSE;

	j = (int)(xp->xp_pattern - cclp->cmdbuff);
	i = 0;
	while (--j > 0)
	{
	    // check for start of menu name
	    if (cclp->cmdbuff[j] == ' '
		    && cclp->cmdbuff[j - 1] != '\\')
	    {
		i = j + 1;
		break;
	    }
	    // check for start of submenu name
	    if (cclp->cmdbuff[j] == '.'
		    && cclp->cmdbuff[j - 1] != '\\')
	    {
		if (found)
		{
		    i = j + 1;
		    break;
		}
		else
		    found = TRUE;
	    }
	}
	if (i > 0)
	    cmdline_del(cclp, i);
	key = p_wc;
	KeyTyped = TRUE;  // in case the key was mapped
	xp->xp_context = EXPAND_NOTHING;
    }

    return key;
}

/*
 * Handle a key pressed when the wild menu for file names (EXPAND_FILES) or
 * directory names (EXPAND_DIRECTORIES) or shell command names
 * (EXPAND_SHELLCMD) is displayed.
 */
    static int
wildmenu_process_key_filenames(cmdline_info_T *cclp, int key, expand_T *xp)
{
    int		i;
    int		j;
    char_u	upseg[5];

    upseg[0] = PATHSEP;
    upseg[1] = '.';
    upseg[2] = '.';
    upseg[3] = PATHSEP;
    upseg[4] = NUL;

    if (key == K_DOWN
	    && cclp->cmdpos > 0
	    && cclp->cmdbuff[cclp->cmdpos - 1] == PATHSEP
	    && (cclp->cmdpos < 3
		|| cclp->cmdbuff[cclp->cmdpos - 2] != '.'
		|| cclp->cmdbuff[cclp->cmdpos - 3] != '.'))
    {
	// go down a directory
	key = p_wc;
	KeyTyped = TRUE;  // in case the key was mapped
    }
    else if (STRNCMP(xp->xp_pattern, upseg + 1, 3) == 0 && key == K_DOWN)
    {
	// If in a direct ancestor, strip off one ../ to go down
	int found = FALSE;

	j = cclp->cmdpos;
	i = (int)(xp->xp_pattern - cclp->cmdbuff);
	while (--j > i)
	{
	    if (has_mbyte)
		j -= (*mb_head_off)(cclp->cmdbuff, cclp->cmdbuff + j);
	    if (vim_ispathsep(cclp->cmdbuff[j]))
	    {
		found = TRUE;
		break;
	    }
	}
	if (found
		&& cclp->cmdbuff[j - 1] == '.'
		&& cclp->cmdbuff[j - 2] == '.'
		&& (vim_ispathsep(cclp->cmdbuff[j - 3]) || j == i + 2))
	{
	    cmdline_del(cclp, j - 2);
	    key = p_wc;
	    KeyTyped = TRUE;  // in case the key was mapped
	}
    }
    else if (key == K_UP)
    {
	// go up a directory
	int found = FALSE;

	j = cclp->cmdpos - 1;
	i = (int)(xp->xp_pattern - cclp->cmdbuff);
	while (--j > i)
	{
	    if (has_mbyte)
		j -= (*mb_head_off)(cclp->cmdbuff, cclp->cmdbuff + j);
	    if (vim_ispathsep(cclp->cmdbuff[j])
#ifdef BACKSLASH_IN_FILENAME
		    && vim_strchr((char_u *)" *?[{`$%#",
			cclp->cmdbuff[j + 1]) == NULL
#endif
	       )
	    {
		if (found)
		{
		    i = j + 1;
		    break;
		}
		else
		    found = TRUE;
	    }
	}

	if (!found)
	    j = i;
	else if (STRNCMP(cclp->cmdbuff + j, upseg, 4) == 0)
	    j += 4;
	else if (STRNCMP(cclp->cmdbuff + j, upseg + 1, 3) == 0
		&& j == i)
	    j += 3;
	else
	    j = 0;
	if (j > 0)
	{
	    // TODO this is only for DOS/UNIX systems - need to put in
	    // machine-specific stuff here and in upseg init
	    cmdline_del(cclp, j);
	    put_on_cmdline(upseg + 1, 3, FALSE);
	}
	else if (cclp->cmdpos > i)
	    cmdline_del(cclp, i);

	// Now complete in the new directory. Set KeyTyped in case the
	// Up key came from a mapping.
	key = p_wc;
	KeyTyped = TRUE;
    }

    return key;
}

/*
 * Handle a key pressed when the wild menu is displayed
 */
    int
wildmenu_process_key(cmdline_info_T *cclp, int key, expand_T *xp)
{
    if (xp->xp_context == EXPAND_MENUNAMES)
	return wildmenu_process_key_menunames(cclp, key, xp);
    else if ((xp->xp_context == EXPAND_FILES
		|| xp->xp_context == EXPAND_DIRECTORIES
		|| xp->xp_context == EXPAND_SHELLCMD))
	return wildmenu_process_key_filenames(cclp, key, xp);

    return key;
}

/*
 * Free expanded names when finished walking through the matches
 */
    void
wildmenu_cleanup(cmdline_info_T *cclp UNUSED)
{
    int skt = KeyTyped;

    if (!p_wmnu || wild_menu_showing == 0)
	return;

#ifdef FEAT_EVAL
    int save_RedrawingDisabled = RedrawingDisabled;
    if (cclp->input_fn)
	RedrawingDisabled = 0;
#endif

#if defined(FEAT_SEARCH_EXTRA)
    // Clear highlighting applied during wildmenu activity
    set_no_hlsearch(TRUE);
#endif

    if (wild_menu_showing == WM_SCROLLED)
    {
	// Entered command line, move it up
	cmdline_row--;
	redrawcmd();
    }
    else if (save_p_ls != -1)
    {
	// restore 'laststatus' and 'winminheight'
	p_ls = save_p_ls;
	p_wmh = save_p_wmh;
	last_status(FALSE);
	update_screen(UPD_VALID);	// redraw the screen NOW
	redrawcmd();
	save_p_ls = -1;
    }
    else
    {
	win_redraw_last_status(topframe);
	redraw_statuslines();
    }
    KeyTyped = skt;
    wild_menu_showing = 0;
#ifdef FEAT_EVAL
    if (cclp->input_fn)
	RedrawingDisabled = save_RedrawingDisabled;
#endif
}

#if defined(FEAT_EVAL)
/*
 * "getcompletion()" function
 */
    void
f_getcompletion(typval_T *argvars, typval_T *rettv)
{
    char_u	*pat;
    char_u	*type;
    expand_T	xpc;
    int		filtered = FALSE;
    int		options = WILD_SILENT | WILD_USE_NL | WILD_ADD_SLASH
					| WILD_NO_BEEP | WILD_HOME_REPLACE;

    if (in_vim9script()
	    && (check_for_string_arg(argvars, 0) == FAIL
		|| check_for_string_arg(argvars, 1) == FAIL
		|| check_for_opt_bool_arg(argvars, 2) == FAIL))
	return;

    pat = tv_get_string(&argvars[0]);
    if (check_for_string_arg(argvars, 1) == FAIL)
	return;
    type = tv_get_string(&argvars[1]);

    if (argvars[2].v_type != VAR_UNKNOWN)
	filtered = tv_get_bool_chk(&argvars[2], NULL);

    if (p_wic)
	options |= WILD_ICASE;

    // For filtered results, 'wildignore' is used
    if (!filtered)
	options |= WILD_KEEP_ALL;

    ExpandInit(&xpc);
    if (STRCMP(type, "cmdline") == 0)
    {
	int cmdline_len = (int)STRLEN(pat);
	set_cmd_context(&xpc, pat, cmdline_len, cmdline_len, FALSE);
	xpc.xp_pattern_len = (int)STRLEN(xpc.xp_pattern);
	xpc.xp_col = cmdline_len;
    }
    else
    {
	char_u	*pattern_start;

	xpc.xp_pattern = pattern_start = pat;
	xpc.xp_pattern_len = (int)STRLEN(xpc.xp_pattern);
	xpc.xp_line = pat;

	xpc.xp_context = cmdcomplete_str_to_type(type);
	switch (xpc.xp_context)
	{
	case EXPAND_NOTHING:
	    semsg(_(e_invalid_argument_str), type);
	    return;

	case EXPAND_USER_DEFINED:
	    // Must be "custom,funcname" pattern
	    if (STRNCMP(type, "custom,", 7) != 0)
	    {
		semsg(_(e_invalid_argument_str), type);
		return;
	    }

	    xpc.xp_arg = type + 7;
	    break;

	case EXPAND_USER_LIST:
	    // Must be "customlist,funcname" pattern
	    if (STRNCMP(type, "customlist,", 11) != 0)
	    {
		semsg(_(e_invalid_argument_str), type);
		return;
	    }

	    xpc.xp_arg = type + 11;
	    break;

# if defined(FEAT_MENU)
	case EXPAND_MENUS:
	    set_context_in_menu_cmd(&xpc, (char_u *)"menu", xpc.xp_pattern, FALSE);
	    xpc.xp_pattern_len -= (int)(xpc.xp_pattern - pattern_start);
	    break;
# endif

# ifdef FEAT_CSCOPE
	case EXPAND_CSCOPE:
	    set_context_in_cscope_cmd(&xpc, xpc.xp_pattern, CMD_cscope);
	    xpc.xp_pattern_len -= (int)(xpc.xp_pattern - pattern_start);
	    break;
# endif

# ifdef FEAT_SIGNS
	case EXPAND_SIGN:
	    set_context_in_sign_cmd(&xpc, xpc.xp_pattern);
	    xpc.xp_pattern_len -= (int)(xpc.xp_pattern - pattern_start);
	    break;
# endif

	case EXPAND_RUNTIME:
	    set_context_in_runtime_cmd(&xpc, xpc.xp_pattern);
	    xpc.xp_pattern_len -= (int)(xpc.xp_pattern - pattern_start);
	    break;

	case EXPAND_SHELLCMDLINE:
	{
	    int context = EXPAND_SHELLCMDLINE;
	    set_context_for_wildcard_arg(NULL, xpc.xp_pattern, FALSE, &xpc,
					 &context);
	    xpc.xp_pattern_len -= (int)(xpc.xp_pattern - pattern_start);
	    break;
	}

	case EXPAND_FILETYPECMD:
	    filetype_expand_what = EXP_FILETYPECMD_ALL;
	    break;

	default:
	    break;
	}
    }

    if (cmdline_fuzzy_completion_supported(&xpc))
	// when fuzzy matching, don't modify the search string
	pat = vim_strnsave(xpc.xp_pattern, xpc.xp_pattern_len);
    else
	pat = addstar(xpc.xp_pattern, xpc.xp_pattern_len, xpc.xp_context);

    if (rettv_list_alloc(rettv) == OK && pat != NULL)
    {
	int	i;

	ExpandOne(&xpc, pat, NULL, options, WILD_ALL_KEEP);

	for (i = 0; i < xpc.xp_numfiles; i++)
	    list_append_string(rettv->vval.v_list, xpc.xp_files[i], -1);
    }
    vim_free(pat);
    ExpandCleanup(&xpc);
}

/*
 * "getcompletiontype()" function
 */
    void
f_getcompletiontype(typval_T *argvars, typval_T *rettv)
{
    char_u	*pat;
    expand_T	xpc;
    int		cmdline_len;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    if (check_for_string_arg(argvars, 0) == FAIL)
	return;

    pat = tv_get_string(&argvars[0]);
    ExpandInit(&xpc);

    cmdline_len = (int)STRLEN(pat);
    set_cmd_context(&xpc, pat, cmdline_len, cmdline_len, FALSE);
    rettv->vval.v_string = cmdcomplete_type_to_str(xpc.xp_context, xpc.xp_arg);

    ExpandCleanup(&xpc);
}

/*
 * "cmdcomplete_info()" function
 */
    void
f_cmdcomplete_info(typval_T *argvars UNUSED, typval_T *rettv)
{
    cmdline_info_T  *ccline = get_cmdline_info();
    dict_T	    *retdict;
    list_T	    *li;
    int		    idx;
    int		    ret = OK;

    if (rettv_dict_alloc(rettv) == FAIL || ccline == NULL
	    || ccline->xpc == NULL || ccline->xpc->xp_files == NULL)
	return;
    retdict = rettv->vval.v_dict;
    ret = dict_add_string(retdict, "cmdline_orig", cmdline_orig);
    if (ret == OK)
	ret = dict_add_number(retdict, "pum_visible", pum_visible());
    if (ret == OK)
	ret = dict_add_number(retdict, "selected", ccline->xpc->xp_selected);
    if (ret == OK)
    {
	li = list_alloc();
	if (li == NULL)
	    return;
	ret = dict_add_list(retdict, "matches", li);
	for (idx = 0; ret == OK && idx < ccline->xpc->xp_numfiles; idx++)
	    list_append_string(li, ccline->xpc->xp_files[idx], -1);
    }
}
#endif // FEAT_EVAL

/*
 * Copy a substring from the current buffer (curbuf), spanning from the given
 * 'start' position to the word boundary after 'end' position.
 * The copied string is stored in '*match', and the actual end position of the
 * matched text is returned in '*match_end'.
 */
    static int
copy_substring_from_pos(pos_T *start, pos_T *end, char_u **match,
	pos_T *match_end)
{
    char_u	*word_end;
    char_u	*line, *start_line, *end_line;
    int		segment_len;
    linenr_T	lnum;
    garray_T	ga;
    int		exacttext = vim_strchr(p_wop, WOP_EXACTTEXT) != NULL;

    if (start->lnum > end->lnum
	    || (start->lnum == end->lnum && start->col >= end->col))
	return FAIL; // invalid range

    // Use a growable string (ga)
    ga_init2(&ga, 1, 128);

    // Append start line from start->col to end
    start_line = ml_get(start->lnum);
    char_u  *start_ptr = start_line + start->col;
    int	    is_single_line = start->lnum == end->lnum;

    segment_len = is_single_line ? (end->col - start->col)
			: (int)(ml_get_len(start->lnum) - start->col);
    if (ga_grow(&ga, segment_len + 2) != OK)
	return FAIL;

    ga_concat_len(&ga, start_ptr, segment_len);
    if (!is_single_line)
    {
	if (exacttext)
	    ga_concat_len(&ga, (char_u *)"\\n", 2);
	else
	    ga_append(&ga, '\n');
    }

    // Append full lines between start and end
    if (!is_single_line)
    {
	for (lnum = start->lnum + 1; lnum < end->lnum; lnum++)
	{
	    line = ml_get(lnum);
	    if (ga_grow(&ga, ml_get_len(lnum) + 2) != OK)
		return FAIL;
	    ga_concat(&ga, line);
	    if (exacttext)
		ga_concat_len(&ga, (char_u *)"\\n", 2);
	    else
		ga_append(&ga, '\n');
	}
    }

    // Append partial end line (up to word end)
    end_line = ml_get(end->lnum);
    word_end = find_word_end(end_line + end->col);
    segment_len = (int)(word_end - end_line);
    if (ga_grow(&ga, segment_len) != OK)
	return FAIL;
    ga_concat_len(&ga, end_line + (is_single_line ? end->col : 0),
	    segment_len - (is_single_line ? end->col : 0));

    // Null-terminate
    if (ga_grow(&ga, 1) != OK)
	return FAIL;
    ga_append(&ga, NUL);

    *match = (char_u *)ga.ga_data;
    match_end->lnum = end->lnum;
    match_end->col = segment_len;

    return OK;
}

/*
 * Returns TRUE if the given string `str` matches the regex pattern `pat`.
 * Honors the 'ignorecase' (p_ic) and 'smartcase' (p_scs) settings to determine
 * case sensitivity.
 */
    static int
is_regex_match(char_u *pat, char_u *str)
{
    if (STRCMP(pat, str) == 0)
	return TRUE;

    regmatch_T	regmatch;
    int		result;

    ++emsg_off;
    ++msg_silent;
    regmatch.regprog = vim_regcomp(pat, RE_MAGIC + RE_STRING);
    --emsg_off;
    --msg_silent;

    if (regmatch.regprog == NULL)
	return FALSE;
    regmatch.rm_ic = p_ic;
    if (p_ic && p_scs)
	regmatch.rm_ic = !pat_has_uppercase(pat);

    ++emsg_off;
    ++msg_silent;
    result = vim_regexec_nl(&regmatch, str, (colnr_T)0);
    --emsg_off;
    --msg_silent;

    vim_regfree(regmatch.regprog);
    return result;
}

/*
 * Constructs a new match string by appending text from the buffer (starting at
 * end_match_pos) to the given pattern `pat`. The result is a concatenation of
 * `pat` and the word following end_match_pos.
 * If 'lowercase' is TRUE, the appended text is converted to lowercase before
 * being combined. Returns the newly allocated match string, or NULL on failure.
 */
    static char_u *
concat_pattern_with_buffer_match(
	char_u *pat,
	int pat_len,
	pos_T *end_match_pos,
	int lowercase UNUSED)
{
    char_u  *line = ml_get(end_match_pos->lnum);
    char_u  *word_end = find_word_end(line + end_match_pos->col);
    int	    match_len = (int)(word_end - (line + end_match_pos->col));
    char_u  *match = alloc(match_len + pat_len + 1);  // +1 for NUL

    if (match == NULL)
	return NULL;
    mch_memmove(match, pat, pat_len);
    if (match_len > 0)
    {
#if defined(FEAT_EVAL) || defined(FEAT_SPELL)
	if (lowercase)
	{
	    char_u  *mword = vim_strnsave(line + end_match_pos->col,
		    match_len);
	    if (mword == NULL)
		goto cleanup;
	    char_u  *lower = strlow_save(mword);
	    vim_free(mword);
	    if (lower == NULL)
		goto cleanup;
	    mch_memmove(match + pat_len, lower, match_len);
	    vim_free(lower);
	}
	else
#endif
	    mch_memmove(match + pat_len, line + end_match_pos->col, match_len);
    }
    match[pat_len + match_len] = NUL;
    return match;

#if defined(FEAT_EVAL) || defined(FEAT_SPELL)
cleanup:
    vim_free(match);
    return NULL;
#endif
}

/*
 * Search for strings matching "pat" in the specified range and return them.
 * Returns OK on success, FAIL otherwise.
 */
    static int
expand_pattern_in_buf(
    char_u	*pat,		    // pattern to match
    int		dir,		    // direction: FORWARD or BACKWARD
    char_u	***matches,	    // return: array with matched strings
    int		*numMatches)	    // return: number of matches
{
    pos_T	cur_match_pos, prev_match_pos, end_match_pos, word_end_pos;
    garray_T	ga;
    int		found_new_match;
    int		looped_around = FALSE;
    int		pat_len;
    int		has_range = FALSE;
    int		compl_started = FALSE;
    int		search_flags;
    char_u	*match, *full_match;
    int		exacttext = vim_strchr(p_wop, WOP_EXACTTEXT) != NULL;

#ifdef FEAT_SEARCH_EXTRA
    has_range = search_first_line != 0;
#endif

    *matches = NULL;
    *numMatches = 0;

    if (pat == NULL || *pat == NUL)
	return FAIL;

    pat_len = (int)STRLEN(pat);
    CLEAR_FIELD(cur_match_pos);
    CLEAR_FIELD(prev_match_pos);
#ifdef FEAT_SEARCH_EXTRA
    if (has_range)
	cur_match_pos.lnum = search_first_line;
    else
#endif
	cur_match_pos = pre_incsearch_pos;

    search_flags = SEARCH_OPT | SEARCH_NOOF | SEARCH_PEEK | SEARCH_NFMSG
	| (has_range ? SEARCH_START : 0);

    ga_init2(&ga, sizeof(char_u *), 10); // Use growable array of char_u*

    for (;;)
    {
	++emsg_off;
	++msg_silent;
	found_new_match = searchit(NULL, curbuf, &cur_match_pos,
		&end_match_pos, dir, pat, pat_len, 1L,
		search_flags, RE_LAST, NULL);
	--msg_silent;
	--emsg_off;

	if (found_new_match == FAIL)
	    break;

#ifdef FEAT_SEARCH_EXTRA
	// If in range mode, check if match is within the range
	if (has_range && (cur_match_pos.lnum < search_first_line
		    || cur_match_pos.lnum > search_last_line))
		break;
#endif

	if (compl_started)
	{
	    // If we've looped back to an earlier match, stop
	    if ((dir == FORWARD && LTOREQ_POS(cur_match_pos, prev_match_pos)) ||
		(dir == BACKWARD && LTOREQ_POS(prev_match_pos, cur_match_pos)))
	    {
		if (looped_around)
		    break;
		else
		    looped_around = TRUE;
	    }
	}

	compl_started = TRUE;
	prev_match_pos = cur_match_pos;

	// Abort if user typed a character or interrupted
	if (char_avail() || got_int)
	{
	    if (got_int)
	    {
		(void)vpeekc();  // Remove <C-C> from input stream
		got_int = FALSE; // Don't abandon the command line
	    }
	    goto cleanup;
	}

	// searchit() can return line number +1 past the last line when
	// searching for "foo\n" if "foo" is at end of buffer.
	if (end_match_pos.lnum > curbuf->b_ml.ml_line_count)
	{
	    cur_match_pos.lnum = 1;
	    cur_match_pos.col = 0;
	    cur_match_pos.coladd = 0;
	    continue;
	}

	// Extract the matching text prepended to completed word
	if (!copy_substring_from_pos(&cur_match_pos, &end_match_pos, &full_match,
		    &word_end_pos))
	    break;

	if (exacttext)
	    match = full_match;
	else
	{
	    // Construct a new match from completed word appended to pattern itself
	    match = concat_pattern_with_buffer_match(pat, pat_len, &end_match_pos,
		    FALSE);

	    // The regex pattern may include '\C' or '\c'. First, try matching the
	    // buffer word as-is. If it doesn't match, try again with the lowercase
	    // version of the word to handle smartcase behavior.
	    if (match == NULL || !is_regex_match(match, full_match))
	    {
		vim_free(match);
		match = concat_pattern_with_buffer_match(pat, pat_len,
			&end_match_pos, TRUE);
		if (match == NULL || !is_regex_match(match, full_match))
		{
		    vim_free(match);
		    vim_free(full_match);
		    continue;
		}
	    }
	    vim_free(full_match);
	}

	// Include this match if it is not a duplicate
	for (int i = 0; i < ga.ga_len; ++i)
	{
	    if (STRCMP(match, ((char_u **)ga.ga_data)[i]) == 0)
	    {
		VIM_CLEAR(match);
		break;
	    }
	}
	if (match != NULL)
	{
	    if (ga_grow(&ga, 1) == FAIL)
		goto cleanup;
	    ((char_u **)ga.ga_data)[ga.ga_len++] = match;
	    if (ga.ga_len > TAG_MANY)
		break;
	}
	if (has_range)
	    cur_match_pos = word_end_pos;
    }

    *matches = (char_u **)ga.ga_data;
    *numMatches = ga.ga_len;
    return OK;

cleanup:
    ga_clear_strings(&ga);
    return FAIL;
}
