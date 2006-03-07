/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

/*
 * os_riscos.c
 *
 * Thomas Leonard <tal197@ecs.soton.ac.uk>
 */

const char *__dynamic_da_name = "Vim heap"; /* Enable and name our dynamic area */
int ro_line_mode = TRUE;  /* For Ex mode we much echo chars to the screen ourselves */
int windowed;		/* Flag - are we running inside a text window? */
int WinLeft, WinTop;	/* We might be started inside a text window */
int ScrollTop;		/* Make cursor movements relative to ScrollTop. */

int old_escape_state = -1;
int old_cursor_state = -1;

#define rgb(r,g,b) ((b<<24) + (g<<16) + (r<<8))
#define NORMAL_FG 0x00000000
#define NORMAL_BG 0xffffffff

/* Convert a DOS colour number to an RGB palette entry.
 * Mappings from X11 rgb/txt file.
 */
    static int
map_colour(dos)
    int dos;		/* Standard DOS colour number. */
{
    switch (dos)
    {
	case 0: return 0;			/* Black */
	case 1: return rgb(0,0,139);		/* DarkBlue */
	case 2: return rgb(0,100,0);		/* DarkGreen */
	case 3: return rgb(0,139,139);		/* DarkCyan */
	case 4: return rgb(139,0,0);		/* DarkRed */
	case 5: return rgb(139,0,139);		/* DarkMagenta */
	case 6: return rgb(165,42,42);		/* Brown, DarkYellow */
	case 7: return rgb(211,211,211);	/* LightGray, LightGrey, Gray, Grey */
	case 8: return rgb(169,169,169);	/* DarkGray, DarkGrey */
	case 9: return rgb(173,216,230);	/* Blue, LightBlue */
	case 10: return rgb(144,238,144);	/* Green, LightGreen */
	case 11: return rgb(224,255,255);	/* Cyan, LightCyan */
	case 12: return rgb(255,0,0);		/* Red, LightRed */
	case 13: return rgb(255,0,255);		/* Magenta, LightMagenta */
	case 14: return rgb(255,255,0);		/* Yellow, LightYellow */
	case 15: return rgb(255,255,255);	/* White */
    }
    return rgb(100,100,100);
}

    static void
text_fg(fg)
    int fg;		/* Foregound colour in the form &BBGGRR00 */
{
    xswi(ColourTrans_SetTextColour, fg, 0, 0, 0);
}

    static void
text_bg(bg)
    int		bg;	/* Backgound colour in the form &BBGGRR00 */
{
    xswi(ColourTrans_SetTextColour, bg, 0, 0, 1 << 7);
}

#define OUT_NORMAL 0
#define OUT_NUMBER 1		/* Reading in a number */

    void
mch_write(s, len)
    char_u  *s;
    int	    len;
{
    static int mode = OUT_NORMAL;
    static int x, y;			/* For reading numbers in. */

    if (!term_console)
    {
	/* Maybe we are running Vim remotely - don't interpret chars */
	while (len--)
	{
	    char_u c = *s++;
	    swi(OS_WriteC, c);
	    /* We might need to send a CR too. This shouldn't
	     * hurt if we don't need it, should it?
	     */
	    if (c == 10)
		swi(OS_WriteI + 13);
	}
	return;
    }

    while (len--)
    {
	char_u c = *s++;
	switch (mode)
	{
	    case OUT_NUMBER:
		if (c < '0' || c > '9')
		{
		    mode = OUT_NORMAL;
		}
		else
		{
		    x = (x * 10) + c - '0';
		    continue;
		}
	    /* note: no break here! */

	    case OUT_NORMAL:
		switch (c)
		{
		    case 1:
			/* Number (in decimal) follows. */
			mode = OUT_NUMBER;
			y = x;
			x = 0;
			break;
		    case 2:
			/* Position cursor. */
			swi(OS_WriteI + 31);
			swi(OS_WriteC, x);
			swi(OS_WriteC, y - ScrollTop);
			break;
		    case 3:
			/* Set scroll region. */
			if (x == Rows -1 && y == 0 && !windowed)
			{
			    /* Whole screen - remove text window.
			     * This is MUCH faster.
			     */
			    swi(OS_WriteI + 26);
			}
			else
			{
			    /* Create a text window. */
			    swi(OS_WriteI + 28);
			    swi(OS_WriteC, WinLeft);
			    swi(OS_WriteC, WinTop + x);
			    swi(OS_WriteC, WinLeft + Columns - 1);
			    swi(OS_WriteC, WinTop + y);
			}
			ScrollTop = y;
			break;
		    case 4:
			/* Normal mode. */
			text_fg(NORMAL_FG);
			text_bg(NORMAL_BG);
			break;
		    case 5:
			/* Reverse mode. */
			text_fg(NORMAL_BG);
			text_bg(NORMAL_FG);
			break;
		    case 10:
			swi(OS_NewLine);
			break;
		    case 14:
			/* Cursor invisible. */
			swi(OS_WriteN,
			     "\027\001\000\000\000\000\000\000\000\000",
			     10);
			break;
		    case 15:
			/* Cursor visible. */
			swi(OS_WriteN,
			     "\027\001\002\000\000\000\000\000\000\000",
			     10);
			break;
		    case 16:
			/* Cursor very visible (flash) */
			swi(OS_WriteN,
			     "\027\001\003\000\000\000\000\000\000\000",
			     10);
		    case 17:
			/* Set foreground colour. */
			text_fg(map_colour(x));
			break;
		    case 18:
			/* Set background colour. */
			text_bg(map_colour(x));
			break;
		    case 19:
			/* Scroll text down. */
			swi(OS_WriteN,
			     "\027\007\000\002\000\000\000\000\000\000",
			     10);
			break;
		    default:
			swi(OS_WriteC, c);
		}
		continue;

	    default:
		printf("[output error]");
		mode = OUT_NORMAL;
	}
    }
}

/*
 * mch_inchar(): low level input funcion.
 * Get a characters from the keyboard.
 * Return the number of characters that are available.
 * If wtime == 0 do not wait for characters.
 * If wtime == n wait n msecs for characters.
 * If wtime == -1 wait forever for characters.
 *
 * TODO: call convert_input() for 'fileencoding' to 'encoding' conversion.
 */
    int
mch_inchar(buf, maxlen, wtime, tb_change_cnt)
    char_u  *buf;
    int	    maxlen;
    long    wtime;
    int	    tb_change_cnt;
{
    int got=0;
    unsigned int start_time = clock();

    if (ro_line_mode)
    {
	/* We're probably in Ex mode - get whole lines at a time. */

	static char_u	line_buffer[256];
	static int	remaining_chars = 0;
	static int	buf_pos = 0;

	/* Do we need to fetch another line? */
	if (remaining_chars == 0)
	{
	    int		old_esc_state;
	    swi(OS_Byte, 200, 1, 0xfe);
	    old_esc_state = r1;

	    buf_pos = 0;
	    if (xswi(OS_ReadLine, line_buffer, 255, 0, 255) & (c_flag | v_flag))
	    {
		got_int = TRUE;	    /* ESC pressed */
		r1 = 0;
	    }
	    line_buffer[r1] = 13;
	    remaining_chars = r1 + 1;	/* Count CR as part of input */

	    swi(OS_Byte, 200, old_esc_state, 0);
	}

	/* Can we send the rest of the buffer back in one go? */
	if (remaining_chars <= maxlen)
	{
	    int	    got = remaining_chars;

	    memcpy(buf, line_buffer + buf_pos, got);
	    remaining_chars = 0;
	    return  got;
	}

	/* Send as much as we can */
	memcpy(buf, line_buffer + buf_pos, maxlen);
	buf_pos += maxlen;
	remaining_chars -= maxlen;

	return maxlen;
    }

    if (!term_console)
    {
	/* Use OS_ReadC for all input.
	 * Avoids problems with remote access getting interference from
	 * the keyboard.
	 */
	if (wtime == 0)
	    return 0;	    /* Ignore quick key checks */

	if (xswi(OS_ReadC) & c_flag)
	{
	    got_int = TRUE;	/* ESC pressed - can this happen? */
	    swi(OS_Byte, 124);	/* Clear Escape state */
	    r0 = 0x1b;		/* It *might* not have been Escape! */
	}
	buf[0] = r0;
	return 1;
    }

    /*
     * OK, here's the plan:
     *
     * 1) Wait until wtime expires or we get a key
     * 2) Get keys until the keyboard buffer is empty or buf is full
     */

    while (xswi(OS_Byte,145,0) & c_flag)
    {
	/* Nothing at all in the keyboard buffer.
	 * Has our time expired yet?
	 */
	if ( (wtime != -1) && (clock() - start_time) >= wtime )
	    return 0;		/* Nothing read - giving up */
    }

    /* We've got one char (in r2) - are there any more? */

    while (got < maxlen)
    {
	buf[got++] = r2;

	if (xswi(OS_Byte,145,0) & c_flag)
	    return got;		/* Keyboard buffer empty */
    }
    return got;			/* buf is full */
}

/*
 * return non-zero if a character is available
 */
    int
mch_char_avail()
{
    if (!term_console)
	return 0;	    /* Can't tell */
    if (xswi(OS_Byte, 152, 0) & c_flag)
	return 0;
    return 1;
}

/* Find out how much free memory we have.
 * I don't know how to work this out exactly but, since we can claim
 * more memory from the OS, let's just report the free pool size.
 * Dynamic area 6 doesn't exist pre 3.6 according to StrongHelp, so
 * we'll use Wimp_SlotSize. If that fails (outside the desktop?)
 * then just return a big number and hope.
 */
    long_u
mch_avail_mem(special)
    int special;
{
    if (xswi(Wimp_SlotSize, -1, -1) & v_flag)
	return 0x7fffffff;
    return r2;
}

    void
mch_delay(msec, ignoreinput)
    long	msec;
    int		ignoreinput;
{
    int		start_time, time_now;
    int		csec = msec / 10;

    swi(OS_ReadMonotonicTime);
    start_time = r0;

    for (;;)
    {
	swi(OS_ReadMonotonicTime);
	time_now = r0;
	if (time_now - start_time > csec)
	    return;
#ifdef FEAT_GUI
	/* In the GUI, allow other programs to run while waiting. */
	if (gui.in_use)
	    gui_mch_wait_for_chars(start_time + csec);
#endif
    }
}

/*
 * If the machine has job control, use it to suspend the program,
 * otherwise fake it by starting a new shell.
 */
    void
mch_suspend()
{
    suspend_shell();
}

    void
mch_init()
{
    /*
     * Read window size first. Calls to mch_get_shellsize() will
     * simply return these values in future so that setting the
     * text window (used for scrolling) won't give strange results.
     */

    int buf[7] = {132, 135, 256, 257, 1, 2, -1};

    /* Command windows are no longer forced open, since if we are
     * in the desktop then we'll use the GUI version.
     * Opening a command window here messes up the GUI version startup
     */
#ifndef FEAT_GUI
    swi(OS_WriteI);
#endif
    swi(OS_ReadVduVariables, buf, buf);
    WinLeft = buf[0];
    WinTop  = buf[1];
    Columns = buf[2];
    Rows    = buf[3] + 1;	/* Seems to be one off (VduVars wrong?) */
    ScrollTop = 0;

    /* Are we running in a textwindow? */
    if (Rows == buf[5] + 1 && Columns == buf[4] + 1)
	windowed = 0;
    else
	windowed = 1;

    /* Choose a nice colour scheme. */
    text_fg(NORMAL_FG);
    text_bg(NORMAL_BG);
}

/*
 * Check_win checks whether we have an interactive stdout.
 */
/* ARGSUSED */
    int
mch_check_win(argc, argv)
    int	    argc;
    char    **argv;
{
    return OK;
}

/*
 * Return TRUE if the input comes from a terminal, FALSE otherwise.
 */
    int
mch_input_isatty()
{
    if (xswi(OS_ChangeRedirection, -1, -1) & v_flag)
	return TRUE;		/* Error - TRUE is probably correct though */
    if (r0 == 0)
	return TRUE;
    return FALSE;
}

#ifdef FEAT_TITLE
    int
mch_can_restore_title()
{
    return FALSE;
}

    int
mch_can_restore_icon()
{
    return FALSE;
}


/*
 * Set the window title and icon.
 */
    void
mch_settitle(title, icon)
    char_u *title;
    char_u *icon;
{
    if (title == NULL)
	title = (char_u *) "<untitled>";
#ifdef FEAT_GUI
    if (gui.in_use && strcmp(title, gui.window_title))
    {
	int length;
	length = strlen(title);
	if (length >= gui.window_title_size)
	    length = gui.window_title_size - 1;
	strncpy(gui.window_title, title, length);
	gui.window_title[length] = 0;
	ro_redraw_title(gui.window_handle);
    }
#endif
    return;
}

/*
 * Restore the window/icon title.
 * "which" is one of:
 *  1  only restore title
 *  2  only restore icon
 *  3  restore title and icon
 */
    void
mch_restore_title(which)
    int which;
{
    return;
}
#endif

/*
 * Insert user name in s[len].
 * Return OK if a name found.
 */
    int
mch_get_user_name(s, len)
    char_u  *s;
    int	    len;
{
    /* RISC OS doesn't support user names. */
    *s = NUL;
    return FAIL;
}

/*
 * Insert host name in s[len].
 */

    void
mch_get_host_name(s, len)
    char_u  *s;
    int	    len;
{
    if (xswi(OS_ReadVarVal, "Machine$Name", s, len, 0, 3) & v_flag)
    {
	/* Variable does not exist (normal operation) */
	vim_strncpy(s, "(unknown)", len - 1);
    }
}

/*
 * return process ID
 */
    long
mch_get_pid()
{
    if (xswi(Wimp_ReadSysInfo, 5) & v_flag)
	return 0;
    return r0;
}

/*
 * Get name of current directory into buffer 'buf' of length 'len' bytes.
 * Return OK for success, FAIL for failure.
 */
    int
mch_dirname(buf, len)
    char_u  *buf;
    int	    len;
{
    if (xswi(OS_FSControl, 37, "@", buf, 0, 0, len) & v_flag)
	return FAIL;
    return OK;
}

/*
 * Get absolute file name into buffer 'buf' of length 'len' bytes.
 *
 * return FAIL for failure, OK for success
 */
    int
mch_FullName(fname, buf, len, force)
    char_u *fname, *buf;
    int len;
    int	force;		/* Also expand when already absolute path name.
			 * Not used under RISC OS.
			 */
{
    if (xswi(OS_FSControl, 37, fname, buf, 0, 0, len) & v_flag)
	return FAIL;
    return OK;
}

/*
 * Return TRUE if "fname" does not depend on the current directory.
 */
    int
mch_isFullName(fname)
    char_u	*fname;
{
    if (strstr(fname, "::") && strstr(fname,".$."))
	return TRUE;
    return FALSE;
}

/*
 * Get file permissions for 'name'.
 * Returns -1 when it doesn't exist.
 */
    long
mch_getperm(name)
    char_u *name;
{
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
    char_u  *name;
    long    perm;
{
    return (chmod((char *)name, (mode_t)perm) == 0 ? OK : FAIL);
}

/*
 * Set hidden flag for "name".
 */
/* ARGSUSED */
    void
mch_hide(name)
    char_u	*name;
{
    /* can't hide a file */
}

/*
 * return TRUE if "name" is a directory
 * return FALSE if "name" is not a directory
 * return FALSE for error
 */
    int
mch_isdir(name)
    char_u *name;
{
    if (xswi(OS_File, 17, name) & v_flag)
	return FALSE;
    if (r0 == 2 || r0 == 3)
	return TRUE;		/* Count image files as directories. */
    return FALSE;
}

/*
 * Return 1 if "name" can be executed, 0 if not.
 * Return -1 if unknown. Requires which to work.
 */
    int
mch_can_exe(name)
    char_u	*name;
{
    char_u	*buf;
    char_u	*p;
    int		retval;

    buf = alloc((unsigned)STRLEN(name) + 7);
    if (buf == NULL)
	return -1;
    sprintf((char *)buf, "which %s", name);
    p = get_cmd_output(buf, NULL, SHELL_SILENT);
    vim_free(buf);
    if (p == NULL)
	return -1;
    /* result can be: "name: Command not found" */
    retval = (*p != NUL && strstr((char *)p, "not found") == NULL);
    vim_free(p);
    return retval;
}

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
    /* Turn off all the horrible filename munging in UnixLib. */
    int __riscosify_control = __RISCOSIFY_NO_PROCESS;
}

    void
mch_exit(r)
    int r;
{
    settmode(TMODE_COOK);
    exiting = TRUE;
    out_flush();
    ml_close_all(TRUE);		/* remove all memfiles */

#ifdef FEAT_GUI
    if (gui.in_use)
	gui_exit(r);
#endif
    swi(OS_NewLine);
    if (old_escape_state != -1)
	swi(OS_Byte, 229, old_escape_state, 0);
    if (old_cursor_state != -1)
	swi(OS_Byte, 4, old_cursor_state);
    exit(r);
}

    void
mch_settmode(tmode)
    int		tmode;	    /* TMODE_RAW or TMODE_COOK */
{
    if (tmode == TMODE_COOK)
    {
	ro_line_mode = TRUE;
	return;
    }

    ro_line_mode = FALSE;

    if (term_console)
    {
	/* Block cursor. */
	swi(OS_WriteN,
		"\027\000\012\000\000\000\000\000\000\000",
		10);

	/* Disable the standard cursor key actions. */
	swi(OS_Byte, 4, 1);
	if (old_cursor_state == -1)
	    old_cursor_state = r1;
    }

    /* Stop Escape from quitting Vim! */
    swi(OS_Byte, 229, 1, 0);
    if (old_escape_state == -1)
	old_escape_state = r1;
}

/*
 * set mouse clicks on or off (only works for xterms)
 */
    void
mch_setmouse(on)
    int	    on;
{
}

/*
 * set screen mode, always fails.
 */
/* ARGSUSED */
    int
mch_screenmode(arg)
    char_u   *arg;
{
    EMSG(_(e_screenmode));
    return FAIL;
}

/*
 * Try to get the current window size.
 * Return OK when size could be determined, FAIL otherwise.
 * Simply return results stored by mch_init() if we are the
 * machine's console. If not, we don't know how big the screen is.
 */
    int
mch_get_shellsize()
{
    /* if size changed: screenalloc will allocate new screen buffers */
    return term_console ? OK : FAIL;
}

/*
 * Can't change the size.
 * Assume the user knows what he's doing and use the new values.
 */
    void
mch_set_shellsize()
{
    /* Assume the user knows what he's doing and use the new values. */
}

/*
 * Rows and/or Columns has changed.
 */
    void
mch_new_shellsize()
{
    /* Nothing to do. */
}

    int
mch_call_shell(cmd, options)
    char_u	*cmd;
    int		options;	/* SHELL_*, see vim.h */
{
    int		retval;
    int		tmode = cur_tmode;

    if (cmd == NULL)
	cmd = (char_u *) "GOS";

#ifdef FEAT_GUI
    if (gui.in_use)
	return gui_mch_call_shell(cmd, options);
#endif
    if (options & SHELL_COOKED)
	settmode(TMODE_COOK);		/* set to normal mode */
    MSG_PUTS("\n");

   /* I don't even want to think about what UnixLib must
    * be doing to allow this to work...
    */
    retval = system(cmd);
    if (retval && !(options & SHELL_SILENT))
	EMSG(strerror(EOPSYS));		/* Doesn't seem to set errno? */

    swi(OS_Byte, 229, 1, 0);		/* Re-disable escape */
    if (tmode == TMODE_RAW)
	settmode(TMODE_RAW);		/* set to raw mode */
    return retval ? FAIL : OK;
}

/*
 * Check for Escape being pressed right now.
 * [ different if !term_console? ]
 */
    void
mch_breakcheck()
{
    if (xswi(OS_Byte, 121, 0xf0) & v_flag)
	return;
    if (r1 == 0xff)
    {
	got_int = TRUE;
	swi(OS_Byte, 15, 1);	/* Flush input buffer */
    }
}

/*
 * Recursively expand one path component into all matching files and/or
 * directories.
 * "path" has backslashes before chars that are not to be expanded.
 * Return the number of matches found.
 */
    int
mch_expandpath(gap, path, flags)
    garray_T	*gap;	/* Grow array for results. */
    char_u	*path;
    int		flags;	/* EW_* flags */
{
    int		got;	/* Number of matches. */
    char_u	*pattern;

   /* Plan:
    *
    * 1) Get first part of path - no wildcards
    * 2) Get next path element (wildcarded)
    * 3) Get rest of path
    *
    * If (3) is nothing then only the leaf is wildcarded - add to gap
    * Otherwise call recursively for each path in (2), passing (3)
    *
    * This is just the header function.
    */

    /* We must be able to modifiy path, so make a copy */
    pattern = vim_strsave(path);
    if (pattern == NULL)
	return 0;
    got = expand_section(gap, (char_u *)"", pattern, flags);
    vim_free(pattern);
    return got;
}

/*
 * expand_section(gap, "$.Dir1.Dir2", "ABBA*.myleaf##")
 *
 * calls expand_section(gap, "$.Dir1.Dir2.ABBA_Gold", "myleaf##")
 *   and expand_section(gap, "$.Dir1.Dir2.ABBA_Live", "myleaf##")
 *
 * If rest is just a leaf then all matches are added to gap.
 *
 * Returns number of items added to gap.
 */
    int
expand_section(gap, root, rest, flags)
    garray_T	*gap;
    char_u	*root;	/* Non-wildcarded path to search */
    char_u	*rest;	/* Wildcarded remainder of path */
    int		flags;	/* Add dirs/files/missing objects. */
{
    static char_u buf[MAXPATHL];	/* Temporary buffer. */
    char_u dir[MAXPATHL];
    int start_element = -1;		/* Start of wildcarded element */
    char_u c;
    int i;
    int got, dir_pos;
    int buflen;			/* Chars used in buf[] */
    int colon = 0;		/* Dir ends in ':' */

    buflen = strlen(root);
    STRNCPY(buf, root, buflen);	/* Copy root into buffer. */

   /*
    * Find end of nonwildcarded section.
    * Count ':' as a path sep since Vim:Bug* is a valid pathname.
    */

    for (i = 0; c = rest[i]; i++)
    {
	if (c == PATHSEP)
	{
	    start_element = i;
	    colon = 0;
	}
	if (c == ':')
	{
	    start_element = i + 1;
	    colon = 1;
	}
	if (c == '#' || c == '*')
	    break;
    }
    if (c == 0)
	start_element = i;

   /*
    * start_element +> terminator for non-wildcarded section.
    * Transfer this bit into buf.
    */
    if (buflen + start_element + 4 >= MAXPATHL)
       return 0;			/* Buffer full */
    if (start_element >= 0)
    {
	if (*root && !colon)
	    buf[buflen++] = PATHSEP;
	strncpy(buf + buflen, rest, start_element);
	buflen += start_element;
    }
    buf[buflen] = 0;

   /*
    * Did we reach the end of the string without hitting any wildcards?
    */
    if (c == 0)
    {
	/* Yes - add combined path to grow array and return. */
	addfile(gap, buf, flags);
	return 1;
    }

    if (start_element < 0 || !colon)
	start_element++;
    rest += start_element;

   /*
    * rest does contain wildcards if we get here.
    *
    * Now : have we reached the leaf names part yet?
    * If so, add all matches (files and dirs) to gap.
    * If not, get next path element and scan all matching directories.
    */

    start_element = -1;
    for (i = 0; rest[i]; i++)
    {
	if (rest[i] == '.')
	{
	    start_element = i;
	    rest[i] = 0;		/* Break string here. */
	    break;
	}
    }

    /* If start_element is -1 then we are matching leaf names */

    r3 = 0;			/* Number of objs read. */
    dir_pos = 0;		/* Position through directory. */
    got = 0;			/* Files added so far. */
    while (dir_pos != -1)
    {
	buf[buflen] = 0;
	if (xswi(OS_GBPB, 9,
		buf,				/* Directory to scan. */
		buf + buflen + (1 - colon),	/* Buffer for result. */
		1,			/* Number of objects to read. */
		dir_pos,		/* Search position. */
		MAXPATHL - 2 - buflen,	/* Size of result buffer. */
		rest)			/* Wildcarded leafname. */
			& v_flag)
	{
	    EMSG(r0 + 4);
	    r4 = -1;
	}
	dir_pos = r4;		/* r4 corrupted by addfile() */
	if (r3 > 0)
	{
	    char_u *path = buf;
	    if (buflen == 0)
		path++;			/* Don't do '.File' */
	    else if (!colon)
		buf[buflen] = '.';		/* Join path and leaf */

	   /* Path -> full path of object found */
	    if (start_element == -1)
	    {
		addfile(gap, path, flags);
		got++;
	    }
	    else
	    {
	       /* Scan into subdirectories and images; ignore files */
		swi(OS_File, 17, path);
		if (r0 == 2 || r0 == 3)
		    got += expand_section(gap,
						path,
						rest + start_element + 1,
						flags);
	    }
	}
    }

    /* Restore the dot if we removed it. */
    if (start_element >= 0)
	rest[start_element] = '.';
    return got;
}

/*
 * mch_expand_wildcards() - this code does wild-card pattern matching using
 * the shell. It isn't used under RISC OS.
 *
 * return OK for success, FAIL for error (you may lose some memory) and put
 * an error message in *file.
 *
 * num_pat is number of input patterns
 * pat is array of pointers to input patterns
 * num_file is pointer to number of matched file names
 * file is pointer to array of pointers to matched file names
 */
    int
mch_expand_wildcards(num_pat, pat, num_file, file, flags)
    int		    num_pat;
    char_u	  **pat;
    int		   *num_file;
    char_u	 ***file;
    int		    flags;		/* EW_* flags */
{
    /* This doesn't get called unless SPECIAL_WILDCHAR is defined. */
    return FAIL;
}

/*
 * Return TRUE if "p" contains wildcards which can be expanded by
 * mch_expandpath().
 */
    int
mch_has_exp_wildcard(p)
    char_u	*p;
{
    if (vim_strpbrk((char_u *)"*#", p))
	return TRUE;
    return FALSE;
}

/* Return TRUE if "p" contains wildcards. */
    int
mch_has_wildcard(p)
    char_u	*p;
{
    if (vim_strpbrk((char_u *)"*#`", p))
	return TRUE;
    return FALSE;
}

    int			/* see Unix unlink(2) */
mch_remove(file)
    char_u *file;	/* Name of file to delete. */
{
    if (xswi(OS_FSControl, 27, file, 0, 0) & v_flag)
	return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

/* Try to make existing scripts work without modification.
 * Return a pointer to the new string (freed by caller), or NULL
 *
 * Two main cases:
 * - Absolute : $VIM/syntax/help.vim
 * - Relative : Adfs::4.$.!Vim.Resources.Syntax/help.vim
 */
    char_u *
mch_munge_fname(fname)
    char_u *fname;
{
    char_u c;
    int len;
    char_u *retval;

    retval = fname = vim_strsave(fname);
    if (fname == NULL)
	return NULL;

    if (strncmp(fname, "$VIM/", 5) == 0)
    {
	strncpy(fname, "Vim:", 4);
	for (fname += 5; c = *fname; fname++)
	{
	    if (c == '.')
		break;
	    if (c == '/')
		fname[-1] = '.';
	    else
		fname[-1] = c;
	}
	fname[-1] = '\0';
    }
    else
    {
	/* Check to see if the file exists without modification. */
	if (xswi(OS_File, 17, fname) & v_flag)
	    r0 == 0;		/* Invalid filename? */
	if (r0)
	    return retval;

	len = strlen(fname);
	if (strcmp(fname + len - 4, ".vim") == 0)
	{
	    fname[len - 4] = '\0';
	    for (; c = *fname; fname++)
	    {
		if (c == '/')
		    *fname = '.';
	    }
	}
    }
    return retval;
}

/* QuickFix reads munged names from the error file.
 * Correct them.
 */
    int
ro_buflist_add(old_name)
    char_u  *old_name;	/* Name of file found by quickfix */
{
    char_u  *fname;
    char_u  *leaf;	/* Pointer to start of leaf in old_name */
    char_u  *ptr;
    char_u  c;
    int	    retval;

    if (old_name == NULL)
	return buflist_add(NULL, 0);

    /* Copy the name so we can mess around with it. */
    fname = vim_strsave(old_name);
    if (fname == NULL)
	/* Out of memory - can't modify name */
	return buflist_add(old_name, 0);

    /* Change `dir/main.c' into `dir.c.main' */
    leaf = fname;
    for (ptr = fname; c = *ptr; ptr++)
    {
	if (c == '/')
	{
	    leaf = ptr + 1;
	    *ptr = '.';
	}
	else if (c == '.')
	    break;
    }
    if (c == '.')
    {
	/* Change `main.c' into `c.main'
	 *	  |    |
	 *      leaf  ptr
	 */
	ptr += old_name - fname;
	*ptr = '\0';
	sprintf(leaf,
		"%s.%s",
		ptr + 1,
		leaf - fname + old_name);
    }

    retval = buflist_add(fname, 0);
    free(fname);
    return retval;
}

/* Change the current directory.
 * Strip trailing dots to make it easier to use with filename completion.
 * Return 0 for success, -1 for failure.
 */
    int
mch_chdir(dir)
    char_u  *dir;
{
    int	    length;
    int	    retval;
    char_u  *new_dir;

    length = strlen(dir);
    if (dir[length - 1] != '.')
	return chdir(dir);	    /* No trailing dots - nothing to do. */
    new_dir = vim_strsave(dir);
    if (new_dir == NULL)
	return chdir(dir);	    /* Can't allocate memory. */

    while (new_dir[--length] == '.')
	new_dir[length] = '\0';

    retval = chdir(new_dir);
    vim_free(new_dir);
    return retval;
}

/* Examine the named file, and set the 'osfiletype' option
 * (in curbuf) to the file's type.
 */
    void
mch_read_filetype(file)
    char_u  *file;
{
    int	    type;
    char_u  type_string[9];
    int	    i;

    if (xswi(OS_File, 23, file) & v_flag)
	type = 0xfff;		/* Default to Text */
    else
	type = r6;

    /* Type is the numerical value - see if we have a textual equivalent */
    swi(OS_FSControl, 18, 0, type);
    ((int *) type_string)[0] = r2;
    ((int *) type_string)[1] = r3;
    type_string[8] = 0;
    for (i = 0; type_string[i] > ' '; i++)
	;
    type_string[i] = 0;

    set_string_option_direct("osfiletype", -1, type_string, OPT_FREE, 0);
    return;
}

    void
mch_set_filetype(file, type)
    char_u  *file;
    char_u  *type;
{
    if (xswi(OS_FSControl, 31, type) & v_flag)
    {
	EMSG(_("E366: Invalid 'osfiletype' option - using Text"));
	r2 = 0xfff;
    }

    swi(OS_File, 18, file, r2);
}

/* Return TRUE if the file's type matches 'type'
 * RISC OS types always start with '&'
 */
    int
mch_check_filetype(fname, type)
    char_u  *fname;
    char_u  *type;
{
    int	    value;
    char    *end;

    if (*type != '&')
	return FALSE;

    value = strtol(type + 1, &end, 16);
    if (*end)
	return FALSE;		/* Invalid type (report error?) */

    if (xswi(OS_File, 23, fname) & v_flag)
	return FALSE;		/* Invalid filename? */

    return (r0 && r6 == value);
}
