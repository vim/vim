/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * os_mac.c -- code for the MacOS
 *
 * This file is mainly based on os_unix.c.
 */

#include "vim.h"

#if defined(__MRC__) || defined(__SC__) /* for Apple MPW Compilers */

#include "StandardFile.h"

/*
 * Implements the corresponding unix function
 */
    int
stat(
    char *p,
    struct stat *p_st)
{
    /*
       TODO: Use functions which fill the FileParam struct (Files.h)
	     and copy these contents to our self-defined stat struct
     */
    return 0;
}
#endif

/*
 * change the current working directory
 */
    int
mch_chdir(char *p_name)
{
#if defined(__MRC__) || defined(__SC__) /* for Apple MPW Compilers */
    /* TODO */
    return FAIL;
#else
    return chdir(p_name);
#endif
}


/*
 * Recursively build up a list of files in "gap" matching the first wildcard
 * in `path'.  Called by mch_expandpath().
 * "path" has backslashes before chars that are not to be expanded.
 */
    static int
mac_expandpath(
    garray_T	*gap,
    char_u	*path,
    int		flags,		/* EW_* flags */
    short	start_at,
    short	as_full)
{
    /*
     * TODO:
     *		+Get Volumes (when looking for files in current dir)
     *		+Make it work when working dir not on select volume
     *		+Cleanup
     */
    short	index = 1;
    OSErr	gErr;
    char_u	dirname[256];
    char_u	cfilename[256];
    long	dirID;
    char_u	*new_name;
    CInfoPBRec	gMyCPB;
    HParamBlockRec gMyHPBlock;
    FSSpec	usedDir;

    char_u	*buf;
    char_u	*p, *s, *e, dany;
    int		start_len, c;
    char_u	*pat;
    regmatch_T	regmatch;

    start_len = gap->ga_len;
    buf = alloc(STRLEN(path) + BASENAMELEN + 5);/* make room for file name */
    if (buf == NULL)
	return 0;

/*
 * Find the first part in the path name that contains a wildcard.
 * Copy it into buf, including the preceding characters.
 */
    p = buf;
    s = buf;
    e = NULL;
#if 1
    STRNCPY(buf, path, start_at);
    p += start_at;
    path += start_at;
#endif

    while (*path)
    {
	if (*path == ':')
	{
	    if (e)
		break;
	    s = p + 1;
	}
	/* should use  WILCARDLIST but what about ` */
	/*	    if (vim_strchr((char_u *)"*?[{~$", *path) != NULL)*/
	else if (vim_strchr((char_u *)WILDCHAR_LIST, *path) != NULL)
	    e = p;
#ifdef FEAT_MBYTE
	if (has_mbyte)
	{
	    int len = (*mb_ptr2len_check)(path);

	    STRNCPY(p, path, len);
	    p += len;
	    path += len;
	}
	else
#endif
	    *p++ = *path++;
    }
    e = p;

    /* now we have one wildcard component between s and e */
    *e = NUL;

    dany = *s;
    *s = NUL;
    backslash_halve(buf);
    *s = dany;

    /* convert the file pattern to a regexp pattern */
    pat = file_pat_to_reg_pat(s, e, NULL, FALSE);
    if (pat == NULL)
    {
	vim_free(buf);
	return 0;
    }

    /* compile the regexp into a program */
    regmatch.rm_ic = FALSE;			/* Don't ever ignore case */
    regmatch.regprog = vim_regcomp(pat, RE_MAGIC);
    vim_free(pat);

    if (regmatch.regprog == NULL)
    {
	vim_free(buf);
	return 0;
    }

    /* open the directory for scanning */
    c = *s;
    *s = NUL;

    if (*buf == NUL)
    {
	as_full = TRUE;
#if 0
	(void) mch_dirname (&dirname[1], 254);
	dirname[0] = STRLEN(&dirname[1]);
#endif
    }
    else
    {
	if (*buf == ':')  /* relative path */
	{
	    (void)mch_dirname(&dirname[1], 254);
	    new_name = concat_fnames(&dirname[1], buf+1, TRUE);
	    STRCPY(&dirname[1], new_name);
	    dirname[0] = STRLEN(new_name);
	    vim_free(new_name);
	}
	else
	{
	    STRCPY(&dirname[1], buf);
	    backslash_halve(&dirname[1]);
	    dirname[0] = STRLEN(buf);
	}
    }
    *s = c;

    FSMakeFSSpec (0, 0, dirname, &usedDir);

    gMyCPB.dirInfo.ioNamePtr    = dirname;
    gMyCPB.dirInfo.ioVRefNum    = usedDir.vRefNum;
    gMyCPB.dirInfo.ioFDirIndex  = 0;
    gMyCPB.dirInfo.ioDrDirID    = 0;

    gErr = PBGetCatInfo(&gMyCPB, false);

    gMyCPB.dirInfo.ioCompletion = NULL;
    dirID = gMyCPB.dirInfo.ioDrDirID;
    do
    {
	gMyCPB.hFileInfo.ioFDirIndex = index;
	gMyCPB.hFileInfo.ioDirID     = dirID;

	gErr = PBGetCatInfo(&gMyCPB, false);

	if (gErr == noErr)
	{
	    vim_strncpy(cfilename, &dirname[1], dirname[0]);
	    if (vim_regexec(&regmatch, cfilename, (colnr_T)0))
	    {
		if (s[-1] != ':')
		{
		    /* TODO: need to copy with cleaned name */
		    STRCPY(s+1, cfilename);
		    s[0] = ':';
		}
		else
		{   /* TODO: need to copy with cleeaned name */
		    STRCPY(s, cfilename);
		}
		start_at = STRLEN(buf);
		STRCAT(buf, path);
		if (mch_has_exp_wildcard(path))	/* handle more wildcards */
		    (void)mac_expandpath(gap, buf, flags, start_at, FALSE);
		else
		{
#ifdef DONT_ADD_PATHSEP_TO_DIR
		    if ((gMyCPB.hFileInfo.ioFlAttrib & ioDirMask) !=0 )
			STRCAT(buf, PATHSEPSTR);
#endif
		    addfile(gap, buf, flags);
		}
	    }
#if 0	/* What is this supposed to do? */
	    if ((gMyCPB.hFileInfo.ioFlAttrib & ioDirMask) != 0)
	    {
	    }
	    else
	    {
	    }
#endif
	}
	index++;
    }
    while (gErr == noErr);

    if (as_full)
    {
	index = 1;
	do
	{
	    gMyHPBlock.volumeParam.ioNamePtr = (char_u *) dirname;
	    gMyHPBlock.volumeParam.ioVRefNum = 0;
	    gMyHPBlock.volumeParam.ioVolIndex = index;

	    gErr = PBHGetVInfo (&gMyHPBlock,false);
	    if (gErr == noErr)
	    {
		vim_strncpy(cfilename, &dirname[1], dirname[0]);
		if (vim_regexec(&regmatch, cfilename, (colnr_T)0))
		{
		    STRCPY(s, cfilename);
		    STRCAT(buf, path);
		    if (mch_has_exp_wildcard(path)) /* handle more wildcards */
			(void)mac_expandpath(gap, s, flags, 0, FALSE);
		    else
		    {
#ifdef DONT_ADD_PATHSEP_TO_DIR
/*			if ((gMyCPB.hFileInfo.ioFlAttrib & ioDirMask) !=0 )
*/			    STRCAT(buf, PATHSEPSTR);
#endif
			addfile(gap, s, flags);
		    }
#if 0
		    STRCAT(cfilename, PATHSEPSTR);
		    addfile (gap, cfilename, flags);
#endif
		}
	    }
	    index++;
	}
	while (gErr == noErr);
    }

    vim_free(regmatch.regprog);

    return gap->ga_len - start_len;
}

/*
 * Recursively build up a list of files in "gap" matching the first wildcard
 * in `path'.  Called by expand_wildcards().
 * "pat" has backslashes before chars that are not to be expanded.
 */
    int
mch_expandpath(
    garray_T	*gap,
    char_u	*path,
    int		flags)		/* EW_* flags */
{
#ifdef USE_UNIXFILENAME
    return unix_expandpath(gap, path, 0, flags, FALSE);
#else
    char_u first = *path;
    short  scan_volume;

    slash_n_colon_adjust(path);

    scan_volume = (first != *path);

    return mac_expandpath(gap, path, flags, 0, scan_volume);
#endif
}

    void
fname_case(name, len)
    char_u	*name;
    int		len;	    /* buffer size, ignored here */
{
    /*
     * TODO: get the real casing for the file
     *       make it called
     *       with USE_FNAME_CASE & USE_LONG_FNAME
     *		    CASE_INSENSITIVE_FILENAME
     *       within setfname, fix_fname, do_ecmd
     */
#ifdef USE_UNIXFILENAME
    OSStatus	status;
    FSRef	refFile;
    UInt32	pathSize = STRLEN(name) + 1;
    char_u	*path;
    Boolean	isDirectory;

    path = alloc(pathSize);
    if (path == NULL)
	return;

    status = FSPathMakeRef((UInt8 *)name, &refFile, &isDirectory);
    if (status)
	return;

    status = FSRefMakePath(&refFile, (UInt8 *)path, pathSize);
    if (status)
	return;

    /* Paranoid: Update the name if only the casing differ.*/
    if (STRICMP(name, path) == 0)
	STRCPY(name, path);
#endif
}
static char_u	*oldtitle = (char_u *) "gVim";

/*
 * check for an "interrupt signal": CTRL-break or CTRL-C
 */
    void
mch_breakcheck()
{
    /*
     * TODO: Scan event for a CTRL-C or COMMAND-. and do: got_int=TRUE;
     *       or only go proccess event?
     *       or do nothing
     */
    EventRecord theEvent;

    if (EventAvail(keyDownMask, &theEvent))
	if ((theEvent.message & charCodeMask) == Ctrl_C && ctrl_c_interrupts)
	    got_int = TRUE;
#if 0
    short	i = 0;
    Boolean     found = false;
    EventRecord theEvent;

    while ((i < 10) && (!found))
    {
	found = EventAvail (keyDownMask, &theEvent);
	if (found)
	{
	  if ((theEvent.modifiers & controlKey) != 0)
	    found = false;
	  if ((theEvent.what == keyDown))
	    found = false;
	  if ((theEvent.message & charCodeMask) == Ctrl_C)
	    {
		found = false;
		got_int = TRUE;
	    }
	}
	i++;
    }
#endif

}

/*
 * Return amount of memory currently available.
 */
    long_u
mch_avail_mem(special)
    int     special;
{
    /*
     * TODO: Use MaxBlock, FreeMeM, PurgeSpace, MaxBlockSys FAQ-266
     *       figure out what the special is for
     *
     * FreeMem  ->   returns all avail memory is application heap
     * MaxBlock ->   returns the biggest contigeous block in application heap
     * PurgeSpace ->
     */
    return MaxBlock();
}

    void
mch_delay(msec, ignoreinput)
    long	msec;
    int		ignoreinput;
{
#if (defined(__MWERKS__) && __MWERKS__ >= 0x2000) \
	|| defined(__MRC__) || defined(__SC__)
    unsigned
#endif
    long   finalTick;

    if (ignoreinput)
	Delay (60*msec/1000, &finalTick);
    else
	/* even thougth we should call gui stuff from here
	   it the simplest way to be safe */
	gui_mch_wait_for_chars(msec);
}

    void
mch_init()
{
    /*
     *  TODO: Verify if needed, or override later.
     */
    Columns = 80;
    Rows = 24;
}

/*
 * Check_win checks whether we have an interactive stdout.
 */
    int
mch_check_win(argc, argv)
    int		argc;
    char	**argv;
{
    /*
     * TODO: Maybe to be remove through NO_CONSOLE
     */
    return OK;
}

/*
 * Return TRUE if the input comes from a terminal, FALSE otherwise.
 */
    int
mch_input_isatty()
{
    /*
     * TODO: Maybe to be remove through NO_CONSOLE
     */
    return OK;
}

#ifdef FEAT_TITLE
/*
 * Set the window title and icon.
 * (The icon is not taken care of).
 */
    void
mch_settitle(title, icon)
    char_u *title;
    char_u *icon;
{
    gui_mch_settitle(title, icon);
}

/*
 * Restore the window/icon title.
 * which is one of:
 *	1  Just restore title
 *      2  Just restore icon
 *	3  Restore title and icon
 * but don't care about the icon.
 */
    void
mch_restore_title(which)
    int which;
{
    mch_settitle((which & 1) ? oldtitle : NULL, NULL);
}
#endif

/*
 * Insert user name in s[len].
 * Return OK if a name found.
 */
    int
mch_get_user_name(s, len)
    char_u	*s;
    int		len;
{
#if !(defined(__MRC__) || defined(__SC__)) /* No solution yet */
    /*
     * TODO: clean up and try getlogin ()
     */
#if defined(HAVE_PWD_H) && defined(HAVE_GETPWUID)
    struct passwd	*pw;
#endif
    uid_t		uid;

    uid = getuid();
#if defined(HAVE_PWD_H) && defined(HAVE_GETPWUID)
    if ((pw = getpwuid(uid)) != NULL
	    && pw->pw_name != NULL && *(pw->pw_name) != NUL)
    {
	vim_strncpy(s, pw->pw_name, len - 1);
	return OK;
    }
#endif
    sprintf((char *)s, "%d", (int)uid);		/* assumes s is long enough */
#endif
    return FAIL;				/* a number is not a name */
}

/*
 * Copy host name into s[len].
 */
    void
mch_get_host_name(s, len)
    char_u	*s;
    int		len;
{
#if defined(__MRC__) || defined(__SC__) || defined(__APPLE_CC__)
    vim_strncpy(s, "Mac", len - 1); /* TODO: use Gestalt information */
#else
    struct utsname vutsname;

    if (uname(&vutsname) < 0)
	*s = NUL;
    else
	vim_strncpy(s, vutsname.nodename, len - 1);
#endif
}

/*
 * return process ID
 */
    long
mch_get_pid()
{
    return (long)getpid();
}

/*
 * Get name of current directory into buffer 'buf' of length 'len' bytes.
 * Return OK for success, FAIL for failure.
 */
    int
mch_dirname(buf, len)
    char_u	*buf;
    int		len;
{
#if defined(__MRC__) || defined(__SC__)
    return FAIL; /* No solution yet */
#else
    /* The last : is already put by getcwd */
    if (getcwd((char *)buf, len) == NULL)
	{
	    STRCPY(buf, strerror(errno));
	    return FAIL;
	}
# ifndef USE_UNIXFILENAME
    else if (*buf != NUL && buf[STRLEN(buf) - 1] == ':')
	buf[STRLEN(buf) - 1] = NUL;	/* remove trailing ':' */
# endif
    return OK;
#endif
}

    void
slash_to_colon(p)
    char_u	*p;
{
    for ( ; *p; ++p)
	if (*p == '/')
	    *p = ':';
}

    char_u *
slash_to_colon_save (p)
    char_u  *p;
{
    char_u	*res;

    res = vim_strsave(p);
    if (res == NULL)
	return p;
    slash_to_colon(res);
    return res;
}

    void
slash_n_colon_adjust (buf)
    char_u *buf;
{
    /*
     * TODO: Make it faster
     */
#ifndef USE_UNIXFILENAME
    char_u  temp[MAXPATHL];
    char_u  *first_colon = vim_strchr(buf, ':');
    char_u  *first_slash = vim_strchr(buf, '/');
    int     full = TRUE;
    char_u  *scanning;
    char_u  *filling;
    char_u  last_copied = NUL;

    if (*buf == NUL)
	return ;

    if ((first_colon == NULL) && (first_slash == NULL))
	full = FALSE;
    if ((first_slash == NULL) && (first_colon != NULL))
	full = TRUE;
    if ((first_colon == NULL) && (first_slash != NULL))
	full =	FALSE;
    if ((first_slash < first_colon) && (first_slash != NULL))
	full = FALSE;
    if ((first_colon < first_slash) && (first_colon != NULL))
	full = TRUE;
    if (first_slash == buf)
	full = TRUE;
    if (first_colon == buf)
	full = FALSE;

    scanning = buf;
    filling  = temp;

    while (*scanning != NUL)
    {
	if (*scanning == '/')
	{
	    if ((scanning[1] != '/') && (scanning[-1] != ':'))
	    {
		*filling++ = ':';
		scanning++;
	    }
	    else
		scanning++;
	}
	else if (*scanning == '.')
	{
	    if ((scanning[1] == NUL) || scanning[1] == '/')
	    {
		if (scanning[1] == NUL)
		    scanning += 1;
		else
		    scanning += 2;
	    }
	    else if (scanning[1] == '.')
	    {
		if ((scanning[2] == NUL) || scanning[2] == '/')
		{
		    *filling++ = ':';
		    if (scanning[2] == NUL)
			scanning +=2;
		    else
			scanning += 3;
		}
		else
		{
		    *filling++ = *scanning++;
		}
	    }
	    else
	    {
		*filling++ = *scanning++;
	    }

	}
	else
	{
	    *filling++ = *scanning++;
	}

    }

    *filling = 0;
    filling = temp;

    if (!full)
    {
	if (buf[0] != ':')
	{
	    buf[0] = ':';
	    buf[1] = NUL;
	}
	else
	    buf[0] = NUL;
    }
    else
    {
	buf[0] = NUL;
	if (filling[0] == ':')
	    filling++;
    }

    STRCAT (buf, filling);
#endif
}

/*
 * Get absolute filename into buffer 'buf' of length 'len' bytes.
 *
 * return FAIL for failure, OK for success
 */
    int
mch_FullName(fname, buf, len, force)
    char_u  *fname, *buf;
    int     len;
    int     force;	    /* also expand when already absolute path name */
{
    /*
     * TODO: Find what TODO
     */
    int		l;
    char_u	olddir[MAXPATHL];
    char_u	newdir[MAXPATHL];
    char_u	*p;
    char_u	c;
    int		retval = OK;

    if (force || !mch_isFullName(fname))
    {
	/*
	 * Forced or not an absolute path.
	 * If the file name has a path, change to that directory for a moment,
	 * and then do the getwd() (and get back to where we were).
	 * This will get the correct path name with "../" things.
	 */
	if ((p = vim_strrchr(fname, ':')) != NULL)
	{
	    p++;
	    if (mch_dirname(olddir, MAXPATHL) == FAIL)
	    {
		p = NULL;		/* can't get current dir: don't chdir */
		retval = FAIL;
	    }
	    else
	    {
		c = *p;
		*p = NUL;
		if (mch_chdir((char *)fname))
		    retval = FAIL;
		else
		    fname = p; /* + 1;*/
		*p = c;
	    }
	}
	if (mch_dirname(buf, len) == FAIL)
	{
	    retval = FAIL;
	    *newdir = NUL;
	}
	l = STRLEN(buf);
	if (STRCMP(fname, ".") != 0)
	{
#ifdef USE_UNIXFILENAME
	    if (l > 0 && buf[l - 1] != '/' && *fname != NUL)
		STRCAT(buf, "/");
#else
	    if (l > 0 && buf[l - 1] != ':' && *fname != NUL)
		STRCAT(buf, ":");
#endif
	}
	if (p != NULL)
	    mch_chdir((char *)olddir);
	if (STRCMP(fname, ".") != 0)
	    STRCAT(buf, fname);
    }
    else
    {
	vim_strncpy(buf, fname, len - 1);
	slash_n_colon_adjust(buf);
    }

    return retval;
}

/*
 * Return TRUE if "fname" does not depend on the current directory.
 */
	int
mch_isFullName(fname)
	char_u		*fname;
{
#ifdef USE_UNIXFILENAME
    return ((fname[0] == '/') || (fname[0] == '~'));
#else
    /*
     * TODO: Make sure fname is always of mac still
     *       i.e: passed throught slash_n_colon_adjust
     */
	char_u	*first_colon = vim_strchr(fname, ':');
	char_u	*first_slash = vim_strchr(fname, '/');

	if (first_colon == fname)
	  return FALSE;
	if (first_slash == fname)
	  return TRUE;
	if ((first_colon < first_slash) && (first_colon != NULL))
	  return TRUE;
	if ((first_slash < first_colon) && (first_slash != NULL))
	  return FALSE;
	if ((first_colon == NULL) && (first_slash != NULL))
	  return FALSE;
	if ((first_slash == NULL) && (first_colon != NULL))
	  return TRUE;
	if ((first_colon == NULL) && (first_slash == NULL))
	  return FALSE;
	return TRUE;
#endif
}

/*
 * Replace all slashes by colons.
 */
    void
slash_adjust(p)
    char_u  *p;
{
#ifndef USE_UNIXFILENAME
    /*
     * TODO: keep escaped '/'
     */

    while (*p)
    {
	if (*p == '/')
	    *p = ':';
	mb_ptr_adv(p);
    }
#endif
}

/*
 * Get file permissions for 'name'.
 * Returns -1 when it doesn't exist.
 */
    long
mch_getperm(name)
    char_u *name;
{
    /*
     * TODO: Maybe use AppleShare info??
     *       Use locked for non writable
     */

    struct stat statb;

    if (stat((char *)name, &statb))
	return -1;
    return statb.st_mode;
}

/*
 * set file permission for 'name' to 'perm'
 *
 * return FAIL for failure, OK otherwise
 */
    int
mch_setperm(name, perm)
    char_u	*name;
    long	perm;
{
    /*
     * TODO: Maybe use AppleShare info??
     *       Use locked for non writable
     */
    return (OK);
}

/*
 * Set hidden flag for "name".
 */
    void
mch_hide(name)
    char_u  *name;
{
    /*
     * TODO: Hide the file throught FileManager FAQ 8-34
     *
     *  *name is mac style start with : for relative
     */
}


/*
 * return TRUE if "name" is a directory
 * return FALSE if "name" is not a directory
 * return FALSE for error
 */
    int
mch_isdir(name)
    char_u  *name;
{
    /*
     * TODO: Find out by FileManager calls ...
     */
    struct stat statb;

#if defined(TARGET_API_MAC_CARBON) && TARGET_API_MAC_CARBON
    /* For some reason the name is sometimes empty,
     * (such as for a not yet named file). An empty
     * filename is interpreted by the MacOS version
     * of stat (at least under Codewarrior) as the
     * current directory.
     */
    /* AK 20020413
     * This is required for Carbon but breaks the
     * explorer plugin in Classic
     */
    if (name[0] == NULL)
	return FALSE;
#endif

    if (stat((char *)name, &statb))
	return FALSE;
#if defined(__MRC__) || defined(__SC__)
    return FALSE;   /* definitely TODO */
#else
    return ((statb.st_mode & S_IFMT) == S_IFDIR ? TRUE : FALSE);
#endif
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Return 1 if "name" can be executed, 0 if not.
 * Return -1 if unknown.
 */
    int
mch_can_exe(name)
    char_u	*name;
{
    /* TODO */
    return -1;
}
#endif

/*
 * Check what "name" is:
 * NODE_NORMAL: file or directory (or doesn't exist)
 * NODE_WRITABLE: writable device, socket, fifo, etc.
 * NODE_OTHER: non-writable things
 */
    int
mch_nodetype(name)
    char_u	*name;
{
    /* TODO */
    return NODE_NORMAL;
}

    void
mch_early_init()
{
}

    void
mch_exit(r)
    int     r;
{
    display_errors();

    ml_close_all(TRUE);		/* remove all memfiles */
    exit(r);
}


    void
mch_settmode(tmode)
    int     tmode;
{
    /*
     * TODO: remove the needs of it.
     */
}

#ifdef FEAT_MOUSE
/*
 * set mouse clicks on or off (only works for xterms)
 */
    void
mch_setmouse(on)
    int     on;
{
    /*
     * TODO: remove the needs of it.
     */
}
#endif

/*
 * set screen mode, always fails.
 */
    int
mch_screenmode(arg)
    char_u	 *arg;
{
    EMSG(_(e_screenmode));
    return FAIL;
}

    int
mch_call_shell(cmd, options)
    char_u	*cmd;
    int		options;	/* SHELL_*, see vim.h */
{
    /*
     * TODO: find a shell or pseudo-shell to call
     *       for some simple useful command
     */

    return (-1);
}

/*
 * Return TRUE if "p" contains a wildcard that can be expanded by
 * mch_expandpath().
 */
    int
mch_has_exp_wildcard(p)
    char_u	*p;
{
    for ( ; *p; mb_ptr_adv(p))
    {
	if (*p == '\\' && p[1] != NUL)
	    ++p;
	else if (vim_strchr((char_u *)WILDCHAR_LIST, *p) != NULL)
	    return TRUE;
    }
    return FALSE;
}

    int
mch_has_wildcard(p)
    char_u	*p;
{
#ifdef USE_UNIXFILENAME
    if (*p == '~' && p[1] != NUL)
	return TRUE;
#endif
    return mch_has_exp_wildcard(p);
}


/*
 * This procedure duplicate a file, it is used in order to keep
 * the footprint of the previous file, when some info can be easily
 * restored with set_perm().
 *
 * Return -1 for failure, 0 for success.
 */
    int
mch_copy_file(from, to)
    char_u *from;
    char_u *to;
{
    char_u  from_str[256];
    char_u  to_str[256];
    char_u  to_name[256];

    HParamBlockRec paramBlock;
    char_u  *char_ptr;
    int	    len;

    /*
     * Convert C string to Pascal string
     */
     char_ptr = from;
     len = 1;
     for (; (*char_ptr != 0) && (len < 255); len++, char_ptr++)
	from_str[len] = *char_ptr;
     from_str[0] = len-1;

     char_ptr = to;
     len = 1;
     for (; (*char_ptr != 0) && (len < 255); len++, char_ptr++)
	to_str[len] = *char_ptr;
     to_str[0] = len-1;

    paramBlock.copyParam.ioCompletion = NULL;
    paramBlock.copyParam.ioNamePtr    = from_str;
 /* paramBlock.copyParam.ioVRefnum    =  overided by ioFilename; */
 /* paramBlock.copyParam.ioDirI       =  overided by ioFilename; */

    paramBlock.copyParam.ioNewName    = to_str;
    paramBlock.copyParam.ioCopyName   = to_name;     /* NIL */
 /* paramBlock.copyParam.ioDstVRefNum = overided by ioNewName;   */
 /* paramBlock.copyParam.ioNewDirID   = overided by ioNewName;   */



    /*
     * First delete the "to" file, this is required on some systems to make
     * the rename() work, on other systems it makes sure that we don't have
     * two files when the rename() fails.
     */
    mch_remove(to);

    /*
     * First try a normal rename, return if it works.
     */
    (void) PBHCopyFile(&paramBlock, false);
    return 0;

}


    int
mch_copy_file_attribute(from, to)
    char_u *from;
    char_u *to;
{
    FSSpec	frFSSpec;
    FSSpec	toFSSpec;
    FInfo	fndrInfo;
    Str255	name;
    ResType	type;
    ResType	sink;
    Handle	resource;
    short	idxTypes;
    short	nbTypes;
    short	idxResources;
    short	nbResources;
    short	ID;
    short	frRFid;
    short	toRFid;
    short	attrs_orig;
    short	attrs_copy;
    short	temp;

    /* TODO: Handle error */
    (void)GetFSSpecFromPath(from, &frFSSpec);
    (void)GetFSSpecFromPath(to  , &toFSSpec);

    /* Copy resource fork */
    temp = 0;

#if 1
     frRFid = FSpOpenResFile (&frFSSpec, fsCurPerm);

     if (frRFid != -1)
     {
	 FSpCreateResFile(&toFSSpec, 'TEXT', UNKNOWN_CREATOR, 0);
	 toRFid = FSpOpenResFile(&toFSSpec, fsRdWrPerm);

	 UseResFile(frRFid);

	 nbTypes = Count1Types();

	 for (idxTypes = 1; idxTypes <= nbTypes; idxTypes++)
	 {
	     Get1IndType(&type, idxTypes);
	     nbResources = Count1Resources(type);

	     for (idxResources = 1; idxResources <= nbResources; idxResources++)
	     {
		 attrs_orig = 0; /* in case GetRes fails */
		 attrs_copy = 0; /* in case GetRes fails */
		 resource = Get1IndResource(type, idxResources);
		 GetResInfo(resource, &ID, &sink, name);
		 HLock(resource);
		 attrs_orig = GetResAttrs(resource);
		 DetachResource(resource);


		 UseResFile(toRFid);
		 AddResource(resource, type, ID, name);
		 attrs_copy = GetResAttrs(resource);
		 attrs_copy = (attrs_copy & 0x2) | (attrs_orig & 0xFD);
		 SetResAttrs(resource, attrs_copy);
		 WriteResource(resource);
		 UpdateResFile(toRFid);

		 temp = GetResAttrs(resource);

		 /*SetResAttrs (resource, 0);*/
		 HUnlock(resource);
		 ReleaseResource(resource);
		 UseResFile(frRFid);
	     }
	 }
	 CloseResFile(toRFid);
	 CloseResFile(frRFid);
     }
#endif
    /* Copy Finder Info */
    (void)FSpGetFInfo(&frFSSpec, &fndrInfo);
    (void)FSpSetFInfo(&toFSSpec, &fndrInfo);

    return (temp == attrs_copy);
}

    int
mch_has_resource_fork (file)
    char_u *file;
{
    FSSpec  fileFSSpec;
    short   fileRFid;

    /* TODO: Handle error */
    (void)GetFSSpecFromPath(file, &fileFSSpec);
    fileRFid = FSpOpenResFile(&fileFSSpec, fsCurPerm);
    if (fileRFid != -1)
	CloseResFile(fileRFid);

    return (fileRFid != -1);
}

    int
mch_get_shellsize(void)
{
    /* never used */
    return OK;
}

    void
mch_set_shellsize(void)
{
    /* never used */
}

/*
 * Rows and/or Columns has changed.
 */
    void
mch_new_shellsize(void)
{
    /* never used */
}

/*
 * Those function were set as #define before, but in order
 * to allow an easier us of os_unix.c for the MacOS X port,
 * they are change to procedure. Thec ompile whould optimize
 * them out.
 */

    int
mch_can_restore_title()
{
    return TRUE;
}

    int
mch_can_restore_icon()
{
    return TRUE;
}

/*
 * If the machine has job control, use it to suspend the program,
 * otherwise fake it by starting a new shell.
 */
    void
mch_suspend()
{
    /* TODO: get calle in #ifndef NO_CONSOLE */
    gui_mch_iconify();
};

