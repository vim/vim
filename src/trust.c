/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * trust.c: Directory trust management.
 *
 * Provides a mechanism for users to approve directories before allowing
 * potentially unsafe operations like modeline processing.  Trust decisions
 * can be permanent (saved to ~/.vim/trust.json) or session-only.
 */

#include "vim.h"

#if defined(FEAT_EVAL)

// Session-only trusted directories (cleared on exit).
// Each entry is a typval_T dict with "path" and permission keys.
static garray_T trust_session_dirs = {0, 0, sizeof(char_u *), 10, NULL};

/*
 * Get the path to the trust.json file.
 * Returns an allocated string or NULL on failure.
 */
    static char_u *
trust_get_filepath(void)
{
    int		dofree = FALSE;
    char_u	*vimdir;
    char_u	*filepath;

    vimdir = vim_getenv((char_u *)"MYVIMDIR", &dofree);
    if (vimdir == NULL)
	return NULL;

    filepath = concat_fnames(vimdir, (char_u *)"trust.json", TRUE);
    if (dofree)
	vim_free(vimdir);
    return filepath;
}

/*
 * Read trust.json and return the parsed list as a typval_T.
 * Returns OK if successful, FAIL otherwise.
 * The caller must clear "rettv" when done.
 */
    static int
trust_load(typval_T *rettv)
{
    char_u	*filepath;
    char_u	*buf = NULL;
    FILE	*fp;
    long	len;
    js_read_T	reader;

    rettv->v_type = VAR_LIST;
    rettv->vval.v_list = NULL;

    filepath = trust_get_filepath();
    if (filepath == NULL)
	return FAIL;

    fp = mch_fopen((char *)filepath, "r");
    vim_free(filepath);
    if (fp == NULL)
	return FAIL;

    // Read entire file
    fseek(fp, 0L, SEEK_END);
    len = ftell(fp);
    fseek(fp, 0L, SEEK_SET);
    if (len <= 0)
    {
	fclose(fp);
	return FAIL;
    }

    buf = alloc((size_t)len + 1);
    if (buf == NULL)
    {
	fclose(fp);
	return FAIL;
    }

    if ((long)fread(buf, 1, (size_t)len, fp) != len)
    {
	vim_free(buf);
	fclose(fp);
	return FAIL;
    }
    buf[len] = NUL;
    fclose(fp);

    // Parse JSON
    reader.js_buf = buf;
    reader.js_fill = NULL;
    reader.js_used = 0;
    if (json_decode(&reader, rettv, 0) != OK || rettv->v_type != VAR_LIST)
    {
	clear_tv(rettv);
	rettv->v_type = VAR_LIST;
	rettv->vval.v_list = NULL;
	vim_free(buf);
	return FAIL;
    }

    vim_free(buf);
    return OK;
}

/*
 * Save trust data to trust.json.
 * "tv" should be a list of dicts.
 */
    static int
trust_save(typval_T *tv)
{
    char_u	*filepath;
    char_u	*json_str;
    FILE	*fp;
    int		dofree = FALSE;
    char_u	*vimdir;

    filepath = trust_get_filepath();
    if (filepath == NULL)
	return FAIL;

    // Ensure the directory exists
    vimdir = vim_getenv((char_u *)"MYVIMDIR", &dofree);
    if (vimdir != NULL)
    {
	if (mch_isdir(vimdir) == FALSE)
	    vim_mkdir_emsg(vimdir, 0755);
	if (dofree)
	    vim_free(vimdir);
    }

    json_str = json_encode(tv, 0);
    if (json_str == NULL)
    {
	vim_free(filepath);
	return FAIL;
    }

    fp = mch_fopen((char *)filepath, "w");
    vim_free(filepath);
    if (fp == NULL)
    {
	vim_free(json_str);
	return FAIL;
    }

    fputs((char *)json_str, fp);
    fputs("\n", fp);
    fclose(fp);
    vim_free(json_str);
    return OK;
}

/*
 * Check if "dir" is a parent (or equal) of "path".
 * Both should be full paths.
 */
    static int
is_path_under_dir(char_u *path, char_u *dir)
{
    size_t dirlen = STRLEN(dir);

    // Remove trailing path separator from dir for comparison
    while (dirlen > 1 && after_pathsep(dir, dir + dirlen))
	--dirlen;

    if (STRNCMP(path, dir, dirlen) != 0)
	return FALSE;

    // path must be exactly dir, or have a path separator after the match
    if (path[dirlen] == NUL || vim_ispathsep(path[dirlen]))
	return TRUE;

    return FALSE;
}

/*
 * Check if directory "dir" is trusted for "permission" in the permanent
 * trust.json file.  Parent directory inheritance applies.
 * Returns TRUE if trusted.
 */
    static int
trust_check_permanent(char_u *dir, char_u *permission)
{
    typval_T	tv;
    list_T	*l;
    listitem_T	*li;

    if (trust_load(&tv) != OK)
	return FALSE;

    l = tv.vval.v_list;
    if (l == NULL)
    {
	clear_tv(&tv);
	return FALSE;
    }

    FOR_ALL_LIST_ITEMS(l, li)
    {
	dictitem_T  *di_path;
	dictitem_T  *di_perm;

	if (li->li_tv.v_type != VAR_DICT || li->li_tv.vval.v_dict == NULL)
	    continue;

	di_path = dict_find(li->li_tv.vval.v_dict, (char_u *)"path", -1);
	if (di_path == NULL || di_path->di_tv.v_type != VAR_STRING)
	    continue;

	di_perm = dict_find(li->li_tv.vval.v_dict, permission, -1);
	if (di_perm == NULL)
	    continue;

	// Check if the permission is true
	if (di_perm->di_tv.v_type == VAR_BOOL
		&& di_perm->di_tv.vval.v_number == VVAL_TRUE)
	{
	    if (is_path_under_dir(dir, di_path->di_tv.vval.v_string))
	    {
		clear_tv(&tv);
		return TRUE;
	    }
	}
	else if (di_perm->di_tv.v_type == VAR_NUMBER
		&& di_perm->di_tv.vval.v_number != 0)
	{
	    if (is_path_under_dir(dir, di_path->di_tv.vval.v_string))
	    {
		clear_tv(&tv);
		return TRUE;
	    }
	}
    }

    clear_tv(&tv);
    return FALSE;
}

/*
 * Check if directory "dir" is trusted for "permission" in the session-only
 * list.  Parent directory inheritance applies.
 * Returns TRUE if trusted.
 */
    static int
trust_check_session(char_u *dir, char_u *permission UNUSED)
{
    int	    i;

    for (i = 0; i < trust_session_dirs.ga_len; ++i)
    {
	char_u *trusted_dir = ((char_u **)trust_session_dirs.ga_data)[i];
	if (is_path_under_dir(dir, trusted_dir))
	    return TRUE;
    }
    return FALSE;
}

/*
 * Add "dir" to the session-only trust list.
 */
    static void
trust_add_session(char_u *dir)
{
    char_u  *copy;

    copy = vim_strsave(dir);
    if (copy == NULL)
	return;

    if (ga_grow(&trust_session_dirs, 1) == OK)
    {
	((char_u **)trust_session_dirs.ga_data)[trust_session_dirs.ga_len] = copy;
	++trust_session_dirs.ga_len;
    }
    else
	vim_free(copy);
}

/*
 * Add "dir" with "permission" to the permanent trust.json file.
 */
    static void
trust_add_permanent(char_u *dir, char_u *permission)
{
    typval_T	tv;
    list_T	*l;
    listitem_T	*li;
    int		found = FALSE;

    if (trust_load(&tv) != OK || tv.vval.v_list == NULL)
    {
	// Create a new list
	clear_tv(&tv);
	if (rettv_list_alloc(&tv) == FAIL)
	    return;
    }

    l = tv.vval.v_list;

    // Check if there's already an entry for this directory
    FOR_ALL_LIST_ITEMS(l, li)
    {
	dictitem_T  *di_path;

	if (li->li_tv.v_type != VAR_DICT || li->li_tv.vval.v_dict == NULL)
	    continue;

	di_path = dict_find(li->li_tv.vval.v_dict, (char_u *)"path", -1);
	if (di_path != NULL && di_path->di_tv.v_type == VAR_STRING
		&& STRCMP(di_path->di_tv.vval.v_string, dir) == 0)
	{
	    // Update existing entry
	    dict_add_bool(li->li_tv.vval.v_dict, (char *)permission, VVAL_TRUE);
	    found = TRUE;
	    break;
	}
    }

    if (!found)
    {
	// Add new entry
	dict_T	*d = dict_alloc();

	if (d != NULL)
	{
	    dict_add_string(d, "path", dir);
	    dict_add_bool(d, (char *)permission, VVAL_TRUE);
	    list_append_dict(l, d);
	}
    }

    trust_save(&tv);
    clear_tv(&tv);
}

/*
 * Check if directory "dir" is trusted for "permission".
 * Checks both permanent and session trust.
 * Returns TRUE if trusted.
 */
    int
trust_check_dir(char_u *dir, char_u *permission)
{
    if (trust_check_session(dir, permission))
	return TRUE;
    if (trust_check_permanent(dir, permission))
	return TRUE;
    return FALSE;
}

/*
 * Show a trust prompt for directory "dir" and permission "permission".
 * Returns TRUE if the user chose to trust the directory (either permanently
 * or for the session).
 */
    int
trust_prompt(char_u *dir, char_u *permission)
{
    char_u	buf[IOSIZE];
    int		answer;

    vim_snprintf((char *)buf, IOSIZE,
	    _("The file contains a modeline.\n"
	      "Trust directory \"%s\" for \"%s\"?"),
	    dir, permission);

    answer = do_dialog(VIM_QUESTION,
	    (char_u *)_("Trust Directory"),
	    buf,
	    (char_u *)_("&Trust permanently\n&Session only\n&No"),
	    3, NULL, FALSE);

    switch (answer)
    {
	case 1:	// Trust permanently
	    trust_add_permanent(dir, permission);
	    return TRUE;
	case 2:	// Session only
	    trust_add_session(dir);
	    return TRUE;
	default: // No or cancelled
	    return FALSE;
    }
}

/*
 * Check if a single line looks like a modeline.
 * Returns TRUE if a modeline pattern is found.
 */
    static int
line_has_modeline(char_u *s)
{
    int	    prev = -1;

    for (; *s != NUL; ++s)
    {
	if (prev == -1 || vim_isspace(prev))
	{
	    if ((prev != -1 && STRNCMP(s, "ex:", (size_t)3) == 0)
		    || STRNCMP(s, "vi:", (size_t)3) == 0)
		return TRUE;
	    // Accept both "vim" and "Vim".
	    if ((s[0] == 'v' || s[0] == 'V') && s[1] == 'i' && s[2] == 'm')
	    {
		char_u *e;

		if (s[3] == '<' || s[3] == '=' || s[3] == '>')
		    e = s + 4;
		else
		    e = s + 3;
		(void)getdigits(&e);
		if (*e == ':')
		    return TRUE;
	    }
	}
	prev = *s;
    }
    return FALSE;
}

/*
 * Check if buffer "buf" contains a modeline in the first or last 'modelines'
 * lines.  Only checks for the presence of the pattern, does not apply it.
 * Returns TRUE if a modeline is found.
 */
    int
has_modeline(buf_T *buf)
{
    linenr_T	lnum;
    int		nmlines = (int)p_mls;

    if (nmlines == 0)
	return FALSE;

    // Check first N lines
    for (lnum = 1; lnum <= buf->b_ml.ml_line_count && lnum <= nmlines; ++lnum)
	if (line_has_modeline(ml_get_buf(buf, lnum, FALSE)))
	    return TRUE;

    // Check last N lines
    for (lnum = buf->b_ml.ml_line_count; lnum > 0
	    && lnum > nmlines
	    && lnum > buf->b_ml.ml_line_count - nmlines; --lnum)
	if (line_has_modeline(ml_get_buf(buf, lnum, FALSE)))
	    return TRUE;

    return FALSE;
}

/*
 * Get the directory of the current buffer.
 * Returns an allocated string or NULL.
 */
    char_u *
trust_get_buf_dir(buf_T *buf)
{
    char_u  *dir;

    if (buf->b_ffname == NULL)
	return NULL;

    dir = vim_strsave(buf->b_ffname);
    if (dir == NULL)
	return NULL;

    // Remove the filename, keeping only the directory
    *gettail(dir) = NUL;

    // Remove trailing path separator (but keep root "/")
    if (STRLEN(dir) > 1 && after_pathsep(dir, dir + STRLEN(dir)))
    {
	size_t len = STRLEN(dir);
	dir[len - 1] = NUL;
    }

    return dir;
}

/*
 * Free the session trust list.
 */
    void
trust_clear_session(void)
{
    int i;

    for (i = 0; i < trust_session_dirs.ga_len; ++i)
	vim_free(((char_u **)trust_session_dirs.ga_data)[i]);
    ga_clear(&trust_session_dirs);
}

#endif // FEAT_EVAL
