/* vi:set ts=8 sw=8 noet:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *			Visual Workshop integration by Gordon Prieur
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * WorkShop Debugging Tools. What are these tools and why are they important?
 * There are two main tools here. The first tool is a tool for delaying or
 * stopping gvim during startup.  The second tool is a protocol log tool.
 *
 * The startup delay tool is called wsdebug_wait(). This is very important for
 * debugging startup problems because gvim will be started automatically from
 * workshop and cannot be run directly from a debugger. The only way to debug
 * a gvim started by workshop is by attaching a debugger to it. Without this
 * tool all starup code will have completed before you can get the pid and
 * attach.
 *
 * The second tool is a protocol log tool. The workshop editor server and gvim
 * pass information back and forth during a workshop session. Sometimes it is
 * very important to peruse this conversation in order to understand what is
 * happening. The wsdebug_log_init() call sets up this protocol log tool and
 * wsdebug() and wstrace() calls output the information to the log.
 *
 * This code must have WSDEBUG defined for it to be compiled into vim/gvim.
 */

#ifdef WSDEBUG

#include "vim.h"

FILE		*ws_debug = NULL;
u_int		 ws_dlevel = 0;		/* ws_debug verbosity level */

void		 wsdebug(char *, ...);
void		 wstrace(char *, ...);

static int	 lookup(char *);
#ifdef USE_WS_ERRORHANDLER
static int	 errorHandler(Display *, XErrorEvent *);
#endif


/*
 * wsdebug_wait	-   This function can be used to delay or stop execution of vim.
 *		    Its normally used to delay startup while attaching a
 *		    debugger to a running process. Since workshop starts gvim
 *		    from a background process this is the only way to debug
 *		    startup problems.
 */

void wsdebug_wait(
	u_int		 wait_flags,	/* tells what to do */
	char		*wait_var,	/* wait environment variable */
	u_int		 wait_secs)	/* how many seconds to wait */
{

	init_homedir();			/* not inited yet */
#ifdef USE_WDDUMP
	WDDump(0, 0, 0);
#endif

	/* for debugging purposes only */
	if (wait_flags & WT_ENV && wait_var && getenv(wait_var) != NULL) {
		sleep(atoi(getenv(wait_var)));
	} else if (wait_flags & WT_WAIT && lookup("~/.gvimwait")) {
		sleep(wait_secs > 0 && wait_secs < 120 ? wait_secs : 20);
	} else if (wait_flags & WT_STOP && lookup("~/.gvimstop")) {
		int w = 1;
		while (w) {
			;
		}
	}
}    /* end wsdebug_wait */


void
wsdebug_log_init(
	char		*log_var,	/* env var with log file */
	char		*level_var)	/* env var with ws_debug level */
{
	char		*file;		/* possible ws_debug output file */
	char		*cp;		/* ws_dlevel pointer */

	if (log_var && (file = getenv(log_var)) != NULL)
	{
		char buf[BUFSIZ];

		vim_snprintf(buf, sizeof(buf), "date > %s", file);
		system(buf);
		ws_debug = fopen(file, "a");
		if (level_var && (cp = getenv(level_var)) != NULL) {
			ws_dlevel = strtoul(cp, NULL, 0);
		} else {
			ws_dlevel = WS_TRACE;	/* default level */
		}
#ifdef USE_WS_ERRORHANDLER
		XSetErrorHandler(errorHandler);
#endif
	}

}    /* end wsdebug_log_init */




void
wstrace(
	char		*fmt,
	...)
{
	va_list		 ap;

	if (ws_debug!= NULL && (ws_dlevel & (WS_TRACE | WS_TRACE_VERBOSE))) {
		va_start(ap, fmt);
		vfprintf(ws_debug, fmt, ap);
		va_end(ap);
		fflush(ws_debug);
	}

}    /* end wstrace */


void
wsdebug(
	char		*fmt,
	...)
{
	va_list		 ap;

	if (ws_debug != NULL) {
		va_start(ap, fmt);
		vfprintf(ws_debug, fmt, ap);
		va_end(ap);
		fflush(ws_debug);
	}

}    /* end wsdebug */


static int
lookup(
	char		*file)
{
	char		 buf[BUFSIZ];

	expand_env((char_u *) file, (char_u *) buf, BUFSIZ);
	return (access(buf, F_OK) == 0);

}    /* end lookup */

#ifdef USE_WS_ERRORHANDLER
static int
errorHandler(
	Display		*dpy,
	XErrorEvent	*err)
{
	char		 msg[256];
	char		 buf[256];

	XGetErrorText(dpy, err->error_code, msg, sizeof(msg));
	wsdebug("\n\nWSDEBUG Vim: X Error of failed request: %s\n", msg);

	sprintf(buf, "%d", err->request_code);
	XGetErrorDatabaseText(dpy,
	    "XRequest", buf, "Unknown", msg, sizeof(msg));
	wsdebug("\tMajor opcode of failed request: %d (%s)\n",
	    err->request_code, msg);
	if (err->request_code > 128) {
		wsdebug("\tMinor opcode of failed request: %d\n",
		    err->minor_code);
	}

	return 0;
}
#endif



#endif /* WSDEBUG */
