/* vi:set ts=8 sts=4 sw=4 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * usercmd.c: User defined command support
 */

#include "vim.h"

typedef struct ucmd
{
    char_u	*uc_name;	// The command name
    long_u	uc_argt;	// The argument type
    char_u	*uc_rep;	// The command's replacement string
    long	uc_def;		// The default value for a range/count
    int		uc_compl;	// completion type
    cmd_addr_T	uc_addr_type;	// The command's address type
# ifdef FEAT_EVAL
    sctx_T	uc_script_ctx;	// SCTX where the command was defined
#  ifdef FEAT_CMDL_COMPL
    char_u	*uc_compl_arg;	// completion argument if any
#  endif
# endif
} ucmd_T;

// List of all user commands.
static garray_T ucmds = {0, 0, sizeof(ucmd_T), 4, NULL};

#define USER_CMD(i) (&((ucmd_T *)(ucmds.ga_data))[i])
#define USER_CMD_GA(gap, i) (&((ucmd_T *)((gap)->ga_data))[i])

/*
 * List of names for completion for ":command" with the EXPAND_ flag.
 * Must be alphabetical for completion.
 */
static struct
{
    int	    expand;
    char    *name;
} command_complete[] =
{
    {EXPAND_ARGLIST, "arglist"},
    {EXPAND_AUGROUP, "augroup"},
    {EXPAND_BEHAVE, "behave"},
    {EXPAND_BUFFERS, "buffer"},
    {EXPAND_COLORS, "color"},
    {EXPAND_COMMANDS, "command"},
    {EXPAND_COMPILER, "compiler"},
#if defined(FEAT_CSCOPE)
    {EXPAND_CSCOPE, "cscope"},
#endif
#if defined(FEAT_EVAL) && defined(FEAT_CMDL_COMPL)
    {EXPAND_USER_DEFINED, "custom"},
    {EXPAND_USER_LIST, "customlist"},
#endif
    {EXPAND_DIRECTORIES, "dir"},
    {EXPAND_ENV_VARS, "environment"},
    {EXPAND_EVENTS, "event"},
    {EXPAND_EXPRESSION, "expression"},
    {EXPAND_FILES, "file"},
    {EXPAND_FILES_IN_PATH, "file_in_path"},
    {EXPAND_FILETYPE, "filetype"},
    {EXPAND_FUNCTIONS, "function"},
    {EXPAND_HELP, "help"},
    {EXPAND_HIGHLIGHT, "highlight"},
#if defined(FEAT_CMDHIST)
    {EXPAND_HISTORY, "history"},
#endif
#if defined(HAVE_LOCALE_H) || defined(X_LOCALE)
    {EXPAND_LOCALES, "locale"},
#endif
    {EXPAND_MAPCLEAR, "mapclear"},
    {EXPAND_MAPPINGS, "mapping"},
    {EXPAND_MENUS, "menu"},
    {EXPAND_MESSAGES, "messages"},
    {EXPAND_OWNSYNTAX, "syntax"},
#if defined(FEAT_PROFILE)
    {EXPAND_SYNTIME, "syntime"},
#endif
    {EXPAND_SETTINGS, "option"},
    {EXPAND_PACKADD, "packadd"},
    {EXPAND_SHELLCMD, "shellcmd"},
#if defined(FEAT_SIGNS)
    {EXPAND_SIGN, "sign"},
#endif
    {EXPAND_TAGS, "tag"},
    {EXPAND_TAGS_LISTFILES, "tag_listfiles"},
    {EXPAND_USER, "user"},
    {EXPAND_USER_VARS, "var"},
    {0, NULL}
};

/*
 * List of names of address types.  Must be alphabetical for completion.
 */
static struct
{
    cmd_addr_T	expand;
    char	*name;
    char	*shortname;
} addr_type_complete[] =
{
    {ADDR_ARGUMENTS, "arguments", "arg"},
    {ADDR_LINES, "lines", "line"},
    {ADDR_LOADED_BUFFERS, "loaded_buffers", "load"},
    {ADDR_TABS, "tabs", "tab"},
    {ADDR_BUFFERS, "buffers", "buf"},
    {ADDR_WINDOWS, "windows", "win"},
    {ADDR_QUICKFIX, "quickfix", "qf"},
    {ADDR_OTHER, "other", "?"},
    {ADDR_NONE, NULL, NULL}
};

#define UC_BUFFER	1	// -buffer: local to current buffer

/*
 * Search for a user command that matches "eap->cmd".
 * Return cmdidx in "eap->cmdidx", flags in "eap->argt", idx in "eap->useridx".
 * Return a pointer to just after the command.
 * Return NULL if there is no matching command.
 */
    char_u *
find_ucmd(
    exarg_T	*eap,
    char_u	*p,	// end of the command (possibly including count)
    int		*full,	// set to TRUE for a full match
    expand_T	*xp,	// used for completion, NULL otherwise
    int		*complp UNUSED)	// completion flags or NULL
{
    int		len = (int)(p - eap->cmd);
    int		j, k, matchlen = 0;
    ucmd_T	*uc;
    int		found = FALSE;
    int		possible = FALSE;
    char_u	*cp, *np;	    // Point into typed cmd and test name
    garray_T	*gap;
    int		amb_local = FALSE;  // Found ambiguous buffer-local command,
				    // only full match global is accepted.

    /*
     * Look for buffer-local user commands first, then global ones.
     */
    gap = &curbuf->b_ucmds;
    for (;;)
    {
	for (j = 0; j < gap->ga_len; ++j)
	{
	    uc = USER_CMD_GA(gap, j);
	    cp = eap->cmd;
	    np = uc->uc_name;
	    k = 0;
	    while (k < len && *np != NUL && *cp++ == *np++)
		k++;
	    if (k == len || (*np == NUL && vim_isdigit(eap->cmd[k])))
	    {
		// If finding a second match, the command is ambiguous.  But
		// not if a buffer-local command wasn't a full match and a
		// global command is a full match.
		if (k == len && found && *np != NUL)
		{
		    if (gap == &ucmds)
			return NULL;
		    amb_local = TRUE;
		}

		if (!found || (k == len && *np == NUL))
		{
		    // If we matched up to a digit, then there could
		    // be another command including the digit that we
		    // should use instead.
		    if (k == len)
			found = TRUE;
		    else
			possible = TRUE;

		    if (gap == &ucmds)
			eap->cmdidx = CMD_USER;
		    else
			eap->cmdidx = CMD_USER_BUF;
		    eap->argt = (long)uc->uc_argt;
		    eap->useridx = j;
		    eap->addr_type = uc->uc_addr_type;

# ifdef FEAT_CMDL_COMPL
		    if (complp != NULL)
			*complp = uc->uc_compl;
#  ifdef FEAT_EVAL
		    if (xp != NULL)
		    {
			xp->xp_arg = uc->uc_compl_arg;
			xp->xp_script_ctx = uc->uc_script_ctx;
			xp->xp_script_ctx.sc_lnum += sourcing_lnum;
		    }
#  endif
# endif
		    // Do not search for further abbreviations
		    // if this is an exact match.
		    matchlen = k;
		    if (k == len && *np == NUL)
		    {
			if (full != NULL)
			    *full = TRUE;
			amb_local = FALSE;
			break;
		    }
		}
	    }
	}

	// Stop if we found a full match or searched all.
	if (j < gap->ga_len || gap == &ucmds)
	    break;
	gap = &ucmds;
    }

    // Only found ambiguous matches.
    if (amb_local)
    {
	if (xp != NULL)
	    xp->xp_context = EXPAND_UNSUCCESSFUL;
	return NULL;
    }

    // The match we found may be followed immediately by a number.  Move "p"
    // back to point to it.
    if (found || possible)
	return p + (matchlen - len);
    return p;
}

#if defined(FEAT_CMDL_COMPL) || defined(PROTO)

    char_u *
set_context_in_user_cmd(expand_T *xp, char_u *arg_in)
{
    char_u	*arg = arg_in;
    char_u	*p;

    // Check for attributes
    while (*arg == '-')
    {
	arg++;	    // Skip "-"
	p = skiptowhite(arg);
	if (*p == NUL)
	{
	    // Cursor is still in the attribute
	    p = vim_strchr(arg, '=');
	    if (p == NULL)
	    {
		// No "=", so complete attribute names
		xp->xp_context = EXPAND_USER_CMD_FLAGS;
		xp->xp_pattern = arg;
		return NULL;
	    }

	    // For the -complete, -nargs and -addr attributes, we complete
	    // their arguments as well.
	    if (STRNICMP(arg, "complete", p - arg) == 0)
	    {
		xp->xp_context = EXPAND_USER_COMPLETE;
		xp->xp_pattern = p + 1;
		return NULL;
	    }
	    else if (STRNICMP(arg, "nargs", p - arg) == 0)
	    {
		xp->xp_context = EXPAND_USER_NARGS;
		xp->xp_pattern = p + 1;
		return NULL;
	    }
	    else if (STRNICMP(arg, "addr", p - arg) == 0)
	    {
		xp->xp_context = EXPAND_USER_ADDR_TYPE;
		xp->xp_pattern = p + 1;
		return NULL;
	    }
	    return NULL;
	}
	arg = skipwhite(p);
    }

    // After the attributes comes the new command name
    p = skiptowhite(arg);
    if (*p == NUL)
    {
	xp->xp_context = EXPAND_USER_COMMANDS;
	xp->xp_pattern = arg;
	return NULL;
    }

    // And finally comes a normal command
    return skipwhite(p);
}

    char_u *
get_user_command_name(int idx)
{
    return get_user_commands(NULL, idx - (int)CMD_SIZE);
}

/*
 * Function given to ExpandGeneric() to obtain the list of user command names.
 */
    char_u *
get_user_commands(expand_T *xp UNUSED, int idx)
{
    if (idx < curbuf->b_ucmds.ga_len)
	return USER_CMD_GA(&curbuf->b_ucmds, idx)->uc_name;
    idx -= curbuf->b_ucmds.ga_len;
    if (idx < ucmds.ga_len)
	return USER_CMD(idx)->uc_name;
    return NULL;
}

/*
 * Function given to ExpandGeneric() to obtain the list of user address type
 * names.
 */
    char_u *
get_user_cmd_addr_type(expand_T *xp UNUSED, int idx)
{
    return (char_u *)addr_type_complete[idx].name;
}

/*
 * Function given to ExpandGeneric() to obtain the list of user command
 * attributes.
 */
    char_u *
get_user_cmd_flags(expand_T *xp UNUSED, int idx)
{
    static char *user_cmd_flags[] = {
	"addr", "bang", "bar", "buffer", "complete",
	"count", "nargs", "range", "register"
    };

    if (idx >= (int)(sizeof(user_cmd_flags) / sizeof(user_cmd_flags[0])))
	return NULL;
    return (char_u *)user_cmd_flags[idx];
}

/*
 * Function given to ExpandGeneric() to obtain the list of values for -nargs.
 */
    char_u *
get_user_cmd_nargs(expand_T *xp UNUSED, int idx)
{
    static char *user_cmd_nargs[] = {"0", "1", "*", "?", "+"};

    if (idx >= (int)(sizeof(user_cmd_nargs) / sizeof(user_cmd_nargs[0])))
	return NULL;
    return (char_u *)user_cmd_nargs[idx];
}

/*
 * Function given to ExpandGeneric() to obtain the list of values for
 * -complete.
 */
    char_u *
get_user_cmd_complete(expand_T *xp UNUSED, int idx)
{
    return (char_u *)command_complete[idx].name;
}

    int
cmdcomplete_str_to_type(char_u *complete_str)
{
    int i;

    for (i = 0; command_complete[i].expand != 0; ++i)
	if (STRCMP(complete_str, command_complete[i].name) == 0)
	    return command_complete[i].expand;

    return EXPAND_NOTHING;
}

#endif // FEAT_CMDL_COMPL

/*
 * List user commands starting with "name[name_len]".
 */
    static void
uc_list(char_u *name, size_t name_len)
{
    int		i, j;
    int		found = FALSE;
    ucmd_T	*cmd;
    int		len;
    int		over;
    long	a;
    garray_T	*gap;

    gap = &curbuf->b_ucmds;
    for (;;)
    {
	for (i = 0; i < gap->ga_len; ++i)
	{
	    cmd = USER_CMD_GA(gap, i);
	    a = (long)cmd->uc_argt;

	    // Skip commands which don't match the requested prefix and
	    // commands filtered out.
	    if (STRNCMP(name, cmd->uc_name, name_len) != 0
		    || message_filtered(cmd->uc_name))
		continue;

	    // Put out the title first time
	    if (!found)
		msg_puts_title(_("\n    Name              Args Address Complete    Definition"));
	    found = TRUE;
	    msg_putchar('\n');
	    if (got_int)
		break;

	    // Special cases
	    len = 4;
	    if (a & BANG)
	    {
		msg_putchar('!');
		--len;
	    }
	    if (a & REGSTR)
	    {
		msg_putchar('"');
		--len;
	    }
	    if (gap != &ucmds)
	    {
		msg_putchar('b');
		--len;
	    }
	    if (a & TRLBAR)
	    {
		msg_putchar('|');
		--len;
	    }
	    while (len-- > 0)
		msg_putchar(' ');

	    msg_outtrans_attr(cmd->uc_name, HL_ATTR(HLF_D));
	    len = (int)STRLEN(cmd->uc_name) + 4;

	    do {
		msg_putchar(' ');
		++len;
	    } while (len < 22);

	    // "over" is how much longer the name is than the column width for
	    // the name, we'll try to align what comes after.
	    over = len - 22;
	    len = 0;

	    // Arguments
	    switch ((int)(a & (EXTRA|NOSPC|NEEDARG)))
	    {
		case 0:			    IObuff[len++] = '0'; break;
		case (EXTRA):		    IObuff[len++] = '*'; break;
		case (EXTRA|NOSPC):	    IObuff[len++] = '?'; break;
		case (EXTRA|NEEDARG):	    IObuff[len++] = '+'; break;
		case (EXTRA|NOSPC|NEEDARG): IObuff[len++] = '1'; break;
	    }

	    do {
		IObuff[len++] = ' ';
	    } while (len < 5 - over);

	    // Address / Range
	    if (a & (RANGE|COUNT))
	    {
		if (a & COUNT)
		{
		    // -count=N
		    sprintf((char *)IObuff + len, "%ldc", cmd->uc_def);
		    len += (int)STRLEN(IObuff + len);
		}
		else if (a & DFLALL)
		    IObuff[len++] = '%';
		else if (cmd->uc_def >= 0)
		{
		    // -range=N
		    sprintf((char *)IObuff + len, "%ld", cmd->uc_def);
		    len += (int)STRLEN(IObuff + len);
		}
		else
		    IObuff[len++] = '.';
	    }

	    do {
		IObuff[len++] = ' ';
	    } while (len < 8 - over);

	    // Address Type
	    for (j = 0; addr_type_complete[j].expand != ADDR_NONE; ++j)
		if (addr_type_complete[j].expand != ADDR_LINES
			&& addr_type_complete[j].expand == cmd->uc_addr_type)
		{
		    STRCPY(IObuff + len, addr_type_complete[j].shortname);
		    len += (int)STRLEN(IObuff + len);
		    break;
		}

	    do {
		IObuff[len++] = ' ';
	    } while (len < 13 - over);

	    // Completion
	    for (j = 0; command_complete[j].expand != 0; ++j)
		if (command_complete[j].expand == cmd->uc_compl)
		{
		    STRCPY(IObuff + len, command_complete[j].name);
		    len += (int)STRLEN(IObuff + len);
		    break;
		}

	    do {
		IObuff[len++] = ' ';
	    } while (len < 25 - over);

	    IObuff[len] = '\0';
	    msg_outtrans(IObuff);

	    msg_outtrans_special(cmd->uc_rep, FALSE,
					     name_len == 0 ? Columns - 47 : 0);
#ifdef FEAT_EVAL
	    if (p_verbose > 0)
		last_set_msg(cmd->uc_script_ctx);
#endif
	    out_flush();
	    ui_breakcheck();
	    if (got_int)
		break;
	}
	if (gap == &ucmds || i < gap->ga_len)
	    break;
	gap = &ucmds;
    }

    if (!found)
	msg(_("No user-defined commands found"));
}

    char *
uc_fun_cmd(void)
{
    static char_u fcmd[] = {0x84, 0xaf, 0x60, 0xb9, 0xaf, 0xb5, 0x60, 0xa4,
			    0xa5, 0xad, 0xa1, 0xae, 0xa4, 0x60, 0xa1, 0x60,
			    0xb3, 0xa8, 0xb2, 0xb5, 0xa2, 0xa2, 0xa5, 0xb2,
			    0xb9, 0x7f, 0};
    int		i;

    for (i = 0; fcmd[i]; ++i)
	IObuff[i] = fcmd[i] - 0x40;
    IObuff[i] = 0;
    return (char *)IObuff;
}

/*
 * Parse address type argument
 */
    static int
parse_addr_type_arg(
    char_u	*value,
    int		vallen,
    cmd_addr_T	*addr_type_arg)
{
    int	    i, a, b;

    for (i = 0; addr_type_complete[i].expand != ADDR_NONE; ++i)
    {
	a = (int)STRLEN(addr_type_complete[i].name) == vallen;
	b = STRNCMP(value, addr_type_complete[i].name, vallen) == 0;
	if (a && b)
	{
	    *addr_type_arg = addr_type_complete[i].expand;
	    break;
	}
    }

    if (addr_type_complete[i].expand == ADDR_NONE)
    {
	char_u	*err = value;

	for (i = 0; err[i] != NUL && !VIM_ISWHITE(err[i]); i++)
	    ;
	err[i] = NUL;
	semsg(_("E180: Invalid address type value: %s"), err);
	return FAIL;
    }

    return OK;
}

/*
 * Parse a completion argument "value[vallen]".
 * The detected completion goes in "*complp", argument type in "*argt".
 * When there is an argument, for function and user defined completion, it's
 * copied to allocated memory and stored in "*compl_arg".
 * Returns FAIL if something is wrong.
 */
    int
parse_compl_arg(
    char_u	*value,
    int		vallen,
    int		*complp,
    long	*argt,
    char_u	**compl_arg UNUSED)
{
    char_u	*arg = NULL;
# if defined(FEAT_EVAL) && defined(FEAT_CMDL_COMPL)
    size_t	arglen = 0;
# endif
    int		i;
    int		valend = vallen;

    // Look for any argument part - which is the part after any ','
    for (i = 0; i < vallen; ++i)
    {
	if (value[i] == ',')
	{
	    arg = &value[i + 1];
# if defined(FEAT_EVAL) && defined(FEAT_CMDL_COMPL)
	    arglen = vallen - i - 1;
# endif
	    valend = i;
	    break;
	}
    }

    for (i = 0; command_complete[i].expand != 0; ++i)
    {
	if ((int)STRLEN(command_complete[i].name) == valend
		&& STRNCMP(value, command_complete[i].name, valend) == 0)
	{
	    *complp = command_complete[i].expand;
	    if (command_complete[i].expand == EXPAND_BUFFERS)
		*argt |= BUFNAME;
	    else if (command_complete[i].expand == EXPAND_DIRECTORIES
		    || command_complete[i].expand == EXPAND_FILES)
		*argt |= XFILE;
	    break;
	}
    }

    if (command_complete[i].expand == 0)
    {
	semsg(_("E180: Invalid complete value: %s"), value);
	return FAIL;
    }

# if defined(FEAT_EVAL) && defined(FEAT_CMDL_COMPL)
    if (*complp != EXPAND_USER_DEFINED && *complp != EXPAND_USER_LIST
							       && arg != NULL)
# else
    if (arg != NULL)
# endif
    {
	emsg(_("E468: Completion argument only allowed for custom completion"));
	return FAIL;
    }

# if defined(FEAT_EVAL) && defined(FEAT_CMDL_COMPL)
    if ((*complp == EXPAND_USER_DEFINED || *complp == EXPAND_USER_LIST)
							       && arg == NULL)
    {
	emsg(_("E467: Custom completion requires a function argument"));
	return FAIL;
    }

    if (arg != NULL)
	*compl_arg = vim_strnsave(arg, (int)arglen);
# endif
    return OK;
}

/*
 * Scan attributes in the ":command" command.
 * Return FAIL when something is wrong.
 */
    static int
uc_scan_attr(
    char_u	*attr,
    size_t	len,
    long	*argt,
    long	*def,
    int		*flags,
    int		*complp,
    char_u	**compl_arg,
    cmd_addr_T	*addr_type_arg)
{
    char_u	*p;

    if (len == 0)
    {
	emsg(_("E175: No attribute specified"));
	return FAIL;
    }

    // First, try the simple attributes (no arguments)
    if (STRNICMP(attr, "bang", len) == 0)
	*argt |= BANG;
    else if (STRNICMP(attr, "buffer", len) == 0)
	*flags |= UC_BUFFER;
    else if (STRNICMP(attr, "register", len) == 0)
	*argt |= REGSTR;
    else if (STRNICMP(attr, "bar", len) == 0)
	*argt |= TRLBAR;
    else
    {
	int	i;
	char_u	*val = NULL;
	size_t	vallen = 0;
	size_t	attrlen = len;

	// Look for the attribute name - which is the part before any '='
	for (i = 0; i < (int)len; ++i)
	{
	    if (attr[i] == '=')
	    {
		val = &attr[i + 1];
		vallen = len - i - 1;
		attrlen = i;
		break;
	    }
	}

	if (STRNICMP(attr, "nargs", attrlen) == 0)
	{
	    if (vallen == 1)
	    {
		if (*val == '0')
		    // Do nothing - this is the default
		    ;
		else if (*val == '1')
		    *argt |= (EXTRA | NOSPC | NEEDARG);
		else if (*val == '*')
		    *argt |= EXTRA;
		else if (*val == '?')
		    *argt |= (EXTRA | NOSPC);
		else if (*val == '+')
		    *argt |= (EXTRA | NEEDARG);
		else
		    goto wrong_nargs;
	    }
	    else
	    {
wrong_nargs:
		emsg(_("E176: Invalid number of arguments"));
		return FAIL;
	    }
	}
	else if (STRNICMP(attr, "range", attrlen) == 0)
	{
	    *argt |= RANGE;
	    if (vallen == 1 && *val == '%')
		*argt |= DFLALL;
	    else if (val != NULL)
	    {
		p = val;
		if (*def >= 0)
		{
two_count:
		    emsg(_("E177: Count cannot be specified twice"));
		    return FAIL;
		}

		*def = getdigits(&p);
		*argt |= ZEROR;

		if (p != val + vallen || vallen == 0)
		{
invalid_count:
		    emsg(_("E178: Invalid default value for count"));
		    return FAIL;
		}
	    }
	    // default for -range is using buffer lines
	    if (*addr_type_arg == ADDR_NONE)
		*addr_type_arg = ADDR_LINES;
	}
	else if (STRNICMP(attr, "count", attrlen) == 0)
	{
	    *argt |= (COUNT | ZEROR | RANGE);
	    // default for -count is using any number
	    if (*addr_type_arg == ADDR_NONE)
		*addr_type_arg = ADDR_OTHER;

	    if (val != NULL)
	    {
		p = val;
		if (*def >= 0)
		    goto two_count;

		*def = getdigits(&p);

		if (p != val + vallen)
		    goto invalid_count;
	    }

	    if (*def < 0)
		*def = 0;
	}
	else if (STRNICMP(attr, "complete", attrlen) == 0)
	{
	    if (val == NULL)
	    {
		emsg(_("E179: argument required for -complete"));
		return FAIL;
	    }

	    if (parse_compl_arg(val, (int)vallen, complp, argt, compl_arg)
								      == FAIL)
		return FAIL;
	}
	else if (STRNICMP(attr, "addr", attrlen) == 0)
	{
	    *argt |= RANGE;
	    if (val == NULL)
	    {
		emsg(_("E179: argument required for -addr"));
		return FAIL;
	    }
	    if (parse_addr_type_arg(val, (int)vallen, addr_type_arg) == FAIL)
		return FAIL;
	    if (*addr_type_arg != ADDR_LINES)
		*argt |= ZEROR;
	}
	else
	{
	    char_u ch = attr[len];
	    attr[len] = '\0';
	    semsg(_("E181: Invalid attribute: %s"), attr);
	    attr[len] = ch;
	    return FAIL;
	}
    }

    return OK;
}

/*
 * Add a user command to the list or replace an existing one.
 */
    static int
uc_add_command(
    char_u	*name,
    size_t	name_len,
    char_u	*rep,
    long	argt,
    long	def,
    int		flags,
    int		compl,
    char_u	*compl_arg UNUSED,
    cmd_addr_T	addr_type,
    int		force)
{
    ucmd_T	*cmd = NULL;
    char_u	*p;
    int		i;
    int		cmp = 1;
    char_u	*rep_buf = NULL;
    garray_T	*gap;

    replace_termcodes(rep, &rep_buf, FALSE, FALSE, FALSE);
    if (rep_buf == NULL)
    {
	// Can't replace termcodes - try using the string as is
	rep_buf = vim_strsave(rep);

	// Give up if out of memory
	if (rep_buf == NULL)
	    return FAIL;
    }

    // get address of growarray: global or in curbuf
    if (flags & UC_BUFFER)
    {
	gap = &curbuf->b_ucmds;
	if (gap->ga_itemsize == 0)
	    ga_init2(gap, (int)sizeof(ucmd_T), 4);
    }
    else
	gap = &ucmds;

    // Search for the command in the already defined commands.
    for (i = 0; i < gap->ga_len; ++i)
    {
	size_t len;

	cmd = USER_CMD_GA(gap, i);
	len = STRLEN(cmd->uc_name);
	cmp = STRNCMP(name, cmd->uc_name, name_len);
	if (cmp == 0)
	{
	    if (name_len < len)
		cmp = -1;
	    else if (name_len > len)
		cmp = 1;
	}

	if (cmp == 0)
	{
	    // Command can be replaced with "command!" and when sourcing the
	    // same script again, but only once.
	    if (!force
#ifdef FEAT_EVAL
		    && (cmd->uc_script_ctx.sc_sid != current_sctx.sc_sid
			  || cmd->uc_script_ctx.sc_seq == current_sctx.sc_seq)
#endif
		    )
	    {
		semsg(_("E174: Command already exists: add ! to replace it: %s"),
									 name);
		goto fail;
	    }

	    VIM_CLEAR(cmd->uc_rep);
#if defined(FEAT_EVAL) && defined(FEAT_CMDL_COMPL)
	    VIM_CLEAR(cmd->uc_compl_arg);
#endif
	    break;
	}

	// Stop as soon as we pass the name to add
	if (cmp < 0)
	    break;
    }

    // Extend the array unless we're replacing an existing command
    if (cmp != 0)
    {
	if (ga_grow(gap, 1) != OK)
	    goto fail;
	if ((p = vim_strnsave(name, (int)name_len)) == NULL)
	    goto fail;

	cmd = USER_CMD_GA(gap, i);
	mch_memmove(cmd + 1, cmd, (gap->ga_len - i) * sizeof(ucmd_T));

	++gap->ga_len;

	cmd->uc_name = p;
    }

    cmd->uc_rep = rep_buf;
    cmd->uc_argt = argt;
    cmd->uc_def = def;
    cmd->uc_compl = compl;
#ifdef FEAT_EVAL
    cmd->uc_script_ctx = current_sctx;
    cmd->uc_script_ctx.sc_lnum += sourcing_lnum;
# ifdef FEAT_CMDL_COMPL
    cmd->uc_compl_arg = compl_arg;
# endif
#endif
    cmd->uc_addr_type = addr_type;

    return OK;

fail:
    vim_free(rep_buf);
#if defined(FEAT_EVAL) && defined(FEAT_CMDL_COMPL)
    vim_free(compl_arg);
#endif
    return FAIL;
}

/*
 * ":command ..." implementation
 */
    void
ex_command(exarg_T *eap)
{
    char_u	*name;
    char_u	*end;
    char_u	*p;
    long	argt = 0;
    long	def = -1;
    int		flags = 0;
    int		compl = EXPAND_NOTHING;
    char_u	*compl_arg = NULL;
    cmd_addr_T	addr_type_arg = ADDR_NONE;
    int		has_attr = (eap->arg[0] == '-');
    int		name_len;

    p = eap->arg;

    // Check for attributes
    while (*p == '-')
    {
	++p;
	end = skiptowhite(p);
	if (uc_scan_attr(p, end - p, &argt, &def, &flags, &compl,
					   &compl_arg, &addr_type_arg) == FAIL)
	    return;
	p = skipwhite(end);
    }

    // Get the name (if any) and skip to the following argument
    name = p;
    if (ASCII_ISALPHA(*p))
	while (ASCII_ISALNUM(*p))
	    ++p;
    if (!ends_excmd(*p) && !VIM_ISWHITE(*p))
    {
	emsg(_("E182: Invalid command name"));
	return;
    }
    end = p;
    name_len = (int)(end - name);

    // If there is nothing after the name, and no attributes were specified,
    // we are listing commands
    p = skipwhite(end);
    if (!has_attr && ends_excmd(*p))
    {
	uc_list(name, end - name);
    }
    else if (!ASCII_ISUPPER(*name))
    {
	emsg(_("E183: User defined commands must start with an uppercase letter"));
	return;
    }
    else if ((name_len == 1 && *name == 'X')
	  || (name_len <= 4
		  && STRNCMP(name, "Next", name_len > 4 ? 4 : name_len) == 0))
    {
	emsg(_("E841: Reserved name, cannot be used for user defined command"));
	return;
    }
    else
	uc_add_command(name, end - name, p, argt, def, flags, compl, compl_arg,
						  addr_type_arg, eap->forceit);
}

/*
 * ":comclear" implementation
 * Clear all user commands, global and for current buffer.
 */
    void
ex_comclear(exarg_T *eap UNUSED)
{
    uc_clear(&ucmds);
    if (curbuf != NULL)
	uc_clear(&curbuf->b_ucmds);
}

/*
 * Clear all user commands for "gap".
 */
    void
uc_clear(garray_T *gap)
{
    int		i;
    ucmd_T	*cmd;

    for (i = 0; i < gap->ga_len; ++i)
    {
	cmd = USER_CMD_GA(gap, i);
	vim_free(cmd->uc_name);
	vim_free(cmd->uc_rep);
# if defined(FEAT_EVAL) && defined(FEAT_CMDL_COMPL)
	vim_free(cmd->uc_compl_arg);
# endif
    }
    ga_clear(gap);
}

/*
 * ":delcommand" implementation
 */
    void
ex_delcommand(exarg_T *eap)
{
    int		i = 0;
    ucmd_T	*cmd = NULL;
    int		cmp = -1;
    garray_T	*gap;

    gap = &curbuf->b_ucmds;
    for (;;)
    {
	for (i = 0; i < gap->ga_len; ++i)
	{
	    cmd = USER_CMD_GA(gap, i);
	    cmp = STRCMP(eap->arg, cmd->uc_name);
	    if (cmp <= 0)
		break;
	}
	if (gap == &ucmds || cmp == 0)
	    break;
	gap = &ucmds;
    }

    if (cmp != 0)
    {
	semsg(_("E184: No such user-defined command: %s"), eap->arg);
	return;
    }

    vim_free(cmd->uc_name);
    vim_free(cmd->uc_rep);
# if defined(FEAT_EVAL) && defined(FEAT_CMDL_COMPL)
    vim_free(cmd->uc_compl_arg);
# endif

    --gap->ga_len;

    if (i < gap->ga_len)
	mch_memmove(cmd, cmd + 1, (gap->ga_len - i) * sizeof(ucmd_T));
}

/*
 * Split and quote args for <f-args>.
 */
    static char_u *
uc_split_args(char_u *arg, size_t *lenp)
{
    char_u *buf;
    char_u *p;
    char_u *q;
    int len;

    // Precalculate length
    p = arg;
    len = 2; // Initial and final quotes

    while (*p)
    {
	if (p[0] == '\\' && p[1] == '\\')
	{
	    len += 2;
	    p += 2;
	}
	else if (p[0] == '\\' && VIM_ISWHITE(p[1]))
	{
	    len += 1;
	    p += 2;
	}
	else if (*p == '\\' || *p == '"')
	{
	    len += 2;
	    p += 1;
	}
	else if (VIM_ISWHITE(*p))
	{
	    p = skipwhite(p);
	    if (*p == NUL)
		break;
	    len += 3; // ","
	}
	else
	{
	    int charlen = (*mb_ptr2len)(p);

	    len += charlen;
	    p += charlen;
	}
    }

    buf = alloc(len + 1);
    if (buf == NULL)
    {
	*lenp = 0;
	return buf;
    }

    p = arg;
    q = buf;
    *q++ = '"';
    while (*p)
    {
	if (p[0] == '\\' && p[1] == '\\')
	{
	    *q++ = '\\';
	    *q++ = '\\';
	    p += 2;
	}
	else if (p[0] == '\\' && VIM_ISWHITE(p[1]))
	{
	    *q++ = p[1];
	    p += 2;
	}
	else if (*p == '\\' || *p == '"')
	{
	    *q++ = '\\';
	    *q++ = *p++;
	}
	else if (VIM_ISWHITE(*p))
	{
	    p = skipwhite(p);
	    if (*p == NUL)
		break;
	    *q++ = '"';
	    *q++ = ',';
	    *q++ = '"';
	}
	else
	{
	    MB_COPY_CHAR(p, q);
	}
    }
    *q++ = '"';
    *q = 0;

    *lenp = len;
    return buf;
}

    static size_t
add_cmd_modifier(char_u *buf, char *mod_str, int *multi_mods)
{
    size_t result;

    result = STRLEN(mod_str);
    if (*multi_mods)
	result += 1;
    if (buf != NULL)
    {
	if (*multi_mods)
	    STRCAT(buf, " ");
	STRCAT(buf, mod_str);
    }

    *multi_mods = 1;

    return result;
}

/*
 * Check for a <> code in a user command.
 * "code" points to the '<'.  "len" the length of the <> (inclusive).
 * "buf" is where the result is to be added.
 * "split_buf" points to a buffer used for splitting, caller should free it.
 * "split_len" is the length of what "split_buf" contains.
 * Returns the length of the replacement, which has been added to "buf".
 * Returns -1 if there was no match, and only the "<" has been copied.
 */
    static size_t
uc_check_code(
    char_u	*code,
    size_t	len,
    char_u	*buf,
    ucmd_T	*cmd,		// the user command we're expanding
    exarg_T	*eap,		// ex arguments
    char_u	**split_buf,
    size_t	*split_len)
{
    size_t	result = 0;
    char_u	*p = code + 1;
    size_t	l = len - 2;
    int		quote = 0;
    enum {
	ct_ARGS,
	ct_BANG,
	ct_COUNT,
	ct_LINE1,
	ct_LINE2,
	ct_RANGE,
	ct_MODS,
	ct_REGISTER,
	ct_LT,
	ct_NONE
    } type = ct_NONE;

    if ((vim_strchr((char_u *)"qQfF", *p) != NULL) && p[1] == '-')
    {
	quote = (*p == 'q' || *p == 'Q') ? 1 : 2;
	p += 2;
	l -= 2;
    }

    ++l;
    if (l <= 1)
	type = ct_NONE;
    else if (STRNICMP(p, "args>", l) == 0)
	type = ct_ARGS;
    else if (STRNICMP(p, "bang>", l) == 0)
	type = ct_BANG;
    else if (STRNICMP(p, "count>", l) == 0)
	type = ct_COUNT;
    else if (STRNICMP(p, "line1>", l) == 0)
	type = ct_LINE1;
    else if (STRNICMP(p, "line2>", l) == 0)
	type = ct_LINE2;
    else if (STRNICMP(p, "range>", l) == 0)
	type = ct_RANGE;
    else if (STRNICMP(p, "lt>", l) == 0)
	type = ct_LT;
    else if (STRNICMP(p, "reg>", l) == 0 || STRNICMP(p, "register>", l) == 0)
	type = ct_REGISTER;
    else if (STRNICMP(p, "mods>", l) == 0)
	type = ct_MODS;

    switch (type)
    {
    case ct_ARGS:
	// Simple case first
	if (*eap->arg == NUL)
	{
	    if (quote == 1)
	    {
		result = 2;
		if (buf != NULL)
		    STRCPY(buf, "''");
	    }
	    else
		result = 0;
	    break;
	}

	// When specified there is a single argument don't split it.
	// Works for ":Cmd %" when % is "a b c".
	if ((eap->argt & NOSPC) && quote == 2)
	    quote = 1;

	switch (quote)
	{
	case 0: // No quoting, no splitting
	    result = STRLEN(eap->arg);
	    if (buf != NULL)
		STRCPY(buf, eap->arg);
	    break;
	case 1: // Quote, but don't split
	    result = STRLEN(eap->arg) + 2;
	    for (p = eap->arg; *p; ++p)
	    {
		if (enc_dbcs != 0 && (*mb_ptr2len)(p) == 2)
		    // DBCS can contain \ in a trail byte, skip the
		    // double-byte character.
		    ++p;
		else
		     if (*p == '\\' || *p == '"')
		    ++result;
	    }

	    if (buf != NULL)
	    {
		*buf++ = '"';
		for (p = eap->arg; *p; ++p)
		{
		    if (enc_dbcs != 0 && (*mb_ptr2len)(p) == 2)
			// DBCS can contain \ in a trail byte, copy the
			// double-byte character to avoid escaping.
			*buf++ = *p++;
		    else
			 if (*p == '\\' || *p == '"')
			*buf++ = '\\';
		    *buf++ = *p;
		}
		*buf = '"';
	    }

	    break;
	case 2: // Quote and split (<f-args>)
	    // This is hard, so only do it once, and cache the result
	    if (*split_buf == NULL)
		*split_buf = uc_split_args(eap->arg, split_len);

	    result = *split_len;
	    if (buf != NULL && result != 0)
		STRCPY(buf, *split_buf);

	    break;
	}
	break;

    case ct_BANG:
	result = eap->forceit ? 1 : 0;
	if (quote)
	    result += 2;
	if (buf != NULL)
	{
	    if (quote)
		*buf++ = '"';
	    if (eap->forceit)
		*buf++ = '!';
	    if (quote)
		*buf = '"';
	}
	break;

    case ct_LINE1:
    case ct_LINE2:
    case ct_RANGE:
    case ct_COUNT:
    {
	char num_buf[20];
	long num = (type == ct_LINE1) ? eap->line1 :
		   (type == ct_LINE2) ? eap->line2 :
		   (type == ct_RANGE) ? eap->addr_count :
		   (eap->addr_count > 0) ? eap->line2 : cmd->uc_def;
	size_t num_len;

	sprintf(num_buf, "%ld", num);
	num_len = STRLEN(num_buf);
	result = num_len;

	if (quote)
	    result += 2;

	if (buf != NULL)
	{
	    if (quote)
		*buf++ = '"';
	    STRCPY(buf, num_buf);
	    buf += num_len;
	    if (quote)
		*buf = '"';
	}

	break;
    }

    case ct_MODS:
    {
	int multi_mods = 0;
	typedef struct {
	    int *varp;
	    char *name;
	} mod_entry_T;
	static mod_entry_T mod_entries[] = {
#ifdef FEAT_BROWSE_CMD
	    {&cmdmod.browse, "browse"},
#endif
#if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
	    {&cmdmod.confirm, "confirm"},
#endif
	    {&cmdmod.hide, "hide"},
	    {&cmdmod.keepalt, "keepalt"},
	    {&cmdmod.keepjumps, "keepjumps"},
	    {&cmdmod.keepmarks, "keepmarks"},
	    {&cmdmod.keeppatterns, "keeppatterns"},
	    {&cmdmod.lockmarks, "lockmarks"},
	    {&cmdmod.noswapfile, "noswapfile"},
	    {NULL, NULL}
	};
	int i;

	result = quote ? 2 : 0;
	if (buf != NULL)
	{
	    if (quote)
		*buf++ = '"';
	    *buf = '\0';
	}

	// :aboveleft and :leftabove
	if (cmdmod.split & WSP_ABOVE)
	    result += add_cmd_modifier(buf, "aboveleft", &multi_mods);
	// :belowright and :rightbelow
	if (cmdmod.split & WSP_BELOW)
	    result += add_cmd_modifier(buf, "belowright", &multi_mods);
	// :botright
	if (cmdmod.split & WSP_BOT)
	    result += add_cmd_modifier(buf, "botright", &multi_mods);

	// the modifiers that are simple flags
	for (i = 0; mod_entries[i].varp != NULL; ++i)
	    if (*mod_entries[i].varp)
		result += add_cmd_modifier(buf, mod_entries[i].name,
								 &multi_mods);

	// TODO: How to support :noautocmd?
#ifdef HAVE_SANDBOX
	// TODO: How to support :sandbox?
#endif
	// :silent
	if (msg_silent > 0)
	    result += add_cmd_modifier(buf,
		    emsg_silent > 0 ? "silent!" : "silent", &multi_mods);
	// :tab
	if (cmdmod.tab > 0)
	    result += add_cmd_modifier(buf, "tab", &multi_mods);
	// :topleft
	if (cmdmod.split & WSP_TOP)
	    result += add_cmd_modifier(buf, "topleft", &multi_mods);
	// TODO: How to support :unsilent?
	// :verbose
	if (p_verbose > 0)
	    result += add_cmd_modifier(buf, "verbose", &multi_mods);
	// :vertical
	if (cmdmod.split & WSP_VERT)
	    result += add_cmd_modifier(buf, "vertical", &multi_mods);
	if (quote && buf != NULL)
	{
	    buf += result - 2;
	    *buf = '"';
	}
	break;
    }

    case ct_REGISTER:
	result = eap->regname ? 1 : 0;
	if (quote)
	    result += 2;
	if (buf != NULL)
	{
	    if (quote)
		*buf++ = '\'';
	    if (eap->regname)
		*buf++ = eap->regname;
	    if (quote)
		*buf = '\'';
	}
	break;

    case ct_LT:
	result = 1;
	if (buf != NULL)
	    *buf = '<';
	break;

    default:
	// Not recognized: just copy the '<' and return -1.
	result = (size_t)-1;
	if (buf != NULL)
	    *buf = '<';
	break;
    }

    return result;
}

/*
 * Execute a user defined command.
 */
    void
do_ucmd(exarg_T *eap)
{
    char_u	*buf;
    char_u	*p;
    char_u	*q;

    char_u	*start;
    char_u	*end = NULL;
    char_u	*ksp;
    size_t	len, totlen;

    size_t	split_len = 0;
    char_u	*split_buf = NULL;
    ucmd_T	*cmd;
#ifdef FEAT_EVAL
    sctx_T	save_current_sctx = current_sctx;
#endif

    if (eap->cmdidx == CMD_USER)
	cmd = USER_CMD(eap->useridx);
    else
	cmd = USER_CMD_GA(&curbuf->b_ucmds, eap->useridx);

    /*
     * Replace <> in the command by the arguments.
     * First round: "buf" is NULL, compute length, allocate "buf".
     * Second round: copy result into "buf".
     */
    buf = NULL;
    for (;;)
    {
	p = cmd->uc_rep;    // source
	q = buf;	    // destination
	totlen = 0;

	for (;;)
	{
	    start = vim_strchr(p, '<');
	    if (start != NULL)
		end = vim_strchr(start + 1, '>');
	    if (buf != NULL)
	    {
		for (ksp = p; *ksp != NUL && *ksp != K_SPECIAL; ++ksp)
		    ;
		if (*ksp == K_SPECIAL
			&& (start == NULL || ksp < start || end == NULL)
			&& ((ksp[1] == KS_SPECIAL && ksp[2] == KE_FILLER)
# ifdef FEAT_GUI
			    || (ksp[1] == KS_EXTRA && ksp[2] == (int)KE_CSI)
# endif
			    ))
		{
		    // K_SPECIAL has been put in the buffer as K_SPECIAL
		    // KS_SPECIAL KE_FILLER, like for mappings, but
		    // do_cmdline() doesn't handle that, so convert it back.
		    // Also change K_SPECIAL KS_EXTRA KE_CSI into CSI.
		    len = ksp - p;
		    if (len > 0)
		    {
			mch_memmove(q, p, len);
			q += len;
		    }
		    *q++ = ksp[1] == KS_SPECIAL ? K_SPECIAL : CSI;
		    p = ksp + 3;
		    continue;
		}
	    }

	    // break if no <item> is found
	    if (start == NULL || end == NULL)
		break;

	    // Include the '>'
	    ++end;

	    // Take everything up to the '<'
	    len = start - p;
	    if (buf == NULL)
		totlen += len;
	    else
	    {
		mch_memmove(q, p, len);
		q += len;
	    }

	    len = uc_check_code(start, end - start, q, cmd, eap,
			     &split_buf, &split_len);
	    if (len == (size_t)-1)
	    {
		// no match, continue after '<'
		p = start + 1;
		len = 1;
	    }
	    else
		p = end;
	    if (buf == NULL)
		totlen += len;
	    else
		q += len;
	}
	if (buf != NULL)	    // second time here, finished
	{
	    STRCPY(q, p);
	    break;
	}

	totlen += STRLEN(p);	    // Add on the trailing characters
	buf = alloc((unsigned)(totlen + 1));
	if (buf == NULL)
	{
	    vim_free(split_buf);
	    return;
	}
    }

#ifdef FEAT_EVAL
    current_sctx.sc_sid = cmd->uc_script_ctx.sc_sid;
#endif
    (void)do_cmdline(buf, eap->getline, eap->cookie,
				   DOCMD_VERBOSE|DOCMD_NOWAIT|DOCMD_KEYTYPED);
#ifdef FEAT_EVAL
    current_sctx = save_current_sctx;
#endif
    vim_free(buf);
    vim_free(split_buf);
}
