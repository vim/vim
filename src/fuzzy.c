/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 *
 * fuzzy.c: fuzzy matching algorithm and related functions
 *
 * Portions of this file are adapted from fzy (https://github.com/jhawthorn/fzy)
 * Original code:
 *   Copyright (c) 2014 John Hawthorn
 *   Licensed under the MIT License.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "vim.h"

#if defined(FEAT_EVAL) || defined(FEAT_PROTO)
static int fuzzy_match_item_compare(const void *s1, const void *s2);
static void fuzzy_match_in_list(list_T *l, char_u *str, int matchseq, char_u *key, callback_T *item_cb, int retmatchpos, list_T *fmatchlist, long max_matches);
static void do_fuzzymatch(typval_T *argvars, typval_T *rettv, int retmatchpos);
#endif
static int fuzzy_match_str_compare(const void *s1, const void *s2);
static void fuzzy_match_str_sort(fuzmatch_str_T *fm, int sz);
static int fuzzy_match_func_compare(const void *s1, const void *s2);
static void fuzzy_match_func_sort(fuzmatch_str_T *fm, int sz);

typedef double score_t;
static score_t match_positions(char_u *needle, char_u *haystack, int_u *positions);
static int has_match(char_u *needle, char_u *haystack);

#define SCORE_MAX INFINITY
#define SCORE_MIN (-INFINITY)
#define SCORE_SCALE 1000

typedef struct
{
    int		idx;		// used for stable sort
    listitem_T	*item;
    int		score;
    list_T	*lmatchpos;
    char_u	*pat;
    char_u	*itemstr;
    int		itemstr_allocated;
    int		startpos;
} fuzzyItem_T;

/*
 * fuzzy_match()
 *
 * Returns TRUE if "pat_arg" matches "str". Also returns the match score in
 * "outScore" and the matching character positions in "matches".
 */
    int
fuzzy_match(
    char_u	*str,
    char_u	*pat_arg,
    int		matchseq,
    int		*outScore,
    int_u	*matches,
    int		maxMatches)
{
    char_u	*save_pat;
    char_u	*pat;
    char_u	*p;
    int		complete = FALSE;
    int		score = 0;
    int		numMatches = 0;
    score_t	fzy_score;

    *outScore = 0;

    save_pat = vim_strsave(pat_arg);
    if (save_pat == NULL)
	return FALSE;
    pat = save_pat;
    p = pat;

    // Try matching each word in 'pat_arg' in 'str'
    while (TRUE)
    {
	if (matchseq)
	    complete = TRUE;
	else
	{
	    // Extract one word from the pattern (separated by space)
	    p = skipwhite(p);
	    if (*p == NUL)
		break;
	    pat = p;
	    while (*p != NUL && !VIM_ISWHITE(PTR2CHAR(p)))
	    {
		if (has_mbyte)
		    MB_PTR_ADV(p);
		else
		    ++p;
	    }
	    if (*p == NUL)		// processed all the words
		complete = TRUE;
	    *p = NUL;
	}

	score = FUZZY_SCORE_NONE;
	if (has_match(pat, str))
	{
	    fzy_score = match_positions(pat, str, matches + numMatches);
	    score = (fzy_score == SCORE_MIN) ? INT_MIN + 1
		: (fzy_score == SCORE_MAX) ? INT_MAX
		: (fzy_score < 0) ? (int)ceil(fzy_score * SCORE_SCALE - 0.5)
		: (int)floor(fzy_score * SCORE_SCALE + 0.5);
	}

	if (score == FUZZY_SCORE_NONE)
	{
	    numMatches = 0;
	    *outScore = FUZZY_SCORE_NONE;
	    break;
	}

	if (score > 0 && *outScore > INT_MAX - score)
	    *outScore = INT_MAX;
	else if (score < 0 && *outScore < INT_MIN + 1 - score)
	    *outScore = INT_MIN + 1;
	else
	    *outScore += score;

	numMatches += MB_CHARLEN(pat);

	if (complete || numMatches >= maxMatches)
	    break;

	// try matching the next word
	++p;
    }

    vim_free(save_pat);
    return numMatches != 0;
}

#if defined(FEAT_EVAL) || defined(FEAT_PROTO)
/*
 * Sort the fuzzy matches in the descending order of the match score.
 * For items with same score, retain the order using the index (stable sort)
 */
    static int
fuzzy_match_item_compare(const void *s1, const void *s2)
{
    int		v1 = ((fuzzyItem_T *)s1)->score;
    int		v2 = ((fuzzyItem_T *)s2)->score;

    if (v1 == v2)
    {
	int exact_match1 = FALSE, exact_match2 = FALSE;
	char_u *pat = ((fuzzyItem_T *)s1)->pat;
	int patlen = (int)STRLEN(pat);
	int startpos = ((fuzzyItem_T *)s1)->startpos;
	exact_match1 = (startpos >= 0) && STRNCMP(pat,
		((fuzzyItem_T *)s1)->itemstr + startpos, patlen) == 0;
	startpos = ((fuzzyItem_T *)s2)->startpos;
	exact_match2 = (startpos >= 0) && STRNCMP(pat,
		((fuzzyItem_T *)s2)->itemstr + startpos, patlen) == 0;

	if (exact_match1 == exact_match2)
	{
	    int idx1 = ((fuzzyItem_T *)s1)->idx;
	    int idx2 = ((fuzzyItem_T *)s2)->idx;
	    return idx1 == idx2 ? 0 : idx1 > idx2 ? 1 : -1;
	}
	else if (exact_match2)
	    return 1;
	return -1;
    }
    else
	return v1 > v2 ? -1 : 1;
}

/*
 * Fuzzy search the string 'str' in a list of 'items' and return the matching
 * strings in 'fmatchlist'.
 * If 'matchseq' is TRUE, then for multi-word search strings, match all the
 * words in sequence.
 * If 'items' is a list of strings, then search for 'str' in the list.
 * If 'items' is a list of dicts, then either use 'key' to lookup the string
 * for each item or use 'item_cb' Funcref function to get the string.
 * If 'retmatchpos' is TRUE, then return a list of positions where 'str'
 * matches for each item.
 */
    static void
fuzzy_match_in_list(
    list_T	*l,
    char_u	*str,
    int		matchseq,
    char_u	*key,
    callback_T	*item_cb,
    int		retmatchpos,
    list_T	*fmatchlist,
    long	max_matches)
{
    long	    len;
    fuzzyItem_T	    *items = NULL;
    listitem_T	    *li;
    long	    match_count = 0;
    int_u	    matches[FUZZY_MATCH_MAX_LEN];

    len = list_len(l);
    if (len == 0)
	return;
    if (max_matches > 0 && len > max_matches)
	len = max_matches;

    items = ALLOC_CLEAR_MULT(fuzzyItem_T, len);
    if (items == NULL)
	return;

    // For all the string items in items, get the fuzzy matching score
    FOR_ALL_LIST_ITEMS(l, li)
    {
	int		score;
	char_u		*itemstr = NULL;
	char_u		*itemstr_copy = NULL;
	typval_T	rettv;
	int		itemstr_allocate = FALSE;
	list_T		*match_positions = NULL;

	if (max_matches > 0 && match_count >= max_matches)
	    break;

	rettv.v_type = VAR_UNKNOWN;
	if (li->li_tv.v_type == VAR_STRING)	// list of strings
	    itemstr = li->li_tv.vval.v_string;
	else if (li->li_tv.v_type == VAR_DICT
				&& (key != NULL || item_cb->cb_name != NULL))
	{
	    // For a dict, either use the specified key to lookup the string or
	    // use the specified callback function to get the string.
	    if (key != NULL)
		itemstr = dict_get_string(li->li_tv.vval.v_dict,
							   (char *)key, FALSE);
	    else
	    {
		typval_T	argv[2];

		// Invoke the supplied callback (if any) to get the dict item
		li->li_tv.vval.v_dict->dv_refcount++;
		argv[0].v_type = VAR_DICT;
		argv[0].vval.v_dict = li->li_tv.vval.v_dict;
		argv[1].v_type = VAR_UNKNOWN;
		if (call_callback(item_cb, -1, &rettv, 1, argv) != FAIL)
		{
		    if (rettv.v_type == VAR_STRING)
		    {
			itemstr = rettv.vval.v_string;
			itemstr_allocate = TRUE;
		    }
		}
		dict_unref(li->li_tv.vval.v_dict);
	    }
	}

	if (itemstr != NULL
		&& fuzzy_match(itemstr, str, matchseq, &score, matches,
						FUZZY_MATCH_MAX_LEN))
	{
	    if (itemstr_allocate)
	    {
		itemstr_copy = vim_strsave(itemstr);
		if (itemstr_copy == NULL)
		{
		    clear_tv(&rettv);
		    continue;
		}
	    }
	    else
		itemstr_copy = itemstr;

	    // Copy the list of matching positions in itemstr to a list, if
	    // "retmatchpos" is set.
	    if (retmatchpos)
	    {
		match_positions = list_alloc();
		if (match_positions == NULL)
		{
		    if (itemstr_allocate && itemstr_copy)
			vim_free(itemstr_copy);
		    clear_tv(&rettv);
		    continue;
		}

		// Fill position information
		int	j = 0;
		char_u	*p = str;
		int	success = TRUE;

		while (*p != NUL && j < FUZZY_MATCH_MAX_LEN && success)
		{
		    if (!VIM_ISWHITE(PTR2CHAR(p)) || matchseq)
		    {
			if (list_append_number(match_positions, matches[j]) == FAIL)
			{
			    success = FALSE;
			    break;
			}
			j++;
		    }
		    if (has_mbyte)
			MB_PTR_ADV(p);
		    else
			++p;
		}

		if (!success)
		{
		    list_free(match_positions);
		    if (itemstr_allocate && itemstr_copy)
			vim_free(itemstr_copy);
		    clear_tv(&rettv);
		    continue;
		}
	    }
	    items[match_count].idx = match_count;
	    items[match_count].item = li;
	    items[match_count].score = score;
	    items[match_count].pat = str;
	    items[match_count].startpos = matches[0];
	    items[match_count].itemstr = itemstr_copy;
	    items[match_count].itemstr_allocated = itemstr_allocate;
	    items[match_count].lmatchpos = match_positions;

	    ++match_count;
	}
	clear_tv(&rettv);
    }

    if (match_count > 0)
    {
	list_T		*retlist;

	// Sort the list by the descending order of the match score
	qsort((void *)items, (size_t)match_count, sizeof(fuzzyItem_T),
		fuzzy_match_item_compare);

	// For matchfuzzy(), return a list of matched strings.
	//	    ['str1', 'str2', 'str3']
	// For matchfuzzypos(), return a list with three items.
	// The first item is a list of matched strings. The second item
	// is a list of lists where each list item is a list of matched
	// character positions. The third item is a list of matching scores.
	//	[['str1', 'str2', 'str3'], [[1, 3], [1, 3], [1, 3]]]
	if (retmatchpos)
	{
	    li = list_find(fmatchlist, 0);
	    if (li == NULL || li->li_tv.vval.v_list == NULL)
		goto done;
	    retlist = li->li_tv.vval.v_list;
	}
	else
	    retlist = fmatchlist;

	// Copy the matching strings to the return list
	for (int i = 0; i < match_count; i++)
	{
	    if (list_append_tv(retlist, &items[i].item->li_tv) == FAIL)
		goto done;
	}

	// next copy the list of matching positions
	if (retmatchpos)
	{
	    li = list_find(fmatchlist, -2);
	    if (li == NULL || li->li_tv.vval.v_list == NULL)
		goto done;
	    retlist = li->li_tv.vval.v_list;

	    for (int i = 0; i < match_count; i++)
	    {
		if (items[i].lmatchpos != NULL)
		{
		    if (list_append_list(retlist, items[i].lmatchpos) == OK)
			items[i].lmatchpos = NULL;
		    else
			goto done;

		}
	    }

	    // copy the matching scores
	    li = list_find(fmatchlist, -1);
	    if (li == NULL || li->li_tv.vval.v_list == NULL)
		goto done;
	    retlist = li->li_tv.vval.v_list;
	    for (int i = 0; i < match_count; i++)
	    {
		if (list_append_number(retlist, items[i].score) == FAIL)
		    goto done;
	    }
	}
    }

done:
    for (int i = 0; i < match_count; i++)
    {
	if (items[i].itemstr_allocated)
	    vim_free(items[i].itemstr);

	if (items[i].lmatchpos)
	    list_free(items[i].lmatchpos);
    }
    vim_free(items);
}

/*
 * Do fuzzy matching. Returns the list of matched strings in 'rettv'.
 * If 'retmatchpos' is TRUE, also returns the matching character positions.
 */
    static void
do_fuzzymatch(typval_T *argvars, typval_T *rettv, int retmatchpos)
{
    callback_T	cb;
    char_u	*key = NULL;
    int		ret;
    int		matchseq = FALSE;
    long	max_matches = 0;

    if (in_vim9script()
	    && (check_for_list_arg(argvars, 0) == FAIL
		|| check_for_string_arg(argvars, 1) == FAIL
		|| check_for_opt_dict_arg(argvars, 2) == FAIL))
	return;

    CLEAR_POINTER(&cb);

    // validate and get the arguments
    if (argvars[0].v_type != VAR_LIST || argvars[0].vval.v_list == NULL)
    {
	semsg(_(e_argument_of_str_must_be_list),
			     retmatchpos ? "matchfuzzypos()" : "matchfuzzy()");
	return;
    }
    if (argvars[1].v_type != VAR_STRING
	    || argvars[1].vval.v_string == NULL)
    {
	semsg(_(e_invalid_argument_str), tv_get_string(&argvars[1]));
	return;
    }

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	dict_T		*d;
	dictitem_T	*di;

	if (check_for_nonnull_dict_arg(argvars, 2) == FAIL)
	    return;

	// To search a dict, either a callback function or a key can be
	// specified.
	d = argvars[2].vval.v_dict;
	if ((di = dict_find(d, (char_u *)"key", -1)) != NULL)
	{
	    if (di->di_tv.v_type != VAR_STRING
		    || di->di_tv.vval.v_string == NULL
		    || *di->di_tv.vval.v_string == NUL)
	    {
		semsg(_(e_invalid_value_for_argument_str_str), "key",
						    tv_get_string(&di->di_tv));
		return;
	    }
	    key = tv_get_string(&di->di_tv);
	}
	else if ((di = dict_find(d, (char_u *)"text_cb", -1)) != NULL)
	{
	    cb = get_callback(&di->di_tv);
	    if (cb.cb_name == NULL)
	    {
		semsg(_(e_invalid_value_for_argument_str), "text_cb");
		return;
	    }
	}

	if ((di = dict_find(d, (char_u *)"limit", -1)) != NULL)
	{
	    if (di->di_tv.v_type != VAR_NUMBER)
	    {
		semsg(_(e_invalid_value_for_argument_str), "limit");
		return;
	    }
	    max_matches = (long)tv_get_number_chk(&di->di_tv, NULL);
	}

	if (dict_has_key(d, "matchseq"))
	    matchseq = TRUE;
    }

    // get the fuzzy matches
    ret = rettv_list_alloc(rettv);
    if (ret == FAIL)
	goto done;
    if (retmatchpos)
    {
	list_T	*l;

	// For matchfuzzypos(), a list with three items are returned. First
	// item is a list of matching strings, the second item is a list of
	// lists with matching positions within each string and the third item
	// is the list of scores of the matches.
	l = list_alloc();
	if (l == NULL)
	    goto done;
	if (list_append_list(rettv->vval.v_list, l) == FAIL)
	{
	    list_free(l);
	    goto done;
	}
	l = list_alloc();
	if (l == NULL)
	    goto done;
	if (list_append_list(rettv->vval.v_list, l) == FAIL)
	{
	    list_free(l);
	    goto done;
	}
	l = list_alloc();
	if (l == NULL)
	    goto done;
	if (list_append_list(rettv->vval.v_list, l) == FAIL)
	{
	    list_free(l);
	    goto done;
	}
    }

    fuzzy_match_in_list(argvars[0].vval.v_list, tv_get_string(&argvars[1]),
	    matchseq, key, &cb, retmatchpos, rettv->vval.v_list, max_matches);

done:
    free_callback(&cb);
}

/*
 * "matchfuzzy()" function
 */
    void
f_matchfuzzy(typval_T *argvars, typval_T *rettv)
{
    do_fuzzymatch(argvars, rettv, FALSE);
}

/*
 * "matchfuzzypos()" function
 */
    void
f_matchfuzzypos(typval_T *argvars, typval_T *rettv)
{
    do_fuzzymatch(argvars, rettv, TRUE);
}
#endif

/*
 * Same as fuzzy_match_item_compare() except for use with a string match
 */
    static int
fuzzy_match_str_compare(const void *s1, const void *s2)
{
    int		v1 = ((fuzmatch_str_T *)s1)->score;
    int		v2 = ((fuzmatch_str_T *)s2)->score;
    int		idx1 = ((fuzmatch_str_T *)s1)->idx;
    int		idx2 = ((fuzmatch_str_T *)s2)->idx;

    if (v1 == v2)
	return idx1 == idx2 ? 0 : idx1 > idx2 ? 1 : -1;
    else
	return v1 > v2 ? -1 : 1;
}

/*
 * Sort fuzzy matches by score
 */
    static void
fuzzy_match_str_sort(fuzmatch_str_T *fm, int sz)
{
    // Sort the list by the descending order of the match score
    qsort((void *)fm, (size_t)sz, sizeof(fuzmatch_str_T),
	    fuzzy_match_str_compare);
}

/*
 * Same as fuzzy_match_item_compare() except for use with a function name
 * string match. <SNR> functions should be sorted to the end.
 */
    static int
fuzzy_match_func_compare(const void *s1, const void *s2)
{
    int		v1 = ((fuzmatch_str_T *)s1)->score;
    int		v2 = ((fuzmatch_str_T *)s2)->score;
    int		idx1 = ((fuzmatch_str_T *)s1)->idx;
    int		idx2 = ((fuzmatch_str_T *)s2)->idx;
    char_u	*str1 = ((fuzmatch_str_T *)s1)->str;
    char_u	*str2 = ((fuzmatch_str_T *)s2)->str;

    if (*str1 != '<' && *str2 == '<')
	return -1;
    if (*str1 == '<' && *str2 != '<')
	return 1;
    if (v1 == v2)
	return idx1 == idx2 ? 0 : idx1 > idx2 ? 1 : -1;
    else
	return v1 > v2 ? -1 : 1;
}

/*
 * Sort fuzzy matches of function names by score.
 * <SNR> functions should be sorted to the end.
 */
    static void
fuzzy_match_func_sort(fuzmatch_str_T *fm, int sz)
{
    // Sort the list by the descending order of the match score
    qsort((void *)fm, (size_t)sz, sizeof(fuzmatch_str_T),
		fuzzy_match_func_compare);
}

/*
 * Fuzzy match 'pat' in 'str'. Returns 0 if there is no match. Otherwise,
 * returns the match score.
 */
    int
fuzzy_match_str(char_u *str, char_u *pat)
{
    int		score = FUZZY_SCORE_NONE;
    int_u	matchpos[FUZZY_MATCH_MAX_LEN];

    if (str == NULL || pat == NULL)
	return score;

    fuzzy_match(str, pat, TRUE, &score, matchpos,
				sizeof(matchpos) / sizeof(matchpos[0]));

    return score;
}

/*
 * Fuzzy match the position of string 'pat' in string 'str'.
 * Returns a dynamic array of matching positions. If there is no match,
 * returns NULL.
 */
    garray_T *
fuzzy_match_str_with_pos(char_u *str UNUSED, char_u *pat UNUSED)
{
#ifdef FEAT_SEARCH_EXTRA
    int		    score = FUZZY_SCORE_NONE;
    garray_T	    *match_positions = NULL;
    int_u	    matches[FUZZY_MATCH_MAX_LEN];
    int		    j = 0;

    if (str == NULL || pat == NULL)
	return NULL;

    match_positions = ALLOC_ONE(garray_T);
    if (match_positions == NULL)
	return NULL;
    ga_init2(match_positions, sizeof(int_u), 10);

    if (!fuzzy_match(str, pat, FALSE, &score, matches, FUZZY_MATCH_MAX_LEN)
	    || score == FUZZY_SCORE_NONE)
    {
	ga_clear(match_positions);
	vim_free(match_positions);
	return NULL;
    }

    for (char_u *p = pat; *p != NUL; MB_PTR_ADV(p))
    {
	if (!VIM_ISWHITE(PTR2CHAR(p)))
	{
	    ga_grow(match_positions, 1);
	    ((int_u *)match_positions->ga_data)[match_positions->ga_len] =
								    matches[j];
	    match_positions->ga_len++;
	    j++;
	}
    }

    return match_positions;
#else
    return NULL;
#endif
}

/*
 * This function splits the line pointed to by `*ptr` into words and performs
 * a fuzzy match for the pattern `pat` on each word. It iterates through the
 * line, moving `*ptr` to the start of each word during the process.
 *
 * If a match is found:
 * - `*ptr` points to the start of the matched word.
 * - `*len` is set to the length of the matched word.
 * - `*score` contains the match score.
 *
 * If no match is found, `*ptr` is updated to the end of the line.
 */
    int
fuzzy_match_str_in_line(
    char_u	**ptr,
    char_u	*pat,
    int		*len,
    pos_T	*current_pos,
    int		*score)
{
    char_u	*str = *ptr;
    char_u	*strBegin = str;
    char_u	*end = NULL;
    char_u	*start = NULL;
    int		found = FALSE;
    char	save_end;
    char_u	*line_end = NULL;

    if (str == NULL || pat == NULL)
	return found;
    line_end = find_line_end(str);

    while (str < line_end)
    {
	// Skip non-word characters
	start = find_word_start(str);
	if (*start == NUL)
	    break;
	end = find_word_end(start);

	// Extract the word from start to end
	save_end = *end;
	*end = NUL;

	// Perform fuzzy match
	*score = fuzzy_match_str(start, pat);
	*end = save_end;

	if (*score != FUZZY_SCORE_NONE)
	{
	    *len = (int)(end - start);
	    found = TRUE;
	    *ptr = start;
	    if (current_pos)
		current_pos->col += (int)(end - strBegin);
	    break;
	}

	// Move to the end of the current word for the next iteration
	str = end;
	// Ensure we continue searching after the current word
	while (*str != NUL && !vim_iswordp(str))
	    MB_PTR_ADV(str);
    }

    if (!found)
	*ptr = line_end;

    return found;
}

/*
 * Search for the next fuzzy match in the specified buffer.
 * This function attempts to find the next occurrence of the given pattern
 * in the buffer, starting from the current position. It handles line wrapping
 * and direction of search.
 *
 * Return TRUE if a match is found, otherwise FALSE.
 */
    int
search_for_fuzzy_match(
    buf_T	*buf,
    pos_T	*pos,
    char_u	*pattern,
    int		dir,
    pos_T	*start_pos,
    int		*len,
    char_u	**ptr,
    int		*score)
{
    pos_T	current_pos = *pos;
    pos_T	circly_end;
    int		found_new_match = FALSE;
    int		looped_around = FALSE;
    int		whole_line = ctrl_x_mode_whole_line();

    if (buf == curbuf)
	circly_end = *start_pos;
    else
    {
	circly_end.lnum = buf->b_ml.ml_line_count;
	circly_end.col = 0;
	circly_end.coladd = 0;
    }

    if (whole_line && start_pos->lnum != pos->lnum)
	current_pos.lnum += dir;

    do
    {

	// Check if looped around and back to start position
	if (looped_around && EQUAL_POS(current_pos, circly_end))
	    break;

	// Ensure current_pos is valid
	if (current_pos.lnum >= 1 && current_pos.lnum <= buf->b_ml.ml_line_count)
	{
	    // Get the current line buffer
	    *ptr = ml_get_buf(buf, current_pos.lnum, FALSE);
	    if (!whole_line)
		*ptr += current_pos.col;

	    // If ptr is end of line is reached, move to next line
	    // or previous line based on direction
	    if (*ptr != NULL && **ptr != NUL)
	    {
		if (!whole_line)
		{
		    // Try to find a fuzzy match in the current line starting
		    // from current position
		    found_new_match = fuzzy_match_str_in_line(ptr, pattern,
						    len, &current_pos, score);
		    if (found_new_match)
		    {
			*pos = current_pos;
			break;
		    }
		    else if (looped_around && current_pos.lnum == circly_end.lnum)
			break;
		}
		else
		{
		    if (fuzzy_match_str(*ptr, pattern) != FUZZY_SCORE_NONE)
		    {
			found_new_match = TRUE;
			*pos = current_pos;
			*len = (int)ml_get_buf_len(buf, current_pos.lnum);
			break;
		    }
		}
	    }
	}

	// Move to the next line or previous line based on direction
	if (dir == FORWARD)
	{
	    if (++current_pos.lnum > buf->b_ml.ml_line_count)
	    {
		if (p_ws)
		{
		    current_pos.lnum = 1;
		    looped_around = TRUE;
		}
		else
		    break;
	    }
	}
	else
	{
	    if (--current_pos.lnum < 1)
	    {
		if (p_ws)
		{
		    current_pos.lnum = buf->b_ml.ml_line_count;
		    looped_around = TRUE;
		}
		else
		    break;

	    }
	}
	current_pos.col = 0;
    } while (TRUE);

    return found_new_match;
}

/*
 * Free an array of fuzzy string matches "fuzmatch[count]".
 */
    void
fuzmatch_str_free(fuzmatch_str_T *fuzmatch, int count)
{
    if (fuzmatch == NULL)
	return;

    for (int i = 0; i < count; ++i)
	vim_free(fuzmatch[i].str);
    vim_free(fuzmatch);
}

/*
 * Copy a list of fuzzy matches into a string list after sorting the matches by
 * the fuzzy score. Frees the memory allocated for 'fuzmatch'.
 * Returns OK on success and FAIL on memory allocation failure.
 */
    int
fuzzymatches_to_strmatches(
	fuzmatch_str_T	*fuzmatch,
	char_u		***matches,
	int		count,
	int		funcsort)
{
    if (count <= 0)
	goto theend;

    *matches = ALLOC_MULT(char_u *, count);
    if (*matches == NULL)
    {
	fuzmatch_str_free(fuzmatch, count);
	return FAIL;
    }

    // Sort the list by the descending order of the match score
    if (funcsort)
	fuzzy_match_func_sort((void *)fuzmatch, (size_t)count);
    else
	fuzzy_match_str_sort((void *)fuzmatch, (size_t)count);

    for (int i = 0; i < count; i++)
	(*matches)[i] = fuzmatch[i].str;

theend:
    vim_free(fuzmatch);
    return OK;
}

/*
 * Fuzzy match algorithm ported from https://github.com/jhawthorn/fzy.
 * This implementation extends the original by supporting multibyte characters.
 */

#define MATCH_MAX_LEN FUZZY_MATCH_MAX_LEN

#define SCORE_GAP_LEADING -0.005
#define SCORE_GAP_TRAILING -0.005
#define SCORE_GAP_INNER -0.01
#define SCORE_MATCH_CONSECUTIVE 1.0
#define SCORE_MATCH_SLASH 0.9
#define SCORE_MATCH_WORD 0.8
#define SCORE_MATCH_CAPITAL 0.7
#define SCORE_MATCH_DOT 0.6

    static int
has_match(char_u *needle, char_u *haystack)
{
    if (!needle || !haystack || !*needle)
	return FAIL;

    char_u *n_ptr = needle;
    char_u *h_ptr = haystack;

    while (*n_ptr)
    {
	int n_char = mb_ptr2char(n_ptr);
	int found = FALSE;

	while (*h_ptr)
	{
	    int h_char = mb_ptr2char(h_ptr);
	    if (h_char == n_char || h_char == MB_TOUPPER(n_char))
	    {
		found = TRUE;
		h_ptr += mb_ptr2len(h_ptr);
		break;
	    }
	    h_ptr += mb_ptr2len(h_ptr);
	}

	if (!found)
	    return FAIL;

	n_ptr += mb_ptr2len(n_ptr);
    }

    return OK;
}

typedef struct match_struct
{
    int needle_len;
    int haystack_len;
    int lower_needle[MATCH_MAX_LEN];     // stores codepoints
    int lower_haystack[MATCH_MAX_LEN];   // stores codepoints
    score_t match_bonus[MATCH_MAX_LEN];
} match_struct;

#define IS_WORD_SEP(c) ((c) == '-' || (c) == '_' || (c) == ' ')
#define IS_PATH_SEP(c) ((c) == '/')
#define IS_DOT(c)      ((c) == '.')

    static score_t
compute_bonus_codepoint(int last_c, int c)
{
    if (ASCII_ISALNUM(c) || vim_iswordc(c))
    {
	if (IS_PATH_SEP(last_c))
	    return SCORE_MATCH_SLASH;
	if (IS_WORD_SEP(last_c))
	    return SCORE_MATCH_WORD;
	if (IS_DOT(last_c))
	    return SCORE_MATCH_DOT;
	if (MB_ISUPPER(c) && MB_ISLOWER(last_c))
	    return SCORE_MATCH_CAPITAL;
    }
    return 0;
}

    static void
setup_match_struct(match_struct *match, char_u *needle, char_u *haystack)
{
    int i = 0;
    char_u *p = needle;
    while (*p != NUL && i < MATCH_MAX_LEN)
    {
	int c = mb_ptr2char(p);
	match->lower_needle[i++] = MB_TOLOWER(c);
	MB_PTR_ADV(p);
    }
    match->needle_len = i;

    i = 0;
    p = haystack;
    int prev_c = '/';
    while (*p != NUL && i < MATCH_MAX_LEN)
    {
	int c = mb_ptr2char(p);
	match->lower_haystack[i] = MB_TOLOWER(c);
	match->match_bonus[i] = compute_bonus_codepoint(prev_c, c);
	prev_c = c;
	MB_PTR_ADV(p);
	i++;
    }
    match->haystack_len = i;
}

    static inline void
match_row(const match_struct *match, int row, score_t *curr_D,
	score_t *curr_M, const score_t *last_D, const score_t *last_M)
{
    int n = match->needle_len;
    int m = match->haystack_len;
    int i = row;

    const int *lower_needle = match->lower_needle;
    const int *lower_haystack = match->lower_haystack;
    const score_t *match_bonus = match->match_bonus;

    score_t prev_score = SCORE_MIN;
    score_t gap_score = i == n - 1 ? SCORE_GAP_TRAILING : SCORE_GAP_INNER;

    // These will not be used with this value, but not all compilers see it
    score_t prev_M = SCORE_MIN, prev_D = SCORE_MIN;

    for (int j = 0; j < m; j++)
    {
	if (lower_needle[i] == lower_haystack[j])
	{
	    score_t score = SCORE_MIN;
	    if (!i)
	    {
		score = (j * SCORE_GAP_LEADING) + match_bonus[j];
	    }
	    else if (j)
	    { /* i > 0 && j > 0*/
		score = MAX(
			prev_M + match_bonus[j],
			// consecutive match, doesn't stack with match_bonus
			prev_D + SCORE_MATCH_CONSECUTIVE);
	    }
	    prev_D = last_D[j];
	    prev_M = last_M[j];
	    curr_D[j] = score;
	    curr_M[j] = prev_score = MAX(score, prev_score + gap_score);
	}
	else
	{
	    prev_D = last_D[j];
	    prev_M = last_M[j];
	    curr_D[j] = SCORE_MIN;
	    curr_M[j] = prev_score = prev_score + gap_score;
	}
    }
}

    static score_t
match_positions(char_u *needle, char_u *haystack, int_u *positions)
{
    if (!needle || !haystack || !*needle)
	return SCORE_MIN;

    match_struct match;
    setup_match_struct(&match, needle, haystack);

    int n = match.needle_len;
    int m = match.haystack_len;

    if (m > MATCH_MAX_LEN || n > m)
    {
	// Unreasonably large candidate: return no score
	// If it is a valid match it will still be returned, it will
	// just be ranked below any reasonably sized candidates
	return SCORE_MIN;
    }
    else if (n == m)
    {
	// Since this method can only be called with a haystack which
	// matches needle. If the lengths of the strings are equal the
	// strings themselves must also be equal (ignoring case).
	if (positions)
	{
	    for (int i = 0; i < n; i++)
		positions[i] = i;
	}
	return SCORE_MAX;
    }

    // ensure n * MATCH_MAX_LEN * 2 won't overflow
    if ((size_t)n > (SIZE_MAX / sizeof(score_t)) / MATCH_MAX_LEN / 2)
	return SCORE_MIN;

    // Allocate for both D and M matrices in one contiguous block
    score_t *block = (score_t*)alloc(sizeof(score_t) * MATCH_MAX_LEN * n * 2);
    if (!block)
	return SCORE_MIN;

    // D[][] Stores the best score for this position ending with a match.
    // M[][] Stores the best possible score at this position.
    score_t (*D)[MATCH_MAX_LEN] = (score_t(*)[MATCH_MAX_LEN])block;
    score_t (*M)[MATCH_MAX_LEN] = (score_t(*)[MATCH_MAX_LEN])(block
							+ MATCH_MAX_LEN * n);

    match_row(&match, 0, D[0], M[0], D[0], M[0]);
    for (int i = 1; i < n; i++)
	match_row(&match, i, D[i], M[i], D[i - 1], M[i - 1]);

    // backtrace to find the positions of optimal matching
    if (positions)
    {
	int match_required = 0;
	for (int i = n - 1, j = m - 1; i >= 0; i--)
	{
	    for (; j >= 0; j--)
	    {
		// There may be multiple paths which result in
		// the optimal weight.
		//
		// For simplicity, we will pick the first one
		// we encounter, the latest in the candidate
		// string.
		if (D[i][j] != SCORE_MIN &&
			(match_required || D[i][j] == M[i][j]))
		{
		    // If this score was determined using
		    // SCORE_MATCH_CONSECUTIVE, the
		    // previous character MUST be a match
		    match_required = i && j &&
			M[i][j] == D[i - 1][j - 1] + SCORE_MATCH_CONSECUTIVE;
		    positions[i] = j--;
		    break;
		}
	    }
	}
    }

    score_t result = M[n - 1][m - 1];
    vim_free(block);
    return result;
}
