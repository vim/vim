/* vi:set ts=8 sw=8:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *			Visual Workshop integration by Gordon Prieur
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * Integration with Sun Workshop.
 *
 * This file should not change much, it's also used by other editors that
 * connect to Workshop.  Consider changing workshop.c instead.
 */
/*
-> consider using MakeSelectionVisible instead of gotoLine hacks
   to show the line properly
     -> consider using glue instead of our own message wrapping functions
	(but can only use glue if we don't have to distribute source)
*/

#include "vim.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#ifdef INET_SOCKETS
#include <netdb.h>
#include <netinet/in.h>
#else
#include <sys/un.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#ifdef HAVE_LIBGEN_H
# include <libgen.h>
#endif
#include <unistd.h>
#include <string.h>

#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/AtomMgr.h>
#include <Xm/PushB.h>

#ifdef HAVE_X11_XPM_H
# include <X11/xpm.h>
#else
# ifdef HAVE_XM_XPMP_H
#  include <Xm/XpmP.h>
# endif
#endif

#ifdef HAVE_UTIL_DEBUG_H
# include <util/debug.h>
#endif
#ifdef HAVE_UTIL_MSGI18N_H
# include <util/msgi18n.h>
#endif

#include "integration.h"	/* <EditPlugin/integration.h> */
#ifdef HAVE_FRAME_H
# include <frame.h>
#endif

#ifndef MAX
# define MAX(a, b)    (a) > (b) ? (a) : (b)
#endif

#ifndef NOCATGETS
# define NOCATGETS(x) x
#endif

/* Functions private to this file */
static void workshop_connection_closed(void);
static void messageFromEserve(XtPointer clientData, int *NOTUSED1, XtInputId *NOTUSED2);
static void workshop_disconnect(void);
static void workshop_sensitivity(int num, char *table);
static void adjust_sign_name(char *filename);
static void process_menuItem(char *);
static void process_toolbarButton(char *);
static void workshop_set_option_first(char *name, char *value);


#define CMDBUFSIZ	2048

#ifdef DEBUG
static FILE *dfd;
static void pldebug(char *, ...);
static void unrecognised_message(char *);

#define HANDLE_ERRORS(cmd)	else unrecognised_message(cmd);
#else
#define HANDLE_ERRORS(cmd)
#endif

/*
 * Version number of the protocol between an editor and eserve.
 * This number should be incremented when the protocol
 * is changed.
 */
#define	PROTOCOL_VERSION	"4.0.0"

static int sd = -1;
static XtInputId inputHandler;		/* Cookie for input */

Boolean save_files = True;		/* When true, save all files before build actions */

void
workshop_connection_closed(void)
{
	/*
	 * socket closed on other end
	 */
	XtRemoveInput(inputHandler);
	inputHandler = 0;
	sd = -1;
}

	static char *
getCommand(void)
{
	int	 len;		/* length of this command */
	char	 lenbuf[7];	/* get the length string here */
	char	*newcb;		/* used to realloc cmdbuf */
	static char	*cmdbuf;/* get the command string here */
	static int	 cbsize;/* size of cmdbuf */

	if ((len = read(sd, &lenbuf, 6)) == 6) {
		lenbuf[6] = 0; /* Terminate buffer such that atoi() works right */
		len = atoi(lenbuf);
		if (cbsize < (len + 1)) {
			newcb = (char *) realloc(cmdbuf,
			    MAX((len + 256), CMDBUFSIZ));
			if (newcb != NULL) {
				cmdbuf = newcb;
				cbsize = MAX((len + 256), CMDBUFSIZ);
			}
		}
		if (cbsize >= len && (len = read(sd, cmdbuf, len)) > 0) {
			cmdbuf[len] = 0;
			return cmdbuf;
		} else {
			return NULL;
		}
	} else {
		if (len == 0) { /* EOF */
			workshop_connection_closed();
		}
		return NULL;
	}

}

/*ARGSUSED*/
void
messageFromEserve(XtPointer clientData, int *NOTUSED1, XtInputId *NOTUSED2)
{
	char	*cmd;		/* the 1st word of the command */

	cmd = getCommand();
	if (cmd == NULL) {
		/* We're being shut down by eserve and the "quit" message
		 * didn't arrive before the socket connection got closed */
		return;
	}
#ifdef DEBUG
	pldebug("%s\n", cmd);
#endif
	switch (*cmd) {
	case 'a':
		if (cmd[1] == 'c' &&
		    strncmp(cmd, NOCATGETS("ack "), 4) == 0) {
			int ackNum;
			char buf[20];

			ackNum = atoi(&cmd[4]);
			vim_snprintf(buf, sizeof(buf),
					       NOCATGETS("ack %d\n"), ackNum);
			write(sd, buf, strlen(buf));
		} else if (strncmp(cmd,
		    NOCATGETS("addMarkType "), 12) == 0) {
			int idx;
			char *color;
			char *sign;

			idx = atoi(strtok(&cmd[12], " "));
			color  = strtok(NULL, NOCATGETS("\001"));
			sign  = strtok(NULL, NOCATGETS("\001"));
			/* Skip space that separates names */
			if (color) {
				color++;
			}
			if (sign) {
				sign++;
			}
			/* Change sign name to accomodate a different size? */
			adjust_sign_name(sign);
			workshop_add_mark_type(idx, color, sign);
		}
		HANDLE_ERRORS(cmd);
		break;

	case 'b':
		if (strncmp(cmd,
		    NOCATGETS("balloon "), 8) == 0) {
			char *tip;

			tip  = strtok(&cmd[8], NOCATGETS("\001"));
			workshop_show_balloon_tip(tip);
		}
		HANDLE_ERRORS(cmd);
		break;

	case 'c':
		if (strncmp(cmd,
		    NOCATGETS("changeMarkType "), 15) == 0) {
			char *file;
			int markId;
			int type;

			file  = strtok(&cmd[15], " ");
			markId = atoi(strtok(NULL, " "));
			type = atoi(strtok(NULL, " "));
			workshop_change_mark_type(file, markId, type);
		}
		HANDLE_ERRORS(cmd);
		break;

	case 'd':
		if (strncmp(cmd, NOCATGETS("deleteMark "), 11) == 0) {
			char *file;
			int markId;

			file  = strtok(&cmd[11], " ");
			markId = atoi(strtok(NULL, " "));
			workshop_delete_mark(file, markId);
		}
		HANDLE_ERRORS(cmd);
		break;

	case 'f':
		if (cmd[1] == 'o' &&
		    strncmp(cmd, NOCATGETS("footerMsg "), 10) == 0) {
			int severity;
			char *message;

			severity =
			    atoi(strtok(&cmd[10], " "));
			message = strtok(NULL, NOCATGETS("\001"));

			workshop_footer_message(message, severity);
		} else if (strncmp(cmd,
		    NOCATGETS("frontFile "), 10) == 0) {
			char *file;

			file  = strtok(&cmd[10], " ");
			workshop_front_file(file);
		}
		HANDLE_ERRORS(cmd);
		break;

	case 'g':
		if (cmd[1] == 'e' &&
		    strncmp(cmd, NOCATGETS("getMarkLine "), 12) == 0) {
			char *file;
			int markid;
			int line;
			char buf[100];

			file  = strtok(&cmd[12], " ");
			markid = atoi(strtok(NULL, " "));
			line = workshop_get_mark_lineno(file, markid);
			vim_snprintf(buf, sizeof(buf),
					     NOCATGETS("markLine %s %d %d\n"),
			    file, markid, line);
			write(sd, buf, strlen(buf));
		} else if (cmd[1] == 'o' && cmd[4] == 'L' &&
		    strncmp(cmd, NOCATGETS("gotoLine "), 9) == 0) {
			char *file;
			int lineno;

			file  = strtok(&cmd[9], " ");
			lineno = atoi(strtok(NULL, " "));
			workshop_goto_line(file, lineno);
		} else if (strncmp(cmd,
		    NOCATGETS("gotoMark "), 9) == 0) {
			char *file;
			int markId;
			char *message;

			file  = strtok(&cmd[9], " ");
			markId = atoi(strtok(NULL, " "));
			message = strtok(NULL, NOCATGETS("\001"));
			workshop_goto_mark(file, markId, message);
#ifdef NOHANDS_SUPPORT_FUNCTIONS
		} else if (strcmp(cmd, NOCATGETS("getCurrentFile")) == 0) {
			char *f = workshop_test_getcurrentfile();
			char buffer[2*MAXPATHLEN];
			vim_snprintf(buffer, sizeof(buffer),
					NOCATGETS("currentFile %d %s"),
				f ? strlen(f) : 0, f ? f : "");
			workshop_send_message(buffer);
		} else if (strcmp(cmd, NOCATGETS("getCursorRow")) == 0) {
			int row = workshop_test_getcursorrow();
			char buffer[2*MAXPATHLEN];
			vim_snprintf(buffer, sizeof(buffer),
					NOCATGETS("cursorRow %d"), row);
			workshop_send_message(buffer);
		} else if (strcmp(cmd, NOCATGETS("getCursorCol")) == 0) {
			int col = workshop_test_getcursorcol();
			char buffer[2*MAXPATHLEN];
			vim_snprintf(buffer, sizeof(buffer),
					NOCATGETS("cursorCol %d"), col);
			workshop_send_message(buffer);
		} else if (strcmp(cmd, NOCATGETS("getCursorRowText")) == 0) {
			char *t = workshop_test_getcursorrowtext();
			char buffer[2*MAXPATHLEN];
			vim_snprintf(buffer, sizeof(buffer),
					NOCATGETS("cursorRowText %d %s"),
				t ? strlen(t) : 0, t ? t : "");
			workshop_send_message(buffer);
		} else if (strcmp(cmd, NOCATGETS("getSelectedText")) == 0) {
			char *t = workshop_test_getselectedtext();
			char buffer[2*MAXPATHLEN];
			vim_snprintf(buffer, sizeof(buffer),
					NOCATGETS("selectedText %d %s"),
				t ? strlen(t) : 0, t ? t : "");
			workshop_send_message(buffer);
#endif
		}
		HANDLE_ERRORS(cmd);
		break;

	case 'l':
		if (strncmp(cmd, NOCATGETS("loadFile "), 9) == 0) {
			char *file;
			int line;
			char *frameid;

			file  = strtok(&cmd[9], " ");
			line = atoi(strtok(NULL, " "));
			frameid = strtok(NULL, " ");
			workshop_load_file(file, line, frameid);
		}
		HANDLE_ERRORS(cmd);
		break;

	case 'm':			/* Menu, minimize, maximize */
		if (cmd[1] == 'e' && cmd[4] == 'B' &&
		    strncmp(cmd, NOCATGETS("menuBegin "), 10) == 0) {
			workshop_menu_begin(&cmd[10]);
		} else if (cmd[1] == 'e' && cmd[4] == 'I' &&
		    strncmp(cmd, NOCATGETS("menuItem "), 9) == 0) {
			process_menuItem(cmd);
		} else if (cmd[1] == 'e' && cmd[4] == 'E' &&
		    strcmp(cmd, NOCATGETS("menuEnd")) == 0) {
			workshop_menu_end();
		} else if (cmd[1] == 'a' &&
		    strcmp(cmd, NOCATGETS("maximize")) == 0) {
			workshop_maximize();
		} else if (strcmp(cmd, NOCATGETS("minimize")) == 0) {
			workshop_minimize();
		}
		HANDLE_ERRORS(cmd);
		break;

	case 'o':
		if (cmd[1] == 'p' &&
		    strcmp(cmd, NOCATGETS("option"))) {
			char *name;
			char *value;

			name  = strtok(&cmd[7], " ");
			value = strtok(NULL, " ");
			workshop_set_option_first(name, value);
		}
		HANDLE_ERRORS(cmd);
		break;

	case 'p':
		if (strcmp(cmd, NOCATGETS("ping")) == 0) {
#if 0
			int pingNum;

			pingNum = atoi(&cmd[5]);
			workshop_send_ack(ackNum);
			WHAT DO I DO HERE?
#endif
		}
		HANDLE_ERRORS(cmd);
		break;

	case 'q':
		if (strncmp(cmd, NOCATGETS("quit"), 4) == 0) {

			/* Close the connection. It's important to do
			 * that now, since workshop_quit might be
			 * looking at open files.  For example, if you
			 * have modified one of the files without
			 * saving, NEdit will ask you what you want to
			 * do, and spin loop by calling
			 * XtAppProcessEvent while waiting for your
			 * reply. In this case, if we still have an
			 * input handler and the socket has been
			 * closed on the other side when eserve
			 * expired, we will hang in IoWait.
			 */
			workshop_disconnect();

			workshop_quit();
		}
		HANDLE_ERRORS(cmd);
		break;

	case 'r':
		if (cmd[1] == 'e' &&
		    strncmp(cmd, NOCATGETS("reloadFile "), 11) == 0) {
			char *file;
			int line;

			file  = strtok(&cmd[11], " ");
			line = atoi(strtok(NULL, " "));
			workshop_reload_file(file, line);
		}
		HANDLE_ERRORS(cmd);
		break;

	case 's':
		if (cmd[1] == 'e' && cmd[2] == 't' &&
		    strncmp(cmd, NOCATGETS("setMark "), 8) == 0) {
			char *file;
			int line;
			int markId;
			int type;

			file  = strtok(&cmd[8], " ");
			line = atoi(strtok(NULL, " "));
			markId = atoi(strtok(NULL, " "));
			type = atoi(strtok(NULL, " "));
			workshop_set_mark(file, line, markId, type);
		} else if (cmd[1] == 'h' &&
		    strncmp(cmd, NOCATGETS("showFile "), 9) == 0) {
			workshop_show_file(&cmd[9]);
		} else if (cmd[1] == 'u' &&
		    strncmp(cmd, NOCATGETS("subMenu "), 8) == 0) {
			char *label;

			label  = strtok(&cmd[8], NOCATGETS("\001"));
			workshop_submenu_begin(label);
		} else if (cmd[1] == 'u' &&
		    strcmp(cmd, NOCATGETS("subMenuEnd")) == 0) {
			workshop_submenu_end();
		} else if (cmd[1] == 'e' && cmd[2] == 'n' &&
		    strncmp(cmd, NOCATGETS("sensitivity "), 12) == 0) {
			int num;
			char *bracket;
			char *table;

			num = atoi(strtok(&cmd[12], " "));
			bracket = strtok(NULL, " ");
			if (*bracket != '[') {
				fprintf(stderr, NOCATGETS("Parsing "
				    "error for sensitivity\n"));
			} else {
				table = strtok(NULL, NOCATGETS("]"));
				workshop_sensitivity(num, table);
			}
		} else if (cmd[1] == 'e' && cmd[2] == 'n' && cmd[3] == 'd' &&
			   strncmp(cmd, NOCATGETS("sendVerb "), 9) == 0) {
			/* Send the given verb back (used for the
			 * debug.lineno callback (such that other tools
			 * can obtain the position coordinates or the
			 * selection) */
			char *verb;

			verb = strtok(&cmd[9], " ");
			workshop_perform_verb(verb, NULL);
		} else if (cmd[1] == 'a' &&
		    strncmp(cmd, NOCATGETS("saveFile "), 9) == 0) {
			workshop_save_file(&cmd[9]);
#ifdef NOHANDS_SUPPORT_FUNCTIONS
		} else if (strncmp(cmd, NOCATGETS("saveSensitivity "), 16) == 0) {
			char *file;

			file  = strtok(&cmd[16], " ");
			workshop_save_sensitivity(file);
#endif
		}
		HANDLE_ERRORS(cmd);
		break;

	case 't':			/* Toolbar */
		if (cmd[8] == 'e' &&
		    strncmp(cmd, NOCATGETS("toolbarBegin"), 12) == 0) {
			workshop_toolbar_begin();
		} else if (cmd[8] == 'u' &&
		    strncmp(cmd, NOCATGETS("toolbarButton"), 13) == 0) {
			process_toolbarButton(cmd);
		} else if (cmd[7] == 'E' &&
		    strcmp(cmd, NOCATGETS("toolbarEnd")) == 0) {
			workshop_toolbar_end();
		}
		HANDLE_ERRORS(cmd);
		break;

#ifdef DEBUG
	default:
		unrecognised_message(cmd);
		break;
#endif
	}
}

static void
process_menuItem(
	char	*cmd)
{
	char *label  = strtok(&cmd[9], NOCATGETS("\001"));
	char *verb  = strtok(NULL, NOCATGETS("\001"));
	char *acc = strtok(NULL, NOCATGETS("\001"));
	char *accText  = strtok(NULL, NOCATGETS("\001"));
	char *name  = strtok(NULL, NOCATGETS("\001"));
	char *sense  = strtok(NULL, NOCATGETS("\n"));
	char *filepos  = strtok(NULL, NOCATGETS("\n"));
	if (*acc == '-') {
		acc = NULL;
	}
	if (*accText == '-') {
		accText = NULL;
	}
	workshop_menu_item(label, verb, acc, accText, name, filepos, sense);

}


static void
process_toolbarButton(
	char	*cmd)			/* button definition */
{
	char *label  = strtok(&cmd[14], NOCATGETS("\001"));
	char *verb  = strtok(NULL, NOCATGETS("\001"));
	char *senseVerb  = strtok(NULL, NOCATGETS("\001"));
	char *filepos  = strtok(NULL, NOCATGETS("\001"));
	char *help  = strtok(NULL, NOCATGETS("\001"));
	char *sense  = strtok(NULL, NOCATGETS("\001"));
	char *file  = strtok(NULL, NOCATGETS("\001"));
	char *left  = strtok(NULL, NOCATGETS("\n"));

	if (!strcmp(label, NOCATGETS("-"))) {
		label = NULL;
	}
	if (!strcmp(help, NOCATGETS("-"))) {
		help = NULL;
	}
	if (!strcmp(file, NOCATGETS("-"))) {
		file = NULL;
	}
	if (!strcmp(senseVerb, NOCATGETS("-"))) {
		senseVerb = NULL;
	}
	workshop_toolbar_button(label, verb, senseVerb, filepos, help,
				sense, file, left);
}


#ifdef DEBUG
void
unrecognised_message(
	char	*cmd)
{
	pldebug("Unrecognised eserve message:\n\t%s\n", cmd);
	/* abort(); */
}
#endif


/* Change sign name to accomodate a different size:
 * Create the filename based on the height. The filename format
 * of multisize icons are:
 *    x.xpm   : largest icon
 *    x1.xpm  : smaller icon
 *    x2.xpm  : smallest icon */
	void
adjust_sign_name(char *filename)
{
	char *s;
	static int fontSize = -1;

	if (fontSize == -1)
		fontSize = workshop_get_font_height();
	if (fontSize == 0)
		return;
	if (filename[0] == '-')
		return;

	/* This is ugly: later we should instead pass the fontheight over
	 * to eserve on startup and let eserve just send the right filenames
	 * to us in the first place

	 * I know that the filename will end with 1.xpm (see
	 * GuiEditor.cc`LispPrintSign if you wonder why) */
	s = filename+strlen(filename)-5;
	if (fontSize <= 11)
		strcpy(s, "2.xpm");
	else if (fontSize <= 15)
		strcpy(s, "1.xpm");
	else
		strcpy(s, ".xpm");
}

/* Were we invoked by WorkShop? This function can be used early during startup
   if you want to do things differently if the editor is started standalone
   or in WorkShop mode. For example, in standalone mode you may not want to
   add a footer/message area or a sign gutter. */
int
workshop_invoked()
{
	static int result = -1;
	if (result == -1) {
		result = (getenv(NOCATGETS("SPRO_EDITOR_SOCKET")) != NULL);
	}
	return result;
}

/* Connect back to eserve */
void	workshop_connect(XtAppContext context)
{
#ifdef INET_SOCKETS
	struct sockaddr_in	server;
	struct hostent *	host;
	int			port;
#else
	struct sockaddr_un	server;
#endif
	char			buf[32];
	char *			address;
#ifdef DEBUG
	char			*file;
#endif

	address = getenv(NOCATGETS("SPRO_EDITOR_SOCKET"));
	if (address == NULL) {
		return;
	}

#ifdef INET_SOCKETS
	port = atoi(address);

	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		PERROR(NOCATGETS("workshop_connect"));
		return;
	}

	/* Get the server internet address and put into addr structure */
	/* fill in the socket address structure and connect to server */
	memset((char *)&server, '\0', sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = port;
	if ((host = gethostbyname(NOCATGETS("localhost"))) == NULL) {
		PERROR(NOCATGETS("gethostbyname"));
		sd = -1;
		return;
	}
	memcpy((char *)&server.sin_addr, host->h_addr, host->h_length);
#else
	if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		PERROR(NOCATGETS("workshop_connect"));
		return;
	}

	server.sun_family = AF_UNIX;
	strcpy(server.sun_path, address);
#endif
	/* Connect to server */
	if (connect(sd, (struct sockaddr *)&server, sizeof(server))) {
		if (errno == ECONNREFUSED) {
			close(sd);
#ifdef INET_SOCKETS
			if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
				PERROR(NOCATGETS("workshop_connect"));
				return;
			}
#else
			if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
				PERROR(NOCATGETS("workshop_connect"));
				return;
			}
#endif
			if (connect(sd, (struct sockaddr *)&server,
						sizeof(server))) {
				PERROR(NOCATGETS("workshop_connect"));
				return;
			}

		} else {
			PERROR(NOCATGETS("workshop_connect"));
			return;
		}
	}

	/* tell notifier we are interested in being called
	 * when there is input on the editor connection socket
	 */
	inputHandler = XtAppAddInput(context, sd, (XtPointer) XtInputReadMask,
				     messageFromEserve, NULL);
#ifdef DEBUG
	if ((file = getenv(NOCATGETS("SPRO_PLUGIN_DEBUG"))) != NULL) {
		char buf[BUFSIZ];

		unlink(file);
		vim_snprintf(buf, sizeof(buf), "date > %s", file);
		system(buf);
		dfd = fopen(file, "a");
	} else {
		dfd = NULL;
	}
#endif

	vim_snprintf(buf, sizeof(buf), NOCATGETS("connected %s %s %s\n"),
		workshop_get_editor_name(),
		PROTOCOL_VERSION,
		workshop_get_editor_version());
	write(sd, buf, strlen(buf));

	vim_snprintf(buf, sizeof(buf), NOCATGETS("ack 1\n"));
	write(sd, buf, strlen(buf));
}

void	workshop_disconnect()
{
	/* Probably need to send some message here */

	/*
	 * socket closed on other end
	 */
	XtRemoveInput(inputHandler);
	close(sd);
	inputHandler = 0;
	sd = -1;

}

/*
 * Utility functions
 */

/* Set icon for the window */
void
workshop_set_icon(Display *display, Widget shell, char **xpmdata,
		  int width, int height)
{
	Pixel		bgPixel;
	XpmAttributes   xpmAttributes;
	XSetWindowAttributes attr;
	Window		iconWindow;
	int		depth;
	int		screenNum;
	Pixmap		pixmap;

	/* Create the pixmap/icon window which is shown when you
	 * iconify the sccs viewer
	 * This code snipped was adapted from Sun WorkShop's source base,
	 * setIcon.cc.
	 */
	XtVaGetValues(shell, XmNbackground, &bgPixel, NULL);
	screenNum = XScreenNumberOfScreen(XtScreen(shell));
	depth = DisplayPlanes(display, screenNum);
	xpmAttributes.valuemask = XpmColorSymbols;
	xpmAttributes.numsymbols = 1;
	xpmAttributes.colorsymbols =
	    (XpmColorSymbol *)XtMalloc(sizeof (XpmColorSymbol) *
	    xpmAttributes.numsymbols);
	xpmAttributes.colorsymbols[0].name = NOCATGETS("BgColor");
	xpmAttributes.colorsymbols[0].value = NULL;
	xpmAttributes.colorsymbols[0].pixel = bgPixel;
	if (XpmCreatePixmapFromData(display,
	    RootWindow(display, screenNum), xpmdata, &pixmap,
	    NULL, &xpmAttributes) >= 0) {
		attr.background_pixmap = pixmap;
		iconWindow = XCreateWindow(display, RootWindow(display,
		    screenNum), 0, 0, width, height, 0, depth,
				(unsigned int)CopyFromParent,
		    CopyFromParent, CWBackPixmap, &attr);

		XtVaSetValues(shell,
		    XtNiconWindow, iconWindow, NULL);
	}
	XtFree((char *)xpmAttributes.colorsymbols);
}

/* Minimize and maximize shells. From libutil's shell.cc. */

/* utility functions from libutil's shell.cc */
static Boolean
isWindowMapped(Display *display, Window win)
{
	XWindowAttributes winAttrs;
	XGetWindowAttributes(display,
			     win,
			     &winAttrs);
	if (winAttrs.map_state == IsViewable) {
		return(True);
	} else {
		return(False);
	}
}

static Boolean
isMapped(Widget widget)
{
	if (widget == NULL) {
		return(False);
	}

	if (XtIsRealized(widget) == False) {
		return(False);
	}

	return(isWindowMapped(XtDisplay(widget), XtWindow(widget)));
}

static Boolean
widgetIsIconified(
	Widget		 w)
{
	Atom		 wm_state;
	Atom		 act_type;		/* actual Atom type returned */
	int		 act_fmt;		/* actual format returned */
	u_long		 nitems_ret;		/* number of items returned */
	u_long		 bytes_after;		/* number of bytes remaining */
	u_long		*property;		/* actual property returned */

	/*
	 * If a window is iconified its WM_STATE is set to IconicState. See
	 * ICCCM Version 2.0, section 4.1.3.1 for more details.
	 */

	wm_state = XmInternAtom(XtDisplay(w), NOCATGETS("WM_STATE"), False);
	if (XtWindow(w) != 0) {			/* only check if window exists! */
		XGetWindowProperty(XtDisplay(w), XtWindow(w), wm_state, 0L, 2L,
		    False, AnyPropertyType, &act_type, &act_fmt, &nitems_ret,
		    &bytes_after, (u_char **) &property);
		if (nitems_ret == 2 && property[0] == IconicState) {
			return True;
		}
	}

	return False;

}    /* end widgetIsIconified */

void
workshop_minimize_shell(Widget shell)
{
	if (shell != NULL &&
	    XtIsObject(shell) &&
	    XtIsRealized(shell) == True) {
		if (isMapped(shell) == True) {
			XIconifyWindow(XtDisplay(shell), XtWindow(shell),
			       XScreenNumberOfScreen(XtScreen(shell)));
		}
		XtVaSetValues(shell,
			      XmNiconic, True,
			      NULL);
	}
}

void workshop_maximize_shell(Widget shell)
{
	if (shell != NULL &&
	    XtIsRealized(shell) == True &&
	    widgetIsIconified(shell) == True &&
	    isMapped(shell) == False) {
		XtMapWidget(shell);
		/* This used to be
		     XtPopdown(shell);
		     XtPopup(shell, XtGrabNone);
		   However, I found that that would drop any transient
		   windows that had been iconified with the window.
		   According to the ICCCM, XtMapWidget should be used
		   to bring a window from Iconic to Normal state.
		   However, Rich Mauri did a lot of work on this during
		   Bart, and found that XtPopDown,XtPopup was required
		   to fix several bugs involving multiple CDE workspaces.
		   I've tested it now and things seem to work fine but
		   I'm leaving this note for history in case this needs
		   to be revisited.
		*/
	}
}


Boolean workshop_get_width_height(int *width, int *height)
{
	static int	wid = 0;
	static int	hgt = 0;
	static Boolean	firstTime = True;
	static Boolean	success = False;

	if (firstTime) {
		char	*settings;

		settings = getenv(NOCATGETS("SPRO_GUI_WIDTH_HEIGHT"));
		if (settings != NULL) {
			wid = atoi(settings);
			settings = strrchr(settings, ':');
			if (settings++ != NULL) {
				hgt = atoi(settings);
			}
			if (wid > 0 && hgt > 0) {
				success = True;
			}
			firstTime = False;
		}
	}

	if (success) {
		*width = wid;
		*height = hgt;
	}
	return success;
}


Boolean workshop_get_rows_cols(int *rows, int *cols)
{
	static int	r = 0;
	static int	c = 0;
	static Boolean	firstTime = True;
	static Boolean	success = False;

	if (firstTime) {
		char	*settings;

		settings = getenv(NOCATGETS("SPRO_GUI_ROWS_COLS"));
		if (settings != NULL) {
			r = atoi(settings);
			settings = strrchr(settings, ':');
			if (settings++ != NULL) {
				c = atoi(settings);
			}
			if (r > 0 && c > 0) {
				success = True;
			}
			firstTime = False;
		}
	}

	if (success) {
		*rows = r;
		*cols = c;
	}
	return success;
}

/*
 * Toolbar code
 */

void workshop_sensitivity(int num, char *table)
{
	/* build up a verb table */
	VerbSense *vs;
	int i;
	char *s;
	if ((num < 1) || (num > 500)) {
		return;
	}

	vs = (VerbSense *)malloc((num+1)*sizeof(VerbSense));

	/* Point to the individual names (destroys the table string, but
	 * that's okay -- this is more efficient than duplicating strings) */
	s = table;
	for (i = 0; i < num; i++) {
		while (*s == ' ') {
			s++;
		}
		vs[i].verb = s;
		while (*s && (*s != ' ') && (*s != '\001')) {
			s++;
		}
		if (*s == 0) {
			vs[i].verb = NULL;
			break;
		}
		if (*s == '\001') {
			*s = 0;
			s++;
		}
		*s = 0;
		s++;
		while (*s == ' ') {
			s++;
		}
		if (*s == '1') {
			vs[i].sense = 1;
		} else {
			vs[i].sense = 0;
		}
		s++;
	}
	vs[i].verb = NULL;

	workshop_frame_sensitivities(vs);

	free(vs);
}

/*
 * Options code
 */
/* Set an editor option.
 * IGNORE an option if you do not recognize it.
 */
void workshop_set_option_first(char *name, char *value)
{
	/* Currently value can only be on/off. This may change later (for
	 * example to set an option like "balloon evaluate delay", but
	 * for now just convert it into a boolean */
	Boolean on = !strcmp(value, "on");

	if (!strcmp(name, "workshopkeys")) {
		workshop_hotkeys(on);
	} else if (!strcmp(name, "savefiles")) {
		save_files = on;
	} else if (!strcmp(name, "balloon")) {
		workshop_balloon_mode(on);
	} else if (!strcmp(name, "balloondelay")) {
		int delay = atoi(value);
		/* Should I validate the number here?? */
		workshop_balloon_delay(delay);
	} else {
		/* Let editor interpret it */
		workshop_set_option(name, value);
	}
}



/*
 * Send information to eserve on certain editor events
 * You must make sure these are called when necessary
 */

void workshop_file_closed(char *filename)
{
	char buffer[2*MAXPATHLEN];
	vim_snprintf(buffer, sizeof(buffer),
			NOCATGETS("deletedFile %s\n"), filename);
	write(sd, buffer, strlen(buffer));
}

void workshop_file_closed_lineno(char *filename, int lineno)
{
	char buffer[2*MAXPATHLEN];
	vim_snprintf(buffer, sizeof(buffer),
			NOCATGETS("deletedFile %s %d\n"), filename, lineno);
	write(sd, buffer, strlen(buffer));
}

void workshop_file_opened(char *filename, int readOnly)
{
	char buffer[2*MAXPATHLEN];
	vim_snprintf(buffer, sizeof(buffer),
			NOCATGETS("loadedFile %s %d\n"), filename, readOnly);
	write(sd, buffer, strlen(buffer));
}


void workshop_file_saved(char *filename)
{
	char buffer[2*MAXPATHLEN];
	vim_snprintf(buffer, sizeof(buffer),
			NOCATGETS("savedFile %s\n"), filename);
	write(sd, buffer, strlen(buffer));

	/* Let editor report any moved marks that the eserve client
	 * should deal with (for example, moving location-based breakpoints) */
	workshop_moved_marks(filename);
}

void workshop_move_mark(char *filename, int markId, int newLineno)
{
	char buffer[2*MAXPATHLEN];
	vim_snprintf(buffer, sizeof(buffer),
			NOCATGETS("moveMark %s %d %d\n"), filename, markId, newLineno);
	write(sd, buffer, strlen(buffer));
}

void workshop_file_modified(char *filename)
{
	char buffer[2*MAXPATHLEN];
	vim_snprintf(buffer, sizeof(buffer),
			NOCATGETS("modifiedFile %s\n"), filename);
	write(sd, buffer, strlen(buffer));
}

void workshop_frame_moved(int new_x, int new_y, int new_w, int new_h)
{
	char buffer[200];

	if (sd >= 0)
	{
		vim_snprintf(buffer, sizeof(buffer),
				NOCATGETS("frameAt %d %d %d %d\n"),
				new_x, new_y, new_w, new_h);
		write(sd, buffer, strlen(buffer));
	}
}

/* A button in the toolbar has been pushed.
 * Clientdata is a pointer used by the editor code to figure out the
 * positions for this toolbar (probably by storing a window pointer,
 * and then fetching the current buffer for that window and looking up
 * cursor and selection positions etc.) */
void workshop_perform_verb(char *verb, void *clientData)
{
	char *filename;
	int curLine;
	int curCol;
	int selStartLine;
	int selStartCol;
	int selEndLine;
	int selEndCol;
	int selLength;
	char *selection;

	char buf[2*MAXPATHLEN];
/* Later: needsFilePos indicates whether or not we need to fetch all this
 * info for this verb... for now, however, it looks as if
 * eserve parsing routines depend on it always being present */

	if (workshop_get_positions(clientData,
				   &filename,
				   &curLine,
				   &curCol,
				   &selStartLine,
				   &selStartCol,
				   &selEndLine,
				   &selEndCol,
				   &selLength,
				   &selection)) {
		if (selection == NULL) {
			selection = NOCATGETS("");
		}

		/* Should I save the files??? This is currently done by checking
		   if the verb is one of a few recognized ones. Later we can pass
		   this list from eserve to the editor (it's currently hardcoded in
		   vi and emacs as well). */
		if (save_files) {
			if (!strcmp(verb, "build.build") || !strcmp(verb, "build.build-file") ||
			    !strcmp(verb, "debug.fix") || !strcmp(verb, "debug.fix-all")) {
				workshop_save_files();
			}
		}

		vim_snprintf(buf, sizeof(buf),
			NOCATGETS("toolVerb %s %s %d,%d %d,%d %d,%d %d %s\n"),
			verb,
			filename,
			curLine, curCol,
			selStartLine, selStartCol,
			selEndLine, selEndCol,
			selLength,
			selection);
		write(sd, buf, strlen(buf));
		if (*selection) {
			free(selection);
		}
	}
}

/* Send a message to eserve */
void workshop_send_message(char *buf)
{
	write(sd, buf, strlen(buf));
}

/* Some methods, like currentFile, cursorPos, etc. are missing here.
 * But it looks like these are used for NoHands testing only so we
 * won't bother requiring editors to implement these
 */


#ifdef DEBUG

void
pldebug(
	char		*fmt,	/* a printf style format line */
	...)
{
	va_list		 ap;

	if (dfd != NULL) {
		va_start(ap, fmt);
		vfprintf(dfd, fmt, ap);
		va_end(ap);
		fflush(dfd);
	}

}    /* end pldebug */

#endif
