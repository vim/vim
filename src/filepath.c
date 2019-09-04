/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * filepath.c: dealing with file names ant paths.
 */

#include "vim.h"

#ifdef MSWIN
/*
 * Functions for ":8" filename modifier: get 8.3 version of a filename.
 */

/*
 * Get the short path (8.3) for the filename in "fnamep".
 * Only works for a valid file name.
 * When the path gets longer "fnamep" is changed and the allocated buffer
 * is put in "bufp".
 * *fnamelen is the length of "fnamep" and set to 0 for a nonexistent path.
 * Returns OK on success, FAIL on failure.
 */
    static int
get_short_pathname(char_u **fnamep, char_u **bufp, int *fnamelen)
{
    int		l, len;
    char_u	*newbuf;

    len = *fnamelen;
    l = GetShortPathName((LPSTR)*fnamep, (LPSTR)*fnamep, len);
    if (l > len - 1)
    {
	/* If that doesn't work (not enough space), then save the string
	 * and try again with a new buffer big enough. */
	newbuf = vim_strnsave(*fnamep, l);
	if (newbuf == NULL)
	    return FAIL;

	vim_free(*bufp);
	*fnamep = *bufp = newbuf;

	/* Really should always succeed, as the buffer is big enough. */
	l = GetShortPathName((LPSTR)*fnamep, (LPSTR)*fnamep, l+1);
    }

    *fnamelen = l;
    return OK;
}

/*
 * Get the short path (8.3) for the filename in "fname". The converted
 * path is returned in "bufp".
 *
 * Some of the directories specified in "fname" may not exist. This function
 * will shorten the existing directories at the beginning of the path and then
 * append the remaining non-existing path.
 *
 * fname - Pointer to the filename to shorten.  On return, contains the
 *	   pointer to the shortened pathname
 * bufp -  Pointer to an allocated buffer for the filename.
 * fnamelen - Length of the filename pointed to by fname
 *
 * Returns OK on success (or nothing done) and FAIL on failure (out of memory).
 */
    static int
shortpath_for_invalid_fname(
    char_u	**fname,
    char_u	**bufp,
    int		*fnamelen)
{
    char_u	*short_fname, *save_fname, *pbuf_unused;
    char_u	*endp, *save_endp;
    char_u	ch;
    int		old_len, len;
    int		new_len, sfx_len;
    int		retval = OK;

    /* Make a copy */
    old_len = *fnamelen;
    save_fname = vim_strnsave(*fname, old_len);
    pbuf_unused = NULL;
    short_fname = NULL;

    endp = save_fname + old_len - 1; /* Find the end of the copy */
    save_endp = endp;

    /*
     * Try shortening the supplied path till it succeeds by removing one
     * directory at a time from the tail of the path.
     */
    len = 0;
    for (;;)
    {
	/* go back one path-separator */
	while (endp > save_fname && !after_pathsep(save_fname, endp + 1))
	    --endp;
	if (endp <= save_fname)
	    break;		/* processed the complete path */

	/*
	 * Replace the path separator with a NUL and try to shorten the
	 * resulting path.
	 */
	ch = *endp;
	*endp = 0;
	short_fname = save_fname;
	len = (int)STRLEN(short_fname) + 1;
	if (get_short_pathname(&short_fname, &pbuf_unused, &len) == FAIL)
	{
	    retval = FAIL;
	    goto theend;
	}
	*endp = ch;	/* preserve the string */

	if (len > 0)
	    break;	/* successfully shortened the path */

	/* failed to shorten the path. Skip the path separator */
	--endp;
    }

    if (len > 0)
    {
	/*
	 * Succeeded in shortening the path. Now concatenate the shortened
	 * path with the remaining path at the tail.
	 */

	/* Compute the length of the new path. */
	sfx_len = (int)(save_endp - endp) + 1;
	new_len = len + sfx_len;

	*fnamelen = new_len;
	vim_free(*bufp);
	if (new_len > old_len)
	{
	    /* There is not enough space in the currently allocated string,
	     * copy it to a buffer big enough. */
	    *fname = *bufp = vim_strnsave(short_fname, new_len);
	    if (*fname == NULL)
	    {
		retval = FAIL;
		goto theend;
	    }
	}
	else
	{
	    /* Transfer short_fname to the main buffer (it's big enough),
	     * unless get_short_pathname() did its work in-place. */
	    *fname = *bufp = save_fname;
	    if (short_fname != save_fname)
		vim_strncpy(save_fname, short_fname, len);
	    save_fname = NULL;
	}

	/* concat the not-shortened part of the path */
	vim_strncpy(*fname + len, endp, sfx_len);
	(*fname)[new_len] = NUL;
    }

theend:
    vim_free(pbuf_unused);
    vim_free(save_fname);

    return retval;
}

/*
 * Get a pathname for a partial path.
 * Returns OK for success, FAIL for failure.
 */
    static int
shortpath_for_partial(
    char_u	**fnamep,
    char_u	**bufp,
    int		*fnamelen)
{
    int		sepcount, len, tflen;
    char_u	*p;
    char_u	*pbuf, *tfname;
    int		hasTilde;

    /* Count up the path separators from the RHS.. so we know which part
     * of the path to return. */
    sepcount = 0;
    for (p = *fnamep; p < *fnamep + *fnamelen; MB_PTR_ADV(p))
	if (vim_ispathsep(*p))
	    ++sepcount;

    /* Need full path first (use expand_env() to remove a "~/") */
    hasTilde = (**fnamep == '~');
    if (hasTilde)
	pbuf = tfname = expand_env_save(*fnamep);
    else
	pbuf = tfname = FullName_save(*fnamep, FALSE);

    len = tflen = (int)STRLEN(tfname);

    if (get_short_pathname(&tfname, &pbuf, &len) == FAIL)
	return FAIL;

    if (len == 0)
    {
	/* Don't have a valid filename, so shorten the rest of the
	 * path if we can. This CAN give us invalid 8.3 filenames, but
	 * there's not a lot of point in guessing what it might be.
	 */
	len = tflen;
	if (shortpath_for_invalid_fname(&tfname, &pbuf, &len) == FAIL)
	    return FAIL;
    }

    /* Count the paths backward to find the beginning of the desired string. */
    for (p = tfname + len - 1; p >= tfname; --p)
    {
	if (has_mbyte)
	    p -= mb_head_off(tfname, p);
	if (vim_ispathsep(*p))
	{
	    if (sepcount == 0 || (hasTilde && sepcount == 1))
		break;
	    else
		sepcount --;
	}
    }
    if (hasTilde)
    {
	--p;
	if (p >= tfname)
	    *p = '~';
	else
	    return FAIL;
    }
    else
	++p;

    /* Copy in the string - p indexes into tfname - allocated at pbuf */
    vim_free(*bufp);
    *fnamelen = (int)STRLEN(p);
    *bufp = pbuf;
    *fnamep = p;

    return OK;
}
#endif // MSWIN

/*
 * Adjust a filename, according to a string of modifiers.
 * *fnamep must be NUL terminated when called.  When returning, the length is
 * determined by *fnamelen.
 * Returns VALID_ flags or -1 for failure.
 * When there is an error, *fnamep is set to NULL.
 */
    int
modify_fname(
    char_u	*src,		// string with modifiers
    int		tilde_file,	// "~" is a file name, not $HOME
    int		*usedlen,	// characters after src that are used
    char_u	**fnamep,	// file name so far
    char_u	**bufp,		// buffer for allocated file name or NULL
    int		*fnamelen)	// length of fnamep
{
    int		valid = 0;
    char_u	*tail;
    char_u	*s, *p, *pbuf;
    char_u	dirname[MAXPATHL];
    int		c;
    int		has_fullname = 0;
#ifdef MSWIN
    char_u	*fname_start = *fnamep;
    int		has_shortname = 0;
#endif

repeat:
    /* ":p" - full path/file_name */
    if (src[*usedlen] == ':' && src[*usedlen + 1] == 'p')
    {
	has_fullname = 1;

	valid |= VALID_PATH;
	*usedlen += 2;

	/* Expand "~/path" for all systems and "~user/path" for Unix and VMS */
	if ((*fnamep)[0] == '~'
#if !defined(UNIX) && !(defined(VMS) && defined(USER_HOME))
		&& ((*fnamep)[1] == '/'
# ifdef BACKSLASH_IN_FILENAME
		    || (*fnamep)[1] == '\\'
# endif
		    || (*fnamep)[1] == NUL)
#endif
		&& !(tilde_file && (*fnamep)[1] == NUL)
	   )
	{
	    *fnamep = expand_env_save(*fnamep);
	    vim_free(*bufp);	/* free any allocated file name */
	    *bufp = *fnamep;
	    if (*fnamep == NULL)
		return -1;
	}

	/* When "/." or "/.." is used: force expansion to get rid of it. */
	for (p = *fnamep; *p != NUL; MB_PTR_ADV(p))
	{
	    if (vim_ispathsep(*p)
		    && p[1] == '.'
		    && (p[2] == NUL
			|| vim_ispathsep(p[2])
			|| (p[2] == '.'
			    && (p[3] == NUL || vim_ispathsep(p[3])))))
		break;
	}

	/* FullName_save() is slow, don't use it when not needed. */
	if (*p != NUL || !vim_isAbsName(*fnamep))
	{
	    *fnamep = FullName_save(*fnamep, *p != NUL);
	    vim_free(*bufp);	/* free any allocated file name */
	    *bufp = *fnamep;
	    if (*fnamep == NULL)
		return -1;
	}

#ifdef MSWIN
# if _WIN32_WINNT >= 0x0500
	if (vim_strchr(*fnamep, '~') != NULL)
	{
	    // Expand 8.3 filename to full path.  Needed to make sure the same
	    // file does not have two different names.
	    // Note: problem does not occur if _WIN32_WINNT < 0x0500.
	    WCHAR *wfname = enc_to_utf16(*fnamep, NULL);
	    WCHAR buf[_MAX_PATH];

	    if (wfname != NULL)
	    {
		if (GetLongPathNameW(wfname, buf, _MAX_PATH))
		{
		    char_u *p = utf16_to_enc(buf, NULL);

		    if (p != NULL)
		    {
			vim_free(*bufp);    // free any allocated file name
			*bufp = *fnamep = p;
		    }
		}
		vim_free(wfname);
	    }
	}
# endif
#endif
	/* Append a path separator to a directory. */
	if (mch_isdir(*fnamep))
	{
	    /* Make room for one or two extra characters. */
	    *fnamep = vim_strnsave(*fnamep, (int)STRLEN(*fnamep) + 2);
	    vim_free(*bufp);	/* free any allocated file name */
	    *bufp = *fnamep;
	    if (*fnamep == NULL)
		return -1;
	    add_pathsep(*fnamep);
	}
    }

    /* ":." - path relative to the current directory */
    /* ":~" - path relative to the home directory */
    /* ":8" - shortname path - postponed till after */
    while (src[*usedlen] == ':'
		  && ((c = src[*usedlen + 1]) == '.' || c == '~' || c == '8'))
    {
	*usedlen += 2;
	if (c == '8')
	{
#ifdef MSWIN
	    has_shortname = 1; /* Postpone this. */
#endif
	    continue;
	}
	pbuf = NULL;
	/* Need full path first (use expand_env() to remove a "~/") */
	if (!has_fullname)
	{
	    if (c == '.' && **fnamep == '~')
		p = pbuf = expand_env_save(*fnamep);
	    else
		p = pbuf = FullName_save(*fnamep, FALSE);
	}
	else
	    p = *fnamep;

	has_fullname = 0;

	if (p != NULL)
	{
	    if (c == '.')
	    {
		mch_dirname(dirname, MAXPATHL);
		s = shorten_fname(p, dirname);
		if (s != NULL)
		{
		    *fnamep = s;
		    if (pbuf != NULL)
		    {
			vim_free(*bufp);   /* free any allocated file name */
			*bufp = pbuf;
			pbuf = NULL;
		    }
		}
	    }
	    else
	    {
		home_replace(NULL, p, dirname, MAXPATHL, TRUE);
		/* Only replace it when it starts with '~' */
		if (*dirname == '~')
		{
		    s = vim_strsave(dirname);
		    if (s != NULL)
		    {
			*fnamep = s;
			vim_free(*bufp);
			*bufp = s;
		    }
		}
	    }
	    vim_free(pbuf);
	}
    }

    tail = gettail(*fnamep);
    *fnamelen = (int)STRLEN(*fnamep);

    /* ":h" - head, remove "/file_name", can be repeated  */
    /* Don't remove the first "/" or "c:\" */
    while (src[*usedlen] == ':' && src[*usedlen + 1] == 'h')
    {
	valid |= VALID_HEAD;
	*usedlen += 2;
	s = get_past_head(*fnamep);
	while (tail > s && after_pathsep(s, tail))
	    MB_PTR_BACK(*fnamep, tail);
	*fnamelen = (int)(tail - *fnamep);
#ifdef VMS
	if (*fnamelen > 0)
	    *fnamelen += 1; /* the path separator is part of the path */
#endif
	if (*fnamelen == 0)
	{
	    /* Result is empty.  Turn it into "." to make ":cd %:h" work. */
	    p = vim_strsave((char_u *)".");
	    if (p == NULL)
		return -1;
	    vim_free(*bufp);
	    *bufp = *fnamep = tail = p;
	    *fnamelen = 1;
	}
	else
	{
	    while (tail > s && !after_pathsep(s, tail))
		MB_PTR_BACK(*fnamep, tail);
	}
    }

    /* ":8" - shortname  */
    if (src[*usedlen] == ':' && src[*usedlen + 1] == '8')
    {
	*usedlen += 2;
#ifdef MSWIN
	has_shortname = 1;
#endif
    }

#ifdef MSWIN
    /*
     * Handle ":8" after we have done 'heads' and before we do 'tails'.
     */
    if (has_shortname)
    {
	/* Copy the string if it is shortened by :h and when it wasn't copied
	 * yet, because we are going to change it in place.  Avoids changing
	 * the buffer name for "%:8". */
	if (*fnamelen < (int)STRLEN(*fnamep) || *fnamep == fname_start)
	{
	    p = vim_strnsave(*fnamep, *fnamelen);
	    if (p == NULL)
		return -1;
	    vim_free(*bufp);
	    *bufp = *fnamep = p;
	}

	/* Split into two implementations - makes it easier.  First is where
	 * there isn't a full name already, second is where there is. */
	if (!has_fullname && !vim_isAbsName(*fnamep))
	{
	    if (shortpath_for_partial(fnamep, bufp, fnamelen) == FAIL)
		return -1;
	}
	else
	{
	    int		l = *fnamelen;

	    /* Simple case, already have the full-name.
	     * Nearly always shorter, so try first time. */
	    if (get_short_pathname(fnamep, bufp, &l) == FAIL)
		return -1;

	    if (l == 0)
	    {
		/* Couldn't find the filename, search the paths. */
		l = *fnamelen;
		if (shortpath_for_invalid_fname(fnamep, bufp, &l) == FAIL)
		    return -1;
	    }
	    *fnamelen = l;
	}
    }
#endif // MSWIN

    /* ":t" - tail, just the basename */
    if (src[*usedlen] == ':' && src[*usedlen + 1] == 't')
    {
	*usedlen += 2;
	*fnamelen -= (int)(tail - *fnamep);
	*fnamep = tail;
    }

    /* ":e" - extension, can be repeated */
    /* ":r" - root, without extension, can be repeated */
    while (src[*usedlen] == ':'
	    && (src[*usedlen + 1] == 'e' || src[*usedlen + 1] == 'r'))
    {
	/* find a '.' in the tail:
	 * - for second :e: before the current fname
	 * - otherwise: The last '.'
	 */
	if (src[*usedlen + 1] == 'e' && *fnamep > tail)
	    s = *fnamep - 2;
	else
	    s = *fnamep + *fnamelen - 1;
	for ( ; s > tail; --s)
	    if (s[0] == '.')
		break;
	if (src[*usedlen + 1] == 'e')		/* :e */
	{
	    if (s > tail)
	    {
		*fnamelen += (int)(*fnamep - (s + 1));
		*fnamep = s + 1;
#ifdef VMS
		/* cut version from the extension */
		s = *fnamep + *fnamelen - 1;
		for ( ; s > *fnamep; --s)
		    if (s[0] == ';')
			break;
		if (s > *fnamep)
		    *fnamelen = s - *fnamep;
#endif
	    }
	    else if (*fnamep <= tail)
		*fnamelen = 0;
	}
	else				/* :r */
	{
	    if (s > tail)	/* remove one extension */
		*fnamelen = (int)(s - *fnamep);
	}
	*usedlen += 2;
    }

    /* ":s?pat?foo?" - substitute */
    /* ":gs?pat?foo?" - global substitute */
    if (src[*usedlen] == ':'
	    && (src[*usedlen + 1] == 's'
		|| (src[*usedlen + 1] == 'g' && src[*usedlen + 2] == 's')))
    {
	char_u	    *str;
	char_u	    *pat;
	char_u	    *sub;
	int	    sep;
	char_u	    *flags;
	int	    didit = FALSE;

	flags = (char_u *)"";
	s = src + *usedlen + 2;
	if (src[*usedlen + 1] == 'g')
	{
	    flags = (char_u *)"g";
	    ++s;
	}

	sep = *s++;
	if (sep)
	{
	    /* find end of pattern */
	    p = vim_strchr(s, sep);
	    if (p != NULL)
	    {
		pat = vim_strnsave(s, (int)(p - s));
		if (pat != NULL)
		{
		    s = p + 1;
		    /* find end of substitution */
		    p = vim_strchr(s, sep);
		    if (p != NULL)
		    {
			sub = vim_strnsave(s, (int)(p - s));
			str = vim_strnsave(*fnamep, *fnamelen);
			if (sub != NULL && str != NULL)
			{
			    *usedlen = (int)(p + 1 - src);
			    s = do_string_sub(str, pat, sub, NULL, flags);
			    if (s != NULL)
			    {
				*fnamep = s;
				*fnamelen = (int)STRLEN(s);
				vim_free(*bufp);
				*bufp = s;
				didit = TRUE;
			    }
			}
			vim_free(sub);
			vim_free(str);
		    }
		    vim_free(pat);
		}
	    }
	    /* after using ":s", repeat all the modifiers */
	    if (didit)
		goto repeat;
	}
    }

    if (src[*usedlen] == ':' && src[*usedlen + 1] == 'S')
    {
	/* vim_strsave_shellescape() needs a NUL terminated string. */
	c = (*fnamep)[*fnamelen];
	if (c != NUL)
	    (*fnamep)[*fnamelen] = NUL;
	p = vim_strsave_shellescape(*fnamep, FALSE, FALSE);
	if (c != NUL)
	    (*fnamep)[*fnamelen] = c;
	if (p == NULL)
	    return -1;
	vim_free(*bufp);
	*bufp = *fnamep = p;
	*fnamelen = (int)STRLEN(p);
	*usedlen += 2;
    }

    return valid;
}

#if defined(FEAT_EVAL) || defined(PROTO)

/*
 * "chdir(dir)" function
 */
    void
f_chdir(typval_T *argvars, typval_T *rettv)
{
    char_u	*cwd;
    cdscope_T	scope = CDSCOPE_GLOBAL;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    if (argvars[0].v_type != VAR_STRING)
	return;

    // Return the current directory
    cwd = alloc(MAXPATHL);
    if (cwd != NULL)
    {
	if (mch_dirname(cwd, MAXPATHL) != FAIL)
	{
#ifdef BACKSLASH_IN_FILENAME
	    slash_adjust(cwd);
#endif
	    rettv->vval.v_string = vim_strsave(cwd);
	}
	vim_free(cwd);
    }

    if (curwin->w_localdir != NULL)
	scope = CDSCOPE_WINDOW;
    else if (curtab->tp_localdir != NULL)
	scope = CDSCOPE_TABPAGE;

    if (!changedir_func(argvars[0].vval.v_string, TRUE, scope))
	// Directory change failed
	VIM_CLEAR(rettv->vval.v_string);
}

/*
 * "delete()" function
 */
    void
f_delete(typval_T *argvars, typval_T *rettv)
{
    char_u	nbuf[NUMBUFLEN];
    char_u	*name;
    char_u	*flags;

    rettv->vval.v_number = -1;
    if (check_restricted() || check_secure())
	return;

    name = tv_get_string(&argvars[0]);
    if (name == NULL || *name == NUL)
    {
	emsg(_(e_invarg));
	return;
    }

    if (argvars[1].v_type != VAR_UNKNOWN)
	flags = tv_get_string_buf(&argvars[1], nbuf);
    else
	flags = (char_u *)"";

    if (*flags == NUL)
	/* delete a file */
	rettv->vval.v_number = mch_remove(name) == 0 ? 0 : -1;
    else if (STRCMP(flags, "d") == 0)
	/* delete an empty directory */
	rettv->vval.v_number = mch_rmdir(name) == 0 ? 0 : -1;
    else if (STRCMP(flags, "rf") == 0)
	/* delete a directory recursively */
	rettv->vval.v_number = delete_recursive(name);
    else
	semsg(_(e_invexpr2), flags);
}

/*
 * "executable()" function
 */
    void
f_executable(typval_T *argvars, typval_T *rettv)
{
    char_u *name = tv_get_string(&argvars[0]);

    /* Check in $PATH and also check directly if there is a directory name. */
    rettv->vval.v_number = mch_can_exe(name, NULL, TRUE);
}

/*
 * "exepath()" function
 */
    void
f_exepath(typval_T *argvars, typval_T *rettv)
{
    char_u *p = NULL;

    (void)mch_can_exe(tv_get_string(&argvars[0]), &p, TRUE);
    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = p;
}

/*
 * "filereadable()" function
 */
    void
f_filereadable(typval_T *argvars, typval_T *rettv)
{
    int		fd;
    char_u	*p;
    int		n;

#ifndef O_NONBLOCK
# define O_NONBLOCK 0
#endif
    p = tv_get_string(&argvars[0]);
    if (*p && !mch_isdir(p) && (fd = mch_open((char *)p,
					      O_RDONLY | O_NONBLOCK, 0)) >= 0)
    {
	n = TRUE;
	close(fd);
    }
    else
	n = FALSE;

    rettv->vval.v_number = n;
}

/*
 * Return 0 for not writable, 1 for writable file, 2 for a dir which we have
 * rights to write into.
 */
    void
f_filewritable(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = filewritable(tv_get_string(&argvars[0]));
}

    void
findfilendir(
    typval_T	*argvars UNUSED,
    typval_T	*rettv,
    int		find_what UNUSED)
{
#ifdef FEAT_SEARCHPATH
    char_u	*fname;
    char_u	*fresult = NULL;
    char_u	*path = *curbuf->b_p_path == NUL ? p_path : curbuf->b_p_path;
    char_u	*p;
    char_u	pathbuf[NUMBUFLEN];
    int		count = 1;
    int		first = TRUE;
    int		error = FALSE;
#endif

    rettv->vval.v_string = NULL;
    rettv->v_type = VAR_STRING;

#ifdef FEAT_SEARCHPATH
    fname = tv_get_string(&argvars[0]);

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	p = tv_get_string_buf_chk(&argvars[1], pathbuf);
	if (p == NULL)
	    error = TRUE;
	else
	{
	    if (*p != NUL)
		path = p;

	    if (argvars[2].v_type != VAR_UNKNOWN)
		count = (int)tv_get_number_chk(&argvars[2], &error);
	}
    }

    if (count < 0 && rettv_list_alloc(rettv) == FAIL)
	error = TRUE;

    if (*fname != NUL && !error)
    {
	do
	{
	    if (rettv->v_type == VAR_STRING || rettv->v_type == VAR_LIST)
		vim_free(fresult);
	    fresult = find_file_in_path_option(first ? fname : NULL,
					       first ? (int)STRLEN(fname) : 0,
					0, first, path,
					find_what,
					curbuf->b_ffname,
					find_what == FINDFILE_DIR
					    ? (char_u *)"" : curbuf->b_p_sua);
	    first = FALSE;

	    if (fresult != NULL && rettv->v_type == VAR_LIST)
		list_append_string(rettv->vval.v_list, fresult, -1);

	} while ((rettv->v_type == VAR_LIST || --count > 0) && fresult != NULL);
    }

    if (rettv->v_type == VAR_STRING)
	rettv->vval.v_string = fresult;
#endif
}

/*
 * "finddir({fname}[, {path}[, {count}]])" function
 */
    void
f_finddir(typval_T *argvars, typval_T *rettv)
{
    findfilendir(argvars, rettv, FINDFILE_DIR);
}

/*
 * "findfile({fname}[, {path}[, {count}]])" function
 */
    void
f_findfile(typval_T *argvars, typval_T *rettv)
{
    findfilendir(argvars, rettv, FINDFILE_FILE);
}

/*
 * "fnamemodify({fname}, {mods})" function
 */
    void
f_fnamemodify(typval_T *argvars, typval_T *rettv)
{
    char_u	*fname;
    char_u	*mods;
    int		usedlen = 0;
    int		len;
    char_u	*fbuf = NULL;
    char_u	buf[NUMBUFLEN];

    fname = tv_get_string_chk(&argvars[0]);
    mods = tv_get_string_buf_chk(&argvars[1], buf);
    if (fname == NULL || mods == NULL)
	fname = NULL;
    else
    {
	len = (int)STRLEN(fname);
	(void)modify_fname(mods, FALSE, &usedlen, &fname, &fbuf, &len);
    }

    rettv->v_type = VAR_STRING;
    if (fname == NULL)
	rettv->vval.v_string = NULL;
    else
	rettv->vval.v_string = vim_strnsave(fname, len);
    vim_free(fbuf);
}

/*
 * "getcwd()" function
 *
 * Return the current working directory of a window in a tab page.
 * First optional argument 'winnr' is the window number or -1 and the second
 * optional argument 'tabnr' is the tab page number.
 *
 * If no arguments are supplied, then return the directory of the current
 * window.
 * If only 'winnr' is specified and is not -1 or 0 then return the directory of
 * the specified window.
 * If 'winnr' is 0 then return the directory of the current window.
 * If both 'winnr and 'tabnr' are specified and 'winnr' is -1 then return the
 * directory of the specified tab page.  Otherwise return the directory of the
 * specified window in the specified tab page.
 * If the window or the tab page doesn't exist then return NULL.
 */
    void
f_getcwd(typval_T *argvars, typval_T *rettv)
{
    win_T	*wp = NULL;
    tabpage_T	*tp = NULL;
    char_u	*cwd;
    int		global = FALSE;

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = NULL;

    if (argvars[0].v_type == VAR_NUMBER
	    && argvars[0].vval.v_number == -1
	    && argvars[1].v_type == VAR_UNKNOWN)
	global = TRUE;
    else
	wp = find_tabwin(&argvars[0], &argvars[1], &tp);

    if (wp != NULL && wp->w_localdir != NULL)
	rettv->vval.v_string = vim_strsave(wp->w_localdir);
    else if (tp != NULL && tp->tp_localdir != NULL)
	rettv->vval.v_string = vim_strsave(tp->tp_localdir);
    else if (wp != NULL || tp != NULL || global)
    {
	if (globaldir != NULL)
	    rettv->vval.v_string = vim_strsave(globaldir);
	else
	{
	    cwd = alloc(MAXPATHL);
	    if (cwd != NULL)
	    {
		if (mch_dirname(cwd, MAXPATHL) != FAIL)
		    rettv->vval.v_string = vim_strsave(cwd);
		vim_free(cwd);
	    }
	}
    }
#ifdef BACKSLASH_IN_FILENAME
    if (rettv->vval.v_string != NULL)
	slash_adjust(rettv->vval.v_string);
#endif
}

/*
 * "getfperm({fname})" function
 */
    void
f_getfperm(typval_T *argvars, typval_T *rettv)
{
    char_u	*fname;
    stat_T	st;
    char_u	*perm = NULL;
    char_u	flags[] = "rwx";
    int		i;

    fname = tv_get_string(&argvars[0]);

    rettv->v_type = VAR_STRING;
    if (mch_stat((char *)fname, &st) >= 0)
    {
	perm = vim_strsave((char_u *)"---------");
	if (perm != NULL)
	{
	    for (i = 0; i < 9; i++)
	    {
		if (st.st_mode & (1 << (8 - i)))
		    perm[i] = flags[i % 3];
	    }
	}
    }
    rettv->vval.v_string = perm;
}

/*
 * "getfsize({fname})" function
 */
    void
f_getfsize(typval_T *argvars, typval_T *rettv)
{
    char_u	*fname;
    stat_T	st;

    fname = tv_get_string(&argvars[0]);

    rettv->v_type = VAR_NUMBER;

    if (mch_stat((char *)fname, &st) >= 0)
    {
	if (mch_isdir(fname))
	    rettv->vval.v_number = 0;
	else
	{
	    rettv->vval.v_number = (varnumber_T)st.st_size;

	    /* non-perfect check for overflow */
	    if ((off_T)rettv->vval.v_number != (off_T)st.st_size)
		rettv->vval.v_number = -2;
	}
    }
    else
	  rettv->vval.v_number = -1;
}

/*
 * "getftime({fname})" function
 */
    void
f_getftime(typval_T *argvars, typval_T *rettv)
{
    char_u	*fname;
    stat_T	st;

    fname = tv_get_string(&argvars[0]);

    if (mch_stat((char *)fname, &st) >= 0)
	rettv->vval.v_number = (varnumber_T)st.st_mtime;
    else
	rettv->vval.v_number = -1;
}

/*
 * "getftype({fname})" function
 */
    void
f_getftype(typval_T *argvars, typval_T *rettv)
{
    char_u	*fname;
    stat_T	st;
    char_u	*type = NULL;
    char	*t;

    fname = tv_get_string(&argvars[0]);

    rettv->v_type = VAR_STRING;
    if (mch_lstat((char *)fname, &st) >= 0)
    {
	if (S_ISREG(st.st_mode))
	    t = "file";
	else if (S_ISDIR(st.st_mode))
	    t = "dir";
	else if (S_ISLNK(st.st_mode))
	    t = "link";
	else if (S_ISBLK(st.st_mode))
	    t = "bdev";
	else if (S_ISCHR(st.st_mode))
	    t = "cdev";
	else if (S_ISFIFO(st.st_mode))
	    t = "fifo";
	else if (S_ISSOCK(st.st_mode))
	    t = "socket";
	else
	    t = "other";
	type = vim_strsave((char_u *)t);
    }
    rettv->vval.v_string = type;
}

/*
 * "glob()" function
 */
    void
f_glob(typval_T *argvars, typval_T *rettv)
{
    int		options = WILD_SILENT|WILD_USE_NL;
    expand_T	xpc;
    int		error = FALSE;

    /* When the optional second argument is non-zero, don't remove matches
     * for 'wildignore' and don't put matches for 'suffixes' at the end. */
    rettv->v_type = VAR_STRING;
    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	if (tv_get_number_chk(&argvars[1], &error))
	    options |= WILD_KEEP_ALL;
	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    if (tv_get_number_chk(&argvars[2], &error))
		rettv_list_set(rettv, NULL);
	    if (argvars[3].v_type != VAR_UNKNOWN
				    && tv_get_number_chk(&argvars[3], &error))
		options |= WILD_ALLLINKS;
	}
    }
    if (!error)
    {
	ExpandInit(&xpc);
	xpc.xp_context = EXPAND_FILES;
	if (p_wic)
	    options += WILD_ICASE;
	if (rettv->v_type == VAR_STRING)
	    rettv->vval.v_string = ExpandOne(&xpc, tv_get_string(&argvars[0]),
						     NULL, options, WILD_ALL);
	else if (rettv_list_alloc(rettv) != FAIL)
	{
	  int i;

	  ExpandOne(&xpc, tv_get_string(&argvars[0]),
						NULL, options, WILD_ALL_KEEP);
	  for (i = 0; i < xpc.xp_numfiles; i++)
	      list_append_string(rettv->vval.v_list, xpc.xp_files[i], -1);

	  ExpandCleanup(&xpc);
	}
    }
    else
	rettv->vval.v_string = NULL;
}

/*
 * "glob2regpat()" function
 */
    void
f_glob2regpat(typval_T *argvars, typval_T *rettv)
{
    char_u	*pat = tv_get_string_chk(&argvars[0]);

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = (pat == NULL)
			 ? NULL : file_pat_to_reg_pat(pat, NULL, NULL, FALSE);
}

/*
 * "globpath()" function
 */
    void
f_globpath(typval_T *argvars, typval_T *rettv)
{
    int		flags = WILD_IGNORE_COMPLETESLASH;
    char_u	buf1[NUMBUFLEN];
    char_u	*file = tv_get_string_buf_chk(&argvars[1], buf1);
    int		error = FALSE;
    garray_T	ga;
    int		i;

    // When the optional second argument is non-zero, don't remove matches
    // for 'wildignore' and don't put matches for 'suffixes' at the end.
    rettv->v_type = VAR_STRING;
    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	if (tv_get_number_chk(&argvars[2], &error))
	    flags |= WILD_KEEP_ALL;
	if (argvars[3].v_type != VAR_UNKNOWN)
	{
	    if (tv_get_number_chk(&argvars[3], &error))
		rettv_list_set(rettv, NULL);
	    if (argvars[4].v_type != VAR_UNKNOWN
				    && tv_get_number_chk(&argvars[4], &error))
		flags |= WILD_ALLLINKS;
	}
    }
    if (file != NULL && !error)
    {
	ga_init2(&ga, (int)sizeof(char_u *), 10);
	globpath(tv_get_string(&argvars[0]), file, &ga, flags);
	if (rettv->v_type == VAR_STRING)
	    rettv->vval.v_string = ga_concat_strings(&ga, "\n");
	else if (rettv_list_alloc(rettv) != FAIL)
	    for (i = 0; i < ga.ga_len; ++i)
		list_append_string(rettv->vval.v_list,
					    ((char_u **)(ga.ga_data))[i], -1);
	ga_clear_strings(&ga);
    }
    else
	rettv->vval.v_string = NULL;
}

/*
 * "isdirectory()" function
 */
    void
f_isdirectory(typval_T *argvars, typval_T *rettv)
{
    rettv->vval.v_number = mch_isdir(tv_get_string(&argvars[0]));
}

/*
 * Evaluate "expr" (= "context") for readdir().
 */
    static int
readdir_checkitem(void *context, char_u *name)
{
    typval_T	*expr = (typval_T *)context;
    typval_T	save_val;
    typval_T	rettv;
    typval_T	argv[2];
    int		retval = 0;
    int		error = FALSE;

    if (expr->v_type == VAR_UNKNOWN)
	return 1;

    prepare_vimvar(VV_VAL, &save_val);
    set_vim_var_string(VV_VAL, name, -1);
    argv[0].v_type = VAR_STRING;
    argv[0].vval.v_string = name;

    if (eval_expr_typval(expr, argv, 1, &rettv) == FAIL)
	goto theend;

    retval = tv_get_number_chk(&rettv, &error);
    if (error)
	retval = -1;
    clear_tv(&rettv);

theend:
    set_vim_var_string(VV_VAL, NULL, 0);
    restore_vimvar(VV_VAL, &save_val);
    return retval;
}

/*
 * Create the directory in which "dir" is located, and higher levels when
 * needed.
 * Return OK or FAIL.
 */
    static int
mkdir_recurse(char_u *dir, int prot)
{
    char_u	*p;
    char_u	*updir;
    int		r = FAIL;

    /* Get end of directory name in "dir".
     * We're done when it's "/" or "c:/". */
    p = gettail_sep(dir);
    if (p <= get_past_head(dir))
	return OK;

    /* If the directory exists we're done.  Otherwise: create it.*/
    updir = vim_strnsave(dir, (int)(p - dir));
    if (updir == NULL)
	return FAIL;
    if (mch_isdir(updir))
	r = OK;
    else if (mkdir_recurse(updir, prot) == OK)
	r = vim_mkdir_emsg(updir, prot);
    vim_free(updir);
    return r;
}

/*
 * "mkdir()" function
 */
    void
f_mkdir(typval_T *argvars, typval_T *rettv)
{
    char_u	*dir;
    char_u	buf[NUMBUFLEN];
    int		prot = 0755;

    rettv->vval.v_number = FAIL;
    if (check_restricted() || check_secure())
	return;

    dir = tv_get_string_buf(&argvars[0], buf);
    if (*dir == NUL)
	return;

    if (*gettail(dir) == NUL)
	/* remove trailing slashes */
	*gettail_sep(dir) = NUL;

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	if (argvars[2].v_type != VAR_UNKNOWN)
	{
	    prot = (int)tv_get_number_chk(&argvars[2], NULL);
	    if (prot == -1)
		return;
	}
	if (STRCMP(tv_get_string(&argvars[1]), "p") == 0)
	{
	    if (mch_isdir(dir))
	    {
		/* With the "p" flag it's OK if the dir already exists. */
		rettv->vval.v_number = OK;
		return;
	    }
	    mkdir_recurse(dir, prot);
	}
    }
    rettv->vval.v_number = vim_mkdir_emsg(dir, prot);
}

/*
 * "readdir()" function
 */
    void
f_readdir(typval_T *argvars, typval_T *rettv)
{
    typval_T	*expr;
    int		ret;
    char_u	*path;
    char_u	*p;
    garray_T	ga;
    int		i;

    if (rettv_list_alloc(rettv) == FAIL)
	return;
    path = tv_get_string(&argvars[0]);
    expr = &argvars[1];

    ret = readdir_core(&ga, path, (void *)expr, readdir_checkitem);
    if (ret == OK && rettv->vval.v_list != NULL && ga.ga_len > 0)
    {
	for (i = 0; i < ga.ga_len; i++)
	{
	    p = ((char_u **)ga.ga_data)[i];
	    list_append_string(rettv->vval.v_list, p, -1);
	}
    }
    ga_clear_strings(&ga);
}

/*
 * "readfile()" function
 */
    void
f_readfile(typval_T *argvars, typval_T *rettv)
{
    int		binary = FALSE;
    int		blob = FALSE;
    int		failed = FALSE;
    char_u	*fname;
    FILE	*fd;
    char_u	buf[(IOSIZE/256)*256];	/* rounded to avoid odd + 1 */
    int		io_size = sizeof(buf);
    int		readlen;		/* size of last fread() */
    char_u	*prev	 = NULL;	/* previously read bytes, if any */
    long	prevlen  = 0;		/* length of data in prev */
    long	prevsize = 0;		/* size of prev buffer */
    long	maxline  = MAXLNUM;
    long	cnt	 = 0;
    char_u	*p;			/* position in buf */
    char_u	*start;			/* start of current line */

    if (argvars[1].v_type != VAR_UNKNOWN)
    {
	if (STRCMP(tv_get_string(&argvars[1]), "b") == 0)
	    binary = TRUE;
	if (STRCMP(tv_get_string(&argvars[1]), "B") == 0)
	    blob = TRUE;

	if (argvars[2].v_type != VAR_UNKNOWN)
	    maxline = (long)tv_get_number(&argvars[2]);
    }

    if (blob)
    {
	if (rettv_blob_alloc(rettv) == FAIL)
	    return;
    }
    else
    {
	if (rettv_list_alloc(rettv) == FAIL)
	    return;
    }

    /* Always open the file in binary mode, library functions have a mind of
     * their own about CR-LF conversion. */
    fname = tv_get_string(&argvars[0]);
    if (*fname == NUL || (fd = mch_fopen((char *)fname, READBIN)) == NULL)
    {
	semsg(_(e_notopen), *fname == NUL ? (char_u *)_("<empty>") : fname);
	return;
    }

    if (blob)
    {
	if (read_blob(fd, rettv->vval.v_blob) == FAIL)
	{
	    emsg("cannot read file");
	    blob_free(rettv->vval.v_blob);
	}
	fclose(fd);
	return;
    }

    while (cnt < maxline || maxline < 0)
    {
	readlen = (int)fread(buf, 1, io_size, fd);

	/* This for loop processes what was read, but is also entered at end
	 * of file so that either:
	 * - an incomplete line gets written
	 * - a "binary" file gets an empty line at the end if it ends in a
	 *   newline.  */
	for (p = buf, start = buf;
		p < buf + readlen || (readlen <= 0 && (prevlen > 0 || binary));
		++p)
	{
	    if (*p == '\n' || readlen <= 0)
	    {
		listitem_T  *li;
		char_u	    *s	= NULL;
		long_u	    len = p - start;

		/* Finished a line.  Remove CRs before NL. */
		if (readlen > 0 && !binary)
		{
		    while (len > 0 && start[len - 1] == '\r')
			--len;
		    /* removal may cross back to the "prev" string */
		    if (len == 0)
			while (prevlen > 0 && prev[prevlen - 1] == '\r')
			    --prevlen;
		}
		if (prevlen == 0)
		    s = vim_strnsave(start, (int)len);
		else
		{
		    /* Change "prev" buffer to be the right size.  This way
		     * the bytes are only copied once, and very long lines are
		     * allocated only once.  */
		    if ((s = vim_realloc(prev, prevlen + len + 1)) != NULL)
		    {
			mch_memmove(s + prevlen, start, len);
			s[prevlen + len] = NUL;
			prev = NULL; /* the list will own the string */
			prevlen = prevsize = 0;
		    }
		}
		if (s == NULL)
		{
		    do_outofmem_msg((long_u) prevlen + len + 1);
		    failed = TRUE;
		    break;
		}

		if ((li = listitem_alloc()) == NULL)
		{
		    vim_free(s);
		    failed = TRUE;
		    break;
		}
		li->li_tv.v_type = VAR_STRING;
		li->li_tv.v_lock = 0;
		li->li_tv.vval.v_string = s;
		list_append(rettv->vval.v_list, li);

		start = p + 1; /* step over newline */
		if ((++cnt >= maxline && maxline >= 0) || readlen <= 0)
		    break;
	    }
	    else if (*p == NUL)
		*p = '\n';
	    /* Check for utf8 "bom"; U+FEFF is encoded as EF BB BF.  Do this
	     * when finding the BF and check the previous two bytes. */
	    else if (*p == 0xbf && enc_utf8 && !binary)
	    {
		/* Find the two bytes before the 0xbf.	If p is at buf, or buf
		 * + 1, these may be in the "prev" string. */
		char_u back1 = p >= buf + 1 ? p[-1]
				     : prevlen >= 1 ? prev[prevlen - 1] : NUL;
		char_u back2 = p >= buf + 2 ? p[-2]
			  : p == buf + 1 && prevlen >= 1 ? prev[prevlen - 1]
			  : prevlen >= 2 ? prev[prevlen - 2] : NUL;

		if (back2 == 0xef && back1 == 0xbb)
		{
		    char_u *dest = p - 2;

		    /* Usually a BOM is at the beginning of a file, and so at
		     * the beginning of a line; then we can just step over it.
		     */
		    if (start == dest)
			start = p + 1;
		    else
		    {
			/* have to shuffle buf to close gap */
			int adjust_prevlen = 0;

			if (dest < buf)
			{
			    adjust_prevlen = (int)(buf - dest); /* must be 1 or 2 */
			    dest = buf;
			}
			if (readlen > p - buf + 1)
			    mch_memmove(dest, p + 1, readlen - (p - buf) - 1);
			readlen -= 3 - adjust_prevlen;
			prevlen -= adjust_prevlen;
			p = dest - 1;
		    }
		}
	    }
	} /* for */

	if (failed || (cnt >= maxline && maxline >= 0) || readlen <= 0)
	    break;
	if (start < p)
	{
	    /* There's part of a line in buf, store it in "prev". */
	    if (p - start + prevlen >= prevsize)
	    {
		/* need bigger "prev" buffer */
		char_u *newprev;

		/* A common use case is ordinary text files and "prev" gets a
		 * fragment of a line, so the first allocation is made
		 * small, to avoid repeatedly 'allocing' large and
		 * 'reallocing' small. */
		if (prevsize == 0)
		    prevsize = (long)(p - start);
		else
		{
		    long grow50pc = (prevsize * 3) / 2;
		    long growmin  = (long)((p - start) * 2 + prevlen);
		    prevsize = grow50pc > growmin ? grow50pc : growmin;
		}
		newprev = vim_realloc(prev, prevsize);
		if (newprev == NULL)
		{
		    do_outofmem_msg((long_u)prevsize);
		    failed = TRUE;
		    break;
		}
		prev = newprev;
	    }
	    /* Add the line part to end of "prev". */
	    mch_memmove(prev + prevlen, start, p - start);
	    prevlen += (long)(p - start);
	}
    } /* while */

    /*
     * For a negative line count use only the lines at the end of the file,
     * free the rest.
     */
    if (!failed && maxline < 0)
	while (cnt > -maxline)
	{
	    listitem_remove(rettv->vval.v_list, rettv->vval.v_list->lv_first);
	    --cnt;
	}

    if (failed)
    {
	// an empty list is returned on error
	list_free(rettv->vval.v_list);
	rettv_list_alloc(rettv);
    }

    vim_free(prev);
    fclose(fd);
}

/*
 * "resolve()" function
 */
    void
f_resolve(typval_T *argvars, typval_T *rettv)
{
    char_u	*p;
#ifdef HAVE_READLINK
    char_u	*buf = NULL;
#endif

    p = tv_get_string(&argvars[0]);
#ifdef FEAT_SHORTCUT
    {
	char_u	*v = NULL;

	v = mch_resolve_path(p, TRUE);
	if (v != NULL)
	    rettv->vval.v_string = v;
	else
	    rettv->vval.v_string = vim_strsave(p);
    }
#else
# ifdef HAVE_READLINK
    {
	char_u	*cpy;
	int	len;
	char_u	*remain = NULL;
	char_u	*q;
	int	is_relative_to_current = FALSE;
	int	has_trailing_pathsep = FALSE;
	int	limit = 100;

	p = vim_strsave(p);

	if (p[0] == '.' && (vim_ispathsep(p[1])
				   || (p[1] == '.' && (vim_ispathsep(p[2])))))
	    is_relative_to_current = TRUE;

	len = STRLEN(p);
	if (len > 0 && after_pathsep(p, p + len))
	{
	    has_trailing_pathsep = TRUE;
	    p[len - 1] = NUL; /* the trailing slash breaks readlink() */
	}

	q = getnextcomp(p);
	if (*q != NUL)
	{
	    /* Separate the first path component in "p", and keep the
	     * remainder (beginning with the path separator). */
	    remain = vim_strsave(q - 1);
	    q[-1] = NUL;
	}

	buf = alloc(MAXPATHL + 1);
	if (buf == NULL)
	    goto fail;

	for (;;)
	{
	    for (;;)
	    {
		len = readlink((char *)p, (char *)buf, MAXPATHL);
		if (len <= 0)
		    break;
		buf[len] = NUL;

		if (limit-- == 0)
		{
		    vim_free(p);
		    vim_free(remain);
		    emsg(_("E655: Too many symbolic links (cycle?)"));
		    rettv->vval.v_string = NULL;
		    goto fail;
		}

		/* Ensure that the result will have a trailing path separator
		 * if the argument has one. */
		if (remain == NULL && has_trailing_pathsep)
		    add_pathsep(buf);

		/* Separate the first path component in the link value and
		 * concatenate the remainders. */
		q = getnextcomp(vim_ispathsep(*buf) ? buf + 1 : buf);
		if (*q != NUL)
		{
		    if (remain == NULL)
			remain = vim_strsave(q - 1);
		    else
		    {
			cpy = concat_str(q - 1, remain);
			if (cpy != NULL)
			{
			    vim_free(remain);
			    remain = cpy;
			}
		    }
		    q[-1] = NUL;
		}

		q = gettail(p);
		if (q > p && *q == NUL)
		{
		    /* Ignore trailing path separator. */
		    q[-1] = NUL;
		    q = gettail(p);
		}
		if (q > p && !mch_isFullName(buf))
		{
		    /* symlink is relative to directory of argument */
		    cpy = alloc(STRLEN(p) + STRLEN(buf) + 1);
		    if (cpy != NULL)
		    {
			STRCPY(cpy, p);
			STRCPY(gettail(cpy), buf);
			vim_free(p);
			p = cpy;
		    }
		}
		else
		{
		    vim_free(p);
		    p = vim_strsave(buf);
		}
	    }

	    if (remain == NULL)
		break;

	    /* Append the first path component of "remain" to "p". */
	    q = getnextcomp(remain + 1);
	    len = q - remain - (*q != NUL);
	    cpy = vim_strnsave(p, STRLEN(p) + len);
	    if (cpy != NULL)
	    {
		STRNCAT(cpy, remain, len);
		vim_free(p);
		p = cpy;
	    }
	    /* Shorten "remain". */
	    if (*q != NUL)
		STRMOVE(remain, q - 1);
	    else
		VIM_CLEAR(remain);
	}

	/* If the result is a relative path name, make it explicitly relative to
	 * the current directory if and only if the argument had this form. */
	if (!vim_ispathsep(*p))
	{
	    if (is_relative_to_current
		    && *p != NUL
		    && !(p[0] == '.'
			&& (p[1] == NUL
			    || vim_ispathsep(p[1])
			    || (p[1] == '.'
				&& (p[2] == NUL
				    || vim_ispathsep(p[2]))))))
	    {
		/* Prepend "./". */
		cpy = concat_str((char_u *)"./", p);
		if (cpy != NULL)
		{
		    vim_free(p);
		    p = cpy;
		}
	    }
	    else if (!is_relative_to_current)
	    {
		/* Strip leading "./". */
		q = p;
		while (q[0] == '.' && vim_ispathsep(q[1]))
		    q += 2;
		if (q > p)
		    STRMOVE(p, p + 2);
	    }
	}

	/* Ensure that the result will have no trailing path separator
	 * if the argument had none.  But keep "/" or "//". */
	if (!has_trailing_pathsep)
	{
	    q = p + STRLEN(p);
	    if (after_pathsep(p, q))
		*gettail_sep(p) = NUL;
	}

	rettv->vval.v_string = p;
    }
# else
    rettv->vval.v_string = vim_strsave(p);
# endif
#endif

    simplify_filename(rettv->vval.v_string);

#ifdef HAVE_READLINK
fail:
    vim_free(buf);
#endif
    rettv->v_type = VAR_STRING;
}

/*
 * "tempname()" function
 */
    void
f_tempname(typval_T *argvars UNUSED, typval_T *rettv)
{
    static int	x = 'A';

    rettv->v_type = VAR_STRING;
    rettv->vval.v_string = vim_tempname(x, FALSE);

    /* Advance 'x' to use A-Z and 0-9, so that there are at least 34 different
     * names.  Skip 'I' and 'O', they are used for shell redirection. */
    do
    {
	if (x == 'Z')
	    x = '0';
	else if (x == '9')
	    x = 'A';
	else
	{
#ifdef EBCDIC
	    if (x == 'I')
		x = 'J';
	    else if (x == 'R')
		x = 'S';
	    else
#endif
		++x;
	}
    } while (x == 'I' || x == 'O');
}

/*
 * "writefile()" function
 */
    void
f_writefile(typval_T *argvars, typval_T *rettv)
{
    int		binary = FALSE;
    int		append = FALSE;
#ifdef HAVE_FSYNC
    int		do_fsync = p_fs;
#endif
    char_u	*fname;
    FILE	*fd;
    int		ret = 0;
    listitem_T	*li;
    list_T	*list = NULL;
    blob_T	*blob = NULL;

    rettv->vval.v_number = -1;
    if (check_secure())
	return;

    if (argvars[0].v_type == VAR_LIST)
    {
	list = argvars[0].vval.v_list;
	if (list == NULL)
	    return;
	for (li = list->lv_first; li != NULL; li = li->li_next)
	    if (tv_get_string_chk(&li->li_tv) == NULL)
		return;
    }
    else if (argvars[0].v_type == VAR_BLOB)
    {
	blob = argvars[0].vval.v_blob;
	if (blob == NULL)
	    return;
    }
    else
    {
	semsg(_(e_invarg2), "writefile()");
	return;
    }

    if (argvars[2].v_type != VAR_UNKNOWN)
    {
	char_u *arg2 = tv_get_string_chk(&argvars[2]);

	if (arg2 == NULL)
	    return;
	if (vim_strchr(arg2, 'b') != NULL)
	    binary = TRUE;
	if (vim_strchr(arg2, 'a') != NULL)
	    append = TRUE;
#ifdef HAVE_FSYNC
	if (vim_strchr(arg2, 's') != NULL)
	    do_fsync = TRUE;
	else if (vim_strchr(arg2, 'S') != NULL)
	    do_fsync = FALSE;
#endif
    }

    fname = tv_get_string_chk(&argvars[1]);
    if (fname == NULL)
	return;

    /* Always open the file in binary mode, library functions have a mind of
     * their own about CR-LF conversion. */
    if (*fname == NUL || (fd = mch_fopen((char *)fname,
				      append ? APPENDBIN : WRITEBIN)) == NULL)
    {
	semsg(_(e_notcreate), *fname == NUL ? (char_u *)_("<empty>") : fname);
	ret = -1;
    }
    else if (blob)
    {
	if (write_blob(fd, blob) == FAIL)
	    ret = -1;
#ifdef HAVE_FSYNC
	else if (do_fsync)
	    // Ignore the error, the user wouldn't know what to do about it.
	    // May happen for a device.
	    vim_ignored = vim_fsync(fileno(fd));
#endif
	fclose(fd);
    }
    else
    {
	if (write_list(fd, list, binary) == FAIL)
	    ret = -1;
#ifdef HAVE_FSYNC
	else if (do_fsync)
	    /* Ignore the error, the user wouldn't know what to do about it.
	     * May happen for a device. */
	    vim_ignored = vim_fsync(fileno(fd));
#endif
	fclose(fd);
    }

    rettv->vval.v_number = ret;
}

#endif // FEAT_EVAL

#if defined(FEAT_BROWSE) || defined(PROTO)
/*
 * Generic browse function.  Calls gui_mch_browse() when possible.
 * Later this may pop-up a non-GUI file selector (external command?).
 */
    char_u *
do_browse(
    int		flags,		/* BROWSE_SAVE and BROWSE_DIR */
    char_u	*title,		/* title for the window */
    char_u	*dflt,		/* default file name (may include directory) */
    char_u	*ext,		/* extension added */
    char_u	*initdir,	/* initial directory, NULL for current dir or
				   when using path from "dflt" */
    char_u	*filter,	/* file name filter */
    buf_T	*buf)		/* buffer to read/write for */
{
    char_u		*fname;
    static char_u	*last_dir = NULL;    /* last used directory */
    char_u		*tofree = NULL;
    int			save_browse = cmdmod.browse;

    /* Must turn off browse to avoid that autocommands will get the
     * flag too!  */
    cmdmod.browse = FALSE;

    if (title == NULL || *title == NUL)
    {
	if (flags & BROWSE_DIR)
	    title = (char_u *)_("Select Directory dialog");
	else if (flags & BROWSE_SAVE)
	    title = (char_u *)_("Save File dialog");
	else
	    title = (char_u *)_("Open File dialog");
    }

    /* When no directory specified, use default file name, default dir, buffer
     * dir, last dir or current dir */
    if ((initdir == NULL || *initdir == NUL) && dflt != NULL && *dflt != NUL)
    {
	if (mch_isdir(dflt))		/* default file name is a directory */
	{
	    initdir = dflt;
	    dflt = NULL;
	}
	else if (gettail(dflt) != dflt)	/* default file name includes a path */
	{
	    tofree = vim_strsave(dflt);
	    if (tofree != NULL)
	    {
		initdir = tofree;
		*gettail(initdir) = NUL;
		dflt = gettail(dflt);
	    }
	}
    }

    if (initdir == NULL || *initdir == NUL)
    {
	/* When 'browsedir' is a directory, use it */
	if (STRCMP(p_bsdir, "last") != 0
		&& STRCMP(p_bsdir, "buffer") != 0
		&& STRCMP(p_bsdir, "current") != 0
		&& mch_isdir(p_bsdir))
	    initdir = p_bsdir;
	/* When saving or 'browsedir' is "buffer", use buffer fname */
	else if (((flags & BROWSE_SAVE) || *p_bsdir == 'b')
		&& buf != NULL && buf->b_ffname != NULL)
	{
	    if (dflt == NULL || *dflt == NUL)
		dflt = gettail(curbuf->b_ffname);
	    tofree = vim_strsave(curbuf->b_ffname);
	    if (tofree != NULL)
	    {
		initdir = tofree;
		*gettail(initdir) = NUL;
	    }
	}
	/* When 'browsedir' is "last", use dir from last browse */
	else if (*p_bsdir == 'l')
	    initdir = last_dir;
	/* When 'browsedir is "current", use current directory.  This is the
	 * default already, leave initdir empty. */
    }

# ifdef FEAT_GUI
    if (gui.in_use)		/* when this changes, also adjust f_has()! */
    {
	if (filter == NULL
#  ifdef FEAT_EVAL
		&& (filter = get_var_value((char_u *)"b:browsefilter")) == NULL
		&& (filter = get_var_value((char_u *)"g:browsefilter")) == NULL
#  endif
	)
	    filter = BROWSE_FILTER_DEFAULT;
	if (flags & BROWSE_DIR)
	{
#  if defined(FEAT_GUI_GTK) || defined(MSWIN)
	    /* For systems that have a directory dialog. */
	    fname = gui_mch_browsedir(title, initdir);
#  else
	    /* Generic solution for selecting a directory: select a file and
	     * remove the file name. */
	    fname = gui_mch_browse(0, title, dflt, ext, initdir, (char_u *)"");
#  endif
#  if !defined(FEAT_GUI_GTK)
	    /* Win32 adds a dummy file name, others return an arbitrary file
	     * name.  GTK+ 2 returns only the directory, */
	    if (fname != NULL && *fname != NUL && !mch_isdir(fname))
	    {
		/* Remove the file name. */
		char_u	    *tail = gettail_sep(fname);

		if (tail == fname)
		    *tail++ = '.';	/* use current dir */
		*tail = NUL;
	    }
#  endif
	}
	else
	    fname = gui_mch_browse(flags & BROWSE_SAVE,
			       title, dflt, ext, initdir, (char_u *)_(filter));

	/* We hang around in the dialog for a while, the user might do some
	 * things to our files.  The Win32 dialog allows deleting or renaming
	 * a file, check timestamps. */
	need_check_timestamps = TRUE;
	did_check_timestamps = FALSE;
    }
    else
# endif
    {
	/* TODO: non-GUI file selector here */
	emsg(_("E338: Sorry, no file browser in console mode"));
	fname = NULL;
    }

    /* keep the directory for next time */
    if (fname != NULL)
    {
	vim_free(last_dir);
	last_dir = vim_strsave(fname);
	if (last_dir != NULL && !(flags & BROWSE_DIR))
	{
	    *gettail(last_dir) = NUL;
	    if (*last_dir == NUL)
	    {
		/* filename only returned, must be in current dir */
		vim_free(last_dir);
		last_dir = alloc(MAXPATHL);
		if (last_dir != NULL)
		    mch_dirname(last_dir, MAXPATHL);
	    }
	}
    }

    vim_free(tofree);
    cmdmod.browse = save_browse;

    return fname;
}
#endif

#if defined(FEAT_EVAL) || defined(PROTO)

/*
 * "browse(save, title, initdir, default)" function
 */
    void
f_browse(typval_T *argvars UNUSED, typval_T *rettv)
{
# ifdef FEAT_BROWSE
    int		save;
    char_u	*title;
    char_u	*initdir;
    char_u	*defname;
    char_u	buf[NUMBUFLEN];
    char_u	buf2[NUMBUFLEN];
    int		error = FALSE;

    save = (int)tv_get_number_chk(&argvars[0], &error);
    title = tv_get_string_chk(&argvars[1]);
    initdir = tv_get_string_buf_chk(&argvars[2], buf);
    defname = tv_get_string_buf_chk(&argvars[3], buf2);

    if (error || title == NULL || initdir == NULL || defname == NULL)
	rettv->vval.v_string = NULL;
    else
	rettv->vval.v_string =
		 do_browse(save ? BROWSE_SAVE : 0,
				 title, defname, NULL, initdir, NULL, curbuf);
# else
    rettv->vval.v_string = NULL;
# endif
    rettv->v_type = VAR_STRING;
}

/*
 * "browsedir(title, initdir)" function
 */
    void
f_browsedir(typval_T *argvars UNUSED, typval_T *rettv)
{
# ifdef FEAT_BROWSE
    char_u	*title;
    char_u	*initdir;
    char_u	buf[NUMBUFLEN];

    title = tv_get_string_chk(&argvars[0]);
    initdir = tv_get_string_buf_chk(&argvars[1], buf);

    if (title == NULL || initdir == NULL)
	rettv->vval.v_string = NULL;
    else
	rettv->vval.v_string = do_browse(BROWSE_DIR,
				    title, NULL, NULL, initdir, NULL, curbuf);
# else
    rettv->vval.v_string = NULL;
# endif
    rettv->v_type = VAR_STRING;
}

#endif // FEAT_EVAL
