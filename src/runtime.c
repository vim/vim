/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * runtime.c: functions for dealing with the runtime directories/files
 */

#include "vim.h"

/*
 * ":runtime [what] {name}"
 */
    void
ex_runtime(exarg_T *eap)
{
    char_u  *arg = eap->arg;
    char_u  *p = skiptowhite(arg);
    int	    len = (int)(p - arg);
    int	    flags = eap->forceit ? DIP_ALL : 0;

    if (STRNCMP(arg, "START", len) == 0)
    {
	flags += DIP_START + DIP_NORTP;
	arg = skipwhite(arg + len);
    }
    else if (STRNCMP(arg, "OPT", len) == 0)
    {
	flags += DIP_OPT + DIP_NORTP;
	arg = skipwhite(arg + len);
    }
    else if (STRNCMP(arg, "PACK", len) == 0)
    {
	flags += DIP_START + DIP_OPT + DIP_NORTP;
	arg = skipwhite(arg + len);
    }
    else if (STRNCMP(arg, "ALL", len) == 0)
    {
	flags += DIP_START + DIP_OPT;
	arg = skipwhite(arg + len);
    }

    source_runtime(arg, flags);
}

    static void
source_callback(char_u *fname, void *cookie UNUSED)
{
    (void)do_source(fname, FALSE, DOSO_NONE);
}

/*
 * Find the file "name" in all directories in "path" and invoke
 * "callback(fname, cookie)".
 * "name" can contain wildcards.
 * When "flags" has DIP_ALL: source all files, otherwise only the first one.
 * When "flags" has DIP_DIR: find directories instead of files.
 * When "flags" has DIP_ERR: give an error message if there is no match.
 *
 * return FAIL when no file could be sourced, OK otherwise.
 */
    int
do_in_path(
    char_u	*path,
    char_u	*name,
    int		flags,
    void	(*callback)(char_u *fname, void *ck),
    void	*cookie)
{
    char_u	*rtp;
    char_u	*np;
    char_u	*buf;
    char_u	*rtp_copy;
    char_u	*tail;
    int		num_files;
    char_u	**files;
    int		i;
    int		did_one = FALSE;
#ifdef AMIGA
    struct Process	*proc = (struct Process *)FindTask(0L);
    APTR		save_winptr = proc->pr_WindowPtr;

    // Avoid a requester here for a volume that doesn't exist.
    proc->pr_WindowPtr = (APTR)-1L;
#endif

    // Make a copy of 'runtimepath'.  Invoking the callback may change the
    // value.
    rtp_copy = vim_strsave(path);
    buf = alloc(MAXPATHL);
    if (buf != NULL && rtp_copy != NULL)
    {
	if (p_verbose > 1 && name != NULL)
	{
	    verbose_enter();
	    smsg(_("Searching for \"%s\" in \"%s\""),
						 (char *)name, (char *)path);
	    verbose_leave();
	}

	// Loop over all entries in 'runtimepath'.
	rtp = rtp_copy;
	while (*rtp != NUL && ((flags & DIP_ALL) || !did_one))
	{
	    size_t buflen;

	    // Copy the path from 'runtimepath' to buf[].
	    copy_option_part(&rtp, buf, MAXPATHL, ",");
	    buflen = STRLEN(buf);

	    // Skip after or non-after directories.
	    if (flags & (DIP_NOAFTER | DIP_AFTER))
	    {
		int is_after = buflen >= 5
				     && STRCMP(buf + buflen - 5, "after") == 0;

		if ((is_after && (flags & DIP_NOAFTER))
			|| (!is_after && (flags & DIP_AFTER)))
		    continue;
	    }

	    if (name == NULL)
	    {
		(*callback)(buf, (void *) &cookie);
		if (!did_one)
		    did_one = (cookie == NULL);
	    }
	    else if (buflen + STRLEN(name) + 2 < MAXPATHL)
	    {
		add_pathsep(buf);
		tail = buf + STRLEN(buf);

		// Loop over all patterns in "name"
		np = name;
		while (*np != NUL && ((flags & DIP_ALL) || !did_one))
		{
		    // Append the pattern from "name" to buf[].
		    copy_option_part(&np, tail, (int)(MAXPATHL - (tail - buf)),
								       "\t ");

		    if (p_verbose > 2)
		    {
			verbose_enter();
			smsg(_("Searching for \"%s\""), buf);
			verbose_leave();
		    }

		    // Expand wildcards, invoke the callback for each match.
		    if (gen_expand_wildcards(1, &buf, &num_files, &files,
				  (flags & DIP_DIR) ? EW_DIR : EW_FILE) == OK)
		    {
			for (i = 0; i < num_files; ++i)
			{
			    (*callback)(files[i], cookie);
			    did_one = TRUE;
			    if (!(flags & DIP_ALL))
				break;
			}
			FreeWild(num_files, files);
		    }
		}
	    }
	}
    }
    vim_free(buf);
    vim_free(rtp_copy);
    if (!did_one && name != NULL)
    {
	char *basepath = path == p_rtp ? "runtimepath" : "packpath";

	if (flags & DIP_ERR)
	    semsg(_(e_dirnotf), basepath, name);
	else if (p_verbose > 0)
	{
	    verbose_enter();
	    smsg(_("not found in '%s': \"%s\""), basepath, name);
	    verbose_leave();
	}
    }

#ifdef AMIGA
    proc->pr_WindowPtr = save_winptr;
#endif

    return did_one ? OK : FAIL;
}

/*
 * Find "name" in "path".  When found, invoke the callback function for
 * it: callback(fname, "cookie")
 * When "flags" has DIP_ALL repeat for all matches, otherwise only the first
 * one is used.
 * Returns OK when at least one match found, FAIL otherwise.
 *
 * If "name" is NULL calls callback for each entry in "path". Cookie is
 * passed by reference in this case, setting it to NULL indicates that callback
 * has done its job.
 */
    static int
do_in_path_and_pp(
    char_u	*path,
    char_u	*name,
    int		flags,
    void	(*callback)(char_u *fname, void *ck),
    void	*cookie)
{
    int		done = FAIL;
    char_u	*s;
    int		len;
    char	*start_dir = "pack/*/start/*/%s";
    char	*opt_dir = "pack/*/opt/*/%s";

    if ((flags & DIP_NORTP) == 0)
	done = do_in_path(path, name, flags, callback, cookie);

    if ((done == FAIL || (flags & DIP_ALL)) && (flags & DIP_START))
    {
	len = (int)(STRLEN(start_dir) + STRLEN(name));
	s = alloc(len);
	if (s == NULL)
	    return FAIL;
	vim_snprintf((char *)s, len, start_dir, name);
	done = do_in_path(p_pp, s, flags, callback, cookie);
	vim_free(s);
    }

    if ((done == FAIL || (flags & DIP_ALL)) && (flags & DIP_OPT))
    {
	len = (int)(STRLEN(opt_dir) + STRLEN(name));
	s = alloc(len);
	if (s == NULL)
	    return FAIL;
	vim_snprintf((char *)s, len, opt_dir, name);
	done = do_in_path(p_pp, s, flags, callback, cookie);
	vim_free(s);
    }

    return done;
}

/*
 * Just like do_in_path_and_pp(), using 'runtimepath' for "path".
 */
    int
do_in_runtimepath(
    char_u	*name,
    int		flags,
    void	(*callback)(char_u *fname, void *ck),
    void	*cookie)
{
    return do_in_path_and_pp(p_rtp, name, flags, callback, cookie);
}

/*
 * Source the file "name" from all directories in 'runtimepath'.
 * "name" can contain wildcards.
 * When "flags" has DIP_ALL: source all files, otherwise only the first one.
 *
 * return FAIL when no file could be sourced, OK otherwise.
 */
    int
source_runtime(char_u *name, int flags)
{
    return source_in_path(p_rtp, name, flags);
}

/*
 * Just like source_runtime(), but use "path" instead of 'runtimepath'.
 */
    int
source_in_path(char_u *path, char_u *name, int flags)
{
    return do_in_path_and_pp(path, name, flags, source_callback, NULL);
}


#if defined(FEAT_EVAL) || defined(PROTO)

/*
 * Expand wildcards in "pat" and invoke do_source() for each match.
 */
    static void
source_all_matches(char_u *pat)
{
    int	    num_files;
    char_u  **files;
    int	    i;

    if (gen_expand_wildcards(1, &pat, &num_files, &files, EW_FILE) == OK)
    {
	for (i = 0; i < num_files; ++i)
	    (void)do_source(files[i], FALSE, DOSO_NONE);
	FreeWild(num_files, files);
    }
}

/*
 * Add the package directory to 'runtimepath'.
 */
    static int
add_pack_dir_to_rtp(char_u *fname)
{
    char_u  *p4, *p3, *p2, *p1, *p;
    char_u  *entry;
    char_u  *insp = NULL;
    int	    c;
    char_u  *new_rtp;
    int	    keep;
    size_t  oldlen;
    size_t  addlen;
    size_t  new_rtp_len;
    char_u  *afterdir = NULL;
    size_t  afterlen = 0;
    char_u  *after_insp = NULL;
    char_u  *ffname = NULL;
    size_t  fname_len;
    char_u  *buf = NULL;
    char_u  *rtp_ffname;
    int	    match;
    int	    retval = FAIL;

    p4 = p3 = p2 = p1 = get_past_head(fname);
    for (p = p1; *p; MB_PTR_ADV(p))
	if (vim_ispathsep_nocolon(*p))
	{
	    p4 = p3; p3 = p2; p2 = p1; p1 = p;
	}

    // now we have:
    // rtp/pack/name/start/name
    //    p4   p3   p2    p1
    //
    // find the part up to "pack" in 'runtimepath'
    c = *++p4; // append pathsep in order to expand symlink
    *p4 = NUL;
    ffname = fix_fname(fname);
    *p4 = c;
    if (ffname == NULL)
	return FAIL;

    // Find "ffname" in "p_rtp", ignoring '/' vs '\' differences.
    // Also stop at the first "after" directory.
    fname_len = STRLEN(ffname);
    buf = alloc(MAXPATHL);
    if (buf == NULL)
	goto theend;
    for (entry = p_rtp; *entry != NUL; )
    {
	char_u *cur_entry = entry;

	copy_option_part(&entry, buf, MAXPATHL, ",");
	if (insp == NULL)
	{
	    add_pathsep(buf);
	    rtp_ffname = fix_fname(buf);
	    if (rtp_ffname == NULL)
		goto theend;
	    match = vim_fnamencmp(rtp_ffname, ffname, fname_len) == 0;
	    vim_free(rtp_ffname);
	    if (match)
		// Insert "ffname" after this entry (and comma).
		insp = entry;
	}

	if ((p = (char_u *)strstr((char *)buf, "after")) != NULL
		&& p > buf
		&& vim_ispathsep(p[-1])
		&& (vim_ispathsep(p[5]) || p[5] == NUL || p[5] == ','))
	{
	    if (insp == NULL)
		// Did not find "ffname" before the first "after" directory,
		// insert it before this entry.
		insp = cur_entry;
	    after_insp = cur_entry;
	    break;
	}
    }

    if (insp == NULL)
	// Both "fname" and "after" not found, append at the end.
	insp = p_rtp + STRLEN(p_rtp);

    // check if rtp/pack/name/start/name/after exists
    afterdir = concat_fnames(fname, (char_u *)"after", TRUE);
    if (afterdir != NULL && mch_isdir(afterdir))
	afterlen = STRLEN(afterdir) + 1; // add one for comma

    oldlen = STRLEN(p_rtp);
    addlen = STRLEN(fname) + 1; // add one for comma
    new_rtp = alloc(oldlen + addlen + afterlen + 1); // add one for NUL
    if (new_rtp == NULL)
	goto theend;

    // We now have 'rtp' parts: {keep}{keep_after}{rest}.
    // Create new_rtp, first: {keep},{fname}
    keep = (int)(insp - p_rtp);
    mch_memmove(new_rtp, p_rtp, keep);
    new_rtp_len = keep;
    if (*insp == NUL)
	new_rtp[new_rtp_len++] = ',';  // add comma before
    mch_memmove(new_rtp + new_rtp_len, fname, addlen - 1);
    new_rtp_len += addlen - 1;
    if (*insp != NUL)
	new_rtp[new_rtp_len++] = ',';  // add comma after

    if (afterlen > 0 && after_insp != NULL)
    {
	int keep_after = (int)(after_insp - p_rtp);

	// Add to new_rtp: {keep},{fname}{keep_after},{afterdir}
	mch_memmove(new_rtp + new_rtp_len, p_rtp + keep,
							keep_after - keep);
	new_rtp_len += keep_after - keep;
	mch_memmove(new_rtp + new_rtp_len, afterdir, afterlen - 1);
	new_rtp_len += afterlen - 1;
	new_rtp[new_rtp_len++] = ',';
	keep = keep_after;
    }

    if (p_rtp[keep] != NUL)
	// Append rest: {keep},{fname}{keep_after},{afterdir}{rest}
	mch_memmove(new_rtp + new_rtp_len, p_rtp + keep, oldlen - keep + 1);
    else
	new_rtp[new_rtp_len] = NUL;

    if (afterlen > 0 && after_insp == NULL)
    {
	// Append afterdir when "after" was not found:
	// {keep},{fname}{rest},{afterdir}
	STRCAT(new_rtp, ",");
	STRCAT(new_rtp, afterdir);
    }

    set_option_value((char_u *)"rtp", 0L, new_rtp, 0);
    vim_free(new_rtp);
    retval = OK;

theend:
    vim_free(buf);
    vim_free(ffname);
    vim_free(afterdir);
    return retval;
}

/*
 * Load scripts in "plugin" and "ftdetect" directories of the package.
 */
    static int
load_pack_plugin(char_u *fname)
{
    static char *plugpat = "%s/plugin/**/*.vim";
    static char *ftpat = "%s/ftdetect/*.vim";
    int		len;
    char_u	*ffname = fix_fname(fname);
    char_u	*pat = NULL;
    int		retval = FAIL;

    if (ffname == NULL)
	return FAIL;
    len = (int)STRLEN(ffname) + (int)STRLEN(ftpat);
    pat = alloc(len);
    if (pat == NULL)
	goto theend;
    vim_snprintf((char *)pat, len, plugpat, ffname);
    source_all_matches(pat);

    {
	char_u *cmd = vim_strsave((char_u *)"g:did_load_filetypes");

	// If runtime/filetype.vim wasn't loaded yet, the scripts will be
	// found when it loads.
	if (cmd != NULL && eval_to_number(cmd) > 0)
	{
	    do_cmdline_cmd((char_u *)"augroup filetypedetect");
	    vim_snprintf((char *)pat, len, ftpat, ffname);
	    source_all_matches(pat);
	    do_cmdline_cmd((char_u *)"augroup END");
	}
	vim_free(cmd);
    }
    vim_free(pat);
    retval = OK;

theend:
    vim_free(ffname);
    return retval;
}

// used for "cookie" of add_pack_plugin()
static int APP_ADD_DIR;
static int APP_LOAD;
static int APP_BOTH;

    static void
add_pack_plugin(char_u *fname, void *cookie)
{
    if (cookie != &APP_LOAD)
    {
	char_u	*buf = alloc(MAXPATHL);
	char_u	*p;
	int	found = FALSE;

	if (buf == NULL)
	    return;
	p = p_rtp;
	while (*p != NUL)
	{
	    copy_option_part(&p, buf, MAXPATHL, ",");
	    if (pathcmp((char *)buf, (char *)fname, -1) == 0)
	    {
		found = TRUE;
		break;
	    }
	}
	vim_free(buf);
	if (!found)
	    // directory is not yet in 'runtimepath', add it
	    if (add_pack_dir_to_rtp(fname) == FAIL)
		return;
    }

    if (cookie != &APP_ADD_DIR)
	load_pack_plugin(fname);
}

/*
 * Add all packages in the "start" directory to 'runtimepath'.
 */
    void
add_pack_start_dirs(void)
{
    do_in_path(p_pp, (char_u *)"pack/*/start/*", DIP_ALL + DIP_DIR,
					       add_pack_plugin, &APP_ADD_DIR);
}

/*
 * Load plugins from all packages in the "start" directory.
 */
    void
load_start_packages(void)
{
    did_source_packages = TRUE;
    do_in_path(p_pp, (char_u *)"pack/*/start/*", DIP_ALL + DIP_DIR,
						  add_pack_plugin, &APP_LOAD);
}

/*
 * ":packloadall"
 * Find plugins in the package directories and source them.
 */
    void
ex_packloadall(exarg_T *eap)
{
    if (!did_source_packages || eap->forceit)
    {
	// First do a round to add all directories to 'runtimepath', then load
	// the plugins. This allows for plugins to use an autoload directory
	// of another plugin.
	add_pack_start_dirs();
	load_start_packages();
    }
}

/*
 * ":packadd[!] {name}"
 */
    void
ex_packadd(exarg_T *eap)
{
    static char *plugpat = "pack/*/%s/%s";
    int		len;
    char	*pat;
    int		round;
    int		res = OK;

    // Round 1: use "start", round 2: use "opt".
    for (round = 1; round <= 2; ++round)
    {
	// Only look under "start" when loading packages wasn't done yet.
	if (round == 1 && did_source_packages)
	    continue;

	len = (int)STRLEN(plugpat) + (int)STRLEN(eap->arg) + 5;
	pat = alloc(len);
	if (pat == NULL)
	    return;
	vim_snprintf(pat, len, plugpat, round == 1 ? "start" : "opt", eap->arg);
	// The first round don't give a "not found" error, in the second round
	// only when nothing was found in the first round.
	res = do_in_path(p_pp, (char_u *)pat,
		DIP_ALL + DIP_DIR + (round == 2 && res == FAIL ? DIP_ERR : 0),
		add_pack_plugin, eap->forceit ? &APP_ADD_DIR : &APP_BOTH);
	vim_free(pat);
    }
}
#endif

/*
 * Expand color scheme, compiler or filetype names.
 * Search from 'runtimepath':
 *   'runtimepath'/{dirnames}/{pat}.vim
 * When "flags" has DIP_START: search also from 'start' of 'packpath':
 *   'packpath'/pack/ * /start/ * /{dirnames}/{pat}.vim
 * When "flags" has DIP_OPT: search also from 'opt' of 'packpath':
 *   'packpath'/pack/ * /opt/ * /{dirnames}/{pat}.vim
 * "dirnames" is an array with one or more directory names.
 */
    int
ExpandRTDir(
    char_u	*pat,
    int		flags,
    int		*num_file,
    char_u	***file,
    char	*dirnames[])
{
    char_u	*s;
    char_u	*e;
    char_u	*match;
    garray_T	ga;
    int		i;
    int		pat_len;

    *num_file = 0;
    *file = NULL;
    pat_len = (int)STRLEN(pat);
    ga_init2(&ga, (int)sizeof(char *), 10);

    for (i = 0; dirnames[i] != NULL; ++i)
    {
	s = alloc(STRLEN(dirnames[i]) + pat_len + 7);
	if (s == NULL)
	{
	    ga_clear_strings(&ga);
	    return FAIL;
	}
	sprintf((char *)s, "%s/%s*.vim", dirnames[i], pat);
	globpath(p_rtp, s, &ga, 0);
	vim_free(s);
    }

    if (flags & DIP_START) {
	for (i = 0; dirnames[i] != NULL; ++i)
	{
	    s = alloc(STRLEN(dirnames[i]) + pat_len + 22);
	    if (s == NULL)
	    {
		ga_clear_strings(&ga);
		return FAIL;
	    }
	    sprintf((char *)s, "pack/*/start/*/%s/%s*.vim", dirnames[i], pat);
	    globpath(p_pp, s, &ga, 0);
	    vim_free(s);
	}
    }

    if (flags & DIP_OPT) {
	for (i = 0; dirnames[i] != NULL; ++i)
	{
	    s = alloc(STRLEN(dirnames[i]) + pat_len + 20);
	    if (s == NULL)
	    {
		ga_clear_strings(&ga);
		return FAIL;
	    }
	    sprintf((char *)s, "pack/*/opt/*/%s/%s*.vim", dirnames[i], pat);
	    globpath(p_pp, s, &ga, 0);
	    vim_free(s);
	}
    }

    for (i = 0; i < ga.ga_len; ++i)
    {
	match = ((char_u **)ga.ga_data)[i];
	s = match;
	e = s + STRLEN(s);
	if (e - 4 > s && STRNICMP(e - 4, ".vim", 4) == 0)
	{
	    e -= 4;
	    for (s = e; s > match; MB_PTR_BACK(match, s))
		if (s < match || vim_ispathsep(*s))
		    break;
	    ++s;
	    *e = NUL;
	    mch_memmove(match, s, e - s + 1);
	}
    }

    if (ga.ga_len == 0)
	return FAIL;

    // Sort and remove duplicates which can happen when specifying multiple
    // directories in dirnames.
    remove_duplicates(&ga);

    *file = ga.ga_data;
    *num_file = ga.ga_len;
    return OK;
}

/*
 * Expand loadplugin names:
 * 'packpath'/pack/ * /opt/{pat}
 */
    int
ExpandPackAddDir(
    char_u	*pat,
    int		*num_file,
    char_u	***file)
{
    char_u	*s;
    char_u	*e;
    char_u	*match;
    garray_T	ga;
    int		i;
    int		pat_len;

    *num_file = 0;
    *file = NULL;
    pat_len = (int)STRLEN(pat);
    ga_init2(&ga, (int)sizeof(char *), 10);

    s = alloc(pat_len + 26);
    if (s == NULL)
    {
	ga_clear_strings(&ga);
	return FAIL;
    }
    sprintf((char *)s, "pack/*/opt/%s*", pat);
    globpath(p_pp, s, &ga, 0);
    vim_free(s);

    for (i = 0; i < ga.ga_len; ++i)
    {
	match = ((char_u **)ga.ga_data)[i];
	s = gettail(match);
	e = s + STRLEN(s);
	mch_memmove(match, s, e - s + 1);
    }

    if (ga.ga_len == 0)
	return FAIL;

    // Sort and remove duplicates which can happen when specifying multiple
    // directories in dirnames.
    remove_duplicates(&ga);

    *file = ga.ga_data;
    *num_file = ga.ga_len;
    return OK;
}
