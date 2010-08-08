/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *			Visual Workshop integration by Gordon Prieur
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

#ifdef HAVE_CONFIG_H
# include "auto/config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/socket.h>
#ifdef HAVE_LIBGEN_H
# include <libgen.h>
#endif
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/PushB.h>

#include "integration.h"	/* <EditPlugin/integration.h> */

#include "vim.h"
#include "version.h"
#include "gui_beval.h"
#include "workshop.h"

void		 workshop_hotkeys(Boolean);

static Boolean	 isShowing(int);
static win_T	*get_window(buf_T *);
static void	 updatePriority(Boolean);
static char	*addUniqueMnemonic(char *, char *);
static char	*fixup(char *);
static char	*get_selection(buf_T *);
static char	*append_selection(int, char *, int *, int *);
static void	 load_buffer_by_name(char *, int);
static void	 load_window(char *, int lnum);
static void	 warp_to_pc(int);
#ifdef FEAT_BEVAL
void		 workshop_beval_cb(BalloonEval *, int);
static int	 computeIndex(int, char_u *, int);
#endif
static char	*fixAccelText(char *);
static void	 addMenu(char *, char *, char *);
static char	*lookupVerb(char *, int);
static void	 coloncmd(char *, Boolean);

extern Widget	 vimShell;
extern Widget	 textArea;
extern XtAppContext app_context;

static int	 tbpri;			/* ToolBar priority */
int		 usingSunWorkShop = 0;	/* set if -ws flag is used */
char		 curMenuName[BUFSIZ];
char		 curMenuPriority[BUFSIZ];

static Boolean	 workshopInitDone = False;
static Boolean	 workshopHotKeysEnabled = False;

/*
 * The following enum is from <gp_dbx/gp_dbx_common.h>. We can't include it
 * here because its C++.
 */
enum
{
    GPLineEval_EVALUATE,		/* evaluate expression */
    GPLineEval_INDIRECT,		/* evaluate *<expression> */
    GPLineEval_TYPE			/* type of expression */
};

/*
 * Store each verb in the MenuMap. This lets us map from a verb to a menu.
 * There may be multiple matches for a single verb in this table.
 */
#define MENU_INC	50		/* menuMap incremental size increases */
typedef struct
{
    char	*name;			/* name of the menu */
    char	*accel;			/* optional accelerator key */
    char	*verb;			/* menu verb */
} MenuMap;
static MenuMap	*menuMap;		/* list of verb/menu mappings */
static int	 menuMapSize;		/* current size of menuMap */
static int	 menuMapMax;		/* allocated size of menuMap */
static char	*initialFileCmd;	/* save command but defer doing it */


    void
workshop_init()
{
    char_u	 buf[64];
    int		 is_dirty = FALSE;
    int		 width, height;
    XtInputMask	 mask;

    /*
     * Turn on MenuBar, ToolBar, and Footer.
     */
    STRCPY(buf, p_go);
    if (vim_strchr(p_go, GO_MENUS) == NULL)
    {
	STRCAT(buf, "m");
	is_dirty = TRUE;
    }
    if (vim_strchr(p_go, GO_TOOLBAR) == NULL)
    {
	STRCAT(buf, "T");
	is_dirty = TRUE;
    }
    if (vim_strchr(p_go, GO_FOOTER) == NULL)
    {
	STRCAT(buf, "F");
	is_dirty = TRUE;
    }
    if (is_dirty)
	set_option_value((char_u *)"go", 0L, buf, 0);

    /*
     * Set size from workshop_get_width_height().
     */
    width = height = 0;
    if (workshop_get_width_height(&width, &height))
    {
	XtVaSetValues(vimShell,
		XmNwidth, width,
		XmNheight, height,
		NULL);
    }

    /*
     * Now read in the initial messages from eserve.
     */
    while ((mask = XtAppPending(app_context))
	    && (mask & XtIMAlternateInput) && !workshopInitDone)
	XtAppProcessEvent(app_context, (XtInputMask)XtIMAlternateInput);
}

    void
workshop_postinit()
{
    do_cmdline_cmd((char_u *)initialFileCmd);
    ALT_INPUT_LOCK_OFF;
    free(initialFileCmd);
    initialFileCmd = NULL;
}

    void
ex_wsverb(exarg_T *eap)
{
    msg_clr_cmdline();
    workshop_perform_verb((char *) eap->arg, NULL);
}

/*
 * Editor name
 * This string is recognized by eserve and should be all lower case.
 * This is how the editor detects that it is talking to gvim instead
 * of NEdit, for example, when the connection is initiated from the editor.
 */
    char *
workshop_get_editor_name()
{
    return "gvim";
}

/*
 * Version number of the editor.
 * This number is communicated along with the protocol
 * version to the application.
 */
    char *
workshop_get_editor_version()
{
    return Version;
}

/*
 * Answer functions: called by eserve
 */

/*
 * Name:
 *	workshop_load_file
 *
 * Function:
 *	Load a given file into the WorkShop buffer.
 */
    void
workshop_load_file(
	char	*filename,		/* the file to load */
	int	 line,			/* an optional line number (or 0) */
	char	*frameid UNUSED)	/* used for multi-frame support */
{
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_load_file(%s, %d)\n", filename, line);
#endif

#ifdef FEAT_BEVAL
    bevalServers |= BEVAL_WORKSHOP;
#endif

    load_window(filename, line);
}

/*
 * Reload the WorkShop buffer
 */
    void
workshop_reload_file(
	char	*filename,
	int	 line)
{
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_reload_file(%s, %d)\n", filename, line);
#endif
    load_window(filename, line);
}

    void
workshop_show_file(
    char	*filename)
{
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_show_file(%s)\n", filename);
#endif

    load_window(filename, 0);
}

    void
workshop_goto_line(
    char	*filename,
    int		 lineno)
{
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_goto_line(%s, %d)\n", filename, lineno);
#endif

    load_window(filename, lineno);
}

    void
workshop_front_file(
	char	*filename UNUSED)
{
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_front_file()\n");
#endif
    /*
     * Assumption: This function will always be called after a call to
     * workshop_show_file(), so the file is always showing.
     */
    if (vimShell != NULL)
	XRaiseWindow(gui.dpy, XtWindow(vimShell));
}

    void
workshop_save_file(
	    char	*filename)
{
    char	 cbuf[BUFSIZ];		/* build vim command here */

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_save_file(%s)\n", filename);
#endif

    /* Save the given file */
    vim_snprintf(cbuf, sizeof(cbuf), "w %s", filename);
    coloncmd(cbuf, TRUE);
}

    void
workshop_save_files()
{
    /* Save the given file */
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_save_files()\n");
#endif

    add_to_input_buf((char_u *) ":wall\n", 6);
}

    void
workshop_quit()
{
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_quit()\n");
#endif

    add_to_input_buf((char_u *) ":qall\n", 6);
}

    void
workshop_minimize()
{
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_minimize()\n");
#endif
    workshop_minimize_shell(vimShell);
}
    void
workshop_maximize()
{
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_maximize()\n");
#endif

    workshop_maximize_shell(vimShell);
}

    void
workshop_add_mark_type(
	int		 idx,
	char		*colorspec,
	char		*sign)
{
    char	 gbuf[BUFSIZ];	/* buffer for sign name */
    char	 cibuf[BUFSIZ];	/* color information */
    char	 cbuf[BUFSIZ];	/* command buffer */
    char	*bp;

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
    {
	char *cp;

	cp = strrchr(sign, '/');
	if (cp == NULL)
	    cp = sign;
	else
	    cp++;		/* skip '/' character */
	wstrace("workshop_add_mark_type(%d, \"%s\", \"%s\")\n", idx,
		colorspec && *colorspec ? colorspec : "<None>", cp);
    }
#endif

    /*
     * Isolate the basename of sign in gbuf. We will use this for the
     * GroupName in the highlight command sent to vim.
     */
    STRCPY(gbuf, gettail((char_u *)sign));
    bp = strrchr(gbuf, '.');
    if (bp != NULL)
	*bp = NUL;

    if (gbuf[0] != '-' && gbuf[1] != NUL)
    {
	if (colorspec != NULL && *colorspec)
	{
	    vim_snprintf(cbuf, sizeof(cbuf),
				  "highlight WS%s guibg=%s", gbuf, colorspec);
	    coloncmd(cbuf, FALSE);
	    vim_snprintf(cibuf, sizeof(cibuf), "linehl=WS%s", gbuf);
	}
	else
	    cibuf[0] = NUL;

	vim_snprintf(cbuf, sizeof(cbuf),
			       "sign define %d %s icon=%s", idx, cibuf, sign);
	coloncmd(cbuf, TRUE);
    }
}

    void
workshop_set_mark(
	char		*filename,	/* filename which gets the mark */
	int		 lineno,	/* line number which gets the mark */
	int		 markId,	/* unique mark identifier */
	int		 idx)		/* which mark to use */
{
    char	cbuf[BUFSIZ];	/* command buffer */

    /* Set mark in a given file */
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_set_mark(%s, %d (ln), %d (id), %d (idx))\n",
		filename, lineno, markId, idx);
#endif

    vim_snprintf(cbuf, sizeof(cbuf), "sign place %d line=%d name=%d file=%s",
					       markId, lineno, idx, filename);
    coloncmd(cbuf, TRUE);
}

    void
workshop_change_mark_type(
	char		*filename,	/* filename which gets the mark */
	int		 markId,	/* unique mark identifier */
	int		 idx)		/* which mark to use */
{
    char	cbuf[BUFSIZ];	/* command buffer */

    /* Change mark type */
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_change_mark_type(%s, %d, %d)\n",
		filename, markId, idx);
#endif

    vim_snprintf(cbuf, sizeof(cbuf),
		      "sign place %d name=%d file=%s", markId, idx, filename);
    coloncmd(cbuf, TRUE);
}

/*
 * Goto the given mark in a file (e.g. show it).
 * If message is not null, display it in the footer.
 */
    void
workshop_goto_mark(
	char		*filename,
	int		 markId,
	char		*message)
{
    char	cbuf[BUFSIZ];	/* command buffer */

    /* Goto mark */
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_goto_mark(%s, %d (id), %s)\n",
		filename, markId, message && *message &&
		!(*message == ' ' && message[1] == NULL) ?
		message : "<None>");
#endif

    vim_snprintf(cbuf, sizeof(cbuf), "sign jump %d file=%s", markId, filename);
    coloncmd(cbuf, TRUE);
    if (message != NULL && *message != NUL)
	gui_mch_set_footer((char_u *)message);
}

    void
workshop_delete_mark(
	char		*filename,
	int		 markId)
{
    char	cbuf[BUFSIZ];	/* command buffer */

    /* Delete mark */
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_delete_mark(%s, %d (id))\n",
		filename, markId);
#endif

    vim_snprintf(cbuf, sizeof(cbuf),
				 "sign unplace %d file=%s", markId, filename);
    coloncmd(cbuf, TRUE);
}

    int
workshop_get_mark_lineno(
	char	*filename,
	int	 markId)
{
    buf_T	*buf;		/* buffer containing filename */
    int		lineno;		/* line number of filename in buf */

    /* Get mark line number */
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_get_mark_lineno(%s, %d)\n",
		filename, markId);
#endif

    lineno = 0;
    buf = buflist_findname((char_u *)filename);
    if (buf != NULL)
	lineno = buf_findsign(buf, markId);

    return lineno;
}


/*
 * Are there any moved marks? If so, call workshop_move_mark on
 * each of them now. This is how eserve can find out if for example
 * breakpoints have moved when a program has been recompiled and
 * reloaded into dbx.
 */
    void
workshop_moved_marks(char *filename UNUSED)
{
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("XXXworkshop_moved_marks(%s)\n", filename);
#endif
}

    int
workshop_get_font_height()
{
    XmFontList	 fontList;	/* fontList made from gui.norm_font */
    XmString	 str;
    Dimension	 w;
    Dimension	 h;

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_get_font_height()\n");
#endif

    /* Pick the proper signs for this font size */
    fontList = gui_motif_create_fontlist((XFontStruct *)gui.norm_font);
    h = 0;
    if (fontList != NULL)
    {
	str = XmStringCreateLocalized("A");
	XmStringExtent(fontList, str, &w, &h);
	XmStringFree(str);
	XmFontListFree(fontList);
    }

    return (int)h;
}

    void
workshop_footer_message(
	char	*message,
	int	severity UNUSED)	/* severity is currently unused */
{
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_footer_message(%s, %d)\n", message, severity);
#endif

    gui_mch_set_footer((char_u *) message);
}

/*
 * workshop_menu_begin() is passed the menu name. We determine its mnemonic
 * here and store its name and priority.
 */
    void
workshop_menu_begin(
	char		*label)
{
    vimmenu_T	*menu;			/* pointer to last menu */
    int		menuPriority = 0;	/* priority of new menu */
    char	mnembuf[64];		/* store menubar mnemonics here */
    char	*name;			/* label with a mnemonic */
    char	*p;			/* used to find mnemonics */
    int		idx;			/* index into mnembuf */

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_menu_begin()\n");
#endif

    /*
     * Look through all existing (non-PopUp and non-Toolbar) menus
     * and gather their mnemonics. Use this list to decide what
     * mnemonic should be used for label.
     */

    idx = 0;
    mnembuf[idx++] = 'H';		/* H is mnemonic for Help */
    for (menu = root_menu; menu != NULL; menu = menu->next)
    {
	if (menu_is_menubar(menu->name))
	{
	    p = strchr((char *)menu->name, '&');
	    if (p != NULL)
		mnembuf[idx++] = *++p;
	}
	if (menu->next != NULL
		&& strcmp((char *) menu->next->dname, "Help") == 0)
	{
	    menuPriority = menu->priority + 10;
	    break;
	}
    }
    mnembuf[idx++] = NUL;
    name = addUniqueMnemonic(mnembuf, label);

    vim_snprintf(curMenuName, sizeof(curMenuName), "%s", name);
    sprintf(curMenuPriority, "%d.0", menuPriority);
}

/*
 * Append the name and priority to strings to be used in vim menu commands.
 */
    void
workshop_submenu_begin(
	char		*label)
{
#ifdef WSDEBUG_TRACE
    if (ws_debug  && ws_dlevel & WS_TRACE
	    && strncmp(curMenuName, "ToolBar", 7) != 0)
	wstrace("workshop_submenu_begin(%s)\n", label);
#endif

    strcat(curMenuName, ".");
    strcat(curMenuName, fixup(label));

    updatePriority(True);
}

/*
 * Remove the submenu name and priority from curMenu*.
 */

    void
workshop_submenu_end()
{
    char		*p;

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE)
	    && strncmp(curMenuName, "ToolBar", 7) != 0)
	wstrace("workshop_submenu_end()\n");
#endif

    p = strrchr(curMenuPriority, '.');
    ASSERT(p != NULL);
    *p = NUL;

    p = strrchr(curMenuName, '.');
    ASSERT(p != NULL);
    *p = NUL;
}

/*
 * This is where menus are really made. Each item will generate an amenu vim
 * command. The globals curMenuName and curMenuPriority contain the name and
 * priority of the parent menu tree.
 */
    void
workshop_menu_item(
	char		*label,
	char		*verb,
	char		*accelerator UNUSED,
	char		*acceleratorText,
	char		*name UNUSED,
	char		*filepos UNUSED,
	char		*sensitive)
{
    char		 cbuf[BUFSIZ];
    char		 namebuf[BUFSIZ];
    char		 accText[BUFSIZ];

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE)
	    && strncmp(curMenuName, "ToolBar", 7) != 0)
    {
	if (ws_dlevel & WS_TRACE_VERBOSE)
	    wsdebug("workshop_menu_item(\n"
		    "\tlabel = \"%s\",\n"
		    "\tverb = %s,\n"
		    "\taccelerator = %s,\n"
		    "\tacceleratorText = \"%s\",\n"
		    "\tname = %s,\n"
		    "\tfilepos = %s,\n"
		    "\tsensitive = %s)\n",
		    label && *label ? label : "<None>",
		    verb && *verb ? verb : "<None>",
		    accelerator && *accelerator ?
		    accelerator : "<None>",
		    acceleratorText && *acceleratorText ?
		    acceleratorText : "<None>",
		    name && *name ? name : "<None>",
		    filepos && *filepos ? filepos : "<None>",
		    sensitive);
	else if (ws_dlevel & WS_TRACE)
	    wstrace("workshop_menu_item(\"%s\", %s)\n",
		    label && *label ? label : "<None>",
		    verb && *verb ? verb : "<None>", sensitive);
    }
#endif
#ifdef WSDEBUG_SENSE
    if (ws_debug)
	wstrace("menu:   %-21.20s%-21.20s(%s)\n", label, verb,
		*sensitive == '1' ? "Sensitive" : "Insensitive");
#endif

    if (acceleratorText != NULL)
	vim_snprintf(accText, sizeof(accText), "<Tab>%s", acceleratorText);
    else
	accText[0] = NUL;
    updatePriority(False);
    vim_snprintf(namebuf, sizeof(namebuf), "%s.%s", curMenuName, fixup(label));
    vim_snprintf(cbuf, sizeof(cbuf), "amenu %s %s%s\t:wsverb %s<CR>",
	    curMenuPriority, namebuf, accText, verb);

    coloncmd(cbuf, TRUE);
    addMenu(namebuf, fixAccelText(acceleratorText), verb);

    if (*sensitive == '0')
    {
	vim_snprintf(cbuf, sizeof(cbuf), "amenu disable %s", namebuf);
	coloncmd(cbuf, TRUE);
    }
}

/*
 * This function is called when a complete WorkShop menu description has been
 * sent over from eserve. We do some menu cleanup.
 */

    void
workshop_menu_end()
{
    Boolean		 using_tearoff;	/* set per current option setting */

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_menu_end()\n");
#endif

    using_tearoff = vim_strchr(p_go, GO_TEAROFF) != NULL;
    gui_mch_toggle_tearoffs(using_tearoff);
}

    void
workshop_toolbar_begin()
{
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_toolbar_begin()\n");
#endif

    coloncmd("aunmenu ToolBar", True);
    tbpri = 10;
}

    void
workshop_toolbar_end()
{
    char_u	buf[64];

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
    {
	wstrace("workshop_toolbar_end()\n");
    }
#endif

    /*
     * Turn on ToolBar.
     */
    STRCPY(buf, p_go);
    if (vim_strchr(p_go, 'T') == NULL)
    {
	STRCAT(buf, "T");
	set_option_value((char_u *)"go", 0L, buf, 0);
    }
    workshopInitDone = True;
}

    void
workshop_toolbar_button(
	char	*label,
	char	*verb,
	char	*senseVerb UNUSED,
	char	*filepos UNUSED,
	char	*help,
	char	*sense,
	char	*file,
	char	*left)
{
    char	cbuf[BUFSIZ + MAXPATHLEN];
    char	namebuf[BUFSIZ];
    static int	tbid = 1;
    char_u	*p;

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE))
	wsdebug("workshop_toolbar_button(\"%s\", %s, %s,\n"
		"\t%s, \"%s\", %s,\n\t\"%s\",\n\t<%s>)\n",
		label   && *label   ? label   : "<None>",
		verb    && *verb    ? verb    : "<None>",
		senseVerb && *senseVerb    ? senseVerb    : "<None>",
		filepos && *filepos ? filepos : "<None>",
		help    && *help    ? help    : "<None>",
		sense   && *sense   ? sense   : "<None>",
		file    && *file    ? file    : "<None>",
		left    && *left    ? left    : "<None>");
    else if (WSDLEVEL(WS_TRACE))
	wstrace("workshop_toolbar_button(\"%s\", %s)\n",
		label   && *label   ? label   : "<None>",
		verb    && *verb    ? verb    : "<None>");
#endif
#ifdef WSDEBUG_SENSE
    if (ws_debug)
	wsdebug("button: %-21.20s%-21.20s(%s)\n", label, verb,
		*sense == '1' ? "Sensitive" : "Insensitive");
#endif

    if (left && *left && atoi(left) > 0)
    {
	/* Add a separator (but pass the width passed after the ':') */
	sprintf(cbuf, "amenu 1.%d ToolBar.-sep%d:%s- <nul>",
		tbpri - 5, tbid++, left);

	coloncmd(cbuf, True);
    }

    p = vim_strsave_escaped((char_u *)label, (char_u *)"\\. ");
    vim_snprintf(namebuf, sizeof(namebuf), "ToolBar.%s", p);
    vim_free(p);
    STRCPY(cbuf, "amenu <silent> ");
    if (file != NULL && *file != NUL)
    {
	p = vim_strsave_escaped((char_u *)file, (char_u *)" ");
	vim_snprintf_add(cbuf, sizeof(cbuf), "icon=%s ", p);
	vim_free(p);
    }
    vim_snprintf_add(cbuf, sizeof(cbuf),"1.%d %s :wsverb %s<CR>",
							tbpri, namebuf, verb);

    /* Define the menu item */
    coloncmd(cbuf, True);

    if (*sense == '0')
    {
	/* If menu isn't sensitive at startup... */
	vim_snprintf(cbuf, sizeof(cbuf), "amenu disable %s", namebuf);
	coloncmd(cbuf, True);
    }

    if (help && *help)
    {
	/* Do the tooltip */
	vim_snprintf(cbuf, sizeof(cbuf), "tmenu %s %s", namebuf, help);
	coloncmd(cbuf, True);
    }

    addMenu(namebuf, NULL, verb);
    tbpri += 10;
}

    void
workshop_frame_sensitivities(
	VerbSense	*vs)		/* list of verbs to (de)sensitize */
{
    VerbSense	*vp;		/* iterate through vs */
    char	*menu_name;	/* used in menu lookup */
    int		 cnt;		/* count of verbs to skip */
    int		 len;		/* length of nonvariant part of command */
    char	 cbuf[4096];

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE) || WSDLEVEL(4))
    {
	wsdebug("workshop_frame_sensitivities(\n");
	for (vp = vs; vp->verb != NULL; vp++)
	    wsdebug("\t%-25s%d\n", vp->verb, vp->sense);
	wsdebug(")\n");
    }
    else if (WSDLEVEL(WS_TRACE))
	wstrace("workshop_frame_sensitivities()\n");
#endif
#ifdef WSDEBUG_SENSE
    if (ws_debug)
	for (vp = vs; vp->verb != NULL; vp++)
	    wsdebug("change: %-21.20s%-21.20s(%s)\n",
		    "", vp->verb, vp->sense == 1 ?
		    "Sensitive" : "Insensitive");
#endif

    /*
     * Look for all matching menu entries for the verb. There may be more
     * than one if the verb has both a menu and toolbar entry.
     */
    for (vp = vs; vp->verb != NULL; vp++)
    {
	cnt = 0;
	strcpy(cbuf, "amenu");
	strcat(cbuf, " ");
	strcat(cbuf, vp->sense ? "enable" : "disable");
	strcat(cbuf, " ");
	len = strlen(cbuf);
	while ((menu_name = lookupVerb(vp->verb, cnt++)) != NULL)
	{
	    strcpy(&cbuf[len], menu_name);
	    coloncmd(cbuf, FALSE);
	}
    }
    gui_update_menus(0);
    gui_mch_flush();
}

    void
workshop_set_option(
	char	*option,		/* name of a supported option */
	char	*value)			/* value to set option to */
{
    char	 cbuf[BUFSIZ];		/* command buffer */

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
    {
	wstrace("workshop_set_option(%s, %s)\n", option, value);
    }
#endif

    cbuf[0] = NUL;
    switch (*option)		/* switch on 1st letter */
    {
	case 's':
	    if (strcmp(option, "syntax") == 0)
		vim_snprintf(cbuf, sizeof(cbuf), "syntax %s", value);
	    else if (strcmp(option, "savefiles") == 0)
	    {
		/* XXX - Not yet implemented */
	    }
	    break;

	case 'l':
	    if (strcmp(option, "lineno") == 0)
		sprintf(cbuf, "set %snu",
			(strcmp(value, "on") == 0) ? "" : "no");
	    break;

	case 'p':
	    if (strcmp(option, "parentheses") == 0)
		sprintf(cbuf, "set %ssm",
			(strcmp(value, "on") == 0) ? "" : "no");
	    break;

	case 'w':
	    /* this option is set by a direct call */
#ifdef WSDEBUG
	    wsdebug("workshop_set_option: "
		    "Got unexpected workshopkeys option");
#endif
	    break;

	case 'b':	/* these options are set from direct calls */
	    if (option[7] == NUL && strcmp(option, "balloon") == 0)
	    {
#ifdef WSDEBUG
		/* set by direct call to workshop_balloon_mode */
		wsdebug("workshop_set_option: "
			"Got unexpected ballooneval option");
#endif
	    }
	    else if (strcmp(option, "balloondelay") == 0)
	    {
#ifdef WSDEBUG
		/* set by direct call to workshop_balloon_delay */
		wsdebug("workshop_set_option: "
			"Got unexpected balloondelay option");
#endif
	    }
	    break;
    }
    if (cbuf[0] != NUL)
	coloncmd(cbuf, TRUE);
}


    void
workshop_balloon_mode(
	Boolean	 on)
{
    char	 cbuf[BUFSIZ];		/* command buffer */

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_balloon_mode(%s)\n", on ? "True" : "False");
#endif

    sprintf(cbuf, "set %sbeval", on ? "" : "no");
    coloncmd(cbuf, TRUE);
}


    void
workshop_balloon_delay(
	int	 delay)
{
    char	 cbuf[BUFSIZ];		/* command buffer */

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_balloon_delay(%d)\n", delay);
#endif

    sprintf(cbuf, "set bdlay=%d", delay);
    coloncmd(cbuf, TRUE);
}


    void
workshop_show_balloon_tip(
	char	*tip)
{
#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_show_balloon_tip(%s)\n", tip);
#endif

    if (balloonEval != NULL)
	gui_mch_post_balloon(balloonEval, (char_u *)tip);
}


    void
workshop_hotkeys(
	Boolean	on)
{
    char	 cbuf[BUFSIZ];		/* command buffer */
    MenuMap	*mp;			/* iterate over menuMap entries */

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_hotkeys(%s)\n", on ? "True" : "False");
#endif

    workshopHotKeysEnabled = on;
    if (workshopHotKeysEnabled)
	for (mp = menuMap; mp < &menuMap[menuMapSize]; mp++)
	{
	    if (mp->accel != NULL)
	    {
		vim_snprintf(cbuf, sizeof(cbuf),
			"map %s :wsverb %s<CR>", mp->accel, mp->verb);
		coloncmd(cbuf, TRUE);
	    }
	}
    else
	for (mp = menuMap; mp < &menuMap[menuMapSize]; mp++)
	{
	    if (mp->accel != NULL)
	    {
		vim_snprintf(cbuf, sizeof(cbuf), "unmap %s", mp->accel);
		coloncmd(cbuf, TRUE);
	    }
	}
}

/*
 * A button in the toolbar has been pushed.
 */
    int
workshop_get_positions(
	void		*clientData UNUSED,
	char	       **filename,	/* output data */
	int		*curLine,	/* output data */
	int		*curCol,	/* output data */
	int		*selStartLine,	/* output data */
	int		*selStartCol,	/* output data */
	int		*selEndLine,	/* output data */
	int		*selEndCol,	/* output data */
	int		*selLength,	/* output data */
	char	       **selection)	/* output data */
{
    static char	 ffname[MAXPATHLEN];

#ifdef WSDEBUG_TRACE
    if (WSDLEVEL(WS_TRACE_VERBOSE | WS_TRACE))
	wstrace("workshop_get_positions(%#x, \"%s\", ...)\n",
		clientData, (curbuf && curbuf->b_sfname != NULL)
				      ? (char *)curbuf->b_sfname : "<None>");
#endif

    if (curbuf->b_ffname == NULL)
	ffname[0] = NUL;
    else
	/* copy so nobody can change b_ffname */
	strcpy(ffname, (char *) curbuf->b_ffname);
    *filename = ffname;
    *curLine = curwin->w_cursor.lnum;
    *curCol = curwin->w_cursor.col;

    if (curbuf->b_visual.vi_mode == 'v' &&
	    equalpos(curwin->w_cursor, curbuf->b_visual.vi_end))
    {
	*selStartLine = curbuf->b_visual.vi_start.lnum;
	*selStartCol = curbuf->b_visual.vi_start.col;
	*selEndLine = curbuf->b_visual.vi_end.lnum;
	*selEndCol = curbuf->b_visual.vi_end.col;
	*selection = get_selection(curbuf);
	if (*selection)
	    *selLength = strlen(*selection);
	else
	    *selLength = 0;
    }
    else
    {
	*selStartLine = *selEndLine = -1;
	*selStartCol = *selEndCol = -1;
	*selLength = 0;
	*selection = "";
    }

    return True;
}



/************************************************************************
 * Utility functions
 ************************************************************************/

    static char *
get_selection(
	buf_T		*buf)		/* buffer whose selection we want */
{
    pos_T	*start;		/* start of the selection */
    pos_T	*end;		/* end of the selection */
    char	*lp;		/* pointer to actual line data */
    int		 llen;		/* length of actual line data */
    char	*sp;		/* pointer to selection  buffer */
    int		 slen;		/* string length in selection buffer */
    int		 size;		/* size of selection buffer */
    char	*new_sp;	/* temp pointer to new sp */
    int		 lnum;		/* line number we are appending */

    if (buf->b_visual.vi_mode == 'v')
    {
	start = &buf->b_visual.vi_start;
	end = &buf->b_visual.vi_end;
	if (start->lnum == end->lnum)
	{
	    /* selection is all on one line */
	    lp = (char *) ml_get_pos(start);
	    llen = end->col - start->col + 1;
	    sp = (char *) malloc(llen + 1);
	    if (sp != NULL)
	    {
		strncpy(sp, lp, llen);
		sp[llen] = NUL;
	    }
	}
	else
	{
	    /* multi-line selection */
	    lp = (char *) ml_get_pos(start);
	    llen = strlen(lp);
	    sp = (char *) malloc(BUFSIZ + llen);
	    if (sp != NULL)
	    {
		size = BUFSIZ + llen;
		strcpy(sp, lp);
		sp[llen] = '\n';
		slen = llen + 1;

		lnum = start->lnum + 1;
		while (lnum < end->lnum)
		    sp = append_selection(lnum++, sp, &size, &slen);

		lp = (char *) ml_get(end->lnum);
		llen = end->col + 1;
		if ((slen + llen) >= size)
		{
		    new_sp = (char *)
			realloc(sp, slen + llen + 1);
		    if (new_sp != NULL)
		    {
			size += llen + 1;
			sp = new_sp;
		    }
		}
		if ((slen + llen) < size)
		{
		    strncpy(&sp[slen], lp, llen);
		    sp[slen + llen] = NUL;
		}

	    }
	}
    }
    else
	sp = NULL;

    return sp;
}

    static char *
append_selection(
	int		 lnum,		/* line number to append */
	char		*sp,		/* pointer to selection buffer */
	int		*size,		/* ptr to size of sp */
	int		*slen)		/* ptr to length of selection string */
{
    char	*lp;		/* line of data from buffer */
    int		 llen;		/* strlen of lp */
    char	*new_sp;	/* temp pointer to new sp */

    lp = (char *)ml_get((linenr_T)lnum);
    llen = strlen(lp);

    if ((*slen + llen) <= *size)
    {
	new_sp = (char *) realloc((void *) sp, BUFSIZ + *slen + llen);
	if (*new_sp != NUL)
	{
	    *size = BUFSIZ + *slen + llen;
	    sp = new_sp;
	}
    }
    if ((*slen + llen) > *size)
    {
	strcat(&sp[*slen], lp);
	*slen += llen;
	sp[*slen++] = '\n';
    }

    return sp;
}



    static void
load_buffer_by_name(
	char	*filename,		/* the file to load */
	int	 lnum)			/* an optional line number (or 0) */
{
    char	 lnumbuf[16];		/* make line number option for :e */
    char	 cbuf[BUFSIZ];		/* command buffer */

    if (lnum > 0)
	sprintf(lnumbuf, "+%d", lnum);
    else
	lnumbuf[0] = NUL;

    vim_snprintf(cbuf, sizeof(cbuf), "e %s %s", lnumbuf, filename);
    coloncmd(cbuf, False);
}


    static void
load_window(
	char	*filename,		/* filename to load */
	int	 lnum)			/* linenumber to go to */
{
    buf_T	*buf;		/* buffer filename is stored in */
    win_T	*win;		/* window filenme is displayed in */

    /*
     * Make sure filename is displayed and is the current window.
     */

    buf = buflist_findname((char_u *)filename);
    if (buf == NULL || (win = get_window(buf)) == NULL)
    {
	/* No buffer or buffer is not in current window */
	/* wsdebug("load_window: load_buffer_by_name(\"%s\", %d)\n",
		filename, lnum); */
	load_buffer_by_name(filename, lnum);
    }
    else
    {
#ifdef FEAT_WINDOWS
	/* buf is in a window */
	if (win != curwin)
	{
	    win_enter(win, False);
	    /* wsdebug("load_window: window enter %s\n",
		    win->w_buffer->b_sfname); */
	}
#endif
	if (lnum > 0 && win->w_cursor.lnum != lnum)
	{
	    warp_to_pc(lnum);
	    /* wsdebug("load_window: warp to %s[%d]\n",
		    win->w_buffer->b_sfname, lnum); */
	}
    }
    out_flush();
}



    static void
warp_to_pc(
	int	 lnum)			/* line number to warp to */
{
    char	 lbuf[256];		/* build line command here */

    if (lnum > 0)
    {
	if (State & INSERT)
	    add_to_input_buf((char_u *) "\033", 1);
	if (isShowing(lnum))
	    sprintf(lbuf, "%dG", lnum);
	else
	    sprintf(lbuf, "%dz.", lnum);
	add_to_input_buf((char_u *) lbuf, strlen(lbuf));
    }
}

    static Boolean
isShowing(
	int	 lnum)			/* tell if line number is showing */
{
    return lnum >= curwin->w_topline && lnum < curwin->w_botline;
}



    static win_T *
get_window(
	buf_T	*buf)		/* buffer to find window for */
{
    win_T	*wp = NULL;	/* window filename is in */

    for (wp = firstwin; wp != NULL; wp = W_NEXT(wp))
	if (buf == wp->w_buffer)
	    break;
    return wp;
}


    static void
updatePriority(
	Boolean		 subMenu)	/* if True then start new submenu pri */
{
    int		 pri;		/* priority of this menu/item */
    char	*p;

    p = strrchr(curMenuPriority, '.');
    ASSERT(p != NULL);
    *p++ = NUL;

    pri = atoi(p) + 10;		/* our new priority */

    if (subMenu)
	vim_snprintf(curMenuPriority, sizeof(curMenuPriority),
					     "%s.%d.0", curMenuPriority, pri);
    else
	vim_snprintf(curMenuPriority, sizeof(curMenuPriority),
					       "%s.%d", curMenuPriority, pri);
}

    static char *
addUniqueMnemonic(
	char		*mnemonics,	/* currently used mnemonics */
	char		*label)		/* label of menu needing mnemonic */
{
    static char	 name[BUFSIZ];	/* buffer for the updated name */
    char	*p;		/* pointer into label */
    char	*found;		/* pointer to possible mnemonic */

    found = NULL;
    for (p = label; *p != NUL; p++)
	if (strchr(mnemonics, *p) == 0)
	    if (found == NULL || (isupper((int)*p) && islower((int)*found)))
		found = p;

    if (found != NULL)
    {
	strncpy(name, label, (found - label));
	strcat(name, "&");
	strcat(name, found);
    }
    else
	strcpy(name, label);

    return name;
}

/*
 * Some characters in a menu name must be escaped in vim. Since this is vim
 * specific, it must be done on this side.
 */
    static char *
fixup(
	char		*label)
{
    static char	 buf[BUFSIZ];
    char		*bp;		/* pointer into buf */
    char		*lp;		/* pointer into label */

    lp = label;
    bp = buf;
    while (*lp != NUL)
    {
	if (*lp == ' ' || *lp == '.')
	    *bp++ = '\\';
	*bp++ = *lp++;
    }
    *bp = NUL;

    return buf;
}


#ifdef NOHANDS_SUPPORT_FUNCTIONS

/* For the NoHands test suite */

    char *
workshop_test_getcurrentfile()
{
    char	*filename, *selection;
    int		curLine, curCol, selStartLine, selStartCol, selEndLine;
    int		selEndCol, selLength;

    if (workshop_get_positions(
		NULL, &filename, &curLine, &curCol, &selStartLine,
		&selStartCol, &selEndLine, &selEndCol, &selLength,
		&selection))
	return filename;
    else
	return NULL;
}

    int
workshop_test_getcursorrow()
{
    return 0;
}

    int
workshop_test_getcursorcol()
{
    char	*filename, *selection;
    int		curLine, curCol, selStartLine, selStartCol, selEndLine;
    int		selEndCol, selLength;

    if (workshop_get_positions(
		NULL, &filename, &curLine, &curCol, &selStartLine,
		&selStartCol, &selEndLine, &selEndCol, &selLength,
		&selection))
	return curCol;
    else
	return -1;
}

    char *
workshop_test_getcursorrowtext()
{
    return NULL;
}

    char *
workshop_test_getselectedtext()
{
    char	*filename, *selection;
    int		curLine, curCol, selStartLine, selStartCol, selEndLine;
    int		selEndCol, selLength;

    if (workshop_get_positions(
		NULL, &filename, &curLine, &curCol, &selStartLine,
		&selStartCol, &selEndLine, &selEndCol, &selLength,
		&selection))
	return selection;
    else
	return NULL;
}

    void
workshop_save_sensitivity(char *filename UNUSED)
{
}

#endif

    static char *
fixAccelText(
	char		*ap)		/* original acceleratorText */
{
    char	buf[256];	/* build in temp buffer */
    char	*shift;		/* shift string of "" */

    if (ap == NULL)
	return NULL;

    /* If the accelerator is shifted use the vim form */
    if (strncmp("Shift+", ap, 6) == 0)
    {
	shift = "S-";
	ap += 6;
    }
    else
	shift = "";

    if (*ap == 'F' && atoi(&ap[1]) > 0)
    {
	vim_snprintf(buf, sizeof(buf), "<%s%s>", shift, ap);
	return strdup(buf);
    }
    else
	return NULL;
}

#ifdef FEAT_BEVAL
    void
workshop_beval_cb(
	BalloonEval	*beval,
	int		 state)
{
    win_T	*wp;
    char_u	*text;
    int		 type;
    linenr_T	 lnum;
    int		 col;
    int		 idx;
    char	 buf[MAXPATHLEN * 2];
    static int	 serialNo = -1;

    if (!p_beval)
	return;

    if (get_beval_info(beval, FALSE, &wp, &lnum, &text, &col) == OK)
    {
	if (text && text[0])
	{
	    /* Send debugger request */
	    if (strlen((char *) text) > (MAXPATHLEN/2))
	    {
		/*
		 * The user has probably selected the entire
		 * buffer or something like that - don't attempt
		 * to evaluate it
		 */
		return;
	    }

	    /*
	     * WorkShop expects the col to be a character index, not
	     * a column number. Compute the index from col. Also set
	     * line to 0 because thats what dbx expects.
	     */
	    idx = computeIndex(col, text, beval->ts);
	    if (idx > 0)
	    {
		lnum = 0;

		/*
		 * If successful, it will respond with a balloon cmd.
		 */
		if (state & ControlMask)
		    /* Evaluate *(expression) */
		    type = (int)GPLineEval_INDIRECT;
		else if (state & ShiftMask)
		    /* Evaluate type(expression) */
		    type = (int)GPLineEval_TYPE;
		else
		    /* Evaluate value(expression) */
		    type = (int)GPLineEval_EVALUATE;

		/* Send request to dbx */
		vim_snprintf(buf, sizeof(buf), "toolVerb debug.balloonEval "
			"%s %ld,0 %d,0 %d,%d %ld %s\n",
			(char *)wp->w_buffer->b_ffname,
			(long)lnum, idx, type, serialNo++,
			(long)strlen((char *)text), (char *)text);
		balloonEval = beval;
		workshop_send_message(buf);
	    }
	}
    }
}

    static int
computeIndex(
	int		 wantedCol,
	char_u		*line,
	int		 ts)
{
    int		 col = 0;
    int		 idx = 0;

    while (line[idx])
    {
	if (line[idx] == '\t')
	    col += ts - (col % ts);
	else
	    col++;
	idx++;
	if (col >= wantedCol)
	    return idx;
    }

    return -1;
}
#endif

    static void
addMenu(
	char		*menu,		/* menu name */
	char		*accel,		/* accelerator text (optional) */
	char		*verb)		/* WorkShop action-verb */
{
    MenuMap		*newMap;
    char		 cbuf[BUFSIZ];

    if (menuMapSize >= menuMapMax)
    {
	newMap = realloc(menuMap,
		sizeof(MenuMap) * (menuMapMax + MENU_INC));
	if (newMap != NULL)
	{
	    menuMap = newMap;
	    menuMapMax += MENU_INC;
	}
    }
    if (menuMapSize < menuMapMax)
    {
	menuMap[menuMapSize].name = strdup(menu);
	menuMap[menuMapSize].accel = accel && *accel ? strdup(accel) : NULL;
	menuMap[menuMapSize++].verb = strdup(verb);
	if (accel && workshopHotKeysEnabled)
	{
	    vim_snprintf(cbuf, sizeof(cbuf),
					"map %s :wsverb %s<CR>", accel, verb);
	    coloncmd(cbuf, TRUE);
	}
    }
}

    static char *
nameStrip(
	char		*raw)		/* menu name, possibly with & chars */
{
    static char		buf[BUFSIZ];	/* build stripped name here */
    char		*bp = buf;

    while (*raw)
    {
	if (*raw != '&')
	    *bp++ = *raw;
	raw++;
    }
    *bp = NUL;
    return buf;
}


    static char *
lookupVerb(
	char	*verb,
	int	skip)		/* number of matches to skip */
{
    int		i;		/* loop iterator */

    for (i = 0; i < menuMapSize; i++)
	if (strcmp(menuMap[i].verb, verb) == 0 && skip-- == 0)
	    return nameStrip(menuMap[i].name);

    return NULL;
}


    static void
coloncmd(
	char	*cmd,		/* the command to print */
	Boolean	force)		/* force cursor update */
{
    char_u	*cpo_save = p_cpo;

#ifdef WSDEBUG
    if (WSDLEVEL(WS_TRACE_COLONCMD))
	wsdebug("Cmd: %s\n", cmd);
#endif

    p_cpo = empty_option;

    ALT_INPUT_LOCK_ON;
    do_cmdline_cmd((char_u *)cmd);
    ALT_INPUT_LOCK_OFF;

    p_cpo = cpo_save;

    if (force)
	gui_update_screen();
}

/*
 * setDollarVim -	Given the run directory, search for the vim install
 *			directory and set $VIM.
 *
 *			We can be running out of SUNWspro/bin or out of
 *			SUNWspro/contrib/contrib6/vim5.6/bin so we check
 *			relative to both of these directories.
 */
    static void
setDollarVim(
	char	*rundir)
{
    char	 buf[MAXPATHLEN];
    char	*cp;

    /*
     * First case: Running from <install-dir>/SUNWspro/bin
     */
    strcpy(buf, rundir);
    strcat(buf, "/../contrib/contrib6/vim" VIM_VERSION_SHORT "/share/vim/"
	    VIM_VERSION_NODOT "/syntax/syntax.vim");
    if (access(buf, R_OK) == 0)
    {
	strcpy(buf, "SPRO_WSDIR=");
	strcat(buf, rundir);
	cp = strrchr(buf, '/');
	if (cp != NULL)
	    strcpy(cp, "/WS6U2");
	putenv(strdup(buf));

	strcpy(buf, "VIM=");
	strcat(buf, rundir);
	strcat(buf, "/../contrib/contrib6/vim" VIM_VERSION_SHORT "/share/vim/"
		VIM_VERSION_NODOT);
	putenv(strdup(buf));
	return;
    }

    /*
     * Second case: Probably running from
     *		<install-dir>/SUNWspro/contrib/contrib6/vim5.6/bin
     */
    strcpy(buf, rundir);
    strcat(buf, "/../../../contrib/contrib6/vim" VIM_VERSION_SHORT
	    "/share/vim/" VIM_VERSION_NODOT "/syntax/syntax.vim");
    if (access(buf, R_OK) == 0)
    {
	strcpy(buf, "SPRO_WSDIR=");
	strcat(buf, rundir);
	cp = strrchr(buf, '/');
	if (cp != NULL)
	    strcpy(cp, "../../../../WS6U2");
	putenv(strdup(buf));

	strcpy(buf, "VIM=");
	strcat(buf, rundir);
	strcat(buf, "/../../../contrib/contrib6/vim" VIM_VERSION_SHORT
		"/share/vim/" VIM_VERSION_NODOT);
	putenv(strdup(buf));
	return;
    }
}

/*
 * findYourself -	Find the directory we are running from. This is used to
 *			set $VIM. We need to set this because users can install
 *			the package in a different directory than the compiled
 *			directory. This is a Sun Visual WorkShop requirement!
 *
 * Note:		We override a user's $VIM because it won't have the
 *			WorkShop specific files. S/he may not like this but its
 *			better than getting the wrong files (especially as the
 *			user is likely to have $VIM set to 5.4 or later).
 */
    void
findYourself(
    char	*argv0)
{
    char	*runpath = NULL;
    char	*path;
    char	*pathbuf;

    if (*argv0 == '/')
	runpath = strdup(argv0);
    else if (*argv0 == '.' || strchr(argv0, '/'))
    {
	runpath = (char *) malloc(MAXPATHLEN);
	if (getcwd(runpath, MAXPATHLEN) == NULL)
	    runpath[0] = NUL;
	strcat(runpath, "/");
	strcat(runpath, argv0);
    }
    else
    {
	path = getenv("PATH");
	if (path != NULL)
	{
	    runpath = (char *) malloc(MAXPATHLEN);
	    pathbuf = strdup(path);
	    path = strtok(pathbuf, ":");
	    do
	    {
		strcpy(runpath, path);
		strcat(runpath, "/");
		strcat(runpath, argv0);
		if (access(runpath, X_OK) == 0)
		    break;
	    } while ((path = strtok(NULL, ":")) != NULL);
	    free(pathbuf);
	}
    }

    if (runpath != NULL)
    {
	char runbuf[MAXPATHLEN];

	/*
	 * We found the run directory. Now find the install dir.
	 */
	(void)vim_FullName((char_u *)runpath, (char_u *)runbuf, MAXPATHLEN, 1);
	path = strrchr(runbuf, '/');
	if (path != NULL)
	    *path = NUL;		/* remove the vim/gvim name */
	path = strrchr(runbuf, '/');
	if (path != NULL)
	{
	    if (strncmp(path, "/bin", 4) == 0)
		setDollarVim(runbuf);
	    else if (strncmp(path, "/src", 4) == 0)
	    {
		*path = NUL;	/* development tree */
		setDollarVim(runbuf);
	    }
	}
	free(runpath);
    }
}
