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
# define CTRL_X_CMDLINE_CTRL_X	17	// CTRL-X typed in CTRL_X_CMDLINE
# define CTRL_X_REGISTER	18	// complete words from registers

# define CTRL_X_MSG(i) ctrl_x_msgs[(i) & ~CTRL_X_WANT_IDENT]

// Message for CTRL-X mode, index is ctrl_x_mode.
static char *ctrl_x_msgs[] =
{
    N_(" Keyword completion (^N^P)"), // CTRL_X_NORMAL, ^P/^N compl.
    N_(" ^X mode (^]^D^E^F^I^K^L^N^O^P^Rs^U^V^Y)"),
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
    N_(" Spelling suggestion (^S^N^P)"),
    N_(" Keyword Local completion (^N^P)"),
    NULL,   // CTRL_X_EVAL doesn't use msg.
    N_(" Command-line completion (^V^N^P)"),
    N_(" Register completion (^N^P)"),
};

#if defined(FEAT_COMPL_FUNC) || defined(FEAT_EVAL)
static char *ctrl_x_mode_names[] = {
    "keyword",
    "ctrl_x",
    "scroll",
    "whole_line",
    "files",
    "tags",
    "path_patterns",
    "path_defines",
    "unknown",		    // CTRL_X_FINISHED
    "dictionary",
    "thesaurus",
    "cmdline",
    "function",
    "omni",
    "spell",
    NULL,		    // CTRL_X_LOCAL_MSG only used in "ctrl_x_msgs"
    "eval",
    "cmdline",
    "register",
};
#endif

/*
 * Structure used to store one match for insert completion.
 */
typedef struct compl_S compl_T;
struct compl_S
{
    compl_T	*cp_next;
    compl_T	*cp_prev;
    compl_T	*cp_match_next;		// matched next compl_T
    string_T	cp_str;			// matched text
    char_u	*(cp_text[CPT_COUNT]);	// text for the menu
#ifdef FEAT_EVAL
    typval_T	cp_user_data;
#endif
    char_u	*cp_fname;		// file containing the match, allocated when
					// cp_flags has CP_FREE_FNAME
    int		cp_flags;		// CP_ values
    int		cp_number;		// sequence number
    int		cp_score;		// fuzzy match score or proximity score
    int		cp_in_match_array;	// collected by compl_match_array
    int		cp_user_abbr_hlattr;	// highlight attribute for abbr
    int		cp_user_kind_hlattr;	// highlight attribute for kind
    int		cp_cpt_source_idx;	// index of this match's source in 'cpt' option
};

// values for cp_flags
# define CP_ORIGINAL_TEXT   1	// the original text when the expansion begun
# define CP_FREE_FNAME	    2	// cp_fname is allocated
# define CP_CONT_S_IPOS	    4	// use CONT_S_IPOS for compl_cont_status
# define CP_EQUAL	    8	// ins_compl_equal() always returns TRUE
# define CP_ICASE	    16	// ins_compl_equal() ignores case
# define CP_FAST	    32	// use fast_breakcheck instead of ui_breakcheck

/*
 * All the current matches are stored in a list.
 * "compl_first_match" points to the start of the list.
 * "compl_curr_match" points to the currently selected entry.
 * "compl_shown_match" is different from compl_curr_match during
 * ins_compl_get_exp(), when new matches are added to the list.
 * "compl_old_match" points to previous "compl_curr_match".
 */
static compl_T    *compl_first_match = NULL;
static compl_T    *compl_curr_match = NULL;
static compl_T    *compl_shown_match = NULL;
static compl_T    *compl_old_match = NULL;

// list used to store the compl_T which have the max score
// used for completefuzzycollect
static compl_T	  **compl_best_matches = NULL;
static int	  compl_num_bests = 0;
// inserted a longest when completefuzzycollect enabled
static int	  compl_cfc_longest_ins = FALSE;

// After using a cursor key <Enter> selects a match in the popup menu,
// otherwise it inserts a line break.
static int	  compl_enter_selects = FALSE;

// When "compl_leader" is not NULL only matches that start with this string
// are used.
static string_T	  compl_leader = {NULL, 0};

static int	  compl_get_longest = FALSE;	// put longest common string
						// in compl_leader

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

static int	  compl_matches = 0;	    // number of completion matches
static string_T	  compl_pattern = {NULL, 0};	 // search pattern for matching items
#ifdef FEAT_COMPL_FUNC
static string_T	  cpt_compl_pattern = {NULL, 0}; // pattern returned by func in 'cpt'
#endif
static int	  compl_direction = FORWARD;
static int	  compl_shows_dir = FORWARD;
static int	  compl_pending = 0;	    // > 1 for postponed CTRL-N
static pos_T	  compl_startpos;
// Length in bytes of the text being completed (this is deleted to be replaced
// by the match.)
static int	  compl_length = 0;
static linenr_T	  compl_lnum = 0;           // lnum where the completion start
static colnr_T	  compl_col = 0;	    // column where the text starts
					    // that is being completed
static colnr_T	  compl_ins_end_col = 0;
static string_T	  compl_orig_text = {NULL, 0};  // text as it was before
					    // completion started
static int	  compl_cont_mode = 0;
static expand_T	  compl_xp;

static win_T	  *compl_curr_win = NULL;  // win where completion is active
static buf_T	  *compl_curr_buf = NULL;  // buf where completion is active

#define COMPL_INITIAL_TIMEOUT_MS    80
// Autocomplete uses a decaying timeout: starting from COMPL_INITIAL_TIMEOUT_MS,
// if the current source exceeds its timeout, it is interrupted and the next
// begins with half the time. A small minimum timeout ensures every source
// gets at least a brief chance.
static int	  compl_autocomplete = FALSE;	    // whether autocompletion is active
static int	  compl_timeout_ms = COMPL_INITIAL_TIMEOUT_MS;
static int	  compl_time_slice_expired = FALSE; // time budget exceeded for current source
static int	  compl_from_nonkeyword = FALSE;    // completion started from non-keyword

// Halve the current completion timeout, simulating exponential decay.
#define COMPL_MIN_TIMEOUT_MS	5
#define DECAY_COMPL_TIMEOUT() \
    do { \
	if (compl_timeout_ms > COMPL_MIN_TIMEOUT_MS) \
	    compl_timeout_ms /= 2; \
    } while (0)

// List of flags for method of completion.
static int	  compl_cont_status = 0;
# define CONT_ADDING	1	// "normal" or "adding" expansion
# define CONT_INTRPT	(2 + 4)	// a ^X interrupted the current expansion
				// it's set only iff N_ADDS is set
# define CONT_N_ADDS	4	// next ^X<> will add-new or expand-current
# define CONT_S_IPOS	8	// next ^X<> will set initial_pos?
				// if so, word-wise-expansion will set SOL
# define CONT_SOL	16	// pattern includes start of line, just for
				// word-wise expansion, not set for ^X^L
# define CONT_LOCAL	32	// for ctrl_x_mode 0, ^X^P/^X^N do a local
				// expansion, (eg use complete=.)

static int	  compl_opt_refresh_always = FALSE;
static int	  compl_opt_suppress_empty = FALSE;

static int	  compl_selected_item = -1;

static int	  *compl_fuzzy_scores;

// Define the structure for completion source (in 'cpt' option) information
typedef struct cpt_source_T
{
    int	cs_refresh_always;  // Whether 'refresh:always' is set for func
    int	cs_startcol;	    // Start column returned by func
    int	cs_max_matches;	    // Max items to display from this source
#ifdef ELAPSED_FUNC
    elapsed_T	compl_start_tv;	    // Timestamp when match collection starts
#endif
} cpt_source_T;

#define STARTCOL_NONE	-9
static cpt_source_T *cpt_sources_array; // Pointer to the array of completion sources
static int	    cpt_sources_count;  // Total number of completion sources specified in the 'cpt' option
static int	    cpt_sources_index = -1;  // Index of the current completion source being expanded

// "compl_match_array" points the currently displayed list of entries in the
// popup menu.  It is NULL when there is no popup menu.
static pumitem_T *compl_match_array = NULL;
static int compl_match_arraysize;

static int ins_compl_add(char_u *str, int len, char_u *fname, char_u **cptext, typval_T *user_data, int cdir, int flags, int adup, int *user_hl, int score);
static void ins_compl_longest_match(compl_T *match);
static void ins_compl_del_pum(void);
static void ins_compl_files(int count, char_u **files, int thesaurus, int flags, regmatch_T *regmatch, char_u *buf, int *dir);
static void ins_compl_free(void);
static int  ins_compl_need_restart(void);
static void ins_compl_new_leader(void);
static int  get_compl_len(void);
static void ins_compl_restart(void);
static void ins_compl_set_original_text(char_u *str, size_t len);
static void ins_compl_fixRedoBufForLeader(char_u *ptr_arg);
# if defined(FEAT_COMPL_FUNC) || defined(FEAT_EVAL)
static void ins_compl_add_list(list_T *list);
static void ins_compl_add_dict(dict_T *dict);
static int get_userdefined_compl_info(colnr_T curs_col, callback_T *cb, int *startcol);
static void get_cpt_func_completion_matches(callback_T *cb);
static callback_T *get_callback_if_cpt_func(char_u *p);
# endif
static int setup_cpt_sources(void);
static int is_cpt_func_refresh_always(void);
static void cpt_sources_clear(void);
static void cpt_compl_refresh(void);
static int  ins_compl_key2dir(int c);
static int  ins_compl_pum_key(int c);
static int  ins_compl_key2count(int c);
static void show_pum(int prev_w_wrow, int prev_w_leftcol);
static unsigned  quote_meta(char_u *dest, char_u *str, int len);
static int ins_compl_has_multiple(void);
static void ins_compl_expand_multiple(char_u *str);
static void ins_compl_longest_insert(char_u *prefix);
static void ins_compl_make_linear(void);
static int ins_compl_make_cyclic(void);

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
    if (!ctrl_x_mode_cmdline())
    {
	// if the next ^X<> won't ADD nothing, then reset compl_cont_status
	if ((compl_cont_status & CONT_N_ADDS) && !p_ac)
	    compl_cont_status |= CONT_INTRPT;
	else
	    compl_cont_status = 0;
	// We're not sure which CTRL-X mode it will be yet
	ctrl_x_mode = CTRL_X_NOT_DEFINED_YET;
	edit_submode = (char_u *)_(CTRL_X_MSG(ctrl_x_mode));
	edit_submode_pre = NULL;
	showmode();
    }
    else
	// CTRL-X in CTRL-X CTRL-V mode behaves differently to make CTRL-X
	// CTRL-V look like CTRL-N
	ctrl_x_mode = CTRL_X_CMDLINE_CTRL_X;

    may_trigger_modechanged();
}

/*
 * Functions to check the current CTRL-X mode.
 */
int ctrl_x_mode_none(void)
    { return ctrl_x_mode == 0; }
int ctrl_x_mode_normal(void)
    { return ctrl_x_mode == CTRL_X_NORMAL; }
int ctrl_x_mode_scroll(void)
    { return ctrl_x_mode == CTRL_X_SCROLL; }
int ctrl_x_mode_whole_line(void)
    { return ctrl_x_mode == CTRL_X_WHOLE_LINE; }
int ctrl_x_mode_files(void)
    { return ctrl_x_mode == CTRL_X_FILES; }
int ctrl_x_mode_tags(void)
    { return ctrl_x_mode == CTRL_X_TAGS; }
int ctrl_x_mode_path_patterns(void)
    { return ctrl_x_mode == CTRL_X_PATH_PATTERNS; }
int ctrl_x_mode_path_defines(void)
    { return ctrl_x_mode == CTRL_X_PATH_DEFINES; }
int ctrl_x_mode_dictionary(void)
    { return ctrl_x_mode == CTRL_X_DICTIONARY; }
int ctrl_x_mode_thesaurus(void)
    { return ctrl_x_mode == CTRL_X_THESAURUS; }
int ctrl_x_mode_cmdline(void)
    { return ctrl_x_mode == CTRL_X_CMDLINE
		|| ctrl_x_mode == CTRL_X_CMDLINE_CTRL_X; }
int ctrl_x_mode_function(void)
    { return ctrl_x_mode == CTRL_X_FUNCTION; }
int ctrl_x_mode_omni(void)
    { return ctrl_x_mode == CTRL_X_OMNI; }
int ctrl_x_mode_spell(void)
    { return ctrl_x_mode == CTRL_X_SPELL; }
static int ctrl_x_mode_eval(void)
    { return ctrl_x_mode == CTRL_X_EVAL; }
int ctrl_x_mode_line_or_eval(void)
    { return ctrl_x_mode == CTRL_X_WHOLE_LINE || ctrl_x_mode == CTRL_X_EVAL; }
int ctrl_x_mode_register(void)
    { return ctrl_x_mode == CTRL_X_REGISTER; }

/*
 * Whether other than default completion has been selected.
 */
    int
ctrl_x_mode_not_default(void)
{
    return ctrl_x_mode != CTRL_X_NORMAL;
}

/*
 * Whether CTRL-X was typed without a following character,
 * not including when in CTRL-X CTRL-V mode.
 */
    int
ctrl_x_mode_not_defined_yet(void)
{
    return ctrl_x_mode == CTRL_X_NOT_DEFINED_YET;
}

/*
 * Return TRUE if currently in "normal" or "adding" insert completion matches
 * state
 */
    int
compl_status_adding(void)
{
    return compl_cont_status & CONT_ADDING;
}

/*
 * Return TRUE if the completion pattern includes start of line, just for
 * word-wise expansion.
 */
    int
compl_status_sol(void)
{
    return compl_cont_status & CONT_SOL;
}

/*
 * Return TRUE if ^X^P/^X^N will do a local completion (i.e. use complete=.)
 */
    int
compl_status_local(void)
{
    return compl_cont_status & CONT_LOCAL;
}

/*
 * Clear the completion status flags
 */
    void
compl_status_clear(void)
{
    compl_cont_status = 0;
}

/*
 * Return TRUE if completion is using the forward direction matches
 */
    static int
compl_dir_forward(void)
{
    return compl_direction == FORWARD;
}

/*
 * Return TRUE if currently showing forward completion matches
 */
    static int
compl_shows_dir_forward(void)
{
    return compl_shows_dir == FORWARD;
}

/*
 * Return TRUE if currently showing backward completion matches
 */
    static int
compl_shows_dir_backward(void)
{
    return compl_shows_dir == BACKWARD;
}

/*
 * Return TRUE if the 'dictionary' or 'thesaurus' option can be used.
 */
    int
has_compl_option(int dict_opt)
{
    if (dict_opt ? (*curbuf->b_p_dict == NUL && *p_dict == NUL
#ifdef FEAT_SPELL
							&& !curwin->w_p_spell
#endif
							)
		 : (*curbuf->b_p_tsr == NUL && *p_tsr == NUL
#ifdef FEAT_COMPL_FUNC
		     && *curbuf->b_p_tsrfu == NUL && *p_tsrfu == NUL
#endif
		   ))
    {
	ctrl_x_mode = CTRL_X_NORMAL;
	edit_submode = NULL;
	msg_attr(dict_opt ? _("'dictionary' option is empty")
			  : _("'thesaurus' option is empty"),
							      HL_ATTR(HLF_E));
	if (emsg_silent == 0 && !in_assert_fails)
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
 * Is the character "c" a valid key to go to or keep us in CTRL-X mode?
 * This depends on the current mode.
 */
    int
vim_is_ctrl_x_key(int c)
{
    // Always allow ^R - let its results then be checked
    if (c == Ctrl_R && ctrl_x_mode != CTRL_X_REGISTER)
	return TRUE;

    // Accept <PageUp> and <PageDown> if the popup menu is visible.
    if (ins_compl_pum_key(c))
	return TRUE;

    switch (ctrl_x_mode)
    {
	case 0:		    // Not in any CTRL-X mode
	    return (c == Ctrl_N || c == Ctrl_P || c == Ctrl_X);
	case CTRL_X_NOT_DEFINED_YET:
	case CTRL_X_CMDLINE_CTRL_X:
	    return (   c == Ctrl_X || c == Ctrl_Y || c == Ctrl_E
		    || c == Ctrl_L || c == Ctrl_F || c == Ctrl_RSB
		    || c == Ctrl_I || c == Ctrl_D || c == Ctrl_P
		    || c == Ctrl_N || c == Ctrl_T || c == Ctrl_V
		    || c == Ctrl_Q || c == Ctrl_U || c == Ctrl_O
		    || c == Ctrl_S || c == Ctrl_K || c == 's'
		    || c == Ctrl_Z || c == Ctrl_R);
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
	case CTRL_X_REGISTER:
	    return (c == Ctrl_R || c == Ctrl_P || c == Ctrl_N);
    }
    internal_error("vim_is_ctrl_x_key()");
    return FALSE;
}

/*
 * Return TRUE if "match" is the original text when the completion began.
 */
    static int
match_at_original_text(compl_T *match)
{
    return match->cp_flags & CP_ORIGINAL_TEXT;
}

/*
 * Returns TRUE if "match" is the first match in the completion list.
 */
    static int
is_first_match(compl_T *match)
{
    return match == compl_first_match;
}

/*
 * Return TRUE when character "c" is part of the item currently being
 * completed.  Used to decide whether to abandon complete mode when the menu
 * is visible.
 */
    int
ins_compl_accept_char(int c)
{
    if (compl_autocomplete && compl_from_nonkeyword)
	return FALSE;

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
	case CTRL_X_CMDLINE_CTRL_X:
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
 * Get the completed text by inferring the case of the originally typed text.
 * If the result is in allocated memory "tofree" is set to it.
 */
    static char_u *
ins_compl_infercase_gettext(
    char_u  *str,
    int	    char_len,
    int	    compl_char_len,
    int	    min_len,
    char_u  **tofree)
{
    int		*wca;			// Wide character array.
    char_u	*p;
    int		i, c;
    int		has_lower = FALSE;
    int		was_letter = FALSE;
    garray_T	gap;

    IObuff[0] = NUL;

    // Allocate wide character array for the completion and fill it.
    wca = ALLOC_MULT(int, char_len);
    if (wca == NULL)
	return IObuff;

    p = str;
    for (i = 0; i < char_len; ++i)
    {
	if (has_mbyte)
	    wca[i] = mb_ptr2char_adv(&p);
	else
	    wca[i] = *(p++);
    }

    // Rule 1: Were any chars converted to lower?
    p = compl_orig_text.string;
    for (i = 0; i < min_len; ++i)
    {
	c = has_mbyte ? mb_ptr2char_adv(&p) : *(p++);
	if (MB_ISLOWER(c))
	{
	    has_lower = TRUE;
	    if (MB_ISUPPER(wca[i]))
	    {
		// Rule 1 is satisfied.
		for (i = compl_char_len; i < char_len; ++i)
		    wca[i] = MB_TOLOWER(wca[i]);
		break;
	    }
	}
    }

    // Rule 2: No lower case, 2nd consecutive letter converted to
    // upper case.
    if (!has_lower)
    {
	p = compl_orig_text.string;
	for (i = 0; i < min_len; ++i)
	{
	    c = has_mbyte ? mb_ptr2char_adv(&p) : *(p++);
	    if (was_letter && MB_ISUPPER(c) && MB_ISLOWER(wca[i]))
	    {
		// Rule 2 is satisfied.
		for (i = compl_char_len; i < char_len; ++i)
		    wca[i] = MB_TOUPPER(wca[i]);
		break;
	    }
	    was_letter = MB_ISLOWER(c) || MB_ISUPPER(c);
	}
    }

    // Copy the original case of the part we typed.
    p = compl_orig_text.string;
    for (i = 0; i < min_len; ++i)
    {
	c = has_mbyte ? mb_ptr2char_adv(&p) : *(p++);
	if (MB_ISLOWER(c))
	    wca[i] = MB_TOLOWER(wca[i]);
	else if (MB_ISUPPER(c))
	    wca[i] = MB_TOUPPER(wca[i]);
    }

    // Generate encoding specific output from wide character array.
    p = IObuff;
    i = 0;
    ga_init2(&gap, 1, 500);
    while (i < char_len)
    {
	if (gap.ga_data != NULL)
	{
	    if (ga_grow(&gap, 10) == FAIL)
	    {
		ga_clear(&gap);
		return (char_u *)"[failed]";
	    }
	    p = (char_u *)gap.ga_data + gap.ga_len;
	    if (has_mbyte)
		gap.ga_len += (*mb_char2bytes)(wca[i++], p);
	    else
	    {
		*p = wca[i++];
		++gap.ga_len;
	    }
	}
	else if ((p - IObuff) + 6 >= IOSIZE)
	{
	    // Multi-byte characters can occupy up to five bytes more than
	    // ASCII characters, and we also need one byte for NUL, so when
	    // getting to six bytes from the edge of IObuff switch to using a
	    // growarray.  Add the character in the next round.
	    if (ga_grow(&gap, IOSIZE) == FAIL)
	    {
		vim_free(wca);
		return (char_u *)"[failed]";
	    }
	    *p = NUL;
	    STRCPY(gap.ga_data, IObuff);
	    gap.ga_len = (int)(p - IObuff);
	}
	else if (has_mbyte)
	    p += (*mb_char2bytes)(wca[i++], p);
	else
	    *(p++) = wca[i++];
    }
    vim_free(wca);

    if (gap.ga_data != NULL)
    {
	*tofree = gap.ga_data;
	return gap.ga_data;
    }

    *p = NUL;
    return IObuff;
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
    int		cont_s_ipos,  // next ^X<> will set initial_pos
    int		score)
{
    char_u	*str = str_arg;
    char_u	*p;
    int		char_len;		// count multi-byte characters
    int		compl_char_len;
    int		min_len;
    int		flags = 0;
    int		res;
    char_u	*tofree = NULL;

    if (p_ic && curbuf->b_p_inf && len > 0)
    {
	// Infer case of completed part.

	// Find actual length of completion.
	if (has_mbyte)
	{
	    p = str;
	    char_len = 0;
	    while (*p != NUL)
	    {
		MB_PTR_ADV(p);
		++char_len;
	    }
	}
	else
	{
	    char_len = len;
	}

	// Find actual length of original text.
	if (has_mbyte)
	{
	    p = compl_orig_text.string;
	    compl_char_len = 0;
	    while (*p != NUL)
	    {
		MB_PTR_ADV(p);
		++compl_char_len;
	    }
	}
	else
	{
	    compl_char_len = compl_length;
	}

	// "char_len" may be smaller than "compl_char_len" when using
	// thesaurus, only use the minimum when comparing.
	min_len = MIN(char_len, compl_char_len);

	str = ins_compl_infercase_gettext(str, char_len,
					  compl_char_len, min_len, &tofree);
    }
    if (cont_s_ipos)
	flags |= CP_CONT_S_IPOS;
    if (icase)
	flags |= CP_ICASE;

    res = ins_compl_add(str, len, fname, NULL, NULL, dir, flags, FALSE, NULL, score);
    vim_free(tofree);
    return res;
}

/*
 * Check if ctrl_x_mode has been configured in 'completefuzzycollect'
 */
    static int
cfc_has_mode(void)
{
    if (ctrl_x_mode_normal() || ctrl_x_mode_dictionary())
	return (cfc_flags & CFC_KEYWORD) != 0;
    else if (ctrl_x_mode_files())
	return (cfc_flags & CFC_FILES) != 0;
    else if (ctrl_x_mode_whole_line())
	return (cfc_flags & CFC_WHOLELINE) != 0;
    else
	return FALSE;
}

/*
 * Returns TRUE if matches should be sorted based on proximity to the cursor.
 */
    static int
is_nearest_active(void)
{
    int flags = get_cot_flags();
    return (compl_autocomplete || (flags & COT_NEAREST))
	&& !(flags & COT_FUZZY);
}

/*
 * Add a match to the list of matches. The arguments are:
 *     str       - text of the match to add
 *     len       - length of "str". If -1, then the length of "str" is
 *		   computed.
 *     fname     - file name to associate with this match.
 *     cptext    - list of strings to use with this match (for abbr, menu, info
 *		   and kind)
 *     user_data - user supplied data (any vim type) for this match
 *     cdir	 - match direction. If 0, use "compl_direction".
 *     flags_arg - match flags (cp_flags)
 *     adup	 - accept this match even if it is already present.
 *     *user_hl  - list of extra highlight attributes for abbr kind.
 * If "cdir" is FORWARD, then the match is added after the current match.
 * Otherwise, it is added before the current match.
 *
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
    int		adup,		    // accept duplicate match
    int		*user_hl,           // user abbr/kind hlattr
    int		score)
{
    compl_T	*match, *current, *prev;
    int		dir = (cdir == 0 ? compl_direction : cdir);
    int		flags = flags_arg;
    int		inserted = FALSE;

    if (flags & CP_FAST)
	fast_breakcheck();
    else
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
	    if (!match_at_original_text(match)
		    && STRNCMP(match->cp_str.string, str, len) == 0
		    && ((int)match->cp_str.length <= len
						 || match->cp_str.string[len] == NUL))
	    {
		if (is_nearest_active() && score > 0 && score < match->cp_score)
		    match->cp_score = score;
		return NOTDONE;
	    }
	    match = match->cp_next;
	} while (match != NULL && !is_first_match(match));
    }

    // Remove any popup menu before changing the list of matches.
    ins_compl_del_pum();

    // Allocate a new match structure.
    // Copy the values to the new match structure.
    match = ALLOC_CLEAR_ONE(compl_T);
    if (match == NULL)
	return FAIL;
    match->cp_number = flags & CP_ORIGINAL_TEXT ? 0 : -1;
    if ((match->cp_str.string = vim_strnsave(str, len)) == NULL)
    {
	vim_free(match);
	return FAIL;
    }

    match->cp_str.length = len;

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
    match->cp_user_abbr_hlattr = user_hl ? user_hl[0] : -1;
    match->cp_user_kind_hlattr = user_hl ? user_hl[1] : -1;
    match->cp_score = score;
    match->cp_cpt_source_idx = cpt_sources_index;

    if (cptext != NULL)
    {
	for (int i = 0; i < CPT_COUNT; ++i)
	{
	    if (cptext[i] != NULL && *cptext[i] != NUL)
		match->cp_text[i] = vim_strsave(cptext[i]);
	}
    }
#ifdef FEAT_EVAL
    if (user_data != NULL)
	match->cp_user_data = *user_data;
#endif

    // Link the new match structure after (FORWARD) or before (BACKWARD) the
    // current match in the list of matches .
    if (compl_first_match == NULL)
	match->cp_next = match->cp_prev = NULL;
    else if (cfc_has_mode() && score > 0 && compl_get_longest)
    {
	current = compl_first_match->cp_next;
	prev = compl_first_match;
	inserted = FALSE;
	// The direction is ignored when using longest and
	// completefuzzycollect, because matches are inserted
	// and sorted by score.
	while (current != NULL && current != compl_first_match)
	{
	    if (current->cp_score < score)
	    {
		match->cp_next = current;
		match->cp_prev = current->cp_prev;
		if (current->cp_prev)
		    current->cp_prev->cp_next = match;
		current->cp_prev = match;
		inserted = TRUE;
		break;
	    }
	    prev = current;
	    current = current->cp_next;
	}
	if (!inserted)
	{
	    prev->cp_next = match;
	    match->cp_prev = prev;
	    match->cp_next = compl_first_match;
	    compl_first_match->cp_prev = match;
	}
    }
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
    if (compl_get_longest && (flags & CP_ORIGINAL_TEXT) == 0 && !cfc_has_mode())
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
	return STRNICMP(match->cp_str.string, str, (size_t)len) == 0;
    return STRNCMP(match->cp_str.string, str, (size_t)len) == 0;
}

/*
 * when len is -1 mean use whole length of p otherwise part of p
 */
    static void
ins_compl_insert_bytes(char_u *p, int len)
{
    if (len == -1)
	len = (int)STRLEN(p);
    ins_bytes_len(p, len);
    compl_ins_end_col = curwin->w_cursor.col;
}

/*
 * Checks if the column is within the currently inserted completion text
 * column range. If it is, it returns a special highlight attribute.
 * -1 means normal item.
 */
    int
ins_compl_col_range_attr(linenr_T lnum, int col)
{
    int	    start_col;
    int	    attr;

    if ((get_cot_flags() & COT_FUZZY)
	    || (attr = syn_name2attr((char_u *)"ComplMatchIns")) == 0)
	return -1;

    start_col = compl_col + (int)ins_compl_leader_len();
    if (!ins_compl_has_multiple())
	return (col >= start_col && col < compl_ins_end_col) ? attr : -1;

    // Multiple lines
    if ((lnum == compl_lnum && col >= start_col && col < MAXCOL) ||
	(lnum > compl_lnum && lnum < curwin->w_cursor.lnum) ||
	(lnum == curwin->w_cursor.lnum && col <= compl_ins_end_col))
	return attr;

    return -1;
}

/*
 * Returns TRUE if the current completion string contains newline characters,
 * indicating it's a multi-line completion.
 */
    static int
ins_compl_has_multiple(void)
{
    return vim_strchr(compl_shown_match->cp_str.string, '\n') != NULL;
}

/*
 * Returns TRUE if the given line number falls within the range of a multi-line
 * completion, i.e. between the starting line (compl_lnum) and current cursor
 * line. Always returns FALSE for single-line completions.
 */
    int
ins_compl_lnum_in_range(linenr_T lnum)
{
    if (!ins_compl_has_multiple())
	return FALSE;
    return lnum >= compl_lnum && lnum <= curwin->w_cursor.lnum;
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

    if (compl_leader.string == NULL)
    {
	// First match, use it as a whole.
	compl_leader.string = vim_strnsave(match->cp_str.string, match->cp_str.length);
	if (compl_leader.string == NULL)
	    return;

	compl_leader.length = match->cp_str.length;
	had_match = (curwin->w_cursor.col > compl_col);
	ins_compl_longest_insert(compl_leader.string);

	// When the match isn't there (to avoid matching itself) remove it
	// again after redrawing.
	if (!had_match)
	    ins_compl_delete();
	compl_used_match = FALSE;

	return;
    }

    // Reduce the text if this match differs from compl_leader.
    p = compl_leader.string;
    s = match->cp_str.string;
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
	compl_leader.length = (size_t)(p - compl_leader.string);

	had_match = (curwin->w_cursor.col > compl_col);
	ins_compl_longest_insert(compl_leader.string);

	// When the match isn't there (to avoid matching itself) remove it
	// again after redrawing.
	if (!had_match)
	    ins_compl_delete();
    }

    compl_used_match = FALSE;
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
    int		add_r = OK;
    int		dir = compl_direction;

    for (int i = 0; i < num_matches && add_r != FAIL; i++)
    {
	add_r = ins_compl_add(matches[i], -1, NULL, NULL, NULL, dir,
				CP_FAST | (icase ? CP_ICASE : 0), FALSE, NULL, 0);
	if (add_r == OK)
	    // if dir was BACKWARD then honor it just once
	    dir = FORWARD;
    }
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

    if (compl_first_match == NULL)
	return 0;

    // Find the end of the list.
    match = compl_first_match;
    // there's always an entry for the compl_orig_text, it doesn't count.
    while (match->cp_next != NULL && !is_first_match(match->cp_next))
    {
	match = match->cp_next;
	++count;
    }
    match->cp_next = compl_first_match;
    compl_first_match->cp_prev = match;

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
    return (int)compl_shown_match->cp_str.length
					    > curwin->w_cursor.col - compl_col;
}

/*
 * Get the local or global value of 'completeopt' flags.
 */
    unsigned int
get_cot_flags(void)
{
    return curbuf->b_cot_flags != 0 ? curbuf->b_cot_flags : cot_flags;
}

/*
 * Update the screen and when there is any scrolling remove the popup menu.
 */
    static void
ins_compl_upd_pum(void)
{
    if (compl_match_array == NULL)
	return;

    int h = curwin->w_cline_height;
    // Update the screen later, before drawing the popup menu over it.
    pum_call_update_screen();
    if (h != curwin->w_cline_height)
	ins_compl_del_pum();
}

/*
 * Remove any popup menu.
 */
    static void
ins_compl_del_pum(void)
{
    if (compl_match_array == NULL)
	return;

    pum_undisplay();
    VIM_CLEAR(compl_match_array);
}

/*
 * Return TRUE if the popup menu should be displayed.
 */
    int
pum_wanted(void)
{
    // 'completeopt' must contain "menu" or "menuone"
    if ((get_cot_flags() & COT_ANY_MENU) == 0 && !compl_autocomplete)
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
    int		i = 0;

    // Don't display the popup menu if there are no matches or there is only
    // one (ignoring the original text).
    compl = compl_first_match;
    do
    {
	if (compl == NULL || (!match_at_original_text(compl) && ++i == 2))
	    break;
	compl = compl->cp_next;
    } while (!is_first_match(compl));

    if ((get_cot_flags() & COT_MENUONE) || compl_autocomplete)
	return (i >= 1);
    return (i >= 2);
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Allocate Dict for the completed item.
 * { word, abbr, menu, kind, info }
 */
    static dict_T *
ins_compl_dict_alloc(compl_T *match)
{
    dict_T *dict = dict_alloc_lock(VAR_FIXED);

    if (dict == NULL)
	return NULL;

    dict_add_string(dict, "word", match->cp_str.string);
    dict_add_string(dict, "abbr", match->cp_text[CPT_ABBR]);
    dict_add_string(dict, "menu", match->cp_text[CPT_MENU]);
    dict_add_string(dict, "kind", match->cp_text[CPT_KIND]);
    dict_add_string(dict, "info", match->cp_text[CPT_INFO]);
    if (match->cp_user_data.v_type == VAR_UNKNOWN)
	dict_add_string(dict, "user_data", (char_u *)"");
    else
	dict_add_tv(dict, "user_data", &match->cp_user_data);

    return dict;
}

/*
 * Trigger the CompleteChanged autocmd event. Invoked each time the Insert mode
 * completion menu is changed.
 */
    static void
trigger_complete_changed_event(int cur)
{
    dict_T	    *v_event;
    dict_T	    *item;
    static int	    recursive = FALSE;
    save_v_event_T  save_v_event;

    if (recursive)
	return;

    item = cur < 0 ? dict_alloc() : ins_compl_dict_alloc(compl_curr_match);
    if (item == NULL)
	return;
    v_event = get_v_event(&save_v_event);
    dict_add_dict(v_event, "completed_item", item);
    pum_set_event_info(v_event);
    dict_set_items_ro(v_event);

    recursive = TRUE;
    textlock++;
    apply_autocmds(EVENT_COMPLETECHANGED, NULL, NULL, FALSE, curbuf);
    textlock--;
    recursive = FALSE;

    restore_v_event(v_event, &save_v_event);
}
#endif

/*
 * Helper functions for mergesort_list().
 */
    static void*
cp_get_next(void *node)
{
    return ((compl_T*)node)->cp_next;
}

    static void
cp_set_next(void *node, void *next)
{
    ((compl_T*)node)->cp_next = (compl_T*)next;
}

    static void*
cp_get_prev(void* node)
{
    return ((compl_T*)node)->cp_prev;
}

    static void
cp_set_prev(void* node, void* prev)
{
    ((compl_T*)node)->cp_prev = (compl_T*)prev;
}

    static int
cp_compare_fuzzy(const void* a, const void* b)
{
    int score_a = ((compl_T*)a)->cp_score;
    int score_b = ((compl_T*)b)->cp_score;
    return (score_b > score_a) ? 1 : (score_b < score_a) ? -1 : 0;
}

    static int
cp_compare_nearest(const void* a, const void* b)
{
    int score_a = ((compl_T*)a)->cp_score;
    int score_b = ((compl_T*)b)->cp_score;
    if (score_a == 0 || score_b == 0)
	return 0;
    return (score_a > score_b) ? 1 : (score_a < score_b) ? -1 : 0;
}

/*
 * Constructs a new string by prepending text from the current line (from
 * startcol to compl_col) to the given source string. Stores the result in
 * dest. Returns OK or FAIL.
 */
    static int
prepend_startcol_text(string_T *dest, string_T *src, int startcol)
{
    int prepend_len = compl_col - startcol;
    int new_length = prepend_len + (int)src->length;

    dest->length = (size_t)new_length;
    dest->string = alloc(new_length + 1);  // +1 for NUL
    if (dest->string == NULL)
    {
	dest->length = 0;
	return FAIL;
    }

    char_u	*line = ml_get(curwin->w_cursor.lnum);

    mch_memmove(dest->string, line + startcol, prepend_len);
    mch_memmove(dest->string + prepend_len, src->string, src->length);
    dest->string[new_length] = NUL;
    return OK;
}

/*
 * Returns the completion leader string adjusted for a specific source's
 * startcol. If the source's startcol is before compl_col, prepends text from
 * the buffer line to the original compl_leader.
 */
    static string_T *
get_leader_for_startcol(compl_T *match, int cached)
{
    static string_T adjusted_leader = {NULL, 0};

    if (match == NULL)
    {
	VIM_CLEAR_STRING(adjusted_leader);
	return NULL;
    }

    if (cpt_sources_array == NULL || compl_leader.string == NULL)
	goto theend;

    int	cpt_idx = match->cp_cpt_source_idx;
    if (cpt_idx < 0 || compl_col <= 0)
	goto theend;
    int	startcol = cpt_sources_array[cpt_idx].cs_startcol;

    if (startcol >= 0 && startcol < compl_col)
    {
	int prepend_len = compl_col - startcol;
	int new_length = prepend_len + (int)compl_leader.length;
	if (cached && (size_t)new_length == adjusted_leader.length
		&& adjusted_leader.string != NULL)
	    return &adjusted_leader;

	VIM_CLEAR_STRING(adjusted_leader);
	if (prepend_startcol_text(&adjusted_leader, &compl_leader,
		    startcol) != OK)
	    goto theend;

	return &adjusted_leader;
    }
theend:
    return &compl_leader;
}

/*
 * Set fuzzy score.
 */
    static void
set_fuzzy_score(void)
{
    compl_T *compl;

    if (!compl_first_match
	    || compl_leader.string == NULL || compl_leader.length == 0)
	return;

    (void)get_leader_for_startcol(NULL, TRUE); // Clear the cache

    compl = compl_first_match;
    do
    {
	compl->cp_score = fuzzy_match_str(compl->cp_str.string,
		get_leader_for_startcol(compl, TRUE)->string);
	compl = compl->cp_next;
    } while (compl != NULL && !is_first_match(compl));
}

/*
 * Sort completion matches, excluding the node that contains the leader.
 */
    static void
sort_compl_match_list(int (*compare)(const void *, const void *))
{
    compl_T     *compl;

    if (!compl_first_match || is_first_match(compl_first_match->cp_next))
	return;

    compl = compl_first_match->cp_prev;
    ins_compl_make_linear();
    if (compl_shows_dir_forward())
    {
	compl_first_match->cp_next->cp_prev = NULL;
	compl_first_match->cp_next = mergesort_list(compl_first_match->cp_next,
		cp_get_next, cp_set_next, cp_get_prev, cp_set_prev, compare);
	compl_first_match->cp_next->cp_prev = compl_first_match;
    }
    else
    {
	compl->cp_prev->cp_next = NULL;
	compl_first_match = mergesort_list(compl_first_match, cp_get_next,
		cp_set_next, cp_get_prev, cp_set_prev, compare);
	compl_T	*tail = compl_first_match;
	while (tail->cp_next != NULL)
	    tail = tail->cp_next;
	tail->cp_next = compl;
	compl->cp_prev = tail;
    }
    (void)ins_compl_make_cyclic();
}

/*
 * Build a popup menu to show the completion matches.
 * Returns the popup menu entry that should be selected. Returns -1 if nothing
 * should be selected.
 */
    static int
ins_compl_build_pum(void)
{
    compl_T     *compl;
    compl_T     *shown_compl = NULL;
    int		did_find_shown_match = FALSE;
    int		shown_match_ok = FALSE;
    int		i = 0;
    int		cur = -1;
    unsigned int cur_cot_flags = get_cot_flags();
    int		compl_no_select = (cur_cot_flags & COT_NOSELECT) != 0
				|| compl_autocomplete;
    int		fuzzy_filter = (cur_cot_flags & COT_FUZZY) != 0;
    compl_T	*match_head = NULL;
    compl_T	*match_tail = NULL;
    compl_T	*match_next = NULL;
    int		*match_count = NULL;
    int		is_forward = compl_shows_dir_forward();
    int		is_cpt_completion = (cpt_sources_array != NULL);
    string_T	*leader;

    // Need to build the popup menu list.
    compl_match_arraysize = 0;

    // If the current match is the original text don't find the first
    // match after it, don't highlight anything.
    if (match_at_original_text(compl_shown_match))
	shown_match_ok = TRUE;

    if (compl_leader.string != NULL
	    && STRCMP(compl_leader.string, compl_orig_text.string) == 0
	    && shown_match_ok == FALSE)
	compl_shown_match = compl_no_select ? compl_first_match
					    : compl_first_match->cp_next;

    if (is_cpt_completion)
    {
	match_count = ALLOC_CLEAR_MULT(int, cpt_sources_count);
	if (match_count == NULL)
	    return -1;
    }

    (void)get_leader_for_startcol(NULL, TRUE); // Clear the cache

    compl = compl_first_match;
    do
    {
	compl->cp_in_match_array = FALSE;

	// Apply 'smartcase' behavior during normal mode
	if (ctrl_x_mode_normal() && !p_inf && compl_leader.string
		&& !ignorecase(compl_leader.string) && !fuzzy_filter)
	    compl->cp_flags &= ~CP_ICASE;

	leader = get_leader_for_startcol(compl, TRUE);

	if (!match_at_original_text(compl)
		&& (leader->string == NULL
		    || ins_compl_equal(compl, leader->string,
			(int)leader->length)
		    || (fuzzy_filter && compl->cp_score > 0)))
	{
	    // Limit number of items from each source if max_items is set.
	    int match_limit_exceeded = FALSE;
	    int cur_source = compl->cp_cpt_source_idx;
	    if (is_forward && cur_source != -1 && is_cpt_completion)
	    {
		match_count[cur_source]++;
		int max_matches = cpt_sources_array[cur_source].cs_max_matches;
		if (max_matches > 0 && match_count[cur_source] > max_matches)
		    match_limit_exceeded = TRUE;
	    }

	    if (!match_limit_exceeded)
	    {
		++compl_match_arraysize;
		compl->cp_in_match_array = TRUE;
		if (match_head == NULL)
		    match_head = compl;
		else
		    match_tail->cp_match_next = compl;
		match_tail = compl;

		if (!shown_match_ok && !fuzzy_filter)
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
		else if (fuzzy_filter)
		{
		    if (i == 0)
			shown_compl = compl;

		    if (!shown_match_ok && compl == compl_shown_match)
		    {
			cur = i;
			shown_match_ok = TRUE;
		    }
		}
		i++;
	    }
	}

	if (compl == compl_shown_match && !fuzzy_filter)
	{
	    did_find_shown_match = TRUE;

	    // When the original text is the shown match don't set
	    // compl_shown_match.
	    if (match_at_original_text(compl))
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
    } while (compl != NULL && !is_first_match(compl));

    vim_free(match_count);

    if (compl_match_arraysize == 0)
	return -1;

    if (fuzzy_filter && !compl_no_select && !shown_match_ok)
    {
	compl_shown_match = shown_compl;
	shown_match_ok = TRUE;
	cur = 0;
    }

    compl_match_array = ALLOC_CLEAR_MULT(pumitem_T, compl_match_arraysize);
    if (compl_match_array == NULL)
	return -1;

    compl = match_head;
    i = 0;
    while (compl != NULL)
    {
	compl_match_array[i].pum_text = compl->cp_text[CPT_ABBR] != NULL
			    ? compl->cp_text[CPT_ABBR] : compl->cp_str.string;
	compl_match_array[i].pum_kind = compl->cp_text[CPT_KIND];
	compl_match_array[i].pum_info = compl->cp_text[CPT_INFO];
	compl_match_array[i].pum_cpt_source_idx = compl->cp_cpt_source_idx;
	compl_match_array[i].pum_user_abbr_hlattr = compl->cp_user_abbr_hlattr;
	compl_match_array[i].pum_user_kind_hlattr = compl->cp_user_kind_hlattr;
	compl_match_array[i++].pum_extra = compl->cp_text[CPT_MENU] != NULL
			    ? compl->cp_text[CPT_MENU] : compl->cp_fname;
	match_next = compl->cp_match_next;
	compl->cp_match_next = NULL;
	compl = match_next;
    }

    if (!shown_match_ok)    // no displayed match at all
	cur = -1;

    return cur;
}

/*
 * Show the popup menu for the list of matches.
 * Also adjusts "compl_shown_match" to an entry that is actually displayed.
 */
    void
ins_compl_show_pum(void)
{
    int		i;
    int		cur = -1;
    colnr_T	col;

    if (!pum_wanted() || !pum_enough_matches())
	return;

    // Update the screen later, before drawing the popup menu over it.
    pum_call_update_screen();

    if (compl_match_array == NULL)
	// Need to build the popup menu list.
	cur = ins_compl_build_pum();
    else
    {
	// popup menu already exists, only need to find the current item.
	for (i = 0; i < compl_match_arraysize; ++i)
	{
	    if (compl_match_array[i].pum_text == compl_shown_match->cp_str.string
		    || compl_match_array[i].pum_text
				      == compl_shown_match->cp_text[CPT_ABBR])
	    {
		cur = i;
		break;
	    }
	}
    }

    if (compl_match_array == NULL)
    {
#ifdef FEAT_EVAL
	if (compl_started && has_completechanged())
	    trigger_complete_changed_event(cur);
#endif
	return;
    }

    // In Replace mode when a $ is displayed at the end of the line only
    // part of the screen would be updated.  We do need to redraw here.
    dollar_vcol = -1;

    // Compute the screen column of the start of the completed text.
    // Use the cursor to get all wrapping and other settings right.
    col = curwin->w_cursor.col;
    curwin->w_cursor.col = compl_col;
    compl_selected_item = cur;
    pum_display(compl_match_array, compl_match_arraysize, cur);
    curwin->w_cursor.col = col;

    // After adding leader, set the current match to shown match.
    if (compl_started && compl_curr_match != compl_shown_match)
	compl_curr_match = compl_shown_match;

#ifdef FEAT_EVAL
    if (has_completechanged())
	trigger_complete_changed_event(cur);
#endif
}

#define DICT_FIRST	(1)	// use just first element in "dict"
#define DICT_EXACT	(2)	// "dict" is the exact name of a file

/*
 * Get current completion leader
 */
    char_u *
ins_compl_leader(void)
{
    return compl_leader.string != NULL ? compl_leader.string : compl_orig_text.string;
}

/*
 * Get current completion leader length
 */
    size_t
ins_compl_leader_len(void)
{
    return compl_leader.string != NULL ? compl_leader.length : compl_orig_text.length;
}

/*
 * Add any identifiers that match the given pattern "pat" in the list of
 * dictionary files "dict_start" to the list of completions.
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

	if (pat_esc == NULL)
	    goto theend;
	size_t len = STRLEN(pat_esc) + 10;
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
	regmatch.regprog = vim_regcomp(pat, magic_isset() ? RE_MAGIC : 0);
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
			    (cfc_has_mode() ? NULL : &regmatch), buf, &dir);
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

/*
 * Add all the words in the line "*buf_arg" from the thesaurus file "fname"
 * skipping the word at 'skip_word'.  Returns OK on success.
 */
    static int
thesaurus_add_words_in_line(
    char_u	*fname,
    char_u	**buf_arg,
    int		dir,
    char_u	*skip_word)
{
    int		status = OK;
    char_u	*ptr;
    char_u	*wstart;

    // Add the other matches on the line
    ptr = *buf_arg;
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
	if (wstart != skip_word)
	{
	    status = ins_compl_add_infercase(wstart, (int)(ptr - wstart), p_ic,
							fname, dir, FALSE, 0);
	    if (status == FAIL)
		break;
	}
    }

    *buf_arg = ptr;
    return status;
}

/*
 * Process "count" dictionary/thesaurus "files" and add the text matching
 * "regmatch".
 */
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
    char_u	*leader = NULL;
    int		leader_len = 0;
    int		in_fuzzy_collect = cfc_has_mode();
    int		score = 0;
    int		len = 0;
    char_u	*line_end = NULL;

    if (in_fuzzy_collect)
    {
	leader = ins_compl_leader();
	leader_len = (int)ins_compl_leader_len();
    }

    for (i = 0; i < count && !got_int && !compl_interrupted
	    && !compl_time_slice_expired; i++)
    {
	fp = mch_fopen((char *)files[i], "r");  // open dictionary file
	if (flags != DICT_EXACT && !shortmess(SHM_COMPLETIONSCAN)
		&& !compl_autocomplete)
	{
	    msg_hist_off = TRUE;	// reset in msg_trunc_attr()
	    vim_snprintf((char *)IObuff, IOSIZE,
			      _("Scanning dictionary: %s"), (char *)files[i]);
	    (void)msg_trunc_attr((char *)IObuff, TRUE, HL_ATTR(HLF_R));
	}

	if (fp == NULL)
	    continue;

	// Read dictionary file line by line.
	// Check each line for a match.
	while (!got_int && !compl_interrupted && !compl_time_slice_expired
	       && !vim_fgets(buf, LSIZE, fp))
	{
	    ptr = buf;
	    if (regmatch != NULL)
	    {
		while (vim_regexec(regmatch, buf, (colnr_T)(ptr - buf)))
		{
		    ptr = regmatch->startp[0];
		    ptr = ctrl_x_mode_line_or_eval() ? find_line_end(ptr)
						    : find_word_end(ptr);
		    add_r = ins_compl_add_infercase(regmatch->startp[0],
			    (int)(ptr - regmatch->startp[0]),
			    p_ic, files[i], *dir, FALSE, 0);
		    if (thesaurus)
		    {
			// For a thesaurus, add all the words in the line
			ptr = buf;
			add_r = thesaurus_add_words_in_line(files[i], &ptr, *dir,
							    regmatch->startp[0]);
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
	    }
	    else if (in_fuzzy_collect && leader_len > 0)
	    {
		line_end = find_line_end(ptr);
		while (ptr < line_end)
		{
		    if (fuzzy_match_str_in_line(&ptr, leader, &len, NULL, &score))
		    {
			char_u *end_ptr = ctrl_x_mode_line_or_eval()
					? find_line_end(ptr) : find_word_end(ptr);
			add_r = ins_compl_add_infercase(ptr, (int)(end_ptr - ptr),
					    p_ic, files[i], *dir, FALSE, score);
			if (add_r == FAIL)
			    break;
			ptr = end_ptr;  // start from next word
			if (compl_get_longest && ctrl_x_mode_normal()
				&& compl_first_match->cp_next
				&& score == compl_first_match->cp_next->cp_score)
			    compl_num_bests++;
		    }
		}
	    }
	    line_breakcheck();
	    ins_compl_check_keys(50, FALSE);
	}
	fclose(fp);
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
    {
	while (*ptr != NUL && *ptr != '\n' && mb_get_class(ptr) <= 1)
	    ptr += (*mb_ptr2len)(ptr);
    }
    else
    {
	while (*ptr != NUL && *ptr != '\n' && !vim_iswordc(*ptr))
	    ++ptr;
    }
    return ptr;
}

/*
 * Find the end of the word.  Assumes it starts inside a word.
 * Returns a pointer to just after the word.
 */
    char_u *
find_word_end(char_u *ptr)
{
    if (has_mbyte)
    {
	int start_class = mb_get_class(ptr);
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
    char_u *
find_line_end(char_u *ptr)
{
    char_u *s = ptr + STRLEN(ptr);
    while (s > ptr && (s[-1] == CAR || s[-1] == NL))
	--s;
    return s;
}

/*
 * Free a completion item in the list
 */
    static void
ins_compl_item_free(compl_T *match)
{
    VIM_CLEAR_STRING(match->cp_str);
    // several entries may use the same fname, free it just once.
    if (match->cp_flags & CP_FREE_FNAME)
	vim_free(match->cp_fname);
    for (int i = 0; i < CPT_COUNT; ++i)
	vim_free(match->cp_text[i]);
#ifdef FEAT_EVAL
    clear_tv(&match->cp_user_data);
#endif
    vim_free(match);
}

/*
 * Free the list of completions
 */
    static void
ins_compl_free(void)
{
    compl_T *match;

    VIM_CLEAR_STRING(compl_pattern);
    VIM_CLEAR_STRING(compl_leader);

    if (compl_first_match == NULL)
	return;

    ins_compl_del_pum();
    pum_clear();

    compl_curr_match = compl_first_match;
    do
    {
	match = compl_curr_match;
	compl_curr_match = compl_curr_match->cp_next;
	ins_compl_item_free(match);
    } while (compl_curr_match != NULL && !is_first_match(compl_curr_match));
    compl_first_match = compl_curr_match = NULL;
    compl_shown_match = NULL;
    compl_old_match = NULL;
}

/*
 * Reset/clear the completion state.
 */
    void
ins_compl_clear(void)
{
    compl_cont_status = 0;
    compl_started = FALSE;
    compl_cfc_longest_ins = FALSE;
    compl_matches = 0;
    compl_selected_item = -1;
    compl_ins_end_col = 0;
    compl_curr_win = NULL;
    compl_curr_buf = NULL;
    VIM_CLEAR_STRING(compl_pattern);
    VIM_CLEAR_STRING(compl_leader);
    edit_submode_extra = NULL;
    VIM_CLEAR_STRING(compl_orig_text);
    compl_enter_selects = FALSE;
    cpt_sources_clear();
    compl_autocomplete = FALSE;
    compl_from_nonkeyword = FALSE;
    compl_num_bests = 0;
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
 * Return True when wp is the actual completion window
 */
    int
ins_compl_win_active(win_T *wp)
{
    return ins_compl_active() && wp == compl_curr_win
	&& wp->w_buffer == compl_curr_buf;
}

/*
 * Selected one of the matches.  When FALSE, the match was either edited or
 * using the longest common string.
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
 * Return the length in bytes of the text being completed
 */
    int
ins_compl_len(void)
{
    return compl_length;
}

/*
 * Return TRUE when the 'completeopt' "preinsert" flag is in effect,
 * otherwise return FALSE.
 */
    static int
ins_compl_has_preinsert(void)
{
    int cur_cot_flags = get_cot_flags();
    return (cur_cot_flags & (COT_PREINSERT | COT_FUZZY | COT_MENUONE))
	== (COT_PREINSERT | COT_MENUONE) && !compl_autocomplete;
}

/*
 * Returns TRUE if the pre-insert effect is valid and the cursor is within
 * the `compl_ins_end_col` range.
 */
    int
ins_compl_preinsert_effect(void)
{
    if (!ins_compl_has_preinsert())
	return FALSE;

    return curwin->w_cursor.col < compl_ins_end_col;
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

    if (ins_compl_preinsert_effect())
	ins_compl_delete();

    line = ml_get_curline();
    p = line + curwin->w_cursor.col;
    MB_PTR_BACK(line, p);

    // Stop completion when the whole word was deleted.  For Omni completion
    // allow the word to be deleted, we won't match everything.
    // Respect the 'backspace' option.
    if ((int)(p - line) - (int)compl_col < 0
	    || ((int)(p - line) - (int)compl_col == 0 && !ctrl_x_mode_omni())
	    || ctrl_x_mode_eval()
	    || (!can_bs(BS_START) && (int)(p - line) - (int)compl_col
							- compl_length < 0))
	return K_BS;

    // Deleted more than what was used to find matches or didn't finish
    // finding all matches: need to look for matches all over again.
    if (curwin->w_cursor.col <= compl_col + compl_length
						  || ins_compl_need_restart())
	ins_compl_restart();

    VIM_CLEAR_STRING(compl_leader);
    compl_leader.length = (size_t)((p - line) - compl_col);
    compl_leader.string = vim_strnsave(line + compl_col, compl_leader.length);
    if (compl_leader.string == NULL)
    {
	compl_leader.length = 0;
	return K_BS;
    }

    ins_compl_new_leader();
    if (compl_shown_match != NULL)
	// Make sure current match is not a hidden item.
	compl_curr_match = compl_shown_match;
    return NUL;
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
	|| ((ctrl_x_mode_function() || ctrl_x_mode_omni())
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
    int	    cur_cot_flags = get_cot_flags();
    int	    save_w_wrow = curwin->w_wrow;
    int	    save_w_leftcol = curwin->w_leftcol;

    ins_compl_del_pum();
    ins_compl_delete();
    ins_compl_insert_bytes(compl_leader.string + get_compl_len(), -1);
    compl_used_match = FALSE;

    if (compl_started)
    {
	ins_compl_set_original_text(compl_leader.string, compl_leader.length);
	if (is_cpt_func_refresh_always())
	    cpt_compl_refresh();
    }
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
	save_w_wrow = curwin->w_wrow;
	save_w_leftcol = curwin->w_leftcol;
	compl_restarting = TRUE;
	if (p_ac)
	    compl_autocomplete = TRUE;
	if (ins_complete(Ctrl_N, FALSE) == FAIL)
	    compl_cont_status = 0;
	compl_restarting = FALSE;
    }

    // When 'cot' contains "fuzzy" set the cp_score and maybe sort
    if (cur_cot_flags & COT_FUZZY)
    {
	set_fuzzy_score();
	// Sort the matches linked list based on fuzzy score
	if (!(cur_cot_flags & COT_NOSORT))
	{
	    sort_compl_match_list(cp_compare_fuzzy);
	    if ((cur_cot_flags & (COT_NOINSERT | COT_NOSELECT)) == COT_NOINSERT
		    && compl_first_match)
	    {
		compl_shown_match = compl_first_match;
		if (compl_shows_dir_forward())
		    compl_shown_match = compl_first_match->cp_next;
	    }
	}
    }

    compl_enter_selects = !compl_used_match && compl_selected_item != -1;

    // Show the popup menu with a different set of matches.
    if (!compl_interrupted)
	show_pum(save_w_wrow, save_w_leftcol);

    // Don't let Enter select the original text when there is no popup menu.
    if (compl_match_array == NULL)
	compl_enter_selects = FALSE;
    else if (ins_compl_has_preinsert() && compl_leader.length > 0)
	ins_compl_insert(TRUE);
}

/*
 * Return the length of the completion, from the completion start column to
 * the cursor column.  Making sure it never goes below zero.
 */
    static int
get_compl_len(void)
{
    int off = (int)curwin->w_cursor.col - (int)compl_col;
    return MAX(0, off);
}

/*
 * Append one character to the match leader.  May reduce the number of
 * matches.
 */
    void
ins_compl_addleader(int c)
{
    int	    cc;

    if (ins_compl_preinsert_effect())
	ins_compl_delete();

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
	VIM_CLEAR_STRING(compl_leader);
	compl_leader.length = (size_t)(curwin->w_cursor.col - compl_col);
	compl_leader.string = vim_strnsave(ml_get_curline() + compl_col,
					     compl_leader.length);
	if (compl_leader.string == NULL)
	{
	    compl_leader.length = 0;
	    return;
	}

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
    cpt_sources_clear();
    compl_autocomplete = FALSE;
    compl_from_nonkeyword = FALSE;
    compl_num_bests = 0;
}

/*
 * Set the first match, the original text.
 */
    static void
ins_compl_set_original_text(char_u *str, size_t len)
{
    // Replace the original text entry.
    // The CP_ORIGINAL_TEXT flag is either at the first item or might possibly
    // be at the last item for backward completion
    if (match_at_original_text(compl_first_match))	// safety check
    {
	char_u	*p = vim_strnsave(str, len);
	if (p != NULL)
	{
	    VIM_CLEAR_STRING(compl_first_match->cp_str);
	    compl_first_match->cp_str.string = p;
	    compl_first_match->cp_str.length = len;
	}
    }
    else if (compl_first_match->cp_prev != NULL
	    && match_at_original_text(compl_first_match->cp_prev))
    {
	char_u *p = vim_strnsave(str, len);
	if (p != NULL)
	{
	    VIM_CLEAR_STRING(compl_first_match->cp_prev->cp_str);
	    compl_first_match->cp_prev->cp_str.string = p;
	    compl_first_match->cp_prev->cp_str.length = len;
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

    p = compl_shown_match->cp_str.string;
    if ((int)compl_shown_match->cp_str.length <= len)   // the match is too short
    {
	size_t	plen;
	compl_T	*cp;

	// When still at the original match use the first entry that matches
	// the leader.
	if (!match_at_original_text(compl_shown_match))
	    return;

	p = NULL;
	plen = 0;
	for (cp = compl_shown_match->cp_next; cp != NULL
		&& !is_first_match(cp); cp = cp->cp_next)
	{
	    if (compl_leader.string == NULL
		    || ins_compl_equal(cp, compl_leader.string,
			(int)compl_leader.length))
	    {
		p = cp->cp_str.string;
		plen = cp->cp_str.length;
		break;
	    }
	}
	if (p == NULL || (int)plen <= len)
	    return;
    }
    p += len;
    c = PTR2CHAR(p);
    ins_compl_addleader(c);
}

/*
 * Set the CTRL-X completion mode based on the key "c" typed after a CTRL-X.
 * Uses the global variables: ctrl_x_mode, edit_submode, edit_submode_pre,
 * compl_cont_mode and compl_cont_status.
 * Returns TRUE when the character is not to be inserted.
 */
    static int
set_ctrl_x_mode(int c)
{
    int	    retval = FALSE;

    switch (c)
    {
	case Ctrl_E:
	case Ctrl_Y:
	    // scroll the window one line up or down
	    ctrl_x_mode = CTRL_X_SCROLL;
	    if (!(State & REPLACE_FLAG))
		edit_submode = (char_u *)_(" (insert) Scroll (^E/^Y)");
	    else
		edit_submode = (char_u *)_(" (replace) Scroll (^E/^Y)");
	    edit_submode_pre = NULL;
	    showmode();
	    break;
	case Ctrl_L:
	    // complete whole line
	    ctrl_x_mode = CTRL_X_WHOLE_LINE;
	    break;
	case Ctrl_F:
	    // complete filenames
	    ctrl_x_mode = CTRL_X_FILES;
	    break;
	case Ctrl_K:
	    // complete words from a dictionary
	    ctrl_x_mode = CTRL_X_DICTIONARY;
	    break;
	case Ctrl_R:
	    // When CTRL-R is followed by '=', don't trigger register completion
	    // This allows expressions like <C-R>=func()<CR> to work normally
	    if (vpeekc() == '=')
		break;
	    ctrl_x_mode = CTRL_X_REGISTER;
	    break;
	case Ctrl_T:
	    // complete words from a thesaurus
	    ctrl_x_mode = CTRL_X_THESAURUS;
	    break;
#ifdef FEAT_COMPL_FUNC
	case Ctrl_U:
	    // user defined completion
	    ctrl_x_mode = CTRL_X_FUNCTION;
	    break;
	case Ctrl_O:
	    // omni completion
	    ctrl_x_mode = CTRL_X_OMNI;
	    break;
#endif
	case 's':
	case Ctrl_S:
	    // complete spelling suggestions
	    ctrl_x_mode = CTRL_X_SPELL;
#ifdef FEAT_SPELL
	    ++emsg_off;	// Avoid getting the E756 error twice.
	    spell_back_to_badword();
	    --emsg_off;
#endif
	    break;
	case Ctrl_RSB:
	    // complete tag names
	    ctrl_x_mode = CTRL_X_TAGS;
	    break;
#ifdef FEAT_FIND_ID
	case Ctrl_I:
	case K_S_TAB:
	    // complete keywords from included files
	    ctrl_x_mode = CTRL_X_PATH_PATTERNS;
	    break;
	case Ctrl_D:
	    // complete definitions from included files
	    ctrl_x_mode = CTRL_X_PATH_DEFINES;
	    break;
#endif
	case Ctrl_V:
	case Ctrl_Q:
	    // complete vim commands
	    ctrl_x_mode = CTRL_X_CMDLINE;
	    break;
	case Ctrl_Z:
	    // stop completion
	    ctrl_x_mode = CTRL_X_NORMAL;
	    edit_submode = NULL;
	    showmode();
	    retval = TRUE;
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

    return retval;
}

/*
 * Trigger CompleteDone event and adds relevant information to v:event
 */
    static void
trigger_complete_done_event(int mode UNUSED, char_u *word UNUSED)
{
#if defined(FEAT_EVAL)
    save_v_event_T	save_v_event;
    dict_T		*v_event = get_v_event(&save_v_event);
    char_u		*mode_str = NULL;

    mode = mode & ~CTRL_X_WANT_IDENT;
    if (ctrl_x_mode_names[mode])
	mode_str = (char_u *)ctrl_x_mode_names[mode];

    (void)dict_add_string(v_event, "complete_word",
				word == NULL ? (char_u *)"" : word);
    (void)dict_add_string(v_event, "complete_type",
				mode_str != NULL ? mode_str : (char_u *)"");

    dict_set_items_ro(v_event);
#endif
    ins_apply_autocmds(EVENT_COMPLETEDONE);

#if defined(FEAT_EVAL)
    restore_v_event(v_event, &save_v_event);
#endif
}

/*
 * Stop insert completion mode
 */
    static int
ins_compl_stop(int c, int prev_mode, int retval)
{
    int		want_cindent;
    char_u	*word = NULL;

    // Remove pre-inserted text when present.
    if (ins_compl_preinsert_effect() && ins_compl_win_active(curwin))
	ins_compl_delete();

    // Get here when we have finished typing a sequence of ^N and
    // ^P or other completion characters in CTRL-X mode.  Free up
    // memory that was used, and make sure we can redo the insert.
    if (compl_curr_match != NULL || compl_leader.string != NULL || c == Ctrl_E)
    {
	char_u	*ptr = NULL;

	// If any of the original typed text has been changed, eg when
	// ignorecase is set, we must add back-spaces to the redo
	// buffer.  We add as few as necessary to delete just the part
	// of the original text that has changed.
	// When using the longest match, edited the match or used
	// CTRL-E then don't use the current match.
	if (compl_curr_match != NULL && compl_used_match && c != Ctrl_E)
	    ptr = compl_curr_match->cp_str.string;
	ins_compl_fixRedoBufForLeader(ptr);
    }

    want_cindent = (get_can_cindent() && cindent_on());

    // When completing whole lines: fix indent for 'cindent'.
    // Otherwise, break line if it's too long.
    if (compl_cont_mode == CTRL_X_WHOLE_LINE)
    {
	// re-indent the current line
	if (want_cindent)
	{
	    do_c_expr_indent();
	    want_cindent = FALSE;	// don't do it again
	}
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
    {
	word = vim_strsave(compl_shown_match->cp_str.string);
	retval = TRUE;
    }

    // CTRL-E means completion is Ended, go back to the typed text.
    // but only do this, if the Popup is still visible
    if (c == Ctrl_E)
    {
	char_u *p = NULL;
	size_t	plen = 0;

	ins_compl_delete();
	if (compl_leader.string != NULL)
	{
	    p = compl_leader.string;
	    plen = compl_leader.length;
	}
	else if (compl_first_match != NULL)
	{
	    p = compl_orig_text.string;
	    plen = compl_orig_text.length;
	}
	if (p != NULL)
	{
	    int	    compl_len = get_compl_len();

	    if ((int)plen > compl_len)
		ins_compl_insert_bytes(p + compl_len, (int)plen - compl_len);
	}
	retval = TRUE;
    }

    auto_format(FALSE, TRUE);

    // Trigger the CompleteDonePre event to give scripts a chance to
    // act upon the completion before clearing the info, and restore
    // ctrl_x_mode, so that complete_info() can be used.
    ctrl_x_mode = prev_mode;
    ins_apply_autocmds(EVENT_COMPLETEDONEPRE);

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
    compl_autocomplete = FALSE;
    compl_from_nonkeyword = FALSE;
    compl_best_matches = 0;

    if (c == Ctrl_C && cmdwin_type != 0)
	// Avoid the popup menu remains displayed when leaving the
	// command line window.
	update_screen(0);
    // Indent now if a key was typed that is in 'cinkeys'.
    if (want_cindent && in_cinkeys(KEY_COMPLETE, ' ', inindent(0)))
	do_c_expr_indent();
    // Trigger the CompleteDone event to give scripts a chance to act
    // upon the end of completion.
    trigger_complete_done_event(prev_mode, word);
    vim_free(word);

    return retval;
}

/*
 * Cancel completion.
 */
    int
ins_compl_cancel(void)
{
    return ins_compl_stop(' ', ctrl_x_mode, TRUE);
}

/*
 * Prepare for Insert mode completion, or stop it.
 * Called just after typing a character in Insert mode.
 * Returns TRUE when the character is not to be inserted;
 */
    int
ins_compl_prep(int c)
{
    int	    retval = FALSE;
    int	    prev_mode = ctrl_x_mode;

    // Forget any previous 'special' messages if this is actually
    // a ^X mode key - bar ^R, in which case we wait to see what it gives us.
    if (c != Ctrl_R && vim_is_ctrl_x_key(c))
	edit_submode_extra = NULL;

    // Ignore end of Select mode mapping and mouse scroll/movement.
    if (c == K_SELECT || c == K_MOUSEDOWN || c == K_MOUSEUP
	    || c == K_MOUSELEFT || c == K_MOUSERIGHT || c == K_MOUSEMOVE
	    || c == K_COMMAND || c == K_SCRIPT_COMMAND)
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

    if (ctrl_x_mode == CTRL_X_CMDLINE_CTRL_X && c != Ctrl_X)
    {
	if (c == Ctrl_V || c == Ctrl_Q || c == Ctrl_Z || ins_compl_pum_key(c)
		|| !vim_is_ctrl_x_key(c))
	{
	    // Not starting another completion mode.
	    ctrl_x_mode = CTRL_X_CMDLINE;

	    // CTRL-X CTRL-Z should stop completion without inserting anything
	    if (c == Ctrl_Z)
		retval = TRUE;
	}
	else
	{
	    ctrl_x_mode = CTRL_X_CMDLINE;

	    // Other CTRL-X keys first stop completion, then start another
	    // completion mode.
	    ins_compl_prep(' ');
	    ctrl_x_mode = CTRL_X_NOT_DEFINED_YET;
	}
    }

    // Set "compl_get_longest" when finding the first matches.
    if (ctrl_x_mode_not_defined_yet()
			   || (ctrl_x_mode_normal() && !compl_started))
    {
	compl_get_longest = (get_cot_flags() & COT_LONGEST) != 0;
	compl_used_match = TRUE;
    }

    if (ctrl_x_mode_not_defined_yet())
	// We have just typed CTRL-X and aren't quite sure which CTRL-X mode
	// it will be yet.  Now we decide.
	retval = set_ctrl_x_mode(c);
    else if (ctrl_x_mode_not_default())
    {
	// We're already in CTRL-X mode, do we stay in it?
	if (!vim_is_ctrl_x_key(c))
	{
	    ctrl_x_mode = ctrl_x_mode_scroll() ? CTRL_X_NORMAL : CTRL_X_FINISHED;
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
	if ((ctrl_x_mode_normal() && c != Ctrl_N && c != Ctrl_P
				       && c != Ctrl_R && !ins_compl_pum_key(c))
		|| ctrl_x_mode == CTRL_X_FINISHED)
	    retval = ins_compl_stop(c, prev_mode, retval);
    }
    else if (ctrl_x_mode == CTRL_X_LOCAL_MSG)
	// Trigger the CompleteDone event to give scripts a chance to act
	// upon the (possibly failed) completion.
	trigger_complete_done_event(ctrl_x_mode, NULL);

    may_trigger_modechanged();

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
    int	    len = 0;
    char_u  *p;
    char_u  *ptr = ptr_arg;

    if (ptr == NULL)
    {
	if (compl_leader.string != NULL)
	    ptr = compl_leader.string;
	else
	    return;  // nothing to do
    }
    if (compl_orig_text.string != NULL)
    {
	p = compl_orig_text.string;
	// Find length of common prefix between original text and new completion
	while (p[len] != NUL && p[len] == ptr[len])
	    len++;
	// Adjust length to not break inside a multi-byte character
	if (len > 0)
	    len -= (*mb_head_off)(p, p + len);
	// Add backspace characters for each remaining character in
	// original text
	for (p += len; *p != NUL; MB_PTR_ADV(p))
	    AppendCharToRedobuff(K_BS);
    }
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
    static win_T    *wp = NULL;
    int		    skip_buffer;

    if (flag == 'w')		// just windows
    {
	if (buf == curbuf || !win_valid(wp))
	    // first call for this flag/expansion or window was closed
	    wp = curwin;

	while (TRUE)
	{
	    // Move to next window (wrap to first window if at the end)
	    wp = (wp->w_next != NULL) ? wp->w_next : firstwin;
	    // Break if we're back at start or found an unscanned buffer
	    if (wp == curwin || !wp->w_buffer->b_scanned)
		break;
	}
	buf = wp->w_buffer;
    }
    else
    {
	// 'b' (just loaded buffers), 'u' (just non-loaded buffers) or 'U'
	// (unlisted buffers)
	// When completing whole lines skip unloaded buffers.
	while (TRUE)
	{
	    // Move to next buffer (wrap to first buffer if at the end)
	    buf = (buf->b_next != NULL) ? buf->b_next : firstbuf;
	    // Break if we're back at start buffer
	    if (buf == curbuf)
		break;

	    // Check buffer conditions based on flag
	    if (flag == 'U')
		skip_buffer = buf->b_p_bl;
	    else
		skip_buffer = !buf->b_p_bl ||
		    (buf->b_ml.ml_mfp == NULL) != (flag == 'u');

	    // Break if we found a buffer that matches our criteria
	    if (!skip_buffer && !buf->b_scanned)
		break;
	}
    }
    return buf;
}

#ifdef FEAT_COMPL_FUNC

# ifdef FEAT_EVAL
static callback_T cfu_cb;	    // 'completefunc' callback function
static callback_T ofu_cb;	    // 'omnifunc' callback function
static callback_T tsrfu_cb;	    // 'thesaurusfunc' callback function
# endif

/*
 * Copy a global callback function to a buffer local callback.
 */
    static void
copy_global_to_buflocal_cb(callback_T *globcb, callback_T *bufcb)
{
    free_callback(bufcb);
    if (globcb->cb_name != NULL && *globcb->cb_name != NUL)
	copy_callback(bufcb, globcb);
}

/*
 * Parse the 'completefunc' option value and set the callback function.
 * Invoked when the 'completefunc' option is set. The option value can be a
 * name of a function (string), or function(<name>) or funcref(<name>) or a
 * lambda expression.
 */
    char *
did_set_completefunc(optset_T *args UNUSED)
{
    if (option_set_callback_func(curbuf->b_p_cfu, &cfu_cb) == FAIL)
	return e_invalid_argument;

    set_buflocal_cfu_callback(curbuf);

    return NULL;
}

/*
 * Copy the global 'completefunc' callback function to the buffer-local
 * 'completefunc' callback for "buf".
 */
    void
set_buflocal_cfu_callback(buf_T *buf UNUSED)
{
# ifdef FEAT_EVAL
    copy_global_to_buflocal_cb(&cfu_cb, &buf->b_cfu_cb);
# endif
}

/*
 * Parse the 'omnifunc' option value and set the callback function.
 * Invoked when the 'omnifunc' option is set. The option value can be a
 * name of a function (string), or function(<name>) or funcref(<name>) or a
 * lambda expression.
 */
    char *
did_set_omnifunc(optset_T *args UNUSED)
{
    if (option_set_callback_func(curbuf->b_p_ofu, &ofu_cb) == FAIL)
	return e_invalid_argument;

    set_buflocal_ofu_callback(curbuf);
    return NULL;
}

/*
 * Copy the global 'omnifunc' callback function to the buffer-local 'omnifunc'
 * callback for "buf".
 */
    void
set_buflocal_ofu_callback(buf_T *buf UNUSED)
{
# ifdef FEAT_EVAL
    copy_global_to_buflocal_cb(&ofu_cb, &buf->b_ofu_cb);
# endif
}

/*
 * Parse the 'thesaurusfunc' option value and set the callback function.
 * Invoked when the 'thesaurusfunc' option is set. The option value can be a
 * name of a function (string), or function(<name>) or funcref(<name>) or a
 * lambda expression.
 */
    char *
did_set_thesaurusfunc(optset_T *args UNUSED)
{
    int	retval;

    if (args->os_flags & OPT_LOCAL)
	// buffer-local option set
	retval = option_set_callback_func(curbuf->b_p_tsrfu,
							&curbuf->b_tsrfu_cb);
    else
    {
	// global option set
	retval = option_set_callback_func(p_tsrfu, &tsrfu_cb);
	// when using :set, free the local callback
	if (!(args->os_flags & OPT_GLOBAL))
	    free_callback(&curbuf->b_tsrfu_cb);
    }

    return retval == FAIL ? e_invalid_argument : NULL;
}

/*
 * Mark the global 'completefunc' 'omnifunc' and 'thesaurusfunc' callbacks with
 * "copyID" so that they are not garbage collected.
 */
    int
set_ref_in_insexpand_funcs(int copyID)
{
    int abort = set_ref_in_callback(&cfu_cb, copyID);
    abort = abort || set_ref_in_callback(&ofu_cb, copyID);
    abort = abort || set_ref_in_callback(&tsrfu_cb, copyID);

    return abort;
}

/*
 * Get the user-defined completion function name for completion "type"
 */
    static char_u *
get_complete_funcname(int type)
{
    switch (type)
    {
	case CTRL_X_FUNCTION:
	    return curbuf->b_p_cfu;
	case CTRL_X_OMNI:
	    return curbuf->b_p_ofu;
	case CTRL_X_THESAURUS:
	    return *curbuf->b_p_tsrfu == NUL ? p_tsrfu : curbuf->b_p_tsrfu;
	default:
	    return (char_u *)"";
    }
}

/*
 * Get the callback to use for insert mode completion.
 */
    static callback_T *
get_insert_callback(int type)
{
    if (type == CTRL_X_FUNCTION)
	return &curbuf->b_cfu_cb;
    if (type == CTRL_X_OMNI)
	return &curbuf->b_ofu_cb;
    // CTRL_X_THESAURUS
    return (*curbuf->b_p_tsrfu != NUL) ? &curbuf->b_tsrfu_cb : &tsrfu_cb;
}

/*
 * Execute user defined complete function 'completefunc', 'omnifunc' or
 * 'thesaurusfunc', and get matches in "matches".
 * "type" can be one of CTRL_X_OMNI, CTRL_X_FUNCTION, or CTRL_X_THESAURUS.
 * Callback function "cb" is set if triggered by a function in the 'cpt'
 * option; otherwise, it is NULL.
 */
    static void
expand_by_function(int type, char_u *base, callback_T *cb)
{
    list_T      *matchlist = NULL;
    dict_T	*matchdict = NULL;
    typval_T	args[3];
    char_u	*funcname;
    pos_T	pos;
    typval_T	rettv;
    int		save_State = State;
    int		retval;
    int		is_cpt_function = (cb != NULL);

    if (!is_cpt_function)
    {
	funcname = get_complete_funcname(type);
	if (*funcname == NUL)
	    return;
	cb = get_insert_callback(type);
    }

    // Call 'completefunc' to obtain the list of matches.
    args[0].v_type = VAR_NUMBER;
    args[0].vval.v_number = 0;
    args[1].v_type = VAR_STRING;
    args[1].vval.v_string = base != NULL ? base : (char_u *)"";
    args[2].v_type = VAR_UNKNOWN;

    pos = curwin->w_cursor;
    // Lock the text to avoid weird things from happening.  Also disallow
    // switching to another window, it should not be needed and may end up in
    // Insert mode in another buffer.
    ++textlock;

    retval = call_callback(cb, 0, &rettv, 2, args);

    // Call a function, which returns a list or dict.
    if (retval == OK)
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
    --textlock;

    curwin->w_cursor = pos;	// restore the cursor position
    check_cursor();  // make sure cursor position is valid, just in case
    validate_cursor();
    if (!EQUAL_POS(curwin->w_cursor, pos))
    {
	emsg(_(e_complete_function_deleted_text));
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

    static inline int
get_user_highlight_attr(char_u *hlname)
{
    if (hlname != NULL && *hlname != NUL)
	return syn_name2attr(hlname);
    return -1;
}
/*
 * Add a match to the list of matches from a typeval_T.
 * If the given string is already in the list of completions, then return
 * NOTDONE, otherwise add it to the list and return OK.  If there is an error,
 * maybe because alloc() returns NULL, then FAIL is returned.
 * When "fast" is TRUE use fast_breakcheck() instead of ui_breakcheck().
 */
    static int
ins_compl_add_tv(typval_T *tv, int dir, int fast)
{
    char_u	*word;
    int		dup = FALSE;
    int		empty = FALSE;
    int		flags = fast ? CP_FAST : 0;
    char_u	*(cptext[CPT_COUNT]);
    typval_T	user_data;
    int		status;
    char_u	*user_abbr_hlname;
    char_u	*user_kind_hlname;
    int		user_hl[2] = { -1, -1 };

    user_data.v_type = VAR_UNKNOWN;
    if (tv->v_type == VAR_DICT && tv->vval.v_dict != NULL)
    {
	word = dict_get_string(tv->vval.v_dict, "word", FALSE);
	cptext[CPT_ABBR] = dict_get_string(tv->vval.v_dict, "abbr", FALSE);
	cptext[CPT_MENU] = dict_get_string(tv->vval.v_dict, "menu", FALSE);
	cptext[CPT_KIND] = dict_get_string(tv->vval.v_dict, "kind", FALSE);
	cptext[CPT_INFO] = dict_get_string(tv->vval.v_dict, "info", FALSE);

	user_abbr_hlname = dict_get_string(tv->vval.v_dict, "abbr_hlgroup", FALSE);
	user_hl[0] = get_user_highlight_attr(user_abbr_hlname);

	user_kind_hlname = dict_get_string(tv->vval.v_dict, "kind_hlgroup", FALSE);
	user_hl[1] = get_user_highlight_attr(user_kind_hlname);

	dict_get_tv(tv->vval.v_dict, "user_data", &user_data);
	if (dict_get_string(tv->vval.v_dict, "icase", FALSE) != NULL
				  && dict_get_number(tv->vval.v_dict, "icase"))
	    flags |= CP_ICASE;
	if (dict_get_string(tv->vval.v_dict, "dup", FALSE) != NULL)
	    dup = dict_get_number(tv->vval.v_dict, "dup");
	if (dict_get_string(tv->vval.v_dict, "empty", FALSE) != NULL)
	    empty = dict_get_number(tv->vval.v_dict, "empty");
	if (dict_get_string(tv->vval.v_dict, "equal", FALSE) != NULL
				  && dict_get_number(tv->vval.v_dict, "equal"))
	    flags |= CP_EQUAL;
    }
    else
    {
	word = tv_get_string_chk(tv);
	CLEAR_FIELD(cptext);
    }
    if (word == NULL || (!empty && *word == NUL))
    {
	clear_tv(&user_data);
	return FAIL;
    }
    status = ins_compl_add(word, -1, NULL, cptext,
				     &user_data, dir, flags, dup, user_hl, 0);
    if (status != OK)
	clear_tv(&user_data);
    return status;
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
    CHECK_LIST_MATERIALIZE(list);
    FOR_ALL_LIST_ITEMS(list, li)
    {
	if (ins_compl_add_tv(&li->li_tv, dir, TRUE) == OK)
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
    unsigned int cur_cot_flags = get_cot_flags();
    int compl_longest = (cur_cot_flags & COT_LONGEST) != 0;
    int compl_no_insert = (cur_cot_flags & COT_NOINSERT) != 0;
    int compl_no_select = (cur_cot_flags & COT_NOSELECT) != 0;

    // If already doing completions stop it.
    if (ctrl_x_mode_not_default())
	ins_compl_prep(' ');
    ins_compl_clear();
    ins_compl_free();
    compl_get_longest = compl_longest;

    compl_direction = FORWARD;
    if (startcol > curwin->w_cursor.col)
	startcol = curwin->w_cursor.col;
    compl_col = startcol;
    compl_lnum = curwin->w_cursor.lnum;
    compl_length = (int)curwin->w_cursor.col - (int)startcol;
    // compl_pattern doesn't need to be set
    compl_orig_text.string = vim_strnsave(ml_get_curline() + compl_col,
							(size_t)compl_length);
    if (p_ic)
	flags |= CP_ICASE;
    if (compl_orig_text.string == NULL)
    {
	compl_orig_text.length = 0;
	return;
    }
    compl_orig_text.length = (size_t)compl_length;
    if (ins_compl_add(compl_orig_text.string,
			(int)compl_orig_text.length, NULL, NULL, NULL, 0,
			flags | CP_FAST, FALSE, NULL, 0) != OK)
	return;

    ctrl_x_mode = CTRL_X_EVAL;

    ins_compl_add_list(list);
    compl_matches = ins_compl_make_cyclic();
    compl_started = TRUE;
    compl_used_match = TRUE;
    compl_cont_status = 0;

    compl_curr_match = compl_first_match;
    int no_select = compl_no_select || compl_longest;
    if (compl_no_insert || no_select)
    {
	ins_complete(K_DOWN, FALSE);
	if (no_select)
	    // Down/Up has no real effect.
	    ins_complete(K_UP, FALSE);
    }
    else
	ins_complete(Ctrl_N, FALSE);
    compl_enter_selects = compl_no_insert;

    // Lazily show the popup menu, unless we got interrupted.
    if (!compl_interrupted)
	show_pum(save_w_wrow, save_w_leftcol);
    may_trigger_modechanged();
    out_flush();
}

/*
 * "complete()" function
 */
    void
f_complete(typval_T *argvars, typval_T *rettv UNUSED)
{
    if (in_vim9script()
	    && (check_for_number_arg(argvars, 0) == FAIL
		|| check_for_list_arg(argvars, 1) == FAIL))
	return;

    if ((State & MODE_INSERT) == 0)
    {
	emsg(_(e_complete_can_only_be_used_in_insert_mode));
	return;
    }

    // Check for undo allowed here, because if something was already inserted
    // the line was already saved for undo and this check isn't done.
    if (!undo_allowed())
	return;

    if (check_for_nonnull_list_arg(argvars, 1) != FAIL)
    {
	int startcol = (int)tv_get_number_chk(&argvars[0], NULL);
	if (startcol > 0)
	    set_completion(startcol - 1, argvars[1].vval.v_list);
    }
}

/*
 * "complete_add()" function
 */
    void
f_complete_add(typval_T *argvars, typval_T *rettv)
{
    if (in_vim9script() && check_for_string_or_dict_arg(argvars, 0) == FAIL)
	return;

    rettv->vval.v_number = ins_compl_add_tv(&argvars[0], 0, FALSE);
}

/*
 * "complete_check()" function
 */
    void
f_complete_check(typval_T *argvars UNUSED, typval_T *rettv)
{
    int save_RedrawingDisabled = RedrawingDisabled;
    RedrawingDisabled = 0;

    ins_compl_check_keys(0, TRUE);
    if (compl_autocomplete && compl_time_slice_expired)
	rettv->vval.v_number = TRUE;
    else
	rettv->vval.v_number = ins_compl_interrupted();

    RedrawingDisabled = save_RedrawingDisabled;
}

/*
 * Add match item to the return list.
 * Returns FAIL if out of memory, OK otherwise.
 */
    static int
add_match_to_list(
    typval_T  *rettv,
    char_u    *str,
    int        len,
    int        pos)
{
    list_T    *match;
    int        ret;

    match = list_alloc();
    if (match == NULL)
	return FAIL;

    if ((ret = list_append_number(match, pos + 1)) == FAIL
	    || (ret = list_append_string(match, str, len)) == FAIL
	    || (ret = list_append_list(rettv->vval.v_list, match)) == FAIL)
    {
	vim_free(match);
	return FAIL;
    }

    return OK;
}

/*
 * "complete_match()" function
 */
    void
f_complete_match(typval_T *argvars, typval_T *rettv)
{
    linenr_T    lnum;
    colnr_T     col;
    char_u      *line = NULL;
    char_u      *ise = NULL;
    regmatch_T  regmatch;
    char_u      *before_cursor = NULL;
    char_u      *cur_end = NULL;
    int          bytepos = 0;
    char_u	part[MAXPATHL];
    int		ret;

    if (rettv_list_alloc(rettv) == FAIL)
	return;

    ise = curbuf->b_p_ise[0] != NUL ? curbuf->b_p_ise : p_ise;

    if (argvars[0].v_type == VAR_UNKNOWN)
    {
	lnum = curwin->w_cursor.lnum;
	col = curwin->w_cursor.col;
    }
    else if (argvars[1].v_type == VAR_UNKNOWN)
    {
	emsg(_(e_invalid_argument));
	return;
    }
    else
    {
	lnum = (linenr_T)tv_get_number(&argvars[0]);
	col = (colnr_T)tv_get_number(&argvars[1]);
	if (lnum < 1 || lnum > curbuf->b_ml.ml_line_count)
	{
	    semsg(_(e_invalid_line_number_nr), lnum);
	    return;
	}
	if (col < 1 || col > ml_get_buf_len(curbuf, lnum))
	{
	    semsg(_(e_invalid_column_number_nr), col + 1);
	    return;
	}
    }

    line = ml_get_buf(curbuf, lnum, FALSE);
    if (line == NULL)
	return;

    before_cursor = vim_strnsave(line, col);
    if (before_cursor == NULL)
	return;

    if (ise == NULL || *ise == NUL)
    {
	regmatch.regprog = vim_regcomp((char_u *)"\\k\\+$", RE_MAGIC);
	if (regmatch.regprog != NULL)
	{
	    if (vim_regexec_nl(&regmatch, before_cursor, (colnr_T)0))
	    {
		char_u	*trig = vim_strnsave(regmatch.startp[0],
			regmatch.endp[0] - regmatch.startp[0]);
		if (trig == NULL)
		{
		    vim_free(before_cursor);
		    vim_regfree(regmatch.regprog);
		    return;
		}

		bytepos = (int)(regmatch.startp[0] - before_cursor);
		ret = add_match_to_list(rettv, trig, -1, bytepos);
		vim_free(trig);
		if (ret == FAIL)
		{
		    vim_free(before_cursor);
		    vim_regfree(regmatch.regprog);
		    return;
		}
	    }
	    vim_regfree(regmatch.regprog);
	}
    }
    else
    {
	char_u	    *p = ise;
	char_u	    *p_space = NULL;

	cur_end = before_cursor + (int)STRLEN(before_cursor);

	while (*p != NUL)
	{
	    int	    len = 0;
	    if (p_space)
	    {
		len = p - p_space - 1;
		memcpy(part, p_space + 1, len);
		p_space = NULL;
	    }
	    else
	    {
		char_u *next_comma = vim_strchr((*p == ',') ? p + 1 : p, ',');
		if (next_comma && *(next_comma + 1) == ' ')
		    p_space = next_comma;

		len = copy_option_part(&p, part, MAXPATHL, ",");
	    }

	    if (len > 0 && len <= col)
	    {
		if (STRNCMP(cur_end - len, part, len) == 0)
		{
		    bytepos = col - len;
		    if (add_match_to_list(rettv, part, len, bytepos) == FAIL)
		    {
			vim_free(before_cursor);
			return;
		    }
		}
	    }
	}
    }

    vim_free(before_cursor);
}

/*
 * Return Insert completion mode name string
 */
    static char_u *
ins_compl_mode(void)
{
    if (ctrl_x_mode_not_defined_yet() || ctrl_x_mode_scroll() || compl_started)
	return (char_u *)ctrl_x_mode_names[ctrl_x_mode & ~CTRL_X_WANT_IDENT];

    return (char_u *)"";
}

/*
 * Assign the sequence number to all the completion matches which don't have
 * one assigned yet.
 */
    static void
ins_compl_update_sequence_numbers(void)
{
    int		number = 0;
    compl_T	*match;

    if (compl_dir_forward())
    {
	// Search backwards for the first valid (!= -1) number.
	// This should normally succeed already at the first loop
	// cycle, so it's fast!
	for (match = compl_curr_match->cp_prev; match != NULL
		&& !is_first_match(match); match = match->cp_prev)
	    if (match->cp_number != -1)
	    {
		number = match->cp_number;
		break;
	    }
	if (match != NULL)
	    // go up and assign all numbers which are not assigned yet
	    for (match = match->cp_next;
		    match != NULL && match->cp_number == -1;
					   match = match->cp_next)
		match->cp_number = ++number;
    }
    else // BACKWARD
    {
	// Search forwards (upwards) for the first valid (!= -1)
	// number.  This should normally succeed already at the
	// first loop cycle, so it's fast!
	for (match = compl_curr_match->cp_next; match != NULL
		&& !is_first_match(match); match = match->cp_next)
	{
	    if (match->cp_number != -1)
	    {
		number = match->cp_number;
		break;
	    }
	}
	if (match != NULL)
	{
	    // go down and assign all numbers which are not assigned yet
	    for (match = match->cp_prev; match
		    && match->cp_number == -1;
					   match = match->cp_prev)
		match->cp_number = ++number;
	}
    }
}

/*
 * Fill the dict of complete_info
 */
    static void
fill_complete_info_dict(dict_T *di, compl_T *match, int add_match)
{
    dict_add_string(di, "word", match->cp_str.string);
    dict_add_string(di, "abbr", match->cp_text[CPT_ABBR]);
    dict_add_string(di, "menu", match->cp_text[CPT_MENU]);
    dict_add_string(di, "kind", match->cp_text[CPT_KIND]);
    dict_add_string(di, "info", match->cp_text[CPT_INFO]);
    if (add_match)
	dict_add_bool(di, "match", match->cp_in_match_array);
    if (match->cp_user_data.v_type == VAR_UNKNOWN)
	// Add an empty string for backwards compatibility
	dict_add_string(di, "user_data", (char_u *)"");
    else
	dict_add_tv(di, "user_data", &match->cp_user_data);
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
#define CI_WHAT_COMPLETED	0x10
#define CI_WHAT_MATCHES		0x20
#define CI_WHAT_ALL		0xff
    int		what_flag;

    if (what_list == NULL)
	what_flag = CI_WHAT_ALL & ~(CI_WHAT_MATCHES | CI_WHAT_COMPLETED);
    else
    {
	what_flag = 0;
	CHECK_LIST_MATERIALIZE(what_list);
	FOR_ALL_LIST_ITEMS(what_list, item)
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
	    else if (STRCMP(what, "completed") == 0)
		what_flag |= CI_WHAT_COMPLETED;
	    else if (STRCMP(what, "matches") == 0)
		what_flag |= CI_WHAT_MATCHES;
	}
    }

    if (ret == OK && (what_flag & CI_WHAT_MODE))
	ret = dict_add_string(retdict, "mode", ins_compl_mode());

    if (ret == OK && (what_flag & CI_WHAT_PUM_VISIBLE))
	ret = dict_add_number(retdict, "pum_visible", pum_visible());

    if (ret == OK && (what_flag & (CI_WHAT_ITEMS | CI_WHAT_SELECTED
				    | CI_WHAT_MATCHES | CI_WHAT_COMPLETED)))
    {
	list_T	    *li = NULL;
	dict_T	    *di;
	compl_T     *match;
	int         selected_idx = -1;
	int	    has_items = what_flag & CI_WHAT_ITEMS;
	int	    has_matches = what_flag & CI_WHAT_MATCHES;
	int	    has_completed = what_flag & CI_WHAT_COMPLETED;

	if (has_items || has_matches)
	{
	    li = list_alloc();
	    if (li == NULL)
		return;
	    ret = dict_add_list(retdict, (has_matches && !has_items)
						? "matches" : "items", li);
	}
	if (ret == OK && what_flag & CI_WHAT_SELECTED)
	    if (compl_curr_match != NULL && compl_curr_match->cp_number == -1)
		ins_compl_update_sequence_numbers();
	if (ret == OK && compl_first_match != NULL)
	{
	    int list_idx = 0;
	    match = compl_first_match;
	    do
	    {
		if (!match_at_original_text(match))
		{
		    if (has_items
			    || (has_matches && match->cp_in_match_array))
		    {
			di = dict_alloc();
			if (di == NULL)
			    return;
			ret = list_append_dict(li, di);
			if (ret != OK)
			    return;
			fill_complete_info_dict(di, match, has_matches && has_items);
		    }
		    if (compl_curr_match != NULL
			    && compl_curr_match->cp_number == match->cp_number)
			selected_idx = list_idx;
		    if (match->cp_in_match_array)
			list_idx += 1;
		}
		match = match->cp_next;
	    }
	    while (match != NULL && !is_first_match(match));
	}
	if (ret == OK && (what_flag & CI_WHAT_SELECTED))
	    ret = dict_add_number(retdict, "selected", selected_idx);

	if (ret == OK && selected_idx != -1 && has_completed)
	{
	    di = dict_alloc();
	    if (di == NULL)
		return;
	    fill_complete_info_dict(di, compl_curr_match, FALSE);
	    ret = dict_add_dict(retdict, "completed", di);
	}
    }
}

/*
 * "complete_info()" function
 */
    void
f_complete_info(typval_T *argvars, typval_T *rettv)
{
    list_T	*what_list = NULL;

    if (rettv_dict_alloc(rettv) == FAIL)
	return;

    if (in_vim9script() && check_for_opt_list_arg(argvars, 0) == FAIL)
	return;

    if (argvars[0].v_type != VAR_UNKNOWN)
    {
	if (check_for_list_arg(argvars, 0) == FAIL)
	    return;
	what_list = argvars[0].vval.v_list;
    }
    get_complete_info(what_list, rettv->vval.v_dict);
}
#endif

/*
 * Returns TRUE when using a user-defined function for thesaurus completion.
 */
    static int
thesaurus_func_complete(int type UNUSED)
{
#ifdef FEAT_COMPL_FUNC
    return type == CTRL_X_THESAURUS
		&& (*curbuf->b_p_tsrfu != NUL || *p_tsrfu != NUL);
#else
    return FALSE;
#endif
}

/*
 * Check if 'cpt' list index can be advanced to the next completion source.
 */
    static int
may_advance_cpt_index(char_u *cpt)
{
    char_u  *p = cpt;

    if (cpt_sources_index == -1)
	return FALSE;
    while (*p == ',' || *p == ' ') // Skip delimiters
	p++;
    return (*p != NUL);
}

/*
 * Return value of process_next_cpt_value()
 */
enum
{
    INS_COMPL_CPT_OK = 1,
    INS_COMPL_CPT_CONT,
    INS_COMPL_CPT_END
};

/*
 * state information used for getting the next set of insert completion
 * matches.
 */
typedef struct
{
    char_u	*e_cpt_copy;		// copy of 'complete'
    char_u	*e_cpt;			// current entry in "e_cpt_copy"
    buf_T	*ins_buf;		// buffer being scanned
    pos_T	*cur_match_pos;		// current match position
    pos_T	prev_match_pos;		// previous match position
    int		set_match_pos;		// save first_match_pos/last_match_pos
    pos_T	first_match_pos;	// first match position
    pos_T	last_match_pos;		// last match position
    int		found_all;		// found all matches of a certain type.
    char_u	*dict;			// dictionary file to search
    int		dict_f;			// "dict" is an exact file name or not
    callback_T	*func_cb;		// callback of function in 'cpt' option
} ins_compl_next_state_T;

/*
 * Process the next 'complete' option value in st->e_cpt.
 *
 * If successful, the arguments are set as below:
 *   st->cpt - pointer to the next option value in "st->cpt"
 *   compl_type_arg - type of insert mode completion to use
 *   st->found_all - all matches of this type are found
 *   st->ins_buf - search for completions in this buffer
 *   st->first_match_pos - position of the first completion match
 *   st->last_match_pos - position of the last completion match
 *   st->set_match_pos - TRUE if the first match position should be saved to
 *			    avoid loops after the search wraps around.
 *   st->dict - name of the dictionary or thesaurus file to search
 *   st->dict_f - flag specifying whether "dict" is an exact file name or not
 *
 * Returns INS_COMPL_CPT_OK if the next value is processed successfully.
 * Returns INS_COMPL_CPT_CONT to skip the current completion source matching
 * the "st->e_cpt" option value and process the next matching source.
 * Returns INS_COMPL_CPT_END if all the values in "st->e_cpt" are processed.
 */
    static int
process_next_cpt_value(
	ins_compl_next_state_T *st,
	int		*compl_type_arg,
	pos_T		*start_match_pos,
	int		fuzzy_collect,
	int		*advance_cpt_idx)
{
    int	    compl_type = -1;
    int	    status = INS_COMPL_CPT_OK;
    int	    skip_source = compl_autocomplete && compl_from_nonkeyword;

    st->found_all = FALSE;
    *advance_cpt_idx = FALSE;

    while (*st->e_cpt == ',' || *st->e_cpt == ' ')
	st->e_cpt++;

    if (*st->e_cpt == '.' && !curbuf->b_scanned && !skip_source
	    && !compl_time_slice_expired)
    {
	st->ins_buf = curbuf;
	st->first_match_pos = *start_match_pos;
	// Move the cursor back one character so that ^N can match the
	// word immediately after the cursor.
	if (ctrl_x_mode_normal() && (!fuzzy_collect && dec(&st->first_match_pos) < 0))
	{
	    // Move the cursor to after the last character in the
	    // buffer, so that word at start of buffer is found
	    // correctly.
	    st->first_match_pos.lnum = st->ins_buf->b_ml.ml_line_count;
	    st->first_match_pos.col = ml_get_len(st->first_match_pos.lnum);
	}
	st->last_match_pos = st->first_match_pos;
	compl_type = 0;

	// Remember the first match so that the loop stops when we
	// wrap and come back there a second time.
	st->set_match_pos = TRUE;
    }
    else if (!skip_source && !compl_time_slice_expired
	    && vim_strchr((char_u *)"buwU", *st->e_cpt) != NULL
	    && (st->ins_buf = ins_compl_next_buf(
					   st->ins_buf, *st->e_cpt)) != curbuf)
    {
	// Scan a buffer, but not the current one.
	if (st->ins_buf->b_ml.ml_mfp != NULL)   // loaded buffer
	{
	    compl_started = TRUE;
	    st->first_match_pos.col = st->last_match_pos.col = 0;
	    st->first_match_pos.lnum = st->ins_buf->b_ml.ml_line_count + 1;
	    st->last_match_pos.lnum = 0;
	    compl_type = 0;
	}
	else	// unloaded buffer, scan like dictionary
	{
	    st->found_all = TRUE;
	    if (st->ins_buf->b_fname == NULL)
	    {
		status = INS_COMPL_CPT_CONT;
		goto done;
	    }
	    compl_type = CTRL_X_DICTIONARY;
	    st->dict = st->ins_buf->b_fname;
	    st->dict_f = DICT_EXACT;
	}
	if (!shortmess(SHM_COMPLETIONSCAN) && !compl_autocomplete)
	{
	    msg_hist_off = TRUE;	// reset in msg_trunc_attr()
	    vim_snprintf((char *)IObuff, IOSIZE, _("Scanning: %s"),
		    st->ins_buf->b_fname == NULL
			? buf_spname(st->ins_buf)
			: st->ins_buf->b_sfname == NULL
			    ? st->ins_buf->b_fname
			    : st->ins_buf->b_sfname);
	    (void)msg_trunc_attr((char *)IObuff, TRUE, HL_ATTR(HLF_R));
	}
    }
    else if (*st->e_cpt == NUL)
	status = INS_COMPL_CPT_END;
    else
    {
	if (ctrl_x_mode_line_or_eval())
	    compl_type = -1;
#ifdef FEAT_COMPL_FUNC
	else if (*st->e_cpt == 'F' || *st->e_cpt == 'o')
	{
	    compl_type = CTRL_X_FUNCTION;
	    st->func_cb = get_callback_if_cpt_func(st->e_cpt);
	    if (!st->func_cb)
		compl_type = -1;
	}
#endif
	else if (!skip_source)
	{
	    if (*st->e_cpt == 'k' || *st->e_cpt == 's')
	    {
		if (*st->e_cpt == 'k')
		    compl_type = CTRL_X_DICTIONARY;
		else
		    compl_type = CTRL_X_THESAURUS;
		if (*++st->e_cpt != ',' && *st->e_cpt != NUL)
		{
		    st->dict = st->e_cpt;
		    st->dict_f = DICT_FIRST;
		}
	    }
#ifdef FEAT_FIND_ID
	    else if (*st->e_cpt == 'i')
		compl_type = CTRL_X_PATH_PATTERNS;
	    else if (*st->e_cpt == 'd')
		compl_type = CTRL_X_PATH_DEFINES;
#endif
	    else if (*st->e_cpt == ']' || *st->e_cpt == 't')
	    {
		compl_type = CTRL_X_TAGS;
		if (!shortmess(SHM_COMPLETIONSCAN) && !compl_autocomplete)
		{
		    msg_hist_off = TRUE;	// reset in msg_trunc_attr()
		    vim_snprintf((char *)IObuff, IOSIZE, _("Scanning tags."));
		    (void)msg_trunc_attr((char *)IObuff, TRUE, HL_ATTR(HLF_R));
		}
	    }
	    else
		compl_type = -1;
	}

	// in any case e_cpt is advanced to the next entry
	(void)copy_option_part(&st->e_cpt, IObuff, IOSIZE, ",");
	*advance_cpt_idx = may_advance_cpt_index(st->e_cpt);

	st->found_all = TRUE;
	if (compl_type == -1)
	    status = INS_COMPL_CPT_CONT;
    }

done:
    *compl_type_arg = compl_type;
    return status;
}

#ifdef FEAT_FIND_ID
/*
 * Get the next set of identifiers or defines matching "compl_pattern" in
 * included files.
 */
    static void
get_next_include_file_completion(int compl_type)
{
    find_pattern_in_path(compl_pattern.string, compl_direction,
	    (int)compl_pattern.length, FALSE, FALSE,
	    (compl_type == CTRL_X_PATH_DEFINES
	     && !(compl_cont_status & CONT_SOL))
	    ? FIND_DEFINE : FIND_ANY, 1L, ACTION_EXPAND,
	    (linenr_T)1, (linenr_T)MAXLNUM, FALSE);
}
#endif

/*
 * Get the next set of words matching "compl_pattern" in dictionary or
 * thesaurus files.
 */
    static void
get_next_dict_tsr_completion(int compl_type, char_u *dict, int dict_f)
{
#ifdef FEAT_COMPL_FUNC
    if (thesaurus_func_complete(compl_type))
	expand_by_function(compl_type, compl_pattern.string, NULL);
    else
#endif
	ins_compl_dictionaries(
		dict != NULL ? dict
		: (compl_type == CTRL_X_THESAURUS
		    ? (*curbuf->b_p_tsr == NUL ? p_tsr : curbuf->b_p_tsr)
		    : (*curbuf->b_p_dict == NUL ? p_dict : curbuf->b_p_dict)),
		compl_pattern.string,
		dict != NULL ? dict_f : 0,
		compl_type == CTRL_X_THESAURUS);
}

/*
 * Get the next set of tag names matching "compl_pattern".
 */
    static void
get_next_tag_completion(void)
{
    int		save_p_ic;
    char_u	**matches;
    int		num_matches;

    // set p_ic according to p_ic, p_scs and pat for find_tags().
    save_p_ic = p_ic;
    p_ic = ignorecase(compl_pattern.string);

    // Find up to TAG_MANY matches.  Avoids that an enormous number
    // of matches is found when compl_pattern is empty
    g_tag_at_cursor = TRUE;
    if (find_tags(compl_pattern.string, &num_matches, &matches,
		TAG_REGEXP | TAG_NAMES | TAG_NOIC | TAG_INS_COMP
		| (ctrl_x_mode_not_default() ? TAG_VERBOSE : 0),
		TAG_MANY, curbuf->b_ffname) == OK && num_matches > 0)
	ins_compl_add_matches(num_matches, matches, p_ic);
    g_tag_at_cursor = FALSE;
    p_ic = save_p_ic;
}

/*
 * Compare function for qsort
 */
    static int
compare_scores(const void *a, const void *b)
{
    int idx_a = *(const int *)a;
    int idx_b = *(const int *)b;
    int score_a = compl_fuzzy_scores[idx_a];
    int score_b = compl_fuzzy_scores[idx_b];
    return score_a == score_b ? (idx_a == idx_b ? 0 : (idx_a < idx_b ? -1 : 1))
			: (score_a > score_b ? -1 : 1);
}

/*
 * insert prefix with redraw
 */
    static void
ins_compl_longest_insert(char_u *prefix)
{
    ins_compl_delete();
    ins_compl_insert_bytes(prefix + get_compl_len(), -1);
    ins_redraw(FALSE);
}

/*
 * Calculate the longest common prefix among the best fuzzy matches
 * stored in compl_best_matches, and insert it as the longest.
 */
    static void
fuzzy_longest_match(void)
{
    char_u	*prefix = NULL;
    int		prefix_len = 0;
    int		i = 0;
    int		j = 0;
    char_u	*match_str = NULL;
    char_u	*prefix_ptr = NULL;
    char_u	*match_ptr = NULL;
    char_u	*leader = NULL;
    size_t	leader_len = 0;
    compl_T	*compl = NULL;
    int		more_candidates = FALSE;
    compl_T	*nn_compl = NULL;

    if (compl_num_bests == 0)
	return;

    nn_compl = compl_first_match->cp_next->cp_next;
    if (nn_compl && nn_compl != compl_first_match)
	more_candidates = TRUE;

    compl = ctrl_x_mode_whole_line() ? compl_first_match
				    : compl_first_match->cp_next;
    if (compl_num_bests == 1)
    {
	// no more candidates insert the match str
	if (!more_candidates)
	{
	    ins_compl_longest_insert(compl->cp_str.string);
	    compl_num_bests = 0;
	}
	compl_num_bests = 0;
	return;
    }

    compl_best_matches = (compl_T **)alloc(compl_num_bests * sizeof(compl_T *));
    if (compl_best_matches == NULL)
	return;
    while (compl != NULL && i < compl_num_bests)
    {
	compl_best_matches[i] = compl;
	compl = compl->cp_next;
	i++;
    }

    prefix = compl_best_matches[0]->cp_str.string;
    prefix_len = (int)compl_best_matches[0]->cp_str.length;

    for (i = 1; i < compl_num_bests; i++)
    {
	match_str = compl_best_matches[i]->cp_str.string;
	prefix_ptr = prefix;
	match_ptr = match_str;
	j = 0;

	while (j < prefix_len && *match_ptr != NUL && *prefix_ptr != NUL)
	{
	    if (STRNCMP(prefix_ptr, match_ptr, mb_ptr2len(prefix_ptr)) != 0)
		break;

	    MB_PTR_ADV(prefix_ptr);
	    MB_PTR_ADV(match_ptr);
	    j++;
	}

	if (j > 0)
	    prefix_len = j;
    }

    leader = ins_compl_leader();
    leader_len = ins_compl_leader_len();

    // skip non-consecutive prefixes
    if (leader_len > 0 && STRNCMP(prefix, leader, leader_len) != 0)
	goto end;

    prefix = vim_strnsave(prefix, prefix_len);
    if (prefix != NULL)
    {
	ins_compl_longest_insert(prefix);
	compl_cfc_longest_ins = TRUE;
	vim_free(prefix);
    }

end:
    vim_free(compl_best_matches);
    compl_best_matches = NULL;
    compl_num_bests = 0;
}

/*
 * Get the next set of filename matching "compl_pattern".
 */
    static void
get_next_filename_completion(void)
{
    char_u	**matches;
    int		num_matches;
    char_u	*ptr;
    garray_T	fuzzy_indices;
    int		i;
    int		score;
    char_u	*leader = ins_compl_leader();
    size_t	leader_len = ins_compl_leader_len();;
    int		in_fuzzy_collect = (cfc_has_mode() && leader_len > 0);
    int		*fuzzy_indices_data;
    char_u	*last_sep = NULL;
    int		need_collect_bests = in_fuzzy_collect && compl_get_longest;
    int		max_score = 0;
    int		current_score = 0;
    int		dir = compl_direction;

#ifdef BACKSLASH_IN_FILENAME
    char pathsep = (curbuf->b_p_csl[0] == 's') ?
	'/' : (curbuf->b_p_csl[0] == 'b') ? '\\' : PATHSEP;
#else
    char pathsep = PATHSEP;
#endif

    if (in_fuzzy_collect)
    {
#ifdef BACKSLASH_IN_FILENAME
	if (curbuf->b_p_csl[0] == 's')
	{
	    for (i = 0; i < (int)leader_len; i++)
	    {
		if (leader[i] == '\\')
		    leader[i] = '/';
	    }
	}
	else if (curbuf->b_p_csl[0] == 'b')
	{
	    for (i = 0; i < (int)leader_len; i++)
	    {
		if (leader[i] == '/')
		    leader[i] = '\\';
	    }
	}
#endif
	last_sep = vim_strrchr(leader, pathsep);
	if (last_sep == NULL)
	{
	    // No path separator or separator is the last character,
	    // fuzzy match the whole leader
	    VIM_CLEAR_STRING(compl_pattern);
	    compl_pattern.string = vim_strnsave((char_u *)"*", 1);
	    if (compl_pattern.string == NULL)
		return;
	    compl_pattern.length = 1;
	}
	else if (*(last_sep + 1) == '\0')
	    in_fuzzy_collect = FALSE;
	else
	{
	    // Split leader into path and file parts
	    int path_len = last_sep - leader + 1;
	    char_u  *path_with_wildcard;

	    path_with_wildcard = alloc(path_len + 2);
	    if (path_with_wildcard != NULL)
	    {
		vim_snprintf((char *)path_with_wildcard, path_len + 2, "%*.*s*", path_len, path_len, leader);
		VIM_CLEAR_STRING(compl_pattern);
		compl_pattern.string = path_with_wildcard;
		compl_pattern.length = path_len + 1;

		// Move leader to the file part
		leader = last_sep + 1;
		leader_len -= path_len;
	    }
	}
    }

    if (expand_wildcards(1, &compl_pattern.string, &num_matches, &matches,
		EW_FILE|EW_DIR|EW_ADDSLASH|EW_SILENT) != OK)
	return;

    // May change home directory back to "~".
    tilde_replace(compl_pattern.string, num_matches, matches);
#ifdef BACKSLASH_IN_FILENAME
    if (curbuf->b_p_csl[0] != NUL)
    {
	for (i = 0; i < num_matches; ++i)
	{
	    ptr = matches[i];
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

    if (in_fuzzy_collect)
    {
	ga_init2(&fuzzy_indices, sizeof(int), 10);
	compl_fuzzy_scores = (int *)alloc(sizeof(int) * num_matches);

	for (i = 0; i < num_matches; i++)
	{
	    ptr = matches[i];
	    score = fuzzy_match_str(ptr, leader);
	    if (score > 0)
	    {
		if (ga_grow(&fuzzy_indices, 1) == OK)
		{
		    ((int *)fuzzy_indices.ga_data)[fuzzy_indices.ga_len] = i;
		    compl_fuzzy_scores[i] = score;
		    fuzzy_indices.ga_len++;
		}
	    }
	}

	// prevent qsort from deref NULL pointer
	if (fuzzy_indices.ga_len > 0)
	{
	    char_u	*match = NULL;
	    fuzzy_indices_data = (int *)fuzzy_indices.ga_data;
	    qsort(fuzzy_indices_data, fuzzy_indices.ga_len, sizeof(int), compare_scores);

	    for (i = 0; i < fuzzy_indices.ga_len; ++i)
	    {
		match = matches[fuzzy_indices_data[i]];
		current_score = compl_fuzzy_scores[fuzzy_indices_data[i]];
		if (ins_compl_add(match, -1, NULL, NULL, NULL, dir,
			CP_FAST | ((p_fic || p_wic) ? CP_ICASE : 0),
			FALSE, NULL, current_score) == OK)
		    dir = FORWARD;

		if (need_collect_bests)
		{
		    if (i == 0 || current_score == max_score)
		    {
			compl_num_bests++;
			max_score = current_score;
		    }
		}
	    }

	    FreeWild(num_matches, matches);
	}
	else if (leader_len > 0)
	{
	    FreeWild(num_matches, matches);
	    num_matches = 0;
	}

	vim_free(compl_fuzzy_scores);
	ga_clear(&fuzzy_indices);

	if (compl_num_bests > 0 && compl_get_longest)
	    fuzzy_longest_match();
	return;
    }

    if (num_matches > 0)
	ins_compl_add_matches(num_matches, matches, p_fic || p_wic);
}

/*
 * Get the next set of command-line completions matching "compl_pattern".
 */
    static void
get_next_cmdline_completion(void)
{
    char_u	**matches;
    int		num_matches;

    if (expand_cmdline(&compl_xp, compl_pattern.string,
		(int)compl_pattern.length, &num_matches, &matches) == EXPAND_OK)
	ins_compl_add_matches(num_matches, matches, FALSE);
}

/*
 * Get the next set of spell suggestions matching "compl_pattern".
 */
    static void
get_next_spell_completion(linenr_T lnum UNUSED)
{
#ifdef FEAT_SPELL
    char_u	**matches;
    int		num_matches;

    num_matches = expand_spelling(lnum, compl_pattern.string, &matches);
    if (num_matches > 0)
	ins_compl_add_matches(num_matches, matches, p_ic);
    else
	vim_free(matches);
#endif
}

/*
 * Return the next word or line from buffer "ins_buf" at position
 * "cur_match_pos" for completion.  The length of the match is set in "len".
 */
    static char_u *
ins_compl_get_next_word_or_line(
	buf_T	*ins_buf,		// buffer being scanned
	pos_T	*cur_match_pos,		// current match position
	int	*match_len,
	int	*cont_s_ipos)		// next ^X<> will set initial_pos
{
    char_u	*ptr;
    int		len;

    *match_len = 0;
    ptr = ml_get_buf(ins_buf, cur_match_pos->lnum, FALSE) +
	cur_match_pos->col;
    len = (int)ml_get_buf_len(ins_buf, cur_match_pos->lnum) - cur_match_pos->col;
    if (ctrl_x_mode_line_or_eval())
    {
	if (compl_status_adding())
	{
	    if (cur_match_pos->lnum >= ins_buf->b_ml.ml_line_count)
		return NULL;
	    ptr = ml_get_buf(ins_buf, cur_match_pos->lnum + 1, FALSE);
	    len = ml_get_buf_len(ins_buf, cur_match_pos->lnum + 1);
	    if (!p_paste)
	    {
		char_u	*tmp_ptr = ptr;

		ptr = skipwhite(tmp_ptr);
		len -= (int)(ptr - tmp_ptr);
	    }
	}
    }
    else
    {
	char_u	*tmp_ptr = ptr;

	if (compl_status_adding() && compl_length <= len)
	{
	    tmp_ptr += compl_length;
	    // Skip if already inside a word.
	    if (vim_iswordp(tmp_ptr))
		return NULL;
	    // Find start of next word.
	    tmp_ptr = find_word_start(tmp_ptr);
	}
	// Find end of this word.
	tmp_ptr = find_word_end(tmp_ptr);
	len = (int)(tmp_ptr - ptr);

	if (compl_status_adding() && len == compl_length)
	{
	    if (cur_match_pos->lnum < ins_buf->b_ml.ml_line_count)
	    {
		// Try next line, if any. the new word will be
		// "join" as if the normal command "J" was used.
		// IOSIZE is always greater than
		// compl_length, so the next STRNCPY always
		// works -- Acevedo
		STRNCPY(IObuff, ptr, len);
		ptr = ml_get_buf(ins_buf, cur_match_pos->lnum + 1, FALSE);
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
		    *cont_s_ipos = TRUE;
		}
		IObuff[len] = NUL;
		ptr = IObuff;
	    }
	    if (len == compl_length)
		return NULL;
	}
    }

    *match_len = len;
    return ptr;
}

/*
 * Get the next set of words matching "compl_pattern" for default completion(s)
 * (normal ^P/^N and ^X^L).
 * Search for "compl_pattern" in the buffer "st->ins_buf" starting from the
 * position "st->start_pos" in the "compl_direction" direction. If
 * "st->set_match_pos" is TRUE, then set the "st->first_match_pos" and
 * "st->last_match_pos".
 * Returns OK if a new next match is found, otherwise returns FAIL.
 */
    static int
get_next_default_completion(ins_compl_next_state_T *st, pos_T *start_pos)
{
    int		found_new_match = FAIL;
    int		save_p_scs;
    int		save_p_ws;
    int		looped_around = FALSE;
    char_u	*ptr = NULL;
    int		len = 0;
    int		in_fuzzy_collect = (cfc_has_mode() && compl_length > 0)
	|| ((get_cot_flags() & COT_FUZZY) && compl_autocomplete);
    char_u	*leader = ins_compl_leader();
    int		score = 0;
    int		in_curbuf = st->ins_buf == curbuf;

    // If 'infercase' is set, don't use 'smartcase' here
    save_p_scs = p_scs;
    if (st->ins_buf->b_p_inf)
	p_scs = FALSE;

    //	Buffers other than curbuf are scanned from the beginning or the
    //	end but never from the middle, thus setting nowrapscan in this
    //	buffer is a good idea, on the other hand, we always set
    //	wrapscan for curbuf to avoid missing matches -- Acevedo,Webb
    save_p_ws = p_ws;
    if (!in_curbuf)
	p_ws = FALSE;
    else if (*st->e_cpt == '.')
	p_ws = TRUE;
    looped_around = FALSE;
    for (;;)
    {
	int	cont_s_ipos = FALSE;

	++msg_silent;  // Don't want messages for wrapscan.

	if (in_fuzzy_collect)
	{
	    found_new_match = search_for_fuzzy_match(st->ins_buf,
			    st->cur_match_pos, leader, compl_direction,
			    start_pos, &len, &ptr, &score);
	}
	// ctrl_x_mode_line_or_eval() || word-wise search that
	// has added a word that was at the beginning of the line
	else if (ctrl_x_mode_whole_line() || ctrl_x_mode_eval() || (compl_cont_status & CONT_SOL))
	    found_new_match = search_for_exact_line(st->ins_buf,
			    st->cur_match_pos, compl_direction, compl_pattern.string);
	else
	    found_new_match = searchit(NULL, st->ins_buf, st->cur_match_pos,
				NULL, compl_direction, compl_pattern.string, (int)compl_pattern.length,
				1L, SEARCH_KEEP + SEARCH_NFMSG, RE_LAST, NULL);
	--msg_silent;
	if (!compl_started || st->set_match_pos)
	{
	    // set "compl_started" even on fail
	    compl_started = TRUE;
	    st->first_match_pos = *st->cur_match_pos;
	    st->last_match_pos = *st->cur_match_pos;
	    st->set_match_pos = FALSE;
	}
	else if (st->first_match_pos.lnum == st->last_match_pos.lnum
		&& st->first_match_pos.col == st->last_match_pos.col)
	{
	    found_new_match = FAIL;
	}
	else if (compl_dir_forward()
		&& (st->prev_match_pos.lnum > st->cur_match_pos->lnum
		    || (st->prev_match_pos.lnum == st->cur_match_pos->lnum
			&& st->prev_match_pos.col >= st->cur_match_pos->col)))
	{
	    if (looped_around)
		found_new_match = FAIL;
	    else
		looped_around = TRUE;
	}
	else if (!compl_dir_forward()
		&& (st->prev_match_pos.lnum < st->cur_match_pos->lnum
		    || (st->prev_match_pos.lnum == st->cur_match_pos->lnum
			&& st->prev_match_pos.col <= st->cur_match_pos->col)))
	{
	    if (looped_around)
		found_new_match = FAIL;
	    else
		looped_around = TRUE;
	}
	st->prev_match_pos = *st->cur_match_pos;
	if (found_new_match == FAIL)
	    break;

	// when ADDING, the text before the cursor matches, skip it
	if (compl_status_adding() && in_curbuf
		&& start_pos->lnum == st->cur_match_pos->lnum
		&& start_pos->col  == st->cur_match_pos->col)
	    continue;

	if (!in_fuzzy_collect)
	    ptr = ins_compl_get_next_word_or_line(st->ins_buf, st->cur_match_pos,
							       &len, &cont_s_ipos);
	if (ptr == NULL || (ins_compl_has_preinsert() && STRCMP(ptr, compl_pattern.string) == 0))
	    continue;

	if (is_nearest_active() && in_curbuf)
	{
	    score = st->cur_match_pos->lnum - curwin->w_cursor.lnum;
	    if (score < 0)
		score = -score;
	    score++;
	}

	if (ins_compl_add_infercase(ptr, len, p_ic,
			in_curbuf ? NULL : st->ins_buf->b_sfname,
			0, cont_s_ipos, score) != NOTDONE)
	{
	    if (in_fuzzy_collect && score == compl_first_match->cp_next->cp_score)
		compl_num_bests++;
	    found_new_match = OK;
	    break;
	}
    }
    p_scs = save_p_scs;
    p_ws = save_p_ws;

    return found_new_match;
}

#ifdef FEAT_COMPL_FUNC
/*
 * Return the callback function associated with "p" if it points to a
 * userfunc.
 */
    static callback_T *
get_callback_if_cpt_func(char_u *p)
{
    static callback_T	cb;
    char_u		buf[LSIZE];
    int			slen;

    if (*p == 'o')
	return &curbuf->b_ofu_cb;
    if (*p == 'F')
    {
	if (*++p != ',' && *p != NUL)
	{
	    free_callback(&cb);
	    slen = copy_option_part(&p, buf, LSIZE, ",");
	    if (slen > 0  && option_set_callback_func(buf, &cb))
		return &cb;
	    return NULL;
	}
	else
	    return &curbuf->b_cfu_cb;
    }
    return NULL;
}
#endif

/*
 * Get completion matches from register contents.
 * Extracts words from all available registers and adds them to the completion list.
 */
    static void
get_register_completion(void)
{
    int		dir = compl_direction;
    yankreg_T	*reg = NULL;
    void	*reg_ptr = NULL;
    int		adding_mode = compl_status_adding();

    for (int i = 0; i < NUM_REGISTERS; i++)
    {
	int regname = 0;
	if (i == 0)
	    regname = '"';    // unnamed register
	else if (i < 10)
	    regname = '0' + i;
	else if (i == DELETION_REGISTER)
	    regname = '-';
#ifdef FEAT_CLIPBOARD
	else if (i == STAR_REGISTER)
	    regname = '*';
	else if (i == PLUS_REGISTER)
	    regname = '+';
#endif
	else
	    regname = 'a' + i - 10;

	// Skip invalid or black hole register
	if (!valid_yank_reg(regname, FALSE) || regname == '_')
	    continue;

	reg_ptr = get_register(regname, FALSE);
	if (reg_ptr == NULL)
	    continue;

	reg = (yankreg_T *)reg_ptr;

	for (int j = 0; j < reg->y_size; j++)
	{
	    char_u *str = reg->y_array[j].string;
	    if (str == NULL)
		continue;

	    if (adding_mode)
	    {
		int str_len = (int)STRLEN(str);
		if (str_len == 0)
		    continue;

		if (!compl_orig_text.string
			|| (p_ic ? STRNICMP(str, compl_orig_text.string,
					    compl_orig_text.length) == 0
				: STRNCMP(str, compl_orig_text.string,
					    compl_orig_text.length) == 0))
		{
		    if (ins_compl_add_infercase(str, str_len, p_ic, NULL,
							dir, FALSE, 0) == OK)
			dir = FORWARD;
		}
	    }
	    else
	    {
		// Calculate the safe end of string to avoid null byte issues
		char_u *str_end = str + STRLEN(str);
		char_u *p = str;

		// Safely iterate through the string
		while (p < str_end && *p != NUL)
		{
		    char_u *old_p = p;
		    p = find_word_start(p);
		    if (p >= str_end || *p == NUL)
			break;

		    char_u *word_end = find_word_end(p);

		    if (word_end <= p)
		    {
			if (has_mbyte)
			    word_end = p + (*mb_ptr2len)(p);
			else
			    word_end = p + 1;
		    }

		    if (word_end > str_end)
			word_end = str_end;

		    int len = (int)(word_end - p);
		    if (len > 0 && (!compl_orig_text.string
				|| (p_ic ? STRNICMP(p, compl_orig_text.string,
						    compl_orig_text.length) == 0
					: STRNCMP(p, compl_orig_text.string,
						    compl_orig_text.length) == 0)))
		    {
			if (ins_compl_add_infercase(p, len, p_ic, NULL,
						    dir, FALSE, 0) == OK)
			    dir = FORWARD;
		    }

		    p = word_end;

		    if (p <= old_p)
		    {
			p = old_p + 1;
			if (has_mbyte && p < str_end)
			    p = old_p + (*mb_ptr2len)(old_p);
		    }
		}
	    }
	}

	// Free the register copy
	put_register(regname, reg_ptr);
    }
}

/*
 * get the next set of completion matches for "type".
 * Returns TRUE if a new match is found. Otherwise returns FALSE.
 */
    static int
get_next_completion_match(int type, ins_compl_next_state_T *st, pos_T *ini)
{
    int	found_new_match = FALSE;

    switch (type)
    {
	case -1:
	    break;
#ifdef FEAT_FIND_ID
	case CTRL_X_PATH_PATTERNS:
	case CTRL_X_PATH_DEFINES:
	    get_next_include_file_completion(type);
	    break;
#endif

	case CTRL_X_DICTIONARY:
	case CTRL_X_THESAURUS:
	    get_next_dict_tsr_completion(type, st->dict, st->dict_f);
	    st->dict = NULL;
	    break;

	case CTRL_X_TAGS:
	    get_next_tag_completion();
	    break;

	case CTRL_X_FILES:
	    get_next_filename_completion();
	    break;

	case CTRL_X_CMDLINE:
	case CTRL_X_CMDLINE_CTRL_X:
	    get_next_cmdline_completion();
	    break;

#ifdef FEAT_COMPL_FUNC
	case CTRL_X_FUNCTION:
	    if (ctrl_x_mode_normal())  // Invoked by a func in 'cpt' option
		get_cpt_func_completion_matches(st->func_cb);
	    else
		expand_by_function(type, compl_pattern.string, NULL);
	    break;
	case CTRL_X_OMNI:
	    expand_by_function(type, compl_pattern.string, NULL);
	    break;
#endif

	case CTRL_X_SPELL:
	    get_next_spell_completion(st->first_match_pos.lnum);
	    break;

	case CTRL_X_REGISTER:
	    get_register_completion();
	    break;

	default:	// normal ^P/^N and ^X^L
	    found_new_match = get_next_default_completion(st, ini);
	    if (found_new_match == FAIL && st->ins_buf == curbuf)
		st->found_all = TRUE;
    }

    // check if compl_curr_match has changed, (e.g. other type of
    // expansion added something)
    if (type != 0 && compl_curr_match != compl_old_match)
	found_new_match = OK;

    return found_new_match;
}

/*
 * Strips carets followed by numbers. This suffix typically represents the
 * max_matches setting.
 */
    static void
strip_caret_numbers_in_place(char_u *str)
{
    char_u  *read = str, *write = str, *p;

    if (str == NULL)
	return;

    while (*read)
    {
	if (*read == '^')
	{
	    p = read + 1;
	    while (vim_isdigit(*p))
		p++;
	    if ((*p == ',' || *p == '\0') && p != read + 1)
	    {
		read = p;
		continue;
	    }
	    else
		*write++ = *read++;
	}
	else
	    *write++ = *read++;
    }
    *write = '\0';
}

/*
 * Call functions specified in the 'cpt' option with findstart=1,
 * and retrieve the startcol.
 */
    static int
prepare_cpt_compl_funcs(void)
{
#ifdef FEAT_COMPL_FUNC
    char_u	*cpt;
    char_u	*p;
    callback_T	*cb = NULL;
    int		idx = 0;
    int		startcol;

    // Make a copy of 'cpt' in case the buffer gets wiped out
    cpt = vim_strsave(curbuf->b_p_cpt);
    if (cpt == NULL)
	return FAIL;
    strip_caret_numbers_in_place(cpt);

    for (p = cpt; *p;)
    {
	while (*p == ',' || *p == ' ') // Skip delimiters
	    p++;
	if (*p == NUL)
	    break;

	cb = get_callback_if_cpt_func(p);
	if (cb)
	{
	    if (get_userdefined_compl_info(curwin->w_cursor.col, cb, &startcol)
		    == FAIL)
	    {
		if (startcol == -3)
		    cpt_sources_array[idx].cs_refresh_always = FALSE;
		else
		    startcol = -2;
	    }
	    cpt_sources_array[idx].cs_startcol = startcol;
	}
	else
	    cpt_sources_array[idx].cs_startcol = STARTCOL_NONE;

	(void)copy_option_part(&p, IObuff, IOSIZE, ","); // Advance p
	idx++;
    }

    vim_free(cpt);
    return OK;
#endif
    return FAIL;
}

/*
 * Start the timer for the current completion source.
 */
    static void
compl_source_start_timer(int source_idx UNUSED)
{
#ifdef ELAPSED_FUNC
    if (compl_autocomplete && cpt_sources_array != NULL)
    {
	ELAPSED_INIT(cpt_sources_array[source_idx].compl_start_tv);
	compl_time_slice_expired = FALSE;
    }
#endif
}

/*
 * Safely advance the cpt_sources_index by one.
 */
    static int
advance_cpt_sources_index_safe(void)
{
    if (cpt_sources_index >= 0 && cpt_sources_index < cpt_sources_count - 1)
    {
	cpt_sources_index++;
	return OK;
    }
#ifdef FEAT_EVAL
    semsg(_(e_list_index_out_of_range_nr), cpt_sources_index);
#endif
    return FAIL;
}

#define COMPL_FUNC_TIMEOUT_MS		300
#define COMPL_FUNC_TIMEOUT_NON_KW_MS	1000
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
    static ins_compl_next_state_T   st;
    static int			    st_cleared = FALSE;
    int		i;
    int		found_new_match;
    int		type = ctrl_x_mode;
    int		may_advance_cpt_idx = FALSE;
    pos_T	start_pos = *ini;

    if (!compl_started)
    {
	buf_T *buf;

	FOR_ALL_BUFFERS(buf)
	    buf->b_scanned = 0;
	if (!st_cleared)
	{
	    CLEAR_FIELD(st);
	    st_cleared = TRUE;
	}
	st.found_all = FALSE;
	st.ins_buf = curbuf;
	vim_free(st.e_cpt_copy);
	// Make a copy of 'complete', in case the buffer is wiped out.
	st.e_cpt_copy = vim_strsave((compl_cont_status & CONT_LOCAL)
					    ? (char_u *)"." : curbuf->b_p_cpt);
	strip_caret_numbers_in_place(st.e_cpt_copy);
	st.e_cpt = st.e_cpt_copy == NULL ? (char_u *)"" : st.e_cpt_copy;

	// In large buffers, timeout may miss nearby matches — search above cursor
#define LOOKBACK_LINE_COUNT	1000
	if (compl_autocomplete && is_nearest_active())
	{
	    start_pos.lnum = MAX(1, start_pos.lnum - LOOKBACK_LINE_COUNT);
	    start_pos.col = 0;
	}
	st.last_match_pos = st.first_match_pos = start_pos;
    }
    else if (st.ins_buf != curbuf && !buf_valid(st.ins_buf))
	st.ins_buf = curbuf;  // In case the buffer was wiped out.

    compl_old_match = compl_curr_match;	// remember the last current match
    st.cur_match_pos = (compl_dir_forward())
				    ? &st.last_match_pos : &st.first_match_pos;

    if (cpt_sources_array != NULL && ctrl_x_mode_normal()
	    && !ctrl_x_mode_line_or_eval()
	    && !(compl_cont_status & CONT_LOCAL))
    {
	cpt_sources_index = 0;
	if (compl_autocomplete)
	{
	    compl_source_start_timer(0);
	    compl_timeout_ms = COMPL_INITIAL_TIMEOUT_MS;
	}
    }

    // For ^N/^P loop over all the flags/windows/buffers in 'complete'.
    for (;;)
    {
	found_new_match = FAIL;
	st.set_match_pos = FALSE;

	// For ^N/^P pick a new entry from e_cpt if compl_started is off,
	// or if found_all says this entry is done.  For ^X^L only use the
	// entries from 'complete' that look in loaded buffers.
	if ((ctrl_x_mode_normal() || ctrl_x_mode_line_or_eval())
					&& (!compl_started || st.found_all))
	{
	    int status = process_next_cpt_value(&st, &type, &start_pos,
		    cfc_has_mode(), &may_advance_cpt_idx);

	    if (status == INS_COMPL_CPT_END)
		break;
	    if (status == INS_COMPL_CPT_CONT)
	    {
		if (may_advance_cpt_idx)
		{
		    if (!advance_cpt_sources_index_safe())
			break;
		    compl_source_start_timer(cpt_sources_index);
		}
		continue;
	    }
	}

	// If complete() was called then compl_pattern has been reset.  The
	// following won't work then, bail out.
	if (compl_pattern.string == NULL)
	    break;

	if (compl_autocomplete && type == CTRL_X_FUNCTION)
	    // LSP servers may sporadically take >1s to respond (e.g., while
	    // loading modules), but other sources might already have matches.
	    // To show results quickly use a short timeout for keyword
	    // completion. Allow longer timeout for non-keyword completion
	    // where only function based sources (e.g. LSP) are active.
	    compl_timeout_ms = compl_from_nonkeyword
		? COMPL_FUNC_TIMEOUT_NON_KW_MS : COMPL_FUNC_TIMEOUT_MS;

	// get the next set of completion matches
	found_new_match = get_next_completion_match(type, &st, &start_pos);

	if (may_advance_cpt_idx)
	{
	    if (!advance_cpt_sources_index_safe())
		break;
	    compl_source_start_timer(cpt_sources_index);
	}

	// break the loop for specialized modes (use 'complete' just for the
	// generic ctrl_x_mode == CTRL_X_NORMAL) or when we've found a new
	// match
	if ((ctrl_x_mode_not_default() && !ctrl_x_mode_line_or_eval())
						|| found_new_match != FAIL)
	{
	    if (got_int)
		break;
	    // Fill the popup menu as soon as possible.
	    if (type != -1)
		ins_compl_check_keys(0, FALSE);

	    if ((ctrl_x_mode_not_default()
			&& !ctrl_x_mode_line_or_eval()) || compl_interrupted)
		break;
	    compl_started = compl_time_slice_expired ? FALSE : TRUE;
	}
	else
	{
	    // Mark a buffer scanned when it has been scanned completely
	    if (buf_valid(st.ins_buf) && (type == 0 || type == CTRL_X_PATH_PATTERNS))
		st.ins_buf->b_scanned = TRUE;

	    compl_started = FALSE;
	}

	// Reset the timeout after collecting matches from function source
	if (compl_autocomplete && type == CTRL_X_FUNCTION)
	    compl_timeout_ms = COMPL_INITIAL_TIMEOUT_MS;

	// For `^P` completion, reset `compl_curr_match` to the head to avoid
	// mixing matches from different sources.
	if (!compl_dir_forward())
	    while (compl_curr_match->cp_prev)
		compl_curr_match = compl_curr_match->cp_prev;
    }
    cpt_sources_index = -1;
    compl_started = TRUE;

    if ((ctrl_x_mode_normal() || ctrl_x_mode_line_or_eval())
	    && *st.e_cpt == NUL)		// Got to end of 'complete'
	found_new_match = FAIL;

    i = -1;		// total of matches, unknown
    if (found_new_match == FAIL || (ctrl_x_mode_not_default()
		&& !ctrl_x_mode_line_or_eval()))
	i = ins_compl_make_cyclic();

    if (cfc_has_mode() && compl_get_longest && compl_num_bests > 0)
	fuzzy_longest_match();

    if (compl_old_match != NULL)
    {
	// If several matches were added (FORWARD) or the search failed and has
	// just been made cyclic then we have to move compl_curr_match to the
	// next or previous entry (if any) -- Acevedo
	compl_curr_match = compl_dir_forward() ? compl_old_match->cp_next
						    : compl_old_match->cp_prev;
	if (compl_curr_match == NULL)
	    compl_curr_match = compl_old_match;
    }
    may_trigger_modechanged();

    if (is_nearest_active())
	sort_compl_match_list(cp_compare_nearest);

    return i;
}

/*
 * Update "compl_shown_match" to the actually shown match, it may differ when
 * "compl_leader" is used to omit some of the matches.
 */
    static void
ins_compl_update_shown_match(void)
{
    string_T	*leader;

    (void)get_leader_for_startcol(NULL, TRUE); // Clear the cache
    leader = get_leader_for_startcol(compl_shown_match, TRUE);

    while (!ins_compl_equal(compl_shown_match,
		leader->string, (int)leader->length)
	    && compl_shown_match->cp_next != NULL
	    && !is_first_match(compl_shown_match->cp_next))
    {
	compl_shown_match = compl_shown_match->cp_next;
	leader = get_leader_for_startcol(compl_shown_match, TRUE);
    }

    // If we didn't find it searching forward, and compl_shows_dir is
    // backward, find the last match.
    if (compl_shows_dir_backward()
	    && !ins_compl_equal(compl_shown_match,
		leader->string, (int)leader->length)
	    && (compl_shown_match->cp_next == NULL
		|| is_first_match(compl_shown_match->cp_next)))
    {
	while (!ins_compl_equal(compl_shown_match,
		    leader->string, (int)leader->length)
		&& compl_shown_match->cp_prev != NULL
		&& !is_first_match(compl_shown_match->cp_prev))
	{
	    compl_shown_match = compl_shown_match->cp_prev;
	    leader = get_leader_for_startcol(compl_shown_match, TRUE);
	}
    }
}

/*
 * Delete the old text being completed.
 */
    void
ins_compl_delete(void)
{
    // In insert mode: Delete the typed part.
    // In replace mode: Put the old characters back, if any.
    int col = compl_col + (compl_status_adding() ? compl_length : 0);
    string_T	remaining = {NULL, 0};
    int	    orig_col;
    int	has_preinsert = ins_compl_preinsert_effect();
    if (has_preinsert)
    {
	col += (int)ins_compl_leader_len();
	curwin->w_cursor.col = compl_ins_end_col;
    }

    if (curwin->w_cursor.lnum > compl_lnum)
    {
	if (curwin->w_cursor.col < ml_get_curline_len())
	{
	    char_u *line = ml_get_cursor();
	    remaining.length = ml_get_cursor_len();
	    remaining.string = vim_strnsave(line, remaining.length);
	    if (remaining.string == NULL)
		return;
	}
	while (curwin->w_cursor.lnum > compl_lnum)
	{
	    if (ml_delete(curwin->w_cursor.lnum) == FAIL)
	    {
		if (remaining.string)
		    vim_free(remaining.string);
		return;
	    }
	    deleted_lines_mark(curwin->w_cursor.lnum, 1L);
	    curwin->w_cursor.lnum--;
	}
	// move cursor to end of line
	curwin->w_cursor.col = ml_get_curline_len();
    }

    if ((int)curwin->w_cursor.col > col)
    {
	if (stop_arrow() == FAIL)
	{
	    if (remaining.string)
		vim_free(remaining.string);
	    return;
	}
	backspace_until_column(col);
	compl_ins_end_col = curwin->w_cursor.col;
    }

    if (remaining.string != NULL)
    {
	orig_col = curwin->w_cursor.col;
	ins_str(remaining.string, remaining.length);
	curwin->w_cursor.col = orig_col;
	vim_free(remaining.string);
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
 * Insert a completion string that contains newlines.
 * The string is split and inserted line by line.
 */
    static void
ins_compl_expand_multiple(char_u *str)
{
    char_u	*start = str;
    char_u	*curr = str;
    int		base_indent = get_indent();

    while (*curr != NUL)
    {
	if (*curr == '\n')
	{
	    // Insert the text chunk before newline
	    if (curr > start)
		ins_char_bytes(start, (int)(curr - start));

	    // Handle newline
	    open_line(FORWARD, OPENLINE_KEEPTRAIL | OPENLINE_FORCE_INDENT, base_indent, NULL);
	    start = curr + 1;
	}
	curr++;
    }

    // Handle remaining text after last newline (if any)
    if (curr > start)
	ins_char_bytes(start, (int)(curr - start));

    compl_ins_end_col = curwin->w_cursor.col;
}

/*
 * Insert the new text being completed.
 * "move_cursor" is used when 'completeopt' includes "preinsert" and when TRUE
 * cursor needs to move back from the inserted text to the compl_leader.
 */
    void
ins_compl_insert(int move_cursor)
{
    int		compl_len = get_compl_len();
    int		preinsert = ins_compl_has_preinsert();
    char_u	*cp_str = compl_shown_match->cp_str.string;
    size_t	cp_str_len = compl_shown_match->cp_str.length;
    size_t	leader_len = ins_compl_leader_len();
    char_u	*has_multiple = vim_strchr(cp_str, '\n');

    // Since completion sources may provide matches with varying start
    // positions, insert only the portion of the match that corresponds to the
    // intended replacement range.
    if (cpt_sources_array != NULL)
    {
	int	cpt_idx = compl_shown_match->cp_cpt_source_idx;
	if (cpt_idx >= 0 && compl_col >= 0)
	{
	    int	startcol = cpt_sources_array[cpt_idx].cs_startcol;
	    if (startcol >= 0 && startcol < (int)compl_col)
	    {
		int skip = (int)compl_col - startcol;
		if ((size_t)skip <= cp_str_len)
		{
		    cp_str_len -= skip;
		    cp_str += skip;
		}
	    }
	}
    }

    // Make sure we don't go over the end of the string, this can happen with
    // illegal bytes.
    if (compl_len < (int)cp_str_len)
    {
	if (has_multiple)
	    ins_compl_expand_multiple(cp_str + compl_len);
	else
	{
	    ins_compl_insert_bytes(cp_str + compl_len, -1);
	    if (preinsert && move_cursor)
		curwin->w_cursor.col -= (colnr_T)(cp_str_len - leader_len);
	}
    }
    if (match_at_original_text(compl_shown_match) || preinsert)
	compl_used_match = FALSE;
    else
	compl_used_match = TRUE;
#ifdef FEAT_EVAL
    {
	dict_T *dict = ins_compl_dict_alloc(compl_shown_match);

	set_vim_var_dict(VV_COMPLETED_ITEM, dict);
    }
#endif
}

/*
 * show the file name for the completion match (if any).  Truncate the file
 * name to avoid a wait for return.
 */
    static void
ins_compl_show_filename(void)
{
    char	*lead = _("match in file");
    int		space = sc_col - vim_strsize((char_u *)lead) - 2;
    char_u	*s;
    char_u	*e;

    if (space <= 0)
	return;

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
    msg_hist_off = TRUE;
    vim_snprintf((char *)IObuff, IOSIZE, "%s %s%s", lead,
	    s > compl_shown_match->cp_fname ? "<" : "", s);
    msg((char *)IObuff);
    msg_hist_off = FALSE;
    redraw_cmdline = FALSE;	    // don't overwrite!
}

/*
 * Find the appropriate completion item when 'complete' ('cpt') includes
 * a 'max_matches' postfix. In this case, we search for a match where
 * 'cp_in_match_array' is set, indicating that the match is also present
 * in 'compl_match_array'.
 */
    static compl_T *
find_next_match_in_menu(void)
{
    int	    is_forward = compl_shows_dir_forward();
    compl_T *match = compl_shown_match;

    do
	match = is_forward ? match->cp_next : match->cp_prev;
    while (match->cp_next && !match->cp_in_match_array
	    && !match_at_original_text(match));
    return match;
}

/*
 * Find the next set of matches for completion. Repeat the completion "todo"
 * times.  The number of matches found is returned in 'num_matches'.
 *
 * If "allow_get_expansion" is TRUE, then ins_compl_get_exp() may be called to
 * get more completions. If it is FALSE, then do nothing when there are no more
 * completions in the given direction.
 *
 * If "advance" is TRUE, then completion will move to the first match.
 * Otherwise, the original text will be shown.
 *
 * Returns OK on success and -1 if the number of matches are unknown.
 */
    static int
find_next_completion_match(
	int	allow_get_expansion,
	int	todo,		// repeat completion this many times
	int	advance,
	int	*num_matches)
{
    int		found_end = FALSE;
    compl_T	*found_compl = NULL;
    unsigned int cur_cot_flags = get_cot_flags();
    int		compl_no_select = (cur_cot_flags & COT_NOSELECT) != 0
				|| compl_autocomplete;
    int		compl_fuzzy_match = (cur_cot_flags & COT_FUZZY) != 0;
    string_T	*leader;


    while (--todo >= 0)
    {
	if (compl_shows_dir_forward() && compl_shown_match->cp_next != NULL)
	{
	    if (compl_match_array != NULL)
		compl_shown_match = find_next_match_in_menu();
	    else
		compl_shown_match = compl_shown_match->cp_next;
	    found_end = (compl_first_match != NULL
		    && (is_first_match(compl_shown_match->cp_next)
			|| is_first_match(compl_shown_match)));
	}
	else if (compl_shows_dir_backward()
		&& compl_shown_match->cp_prev != NULL)
	{
	    found_end = is_first_match(compl_shown_match);
	    if (compl_match_array != NULL)
		compl_shown_match = find_next_match_in_menu();
	    else
		compl_shown_match = compl_shown_match->cp_prev;
	    found_end |= is_first_match(compl_shown_match);
	}
	else
	{
	    if (!allow_get_expansion)
	    {
		if (advance)
		{
		    if (compl_shows_dir_backward())
			compl_pending -= todo + 1;
		    else
			compl_pending += todo + 1;
		}
		return -1;
	    }

	    if (!compl_no_select && advance)
	    {
		if (compl_shows_dir_backward())
		    --compl_pending;
		else
		    ++compl_pending;
	    }

	    // Find matches.
	    *num_matches = ins_compl_get_exp(&compl_startpos);

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

	leader = get_leader_for_startcol(compl_shown_match, FALSE);

	if (!match_at_original_text(compl_shown_match)
		&& leader->string != NULL
		&& !ins_compl_equal(compl_shown_match,
		    leader->string, (int)leader->length)
		&& !(compl_fuzzy_match && compl_shown_match->cp_score > 0))
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

    return OK;
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
    int	    insert_match)	// Insert the newly selected match
{
    int	    num_matches = -1;
    int	    todo = count;
    int	    advance;
    int	    started = compl_started;
    buf_T   *orig_curbuf = curbuf;
    unsigned int cur_cot_flags = get_cot_flags();
    int	    compl_no_insert = (cur_cot_flags & COT_NOINSERT) != 0
				|| compl_autocomplete;
    int	    compl_fuzzy_match = (cur_cot_flags & COT_FUZZY) != 0;
    int	    compl_preinsert = ins_compl_has_preinsert();

    // When user complete function return -1 for findstart which is next
    // time of 'always', compl_shown_match become NULL.
    if (compl_shown_match == NULL)
	return -1;

    if (compl_leader.string != NULL
	    && !match_at_original_text(compl_shown_match)
	    && !compl_fuzzy_match)
	// Update "compl_shown_match" to the actually shown match
	ins_compl_update_shown_match();

    if (allow_get_expansion && insert_match
	    && (!compl_get_longest || compl_used_match))
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
    if (find_next_completion_match(allow_get_expansion, todo, advance,
							&num_matches) == -1)
	return -1;

    if (curbuf != orig_curbuf)
    {
	// In case some completion function switched buffer, don't want to
	// insert the completion elsewhere.
	return -1;
    }

    // Insert the text of the new completion, or the compl_leader.
    if (compl_no_insert && !started && !compl_preinsert)
    {
	ins_compl_insert_bytes(compl_orig_text.string + get_compl_len(), -1);
	compl_used_match = FALSE;
    }
    else if (insert_match)
    {
	if (!compl_get_longest || compl_used_match)
	    ins_compl_insert(TRUE);
	else
	    ins_compl_insert_bytes(compl_leader.string + get_compl_len(), -1);
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
    if (compl_no_insert && !started && compl_selected_item != -1)
	compl_enter_selects = TRUE;
    else
	compl_enter_selects = !insert_match && compl_match_array != NULL;

    // Show the file name for the match (if any)
    if (compl_shown_match->cp_fname != NULL)
	ins_compl_show_filename();

    return num_matches;
}

/*
 * Check if the current completion source exceeded its timeout. If so, stop
 * collecting, and halve the timeout.
 */
    static void
check_elapsed_time(void)
{
#ifdef ELAPSED_FUNC
    if (cpt_sources_array == NULL)
	return;

    elapsed_T	*start_tv
	= &cpt_sources_array[cpt_sources_index].compl_start_tv;
    long	elapsed_ms = ELAPSED_FUNC(*start_tv);

    if (elapsed_ms > compl_timeout_ms)
    {
	compl_time_slice_expired = TRUE;
	DECAY_COMPL_TIMEOUT();
    }
#endif
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
    if (c != NUL
#ifdef FEAT_EVAL
	    // If test_override("char_avail", 1) was called, ignore characters
	    // waiting in the typeahead buffer.
	    && !disable_char_avail_for_testing
#endif
       )
    {
	if (vim_is_ctrl_x_key(c) && c != Ctrl_X && c != Ctrl_R)
	{
	    c = safe_vgetc();	// Eat the character
	    compl_shows_dir = ins_compl_key2dir(c);
	    (void)ins_compl_next(FALSE, ins_compl_key2count(c),
				      c != K_UP && c != K_DOWN);
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
    else if (compl_autocomplete)
	check_elapsed_time();

    if (compl_pending != 0 && !got_int && !(cot_flags & COT_NOINSERT)
	    && !compl_autocomplete)
    {
	// Insert the first match immediately and advance compl_shown_match,
	// before finding other matches.
	int todo = compl_pending > 0 ? compl_pending : -compl_pending;

	compl_pending = 0;
	(void)ins_compl_next(FALSE, todo, TRUE);
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
    if (ins_compl_pum_key(c) && c != K_UP && c != K_DOWN)
    {
	int h = pum_get_height();
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
 * Get the pattern, column and length for normal completion (CTRL-N CTRL-P
 * completion)
 * Sets the global variables: compl_col, compl_length and compl_pattern.
 * Uses the global variables: compl_cont_status and ctrl_x_mode
 */
    static int
get_normal_compl_info(char_u *line, int startcol, colnr_T curs_col)
{
    if ((compl_cont_status & CONT_SOL) || ctrl_x_mode_path_defines())
    {
	if (!compl_status_adding())
	{
	    while (--startcol >= 0 && vim_isIDc(line[startcol]))
		;
	    compl_col += ++startcol;
	    compl_length = curs_col - startcol;
	}
	if (p_ic)
	{
	    compl_pattern.string = str_foldcase(line + compl_col,
		    compl_length, NULL, 0);
	    if (compl_pattern.string == NULL)
	    {
		compl_pattern.length = 0;
		return FAIL;
	    }
	    compl_pattern.length = STRLEN(compl_pattern.string);
	}
	else
	{
	    compl_pattern.string = vim_strnsave(line + compl_col, (size_t)compl_length);
	    if (compl_pattern.string == NULL)
	    {
		compl_pattern.length = 0;
		return FAIL;
	    }
	    compl_pattern.length = (size_t)compl_length;
	}
    }
    else if (compl_status_adding())
    {
	char_u	    *prefix = (char_u *)"\\<";
	size_t	    prefixlen = STRLEN_LITERAL("\\<");
	size_t	    n;

	if (!vim_iswordp(line + compl_col)
		|| (compl_col > 0
		    && (vim_iswordp(mb_prevptr(line, line + compl_col)))))
	{
	    prefix = (char_u *)"";
	    prefixlen = 0;
	}

	// we need up to 2 extra chars for the prefix
	n = quote_meta(NULL, line + compl_col, compl_length) + prefixlen;
	compl_pattern.string = alloc(n);
	if (compl_pattern.string == NULL)
	{
	    compl_pattern.length = 0;
	    return FAIL;
	}
	STRCPY((char *)compl_pattern.string, prefix);
	(void)quote_meta(compl_pattern.string + prefixlen,
		line + compl_col, compl_length);
	compl_pattern.length = n - 1;
    }
    else if (--startcol < 0
	    || !vim_iswordp(mb_prevptr(line, line + startcol + 1)))
    {
	size_t	len = STRLEN_LITERAL("\\<\\k\\k");

	// Match any word of at least two chars
	compl_pattern.string = vim_strnsave((char_u *)"\\<\\k\\k", len);
	if (compl_pattern.string == NULL)
	{
	    compl_pattern.length = 0;
	    return FAIL;
	}
	compl_pattern.length = len;
	compl_col += curs_col;
	compl_length = 0;
	compl_from_nonkeyword = TRUE;
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
	{
	    while (--startcol >= 0 && vim_iswordc(line[startcol]))
		;
	}

	compl_col += ++startcol;
	compl_length = (int)curs_col - startcol;
	if (compl_length == 1)
	{
	    // Only match word with at least two chars -- webb
	    // there's no need to call quote_meta,
	    // alloc(7) is enough  -- Acevedo
	    compl_pattern.string = alloc(7);
	    if (compl_pattern.string == NULL)
	    {
		compl_pattern.length = 0;
		return FAIL;
	    }
	    STRCPY((char *)compl_pattern.string, "\\<");
	    (void)quote_meta(compl_pattern.string + 2, line + compl_col, 1);
	    STRCAT((char *)compl_pattern.string, "\\k");
	    compl_pattern.length = STRLEN(compl_pattern.string);
	}
	else
	{
	    size_t  n = quote_meta(NULL, line + compl_col, compl_length) + 2;

	    compl_pattern.string = alloc(n);
	    if (compl_pattern.string == NULL)
	    {
		compl_pattern.length = 0;
		return FAIL;
	    }
	    STRCPY((char *)compl_pattern.string, "\\<");
	    (void)quote_meta(compl_pattern.string + 2, line + compl_col,
		    compl_length);
	    compl_pattern.length = n - 1;
	}
    }

    // Call functions in 'complete' with 'findstart=1'
    if (ctrl_x_mode_normal() && !(compl_cont_status & CONT_LOCAL))
    {
	// ^N completion, not complete() or ^X^N
	if (setup_cpt_sources() == FAIL || prepare_cpt_compl_funcs() == FAIL)
	    return FAIL;
    }

    return OK;
}

/*
 * Get the pattern, column and length for whole line completion or for the
 * complete() function.
 * Sets the global variables: compl_col, compl_length and compl_pattern.
 */
    static int
get_wholeline_compl_info(char_u *line, colnr_T curs_col)
{
    compl_col = (colnr_T)getwhitecols(line);
    compl_length = (int)curs_col - (int)compl_col;
    if (compl_length < 0)	// cursor in indent: empty pattern
	compl_length = 0;
    if (p_ic)
    {
	compl_pattern.string = str_foldcase(line + compl_col, compl_length,
		NULL, 0);
	if (compl_pattern.string == NULL)
	{
	    compl_pattern.length = 0;
	    return FAIL;
	}
	compl_pattern.length = STRLEN(compl_pattern.string);
    }
    else
    {
	compl_pattern.string = vim_strnsave(line + compl_col, (size_t)compl_length);
	if (compl_pattern.string == NULL)
	{
	    compl_pattern.length = 0;
	    return FAIL;
	}
	compl_pattern.length = (size_t)compl_length;
    }

    return OK;
}

/*
 * Get the pattern, column and length for filename completion.
 * Sets the global variables: compl_col, compl_length and compl_pattern.
 */
    static int
get_filename_compl_info(char_u *line, int startcol, colnr_T curs_col)
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
    compl_pattern.string = addstar(line + compl_col, compl_length, EXPAND_FILES);
    if (compl_pattern.string == NULL)
    {
	compl_pattern.length = 0;
	return FAIL;
    }

    compl_pattern.length = STRLEN(compl_pattern.string);

    return OK;
}

/*
 * Get the pattern, column and length for command-line completion.
 * Sets the global variables: compl_col, compl_length and compl_pattern.
 */
    static int
get_cmdline_compl_info(char_u *line, colnr_T curs_col)
{
    compl_pattern.string = vim_strnsave(line, curs_col);
    if (compl_pattern.string == NULL)
    {
	compl_pattern.length = 0;
	return FAIL;
    }
    compl_pattern.length = curs_col;
    set_cmd_context(&compl_xp, compl_pattern.string,
	    (int)compl_pattern.length, curs_col, FALSE);
    if (compl_xp.xp_context == EXPAND_UNSUCCESSFUL
	    || compl_xp.xp_context == EXPAND_NOTHING)
	// No completion possible, use an empty pattern to get a
	// "pattern not found" message.
	compl_col = curs_col;
    else
	compl_col = (int)(compl_xp.xp_pattern - compl_pattern.string);
    compl_length = curs_col - compl_col;

    return OK;
}

#ifdef FEAT_COMPL_FUNC
/*
 * Set global variables related to completion:
 * compl_col, compl_length, compl_pattern, and cpt_compl_pattern.
 */
    static int
set_compl_globals(
    int		startcol UNUSED,
    colnr_T	curs_col UNUSED,
    int		is_cpt_compl UNUSED)
{
    if (is_cpt_compl)
    {
	VIM_CLEAR_STRING(cpt_compl_pattern);
	if (startcol < compl_col)
	    return prepend_startcol_text(&cpt_compl_pattern, &compl_orig_text,
		    startcol);
	else
	{
	    cpt_compl_pattern.string = vim_strnsave(compl_orig_text.string,
		    compl_orig_text.length);
	    cpt_compl_pattern.length = compl_orig_text.length;
	}
    }
    else
    {
	if (startcol < 0 || startcol > curs_col)
	    startcol = curs_col;

	// Re-obtain line in case it has changed
	char_u	*line = ml_get(curwin->w_cursor.lnum);
	int	len = curs_col - startcol;

	compl_pattern.string = vim_strnsave(line + startcol, (size_t)len);
	if (compl_pattern.string == NULL)
	{
	    compl_pattern.length = 0;
	    return FAIL;
	}
	compl_pattern.length = (size_t)len;
	compl_col = startcol;
	compl_length = len;
    }

    return OK;
}
#endif

/*
 * Get the pattern, column and length for user defined completion ('omnifunc',
 * 'completefunc' and 'thesaurusfunc')
 * Uses the global variable: spell_bad_len
 * Callback function "cb" is set if triggered by a function in the 'cpt'
 * option; otherwise, it is NULL.
 * "startcol", when not NULL, contains the column returned by function.
 */
    static int
get_userdefined_compl_info(
    colnr_T	curs_col UNUSED,
    callback_T	*cb UNUSED,
    int		*startcol UNUSED)
{
    int		ret = FAIL;

#ifdef FEAT_COMPL_FUNC
    // Call user defined function 'completefunc' with "a:findstart"
    // set to 1 to obtain the length of text to use for completion.
    typval_T	args[3];
    int		col;
    char_u	*funcname = NULL;
    pos_T	pos;
    int		save_State = State;
    int		is_cpt_function = (cb != NULL);

    if (!is_cpt_function)
    {
	// Call 'completefunc' or 'omnifunc' or 'thesaurusfunc' and get pattern
	// length as a string
	funcname = get_complete_funcname(ctrl_x_mode);
	if (*funcname == NUL)
	{
	    semsg(_(e_option_str_is_not_set), ctrl_x_mode_function()
		    ? "completefunc" : "omnifunc");
	    return FAIL;
	}
	cb = get_insert_callback(ctrl_x_mode);
    }

    args[0].v_type = VAR_NUMBER;
    args[0].vval.v_number = 1;
    args[1].v_type = VAR_STRING;
    args[1].vval.v_string = (char_u *)"";
    args[2].v_type = VAR_UNKNOWN;
    pos = curwin->w_cursor;
    ++textlock;
    col = call_callback_retnr(cb, 2, args);
    --textlock;

    State = save_State;
    curwin->w_cursor = pos;	// restore the cursor position
    check_cursor();  // make sure cursor position is valid, just in case
    validate_cursor();
    if (!EQUAL_POS(curwin->w_cursor, pos))
    {
	emsg(_(e_complete_function_deleted_text));
	return FAIL;
    }

    if (startcol != NULL)
	*startcol = col;

    // Return value -2 means the user complete function wants to cancel the
    // complete without an error, do the same if the function did not execute
    // successfully.
    if (col == -2 || aborting())
	return FAIL;

    // Return value -3 does the same as -2 and leaves CTRL-X mode.
    if (col == -3)
    {
	if (is_cpt_function)
	    return FAIL;
	ctrl_x_mode = CTRL_X_NORMAL;
	edit_submode = NULL;
	if (!shortmess(SHM_COMPLETIONMENU))
	    msg_clr_cmdline();
	return FAIL;
    }

    // Reset extended parameters of completion, when starting new
    // completion.
    compl_opt_refresh_always = FALSE;
    compl_opt_suppress_empty = FALSE;

    ret = !is_cpt_function ? set_compl_globals(col, curs_col, FALSE) : OK;
#endif

    return ret;
}

/*
 * Get the pattern, column and length for spell completion.
 * Sets the global variables: compl_col, compl_length and compl_pattern.
 * Uses the global variable: spell_bad_len
 */
    static int
get_spell_compl_info(int startcol UNUSED, colnr_T curs_col UNUSED)
{
    int		ret = FAIL;
#ifdef FEAT_SPELL
    char_u	*line = NULL;

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
    compl_pattern.string = vim_strnsave(line + compl_col, (size_t)compl_length);
    if (compl_pattern.string == NULL)
    {
	compl_pattern.length = 0;
	return FAIL;
    }

    compl_pattern.length = (size_t)compl_length;
    ret = OK;
#endif

    return ret;
}

/*
 * Get the completion pattern, column and length.
 * "startcol" - start column number of the completion pattern/text
 * "cur_col" - current cursor column
 * On return, "line_invalid" is set to TRUE, if the current line may have
 * become invalid and needs to be fetched again.
 * Returns OK on success.
 */
    static int
compl_get_info(char_u *line, int startcol, colnr_T curs_col, int *line_invalid)
{
    if (ctrl_x_mode_normal() || ctrl_x_mode_register()
	    || (ctrl_x_mode & CTRL_X_WANT_IDENT
		&& !thesaurus_func_complete(ctrl_x_mode)))
    {
	if (get_normal_compl_info(line, startcol, curs_col) != OK)
	    return FAIL;
	*line_invalid = TRUE; // 'cpt' func may have invalidated "line"
    }
    else if (ctrl_x_mode_line_or_eval())
    {
	return get_wholeline_compl_info(line, curs_col);
    }
    else if (ctrl_x_mode_files())
    {
	return get_filename_compl_info(line, startcol, curs_col);
    }
    else if (ctrl_x_mode == CTRL_X_CMDLINE)
    {
	return get_cmdline_compl_info(line, curs_col);
    }
    else if (ctrl_x_mode_function() || ctrl_x_mode_omni()
	    || thesaurus_func_complete(ctrl_x_mode))
    {
	if (get_userdefined_compl_info(curs_col, NULL, NULL) != OK)
	    return FAIL;
	*line_invalid = TRUE;	// "line" may have become invalid
    }
    else if (ctrl_x_mode_spell())
    {
	if (get_spell_compl_info(startcol, curs_col) == FAIL)
	    return FAIL;
	*line_invalid = TRUE;	// "line" may have become invalid
    }
    else
    {
	internal_error("ins_complete()");
	return FAIL;
    }

    return OK;
}

/*
 * Continue an interrupted completion mode search in "line".
 *
 * If this same ctrl_x_mode has been interrupted use the text from
 * "compl_startpos" to the cursor as a pattern to add a new word instead of
 * expand the one before the cursor, in word-wise if "compl_startpos" is not in
 * the same line as the cursor then fix it (the line has been split because it
 * was longer than 'tw').  if SOL is set then skip the previous pattern, a word
 * at the beginning of the line has been inserted, we'll look for that.
 */
    static void
ins_compl_continue_search(char_u *line)
{
    // it is a continued search
    compl_cont_status &= ~CONT_INTRPT;	// remove INTRPT
    if (ctrl_x_mode_normal() || ctrl_x_mode_path_patterns()
						|| ctrl_x_mode_path_defines())
    {
	if (compl_startpos.lnum != curwin->w_cursor.lnum)
	{
	    // line (probably) wrapped, set compl_startpos to the
	    // first non_blank in the line, if it is not a wordchar
	    // include it to get a better pattern, but then we don't
	    // want the "\\<" prefix, check it below
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
    else if (ctrl_x_mode_line_or_eval() || ctrl_x_mode_register())
	compl_cont_status = CONT_ADDING | CONT_N_ADDS;
    else
	compl_cont_status = 0;
}

/*
 * start insert mode completion
 */
    static int
ins_compl_start(void)
{
    char_u	*line = NULL;
    int		startcol = 0;	    // column where searched text starts
    colnr_T	curs_col;	    // cursor column
    int		line_invalid = FALSE;
    int		save_did_ai = did_ai;
    int		flags = CP_ORIGINAL_TEXT;

    // First time we hit ^N or ^P (in a row, I mean)
    did_ai = FALSE;
    did_si = FALSE;
    can_si = FALSE;
    can_si_back = FALSE;
    if (stop_arrow() == FAIL)
	return FAIL;

    line = ml_get(curwin->w_cursor.lnum);
    curs_col = curwin->w_cursor.col;
    compl_pending = 0;
    compl_lnum = curwin->w_cursor.lnum;

    if ((compl_cont_status & CONT_INTRPT) == CONT_INTRPT
	    && compl_cont_mode == ctrl_x_mode)
	// this same ctrl-x_mode was interrupted previously. Continue the
	// completion.
	ins_compl_continue_search(line);
    else
	compl_cont_status &= CONT_LOCAL;

    if (!compl_status_adding())	// normal expansion
    {
	compl_cont_mode = ctrl_x_mode;
	if (ctrl_x_mode_not_default())
	    // Remove LOCAL if ctrl_x_mode != CTRL_X_NORMAL
	    compl_cont_status = 0;
	compl_cont_status |= CONT_N_ADDS;
	compl_startpos = curwin->w_cursor;
	startcol = (int)curs_col;
	compl_col = 0;
    }

    // Work out completion pattern and original text -- webb
    if (compl_get_info(line, startcol, curs_col, &line_invalid) == FAIL)
    {
	if (ctrl_x_mode_function() || ctrl_x_mode_omni()
				|| thesaurus_func_complete(ctrl_x_mode))
	    // restore did_ai, so that adding comment leader works
	    did_ai = save_did_ai;
	return FAIL;
    }
    // If "line" was changed while getting completion info get it again.
    if (line_invalid)
	line = ml_get(curwin->w_cursor.lnum);

    if (compl_status_adding())
    {
	if (!shortmess(SHM_COMPLETIONMENU))
	    edit_submode_pre = (char_u *)_(" Adding");
	if (ctrl_x_mode_line_or_eval())
	{
	    // Insert a new line, keep indentation but ignore 'comments'.
	    char_u *old = curbuf->b_p_com;

	    curbuf->b_p_com = (char_u *)"";
	    compl_startpos.lnum = curwin->w_cursor.lnum;
	    compl_startpos.col = compl_col;
	    ins_eol('\r');
	    curbuf->b_p_com = old;
	    compl_length = 0;
	    compl_col = curwin->w_cursor.col;
	    compl_lnum = curwin->w_cursor.lnum;
	}
	else if (ctrl_x_mode_normal() && cfc_has_mode())
	{
	    compl_startpos = curwin->w_cursor;
	    compl_cont_status &= CONT_S_IPOS;
	}
    }
    else
    {
	edit_submode_pre = NULL;
	compl_startpos.col = compl_col;
    }

    if (!shortmess(SHM_COMPLETIONMENU) && !compl_autocomplete)
    {
	if (compl_cont_status & CONT_LOCAL)
	    edit_submode = (char_u *)_(ctrl_x_msgs[CTRL_X_LOCAL_MSG]);
	else
	    edit_submode = (char_u *)_(CTRL_X_MSG(ctrl_x_mode));
    }

    // If any of the original typed text has been changed we need to fix
    // the redo buffer.
    ins_compl_fixRedoBufForLeader(NULL);

    // Always add completion for the original text.
    VIM_CLEAR_STRING(compl_orig_text);
    compl_orig_text.length = (size_t)compl_length;
    compl_orig_text.string = vim_strnsave(line + compl_col, (size_t)compl_length);
    if (p_ic)
	flags |= CP_ICASE;
    if (compl_orig_text.string == NULL
	    || ins_compl_add(compl_orig_text.string,
		(int)compl_orig_text.length,
		NULL, NULL, NULL, 0, flags, FALSE, NULL, 0) != OK)
    {
	VIM_CLEAR_STRING(compl_pattern);
	VIM_CLEAR_STRING(compl_orig_text);
	return FAIL;
    }

    // showmode might reset the internal line pointers, so it must
    // be called before line = ml_get(), or when this address is no
    // longer needed.  -- Acevedo.
    if (!shortmess(SHM_COMPLETIONMENU) && !compl_autocomplete)
    {
	edit_submode_extra = (char_u *)_("-- Searching...");
	edit_submode_highl = HLF_COUNT;
	showmode();
	edit_submode_extra = NULL;
	out_flush();
    }

    return OK;
}

/*
 * display the completion status message
 */
    static void
ins_compl_show_statusmsg(void)
{
    // we found no match if the list has only the "compl_orig_text"-entry
    if (is_first_match(compl_first_match->cp_next))
    {
	edit_submode_extra = compl_status_adding() && compl_length > 1
				? (char_u *)_("Hit end of paragraph")
				: (char_u *)_("Pattern not found");
	edit_submode_highl = HLF_E;
    }

    if (edit_submode_extra == NULL)
    {
	if (match_at_original_text(compl_curr_match))
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
	    compl_curr_match->cp_number = 1;
	}
	else
	{
#if defined(FEAT_COMPL_FUNC) || defined(FEAT_EVAL)
	    // Update completion sequence number when needed.
	    if (compl_curr_match->cp_number == -1)
		ins_compl_update_sequence_numbers();
#endif
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
		{
		    msg_hist_off = TRUE;
		    msg_attr((char *)edit_submode_extra,
			    edit_submode_highl < HLF_COUNT
			    ? HL_ATTR(edit_submode_highl) : 0);
		    msg_hist_off = FALSE;
		}
	    }
	    else
		msg_clr_cmdline();	// necessary for "noshowmode"
	}
    }
}

/*
 * Do Insert mode completion.
 * Called when character "c" was typed, which has a meaning for completion.
 * Returns OK if completion was done, FAIL if something failed (out of mem).
 */
    int
ins_complete(int c, int enable_pum)
{
    int		n;
    int		save_w_wrow;
    int		save_w_leftcol;
    int		insert_match;

    compl_direction = ins_compl_key2dir(c);
    insert_match = ins_compl_use_match(c);

    if (!compl_started)
    {
	if (ins_compl_start() == FAIL)
	    return FAIL;
    }
    else if (insert_match && stop_arrow() == FAIL)
	return FAIL;

    compl_curr_win = curwin;
    compl_curr_buf = curwin->w_buffer;
    compl_shown_match = compl_curr_match;
    compl_shows_dir = compl_direction;

    // Find next match (and following matches).
    save_w_wrow = curwin->w_wrow;
    save_w_leftcol = curwin->w_leftcol;
    n = ins_compl_next(TRUE, ins_compl_key2count(c), insert_match);

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
    if (is_first_match(compl_first_match->cp_next))
    {
	// remove N_ADDS flag, so next ^X<> won't try to go to ADDING mode,
	// because we couldn't expand anything at first place, but if we used
	// ^P, ^N, ^X^I or ^X^D we might want to add-expand a single-char-word
	// (such as M in M'exico) if not tried already.  -- Acevedo
	if (compl_length > 1
		|| compl_status_adding()
		|| (ctrl_x_mode_not_default()
		    && !ctrl_x_mode_path_patterns()
		    && !ctrl_x_mode_path_defines()))
	    compl_cont_status &= ~CONT_N_ADDS;
    }

    if (compl_curr_match->cp_flags & CP_CONT_S_IPOS)
	compl_cont_status |= CONT_S_IPOS;
    else
	compl_cont_status &= ~CONT_S_IPOS;

    if (!shortmess(SHM_COMPLETIONMENU) && !compl_autocomplete)
	ins_compl_show_statusmsg();

    // Show the popup menu, unless we got interrupted.
    if (enable_pum && !compl_interrupted)
	show_pum(save_w_wrow, save_w_leftcol);

    compl_was_interrupted = compl_interrupted;
    compl_interrupted = FALSE;

    return OK;
}

/*
 * Returns TRUE if the given character 'c' can be used to trigger
 * autocompletion.
 */
    int
ins_compl_setup_autocompl(int c)
{
#ifdef ELAPSED_FUNC
    if (vim_isprintc(c))
    {
	compl_autocomplete = TRUE;
	return TRUE;
    }
#endif
    return FALSE;
}

/*
 * Remove (if needed) and show the popup menu
 */
    static void
show_pum(int prev_w_wrow, int prev_w_leftcol)
{
    // RedrawingDisabled may be set when invoked through complete().
    int save_RedrawingDisabled = RedrawingDisabled;
    RedrawingDisabled = 0;

    // If the cursor moved or the display scrolled we need to remove the pum
    // first.
    setcursor();
    if (prev_w_wrow != curwin->w_wrow || prev_w_leftcol != curwin->w_leftcol)
	ins_compl_del_pum();

    ins_compl_show_pum();
    setcursor();

    RedrawingDisabled = save_RedrawingDisabled;
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
		if (ctrl_x_mode_dictionary() || ctrl_x_mode_thesaurus())
		    break;
		// FALLTHROUGH
	    case '~':
		if (!magic_isset())	// quote these only if magic is set
		    break;
		// FALLTHROUGH
	    case '\\':
		if (ctrl_x_mode_dictionary() || ctrl_x_mode_thesaurus())
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
	    int mb_len = (*mb_ptr2len)(src) - 1;
	    if (mb_len > 0 && len >= mb_len)
		for (int i = 0; i < mb_len; ++i)
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
    VIM_CLEAR_STRING(compl_orig_text);
# ifdef FEAT_EVAL
    free_callback(&cfu_cb);
    free_callback(&ofu_cb);
    free_callback(&tsrfu_cb);
# endif
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

    spell_bad_len = spell_move_to(curwin, BACKWARD, SMT_ALL, TRUE, NULL);
    if (curwin->w_cursor.col != tpos.col)
	start_arrow(&tpos);
}
#endif

/*
 * Reset the info associated with completion sources.
 */
    static void
cpt_sources_clear(void)
{
    VIM_CLEAR(cpt_sources_array);
    cpt_sources_index = -1;
    cpt_sources_count = 0;
}

/*
 * Setup completion sources.
 */
    static int
setup_cpt_sources(void)
{
    char_u  buf[LSIZE];
    int	    slen;
    int	    count = 0, idx = 0;
    char_u  *p, *cpt;

    // Make a copy of 'cpt' in case the buffer gets wiped out
    cpt = vim_strsave(curbuf->b_p_cpt);
    if (cpt == NULL)
	return FAIL;

    for (p = cpt; *p;)
    {
	while (*p == ',' || *p == ' ') // Skip delimiters
	    p++;
	if (*p) // If not end of string, count this segment
	{
	    (void)copy_option_part(&p, buf, LSIZE, ","); // Advance p
	    count++;
	}
    }
    if (count == 0)
	goto theend;

    cpt_sources_clear();
    cpt_sources_count = count;
    cpt_sources_array = ALLOC_CLEAR_MULT(cpt_source_T, count);
    if (cpt_sources_array == NULL)
    {
	cpt_sources_count = 0;
	vim_free(cpt);
	return FAIL;
    }

    for (p = cpt; *p;)
    {
	while (*p == ',' || *p == ' ') // Skip delimiters
	    p++;
	if (*p) // If not end of string, count this segment
	{
	    char_u *t;

	    vim_memset(buf, 0, LSIZE);
	    slen = copy_option_part(&p, buf, LSIZE, ","); // Advance p
	    if (slen > 0 && (t = vim_strchr(buf, '^')) != NULL)
		cpt_sources_array[idx].cs_max_matches = atoi((char *)t + 1);
	    idx++;
	}
    }

theend:
    vim_free(cpt);
    return OK;
}

/*
 * Return TRUE if any of the completion sources have 'refresh' set to 'always'.
 */
    static int
is_cpt_func_refresh_always(void)
{
#ifdef FEAT_COMPL_FUNC
    for (int i = 0; i < cpt_sources_count; i++)
	if (cpt_sources_array[i].cs_refresh_always)
	    return TRUE;
#endif
    return FALSE;
}

/*
 * Make the completion list non-cyclic.
 */
    static void
ins_compl_make_linear(void)
{
    compl_T *m;

    if (compl_first_match == NULL || compl_first_match->cp_prev == NULL)
	return;
    m = compl_first_match->cp_prev;
    m->cp_next = NULL;
    compl_first_match->cp_prev = NULL;
}

/*
 * Remove the matches linked to the current completion source (as indicated by
 * cpt_sources_index) from the completion list.
 */
#ifdef FEAT_COMPL_FUNC
    static compl_T *
remove_old_matches(void)
{
    compl_T *sublist_start = NULL, *sublist_end = NULL, *insert_at = NULL;
    compl_T *current, *next;
    int	    compl_shown_removed = FALSE;
    int	    forward = (compl_first_match->cp_cpt_source_idx < 0);

    compl_direction = forward ? FORWARD : BACKWARD;
    compl_shows_dir = compl_direction;

    // Identify the sublist of old matches that needs removal
    for (current = compl_first_match; current != NULL; current = current->cp_next)
    {
	if (current->cp_cpt_source_idx < cpt_sources_index &&
		(forward || (!forward && !insert_at)))
	    insert_at = current;

	if (current->cp_cpt_source_idx == cpt_sources_index)
	{
	    if (!sublist_start)
		sublist_start = current;
	    sublist_end = current;
	    if (!compl_shown_removed && compl_shown_match == current)
		compl_shown_removed = TRUE;
	}

	if ((forward && current->cp_cpt_source_idx > cpt_sources_index)
		|| (!forward && insert_at))
	    break;
    }

    // Re-assign compl_shown_match if necessary
    if (compl_shown_removed)
    {
	if (forward)
	    compl_shown_match = compl_first_match;
	else
	{  // Last node will have the prefix that is being completed
	    for (current = compl_first_match; current->cp_next != NULL;
		    current = current->cp_next)
		;
	    compl_shown_match = current;
	}
    }

    if (!sublist_start) // No nodes to remove
	return insert_at;

    // Update links to remove sublist
    if (sublist_start->cp_prev)
	sublist_start->cp_prev->cp_next = sublist_end->cp_next;
    else
	compl_first_match = sublist_end->cp_next;

    if (sublist_end->cp_next)
	sublist_end->cp_next->cp_prev = sublist_start->cp_prev;

    // Free all nodes in the sublist
    sublist_end->cp_next = NULL;
    for (current = sublist_start; current != NULL; current = next)
    {
	next = current->cp_next;
	ins_compl_item_free(current);
    }

    return insert_at;
}
#endif

/*
 * Retrieve completion matches using the callback function "cb" and store the
 * 'refresh:always' flag.
 */
#ifdef FEAT_COMPL_FUNC
    static void
get_cpt_func_completion_matches(callback_T *cb UNUSED)
{
    int	startcol = cpt_sources_array[cpt_sources_index].cs_startcol;

    if (startcol == -2 || startcol == -3)
	return;

    if (set_compl_globals(startcol, curwin->w_cursor.col, TRUE) == OK)
    {
	expand_by_function(0, cpt_compl_pattern.string, cb);

	cpt_sources_array[cpt_sources_index].cs_refresh_always =
	    compl_opt_refresh_always;
	compl_opt_refresh_always = FALSE;
    }
}
#endif

/*
 * Retrieve completion matches from functions in the 'cpt' option where the
 * 'refresh:always' flag is set.
 */
    static void
cpt_compl_refresh(void)
{
#ifdef FEAT_COMPL_FUNC
    char_u	*cpt;
    char_u	*p;
    callback_T	*cb = NULL;
    int		startcol, ret;

    // Make the completion list linear (non-cyclic)
    ins_compl_make_linear();
    // Make a copy of 'cpt' in case the buffer gets wiped out
    cpt = vim_strsave(curbuf->b_p_cpt);
    strip_caret_numbers_in_place(cpt);

    cpt_sources_index = 0;
    for (p = cpt; *p;)
    {
	while (*p == ',' || *p == ' ') // Skip delimiters
	    p++;
	if (*p == NUL)
	    break;

	if (cpt_sources_array[cpt_sources_index].cs_refresh_always)
	{
	    cb = get_callback_if_cpt_func(p);
	    if (cb)
	    {
		compl_curr_match = remove_old_matches();
		ret = get_userdefined_compl_info(curwin->w_cursor.col, cb,
			&startcol);
		if (ret == FAIL)
		{
		    if (startcol == -3)
			cpt_sources_array[cpt_sources_index].cs_refresh_always
			    = FALSE;
		    else
			startcol = -2;
		}
		cpt_sources_array[cpt_sources_index].cs_startcol = startcol;
		if (ret == OK)
		{
		    compl_source_start_timer(cpt_sources_index);
		    get_cpt_func_completion_matches(cb);
		}
	    }
	    else
		cpt_sources_array[cpt_sources_index].cs_startcol
		    = STARTCOL_NONE;
	}

	(void)copy_option_part(&p, IObuff, IOSIZE, ","); // Advance p
	if (may_advance_cpt_index(p))
	    (void)advance_cpt_sources_index_safe();
    }
    cpt_sources_index = -1;

    vim_free(cpt);
    // Make the list cyclic
    compl_matches = ins_compl_make_cyclic();
#endif
}
