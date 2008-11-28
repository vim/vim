/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved		by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * Porting to GTK+ was done by:
 *
 * (C) 1998,1999,2000 by Marcin Dalecki <martin@dalecki.de>
 *
 * With GREAT support and continuous encouragements by Andy Kahn and of
 * course Bram Moolenaar!
 *
 * Support for GTK+ 2 was added by:
 *
 * (C) 2002,2003  Jason Hildebrand  <jason@peaceworks.ca>
 *		  Daniel Elstner  <daniel.elstner@gmx.net>
 */

#include "vim.h"

#ifdef FEAT_GUI_GNOME
/* Gnome redefines _() and N_().  Grrr... */
# ifdef _
#  undef _
# endif
# ifdef N_
#  undef N_
# endif
# ifdef textdomain
#  undef textdomain
# endif
# ifdef bindtextdomain
#  undef bindtextdomain
# endif
# ifdef bind_textdomain_codeset
#  undef bind_textdomain_codeset
# endif
# if defined(FEAT_GETTEXT) && !defined(ENABLE_NLS)
#  define ENABLE_NLS	/* so the texts in the dialog boxes are translated */
# endif
# include <gnome.h>
# include "version.h"
#  ifdef HAVE_GTK2
/* missing prototype in bonobo-dock-item.h */
extern void bonobo_dock_item_set_behavior(BonoboDockItem *dock_item, BonoboDockItemBehavior beh);
#  endif
#endif

#if !defined(FEAT_GUI_GTK) && defined(PROTO)
/* When generating prototypes we don't want syntax errors. */
# define GdkAtom int
# define GdkEventExpose int
# define GdkEventFocus int
# define GdkEventVisibility int
# define GdkEventProperty int
# define GtkContainer int
# define GtkTargetEntry int
# define GtkType int
# define GtkWidget int
# define gint int
# define gpointer int
# define guint int
# define GdkEventKey int
# define GdkEventSelection int
# define GtkSelectionData int
# define GdkEventMotion int
# define GdkEventButton int
# define GdkDragContext int
# define GdkEventConfigure int
# define GdkEventClient int
#else
# include <gdk/gdkkeysyms.h>
# include <gdk/gdk.h>
# ifdef WIN3264
#  include <gdk/gdkwin32.h>
# else
#  include <gdk/gdkx.h>
# endif

# include <gtk/gtk.h>
# include "gui_gtk_f.h"
#endif

#ifdef HAVE_X11_SUNKEYSYM_H
# include <X11/Sunkeysym.h>
#endif

/*
 * Easy-to-use macro for multihead support.
 */
#ifdef HAVE_GTK_MULTIHEAD
# define GET_X_ATOM(atom)	gdk_x11_atom_to_xatom_for_display( \
				    gtk_widget_get_display(gui.mainwin), atom)
#else
# define GET_X_ATOM(atom)	((Atom)(atom))
#endif

/* Selection type distinguishers */
enum
{
    TARGET_TYPE_NONE,
    TARGET_UTF8_STRING,
    TARGET_STRING,
    TARGET_COMPOUND_TEXT,
    TARGET_TEXT,
    TARGET_TEXT_URI_LIST,
    TARGET_TEXT_PLAIN,
    TARGET_VIM,
    TARGET_VIMENC
};

/*
 * Table of selection targets supported by Vim.
 * Note: Order matters, preferred types should come first.
 */
static const GtkTargetEntry selection_targets[] =
{
    {VIMENC_ATOM_NAME,	0, TARGET_VIMENC},
    {VIM_ATOM_NAME,	0, TARGET_VIM},
#ifdef FEAT_MBYTE
    {"UTF8_STRING",	0, TARGET_UTF8_STRING},
#endif
    {"COMPOUND_TEXT",	0, TARGET_COMPOUND_TEXT},
    {"TEXT",		0, TARGET_TEXT},
    {"STRING",		0, TARGET_STRING}
};
#define N_SELECTION_TARGETS (sizeof(selection_targets) / sizeof(selection_targets[0]))

#ifdef FEAT_DND
/*
 * Table of DnD targets supported by Vim.
 * Note: Order matters, preferred types should come first.
 */
static const GtkTargetEntry dnd_targets[] =
{
    {"text/uri-list",	0, TARGET_TEXT_URI_LIST},
# ifdef FEAT_MBYTE
    {"UTF8_STRING",	0, TARGET_UTF8_STRING},
# endif
    {"STRING",		0, TARGET_STRING},
    {"text/plain",	0, TARGET_TEXT_PLAIN}
};
# define N_DND_TARGETS (sizeof(dnd_targets) / sizeof(dnd_targets[0]))
#endif


#ifdef HAVE_GTK2
/*
 * "Monospace" is a standard font alias that should be present
 * on all proper Pango/fontconfig installations.
 */
# define DEFAULT_FONT	"Monospace 10"

#else /* !HAVE_GTK2 */
/*
 * This is the single only fixed width font in X11, which seems to be present
 * on all servers and available in all the variants we need.
 */
# define DEFAULT_FONT	"-adobe-courier-medium-r-normal-*-14-*-*-*-m-*-*-*"

#endif /* !HAVE_GTK2 */

#if !(defined(FEAT_GUI_GNOME) && defined(FEAT_SESSION))
/*
 * Atoms used to communicate save-yourself from the X11 session manager. There
 * is no need to move them into the GUI struct, since they should be constant.
 */
static GdkAtom wm_protocols_atom = GDK_NONE;
static GdkAtom save_yourself_atom = GDK_NONE;
#endif

/*
 * Atoms used to control/reference X11 selections.
 */
#ifdef FEAT_MBYTE
static GdkAtom utf8_string_atom = GDK_NONE;
#endif
#ifndef HAVE_GTK2
static GdkAtom compound_text_atom = GDK_NONE;
static GdkAtom text_atom = GDK_NONE;
#endif
static GdkAtom vim_atom = GDK_NONE;	/* Vim's own special selection format */
#ifdef FEAT_MBYTE
static GdkAtom vimenc_atom = GDK_NONE;	/* Vim's extended selection format */
#endif

/*
 * Keycodes recognized by vim.
 * NOTE: when changing this, the table in gui_x11.c probably needs the same
 * change!
 */
static struct special_key
{
    guint key_sym;
    char_u code0;
    char_u code1;
}
const special_keys[] =
{
    {GDK_Up,		'k', 'u'},
    {GDK_Down,		'k', 'd'},
    {GDK_Left,		'k', 'l'},
    {GDK_Right,		'k', 'r'},
    {GDK_F1,		'k', '1'},
    {GDK_F2,		'k', '2'},
    {GDK_F3,		'k', '3'},
    {GDK_F4,		'k', '4'},
    {GDK_F5,		'k', '5'},
    {GDK_F6,		'k', '6'},
    {GDK_F7,		'k', '7'},
    {GDK_F8,		'k', '8'},
    {GDK_F9,		'k', '9'},
    {GDK_F10,		'k', ';'},
    {GDK_F11,		'F', '1'},
    {GDK_F12,		'F', '2'},
    {GDK_F13,		'F', '3'},
    {GDK_F14,		'F', '4'},
    {GDK_F15,		'F', '5'},
    {GDK_F16,		'F', '6'},
    {GDK_F17,		'F', '7'},
    {GDK_F18,		'F', '8'},
    {GDK_F19,		'F', '9'},
    {GDK_F20,		'F', 'A'},
    {GDK_F21,		'F', 'B'},
    {GDK_Pause,		'F', 'B'}, /* Pause == F21 according to netbeans.txt */
    {GDK_F22,		'F', 'C'},
    {GDK_F23,		'F', 'D'},
    {GDK_F24,		'F', 'E'},
    {GDK_F25,		'F', 'F'},
    {GDK_F26,		'F', 'G'},
    {GDK_F27,		'F', 'H'},
    {GDK_F28,		'F', 'I'},
    {GDK_F29,		'F', 'J'},
    {GDK_F30,		'F', 'K'},
    {GDK_F31,		'F', 'L'},
    {GDK_F32,		'F', 'M'},
    {GDK_F33,		'F', 'N'},
    {GDK_F34,		'F', 'O'},
    {GDK_F35,		'F', 'P'},
#ifdef SunXK_F36
    {SunXK_F36,		'F', 'Q'},
    {SunXK_F37,		'F', 'R'},
#endif
    {GDK_Help,		'%', '1'},
    {GDK_Undo,		'&', '8'},
    {GDK_BackSpace,	'k', 'b'},
    {GDK_Insert,	'k', 'I'},
    {GDK_Delete,	'k', 'D'},
    {GDK_3270_BackTab,	'k', 'B'},
    {GDK_Clear,		'k', 'C'},
    {GDK_Home,		'k', 'h'},
    {GDK_End,		'@', '7'},
    {GDK_Prior,		'k', 'P'},
    {GDK_Next,		'k', 'N'},
    {GDK_Print,		'%', '9'},
    /* Keypad keys: */
    {GDK_KP_Left,	'k', 'l'},
    {GDK_KP_Right,	'k', 'r'},
    {GDK_KP_Up,		'k', 'u'},
    {GDK_KP_Down,	'k', 'd'},
    {GDK_KP_Insert,	KS_EXTRA, (char_u)KE_KINS},
    {GDK_KP_Delete,	KS_EXTRA, (char_u)KE_KDEL},
    {GDK_KP_Home,	'K', '1'},
    {GDK_KP_End,	'K', '4'},
    {GDK_KP_Prior,	'K', '3'},  /* page up */
    {GDK_KP_Next,	'K', '5'},  /* page down */

    {GDK_KP_Add,	'K', '6'},
    {GDK_KP_Subtract,	'K', '7'},
    {GDK_KP_Divide,	'K', '8'},
    {GDK_KP_Multiply,	'K', '9'},
    {GDK_KP_Enter,	'K', 'A'},
    {GDK_KP_Decimal,	'K', 'B'},

    {GDK_KP_0,		'K', 'C'},
    {GDK_KP_1,		'K', 'D'},
    {GDK_KP_2,		'K', 'E'},
    {GDK_KP_3,		'K', 'F'},
    {GDK_KP_4,		'K', 'G'},
    {GDK_KP_5,		'K', 'H'},
    {GDK_KP_6,		'K', 'I'},
    {GDK_KP_7,		'K', 'J'},
    {GDK_KP_8,		'K', 'K'},
    {GDK_KP_9,		'K', 'L'},

    /* End of list marker: */
    {0, 0, 0}
};

/*
 * Flags for command line options table below.
 */
#define ARG_FONT	1
#define ARG_GEOMETRY	2
#define ARG_REVERSE	3
#define ARG_NOREVERSE	4
#define ARG_BACKGROUND	5
#define ARG_FOREGROUND	6
#define ARG_ICONIC	7
#define ARG_ROLE	8
#define ARG_NETBEANS	9
#define ARG_XRM		10	/* ignored */
#define ARG_MENUFONT	11	/* ignored */
#define ARG_INDEX_MASK	0x00ff
#define ARG_HAS_VALUE	0x0100	/* a value is expected after the argument */
#define ARG_NEEDS_GUI	0x0200	/* need to initialize the GUI for this	  */
#define ARG_FOR_GTK	0x0400	/* argument is handled by GTK+ or GNOME   */
#define ARG_COMPAT_LONG	0x0800	/* accept -foo but substitute with --foo  */
#define ARG_KEEP	0x1000	/* don't remove argument from argv[] */

/*
 * This table holds all the X GUI command line options allowed.  This includes
 * the standard ones so that we can skip them when Vim is started without the
 * GUI (but the GUI might start up later).
 *
 * When changing this, also update doc/gui_x11.txt and the usage message!!!
 */
typedef struct
{
    const char	    *name;
    unsigned int    flags;
}
cmdline_option_T;

static const cmdline_option_T cmdline_options[] =
{
    /* We handle these options ourselves */
    {"-fn",		ARG_FONT|ARG_HAS_VALUE},
    {"-font",		ARG_FONT|ARG_HAS_VALUE},
    {"-geom",		ARG_GEOMETRY|ARG_HAS_VALUE},
    {"-geometry",	ARG_GEOMETRY|ARG_HAS_VALUE},
    {"-rv",		ARG_REVERSE},
    {"-reverse",	ARG_REVERSE},
    {"+rv",		ARG_NOREVERSE},
    {"+reverse",	ARG_NOREVERSE},
    {"-bg",		ARG_BACKGROUND|ARG_HAS_VALUE},
    {"-background",	ARG_BACKGROUND|ARG_HAS_VALUE},
    {"-fg",		ARG_FOREGROUND|ARG_HAS_VALUE},
    {"-foreground",	ARG_FOREGROUND|ARG_HAS_VALUE},
    {"-iconic",		ARG_ICONIC},
#ifdef HAVE_GTK2
    {"--role",		ARG_ROLE|ARG_HAS_VALUE},
#endif
#ifdef FEAT_NETBEANS_INTG
    {"-nb",		ARG_NETBEANS},	      /* non-standard value format */
    {"-xrm",		ARG_XRM|ARG_HAS_VALUE},		/* not implemented */
    {"-mf",		ARG_MENUFONT|ARG_HAS_VALUE},	/* not implemented */
    {"-menufont",	ARG_MENUFONT|ARG_HAS_VALUE},	/* not implemented */
#endif
#if 0 /* not implemented; these arguments don't make sense for GTK+ */
    {"-boldfont",	ARG_HAS_VALUE},
    {"-italicfont",	ARG_HAS_VALUE},
    {"-bw",		ARG_HAS_VALUE},
    {"-borderwidth",	ARG_HAS_VALUE},
    {"-sw",		ARG_HAS_VALUE},
    {"-scrollbarwidth",	ARG_HAS_VALUE},
#endif
    /* Arguments handled by GTK (and GNOME) internally. */
    {"--g-fatal-warnings",	ARG_FOR_GTK},
    {"--gdk-debug",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--gdk-no-debug",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--gtk-debug",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--gtk-no-debug",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--gtk-module",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--sync",			ARG_FOR_GTK},
    {"--display",		ARG_FOR_GTK|ARG_HAS_VALUE|ARG_COMPAT_LONG},
    {"--name",			ARG_FOR_GTK|ARG_HAS_VALUE|ARG_COMPAT_LONG},
    {"--class",			ARG_FOR_GTK|ARG_HAS_VALUE|ARG_COMPAT_LONG},
#ifdef HAVE_GTK2
    {"--screen",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--gxid-host",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--gxid-port",		ARG_FOR_GTK|ARG_HAS_VALUE},
#else /* these don't seem to exist anymore */
    {"--no-xshm",		ARG_FOR_GTK},
    {"--xim-preedit",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--xim-status",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--gxid_host",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--gxid_port",		ARG_FOR_GTK|ARG_HAS_VALUE},
#endif
#ifdef FEAT_GUI_GNOME
    {"--load-modules",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--sm-client-id",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--sm-config-prefix",	ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--sm-disable",		ARG_FOR_GTK},
    {"--oaf-ior-fd",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--oaf-activate-iid",	ARG_FOR_GTK|ARG_HAS_VALUE},
    {"--oaf-private",		ARG_FOR_GTK},
    {"--enable-sound",		ARG_FOR_GTK},
    {"--disable-sound",		ARG_FOR_GTK},
    {"--espeaker",		ARG_FOR_GTK|ARG_HAS_VALUE},
    {"-?",			ARG_FOR_GTK|ARG_NEEDS_GUI},
    {"--help",			ARG_FOR_GTK|ARG_NEEDS_GUI|ARG_KEEP},
    {"--usage",			ARG_FOR_GTK|ARG_NEEDS_GUI},
# if 0 /* conflicts with Vim's own --version argument */
    {"--version",		ARG_FOR_GTK|ARG_NEEDS_GUI},
# endif
    {"--disable-crash-dialog",	ARG_FOR_GTK},
#endif
    {NULL, 0}
};

static int    gui_argc = 0;
static char **gui_argv = NULL;

#ifdef HAVE_GTK2
static const char *role_argument = NULL;
#endif
#if defined(FEAT_GUI_GNOME) && defined(FEAT_SESSION)
static const char *restart_command = NULL;
#endif
static int found_iconic_arg = FALSE;

#ifdef FEAT_GUI_GNOME
/*
 * Can't use Gnome if --socketid given
 */
static int using_gnome = 0;
#else
# define using_gnome 0
#endif

/*
 * Parse the GUI related command-line arguments.  Any arguments used are
 * deleted from argv, and *argc is decremented accordingly.  This is called
 * when vim is started, whether or not the GUI has been started.
 */
    void
gui_mch_prepare(int *argc, char **argv)
{
    const cmdline_option_T  *option;
    int			    i	= 0;
    int			    len = 0;

#if defined(FEAT_GUI_GNOME) && defined(FEAT_SESSION)
    /*
     * Determine the command used to invoke Vim, to be passed as restart
     * command to the session manager.	If argv[0] contains any directory
     * components try building an absolute path, otherwise leave it as is.
     */
    restart_command = argv[0];

    if (strchr(argv[0], G_DIR_SEPARATOR) != NULL)
    {
	char_u buf[MAXPATHL];

	if (mch_FullName((char_u *)argv[0], buf, (int)sizeof(buf), TRUE) == OK)
	    /* Tiny leak; doesn't matter, and usually we don't even get here */
	    restart_command = (char *)vim_strsave(buf);
    }
#endif

    /*
     * Move all the entries in argv which are relevant to GTK+ and GNOME
     * into gui_argv.  Freed later in gui_mch_init().
     */
    gui_argc = 0;
    gui_argv = (char **)alloc((unsigned)((*argc + 1) * sizeof(char *)));

    g_return_if_fail(gui_argv != NULL);

    gui_argv[gui_argc++] = argv[i++];

    while (i < *argc)
    {
	/* Don't waste CPU cycles on non-option arguments. */
	if (argv[i][0] != '-' && argv[i][0] != '+')
	{
	    ++i;
	    continue;
	}

	/* Look for argv[i] in cmdline_options[] table. */
	for (option = &cmdline_options[0]; option->name != NULL; ++option)
	{
	    len = strlen(option->name);

	    if (strncmp(argv[i], option->name, len) == 0)
	    {
		if (argv[i][len] == '\0')
		    break;
		/* allow --foo=bar style */
		if (argv[i][len] == '=' && (option->flags & ARG_HAS_VALUE))
		    break;
#ifdef FEAT_NETBEANS_INTG
		/* darn, -nb has non-standard syntax */
		if (vim_strchr((char_u *)":=", argv[i][len]) != NULL
			&& (option->flags & ARG_INDEX_MASK) == ARG_NETBEANS)
		    break;
#endif
	    }
	    else if ((option->flags & ARG_COMPAT_LONG)
			&& strcmp(argv[i], option->name + 1) == 0)
	    {
		/* Replace the standard X arguments "-name" and "-display"
		 * with their GNU-style long option counterparts. */
		argv[i] = (char *)option->name;
		break;
	    }
	}
	if (option->name == NULL) /* no match */
	{
	    ++i;
	    continue;
	}

	if (option->flags & ARG_FOR_GTK)
	{
	    /* Move the argument into gui_argv, which
	     * will later be passed to gtk_init_check() */
	    gui_argv[gui_argc++] = argv[i];
	}
	else
	{
	    char *value = NULL;

	    /* Extract the option's value if there is one.
	     * Accept both "--foo bar" and "--foo=bar" style. */
	    if (option->flags & ARG_HAS_VALUE)
	    {
		if (argv[i][len] == '=')
		    value = &argv[i][len + 1];
		else if (i + 1 < *argc && strcmp(argv[i + 1], "--") != 0)
		    value = argv[i + 1];
	    }

	    /* Check for options handled by Vim itself */
	    switch (option->flags & ARG_INDEX_MASK)
	    {
		case ARG_REVERSE:
		    found_reverse_arg = TRUE;
		    break;
		case ARG_NOREVERSE:
		    found_reverse_arg = FALSE;
		    break;
		case ARG_FONT:
		    font_argument = value;
		    break;
		case ARG_GEOMETRY:
		    if (value != NULL)
			gui.geom = vim_strsave((char_u *)value);
		    break;
		case ARG_BACKGROUND:
		    background_argument = value;
		    break;
		case ARG_FOREGROUND:
		    foreground_argument = value;
		    break;
		case ARG_ICONIC:
		    found_iconic_arg = TRUE;
		    break;
#ifdef HAVE_GTK2
		case ARG_ROLE:
		    role_argument = value; /* used later in gui_mch_open() */
		    break;
#endif
#ifdef FEAT_NETBEANS_INTG
		case ARG_NETBEANS:
		    ++usingNetbeans;
		    gui.dofork = FALSE; /* don't fork() when starting GUI */
		    netbeansArg = argv[i];
		    break;
#endif
		default:
		    break;
	    }
	}

	/* These arguments make gnome_program_init() print a message and exit.
	 * Must start the GUI for this, otherwise ":gui" will exit later! */
	if (option->flags & ARG_NEEDS_GUI)
	    gui.starting = TRUE;

	if (option->flags & ARG_KEEP)
	    ++i;
	else
	{
	    /* Remove the flag from the argument vector. */
	    if (--*argc > i)
	    {
		int n_strip = 1;

		/* Move the argument's value as well, if there is one. */
		if ((option->flags & ARG_HAS_VALUE)
			&& argv[i][len] != '='
			&& strcmp(argv[i + 1], "--") != 0)
		{
		    ++n_strip;
		    --*argc;
		    if (option->flags & ARG_FOR_GTK)
			gui_argv[gui_argc++] = argv[i + 1];
		}

		if (*argc > i)
		    mch_memmove(&argv[i], &argv[i + n_strip],
						(*argc - i) * sizeof(char *));
	    }
	    argv[*argc] = NULL;
	}
    }

    gui_argv[gui_argc] = NULL;
}

#if defined(EXITFREE) || defined(PROTO)
    void
gui_mch_free_all()
{
    vim_free(gui_argv);
}
#endif

/*
 * This should be maybe completely removed.
 * Doesn't seem possible, since check_copy_area() relies on
 * this information.  --danielk
 */
/*ARGSUSED*/
    static gint
visibility_event(GtkWidget *widget, GdkEventVisibility *event, gpointer data)
{
    gui.visibility = event->state;
    /*
     * When we do an gdk_window_copy_area(), and the window is partially
     * obscured, we want to receive an event to tell us whether it worked
     * or not.
     */
    if (gui.text_gc != NULL)
	gdk_gc_set_exposures(gui.text_gc,
			     gui.visibility != GDK_VISIBILITY_UNOBSCURED);
    return FALSE;
}

/*
 * Redraw the corresponding portions of the screen.
 */
/*ARGSUSED*/
    static gint
expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    /* Skip this when the GUI isn't set up yet, will redraw later. */
    if (gui.starting)
	return FALSE;

    out_flush();		/* make sure all output has been processed */
    gui_redraw(event->area.x, event->area.y,
	       event->area.width, event->area.height);

    /* Clear the border areas if needed */
    if (event->area.x < FILL_X(0))
	gdk_window_clear_area(gui.drawarea->window, 0, 0, FILL_X(0), 0);
    if (event->area.y < FILL_Y(0))
	gdk_window_clear_area(gui.drawarea->window, 0, 0, 0, FILL_Y(0));
    if (event->area.x > FILL_X(Columns))
	gdk_window_clear_area(gui.drawarea->window,
			      FILL_X((int)Columns), 0, 0, 0);
    if (event->area.y > FILL_Y(Rows))
	gdk_window_clear_area(gui.drawarea->window, 0, FILL_Y((int)Rows), 0, 0);

    return FALSE;
}

#ifdef FEAT_CLIENTSERVER
/*
 * Handle changes to the "Comm" property
 */
/*ARGSUSED2*/
    static gint
property_event(GtkWidget *widget, GdkEventProperty *event, gpointer data)
{
    if (event->type == GDK_PROPERTY_NOTIFY
	    && event->state == (int)GDK_PROPERTY_NEW_VALUE
	    && GDK_WINDOW_XWINDOW(event->window) == commWindow
	    && GET_X_ATOM(event->atom) == commProperty)
    {
	XEvent xev;

	/* Translate to XLib */
	xev.xproperty.type = PropertyNotify;
	xev.xproperty.atom = commProperty;
	xev.xproperty.window = commWindow;
	xev.xproperty.state = PropertyNewValue;
	serverEventProc(GDK_WINDOW_XDISPLAY(widget->window), &xev);

	if (gtk_main_level() > 0)
	    gtk_main_quit();
    }
    return FALSE;
}
#endif


/****************************************************************************
 * Focus handlers:
 */


/*
 * This is a simple state machine:
 * BLINK_NONE	not blinking at all
 * BLINK_OFF	blinking, cursor is not shown
 * BLINK_ON	blinking, cursor is shown
 */

#define BLINK_NONE  0
#define BLINK_OFF   1
#define BLINK_ON    2

static int blink_state = BLINK_NONE;
static long_u blink_waittime = 700;
static long_u blink_ontime = 400;
static long_u blink_offtime = 250;
static guint blink_timer = 0;

    void
gui_mch_set_blinking(long waittime, long on, long off)
{
    blink_waittime = waittime;
    blink_ontime = on;
    blink_offtime = off;
}

/*
 * Stop the cursor blinking.  Show the cursor if it wasn't shown.
 */
    void
gui_mch_stop_blink(void)
{
    if (blink_timer)
    {
	gtk_timeout_remove(blink_timer);
	blink_timer = 0;
    }
    if (blink_state == BLINK_OFF)
	gui_update_cursor(TRUE, FALSE);
    blink_state = BLINK_NONE;
}

/*ARGSUSED*/
    static gint
blink_cb(gpointer data)
{
    if (blink_state == BLINK_ON)
    {
	gui_undraw_cursor();
	blink_state = BLINK_OFF;
	blink_timer = gtk_timeout_add((guint32)blink_offtime,
				   (GtkFunction) blink_cb, NULL);
    }
    else
    {
	gui_update_cursor(TRUE, FALSE);
	blink_state = BLINK_ON;
	blink_timer = gtk_timeout_add((guint32)blink_ontime,
				   (GtkFunction) blink_cb, NULL);
    }

    return FALSE;		/* don't happen again */
}

/*
 * Start the cursor blinking.  If it was already blinking, this restarts the
 * waiting time and shows the cursor.
 */
    void
gui_mch_start_blink(void)
{
    if (blink_timer)
	gtk_timeout_remove(blink_timer);
    /* Only switch blinking on if none of the times is zero */
    if (blink_waittime && blink_ontime && blink_offtime && gui.in_focus)
    {
	blink_timer = gtk_timeout_add((guint32)blink_waittime,
				   (GtkFunction) blink_cb, NULL);
	blink_state = BLINK_ON;
	gui_update_cursor(TRUE, FALSE);
    }
}

/*ARGSUSED*/
    static gint
enter_notify_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
    if (blink_state == BLINK_NONE)
	gui_mch_start_blink();

    /* make sure keyboard input goes there */
    if (gtk_socket_id == 0 || !GTK_WIDGET_HAS_FOCUS(gui.drawarea))
	gtk_widget_grab_focus(gui.drawarea);

    return FALSE;
}

/*ARGSUSED*/
    static gint
leave_notify_event(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
    if (blink_state != BLINK_NONE)
	gui_mch_stop_blink();

    return FALSE;
}

/*ARGSUSED*/
    static gint
focus_in_event(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
    gui_focus_change(TRUE);

    if (blink_state == BLINK_NONE)
	gui_mch_start_blink();

    /* make sure keyboard input goes to the draw area (if this is focus for a
     * window) */
    if (widget != gui.drawarea)
	gtk_widget_grab_focus(gui.drawarea);

    /* make sure the input buffer is read */
    if (gtk_main_level() > 0)
	gtk_main_quit();

    return TRUE;
}

/*ARGSUSED*/
    static gint
focus_out_event(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
    gui_focus_change(FALSE);

    if (blink_state != BLINK_NONE)
	gui_mch_stop_blink();

    /* make sure the input buffer is read */
    if (gtk_main_level() > 0)
	gtk_main_quit();

    return TRUE;
}


#ifdef HAVE_GTK2
/*
 * Translate a GDK key value to UTF-8 independently of the current locale.
 * The output is written to string, which must have room for at least 6 bytes
 * plus the NUL terminator.  Returns the length in bytes.
 *
 * This function is used in the GTK+ 2 GUI only.  The GTK+ 1 code makes use
 * of GdkEventKey::string instead.  But event->string is evil; see here why:
 * http://developer.gnome.org/doc/API/2.0/gdk/gdk-Event-Structures.html#GdkEventKey
 */
    static int
keyval_to_string(unsigned int keyval, unsigned int state, char_u *string)
{
    int	    len;
    guint32 uc;

    uc = gdk_keyval_to_unicode(keyval);
    if (uc != 0)
    {
	/* Check for CTRL-foo */
	if ((state & GDK_CONTROL_MASK) && uc >= 0x20 && uc < 0x80)
	{
	    /* These mappings look arbitrary at the first glance, but in fact
	     * resemble quite exactly the behaviour of the GTK+ 1.2 GUI on my
	     * machine.  The only difference is BS vs. DEL for CTRL-8 (makes
	     * more sense and is consistent with usual terminal behaviour). */
	    if (uc >= '@')
		string[0] = uc & 0x1F;
	    else if (uc == '2')
		string[0] = NUL;
	    else if (uc >= '3' && uc <= '7')
		string[0] = uc ^ 0x28;
	    else if (uc == '8')
		string[0] = BS;
	    else if (uc == '?')
		string[0] = DEL;
	    else
		string[0] = uc;
	    len = 1;
	}
	else
	{
	    /* Translate a normal key to UTF-8.  This doesn't work for dead
	     * keys of course, you _have_ to use an input method for that. */
	    len = utf_char2bytes((int)uc, string);
	}
    }
    else
    {
	/* Translate keys which are represented by ASCII control codes in Vim.
	 * There are only a few of those; most control keys are translated to
	 * special terminal-like control sequences. */
	len = 1;
	switch (keyval)
	{
	    case GDK_Tab: case GDK_KP_Tab: case GDK_ISO_Left_Tab:
		string[0] = TAB;
		break;
	    case GDK_Linefeed:
		string[0] = NL;
		break;
	    case GDK_Return: case GDK_ISO_Enter: case GDK_3270_Enter:
		string[0] = CAR;
		break;
	    case GDK_Escape:
		string[0] = ESC;
		break;
	    default:
		len = 0;
		break;
	}
    }
    string[len] = NUL;

    return len;
}
#endif /* HAVE_GTK2 */

    static int
modifiers_gdk2vim(guint state)
{
    int modifiers = 0;

    if (state & GDK_SHIFT_MASK)
	modifiers |= MOD_MASK_SHIFT;
    if (state & GDK_CONTROL_MASK)
	modifiers |= MOD_MASK_CTRL;
    if (state & GDK_MOD1_MASK)
	modifiers |= MOD_MASK_ALT;
    if (state & GDK_MOD4_MASK)
	modifiers |= MOD_MASK_META;

    return modifiers;
}

    static int
modifiers_gdk2mouse(guint state)
{
    int modifiers = 0;

    if (state & GDK_SHIFT_MASK)
	modifiers |= MOUSE_SHIFT;
    if (state & GDK_CONTROL_MASK)
	modifiers |= MOUSE_CTRL;
    if (state & GDK_MOD1_MASK)
	modifiers |= MOUSE_ALT;

    return modifiers;
}

/*
 * Main keyboard handler:
 */
/*ARGSUSED*/
    static gint
key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
#ifdef HAVE_GTK2
    /* 256 bytes is way over the top, but for safety let's reduce it only
     * for GTK+ 2 where we know for sure how large the string might get.
     * (That is, up to 6 bytes + NUL + CSI escapes + safety measure.) */
    char_u	string[32], string2[32];
#else
    char_u	string[256], string2[256];
#endif
    guint	key_sym;
    int		len;
    int		i;
    int		modifiers;
    int		key;
    guint	state;
    char_u	*s, *d;

    key_sym = event->keyval;
    state = event->state;
#ifndef HAVE_GTK2 /* deprecated */
    len = event->length;
    g_assert(len <= sizeof(string));
#endif

#ifndef HAVE_GTK2
    /*
     * It appears as if we always want to consume a key-press (there currently
     * aren't any 'return FALSE's), so we always do this: when running in a
     * GtkPlug and not a window, we must prevent emission of the key_press
     * EVENT from continuing (which is 'beyond' the level of stopping mere
     * signals by returning FALSE), otherwise things like tab/cursor-keys are
     * processed by the GtkPlug default handler, which moves input focus away
     * from us!
     * Note: This should no longer be necessary with GTK+ 2.
     */
    if (gtk_socket_id != 0)
	gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "key_press_event");
#endif

#ifdef FEAT_XIM
    if (xim_queue_key_press_event(event, TRUE))
	return TRUE;
#endif

#ifdef FEAT_HANGULIN
    if (key_sym == GDK_space && (state & GDK_SHIFT_MASK))
    {
	hangul_input_state_toggle();
	return TRUE;
    }
#endif

#ifdef SunXK_F36
    /*
     * These keys have bogus lookup strings, and trapping them here is
     * easier than trying to XRebindKeysym() on them with every possible
     * combination of modifiers.
     */
    if (key_sym == SunXK_F36 || key_sym == SunXK_F37)
	len = 0;
    else
#endif
    {
#ifdef HAVE_GTK2
	len = keyval_to_string(key_sym, state, string2);

	/* Careful: convert_input() doesn't handle the NUL character.
	 * No need to convert pure ASCII anyway, thus the len > 1 check. */
	if (len > 1 && input_conv.vc_type != CONV_NONE)
	    len = convert_input(string2, len, sizeof(string2));

	s = string2;
#else
# ifdef FEAT_MBYTE
	if (input_conv.vc_type != CONV_NONE)
	{
	    mch_memmove(string2, event->string, len);
	    len = convert_input(string2, len, sizeof(string2));
	    s = string2;
	}
	else
# endif
	    s = (char_u *)event->string;
#endif

	d = string;
	for (i = 0; i < len; ++i)
	{
	    *d++ = s[i];
	    if (d[-1] == CSI && d + 2 < string + sizeof(string))
	    {
		/* Turn CSI into K_CSI. */
		*d++ = KS_EXTRA;
		*d++ = (int)KE_CSI;
	    }
	}
	len = d - string;
    }

    /* Shift-Tab results in Left_Tab, but we want <S-Tab> */
    if (key_sym == GDK_ISO_Left_Tab)
    {
	key_sym = GDK_Tab;
	state |= GDK_SHIFT_MASK;
    }

#ifndef HAVE_GTK2 /* for GTK+ 2, we handle this in keyval_to_string() */
    if ((key_sym == GDK_2 || key_sym == GDK_at) && (state & GDK_CONTROL_MASK))
    {
	string[0] = NUL;	/* CTRL-2 and CTRL-@ is NUL */
	len = 1;
    }
    else if (len == 0 && (key_sym == GDK_space || key_sym == GDK_Tab))
    {
	/* When there are modifiers, these keys get zero length; we need the
	 * original key here to be able to add a modifier below. */
	string[0] = (key_sym & 0xff);
	len = 1;
    }
#endif

#ifdef FEAT_MENU
    /* If there is a menu and 'wak' is "yes", or 'wak' is "menu" and the key
     * is a menu shortcut, we ignore everything with the ALT modifier. */
    if ((state & GDK_MOD1_MASK)
	    && gui.menu_is_active
	    && (*p_wak == 'y'
		|| (*p_wak == 'm'
		    && len == 1
		    && gui_is_menu_shortcut(string[0]))))
# ifdef HAVE_GTK2
	/* For GTK2 we return false to signify that we haven't handled the
	 * keypress, so that gtk will handle the mnemonic or accelerator. */
	return FALSE;
# else
	return TRUE;
# endif
#endif

    /* Check for Alt/Meta key (Mod1Mask), but not for a BS, DEL or character
     * that already has the 8th bit set.
     * Don't do this for <S-M-Tab>, that should become K_S_TAB with ALT.
     * Don't do this for double-byte encodings, it turns the char into a lead
     * byte. */
    if (len == 1
	    && (state & GDK_MOD1_MASK)
	    && !(key_sym == GDK_BackSpace || key_sym == GDK_Delete)
	    && (string[0] & 0x80) == 0
	    && !(key_sym == GDK_Tab && (state & GDK_SHIFT_MASK))
#ifdef FEAT_MBYTE
	    && !enc_dbcs
#endif
	    )
    {
	string[0] |= 0x80;
	state &= ~GDK_MOD1_MASK;	/* don't use it again */
#ifdef FEAT_MBYTE
	if (enc_utf8) /* convert to utf-8 */
	{
	    string[1] = string[0] & 0xbf;
	    string[0] = ((unsigned)string[0] >> 6) + 0xc0;
	    if (string[1] == CSI)
	    {
		string[2] = KS_EXTRA;
		string[3] = (int)KE_CSI;
		len = 4;
	    }
	    else
		len = 2;
	}
#endif
    }

    /* Check for special keys.	Also do this when len == 1 (key has an ASCII
     * value) to detect backspace, delete and keypad keys. */
    if (len == 0 || len == 1)
    {
	for (i = 0; special_keys[i].key_sym != 0; i++)
	{
	    if (special_keys[i].key_sym == key_sym)
	    {
		string[0] = CSI;
		string[1] = special_keys[i].code0;
		string[2] = special_keys[i].code1;
		len = -3;
		break;
	    }
	}
    }

    if (len == 0)   /* Unrecognized key */
	return TRUE;

#if defined(FEAT_XIM) && defined(FEAT_GUI_GTK) && !defined(HAVE_GTK2)
    /* Cancel or type backspace. For GTK2, im_commit_cb() does the same. */
    preedit_start_col = MAXCOL;
    xim_changed_while_preediting = TRUE;
#endif

    /* Special keys (and a few others) may have modifiers. Also when using a
     * double-byte encoding (can't set the 8th bit). */
    if (len == -3 || key_sym == GDK_space || key_sym == GDK_Tab
	    || key_sym == GDK_Return || key_sym == GDK_Linefeed
	    || key_sym == GDK_Escape || key_sym == GDK_KP_Tab
	    || key_sym == GDK_ISO_Enter || key_sym == GDK_3270_Enter
#ifdef FEAT_MBYTE
	    || (enc_dbcs && len == 1 && (state & GDK_MOD1_MASK))
#endif
	    )
    {
	modifiers = modifiers_gdk2vim(state);

	/*
	 * For some keys a shift modifier is translated into another key
	 * code.
	 */
	if (len == -3)
	    key = TO_SPECIAL(string[1], string[2]);
	else
	    key = string[0];

	key = simplify_key(key, &modifiers);
	if (key == CSI)
	    key = K_CSI;
	if (IS_SPECIAL(key))
	{
	    string[0] = CSI;
	    string[1] = K_SECOND(key);
	    string[2] = K_THIRD(key);
	    len = 3;
	}
	else
	{
	    string[0] = key;
	    len = 1;
	}

	if (modifiers != 0)
	{
	    string2[0] = CSI;
	    string2[1] = KS_MODIFIER;
	    string2[2] = modifiers;
	    add_to_input_buf(string2, 3);
	}
    }

    if (len == 1 && ((string[0] == Ctrl_C && ctrl_c_interrupts)
		   || (string[0] == intr_char && intr_char != Ctrl_C)))
    {
	trash_input_buf();
	got_int = TRUE;
    }

    add_to_input_buf(string, len);

    /* blank out the pointer if necessary */
    if (p_mh)
	gui_mch_mousehide(TRUE);

    if (gtk_main_level() > 0)
	gtk_main_quit();

    return TRUE;
}

#if defined(FEAT_XIM) && defined(HAVE_GTK2)
/*ARGSUSED0*/
    static gboolean
key_release_event(GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    /*
     * GTK+ 2 input methods may do fancy stuff on key release events too.
     * With the default IM for instance, you can enter any UCS code point
     * by holding down CTRL-SHIFT and typing hexadecimal digits.
     */
    return xim_queue_key_press_event(event, FALSE);
}
#endif


/****************************************************************************
 * Selection handlers:
 */

/*ARGSUSED*/
    static gint
selection_clear_event(GtkWidget		*widget,
		      GdkEventSelection	*event,
		      gpointer		user_data)
{
    if (event->selection == clip_plus.gtk_sel_atom)
	clip_lose_selection(&clip_plus);
    else
	clip_lose_selection(&clip_star);

    if (gtk_main_level() > 0)
	gtk_main_quit();

    return TRUE;
}

#define RS_NONE	0	/* selection_received_cb() not called yet */
#define RS_OK	1	/* selection_received_cb() called and OK */
#define RS_FAIL	2	/* selection_received_cb() called and failed */
static int received_selection = RS_NONE;

/*ARGSUSED*/
    static void
selection_received_cb(GtkWidget		*widget,
		      GtkSelectionData	*data,
		      guint		time_,
		      gpointer		user_data)
{
    VimClipboard    *cbd;
    char_u	    *text;
    char_u	    *tmpbuf = NULL;
#ifdef HAVE_GTK2
    guchar	    *tmpbuf_utf8 = NULL;
#endif
    int		    len;
    int		    motion_type;

    if (data->selection == clip_plus.gtk_sel_atom)
	cbd = &clip_plus;
    else
	cbd = &clip_star;

    text = (char_u *)data->data;
    len  = data->length;
    motion_type = MCHAR;

    if (text == NULL || len <= 0)
    {
	received_selection = RS_FAIL;
	/* clip_free_selection(cbd); ??? */

	if (gtk_main_level() > 0)
	    gtk_main_quit();

	return;
    }

    if (data->type == vim_atom)
    {
	motion_type = *text++;
	--len;
    }

#ifdef FEAT_MBYTE
    else if (data->type == vimenc_atom)
    {
	char_u		*enc;
	vimconv_T	conv;

	motion_type = *text++;
	--len;

	enc = text;
	text += STRLEN(text) + 1;
	len -= text - enc;

	/* If the encoding of the text is different from 'encoding', attempt
	 * converting it. */
	conv.vc_type = CONV_NONE;
	convert_setup(&conv, enc, p_enc);
	if (conv.vc_type != CONV_NONE)
	{
	    tmpbuf = string_convert(&conv, text, &len);
	    if (tmpbuf != NULL)
		text = tmpbuf;
	    convert_setup(&conv, NULL, NULL);
	}
    }
#endif

#ifdef HAVE_GTK2
    /* gtk_selection_data_get_text() handles all the nasty details
     * and targets and encodings etc.  This rocks so hard. */
    else
    {
	tmpbuf_utf8 = gtk_selection_data_get_text(data);
	if (tmpbuf_utf8 != NULL)
	{
	    len = STRLEN(tmpbuf_utf8);
	    if (input_conv.vc_type != CONV_NONE)
	    {
		tmpbuf = string_convert(&input_conv, tmpbuf_utf8, &len);
		if (tmpbuf != NULL)
		    text = tmpbuf;
	    }
	    else
		text = tmpbuf_utf8;
	}
    }
#else /* !HAVE_GTK2 */
# ifdef FEAT_MBYTE
    else if (data->type == utf8_string_atom)
    {
	vimconv_T conv;

	conv.vc_type = CONV_NONE;
	convert_setup(&conv, (char_u *)"utf-8", p_enc);

	if (conv.vc_type != CONV_NONE)
	{
	    tmpbuf = string_convert(&conv, text, &len);
	    convert_setup(&conv, NULL, NULL);
	}
	if (tmpbuf != NULL)
	    text = tmpbuf;
    }
# endif
    else if (data->type == compound_text_atom || data->type == text_atom)
    {
	char	    **list = NULL;
	int	    count;
	int	    i;
	unsigned    tmplen = 0;

	count = gdk_text_property_to_text_list(data->type, data->format,
					       data->data, data->length,
					       &list);
	for (i = 0; i < count; ++i)
	    tmplen += strlen(list[i]);

	tmpbuf = alloc(tmplen + 1);
	if (tmpbuf != NULL)
	{
	    tmpbuf[0] = NUL;
	    for (i = 0; i < count; ++i)
		STRCAT(tmpbuf, list[i]);
	    text = tmpbuf;
	    len  = tmplen;
	}

	if (list != NULL)
	    gdk_free_text_list(list);
    }
#endif /* !HAVE_GTK2 */

    clip_yank_selection(motion_type, text, (long)len, cbd);
    received_selection = RS_OK;
    vim_free(tmpbuf);
#ifdef HAVE_GTK2
    g_free(tmpbuf_utf8);
#endif

    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*
 * Prepare our selection data for passing it to the external selection
 * client.
 */
/*ARGSUSED*/
    static void
selection_get_cb(GtkWidget	    *widget,
		 GtkSelectionData   *selection_data,
		 guint		    info,
		 guint		    time_,
		 gpointer	    user_data)
{
    char_u	    *string;
    char_u	    *tmpbuf;
    long_u	    tmplen;
    int		    length;
    int		    motion_type;
    GdkAtom	    type;
    VimClipboard    *cbd;

    if (selection_data->selection == clip_plus.gtk_sel_atom)
	cbd = &clip_plus;
    else
	cbd = &clip_star;

    if (!cbd->owned)
	return;			/* Shouldn't ever happen */

    if (info != (guint)TARGET_STRING
#ifdef FEAT_MBYTE
	    && info != (guint)TARGET_UTF8_STRING
	    && info != (guint)TARGET_VIMENC
#endif
	    && info != (guint)TARGET_VIM
	    && info != (guint)TARGET_COMPOUND_TEXT
	    && info != (guint)TARGET_TEXT)
	return;

    /* get the selection from the '*'/'+' register */
    clip_get_selection(cbd);

    motion_type = clip_convert_selection(&string, &tmplen, cbd);
    if (motion_type < 0 || string == NULL)
	return;
    /* Due to int arguments we can't handle more than G_MAXINT.  Also
     * reserve one extra byte for NUL or the motion type; just in case.
     * (Not that pasting 2G of text is ever going to work, but... ;-) */
    length = MIN(tmplen, (long_u)(G_MAXINT - 1));

    if (info == (guint)TARGET_VIM)
    {
	tmpbuf = alloc((unsigned)length + 1);
	if (tmpbuf != NULL)
	{
	    tmpbuf[0] = motion_type;
	    mch_memmove(tmpbuf + 1, string, (size_t)length);
	}
	/* For our own format, the first byte contains the motion type */
	++length;
	vim_free(string);
	string = tmpbuf;
	type = vim_atom;
    }

#ifdef FEAT_MBYTE
    else if (info == (guint)TARGET_VIMENC)
    {
	int l = STRLEN(p_enc);

	/* contents: motion_type 'encoding' NUL text */
	tmpbuf = alloc((unsigned)length + l + 2);
	if (tmpbuf != NULL)
	{
	    tmpbuf[0] = motion_type;
	    STRCPY(tmpbuf + 1, p_enc);
	    mch_memmove(tmpbuf + l + 2, string, (size_t)length);
	}
	length += l + 2;
	vim_free(string);
	string = tmpbuf;
	type = vimenc_atom;
    }
#endif

#ifdef HAVE_GTK2
    /* gtk_selection_data_set_text() handles everything for us.  This is
     * so easy and simple and cool, it'd be insane not to use it. */
    else
    {
	if (output_conv.vc_type != CONV_NONE)
	{
	    tmpbuf = string_convert(&output_conv, string, &length);
	    vim_free(string);
	    if (tmpbuf == NULL)
		return;
	    string = tmpbuf;
	}
	/* Validate the string to avoid runtime warnings */
	if (g_utf8_validate((const char *)string, (gssize)length, NULL))
	{
	    gtk_selection_data_set_text(selection_data,
					(const char *)string, length);
	}
	vim_free(string);
	return;
    }
#else /* !HAVE_GTK2 */
# ifdef FEAT_MBYTE
    else if (info == (guint)TARGET_UTF8_STRING)
    {
	vimconv_T conv;

	conv.vc_type = CONV_NONE;
	convert_setup(&conv, p_enc, (char_u *)"utf-8");

	if (conv.vc_type != CONV_NONE)
	{
	    tmpbuf = string_convert(&conv, string, &length);
	    convert_setup(&conv, NULL, NULL);
	    vim_free(string);
	    string = tmpbuf;
	}
	type = utf8_string_atom;
    }
# endif
    else if (info == (guint)TARGET_COMPOUND_TEXT
		|| info == (guint)TARGET_TEXT)
    {
	int format;

	/* Copy the string to ensure NUL-termination */
	tmpbuf = vim_strnsave(string, length);
	vim_free(string);
	if (tmpbuf != NULL)
	{
	    gdk_string_to_compound_text((const char *)tmpbuf,
					&type, &format, &string, &length);
	    vim_free(tmpbuf);
	    selection_data->type = type;
	    selection_data->format = format;
	    gtk_selection_data_set(selection_data, type, format, string, length);
	    gdk_free_compound_text(string);
	}
	return;
    }
    else
    {
	type = GDK_TARGET_STRING;
    }
#endif /* !HAVE_GTK2 */

    if (string != NULL)
    {
	selection_data->type = selection_data->target;
	selection_data->format = 8;	/* 8 bits per char */

	gtk_selection_data_set(selection_data, type, 8, string, length);
	vim_free(string);
    }
}

/*
 * Check if the GUI can be started.  Called before gvimrc is sourced.
 * Return OK or FAIL.
 */
    int
gui_mch_init_check(void)
{
#ifndef HAVE_GTK2
    /* This is needed to make the locale handling consistent between the GUI
     * and the rest of VIM. */
    gtk_set_locale();
#endif

#ifdef FEAT_GUI_GNOME
    if (gtk_socket_id == 0)
	using_gnome = 1;
#endif

    /* Don't use gtk_init() or gnome_init(), it exits on failure. */
    if (!gtk_init_check(&gui_argc, &gui_argv))
    {
	gui.dying = TRUE;
	EMSG(_((char *)e_opendisp));
	return FAIL;
    }

    return OK;
}


/****************************************************************************
 * Mouse handling callbacks
 */


static guint mouse_click_timer = 0;
static int mouse_timed_out = TRUE;

/*
 * Timer used to recognize multiple clicks of the mouse button
 */
    static gint
mouse_click_timer_cb(gpointer data)
{
    /* we don't use this information currently */
    int *timed_out = (int *) data;

    *timed_out = TRUE;
    return FALSE;		/* don't happen again */
}

static guint motion_repeat_timer  = 0;
static int   motion_repeat_offset = FALSE;
static gint  motion_repeat_timer_cb(gpointer);

    static void
process_motion_notify(int x, int y, GdkModifierType state)
{
    int	    button;
    int_u   vim_modifiers;

    button = (state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK |
		       GDK_BUTTON3_MASK | GDK_BUTTON4_MASK |
		       GDK_BUTTON5_MASK))
	      ? MOUSE_DRAG : ' ';

    /* If our pointer is currently hidden, then we should show it. */
    gui_mch_mousehide(FALSE);

    /* Just moving the rodent above the drawing area without any button
     * being pressed. */
    if (button != MOUSE_DRAG)
    {
	gui_mouse_moved(x, y);
	return;
    }

    /* translate modifier coding between the main engine and GTK */
    vim_modifiers = modifiers_gdk2mouse(state);

    /* inform the editor engine about the occurrence of this event */
    gui_send_mouse_event(button, x, y, FALSE, vim_modifiers);

    if (gtk_main_level() > 0)
	gtk_main_quit();

    /*
     * Auto repeat timer handling.
     */
    if (x < 0 || y < 0
	    || x >= gui.drawarea->allocation.width
	    || y >= gui.drawarea->allocation.height)
    {

	int dx;
	int dy;
	int offshoot;
	int delay = 10;

	/* Calculate the maximal distance of the cursor from the drawing area.
	 * (offshoot can't become negative here!).
	 */
	dx = x < 0 ? -x : x - gui.drawarea->allocation.width;
	dy = y < 0 ? -y : y - gui.drawarea->allocation.height;

	offshoot = dx > dy ? dx : dy;

	/* Make a linearly declaying timer delay with a threshold of 5 at a
	 * distance of 127 pixels from the main window.
	 *
	 * One could think endlessly about the most ergonomic variant here.
	 * For example it could make sense to calculate the distance from the
	 * drags start instead...
	 *
	 * Maybe a parabolic interpolation would suite us better here too...
	 */
	if (offshoot > 127)
	{
	    /* 5 appears to be somehow near to my perceptual limits :-). */
	    delay = 5;
	}
	else
	{
	    delay = (130 * (127 - offshoot)) / 127 + 5;
	}

	/* shoot again */
	if (!motion_repeat_timer)
	    motion_repeat_timer = gtk_timeout_add((guint32)delay,
						motion_repeat_timer_cb, NULL);
    }
}

/*
 * Timer used to recognize multiple clicks of the mouse button.
 */
/*ARGSUSED0*/
    static gint
motion_repeat_timer_cb(gpointer data)
{
    int		    x;
    int		    y;
    GdkModifierType state;

    gdk_window_get_pointer(gui.drawarea->window, &x, &y, &state);

    if (!(state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK |
		   GDK_BUTTON3_MASK | GDK_BUTTON4_MASK |
		   GDK_BUTTON5_MASK)))
    {
	motion_repeat_timer = 0;
	return FALSE;
    }

    /* If there already is a mouse click in the input buffer, wait another
     * time (otherwise we would create a backlog of clicks) */
    if (vim_used_in_input_buf() > 10)
	return TRUE;

    motion_repeat_timer = 0;

    /*
     * Fake a motion event.
     * Trick: Pretend the mouse moved to the next character on every other
     * event, otherwise drag events will be discarded, because they are still
     * in the same character.
     */
    if (motion_repeat_offset)
	x += gui.char_width;

    motion_repeat_offset = !motion_repeat_offset;
    process_motion_notify(x, y, state);

    /* Don't happen again.  We will get reinstalled in the synthetic event
     * if needed -- thus repeating should still work. */
    return FALSE;
}

/*ARGSUSED2*/
    static gint
motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    if (event->is_hint)
    {
	int		x;
	int		y;
	GdkModifierType	state;

	gdk_window_get_pointer(widget->window, &x, &y, &state);
	process_motion_notify(x, y, state);
    }
    else
    {
	process_motion_notify((int)event->x, (int)event->y,
			      (GdkModifierType)event->state);
    }

    return TRUE; /* handled */
}


/*
 * Mouse button handling.  Note please that we are capturing multiple click's
 * by our own timeout mechanism instead of the one provided by GTK+ itself.
 * This is due to the way the generic VIM code is recognizing multiple clicks.
 */
/*ARGSUSED2*/
    static gint
button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    int button;
    int repeated_click = FALSE;
    int x, y;
    int_u vim_modifiers;

    /* Make sure we have focus now we've been selected */
    if (gtk_socket_id != 0 && !GTK_WIDGET_HAS_FOCUS(widget))
	gtk_widget_grab_focus(widget);

    /*
     * Don't let additional events about multiple clicks send by GTK to us
     * after the initial button press event confuse us.
     */
    if (event->type != GDK_BUTTON_PRESS)
	return FALSE;

    x = event->x;
    y = event->y;

    /* Handle multiple clicks */
    if (!mouse_timed_out && mouse_click_timer)
    {
	gtk_timeout_remove(mouse_click_timer);
	mouse_click_timer = 0;
	repeated_click = TRUE;
    }

    mouse_timed_out = FALSE;
    mouse_click_timer = gtk_timeout_add((guint32)p_mouset,
				  mouse_click_timer_cb, &mouse_timed_out);

    switch (event->button)
    {
    case 1:
	button = MOUSE_LEFT;
	break;
    case 2:
	button = MOUSE_MIDDLE;
	break;
    case 3:
	button = MOUSE_RIGHT;
	break;
#ifndef HAVE_GTK2
    case 4:
	button = MOUSE_4;
	break;
    case 5:
	button = MOUSE_5;
	break;
#endif
    default:
	return FALSE;		/* Unknown button */
    }

#ifdef FEAT_XIM
    /* cancel any preediting */
    if (im_is_preediting())
	xim_reset();
#endif

    vim_modifiers = modifiers_gdk2mouse(event->state);

    gui_send_mouse_event(button, x, y, repeated_click, vim_modifiers);
    if (gtk_main_level() > 0)
	gtk_main_quit(); /* make sure the above will be handled immediately */

    return TRUE;
}

#ifdef HAVE_GTK2
/*
 * GTK+ 2 doesn't handle mouse buttons 4, 5, 6 and 7 the same way as GTK+ 1.
 * Instead, it abstracts scrolling via the new GdkEventScroll.
 */
/*ARGSUSED2*/
    static gboolean
scroll_event(GtkWidget *widget, GdkEventScroll *event, gpointer data)
{
    int	    button;
    int_u   vim_modifiers;

    if (gtk_socket_id != 0 && !GTK_WIDGET_HAS_FOCUS(widget))
	gtk_widget_grab_focus(widget);

    switch (event->direction)
    {
	case GDK_SCROLL_UP:
	    button = MOUSE_4;
	    break;
	case GDK_SCROLL_DOWN:
	    button = MOUSE_5;
	    break;
	default: /* We don't care about left and right...  Yet. */
	    return FALSE;
    }

# ifdef FEAT_XIM
    /* cancel any preediting */
    if (im_is_preediting())
	xim_reset();
# endif

    vim_modifiers = modifiers_gdk2mouse(event->state);

    gui_send_mouse_event(button, (int)event->x, (int)event->y,
							FALSE, vim_modifiers);

    if (gtk_main_level() > 0)
	gtk_main_quit(); /* make sure the above will be handled immediately */

    return TRUE;
}
#endif /* HAVE_GTK2 */


/*ARGSUSED*/
    static gint
button_release_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    int x, y;
    int_u vim_modifiers;

    /* Remove any motion "machine gun" timers used for automatic further
       extension of allocation areas if outside of the applications window
       area .*/
    if (motion_repeat_timer)
    {
	gtk_timeout_remove(motion_repeat_timer);
	motion_repeat_timer = 0;
    }

    x = event->x;
    y = event->y;

    vim_modifiers = modifiers_gdk2mouse(event->state);

    gui_send_mouse_event(MOUSE_RELEASE, x, y, FALSE, vim_modifiers);
    if (gtk_main_level() > 0)
	gtk_main_quit();	/* make sure it will be handled immediately */

    return TRUE;
}


#ifdef FEAT_DND
/****************************************************************************
 * Drag aNd Drop support handlers.
 */

/*
 * Count how many items there may be and separate them with a NUL.
 * Apparently the items are separated with \r\n.  This is not documented,
 * thus be careful not to go past the end.	Also allow separation with
 * NUL characters.
 */
    static int
count_and_decode_uri_list(char_u *out, char_u *raw, int len)
{
    int		i;
    char_u	*p = out;
    int		count = 0;

    for (i = 0; i < len; ++i)
    {
	if (raw[i] == NUL || raw[i] == '\n' || raw[i] == '\r')
	{
	    if (p > out && p[-1] != NUL)
	    {
		++count;
		*p++ = NUL;
	    }
	}
	else if (raw[i] == '%' && i + 2 < len && hexhex2nr(raw + i + 1) > 0)
	{
	    *p++ = hexhex2nr(raw + i + 1);
	    i += 2;
	}
	else
	    *p++ = raw[i];
    }
    if (p > out && p[-1] != NUL)
    {
	*p = NUL;	/* last item didn't have \r or \n */
	++count;
    }
    return count;
}

/*
 * Parse NUL separated "src" strings.  Make it an array "outlist" form.  On
 * this process, URI which protocol is not "file:" are removed.  Return
 * length of array (less than "max").
 */
    static int
filter_uri_list(char_u **outlist, int max, char_u *src)
{
    int	i, j;

    for (i = j = 0; i < max; ++i)
    {
	outlist[i] = NULL;
	if (STRNCMP(src, "file:", 5) == 0)
	{
	    src += 5;
	    if (STRNCMP(src, "//localhost", 11) == 0)
		src += 11;
	    while (src[0] == '/' && src[1] == '/')
		++src;
	    outlist[j++] = vim_strsave(src);
	}
	src += STRLEN(src) + 1;
    }
    return j;
}

    static char_u **
parse_uri_list(int *count, char_u *data, int len)
{
    int	    n	    = 0;
    char_u  *tmp    = NULL;
    char_u  **array = NULL;;

    if (data != NULL && len > 0 && (tmp = (char_u *)alloc(len + 1)) != NULL)
    {
	n = count_and_decode_uri_list(tmp, data, len);
	if (n > 0 && (array = (char_u **)alloc(n * sizeof(char_u *))) != NULL)
	    n = filter_uri_list(array, n, tmp);
    }
    vim_free(tmp);
    *count = n;
    return array;
}

    static void
drag_handle_uri_list(GdkDragContext	*context,
		     GtkSelectionData	*data,
		     guint		time_,
		     GdkModifierType	state,
		     gint		x,
		     gint		y)
{
    char_u  **fnames;
    int	    nfiles = 0;

    fnames = parse_uri_list(&nfiles, data->data, data->length);

    if (fnames != NULL && nfiles > 0)
    {
	int_u   modifiers;

	gtk_drag_finish(context, TRUE, FALSE, time_); /* accept */

	modifiers = modifiers_gdk2mouse(state);

	gui_handle_drop(x, y, modifiers, fnames, nfiles);
    }
    else
	vim_free(fnames);
}

    static void
drag_handle_text(GdkDragContext	    *context,
		 GtkSelectionData   *data,
		 guint		    time_,
		 GdkModifierType    state)
{
    char_u  dropkey[6] = {CSI, KS_MODIFIER, 0, CSI, KS_EXTRA, (char_u)KE_DROP};
    char_u  *text;
    int	    len;
# ifdef FEAT_MBYTE
    char_u  *tmpbuf = NULL;
# endif

    text = data->data;
    len  = data->length;

# ifdef FEAT_MBYTE
    if (data->type == utf8_string_atom)
    {
#  ifdef HAVE_GTK2
	if (input_conv.vc_type != CONV_NONE)
	    tmpbuf = string_convert(&input_conv, text, &len);
#  else
	vimconv_T conv;

	conv.vc_type = CONV_NONE;
	convert_setup(&conv, (char_u *)"utf-8", p_enc);

	if (conv.vc_type != CONV_NONE)
	{
	    tmpbuf = string_convert(&conv, text, &len);
	    convert_setup(&conv, NULL, NULL);
	}
#  endif
	if (tmpbuf != NULL)
	    text = tmpbuf;
    }
# endif /* FEAT_MBYTE */

    dnd_yank_drag_data(text, (long)len);
    gtk_drag_finish(context, TRUE, FALSE, time_); /* accept */
# ifdef FEAT_MBYTE
    vim_free(tmpbuf);
# endif

    dropkey[2] = modifiers_gdk2vim(state);

    if (dropkey[2] != 0)
	add_to_input_buf(dropkey, (int)sizeof(dropkey));
    else
	add_to_input_buf(dropkey + 3, (int)(sizeof(dropkey) - 3));

    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*
 * DND receiver.
 */
/*ARGSUSED2*/
    static void
drag_data_received_cb(GtkWidget		*widget,
		      GdkDragContext	*context,
		      gint		x,
		      gint		y,
		      GtkSelectionData	*data,
		      guint		info,
		      guint		time_,
		      gpointer		user_data)
{
    GdkModifierType state;

    /* Guard against trash */
    if (data->data == NULL
	    || data->length <= 0
	    || data->format != 8
	    || data->data[data->length] != '\0')
    {
	gtk_drag_finish(context, FALSE, FALSE, time_);
	return;
    }

    /* Get the current modifier state for proper distinguishment between
     * different operations later. */
    gdk_window_get_pointer(widget->window, NULL, NULL, &state);

    /* Not sure about the role of "text/plain" here... */
    if (info == (guint)TARGET_TEXT_URI_LIST)
	drag_handle_uri_list(context, data, time_, state, x, y);
    else
	drag_handle_text(context, data, time_, state);

}
#endif /* FEAT_DND */


#if defined(FEAT_GUI_GNOME) && defined(FEAT_SESSION)
/*
 * GnomeClient interact callback.  Check for unsaved buffers that cannot
 * be abandoned and pop up a dialog asking the user for confirmation if
 * necessary.
 */
/*ARGSUSED0*/
    static void
sm_client_check_changed_any(GnomeClient	    *client,
			    gint	    key,
			    GnomeDialogType type,
			    gpointer	    data)
{
    cmdmod_T	save_cmdmod;
    gboolean	shutdown_cancelled;

    save_cmdmod = cmdmod;

# ifdef FEAT_BROWSE
    cmdmod.browse = TRUE;
# endif
# if defined(FEAT_GUI_DIALOG) || defined(FEAT_CON_DIALOG)
    cmdmod.confirm = TRUE;
# endif
    /*
     * If there are changed buffers, present the user with
     * a dialog if possible, otherwise give an error message.
     */
    shutdown_cancelled = check_changed_any(FALSE);

    exiting = FALSE;
    cmdmod = save_cmdmod;
    setcursor(); /* position the cursor */
    out_flush();
    /*
     * If the user hit the [Cancel] button the whole shutdown
     * will be cancelled.  Wow, quite powerful feature (:
     */
    gnome_interaction_key_return(key, shutdown_cancelled);
}

/*
 * Generate a script that can be used to restore the current editing session.
 * Save the value of v:this_session before running :mksession in order to make
 * automagic session save fully transparent.  Return TRUE on success.
 */
    static int
write_session_file(char_u *filename)
{
    char_u	    *escaped_filename;
    char	    *mksession_cmdline;
    unsigned int    save_ssop_flags;
    int		    failed;

    /*
     * Build an ex command line to create a script that restores the current
     * session if executed.  Escape the filename to avoid nasty surprises.
     */
    escaped_filename = vim_strsave_escaped(filename, escape_chars);
    if (escaped_filename == NULL)
	return FALSE;
    mksession_cmdline = g_strconcat("mksession ", (char *)escaped_filename,
									NULL);
    vim_free(escaped_filename);

    /*
     * Use a reasonable hardcoded set of 'sessionoptions' flags to avoid
     * unpredictable effects when the session is saved automatically.  Also,
     * we definitely need SSOP_GLOBALS to be able to restore v:this_session.
     * Don't use SSOP_BUFFERS to prevent the buffer list from becoming
     * enormously large if the GNOME session feature is used regularly.
     */
    save_ssop_flags = ssop_flags;
    ssop_flags = (SSOP_BLANK|SSOP_CURDIR|SSOP_FOLDS|SSOP_GLOBALS
		  |SSOP_HELP|SSOP_OPTIONS|SSOP_WINSIZE|SSOP_TABPAGES);

    do_cmdline_cmd((char_u *)"let Save_VV_this_session = v:this_session");
    failed = (do_cmdline_cmd((char_u *)mksession_cmdline) == FAIL);
    do_cmdline_cmd((char_u *)"let v:this_session = Save_VV_this_session");
    do_unlet((char_u *)"Save_VV_this_session", TRUE);

    ssop_flags = save_ssop_flags;
    g_free(mksession_cmdline);
    /*
     * Reopen the file and append a command to restore v:this_session,
     * as if this save never happened.	This is to avoid conflicts with
     * the user's own sessions.  FIXME: It's probably less hackish to add
     * a "stealth" flag to 'sessionoptions' -- gotta ask Bram.
     */
    if (!failed)
    {
	FILE *fd;

	fd = open_exfile(filename, TRUE, APPENDBIN);

	failed = (fd == NULL
	       || put_line(fd, "let v:this_session = Save_VV_this_session") == FAIL
	       || put_line(fd, "unlet Save_VV_this_session") == FAIL);

	if (fd != NULL && fclose(fd) != 0)
	    failed = TRUE;

	if (failed)
	    mch_remove(filename);
    }

    return !failed;
}

/*
 * "save_yourself" signal handler.  Initiate an interaction to ask the user
 * for confirmation if necessary.  Save the current editing session and tell
 * the session manager how to restart Vim.
 */
/*ARGSUSED1*/
    static gboolean
sm_client_save_yourself(GnomeClient	    *client,
			gint		    phase,
			GnomeSaveStyle	    save_style,
			gboolean	    shutdown,
			GnomeInteractStyle  interact_style,
			gboolean	    fast,
			gpointer	    data)
{
    static const char	suffix[] = "-session.vim";
    char		*session_file;
    unsigned int	len;
    gboolean		success;

    /* Always request an interaction if possible.  check_changed_any()
     * won't actually show a dialog unless any buffers have been modified.
     * There doesn't seem to be an obvious way to check that without
     * automatically firing the dialog.  Anyway, it works just fine. */
    if (interact_style == GNOME_INTERACT_ANY)
	gnome_client_request_interaction(client, GNOME_DIALOG_NORMAL,
					 &sm_client_check_changed_any,
					 NULL);
    out_flush();
    ml_sync_all(FALSE, FALSE); /* preserve all swap files */

    /* The path is unique for each session save.  We do neither know nor care
     * which session script will actually be used later.  This decision is in
     * the domain of the session manager. */
    session_file = gnome_config_get_real_path(
			gnome_client_get_config_prefix(client));
    len = strlen(session_file);

    if (len > 0 && session_file[len-1] == G_DIR_SEPARATOR)
	--len; /* get rid of the superfluous trailing '/' */

    session_file = g_renew(char, session_file, len + sizeof(suffix));
    memcpy(session_file + len, suffix, sizeof(suffix));

    success = write_session_file((char_u *)session_file);

    if (success)
    {
	const char  *argv[8];
	int	    i;

	/* Tell the session manager how to wipe out the stored session data.
	 * This isn't as dangerous as it looks, don't worry :)	session_file
	 * is a unique absolute filename.  Usually it'll be something like
	 * `/home/user/.gnome2/vim-XXXXXX-session.vim'. */
	i = 0;
	argv[i++] = "rm";
	argv[i++] = session_file;
	argv[i] = NULL;

	gnome_client_set_discard_command(client, i, (char **)argv);

	/* Tell the session manager how to restore the just saved session.
	 * This is easily done thanks to Vim's -S option.  Pass the -f flag
	 * since there's no need to fork -- it might even cause confusion.
	 * Also pass the window role to give the WM something to match on.
	 * The role is set in gui_mch_open(), thus should _never_ be NULL. */
	i = 0;
	argv[i++] = restart_command;
	argv[i++] = "-f";
	argv[i++] = "-g";
# ifdef HAVE_GTK2
	argv[i++] = "--role";
	argv[i++] = gtk_window_get_role(GTK_WINDOW(gui.mainwin));
# endif
	argv[i++] = "-S";
	argv[i++] = session_file;
	argv[i] = NULL;

	gnome_client_set_restart_command(client, i, (char **)argv);
	gnome_client_set_clone_command(client, 0, NULL);
    }

    g_free(session_file);

    return success;
}

/*
 * Called when the session manager wants us to die.  There isn't much to save
 * here since "save_yourself" has been emitted before (unless serious trouble
 * is happening).
 */
/*ARGSUSED0*/
    static void
sm_client_die(GnomeClient *client, gpointer data)
{
    /* Don't write messages to the GUI anymore */
    full_screen = FALSE;

    vim_strncpy(IObuff, (char_u *)
		    _("Vim: Received \"die\" request from session manager\n"),
		    IOSIZE - 1);
    preserve_exit();
}

/*
 * Connect our signal handlers to be notified on session save and shutdown.
 */
    static void
setup_save_yourself(void)
{
    GnomeClient *client;

    client = gnome_master_client();

    if (client != NULL)
    {
	/* Must use the deprecated gtk_signal_connect() for compatibility
	 * with GNOME 1.  Arrgh, zombies! */
	gtk_signal_connect(GTK_OBJECT(client), "save_yourself",
			   GTK_SIGNAL_FUNC(&sm_client_save_yourself), NULL);
	gtk_signal_connect(GTK_OBJECT(client), "die",
			   GTK_SIGNAL_FUNC(&sm_client_die), NULL);
    }
}

#else /* !(FEAT_GUI_GNOME && FEAT_SESSION) */

# ifdef USE_XSMP
/*
 * GTK tells us that XSMP needs attention
 */
/*ARGSUSED*/
    static gboolean
local_xsmp_handle_requests(source, condition, data)
    GIOChannel		*source;
    GIOCondition	condition;
    gpointer		data;
{
    if (condition == G_IO_IN)
    {
	/* Do stuff; maybe close connection */
	if (xsmp_handle_requests() == FAIL)
	    g_io_channel_unref((GIOChannel *)data);
	return TRUE;
    }
    /* Error */
    g_io_channel_unref((GIOChannel *)data);
    xsmp_close();
    return TRUE;
}
# endif /* USE_XSMP */

/*
 * Setup the WM_PROTOCOLS to indicate we want the WM_SAVE_YOURSELF event.
 * This is an ugly use of X functions.	GTK doesn't offer an alternative.
 */
    static void
setup_save_yourself(void)
{
    Atom    *existing_atoms = NULL;
    int	    count = 0;

#ifdef USE_XSMP
    if (xsmp_icefd != -1)
    {
	/*
	 * Use XSMP is preference to legacy WM_SAVE_YOURSELF;
	 * set up GTK IO monitor
	 */
	GIOChannel *g_io = g_io_channel_unix_new(xsmp_icefd);

	g_io_add_watch(g_io, G_IO_IN | G_IO_ERR | G_IO_HUP,
				  local_xsmp_handle_requests, (gpointer)g_io);
    }
    else
#endif
    {
	/* Fall back to old method */

	/* first get the existing value */
	if (XGetWMProtocols(GDK_WINDOW_XDISPLAY(gui.mainwin->window),
		    GDK_WINDOW_XWINDOW(gui.mainwin->window),
		    &existing_atoms, &count))
	{
	    Atom	*new_atoms;
	    Atom	save_yourself_xatom;
	    int	i;

	    save_yourself_xatom = GET_X_ATOM(save_yourself_atom);

	    /* check if WM_SAVE_YOURSELF isn't there yet */
	    for (i = 0; i < count; ++i)
		if (existing_atoms[i] == save_yourself_xatom)
		    break;

	    if (i == count)
	    {
		/* allocate an Atoms array which is one item longer */
		new_atoms = (Atom *)alloc((unsigned)((count + 1)
							     * sizeof(Atom)));
		if (new_atoms != NULL)
		{
		    memcpy(new_atoms, existing_atoms, count * sizeof(Atom));
		    new_atoms[count] = save_yourself_xatom;
		    XSetWMProtocols(GDK_WINDOW_XDISPLAY(gui.mainwin->window),
			    GDK_WINDOW_XWINDOW(gui.mainwin->window),
			    new_atoms, count + 1);
		    vim_free(new_atoms);
		}
	    }
	    XFree(existing_atoms);
	}
    }
}

# ifdef HAVE_GTK2
/*
 * Installing a global event filter seems to be the only way to catch
 * client messages of type WM_PROTOCOLS without overriding GDK's own
 * client message event filter.  Well, that's still better than trying
 * to guess what the GDK filter had done if it had been invoked instead
 * (This is what we did for GTK+ 1.2, see below).
 *
 * GTK2_FIXME:	This doesn't seem to work.  For some reason we never
 * receive WM_SAVE_YOURSELF even though everything is set up correctly.
 * I have the nasty feeling modern session managers just don't send this
 * deprecated message anymore.	Addition: confirmed by several people.
 *
 * The GNOME session support is much cooler anyway.  Unlike this ugly
 * WM_SAVE_YOURSELF hack it actually stores the session...  And yes,
 * it should work with KDE as well.
 */
/*ARGSUSED1*/
    static GdkFilterReturn
global_event_filter(GdkXEvent *xev, GdkEvent *event, gpointer data)
{
    XEvent *xevent = (XEvent *)xev;

    if (xevent != NULL
	    && xevent->type == ClientMessage
	    && xevent->xclient.message_type == GET_X_ATOM(wm_protocols_atom)
	    && xevent->xclient.data.l[0] == GET_X_ATOM(save_yourself_atom))
    {
	out_flush();
	ml_sync_all(FALSE, FALSE); /* preserve all swap files */
	/*
	 * Set the window's WM_COMMAND property, to let the window manager
	 * know we are done saving ourselves.  We don't want to be
	 * restarted, thus set argv to NULL.
	 */
	XSetCommand(GDK_WINDOW_XDISPLAY(gui.mainwin->window),
		    GDK_WINDOW_XWINDOW(gui.mainwin->window),
		    NULL, 0);
	return GDK_FILTER_REMOVE;
    }

    return GDK_FILTER_CONTINUE;
}

# else /* !HAVE_GTK2 */

/*
 * GDK handler for X ClientMessage events.
 */
/*ARGSUSED2*/
    static GdkFilterReturn
gdk_wm_protocols_filter(GdkXEvent *xev, GdkEvent *event, gpointer data)
{
    /* From example in gdkevents.c/gdk_wm_protocols_filter */
    XEvent *xevent = (XEvent *)xev;

    if (xevent != NULL)
    {
	if (xevent->xclient.data.l[0] == GET_X_ATOM(save_yourself_atom))
	{
	    out_flush();
	    ml_sync_all(FALSE, FALSE);	    /* preserve all swap files */

	    /* Set the window's WM_COMMAND property, to let the window manager
	     * know we are done saving ourselves.  We don't want to be
	     * restarted, thus set argv to NULL. */
	    XSetCommand(GDK_WINDOW_XDISPLAY(gui.mainwin->window),
			GDK_WINDOW_XWINDOW(gui.mainwin->window),
			NULL, 0);
	}
	/*
	 * Functionality from gdkevents.c/gdk_wm_protocols_filter;
	 * Registering this filter apparently overrides the default GDK one,
	 * so we need to perform its functionality.  There seems no way to
	 * register for WM_PROTOCOLS, and only process the WM_SAVE_YOURSELF
	 * bit; it's all or nothing.  Update: No, there is a way -- but it
	 * only works with GTK+ 2 apparently.  See above.
	 */
	else if (xevent->xclient.data.l[0] == GET_X_ATOM(gdk_wm_delete_window))
	{
	    event->any.type = GDK_DELETE;
	    return GDK_FILTER_TRANSLATE;
	}
    }

    return GDK_FILTER_REMOVE;
}
# endif /* !HAVE_GTK2 */

#endif /* !(FEAT_GUI_GNOME && FEAT_SESSION) */


/*
 * Setup the window icon & xcmdsrv comm after the main window has been realized.
 */
/*ARGSUSED*/
    static void
mainwin_realize(GtkWidget *widget, gpointer data)
{
/* If you get an error message here, you still need to unpack the runtime
 * archive! */
#ifdef magick
# undef magick
#endif
#ifdef HAVE_GTK2
  /* A bit hackish, but avoids casting later and allows optimization */
# define static static const
#endif
#define magick vim32x32
#include "../runtime/vim32x32.xpm"
#undef magick
#define magick vim16x16
#include "../runtime/vim16x16.xpm"
#undef magick
#define magick vim48x48
#include "../runtime/vim48x48.xpm"
#undef magick
#ifdef HAVE_GTK2
# undef static
#endif

    /* When started with "--echo-wid" argument, write window ID on stdout. */
    if (echo_wid_arg)
    {
	printf("WID: %ld\n", (long)GDK_WINDOW_XWINDOW(gui.mainwin->window));
	fflush(stdout);
    }

    if (vim_strchr(p_go, GO_ICON) != NULL)
    {
	/*
	 * Add an icon to the main window. For fun and convenience of the user.
	 */
#ifdef HAVE_GTK2
	GList *icons = NULL;

	icons = g_list_prepend(icons, gdk_pixbuf_new_from_xpm_data(vim16x16));
	icons = g_list_prepend(icons, gdk_pixbuf_new_from_xpm_data(vim32x32));
	icons = g_list_prepend(icons, gdk_pixbuf_new_from_xpm_data(vim48x48));

	gtk_window_set_icon_list(GTK_WINDOW(gui.mainwin), icons);

	g_list_foreach(icons, (GFunc)&g_object_unref, NULL);
	g_list_free(icons);

#else /* !HAVE_GTK2 */

	GdkPixmap   *icon;
	GdkBitmap   *icon_mask = NULL;
	char	    **magick = vim32x32;
	Display	    *xdisplay;
	Window	    root_window;
	XIconSize   *size;
	int	    number_sizes;
	/*
	 * Adjust the icon to the preferences of the actual window manager.
	 * This is once again a workaround for a deficiency in GTK+ 1.2.
	 */
	xdisplay = GDK_WINDOW_XDISPLAY(gui.mainwin->window);
	root_window = XRootWindow(xdisplay, DefaultScreen(xdisplay));
	if (XGetIconSizes(xdisplay, root_window, &size, &number_sizes))
	{
	    if (number_sizes > 0)
	    {
		if (size->max_height >= 48 && size->max_height >= 48)
		    magick = vim48x48;
		else if (size->max_height >= 32 && size->max_height >= 32)
		    magick = vim32x32;
		else if (size->max_height >= 16 && size->max_height >= 16)
		    magick = vim16x16;
	    }
	    XFree(size);
	}
	icon = gdk_pixmap_create_from_xpm_d(gui.mainwin->window,
					    &icon_mask, NULL, magick);
	if (icon != NULL)
	    /* Note: for some reason gdk_window_set_icon() doesn't acquire
	     * a reference on the pixmap, thus we _have_ to leak it. */
	    gdk_window_set_icon(gui.mainwin->window, NULL, icon, icon_mask);

#endif /* !HAVE_GTK2 */
    }

#if !(defined(FEAT_GUI_GNOME) && defined(FEAT_SESSION))
    /* Register a handler for WM_SAVE_YOURSELF with GDK's low-level X I/F */
# ifdef HAVE_GTK2
    gdk_window_add_filter(NULL, &global_event_filter, NULL);
# else
    gdk_add_client_message_filter(wm_protocols_atom,
				  &gdk_wm_protocols_filter, NULL);
# endif
#endif
    /* Setup to indicate to the window manager that we want to catch the
     * WM_SAVE_YOURSELF event.	For GNOME, this connects to the session
     * manager instead. */
#if defined(FEAT_GUI_GNOME) && defined(FEAT_SESSION)
    if (using_gnome)
#endif
	setup_save_yourself();

#ifdef FEAT_CLIENTSERVER
    if (serverName == NULL && serverDelayedStartName != NULL)
    {
	/* This is a :gui command in a plain vim with no previous server */
	commWindow = GDK_WINDOW_XWINDOW(gui.mainwin->window);

	(void)serverRegisterName(GDK_WINDOW_XDISPLAY(gui.mainwin->window),
				 serverDelayedStartName);
    }
    else
    {
	/*
	 * Cannot handle "XLib-only" windows with gtk event routines, we'll
	 * have to change the "server" registration to that of the main window
	 * If we have not registered a name yet, remember the window
	 */
	serverChangeRegisteredWindow(GDK_WINDOW_XDISPLAY(gui.mainwin->window),
				     GDK_WINDOW_XWINDOW(gui.mainwin->window));
    }
    gtk_widget_add_events(gui.mainwin, GDK_PROPERTY_CHANGE_MASK);
    gtk_signal_connect(GTK_OBJECT(gui.mainwin), "property_notify_event",
		       GTK_SIGNAL_FUNC(property_event), NULL);
#endif
}

    static GdkCursor *
create_blank_pointer(void)
{
    GdkWindow	*root_window = NULL;
    GdkPixmap	*blank_mask;
    GdkCursor	*cursor;
    GdkColor	color = { 0, 0, 0, 0 };
    char	blank_data[] = { 0x0 };

#ifdef HAVE_GTK_MULTIHEAD
    root_window = gtk_widget_get_root_window(gui.mainwin);
#endif

    /* Create a pseudo blank pointer, which is in fact one pixel by one pixel
     * in size. */
    blank_mask = gdk_bitmap_create_from_data(root_window, blank_data, 1, 1);
    cursor = gdk_cursor_new_from_pixmap(blank_mask, blank_mask,
					&color, &color, 0, 0);
    gdk_bitmap_unref(blank_mask);

    return cursor;
}

#ifdef HAVE_GTK_MULTIHEAD
/*ARGSUSED1*/
    static void
mainwin_screen_changed_cb(GtkWidget  *widget,
			  GdkScreen  *previous_screen,
			  gpointer   data)
{
    if (!gtk_widget_has_screen(widget))
	return;

    /*
     * Recreate the invisible mouse cursor.
     */
    if (gui.blank_pointer != NULL)
	gdk_cursor_unref(gui.blank_pointer);

    gui.blank_pointer = create_blank_pointer();

    if (gui.pointer_hidden && gui.drawarea->window != NULL)
	gdk_window_set_cursor(gui.drawarea->window, gui.blank_pointer);

    /*
     * Create a new PangoContext for this screen, and initialize it
     * with the current font if necessary.
     */
    if (gui.text_context != NULL)
	g_object_unref(gui.text_context);

    gui.text_context = gtk_widget_create_pango_context(widget);
    pango_context_set_base_dir(gui.text_context, PANGO_DIRECTION_LTR);

    if (gui.norm_font != NULL)
    {
	gui_mch_init_font(p_guifont, FALSE);
	gui_set_shellsize(FALSE, FALSE, RESIZE_BOTH);
    }
}
#endif /* HAVE_GTK_MULTIHEAD */

/*
 * After the drawing area comes up, we calculate all colors and create the
 * dummy blank cursor.
 *
 * Don't try to set any VIM scrollbar sizes anywhere here. I'm relying on the
 * fact that the main VIM engine doesn't take them into account anywhere.
 */
/*ARGSUSED1*/
    static void
drawarea_realize_cb(GtkWidget *widget, gpointer data)
{
    GtkWidget *sbar;

#ifdef FEAT_XIM
    xim_init();
#endif
    gui_mch_new_colors();
    gui.text_gc = gdk_gc_new(gui.drawarea->window);

    gui.blank_pointer = create_blank_pointer();
    if (gui.pointer_hidden)
	gdk_window_set_cursor(widget->window, gui.blank_pointer);

    /* get the actual size of the scrollbars, if they are realized */
    sbar = firstwin->w_scrollbars[SBAR_LEFT].id;
    if (!sbar || (!gui.which_scrollbars[SBAR_LEFT]
				    && firstwin->w_scrollbars[SBAR_RIGHT].id))
	sbar = firstwin->w_scrollbars[SBAR_RIGHT].id;
    if (sbar && GTK_WIDGET_REALIZED(sbar) && sbar->allocation.width)
	gui.scrollbar_width = sbar->allocation.width;

    sbar = gui.bottom_sbar.id;
    if (sbar && GTK_WIDGET_REALIZED(sbar) && sbar->allocation.height)
	gui.scrollbar_height = sbar->allocation.height;
}

/*
 * Properly clean up on shutdown.
 */
/*ARGSUSED0*/
    static void
drawarea_unrealize_cb(GtkWidget *widget, gpointer data)
{
    /* Don't write messages to the GUI anymore */
    full_screen = FALSE;

#ifdef FEAT_XIM
    im_shutdown();
#endif
#ifdef HAVE_GTK2
    if (gui.ascii_glyphs != NULL)
    {
	pango_glyph_string_free(gui.ascii_glyphs);
	gui.ascii_glyphs = NULL;
    }
    if (gui.ascii_font != NULL)
    {
	g_object_unref(gui.ascii_font);
	gui.ascii_font = NULL;
    }
    g_object_unref(gui.text_context);
    gui.text_context = NULL;

    g_object_unref(gui.text_gc);
    gui.text_gc = NULL;

    gdk_cursor_unref(gui.blank_pointer);
    gui.blank_pointer = NULL;
#else
    gdk_gc_unref(gui.text_gc);
    gui.text_gc = NULL;

    gdk_cursor_destroy(gui.blank_pointer);
    gui.blank_pointer = NULL;
#endif
}

/*ARGSUSED0*/
    static void
drawarea_style_set_cb(GtkWidget	*widget,
		      GtkStyle	*previous_style,
		      gpointer	data)
{
    gui_mch_new_colors();
}

/*
 * Callback routine for the "delete_event" signal on the toplevel window.
 * Tries to vim gracefully, or refuses to exit with changed buffers.
 */
/*ARGSUSED*/
    static gint
delete_event_cb(GtkWidget *widget, GdkEventAny *event, gpointer data)
{
    gui_shell_closed();
    return TRUE;
}

#if defined(FEAT_MENU) || defined(FEAT_TOOLBAR) || defined(FEAT_GUI_TABLINE)
    static int
get_item_dimensions(GtkWidget *widget, GtkOrientation orientation)
{
    GtkOrientation item_orientation = GTK_ORIENTATION_HORIZONTAL;

#ifdef FEAT_GUI_GNOME
    if (using_gnome && widget != NULL)
    {
# ifdef HAVE_GTK2
	GtkWidget *parent;
	BonoboDockItem *dockitem;

	parent	 = gtk_widget_get_parent(widget);
	if (G_TYPE_FROM_INSTANCE(parent) == BONOBO_TYPE_DOCK_ITEM)
	{
	    /* Only menu & toolbar are dock items.  Could tabline be?
	     * Seem to be only the 2 defined in GNOME */
	    widget = parent;
	    dockitem = BONOBO_DOCK_ITEM(widget);

	    if (dockitem == NULL || dockitem->is_floating)
		return 0;
	    item_orientation = bonobo_dock_item_get_orientation(dockitem);
	}
# else
	GnomeDockItem *dockitem;

	widget	 = widget->parent;
	dockitem = GNOME_DOCK_ITEM(widget);

	if (dockitem == NULL || dockitem->is_floating)
	    return 0;
	item_orientation = gnome_dock_item_get_orientation(dockitem);
# endif
    }
#endif
    if (widget != NULL
	    && item_orientation == orientation
	    && GTK_WIDGET_REALIZED(widget)
	    && GTK_WIDGET_VISIBLE(widget))
    {
	if (orientation == GTK_ORIENTATION_HORIZONTAL)
	    return widget->allocation.height;
	else
	    return widget->allocation.width;
    }
    return 0;
}
#endif

    static int
get_menu_tool_width(void)
{
    int width = 0;

#ifdef FEAT_GUI_GNOME /* these are never vertical without GNOME */
# ifdef FEAT_MENU
    width += get_item_dimensions(gui.menubar, GTK_ORIENTATION_VERTICAL);
# endif
# ifdef FEAT_TOOLBAR
    width += get_item_dimensions(gui.toolbar, GTK_ORIENTATION_VERTICAL);
# endif
# ifdef FEAT_GUI_TABLINE
    if (gui.tabline != NULL)
	width += get_item_dimensions(gui.tabline, GTK_ORIENTATION_VERTICAL);
# endif
#endif

    return width;
}

    static int
get_menu_tool_height(void)
{
    int height = 0;

#ifdef FEAT_MENU
    height += get_item_dimensions(gui.menubar, GTK_ORIENTATION_HORIZONTAL);
#endif
#ifdef FEAT_TOOLBAR
    height += get_item_dimensions(gui.toolbar, GTK_ORIENTATION_HORIZONTAL);
#endif
#ifdef FEAT_GUI_TABLINE
    if (gui.tabline != NULL)
	height += get_item_dimensions(gui.tabline, GTK_ORIENTATION_HORIZONTAL);
#endif

    return height;
}

/* This controls whether we can set the real window hints at
 * start-up when in a GtkPlug.
 * 0 = normal processing (default)
 * 1 = init. hints set, no-one's tried to reset since last check
 * 2 = init. hints set, attempt made to change hints
 */
static int init_window_hints_state = 0;

    static void
update_window_manager_hints(int force_width, int force_height)
{
    static int old_width  = 0;
    static int old_height = 0;
    static int old_min_width  = 0;
    static int old_min_height = 0;
    static int old_char_width  = 0;
    static int old_char_height = 0;

    int width;
    int height;
    int min_width;
    int min_height;

    /* At start-up, don't try to set the hints until the initial
     * values have been used (those that dictate our initial size)
     * Let forced (i.e., correct) values thruogh always.
     */
    if (!(force_width && force_height)  &&  init_window_hints_state > 0)
    {
	/* Don't do it! */
	init_window_hints_state = 2;
	return;
    }

    /* This also needs to be done when the main window isn't there yet,
     * otherwise the hints don't work. */
    width  = gui_get_base_width();
    height = gui_get_base_height();
# ifdef FEAT_MENU
    height += tabline_height() * gui.char_height;
# endif
# ifdef HAVE_GTK2
    width  += get_menu_tool_width();
    height += get_menu_tool_height();
# endif

    /* GtkSockets use GtkPlug's [gui,mainwin] min-size hints to determine
     * their actual widget size.  When we set our size ourselves (e.g.,
     * 'set columns=' or init. -geom) we briefly set the min. to the size
     * we wish to be instead of the legitimate minimum so that we actually
     * resize correctly.
     */
    if (force_width && force_height)
    {
	min_width  = force_width;
	min_height = force_height;
    }
    else
    {
	min_width  = width  + MIN_COLUMNS * gui.char_width;
	min_height = height + MIN_LINES   * gui.char_height;
    }

    /* Avoid an expose event when the size didn't change. */
    if (width != old_width
	    || height != old_height
	    || min_width != old_min_width
	    || min_height != old_min_height
	    || gui.char_width != old_char_width
	    || gui.char_height != old_char_height)
    {
	GdkGeometry	geometry;
	GdkWindowHints	geometry_mask;

	geometry.width_inc   = gui.char_width;
	geometry.height_inc  = gui.char_height;
	geometry.base_width  = width;
	geometry.base_height = height;
	geometry.min_width   = min_width;
	geometry.min_height  = min_height;
	geometry_mask	     = GDK_HINT_BASE_SIZE|GDK_HINT_RESIZE_INC
			       |GDK_HINT_MIN_SIZE;
# ifdef HAVE_GTK2
	/* Using gui.formwin as geometry widget doesn't work as expected
	 * with GTK+ 2 -- dunno why.  Presumably all the resizing hacks
	 * in Vim confuse GTK+. */
	gtk_window_set_geometry_hints(GTK_WINDOW(gui.mainwin), gui.mainwin,
				      &geometry, geometry_mask);
# else
	gtk_window_set_geometry_hints(GTK_WINDOW(gui.mainwin), gui.formwin,
				      &geometry, geometry_mask);
# endif
	old_width       = width;
	old_height      = height;
	old_min_width   = min_width;
	old_min_height  = min_height;
	old_char_width  = gui.char_width;
	old_char_height = gui.char_height;
    }
}

#ifdef FEAT_TOOLBAR

# ifdef HAVE_GTK2
/*
 * This extra effort wouldn't be necessary if we only used stock icons in the
 * toolbar, as we do for all builtin icons.  But user-defined toolbar icons
 * shouldn't be treated differently, thus we do need this.
 */
    static void
icon_size_changed_foreach(GtkWidget *widget, gpointer user_data)
{
    if (GTK_IS_IMAGE(widget))
    {
	GtkImage *image = (GtkImage *)widget;

	/* User-defined icons are stored in a GtkIconSet */
	if (gtk_image_get_storage_type(image) == GTK_IMAGE_ICON_SET)
	{
	    GtkIconSet	*icon_set;
	    GtkIconSize	icon_size;

	    gtk_image_get_icon_set(image, &icon_set, &icon_size);
	    icon_size = (GtkIconSize)(long)user_data;

	    gtk_icon_set_ref(icon_set);
	    gtk_image_set_from_icon_set(image, icon_set, icon_size);
	    gtk_icon_set_unref(icon_set);
	}
    }
    else if (GTK_IS_CONTAINER(widget))
    {
	gtk_container_foreach((GtkContainer *)widget,
			      &icon_size_changed_foreach,
			      user_data);
    }
}
# endif /* HAVE_GTK2 */

    static void
set_toolbar_style(GtkToolbar *toolbar)
{
    GtkToolbarStyle style;
# ifdef HAVE_GTK2
    GtkIconSize	    size;
    GtkIconSize	    oldsize;
# endif

# ifdef HAVE_GTK2
    if ((toolbar_flags & (TOOLBAR_TEXT | TOOLBAR_ICONS | TOOLBAR_HORIZ))
		      == (TOOLBAR_TEXT | TOOLBAR_ICONS | TOOLBAR_HORIZ))
	style = GTK_TOOLBAR_BOTH_HORIZ;
    else
# endif
    if ((toolbar_flags & (TOOLBAR_TEXT | TOOLBAR_ICONS))
		      == (TOOLBAR_TEXT | TOOLBAR_ICONS))
	style = GTK_TOOLBAR_BOTH;
    else if (toolbar_flags & TOOLBAR_TEXT)
	style = GTK_TOOLBAR_TEXT;
    else
	style = GTK_TOOLBAR_ICONS;

    gtk_toolbar_set_style(toolbar, style);
    gtk_toolbar_set_tooltips(toolbar, (toolbar_flags & TOOLBAR_TOOLTIPS) != 0);

# ifdef HAVE_GTK2
    switch (tbis_flags)
    {
	case TBIS_TINY:	    size = GTK_ICON_SIZE_MENU;		break;
	case TBIS_SMALL:    size = GTK_ICON_SIZE_SMALL_TOOLBAR;	break;
	case TBIS_MEDIUM:   size = GTK_ICON_SIZE_BUTTON;	break;
	case TBIS_LARGE:    size = GTK_ICON_SIZE_LARGE_TOOLBAR;	break;
	default:	    size = GTK_ICON_SIZE_INVALID;	break;
    }
    oldsize = gtk_toolbar_get_icon_size(toolbar);

    if (size == GTK_ICON_SIZE_INVALID)
    {
	/* Let global user preferences decide the icon size. */
	gtk_toolbar_unset_icon_size(toolbar);
	size = gtk_toolbar_get_icon_size(toolbar);
    }
    if (size != oldsize)
    {
	gtk_container_foreach(GTK_CONTAINER(toolbar),
			      &icon_size_changed_foreach,
			      GINT_TO_POINTER((int)size));
    }
    gtk_toolbar_set_icon_size(toolbar, size);
# endif
}

#endif /* FEAT_TOOLBAR */

#if defined(FEAT_GUI_TABLINE) || defined(PROTO)
static int ignore_tabline_evt = FALSE;
static GtkWidget *tabline_menu;
static GtkTooltips *tabline_tooltip;
static int clicked_page;	    /* page clicked in tab line */

/*
 * Handle selecting an item in the tab line popup menu.
 */
/*ARGSUSED*/
    static void
tabline_menu_handler(GtkMenuItem *item, gpointer user_data)
{
    /* Add the string cmd into input buffer */
    send_tabline_menu_event(clicked_page, (int)(long)user_data);

    if (gtk_main_level() > 0)
	gtk_main_quit();
}

    static void
add_tabline_menu_item(GtkWidget *menu, char_u *text, int resp)
{
    GtkWidget	*item;
    char_u	*utf_text;

    utf_text = CONVERT_TO_UTF8(text);
    item = gtk_menu_item_new_with_label((const char *)utf_text);
    gtk_widget_show(item);
    CONVERT_TO_UTF8_FREE(utf_text);

    gtk_container_add(GTK_CONTAINER(menu), item);
    gtk_signal_connect(GTK_OBJECT(item), "activate",
	    GTK_SIGNAL_FUNC(tabline_menu_handler),
	    (gpointer)(long)resp);
}

/*
 * Create a menu for the tab line.
 */
    static GtkWidget *
create_tabline_menu(void)
{
    GtkWidget *menu;

    menu = gtk_menu_new();
    add_tabline_menu_item(menu, (char_u *)_("Close"), TABLINE_MENU_CLOSE);
    add_tabline_menu_item(menu, (char_u *)_("New tab"), TABLINE_MENU_NEW);
    add_tabline_menu_item(menu, (char_u *)_("Open Tab..."), TABLINE_MENU_OPEN);

    return menu;
}

    static gboolean
on_tabline_menu(GtkWidget *widget, GdkEvent *event)
{
    /* Was this button press event ? */
    if (event->type == GDK_BUTTON_PRESS)
    {
	GdkEventButton *bevent = (GdkEventButton *)event;
	int		x = bevent->x;
	int		y = bevent->y;
	GtkWidget	*tabwidget;
	GdkWindow	*tabwin;

	/* When ignoring events return TRUE so that the selected page doesn't
	 * change. */
	if (hold_gui_events
# ifdef FEAT_CMDWIN
		|| cmdwin_type != 0
# endif
	   )
	    return TRUE;

	tabwin = gdk_window_at_pointer(&x, &y);
	gdk_window_get_user_data(tabwin, (gpointer)&tabwidget);
	clicked_page = (int)(long)gtk_object_get_user_data(
						       GTK_OBJECT(tabwidget));

	/* If the event was generated for 3rd button popup the menu. */
	if (bevent->button == 3)
	{
	    gtk_menu_popup(GTK_MENU(widget), NULL, NULL, NULL, NULL,
						bevent->button, bevent->time);
	    /* We handled the event. */
	    return TRUE;
	}
	else if (bevent->button == 1)
	{
	    if (clicked_page == 0)
	    {
		/* Click after all tabs moves to next tab page.  When "x" is
		 * small guess it's the left button. */
		if (send_tabline_event(x < 50 ? -1 : 0) && gtk_main_level() > 0)
		    gtk_main_quit();
	    }
#ifndef HAVE_GTK2
	    else
		gtk_notebook_set_page(GTK_NOTEBOOK(gui.tabline),
							    clicked_page - 1);
#endif
	}
    }

    /* We didn't handle the event. */
    return FALSE;
}

/*
 * Handle selecting one of the tabs.
 */
/*ARGSUSED*/
    static void
on_select_tab(
	GtkNotebook	*notebook,
	GtkNotebookPage *page,
	gint		idx,
	gpointer	data)
{
    if (!ignore_tabline_evt)
    {
	if (send_tabline_event(idx + 1) && gtk_main_level() > 0)
	    gtk_main_quit();
    }
}

#ifndef HAVE_GTK2
static int showing_tabline = 0;
#endif

/*
 * Show or hide the tabline.
 */
    void
gui_mch_show_tabline(int showit)
{
    if (gui.tabline == NULL)
	return;

#ifdef HAVE_GTK2
    /* gtk_notebook_get_show_tabs does not exist in gtk+-1.2.10 */
    if (!showit != !gtk_notebook_get_show_tabs(GTK_NOTEBOOK(gui.tabline)))
#else
    if (!showit != !showing_tabline)
#endif
    {
	/* Note: this may cause a resize event */
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(gui.tabline), showit);
	update_window_manager_hints(0, 0);
#ifndef HAVE_GTK2
	showing_tabline = showit;
#endif
	if (showit)
	    GTK_WIDGET_UNSET_FLAGS(GTK_WIDGET(gui.tabline), GTK_CAN_FOCUS);
    }

    gui_mch_update();
}

/*
 * Return TRUE when tabline is displayed.
 */
    int
gui_mch_showing_tabline(void)
{
    return gui.tabline != NULL
#ifdef HAVE_GTK2
	    /* gtk_notebook_get_show_tabs does not exist in gtk+-1.2.10 */
		     && gtk_notebook_get_show_tabs(GTK_NOTEBOOK(gui.tabline))
#else
		     && showing_tabline
#endif
		     ;
}

/*
 * Update the labels of the tabline.
 */
    void
gui_mch_update_tabline(void)
{
    GtkWidget	    *page;
    GtkWidget	    *event_box;
    GtkWidget	    *label;
    tabpage_T	    *tp;
    int		    nr = 0;
    int		    tab_num;
    int		    curtabidx = 0;
    char_u	    *labeltext;

    if (gui.tabline == NULL)
	return;

    ignore_tabline_evt = TRUE;

    /* Add a label for each tab page.  They all contain the same text area. */
    for (tp = first_tabpage; tp != NULL; tp = tp->tp_next, ++nr)
    {
	if (tp == curtab)
	    curtabidx = nr;

	tab_num = nr + 1;

	page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(gui.tabline), nr);
	if (page == NULL)
	{
	    /* Add notebook page */
	    page = gtk_vbox_new(FALSE, 0);
	    gtk_widget_show(page);
	    event_box = gtk_event_box_new();
	    gtk_widget_show(event_box);
	    label = gtk_label_new("-Empty-");
	    gtk_misc_set_padding(GTK_MISC(label), 2, 2);
	    gtk_container_add(GTK_CONTAINER(event_box), label);
	    gtk_widget_show(label);
	    gtk_notebook_insert_page(GTK_NOTEBOOK(gui.tabline),
		    page,
		    event_box,
		    nr++);
	}

	event_box = gtk_notebook_get_tab_label(GTK_NOTEBOOK(gui.tabline), page);
	gtk_object_set_user_data(GTK_OBJECT(event_box),
						     (gpointer)(long)tab_num);
	label = GTK_BIN(event_box)->child;
	get_tabline_label(tp, FALSE);
	labeltext = CONVERT_TO_UTF8(NameBuff);
	gtk_label_set_text(GTK_LABEL(label), (const char *)labeltext);
	CONVERT_TO_UTF8_FREE(labeltext);

	get_tabline_label(tp, TRUE);
	labeltext = CONVERT_TO_UTF8(NameBuff);
	gtk_tooltips_set_tip(GTK_TOOLTIPS(tabline_tooltip), event_box,
			     (const char *)labeltext, NULL);
	CONVERT_TO_UTF8_FREE(labeltext);
    }

    /* Remove any old labels. */
    while (gtk_notebook_get_nth_page(GTK_NOTEBOOK(gui.tabline), nr) != NULL)
	gtk_notebook_remove_page(GTK_NOTEBOOK(gui.tabline), nr);

    if (gtk_notebook_current_page(GTK_NOTEBOOK(gui.tabline)) != curtabidx)
	gtk_notebook_set_page(GTK_NOTEBOOK(gui.tabline), curtabidx);

    /* Make sure everything is in place before drawing text. */
    gui_mch_update();

    ignore_tabline_evt = FALSE;
}

/*
 * Set the current tab to "nr".  First tab is 1.
 */
    void
gui_mch_set_curtab(nr)
    int		nr;
{
    if (gui.tabline == NULL)
	return;

    ignore_tabline_evt = TRUE;
    if (gtk_notebook_current_page(GTK_NOTEBOOK(gui.tabline)) != nr - 1)
	gtk_notebook_set_page(GTK_NOTEBOOK(gui.tabline), nr - 1);
    ignore_tabline_evt = FALSE;
}

#endif /* FEAT_GUI_TABLINE */

/*
 * Initialize the GUI.	Create all the windows, set up all the callbacks etc.
 * Returns OK for success, FAIL when the GUI can't be started.
 */
    int
gui_mch_init(void)
{
    GtkWidget *vbox;

#ifdef FEAT_GUI_GNOME
    /* Initialize the GNOME libraries.	gnome_program_init()/gnome_init()
     * exits on failure, but that's a non-issue because we already called
     * gtk_init_check() in gui_mch_init_check(). */
    if (using_gnome)
# ifdef HAVE_GTK2
	gnome_program_init(VIMPACKAGE, VIM_VERSION_SHORT,
			   LIBGNOMEUI_MODULE, gui_argc, gui_argv, NULL);
# else
	gnome_init(VIMPACKAGE, VIM_VERSION_SHORT, gui_argc, gui_argv);
# endif
#endif
    vim_free(gui_argv);
    gui_argv = NULL;

#ifdef HAVE_GTK2
# if GLIB_CHECK_VERSION(2,1,3)
    /* Set the human-readable application name */
    g_set_application_name("Vim");
# endif
    /*
     * Force UTF-8 output no matter what the value of 'encoding' is.
     * did_set_string_option() in option.c prohibits changing 'termencoding'
     * to something else than UTF-8 if the GUI is in use.
     */
    set_option_value((char_u *)"termencoding", 0L, (char_u *)"utf-8", 0);

# ifdef FEAT_TOOLBAR
    gui_gtk_register_stock_icons();
# endif
    /* FIXME: Need to install the classic icons and a gtkrc.classic file.
     * The hard part is deciding install locations and the Makefile magic. */
# if 0
    gtk_rc_parse("gtkrc");
# endif
#endif

    /* Initialize values */
    gui.border_width = 2;
    gui.scrollbar_width = SB_DEFAULT_WIDTH;
    gui.scrollbar_height = SB_DEFAULT_WIDTH;
    /* LINTED: avoid warning: conversion to 'unsigned long' */
    gui.fgcolor = g_new0(GdkColor, 1);
    /* LINTED: avoid warning: conversion to 'unsigned long' */
    gui.bgcolor = g_new0(GdkColor, 1);
    /* LINTED: avoid warning: conversion to 'unsigned long' */
    gui.spcolor = g_new0(GdkColor, 1);

    /* Initialise atoms */
#ifdef FEAT_MBYTE
    utf8_string_atom = gdk_atom_intern("UTF8_STRING", FALSE);
#endif
#ifndef HAVE_GTK2
    compound_text_atom = gdk_atom_intern("COMPOUND_TEXT", FALSE);
    text_atom = gdk_atom_intern("TEXT", FALSE);
#endif

    /* Set default foreground and background colors. */
    gui.norm_pixel = gui.def_norm_pixel;
    gui.back_pixel = gui.def_back_pixel;

    if (gtk_socket_id != 0)
    {
	GtkWidget *plug;

	/* Use GtkSocket from another app. */
#ifdef HAVE_GTK_MULTIHEAD
	plug = gtk_plug_new_for_display(gdk_display_get_default(),
					gtk_socket_id);
#else
	plug = gtk_plug_new(gtk_socket_id);
#endif
	if (plug != NULL && GTK_PLUG(plug)->socket_window != NULL)
	{
	    gui.mainwin = plug;
	}
	else
	{
	    g_warning("Connection to GTK+ socket (ID %u) failed",
		      (unsigned int)gtk_socket_id);
	    /* Pretend we never wanted it if it failed (get own window) */
	    gtk_socket_id = 0;
	}
    }

    if (gtk_socket_id == 0)
    {
#ifdef FEAT_GUI_GNOME
	if (using_gnome)
	{
	    gui.mainwin = gnome_app_new("Vim", NULL);
# ifdef USE_XSMP
	    /* Use the GNOME save-yourself functionality now. */
	    xsmp_close();
# endif
	}
	else
#endif
	    gui.mainwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    }

    gtk_widget_set_name(gui.mainwin, "vim-main-window");

#ifdef HAVE_GTK2
    /* Create the PangoContext used for drawing all text. */
    gui.text_context = gtk_widget_create_pango_context(gui.mainwin);
    pango_context_set_base_dir(gui.text_context, PANGO_DIRECTION_LTR);
#endif

#ifndef HAVE_GTK2
    gtk_window_set_policy(GTK_WINDOW(gui.mainwin), TRUE, TRUE, TRUE);
#endif
    gtk_container_border_width(GTK_CONTAINER(gui.mainwin), 0);
    gtk_widget_add_events(gui.mainwin, GDK_VISIBILITY_NOTIFY_MASK);

    gtk_signal_connect(GTK_OBJECT(gui.mainwin), "delete_event",
		       GTK_SIGNAL_FUNC(&delete_event_cb), NULL);

    gtk_signal_connect(GTK_OBJECT(gui.mainwin), "realize",
		       GTK_SIGNAL_FUNC(&mainwin_realize), NULL);
#ifdef HAVE_GTK_MULTIHEAD
    g_signal_connect(G_OBJECT(gui.mainwin), "screen_changed",
		     G_CALLBACK(&mainwin_screen_changed_cb), NULL);
#endif
#ifdef HAVE_GTK2
    gui.accel_group = gtk_accel_group_new();
    gtk_window_add_accel_group(GTK_WINDOW(gui.mainwin), gui.accel_group);
#else
    gui.accel_group = gtk_accel_group_get_default();
#endif

    /* A vertical box holds the menubar, toolbar and main text window. */
    vbox = gtk_vbox_new(FALSE, 0);

#ifdef FEAT_GUI_GNOME
    if (using_gnome)
    {
# if defined(HAVE_GTK2) && defined(FEAT_MENU)
	/* automagically restore menubar/toolbar placement */
	gnome_app_enable_layout_config(GNOME_APP(gui.mainwin), TRUE);
# endif
	gnome_app_set_contents(GNOME_APP(gui.mainwin), vbox);
    }
    else
#endif
    {
	gtk_container_add(GTK_CONTAINER(gui.mainwin), vbox);
	gtk_widget_show(vbox);
    }

#ifdef FEAT_MENU
    /*
     * Create the menubar and handle
     */
    gui.menubar = gtk_menu_bar_new();
    gtk_widget_set_name(gui.menubar, "vim-menubar");

# ifdef HAVE_GTK2
    /* Avoid that GTK takes <F10> away from us. */
    {
	GtkSettings *gtk_settings;

	gtk_settings = gtk_settings_get_for_screen(gdk_screen_get_default());
	g_object_set(gtk_settings, "gtk-menu-bar-accel", NULL, NULL);
    }
# endif


# ifdef FEAT_GUI_GNOME
    if (using_gnome)
    {
#  ifdef HAVE_GTK2
	BonoboDockItem *dockitem;

	gnome_app_set_menus(GNOME_APP(gui.mainwin), GTK_MENU_BAR(gui.menubar));
	dockitem = gnome_app_get_dock_item_by_name(GNOME_APP(gui.mainwin),
						   GNOME_APP_MENUBAR_NAME);
	/* We don't want the menu to float. */
	bonobo_dock_item_set_behavior(dockitem,
		bonobo_dock_item_get_behavior(dockitem)
				       | BONOBO_DOCK_ITEM_BEH_NEVER_FLOATING);
	gui.menubar_h = GTK_WIDGET(dockitem);
#  else
	gui.menubar_h = gnome_dock_item_new("VimMainMenu",
					    GNOME_DOCK_ITEM_BEH_EXCLUSIVE |
					    GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL);
	gtk_container_add(GTK_CONTAINER(gui.menubar_h), gui.menubar);

	gnome_dock_add_item(GNOME_DOCK(GNOME_APP(gui.mainwin)->dock),
			    GNOME_DOCK_ITEM(gui.menubar_h),
			    GNOME_DOCK_TOP, /* placement */
			    1,	/* band_num */
			    0,	/* band_position */
			    0,	/* offset */
			    TRUE);
	gtk_widget_show(gui.menubar);
#  endif
    }
    else
# endif	/* FEAT_GUI_GNOME */
    {
	/* Always show the menubar, otherwise <F10> doesn't work.  It may be
	 * disabled in gui_init() later. */
	gtk_widget_show(gui.menubar);
	gtk_box_pack_start(GTK_BOX(vbox), gui.menubar, FALSE, FALSE, 0);
    }
#endif	/* FEAT_MENU */

#ifdef FEAT_TOOLBAR
    /*
     * Create the toolbar and handle
     */
# ifdef HAVE_GTK2
    /* some aesthetics on the toolbar */
    gtk_rc_parse_string(
	    "style \"vim-toolbar-style\" {\n"
	    "  GtkToolbar::button_relief = GTK_RELIEF_NONE\n"
	    "}\n"
	    "widget \"*.vim-toolbar\" style \"vim-toolbar-style\"\n");
    gui.toolbar = gtk_toolbar_new();
    gtk_widget_set_name(gui.toolbar, "vim-toolbar");
# else
    gui.toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,
				  GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_button_relief(GTK_TOOLBAR(gui.toolbar), GTK_RELIEF_NONE);
# endif
    set_toolbar_style(GTK_TOOLBAR(gui.toolbar));

# ifdef FEAT_GUI_GNOME
    if (using_gnome)
    {
#  ifdef HAVE_GTK2
	BonoboDockItem *dockitem;

	gnome_app_set_toolbar(GNOME_APP(gui.mainwin), GTK_TOOLBAR(gui.toolbar));
	dockitem = gnome_app_get_dock_item_by_name(GNOME_APP(gui.mainwin),
						   GNOME_APP_TOOLBAR_NAME);
	gui.toolbar_h = GTK_WIDGET(dockitem);
	/* When the toolbar is floating it gets stuck.  So long as that isn't
	 * fixed let's disallow floating. */
	bonobo_dock_item_set_behavior(dockitem,
		bonobo_dock_item_get_behavior(dockitem)
				       | BONOBO_DOCK_ITEM_BEH_NEVER_FLOATING);
	gtk_container_set_border_width(GTK_CONTAINER(gui.toolbar), 0);
#  else
	GtkWidget *dockitem;

	dockitem = gnome_dock_item_new("VimToolBar",
				       GNOME_DOCK_ITEM_BEH_EXCLUSIVE);
	gtk_container_add(GTK_CONTAINER(dockitem), GTK_WIDGET(gui.toolbar));
	gui.toolbar_h = dockitem;

	gnome_dock_add_item(GNOME_DOCK(GNOME_APP(gui.mainwin)->dock),
			    GNOME_DOCK_ITEM(dockitem),
			    GNOME_DOCK_TOP,	/* placement */
			    1,	/* band_num */
			    1,	/* band_position */
			    0,	/* offset */
			    TRUE);
	gtk_container_border_width(GTK_CONTAINER(gui.toolbar), 2);
	gtk_widget_show(gui.toolbar);
#  endif
    }
    else
# endif	/* FEAT_GUI_GNOME */
    {
# ifndef HAVE_GTK2
	gtk_container_border_width(GTK_CONTAINER(gui.toolbar), 1);
# endif
	if (vim_strchr(p_go, GO_TOOLBAR) != NULL
		&& (toolbar_flags & (TOOLBAR_TEXT | TOOLBAR_ICONS)))
	    gtk_widget_show(gui.toolbar);
	gtk_box_pack_start(GTK_BOX(vbox), gui.toolbar, FALSE, FALSE, 0);
    }
#endif /* FEAT_TOOLBAR */

#ifdef FEAT_GUI_TABLINE
    /*
     * Use a Notebook for the tab pages labels.  The labels are hidden by
     * default.
     */
    gui.tabline = gtk_notebook_new();
    gtk_widget_show(gui.tabline);
    gtk_box_pack_start(GTK_BOX(vbox), gui.tabline, FALSE, FALSE, 0);
    gtk_notebook_set_show_border(GTK_NOTEBOOK(gui.tabline), FALSE);
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(gui.tabline), FALSE);
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(gui.tabline), TRUE);
    gtk_notebook_set_tab_border(GTK_NOTEBOOK(gui.tabline), FALSE);

    tabline_tooltip = gtk_tooltips_new();
    gtk_tooltips_enable(GTK_TOOLTIPS(tabline_tooltip));

    {
	GtkWidget *page, *label, *event_box;

	/* Add the first tab. */
	page = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(page);
	gtk_container_add(GTK_CONTAINER(gui.tabline), page);
	label = gtk_label_new("-Empty-");
	gtk_widget_show(label);
	event_box = gtk_event_box_new();
	gtk_widget_show(event_box);
	gtk_object_set_user_data(GTK_OBJECT(event_box), (gpointer)1L);
	gtk_misc_set_padding(GTK_MISC(label), 2, 2);
	gtk_container_add(GTK_CONTAINER(event_box), label);
	gtk_notebook_set_tab_label(GTK_NOTEBOOK(gui.tabline), page, event_box);
    }

    gtk_signal_connect(GTK_OBJECT(gui.tabline), "switch_page",
		       GTK_SIGNAL_FUNC(on_select_tab), NULL);

    /* Create a popup menu for the tab line and connect it. */
    tabline_menu = create_tabline_menu();
    gtk_signal_connect_object(GTK_OBJECT(gui.tabline), "button_press_event",
	    GTK_SIGNAL_FUNC(on_tabline_menu), GTK_OBJECT(tabline_menu));
#endif

    gui.formwin = gtk_form_new();
    gtk_container_border_width(GTK_CONTAINER(gui.formwin), 0);
    gtk_widget_set_events(gui.formwin, GDK_EXPOSURE_MASK);

    gui.drawarea = gtk_drawing_area_new();

    /* Determine which events we will filter. */
    gtk_widget_set_events(gui.drawarea,
			  GDK_EXPOSURE_MASK |
			  GDK_ENTER_NOTIFY_MASK |
			  GDK_LEAVE_NOTIFY_MASK |
			  GDK_BUTTON_PRESS_MASK |
			  GDK_BUTTON_RELEASE_MASK |
#ifdef HAVE_GTK2
			  GDK_SCROLL_MASK |
#endif
			  GDK_KEY_PRESS_MASK |
			  GDK_KEY_RELEASE_MASK |
			  GDK_POINTER_MOTION_MASK |
			  GDK_POINTER_MOTION_HINT_MASK);

    gtk_widget_show(gui.drawarea);
    gtk_form_put(GTK_FORM(gui.formwin), gui.drawarea, 0, 0);
    gtk_widget_show(gui.formwin);
    gtk_box_pack_start(GTK_BOX(vbox), gui.formwin, TRUE, TRUE, 0);

    /* For GtkSockets, key-presses must go to the focus widget (drawarea)
     * and not the window. */
    gtk_signal_connect((gtk_socket_id == 0) ? GTK_OBJECT(gui.mainwin)
					    : GTK_OBJECT(gui.drawarea),
		       "key_press_event",
		       GTK_SIGNAL_FUNC(key_press_event), NULL);
#if defined(FEAT_XIM) && defined(HAVE_GTK2)
    /* Also forward key release events for the benefit of GTK+ 2 input
     * modules.  Try CTRL-SHIFT-xdigits to enter a Unicode code point. */
    g_signal_connect((gtk_socket_id == 0) ? G_OBJECT(gui.mainwin)
					  : G_OBJECT(gui.drawarea),
		     "key_release_event",
		     G_CALLBACK(&key_release_event), NULL);
#endif
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "realize",
		       GTK_SIGNAL_FUNC(drawarea_realize_cb), NULL);
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "unrealize",
		       GTK_SIGNAL_FUNC(drawarea_unrealize_cb), NULL);

    gtk_signal_connect_after(GTK_OBJECT(gui.drawarea), "style_set",
			     GTK_SIGNAL_FUNC(&drawarea_style_set_cb), NULL);

    gui.visibility = GDK_VISIBILITY_UNOBSCURED;

#if !(defined(FEAT_GUI_GNOME) && defined(FEAT_SESSION))
    wm_protocols_atom = gdk_atom_intern("WM_PROTOCOLS", FALSE);
    save_yourself_atom = gdk_atom_intern("WM_SAVE_YOURSELF", FALSE);
#endif

    if (gtk_socket_id != 0)
	/* make sure keybord input can go to the drawarea */
	GTK_WIDGET_SET_FLAGS(gui.drawarea, GTK_CAN_FOCUS);

    /*
     * Set clipboard specific atoms
     */
    vim_atom = gdk_atom_intern(VIM_ATOM_NAME, FALSE);
#ifdef FEAT_MBYTE
    vimenc_atom = gdk_atom_intern(VIMENC_ATOM_NAME, FALSE);
#endif
    clip_star.gtk_sel_atom = GDK_SELECTION_PRIMARY;
    clip_plus.gtk_sel_atom = gdk_atom_intern("CLIPBOARD", FALSE);

    /*
     * Start out by adding the configured border width into the border offset.
     */
    gui.border_offset = gui.border_width;

    gtk_signal_connect(GTK_OBJECT(gui.mainwin), "visibility_notify_event",
		       GTK_SIGNAL_FUNC(visibility_event), NULL);
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "expose_event",
		       GTK_SIGNAL_FUNC(expose_event), NULL);

    /*
     * Only install these enter/leave callbacks when 'p' in 'guioptions'.
     * Only needed for some window managers.
     */
    if (vim_strchr(p_go, GO_POINTER) != NULL)
    {
	gtk_signal_connect(GTK_OBJECT(gui.drawarea), "leave_notify_event",
			   GTK_SIGNAL_FUNC(leave_notify_event), NULL);
	gtk_signal_connect(GTK_OBJECT(gui.drawarea), "enter_notify_event",
			   GTK_SIGNAL_FUNC(enter_notify_event), NULL);
    }

    /* Real windows can get focus ... GtkPlug, being a mere container can't,
     * only its widgets.  Arguably, this could be common code and we not use
     * the window focus at all, but let's be safe.
     */
    if (gtk_socket_id == 0)
    {
	gtk_signal_connect(GTK_OBJECT(gui.mainwin), "focus_out_event",
			       GTK_SIGNAL_FUNC(focus_out_event), NULL);
	gtk_signal_connect(GTK_OBJECT(gui.mainwin), "focus_in_event",
			       GTK_SIGNAL_FUNC(focus_in_event), NULL);
    }
    else
    {
	gtk_signal_connect(GTK_OBJECT(gui.drawarea), "focus_out_event",
			       GTK_SIGNAL_FUNC(focus_out_event), NULL);
	gtk_signal_connect(GTK_OBJECT(gui.drawarea), "focus_in_event",
			       GTK_SIGNAL_FUNC(focus_in_event), NULL);
#ifdef FEAT_GUI_TABLINE
	gtk_signal_connect(GTK_OBJECT(gui.tabline), "focus_out_event",
			       GTK_SIGNAL_FUNC(focus_out_event), NULL);
	gtk_signal_connect(GTK_OBJECT(gui.tabline), "focus_in_event",
			       GTK_SIGNAL_FUNC(focus_in_event), NULL);
#endif /* FEAT_GUI_TABLINE */
    }

    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "motion_notify_event",
		       GTK_SIGNAL_FUNC(motion_notify_event), NULL);
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "button_press_event",
		       GTK_SIGNAL_FUNC(button_press_event), NULL);
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "button_release_event",
		       GTK_SIGNAL_FUNC(button_release_event), NULL);
#ifdef HAVE_GTK2
    g_signal_connect(G_OBJECT(gui.drawarea), "scroll_event",
		     G_CALLBACK(&scroll_event), NULL);
#endif

    /*
     * Add selection handler functions.
     */
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "selection_clear_event",
		       GTK_SIGNAL_FUNC(selection_clear_event), NULL);
    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "selection_received",
		       GTK_SIGNAL_FUNC(selection_received_cb), NULL);

    /*
     * Add selection targets for PRIMARY and CLIPBOARD selections.
     */
    gtk_selection_add_targets(gui.drawarea,
			      (GdkAtom)GDK_SELECTION_PRIMARY,
			      selection_targets, N_SELECTION_TARGETS);
    gtk_selection_add_targets(gui.drawarea,
			      (GdkAtom)clip_plus.gtk_sel_atom,
			      selection_targets, N_SELECTION_TARGETS);

    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "selection_get",
		       GTK_SIGNAL_FUNC(selection_get_cb), NULL);

    /* Pretend we don't have input focus, we will get an event if we do. */
    gui.in_focus = FALSE;

    return OK;
}

#if (defined(FEAT_GUI_GNOME) && defined(FEAT_SESSION)) || defined(PROTO)
/*
 * This is called from gui_start() after a fork() has been done.
 * We have to tell the session manager our new PID.
 */
    void
gui_mch_forked(void)
{
    if (using_gnome)
    {
	GnomeClient *client;

	client = gnome_master_client();

	if (client != NULL)
	    gnome_client_set_process_id(client, getpid());
    }
}
#endif /* FEAT_GUI_GNOME && FEAT_SESSION */

/*
 * Called when the foreground or background color has been changed.
 * This used to change the graphics contexts directly but we are
 * currently manipulating them where desired.
 */
    void
gui_mch_new_colors(void)
{
    if (gui.drawarea != NULL && gui.drawarea->window != NULL)
    {
	GdkColor color = { 0, 0, 0, 0 };

	color.pixel = gui.back_pixel;
	gdk_window_set_background(gui.drawarea->window, &color);
    }
}

/*
 * This signal informs us about the need to rearrange our sub-widgets.
 */
/*ARGSUSED*/
    static gint
form_configure_event(GtkWidget *widget, GdkEventConfigure *event,
		     gpointer data)
{
    int usable_height = event->height;

    /* When in a GtkPlug, we can't guarantee valid heights (as a round
     * no. of char-heights), so we have to manually sanitise them.
     * Widths seem to sort themselves out, don't ask me why.
     */
    if (gtk_socket_id != 0)
	usable_height -= (gui.char_height - (gui.char_height/2)); /* sic. */

    gtk_form_freeze(GTK_FORM(gui.formwin));
    gui_resize_shell(event->width, usable_height);
    gtk_form_thaw(GTK_FORM(gui.formwin));

    return TRUE;
}

/*
 * Function called when window already closed.
 * We can't do much more here than to trying to preserve what had been done,
 * since the window is already inevitably going away.
 */
/*ARGSUSED0*/
    static void
mainwin_destroy_cb(GtkObject *object, gpointer data)
{
    /* Don't write messages to the GUI anymore */
    full_screen = FALSE;

    gui.mainwin  = NULL;
    gui.drawarea = NULL;

    if (!exiting) /* only do anything if the destroy was unexpected */
    {
	vim_strncpy(IObuff,
		(char_u *)_("Vim: Main window unexpectedly destroyed\n"),
		IOSIZE - 1);
	preserve_exit();
    }
}


/*
 * Bit of a hack to ensure we start GtkPlug windows with the correct window
 * hints (and thus the required size from -geom), but that after that we
 * put the hints back to normal (the actual minimum size) so we may
 * subsequently be resized smaller.  GtkSocket (the parent end) uses the
 * plug's window 'min hints to set *it's* minimum size, but that's also the
 * only way we have of making ourselves bigger (by set lines/columns).
 * Thus set hints at start-up to ensure correct init. size, then a
 * second after the final attempt to reset the real minimum hinst (done by
 * scrollbar init.), actually do the standard hinst and stop the timer.
 * We'll not let the default hints be set while this timer's active.
 */
/*ARGSUSED*/
    static gboolean
check_startup_plug_hints(gpointer data)
{
    if (init_window_hints_state == 1)
    {
	/* Safe to use normal hints now */
	init_window_hints_state = 0;
	update_window_manager_hints(0, 0);
	return FALSE;   /* stop timer */
    }

    /* Keep on trying */
    init_window_hints_state = 1;
    return TRUE;
}


/*
 * Open the GUI window which was created by a call to gui_mch_init().
 */
    int
gui_mch_open(void)
{
    guicolor_T fg_pixel = INVALCOLOR;
    guicolor_T bg_pixel = INVALCOLOR;

#ifdef HAVE_GTK2
    /*
     * Allow setting a window role on the command line, or invent one
     * if none was specified.  This is mainly useful for GNOME session
     * support; allowing the WM to restore window placement.
     */
    if (role_argument != NULL)
    {
	gtk_window_set_role(GTK_WINDOW(gui.mainwin), role_argument);
    }
    else
    {
	char *role;

	/* Invent a unique-enough ID string for the role */
	role = g_strdup_printf("vim-%u-%u-%u",
			       (unsigned)mch_get_pid(),
			       (unsigned)g_random_int(),
			       (unsigned)time(NULL));

	gtk_window_set_role(GTK_WINDOW(gui.mainwin), role);
	g_free(role);
    }
#endif

    if (gui_win_x != -1 && gui_win_y != -1)
#ifdef HAVE_GTK2
	gtk_window_move(GTK_WINDOW(gui.mainwin), gui_win_x, gui_win_y);
#else
	gtk_widget_set_uposition(gui.mainwin, gui_win_x, gui_win_y);
#endif

    /* Determine user specified geometry, if present. */
    if (gui.geom != NULL)
    {
	int		mask;
	unsigned int	w, h;
	int		x = 0;
	int		y = 0;
	guint		pixel_width;
	guint		pixel_height;

	mask = XParseGeometry((char *)gui.geom, &x, &y, &w, &h);

	if (mask & WidthValue)
	    Columns = w;
	if (mask & HeightValue)
	{
	    if (p_window > h - 1 || !option_was_set((char_u *)"window"))
		p_window = h - 1;
	    Rows = h;
	}

	pixel_width = (guint)(gui_get_base_width() + Columns * gui.char_width);
	pixel_height = (guint)(gui_get_base_height() + Rows * gui.char_height);

#ifdef HAVE_GTK2
	pixel_width  += get_menu_tool_width();
	pixel_height += get_menu_tool_height();
#endif

	if (mask & (XValue | YValue))
	{
	    int ww, hh;
	    gui_mch_get_screen_dimensions(&ww, &hh);
	    hh += p_ghr + get_menu_tool_height();
	    ww += get_menu_tool_width();
	    if (mask & XNegative)
		x += ww - pixel_width;
	    if (mask & YNegative)
		y += hh - pixel_height;
#ifdef HAVE_GTK2
	    gtk_window_move(GTK_WINDOW(gui.mainwin), x, y);
#else
	    gtk_widget_set_uposition(gui.mainwin, x, y);
#endif
	}
	vim_free(gui.geom);
	gui.geom = NULL;

	/* From now until everyone's stopped trying to set the window hints
	 * to their correct minimum values, stop them being set as we need
	 * them to remain at our required size for the parent GtkSocket to
	 * give us the right initial size.
	 */
	if (gtk_socket_id != 0  &&  (mask & WidthValue || mask & HeightValue))
	{
	    update_window_manager_hints(pixel_width, pixel_height);
	    init_window_hints_state = 1;
	    g_timeout_add(1000, check_startup_plug_hints, NULL);
	}
    }

    gtk_form_set_size(GTK_FORM(gui.formwin),
	    (guint)(gui_get_base_width() + Columns * gui.char_width),
	    (guint)(gui_get_base_height() + Rows * gui.char_height));
    update_window_manager_hints(0, 0);

    if (foreground_argument != NULL)
	fg_pixel = gui_get_color((char_u *)foreground_argument);
    if (fg_pixel == INVALCOLOR)
	fg_pixel = gui_get_color((char_u *)"Black");

    if (background_argument != NULL)
	bg_pixel = gui_get_color((char_u *)background_argument);
    if (bg_pixel == INVALCOLOR)
	bg_pixel = gui_get_color((char_u *)"White");

    if (found_reverse_arg)
    {
	gui.def_norm_pixel = bg_pixel;
	gui.def_back_pixel = fg_pixel;
    }
    else
    {
	gui.def_norm_pixel = fg_pixel;
	gui.def_back_pixel = bg_pixel;
    }

    /* Get the colors from the "Normal" and "Menu" group (set in syntax.c or
     * in a vimrc file) */
    set_normal_colors();

    /* Check that none of the colors are the same as the background color */
    gui_check_colors();

    /* Get the colors for the highlight groups (gui_check_colors() might have
     * changed them). */
    highlight_gui_started();	/* re-init colors and fonts */

    gtk_signal_connect(GTK_OBJECT(gui.mainwin), "destroy",
		       GTK_SIGNAL_FUNC(mainwin_destroy_cb), NULL);

#ifdef FEAT_HANGULIN
    hangul_keyboard_set();
#endif

    /*
     * Notify the fixed area about the need to resize the contents of the
     * gui.formwin, which we use for random positioning of the included
     * components.
     *
     * We connect this signal deferred finally after anything is in place,
     * since this is intended to handle resizements coming from the window
     * manager upon us and should not interfere with what VIM is requesting
     * upon startup.
     */
    gtk_signal_connect(GTK_OBJECT(gui.formwin), "configure_event",
		       GTK_SIGNAL_FUNC(form_configure_event), NULL);

#ifdef FEAT_DND
    /*
     * Set up for receiving DND items.
     */
    gtk_drag_dest_set(gui.drawarea,
		      GTK_DEST_DEFAULT_ALL,
		      dnd_targets, N_DND_TARGETS,
		      GDK_ACTION_COPY);

    gtk_signal_connect(GTK_OBJECT(gui.drawarea), "drag_data_received",
		       GTK_SIGNAL_FUNC(drag_data_received_cb), NULL);
#endif

#ifdef HAVE_GTK2
	/* With GTK+ 2, we need to iconify the window before calling show()
	 * to avoid mapping the window for a short time.  This is just as one
	 * would expect it to work, but it's different in GTK+ 1.  The funny
	 * thing is that iconifying after show() _does_ work with GTK+ 1.
	 * (BTW doing this in the "realize" handler makes no difference.) */
	if (found_iconic_arg && gtk_socket_id == 0)
	    gui_mch_iconify();
#endif

    {
#if defined(FEAT_GUI_GNOME) && defined(HAVE_GTK2) && defined(FEAT_MENU)
	unsigned long menu_handler = 0;
# ifdef FEAT_TOOLBAR
	unsigned long tool_handler = 0;
# endif
	/*
	 * Urgh hackish :/  For some reason BonoboDockLayout always forces a
	 * show when restoring the saved layout configuration.	We can't just
	 * hide the widgets again after gtk_widget_show(gui.mainwin) since it's
	 * a toplevel window and thus will be realized immediately.  Instead,
	 * connect signal handlers to hide the widgets just after they've been
	 * marked visible, but before the main window is realized.
	 */
	if (using_gnome && vim_strchr(p_go, GO_MENUS) == NULL)
	    menu_handler = g_signal_connect_after(gui.menubar_h, "show",
						  G_CALLBACK(&gtk_widget_hide),
						  NULL);
# ifdef FEAT_TOOLBAR
	if (using_gnome && vim_strchr(p_go, GO_TOOLBAR) == NULL
		&& (toolbar_flags & (TOOLBAR_TEXT | TOOLBAR_ICONS)))
	    tool_handler = g_signal_connect_after(gui.toolbar_h, "show",
						  G_CALLBACK(&gtk_widget_hide),
						  NULL);
# endif
#endif
	gtk_widget_show(gui.mainwin);

#if defined(FEAT_GUI_GNOME) && defined(HAVE_GTK2) && defined(FEAT_MENU)
	if (menu_handler != 0)
	    g_signal_handler_disconnect(gui.menubar_h, menu_handler);
# ifdef FEAT_TOOLBAR
	if (tool_handler != 0)
	    g_signal_handler_disconnect(gui.toolbar_h, tool_handler);
# endif
#endif
    }

#ifndef HAVE_GTK2
	/* With GTK+ 1, we need to iconify the window after calling show().
	 * See the comment above for details. */
	if (found_iconic_arg && gtk_socket_id == 0)
	    gui_mch_iconify();
#endif

    return OK;
}


/*ARGSUSED0*/
    void
gui_mch_exit(int rc)
{
    if (gui.mainwin != NULL)
	gtk_widget_destroy(gui.mainwin);

    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*
 * Get the position of the top left corner of the window.
 */
    int
gui_mch_get_winpos(int *x, int *y)
{
#ifdef HAVE_GTK2
    gtk_window_get_position(GTK_WINDOW(gui.mainwin), x, y);
#else
    /* For some people this must be gdk_window_get_origin() for a correct
     * result.	Where is the documentation! */
    gdk_window_get_root_origin(gui.mainwin->window, x, y);
#endif
    return OK;
}

/*
 * Set the position of the top left corner of the window to the given
 * coordinates.
 */
    void
gui_mch_set_winpos(int x, int y)
{
#ifdef HAVE_GTK2
    gtk_window_move(GTK_WINDOW(gui.mainwin), x, y);
#else
    gdk_window_move(gui.mainwin->window, x, y);
#endif
}

#ifdef HAVE_GTK2
#if 0
static int resize_idle_installed = FALSE;
/*
 * Idle handler to force resize.  Used by gui_mch_set_shellsize() to ensure
 * the shell size doesn't exceed the window size, i.e. if the window manager
 * ignored our size request.  Usually this happens if the window is maximized.
 *
 * FIXME: It'd be nice if we could find a little more orthodox solution.
 * See also the remark below in gui_mch_set_shellsize().
 *
 * DISABLED: When doing ":set lines+=1" this function would first invoke
 * gui_resize_shell() with the old size, then the normal callback would
 * report the new size through form_configure_event().  That caused the window
 * layout to be messed up.
 */
/*ARGSUSED0*/
    static gboolean
force_shell_resize_idle(gpointer data)
{
    if (gui.mainwin != NULL
	    && GTK_WIDGET_REALIZED(gui.mainwin)
	    && GTK_WIDGET_VISIBLE(gui.mainwin))
    {
	int width;
	int height;

	gtk_window_get_size(GTK_WINDOW(gui.mainwin), &width, &height);

	width  -= get_menu_tool_width();
	height -= get_menu_tool_height();

	gui_resize_shell(width, height);
    }

    resize_idle_installed = FALSE;
    return FALSE; /* don't call me again */
}
#endif
#endif /* HAVE_GTK2 */

/*
 * Set the windows size.
 */
/*ARGSUSED2*/
    void
gui_mch_set_shellsize(int width, int height,
		      int min_width,  int min_height,
		      int base_width, int base_height,
		      int direction)
{
#ifndef HAVE_GTK2
    /* Hack: When the form already is at the desired size, the window might
     * have been resized with the mouse.  Force a resize by setting a
     * different size first. */
    if (GTK_FORM(gui.formwin)->width == width
	    && GTK_FORM(gui.formwin)->height == height)
    {
	gtk_form_set_size(GTK_FORM(gui.formwin), width + 1, height + 1);
	gui_mch_update();
    }
    gtk_form_set_size(GTK_FORM(gui.formwin), width, height);
#endif

    /* give GTK+ a chance to put all widget's into place */
    gui_mch_update();

#ifndef HAVE_GTK2
    /* this will cause the proper resizement to happen too */
    update_window_manager_hints(0, 0);

#else  /* HAVE_GTK2 */
    /* this will cause the proper resizement to happen too */
    if (gtk_socket_id == 0)
	update_window_manager_hints(0, 0);

    /* With GTK+ 2, changing the size of the form widget doesn't resize
     * the window.  So let's do it the other way around and resize the
     * main window instead. */
    width  += get_menu_tool_width();
    height += get_menu_tool_height();

    if (gtk_socket_id == 0)
	gtk_window_resize(GTK_WINDOW(gui.mainwin), width, height);
    else
	update_window_manager_hints(width, height);

#if 0
    if (!resize_idle_installed)
    {
	g_idle_add_full(GDK_PRIORITY_EVENTS + 10,
			&force_shell_resize_idle, NULL, NULL);
	resize_idle_installed = TRUE;
    }
#endif
    /*
     * Wait until all events are processed to prevent a crash because the
     * real size of the drawing area doesn't reflect Vim's internal ideas.
     *
     * This is a bit of a hack, since Vim is a terminal application with a GUI
     * on top, while the GUI expects to be the boss.
     */
    gui_mch_update();
#endif
}


/*
 * The screen size is used to make sure the initial window doesn't get bigger
 * than the screen.  This subtracts some room for menubar, toolbar and window
 * decorations.
 */
    void
gui_mch_get_screen_dimensions(int *screen_w, int *screen_h)
{
#ifdef HAVE_GTK_MULTIHEAD
    GdkScreen* screen;

    if (gui.mainwin != NULL && gtk_widget_has_screen(gui.mainwin))
	screen = gtk_widget_get_screen(gui.mainwin);
    else
	screen = gdk_screen_get_default();

    *screen_w = gdk_screen_get_width(screen);
    *screen_h = gdk_screen_get_height(screen) - p_ghr;
#else
    *screen_w = gdk_screen_width();
    /* Subtract 'guiheadroom' from the height to allow some room for the
     * window manager (task list and window title bar). */
    *screen_h = gdk_screen_height() - p_ghr;
#endif

    /*
     * FIXME: dirty trick: Because the gui_get_base_height() doesn't include
     * the toolbar and menubar for GTK, we subtract them from the screen
     * hight, so that the window size can be made to fit on the screen.
     * This should be completely changed later.
     */
    *screen_w -= get_menu_tool_width();
    *screen_h -= get_menu_tool_height();
}

#if defined(FEAT_TITLE) || defined(PROTO)
/*ARGSUSED*/
    void
gui_mch_settitle(char_u *title, char_u *icon)
{
# ifdef HAVE_GTK2
    if (title != NULL && output_conv.vc_type != CONV_NONE)
	title = string_convert(&output_conv, title, NULL);
# endif

    gtk_window_set_title(GTK_WINDOW(gui.mainwin), (const char *)title);

# ifdef HAVE_GTK2
    if (output_conv.vc_type != CONV_NONE)
	vim_free(title);
# endif
}
#endif /* FEAT_TITLE */

#if defined(FEAT_MENU) || defined(PROTO)
    void
gui_mch_enable_menu(int showit)
{
    GtkWidget *widget;

# ifdef FEAT_GUI_GNOME
    if (using_gnome)
	widget = gui.menubar_h;
    else
# endif
	widget = gui.menubar;

    /* Do not disable the menu while starting up, otherwise F10 doesn't work. */
    if (!showit != !GTK_WIDGET_VISIBLE(widget) && !gui.starting)
    {
	if (showit)
	    gtk_widget_show(widget);
	else
	    gtk_widget_hide(widget);

	update_window_manager_hints(0, 0);
    }
}
#endif /* FEAT_MENU */

#if defined(FEAT_TOOLBAR) || defined(PROTO)
    void
gui_mch_show_toolbar(int showit)
{
    GtkWidget *widget;

    if (gui.toolbar == NULL)
	return;

# ifdef FEAT_GUI_GNOME
    if (using_gnome)
	widget = gui.toolbar_h;
    else
# endif
	widget = gui.toolbar;

    if (showit)
	set_toolbar_style(GTK_TOOLBAR(gui.toolbar));

    if (!showit != !GTK_WIDGET_VISIBLE(widget))
    {
	if (showit)
	    gtk_widget_show(widget);
	else
	    gtk_widget_hide(widget);

	update_window_manager_hints(0, 0);
    }
}
#endif /* FEAT_TOOLBAR */

#ifndef HAVE_GTK2
/*
 * Get a font structure for highlighting.
 * "cbdata" is a pointer to the global gui structure.
 */
/*ARGSUSED*/
    static void
font_sel_ok(GtkWidget *wgt, gpointer cbdata)
{
    gui_T *vw = (gui_T *)cbdata;
    GtkFontSelectionDialog *fs = (GtkFontSelectionDialog *)vw->fontdlg;

    if (vw->fontname)
	g_free(vw->fontname);

    vw->fontname = (char_u *)gtk_font_selection_dialog_get_font_name(fs);
    gtk_widget_hide(vw->fontdlg);
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*ARGSUSED*/
    static void
font_sel_cancel(GtkWidget *wgt, gpointer cbdata)
{
    gui_T *vw = (gui_T *)cbdata;

    gtk_widget_hide(vw->fontdlg);
    if (gtk_main_level() > 0)
	gtk_main_quit();
}

/*ARGSUSED*/
    static void
font_sel_destroy(GtkWidget *wgt, gpointer cbdata)
{
    gui_T *vw = (gui_T *)cbdata;

    vw->fontdlg = NULL;
    if (gtk_main_level() > 0)
	gtk_main_quit();
}
#endif /* !HAVE_GTK2 */

#ifdef HAVE_GTK2
/*
 * Check if a given font is a CJK font. This is done in a very crude manner. It
 * just see if U+04E00 for zh and ja and U+AC00 for ko are covered in a given
 * font. Consequently, this function cannot  be used as a general purpose check
 * for CJK-ness for which fontconfig APIs should be used.  This is only used by
 * gui_mch_init_font() to deal with 'CJK fixed width fonts'.
 */
    static int
is_cjk_font(PangoFontDescription *font_desc)
{
    static const char * const cjk_langs[] =
	{"zh_CN", "zh_TW", "zh_HK", "ja", "ko"};

    PangoFont	*font;
    unsigned	i;
    int		is_cjk = FALSE;

    font = pango_context_load_font(gui.text_context, font_desc);

    if (font == NULL)
	return FALSE;

    for (i = 0; !is_cjk && i < G_N_ELEMENTS(cjk_langs); ++i)
    {
	PangoCoverage	*coverage;
	gunichar	uc;

	coverage = pango_font_get_coverage(
		font, pango_language_from_string(cjk_langs[i]));

	if (coverage != NULL)
	{
	    uc = (cjk_langs[i][0] == 'k') ? 0xAC00 : 0x4E00;
	    is_cjk = (pango_coverage_get(coverage, uc) == PANGO_COVERAGE_EXACT);
	    pango_coverage_unref(coverage);
	}
    }

    g_object_unref(font);

    return is_cjk;
}
#endif /* HAVE_GTK2 */

/*
 * Adjust gui.char_height (after 'linespace' was changed).
 */
    int
gui_mch_adjust_charheight(void)
{
#ifdef HAVE_GTK2
    PangoFontMetrics	*metrics;
    int			ascent;
    int			descent;

    metrics = pango_context_get_metrics(gui.text_context, gui.norm_font,
				pango_context_get_language(gui.text_context));
    ascent  = pango_font_metrics_get_ascent(metrics);
    descent = pango_font_metrics_get_descent(metrics);

    pango_font_metrics_unref(metrics);

    gui.char_height = (ascent + descent + PANGO_SCALE - 1) / PANGO_SCALE
								+ p_linespace;
    /* LINTED: avoid warning: bitwise operation on signed value */
    gui.char_ascent = PANGO_PIXELS(ascent + p_linespace * PANGO_SCALE / 2);

#else /* !HAVE_GTK2 */

    gui.char_height = gui.current_font->ascent + gui.current_font->descent
								+ p_linespace;
    gui.char_ascent = gui.current_font->ascent + p_linespace / 2;

#endif /* !HAVE_GTK2 */

    /* A not-positive value of char_height may crash Vim.  Only happens
     * if 'linespace' is negative (which does make sense sometimes). */
    gui.char_ascent = MAX(gui.char_ascent, 0);
    gui.char_height = MAX(gui.char_height, gui.char_ascent + 1);

    return OK;
}

#if defined(FEAT_XFONTSET) || defined(PROTO)
/*
 * Try to load the requested fontset.
 */
/*ARGSUSED2*/
    GuiFontset
gui_mch_get_fontset(char_u *name, int report_error, int fixed_width)
{
    GdkFont *font;

    if (!gui.in_use || name == NULL)
	return NOFONT;

    font = gdk_fontset_load((gchar *)name);

    if (font == NULL)
    {
	if (report_error)
	    EMSG2(_(e_fontset), name);
	return NOFONT;
    }
    /* TODO: check if the font is fixed width. */

    /* reference this font as being in use */
    gdk_font_ref(font);

    return (GuiFontset)font;
}
#endif /* FEAT_XFONTSET */

#ifndef HAVE_GTK2
/*
 * Put up a font dialog and return the selected font name in allocated memory.
 * "oldval" is the previous value.
 * Return NULL when cancelled.
 */
    char_u *
gui_mch_font_dialog(char_u *oldval)
{
    char_u *fontname = NULL;

    if (!gui.fontdlg)
    {
	GtkFontSelectionDialog	*fsd = NULL;

	gui.fontdlg = gtk_font_selection_dialog_new(_("Font Selection"));
	fsd = GTK_FONT_SELECTION_DIALOG(gui.fontdlg);
	gtk_window_set_modal(GTK_WINDOW(gui.fontdlg), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(gui.fontdlg),
		GTK_WINDOW(gui.mainwin));
	gtk_signal_connect(GTK_OBJECT(gui.fontdlg), "destroy",
		GTK_SIGNAL_FUNC(font_sel_destroy), &gui);
	gtk_signal_connect(GTK_OBJECT(fsd->ok_button), "clicked",
		GTK_SIGNAL_FUNC(font_sel_ok), &gui);
	gtk_signal_connect(GTK_OBJECT(fsd->cancel_button), "clicked",
		GTK_SIGNAL_FUNC(font_sel_cancel), &gui);
    }

    if (oldval != NULL && *oldval != NUL)
	gtk_font_selection_dialog_set_font_name(
		GTK_FONT_SELECTION_DIALOG(gui.fontdlg), (char *)oldval);

    if (gui.fontname)
    {
	g_free(gui.fontname);
	gui.fontname = NULL;
    }
    gtk_window_position(GTK_WINDOW(gui.fontdlg), GTK_WIN_POS_MOUSE);
    gtk_widget_show(gui.fontdlg);
    {
	static gchar	*spacings[] = {"c", "m", NULL};

	/* In GTK 1.2.3 this must be after the gtk_widget_show() call,
	 * otherwise everything is blocked for ten seconds. */
	gtk_font_selection_dialog_set_filter(
		GTK_FONT_SELECTION_DIALOG(gui.fontdlg),
		GTK_FONT_FILTER_BASE,
		GTK_FONT_ALL, NULL, NULL,
		NULL, NULL, spacings, NULL);
    }

    /* Wait for the font dialog to be closed. */
    while (gui.fontdlg && GTK_WIDGET_DRAWABLE(gui.fontdlg))
	gtk_main_iteration_do(TRUE);

    if (gui.fontname != NULL)
    {
	/* Apparently some font names include a comma, need to escape that,
	 * because in 'guifont' it separates names. */
	fontname = vim_strsave_escaped(gui.fontname, (char_u *)",");
	g_free(gui.fontname);
	gui.fontname = NULL;
    }
    return fontname;
}
#endif /* !HAVE_GTK2 */

#ifdef HAVE_GTK2
/*
 * Put up a font dialog and return the selected font name in allocated memory.
 * "oldval" is the previous value.  Return NULL when cancelled.
 * This should probably go into gui_gtk.c.  Hmm.
 * FIXME:
 * The GTK2 font selection dialog has no filtering API.  So we could either
 * a) implement our own (possibly copying the code from somewhere else) or
 * b) just live with it.
 */
    char_u *
gui_mch_font_dialog(char_u *oldval)
{
    GtkWidget	*dialog;
    int		response;
    char_u	*fontname = NULL;
    char_u	*oldname;

    dialog = gtk_font_selection_dialog_new(NULL);

    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gui.mainwin));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);

    if (oldval != NULL && oldval[0] != NUL)
    {
	if (output_conv.vc_type != CONV_NONE)
	    oldname = string_convert(&output_conv, oldval, NULL);
	else
	    oldname = oldval;

	/* Annoying bug in GTK (or Pango): if the font name does not include a
	 * size, zero is used.  Use default point size ten. */
	if (!vim_isdigit(oldname[STRLEN(oldname) - 1]))
	{
	    char_u	*p = vim_strnsave(oldname, STRLEN(oldname) + 3);

	    if (p != NULL)
	    {
		STRCPY(p + STRLEN(p), " 10");
		if (oldname != oldval)
		    vim_free(oldname);
		oldname = p;
	    }
	}

	gtk_font_selection_dialog_set_font_name(
		GTK_FONT_SELECTION_DIALOG(dialog), (const char *)oldname);

	if (oldname != oldval)
	    vim_free(oldname);
    }

    response = gtk_dialog_run(GTK_DIALOG(dialog));

    if (response == GTK_RESPONSE_OK)
    {
	char *name;

	name = gtk_font_selection_dialog_get_font_name(
			    GTK_FONT_SELECTION_DIALOG(dialog));
	if (name != NULL)
	{
	    char_u  *p;

	    /* Apparently some font names include a comma, need to escape
	     * that, because in 'guifont' it separates names. */
	    p = vim_strsave_escaped((char_u *)name, (char_u *)",");
	    g_free(name);
	    if (p != NULL && input_conv.vc_type != CONV_NONE)
	    {
		fontname = string_convert(&input_conv, p, NULL);
		vim_free(p);
	    }
	    else
		fontname = p;
	}
    }

    if (response != GTK_RESPONSE_NONE)
	gtk_widget_destroy(dialog);

    return fontname;
}

/*
 * Some monospace fonts don't support a bold weight, and fall back
 * silently to the regular weight.  But this is no good since our text
 * drawing function can emulate bold by overstriking.  So let's try
 * to detect whether bold weight is actually available and emulate it
 * otherwise.
 *
 * Note that we don't need to check for italic style since Xft can
 * emulate italic on its own, provided you have a proper fontconfig
 * setup.  We wouldn't be able to emulate it in Vim anyway.
 */
    static void
get_styled_font_variants(void)
{
    PangoFontDescription    *bold_font_desc;
    PangoFont		    *plain_font;
    PangoFont		    *bold_font;

    gui.font_can_bold = FALSE;

    plain_font = pango_context_load_font(gui.text_context, gui.norm_font);

    if (plain_font == NULL)
	return;

    bold_font_desc = pango_font_description_copy_static(gui.norm_font);
    pango_font_description_set_weight(bold_font_desc, PANGO_WEIGHT_BOLD);

    bold_font = pango_context_load_font(gui.text_context, bold_font_desc);
    /*
     * The comparison relies on the unique handle nature of a PangoFont*,
     * i.e. it's assumed that a different PangoFont* won't refer to the
     * same font.  Seems to work, and failing here isn't critical anyway.
     */
    if (bold_font != NULL)
    {
	gui.font_can_bold = (bold_font != plain_font);
	g_object_unref(bold_font);
    }

    pango_font_description_free(bold_font_desc);
    g_object_unref(plain_font);
}

#else /* !HAVE_GTK2 */

/*
 * There is only one excuse I can give for the following attempt to manage font
 * styles:
 *
 * I HATE THE BRAIN DEAD WAY X11 IS HANDLING FONTS (--mdcki)
 * (Me too. --danielk)
 */
    static void
get_styled_font_variants(char_u * font_name)
{
    char	*chunk[32];
    char	*sdup;
    char	*tmp;
    int		len, i;
    GuiFont	*styled_font[3];

    styled_font[0] = &gui.bold_font;
    styled_font[1] = &gui.ital_font;
    styled_font[2] = &gui.boldital_font;

    /* First free whatever was freviously there. */
    for (i = 0; i < 3; ++i)
	if (*styled_font[i])
	{
	    gdk_font_unref(*styled_font[i]);
	    *styled_font[i] = NULL;
	}

    if ((sdup = g_strdup((const char *)font_name)) == NULL)
	return;

    /* split up the whole */
    i = 0;
    for (tmp = sdup; *tmp != '\0'; ++tmp)
    {
	if (*tmp == '-')
	{
	    *tmp = '\0';

	    if (i == 32)
		break;

	    chunk[i] = tmp + 1;
	    ++i;
	}
    }

    if (i == 14)
    {
	GdkFont		*font = NULL;
	const char	*bold_chunk[3]	    = { "bold", NULL,	"bold" };
	const char	*italic_chunk[3]    = { NULL,	"o",	"o" };

	/* font name was complete */
	len = strlen((const char *)font_name) + 32;

	for (i = 0; i < 3; ++i)
	{
	    char *styled_name;
	    int j;

	    styled_name = (char *)alloc(len);
	    if (styled_name == NULL)
	    {
		g_free(sdup);
		return;
	    }

	    *styled_name = '\0';

	    for (j = 0; j < 14; ++j)
	    {
		strcat(styled_name, "-");
		if (j == 2 && bold_chunk[i] != NULL)
		    strcat(styled_name, bold_chunk[i]);
		else if (j == 3 && italic_chunk[i] != NULL)
		    strcat(styled_name, italic_chunk[i]);
		else
		    strcat(styled_name, chunk[j]);
	    }

	    font = gui_mch_get_font((char_u *)styled_name, FALSE);
	    if (font != NULL)
		*styled_font[i] = font;

	    vim_free(styled_name);
	}
    }

    g_free(sdup);
}
#endif /* !HAVE_GTK2 */

#ifdef HAVE_GTK2
static PangoEngineShape *default_shape_engine = NULL;

/*
 * Create a map from ASCII characters in the range [32,126] to glyphs
 * of the current font.  This is used by gui_gtk2_draw_string() to skip
 * the itemize and shaping process for the most common case.
 */
    static void
ascii_glyph_table_init(void)
{
    char_u	    ascii_chars[128];
    PangoAttrList   *attr_list;
    GList	    *item_list;
    int		    i;

    if (gui.ascii_glyphs != NULL)
	pango_glyph_string_free(gui.ascii_glyphs);
    if (gui.ascii_font != NULL)
	g_object_unref(gui.ascii_font);

    gui.ascii_glyphs = NULL;
    gui.ascii_font   = NULL;

    /* For safety, fill in question marks for the control characters. */
    for (i = 0; i < 32; ++i)
	ascii_chars[i] = '?';
    for (; i < 127; ++i)
	ascii_chars[i] = i;
    ascii_chars[i] = '?';

    attr_list = pango_attr_list_new();
    item_list = pango_itemize(gui.text_context, (const char *)ascii_chars,
			      0, sizeof(ascii_chars), attr_list, NULL);

    if (item_list != NULL && item_list->next == NULL) /* play safe */
    {
	PangoItem   *item;
	int	    width;

	item  = (PangoItem *)item_list->data;
	width = gui.char_width * PANGO_SCALE;

	/* Remember the shape engine used for ASCII. */
	default_shape_engine = item->analysis.shape_engine;

	gui.ascii_font = item->analysis.font;
	g_object_ref(gui.ascii_font);

	gui.ascii_glyphs = pango_glyph_string_new();

	pango_shape((const char *)ascii_chars, sizeof(ascii_chars),
		    &item->analysis, gui.ascii_glyphs);

	g_return_if_fail(gui.ascii_glyphs->num_glyphs == sizeof(ascii_chars));

	for (i = 0; i < gui.ascii_glyphs->num_glyphs; ++i)
	{
	    PangoGlyphGeometry *geom;

	    geom = &gui.ascii_glyphs->glyphs[i].geometry;
	    geom->x_offset += MAX(0, width - geom->width) / 2;
	    geom->width = width;
	}
    }

    g_list_foreach(item_list, (GFunc)&pango_item_free, NULL);
    g_list_free(item_list);
    pango_attr_list_unref(attr_list);
}
#endif /* HAVE_GTK2 */

/*
 * Initialize Vim to use the font or fontset with the given name.
 * Return FAIL if the font could not be loaded, OK otherwise.
 */
/*ARGSUSED1*/
    int
gui_mch_init_font(char_u *font_name, int fontset)
{
#ifdef HAVE_GTK2
    PangoFontDescription    *font_desc;
    PangoLayout		    *layout;
    int			    width;

    /* If font_name is NULL, this means to use the default, which should
     * be present on all proper Pango/fontconfig installations. */
    if (font_name == NULL)
	font_name = (char_u *)DEFAULT_FONT;

    font_desc = gui_mch_get_font(font_name, FALSE);

    if (font_desc == NULL)
	return FAIL;

    gui_mch_free_font(gui.norm_font);
    gui.norm_font = font_desc;

    pango_context_set_font_description(gui.text_context, font_desc);

    layout = pango_layout_new(gui.text_context);
    pango_layout_set_text(layout, "MW", 2);
    pango_layout_get_size(layout, &width, NULL);
    /*
     * Set char_width to half the width obtained from pango_layout_get_size()
     * for CJK fixed_width/bi-width fonts.  An unpatched version of Xft leads
     * Pango to use the same width for both non-CJK characters (e.g. Latin
     * letters and numbers) and CJK characters.  This results in 's p a c e d
     * o u t' rendering when a CJK 'fixed width' font is used. To work around
     * that, divide the width returned by Pango by 2 if cjk_width is equal to
     * width for CJK fonts.
     *
     * For related bugs, see:
     * http://bugzilla.gnome.org/show_bug.cgi?id=106618
     * http://bugzilla.gnome.org/show_bug.cgi?id=106624
     *
     * With this, for all four of the following cases, Vim works fine:
     *	   guifont=CJK_fixed_width_font
     *	   guifont=Non_CJK_fixed_font
     *	   guifont=Non_CJK_fixed_font,CJK_Fixed_font
     *	   guifont=Non_CJK_fixed_font guifontwide=CJK_fixed_font
     */
    if (is_cjk_font(gui.norm_font))
    {
	int cjk_width;

	/* Measure the text extent of U+4E00 and U+4E8C */
	pango_layout_set_text(layout, "\344\270\200\344\272\214", -1);
	pango_layout_get_size(layout, &cjk_width, NULL);

	if (width == cjk_width)  /* Xft not patched */
	    width /= 2;
    }
    g_object_unref(layout);

    gui.char_width = (width / 2 + PANGO_SCALE - 1) / PANGO_SCALE;

    /* A zero width may cause a crash.	Happens for semi-invalid fontsets. */
    if (gui.char_width <= 0)
	gui.char_width = 8;

    gui_mch_adjust_charheight();

    /* Set the fontname, which will be used for information purposes */
    hl_set_font_name(font_name);

    get_styled_font_variants();
    ascii_glyph_table_init();

    /* Avoid unnecessary overhead if 'guifontwide' is equal to 'guifont'. */
    if (gui.wide_font != NULL
	&& pango_font_description_equal(gui.norm_font, gui.wide_font))
    {
	pango_font_description_free(gui.wide_font);
	gui.wide_font = NULL;
    }

#else /* !HAVE_GTK2 */

    GdkFont	*font = NULL;

# ifdef FEAT_XFONTSET
    /* Try loading a fontset.  If this fails we try loading a normal font. */
    if (fontset && font_name != NULL)
	font = gui_mch_get_fontset(font_name, TRUE, TRUE);

    if (font == NULL)
# endif
    {
	/* If font_name is NULL, this means to use the default, which should
	 * be present on all X11 servers. */
	if (font_name == NULL)
	    font_name = (char_u *)DEFAULT_FONT;
	font = gui_mch_get_font(font_name, FALSE);
    }

    if (font == NULL)
	return FAIL;

    gui_mch_free_font(gui.norm_font);
# ifdef FEAT_XFONTSET
    gui_mch_free_fontset(gui.fontset);
    if (font->type == GDK_FONT_FONTSET)
    {
	gui.norm_font = NOFONT;
	gui.fontset = (GuiFontset)font;
	/* Use two bytes, this works around the problem that the result would
	 * be zero if no 8-bit font was found. */
	gui.char_width = gdk_string_width(font, "xW") / 2;
    }
    else
# endif
    {
	gui.norm_font = font;
# ifdef FEAT_XFONTSET
	gui.fontset = NOFONTSET;
# endif
	gui.char_width = ((XFontStruct *)
				      GDK_FONT_XFONT(font))->max_bounds.width;
    }

    /* A zero width may cause a crash.	Happens for semi-invalid fontsets. */
    if (gui.char_width <= 0)
	gui.char_width = 8;

    gui.char_height = font->ascent + font->descent + p_linespace;
    gui.char_ascent = font->ascent + p_linespace / 2;

    /* A not-positive value of char_height may crash Vim.  Only happens
     * if 'linespace' is negative (which does make sense sometimes). */
    gui.char_ascent = MAX(gui.char_ascent, 0);
    gui.char_height = MAX(gui.char_height, gui.char_ascent + 1);

    /* Set the fontname, which will be used for information purposes */
    hl_set_font_name(font_name);

    if (font->type != GDK_FONT_FONTSET)
	get_styled_font_variants(font_name);

    /* Synchronize the fonts used in user input dialogs, since otherwise
     * search/replace will be esp. annoying in case of international font
     * usage.
     */
    gui_gtk_synch_fonts();

# ifdef FEAT_XIM
    /* Adjust input management behaviour to the capabilities of the new
     * fontset */
    xim_decide_input_style();
    if (xim_get_status_area_height())
    {
	/* Status area is required.  Just create the empty container so that
	 * mainwin will allocate the extra space for status area. */
	GtkWidget *alignment = gtk_alignment_new((gfloat)0.5, (gfloat)0.5,
						    (gfloat)1.0, (gfloat)1.0);

	gtk_widget_set_usize(alignment, 20, gui.char_height + 2);
	gtk_box_pack_end(GTK_BOX(GTK_BIN(gui.mainwin)->child),
			 alignment, FALSE, FALSE, 0);
	gtk_widget_show(alignment);
    }
# endif
#endif /* !HAVE_GTK2 */

    /* Preserve the logical dimensions of the screen. */
    update_window_manager_hints(0, 0);

    return OK;
}

/*
 * Get a reference to the font "name".
 * Return zero for failure.
 */
    GuiFont
gui_mch_get_font(char_u *name, int report_error)
{
#ifdef HAVE_GTK2
    PangoFontDescription    *font;
#else
    GdkFont		    *font;
#endif

    /* can't do this when GUI is not running */
    if (!gui.in_use || name == NULL)
	return NULL;

#ifdef HAVE_GTK2
    if (output_conv.vc_type != CONV_NONE)
    {
	char_u *buf;

	buf = string_convert(&output_conv, name, NULL);
	if (buf != NULL)
	{
	    font = pango_font_description_from_string((const char *)buf);
	    vim_free(buf);
	}
	else
	    font = NULL;
    }
    else
	font = pango_font_description_from_string((const char *)name);

    if (font != NULL)
    {
	PangoFont *real_font;

	/* pango_context_load_font() bails out if no font size is set */
	if (pango_font_description_get_size(font) <= 0)
	    pango_font_description_set_size(font, 10 * PANGO_SCALE);

	real_font = pango_context_load_font(gui.text_context, font);

	if (real_font == NULL)
	{
	    pango_font_description_free(font);
	    font = NULL;
	}
	else
	    g_object_unref(real_font);
    }
#else
    font = gdk_font_load((const gchar *)name);
#endif

    if (font == NULL)
    {
	if (report_error)
	    EMSG2(_((char *)e_font), name);
	return NULL;
    }

#ifdef HAVE_GTK2
    /*
     * The fixed-width check has been disabled for GTK+ 2.  Rationale:
     *
     *	 - The check tends to report false positives, particularly
     *	   in non-Latin locales or with old X fonts.
     *	 - Thanks to our fixed-width hack in gui_gtk2_draw_string(),
     *	   GTK+ 2 Vim is actually capable of displaying variable width
     *	   fonts.  Those will just be spaced out like in AA xterm.
     *	 - Failing here for the default font causes GUI startup to fail
     *	   even with wiped out configuration files.
     *	 - The font dialog displays all fonts unfiltered, and it's rather
     *	   annoying if 95% of the listed fonts produce an error message.
     */
# if 0
    {
	/* Check that this is a mono-spaced font.  Naturally, this is a bit
	 * hackish -- fixed-width isn't really suitable for i18n text :/ */
	PangoLayout	*layout;
	unsigned int	i;
	int		last_width   = -1;
	const char	test_chars[] = { 'W', 'i', ',', 'x' }; /* arbitrary */

	layout = pango_layout_new(gui.text_context);
	pango_layout_set_font_description(layout, font);

	for (i = 0; i < G_N_ELEMENTS(test_chars); ++i)
	{
	    int width;

	    pango_layout_set_text(layout, &test_chars[i], 1);
	    pango_layout_get_size(layout, &width, NULL);

	    if (last_width >= 0 && width != last_width)
	    {
		pango_font_description_free(font);
		font = NULL;
		break;
	    }

	    last_width = width;
	}

	g_object_unref(layout);
    }
# endif
#else /* !HAVE_GTK2 */
    {
	XFontStruct *xfont;

	/* reference this font as being in use */
	gdk_font_ref(font);

	/* Check that this is a mono-spaced font.
	 */
	xfont = (XFontStruct *) GDK_FONT_XFONT(font);

	if (xfont->max_bounds.width != xfont->min_bounds.width)
	{
	    gdk_font_unref(font);
	    font = NULL;
	}
    }
#endif /* !HAVE_GTK2 */

#if !defined(HAVE_GTK2) || 0 /* disabled for GTK+ 2, see above */
    if (font == NULL && report_error)
	EMSG2(_(e_fontwidth), name);
#endif

    return font;
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Return the name of font "font" in allocated memory.
 */
/*ARGSUSED*/
    char_u *
gui_mch_get_fontname(GuiFont font, char_u *name)
{
# ifdef HAVE_GTK2
    if (font != NOFONT)
    {
	char	*pangoname = pango_font_description_to_string(font);

	if (pangoname != NULL)
	{
	    char_u	*s = vim_strsave((char_u *)pangoname);

	    g_free(pangoname);
	    return s;
	}
    }
# else
    /* Don't know how to get the name, return what we got. */
    if (name != NULL)
	return vim_strsave(name);
# endif
    return NULL;
}
#endif

#if !defined(HAVE_GTK2) || defined(PROTO)
/*
 * Set the current text font.
 * Since we create all GC on demand, we use just gui.current_font to
 * indicate the desired current font.
 */
    void
gui_mch_set_font(GuiFont font)
{
    gui.current_font = font;
}
#endif

#if defined(FEAT_XFONTSET) || defined(PROTO)
/*
 * Set the current text fontset.
 */
    void
gui_mch_set_fontset(GuiFontset fontset)
{
    gui.current_font = fontset;
}
#endif

/*
 * If a font is not going to be used, free its structure.
 */
    void
gui_mch_free_font(GuiFont font)
{
    if (font != NOFONT)
#ifdef HAVE_GTK2
	pango_font_description_free(font);
#else
	gdk_font_unref(font);
#endif
}

#if defined(FEAT_XFONTSET) || defined(PROTO)
/*
 * If a fontset is not going to be used, free its structure.
 */
    void
gui_mch_free_fontset(GuiFontset fontset)
{
    if (fontset != NOFONTSET)
	gdk_font_unref(fontset);
}
#endif


/*
 * Return the Pixel value (color) for the given color name.  This routine was
 * pretty much taken from example code in the Silicon Graphics OSF/Motif
 * Programmer's Guide.
 * Return INVALCOLOR for error.
 */
    guicolor_T
gui_mch_get_color(char_u *name)
{
    /* A number of colors that some X11 systems don't have */
    static const char *const vimnames[][2] =
    {
	{"LightRed",	"#FFBBBB"},
	{"LightGreen",	"#88FF88"},
	{"LightMagenta","#FFBBFF"},
	{"DarkCyan",	"#008888"},
	{"DarkBlue",	"#0000BB"},
	{"DarkRed",	"#BB0000"},
	{"DarkMagenta", "#BB00BB"},
	{"DarkGrey",	"#BBBBBB"},
	{"DarkYellow",	"#BBBB00"},
	{"Gray10",	"#1A1A1A"},
	{"Grey10",	"#1A1A1A"},
	{"Gray20",	"#333333"},
	{"Grey20",	"#333333"},
	{"Gray30",	"#4D4D4D"},
	{"Grey30",	"#4D4D4D"},
	{"Gray40",	"#666666"},
	{"Grey40",	"#666666"},
	{"Gray50",	"#7F7F7F"},
	{"Grey50",	"#7F7F7F"},
	{"Gray60",	"#999999"},
	{"Grey60",	"#999999"},
	{"Gray70",	"#B3B3B3"},
	{"Grey70",	"#B3B3B3"},
	{"Gray80",	"#CCCCCC"},
	{"Grey80",	"#CCCCCC"},
	{"Gray90",	"#E5E5E5"},
	{"Grey90",	"#E5E5E5"},
	{NULL, NULL}
    };

    if (!gui.in_use)		/* can't do this when GUI not running */
	return INVALCOLOR;

    while (name != NULL)
    {
	GdkColor    color;
	int	    parsed;
	int	    i;

	parsed = gdk_color_parse((const char *)name, &color);

#ifndef HAVE_GTK2 /* ohh, lovely GTK+ 2, eases our pain :) */
	/*
	 * Since we have already called gtk_set_locale here the bugger
	 * XParseColor will accept only explicit color names in the language
	 * of the current locale.  However this will interfere with:
	 * 1. Vim's global startup files
	 * 2. Explicit color names in .vimrc
	 *
	 * Therefore we first try to parse the color in the current locale and
	 * if it fails, we fall back to the portable "C" one.
	 */
	if (!parsed)
	{
	    char *current;

	    current = setlocale(LC_ALL, NULL);
	    if (current != NULL)
	    {
		char *saved;

		saved = g_strdup(current);
		setlocale(LC_ALL, "C");

		parsed = gdk_color_parse((const gchar *)name, &color);

		setlocale(LC_ALL, saved);
		gtk_set_locale();

		g_free(saved);
	    }
	}
#endif /* !HAVE_GTK2 */

	if (parsed)
	{
#ifdef HAVE_GTK2
	    gdk_colormap_alloc_color(gtk_widget_get_colormap(gui.drawarea),
				     &color, FALSE, TRUE);
#else
	    gdk_color_alloc(gtk_widget_get_colormap(gui.drawarea), &color);
#endif
	    return (guicolor_T)color.pixel;
	}
	/* add a few builtin names and try again */
	for (i = 0; ; ++i)
	{
	    if (vimnames[i][0] == NULL)
	    {
		name = NULL;
		break;
	    }
	    if (STRICMP(name, vimnames[i][0]) == 0)
	    {
		name = (char_u *)vimnames[i][1];
		break;
	    }
	}
    }

    return INVALCOLOR;
}

/*
 * Set the current text foreground color.
 */
    void
gui_mch_set_fg_color(guicolor_T color)
{
    gui.fgcolor->pixel = (unsigned long)color;
}

/*
 * Set the current text background color.
 */
    void
gui_mch_set_bg_color(guicolor_T color)
{
    gui.bgcolor->pixel = (unsigned long)color;
}

/*
 * Set the current text special color.
 */
    void
gui_mch_set_sp_color(guicolor_T color)
{
    gui.spcolor->pixel = (unsigned long)color;
}

#ifdef HAVE_GTK2
/*
 * Function-like convenience macro for the sake of efficiency.
 */
#define INSERT_PANGO_ATTR(Attribute, AttrList, Start, End)  \
    G_STMT_START{					    \
	PangoAttribute *tmp_attr_;			    \
	tmp_attr_ = (Attribute);			    \
	tmp_attr_->start_index = (Start);		    \
	tmp_attr_->end_index = (End);			    \
	pango_attr_list_insert((AttrList), tmp_attr_);	    \
    }G_STMT_END

    static void
apply_wide_font_attr(char_u *s, int len, PangoAttrList *attr_list)
{
    char_u  *start = NULL;
    char_u  *p;
    int	    uc;

    for (p = s; p < s + len; p += utf_byte2len(*p))
    {
	uc = utf_ptr2char(p);

	if (start == NULL)
	{
	    if (uc >= 0x80 && utf_char2cells(uc) == 2)
		start = p;
	}
	else if (uc < 0x80 /* optimization shortcut */
		 || (utf_char2cells(uc) != 2 && !utf_iscomposing(uc)))
	{
	    INSERT_PANGO_ATTR(pango_attr_font_desc_new(gui.wide_font),
			      attr_list, start - s, p - s);
	    start = NULL;
	}
    }

    if (start != NULL)
	INSERT_PANGO_ATTR(pango_attr_font_desc_new(gui.wide_font),
			  attr_list, start - s, len);
}

    static int
count_cluster_cells(char_u *s, PangoItem *item,
		    PangoGlyphString* glyphs, int i,
		    int *cluster_width,
		    int *last_glyph_rbearing)
{
    char_u  *p;
    int	    next;	/* glyph start index of next cluster */
    int	    start, end; /* string segment of current cluster */
    int	    width;	/* real cluster width in Pango units */
    int	    uc;
    int	    cellcount = 0;

    width = glyphs->glyphs[i].geometry.width;

    for (next = i + 1; next < glyphs->num_glyphs; ++next)
    {
	if (glyphs->glyphs[next].attr.is_cluster_start)
	    break;
	else if (glyphs->glyphs[next].geometry.width > width)
	    width = glyphs->glyphs[next].geometry.width;
    }

    start = item->offset + glyphs->log_clusters[i];
    end   = item->offset + ((next < glyphs->num_glyphs) ?
			    glyphs->log_clusters[next] : item->length);

    for (p = s + start; p < s + end; p += utf_byte2len(*p))
    {
	uc = utf_ptr2char(p);
	if (uc < 0x80)
	    ++cellcount;
	else if (!utf_iscomposing(uc))
	    cellcount += utf_char2cells(uc);
    }

    if (last_glyph_rbearing != NULL
	    && cellcount > 0 && next == glyphs->num_glyphs)
    {
	PangoRectangle ink_rect;
	/*
	 * If a certain combining mark had to be taken from a non-monospace
	 * font, we have to compensate manually by adapting x_offset according
	 * to the ink extents of the previous glyph.
	 */
	pango_font_get_glyph_extents(item->analysis.font,
				     glyphs->glyphs[i].glyph,
				     &ink_rect, NULL);

	if (PANGO_RBEARING(ink_rect) > 0)
	    *last_glyph_rbearing = PANGO_RBEARING(ink_rect);
    }

    if (cellcount > 0)
	*cluster_width = width;

    return cellcount;
}

/*
 * If there are only combining characters in the cluster, we cannot just
 * change the width of the previous glyph since there is none.	Therefore
 * some guesswork is needed.
 *
 * If ink_rect.x is negative Pango apparently has taken care of the composing
 * by itself.  Actually setting x_offset = 0 should be sufficient then, but due
 * to problems with composing from different fonts we still need to fine-tune
 * x_offset to avoid uglyness.
 *
 * If ink_rect.x is not negative, force overstriking by pointing x_offset to
 * the position of the previous glyph.	Apparently this happens only with old
 * X fonts which don't provide the special combining information needed by
 * Pango.
 */
    static void
setup_zero_width_cluster(PangoItem *item, PangoGlyphInfo *glyph,
			 int last_cellcount, int last_cluster_width,
			 int last_glyph_rbearing)
{
    PangoRectangle  ink_rect;
    PangoRectangle  logical_rect;
    int		    width;

    width = last_cellcount * gui.char_width * PANGO_SCALE;
    glyph->geometry.x_offset = -width + MAX(0, width - last_cluster_width) / 2;
    glyph->geometry.width = 0;

    pango_font_get_glyph_extents(item->analysis.font,
				 glyph->glyph,
				 &ink_rect, &logical_rect);
    if (ink_rect.x < 0)
    {
	glyph->geometry.x_offset += last_glyph_rbearing;
	glyph->geometry.y_offset  = logical_rect.height
		- (gui.char_height - p_linespace) * PANGO_SCALE;
    }
}

    static void
draw_glyph_string(int row, int col, int num_cells, int flags,
		  PangoFont *font, PangoGlyphString *glyphs)
{
    if (!(flags & DRAW_TRANSP))
    {
	gdk_gc_set_foreground(gui.text_gc, gui.bgcolor);

	gdk_draw_rectangle(gui.drawarea->window,
			   gui.text_gc,
			   TRUE,
			   FILL_X(col),
			   FILL_Y(row),
			   num_cells * gui.char_width,
			   gui.char_height);
    }

    gdk_gc_set_foreground(gui.text_gc, gui.fgcolor);

    gdk_draw_glyphs(gui.drawarea->window,
		    gui.text_gc,
		    font,
		    TEXT_X(col),
		    TEXT_Y(row),
		    glyphs);

    /* redraw the contents with an offset of 1 to emulate bold */
    if ((flags & DRAW_BOLD) && !gui.font_can_bold)
	gdk_draw_glyphs(gui.drawarea->window,
			gui.text_gc,
			font,
			TEXT_X(col) + 1,
			TEXT_Y(row),
			glyphs);
}

#endif /* HAVE_GTK2 */

/*
 * Draw underline and undercurl at the bottom of the character cell.
 */
    static void
draw_under(int flags, int row, int col, int cells)
{
    int			i;
    int			offset;
    const static int	val[8] = {1, 0, 0, 0, 1, 2, 2, 2 };
    int			y = FILL_Y(row + 1) - 1;

    /* Undercurl: draw curl at the bottom of the character cell. */
    if (flags & DRAW_UNDERC)
    {
	gdk_gc_set_foreground(gui.text_gc, gui.spcolor);
	for (i = FILL_X(col); i < FILL_X(col + cells); ++i)
	{
	    offset = val[i % 8];
	    gdk_draw_point(gui.drawarea->window, gui.text_gc, i, y - offset);
	}
	gdk_gc_set_foreground(gui.text_gc, gui.fgcolor);
    }

    /* Underline: draw a line at the bottom of the character cell. */
    if (flags & DRAW_UNDERL)
    {
	/* When p_linespace is 0, overwrite the bottom row of pixels.
	 * Otherwise put the line just below the character. */
	if (p_linespace > 1)
	    y -= p_linespace - 1;
	gdk_draw_line(gui.drawarea->window, gui.text_gc,
		      FILL_X(col), y,
		      FILL_X(col + cells) - 1, y);
    }
}

#if defined(HAVE_GTK2) || defined(PROTO)
    int
gui_gtk2_draw_string(int row, int col, char_u *s, int len, int flags)
{
    GdkRectangle	area;		    /* area for clip mask	  */
    PangoGlyphString	*glyphs;	    /* glyphs of current item	  */
    int			column_offset = 0;  /* column offset in cells	  */
    int			i;
    char_u		*conv_buf = NULL;   /* result of UTF-8 conversion */
    char_u		*new_conv_buf;
    int			convlen;
    char_u		*sp, *bp;
    int			plen;

    if (gui.text_context == NULL || gui.drawarea->window == NULL)
	return len;

    if (output_conv.vc_type != CONV_NONE)
    {
	/*
	 * Convert characters from 'encoding' to 'termencoding', which is set
	 * to UTF-8 by gui_mch_init().	did_set_string_option() in option.c
	 * prohibits changing this to something else than UTF-8 if the GUI is
	 * in use.
	 */
	convlen = len;
	conv_buf = string_convert(&output_conv, s, &convlen);
	g_return_val_if_fail(conv_buf != NULL, len);

	/* Correct for differences in char width: some chars are
	 * double-wide in 'encoding' but single-wide in utf-8.  Add a space to
	 * compensate for that. */
	for (sp = s, bp = conv_buf; sp < s + len && bp < conv_buf + convlen; )
	{
	    plen = utf_ptr2len(bp);
	    if ((*mb_ptr2cells)(sp) == 2 && utf_ptr2cells(bp) == 1)
	    {
		new_conv_buf = alloc(convlen + 2);
		if (new_conv_buf == NULL)
		    return len;
		plen += bp - conv_buf;
		mch_memmove(new_conv_buf, conv_buf, plen);
		new_conv_buf[plen] = ' ';
		mch_memmove(new_conv_buf + plen + 1, conv_buf + plen,
							  convlen - plen + 1);
		vim_free(conv_buf);
		conv_buf = new_conv_buf;
		++convlen;
		bp = conv_buf + plen;
		plen = 1;
	    }
	    sp += (*mb_ptr2len)(sp);
	    bp += plen;
	}
	s = conv_buf;
	len = convlen;
    }

    /*
     * Restrict all drawing to the current screen line in order to prevent
     * fuzzy font lookups from messing up the screen.
     */
    area.x = gui.border_offset;
    area.y = FILL_Y(row);
    area.width	= gui.num_cols * gui.char_width;
    area.height = gui.char_height;

    gdk_gc_set_clip_origin(gui.text_gc, 0, 0);
    gdk_gc_set_clip_rectangle(gui.text_gc, &area);

    glyphs = pango_glyph_string_new();

    /*
     * Optimization hack:  If possible, skip the itemize and shaping process
     * for pure ASCII strings.	This optimization is particularly effective
     * because Vim draws space characters to clear parts of the screen.
     */
    if (!(flags & DRAW_ITALIC)
	    && !((flags & DRAW_BOLD) && gui.font_can_bold)
	    && gui.ascii_glyphs != NULL)
    {
	char_u *p;

	for (p = s; p < s + len; ++p)
	    if (*p & 0x80)
		goto not_ascii;

	pango_glyph_string_set_size(glyphs, len);

	for (i = 0; i < len; ++i)
	{
	    glyphs->glyphs[i] = gui.ascii_glyphs->glyphs[s[i]];
	    glyphs->log_clusters[i] = i;
	}

	draw_glyph_string(row, col, len, flags, gui.ascii_font, glyphs);

	column_offset = len;
    }
    else
not_ascii:
    {
	PangoAttrList	*attr_list;
	GList		*item_list;
	int		cluster_width;
	int		last_glyph_rbearing;
	int		cells = 0;  /* cells occupied by current cluster */
#if 0
	int		monospace13 = STRICMP(p_guifont, "monospace 13") == 0;
#endif

	/* Safety check: pango crashes when invoked with invalid utf-8
	 * characters. */
	if (!utf_valid_string(s, s + len))
	{
	    column_offset = len;
	    goto skipitall;
	}

	/* original width of the current cluster */
	cluster_width = PANGO_SCALE * gui.char_width;

	/* right bearing of the last non-composing glyph */
	last_glyph_rbearing = PANGO_SCALE * gui.char_width;

	attr_list = pango_attr_list_new();

	/* If 'guifontwide' is set then use that for double-width characters.
	 * Otherwise just go with 'guifont' and let Pango do its thing. */
	if (gui.wide_font != NULL)
	    apply_wide_font_attr(s, len, attr_list);

	if ((flags & DRAW_BOLD) && gui.font_can_bold)
	    INSERT_PANGO_ATTR(pango_attr_weight_new(PANGO_WEIGHT_BOLD),
			      attr_list, 0, len);
	if (flags & DRAW_ITALIC)
	    INSERT_PANGO_ATTR(pango_attr_style_new(PANGO_STYLE_ITALIC),
			      attr_list, 0, len);
	/*
	 * Break the text into segments with consistent directional level
	 * and shaping engine.	Pure Latin text needs only a single segment,
	 * so there's no need to worry about the loop's efficiency.  Better
	 * try to optimize elsewhere, e.g. reducing exposes and stuff :)
	 */
	item_list = pango_itemize(gui.text_context,
				  (const char *)s, 0, len, attr_list, NULL);

	while (item_list != NULL)
	{
	    PangoItem	*item;
	    int		item_cells = 0; /* item length in cells */

	    item = (PangoItem *)item_list->data;
	    item_list = g_list_delete_link(item_list, item_list);
	    /*
	     * Increment the bidirectional embedding level by 1 if it is not
	     * even.  An odd number means the output will be RTL, but we don't
	     * want that since Vim handles right-to-left text on its own.  It
	     * would probably be sufficient to just set level = 0, but you can
	     * never know :)
	     *
	     * Unfortunately we can't take advantage of Pango's ability to
	     * render both LTR and RTL at the same time.  In order to support
	     * that, Vim's main screen engine would have to make use of Pango
	     * functionality.
	     */
	    item->analysis.level = (item->analysis.level + 1) & (~1U);

	    /* HACK: Overrule the shape engine, we don't want shaping to be
	     * done, because drawing the cursor would change the display. */
	    item->analysis.shape_engine = default_shape_engine;

	    pango_shape((const char *)s + item->offset, item->length,
			&item->analysis, glyphs);
	    /*
	     * Fixed-width hack: iterate over the array and assign a fixed
	     * width to each glyph, thus overriding the choice made by the
	     * shaping engine.	We use utf_char2cells() to determine the
	     * number of cells needed.
	     *
	     * Also perform all kind of dark magic to get composing
	     * characters right (and pretty too of course).
	     */
	    for (i = 0; i < glyphs->num_glyphs; ++i)
	    {
		PangoGlyphInfo *glyph;

		glyph = &glyphs->glyphs[i];

		if (glyph->attr.is_cluster_start)
		{
		    int cellcount;

		    cellcount = count_cluster_cells(
			s, item, glyphs, i, &cluster_width,
			(item_list != NULL) ? &last_glyph_rbearing : NULL);

		    if (cellcount > 0)
		    {
			int width;

			width = cellcount * gui.char_width * PANGO_SCALE;
			glyph->geometry.x_offset +=
					    MAX(0, width - cluster_width) / 2;
			glyph->geometry.width = width;
		    }
		    else
		    {
			/* If there are only combining characters in the
			 * cluster, we cannot just change the width of the
			 * previous glyph since there is none.	Therefore
			 * some guesswork is needed. */
			setup_zero_width_cluster(item, glyph, cells,
						 cluster_width,
						 last_glyph_rbearing);
		    }

		    item_cells += cellcount;
		    cells = cellcount;
		}
		else if (i > 0)
		{
		    int width;

		    /* There is a previous glyph, so we deal with combining
		     * characters the canonical way.  That is, setting the
		     * width of the previous glyph to 0. */
		    glyphs->glyphs[i - 1].geometry.width = 0;
		    width = cells * gui.char_width * PANGO_SCALE;
		    glyph->geometry.x_offset +=
					    MAX(0, width - cluster_width) / 2;
#if 0
		    /* Dirty hack: for "monospace 13" font there is a bug that
		     * draws composing chars in the wrong position.  Add
		     * "width" to the offset to work around that. */
		    if (monospace13)
			glyph->geometry.x_offset = width;
#endif

		    glyph->geometry.width = width;
		}
		else /* i == 0 "cannot happen" */
		{
		    glyph->geometry.width = 0;
		}
	    }

	    /*** Aaaaand action! ***/
	    draw_glyph_string(row, col + column_offset, item_cells,
			      flags, item->analysis.font, glyphs);

	    pango_item_free(item);

	    column_offset += item_cells;
	}

	pango_attr_list_unref(attr_list);
    }

skipitall:
    /* Draw underline and undercurl. */
    draw_under(flags, row, col, column_offset);

    pango_glyph_string_free(glyphs);
    vim_free(conv_buf);

    gdk_gc_set_clip_rectangle(gui.text_gc, NULL);

    return column_offset;
}
#endif /* HAVE_GTK2 */

#if !defined(HAVE_GTK2) || defined(PROTO)
    void
gui_mch_draw_string(int row, int col, char_u *s, int len, int flags)
{
    static XChar2b	*buf = NULL;
    static int		buflen = 0;
    int			is_wide;
    XChar2b		*text;
    int			textlen;
    XFontStruct		*xfont;
    char_u		*p;
# ifdef FEAT_MBYTE
    unsigned		c;
# endif
    int			width;

    if (gui.current_font == NULL || gui.drawarea->window == NULL)
	return;

    /*
     * Yeah yeah apparently the font support in GTK+ 1.2 only cares for either:
     * asians or 8-bit fonts. It is broken there, but no wonder the whole font
     * stuff is broken in X11 in first place. And the internationalization API
     * isn't something you would really like to use.
     */

    xfont = (XFontStruct *)((GdkFontPrivate*)gui.current_font)->xfont;
    is_wide = ((xfont->min_byte1 != 0 || xfont->max_byte1 != 0)
# ifdef FEAT_XFONTSET
	    && gui.fontset == NOFONTSET
# endif
	    );

    if (is_wide)
    {
	/* Convert a byte sequence to 16 bit characters for the Gdk functions.
	 * Need a buffer for the 16 bit characters.  Keep it between calls,
	 * because allocating it each time is slow. */
	if (buflen < len)
	{
	    XtFree((char *)buf);
	    buf = (XChar2b *)XtMalloc(len * sizeof(XChar2b));
	    buflen = len;
	}

	p = s;
	textlen = 0;
	width = 0;
	while (p < s + len)
	{
# ifdef FEAT_MBYTE
	    if (enc_utf8)
	    {
		c = utf_ptr2char(p);
		if (c >= 0x10000)	/* show chars > 0xffff as ? */
		    c = 0xbf;
		buf[textlen].byte1 = c >> 8;
		buf[textlen].byte2 = c;
		p += utf_ptr2len(p);
		width += utf_char2cells(c);
	    }
	    else
# endif
	    {
		buf[textlen].byte1 = '\0';	/* high eight bits */
		buf[textlen].byte2 = *p;	/* low eight bits */
		++p;
		++width;
	    }
	    ++textlen;
	}
	text = buf;
	textlen = textlen * 2;
    }
    else
    {
	text = (XChar2b *)s;
	textlen = len;
# ifdef FEAT_MBYTE
	if (has_mbyte)
	{
	    width = 0;
	    for (p = s; p < s + len; p += (*mb_ptr2len)(p))
		width += (*mb_ptr2cells)(p);
	}
	else
# endif
	    width = len;
    }

    if (!(flags & DRAW_TRANSP))
    {
	gdk_gc_set_foreground(gui.text_gc, gui.bgcolor);
	gdk_draw_rectangle(gui.drawarea->window,
			   gui.text_gc,
			   TRUE,
			   FILL_X(col), FILL_Y(row),
			   width * gui.char_width, gui.char_height);
    }
    gdk_gc_set_foreground(gui.text_gc, gui.fgcolor);
    gdk_draw_text(gui.drawarea->window,
		  gui.current_font,
		  gui.text_gc,
		  TEXT_X(col), TEXT_Y(row),
		  (const gchar *)text, textlen);

    /* redraw the contents with an offset of 1 to emulate bold */
    if (flags & DRAW_BOLD)
	gdk_draw_text(gui.drawarea->window,
		      gui.current_font,
		      gui.text_gc,
		      TEXT_X(col) + 1, TEXT_Y(row),
		      (const gchar *)text, textlen);

    /* Draw underline and undercurl. */
    draw_under(flags, row, col, width);
}
#endif /* !HAVE_GTK2 */

/*
 * Return OK if the key with the termcap name "name" is supported.
 */
    int
gui_mch_haskey(char_u *name)
{
    int i;

    for (i = 0; special_keys[i].key_sym != 0; i++)
	if (name[0] == special_keys[i].code0
		&& name[1] == special_keys[i].code1)
	    return OK;
    return FAIL;
}

#if defined(FEAT_TITLE) \
	|| (defined(FEAT_XIM) && !defined(HAVE_GTK2)) \
	|| defined(PROTO)
/*
 * Return the text window-id and display.  Only required for X-based GUI's
 */
    int
gui_get_x11_windis(Window *win, Display **dis)
{
    if (gui.mainwin != NULL && gui.mainwin->window != NULL)
    {
	*dis = GDK_WINDOW_XDISPLAY(gui.mainwin->window);
	*win = GDK_WINDOW_XWINDOW(gui.mainwin->window);
	return OK;
    }

    *dis = NULL;
    *win = 0;
    return FAIL;
}
#endif

#if defined(FEAT_CLIENTSERVER) \
	|| (defined(FEAT_X11) && defined(FEAT_CLIPBOARD)) || defined(PROTO)

    Display *
gui_mch_get_display(void)
{
    if (gui.mainwin != NULL && gui.mainwin->window != NULL)
	return GDK_WINDOW_XDISPLAY(gui.mainwin->window);
    else
	return NULL;
}
#endif

    void
gui_mch_beep(void)
{
#ifdef HAVE_GTK_MULTIHEAD
    GdkDisplay *display;

    if (gui.mainwin != NULL && GTK_WIDGET_REALIZED(gui.mainwin))
	display = gtk_widget_get_display(gui.mainwin);
    else
	display = gdk_display_get_default();

    if (display != NULL)
	gdk_display_beep(display);
#else
    gdk_beep();
#endif
}

    void
gui_mch_flash(int msec)
{
    GdkGCValues	values;
    GdkGC	*invert_gc;

    if (gui.drawarea->window == NULL)
	return;

    values.foreground.pixel = gui.norm_pixel ^ gui.back_pixel;
    values.background.pixel = gui.norm_pixel ^ gui.back_pixel;
    values.function = GDK_XOR;
    invert_gc = gdk_gc_new_with_values(gui.drawarea->window,
				       &values,
				       GDK_GC_FOREGROUND |
				       GDK_GC_BACKGROUND |
				       GDK_GC_FUNCTION);
    gdk_gc_set_exposures(invert_gc,
			 gui.visibility != GDK_VISIBILITY_UNOBSCURED);
    /*
     * Do a visual beep by changing back and forth in some undetermined way,
     * the foreground and background colors.  This is due to the fact that
     * there can't be really any prediction about the effects of XOR on
     * arbitrary X11 servers. However this seems to be enough for what we
     * intend it to do.
     */
    gdk_draw_rectangle(gui.drawarea->window, invert_gc,
		       TRUE,
		       0, 0,
		       FILL_X((int)Columns) + gui.border_offset,
		       FILL_Y((int)Rows) + gui.border_offset);

    gui_mch_flush();
    ui_delay((long)msec, TRUE);	/* wait so many msec */

    gdk_draw_rectangle(gui.drawarea->window, invert_gc,
		       TRUE,
		       0, 0,
		       FILL_X((int)Columns) + gui.border_offset,
		       FILL_Y((int)Rows) + gui.border_offset);

    gdk_gc_destroy(invert_gc);
}

/*
 * Invert a rectangle from row r, column c, for nr rows and nc columns.
 */
    void
gui_mch_invert_rectangle(int r, int c, int nr, int nc)
{
    GdkGCValues values;
    GdkGC *invert_gc;

    if (gui.drawarea->window == NULL)
	return;

    values.foreground.pixel = gui.norm_pixel ^ gui.back_pixel;
    values.background.pixel = gui.norm_pixel ^ gui.back_pixel;
    values.function = GDK_XOR;
    invert_gc = gdk_gc_new_with_values(gui.drawarea->window,
				       &values,
				       GDK_GC_FOREGROUND |
				       GDK_GC_BACKGROUND |
				       GDK_GC_FUNCTION);
    gdk_gc_set_exposures(invert_gc, gui.visibility !=
						   GDK_VISIBILITY_UNOBSCURED);
    gdk_draw_rectangle(gui.drawarea->window, invert_gc,
		       TRUE,
		       FILL_X(c), FILL_Y(r),
		       (nc) * gui.char_width, (nr) * gui.char_height);
    gdk_gc_destroy(invert_gc);
}

/*
 * Iconify the GUI window.
 */
    void
gui_mch_iconify(void)
{
#ifdef HAVE_GTK2
    gtk_window_iconify(GTK_WINDOW(gui.mainwin));
#else
    XIconifyWindow(GDK_WINDOW_XDISPLAY(gui.mainwin->window),
		   GDK_WINDOW_XWINDOW(gui.mainwin->window),
		   DefaultScreen(GDK_WINDOW_XDISPLAY(gui.mainwin->window)));
#endif
}

#if defined(FEAT_EVAL) || defined(PROTO)
/*
 * Bring the Vim window to the foreground.
 */
    void
gui_mch_set_foreground(void)
{
# ifdef HAVE_GTK2
    gtk_window_present(GTK_WINDOW(gui.mainwin));
# else
    gdk_window_raise(gui.mainwin->window);
# endif
}
#endif

/*
 * Draw a cursor without focus.
 */
    void
gui_mch_draw_hollow_cursor(guicolor_T color)
{
    int		i = 1;

    if (gui.drawarea->window == NULL)
	return;

    gui_mch_set_fg_color(color);

    gdk_gc_set_foreground(gui.text_gc, gui.fgcolor);
#ifdef FEAT_MBYTE
    if (mb_lefthalve(gui.row, gui.col))
	i = 2;
#endif
    gdk_draw_rectangle(gui.drawarea->window, gui.text_gc,
	    FALSE,
	    FILL_X(gui.col), FILL_Y(gui.row),
	    i * gui.char_width - 1, gui.char_height - 1);
}

/*
 * Draw part of a cursor, "w" pixels wide, and "h" pixels high, using
 * color "color".
 */
    void
gui_mch_draw_part_cursor(int w, int h, guicolor_T color)
{
    if (gui.drawarea->window == NULL)
	return;

    gui_mch_set_fg_color(color);

    gdk_gc_set_foreground(gui.text_gc, gui.fgcolor);
    gdk_draw_rectangle(gui.drawarea->window, gui.text_gc,
	    TRUE,
#ifdef FEAT_RIGHTLEFT
	    /* vertical line should be on the right of current point */
	    CURSOR_BAR_RIGHT ? FILL_X(gui.col + 1) - w :
#endif
	    FILL_X(gui.col),
	    FILL_Y(gui.row) + gui.char_height - h,
	    w, h);
}


/*
 * Catch up with any queued X11 events.  This may put keyboard input into the
 * input buffer, call resize call-backs, trigger timers etc.  If there is
 * nothing in the X11 event queue (& no timers pending), then we return
 * immediately.
 */
    void
gui_mch_update(void)
{
    while (gtk_events_pending() && !vim_is_input_buf_full())
	gtk_main_iteration_do(FALSE);
}

    static gint
input_timer_cb(gpointer data)
{
    int *timed_out = (int *) data;

    /* Just inform the caller about the occurrence of it */
    *timed_out = TRUE;

    if (gtk_main_level() > 0)
	gtk_main_quit();

    return FALSE;		/* don't happen again */
}

#ifdef FEAT_SNIFF
/*
 * Callback function, used when data is available on the SNiFF connection.
 */
/* ARGSUSED */
    static void
sniff_request_cb(
    gpointer	data,
    gint	source_fd,
    GdkInputCondition condition)
{
    static char_u bytes[3] = {CSI, (int)KS_EXTRA, (int)KE_SNIFF};

    add_to_input_buf(bytes, 3);

    if (gtk_main_level() > 0)
	gtk_main_quit();
}
#endif

/*
 * GUI input routine called by gui_wait_for_chars().  Waits for a character
 * from the keyboard.
 *  wtime == -1     Wait forever.
 *  wtime == 0	    This should never happen.
 *  wtime > 0	    Wait wtime milliseconds for a character.
 * Returns OK if a character was found to be available within the given time,
 * or FAIL otherwise.
 */
    int
gui_mch_wait_for_chars(long wtime)
{
    int focus;
    guint timer;
    static int timed_out;
#ifdef FEAT_SNIFF
    static int	sniff_on = 0;
    static gint	sniff_input_id = 0;
#endif

#ifdef FEAT_SNIFF
    if (sniff_on && !want_sniff_request)
    {
	if (sniff_input_id)
	    gdk_input_remove(sniff_input_id);
	sniff_on = 0;
    }
    else if (!sniff_on && want_sniff_request)
    {
	/* Add fd_from_sniff to watch for available data in main loop. */
	sniff_input_id = gdk_input_add(fd_from_sniff,
			       GDK_INPUT_READ, sniff_request_cb, NULL);
	sniff_on = 1;
    }
#endif

    timed_out = FALSE;

    /* this timeout makes sure that we will return if no characters arrived in
     * time */

    if (wtime > 0)
	timer = gtk_timeout_add((guint32)wtime, input_timer_cb, &timed_out);
    else
	timer = 0;

    focus = gui.in_focus;

    do
    {
	/* Stop or start blinking when focus changes */
	if (gui.in_focus != focus)
	{
	    if (gui.in_focus)
		gui_mch_start_blink();
	    else
		gui_mch_stop_blink();
	    focus = gui.in_focus;
	}

#if defined(FEAT_NETBEANS_INTG)
	/* Process the queued netbeans messages. */
	netbeans_parse_messages();
#endif

	/*
	 * Loop in GTK+ processing  until a timeout or input occurs.
	 * Skip this if input is available anyway (can happen in rare
	 * situations, sort of race condition).
	 */
	if (!input_available())
	    gtk_main();

	/* Got char, return immediately */
	if (input_available())
	{
	    if (timer != 0 && !timed_out)
		gtk_timeout_remove(timer);
	    return OK;
	}
    } while (wtime < 0 || !timed_out);

    /*
     * Flush all eventually pending (drawing) events.
     */
    gui_mch_update();

    return FAIL;
}


/****************************************************************************
 * Output drawing routines.
 ****************************************************************************/


/* Flush any output to the screen */
    void
gui_mch_flush(void)
{
#ifdef HAVE_GTK_MULTIHEAD
    if (gui.mainwin != NULL && GTK_WIDGET_REALIZED(gui.mainwin))
	gdk_display_sync(gtk_widget_get_display(gui.mainwin));
#else
    gdk_flush(); /* historical misnomer: calls XSync(), not XFlush() */
#endif
#ifdef HAVE_GTK2
    /* This happens to actually do what gui_mch_flush() is supposed to do,
     * according to the comment above. */
    if (gui.drawarea != NULL && gui.drawarea->window != NULL)
	gdk_window_process_updates(gui.drawarea->window, FALSE);
#endif
}

/*
 * Clear a rectangular region of the screen from text pos (row1, col1) to
 * (row2, col2) inclusive.
 */
    void
gui_mch_clear_block(int row1, int col1, int row2, int col2)
{
    GdkColor color;

    if (gui.drawarea->window == NULL)
	return;

    color.pixel = gui.back_pixel;

    gdk_gc_set_foreground(gui.text_gc, &color);

    /* Clear one extra pixel at the far right, for when bold characters have
     * spilled over to the window border. */
    gdk_draw_rectangle(gui.drawarea->window, gui.text_gc, TRUE,
		       FILL_X(col1), FILL_Y(row1),
		       (col2 - col1 + 1) * gui.char_width
						      + (col2 == Columns - 1),
		       (row2 - row1 + 1) * gui.char_height);
}

    void
gui_mch_clear_all(void)
{
    if (gui.drawarea->window != NULL)
	gdk_window_clear(gui.drawarea->window);
}

/*
 * Redraw any text revealed by scrolling up/down.
 */
    static void
check_copy_area(void)
{
    GdkEvent	*event;
    int		expose_count;

    if (gui.visibility != GDK_VISIBILITY_PARTIAL)
	return;

    /* Avoid redrawing the cursor while scrolling or it'll end up where
     * we don't want it to be.	I'm not sure if it's correct to call
     * gui_dont_update_cursor() at this point but it works as a quick
     * fix for now. */
    gui_dont_update_cursor();

    do
    {
	/* Wait to check whether the scroll worked or not. */
	event = gdk_event_get_graphics_expose(gui.drawarea->window);

	if (event == NULL)
	    break; /* received NoExpose event */

	gui_redraw(event->expose.area.x, event->expose.area.y,
		   event->expose.area.width, event->expose.area.height);

	expose_count = event->expose.count;
	gdk_event_free(event);
    }
    while (expose_count > 0); /* more events follow */

    gui_can_update_cursor();
}

/*
 * Delete the given number of lines from the given row, scrolling up any
 * text further down within the scroll region.
 */
    void
gui_mch_delete_lines(int row, int num_lines)
{
    if (gui.visibility == GDK_VISIBILITY_FULLY_OBSCURED)
	return;			/* Can't see the window */

    gdk_gc_set_foreground(gui.text_gc, gui.fgcolor);
    gdk_gc_set_background(gui.text_gc, gui.bgcolor);

    /* copy one extra pixel, for when bold has spilled over */
    gdk_window_copy_area(gui.drawarea->window, gui.text_gc,
	    FILL_X(gui.scroll_region_left), FILL_Y(row),
	    gui.drawarea->window,
	    FILL_X(gui.scroll_region_left),
	    FILL_Y(row + num_lines),
	    gui.char_width * (gui.scroll_region_right
					    - gui.scroll_region_left + 1) + 1,
	    gui.char_height * (gui.scroll_region_bot - row - num_lines + 1));

    gui_clear_block(gui.scroll_region_bot - num_lines + 1,
						       gui.scroll_region_left,
		    gui.scroll_region_bot, gui.scroll_region_right);
    check_copy_area();
}

/*
 * Insert the given number of lines before the given row, scrolling down any
 * following text within the scroll region.
 */
    void
gui_mch_insert_lines(int row, int num_lines)
{
    if (gui.visibility == GDK_VISIBILITY_FULLY_OBSCURED)
	return;			/* Can't see the window */

    gdk_gc_set_foreground(gui.text_gc, gui.fgcolor);
    gdk_gc_set_background(gui.text_gc, gui.bgcolor);

    /* copy one extra pixel, for when bold has spilled over */
    gdk_window_copy_area(gui.drawarea->window, gui.text_gc,
	    FILL_X(gui.scroll_region_left), FILL_Y(row + num_lines),
	    gui.drawarea->window,
	    FILL_X(gui.scroll_region_left), FILL_Y(row),
	    gui.char_width * (gui.scroll_region_right
					    - gui.scroll_region_left + 1) + 1,
	    gui.char_height * (gui.scroll_region_bot - row - num_lines + 1));

    gui_clear_block(row, gui.scroll_region_left,
				row + num_lines - 1, gui.scroll_region_right);
    check_copy_area();
}

/*
 * X Selection stuff, for cutting and pasting text to other windows.
 */
    void
clip_mch_request_selection(VimClipboard *cbd)
{
    GdkAtom	target;
    unsigned	i;
    int		nbytes;
    char_u	*buffer;
    time_t	start;

    for (i = 0; i < N_SELECTION_TARGETS; ++i)
    {
	received_selection = RS_NONE;
	target = gdk_atom_intern(selection_targets[i].target, FALSE);

	gtk_selection_convert(gui.drawarea,
			      cbd->gtk_sel_atom, target,
			      (guint32)GDK_CURRENT_TIME);

	/* Hack: Wait up to three seconds for the selection.  A hang was
	 * noticed here when using the netrw plugin combined with ":gui"
	 * during the FocusGained event. */
	start = time(NULL);
	while (received_selection == RS_NONE && time(NULL) < start + 3)
	    gtk_main();	/* wait for selection_received_cb */

	if (received_selection != RS_FAIL)
	    return;
    }

    /* Final fallback position - use the X CUT_BUFFER0 store */
    nbytes = 0;
    buffer = (char_u *)XFetchBuffer(GDK_WINDOW_XDISPLAY(gui.mainwin->window),
				    &nbytes, 0);
    if (nbytes > 0)
    {
	/* Got something */
	clip_yank_selection(MCHAR, buffer, (long)nbytes, cbd);
	if (p_verbose > 0)
	{
	    verbose_enter();
	    smsg((char_u *)_("Used CUT_BUFFER0 instead of empty selection"));
	    verbose_leave();
	}
    }
    if (buffer != NULL)
	XFree(buffer);
}

/*
 * Disown the selection.
 */
/*ARGSUSED*/
    void
clip_mch_lose_selection(VimClipboard *cbd)
{
    /* WEIRD: when using NULL to actually disown the selection, we lose the
     * selection the first time we own it. */
    /*
    gtk_selection_owner_set(NULL, cbd->gtk_sel_atom, (guint32)GDK_CURRENT_TIME);
    gui_mch_update();
     */
}

/*
 * Own the selection and return OK if it worked.
 */
    int
clip_mch_own_selection(VimClipboard *cbd)
{
    int success;

    success = gtk_selection_owner_set(gui.drawarea, cbd->gtk_sel_atom,
				      (guint32)GDK_CURRENT_TIME);
    gui_mch_update();
    return (success) ? OK : FAIL;
}

/*
 * Send the current selection to the clipboard.  Do nothing for X because we
 * will fill in the selection only when requested by another app.
 */
/*ARGSUSED*/
    void
clip_mch_set_selection(VimClipboard *cbd)
{
}


#if defined(FEAT_MENU) || defined(PROTO)
/*
 * Make a menu item appear either active or not active (grey or not grey).
 */
    void
gui_mch_menu_grey(vimmenu_T *menu, int grey)
{
    if (menu->id == NULL)
	return;

    if (menu_is_separator(menu->name))
	grey = TRUE;

    gui_mch_menu_hidden(menu, FALSE);
    /* Be clever about bitfields versus true booleans here! */
    if (!GTK_WIDGET_SENSITIVE(menu->id) == !grey)
    {
	gtk_widget_set_sensitive(menu->id, !grey);
	gui_mch_update();
    }
}

/*
 * Make menu item hidden or not hidden.
 */
    void
gui_mch_menu_hidden(vimmenu_T *menu, int hidden)
{
    if (menu->id == 0)
	return;

    if (hidden)
    {
	if (GTK_WIDGET_VISIBLE(menu->id))
	{
	    gtk_widget_hide(menu->id);
	    gui_mch_update();
	}
    }
    else
    {
	if (!GTK_WIDGET_VISIBLE(menu->id))
	{
	    gtk_widget_show(menu->id);
	    gui_mch_update();
	}
    }
}

/*
 * This is called after setting all the menus to grey/hidden or not.
 */
    void
gui_mch_draw_menubar(void)
{
    /* just make sure that the visual changes get effect immediately */
    gui_mch_update();
}
#endif /* FEAT_MENU */

/*
 * Scrollbar stuff.
 */
    void
gui_mch_enable_scrollbar(scrollbar_T *sb, int flag)
{
    if (sb->id == NULL)
	return;

    if (flag)
	gtk_widget_show(sb->id);
    else
	gtk_widget_hide(sb->id);

    update_window_manager_hints(0, 0);
}


/*
 * Return the RGB value of a pixel as long.
 */
    long_u
gui_mch_get_rgb(guicolor_T pixel)
{
    GdkColor color;
#ifndef HAVE_GTK2
    GdkColorContext *cc;

    cc = gdk_color_context_new(gtk_widget_get_visual(gui.drawarea),
			       gtk_widget_get_colormap(gui.drawarea));
    color.pixel = pixel;
    gdk_color_context_query_color(cc, &color);

    gdk_color_context_free(cc);
#else
    gdk_colormap_query_color(gtk_widget_get_colormap(gui.drawarea),
			     (unsigned long)pixel, &color);
#endif

    return (((unsigned)color.red   & 0xff00) << 8)
	 |  ((unsigned)color.green & 0xff00)
	 | (((unsigned)color.blue  & 0xff00) >> 8);
}

/*
 * Get current mouse coordinates in text window.
 */
    void
gui_mch_getmouse(int *x, int *y)
{
    gdk_window_get_pointer(gui.drawarea->window, x, y, NULL);
}

    void
gui_mch_setmouse(int x, int y)
{
    /* Sorry for the Xlib call, but we can't avoid it, since there is no
     * internal GDK mechanism present to accomplish this.  (and for good
     * reason...) */
    XWarpPointer(GDK_WINDOW_XDISPLAY(gui.drawarea->window),
		 (Window)0, GDK_WINDOW_XWINDOW(gui.drawarea->window),
		 0, 0, 0U, 0U, x, y);
}


#ifdef FEAT_MOUSESHAPE
/* The last set mouse pointer shape is remembered, to be used when it goes
 * from hidden to not hidden. */
static int last_shape = 0;
#endif

/*
 * Use the blank mouse pointer or not.
 *
 * hide: TRUE = use blank ptr, FALSE = use parent ptr
 */
    void
gui_mch_mousehide(int hide)
{
    if (gui.pointer_hidden != hide)
    {
	gui.pointer_hidden = hide;
	if (gui.drawarea->window && gui.blank_pointer != NULL)
	{
	    if (hide)
		gdk_window_set_cursor(gui.drawarea->window, gui.blank_pointer);
	    else
#ifdef FEAT_MOUSESHAPE
		mch_set_mouse_shape(last_shape);
#else
		gdk_window_set_cursor(gui.drawarea->window, NULL);
#endif
	}
    }
}

#if defined(FEAT_MOUSESHAPE) || defined(PROTO)

/* Table for shape IDs.  Keep in sync with the mshape_names[] table in
 * misc2.c! */
static const int mshape_ids[] =
{
    GDK_LEFT_PTR,		/* arrow */
    GDK_CURSOR_IS_PIXMAP,	/* blank */
    GDK_XTERM,			/* beam */
    GDK_SB_V_DOUBLE_ARROW,	/* updown */
    GDK_SIZING,			/* udsizing */
    GDK_SB_H_DOUBLE_ARROW,	/* leftright */
    GDK_SIZING,			/* lrsizing */
    GDK_WATCH,			/* busy */
    GDK_X_CURSOR,		/* no */
    GDK_CROSSHAIR,		/* crosshair */
    GDK_HAND1,			/* hand1 */
    GDK_HAND2,			/* hand2 */
    GDK_PENCIL,			/* pencil */
    GDK_QUESTION_ARROW,		/* question */
    GDK_RIGHT_PTR,		/* right-arrow */
    GDK_CENTER_PTR,		/* up-arrow */
    GDK_LEFT_PTR		/* last one */
};

    void
mch_set_mouse_shape(int shape)
{
    int		   id;
    GdkCursor	   *c;

    if (gui.drawarea->window == NULL)
	return;

    if (shape == MSHAPE_HIDE || gui.pointer_hidden)
	gdk_window_set_cursor(gui.drawarea->window, gui.blank_pointer);
    else
    {
	if (shape >= MSHAPE_NUMBERED)
	{
	    id = shape - MSHAPE_NUMBERED;
	    if (id >= GDK_LAST_CURSOR)
		id = GDK_LEFT_PTR;
	    else
		id &= ~1;	/* they are always even (why?) */
	}
	else if (shape < sizeof(mshape_ids) / sizeof(int))
	    id = mshape_ids[shape];
	else
	    return;
# ifdef HAVE_GTK_MULTIHEAD
	c = gdk_cursor_new_for_display(
		gtk_widget_get_display(gui.drawarea), (GdkCursorType)id);
# else
	c = gdk_cursor_new((GdkCursorType)id);
# endif
	gdk_window_set_cursor(gui.drawarea->window, c);
	gdk_cursor_destroy(c); /* Unref, actually.  Bloody GTK+ 1. */
    }
    if (shape != MSHAPE_HIDE)
	last_shape = shape;
}
#endif /* FEAT_MOUSESHAPE */


#if defined(FEAT_SIGN_ICONS) || defined(PROTO)
/*
 * Signs are currently always 2 chars wide.  With GTK+ 2, the image will be
 * scaled down if the current font is not big enough, or scaled up if the image
 * size is less than 3/4 of the maximum sign size.  With GTK+ 1, the pixmap
 * will be cut off if the current font is not big enough, or centered if it's
 * too small.
 */
# define SIGN_WIDTH  (2 * gui.char_width)
# define SIGN_HEIGHT (gui.char_height)
# define SIGN_ASPECT ((double)SIGN_HEIGHT / (double)SIGN_WIDTH)

# ifdef HAVE_GTK2

    void
gui_mch_drawsign(int row, int col, int typenr)
{
    GdkPixbuf *sign;

    sign = (GdkPixbuf *)sign_get_image(typenr);

    if (sign != NULL && gui.drawarea != NULL && gui.drawarea->window != NULL)
    {
	int width;
	int height;
	int xoffset;
	int yoffset;
	int need_scale;

	width  = gdk_pixbuf_get_width(sign);
	height = gdk_pixbuf_get_height(sign);
	/*
	 * Decide whether we need to scale.  Allow one pixel of border
	 * width to be cut off, in order to avoid excessive scaling for
	 * tiny differences in font size.
	 */
	need_scale = (width > SIGN_WIDTH + 2
		      || height > SIGN_HEIGHT + 2
		      || (width < 3 * SIGN_WIDTH / 4
			  && height < 3 * SIGN_HEIGHT / 4));
	if (need_scale)
	{
	    double aspect;

	    /* Keep the original aspect ratio */
	    aspect = (double)height / (double)width;
	    width  = (double)SIGN_WIDTH * SIGN_ASPECT / aspect;
	    width  = MIN(width, SIGN_WIDTH);
	    height = (double)width * aspect;

	    /* This doesn't seem to be worth caching, and doing so
	     * would complicate the code quite a bit. */
	    sign = gdk_pixbuf_scale_simple(sign, width, height,
					   GDK_INTERP_BILINEAR);
	    if (sign == NULL)
		return; /* out of memory */
	}

	/* The origin is the upper-left corner of the pixmap.  Therefore
	 * these offset may become negative if the pixmap is smaller than
	 * the 2x1 cells reserved for the sign icon. */
	xoffset = (width  - SIGN_WIDTH)  / 2;
	yoffset = (height - SIGN_HEIGHT) / 2;

	gdk_gc_set_foreground(gui.text_gc, gui.bgcolor);

	gdk_draw_rectangle(gui.drawarea->window,
			   gui.text_gc,
			   TRUE,
			   FILL_X(col),
			   FILL_Y(row),
			   SIGN_WIDTH,
			   SIGN_HEIGHT);

#  if GTK_CHECK_VERSION(2,1,1)
	gdk_draw_pixbuf(gui.drawarea->window,
			NULL,
			sign,
			MAX(0, xoffset),
			MAX(0, yoffset),
			FILL_X(col) - MIN(0, xoffset),
			FILL_Y(row) - MIN(0, yoffset),
			MIN(width,  SIGN_WIDTH),
			MIN(height, SIGN_HEIGHT),
			GDK_RGB_DITHER_NORMAL,
			0, 0);
#  else
	gdk_pixbuf_render_to_drawable_alpha(sign,
					    gui.drawarea->window,
					    MAX(0, xoffset),
					    MAX(0, yoffset),
					    FILL_X(col) - MIN(0, xoffset),
					    FILL_Y(row) - MIN(0, yoffset),
					    MIN(width,	SIGN_WIDTH),
					    MIN(height, SIGN_HEIGHT),
					    GDK_PIXBUF_ALPHA_BILEVEL,
					    127,
					    GDK_RGB_DITHER_NORMAL,
					    0, 0);
#  endif
	if (need_scale)
	    g_object_unref(sign);
    }
}

    void *
gui_mch_register_sign(char_u *signfile)
{
    if (signfile[0] != NUL && signfile[0] != '-' && gui.in_use)
    {
	GdkPixbuf   *sign;
	GError	    *error = NULL;
	char_u	    *message;

	sign = gdk_pixbuf_new_from_file((const char *)signfile, &error);

	if (error == NULL)
	    return sign;

	message = (char_u *)error->message;

	if (message != NULL && input_conv.vc_type != CONV_NONE)
	    message = string_convert(&input_conv, message, NULL);

	if (message != NULL)
	{
	    /* The error message is already translated and will be more
	     * descriptive than anything we could possibly do ourselves. */
	    EMSG2("E255: %s", message);

	    if (input_conv.vc_type != CONV_NONE)
		vim_free(message);
	}
	g_error_free(error);
    }

    return NULL;
}

    void
gui_mch_destroy_sign(void *sign)
{
    if (sign != NULL)
	g_object_unref(sign);
}

# else /* !HAVE_GTK2 */

typedef struct
{
    GdkPixmap *pixmap;
    GdkBitmap *mask;
}
signicon_T;

    void
gui_mch_drawsign(int row, int col, int typenr)
{
    signicon_T *sign;

    sign = (signicon_T *)sign_get_image(typenr);

    if (sign != NULL && sign->pixmap != NULL
	&& gui.drawarea != NULL && gui.drawarea->window != NULL)
    {
	int width;
	int height;
	int xoffset;
	int yoffset;

	gdk_window_get_size(sign->pixmap, &width, &height);

	/* The origin is the upper-left corner of the pixmap.  Therefore
	 * these offset may become negative if the pixmap is smaller than
	 * the 2x1 cells reserved for the sign icon. */
	xoffset = (width  - SIGN_WIDTH)  / 2;
	yoffset = (height - SIGN_HEIGHT) / 2;

	gdk_gc_set_foreground(gui.text_gc, gui.bgcolor);

	gdk_draw_rectangle(gui.drawarea->window,
			   gui.text_gc,
			   TRUE,
			   FILL_X(col),
			   FILL_Y(row),
			   SIGN_WIDTH,
			   SIGN_HEIGHT);

	/* Set the clip mask for bilevel transparency */
	if (sign->mask != NULL)
	{
	    gdk_gc_set_clip_origin(gui.text_gc,
				   FILL_X(col) - xoffset,
				   FILL_Y(row) - yoffset);
	    gdk_gc_set_clip_mask(gui.text_gc, sign->mask);
	}

	gdk_draw_pixmap(gui.drawarea->window,
			gui.text_gc,
			sign->pixmap,
			MAX(0, xoffset),
			MAX(0, yoffset),
			FILL_X(col) - MIN(0, xoffset),
			FILL_Y(row) - MIN(0, yoffset),
			MIN(width,  SIGN_WIDTH),
			MIN(height, SIGN_HEIGHT));

	gdk_gc_set_clip_mask(gui.text_gc, NULL);
    }
}

    void *
gui_mch_register_sign(char_u *signfile)
{
    signicon_T *sign = NULL;

    if (signfile[0] != NUL && signfile[0] != '-'
	    && gui.drawarea != NULL && gui.drawarea->window != NULL)
    {
	sign = (signicon_T *)alloc(sizeof(signicon_T));

	if (sign != NULL) /* NULL == OOM == "cannot really happen" */
	{
	    sign->mask = NULL;
	    sign->pixmap = gdk_pixmap_colormap_create_from_xpm(
		    gui.drawarea->window, NULL,
		    &sign->mask, NULL,
		    (const char *)signfile);

	    if (sign->pixmap == NULL)
	    {
		vim_free(sign);
		sign = NULL;
		EMSG(_(e_signdata));
	    }
	}
    }
    return sign;
}

    void
gui_mch_destroy_sign(void *sign)
{
    if (sign != NULL)
    {
	signicon_T *signicon = (signicon_T *)sign;

	if (signicon->pixmap != NULL)
	    gdk_pixmap_unref(signicon->pixmap);
	if (signicon->mask != NULL)
	    gdk_bitmap_unref(signicon->mask);

	vim_free(signicon);
    }
}
# endif /* !HAVE_GTK2 */

#endif /* FEAT_SIGN_ICONS */
