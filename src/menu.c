/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *				GUI/Motif support by Robert Webb
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * Code for menus.  Used for the GUI and 'wildmenu'.
 */

#include "vim.h"

#if defined(FEAT_MENU) || defined(PROTO)

#define MENUDEPTH   10		/* maximum depth of menus */

#ifdef FEAT_GUI_W32
static int add_menu_path __ARGS((char_u *, vimmenu_T *, int *, char_u *, int));
#else
static int add_menu_path __ARGS((char_u *, vimmenu_T *, int *, char_u *));
#endif
static int menu_nable_recurse __ARGS((vimmenu_T *menu, char_u *name, int modes, int enable));
static int remove_menu __ARGS((vimmenu_T **, char_u *, int, int silent));
static void free_menu __ARGS((vimmenu_T **menup));
static void free_menu_string __ARGS((vimmenu_T *, int));
static int show_menus __ARGS((char_u *, int));
static void show_menus_recursive __ARGS((vimmenu_T *, int, int));
static int menu_name_equal __ARGS((char_u *name, vimmenu_T *menu));
static int menu_namecmp __ARGS((char_u *name, char_u *mname));
static int get_menu_cmd_modes __ARGS((char_u *, int, int *, int *));
static char_u *popup_mode_name __ARGS((char_u *name, int idx));
static char_u *menu_text __ARGS((char_u *text, int *mnemonic, char_u **actext));
#ifdef FEAT_GUI
static int get_menu_mode __ARGS((void));
static void gui_update_menus_recurse __ARGS((vimmenu_T *, int));
#endif

#if defined(FEAT_GUI_W32) && defined(FEAT_TEAROFF)
static void gui_create_tearoffs_recurse __ARGS((vimmenu_T *menu, const char_u *pname, int *pri_tab, int pri_idx));
static void gui_add_tearoff __ARGS((char_u *tearpath, int *pri_tab, int pri_idx));
static void gui_destroy_tearoffs_recurse __ARGS((vimmenu_T *menu));
static int s_tearoffs = FALSE;
#endif

static int menu_is_hidden __ARGS((char_u *name));
#if defined(FEAT_CMDL_COMPL) || (defined(FEAT_GUI_W32) && defined(FEAT_TEAROFF))
static int menu_is_tearoff __ARGS((char_u *name));
#endif

#if defined(FEAT_MULTI_LANG) || defined(FEAT_TOOLBAR)
static char_u *menu_skip_part __ARGS((char_u *p));
#endif
#ifdef FEAT_MULTI_LANG
static char_u *menutrans_lookup __ARGS((char_u *name, int len));
#endif

/* The character for each menu mode */
static char_u	menu_mode_chars[] = {'n', 'v', 's', 'o', 'i', 'c', 't'};

static char_u e_notsubmenu[] = N_("E327: Part of menu-item path is not sub-menu");
static char_u e_othermode[] = N_("E328: Menu only exists in another mode");
static char_u e_nomenu[] = N_("E329: No menu \"%s\"");

#ifdef FEAT_TOOLBAR
static const char *toolbar_names[] =
{
    /*  0 */ "New", "Open", "Save", "Undo", "Redo",
    /*  5 */ "Cut", "Copy", "Paste", "Print", "Help",
    /* 10 */ "Find", "SaveAll", "SaveSesn", "NewSesn", "LoadSesn",
    /* 15 */ "RunScript", "Replace", "WinClose", "WinMax", "WinMin",
    /* 20 */ "WinSplit", "Shell", "FindPrev", "FindNext", "FindHelp",
    /* 25 */ "Make", "TagJump", "RunCtags", "WinVSplit", "WinMaxWidth",
    /* 30 */ "WinMinWidth", "Exit"
};
# define TOOLBAR_NAME_COUNT (sizeof(toolbar_names) / sizeof(char *))
#endif

/*
 * Do the :menu command and relatives.
 */
    void
ex_menu(eap)
    exarg_T	*eap;		    /* Ex command arguments */
{
    char_u	*menu_path;
    int		modes;
    char_u	*map_to;
    int		noremap;
    int		silent = FALSE;
    int		special = FALSE;
    int		unmenu;
    char_u	*map_buf;
    char_u	*arg;
    char_u	*p;
    int		i;
#if defined(FEAT_GUI) && !defined(FEAT_GUI_GTK)
    int		old_menu_height;
# if defined(FEAT_TOOLBAR) && !defined(FEAT_GUI_W32) && !defined(FEAT_GUI_W16)
    int		old_toolbar_height;
# endif
#endif
    int		pri_tab[MENUDEPTH + 1];
    int		enable = MAYBE;	    /* TRUE for "menu enable", FALSE for "menu
				     * disable */
#ifdef FEAT_MULTI_LANG
    char_u	*tofree = NULL;
    char_u	*new_cmd;
#endif
#ifdef FEAT_TOOLBAR
    char_u	*icon = NULL;
#endif
    vimmenu_T	menuarg;

    modes = get_menu_cmd_modes(eap->cmd, eap->forceit, &noremap, &unmenu);
    arg = eap->arg;

    for (;;)
    {
	if (STRNCMP(arg, "<script>", 8) == 0)
	{
	    noremap = REMAP_SCRIPT;
	    arg = skipwhite(arg + 8);
	    continue;
	}
	if (STRNCMP(arg, "<silent>", 8) == 0)
	{
	    silent = TRUE;
	    arg = skipwhite(arg + 8);
	    continue;
	}
	if (STRNCMP(arg, "<special>", 9) == 0)
	{
	    special = TRUE;
	    arg = skipwhite(arg + 9);
	    continue;
	}
	break;
    }


    /* Locate an optional "icon=filename" argument. */
    if (STRNCMP(arg, "icon=", 5) == 0)
    {
	arg += 5;
#ifdef FEAT_TOOLBAR
	icon = arg;
#endif
	while (*arg != NUL && *arg != ' ')
	{
	    if (*arg == '\\')
		mch_memmove(arg, arg + 1, STRLEN(arg));
	    mb_ptr_adv(arg);
	}
	if (*arg != NUL)
	{
	    *arg++ = NUL;
	    arg = skipwhite(arg);
	}
    }

    /*
     * Fill in the priority table.
     */
    for (p = arg; *p; ++p)
	if (!VIM_ISDIGIT(*p) && *p != '.')
	    break;
    if (vim_iswhite(*p))
    {
	for (i = 0; i < MENUDEPTH && !vim_iswhite(*arg); ++i)
	{
	    pri_tab[i] = getdigits(&arg);
	    if (pri_tab[i] == 0)
		pri_tab[i] = 500;
	    if (*arg == '.')
		++arg;
	}
	arg = skipwhite(arg);
    }
    else if (eap->addr_count && eap->line2 != 0)
    {
	pri_tab[0] = eap->line2;
	i = 1;
    }
    else
	i = 0;
    while (i < MENUDEPTH)
	pri_tab[i++] = 500;
    pri_tab[MENUDEPTH] = -1;		/* mark end of the table */

    /*
     * Check for "disable" or "enable" argument.
     */
    if (STRNCMP(arg, "enable", 6) == 0 && vim_iswhite(arg[6]))
    {
	enable = TRUE;
	arg = skipwhite(arg + 6);
    }
    else if (STRNCMP(arg, "disable", 7) == 0 && vim_iswhite(arg[7]))
    {
	enable = FALSE;
	arg = skipwhite(arg + 7);
    }

    /*
     * If there is no argument, display all menus.
     */
    if (*arg == NUL)
    {
	show_menus(arg, modes);
	return;
    }

#ifdef FEAT_TOOLBAR
    /*
     * Need to get the toolbar icon index before doing the translation.
     */
    menuarg.iconidx = -1;
    menuarg.icon_builtin = FALSE;
    if (menu_is_toolbar(arg))
    {
	menu_path = menu_skip_part(arg);
	if (*menu_path == '.')
	{
	    p = menu_skip_part(++menu_path);
	    if (STRNCMP(menu_path, "BuiltIn", 7) == 0)
	    {
		if (skipdigits(menu_path + 7) == p)
		{
		    menuarg.iconidx = atoi((char *)menu_path + 7);
		    if (menuarg.iconidx >= TOOLBAR_NAME_COUNT)
			menuarg.iconidx = -1;
		    else
			menuarg.icon_builtin = TRUE;
		}
	    }
	    else
	    {
		for (i = 0; i < TOOLBAR_NAME_COUNT; ++i)
		    if (STRNCMP(toolbar_names[i], menu_path, p - menu_path)
									 == 0)
		    {
			menuarg.iconidx = i;
			break;
		    }
	    }
	}
    }
#endif

#ifdef FEAT_MULTI_LANG
    /*
     * Translate menu names as specified with ":menutrans" commands.
     */
    menu_path = arg;
    while (*menu_path)
    {
	/* find the end of one part and check if it should be translated */
	p = menu_skip_part(menu_path);
	map_to = menutrans_lookup(menu_path, (int)(p - menu_path));
	if (map_to != NULL)
	{
	    /* found a match: replace with the translated part */
	    i = (int)STRLEN(map_to);
	    new_cmd = alloc((unsigned)STRLEN(arg) + i + 1);
	    if (new_cmd == NULL)
		break;
	    mch_memmove(new_cmd, arg, menu_path - arg);
	    mch_memmove(new_cmd + (menu_path - arg), map_to, (size_t)i);
	    STRCPY(new_cmd + (menu_path - arg) + i, p);
	    p = new_cmd + (menu_path - arg) + i;
	    vim_free(tofree);
	    tofree = new_cmd;
	    arg = new_cmd;
	}
	if (*p != '.')
	    break;
	menu_path = p + 1;
    }
#endif

    /*
     * Isolate the menu name.
     * Skip the menu name, and translate <Tab> into a real TAB.
     */
    menu_path = arg;
    if (*menu_path == '.')
    {
	EMSG2(_(e_invarg2), menu_path);
	goto theend;
    }

    while (*arg && !vim_iswhite(*arg))
    {
	if ((*arg == '\\' || *arg == Ctrl_V) && arg[1] != NUL)
	    arg++;
	else if (STRNICMP(arg, "<TAB>", 5) == 0)
	{
	    *arg = TAB;
	    mch_memmove(arg + 1, arg + 5, STRLEN(arg + 4));
	}
	arg++;
    }
    if (*arg != NUL)
	*arg++ = NUL;
    arg = skipwhite(arg);
    map_to = arg;

    /*
     * If there is only a menu name, display menus with that name.
     */
    if (*map_to == NUL && !unmenu && enable == MAYBE)
    {
	show_menus(menu_path, modes);
	goto theend;
    }
    else if (*map_to != NUL && (unmenu || enable != MAYBE))
    {
	EMSG(_(e_trailing));
	goto theend;
    }
#if defined(FEAT_GUI) && !(defined(FEAT_GUI_GTK) || defined(FEAT_GUI_PHOTON))
    old_menu_height = gui.menu_height;
# if defined(FEAT_TOOLBAR) && !defined(FEAT_GUI_W32) && !defined(FEAT_GUI_W16)
    old_toolbar_height = gui.toolbar_height;
# endif
#endif

    if (enable != MAYBE)
    {
	/*
	 * Change sensitivity of the menu.
	 * For the PopUp menu, remove a menu for each mode separately.
	 * Careful: menu_nable_recurse() changes menu_path.
	 */
	if (STRCMP(menu_path, "*") == 0)	/* meaning: do all menus */
	    menu_path = (char_u *)"";

	if (menu_is_popup(menu_path))
	{
	    for (i = 0; i < MENU_INDEX_TIP; ++i)
		if (modes & (1 << i))
		{
		    p = popup_mode_name(menu_path, i);
		    if (p != NULL)
		    {
			menu_nable_recurse(root_menu, p, MENU_ALL_MODES,
								      enable);
			vim_free(p);
		    }
		}
	}
	menu_nable_recurse(root_menu, menu_path, modes, enable);
    }
    else if (unmenu)
    {
	/*
	 * Delete menu(s).
	 */
	if (STRCMP(menu_path, "*") == 0)	/* meaning: remove all menus */
	    menu_path = (char_u *)"";

	/*
	 * For the PopUp menu, remove a menu for each mode separately.
	 */
	if (menu_is_popup(menu_path))
	{
	    for (i = 0; i < MENU_INDEX_TIP; ++i)
		if (modes & (1 << i))
		{
		    p = popup_mode_name(menu_path, i);
		    if (p != NULL)
		    {
			remove_menu(&root_menu, p, MENU_ALL_MODES, TRUE);
			vim_free(p);
		    }
		}
	}

	/* Careful: remove_menu() changes menu_path */
	remove_menu(&root_menu, menu_path, modes, FALSE);
    }
    else
    {
	/*
	 * Add menu(s).
	 * Replace special key codes.
	 */
	if (STRICMP(map_to, "<nop>") == 0)	/* "<Nop>" means nothing */
	{
	    map_to = (char_u *)"";
	    map_buf = NULL;
	}
	else if (modes & MENU_TIP_MODE)
	    map_buf = NULL;	/* Menu tips are plain text. */
	else
	    map_to = replace_termcodes(map_to, &map_buf, FALSE, TRUE, special);
	menuarg.modes = modes;
#ifdef FEAT_TOOLBAR
	menuarg.iconfile = icon;
#endif
	menuarg.noremap[0] = noremap;
	menuarg.silent[0] = silent;
	add_menu_path(menu_path, &menuarg, pri_tab, map_to
#ifdef FEAT_GUI_W32
		, TRUE
#endif
		);

	/*
	 * For the PopUp menu, add a menu for each mode separately.
	 */
	if (menu_is_popup(menu_path))
	{
	    for (i = 0; i < MENU_INDEX_TIP; ++i)
		if (modes & (1 << i))
		{
		    p = popup_mode_name(menu_path, i);
		    if (p != NULL)
		    {
			/* Include all modes, to make ":amenu" work */
			menuarg.modes = modes;
#ifdef FEAT_TOOLBAR
			menuarg.iconfile = NULL;
			menuarg.iconidx = -1;
			menuarg.icon_builtin = FALSE;
#endif
			add_menu_path(p, &menuarg, pri_tab, map_to
#ifdef FEAT_GUI_W32
				, TRUE
#endif
					 );
			vim_free(p);
		    }
		}
	}

	vim_free(map_buf);
    }

#if defined(FEAT_GUI) && !(defined(FEAT_GUI_GTK))
    /* If the menubar height changed, resize the window */
    if (gui.in_use
	    && (gui.menu_height != old_menu_height
# if defined(FEAT_TOOLBAR) && !defined(FEAT_GUI_W32) && !defined(FEAT_GUI_W16)
		|| gui.toolbar_height != old_toolbar_height
# endif
	    ))
	gui_set_shellsize(FALSE, FALSE, RESIZE_VERT);
#endif

theend:
#ifdef FEAT_MULTI_LANG
    vim_free(tofree);
#else
    ;
#endif
}

/*
 * Add the menu with the given name to the menu hierarchy
 */
    static int
add_menu_path(menu_path, menuarg, pri_tab, call_data
#ifdef FEAT_GUI_W32
	, addtearoff
#endif
	)
    char_u	*menu_path;
    vimmenu_T	*menuarg;	/* passes modes, iconfile, iconidx,
				   icon_builtin, silent[0], noremap[0] */
    int		*pri_tab;
    char_u	*call_data;
#ifdef FEAT_GUI_W32
    int		addtearoff;	/* may add tearoff item */
#endif
{
    char_u	*path_name;
    int		modes = menuarg->modes;
    vimmenu_T	**menup;
    vimmenu_T	*menu = NULL;
    vimmenu_T	*parent;
    vimmenu_T	**lower_pri;
    char_u	*p;
    char_u	*name;
    char_u	*dname;
    char_u	*next_name;
    int		i;
    int		c;
#ifdef FEAT_GUI
    int		idx;
    int		new_idx;
#endif
    int		pri_idx = 0;
    int		old_modes = 0;
    int		amenu;

    /* Make a copy so we can stuff around with it, since it could be const */
    path_name = vim_strsave(menu_path);
    if (path_name == NULL)
	return FAIL;
    menup = &root_menu;
    parent = NULL;
    name = path_name;
    while (*name)
    {
	/* Get name of this element in the menu hierarchy, and the simplified
	 * name (without mnemonic and accelerator text). */
	next_name = menu_name_skip(name);
	dname = menu_text(name, NULL, NULL);

	/* See if it's already there */
	lower_pri = menup;
#ifdef FEAT_GUI
	idx = 0;
	new_idx = 0;
#endif
	menu = *menup;
	while (menu != NULL)
	{
	    if (menu_name_equal(name, menu) || menu_name_equal(dname, menu))
	    {
		if (*next_name == NUL && menu->children != NULL)
		{
		    if (!sys_menu)
			EMSG(_("E330: Menu path must not lead to a sub-menu"));
		    goto erret;
		}
		if (*next_name != NUL && menu->children == NULL
#ifdef FEAT_GUI_W32
			&& addtearoff
#endif
			)
		{
		    if (!sys_menu)
			EMSG(_(e_notsubmenu));
		    goto erret;
		}
		break;
	    }
	    menup = &menu->next;

	    /* Count menus, to find where this one needs to be inserted.
	     * Ignore menus that are not in the menubar (PopUp and Toolbar) */
	    if (parent != NULL || menu_is_menubar(menu->name))
	    {
#ifdef FEAT_GUI
		++idx;
#endif
		if (menu->priority <= pri_tab[pri_idx])
		{
		    lower_pri = menup;
#ifdef FEAT_GUI
		    new_idx = idx;
#endif
		}
	    }
	    menu = menu->next;
	}

	if (menu == NULL)
	{
	    if (*next_name == NUL && parent == NULL)
	    {
		EMSG(_("E331: Must not add menu items directly to menu bar"));
		goto erret;
	    }

	    if (menu_is_separator(dname) && *next_name != NUL)
	    {
		EMSG(_("E332: Separator cannot be part of a menu path"));
		goto erret;
	    }

	    /* Not already there, so lets add it */
	    menu = (vimmenu_T *)alloc_clear((unsigned)sizeof(vimmenu_T));
	    if (menu == NULL)
		goto erret;

	    menu->modes = modes;
	    menu->enabled = MENU_ALL_MODES;
	    menu->name = vim_strsave(name);
	    /* separate mnemonic and accelerator text from actual menu name */
	    menu->dname = menu_text(name, &menu->mnemonic, &menu->actext);
	    menu->priority = pri_tab[pri_idx];
	    menu->parent = parent;
#ifdef FEAT_GUI_MOTIF
	    menu->sensitive = TRUE;	    /* the default */
#endif
#ifdef FEAT_BEVAL_TIP
	    menu->tip = NULL;
#endif
#ifdef FEAT_GUI_ATHENA
	    menu->image = None;		    /* X-Windows definition for NULL*/
#endif

	    /*
	     * Add after menu that has lower priority.
	     */
	    menu->next = *lower_pri;
	    *lower_pri = menu;

	    old_modes = 0;

#ifdef FEAT_TOOLBAR
	    menu->iconidx = menuarg->iconidx;
	    menu->icon_builtin = menuarg->icon_builtin;
	    if (*next_name == NUL && menuarg->iconfile != NULL)
		menu->iconfile = vim_strsave(menuarg->iconfile);
#endif
#if defined(FEAT_GUI_W32) && defined(FEAT_TEAROFF)
	    /* the tearoff item must be present in the modes of each item. */
	    if (parent != NULL && menu_is_tearoff(parent->children->dname))
		parent->children->modes |= modes;
#endif
	}
	else
	{
	    old_modes = menu->modes;

	    /*
	     * If this menu option was previously only available in other
	     * modes, then make sure it's available for this one now
	     * Also enable a menu when it's created or changed.
	     */
#ifdef FEAT_GUI_W32
	    /* If adding a tearbar (addtearoff == FALSE) don't update modes */
	    if (addtearoff)
#endif
	    {
		menu->modes |= modes;
		menu->enabled |= modes;
	    }
	}

#ifdef FEAT_GUI
	/*
	 * Add the menu item when it's used in one of the modes, but not when
	 * only a tooltip is defined.
	 */
	if ((old_modes & MENU_ALL_MODES) == 0
		&& (menu->modes & MENU_ALL_MODES) != 0)
	{
	    if (gui.in_use)  /* Otherwise it will be added when GUI starts */
	    {
		if (*next_name == NUL)
		{
		    /* Real menu item, not sub-menu */
		    gui_mch_add_menu_item(menu, new_idx);

		    /* Want to update menus now even if mode not changed */
		    force_menu_update = TRUE;
		}
		else
		{
		    /* Sub-menu (not at end of path yet) */
		    gui_mch_add_menu(menu, new_idx);
		}
	    }

# if defined(FEAT_GUI_W32) & defined(FEAT_TEAROFF)
	    /* When adding a new submenu, may add a tearoff item */
	    if (	addtearoff
		    && *next_name
		    && vim_strchr(p_go, GO_TEAROFF) != NULL
		    && menu_is_menubar(name))
	    {
		char_u		*tearpath;

		/*
		 * The pointers next_name & path_name refer to a string with
		 * \'s and ^V's stripped out. But menu_path is a "raw"
		 * string, so we must correct for special characters.
		 */
		tearpath = alloc((unsigned int)STRLEN(menu_path) + TEAR_LEN + 2);
		if (tearpath != NULL)
		{
		    char_u  *s;
		    int	    idx;

		    STRCPY(tearpath, menu_path);
		    idx = (int)(next_name - path_name - 1);
		    for (s = tearpath; *s && s < tearpath + idx; mb_ptr_adv(s))
		    {
			if ((*s == '\\' || *s == Ctrl_V) && s[1])
			{
			    ++idx;
			    ++s;
			}
		    }
		    tearpath[idx] = NUL;
		    gui_add_tearoff(tearpath, pri_tab, pri_idx);
		    vim_free(tearpath);
		}
	    }
# endif
	}
#endif /* FEAT_GUI */

	menup = &menu->children;
	parent = menu;
	name = next_name;
	vim_free(dname);
	if (pri_tab[pri_idx + 1] != -1)
	    ++pri_idx;
    }
    vim_free(path_name);

    /*
     * Only add system menu items which have not been defined yet.
     * First check if this was an ":amenu".
     */
    amenu = ((modes & (MENU_NORMAL_MODE | MENU_INSERT_MODE)) ==
				       (MENU_NORMAL_MODE | MENU_INSERT_MODE));
    if (sys_menu)
	modes &= ~old_modes;

    if (menu != NULL && modes)
    {
#ifdef FEAT_GUI
	menu->cb = gui_menu_cb;
#endif
	p = (call_data == NULL) ? NULL : vim_strsave(call_data);

	/* loop over all modes, may add more than one */
	for (i = 0; i < MENU_MODES; ++i)
	{
	    if (modes & (1 << i))
	    {
		/* free any old menu */
		free_menu_string(menu, i);

		/* For "amenu", may insert an extra character.
		 * Don't do this if adding a tearbar (addtearoff == FALSE).
		 * Don't do this for "<Nop>". */
		c = 0;
		if (amenu && call_data != NULL && *call_data != NUL
#ifdef FEAT_GUI_W32
		       && addtearoff
#endif
				       )
		{
		    switch (1 << i)
		    {
			case MENU_VISUAL_MODE:
			case MENU_SELECT_MODE:
			case MENU_OP_PENDING_MODE:
			case MENU_CMDLINE_MODE:
			    c = Ctrl_C;
			    break;
			case MENU_INSERT_MODE:
			    c = Ctrl_O;
			    break;
		    }
		}

		if (c)
		{
		    menu->strings[i] = alloc((unsigned)(STRLEN(call_data) + 4));
		    if (menu->strings[i] != NULL)
		    {
			menu->strings[i][0] = c;
			STRCPY(menu->strings[i] + 1, call_data);
			if (c == Ctrl_C)
			{
			    int	    len = (int)STRLEN(menu->strings[i]);

			    /* Append CTRL-\ CTRL-G to obey 'insertmode'. */
			    menu->strings[i][len] = Ctrl_BSL;
			    menu->strings[i][len + 1] = Ctrl_G;
			    menu->strings[i][len + 2] = NUL;
			}
		    }
		}
		else
		    menu->strings[i] = p;
		menu->noremap[i] = menuarg->noremap[0];
		menu->silent[i] = menuarg->silent[0];
	    }
	}
#if defined(FEAT_TOOLBAR) && !defined(FEAT_GUI_W32) \
	&& (defined(FEAT_BEVAL) || defined(FEAT_GUI_GTK))
	/* Need to update the menu tip. */
	if (modes & MENU_TIP_MODE)
	    gui_mch_menu_set_tip(menu);
#endif
    }
    return OK;

erret:
    vim_free(path_name);
    vim_free(dname);
    return FAIL;
}

/*
 * Set the (sub)menu with the given name to enabled or disabled.
 * Called recursively.
 */
    static int
menu_nable_recurse(menu, name, modes, enable)
    vimmenu_T	*menu;
    char_u	*name;
    int		modes;
    int		enable;
{
    char_u	*p;

    if (menu == NULL)
	return OK;		/* Got to bottom of hierarchy */

    /* Get name of this element in the menu hierarchy */
    p = menu_name_skip(name);

    /* Find the menu */
    while (menu != NULL)
    {
	if (*name == NUL || *name == '*' || menu_name_equal(name, menu))
	{
	    if (*p != NUL)
	    {
		if (menu->children == NULL)
		{
		    EMSG(_(e_notsubmenu));
		    return FAIL;
		}
		if (menu_nable_recurse(menu->children, p, modes, enable)
								      == FAIL)
		    return FAIL;
	    }
	    else
		if (enable)
		    menu->enabled |= modes;
		else
		    menu->enabled &= ~modes;

	    /*
	     * When name is empty, we are doing all menu items for the given
	     * modes, so keep looping, otherwise we are just doing the named
	     * menu item (which has been found) so break here.
	     */
	    if (*name != NUL && *name != '*')
		break;
	}
	menu = menu->next;
    }
    if (*name != NUL && *name != '*' && menu == NULL)
    {
	EMSG2(_(e_nomenu), name);
	return FAIL;
    }

#ifdef FEAT_GUI
    /* Want to update menus now even if mode not changed */
    force_menu_update = TRUE;
#endif

    return OK;
}

/*
 * Remove the (sub)menu with the given name from the menu hierarchy
 * Called recursively.
 */
    static int
remove_menu(menup, name, modes, silent)
    vimmenu_T	**menup;
    char_u	*name;
    int		modes;
    int		silent;		/* don't give error messages */
{
    vimmenu_T	*menu;
    vimmenu_T	*child;
    char_u	*p;

    if (*menup == NULL)
	return OK;		/* Got to bottom of hierarchy */

    /* Get name of this element in the menu hierarchy */
    p = menu_name_skip(name);

    /* Find the menu */
    while ((menu = *menup) != NULL)
    {
	if (*name == NUL || menu_name_equal(name, menu))
	{
	    if (*p != NUL && menu->children == NULL)
	    {
		if (!silent)
		    EMSG(_(e_notsubmenu));
		return FAIL;
	    }
	    if ((menu->modes & modes) != 0x0)
	    {
#if defined(FEAT_GUI_W32) & defined(FEAT_TEAROFF)
		/*
		 * If we are removing all entries for this menu,MENU_ALL_MODES,
		 * Then kill any tearoff before we start
		 */
		if (*p == NUL && modes == MENU_ALL_MODES)
		{
		    if (IsWindow(menu->tearoff_handle))
			DestroyWindow(menu->tearoff_handle);
		}
#endif
		if (remove_menu(&menu->children, p, modes, silent) == FAIL)
		    return FAIL;
	    }
	    else if (*name != NUL)
	    {
		if (!silent)
		    EMSG(_(e_othermode));
		return FAIL;
	    }

	    /*
	     * When name is empty, we are removing all menu items for the given
	     * modes, so keep looping, otherwise we are just removing the named
	     * menu item (which has been found) so break here.
	     */
	    if (*name != NUL)
		break;

	    /* Remove the menu item for the given mode[s].  If the menu item
	     * is no longer valid in ANY mode, delete it */
	    menu->modes &= ~modes;
	    if (modes & MENU_TIP_MODE)
		free_menu_string(menu, MENU_INDEX_TIP);
	    if ((menu->modes & MENU_ALL_MODES) == 0)
		free_menu(menup);
	    else
		menup = &menu->next;
	}
	else
	    menup = &menu->next;
    }
    if (*name != NUL)
    {
	if (menu == NULL)
	{
	    if (!silent)
		EMSG2(_(e_nomenu), name);
	    return FAIL;
	}


	/* Recalculate modes for menu based on the new updated children */
	menu->modes &= ~modes;
#if defined(FEAT_GUI_W32) & defined(FEAT_TEAROFF)
	if ((s_tearoffs) && (menu->children != NULL)) /* there's a tear bar.. */
	    child = menu->children->next; /* don't count tearoff bar */
	else
#endif
	    child = menu->children;
	for ( ; child != NULL; child = child->next)
	    menu->modes |= child->modes;
	if (modes & MENU_TIP_MODE)
	{
	    free_menu_string(menu, MENU_INDEX_TIP);
#if defined(FEAT_TOOLBAR) && !defined(FEAT_GUI_W32) \
	    && (defined(FEAT_BEVAL) || defined(FEAT_GUI_GTK))
	    /* Need to update the menu tip. */
	    if (gui.in_use)
		gui_mch_menu_set_tip(menu);
#endif
	}
	if ((menu->modes & MENU_ALL_MODES) == 0)
	{
	    /* The menu item is no longer valid in ANY mode, so delete it */
#if defined(FEAT_GUI_W32) & defined(FEAT_TEAROFF)
	    if (s_tearoffs && menu->children != NULL) /* there's a tear bar.. */
		free_menu(&menu->children);
#endif
	    *menup = menu;
	    free_menu(menup);
	}
    }

    return OK;
}

/*
 * Free the given menu structure and remove it from the linked list.
 */
    static void
free_menu(menup)
    vimmenu_T	**menup;
{
    int		i;
    vimmenu_T	*menu;

    menu = *menup;

#ifdef FEAT_GUI
    /* Free machine specific menu structures (only when already created) */
    /* Also may rebuild a tearoff'ed menu */
    if (gui.in_use)
	gui_mch_destroy_menu(menu);
#endif

    /* Don't change *menup until after calling gui_mch_destroy_menu(). The
     * MacOS code needs the original structure to properly delete the menu. */
    *menup = menu->next;
    vim_free(menu->name);
    vim_free(menu->dname);
    vim_free(menu->actext);
#ifdef FEAT_TOOLBAR
    vim_free(menu->iconfile);
# ifdef FEAT_GUI_MOTIF
    vim_free(menu->xpm_fname);
# endif
#endif
    for (i = 0; i < MENU_MODES; i++)
	free_menu_string(menu, i);
    vim_free(menu);

#ifdef FEAT_GUI
    /* Want to update menus now even if mode not changed */
    force_menu_update = TRUE;
#endif
}

/*
 * Free the menu->string with the given index.
 */
    static void
free_menu_string(menu, idx)
    vimmenu_T	*menu;
    int		idx;
{
    int		count = 0;
    int		i;

    for (i = 0; i < MENU_MODES; i++)
	if (menu->strings[i] == menu->strings[idx])
	    count++;
    if (count == 1)
	vim_free(menu->strings[idx]);
    menu->strings[idx] = NULL;
}

/*
 * Show the mapping associated with a menu item or hierarchy in a sub-menu.
 */
    static int
show_menus(path_name, modes)
    char_u  *path_name;
    int	    modes;
{
    char_u	*p;
    char_u	*name;
    vimmenu_T	*menu;
    vimmenu_T	*parent = NULL;

    menu = root_menu;
    name = path_name = vim_strsave(path_name);
    if (path_name == NULL)
	return FAIL;

    /* First, find the (sub)menu with the given name */
    while (*name)
    {
	p = menu_name_skip(name);
	while (menu != NULL)
	{
	    if (menu_name_equal(name, menu))
	    {
		/* Found menu */
		if (*p != NUL && menu->children == NULL)
		{
		    EMSG(_(e_notsubmenu));
		    vim_free(path_name);
		    return FAIL;
		}
		else if ((menu->modes & modes) == 0x0)
		{
		    EMSG(_(e_othermode));
		    vim_free(path_name);
		    return FAIL;
		}
		break;
	    }
	    menu = menu->next;
	}
	if (menu == NULL)
	{
	    EMSG2(_(e_nomenu), name);
	    vim_free(path_name);
	    return FAIL;
	}
	name = p;
	parent = menu;
	menu = menu->children;
    }

    /* Now we have found the matching menu, and we list the mappings */
						    /* Highlight title */
    MSG_PUTS_TITLE(_("\n--- Menus ---"));

    show_menus_recursive(parent, modes, 0);
    return OK;
}

/*
 * Recursively show the mappings associated with the menus under the given one
 */
    static void
show_menus_recursive(menu, modes, depth)
    vimmenu_T	*menu;
    int		modes;
    int		depth;
{
    int		i;
    int		bit;

    if (menu != NULL && (menu->modes & modes) == 0x0)
	return;

    if (menu != NULL)
    {
	msg_putchar('\n');
	if (got_int)		/* "q" hit for "--more--" */
	    return;
	for (i = 0; i < depth; i++)
	    MSG_PUTS("  ");
	if (menu->priority)
	{
	    msg_outnum((long)menu->priority);
	    MSG_PUTS(" ");
	}
				/* Same highlighting as for directories!? */
	msg_outtrans_attr(menu->name, hl_attr(HLF_D));
    }

    if (menu != NULL && menu->children == NULL)
    {
	for (bit = 0; bit < MENU_MODES; bit++)
	    if ((menu->modes & modes & (1 << bit)) != 0)
	    {
		msg_putchar('\n');
		if (got_int)		/* "q" hit for "--more--" */
		    return;
		for (i = 0; i < depth + 2; i++)
		    MSG_PUTS("  ");
		msg_putchar(menu_mode_chars[bit]);
		if (menu->noremap[bit] == REMAP_NONE)
		    msg_putchar('*');
		else if (menu->noremap[bit] == REMAP_SCRIPT)
		    msg_putchar('&');
		else
		    msg_putchar(' ');
		if (menu->silent[bit])
		    msg_putchar('s');
		else
		    msg_putchar(' ');
		if ((menu->modes & menu->enabled & (1 << bit)) == 0)
		    msg_putchar('-');
		else
		    msg_putchar(' ');
		MSG_PUTS(" ");
		if (*menu->strings[bit] == NUL)
		    msg_puts_attr((char_u *)"<Nop>", hl_attr(HLF_8));
		else
		    msg_outtrans_special(menu->strings[bit], FALSE);
	    }
    }
    else
    {
	if (menu == NULL)
	{
	    menu = root_menu;
	    depth--;
	}
	else
	    menu = menu->children;

	/* recursively show all children.  Skip PopUp[nvoci]. */
	for (; menu != NULL && !got_int; menu = menu->next)
	    if (!menu_is_hidden(menu->dname))
		show_menus_recursive(menu, modes, depth + 1);
    }
}

#ifdef FEAT_CMDL_COMPL

/*
 * Used when expanding menu names.
 */
static vimmenu_T	*expand_menu = NULL;
static int		expand_modes = 0x0;
static int		expand_emenu;	/* TRUE for ":emenu" command */

/*
 * Work out what to complete when doing command line completion of menu names.
 */
    char_u *
set_context_in_menu_cmd(xp, cmd, arg, forceit)
    expand_T	*xp;
    char_u	*cmd;
    char_u	*arg;
    int		forceit;
{
    char_u	*after_dot;
    char_u	*p;
    char_u	*path_name = NULL;
    char_u	*name;
    int		unmenu;
    vimmenu_T	*menu;
    int		expand_menus;

    xp->xp_context = EXPAND_UNSUCCESSFUL;


    /* Check for priority numbers, enable and disable */
    for (p = arg; *p; ++p)
	if (!VIM_ISDIGIT(*p) && *p != '.')
	    break;

    if (!vim_iswhite(*p))
    {
	if (STRNCMP(arg, "enable", 6) == 0
		&& (arg[6] == NUL ||  vim_iswhite(arg[6])))
	    p = arg + 6;
	else if (STRNCMP(arg, "disable", 7) == 0
		&& (arg[7] == NUL || vim_iswhite(arg[7])))
	    p = arg + 7;
	else
	    p = arg;
    }

    while (*p != NUL && vim_iswhite(*p))
	++p;

    arg = after_dot = p;

    for (; *p && !vim_iswhite(*p); ++p)
    {
	if ((*p == '\\' || *p == Ctrl_V) && p[1] != NUL)
	    p++;
	else if (*p == '.')
	    after_dot = p + 1;
    }

    /* ":tearoff" and ":popup" only use menus, not entries */
    expand_menus = !((*cmd == 't' && cmd[1] == 'e') || *cmd == 'p');
    expand_emenu = (*cmd == 'e');
    if (expand_menus && vim_iswhite(*p))
	return NULL;	/* TODO: check for next command? */
    if (*p == NUL)		/* Complete the menu name */
    {
	/*
	 * With :unmenu, you only want to match menus for the appropriate mode.
	 * With :menu though you might want to add a menu with the same name as
	 * one in another mode, so match menus from other modes too.
	 */
	expand_modes = get_menu_cmd_modes(cmd, forceit, NULL, &unmenu);
	if (!unmenu)
	    expand_modes = MENU_ALL_MODES;

	menu = root_menu;
	if (after_dot != arg)
	{
	    path_name = alloc((unsigned)(after_dot - arg));
	    if (path_name == NULL)
		return NULL;
	    vim_strncpy(path_name, arg, after_dot - arg - 1);
	}
	name = path_name;
	while (name != NULL && *name)
	{
	    p = menu_name_skip(name);
	    while (menu != NULL)
	    {
		if (menu_name_equal(name, menu))
		{
		    /* Found menu */
		    if ((*p != NUL && menu->children == NULL)
			|| ((menu->modes & expand_modes) == 0x0))
		    {
			/*
			 * Menu path continues, but we have reached a leaf.
			 * Or menu exists only in another mode.
			 */
			vim_free(path_name);
			return NULL;
		    }
		    break;
		}
		menu = menu->next;
	    }
	    if (menu == NULL)
	    {
		/* No menu found with the name we were looking for */
		vim_free(path_name);
		return NULL;
	    }
	    name = p;
	    menu = menu->children;
	}
	vim_free(path_name);

	xp->xp_context = expand_menus ? EXPAND_MENUNAMES : EXPAND_MENUS;
	xp->xp_pattern = after_dot;
	expand_menu = menu;
    }
    else			/* We're in the mapping part */
	xp->xp_context = EXPAND_NOTHING;
    return NULL;
}

/*
 * Function given to ExpandGeneric() to obtain the list of (sub)menus (not
 * entries).
 */
/*ARGSUSED*/
    char_u *
get_menu_name(xp, idx)
    expand_T	*xp;
    int		idx;
{
    static vimmenu_T	*menu = NULL;
    char_u		*str;

    if (idx == 0)	    /* first call: start at first item */
	menu = expand_menu;

    /* Skip PopUp[nvoci]. */
    while (menu != NULL && (menu_is_hidden(menu->dname)
	    || menu_is_separator(menu->dname)
	    || menu_is_tearoff(menu->dname)
	    || menu->children == NULL))
	menu = menu->next;

    if (menu == NULL)	    /* at end of linked list */
	return NULL;

    if (menu->modes & expand_modes)
	str = menu->dname;
    else
	str = (char_u *)"";

    /* Advance to next menu entry. */
    menu = menu->next;

    return str;
}

/*
 * Function given to ExpandGeneric() to obtain the list of menus and menu
 * entries.
 */
/*ARGSUSED*/
    char_u *
get_menu_names(xp, idx)
    expand_T	*xp;
    int		idx;
{
    static vimmenu_T	*menu = NULL;
    static char_u	tbuffer[256]; /*hack*/
    char_u		*str;

    if (idx == 0)	    /* first call: start at first item */
	menu = expand_menu;

    /* Skip Browse-style entries, popup menus and separators. */
    while (menu != NULL
	    && (   menu_is_hidden(menu->dname)
		|| (expand_emenu && menu_is_separator(menu->dname))
		|| menu_is_tearoff(menu->dname)
#ifndef FEAT_BROWSE
		|| menu->dname[STRLEN(menu->dname) - 1] == '.'
#endif
	       ))
	menu = menu->next;

    if (menu == NULL)	    /* at end of linked list */
	return NULL;

    if (menu->modes & expand_modes)
    {
	if (menu->children != NULL)
	{
	    STRCPY(tbuffer, menu->dname);
	    /* hack on menu separators:  use a 'magic' char for the separator
	     * so that '.' in names gets escaped properly */
	    STRCAT(tbuffer, "\001");
	    str = tbuffer;
	}
	else
	    str = menu->dname;
    }
    else
	str = (char_u *)"";

    /* Advance to next menu entry. */
    menu = menu->next;

    return str;
}
#endif /* FEAT_CMDL_COMPL */

/*
 * Skip over this element of the menu path and return the start of the next
 * element.  Any \ and ^Vs are removed from the current element.
 * "name" may be modified.
 */
    char_u *
menu_name_skip(name)
    char_u  *name;
{
    char_u  *p;

    for (p = name; *p && *p != '.'; mb_ptr_adv(p))
    {
	if (*p == '\\' || *p == Ctrl_V)
	{
	    mch_memmove(p, p + 1, STRLEN(p));
	    if (*p == NUL)
		break;
	}
    }
    if (*p)
	*p++ = NUL;
    return p;
}

/*
 * Return TRUE when "name" matches with menu "menu".  The name is compared in
 * two ways: raw menu name and menu name without '&'.  ignore part after a TAB.
 */
    static int
menu_name_equal(name, menu)
    char_u	*name;
    vimmenu_T	*menu;
{
    return (menu_namecmp(name, menu->name) || menu_namecmp(name, menu->dname));
}

    static int
menu_namecmp(name, mname)
    char_u	*name;
    char_u	*mname;
{
    int		i;

    for (i = 0; name[i] != NUL && name[i] != TAB; ++i)
	if (name[i] != mname[i])
	    break;
    return ((name[i] == NUL || name[i] == TAB)
	    && (mname[i] == NUL || mname[i] == TAB));
}

/*
 * Return the modes specified by the given menu command (eg :menu! returns
 * MENU_CMDLINE_MODE | MENU_INSERT_MODE).
 * If "noremap" is not NULL, then the flag it points to is set according to
 * whether the command is a "nore" command.
 * If "unmenu" is not NULL, then the flag it points to is set according to
 * whether the command is an "unmenu" command.
 */
    static int
get_menu_cmd_modes(cmd, forceit, noremap, unmenu)
    char_u  *cmd;
    int	    forceit;	    /* Was there a "!" after the command? */
    int	    *noremap;
    int	    *unmenu;
{
    int	    modes;

    switch (*cmd++)
    {
	case 'v':			/* vmenu, vunmenu, vnoremenu */
	    modes = MENU_VISUAL_MODE | MENU_SELECT_MODE;
	    break;
	case 'x':			/* xmenu, xunmenu, xnoremenu */
	    modes = MENU_VISUAL_MODE;
	    break;
	case 's':			/* smenu, sunmenu, snoremenu */
	    modes = MENU_SELECT_MODE;
	    break;
	case 'o':			/* omenu */
	    modes = MENU_OP_PENDING_MODE;
	    break;
	case 'i':			/* imenu */
	    modes = MENU_INSERT_MODE;
	    break;
	case 't':
	    modes = MENU_TIP_MODE;	/* tmenu */
	    break;
	case 'c':			/* cmenu */
	    modes = MENU_CMDLINE_MODE;
	    break;
	case 'a':			/* amenu */
	    modes = MENU_INSERT_MODE | MENU_CMDLINE_MODE | MENU_NORMAL_MODE
				    | MENU_VISUAL_MODE | MENU_SELECT_MODE
				    | MENU_OP_PENDING_MODE;
	    break;
	case 'n':
	    if (*cmd != 'o')		/* nmenu, not noremenu */
	    {
		modes = MENU_NORMAL_MODE;
		break;
	    }
	    /* FALLTHROUGH */
	default:
	    --cmd;
	    if (forceit)		/* menu!! */
		modes = MENU_INSERT_MODE | MENU_CMDLINE_MODE;
	    else			/* menu */
		modes = MENU_NORMAL_MODE | MENU_VISUAL_MODE | MENU_SELECT_MODE
						       | MENU_OP_PENDING_MODE;
    }

    if (noremap != NULL)
	*noremap = (*cmd == 'n' ? REMAP_NONE : REMAP_YES);
    if (unmenu != NULL)
	*unmenu = (*cmd == 'u');
    return modes;
}

/*
 * Modify a menu name starting with "PopUp" to include the mode character.
 * Returns the name in allocated memory (NULL for failure).
 */
    static char_u *
popup_mode_name(name, idx)
    char_u	*name;
    int		idx;
{
    char_u	*p;
    int		len = (int)STRLEN(name);

    p = vim_strnsave(name, len + 1);
    if (p != NULL)
    {
	mch_memmove(p + 6, p + 5, (size_t)(len - 4));
	p[5] = menu_mode_chars[idx];
    }
    return p;
}

#if defined(FEAT_GUI) || defined(PROTO)
/*
 * Return the index into the menu->strings or menu->noremap arrays for the
 * current state.  Returns MENU_INDEX_INVALID if there is no mapping for the
 * given menu in the current mode.
 */
    int
get_menu_index(menu, state)
    vimmenu_T	*menu;
    int		state;
{
    int		idx;

    if ((state & INSERT))
	idx = MENU_INDEX_INSERT;
    else if (state & CMDLINE)
	idx = MENU_INDEX_CMDLINE;
#ifdef FEAT_VISUAL
    else if (VIsual_active)
    {
	if (VIsual_select)
	    idx = MENU_INDEX_SELECT;
	else
	    idx = MENU_INDEX_VISUAL;
    }
#endif
    else if (state == HITRETURN || state == ASKMORE)
	idx = MENU_INDEX_CMDLINE;
    else if (finish_op)
	idx = MENU_INDEX_OP_PENDING;
    else if ((state & NORMAL))
	idx = MENU_INDEX_NORMAL;
    else
	idx = MENU_INDEX_INVALID;

    if (idx != MENU_INDEX_INVALID && menu->strings[idx] == NULL)
	idx = MENU_INDEX_INVALID;
    return idx;
}
#endif

/*
 * Duplicate the menu item text and then process to see if a mnemonic key
 * and/or accelerator text has been identified.
 * Returns a pointer to allocated memory, or NULL for failure.
 * If mnemonic != NULL, *mnemonic is set to the character after the first '&'.
 * If actext != NULL, *actext is set to the text after the first TAB.
 */
    static char_u *
menu_text(str, mnemonic, actext)
    char_u	*str;
    int		*mnemonic;
    char_u	**actext;
{
    char_u	*p;
    char_u	*text;

    /* Locate accelerator text, after the first TAB */
    p = vim_strchr(str, TAB);
    if (p != NULL)
    {
	if (actext != NULL)
	    *actext = vim_strsave(p + 1);
	text = vim_strnsave(str, (int)(p - str));
    }
    else
	text = vim_strsave(str);

    /* Find mnemonic characters "&a" and reduce "&&" to "&". */
    for (p = text; p != NULL; )
    {
	p = vim_strchr(p, '&');
	if (p != NULL)
	{
	    if (p[1] == NUL)	    /* trailing "&" */
		break;
	    if (mnemonic != NULL && p[1] != '&')
#if !defined(__MVS__) || defined(MOTIF390_MNEMONIC_FIXED)
		*mnemonic = p[1];
#else
	    {
		/*
		 * Well there is a bug in the Motif libraries on OS390 Unix.
		 * The mnemonic keys needs to be converted to ASCII values
		 * first.
		 * This behavior has been seen in 2.8 and 2.9.
		 */
		char c = p[1];
		__etoa_l(&c, 1);
		*mnemonic = c;
	    }
#endif
	    mch_memmove(p, p + 1, STRLEN(p));
	    p = p + 1;
	}
    }
    return text;
}

/*
 * Return TRUE if "name" can be a menu in the MenuBar.
 */
    int
menu_is_menubar(name)
    char_u	*name;
{
    return (!menu_is_popup(name)
	    && !menu_is_toolbar(name)
	    && *name != MNU_HIDDEN_CHAR);
}

/*
 * Return TRUE if "name" is a popup menu name.
 */
    int
menu_is_popup(name)
    char_u	*name;
{
    return (STRNCMP(name, "PopUp", 5) == 0);
}

#if (defined(FEAT_GUI_MOTIF) && (XmVersion <= 1002)) || defined(PROTO)
/*
 * Return TRUE if "name" is part of a popup menu.
 */
    int
menu_is_child_of_popup(menu)
    vimmenu_T *menu;
{
    while (menu->parent != NULL)
	menu = menu->parent;
    return menu_is_popup(menu->name);
}
#endif

/*
 * Return TRUE if "name" is a toolbar menu name.
 */
    int
menu_is_toolbar(name)
    char_u	*name;
{
    return (STRNCMP(name, "ToolBar", 7) == 0);
}

/*
 * Return TRUE if the name is a menu separator identifier: Starts and ends
 * with '-'
 */
    int
menu_is_separator(name)
    char_u *name;
{
    return (name[0] == '-' && name[STRLEN(name) - 1] == '-');
}

/*
 * Return TRUE if the menu is hidden:  Starts with ']'
 */
    static int
menu_is_hidden(name)
    char_u *name;
{
    return (name[0] == ']') || (menu_is_popup(name) && name[5] != NUL);
}

#if defined(FEAT_CMDL_COMPL) \
	|| (defined(FEAT_GUI_W32) && defined(FEAT_TEAROFF))
/*
 * Return TRUE if the menu is the tearoff menu.
 */
/*ARGSUSED*/
    static int
menu_is_tearoff(name)
    char_u *name;
{
#ifdef FEAT_GUI
    return (STRCMP(name, TEAR_STRING) == 0);
#else
    return FALSE;
#endif
}
#endif

#ifdef FEAT_GUI

    static int
get_menu_mode()
{
#ifdef FEAT_VISUAL
    if (VIsual_active)
    {
	if (VIsual_select)
	    return MENU_INDEX_SELECT;
	return MENU_INDEX_VISUAL;
    }
#endif
    if (State & INSERT)
	return MENU_INDEX_INSERT;
    if ((State & CMDLINE) || State == ASKMORE || State == HITRETURN)
	return MENU_INDEX_CMDLINE;
    if (finish_op)
	return MENU_INDEX_OP_PENDING;
    if (State & NORMAL)
	return MENU_INDEX_NORMAL;
    if (State & LANGMAP)	/* must be a "r" command, like Insert mode */
	return MENU_INDEX_INSERT;
    return MENU_INDEX_INVALID;
}

/*
 * After we have started the GUI, then we can create any menus that have been
 * defined.  This is done once here.  add_menu_path() may have already been
 * called to define these menus, and may be called again.  This function calls
 * itself recursively.	Should be called at the top level with:
 * gui_create_initial_menus(root_menu, NULL);
 */
    void
gui_create_initial_menus(menu)
    vimmenu_T	*menu;
{
    int		idx = 0;

    while (menu != NULL)
    {
	/* Don't add a menu when only a tip was defined. */
	if (menu->modes & MENU_ALL_MODES)
	{
	    if (menu->children != NULL)
	    {
		gui_mch_add_menu(menu, idx);
		gui_create_initial_menus(menu->children);
	    }
	    else
		gui_mch_add_menu_item(menu, idx);
	}
	menu = menu->next;
	++idx;
    }
}

/*
 * Used recursively by gui_update_menus (see below)
 */
    static void
gui_update_menus_recurse(menu, mode)
    vimmenu_T	*menu;
    int		mode;
{
    int		grey;

    while (menu)
    {
	if ((menu->modes & menu->enabled & mode)
#if defined(FEAT_GUI_W32) && defined(FEAT_TEAROFF)
		|| menu_is_tearoff(menu->dname)
#endif
	   )
	    grey = FALSE;
	else
	    grey = TRUE;
#ifdef FEAT_GUI_ATHENA
	/* Hiding menus doesn't work for Athena, it can cause a crash. */
	gui_mch_menu_grey(menu, grey);
#else
	/* Never hide a toplevel menu, it may make the menubar resize or
	 * disappear. Same problem for ToolBar items. */
	if (vim_strchr(p_go, GO_GREY) != NULL || menu->parent == NULL
# ifdef FEAT_TOOLBAR
		|| menu_is_toolbar(menu->parent->name)
# endif
		   )
	    gui_mch_menu_grey(menu, grey);
	else
	    gui_mch_menu_hidden(menu, grey);
#endif
	gui_update_menus_recurse(menu->children, mode);
	menu = menu->next;
    }
}

/*
 * Make sure only the valid menu items appear for this mode.  If
 * force_menu_update is not TRUE, then we only do this if the mode has changed
 * since last time.  If "modes" is not 0, then we use these modes instead.
 */
    void
gui_update_menus(modes)
    int	    modes;
{
    static int	    prev_mode = -1;
    int		    mode = 0;

    if (modes != 0x0)
	mode = modes;
    else
    {
	mode = get_menu_mode();
	if (mode == MENU_INDEX_INVALID)
	    mode = 0;
	else
	    mode = (1 << mode);
    }

    if (force_menu_update || mode != prev_mode)
    {
	gui_update_menus_recurse(root_menu, mode);
	gui_mch_draw_menubar();
	prev_mode = mode;
	force_menu_update = FALSE;
#ifdef FEAT_GUI_W32
	/* This can leave a tearoff as active window - make sure we
	 * have the focus <negri>*/
	gui_mch_activate_window();
#endif
    }
}

#if defined(FEAT_GUI_MSWIN) || defined(FEAT_GUI_MOTIF) \
    || defined(FEAT_GUI_GTK) || defined(FEAT_GUI_PHOTON) || defined(PROTO)
/*
 * Check if a key is used as a mnemonic for a toplevel menu.
 * Case of the key is ignored.
 */
    int
gui_is_menu_shortcut(key)
    int		key;
{
    vimmenu_T	*menu;

    if (key < 256)
	key = TOLOWER_LOC(key);
    for (menu = root_menu; menu != NULL; menu = menu->next)
	if (menu->mnemonic == key
		|| (menu->mnemonic < 256 && TOLOWER_LOC(menu->mnemonic) == key))
	    return TRUE;
    return FALSE;
}
#endif

/*
 * Display the Special "PopUp" menu as a pop-up at the current mouse
 * position.  The "PopUpn" menu is for Normal mode, "PopUpi" for Insert mode,
 * etc.
 */
    void
gui_show_popupmenu()
{
    vimmenu_T	*menu;
    int		mode;

    mode = get_menu_mode();
    if (mode == MENU_INDEX_INVALID)
	return;
    mode = menu_mode_chars[mode];

#ifdef FEAT_AUTOCMD
    {
	char_u	    ename[2];

	ename[0] = mode;
	ename[1] = NUL;
	apply_autocmds(EVENT_MENUPOPUP, ename, NULL, FALSE, curbuf);
    }
#endif

    for (menu = root_menu; menu != NULL; menu = menu->next)
	if (STRNCMP("PopUp", menu->name, 5) == 0 && menu->name[5] == mode)
	    break;

    /* Only show a popup when it is defined and has entries */
    if (menu != NULL && menu->children != NULL)
	gui_mch_show_popupmenu(menu);
}
#endif /* FEAT_GUI */

#if (defined(FEAT_GUI_W32) && defined(FEAT_TEAROFF)) || defined(PROTO)

/*
 * Deal with tearoff items that are added like a menu item.
 * Currently only for Win32 GUI.  Others may follow later.
 */

    void
gui_mch_toggle_tearoffs(int enable)
{
    int		pri_tab[MENUDEPTH + 1];
    int		i;

    if (enable)
    {
	for (i = 0; i < MENUDEPTH; ++i)
	    pri_tab[i] = 500;
	pri_tab[MENUDEPTH] = -1;
	gui_create_tearoffs_recurse(root_menu, (char_u *)"", pri_tab, 0);
    }
    else
	gui_destroy_tearoffs_recurse(root_menu);
    s_tearoffs = enable;
}

/*
 * Recursively add tearoff items
 */
    static void
gui_create_tearoffs_recurse(menu, pname, pri_tab, pri_idx)
    vimmenu_T		*menu;
    const char_u	*pname;
    int			*pri_tab;
    int			pri_idx;
{
    char_u	*newpname = NULL;
    int		len;
    char_u	*s;
    char_u	*d;

    if (pri_tab[pri_idx + 1] != -1)
	++pri_idx;
    while (menu != NULL)
    {
	if (menu->children != NULL && menu_is_menubar(menu->name))
	{
	    /* Add the menu name to the menu path.  Insert a backslash before
	     * dots (it's used to separate menu names). */
	    len = (int)STRLEN(pname) + (int)STRLEN(menu->name);
	    for (s = menu->name; *s; ++s)
		if (*s == '.' || *s == '\\')
		    ++len;
	    newpname = alloc(len + TEAR_LEN + 2);
	    if (newpname != NULL)
	    {
		STRCPY(newpname, pname);
		d = newpname + STRLEN(newpname);
		for (s = menu->name; *s; ++s)
		{
		    if (*s == '.' || *s == '\\')
			*d++ = '\\';
		    *d++ = *s;
		}
		*d = NUL;

		/* check if tearoff already exists */
		if (STRCMP(menu->children->name, TEAR_STRING) != 0)
		{
		    gui_add_tearoff(newpname, pri_tab, pri_idx - 1);
		    *d = NUL;			/* remove TEAR_STRING */
		}

		STRCAT(newpname, ".");
		gui_create_tearoffs_recurse(menu->children, newpname,
							    pri_tab, pri_idx);
		vim_free(newpname);
	    }
	}
	menu = menu->next;
    }
}

/*
 * Add tear-off menu item for a submenu.
 * "tearpath" is the menu path, and must have room to add TEAR_STRING.
 */
    static void
gui_add_tearoff(tearpath, pri_tab, pri_idx)
    char_u	*tearpath;
    int		*pri_tab;
    int		pri_idx;
{
    char_u	*tbuf;
    int		t;
    vimmenu_T	menuarg;

    tbuf = alloc(5 + (unsigned int)STRLEN(tearpath));
    if (tbuf != NULL)
    {
	tbuf[0] = K_SPECIAL;
	tbuf[1] = K_SECOND(K_TEAROFF);
	tbuf[2] = K_THIRD(K_TEAROFF);
	STRCPY(tbuf + 3, tearpath);
	STRCAT(tbuf + 3, "\r");

	STRCAT(tearpath, ".");
	STRCAT(tearpath, TEAR_STRING);

	/* Priority of tear-off is always 1 */
	t = pri_tab[pri_idx + 1];
	pri_tab[pri_idx + 1] = 1;

#ifdef FEAT_TOOLBAR
	menuarg.iconfile = NULL;
	menuarg.iconidx = -1;
	menuarg.icon_builtin = FALSE;
#endif
	menuarg.noremap[0] = REMAP_NONE;
	menuarg.silent[0] = TRUE;

	menuarg.modes = MENU_ALL_MODES;
	add_menu_path(tearpath, &menuarg, pri_tab, tbuf, FALSE);

	menuarg.modes = MENU_TIP_MODE;
	add_menu_path(tearpath, &menuarg, pri_tab,
		(char_u *)_("Tear off this menu"), FALSE);

	pri_tab[pri_idx + 1] = t;
	vim_free(tbuf);
    }
}

/*
 * Recursively destroy tearoff items
 */
    static void
gui_destroy_tearoffs_recurse(menu)
    vimmenu_T	*menu;
{
    while (menu)
    {
	if (menu->children)
	{
	    /* check if tearoff exists */
	    if (STRCMP(menu->children->name, TEAR_STRING) == 0)
	    {
		/* Disconnect the item and free the memory */
		free_menu(&menu->children);
	    }
	    if (menu->children != NULL) /* if not the last one */
		gui_destroy_tearoffs_recurse(menu->children);
	}
	menu = menu->next;
    }
}

#endif /* FEAT_GUI_W32 && FEAT_TEAROFF */

/*
 * Given a menu descriptor, e.g. "File.New", find it in the menu hierarchy and
 * execute it.
 */
    void
ex_emenu(eap)
    exarg_T	*eap;
{
    vimmenu_T	*menu;
    char_u	*name;
    char_u	*saved_name;
    char_u	*p;
    int		idx;
    char_u	*mode;

    saved_name = vim_strsave(eap->arg);
    if (saved_name == NULL)
	return;

    menu = root_menu;
    name = saved_name;
    while (*name)
    {
	/* Find in the menu hierarchy */
	p = menu_name_skip(name);

	while (menu != NULL)
	{
	    if (menu_name_equal(name, menu))
	    {
		if (*p == NUL && menu->children != NULL)
		{
		    EMSG(_("E333: Menu path must lead to a menu item"));
		    menu = NULL;
		}
		else if (*p != NUL && menu->children == NULL)
		{
		    EMSG(_(e_notsubmenu));
		    menu = NULL;
		}
		break;
	    }
	    menu = menu->next;
	}
	if (menu == NULL || *p == NUL)
	    break;
	menu = menu->children;
	name = p;
    }
    vim_free(saved_name);
    if (menu == NULL)
    {
	EMSG2(_("E334: Menu not found: %s"), eap->arg);
	return;
    }

    /* Found the menu, so execute.
     * Use the Insert mode entry when returning to Insert mode. */
    if (restart_edit
#ifdef FEAT_EVAL
	    && !current_SID
#endif
	    )
    {
	mode = (char_u *)"Insert";
	idx = MENU_INDEX_INSERT;
    }
    else if (eap->addr_count)
    {
	pos_T	tpos;

	mode = (char_u *)"Visual";
	idx = MENU_INDEX_VISUAL;

	/* GEDDES: This is not perfect - but it is a
	 * quick way of detecting whether we are doing this from a
	 * selection - see if the range matches up with the visual
	 * select start and end.  */
	if ((curbuf->b_visual.vi_start.lnum == eap->line1)
		&& (curbuf->b_visual.vi_end.lnum) == eap->line2)
	{
	    /* Set it up for visual mode - equivalent to gv.  */
	    VIsual_mode = curbuf->b_visual.vi_mode;
	    tpos = curbuf->b_visual.vi_end;
	    curwin->w_cursor = curbuf->b_visual.vi_start;
	    curwin->w_curswant = curbuf->b_visual.vi_curswant;
	}
	else
	{
	    /* Set it up for line-wise visual mode */
	    VIsual_mode = 'V';
	    curwin->w_cursor.lnum = eap->line1;
	    curwin->w_cursor.col = 1;
	    tpos.lnum = eap->line2;
	    tpos.col = MAXCOL;
#ifdef FEAT_VIRTUALEDIT
	    tpos.coladd = 0;
#endif
	}

	/* Activate visual mode */
	VIsual_active = TRUE;
	VIsual_reselect = TRUE;
	check_cursor();
	VIsual = curwin->w_cursor;
	curwin->w_cursor = tpos;

	check_cursor();

	/* Adjust the cursor to make sure it is in the correct pos
	 * for exclusive mode */
	if (*p_sel == 'e' && gchar_cursor() != NUL)
	    ++curwin->w_cursor.col;
    }
    else
    {
	mode = (char_u *)"Normal";
	idx = MENU_INDEX_NORMAL;
    }

    if (idx != MENU_INDEX_INVALID && menu->strings[idx] != NULL)
    {
	/* When executing a script or function execute the commands right now.
	 * Otherwise put them in the typeahead buffer. */
#ifdef FEAT_En
	if (current_SID != 0)
	    exec_normal_cmd(menu->strings[idx], menu->noremap[idx],
							   menu->silent[idx]);
	else
#endif
	    ins_typebuf(menu->strings[idx], menu->noremap[idx], 0,
						     TRUE, menu->silent[idx]);
    }
    else
	EMSG2(_("E335: Menu not defined for %s mode"), mode);
}

#if defined(FEAT_GUI_MSWIN) \
	|| (defined(FEAT_GUI_GTK) && defined(FEAT_MENU)) \
	|| defined(FEAT_BEVAL_TIP) || defined(PROTO)
/*
 * Given a menu descriptor, e.g. "File.New", find it in the menu hierarchy.
 */
    vimmenu_T *
gui_find_menu(path_name)
    char_u *path_name;
{
    vimmenu_T	*menu = NULL;
    char_u	*name;
    char_u	*saved_name;
    char_u	*p;

    menu = root_menu;

    saved_name = vim_strsave(path_name);
    if (saved_name == NULL)
	return NULL;

    name = saved_name;
    while (*name)
    {
	/* find the end of one dot-separated name and put a NUL at the dot */
	p = menu_name_skip(name);

	while (menu != NULL)
	{
	    if (STRCMP(name, menu->name) == 0 || STRCMP(name, menu->dname) == 0)
	    {
		if (menu->children == NULL)
		{
		    /* found a menu item instead of a sub-menu */
		    if (*p == NUL)
			EMSG(_("E336: Menu path must lead to a sub-menu"));
		    else
			EMSG(_(e_notsubmenu));
		    menu = NULL;
		    goto theend;
		}
		if (*p == NUL)	    /* found a full match */
		    goto theend;
		break;
	    }
	    menu = menu->next;
	}
	if (menu == NULL)	/* didn't find it */
	    break;

	/* Found a match, search the sub-menu. */
	menu = menu->children;
	name = p;
    }

    if (menu == NULL)
	EMSG(_("E337: Menu not found - check menu names"));
theend:
    vim_free(saved_name);
    return menu;
}
#endif

#ifdef FEAT_MULTI_LANG
/*
 * Translation of menu names.  Just a simple lookup table.
 */

typedef struct
{
    char_u	*from;		/* English name */
    char_u	*from_noamp;	/* same, without '&' */
    char_u	*to;		/* translated name */
} menutrans_T;

static garray_T menutrans_ga = {0, 0, 0, 0, NULL};
#endif

/*
 * ":menutrans".
 * This function is also defined without the +multi_lang feature, in which
 * case the commands are ignored.
 */
/*ARGSUSED*/
    void
ex_menutranslate(eap)
    exarg_T	*eap;
{
#ifdef FEAT_MULTI_LANG
    char_u		*arg = eap->arg;
    menutrans_T		*tp;
    int			i;
    char_u		*from, *from_noamp, *to;

    if (menutrans_ga.ga_itemsize == 0)
	ga_init2(&menutrans_ga, (int)sizeof(menutrans_T), 5);

    /*
     * ":menutrans clear": clear all translations.
     */
    if (STRNCMP(arg, "clear", 5) == 0 && ends_excmd(*skipwhite(arg + 5)))
    {
	tp = (menutrans_T *)menutrans_ga.ga_data;
	for (i = 0; i < menutrans_ga.ga_len; ++i)
	{
	    vim_free(tp[i].from);
	    vim_free(tp[i].from_noamp);
	    vim_free(tp[i].to);
	}
	ga_clear(&menutrans_ga);
# ifdef FEAT_EVAL
	/* Delete all "menutrans_" global variables. */
	del_menutrans_vars();
# endif
    }
    else
    {
	/* ":menutrans from to": add translation */
	from = arg;
	arg = menu_skip_part(arg);
	to = skipwhite(arg);
	*arg = NUL;
	arg = menu_skip_part(to);
	if (arg == to)
	    EMSG(_(e_invarg));
	else
	{
	    if (ga_grow(&menutrans_ga, 1) == OK)
	    {
		tp = (menutrans_T *)menutrans_ga.ga_data;
		from = vim_strsave(from);
		if (from != NULL)
		{
		    from_noamp = menu_text(from, NULL, NULL);
		    to = vim_strnsave(to, (int)(arg - to));
		    if (from_noamp != NULL && to != NULL)
		    {
			tp[menutrans_ga.ga_len].from = from;
			tp[menutrans_ga.ga_len].from_noamp = from_noamp;
			tp[menutrans_ga.ga_len].to = to;
			++menutrans_ga.ga_len;
		    }
		    else
		    {
			vim_free(from);
			vim_free(from_noamp);
			vim_free(to);
		    }
		}
	    }
	}
    }
#endif
}

#if defined(FEAT_MULTI_LANG) || defined(FEAT_TOOLBAR)
/*
 * Find the character just after one part of a menu name.
 */
    static char_u *
menu_skip_part(p)
    char_u	*p;
{
    while (*p != NUL && *p != '.' && !vim_iswhite(*p))
    {
	if ((*p == '\\' || *p == Ctrl_V) && p[1] != NUL)
	    ++p;
	++p;
    }
    return p;
}
#endif

#ifdef FEAT_MULTI_LANG
/*
 * Lookup part of a menu name in the translations.
 * Return a pointer to the translation or NULL if not found.
 */
    static char_u *
menutrans_lookup(name, len)
    char_u	*name;
    int		len;
{
    menutrans_T		*tp = (menutrans_T *)menutrans_ga.ga_data;
    int			i;
    char_u		*dname;

    for (i = 0; i < menutrans_ga.ga_len; ++i)
	if (STRNCMP(name, tp[i].from, len) == 0 && tp[i].from[len] == NUL)
	    return tp[i].to;

    /* Now try again while ignoring '&' characters. */
    i = name[len];
    name[len] = NUL;
    dname = menu_text(name, NULL, NULL);
    name[len] = i;
    if (dname != NULL)
    {
	for (i = 0; i < menutrans_ga.ga_len; ++i)
	    if (STRCMP(dname, tp[i].from_noamp) == 0)
	    {
		vim_free(dname);
		return tp[i].to;
	    }
	vim_free(dname);
    }

    return NULL;
}
#endif /* FEAT_MULTI_LANG */

#endif /* FEAT_MENU */
