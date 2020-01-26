/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * insexpand.c: functions for Insert mode completion
 */

#include "vim.h"

/*
 * Definitions used for CTRL-X submode.
 * Note: If you change CTRL-X submode, you must also maintain ctrl_x_msgs[] and
 * ctrl_x_mode_names[] below.
 */
# define CTRL_X_WANT_IDENT	0x100

# define CTRL_X_NORMAL		0  // CTRL-N CTRL-P completion, default
# define CTRL_X_NOT_DEFINED_YET	1
# define CTRL_X_SCROLL		2
# define CTRL_X_WHOLE_LINE	3
# define CTRL_X_FILES		4
# define CTRL_X_TAGS		(5 + CTRL_X_WANT_IDENT)
# define CTRL_X_PATH_PATTERNS	(6 + CTRL_X_WANT_IDENT)
# define CTRL_X_PATH_DEFINES	(7 + CTRL_X_WANT_IDENT)
# define CTRL_X_FINISHED		8
# define CTRL_X_DICTIONARY	(9 + CTRL_X_WANT_IDENT)
# define CTRL_X_THESAURUS	(10 + CTRL_X_WANT_IDENT)
# define CTRL_X_CMDLINE		11
# define CTRL_X_FUNCTION	12
# define CTRL_X_OMNI		13
# define CTRL_X_SPELL		14
# define CTRL_X_LOCAL_MSG	15	// only used in "ctrl_x_msgs"
# define CTRL_X_EVAL		16	// for builtin function complete()

# define CTRL_X_MSG(i) ctrl_x_msgs[(i) & ~CTRL_X_WANT_IDENT]

// Message for CTRL-X mode, index is ctrl_x_mode.
static char *ctrl_x_msgs[] =
{
    N_(" Keyword completion (^N^P)"), // CTRL_X_NORMAL, ^P/^N compl.
    N_(" ^X mode (^]^D^E^F^I^K^L^N^O^Ps^U^V^Y)"),
    NULL, // CTRL_X_SCROLL: depends on state
    N_(" Whole line completion (^L^N^P)"),
    N_(" File name completion (^F^N^P)"),
    N_(" Tag completion (^]^N^P)"),
    N_(" Path pattern completion (^N^P)"),
    N_(" Definition completion (^D^N^P)"),
    NULL, // CTRL_X_FINISHED
    N_(" Dictionary completion (^K^N^P)"),
    N_(" Thesaurus completion (^T^N^P)"),
    N_(" Command-line completion (^V^N^P)"),
    N_(" User defined completion (^U^N^P)"),
    N_(" Omni completion (^O^N^P)"),
    N_(" Spelling suggestion (s^N^P)"),
    N_(" Keyword Local completion (^N^P)"),
    NULL,   // CTRL_X_EVAL doesn't use msg.
};

#if defined(FEAT_COMPL_FUNC) || defined(FEAT_EVAL)
static char *ctrl_x_mode_names[] = {
	"keyword",
	"ctrl_x",
	"unknown",	    // CTRL_X_SCROLL
	"whole_line",
	"files",
	"tags",
	"path_patterns",
	"path_defines",
	"unknown",	    // CTRL_X_FINISHED
	"dictionary",
	"thesaurus",
	"cmdline",
	"function",
	"omni",
	"spell",
	NULL,		    // CTRL_X_LOCAL_MSG only used in "ctrl_x_msgs"
	"eval"
};
#endif

/*
 * Array indexes used for cp_text[].
 */
#define CPT_ABBR	0	// "abbr"
#define CPT_MENU	1	// "menu"
#define CPT_KIND	2	// "kind"
#define CPT_INFO	3	// "info"
#define CPT_COUNT	4	// Number of entries

/*
 * Structure used to store one match for insert completion.
 */
typedef struct compl_S compl_T;
struct compl_S
{
    compl_T	*cp_next;
    compl_T	*cp_prev;
    char_u	*cp_str;	// matched text
    char_u	*(cp_text[CPT_COUNT]);	// text for the menu
#ifdef FEAT_EVAL
    typval_T	cp_user_data;
#endif
    char_u	*cp_fname;	// file containing the match, allocated when
				// cp_flags has CP_FREE_FNAME
    int		cp_flags;	// CP_ values
    int		cp_number;	// sequence number
};

// values for cp_flags
# define CP_ORIGINAL_TEXT   1	// the original text when the expansion begun
# define CP_FREE_FNAME	    2	// cp_fname is allocated
# define CP_CONT_S_IPOS	    4	// use CONT_S_IPOS for compl_cont_status
# define CP_EQUAL	    8	// ins_compl_equal() always returns TRUE
# define CP_ICASE	    16	// ins_compl_equal() ignores case

static char e_hitend[] = N_("Hit end of paragraph");
# ifdef FEAT_COMPL_FUNC
static char e_complwin[] = N_("E839: Completion function changed window");
static char e_compldel[] = N_("E840: Completion function deleted text");
# endif

/*
 * All the current matches are stored in a list.
 * "compl_first_match" points to the start of the list.
 * "compl_curr_match" points to the currently selected entry.
 * "compl_shown_match" is different from compl_curr_match during
 * ins_compl_get_exp().
 */
static compl_T    *compl_first_match = NULL;
static compl_T    *compl_curr_match = NULL;
static compl_T    *compl_shown_match = NULL;
static compl_T    *compl_old_match = NULL;

// After using a cursor key <Enter> selects a match in the popup menu,
// otherwise it inserts a line break.
static int	  compl_enter_selects = FALSE;

// When "compl_leader" is not NULL only matches that start with this string
// are used.
static char_u	  *compl_leader = NULL;

static int	  compl_get_longest = FALSE;	// put longest common string
						// in compl_leader

static int	  compl_no_insert = FALSE;	// FALSE: select & insert
						// TRUE: noinsert
static int	  compl_no_select = FALSE;	// FALSE: select & insert
						// TRUE: noselect

// Selected one of the matches.  When FALSE the match was edited or using the
// longest common string.
static int	  compl_used_match;

// didn't finish finding completions.
static int	  compl_was_interrupted = FALSE;

// Set when character typed while looking for matches and it means we should
// stop looking for matches.
static int	  compl_interrupted = FALSE;

static int	  compl_restarting = FALSE;	// don't insert match

// When the first completion is done "compl_started" is set.  When it's
// FALSE the word to be completed must be located.
static int	  compl_started = FALSE;

// Which Ctrl-X mode are we in?
static int	  ctrl_x_mode = CTRL_X_NORMAL;

static int	  compl_matches = 0;
static char_u	  *compl_pattern = NULL;
static int	  compl_direction = FORWARD;
static int	  compl_shows_dir = FORWARD;
static int	  compl_pending = 0;	    // > 1 for postponed CTRL-N
static pos_T	  compl_startpos;
static colnr_T	  compl_col = 0;	    // column where the text starts
					    // that is being completed
static char_u	  *compl_orig_text = NULL;  // text as it was before
					    // completion started
static int	  compl_cont_mode = 0;
static expand_T	  compl_xp;

static int	  compl_opt_refresh_always = FALSE;
static int	  compl_opt_suppress_empty = FALSE;

static int ins_compl_add(char_u *str, int len, char_u *fname, char_u **cptext, typval_T *user_data, int cdir, int flags, int adup);
static void ins_compl_longest_match(compl_T *match);
static void ins_compl_del_pum(void);
static void ins_compl_files(int count, char_u **files, int thesaurus, int flags, regmatch_T *regmatch, char_u *buf, int *dir);
static char_u *find_line_end(char_u *ptr);
static void ins_compl_free(void);
static int  ins_compl_need_restart(void);
static void ins_compl_new_leader(void);
static int  ins_compl_len(void);
static void ins_compl_restart(void);
static void ins_compl_set_original_text(char_u *str);
static void ins_compl_fixRedoBufForLeader(char_u *ptr_arg);
# if defined(FEAT_COMPL_FUNC) || defined(FEAT_EVAL)
static void ins_compl_add_list(list_T *list);
static void ins_compl_add_dict(dict_T *dict);
# endif
static int  ins_compl_key2dir(int c);
static int  ins_compl_pum_key(int c);
static int  ins_compl_key2count(int c);
static void show_pum(int prev_w_wrow, int prev_w_leftcol);
static unsigned  quote_meta(char_u *dest, char_u *str, int len);

#ifdef FEAT_SPELL
static void spell_back_to_badword(void);
static int  spell_bad_len = 0;	// length of located bad word
#endif

/*
 * CTRL-X pressed in Insert mode.
 */
    void
ins_ctrl_x(void)
{
    // CTRL-X after CTRL-X CTRL-V doesn't do anything, so that CTRL-X
    // CTRL-V works like CTRL-N
    if (ctrl_x_mode != CTRL_X_CMDLINE)
    {
	// if the next ^X<> won't ADD nothing, then reset
	// compl_cont_status
	if (compl_cont_status & CONT_N_ADDS)
	    compl_cont_status |= CONT_INTRPT;
	else
	    compl_cont_status = 0;
	// We're not sure which CTRL-X mode it will be yet
	ctrl_x_mode = CTRL_X_NOT_DEFINED_YET;
	edit_submode = (char_u *)_(CTRL_X_MSG(ctrl_x_mode));
	edit_submode_pre = NULL;
	showmode();
    }
}

/*
 * Functions to check the current CTRL-X mode.
 */
int ctrl_x_mode_none(void) { return ctrl_x_mode == 0; }
int ctrl_x_mode_normal(void) { return ctrl_x_mode == CTRL_X_NORMAL; }
int ctrl_x_mode_scroll(void) { return ctrl_x_mode == CTRL_X_SCROLL; }
int ctrl_x_mode_whole_line(void) { return ctrl_x_mode == CTRL_X_WHOLE_LINE; }
int ctrl_x_mode_files(void) { return ctrl_x_mode == CTRL_X_FILES; }
int ctrl_x_mode_tags(void) { return ctrl_x_mode == CTRL_X_TAGS; }
int ctrl_x_mode_path_patterns(void) {
				  return ctrl_x_mode == CTRL_X_PATH_PATTERNS; }
int ctrl_x_mode_path_defines(void) {
				   return ctrl_x_mode == CTRL_X_PATH_DEFINES; }
int ctrl_x_mode_dictionary(void) { return ctrl_x_mode == CTRL_X_DICTIONARY; }
int ctrl_x_mode_thesaurus(void) { return ctrl_x_mode == CTRL_X_THESAURUS; }
int ctrl_x_mode_cmdline(void) { return ctrl_x_mode == CTRL_X_CMDLINE; }
int ctrl_x_mode_function(void) { return ctrl_x_mode == CTRL_X_FUNCTION; }
int ctrl_x_mode_omni(void) { return ctrl_x_mode == CTRL_X_OMNI; }
int ctrl_x_mode_spell(void) { return ctrl_x_mode == CTRL_X_SPELL; }
int ctrl_x_mode_line_or_eval(void) {
       return ctrl_x_mode == CTRL_X_WHOLE_LINE || ctrl_x_mode == CTRL_X_EVAL; }

/*
 * Whether other than default completion has been selected.
 */
    int
ctrl_x_mode_not_default(void)
{
    return ctrl_x_mode != CTRL_X_NORMAL;
}

/*
 * Whether CTRL-X was typed without a following character.
 */
    int
ctrl_x_mode_not_defined_yet(void)
{
    return ctrl_x_mode == CTRL_X_NOT_DEFINED_YET;
}

/*
 * Return TRUE if the 'dict' or 'tsr' option can be used.
 */
    int
has_compl_option(int dict_opt)
{
    if (dict_opt ? (*curbuf->b_p_dict == NUL && *p_dict == NUL
#ifdef FEAT_SPELL
							&& !curwin->w_p_spell
#endif
							)
		 : (*curbuf->b_p_tsr == NUL && *p_tsr == NUL))
    {
	ctrl_x_mode = CTRL_X_NORMAL;
	edit_submode = NULL;
	msg_attr(dict_opt ? _("'dictionary' option is empty")
			  : _("'thesaurus' option is empty"),
							      HL_ATTR(HLF_E));
	if (emsg_silent == 0)
	{
	    vim_beep(BO_COMPL);
	    setcursor();
	    out_flush();
#ifdef FEAT_EVAL
	    if (!get_vim_var_nr(VV_TESTING))
#endif
		ui_delay(2004L, FALSE);
	}
	return FALSE;
    }
    return TRUE;
}

/*
 * Is the character 'c' a valid key to go to or keep us in CTRL-X mode?
 * This depends on the current mode.
 */
    int
vim_is_ctrl_x_key(int c)
{
    // Always allow ^R - let its results then be checked
    if (c == Ctrl_R)
	return TRUE;

    // Accept <PageUp> and <PageDown> if the popup menu is visible.
    if (ins_compl_pum_key(c))
	return TRUE;

    switch (ctrl_x_mode)
    {
	case 0:		    // Not in any CTRL-X mode
	    return (c == Ctrl_N || c == Ctrl_P || c == Ctrl_X);
	case CTRL_X_NOT_DEFINED_YET:
	    return (   c == Ctrl_X || c == Ctrl_Y || c == Ctrl_E
		    || c == Ctrl_L || c == Ctrl_F || c == Ctrl_RSB
		    || c == Ctrl_I || c == Ctrl_D || c == Ctrl_P
		    || c == Ctrl_N || c == Ctrl_T || c == Ctrl_V
		    || c == Ctrl_Q || c == Ctrl_U || c == Ctrl_O
		    || c == Ctrl_S || c == Ctrl_K || c == 's');
	case CTRL_X_SCROLL:
	    return (c == Ctrl_Y || c == Ctrl_E);
	case CTRL_X_WHOLE_LINE:
	    return (c == Ctrl_L || c == Ctrl_P || c == Ctrl_N);
	case CTRL_X_FILES:
	    return (c == Ctrl_F || c == Ctrl_P || c == Ctrl_N);
	case CTRL_X_DICTIONARY:
	    return (c == Ctrl_K || c == Ctrl_P || c == Ctrl_N);
	case CTRL_X_THESAURUS:
	    return (c == Ctrl_T || c == Ctrl_P || c == Ctrl_N);
	case CTRL_X_TAGS:
	    return (c == Ctrl_RSB || c == Ctrl_P || c == Ctrl_N);
#ifdef FEAT_FIND_ID
	case CTRL_X_PATH_PATTERNS:
	    return (c == Ctrl_P || c == Ctrl_N);
	case CTRL_X_PATH_DEFINES:
	    return (c == Ctrl_D || c == Ctrl_P || c == Ctrl_N);
#endif
	case CTRL_X_CMDLINE:
	    return (c == Ctrl_V || c == Ctrl_Q || c == Ctrl_P || c == Ctrl_N
		    || c == Ctrl_X);
#ifdef FEAT_COMPL_FUNC
	case CTRL_X_FUNCTION:
	    return (c == Ctrl_U || c == Ctrl_P || c == Ctrl_N);
	case CTRL_X_OMNI:
	    return (c == Ctrl_O || c == Ctrl_P || c == Ctrl_N);
#endif
	case CTRL_X_SPELL:
	    return (c == Ctrl_S || c == Ctrl_P || c == Ctrl_N);
	case CTRL_X_EVAL:
	    return (c == Ctrl_P || c == Ctrl_N);
    }
    internal_error("vim_is_ctrl_x_key()");
    return FALSE;
}

/*
 * Return TRUE when character "c" is part of the item currently being
 * completed.  Used to decide whether to abandon complete mode when the menu
 * is visible.
 */
    int
ins_compl_accept_char(int c)
{
    if (ctrl_x_mode & CTRL_X_WANT_IDENT)
	// When expanding an identifier only accept identifier chars.
	return vim_isIDc(c);

    switch (ctrl_x_mode)
    {
	case CTRL_X_FILES:
	    // When expanding file name only accept file name chars. But not
	    // path separators, so that "proto/<Tab>" expands files in
	    // "proto", not "proto/" as a whole
	    return vim_isfilec(c) && !vim_ispathsep(c);

	case CTRL_X_CMDLINE:
	case CTRL_X_OMNI:
	    // Command line and Omni completion can work with just about any
	    // printable character, but do stop at white space.
	    return vim_isprintc(c) && !VIM_ISWHITE(c);

	case CTRL_X_WHOLE_LINE:
	    // For while line completion a space can be part of the line.
	    return vim_isprintc(c);
    }
    return vim_iswordc(c);
}

/*
 * This is like ins_compl_add(), but if 'ic' and 'inf' are set, then the
 * case of the originally typed text is used, and the case of the completed
 * text is inferred, ie this tries to work out what case you probably wanted
 * the rest of the word to be in -- webb
 */
    int
ins_compl_add_infercase(
    char_u	*str_arg,
    int		len,
    int		icase,
    char_u	*fname,
    int		dir,
    int		cont_s_ipos)  // next ^X<> will set initial_pos
{
    char_u	*str = str_arg;
    char_u	*p;
    int		i, c;
    int		actual_len;		// Take multi-byte characters
    int		actual_compl_length;	// into account.
    int		min_len;
    int		*wca;			// Wide character array.
    int		has_lower = FALSE;
    int		was_letter = FALSE;
    int		flags = 0;

    if (p_ic && curbuf->b_p_inf && len > 0)
    {
	// Infer case of completed part.

	// Find actual length of completion.
	if (has_mbyte)
	{
	    p = str;
	    actual_len = 0;
	    while (*p != NUL)
	    {
		MB_PTR_ADV(p);
		++actual_len;
	    }
	}
	else
	    actual_len = len;

	// Find actual length of original text.
	if (has_mbyte)
	{
	    p = compl_orig_text;
	    actual_compl_length = 0;
	    while (*p != NUL)
	    {
		MB_PTR_ADV(p);
		++actual_compl_length;
	    }
	}
	else
	    actual_compl_length = compl_length;

	// "actual_len" may be smaller than "actual_compl_length" when using
	// thesaurus, only use the minimum when comparing.
	min_len = actual_len < actual_compl_length
					   ? actual_len : actual_compl_length;

	// Allocate wide character array for the completion and fill it.
	wca = ALLOC_MULT(int, actual_len);
	if (wca != NULL)
	{
	    p = str;
	    for (i = 0; i < actual_len; ++i)
		if (has_mbyte)
		    wca[i] = mb_ptr2char_adv(&p);
		else
		    wca[i] = *(p++);

	    // Rule 1: Were any chars converted to lower?
	    p = compl_orig_text;
	    for (i = 0; i < min_len; ++i)
	    {
		if (has_mbyte)
		    c = mb_ptr2char_adv(&p);
		else
		    c = *(p++);
		if (MB_ISLOWER(c))
		{
		    has_lower = TRUE;
		    if (MB_ISUPPER(wca[i]))
		    {
			// Rule 1 is satisfied.
			for (i = actual_compl_length; i < actual_len; ++i)
			    wca[i] = MB_TOLOWER(wca[i]);
			break;
		    }
		}
	    }

	    // Rule 2: No lower case, 2nd consecutive letter converted to
	    // upper case.
	    if (!has_lower)
	    {
		p = compl_orig_text;
		for (i = 0; i < min_len; ++i)
		{
		    if (has_mbyte)
			c = mb_ptr2char_adv(&p);
		    else
			c = *(p++);
		    if (was_letter && MB_ISUPPER(c) && MB_ISLOWER(wca[i]))
		    {
			// Rule 2 is satisfied.
			for (i = actual_compl_length; i < actual_len; ++i)
			    wca[i] = MB_TOUPPER(wca[i]);
			break;
		    }
		    was_letter = MB_ISLOWER(c) || MB_ISUPPER(c);
		}
	    }

	    // Copy the original case of the part we typed.
	    p = compl_orig_text;
	    for (i = 0; i < min_len; ++i)
	    {
		if (has_mbyte)
		    c = mb_ptr2char_adv(&p);
		else
		    c = *(p++);
		if (MB_ISLOWER(c))
		    wca[i] = MB_TOLOWER(wca[i]);
		else if (MB_ISUPPER(c))
		    wca[i] = MB_TOUPPER(wca[i]);
	    }

	    // Generate encoding specific output from wide character array.
	    // Multi-byte characters can occupy up to five bytes more than
	    // ASCII characters, and we also need one byte for NUL, so stay
	    // six bytes away from the edge of IObuff.
	    p = IObuff;
	    i = 0;
	    while (i < actual_len && (p - IObuff + 6) < IOSIZE)
		if (has_mbyte)
		    p += (*mb_char2bytes)(wca[i++], p);
		else
		    *(p++) = wca[i++];
	    *p = NUL;

	    vim_free(wca);
	}

	str = IObuff;
    }
    if (cont_s_ipos)
	flags |= CP_CONT_S_IPOS;
    if (icase)
	flags |= CP_ICASE;

    return ins_compl_add(str, len, fname, NULL, NULL, dir, flags, FALSE);
}

/*
 * Add a match to the list of matches.
 * If the given string is already in the list of completions, then return
 * NOTDONE, otherwise add it to the list and return OK.  If there is an error,
 * maybe because alloc() returns NULL, then FAIL is returned.
 */
    static int
ins_compl_add(
    char_u	*str,
    int		len,
    char_u	*fname,
    char_u	**cptext,	    // extra text for popup menu or NULL
    typval_T	*user_data UNUSED,  // "user_data" entry or NULL
    int		cdir,
    int		flags_arg,
    int		adup)		// accept duplicate match
{
    compl_T	*match;
    int		dir = (cdir == 0 ? compl_direction : cdir);
    int		flags = flags_arg;

    ui_breakcheck();
    if (got_int)
	return FAIL;
    if (len < 0)
	len = (int)STRLEN(str);

    // If the same match is already present, don't add it.
    if (compl_first_match != NULL && !adup)
    {
	match = compl_first_match;
	do
	{
	    if (    !(match->cp_flags & CP_ORIGINAL_TEXT)
		    && STRNCMP(match->cp_str, str, len) == 0
		    && match->cp_str[len] == NUL)
		return NOTDONE;
	    match = match->cp_next;
	} while (match != NULL && match != compl_first_match);
    }

    // Remove any popup menu before changing the list of matches.
    ins_compl_del_pum();

    // Allocate a new match structure.
    // Copy the values to the new match structure.
    match = ALLOC_CLEAR_ONE(compl_T);
    if (match == NULL)
	return FAIL;
    match->cp_number = -1;
    if (flags & CP_ORIGINAL_TEXT)
	match->cp_number = 0;
    if ((match->cp_str = vim_strnsave(str, len)) == NULL)
    {
	vim_free(match);
	return FAIL;
    }

    // match-fname is:
    // - compl_curr_match->cp_fname if it is a string equal to fname.
    // - a copy of fname, CP_FREE_FNAME is set to free later THE allocated mem.
    // - NULL otherwise.	--Acevedo
    if (fname != NULL
	    && compl_curr_match != NULL
	    && compl_curr_match->cp_fname != NULL
	    && STRCMP(fname, compl_curr_match->cp_fname) == 0)
	match->cp_fname = compl_curr_match->cp_fname;
    else if (fname != NULL)
    {
	match->cp_fname = vim_strsave(fname);
	flags |= CP_FREE_FNAME;
    }
    else
	match->cp_fname = NULL;
    match->cp_flags = flags;

    if (cptext != NULL)
    {
	int i;

	for (i = 0; i < CPT_COUNT; ++i)
	    if (cptext[i] != NULL && *cptext[i] != NUL)
		match->cp_text[i] = vim_strsave(cptext[i]);
    }
#ifdef FEAT_EVAL
    if (user_data != NULL)
	match->cp_user_data = *user_data;
#endif

    // Link the new match structure in the list of matches.
    if (compl_first_match == NULL)
	match->cp_next = match->cp_prev = NULL;
    else if (dir == FORWARD)
    {
	match->cp_next = compl_curr_match->cp_next;
	match->cp_prev = compl_curr_match;
    }
    else	// BACKWARD
    {
	match->cp_next = compl_curr_match;
	match->cp_prev = compl_curr_match->cp_prev;
    }
    if (match->cp_next)
	match->cp_next->cp_prev = match;
    if (match->cp_prev)
	match->cp_prev->cp_next = match;
    else	// if there's nothing before, it is the first match
	compl_first_match = match;
    compl_curr_match = match;

    // Find the longest common string if still doing that.
    if (compl_get_longest && (flags & CP_ORIGINAL_TEXT) == 0)
	ins_compl_longest_match(match);

    return OK;
}

/*
 * Return TRUE if "str[len]" matches with match->cp_str, considering
 * match->cp_flags.
 */
    static int
ins_compl_equal(compl_T *match, char_u *str, int len)
{
    if (match->cp_flags & CP_EQUAL)
	return TRUE;
    if (match->cp_flags & CP_ICASE)
	return STRNICMP(match->cp_str, str, (size_t)len) == 0;
    return STRNCMP(match->cp_str, str, (size_t)len) == 0;
}

/*
 * Reduce the longest common string for match "match".
 */
    static void
ins_compl_longest_match(compl_T *match)
{
    char_u	*p, *s;
    int		c1, c2;
    int		had_match;

    if (compl_leader == NULL)
    {
	// First match, use it as a whole.
	compl_leader = vim_strsave(match->cp_str);
	if (compl_leader != NULL)
	{
	    had_match = (curwin->w_cursor.col > compl_col);
	    ins_compl_delete();
	    ins_bytes(compl_leader + ins_compl_len());
	    ins_redraw(FALSE);

	    // When the match isn't there (to avoid matching itself) remove it
	    // again after redrawing.
	    if (!had_match)
		ins_compl_delete();
	    compl_used_match = FALSE;
	}
    }
    else
    {
	// Reduce the text if this match differs from compl_leader.
	p = compl_leader;
	s = match->cp_str;
	while (*p != NUL)
	{
	    if (has_mbyte)
	    {
		c1 = mb_ptr2char(p);
		c2 = mb_ptr2char(s);
	    }
	    else
	    {
		c1 = *p;
		c2 = *s;
	    }
	    if ((match->cp_flags & CP_ICASE)
			     ? (MB_TOLOWER(c1) != MB_TOLOWER(c2)) : (c1 != c2))
		break;
	    if (has_mbyte)
	    {
		MB_PTR_ADV(p);
		MB_PTR_ADV(s);
	    }
	    else
	    {
		++p;
		++s;
	    }
	}

	if (*p != NUL)
	{
	    // Leader was shortened, need to change the inserted text.
	    *p = NUL;
	    had_match = (curwin->w_cursor.col > compl_col);
	    ins_compl_delete();
	    ins_bytes(compl_leader + ins_compl_len());
	    ins_redraw(FALSE);

	    // When the match isn't there (to avoid matching itself) remove it
	    // again after redrawing.
	    if (!had_match)
		ins_compl_delete();
	}

	compl_used_match = FALSE;
    }
}

/*
 * Add an array of matches to the list of matches.
 * Frees matches[].
 */
    static void
ins_compl_add_matches(
    int		num_matches,
    char_u	**matches,
    int		icase)
{
    int		i;
    int		add_r = OK;
    int		dir = compl_direction;

    for (i = 0; i < num_matches && add_r != FAIL; i++)
	if ((add_r = ins_compl_add(matches[i], -1, NULL, NULL, NULL, dir,
					   icase ? CP_ICASE : 0, FALSE)) == OK)
	    // if dir was BACKWARD then honor it just once
	    dir = FORWARD;
    FreeWild(num_matches, matches);
}

/*
 * Make the completion list cyclic.
 * Return the number of matches (excluding the original).
 */
    static int
ins_compl_make_cyclic(void)
{
    compl_T *match;
    int	    count = 0;

    if (compl_first_match != NULL)
    {
	// Find the end of the list.
	match = compl_first_match;
	// there's always an entry for the compl_orig_text, it doesn't count.
	while (match->cp_next != NULL && match->cp_next != compl_first_match)
	{
	    match = match->cp_next;
	    ++count;
	}
	match->cp_next = compl_first_match;
	compl_first_match->cp_prev = match;
    }
    return count;
}

/*
 * Return whether there currently is a shown match.
 */
    int
ins_compl_has_shown_match(void)
{
    return compl_shown_match == NULL
	|| compl_shown_match != compl_shown_match->cp_next;
}

/*
 * Return whether the shown match is long enough.
 */
    int
ins_compl_long_shown_match(void)
{
    return (int)STRLEN(compl_shown_match->cp_str)
					    > curwin->w_cursor.col - compl_col;
}

/*
 * Set variables that store noselect and noinsert behavior from the
 * 'completeopt' value.
 */
    void
completeopt_was_set(void)
{
    compl_no_insert = FALSE;
    compl_no_select = FALSE;
    if (strstr((char *)p_cot, "noselect") != NULL)
	compl_no_select = TRUE;
    if (strstr((char *)p_cot, "noinsert") != NULL)
	compl_no_insert = TRUE;
}


// "compl_match_array" points the currently displayed list of entries in the
// popup menu.  It is NULL when there is no popup menu.
static pumitem_T *compl_match_array = NULL;
static int compl_match_arraysize;

/*
 * Update the screen and when there is any scrolling remove the popup menu.
 */
    static void
ins_compl_upd_pum(void)
{
    int		h;

    if (compl_match_array != NULL)
    {
	h = curwin->w_cline_height;
	// Update the screen later, before drawing the popup menu over it.
	pum_call_update_screen();
	if (h != curwin->w_cline_height)
	    ins_compl_del_pum();
    }
}

/*
 * Remove any popup menu.
 */
    static void
ins_compl_del_pum(void)
{
    if (compl_match_array != NULL)
    {
	pum_undisplay();
	VIM_CLEAR(compl_match_array);
    }
}

/*
 * Return TRUE if the popup menu should be displayed.
 */
    int
pum_wanted(void)
{
    // 'completeopt' must contain "menu" or "menuone"
    if (vim_strchr(p_cot, 'm') == NULL)
	return FALSE;

    // The display looks bad on a B&W display.
    if (t_colors < 8
#ifdef FEAT_GUI
	    && !gui.in_use
#endif
	    )
	return FALSE;
    return TRUE;
}

/*
 * Return TRUE if there are two or more matches to be shown in the popup menu.
 * One if 'completopt' contains "menuone".
 */
    static int
pum_enough_matches(void)
{
    compl_T     *compl;
    int		i;

    // Don't display the popup menu if there are no matches or there is only
    // one (ignoring the original text).
    compl = compl_first_match;
    i = 0;
    do
    {
	if (compl == NULL
		      || ((compl->cp_flags & CP_ORIGINAL_TEXT) == 0 && ++i == 2))
	    break;
	compl = compl->cp_next;
    } while (compl != compl_first_match);

    if (strstr((char *)p_cot, "menuone") != NULL)
	return (i >= 1);
    return (i >= 2);
}

#ifdef FEAT_EVAL
/*
 * Allocate Dict for the completed item.
 * { word, abbr, menu, kind, info }
 */
    static dict_T *
ins_compl_dict_alloc(compl_T *match)
{
    dict_T *dict = dict_alloc_lock(VAR_FIXED);

    if (dict != NULL)
    {
	dict_add_string(dict, "word", match->cp_str);
	dict_add_string(dict, "abbr", match->cp_text[CPT_ABBR]);
	dict_add_string(dict, "menu", match->cp_text[CPT_MENU]);
	dict_add_string(dict, "kind", match->cp_text[CPT_KIND]);
	dict_add_string(dict, "info", match->cp_text[CPT_INFO]);
	if (match->cp_user_data.v_type == VAR_UNKNOWN)
	    dict_add_string(dict, "user_data", (char_u *)"");
	else
	    dict_add_tv(dict, "user_data", &match->cp_user_data);
    }
    return dict;
}

    static void
trigger_complete_changed_event(int cur)
{
    dict_T	    *v_event;
    dict_T	    *item;
    static int	    recursive = FALSE;

    if (recursive)
	return;

    v_event = get_vim_var_dict(VV_EVENT);
    if (cur < 0)
	item = dict_alloc();
    else
	item = ins_compl_dict_alloc(compl_curr_match);
    if (item == NULL)
	return;
    dict_add_dict(v_event, "completed_item", item);
    pum_set_event_info(v_event);
    dict_set_items_ro(v_event);

    recursive = TRUE;
    textlock++;
    apply_autocmds(EVENT_COMPLETECHANGED, NULL, NULL, FALSE, curbuf);
    textlock--;
    recursive = FALSE;

    dict_free_contents(v_event);
    hash_init(&v_event->dv_hashtab);
}
#endif

/*
 * Show the popup menu for the list of matches.
 * Also adjusts "compl_shown_match" to an entry that is actually displayed.
 */
    void
ins_compl_show_pum(void)
{
    compl_T     *compl;
    compl_T     *shown_compl = NULL;
    int		did_find_shown_match = FALSE;
    int		shown_match_ok = FALSE;
    int		i;
    int		cur = -1;
    colnr_T	col;
    int		lead_len = 0;

    if (!pum_wanted() || !pum_enough_matches())
	return;

#if defined(FEAT_EVAL)
    // Dirty hard-coded hack: remove any matchparen highlighting.
    do_cmdline_cmd((char_u *)"if exists('g:loaded_matchparen')|3match none|endif");
#endif

    // Update the screen later, before drawing the popup menu over it.
    pum_call_update_screen();

    if (compl_match_array == NULL)
    {
	// Need to build the popup menu list.
	compl_match_arraysize = 0;
	compl = compl_first_match;
	if (compl_leader != NULL)
	    lead_len = (int)STRLEN(compl_leader);
	do
	{
	    if ((compl->cp_flags & CP_ORIGINAL_TEXT) == 0
		    && (compl_leader == NULL
			|| ins_compl_equal(compl, compl_leader, lead_len)))
		++compl_match_arraysize;
	    compl = compl->cp_next;
	} while (compl != NULL && compl != compl_first_match);
	if (compl_match_arraysize == 0)
	    return;
	compl_match_array = ALLOC_CLEAR_MULT(pumitem_T, compl_match_arraysize);
	if (compl_match_array != NULL)
	{
	    // If the current match is the original text don't find the first
	    // match after it, don't highlight anything.
	    if (compl_shown_match->cp_flags & CP_ORIGINAL_TEXT)
		shown_match_ok = TRUE;

	    i = 0;
	    compl = compl_first_match;
	    do
	    {
		if ((compl->cp_flags & CP_ORIGINAL_TEXT) == 0
			&& (compl_leader == NULL
			    || ins_compl_equal(compl, compl_leader, lead_len)))
		{
		    if (!shown_match_ok)
		    {
			if (compl == compl_shown_match || did_find_shown_match)
			{
			    // This item is the shown match or this is the
			    // first displayed item after the shown match.
			    compl_shown_match = compl;
			    did_find_shown_match = TRUE;
			    shown_match_ok = TRUE;
			}
			else
			    // Remember this displayed match for when the
			    // shown match is just below it.
			    shown_compl = compl;
			cur = i;
		    }

		    if (compl->cp_text[CPT_ABBR] != NULL)
			compl_match_array[i].pum_text =
						     compl->cp_text[CPT_ABBR];
		    else
			compl_match_array[i].pum_text = compl->cp_str;
		    compl_match_array[i].pum_kind = compl->cp_text[CPT_KIND];
		    compl_match_array[i].pum_info = compl->cp_text[CPT_INFO];
		    if (compl->cp_text[CPT_MENU] != NULL)
			compl_match_array[i++].pum_extra =
						     compl->cp_text[CPT_MENU];
		    else
			compl_match_array[i++].pum_extra = compl->cp_fname;
		}

		if (compl == compl_shown_match)
		{
		    did_find_shown_match = TRUE;

		    // When the original text is the shown match don't set
		    // compl_shown_match.
		    if (compl->cp_flags & CP_ORIGINAL_TEXT)
			shown_match_ok = TRUE;

		    if (!shown_match_ok && shown_compl != NULL)
		    {
			// The shown match isn't displayed, set it to the
			// previously displayed match.
			compl_shown_match = shown_compl;
			shown_match_ok = TRUE;
		    }
		}
		compl = compl->cp_next;
	    } while (compl != NULL && compl != compl_first_match);

	    if (!shown_match_ok)    // no displayed match at all
		cur = -1;
	}
    }
    else
    {
	// popup menu already exists, only need to find the current item.
	for (i = 0; i < compl_match_arraysize; ++i)
	    if (compl_match_array[i].pum_text == compl_shown_match->cp_str
		    || compl_match_array[i].pum_text
				      == compl_shown_match->cp_text[CPT_ABBR])
	    {
		cur = i;
		break;
	    }
    }

    if (compl_match_array != NULL)
    {
	// In Replace mode when a $ is displayed at the end of the line only
	// part of the screen would be updated.  We do need to redraw here.
	dollar_vcol = -1;

	// Compute the screen column of the start of the completed text.
	// Use the cursor to get all wrapping and other settings right.
	col = curwin->w_cursor.col;
	curwin->w_cursor.col = compl_col;
	pum_display(compl_match_array, compl_match_arraysize, cur);
	curwin->w_cursor.col = col;

#ifdef FEAT_EVAL
	if (has_completechanged())
	    trigger_complete_changed_event(cur);
#endif
    }
}

#define DICT_FIRST	(1)	// use just first element in "dict"
#define DICT_EXACT	(2)	// "dict" is the exact name of a file

/*
 * Add any identifiers that match the given pattern in the list of dictionary
 * files "dict_start" to the list of completions.
 */
    static void
ins_compl_dictionaries(
    char_u	*dict_start,
    char_u	*pat,
    int		flags,		// DICT_FIRST and/or DICT_EXACT
    int		thesaurus)	// Thesaurus completion
{
    char_u	*dict = dict_start;
    char_u	*ptr;
    char_u	*buf;
    regmatch_T	regmatch;
    char_u	**files;
    int		count;
    int		save_p_scs;
    int		dir = compl_direction;

    if (*dict == NUL)
    {
#ifdef FEAT_SPELL
	// When 'dictionary' is empty and spell checking is enabled use
	// "spell".
	if (!thesaurus && curwin->w_p_spell)
	    dict = (char_u *)"spell";
	else
#endif
	    return;
    }

    buf = alloc(LSIZE);
    if (buf == NULL)
	return;
    regmatch.regprog = NULL;	// so that we can goto theend

    // If 'infercase' is set, don't use 'smartcase' here
    save_p_scs = p_scs;
    if (curbuf->b_p_inf)
	p_scs = FALSE;

    // When invoked to match whole lines for CTRL-X CTRL-L adjust the pattern
    // to only match at the start of a line.  Otherwise just match the
    // pattern. Also need to double backslashes.
    if (ctrl_x_mode_line_or_eval())
    {
	char_u *pat_esc = vim_strsave_escaped(pat, (char_u *)"\\");
	size_t len;

	if (pat_esc == NULL)
	    goto theend;
	len = STRLEN(pat_esc) + 10;
	ptr = alloc(len);
	if (ptr == NULL)
	{
	    vim_free(pat_esc);
	    goto theend;
	}
	vim_snprintf((char *)ptr, len, "^\\s*\\zs\\V%s", pat_esc);
	regmatch.regprog = vim_regcomp(ptr, RE_MAGIC);
	vim_free(pat_esc);
	vim_free(ptr);
    }
    else
    {
	regmatch.regprog = vim_regcomp(pat, p_magic ? RE_MAGIC : 0);
	if (regmatch.regprog == NULL)
	    goto theend;
    }

    // ignore case depends on 'ignorecase', 'smartcase' and "pat"
    regmatch.rm_ic = ignorecase(pat);
    while (*dict != NUL && !got_int && !compl_interrupted)
    {
	// copy one dictionary file name into buf
	if (flags == DICT_EXACT)
	{
	    count = 1;
	    files = &dict;
	}
	else
	{
	    // Expand wildcards in the dictionary name, but do not allow
	    // backticks (for security, the 'dict' option may have been set in
	    // a modeline).
	    copy_option_part(&dict, buf, LSIZE, ",");
# ifdef FEAT_SPELL
	    if (!thesaurus && STRCMP(buf, "spell") == 0)
		count = -1;
	    else
# endif
		if (vim_strchr(buf, '`') != NULL
		    || expand_wildcards(1, &buf, &count, &files,
						     EW_FILE|EW_SILENT) != OK)
		count = 0;
	}

# ifdef FEAT_SPELL
	if (count == -1)
	{
	    // Complete from active spelling.  Skip "\<" in the pattern, we
	    // don't use it as a RE.
	    if (pat[0] == '\\' && pat[1] == '<')
		ptr = pat + 2;
	    else
		ptr = pat;
	    spell_dump_compl(ptr, regmatch.rm_ic, &dir, 0);
	}
	else
# endif
	    if (count > 0)	// avoid warning for using "files" uninit
	{
	    ins_compl_files(count, files, thesaurus, flags,
							&regmatch, buf, &dir);
	    if (flags != DICT_EXACT)
		FreeWild(count, files);
	}
	if (flags != 0)
	    break;
    }

theend:
    p_scs = save_p_scs;
    vim_regfree(regmatch.regprog);
    vim_free(buf);
}

    static void
ins_compl_files(
    int		count,
    char_u	**files,
    int		thesaurus,
    int		flags,
    regmatch_T	*regmatch,
    char_u	*buf,
    int		*dir)
{
    char_u	*ptr;
    int		i;
    FILE	*fp;
    int		add_r;

    for (i = 0; i < count && !got_int && !compl_interrupted; i++)
    {
	fp = mch_fopen((char *)files[i], "r");  // open dictionary file
	if (flags != DICT_EXACT)
	{
	    vim_snprintf((char *)IObuff, IOSIZE,
			      _("Scanning dictionary: %s"), (char *)files[i]);
	    (void)msg_trunc_attr((char *)IObuff, TRUE, HL_ATTR(HLF_R));
	}

	if (fp != NULL)
	{
	    // Read dictionary file line by line.
	    // Check each line for a match.
	    while (!got_int && !compl_interrupted
					    && !vim_fgets(buf, LSIZE, fp))
	    {
		ptr = buf;
		while (vim_regexec(regmatch, buf, (colnr_T)(ptr - buf)))
		{
		    ptr = regmatch->startp[0];
		    if (ctrl_x_mode_line_or_eval())
			ptr = find_line_end(ptr);
		    else
			ptr = find_word_end(ptr);
		    add_r = ins_compl_add_infercase(regmatch->startp[0],
					  (int)(ptr - regmatch->startp[0]),
						  p_ic, files[i], *dir, FALSE);
		    if (thesaurus)
		    {
			char_u *wstart;

			// Add the other matches on the line
			ptr = buf;
			while (!got_int)
			{
			    // Find start of the next word.  Skip white
			    // space and punctuation.
			    ptr = find_word_start(ptr);
			    if (*ptr == NUL || *ptr == NL)
				break;
			    wstart = ptr;

			    // Find end of the word.
			    if (has_mbyte)
				// Japanese words may have characters in
				// different classes, only separate words
				// with single-byte non-word characters.
				while (*ptr != NUL)
				{
				    int l = (*mb_ptr2len)(ptr);

				    if (l < 2 && !vim_iswordc(*ptr))
					break;
				    ptr += l;
				}
			    else
				ptr = find_word_end(ptr);

			    // Add the word. Skip the regexp match.
			    if (wstart != regmatch->startp[0])
				add_r = ins_compl_add_infercase(wstart,
					(int)(ptr - wstart),
					p_ic, files[i], *dir, FALSE);
			}
		    }
		    if (add_r == OK)
			// if dir was BACKWARD then honor it just once
			*dir = FORWARD;
		    else if (add_r == FAIL)
			break;
		    // avoid expensive call to vim_regexec() when at end
		    // of line
		    if (*ptr == '\n' || got_int)
			break;
		}
		line_breakcheck();
		ins_compl_check_keys(50, FALSE);
	    }
	    fclose(fp);
	}
    }
}

/*
 * Find the start of the next word.
 * Returns a pointer to the first char of the word.  Also stops at a NUL.
 */
    char_u *
find_word_start(char_u *ptr)
{
    if (has_mbyte)
	while (*ptr != NUL && *ptr != '\n' && mb_get_class(ptr) <= 1)
	    ptr += (*mb_ptr2len)(ptr);
    else
	while (*ptr != NUL && *ptr != '\n' && !vim_iswordc(*ptr))
	    ++ptr;
    return ptr;
}

/*
 * Find the end of the word.  Assumes it starts inside a word.
 * Returns a pointer to just after the word.
 */
    char_u *
find_word_end(char_u *ptr)
{
    int		start_class;

    if (has_mbyte)
    {
	start_class = mb_get_class(ptr);
	if (start_class > 1)
	    while (*ptr != NUL)
	    {
		ptr += (*mb_ptr2len)(ptr);
		if (mb_get_class(ptr) != start_class)
		    break;
	    }
    }
    else
	while (vim_iswordc(*ptr))
	    ++ptr;
    return ptr;
}

/*
 * Find the end of the line, omitting CR and NL at the end.
 * Returns a pointer to just after the line.
 */
    static char_u *
find_line_end(char_u *ptr)
{
    char_u	*s;

    s = ptr + STRLEN(ptr);
    while (s > ptr && (s[-1] == CAR || s[-1] == NL))
	--s;
    return s;
}

/*
 * Free the list of completions
 */
    static void
ins_compl_free(void)
{
    compl_T *match;
    int	    i;

    VIM_CLEAR(compl_pattern);
    VIM_CLEAR(compl_leader);

    if (compl_first_match == NULL)
	return;

    ins_compl_del_pum();
    pum_clear();

    compl_curr_match = compl_first_match;
    do
    {
	match = compl_curr_match;
	compl_curr_match = compl_curr_match->cp_next;
	vim_free(match->cp_str);
	// several entries may use the same fname, free it just once.
	if (match->cp_flags & CP_FREE_FNAME)
	    vim_free(match->cp_fname);
	for (i = 0; i < CPT_COUNT; ++i)
	    vim_free(match->cp_text[i]);
#ifdef FEAT_EVAL
	clear_tv(&match->cp_user_data);
#endif
	vim_free(match);
    } while (compl_curr_match != NULL && compl_curr_match != compl_first_match);
    compl_first_match = compl_curr_match = NULL;
    compl_shown_match = NULL;
    compl_old_match = NULL;
}

    void
ins_compl_clear(void)
{
    compl_cont_status = 0;
    compl_started = FALSE;
    compl_matches = 0;
    VIM_CLEAR(compl_pattern);
    VIM_CLEAR(compl_leader);
    edit_submode_extra = NULL;
    VIM_CLEAR(compl_orig_text);
    compl_enter_selects = FALSE;
#ifdef FEAT_EVAL
    // clear v:completed_item
    set_vim_var_dict(VV_COMPLETED_ITEM, dict_alloc_lock(VAR_FIXED));
#endif
}

/*
 * Return TRUE when Insert completion is active.
 */
    int
ins_compl_active(void)
{
    return compl_started;
}

/*
 * Selected one of the matches.  When FALSE the match was edited or using the
 * longest common string.
 */
    int
ins_compl_used_match(void)
{
    return compl_used_match;
}

/*
 * Initialize get longest common string.
 */
    void
ins_compl_init_get_longest(void)
{
    compl_get_longest = FALSE;
}

/*
 * Returns TRUE when insert completion is interrupted.
 */
    int
ins_compl_interrupted(void)
{
    return compl_interrupted;
}

/*
 * Returns TRUE if the <Enter> key selects a match in the completion popup
 * menu.
 */
    int
ins_compl_enter_selects(void)
{
    return compl_enter_selects;
}

/*
 * Return the column where the text starts that is being completed
 */
    colnr_T
ins_compl_col(void)
{
    return compl_col;
}

/*
 * Delete one character before the cursor and show the subset of the matches
 * that match the word that is now before the cursor.
 * Returns the character to be used, NUL if the work is done and another char
 * to be got from the user.
 */
    int
ins_compl_bs(void)
{
    char_u	*line;
    char_u	*p;

    line = ml_get_curline();
    p = line + curwin->w_cursor.col;
    MB_PTR_BACK(line, p);

    // Stop completion when the whole word was deleted.  For Omni completion
    // allow the word to be deleted, we won't match everything.
    // Respect the 'backspace' option.
    if ((int)(p - line) - (int)compl_col < 0
	    || ((int)(p - line) - (int)compl_col == 0
		&& ctrl_x_mode != CTRL_X_OMNI) || ctrl_x_mode == CTRL_X_EVAL
	    || (!can_bs(BS_START) && (int)(p - line) - (int)compl_col
							- compl_length < 0))
	return K_BS;

    // Deleted more than what was used to find matches or didn't finish
    // finding all matches: need to look for matches all over again.
    if (curwin->w_cursor.col <= compl_col + compl_length
						  || ins_compl_need_restart())
	ins_compl_restart();

    vim_free(compl_leader);
    compl_leader = vim_strnsave(line + compl_col, (int)(p - line) - compl_col);
    if (compl_leader != NULL)
    {
	ins_compl_new_leader();
	if (compl_shown_match != NULL)
	    // Make sure current match is not a hidden item.
	    compl_curr_match = compl_shown_match;
	return NUL;
    }
    return K_BS;
}

/*
 * Return TRUE when we need to find matches again, ins_compl_restart() is to
 * be called.
 */
    static int
ins_compl_need_restart(void)
{
    // Return TRUE if we didn't complete finding matches or when the
    // 'completefunc' returned "always" in the "refresh" dictionary item.
    return compl_was_interrupted
	|| ((ctrl_x_mode == CTRL_X_FUNCTION || ctrl_x_mode == CTRL_X_OMNI)
						  && compl_opt_refresh_always);
}

/*
 * Called after changing "compl_leader".
 * Show the popup menu with a different set of matches.
 * May also search for matches again if the previous search was interrupted.
 */
    static void
ins_compl_new_leader(void)
{
    ins_compl_del_pum();
    ins_compl_delete();
    ins_bytes(compl_leader + ins_compl_len());
    compl_used_match = FALSE;

    if (compl_started)
	ins_compl_set_original_text(compl_leader);
    else
    {
#ifdef FEAT_SPELL
	spell_bad_len = 0;	// need to redetect bad word
#endif
	// Matches were cleared, need to search for them now.  Before drawing
	// the popup menu display the changed text before the cursor.  Set
	// "compl_restarting" to avoid that the first match is inserted.
	pum_call_update_screen();
#ifdef FEAT_GUI
	if (gui.in_use)
	{
	    // Show the cursor after the match, not after the redrawn text.
	    setcursor();
	    out_flush_cursor(FALSE, FALSE);
	}
#endif
	compl_restarting = TRUE;
	if (ins_complete(Ctrl_N, TRUE) == FAIL)
	    compl_cont_status = 0;
	compl_restarting = FALSE;
    }

    compl_enter_selects = !compl_used_match;

    // Show the popup menu with a different set of matches.
    ins_compl_show_pum();

    // Don't let Enter select the original text when there is no popup menu.
    if (compl_match_array == NULL)
	compl_enter_selects = FALSE;
}

/*
 * Return the length of the completion, from the completion start column to
 * the cursor column.  Making sure it never goes below zero.
 */
    static int
ins_compl_len(void)
{
    int off = (int)curwin->w_cursor.col - (int)compl_col;

    if (off < 0)
	return 0;
    return off;
}

/*
 * Append one character to the match leader.  May reduce the number of
 * matches.
 */
    void
ins_compl_addleader(int c)
{
    int		cc;

    if (stop_arrow() == FAIL)
	return;
    if (has_mbyte && (cc = (*mb_char2len)(c)) > 1)
    {
	char_u	buf[MB_MAXBYTES + 1];

	(*mb_char2bytes)(c, buf);
	buf[cc] = NUL;
	ins_char_bytes(buf, cc);
	if (compl_opt_refresh_always)
	    AppendToRedobuff(buf);
    }
    else
    {
	ins_char(c);
	if (compl_opt_refresh_always)
	    AppendCharToRedobuff(c);
    }

    // If we didn't complete finding matches we must search again.
    if (ins_compl_need_restart())
	ins_compl_restart();

    // When 'always' is set, don't reset compl_leader. While completing,
    // cursor doesn't point original position, changing compl_leader would
    // break redo.
    if (!compl_opt_refresh_always)
    {
	vim_free(compl_leader);
	compl_leader = vim_strnsave(ml_get_curline() + compl_col,
				     (int)(curwin->w_cursor.col - compl_col));
	if (compl_leader != NULL)
	    ins_compl_new_leader();
    }
}

/*
 * Setup for finding completions again without leaving CTRL-X mode.  Used when
 * BS or a key was typed while still searching for matches.
 */
    static void
ins_compl_restart(void)
{
    ins_compl_free();
    compl_started = FALSE;
    compl_matches = 0;
    compl_cont_status = 0;
    compl_cont_mode = 0;
}

/*
 * Set the first match, the original text.
 */
    static void
ins_compl_set_original_text(char_u *str)
{
    char_u	*p;

    // Replace the original text entry.
    // The CP_ORIGINAL_TEXT flag is either at the first item or might possibly be
    // at the last item for backward completion
    if (compl_first_match->cp_flags & CP_ORIGINAL_TEXT)	// safety check
    {
	p = vim_strsave(str);
	if (p != NULL)
	{
	    vim_free(compl_first_match->cp_str);
	    compl_first_match->cp_str = p;
	}
    }
    else if (compl_first_match->cp_prev != NULL
	    && (compl_first_match->cp_prev->cp_flags & CP_ORIGINAL_TEXT))
    {
       p = vim_strsave(str);
       if (p != NULL)
       {
           vim_free(compl_first_match->cp_prev->cp_str);
           compl_first_match->cp_prev->cp_str = p;
       }
    }
}

/*
 * Append one character to the match leader.  May reduce the number of
 * matches.
 */
    void
ins_compl_addfrommatch(void)
{
    char_u	*p;
    int		len = (int)curwin->w_cursor.col - (int)compl_col;
    int		c;
    compl_T	*cp;

    p = compl_shown_match->cp_str;
    if ((int)STRLEN(p) <= len)   // the match is too short
    {
	// When still at the original match use the first entry that matches
	// the leader.
	if (compl_shown_match->cp_flags & CP_ORIGINAL_TEXT)
	{
	    p = NULL;
	    for (cp = compl_shown_match->cp_next; cp != NULL
				 && cp != compl_first_match; cp = cp->cp_next)
	    {
		if (compl_leader == NULL
			|| ins_compl_equal(cp, compl_leader,
						   (int)STRLEN(compl_leader)))
		{
		    p = cp->cp_str;
		    break;
		}
	    }
	    if (p == NULL || (int)STRLEN(p) <= len)
		return;
	}
	else
	    return;
    }
    p += len;
    c = PTR2CHAR(p);
    ins_compl_addleader(c);
}

/*
 * Prepare for Insert mode completion, or stop it.
 * Called just after typing a character in Insert mode.
 * Returns TRUE when the character is not to be inserted;
 */
    int
ins_compl_prep(int c)
{
    char_u	*ptr;
#ifdef FEAT_CINDENT
    int		want_cindent;
#endif
    int		retval = FALSE;
    int		prev_mode = ctrl_x_mode;

    // Forget any previous 'special' messages if this is actually
    // a ^X mode key - bar ^R, in which case we wait to see what it gives us.
    if (c != Ctrl_R && vim_is_ctrl_x_key(c))
	edit_submode_extra = NULL;

    // Ignore end of Select mode mapping and mouse scroll buttons.
    if (c == K_SELECT || c == K_MOUSEDOWN || c == K_MOUSEUP
	    || c == K_MOUSELEFT || c == K_MOUSERIGHT)
	return retval;

#ifdef FEAT_PROP_POPUP
    // Ignore mouse events in a popup window
    if (is_mouse_key(c))
    {
	// Ignore drag and release events, the position does not need to be in
	// the popup and it may have just closed.
	if (c == K_LEFTRELEASE
		|| c == K_LEFTRELEASE_NM
		|| c == K_MIDDLERELEASE
		|| c == K_RIGHTRELEASE
		|| c == K_X1RELEASE
		|| c == K_X2RELEASE
		|| c == K_LEFTDRAG
		|| c == K_MIDDLEDRAG
		|| c == K_RIGHTDRAG
		|| c == K_X1DRAG
		|| c == K_X2DRAG)
	    return retval;
	if (popup_visible)
	{
	    int	    row = mouse_row;
	    int	    col = mouse_col;
	    win_T   *wp = mouse_find_win(&row, &col, FIND_POPUP);

	    if (wp != NULL && WIN_IS_POPUP(wp))
		return retval;
	}
    }
#endif

    // Set "compl_get_longest" when finding the first matches.
    if (ctrl_x_mode == CTRL_X_NOT_DEFINED_YET
			   || (ctrl_x_mode == CTRL_X_NORMAL && !compl_started))
    {
	compl_get_longest = (strstr((char *)p_cot, "longest") != NULL);
	compl_used_match = TRUE;

    }

    if (ctrl_x_mode == CTRL_X_NOT_DEFINED_YET)
    {
	// We have just typed CTRL-X and aren't quite sure which CTRL-X mode
	// it will be yet.  Now we decide.
	switch (c)
	{
	    case Ctrl_E:
	    case Ctrl_Y:
		ctrl_x_mode = CTRL_X_SCROLL;
		if (!(State & REPLACE_FLAG))
		    edit_submode = (char_u *)_(" (insert) Scroll (^E/^Y)");
		else
		    edit_submode = (char_u *)_(" (replace) Scroll (^E/^Y)");
		edit_submode_pre = NULL;
		showmode();
		break;
	    case Ctrl_L:
		ctrl_x_mode = CTRL_X_WHOLE_LINE;
		break;
	    case Ctrl_F:
		ctrl_x_mode = CTRL_X_FILES;
		break;
	    case Ctrl_K:
		ctrl_x_mode = CTRL_X_DICTIONARY;
		break;
	    case Ctrl_R:
		// Simply allow ^R to happen without affecting ^X mode
		break;
	    case Ctrl_T:
		ctrl_x_mode = CTRL_X_THESAURUS;
		break;
#ifdef FEAT_COMPL_FUNC
	    case Ctrl_U:
		ctrl_x_mode = CTRL_X_FUNCTION;
		break;
	    case Ctrl_O:
		ctrl_x_mode = CTRL_X_OMNI;
		break;
#endif
	    case 's':
	    case Ctrl_S:
		ctrl_x_mode = CTRL_X_SPELL;
#ifdef FEAT_SPELL
		++emsg_off;	// Avoid getting the E756 error twice.
		spell_back_to_badword();
		--emsg_off;
#endif
		break;
	    case Ctrl_RSB:
		ctrl_x_mode = CTRL_X_TAGS;
		break;
#ifdef FEAT_FIND_ID
	    case Ctrl_I:
	    case K_S_TAB:
		ctrl_x_mode = CTRL_X_PATH_PATTERNS;
		break;
	    case Ctrl_D:
		ctrl_x_mode = CTRL_X_PATH_DEFINES;
		break;
#endif
	    case Ctrl_V:
	    case Ctrl_Q:
		ctrl_x_mode = CTRL_X_CMDLINE;
		break;
	    case Ctrl_P:
	    case Ctrl_N:
		// ^X^P means LOCAL expansion if nothing interrupted (eg we
		// just started ^X mode, or there were enough ^X's to cancel
		// the previous mode, say ^X^F^X^X^P or ^P^X^X^X^P, see below)
		// do normal expansion when interrupting a different mode (say
		// ^X^F^X^P or ^P^X^X^P, see below)
		// nothing changes if interrupting mode 0, (eg, the flag
		// doesn't change when going to ADDING mode  -- Acevedo
		if (!(compl_cont_status & CONT_INTRPT))
		    compl_cont_status |= CONT_LOCAL;
		else if (compl_cont_mode != 0)
		    compl_cont_status &= ~CONT_LOCAL;
		// FALLTHROUGH
	    default:
		// If we have typed at least 2 ^X's... for modes != 0, we set
		// compl_cont_status = 0 (eg, as if we had just started ^X
		// mode).
		// For mode 0, we set "compl_cont_mode" to an impossible
		// value, in both cases ^X^X can be used to restart the same
		// mode (avoiding ADDING mode).
		// Undocumented feature: In a mode != 0 ^X^P and ^X^X^P start
		// 'complete' and local ^P expansions respectively.
		// In mode 0 an extra ^X is needed since ^X^P goes to ADDING
		// mode  -- Acevedo
		if (c == Ctrl_X)
		{
		    if (compl_cont_mode != 0)
			compl_cont_status = 0;
		    else
			compl_cont_mode = CTRL_X_NOT_DEFINED_YET;
		}
		ctrl_x_mode = CTRL_X_NORMAL;
		edit_submode = NULL;
		showmode();
		break;
	}
    }
    else if (ctrl_x_mode != CTRL_X_NORMAL)
    {
	// We're already in CTRL-X mode, do we stay in it?
	if (!vim_is_ctrl_x_key(c))
	{
	    if (ctrl_x_mode == CTRL_X_SCROLL)
		ctrl_x_mode = CTRL_X_NORMAL;
	    else
		ctrl_x_mode = CTRL_X_FINISHED;
	    edit_submode = NULL;
	}
	showmode();
    }

    if (compl_started || ctrl_x_mode == CTRL_X_FINISHED)
    {
	// Show error message from attempted keyword completion (probably
	// 'Pattern not found') until another key is hit, then go back to
	// showing what mode we are in.
	showmode();
	if ((ctrl_x_mode == CTRL_X_NORMAL && c != Ctrl_N && c != Ctrl_P
				       && c != Ctrl_R && !ins_compl_pum_key(c))
		|| ctrl_x_mode == CTRL_X_FINISHED)
	{
	    // Get here when we have finished typing a sequence of ^N and
	    // ^P or other completion characters in CTRL-X mode.  Free up
	    // memory that was used, and make sure we can redo the insert.
	    if (compl_curr_match != NULL || compl_leader != NULL || c == Ctrl_E)
	    {
		// If any of the original typed text has been changed, eg when
		// ignorecase is set, we must add back-spaces to the redo
		// buffer.  We add as few as necessary to delete just the part
		// of the original text that has changed.
		// When using the longest match, edited the match or used
		// CTRL-E then don't use the current match.
		if (compl_curr_match != NULL && compl_used_match && c != Ctrl_E)
		    ptr = compl_curr_match->cp_str;
		else
		    ptr = NULL;
		ins_compl_fixRedoBufForLeader(ptr);
	    }

#ifdef FEAT_CINDENT
	    want_cindent = (get_can_cindent() && cindent_on());
#endif
	    // When completing whole lines: fix indent for 'cindent'.
	    // Otherwise, break line if it's too long.
	    if (compl_cont_mode == CTRL_X_WHOLE_LINE)
	    {
#ifdef FEAT_CINDENT
		// re-indent the current line
		if (want_cindent)
		{
		    do_c_expr_indent();
		    want_cindent = FALSE;	// don't do it again
		}
#endif
	    }
	    else
	    {
		int prev_col = curwin->w_cursor.col;

		// put the cursor on the last char, for 'tw' formatting
		if (prev_col > 0)
		    dec_cursor();
		// only format when something was inserted
		if (!arrow_used && !ins_need_undo_get() && c != Ctrl_E)
		    insertchar(NUL, 0, -1);
		if (prev_col > 0
			     && ml_get_curline()[curwin->w_cursor.col] != NUL)
		    inc_cursor();
	    }

	    // If the popup menu is displayed pressing CTRL-Y means accepting
	    // the selection without inserting anything.  When
	    // compl_enter_selects is set the Enter key does the same.
	    if ((c == Ctrl_Y || (compl_enter_selects
				   && (c == CAR || c == K_KENTER || c == NL)))
		    && pum_visible())
		retval = TRUE;

	    // CTRL-E means completion is Ended, go back to the typed text.
	    // but only do this, if the Popup is still visible
	    if (c == Ctrl_E)
	    {
		ins_compl_delete();
		if (compl_leader != NULL)
		    ins_bytes(compl_leader + ins_compl_len());
		else if (compl_first_match != NULL)
		    ins_bytes(compl_orig_text + ins_compl_len());
		retval = TRUE;
	    }

	    auto_format(FALSE, TRUE);

	    {
		int new_mode = ctrl_x_mode;

		// Trigger the CompleteDone event to give scripts a chance to
		// act upon the completion.  Do this before clearing the info,
		// and restore ctrl_x_mode, so that complete_info() can be
		// used.
		ctrl_x_mode = prev_mode;
		ins_apply_autocmds(EVENT_COMPLETEDONE);
		ctrl_x_mode = new_mode;
	    }

	    ins_compl_free();
	    compl_started = FALSE;
	    compl_matches = 0;
	    if (!shortmess(SHM_COMPLETIONMENU))
		msg_clr_cmdline();	// necessary for "noshowmode"
	    ctrl_x_mode = CTRL_X_NORMAL;
	    compl_enter_selects = FALSE;
	    if (edit_submode != NULL)
	    {
		edit_submode = NULL;
		showmode();
	    }

#ifdef FEAT_CMDWIN
	    if (c == Ctrl_C && cmdwin_type != 0)
		// Avoid the popup menu remains displayed when leaving the
		// command line window.
		update_screen(0);
#endif
#ifdef FEAT_CINDENT
	    // Indent now if a key was typed that is in 'cinkeys'.
	    if (want_cindent && in_cinkeys(KEY_COMPLETE, ' ', inindent(0)))
		do_c_expr_indent();
#endif
	}
    }
    else if (ctrl_x_mode == CTRL_X_LOCAL_MSG)
	// Trigger the CompleteDone event to give scripts a chance to act
	// upon the (possibly failed) completion.
	ins_apply_autocmds(EVENT_COMPLETEDONE);

    // reset continue_* if we left expansion-mode, if we stay they'll be
    // (re)set properly in ins_complete()
    if (!vim_is_ctrl_x_key(c))
    {
	compl_cont_status = 0;
	compl_cont_mode = 0;
    }

    return retval;
}

/*
 * Fix the redo buffer for the completion leader replacing some of the typed
 * text.  This inserts backspaces and appends the changed text.
 * "ptr" is the known leader text or NUL.
 */
    static void
ins_compl_fixRedoBufForLeader(char_u *ptr_arg)
{
    int	    len;
    char_u  *p;
    char_u  *ptr = ptr_arg;

    if (ptr == NULL)
    {
	if (compl_leader != NULL)
	    ptr = compl_leader;
	else
	    return;  // nothing to do
    }
    if (compl_orig_text != NULL)
    {
	p = compl_orig_text;
	for (len = 0; p[len] != NUL && p[len] == ptr[len]; ++len)
	    ;
	if (len > 0)
	    len -= (*mb_head_off)(p, p + len);
	for (p += len; *p != NUL; MB_PTR_ADV(p))
	    AppendCharToRedobuff(K_BS);
    }
    else
	len = 0;
    if (ptr != NULL)
	AppendToRedobuffLit(ptr + len, -1);
}

/*
 * Loops through the list of windows, loaded-buffers or non-loaded-buffers
 * (depending on flag) starting from buf and looking for a non-scanned
 * buffer (other than curbuf).	curbuf is special, if it is called with
 * buf=curbuf then it has to be the first call for a given flag/expansion.
 *
 * Returns the buffer to scan, if any, otherwise returns curbuf -- Acevedo
 */
    static buf_T *
ins_compl_next_buf(buf_T *buf, int flag)
{
    static win_T *wp = NULL;

    if (flag == 'w')		// just windows
    {
	if (buf == curbuf || wp == NULL)  // first call for this flag/expansion
	    wp = curwin;
	while ((wp = (wp->w_next != NULL ? wp->w_next : firstwin)) != curwin
		&& wp->w_buffer->b_scanned)
	    ;
	buf = wp->w_buffer;
    }
    else
	// 'b' (just loaded buffers), 'u' (just non-loaded buffers) or 'U'
	// (unlisted buffers)
	// When completing whole lines skip unloaded buffers.
	while ((buf = (buf->b_next != NULL ? buf->b_next : firstbuf)) != curbuf
		&& ((flag == 'U'
			? buf->b_p_bl
			: (!buf->b_p_bl
			    || (buf->b_ml.ml_mfp == NULL) != (flag == 'u')))
		    || buf->b_scanned))
	    ;
    return buf;
}

#ifdef FEAT_COMPL_FUNC
/*
 * Execute user defined complete function 'completefunc' or 'omnifunc', and
 * get matches in "matches".
 */
    static void
expand_by_function(
    int		type,	    // CTRL_X_OMNI or CTRL_X_FUNCTION
    char_u	*base)
{
    list_T      *matchlist = NULL;
    dict_T	*matchdict = NULL;
    typval_T	args[3];
    char_u	*funcname;
    pos_T	pos;
    win_T	*curwin_save;
    buf_T	*curbuf_save;
    typval_T	rettv;
    int		save_State = State;

    funcname = (type == CTRL_X_FUNCTION) ? curbuf->b_p_cfu : curbuf->b_p_ofu;
    if (*funcname == NUL)
	return;

    // Call 'completefunc' to obtain the list of matches.
    args[0].v_type = VAR_NUMBER;
    args[0].vval.v_number = 0;
    args[1].v_type = VAR_STRING;
    args[1].vval.v_string = base != NULL ? base : (char_u *)"";
    args[2].v_type = VAR_UNKNOWN;

    pos = curwin->w_cursor;
    curwin_save = curwin;
    curbuf_save = curbuf;

    // Call a function, which returns a list or dict.
    if (call_vim_function(funcname, 2, args, &rettv) == OK)
    {
	switch (rettv.v_type)
	{
	    case VAR_LIST:
		matchlist = rettv.vval.v_list;
		break;
	    case VAR_DICT:
		matchdict = rettv.vval.v_dict;
		break;
	    case VAR_SPECIAL:
		if (rettv.vval.v_number == VVAL_NONE)
		    compl_opt_suppress_empty = TRUE;
		// FALLTHROUGH
	    default:
		// TODO: Give error message?
		clear_tv(&rettv);
		break;
	}
    }

    if (curwin_save != curwin || curbuf_save != curbuf)
    {
	emsg(_(e_complwin));
	goto theend;
    }
    curwin->w_cursor = pos;	// restore the cursor position
    validate_cursor();
    if (!EQUAL_POS(curwin->w_cursor, pos))
    {
	emsg(_(e_compldel));
	goto theend;
    }

    if (matchlist != NULL)
	ins_compl_add_list(matchlist);
    else if (matchdict != NULL)
	ins_compl_add_dict(matchdict);

theend:
    // Restore State, it might have been changed.
    State = save_State;

    if (matchdict != NULL)
	dict_unref(matchdict);
    if (matchlist != NULL)
	list_unref(matchlist);
}
#endif // FEAT_COMPL_FUNC

#if defined(FEAT_COMPL_FUNC) || defined(FEAT_EVAL) || defined(PROTO)
/*
 * Add a match to the list of matches from a typeval_T.
 * If the given string is already in the list of completions, then return
 * NOTDONE, otherwise add it to the list and return OK.  If there is an error,
 * maybe because alloc() returns NULL, then FAIL is returned.
 */
    static int
ins_compl_add_tv(typval_T *tv, int dir)
{
    char_u	*word;
    int		dup = FALSE;
    int		empty = FALSE;
    int		flags = 0;
    char_u	*(cptext[CPT_COUNT]);
    typval_T	user_data;

    user_data.v_type = VAR_UNKNOWN;
    if (tv->v_type == VAR_DICT && tv->vval.v_dict != NULL)
    {
	word = dict_get_string(tv->vval.v_dict, (char_u *)"word", FALSE);
	cptext[CPT_ABBR] = dict_get_string(tv->vval.v_dict,
						     (char_u *)"abbr", FALSE);
	cptext[CPT_MENU] = dict_get_string(tv->vval.v_dict,
						     (char_u *)"menu", FALSE);
	cptext[CPT_KIND] = dict_get_string(tv->vval.v_dict,
						     (char_u *)"kind", FALSE);
	cptext[CPT_INFO] = dict_get_string(tv->vval.v_dict,
						     (char_u *)"info", FALSE);
	dict_get_tv(tv->vval.v_dict, (char_u *)"user_data", &user_data);
	if (dict_get_string(tv->vval.v_dict, (char_u *)"icase", FALSE) != NULL
			&& dict_get_number(tv->vval.v_dict, (char_u *)"icase"))
	    flags |= CP_ICASE;
	if (dict_get_string(tv->vval.v_dict, (char_u *)"dup", FALSE) != NULL)
	    dup = dict_get_number(tv->vval.v_dict, (char_u *)"dup");
	if (dict_get_string(tv->vval.v_dict, (char_u *)"empty", FALSE) != NULL)
	    empty = dict_get_number(tv->vval.v_dict, (char_u *)"empty");
	if (dict_get_string(tv->vval.v_dict, (char_u *)"equal", FALSE) != NULL
			&& dict_get_number(tv->vval.v_dict, (char_u *)"equal"))
	    flags |= CP_EQUAL;
    }
    else
    {
	word = tv_get_string_chk(tv);
	vim_memset(cptext, 0, sizeof(cptext));
    }
    if (word == NULL || (!empty && *word == NUL))
	return FAIL;
    return ins_compl_add(word, -1, NULL, cptext, &user_data, dir, flags, dup);
}

/*
 * Add completions from a list.
 */
    static void
ins_compl_add_list(list_T *list)
{
    listitem_T	*li;
    int		dir = compl_direction;

    // Go through the List with matches and add each of them.
    for (li = list->lv_first; li != NULL; li = li->li_next)
    {
	if (ins_compl_add_tv(&li->li_tv, dir) == OK)
	    // if dir was BACKWARD then honor it just once
	    dir = FORWARD;
	else if (did_emsg)
	    break;
    }
}

/*
 * Add completions from a dict.
 */
    static void
ins_compl_add_dict(dict_T *dict)
{
    dictitem_T	*di_refresh;
    dictitem_T	*di_words;

    // Check for optional "refresh" item.
    compl_opt_refresh_always = FALSE;
    di_refresh = dict_find(dict, (char_u *)"refresh", 7);
    if (di_refresh != NULL && di_refresh->di_tv.v_type == VAR_STRING)
    {
	char_u	*v = di_refresh->di_tv.vval.v_string;

	if (v != NULL && STRCMP(v, (char_u *)"always") == 0)
	    compl_opt_refresh_always = TRUE;
    }

    // Add completions from a "words" list.
    di_words = dict_find(dict, (char_u *)"words", 5);
    if (di_words != NULL && di_words->di_tv.v_type == VAR_LIST)
	ins_compl_add_list(di_words->di_tv.vval.v_list);
}

/*
 * Start completion for the complete() function.
 * "startcol" is where the matched text starts (1 is first column).
 * "list" is the list of matches.
 */
    static void
set_completion(colnr_T startcol, list_T *list)
{
    int save_w_wrow = curwin->w_wrow;
    int save_w_leftcol = curwin->w_leftcol;
    int flags = CP_ORIGINAL_TEXT;

    // If already doing completions stop it.
    if (ctrl_x_mode != CTRL_X_NORMAL)
	ins_compl_prep(' ');
    ins_compl_clear();
    ins_compl_free();

    compl_direction = FORWARD;
    if (startcol > curwin->w_cursor.col)
	startcol = curwin->w_cursor.col;
    compl_col = startcol;
    compl_length = (int)curwin->w_cursor.col - (int)startcol;
    // compl_pattern doesn't need to be set
    compl_orig_text = vim_strnsave(ml_get_curline() + compl_col, compl_length);
    if (p_ic)
	flags |= CP_ICASE;
    if (compl_orig_text == NULL || ins_compl_add(compl_orig_text,
				  -1, NULL, NULL, NULL, 0, flags, FALSE) != OK)
	return;

    ctrl_x_mode = CTRL_X_EVAL;

    ins_compl_add_list(list);
    compl_matches = ins_compl_make_cyclic();
    compl_started = TRUE;
    compl_used_match = TRUE;
    compl_cont_status = 0;

    compl_curr_match = compl_first_match;
    if (compl_no_insert || compl_no_select)
    {
	ins_complete(K_DOWN, FALSE);
	if (compl_no_select)
	    // Down/Up has no real effect.
	    ins_complete(K_UP, FALSE);
    }
    else
	ins_complete(Ctrl_N, FALSE);
    compl_enter_selects = compl_no_insert;

    // Lazily show the popup menu, unless we got interrupted.
    if (!compl_interrupted)
	show_pum(save_w_wrow, save_w_leftcol);
    out_flush();
}

/*
 * "complete()" function
 */
    void
f_complete(typval_T *argvars, typval_T *rettv UNUSED)
{
    int	    startcol;

    if ((State & INSERT) == 0)
    {
	emsg(_("E785: complete() can only be used in Insert mode"));
	return;
    }

    // Check for undo allowed here, because if something was already inserted
    // the line was already saved for undo and this check isn't done.
    if (!undo_allowed())
	return;

    if (argvars[1].v_type != VAR_LIST || argvars[1].vval.v_list == NULL)
    {
	emsg(_(e_invarg));
	return;
    }

    startcol = (int)tv_get_number_chk(&argvars[0], NULL);
    if (startcol <= 0)
	return;

    set_completion(startcol - 1, argvars[1].vval.v_list);
}

/*
 * "complete_add()" function
 */
    void
f_complete_add(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = ins_compl_add_tv(&argvars[0], 0);
}

/*
 * "complete_check()" function
 */
    void
f_complete_check(typval_T *argvars UNUSED, typval_T *rettv)
{
    int		saved = RedrawingDisabled;

    RedrawingDisabled = 0;
    ins_compl_check_keys(0, TRUE);
    rettv->vval.v_number = ins_compl_interrupted();
    RedrawingDisabled = saved;
}

/*
 * Return Insert completion mode name string
 */
    static char_u *
ins_compl_mode(void)
{
    if (ctrl_x_mode == CTRL_X_NOT_DEFINED_YET || compl_started)
	return (char_u *)ctrl_x_mode_names[ctrl_x_mode & ~CTRL_X_WANT_IDENT];

    return (char_u *)"";
}

/*
 * Get complete information
 */
    static void
get_complete_info(list_T *what_list, dict_T *retdict)
{
    int		ret = OK;
    listitem_T	*item;
#define CI_WHAT_MODE		0x01
#define CI_WHAT_PUM_VISIBLE	0x02
#define CI_WHAT_ITEMS		0x04
#define CI_WHAT_SELECTED	0x08
#define CI_WHAT_INSERTED	0x10
#define CI_WHAT_ALL		0xff
    int		what_flag;

    if (what_list == NULL)
	what_flag = CI_WHAT_ALL;
    else
    {
	what_flag = 0;
	for (item = what_list->lv_first; item != NULL; item = item->li_next)
	{
	    char_u *what = tv_get_string(&item->li_tv);

	    if (STRCMP(what, "mode") == 0)
		what_flag |= CI_WHAT_MODE;
	    else if (STRCMP(what, "pum_visible") == 0)
		what_flag |= CI_WHAT_PUM_VISIBLE;
	    else if (STRCMP(what, "items") == 0)
		what_flag |= CI_WHAT_ITEMS;
	    else if (STRCMP(what, "selected") == 0)
		what_flag |= CI_WHAT_SELECTED;
	    else if (STRCMP(what, "inserted") == 0)
		what_flag |= CI_WHAT_INSERTED;
	}
    }

    if (ret == OK && (what_flag & CI_WHAT_MODE))
	ret = dict_add_string(retdict, "mode", ins_compl_mode());

    if (ret == OK && (what_flag & CI_WHAT_PUM_VISIBLE))
	ret = dict_add_number(retdict, "pum_visible", pum_visible());

    if (ret == OK && (what_flag & CI_WHAT_ITEMS))
    {
	list_T	    *li;
	dict_T	    *di;
	compl_T     *match;

	li = list_alloc();
	if (li == NULL)
	    return;
	ret = dict_add_list(retdict, "items", li);
	if (ret == OK && compl_first_match != NULL)
	{
	    match = compl_first_match;
	    do
	    {
		if (!(match->cp_flags & CP_ORIGINAL_TEXT))
		{
		    di = dict_alloc();
		    if (di == NULL)
			return;
		    ret = list_append_dict(li, di);
		    if (ret != OK)
			return;
		    dict_add_string(di, "word", match->cp_str);
		    dict_add_string(di, "abbr", match->cp_text[CPT_ABBR]);
		    dict_add_string(di, "menu", match->cp_text[CPT_MENU]);
		    dict_add_string(di, "kind", match->cp_text[CPT_KIND]);
		    dict_add_string(di, "info", match->cp_text[CPT_INFO]);
		    if (match->cp_user_data.v_type == VAR_UNKNOWN)
			// Add an empty string for backwards compatibility
			dict_add_string(di, "user_data", (char_u *)"");
		    else
			dict_add_tv(di, "user_data", &match->cp_user_data);
		}
		match = match->cp_next;
	    }
	    while (match != NULL && match != compl_first_match);
	}
    }

    if (ret == OK && (what_flag & CI_WHAT_SELECTED))
	ret = dict_add_number(retdict, "selected", (compl_curr_match != NULL) ?
			compl_curr_match->cp_number - 1 : -1);

    // TODO
    // if (ret == OK && (what_flag & CI_WHAT_INSERTED))
}

/*
 * "complete_info()" function
 */
    void
f_complete_info(typval_T *argvars, typval_T *rettv)
{
    list_T	*what_list = NULL;

    if (rettv_dict_alloc(rettv) != OK)
	return;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	if (argvars[0].v_type != VAR_LIST)
	{
	    emsg(_(e_listreq));
	    return;
	}
	what_list = argvars[0].vval.v_list;
    }
    get_complete_info(what_list, rettv->vval.v_dict);
}
#endif

/*
 * Get the next expansion(s), using "compl_pattern".
 * The search starts at position "ini" in curbuf and in the direction
 * compl_direction.
 * When "compl_started" is FALSE start at that position, otherwise continue
 * where we stopped searching before.
 * This may return before finding all the matches.
 * Return the total number of matches or -1 if still unknown -- Acevedo
 */
    static int
ins_compl_get_exp(pos_T *ini)
{
    static pos_T	first_match_pos;
    static pos_T	last_match_pos;
    static char_u	*e_cpt = (char_u *)"";	// curr. entry in 'complete'
    static int		found_all = FALSE;	// Found all matches of a
						// certain type.
    static buf_T	*ins_buf = NULL;	// buffer being scanned

    pos_T	*pos;
    char_u	**matches;
    int		save_p_scs;
    int		save_p_ws;
    int		save_p_ic;
    int		i;
    int		num_matches;
    int		len;
    int		found_new_match;
    int		type = ctrl_x_mode;
    char_u	*ptr;
    char_u	*dict = NULL;
    int		dict_f = 0;
    int		set_match_pos;

    if (!compl_started)
    {
	FOR_ALL_BUFFERS(ins_buf)
	    ins_buf->b_scanned = 0;
	found_all = FALSE;
	ins_buf = curbuf;
	e_cpt = (compl_cont_status & CONT_LOCAL)
					    ? (char_u *)"." : curbuf->b_p_cpt;
	last_match_pos = first_match_pos = *ini;
    }
    else if (ins_buf != curbuf && !buf_valid(ins_buf))
	ins_buf = curbuf;  // In case the buffer was wiped out.

    compl_old_match = compl_curr_match;	// remember the last current match
    pos = (compl_direction == FORWARD) ? &last_match_pos : &first_match_pos;

    // For ^N/^P loop over all the flags/windows/buffers in 'complete'.
    for (;;)
    {
	found_new_match = FAIL;
	set_match_pos = FALSE;

	// For ^N/^P pick a new entry from e_cpt if compl_started is off,
	// or if found_all says this entry is done.  For ^X^L only use the
	// entries from 'complete' that look in loaded buffers.
	if ((ctrl_x_mode == CTRL_X_NORMAL
		    || ctrl_x_mode_line_or_eval())
					&& (!compl_started || found_all))
	{
	    found_all = FALSE;
	    while (*e_cpt == ',' || *e_cpt == ' ')
		e_cpt++;
	    if (*e_cpt == '.' && !curbuf->b_scanned)
	    {
		ins_buf = curbuf;
		first_match_pos = *ini;
		// Move the cursor back one character so that ^N can match the
		// word immediately after the cursor.
		if (ctrl_x_mode == CTRL_X_NORMAL && dec(&first_match_pos) < 0)
		{
		    // Move the cursor to after the last character in the
		    // buffer, so that word at start of buffer is found
		    // correctly.
		    first_match_pos.lnum = ins_buf->b_ml.ml_line_count;
		    first_match_pos.col =
				 (colnr_T)STRLEN(ml_get(first_match_pos.lnum));
		}
		last_match_pos = first_match_pos;
		type = 0;

		// Remember the first match so that the loop stops when we
		// wrap and come back there a second time.
		set_match_pos = TRUE;
	    }
	    else if (vim_strchr((char_u *)"buwU", *e_cpt) != NULL
		 && (ins_buf = ins_compl_next_buf(ins_buf, *e_cpt)) != curbuf)
	    {
		// Scan a buffer, but not the current one.
		if (ins_buf->b_ml.ml_mfp != NULL)   // loaded buffer
		{
		    compl_started = TRUE;
		    first_match_pos.col = last_match_pos.col = 0;
		    first_match_pos.lnum = ins_buf->b_ml.ml_line_count + 1;
		    last_match_pos.lnum = 0;
		    type = 0;
		}
		else	// unloaded buffer, scan like dictionary
		{
		    found_all = TRUE;
		    if (ins_buf->b_fname == NULL)
			continue;
		    type = CTRL_X_DICTIONARY;
		    dict = ins_buf->b_fname;
		    dict_f = DICT_EXACT;
		}
		vim_snprintf((char *)IObuff, IOSIZE, _("Scanning: %s"),
			ins_buf->b_fname == NULL
			    ? buf_spname(ins_buf)
			    : ins_buf->b_sfname == NULL
				? ins_buf->b_fname
				: ins_buf->b_sfname);
		(void)msg_trunc_attr((char *)IObuff, TRUE, HL_ATTR(HLF_R));
	    }
	    else if (*e_cpt == NUL)
		break;
	    else
	    {
		if (ctrl_x_mode_line_or_eval())
		    type = -1;
		else if (*e_cpt == 'k' || *e_cpt == 's')
		{
		    if (*e_cpt == 'k')
			type = CTRL_X_DICTIONARY;
		    else
			type = CTRL_X_THESAURUS;
		    if (*++e_cpt != ',' && *e_cpt != NUL)
		    {
			dict = e_cpt;
			dict_f = DICT_FIRST;
		    }
		}
#ifdef FEAT_FIND_ID
		else if (*e_cpt == 'i')
		    type = CTRL_X_PATH_PATTERNS;
		else if (*e_cpt == 'd')
		    type = CTRL_X_PATH_DEFINES;
#endif
		else if (*e_cpt == ']' || *e_cpt == 't')
		{
		    type = CTRL_X_TAGS;
		    vim_snprintf((char *)IObuff, IOSIZE, _("Scanning tags."));
		    (void)msg_trunc_attr((char *)IObuff, TRUE, HL_ATTR(HLF_R));
		}
		else
		    type = -1;

		// in any case e_cpt is advanced to the next entry
		(void)copy_option_part(&e_cpt, IObuff, IOSIZE, ",");

		found_all = TRUE;
		if (type == -1)
		    continue;
	    }
	}

	// If complete() was called then compl_pattern has been reset.  The
	// following won't work then, bail out.
	if (compl_pattern == NULL)
	    break;

	switch (type)
	{
	case -1:
	    break;
#ifdef FEAT_FIND_ID
	case CTRL_X_PATH_PATTERNS:
	case CTRL_X_PATH_DEFINES:
	    find_pattern_in_path(compl_pattern, compl_direction,
				 (int)STRLEN(compl_pattern), FALSE, FALSE,
				 (type == CTRL_X_PATH_DEFINES
				  && !(compl_cont_status & CONT_SOL))
				 ? FIND_DEFINE : FIND_ANY, 1L, ACTION_EXPAND,
				 (linenr_T)1, (linenr_T)MAXLNUM);
	    break;
#endif

	case CTRL_X_DICTIONARY:
	case CTRL_X_THESAURUS:
	    ins_compl_dictionaries(
		    dict != NULL ? dict
			 : (type == CTRL_X_THESAURUS
			     ? (*curbuf->b_p_tsr == NUL
				 ? p_tsr
				 : curbuf->b_p_tsr)
			     : (*curbuf->b_p_dict == NUL
				 ? p_dict
				 : curbuf->b_p_dict)),
			    compl_pattern,
				 dict != NULL ? dict_f
					       : 0, type == CTRL_X_THESAURUS);
	    dict = NULL;
	    break;

	case CTRL_X_TAGS:
	    // set p_ic according to p_ic, p_scs and pat for find_tags().
	    save_p_ic = p_ic;
	    p_ic = ignorecase(compl_pattern);

	    // Find up to TAG_MANY matches.  Avoids that an enormous number
	    // of matches is found when compl_pattern is empty
	    g_tag_at_cursor = TRUE;
	    if (find_tags(compl_pattern, &num_matches, &matches,
		    TAG_REGEXP | TAG_NAMES | TAG_NOIC | TAG_INS_COMP
		    | (ctrl_x_mode != CTRL_X_NORMAL ? TAG_VERBOSE : 0),
		    TAG_MANY, curbuf->b_ffname) == OK && num_matches > 0)
		ins_compl_add_matches(num_matches, matches, p_ic);
	    g_tag_at_cursor = FALSE;
	    p_ic = save_p_ic;
	    break;

	case CTRL_X_FILES:
	    if (expand_wildcards(1, &compl_pattern, &num_matches, &matches,
				  EW_FILE|EW_DIR|EW_ADDSLASH|EW_SILENT) == OK)
	    {

		// May change home directory back to "~".
		tilde_replace(compl_pattern, num_matches, matches);
#ifdef BACKSLASH_IN_FILENAME
		if (curbuf->b_p_csl[0] != NUL)
		{
		    int	    i;

		    for (i = 0; i < num_matches; ++i)
		    {
			char_u	*ptr = matches[i];

			while (*ptr != NUL)
			{
			    if (curbuf->b_p_csl[0] == 's' && *ptr == '\\')
				*ptr = '/';
			    else if (curbuf->b_p_csl[0] == 'b' && *ptr == '/')
				*ptr = '\\';
			    ptr += (*mb_ptr2len)(ptr);
			}
		    }
		}
#endif
		ins_compl_add_matches(num_matches, matches, p_fic || p_wic);
	    }
	    break;

	case CTRL_X_CMDLINE:
	    if (expand_cmdline(&compl_xp, compl_pattern,
			(int)STRLEN(compl_pattern),
					 &num_matches, &matches) == EXPAND_OK)
		ins_compl_add_matches(num_matches, matches, FALSE);
	    break;

#ifdef FEAT_COMPL_FUNC
	case CTRL_X_FUNCTION:
	case CTRL_X_OMNI:
	    expand_by_function(type, compl_pattern);
	    break;
#endif

	case CTRL_X_SPELL:
#ifdef FEAT_SPELL
	    num_matches = expand_spelling(first_match_pos.lnum,
						     compl_pattern, &matches);
	    if (num_matches > 0)
		ins_compl_add_matches(num_matches, matches, p_ic);
#endif
	    break;

	default:	// normal ^P/^N and ^X^L
	    // If 'infercase' is set, don't use 'smartcase' here
	    save_p_scs = p_scs;
	    if (ins_buf->b_p_inf)
		p_scs = FALSE;

	    //	Buffers other than curbuf are scanned from the beginning or the
	    //	end but never from the middle, thus setting nowrapscan in this
	    //	buffer is a good idea, on the other hand, we always set
	    //	wrapscan for curbuf to avoid missing matches -- Acevedo,Webb
	    save_p_ws = p_ws;
	    if (ins_buf != curbuf)
		p_ws = FALSE;
	    else if (*e_cpt == '.')
		p_ws = TRUE;
	    for (;;)
	    {
		int	cont_s_ipos = FALSE;

		++msg_silent;  // Don't want messages for wrapscan.

		// ctrl_x_mode_line_or_eval() || word-wise search that
		// has added a word that was at the beginning of the line
		if (ctrl_x_mode_line_or_eval()
			|| (compl_cont_status & CONT_SOL))
		    found_new_match = search_for_exact_line(ins_buf, pos,
					      compl_direction, compl_pattern);
		else
		    found_new_match = searchit(NULL, ins_buf, pos, NULL,
							      compl_direction,
				 compl_pattern, 1L, SEARCH_KEEP + SEARCH_NFMSG,
								RE_LAST, NULL);
		--msg_silent;
		if (!compl_started || set_match_pos)
		{
		    // set "compl_started" even on fail
		    compl_started = TRUE;
		    first_match_pos = *pos;
		    last_match_pos = *pos;
		    set_match_pos = FALSE;
		}
		else if (first_match_pos.lnum == last_match_pos.lnum
				 && first_match_pos.col == last_match_pos.col)
		    found_new_match = FAIL;
		if (found_new_match == FAIL)
		{
		    if (ins_buf == curbuf)
			found_all = TRUE;
		    break;
		}

		// when ADDING, the text before the cursor matches, skip it
		if (	(compl_cont_status & CONT_ADDING) && ins_buf == curbuf
			&& ini->lnum == pos->lnum
			&& ini->col  == pos->col)
		    continue;
		ptr = ml_get_buf(ins_buf, pos->lnum, FALSE) + pos->col;
		if (ctrl_x_mode_line_or_eval())
		{
		    if (compl_cont_status & CONT_ADDING)
		    {
			if (pos->lnum >= ins_buf->b_ml.ml_line_count)
			    continue;
			ptr = ml_get_buf(ins_buf, pos->lnum + 1, FALSE);
			if (!p_paste)
			    ptr = skipwhite(ptr);
		    }
		    len = (int)STRLEN(ptr);
		}
		else
		{
		    char_u	*tmp_ptr = ptr;

		    if (compl_cont_status & CONT_ADDING)
		    {
			tmp_ptr += compl_length;
			// Skip if already inside a word.
			if (vim_iswordp(tmp_ptr))
			    continue;
			// Find start of next word.
			tmp_ptr = find_word_start(tmp_ptr);
		    }
		    // Find end of this word.
		    tmp_ptr = find_word_end(tmp_ptr);
		    len = (int)(tmp_ptr - ptr);

		    if ((compl_cont_status & CONT_ADDING)
						       && len == compl_length)
		    {
			if (pos->lnum < ins_buf->b_ml.ml_line_count)
			{
			    // Try next line, if any. the new word will be
			    // "join" as if the normal command "J" was used.
			    // IOSIZE is always greater than
			    // compl_length, so the next STRNCPY always
			    // works -- Acevedo
			    STRNCPY(IObuff, ptr, len);
			    ptr = ml_get_buf(ins_buf, pos->lnum + 1, FALSE);
			    tmp_ptr = ptr = skipwhite(ptr);
			    // Find start of next word.
			    tmp_ptr = find_word_start(tmp_ptr);
			    // Find end of next word.
			    tmp_ptr = find_word_end(tmp_ptr);
			    if (tmp_ptr > ptr)
			    {
				if (*ptr != ')' && IObuff[len - 1] != TAB)
				{
				    if (IObuff[len - 1] != ' ')
					IObuff[len++] = ' ';
				    // IObuf =~ "\k.* ", thus len >= 2
				    if (p_js
					&& (IObuff[len - 2] == '.'
					    || (vim_strchr(p_cpo, CPO_JOINSP)
								       == NULL
						&& (IObuff[len - 2] == '?'
						 || IObuff[len - 2] == '!'))))
					IObuff[len++] = ' ';
				}
				// copy as much as possible of the new word
				if (tmp_ptr - ptr >= IOSIZE - len)
				    tmp_ptr = ptr + IOSIZE - len - 1;
				STRNCPY(IObuff + len, ptr, tmp_ptr - ptr);
				len += (int)(tmp_ptr - ptr);
				cont_s_ipos = TRUE;
			    }
			    IObuff[len] = NUL;
			    ptr = IObuff;
			}
			if (len == compl_length)
			    continue;
		    }
		}
		if (ins_compl_add_infercase(ptr, len, p_ic,
				 ins_buf == curbuf ? NULL : ins_buf->b_sfname,
					   0, cont_s_ipos) != NOTDONE)
		{
		    found_new_match = OK;
		    break;
		}
	    }
	    p_scs = save_p_scs;
	    p_ws = save_p_ws;
	}

	// check if compl_curr_match has changed, (e.g. other type of
	// expansion added something)
	if (type != 0 && compl_curr_match != compl_old_match)
	    found_new_match = OK;

	// break the loop for specialized modes (use 'complete' just for the
	// generic ctrl_x_mode == CTRL_X_NORMAL) or when we've found a new
	// match
	if ((ctrl_x_mode != CTRL_X_NORMAL
		    && !ctrl_x_mode_line_or_eval()) || found_new_match != FAIL)
	{
	    if (got_int)
		break;
	    // Fill the popup menu as soon as possible.
	    if (type != -1)
		ins_compl_check_keys(0, FALSE);

	    if ((ctrl_x_mode != CTRL_X_NORMAL
			&& !ctrl_x_mode_line_or_eval()) || compl_interrupted)
		break;
	    compl_started = TRUE;
	}
	else
	{
	    // Mark a buffer scanned when it has been scanned completely
	    if (type == 0 || type == CTRL_X_PATH_PATTERNS)
		ins_buf->b_scanned = TRUE;

	    compl_started = FALSE;
	}
    }
    compl_started = TRUE;

    if ((ctrl_x_mode == CTRL_X_NORMAL || ctrl_x_mode_line_or_eval())
	    && *e_cpt == NUL)		// Got to end of 'complete'
	found_new_match = FAIL;

    i = -1;		// total of matches, unknown
    if (found_new_match == FAIL || (ctrl_x_mode != CTRL_X_NORMAL
					       && !ctrl_x_mode_line_or_eval()))
	i = ins_compl_make_cyclic();

    if (compl_old_match != NULL)
    {
	// If several matches were added (FORWARD) or the search failed and has
	// just been made cyclic then we have to move compl_curr_match to the
	// next or previous entry (if any) -- Acevedo
	compl_curr_match = compl_direction == FORWARD ? compl_old_match->cp_next
						    : compl_old_match->cp_prev;
	if (compl_curr_match == NULL)
	    compl_curr_match = compl_old_match;
    }
    return i;
}

/*
 * Delete the old text being completed.
 */
    void
ins_compl_delete(void)
{
    int	    col;

    // In insert mode: Delete the typed part.
    // In replace mode: Put the old characters back, if any.
    col = compl_col + (compl_cont_status & CONT_ADDING ? compl_length : 0);
    if ((int)curwin->w_cursor.col > col)
    {
	if (stop_arrow() == FAIL)
	    return;
	backspace_until_column(col);
    }

    // TODO: is this sufficient for redrawing?  Redrawing everything causes
    // flicker, thus we can't do that.
    changed_cline_bef_curs();
#ifdef FEAT_EVAL
    // clear v:completed_item
    set_vim_var_dict(VV_COMPLETED_ITEM, dict_alloc_lock(VAR_FIXED));
#endif
}

/*
 * Insert the new text being completed.
 * "in_compl_func" is TRUE when called from complete_check().
 */
    void
ins_compl_insert(int in_compl_func)
{
    ins_bytes(compl_shown_match->cp_str + ins_compl_len());
    if (compl_shown_match->cp_flags & CP_ORIGINAL_TEXT)
	compl_used_match = FALSE;
    else
	compl_used_match = TRUE;
#ifdef FEAT_EVAL
    {
	dict_T *dict = ins_compl_dict_alloc(compl_shown_match);

	set_vim_var_dict(VV_COMPLETED_ITEM, dict);
    }
#endif
    if (!in_compl_func)
	compl_curr_match = compl_shown_match;
}

/*
 * Fill in the next completion in the current direction.
 * If "allow_get_expansion" is TRUE, then we may call ins_compl_get_exp() to
 * get more completions.  If it is FALSE, then we just do nothing when there
 * are no more completions in a given direction.  The latter case is used when
 * we are still in the middle of finding completions, to allow browsing
 * through the ones found so far.
 * Return the total number of matches, or -1 if still unknown -- webb.
 *
 * compl_curr_match is currently being used by ins_compl_get_exp(), so we use
 * compl_shown_match here.
 *
 * Note that this function may be called recursively once only.  First with
 * "allow_get_expansion" TRUE, which calls ins_compl_get_exp(), which in turn
 * calls this function with "allow_get_expansion" FALSE.
 */
    static int
ins_compl_next(
    int	    allow_get_expansion,
    int	    count,		// repeat completion this many times; should
				// be at least 1
    int	    insert_match,	// Insert the newly selected match
    int	    in_compl_func)	// called from complete_check()
{
    int	    num_matches = -1;
    int	    todo = count;
    compl_T *found_compl = NULL;
    int	    found_end = FALSE;
    int	    advance;
    int	    started = compl_started;

    // When user complete function return -1 for findstart which is next
    // time of 'always', compl_shown_match become NULL.
    if (compl_shown_match == NULL)
	return -1;

    if (compl_leader != NULL
			&& (compl_shown_match->cp_flags & CP_ORIGINAL_TEXT) == 0)
    {
	// Set "compl_shown_match" to the actually shown match, it may differ
	// when "compl_leader" is used to omit some of the matches.
	while (!ins_compl_equal(compl_shown_match,
				      compl_leader, (int)STRLEN(compl_leader))
		&& compl_shown_match->cp_next != NULL
		&& compl_shown_match->cp_next != compl_first_match)
	    compl_shown_match = compl_shown_match->cp_next;

	// If we didn't find it searching forward, and compl_shows_dir is
	// backward, find the last match.
	if (compl_shows_dir == BACKWARD
		&& !ins_compl_equal(compl_shown_match,
				      compl_leader, (int)STRLEN(compl_leader))
		&& (compl_shown_match->cp_next == NULL
		    || compl_shown_match->cp_next == compl_first_match))
	{
	    while (!ins_compl_equal(compl_shown_match,
				      compl_leader, (int)STRLEN(compl_leader))
		    && compl_shown_match->cp_prev != NULL
		    && compl_shown_match->cp_prev != compl_first_match)
		compl_shown_match = compl_shown_match->cp_prev;
	}
    }

    if (allow_get_expansion && insert_match
	    && (!(compl_get_longest || compl_restarting) || compl_used_match))
	// Delete old text to be replaced
	ins_compl_delete();

    // When finding the longest common text we stick at the original text,
    // don't let CTRL-N or CTRL-P move to the first match.
    advance = count != 1 || !allow_get_expansion || !compl_get_longest;

    // When restarting the search don't insert the first match either.
    if (compl_restarting)
    {
	advance = FALSE;
	compl_restarting = FALSE;
    }

    // Repeat this for when <PageUp> or <PageDown> is typed.  But don't wrap
    // around.
    while (--todo >= 0)
    {
	if (compl_shows_dir == FORWARD && compl_shown_match->cp_next != NULL)
	{
	    compl_shown_match = compl_shown_match->cp_next;
	    found_end = (compl_first_match != NULL
			   && (compl_shown_match->cp_next == compl_first_match
			       || compl_shown_match == compl_first_match));
	}
	else if (compl_shows_dir == BACKWARD
					&& compl_shown_match->cp_prev != NULL)
	{
	    found_end = (compl_shown_match == compl_first_match);
	    compl_shown_match = compl_shown_match->cp_prev;
	    found_end |= (compl_shown_match == compl_first_match);
	}
	else
	{
	    if (!allow_get_expansion)
	    {
		if (advance)
		{
		    if (compl_shows_dir == BACKWARD)
			compl_pending -= todo + 1;
		    else
			compl_pending += todo + 1;
		}
		return -1;
	    }

	    if (!compl_no_select && advance)
	    {
		if (compl_shows_dir == BACKWARD)
		    --compl_pending;
		else
		    ++compl_pending;
	    }

	    // Find matches.
	    num_matches = ins_compl_get_exp(&compl_startpos);

	    // handle any pending completions
	    while (compl_pending != 0 && compl_direction == compl_shows_dir
								   && advance)
	    {
		if (compl_pending > 0 && compl_shown_match->cp_next != NULL)
		{
		    compl_shown_match = compl_shown_match->cp_next;
		    --compl_pending;
		}
		if (compl_pending < 0 && compl_shown_match->cp_prev != NULL)
		{
		    compl_shown_match = compl_shown_match->cp_prev;
		    ++compl_pending;
		}
		else
		    break;
	    }
	    found_end = FALSE;
	}
	if ((compl_shown_match->cp_flags & CP_ORIGINAL_TEXT) == 0
		&& compl_leader != NULL
		&& !ins_compl_equal(compl_shown_match,
				     compl_leader, (int)STRLEN(compl_leader)))
	    ++todo;
	else
	    // Remember a matching item.
	    found_compl = compl_shown_match;

	// Stop at the end of the list when we found a usable match.
	if (found_end)
	{
	    if (found_compl != NULL)
	    {
		compl_shown_match = found_compl;
		break;
	    }
	    todo = 1;	    // use first usable match after wrapping around
	}
    }

    // Insert the text of the new completion, or the compl_leader.
    if (compl_no_insert && !started)
    {
	ins_bytes(compl_orig_text + ins_compl_len());
	compl_used_match = FALSE;
    }
    else if (insert_match)
    {
	if (!compl_get_longest || compl_used_match)
	    ins_compl_insert(in_compl_func);
	else
	    ins_bytes(compl_leader + ins_compl_len());
    }
    else
	compl_used_match = FALSE;

    if (!allow_get_expansion)
    {
	// may undisplay the popup menu first
	ins_compl_upd_pum();

	if (pum_enough_matches())
	    // Will display the popup menu, don't redraw yet to avoid flicker.
	    pum_call_update_screen();
	else
	    // Not showing the popup menu yet, redraw to show the user what was
	    // inserted.
	    update_screen(0);

	// display the updated popup menu
	ins_compl_show_pum();
#ifdef FEAT_GUI
	if (gui.in_use)
	{
	    // Show the cursor after the match, not after the redrawn text.
	    setcursor();
	    out_flush_cursor(FALSE, FALSE);
	}
#endif

	// Delete old text to be replaced, since we're still searching and
	// don't want to match ourselves!
	ins_compl_delete();
    }

    // Enter will select a match when the match wasn't inserted and the popup
    // menu is visible.
    if (compl_no_insert && !started)
	compl_enter_selects = TRUE;
    else
	compl_enter_selects = !insert_match && compl_match_array != NULL;

    // Show the file name for the match (if any)
    // Truncate the file name to avoid a wait for return.
    if (compl_shown_match->cp_fname != NULL)
    {
	char	*lead = _("match in file");
	int	space = sc_col - vim_strsize((char_u *)lead) - 2;
	char_u	*s;
	char_u	*e;

	if (space > 0)
	{
	    // We need the tail that fits.  With double-byte encoding going
	    // back from the end is very slow, thus go from the start and keep
	    // the text that fits in "space" between "s" and "e".
	    for (s = e = compl_shown_match->cp_fname; *e != NUL; MB_PTR_ADV(e))
	    {
		space -= ptr2cells(e);
		while (space < 0)
		{
		    space += ptr2cells(s);
		    MB_PTR_ADV(s);
		}
	    }
	    vim_snprintf((char *)IObuff, IOSIZE, "%s %s%s", lead,
				s > compl_shown_match->cp_fname ? "<" : "", s);
	    msg((char *)IObuff);
	    redraw_cmdline = FALSE;	    // don't overwrite!
	}
    }

    return num_matches;
}

/*
 * Call this while finding completions, to check whether the user has hit a key
 * that should change the currently displayed completion, or exit completion
 * mode.  Also, when compl_pending is not zero, show a completion as soon as
 * possible. -- webb
 * "frequency" specifies out of how many calls we actually check.
 * "in_compl_func" is TRUE when called from complete_check(), don't set
 * compl_curr_match.
 */
    void
ins_compl_check_keys(int frequency, int in_compl_func)
{
    static int	count = 0;
    int		c;

    // Don't check when reading keys from a script, :normal or feedkeys().
    // That would break the test scripts.  But do check for keys when called
    // from complete_check().
    if (!in_compl_func && (using_script() || ex_normal_busy))
	return;

    // Only do this at regular intervals
    if (++count < frequency)
	return;
    count = 0;

    // Check for a typed key.  Do use mappings, otherwise vim_is_ctrl_x_key()
    // can't do its work correctly.
    c = vpeekc_any();
    if (c != NUL)
    {
	if (vim_is_ctrl_x_key(c) && c != Ctrl_X && c != Ctrl_R)
	{
	    c = safe_vgetc();	// Eat the character
	    compl_shows_dir = ins_compl_key2dir(c);
	    (void)ins_compl_next(FALSE, ins_compl_key2count(c),
				      c != K_UP && c != K_DOWN, in_compl_func);
	}
	else
	{
	    // Need to get the character to have KeyTyped set.  We'll put it
	    // back with vungetc() below.  But skip K_IGNORE.
	    c = safe_vgetc();
	    if (c != K_IGNORE)
	    {
		// Don't interrupt completion when the character wasn't typed,
		// e.g., when doing @q to replay keys.
		if (c != Ctrl_R && KeyTyped)
		    compl_interrupted = TRUE;

		vungetc(c);
	    }
	}
    }
    if (compl_pending != 0 && !got_int && !compl_no_insert)
    {
	int todo = compl_pending > 0 ? compl_pending : -compl_pending;

	compl_pending = 0;
	(void)ins_compl_next(FALSE, todo, TRUE, in_compl_func);
    }
}

/*
 * Decide the direction of Insert mode complete from the key typed.
 * Returns BACKWARD or FORWARD.
 */
    static int
ins_compl_key2dir(int c)
{
    if (c == Ctrl_P || c == Ctrl_L
	    || c == K_PAGEUP || c == K_KPAGEUP || c == K_S_UP || c == K_UP)
	return BACKWARD;
    return FORWARD;
}

/*
 * Return TRUE for keys that are used for completion only when the popup menu
 * is visible.
 */
    static int
ins_compl_pum_key(int c)
{
    return pum_visible() && (c == K_PAGEUP || c == K_KPAGEUP || c == K_S_UP
		     || c == K_PAGEDOWN || c == K_KPAGEDOWN || c == K_S_DOWN
		     || c == K_UP || c == K_DOWN);
}

/*
 * Decide the number of completions to move forward.
 * Returns 1 for most keys, height of the popup menu for page-up/down keys.
 */
    static int
ins_compl_key2count(int c)
{
    int		h;

    if (ins_compl_pum_key(c) && c != K_UP && c != K_DOWN)
    {
	h = pum_get_height();
	if (h > 3)
	    h -= 2; // keep some context
	return h;
    }
    return 1;
}

/*
 * Return TRUE if completion with "c" should insert the match, FALSE if only
 * to change the currently selected completion.
 */
    static int
ins_compl_use_match(int c)
{
    switch (c)
    {
	case K_UP:
	case K_DOWN:
	case K_PAGEDOWN:
	case K_KPAGEDOWN:
	case K_S_DOWN:
	case K_PAGEUP:
	case K_KPAGEUP:
	case K_S_UP:
	    return FALSE;
    }
    return TRUE;
}

/*
 * Do Insert mode completion.
 * Called when character "c" was typed, which has a meaning for completion.
 * Returns OK if completion was done, FAIL if something failed (out of mem).
 */
    int
ins_complete(int c, int enable_pum)
{
    char_u	*line;
    int		startcol = 0;	    // column where searched text starts
    colnr_T	curs_col;	    // cursor column
    int		n;
    int		save_w_wrow;
    int		save_w_leftcol;
    int		insert_match;
#ifdef FEAT_COMPL_FUNC
    int		save_did_ai = did_ai;
#endif
    int		flags = CP_ORIGINAL_TEXT;

    compl_direction = ins_compl_key2dir(c);
    insert_match = ins_compl_use_match(c);

    if (!compl_started)
    {
	// First time we hit ^N or ^P (in a row, I mean)

	did_ai = FALSE;
#ifdef FEAT_SMARTINDENT
	did_si = FALSE;
	can_si = FALSE;
	can_si_back = FALSE;
#endif
	if (stop_arrow() == FAIL)
	    return FAIL;

	line = ml_get(curwin->w_cursor.lnum);
	curs_col = curwin->w_cursor.col;
	compl_pending = 0;

	// If this same ctrl_x_mode has been interrupted use the text from
	// "compl_startpos" to the cursor as a pattern to add a new word
	// instead of expand the one before the cursor, in word-wise if
	// "compl_startpos" is not in the same line as the cursor then fix it
	// (the line has been split because it was longer than 'tw').  if SOL
	// is set then skip the previous pattern, a word at the beginning of
	// the line has been inserted, we'll look for that  -- Acevedo.
	if ((compl_cont_status & CONT_INTRPT) == CONT_INTRPT
					    && compl_cont_mode == ctrl_x_mode)
	{
	    // it is a continued search
	    compl_cont_status &= ~CONT_INTRPT;	// remove INTRPT
	    if (ctrl_x_mode == CTRL_X_NORMAL
		    || ctrl_x_mode == CTRL_X_PATH_PATTERNS
		    || ctrl_x_mode == CTRL_X_PATH_DEFINES)
	    {
		if (compl_startpos.lnum != curwin->w_cursor.lnum)
		{
		    // line (probably) wrapped, set compl_startpos to the
		    // first non_blank in the line, if it is not a wordchar
		    // include it to get a better pattern, but then we don't
		    // want the "\\<" prefix, check it bellow
		    compl_col = (colnr_T)getwhitecols(line);
		    compl_startpos.col = compl_col;
		    compl_startpos.lnum = curwin->w_cursor.lnum;
		    compl_cont_status &= ~CONT_SOL;   // clear SOL if present
		}
		else
		{
		    // S_IPOS was set when we inserted a word that was at the
		    // beginning of the line, which means that we'll go to SOL
		    // mode but first we need to redefine compl_startpos
		    if (compl_cont_status & CONT_S_IPOS)
		    {
			compl_cont_status |= CONT_SOL;
			compl_startpos.col = (colnr_T)(skipwhite(
						line + compl_length
						+ compl_startpos.col) - line);
		    }
		    compl_col = compl_startpos.col;
		}
		compl_length = curwin->w_cursor.col - (int)compl_col;
		// IObuff is used to add a "word from the next line" would we
		// have enough space?  just being paranoid
#define	MIN_SPACE 75
		if (compl_length > (IOSIZE - MIN_SPACE))
		{
		    compl_cont_status &= ~CONT_SOL;
		    compl_length = (IOSIZE - MIN_SPACE);
		    compl_col = curwin->w_cursor.col - compl_length;
		}
		compl_cont_status |= CONT_ADDING | CONT_N_ADDS;
		if (compl_length < 1)
		    compl_cont_status &= CONT_LOCAL;
	    }
	    else if (ctrl_x_mode_line_or_eval())
		compl_cont_status = CONT_ADDING | CONT_N_ADDS;
	    else
		compl_cont_status = 0;
	}
	else
	    compl_cont_status &= CONT_LOCAL;

	if (!(compl_cont_status & CONT_ADDING))	// normal expansion
	{
	    compl_cont_mode = ctrl_x_mode;
	    if (ctrl_x_mode != CTRL_X_NORMAL)
		// Remove LOCAL if ctrl_x_mode != CTRL_X_NORMAL
		compl_cont_status = 0;
	    compl_cont_status |= CONT_N_ADDS;
	    compl_startpos = curwin->w_cursor;
	    startcol = (int)curs_col;
	    compl_col = 0;
	}

	// Work out completion pattern and original text -- webb
	if (ctrl_x_mode == CTRL_X_NORMAL || (ctrl_x_mode & CTRL_X_WANT_IDENT))
	{
	    if ((compl_cont_status & CONT_SOL)
		    || ctrl_x_mode == CTRL_X_PATH_DEFINES)
	    {
		if (!(compl_cont_status & CONT_ADDING))
		{
		    while (--startcol >= 0 && vim_isIDc(line[startcol]))
			;
		    compl_col += ++startcol;
		    compl_length = curs_col - startcol;
		}
		if (p_ic)
		    compl_pattern = str_foldcase(line + compl_col,
						       compl_length, NULL, 0);
		else
		    compl_pattern = vim_strnsave(line + compl_col,
								compl_length);
		if (compl_pattern == NULL)
		    return FAIL;
	    }
	    else if (compl_cont_status & CONT_ADDING)
	    {
		char_u	    *prefix = (char_u *)"\\<";

		// we need up to 2 extra chars for the prefix
		compl_pattern = alloc(quote_meta(NULL, line + compl_col,
							   compl_length) + 2);
		if (compl_pattern == NULL)
		    return FAIL;
		if (!vim_iswordp(line + compl_col)
			|| (compl_col > 0
			 && (vim_iswordp(mb_prevptr(line, line + compl_col)))))
		    prefix = (char_u *)"";
		STRCPY((char *)compl_pattern, prefix);
		(void)quote_meta(compl_pattern + STRLEN(prefix),
					      line + compl_col, compl_length);
	    }
	    else if (--startcol < 0
		    || !vim_iswordp(mb_prevptr(line, line + startcol + 1)))
	    {
		// Match any word of at least two chars
		compl_pattern = vim_strsave((char_u *)"\\<\\k\\k");
		if (compl_pattern == NULL)
		    return FAIL;
		compl_col += curs_col;
		compl_length = 0;
	    }
	    else
	    {
		// Search the point of change class of multibyte character
		// or not a word single byte character backward.
		if (has_mbyte)
		{
		    int base_class;
		    int head_off;

		    startcol -= (*mb_head_off)(line, line + startcol);
		    base_class = mb_get_class(line + startcol);
		    while (--startcol >= 0)
		    {
			head_off = (*mb_head_off)(line, line + startcol);
			if (base_class != mb_get_class(line + startcol
								  - head_off))
			    break;
			startcol -= head_off;
		    }
		}
		else
		    while (--startcol >= 0 && vim_iswordc(line[startcol]))
			;
		compl_col += ++startcol;
		compl_length = (int)curs_col - startcol;
		if (compl_length == 1)
		{
		    // Only match word with at least two chars -- webb
		    // there's no need to call quote_meta,
		    // alloc(7) is enough  -- Acevedo
		    compl_pattern = alloc(7);
		    if (compl_pattern == NULL)
			return FAIL;
		    STRCPY((char *)compl_pattern, "\\<");
		    (void)quote_meta(compl_pattern + 2, line + compl_col, 1);
		    STRCAT((char *)compl_pattern, "\\k");
		}
		else
		{
		    compl_pattern = alloc(quote_meta(NULL, line + compl_col,
							   compl_length) + 2);
		    if (compl_pattern == NULL)
			return FAIL;
		    STRCPY((char *)compl_pattern, "\\<");
		    (void)quote_meta(compl_pattern + 2, line + compl_col,
								compl_length);
		}
	    }
	}
	else if (ctrl_x_mode_line_or_eval())
	{
	    compl_col = (colnr_T)getwhitecols(line);
	    compl_length = (int)curs_col - (int)compl_col;
	    if (compl_length < 0)	// cursor in indent: empty pattern
		compl_length = 0;
	    if (p_ic)
		compl_pattern = str_foldcase(line + compl_col, compl_length,
								     NULL, 0);
	    else
		compl_pattern = vim_strnsave(line + compl_col, compl_length);
	    if (compl_pattern == NULL)
		return FAIL;
	}
	else if (ctrl_x_mode == CTRL_X_FILES)
	{
	    // Go back to just before the first filename character.
	    if (startcol > 0)
	    {
		char_u	*p = line + startcol;

		MB_PTR_BACK(line, p);
		while (p > line && vim_isfilec(PTR2CHAR(p)))
		    MB_PTR_BACK(line, p);
		if (p == line && vim_isfilec(PTR2CHAR(p)))
		    startcol = 0;
		else
		    startcol = (int)(p - line) + 1;
	    }

	    compl_col += startcol;
	    compl_length = (int)curs_col - startcol;
	    compl_pattern = addstar(line + compl_col, compl_length,
								EXPAND_FILES);
	    if (compl_pattern == NULL)
		return FAIL;
	}
	else if (ctrl_x_mode == CTRL_X_CMDLINE)
	{
	    compl_pattern = vim_strnsave(line, curs_col);
	    if (compl_pattern == NULL)
		return FAIL;
	    set_cmd_context(&compl_xp, compl_pattern,
				  (int)STRLEN(compl_pattern), curs_col, FALSE);
	    if (compl_xp.xp_context == EXPAND_UNSUCCESSFUL
		    || compl_xp.xp_context == EXPAND_NOTHING)
		// No completion possible, use an empty pattern to get a
		// "pattern not found" message.
		compl_col = curs_col;
	    else
		compl_col = (int)(compl_xp.xp_pattern - compl_pattern);
	    compl_length = curs_col - compl_col;
	}
	else if (ctrl_x_mode == CTRL_X_FUNCTION || ctrl_x_mode == CTRL_X_OMNI)
	{
#ifdef FEAT_COMPL_FUNC
	    // Call user defined function 'completefunc' with "a:findstart"
	    // set to 1 to obtain the length of text to use for completion.
	    typval_T	args[3];
	    int		col;
	    char_u	*funcname;
	    pos_T	pos;
	    win_T	*curwin_save;
	    buf_T	*curbuf_save;
	    int		save_State = State;

	    // Call 'completefunc' or 'omnifunc' and get pattern length as a
	    // string
	    funcname = ctrl_x_mode == CTRL_X_FUNCTION
					  ? curbuf->b_p_cfu : curbuf->b_p_ofu;
	    if (*funcname == NUL)
	    {
		semsg(_(e_notset), ctrl_x_mode == CTRL_X_FUNCTION
					     ? "completefunc" : "omnifunc");
		// restore did_ai, so that adding comment leader works
		did_ai = save_did_ai;
		return FAIL;
	    }

	    args[0].v_type = VAR_NUMBER;
	    args[0].vval.v_number = 1;
	    args[1].v_type = VAR_STRING;
	    args[1].vval.v_string = (char_u *)"";
	    args[2].v_type = VAR_UNKNOWN;
	    pos = curwin->w_cursor;
	    curwin_save = curwin;
	    curbuf_save = curbuf;
	    col = call_func_retnr(funcname, 2, args);

	    State = save_State;
	    if (curwin_save != curwin || curbuf_save != curbuf)
	    {
		emsg(_(e_complwin));
		return FAIL;
	    }
	    curwin->w_cursor = pos;	// restore the cursor position
	    validate_cursor();
	    if (!EQUAL_POS(curwin->w_cursor, pos))
	    {
		emsg(_(e_compldel));
		return FAIL;
	    }

	    // Return value -2 means the user complete function wants to
	    // cancel the complete without an error.
	    // Return value -3 does the same as -2 and leaves CTRL-X mode.
	    if (col == -2)
		return FAIL;
	    if (col == -3)
	    {
		ctrl_x_mode = CTRL_X_NORMAL;
		edit_submode = NULL;
		if (!shortmess(SHM_COMPLETIONMENU))
		    msg_clr_cmdline();
		return FAIL;
	    }

	    // Reset extended parameters of completion, when start new
	    // completion.
	    compl_opt_refresh_always = FALSE;
	    compl_opt_suppress_empty = FALSE;

	    if (col < 0)
		col = curs_col;
	    compl_col = col;
	    if (compl_col > curs_col)
		compl_col = curs_col;

	    // Setup variables for completion.  Need to obtain "line" again,
	    // it may have become invalid.
	    line = ml_get(curwin->w_cursor.lnum);
	    compl_length = curs_col - compl_col;
	    compl_pattern = vim_strnsave(line + compl_col, compl_length);
	    if (compl_pattern == NULL)
#endif
		return FAIL;
	}
	else if (ctrl_x_mode == CTRL_X_SPELL)
	{
#ifdef FEAT_SPELL
	    if (spell_bad_len > 0)
		compl_col = curs_col - spell_bad_len;
	    else
		compl_col = spell_word_start(startcol);
	    if (compl_col >= (colnr_T)startcol)
	    {
		compl_length = 0;
		compl_col = curs_col;
	    }
	    else
	    {
		spell_expand_check_cap(compl_col);
		compl_length = (int)curs_col - compl_col;
	    }
	    // Need to obtain "line" again, it may have become invalid.
	    line = ml_get(curwin->w_cursor.lnum);
	    compl_pattern = vim_strnsave(line + compl_col, compl_length);
	    if (compl_pattern == NULL)
#endif
		return FAIL;
	}
	else
	{
	    internal_error("ins_complete()");
	    return FAIL;
	}

	if (compl_cont_status & CONT_ADDING)
	{
	    edit_submode_pre = (char_u *)_(" Adding");
	    if (ctrl_x_mode_line_or_eval())
	    {
		// Insert a new line, keep indentation but ignore 'comments'
		char_u *old = curbuf->b_p_com;

		curbuf->b_p_com = (char_u *)"";
		compl_startpos.lnum = curwin->w_cursor.lnum;
		compl_startpos.col = compl_col;
		ins_eol('\r');
		curbuf->b_p_com = old;
		compl_length = 0;
		compl_col = curwin->w_cursor.col;
	    }
	}
	else
	{
	    edit_submode_pre = NULL;
	    compl_startpos.col = compl_col;
	}

	if (compl_cont_status & CONT_LOCAL)
	    edit_submode = (char_u *)_(ctrl_x_msgs[CTRL_X_LOCAL_MSG]);
	else
	    edit_submode = (char_u *)_(CTRL_X_MSG(ctrl_x_mode));

	// If any of the original typed text has been changed we need to fix
	// the redo buffer.
	ins_compl_fixRedoBufForLeader(NULL);

	// Always add completion for the original text.
	vim_free(compl_orig_text);
	compl_orig_text = vim_strnsave(line + compl_col, compl_length);
	if (p_ic)
	    flags |= CP_ICASE;
	if (compl_orig_text == NULL || ins_compl_add(compl_orig_text,
				  -1, NULL, NULL, NULL, 0, flags, FALSE) != OK)
	{
	    VIM_CLEAR(compl_pattern);
	    VIM_CLEAR(compl_orig_text);
	    return FAIL;
	}

	// showmode might reset the internal line pointers, so it must
	// be called before line = ml_get(), or when this address is no
	// longer needed.  -- Acevedo.
	edit_submode_extra = (char_u *)_("-- Searching...");
	edit_submode_highl = HLF_COUNT;
	showmode();
	edit_submode_extra = NULL;
	out_flush();
    }
    else if (insert_match && stop_arrow() == FAIL)
	return FAIL;

    compl_shown_match = compl_curr_match;
    compl_shows_dir = compl_direction;

    // Find next match (and following matches).
    save_w_wrow = curwin->w_wrow;
    save_w_leftcol = curwin->w_leftcol;
    n = ins_compl_next(TRUE, ins_compl_key2count(c), insert_match, FALSE);

    // may undisplay the popup menu
    ins_compl_upd_pum();

    if (n > 1)		// all matches have been found
	compl_matches = n;
    compl_curr_match = compl_shown_match;
    compl_direction = compl_shows_dir;

    // Eat the ESC that vgetc() returns after a CTRL-C to avoid leaving Insert
    // mode.
    if (got_int && !global_busy)
    {
	(void)vgetc();
	got_int = FALSE;
    }

    // we found no match if the list has only the "compl_orig_text"-entry
    if (compl_first_match == compl_first_match->cp_next)
    {
	edit_submode_extra = (compl_cont_status & CONT_ADDING)
			&& compl_length > 1
			     ? (char_u *)_(e_hitend) : (char_u *)_(e_patnotf);
	edit_submode_highl = HLF_E;
	// remove N_ADDS flag, so next ^X<> won't try to go to ADDING mode,
	// because we couldn't expand anything at first place, but if we used
	// ^P, ^N, ^X^I or ^X^D we might want to add-expand a single-char-word
	// (such as M in M'exico) if not tried already.  -- Acevedo
	if (	   compl_length > 1
		|| (compl_cont_status & CONT_ADDING)
		|| (ctrl_x_mode != CTRL_X_NORMAL
		    && ctrl_x_mode != CTRL_X_PATH_PATTERNS
		    && ctrl_x_mode != CTRL_X_PATH_DEFINES))
	    compl_cont_status &= ~CONT_N_ADDS;
    }

    if (compl_curr_match->cp_flags & CP_CONT_S_IPOS)
	compl_cont_status |= CONT_S_IPOS;
    else
	compl_cont_status &= ~CONT_S_IPOS;

    if (edit_submode_extra == NULL)
    {
	if (compl_curr_match->cp_flags & CP_ORIGINAL_TEXT)
	{
	    edit_submode_extra = (char_u *)_("Back at original");
	    edit_submode_highl = HLF_W;
	}
	else if (compl_cont_status & CONT_S_IPOS)
	{
	    edit_submode_extra = (char_u *)_("Word from other line");
	    edit_submode_highl = HLF_COUNT;
	}
	else if (compl_curr_match->cp_next == compl_curr_match->cp_prev)
	{
	    edit_submode_extra = (char_u *)_("The only match");
	    edit_submode_highl = HLF_COUNT;
	}
	else
	{
	    // Update completion sequence number when needed.
	    if (compl_curr_match->cp_number == -1)
	    {
		int		number = 0;
		compl_T		*match;

		if (compl_direction == FORWARD)
		{
		    // search backwards for the first valid (!= -1) number.
		    // This should normally succeed already at the first loop
		    // cycle, so it's fast!
		    for (match = compl_curr_match->cp_prev; match != NULL
			    && match != compl_first_match;
						       match = match->cp_prev)
			if (match->cp_number != -1)
			{
			    number = match->cp_number;
			    break;
			}
		    if (match != NULL)
			// go up and assign all numbers which are not assigned
			// yet
			for (match = match->cp_next;
				match != NULL && match->cp_number == -1;
						       match = match->cp_next)
			    match->cp_number = ++number;
		}
		else // BACKWARD
		{
		    // search forwards (upwards) for the first valid (!= -1)
		    // number.  This should normally succeed already at the
		    // first loop cycle, so it's fast!
		    for (match = compl_curr_match->cp_next; match != NULL
			    && match != compl_first_match;
						       match = match->cp_next)
			if (match->cp_number != -1)
			{
			    number = match->cp_number;
			    break;
			}
		    if (match != NULL)
			// go down and assign all numbers which are not
			// assigned yet
			for (match = match->cp_prev; match
				&& match->cp_number == -1;
						       match = match->cp_prev)
			    match->cp_number = ++number;
		}
	    }

	    // The match should always have a sequence number now, this is
	    // just a safety check.
	    if (compl_curr_match->cp_number != -1)
	    {
		// Space for 10 text chars. + 2x10-digit no.s = 31.
		// Translations may need more than twice that.
		static char_u match_ref[81];

		if (compl_matches > 0)
		    vim_snprintf((char *)match_ref, sizeof(match_ref),
				_("match %d of %d"),
				compl_curr_match->cp_number, compl_matches);
		else
		    vim_snprintf((char *)match_ref, sizeof(match_ref),
				_("match %d"),
				compl_curr_match->cp_number);
		edit_submode_extra = match_ref;
		edit_submode_highl = HLF_R;
		if (dollar_vcol >= 0)
		    curs_columns(FALSE);
	    }
	}
    }

    // Show a message about what (completion) mode we're in.
    if (!compl_opt_suppress_empty)
    {
	showmode();
	if (!shortmess(SHM_COMPLETIONMENU))
	{
	    if (edit_submode_extra != NULL)
	    {
		if (!p_smd)
		    msg_attr((char *)edit_submode_extra,
			    edit_submode_highl < HLF_COUNT
			    ? HL_ATTR(edit_submode_highl) : 0);
	    }
	    else
		msg_clr_cmdline();	// necessary for "noshowmode"
	}
    }

    // Show the popup menu, unless we got interrupted.
    if (enable_pum && !compl_interrupted)
	show_pum(save_w_wrow, save_w_leftcol);

    compl_was_interrupted = compl_interrupted;
    compl_interrupted = FALSE;

    return OK;
}

    static void
show_pum(int prev_w_wrow, int prev_w_leftcol)
{
    // RedrawingDisabled may be set when invoked through complete().
    int n = RedrawingDisabled;

    RedrawingDisabled = 0;

    // If the cursor moved or the display scrolled we need to remove the pum
    // first.
    setcursor();
    if (prev_w_wrow != curwin->w_wrow || prev_w_leftcol != curwin->w_leftcol)
	ins_compl_del_pum();

    ins_compl_show_pum();
    setcursor();
    RedrawingDisabled = n;
}

/*
 * Looks in the first "len" chars. of "src" for search-metachars.
 * If dest is not NULL the chars. are copied there quoting (with
 * a backslash) the metachars, and dest would be NUL terminated.
 * Returns the length (needed) of dest
 */
    static unsigned
quote_meta(char_u *dest, char_u *src, int len)
{
    unsigned	m = (unsigned)len + 1;  // one extra for the NUL

    for ( ; --len >= 0; src++)
    {
	switch (*src)
	{
	    case '.':
	    case '*':
	    case '[':
		if (ctrl_x_mode == CTRL_X_DICTIONARY
					   || ctrl_x_mode == CTRL_X_THESAURUS)
		    break;
		// FALLTHROUGH
	    case '~':
		if (!p_magic)	// quote these only if magic is set
		    break;
		// FALLTHROUGH
	    case '\\':
		if (ctrl_x_mode == CTRL_X_DICTIONARY
					   || ctrl_x_mode == CTRL_X_THESAURUS)
		    break;
		// FALLTHROUGH
	    case '^':		// currently it's not needed.
	    case '$':
		m++;
		if (dest != NULL)
		    *dest++ = '\\';
		break;
	}
	if (dest != NULL)
	    *dest++ = *src;
	// Copy remaining bytes of a multibyte character.
	if (has_mbyte)
	{
	    int i, mb_len;

	    mb_len = (*mb_ptr2len)(src) - 1;
	    if (mb_len > 0 && len >= mb_len)
		for (i = 0; i < mb_len; ++i)
		{
		    --len;
		    ++src;
		    if (dest != NULL)
			*dest++ = *src;
		}
	}
    }
    if (dest != NULL)
	*dest = NUL;

    return m;
}

#if defined(EXITFREE) || defined(PROTO)
    void
free_insexpand_stuff(void)
{
    VIM_CLEAR(compl_orig_text);
}
#endif

#ifdef FEAT_SPELL
/*
 * Called when starting CTRL_X_SPELL mode: Move backwards to a previous badly
 * spelled word, if there is one.
 */
    static void
spell_back_to_badword(void)
{
    pos_T	tpos = curwin->w_cursor;

    spell_bad_len = spell_move_to(curwin, BACKWARD, TRUE, TRUE, NULL);
    if (curwin->w_cursor.col != tpos.col)
	start_arrow(&tpos);
}
#endif
