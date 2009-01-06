/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *			Netbeans integration by David Weatherford
 *			Adopted for Win32 by Sergey Khorev
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 */

/*
 * Implements client side of org.netbeans.modules.emacs editor
 * integration protocol.  Be careful!  The protocol uses offsets
 * which are *between* characters, whereas vim uses line number
 * and column number which are *on* characters.
 * See ":help netbeans-protocol" for explanation.
 */

#if defined(MSDOS) || defined(WIN16) || defined(WIN32) || defined(_WIN64)
# include "vimio.h"	/* for mch_open(), must be before vim.h */
#endif

#include "vim.h"

#if defined(FEAT_NETBEANS_INTG) || defined(PROTO)

/* Note: when making changes here also adjust configure.in. */
#ifdef WIN32
# ifdef DEBUG
#  include <tchar.h>	/* for _T definition for TRACEn macros */
# endif
/* WinSock API is separated from C API, thus we can't use read(), write(),
 * errno... */
# define sock_errno WSAGetLastError()
# define ECONNREFUSED WSAECONNREFUSED
# ifdef EINTR
#  undef EINTR
# endif
# define EINTR WSAEINTR
# define sock_write(sd, buf, len) send(sd, buf, len, 0)
# define sock_read(sd, buf, len) recv(sd, buf, len, 0)
# define sock_close(sd) closesocket(sd)
# define sleep(t) Sleep(t*1000) /* WinAPI Sleep() accepts milliseconds */
#else
# include <netdb.h>
# include <netinet/in.h>
# include <sys/socket.h>
# ifdef HAVE_LIBGEN_H
#  include <libgen.h>
# endif
# define sock_errno errno
# define sock_write(sd, buf, len) write(sd, buf, len)
# define sock_read(sd, buf, len) read(sd, buf, len)
# define sock_close(sd) close(sd)
#endif

#include "version.h"

#define INET_SOCKETS

#define GUARDED		10000 /* typenr for "guarded" annotation */
#define GUARDEDOFFSET 1000000 /* base for "guarded" sign id's */

/* The first implementation (working only with Netbeans) returned "1.1".  The
 * protocol implemented here also supports A-A-P. */
static char *ExtEdProtocolVersion = "2.4";

static long pos2off __ARGS((buf_T *, pos_T *));
static pos_T *off2pos __ARGS((buf_T *, long));
static pos_T *get_off_or_lnum __ARGS((buf_T *buf, char_u **argp));
static long get_buf_size __ARGS((buf_T *));
static void netbeans_keystring __ARGS((int key, char *keystr));
static void special_keys __ARGS((char_u *args));

static void netbeans_connect __ARGS((void));
static int getConnInfo __ARGS((char *file, char **host, char **port, char **password));

static void nb_init_graphics __ARGS((void));
static void coloncmd __ARGS((char *cmd, ...));
static void nb_set_curbuf __ARGS((buf_T *buf));
#ifdef FEAT_GUI_MOTIF
static void messageFromNetbeans __ARGS((XtPointer, int *, XtInputId *));
#endif
#ifdef FEAT_GUI_GTK
static void messageFromNetbeans __ARGS((gpointer, gint, GdkInputCondition));
#endif
static void nb_parse_cmd __ARGS((char_u *));
static int  nb_do_cmd __ARGS((int, char_u *, int, int, char_u *));
static void nb_send __ARGS((char *buf, char *fun));

#ifdef WIN64
typedef __int64 NBSOCK;
#else
typedef int NBSOCK;
#endif

static NBSOCK sd = -1;			/* socket fd for Netbeans connection */
#ifdef FEAT_GUI_MOTIF
static XtInputId inputHandler;		/* Cookie for input */
#endif
#ifdef FEAT_GUI_GTK
static gint inputHandler;		/* Cookie for input */
#endif
#ifdef FEAT_GUI_W32
static int  inputHandler = -1;		/* simply ret.value of WSAAsyncSelect() */
extern HWND s_hwnd;			/* Gvim's Window handle */
#endif
static int r_cmdno;			/* current command number for reply */
static int haveConnection = FALSE;	/* socket is connected and
					   initialization is done */
#ifdef FEAT_GUI_MOTIF
static void netbeans_Xt_connect __ARGS((void *context));
#endif
#ifdef FEAT_GUI_GTK
static void netbeans_gtk_connect __ARGS((void));
#endif
#ifdef FEAT_GUI_W32
static void netbeans_w32_connect __ARGS((void));
#endif

static int dosetvisible = FALSE;

/*
 * Include the debugging code if wanted.
 */
#ifdef NBDEBUG
# include "nbdebug.c"
#endif

/* Connect back to Netbeans process */
#ifdef FEAT_GUI_MOTIF
    static void
netbeans_Xt_connect(void *context)
{
    netbeans_connect();
    if (sd > 0)
    {
	/* tell notifier we are interested in being called
	 * when there is input on the editor connection socket
	 */
	inputHandler = XtAppAddInput((XtAppContext)context, sd,
			     (XtPointer)(XtInputReadMask + XtInputExceptMask),
						   messageFromNetbeans, NULL);
    }
}

    static void
netbeans_disconnect(void)
{
    if (inputHandler != (XtInputId)NULL)
    {
	XtRemoveInput(inputHandler);
	inputHandler = (XtInputId)NULL;
    }
    sd = -1;
    haveConnection = FALSE;
# ifdef FEAT_BEVAL
    bevalServers &= ~BEVAL_NETBEANS;
# endif
}
#endif /* FEAT_MOTIF_GUI */

#ifdef FEAT_GUI_GTK
    static void
netbeans_gtk_connect(void)
{
    netbeans_connect();
    if (sd > 0)
    {
	/*
	 * Tell gdk we are interested in being called when there
	 * is input on the editor connection socket
	 */
	inputHandler = gdk_input_add((gint)sd, (GdkInputCondition)
		((int)GDK_INPUT_READ + (int)GDK_INPUT_EXCEPTION),
						   messageFromNetbeans, NULL);
    }
}

    static void
netbeans_disconnect(void)
{
    if (inputHandler != 0)
    {
	gdk_input_remove(inputHandler);
	inputHandler = 0;
    }
    sd = -1;
    haveConnection = FALSE;
# ifdef FEAT_BEVAL
    bevalServers &= ~BEVAL_NETBEANS;
# endif
}
#endif /* FEAT_GUI_GTK */

#if defined(FEAT_GUI_W32) || defined(PROTO)
    static void
netbeans_w32_connect(void)
{
    netbeans_connect();
    if (sd > 0)
    {
	/*
	 * Tell Windows we are interested in receiving message when there
	 * is input on the editor connection socket
	 */
	inputHandler = WSAAsyncSelect(sd, s_hwnd, WM_NETBEANS, FD_READ);
    }
}

    static void
netbeans_disconnect(void)
{
    if (inputHandler == 0)
    {
	WSAAsyncSelect(sd, s_hwnd, 0, 0);
	inputHandler = -1;
    }
    sd = -1;
    haveConnection = FALSE;
# ifdef FEAT_BEVAL
    bevalServers &= ~BEVAL_NETBEANS;
# endif
}
#endif /* FEAT_GUI_W32 */

#define NB_DEF_HOST "localhost"
#define NB_DEF_ADDR "3219"
#define NB_DEF_PASS "changeme"

    static void
netbeans_connect(void)
{
#ifdef INET_SOCKETS
    struct sockaddr_in	server;
    struct hostent *	host;
# ifdef FEAT_GUI_W32
    u_short		port;
# else
    int			port;
#endif
#else
    struct sockaddr_un	server;
#endif
    char	buf[32];
    char	*hostname = NULL;
    char	*address = NULL;
    char	*password = NULL;
    char	*fname;
    char	*arg = NULL;

    if (netbeansArg[3] == '=')
    {
	/* "-nb=fname": Read info from specified file. */
	if (getConnInfo(netbeansArg + 4, &hostname, &address, &password)
								      == FAIL)
	    return;
    }
    else
    {
	if (netbeansArg[3] == ':')
	    /* "-nb:<host>:<addr>:<password>": get info from argument */
	    arg = netbeansArg + 4;
	if (arg == NULL && (fname = getenv("__NETBEANS_CONINFO")) != NULL)
	{
	    /* "-nb": get info from file specified in environment */
	    if (getConnInfo(fname, &hostname, &address, &password) == FAIL)
		return;
	}
	else
	{
	    if (arg != NULL)
	    {
		/* "-nb:<host>:<addr>:<password>": get info from argument */
		hostname = arg;
		address = strchr(hostname, ':');
		if (address != NULL)
		{
		    *address++ = '\0';
		    password = strchr(address, ':');
		    if (password != NULL)
			*password++ = '\0';
		}
	    }

	    /* Get the missing values from the environment. */
	    if (hostname == NULL || *hostname == '\0')
		hostname = getenv("__NETBEANS_HOST");
	    if (address == NULL)
		address = getenv("__NETBEANS_SOCKET");
	    if (password == NULL)
		password = getenv("__NETBEANS_VIM_PASSWORD");

	    /* Move values to allocated memory. */
	    if (hostname != NULL)
		hostname = (char *)vim_strsave((char_u *)hostname);
	    if (address != NULL)
		address = (char *)vim_strsave((char_u *)address);
	    if (password != NULL)
		password = (char *)vim_strsave((char_u *)password);
	}
    }

    /* Use the default when a value is missing. */
    if (hostname == NULL || *hostname == '\0')
    {
	vim_free(hostname);
	hostname = (char *)vim_strsave((char_u *)NB_DEF_HOST);
    }
    if (address == NULL || *address == '\0')
    {
	vim_free(address);
	address = (char *)vim_strsave((char_u *)NB_DEF_ADDR);
    }
    if (password == NULL || *password == '\0')
    {
	vim_free(password);
	password = (char *)vim_strsave((char_u *)NB_DEF_PASS);
    }
    if (hostname == NULL || address == NULL || password == NULL)
	goto theend;	    /* out of memory */

#ifdef INET_SOCKETS
    port = atoi(address);

    if ((sd = (NBSOCK)socket(AF_INET, SOCK_STREAM, 0)) == (NBSOCK)-1)
    {
	nbdebug(("error in socket() in netbeans_connect()\n"));
	PERROR("socket() in netbeans_connect()");
	goto theend;
    }

    /* Get the server internet address and put into addr structure */
    /* fill in the socket address structure and connect to server */
    memset((char *)&server, '\0', sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    if ((host = gethostbyname(hostname)) == NULL)
    {
	if (mch_access(hostname, R_OK) >= 0)
	{
	    /* DEBUG: input file */
	    sd = mch_open(hostname, O_RDONLY, 0);
	    goto theend;
	}
	nbdebug(("error in gethostbyname() in netbeans_connect()\n"));
	PERROR("gethostbyname() in netbeans_connect()");
	sd = -1;
	goto theend;
    }
    memcpy((char *)&server.sin_addr, host->h_addr, host->h_length);
#else
    if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
	nbdebug(("error in socket() in netbeans_connect()\n"));
	PERROR("socket() in netbeans_connect()");
	goto theend;
    }

    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, address);
#endif
    /* Connect to server */
    if (connect(sd, (struct sockaddr *)&server, sizeof(server)))
    {
	nbdebug(("netbeans_connect: Connect failed with errno %d\n", sock_errno));
	if (sock_errno == ECONNREFUSED)
	{
	    sock_close(sd);
#ifdef INET_SOCKETS
	    if ((sd = (NBSOCK)socket(AF_INET, SOCK_STREAM, 0)) == (NBSOCK)-1)
	    {
		nbdebug(("socket()#2 in netbeans_connect()\n"));
		PERROR("socket()#2 in netbeans_connect()");
		goto theend;
	    }
#else
	    if ((sd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	    {
		nbdebug(("socket()#2 in netbeans_connect()\n"));
		PERROR("socket()#2 in netbeans_connect()");
		goto theend;
	    }
#endif
	    if (connect(sd, (struct sockaddr *)&server, sizeof(server)))
	    {
		int retries = 36;
		int success = FALSE;
		while (retries--
			     && ((sock_errno == ECONNREFUSED) || (sock_errno == EINTR)))
		{
		    nbdebug(("retrying...\n"));
		    sleep(5);
		    if (connect(sd, (struct sockaddr *)&server,
				sizeof(server)) == 0)
		    {
			success = TRUE;
			break;
		    }
		}
		if (!success)
		{
		    /* Get here when the server can't be found. */
		    nbdebug(("Cannot connect to Netbeans #2\n"));
		    PERROR(_("Cannot connect to Netbeans #2"));
		    getout(1);
		}
	    }

	}
	else
	{
	    nbdebug(("Cannot connect to Netbeans\n"));
	    PERROR(_("Cannot connect to Netbeans"));
	    getout(1);
	}
    }

    vim_snprintf(buf, sizeof(buf), "AUTH %s\n", password);
    nb_send(buf, "netbeans_connect");

    sprintf(buf, "0:version=0 \"%s\"\n", ExtEdProtocolVersion);
    nb_send(buf, "externaleditor_version");

/*    nb_init_graphics();  delay until needed */

    haveConnection = TRUE;

theend:
    vim_free(hostname);
    vim_free(address);
    vim_free(password);
    return;
}

/*
 * Obtain the NetBeans hostname, port address and password from a file.
 * Return the strings in allocated memory.
 * Return FAIL if the file could not be read, OK otherwise (no matter what it
 * contains).
 */
    static int
getConnInfo(char *file, char **host, char **port, char **auth)
{
    FILE *fp;
    char_u buf[BUFSIZ];
    char_u *lp;
    char_u *nl;
#ifdef UNIX
    struct stat	st;

    /*
     * For Unix only accept the file when it's not accessible by others.
     * The open will then fail if we don't own the file.
     */
    if (mch_stat(file, &st) == 0 && (st.st_mode & 0077) != 0)
    {
	nbdebug(("Wrong access mode for NetBeans connection info file: \"%s\"\n",
								       file));
	EMSG2(_("E668: Wrong access mode for NetBeans connection info file: \"%s\""),
									file);
	return FAIL;
    }
#endif

    fp = mch_fopen(file, "r");
    if (fp == NULL)
    {
	nbdebug(("Cannot open NetBeans connection info file\n"));
	PERROR("E660: Cannot open NetBeans connection info file");
	return FAIL;
    }

    /* Read the file. There should be one of each parameter */
    while ((lp = (char_u *)fgets((char *)buf, BUFSIZ, fp)) != NULL)
    {
	if ((nl = vim_strchr(lp, '\n')) != NULL)
	    *nl = 0;	    /* strip off the trailing newline */

	if (STRNCMP(lp, "host=", 5) == 0)
	{
	    vim_free(*host);
	    *host = (char *)vim_strsave(&buf[5]);
	}
	else if (STRNCMP(lp, "port=", 5) == 0)
	{
	    vim_free(*port);
	    *port = (char *)vim_strsave(&buf[5]);
	}
	else if (STRNCMP(lp, "auth=", 5) == 0)
	{
	    vim_free(*auth);
	    *auth = (char *)vim_strsave(&buf[5]);
	}
    }
    fclose(fp);

    return OK;
}


struct keyqueue
{
    int		     key;
    struct keyqueue *next;
    struct keyqueue *prev;
};

typedef struct keyqueue keyQ_T;

static keyQ_T keyHead; /* dummy node, header for circular queue */


/*
 * Queue up key commands sent from netbeans.
 */
    static void
postpone_keycommand(int key)
{
    keyQ_T *node;

    node = (keyQ_T *)alloc(sizeof(keyQ_T));

    if (keyHead.next == NULL) /* initialize circular queue */
    {
	keyHead.next = &keyHead;
	keyHead.prev = &keyHead;
    }

    /* insert node at tail of queue */
    node->next = &keyHead;
    node->prev = keyHead.prev;
    keyHead.prev->next = node;
    keyHead.prev = node;

    node->key = key;
}

/*
 * Handle any queued-up NetBeans keycommands to be send.
 */
    static void
handle_key_queue(void)
{
    while (keyHead.next && keyHead.next != &keyHead)
    {
	/* first, unlink the node */
	keyQ_T *node = keyHead.next;
	keyHead.next = node->next;
	node->next->prev = node->prev;

	/* now, send the keycommand */
	netbeans_keycommand(node->key);

	/* Finally, dispose of the node */
	vim_free(node);
    }
}


struct cmdqueue
{
    char_u	    *buffer;
    struct cmdqueue *next;
    struct cmdqueue *prev;
};

typedef struct cmdqueue queue_T;

static queue_T head;  /* dummy node, header for circular queue */


/*
 * Put the buffer on the work queue; possibly save it to a file as well.
 */
    static void
save(char_u *buf, int len)
{
    queue_T *node;

    node = (queue_T *)alloc(sizeof(queue_T));
    if (node == NULL)
	return;	    /* out of memory */
    node->buffer = alloc(len + 1);
    if (node->buffer == NULL)
    {
	vim_free(node);
	return;	    /* out of memory */
    }
    mch_memmove(node->buffer, buf, (size_t)len);
    node->buffer[len] = NUL;

    if (head.next == NULL)   /* initialize circular queue */
    {
	head.next = &head;
	head.prev = &head;
    }

    /* insert node at tail of queue */
    node->next = &head;
    node->prev = head.prev;
    head.prev->next = node;
    head.prev = node;

#ifdef NBDEBUG
    {
	static int outfd = -2;

	/* possibly write buffer out to a file */
	if (outfd == -3)
	    return;

	if (outfd == -2)
	{
	    char *file = getenv("__NETBEANS_SAVE");
	    if (file == NULL)
		outfd = -3;
	    else
		outfd = mch_open(file, O_WRONLY|O_CREAT|O_TRUNC, 0666);
	}

	if (outfd >= 0)
	    write(outfd, buf, len);
    }
#endif
}


/*
 * While there's still a command in the work queue, parse and execute it.
 */
    void
netbeans_parse_messages(void)
{
    char_u	*p;
    queue_T	*node;

    while (head.next != NULL && head.next != &head)
    {
	node = head.next;

	/* Locate the first line in the first buffer. */
	p = vim_strchr(node->buffer, '\n');
	if (p == NULL)
	{
	    /* Command isn't complete.  If there is no following buffer,
	     * return (wait for more). If there is another buffer following,
	     * prepend the text to that buffer and delete this one.  */
	    if (node->next == &head)
		return;
	    p = alloc((unsigned)(STRLEN(node->buffer)
					   + STRLEN(node->next->buffer) + 1));
	    if (p == NULL)
		return;	    /* out of memory */
	    STRCPY(p, node->buffer);
	    STRCAT(p, node->next->buffer);
	    vim_free(node->next->buffer);
	    node->next->buffer = p;

	    /* dispose of the node and buffer */
	    head.next = node->next;
	    node->next->prev = node->prev;
	    vim_free(node->buffer);
	    vim_free(node);
	}
	else
	{
	    /* There is a complete command at the start of the buffer.
	     * Terminate it with a NUL.  When no more text is following unlink
	     * the buffer.  Do this before executing, because new buffers can
	     * be added while busy handling the command. */
	    *p++ = NUL;
	    if (*p == NUL)
	    {
		head.next = node->next;
		node->next->prev = node->prev;
	    }

	    /* now, parse and execute the commands */
	    nb_parse_cmd(node->buffer);

	    if (*p == NUL)
	    {
		/* buffer finished, dispose of the node and buffer */
		vim_free(node->buffer);
		vim_free(node);
	    }
	    else
	    {
		/* more follows, move to the start */
		STRMOVE(node->buffer, p);
	    }
	}
    }
}

/* Buffer size for reading incoming messages. */
#define MAXMSGSIZE 4096

/*
 * Read and process a command from netbeans.
 */
/*ARGSUSED*/
#if defined(FEAT_GUI_W32) || defined(PROTO)
/* Use this one when generating prototypes, the others are static. */
    void
messageFromNetbeansW32()
#else
# ifdef FEAT_GUI_MOTIF
    static void
messageFromNetbeans(XtPointer clientData, int *unused1, XtInputId *unused2)
# endif
# ifdef FEAT_GUI_GTK
    static void
messageFromNetbeans(gpointer clientData, gint unused1,
						    GdkInputCondition unused2)
# endif
#endif
{
    static char_u	*buf = NULL;
    int			len;
    int			readlen = 0;
#ifndef FEAT_GUI_GTK
    static int		level = 0;
#endif

    if (sd < 0)
    {
	nbdebug(("messageFromNetbeans() called without a socket\n"));
	return;
    }

#ifndef FEAT_GUI_GTK
    ++level;  /* recursion guard; this will be called from the X event loop */
#endif

    /* Allocate a buffer to read into. */
    if (buf == NULL)
    {
	buf = alloc(MAXMSGSIZE);
	if (buf == NULL)
	    return;	/* out of memory! */
    }

    /* Keep on reading for as long as there is something to read. */
    for (;;)
    {
	len = sock_read(sd, buf, MAXMSGSIZE);
	if (len <= 0)
	    break;	/* error or nothing more to read */

	/* Store the read message in the queue. */
	save(buf, len);
	readlen += len;
	if (len < MAXMSGSIZE)
	    break;	/* did read everything that's available */
    }

    if (readlen <= 0)
    {
	/* read error or didn't read anything */
	netbeans_disconnect();
	nbdebug(("messageFromNetbeans: Error in read() from socket\n"));
	if (len < 0)
	{
	    nbdebug(("read from Netbeans socket\n"));
	    PERROR(_("read from Netbeans socket"));
	}
	return; /* don't try to parse it */
    }

#if defined(FEAT_GUI_GTK) || defined(FEAT_GUI_W32)
    /* Let the main loop handle messages. */
# ifdef FEAT_GUI_GTK
    if (gtk_main_level() > 0)
	gtk_main_quit();
# endif
#else
    /* Parse the messages now, but avoid recursion. */
    if (level == 1)
	netbeans_parse_messages();

    --level;
#endif
}

/*
 * Handle one NUL terminated command.
 *
 * format of a command from netbeans:
 *
 *    6:setTitle!84 "a.c"
 *
 *    bufno
 *     colon
 *      cmd
 *		!
 *		 cmdno
 *		    args
 *
 * for function calls, the ! is replaced by a /
 */
    static void
nb_parse_cmd(char_u *cmd)
{
    char	*verb;
    char	*q;
    int		bufno;
    int		isfunc = -1;

    if (STRCMP(cmd, "DISCONNECT") == 0)
    {
	/* We assume the server knows that we can safely exit! */
	if (sd >= 0)
	    sock_close(sd);
	/* Disconnect before exiting, Motif hangs in a Select error
	 * message otherwise. */
	netbeans_disconnect();
	getout(0);
	/* NOTREACHED */
    }

    if (STRCMP(cmd, "DETACH") == 0)
    {
	/* The IDE is breaking the connection. */
	if (sd >= 0)
	    sock_close(sd);
	netbeans_disconnect();
	return;
    }

    bufno = strtol((char *)cmd, &verb, 10);

    if (*verb != ':')
    {
	nbdebug(("    missing colon: %s\n", cmd));
	EMSG2("E627: missing colon: %s", cmd);
	return;
    }
    ++verb; /* skip colon */

    for (q = verb; *q; q++)
    {
	if (*q == '!')
	{
	    *q++ = NUL;
	    isfunc = 0;
	    break;
	}
	else if (*q == '/')
	{
	    *q++ = NUL;
	    isfunc = 1;
	    break;
	}
    }

    if (isfunc < 0)
    {
	nbdebug(("    missing ! or / in: %s\n", cmd));
	EMSG2("E628: missing ! or / in: %s", cmd);
	return;
    }

    r_cmdno = strtol(q, &q, 10);

    q = (char *)skipwhite((char_u *)q);

    if (nb_do_cmd(bufno, (char_u *)verb, isfunc, r_cmdno, (char_u *)q) == FAIL)
    {
#ifdef NBDEBUG
	/*
	 * This happens because the ExtEd can send a cammand or 2 after
	 * doing a stopDocumentListen command. It doesn't harm anything
	 * so I'm disabling it except for debugging.
	 */
	nbdebug(("nb_parse_cmd: Command error for \"%s\"\n", cmd));
	EMSG("E629: bad return from nb_do_cmd");
#endif
    }
}

struct nbbuf_struct
{
    buf_T		*bufp;
    unsigned int	 fireChanges:1;
    unsigned int	 initDone:1;
    unsigned int	 insertDone:1;
    unsigned int	 modified:1;
    int			 nbbuf_number;
    char		*displayname;
    int			*signmap;
    short_u		 signmaplen;
    short_u		 signmapused;
};

typedef struct nbbuf_struct nbbuf_T;

static nbbuf_T *buf_list = 0;
static int buf_list_size = 0;	/* size of buf_list */
static int buf_list_used = 0;	/* nr of entries in buf_list actually in use */

static char **globalsignmap;
static int globalsignmaplen;
static int globalsignmapused;

static int  mapsigntype __ARGS((nbbuf_T *, int localsigntype));
static void addsigntype __ARGS((nbbuf_T *, int localsigntype, char_u *typeName,
			char_u *tooltip, char_u *glyphfile,
			int usefg, int fg, int usebg, int bg));
static void print_read_msg __ARGS((nbbuf_T *buf));
static void print_save_msg __ARGS((nbbuf_T *buf, long nchars));

static int curPCtype = -1;

/*
 * Get the Netbeans buffer number for the specified buffer.
 */
    static int
nb_getbufno(buf_T *bufp)
{
    int i;

    for (i = 0; i < buf_list_used; i++)
	if (buf_list[i].bufp == bufp)
	    return i;
    return -1;
}

/*
 * Is this a NetBeans-owned buffer?
 */
    int
isNetbeansBuffer(buf_T *bufp)
{
    return usingNetbeans && bufp->b_netbeans_file;
}

/*
 * NetBeans and Vim have different undo models. In Vim, the file isn't
 * changed if changes are undone via the undo command. In NetBeans, once
 * a change has been made the file is marked as modified until saved. It
 * doesn't matter if the change was undone.
 *
 * So this function is for the corner case where Vim thinks a buffer is
 * unmodified but NetBeans thinks it IS modified.
 */
    int
isNetbeansModified(buf_T *bufp)
{
    if (usingNetbeans && bufp->b_netbeans_file)
    {
	int bufno = nb_getbufno(bufp);

	if (bufno > 0)
	    return buf_list[bufno].modified;
	else
	    return FALSE;
    }
    else
	return FALSE;
}

/*
 * Given a Netbeans buffer number, return the netbeans buffer.
 * Returns NULL for 0 or a negative number. A 0 bufno means a
 * non-buffer related command has been sent.
 */
    static nbbuf_T *
nb_get_buf(int bufno)
{
    /* find or create a buffer with the given number */
    int incr;

    if (bufno <= 0)
	return NULL;

    if (!buf_list)
    {
	/* initialize */
	buf_list = (nbbuf_T *)alloc_clear(100 * sizeof(nbbuf_T));
	buf_list_size = 100;
    }
    if (bufno >= buf_list_used) /* new */
    {
	if (bufno >= buf_list_size) /* grow list */
	{
	    incr = bufno - buf_list_size + 90;
	    buf_list_size += incr;
	    buf_list = (nbbuf_T *)vim_realloc(
				   buf_list, buf_list_size * sizeof(nbbuf_T));
	    memset(buf_list + buf_list_size - incr, 0, incr * sizeof(nbbuf_T));
	}

	while (buf_list_used <= bufno)
	{
	    /* Default is to fire text changes. */
	    buf_list[buf_list_used].fireChanges = 1;
	    ++buf_list_used;
	}
    }

    return buf_list + bufno;
}

/*
 * Return the number of buffers that are modified.
 */
    static int
count_changed_buffers(void)
{
    buf_T	*bufp;
    int		n;

    n = 0;
    for (bufp = firstbuf; bufp != NULL; bufp = bufp->b_next)
	if (bufp->b_changed)
	    ++n;
    return n;
}

/*
 * End the netbeans session.
 */
    void
netbeans_end(void)
{
    int i;
    static char buf[128];

    if (!haveConnection)
	return;

    for (i = 0; i < buf_list_used; i++)
    {
	if (!buf_list[i].bufp)
	    continue;
	if (netbeansForcedQuit)
	{
	    /* mark as unmodified so NetBeans won't put up dialog on "killed" */
	    sprintf(buf, "%d:unmodified=%d\n", i, r_cmdno);
	    nbdebug(("EVT: %s", buf));
	    nb_send(buf, "netbeans_end");
	}
	sprintf(buf, "%d:killed=%d\n", i, r_cmdno);
	nbdebug(("EVT: %s", buf));
/*	nb_send(buf, "netbeans_end");    avoid "write failed" messages */
	if (sd >= 0)
	    ignored = sock_write(sd, buf, (int)STRLEN(buf));
    }
}

/*
 * Send a message to netbeans.
 */
    static void
nb_send(char *buf, char *fun)
{
    /* Avoid giving pages full of error messages when the other side has
     * exited, only mention the first error until the connection works again. */
    static int did_error = FALSE;

    if (sd < 0)
    {
	if (!did_error)
	{
	    nbdebug(("    %s(): write while not connected\n", fun));
	    EMSG2("E630: %s(): write while not connected", fun);
	}
	did_error = TRUE;
    }
    else if (sock_write(sd, buf, (int)STRLEN(buf)) != (int)STRLEN(buf))
    {
	if (!did_error)
	{
	    nbdebug(("    %s(): write failed\n", fun));
	    EMSG2("E631: %s(): write failed", fun);
	}
	did_error = TRUE;
    }
    else
	did_error = FALSE;
}

/*
 * Some input received from netbeans requires a response. This function
 * handles a response with no information (except the command number).
 */
    static void
nb_reply_nil(int cmdno)
{
    char reply[32];

    if (!haveConnection)
	return;

    nbdebug(("REP %d: <none>\n", cmdno));

    sprintf(reply, "%d\n", cmdno);
    nb_send(reply, "nb_reply_nil");
}


/*
 * Send a response with text.
 * "result" must have been quoted already (using nb_quote()).
 */
    static void
nb_reply_text(int cmdno, char_u *result)
{
    char_u *reply;

    if (!haveConnection)
	return;

    nbdebug(("REP %d: %s\n", cmdno, (char *)result));

    reply = alloc((unsigned)STRLEN(result) + 32);
    sprintf((char *)reply, "%d %s\n", cmdno, (char *)result);
    nb_send((char *)reply, "nb_reply_text");

    vim_free(reply);
}


/*
 * Send a response with a number result code.
 */
    static void
nb_reply_nr(int cmdno, long result)
{
    char reply[32];

    if (!haveConnection)
	return;

    nbdebug(("REP %d: %ld\n", cmdno, result));

    sprintf(reply, "%d %ld\n", cmdno, result);
    nb_send(reply, "nb_reply_nr");
}


/*
 * Encode newline, ret, backslash, double quote for transmission to NetBeans.
 */
    static char_u *
nb_quote(char_u *txt)
{
    char_u *buf = alloc((unsigned)(2 * STRLEN(txt) + 1));
    char_u *p = txt;
    char_u *q = buf;

    if (buf == NULL)
	return NULL;
    for (; *p; p++)
    {
	switch (*p)
	{
	    case '\"':
	    case '\\':
		*q++ = '\\'; *q++ = *p; break;
	 /* case '\t': */
	 /*     *q++ = '\\'; *q++ = 't'; break; */
	    case '\n':
		*q++ = '\\'; *q++ = 'n'; break;
	    case '\r':
		*q++ = '\\'; *q++ = 'r'; break;
	    default:
		*q++ = *p;
		break;
	}
    }
    *q++ = '\0';

    return buf;
}


/*
 * Remove top level double quotes; convert backslashed chars.
 * Returns an allocated string (NULL for failure).
 * If "endp" is not NULL it is set to the character after the terminating
 * quote.
 */
    static char *
nb_unquote(char_u *p, char_u **endp)
{
    char *result = 0;
    char *q;
    int done = 0;

    /* result is never longer than input */
    result = (char *)alloc_clear((unsigned)STRLEN(p) + 1);
    if (result == NULL)
	return NULL;

    if (*p++ != '"')
    {
	nbdebug(("nb_unquote called with string that doesn't start with a quote!: %s\n",
			p));
	result[0] = NUL;
	return result;
    }

    for (q = result; !done && *p != NUL;)
    {
	switch (*p)
	{
	    case '"':
		/*
		 * Unbackslashed dquote marks the end, if first char was dquote.
		 */
		done = 1;
		break;

	    case '\\':
		++p;
		switch (*p)
		{
		    case '\\':	*q++ = '\\';	break;
		    case 'n':	*q++ = '\n';	break;
		    case 't':	*q++ = '\t';	break;
		    case 'r':	*q++ = '\r';	break;
		    case '"':	*q++ = '"';	break;
		    case NUL:	--p;		break;
		    /* default: skip over illegal chars */
		}
		++p;
		break;

	    default:
		*q++ = *p++;
	}
    }

    if (endp != NULL)
	*endp = p;

    return result;
}

/*
 * Remove from "first" byte to "last" byte (inclusive), at line "lnum" of the
 * current buffer.  Remove to end of line when "last" is MAXCOL.
 */
    static void
nb_partialremove(linenr_T lnum, colnr_T first, colnr_T last)
{
    char_u *oldtext, *newtext;
    int oldlen;
    int lastbyte = last;

    oldtext = ml_get(lnum);
    oldlen = (int)STRLEN(oldtext);
    if (first >= (colnr_T)oldlen || oldlen == 0)  /* just in case */
	return;
    if (lastbyte >= oldlen)
	lastbyte = oldlen - 1;
    newtext = alloc(oldlen - (int)(lastbyte - first));
    if (newtext != NULL)
    {
	mch_memmove(newtext, oldtext, first);
	STRMOVE(newtext + first, oldtext + lastbyte + 1);
	nbdebug(("    NEW LINE %d: %s\n", lnum, newtext));
	ml_replace(lnum, newtext, FALSE);
    }
}

/*
 * Replace the "first" line with the concatenation of the "first" and
 * the "other" line. The "other" line is not removed.
 */
    static void
nb_joinlines(linenr_T first, linenr_T other)
{
    int len_first, len_other;
    char_u *p;

    len_first = (int)STRLEN(ml_get(first));
    len_other = (int)STRLEN(ml_get(other));
    p = alloc((unsigned)(len_first + len_other + 1));
    if (p != NULL)
    {
      mch_memmove(p, ml_get(first), len_first);
      mch_memmove(p + len_first, ml_get(other), len_other + 1);
      ml_replace(first, p, FALSE);
    }
}

#define SKIP_STOP 2
#define streq(a,b) (strcmp(a,b) == 0)
static int needupdate = 0;
static int inAtomic = 0;

/*
 * Do the actual processing of a single netbeans command or function.
 * The difference between a command and function is that a function
 * gets a response (it's required) but a command does not.
 * For arguments see comment for nb_parse_cmd().
 */
    static int
nb_do_cmd(
    int		bufno,
    char_u	*cmd,
    int		func,
    int		cmdno,
    char_u	*args)	    /* points to space before arguments or NUL */
{
    int		doupdate = 0;
    long	off = 0;
    nbbuf_T	*buf = nb_get_buf(bufno);
    static int	skip = 0;
    int		retval = OK;
    char	*cp;	    /* for when a char pointer is needed */

    nbdebug(("%s %d: (%d) %s %s\n", (func) ? "FUN" : "CMD", cmdno, bufno, cmd,
	STRCMP(cmd, "insert") == 0 ? "<text>" : (char *)args));

    if (func)
    {
/* =====================================================================*/
	if (streq((char *)cmd, "getModified"))
	{
	    if (buf == NULL || buf->bufp == NULL)
		/* Return the number of buffers that are modified. */
		nb_reply_nr(cmdno, (long)count_changed_buffers());
	    else
		/* Return whether the buffer is modified. */
		nb_reply_nr(cmdno, (long)(buf->bufp->b_changed
					   || isNetbeansModified(buf->bufp)));
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "saveAndExit"))
	{
	    /* Note: this will exit Vim if successful. */
	    coloncmd(":confirm qall");

	    /* We didn't exit: return the number of changed buffers. */
	    nb_reply_nr(cmdno, (long)count_changed_buffers());
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "getCursor"))
	{
	    char_u text[200];

	    /* Note: nb_getbufno() may return -1.  This indicates the IDE
	     * didn't assign a number to the current buffer in response to a
	     * fileOpened event. */
	    sprintf((char *)text, "%d %ld %d %ld",
		    nb_getbufno(curbuf),
		    (long)curwin->w_cursor.lnum,
		    (int)curwin->w_cursor.col,
		    pos2off(curbuf, &curwin->w_cursor));
	    nb_reply_text(cmdno, text);
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "getAnno"))
	{
	    long linenum = 0;
#ifdef FEAT_SIGNS
	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    Invalid buffer identifier in getAnno\n"));
		EMSG("E652: Invalid buffer identifier in getAnno");
		retval = FAIL;
	    }
	    else
	    {
		int serNum;

		cp = (char *)args;
		serNum = strtol(cp, &cp, 10);
		/* If the sign isn't found linenum will be zero. */
		linenum = (long)buf_findsign(buf->bufp, serNum);
	    }
#endif
	    nb_reply_nr(cmdno, linenum);
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "getLength"))
	{
	    long len = 0;

	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in getLength\n"));
		EMSG("E632: invalid buffer identifier in getLength");
		retval = FAIL;
	    }
	    else
	    {
		len = get_buf_size(buf->bufp);
	    }
	    nb_reply_nr(cmdno, len);
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "getText"))
	{
	    long	len;
	    linenr_T	nlines;
	    char_u	*text = NULL;
	    linenr_T	lno = 1;
	    char_u	*p;
	    char_u	*line;

	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in getText\n"));
		EMSG("E633: invalid buffer identifier in getText");
		retval = FAIL;
	    }
	    else
	    {
		len = get_buf_size(buf->bufp);
		nlines = buf->bufp->b_ml.ml_line_count;
		text = alloc((unsigned)((len > 0)
						 ? ((len + nlines) * 2) : 4));
		if (text == NULL)
		{
		    nbdebug(("    nb_do_cmd: getText has null text field\n"));
		    retval = FAIL;
		}
		else
		{
		    p = text;
		    *p++ = '\"';
		    for (; lno <= nlines ; lno++)
		    {
			line = nb_quote(ml_get_buf(buf->bufp, lno, FALSE));
			if (line != NULL)
			{
			    STRCPY(p, line);
			    p += STRLEN(line);
			    *p++ = '\\';
			    *p++ = 'n';
			    vim_free(line);
			}
		    }
		    *p++ = '\"';
		    *p = '\0';
		}
	    }
	    if (text == NULL)
		nb_reply_text(cmdno, (char_u *)"");
	    else
	    {
		nb_reply_text(cmdno, text);
		vim_free(text);
	    }
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "remove"))
	{
	    long count;
	    pos_T first, last;
	    pos_T *pos;
	    pos_T *next;
	    linenr_T del_from_lnum, del_to_lnum;  /* lines to be deleted as a whole */
	    int oldFire = netbeansFireChanges;
	    int oldSuppress = netbeansSuppressNoLines;
	    int wasChanged;

	    if (skip >= SKIP_STOP)
	    {
		nbdebug(("    Skipping %s command\n", (char *) cmd));
		nb_reply_nil(cmdno);
		return OK;
	    }

	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in remove\n"));
		EMSG("E634: invalid buffer identifier in remove");
		retval = FAIL;
	    }
	    else
	    {
		netbeansFireChanges = FALSE;
		netbeansSuppressNoLines = TRUE;

		nb_set_curbuf(buf->bufp);
		wasChanged = buf->bufp->b_changed;
		cp = (char *)args;
		off = strtol(cp, &cp, 10);
		count = strtol(cp, &cp, 10);
		args = (char_u *)cp;
		/* delete "count" chars, starting at "off" */
		pos = off2pos(buf->bufp, off);
		if (!pos)
		{
		    nbdebug(("    !bad position\n"));
		    nb_reply_text(cmdno, (char_u *)"!bad position");
		    netbeansFireChanges = oldFire;
		    netbeansSuppressNoLines = oldSuppress;
		    return FAIL;
		}
		first = *pos;
		nbdebug(("    FIRST POS: line %d, col %d\n", first.lnum, first.col));
		pos = off2pos(buf->bufp, off+count-1);
		if (!pos)
		{
		    nbdebug(("    !bad count\n"));
		    nb_reply_text(cmdno, (char_u *)"!bad count");
		    netbeansFireChanges = oldFire;
		    netbeansSuppressNoLines = oldSuppress;
		    return FAIL;
		}
		last = *pos;
		nbdebug(("    LAST POS: line %d, col %d\n", last.lnum, last.col));
		del_from_lnum = first.lnum;
		del_to_lnum = last.lnum;
		doupdate = 1;

		/* Get the position of the first byte after the deleted
		 * section.  "next" is NULL when deleting to the end of the
		 * file. */
		next = off2pos(buf->bufp, off + count);

		/* Remove part of the first line. */
		if (first.col != 0 || (next != NULL && first.lnum == next->lnum))
		{
		    if (first.lnum != last.lnum
			    || (next != NULL && first.lnum != next->lnum))
		    {
			/* remove to the end of the first line */
			nb_partialremove(first.lnum, first.col,
							     (colnr_T)MAXCOL);
			if (first.lnum == last.lnum)
			{
			    /* Partial line to remove includes the end of
			     * line.  Join the line with the next one, have
			     * the next line deleted below. */
			    nb_joinlines(first.lnum, next->lnum);
			    del_to_lnum = next->lnum;
			}
		    }
		    else
		    {
			/* remove within one line */
			nb_partialremove(first.lnum, first.col, last.col);
		    }
		    ++del_from_lnum;  /* don't delete the first line */
		}

		/* Remove part of the last line. */
		if (first.lnum != last.lnum && next != NULL
			&& next->col != 0 && last.lnum == next->lnum)
		{
		    nb_partialremove(last.lnum, 0, last.col);
		    if (del_from_lnum > first.lnum)
		    {
			/* Join end of last line to start of first line; last
			 * line is deleted below. */
			nb_joinlines(first.lnum, last.lnum);
		    }
		    else
			/* First line is deleted as a whole, keep the last
			 * line. */
			--del_to_lnum;
		}

		/* First is partial line; last line to remove includes
		 * the end of line; join first line to line following last
		 * line; line following last line is deleted below. */
		if (first.lnum != last.lnum && del_from_lnum > first.lnum
			&& next != NULL && last.lnum != next->lnum)
		{
		    nb_joinlines(first.lnum, next->lnum);
		    del_to_lnum = next->lnum;
		}

		/* Delete whole lines if there are any. */
		if (del_to_lnum >= del_from_lnum)
		{
		    int i;

		    /* delete signs from the lines being deleted */
		    for (i = del_from_lnum; i <= del_to_lnum; i++)
		    {
			int id = buf_findsign_id(buf->bufp, (linenr_T)i);
			if (id > 0)
			{
			    nbdebug(("    Deleting sign %d on line %d\n", id, i));
			    buf_delsign(buf->bufp, id);
			}
			else
			    nbdebug(("    No sign on line %d\n", i));
		    }

		    nbdebug(("    Deleting lines %d through %d\n", del_from_lnum, del_to_lnum));
		    curwin->w_cursor.lnum = del_from_lnum;
		    curwin->w_cursor.col = 0;
		    del_lines(del_to_lnum - del_from_lnum + 1, FALSE);
		}

		/* Leave cursor at first deleted byte. */
		curwin->w_cursor = first;
		check_cursor_lnum();
		buf->bufp->b_changed = wasChanged; /* logically unchanged */
		netbeansFireChanges = oldFire;
		netbeansSuppressNoLines = oldSuppress;

		u_blockfree(buf->bufp);
		u_clearall(buf->bufp);
	    }
	    nb_reply_nil(cmdno);
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "insert"))
	{
	    char_u	*to_free;

	    if (skip >= SKIP_STOP)
	    {
		nbdebug(("    Skipping %s command\n", (char *) cmd));
		nb_reply_nil(cmdno);
		return OK;
	    }

	    /* get offset */
	    cp = (char *)args;
	    off = strtol(cp, &cp, 10);
	    args = (char_u *)cp;

	    /* get text to be inserted */
	    args = skipwhite(args);
	    args = to_free = (char_u *)nb_unquote(args, NULL);
	    /*
	    nbdebug(("    CHUNK[%d]: %d bytes at offset %d\n",
		    buf->bufp->b_ml.ml_line_count, STRLEN(args), off));
	    */

	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in insert\n"));
		EMSG("E635: invalid buffer identifier in insert");
		retval = FAIL;
	    }
	    else if (args != NULL)
	    {
		int	ff_detected = EOL_UNKNOWN;
		int	buf_was_empty = (buf->bufp->b_ml.ml_flags & ML_EMPTY);
		size_t	len = 0;
		int	added = 0;
		int	oldFire = netbeansFireChanges;
		int	old_b_changed;
		char_u	*nl;
		linenr_T lnum;
		linenr_T lnum_start;
		pos_T	*pos;

		netbeansFireChanges = 0;

		/* Jump to the buffer where we insert.  After this "curbuf"
		 * can be used. */
		nb_set_curbuf(buf->bufp);
		old_b_changed = curbuf->b_changed;

		/* Convert the specified character offset into a lnum/col
		 * position. */
		pos = off2pos(curbuf, off);
		if (pos != NULL)
		{
		    if (pos->lnum <= 0)
			lnum_start = 1;
		    else
			lnum_start = pos->lnum;
		}
		else
		{
		    /* If the given position is not found, assume we want
		     * the end of the file.  See setLocAndSize HACK. */
		    if (buf_was_empty)
			lnum_start = 1;	    /* above empty line */
		    else
			lnum_start = curbuf->b_ml.ml_line_count + 1;
		}

		/* "lnum" is the line where we insert: either append to it or
		 * insert a new line above it. */
		lnum = lnum_start;

		/* Loop over the "\n" separated lines of the argument. */
		doupdate = 1;
		while (*args != NUL)
		{
		    nl = vim_strchr(args, '\n');
		    if (nl == NULL)
		    {
			/* Incomplete line, probably truncated.  Next "insert"
			 * command should append to this one. */
			len = STRLEN(args);
		    }
		    else
		    {
			len = nl - args;

			/*
			 * We need to detect EOL style, because the commands
			 * use a character offset.
			 */
			if (nl > args && nl[-1] == '\r')
			{
			    ff_detected = EOL_DOS;
			    --len;
			}
			else
			    ff_detected = EOL_UNIX;
		    }
		    args[len] = NUL;

		    if (lnum == lnum_start
			    && ((pos != NULL && pos->col > 0)
				|| (lnum == 1 && buf_was_empty)))
		    {
			char_u *oldline = ml_get(lnum);
			char_u *newline;

			/* Insert halfway a line.  For simplicity we assume we
			 * need to append to the line. */
			newline = alloc_check((unsigned)(STRLEN(oldline) + len + 1));
			if (newline != NULL)
			{
			    STRCPY(newline, oldline);
			    STRCAT(newline, args);
			    ml_replace(lnum, newline, FALSE);
			}
		    }
		    else
		    {
			/* Append a new line.  Not that we always do this,
			 * also when the text doesn't end in a "\n". */
			ml_append((linenr_T)(lnum - 1), args, (colnr_T)(len + 1), FALSE);
			++added;
		    }

		    if (nl == NULL)
			break;
		    ++lnum;
		    args = nl + 1;
		}

		/* Adjust the marks below the inserted lines. */
		appended_lines_mark(lnum_start - 1, (long)added);

		/*
		 * When starting with an empty buffer set the fileformat.
		 * This is just guessing...
		 */
		if (buf_was_empty)
		{
		    if (ff_detected == EOL_UNKNOWN)
#if defined(MSDOS) || defined(MSWIN) || defined(OS2)
			ff_detected = EOL_DOS;
#else
			ff_detected = EOL_UNIX;
#endif
		    set_fileformat(ff_detected, OPT_LOCAL);
		    curbuf->b_start_ffc = *curbuf->b_p_ff;
		}

		/*
		 * XXX - GRP - Is the next line right? If I've inserted
		 * text the buffer has been updated but not written. Will
		 * netbeans guarantee to write it? Even if I do a :q! ?
		 */
		curbuf->b_changed = old_b_changed; /* logically unchanged */
		netbeansFireChanges = oldFire;

		/* Undo info is invalid now... */
		u_blockfree(curbuf);
		u_clearall(curbuf);
	    }
	    vim_free(to_free);
	    nb_reply_nil(cmdno); /* or !error */
	}
	else
	{
	    nbdebug(("UNIMPLEMENTED FUNCTION: %s\n", cmd));
	    nb_reply_nil(cmdno);
	    retval = FAIL;
	}
    }
    else /* Not a function; no reply required. */
    {
/* =====================================================================*/
	if (streq((char *)cmd, "create"))
	{
	    /* Create a buffer without a name. */
	    if (buf == NULL)
	    {
		nbdebug(("    invalid buffer identifier in create\n"));
		EMSG("E636: invalid buffer identifier in create");
		return FAIL;
	    }
	    vim_free(buf->displayname);
	    buf->displayname = NULL;

	    netbeansReadFile = 0; /* don't try to open disk file */
	    do_ecmd(0, NULL, 0, 0, ECMD_ONE, ECMD_HIDE + ECMD_OLDBUF, curwin);
	    netbeansReadFile = 1;
	    buf->bufp = curbuf;
	    maketitle();
	    buf->insertDone = FALSE;
	    gui_update_menus(0);
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "insertDone"))
	{
	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in insertDone\n"));
	    }
	    else
	    {
		buf->bufp->b_start_eol = *args == 'T';
		buf->insertDone = TRUE;
		args += 2;
		buf->bufp->b_p_ro = *args == 'T';
		print_read_msg(buf);
	    }
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "saveDone"))
	{
	    long savedChars = atol((char *)args);

	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in saveDone\n"));
	    }
	    else
		print_save_msg(buf, savedChars);
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "startDocumentListen"))
	{
	    if (buf == NULL)
	    {
		nbdebug(("    invalid buffer identifier in startDocumentListen\n"));
		EMSG("E637: invalid buffer identifier in startDocumentListen");
		return FAIL;
	    }
	    buf->fireChanges = 1;
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "stopDocumentListen"))
	{
	    if (buf == NULL)
	    {
		nbdebug(("    invalid buffer identifier in stopDocumentListen\n"));
		EMSG("E638: invalid buffer identifier in stopDocumentListen");
		return FAIL;
	    }
	    buf->fireChanges = 0;
	    if (buf->bufp != NULL && buf->bufp->b_was_netbeans_file)
	    {
		if (!buf->bufp->b_netbeans_file)
		{
		    nbdebug(("E658: NetBeans connection lost for buffer %ld\n", buf->bufp->b_fnum));
		    EMSGN(_("E658: NetBeans connection lost for buffer %ld"),
							   buf->bufp->b_fnum);
		}
		else
		{
		    /* NetBeans uses stopDocumentListen when it stops editing
		     * a file.  It then expects the buffer in Vim to
		     * disappear. */
		    do_bufdel(DOBUF_DEL, (char_u *)"", 1,
				  buf->bufp->b_fnum, buf->bufp->b_fnum, TRUE);
		    vim_memset(buf, 0, sizeof(nbbuf_T));
		}
	    }
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "setTitle"))
	{
	    if (buf == NULL)
	    {
		nbdebug(("    invalid buffer identifier in setTitle\n"));
		EMSG("E639: invalid buffer identifier in setTitle");
		return FAIL;
	    }
	    vim_free(buf->displayname);
	    buf->displayname = nb_unquote(args, NULL);
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "initDone"))
	{
	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in initDone\n"));
		EMSG("E640: invalid buffer identifier in initDone");
		return FAIL;
	    }
	    doupdate = 1;
	    buf->initDone = TRUE;
	    nb_set_curbuf(buf->bufp);
#if defined(FEAT_AUTOCMD)
	    apply_autocmds(EVENT_BUFREADPOST, 0, 0, FALSE, buf->bufp);
#endif

	    /* handle any postponed key commands */
	    handle_key_queue();
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "setBufferNumber")
		|| streq((char *)cmd, "putBufferNumber"))
	{
	    char_u	*path;
	    buf_T	*bufp;

	    if (buf == NULL)
	    {
		nbdebug(("    invalid buffer identifier in setBufferNumber\n"));
		EMSG("E641: invalid buffer identifier in setBufferNumber");
		return FAIL;
	    }
	    path = (char_u *)nb_unquote(args, NULL);
	    if (path == NULL)
		return FAIL;
	    bufp = buflist_findname(path);
	    vim_free(path);
	    if (bufp == NULL)
	    {
	    	nbdebug(("    File %s not found in setBufferNumber\n", args));
		EMSG2("E642: File %s not found in setBufferNumber", args);
		return FAIL;
	    }
	    buf->bufp = bufp;
	    buf->nbbuf_number = bufp->b_fnum;

	    /* "setBufferNumber" has the side effect of jumping to the buffer
	     * (don't know why!).  Don't do that for "putBufferNumber". */
	    if (*cmd != 'p')
		coloncmd(":buffer %d", bufp->b_fnum);
	    else
	    {
		buf->initDone = TRUE;

		/* handle any postponed key commands */
		handle_key_queue();
	    }

#if 0  /* never used */
	    buf->internalname = (char *)alloc_clear(8);
	    sprintf(buf->internalname, "<%d>", bufno);
	    buf->netbeansOwns = 0;
#endif
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "setFullName"))
	{
	    if (buf == NULL)
	    {
		nbdebug(("    invalid buffer identifier in setFullName\n"));
		EMSG("E643: invalid buffer identifier in setFullName");
		return FAIL;
	    }
	    vim_free(buf->displayname);
	    buf->displayname = nb_unquote(args, NULL);

	    netbeansReadFile = 0; /* don't try to open disk file */
	    do_ecmd(0, (char_u *)buf->displayname, 0, 0, ECMD_ONE,
					     ECMD_HIDE + ECMD_OLDBUF, curwin);
	    netbeansReadFile = 1;
	    buf->bufp = curbuf;
	    maketitle();
	    gui_update_menus(0);
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "editFile"))
	{
	    if (buf == NULL)
	    {
		nbdebug(("    invalid buffer identifier in editFile\n"));
		EMSG("E644: invalid buffer identifier in editFile");
		return FAIL;
	    }
	    /* Edit a file: like create + setFullName + read the file. */
	    vim_free(buf->displayname);
	    buf->displayname = nb_unquote(args, NULL);
	    do_ecmd(0, (char_u *)buf->displayname, NULL, NULL, ECMD_ONE,
					     ECMD_HIDE + ECMD_OLDBUF, curwin);
	    buf->bufp = curbuf;
	    buf->initDone = TRUE;
	    doupdate = 1;
#if defined(FEAT_TITLE)
	    maketitle();
#endif
	    gui_update_menus(0);
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "setVisible"))
	{
	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in setVisible\n"));
		/* This message was commented out, probably because it can
		 * happen when shutting down. */
		if (p_verbose > 0)
		    EMSG("E645: invalid buffer identifier in setVisible");
		return FAIL;
	    }
	    if (streq((char *)args, "T") && buf->bufp != curbuf)
	    {
		exarg_T exarg;
		exarg.cmd = (char_u *)"goto";
		exarg.forceit = FALSE;
		dosetvisible = TRUE;
		goto_buffer(&exarg, DOBUF_FIRST, FORWARD, buf->bufp->b_fnum);
		doupdate = 1;
		dosetvisible = FALSE;

		/* Side effect!!!. */
		if (!gui.starting)
		    gui_mch_set_foreground();
	    }
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "raise"))
	{
	    /* Bring gvim to the foreground. */
	    if (!gui.starting)
		gui_mch_set_foreground();
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "setModified"))
	{
	    int prev_b_changed;

	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in setModified\n"));
		/* This message was commented out, probably because it can
		 * happen when shutting down. */
		if (p_verbose > 0)
		    EMSG("E646: invalid buffer identifier in setModified");
		return FAIL;
	    }
	    prev_b_changed = buf->bufp->b_changed;
	    if (streq((char *)args, "T"))
		buf->bufp->b_changed = TRUE;
	    else
	    {
		struct stat	st;

		/* Assume NetBeans stored the file.  Reset the timestamp to
		 * avoid "file changed" warnings. */
		if (buf->bufp->b_ffname != NULL
			&& mch_stat((char *)buf->bufp->b_ffname, &st) >= 0)
		    buf_store_time(buf->bufp, &st, buf->bufp->b_ffname);
		buf->bufp->b_changed = FALSE;
	    }
	    buf->modified = buf->bufp->b_changed;
	    if (prev_b_changed != buf->bufp->b_changed)
	    {
#ifdef FEAT_WINDOWS
		check_status(buf->bufp);
		redraw_tabline = TRUE;
#endif
#ifdef FEAT_TITLE
		maketitle();
#endif
		update_screen(0);
	    }
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "setModtime"))
	{
	    if (buf == NULL || buf->bufp == NULL)
		nbdebug(("    invalid buffer identifier in setModtime\n"));
	    else
		buf->bufp->b_mtime = atoi((char *)args);
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "setReadOnly"))
	{
	    if (buf == NULL || buf->bufp == NULL)
		nbdebug(("    invalid buffer identifier in setReadOnly\n"));
	    else if (streq((char *)args, "T"))
		buf->bufp->b_p_ro = TRUE;
	    else
		buf->bufp->b_p_ro = FALSE;
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "setMark"))
	{
	    /* not yet */
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "showBalloon"))
	{
#if defined(FEAT_BEVAL)
	    static char	*text = NULL;

	    /*
	     * Set up the Balloon Expression Evaluation area.
	     * Ignore 'ballooneval' here.
	     * The text pointer must remain valid for a while.
	     */
	    if (balloonEval != NULL)
	    {
		vim_free(text);
		text = nb_unquote(args, NULL);
		if (text != NULL)
		    gui_mch_post_balloon(balloonEval, (char_u *)text);
	    }
#endif
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "setDot"))
	{
	    pos_T *pos;
#ifdef NBDEBUG
	    char_u *s;
#endif

	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in setDot\n"));
		EMSG("E647: invalid buffer identifier in setDot");
		return FAIL;
	    }

	    nb_set_curbuf(buf->bufp);

#ifdef FEAT_VISUAL
	    /* Don't want Visual mode now. */
	    if (VIsual_active)
		end_visual_mode();
#endif
#ifdef NBDEBUG
	    s = args;
#endif
	    pos = get_off_or_lnum(buf->bufp, &args);
	    if (pos)
	    {
		curwin->w_cursor = *pos;
		check_cursor();
#ifdef FEAT_FOLDING
		foldOpenCursor();
#endif
	    }
	    else
		nbdebug(("    BAD POSITION in setDot: %s\n", s));

	    /* gui_update_cursor(TRUE, FALSE); */
	    /* update_curbuf(NOT_VALID); */
	    update_topline();		/* scroll to show the line */
	    update_screen(VALID);
	    setcursor();
	    out_flush();
	    gui_update_cursor(TRUE, FALSE);
	    gui_mch_flush();
	    /* Quit a hit-return or more prompt. */
	    if (State == HITRETURN || State == ASKMORE)
	    {
#ifdef FEAT_GUI_GTK
		if (gtk_main_level() > 0)
		    gtk_main_quit();
#endif
	    }
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "close"))
	{
#ifdef NBDEBUG
	    char *name = "<NONE>";
#endif

	    if (buf == NULL)
	    {
		nbdebug(("    invalid buffer identifier in close\n"));
		EMSG("E648: invalid buffer identifier in close");
		return FAIL;
	    }

#ifdef NBDEBUG
	    if (buf->displayname != NULL)
		name = buf->displayname;
#endif
	    if (buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in close\n"));
		/* This message was commented out, probably because it can
		 * happen when shutting down. */
		if (p_verbose > 0)
		    EMSG("E649: invalid buffer identifier in close");
	    }
	    nbdebug(("    CLOSE %d: %s\n", bufno, name));
	    need_mouse_correct = TRUE;
	    if (buf->bufp != NULL)
		do_buffer(DOBUF_WIPE, DOBUF_FIRST, FORWARD,
						     buf->bufp->b_fnum, TRUE);
	    buf->bufp = NULL;
	    buf->initDone = FALSE;
	    doupdate = 1;
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "setStyle")) /* obsolete... */
	{
	    nbdebug(("    setStyle is obsolete!\n"));
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "setExitDelay"))
	{
	    /* Only used in version 2.1. */
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "defineAnnoType"))
	{
#ifdef FEAT_SIGNS
	    int typeNum;
	    char_u *typeName;
	    char_u *tooltip;
	    char_u *p;
	    char_u *glyphFile;
	    int use_fg = 0;
	    int use_bg = 0;
	    int fg = -1;
	    int bg = -1;

	    if (buf == NULL)
	    {
		nbdebug(("    invalid buffer identifier in defineAnnoType\n"));
		EMSG("E650: invalid buffer identifier in defineAnnoType");
		return FAIL;
	    }

	    cp = (char *)args;
	    typeNum = strtol(cp, &cp, 10);
	    args = (char_u *)cp;
	    args = skipwhite(args);
	    typeName = (char_u *)nb_unquote(args, &args);
	    args = skipwhite(args + 1);
	    tooltip = (char_u *)nb_unquote(args, &args);
	    args = skipwhite(args + 1);

	    p = (char_u *)nb_unquote(args, &args);
	    glyphFile = vim_strsave_escaped(p, escape_chars);
	    vim_free(p);

	    args = skipwhite(args + 1);
	    if (STRNCMP(args, "none", 4) == 0)
		args += 5;
	    else
	    {
		use_fg = 1;
		cp = (char *)args;
		fg = strtol(cp, &cp, 10);
		args = (char_u *)cp;
	    }
	    if (STRNCMP(args, "none", 4) == 0)
		args += 5;
	    else
	    {
		use_bg = 1;
		cp = (char *)args;
		bg = strtol(cp, &cp, 10);
		args = (char_u *)cp;
	    }
	    if (typeName != NULL && tooltip != NULL && glyphFile != NULL)
		addsigntype(buf, typeNum, typeName, tooltip, glyphFile,
						      use_fg, fg, use_bg, bg);
	    else
		vim_free(typeName);

	    /* don't free typeName; it's used directly in addsigntype() */
	    vim_free(tooltip);
	    vim_free(glyphFile);

#endif
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "addAnno"))
	{
#ifdef FEAT_SIGNS
	    int serNum;
	    int localTypeNum;
	    int typeNum;
	    pos_T *pos;

	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in addAnno\n"));
		EMSG("E651: invalid buffer identifier in addAnno");
		return FAIL;
	    }

	    doupdate = 1;

	    cp = (char *)args;
	    serNum = strtol(cp, &cp, 10);

	    /* Get the typenr specific for this buffer and convert it to
	     * the global typenumber, as used for the sign name. */
	    localTypeNum = strtol(cp, &cp, 10);
	    args = (char_u *)cp;
	    typeNum = mapsigntype(buf, localTypeNum);

	    pos = get_off_or_lnum(buf->bufp, &args);

	    cp = (char *)args;
	    ignored = (int)strtol(cp, &cp, 10);
	    args = (char_u *)cp;
# ifdef NBDEBUG
	    if (ignored != -1)
	    {
		nbdebug(("    partial line annotation -- Not Yet Implemented!\n"));
	    }
# endif
	    if (serNum >= GUARDEDOFFSET)
	    {
		nbdebug(("    too many annotations! ignoring...\n"));
		return FAIL;
	    }
	    if (pos)
	    {
		coloncmd(":sign place %d line=%d name=%d buffer=%d",
			   serNum, pos->lnum, typeNum, buf->bufp->b_fnum);
		if (typeNum == curPCtype)
		    coloncmd(":sign jump %d buffer=%d", serNum,
						       buf->bufp->b_fnum);
	    }
#endif
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "removeAnno"))
	{
#ifdef FEAT_SIGNS
	    int serNum;

	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in removeAnno\n"));
		return FAIL;
	    }
	    doupdate = 1;
	    cp = (char *)args;
	    serNum = strtol(cp, &cp, 10);
	    args = (char_u *)cp;
	    coloncmd(":sign unplace %d buffer=%d",
		     serNum, buf->bufp->b_fnum);
	    redraw_buf_later(buf->bufp, NOT_VALID);
#endif
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "moveAnnoToFront"))
	{
#ifdef FEAT_SIGNS
	    nbdebug(("    moveAnnoToFront: Not Yet Implemented!\n"));
#endif
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "guard") || streq((char *)cmd, "unguard"))
	{
	    int len;
	    pos_T first;
	    pos_T last;
	    pos_T *pos;
	    int un = (cmd[0] == 'u');
	    static int guardId = GUARDEDOFFSET;

	    if (skip >= SKIP_STOP)
	    {
		nbdebug(("    Skipping %s command\n", (char *) cmd));
		return OK;
	    }

	    nb_init_graphics();

	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in %s command\n", cmd));
		return FAIL;
	    }
	    nb_set_curbuf(buf->bufp);
	    cp = (char *)args;
	    off = strtol(cp, &cp, 10);
	    len = strtol(cp, NULL, 10);
	    args = (char_u *)cp;
	    pos = off2pos(buf->bufp, off);
	    doupdate = 1;
	    if (!pos)
		nbdebug(("    no such start pos in %s, %ld\n", cmd, off));
	    else
	    {
		first = *pos;
		pos = off2pos(buf->bufp, off + len - 1);
		if (pos != NULL && pos->col == 0)
		{
			/*
			 * In Java Swing the offset is a position between 2
			 * characters. If col == 0 then we really want the
			 * previous line as the end.
			 */
			pos = off2pos(buf->bufp, off + len - 2);
		}
		if (!pos)
		    nbdebug(("    no such end pos in %s, %ld\n",
			    cmd, off + len - 1));
		else
		{
		    long lnum;
		    last = *pos;
		    /* set highlight for region */
		    nbdebug(("    %sGUARD %ld,%d to %ld,%d\n", (un) ? "UN" : "",
			     first.lnum, first.col,
			     last.lnum, last.col));
#ifdef FEAT_SIGNS
		    for (lnum = first.lnum; lnum <= last.lnum; lnum++)
		    {
			if (un)
			{
			    /* never used */
			}
			else
			{
			    if (buf_findsigntype_id(buf->bufp, lnum,
				GUARDED) == 0)
			    {
				coloncmd(
				    ":sign place %d line=%d name=%d buffer=%d",
				     guardId++, lnum, GUARDED,
				     buf->bufp->b_fnum);
			    }
			}
		    }
#endif
		    redraw_buf_later(buf->bufp, NOT_VALID);
		}
	    }
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "startAtomic"))
	{
	    inAtomic = 1;
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "endAtomic"))
	{
	    inAtomic = 0;
	    if (needupdate)
	    {
		doupdate = 1;
		needupdate = 0;
	    }
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "save"))
	{
	    /*
	     * NOTE - This command is obsolete wrt NetBeans. Its left in
	     * only for historical reasons.
	     */
	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in %s command\n", cmd));
		return FAIL;
	    }

	    /* the following is taken from ex_cmds.c (do_wqall function) */
	    if (bufIsChanged(buf->bufp))
	    {
		/* Only write if the buffer can be written. */
		if (p_write
			&& !buf->bufp->b_p_ro
			&& buf->bufp->b_ffname != NULL
#ifdef FEAT_QUICKFIX
			&& !bt_dontwrite(buf->bufp)
#endif
			)
		{
		    buf_write_all(buf->bufp, FALSE);
#ifdef FEAT_AUTOCMD
		    /* an autocommand may have deleted the buffer */
		    if (!buf_valid(buf->bufp))
			buf->bufp = NULL;
#endif
		}
	    }
	    else
	    {
	        nbdebug(("    Buffer has no changes!\n"));
	    }
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "netbeansBuffer"))
	{
	    if (buf == NULL || buf->bufp == NULL)
	    {
		nbdebug(("    invalid buffer identifier in %s command\n", cmd));
		return FAIL;
	    }
	    if (*args == 'T')
	    {
		buf->bufp->b_netbeans_file = TRUE;
		buf->bufp->b_was_netbeans_file = TRUE;
	    }
	    else
		buf->bufp->b_netbeans_file = FALSE;
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "specialKeys"))
	{
	    special_keys(args);
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "actionMenuItem"))
	{
	    /* not used yet */
/* =====================================================================*/
	}
	else if (streq((char *)cmd, "version"))
	{
	    /* not used yet */
	}
	else
	{
	    nbdebug(("Unrecognised command: %s\n", cmd));
	}
	/*
	 * Unrecognized command is ignored.
	 */
    }
    if (inAtomic && doupdate)
    {
	needupdate = 1;
	doupdate = 0;
    }

    /*
     * Is this needed? I moved the netbeans_Xt_connect() later during startup
     * and it may no longer be necessary. If its not needed then needupdate
     * and doupdate can also be removed.
     */
    if (buf != NULL && buf->initDone && doupdate)
    {
	update_screen(NOT_VALID);
	setcursor();
	out_flush();
	gui_update_cursor(TRUE, FALSE);
	gui_mch_flush();
	/* Quit a hit-return or more prompt. */
	if (State == HITRETURN || State == ASKMORE)
	{
#ifdef FEAT_GUI_GTK
	    if (gtk_main_level() > 0)
		gtk_main_quit();
#endif
	}
    }

    return retval;
}


/*
 * If "buf" is not the current buffer try changing to a window that edits this
 * buffer.  If there is no such window then close the current buffer and set
 * the current buffer as "buf".
 */
    static void
nb_set_curbuf(buf_T *buf)
{
    if (curbuf != buf && buf_jump_open_win(buf) == NULL)
	set_curbuf(buf, DOBUF_GOTO);
}

/*
 * Process a vim colon command.
 */
    static void
coloncmd(char *cmd, ...)
{
    char buf[1024];
    va_list ap;

    va_start(ap, cmd);
    vsprintf(buf, cmd, ap);
    va_end(ap);

    nbdebug(("    COLONCMD %s\n", buf));

/*     ALT_INPUT_LOCK_ON; */
    do_cmdline((char_u *)buf, NULL, NULL, DOCMD_NOWAIT | DOCMD_KEYTYPED);
/*     ALT_INPUT_LOCK_OFF; */

    setcursor();		/* restore the cursor position */
    out_flush();		/* make sure output has been written */

    gui_update_cursor(TRUE, FALSE);
    gui_mch_flush();
}


/*
 * Parse the specialKeys argument and issue the appropriate map commands.
 */
    static void
special_keys(char_u *args)
{
    char *save_str = nb_unquote(args, NULL);
    char *tok = strtok(save_str, " ");
    char *sep;
    char keybuf[64];
    char cmdbuf[256];

    while (tok != NULL)
    {
	int i = 0;

	if ((sep = strchr(tok, '-')) != NULL)
	{
	    *sep = NUL;
	    while (*tok)
	    {
		switch (*tok)
		{
		    case 'A':
		    case 'M':
		    case 'C':
		    case 'S':
			keybuf[i++] = *tok;
			keybuf[i++] = '-';
			break;
		}
		tok++;
	    }
	    tok++;
	}

	strcpy(&keybuf[i], tok);
	vim_snprintf(cmdbuf, sizeof(cmdbuf),
				"<silent><%s> :nbkey %s<CR>", keybuf, keybuf);
	do_map(0, (char_u *)cmdbuf, NORMAL, FALSE);
	tok = strtok(NULL, " ");
    }
    vim_free(save_str);
}


    void
ex_nbkey(eap)
    exarg_T	*eap;
{
    netbeans_keystring(0, (char *)eap->arg);
}


/*
 * Initialize highlights and signs for use by netbeans  (mostly obsolete)
 */
    static void
nb_init_graphics(void)
{
    static int did_init = FALSE;

    if (!did_init)
    {
	coloncmd(":highlight NBGuarded guibg=Cyan guifg=Black");
	coloncmd(":sign define %d linehl=NBGuarded", GUARDED);

	did_init = TRUE;
    }
}

/*
 * Convert key to netbeans name.
 */
    static void
netbeans_keyname(int key, char *buf)
{
    char *name = 0;
    char namebuf[2];
    int ctrl  = 0;
    int shift = 0;
    int alt   = 0;

    if (mod_mask & MOD_MASK_CTRL)
	ctrl = 1;
    if (mod_mask & MOD_MASK_SHIFT)
	shift = 1;
    if (mod_mask & MOD_MASK_ALT)
	alt = 1;


    switch (key)
    {
	case K_F1:		name = "F1";		break;
	case K_S_F1:	name = "F1";	shift = 1;	break;
	case K_F2:		name = "F2";		break;
	case K_S_F2:	name = "F2";	shift = 1;	break;
	case K_F3:		name = "F3";		break;
	case K_S_F3:	name = "F3";	shift = 1;	break;
	case K_F4:		name = "F4";		break;
	case K_S_F4:	name = "F4";	shift = 1;	break;
	case K_F5:		name = "F5";		break;
	case K_S_F5:	name = "F5";	shift = 1;	break;
	case K_F6:		name = "F6";		break;
	case K_S_F6:	name = "F6";	shift = 1;	break;
	case K_F7:		name = "F7";		break;
	case K_S_F7:	name = "F7";	shift = 1;	break;
	case K_F8:		name = "F8";		break;
	case K_S_F8:	name = "F8";	shift = 1;	break;
	case K_F9:		name = "F9";		break;
	case K_S_F9:	name = "F9";	shift = 1;	break;
	case K_F10:		name = "F10";		break;
	case K_S_F10:	name = "F10";	shift = 1;	break;
	case K_F11:		name = "F11";		break;
	case K_S_F11:	name = "F11";	shift = 1;	break;
	case K_F12:		name = "F12";		break;
	case K_S_F12:	name = "F12";	shift = 1;	break;
	default:
			if (key >= ' ' && key <= '~')
			{
			    /* Allow ASCII characters. */
			    name = namebuf;
			    namebuf[0] = key;
			    namebuf[1] = NUL;
			}
			else
			    name = "X";
			break;
    }

    buf[0] = '\0';
    if (ctrl)
	strcat(buf, "C");
    if (shift)
	strcat(buf, "S");
    if (alt)
	strcat(buf, "M"); /* META */
    if (ctrl || shift || alt)
	strcat(buf, "-");
    strcat(buf, name);
}

#ifdef FEAT_BEVAL
/*
 * Function to be called for balloon evaluation.  Grabs the text under the
 * cursor and sends it to the debugger for evaluation.  The debugger should
 * respond with a showBalloon command when there is a useful result.
 */
/*ARGSUSED*/
    void
netbeans_beval_cb(
	BalloonEval	*beval,
	int		 state)
{
    win_T	*wp;
    char_u	*text;
    linenr_T	lnum;
    int		col;
    char	buf[MAXPATHL * 2 + 25];
    char_u	*p;

    /* Don't do anything when 'ballooneval' is off, messages scrolled the
     * windows up or we have no connection. */
    if (!p_beval || msg_scrolled > 0 || !haveConnection)
	return;

    if (get_beval_info(beval, TRUE, &wp, &lnum, &text, &col) == OK)
    {
	/* Send debugger request.  Only when the text is of reasonable
	 * length. */
	if (text != NULL && text[0] != NUL && STRLEN(text) < MAXPATHL)
	{
	    p = nb_quote(text);
	    if (p != NULL)
	    {
		vim_snprintf(buf, sizeof(buf),
				       "0:balloonText=%d \"%s\"\n", r_cmdno, p);
		vim_free(p);
	    }
	    nbdebug(("EVT: %s", buf));
	    nb_send(buf, "netbeans_beval_cb");
	}
	vim_free(text);
    }
}
#endif

/*
 * Tell netbeans that the window was opened, ready for commands.
 */
    void
netbeans_startup_done(void)
{
    char *cmd = "0:startupDone=0\n";

    if (usingNetbeans)
#ifdef FEAT_GUI_MOTIF
	netbeans_Xt_connect(app_context);
#else
# ifdef FEAT_GUI_GTK
	netbeans_gtk_connect();
# else
#  ifdef FEAT_GUI_W32
	netbeans_w32_connect();
#  endif
# endif
#endif

    if (!haveConnection)
	return;

#ifdef FEAT_BEVAL
    bevalServers |= BEVAL_NETBEANS;
#endif

    nbdebug(("EVT: %s", cmd));
    nb_send(cmd, "netbeans_startup_done");
}

/*
 * Tell netbeans that we're exiting. This should be called right
 * before calling exit.
 */
    void
netbeans_send_disconnect()
{
    char buf[128];

    if (haveConnection)
    {
	sprintf(buf, "0:disconnect=%d\n", r_cmdno);
	nbdebug(("EVT: %s", buf));
	nb_send(buf, "netbeans_disconnect");
    }
}

#if defined(FEAT_GUI_MOTIF) || defined(FEAT_GUI_W32) || defined(PROTO)
/*
 * Tell netbeans that the window was moved or resized.
 */
    void
netbeans_frame_moved(int new_x, int new_y)
{
    char buf[128];

    if (!haveConnection)
	return;

    sprintf(buf, "0:geometry=%d %d %d %d %d\n",
		    r_cmdno, (int)Columns, (int)Rows, new_x, new_y);
    /*nbdebug(("EVT: %s", buf)); happens too many times during a move */
    nb_send(buf, "netbeans_frame_moved");
}
#endif

/*
 * Tell netbeans the user opened or activated a file.
 */
    void
netbeans_file_activated(buf_T *bufp)
{
    int bufno = nb_getbufno(bufp);
    nbbuf_T *bp = nb_get_buf(bufno);
    char    buffer[2*MAXPATHL];
    char_u  *q;

    if (!haveConnection || dosetvisible)
	return;

    q = nb_quote(bufp->b_ffname);
    if (q == NULL || bp == NULL)
	return;

    vim_snprintf(buffer, sizeof(buffer),  "%d:fileOpened=%d \"%s\" %s %s\n",
	    bufno,
	    bufno,
	    (char *)q,
	    "T",  /* open in NetBeans */
	    "F"); /* modified */

    vim_free(q);
    nbdebug(("EVT: %s", buffer));

    nb_send(buffer, "netbeans_file_opened");
}

/*
 * Tell netbeans the user opened a file.
 */
    void
netbeans_file_opened(buf_T *bufp)
{
    int bufno = nb_getbufno(bufp);
    char    buffer[2*MAXPATHL];
    char_u  *q;
    nbbuf_T *bp = nb_get_buf(nb_getbufno(bufp));
    int	    bnum;

    if (!haveConnection)
	return;

    q = nb_quote(bufp->b_ffname);
    if (q == NULL)
	return;
    if (bp != NULL)
	bnum = bufno;
    else
	bnum = 0;

    vim_snprintf(buffer, sizeof(buffer), "%d:fileOpened=%d \"%s\" %s %s\n",
	    bnum,
	    0,
	    (char *)q,
	    "T",  /* open in NetBeans */
	    "F"); /* modified */

    vim_free(q);
    nbdebug(("EVT: %s", buffer));

    nb_send(buffer, "netbeans_file_opened");
    if (p_acd && vim_chdirfile(bufp->b_ffname) == OK)
	shorten_fnames(TRUE);
}

/*
 * Tell netbeans that a file was deleted or wiped out.
 */
    void
netbeans_file_killed(buf_T *bufp)
{
    int		bufno = nb_getbufno(bufp);
    nbbuf_T	*nbbuf = nb_get_buf(bufno);
    char	buffer[2*MAXPATHL];

    if (!haveConnection || bufno == -1)
	return;

    nbdebug(("netbeans_file_killed:\n"));
    nbdebug(("    Killing bufno: %d", bufno));

    sprintf(buffer, "%d:killed=%d\n", bufno, r_cmdno);

    nbdebug(("EVT: %s", buffer));

    nb_send(buffer, "netbeans_file_killed");

    if (nbbuf != NULL)
	nbbuf->bufp = NULL;
}

/*
 * Get a pointer to the Netbeans buffer for Vim buffer "bufp".
 * Return NULL if there is no such buffer or changes are not to be reported.
 * Otherwise store the buffer number in "*bufnop".
 */
    static nbbuf_T *
nb_bufp2nbbuf_fire(buf_T *bufp, int *bufnop)
{
    int		bufno;
    nbbuf_T	*nbbuf;

    if (!haveConnection || !netbeansFireChanges)
	return NULL;		/* changes are not reported at all */

    bufno = nb_getbufno(bufp);
    if (bufno <= 0)
	return NULL;		/* file is not known to NetBeans */

    nbbuf = nb_get_buf(bufno);
    if (nbbuf != NULL && !nbbuf->fireChanges)
	return NULL;		/* changes in this buffer are not reported */

    *bufnop = bufno;
    return nbbuf;
}

/*
 * Tell netbeans the user inserted some text.
 */
    void
netbeans_inserted(
    buf_T	*bufp,
    linenr_T	linenr,
    colnr_T	col,
    char_u	*txt,
    int		newlen)
{
    char_u	*buf;
    int		bufno;
    nbbuf_T	*nbbuf;
    pos_T	pos;
    long	off;
    char_u	*p;
    char_u	*newtxt;

    nbbuf = nb_bufp2nbbuf_fire(bufp, &bufno);
    if (nbbuf == NULL)
	return;

    /* Don't mark as modified for initial read */
    if (nbbuf->insertDone)
	nbbuf->modified = 1;

    pos.lnum = linenr;
    pos.col = col;
    off = pos2off(bufp, &pos);

    /* send the "insert" EVT */
    newtxt = alloc(newlen + 1);
    vim_strncpy(newtxt, txt, newlen);
    p = nb_quote(newtxt);
    if (p != NULL)
    {
	buf = alloc(128 + 2*newlen);
	sprintf((char *)buf, "%d:insert=%d %ld \"%s\"\n",
						      bufno, r_cmdno, off, p);
	nbdebug(("EVT: %s", buf));
	nb_send((char *)buf, "netbeans_inserted");
	vim_free(p);
	vim_free(buf);
    }
    vim_free(newtxt);
}

/*
 * Tell netbeans some bytes have been removed.
 */
    void
netbeans_removed(
    buf_T	*bufp,
    linenr_T	linenr,
    colnr_T	col,
    long	len)
{
    char_u	buf[128];
    int		bufno;
    nbbuf_T	*nbbuf;
    pos_T	pos;
    long	off;

    nbbuf = nb_bufp2nbbuf_fire(bufp, &bufno);
    if (nbbuf == NULL)
	return;

    if (len < 0)
    {
	nbdebug(("Negative len %ld in netbeans_removed()!\n", len));
	return;
    }

    nbbuf->modified = 1;

    pos.lnum = linenr;
    pos.col = col;

    off = pos2off(bufp, &pos);

    sprintf((char *)buf, "%d:remove=%d %ld %ld\n", bufno, r_cmdno, off, len);
    nbdebug(("EVT: %s", buf));
    nb_send((char *)buf, "netbeans_removed");
}

/*
 * Send netbeans an unmodufied command.
 */
/*ARGSUSED*/
    void
netbeans_unmodified(buf_T *bufp)
{
#if 0
    char_u	buf[128];
    int		bufno;
    nbbuf_T	*nbbuf;

    /* This has been disabled, because NetBeans considers a buffer modified
     * even when all changes have been undone. */
    nbbuf = nb_bufp2nbbuf_fire(bufp, &bufno);
    if (nbbuf == NULL)
	return;

    nbbuf->modified = 0;

    sprintf((char *)buf, "%d:unmodified=%d\n", bufno, r_cmdno);
    nbdebug(("EVT: %s", buf));
    nb_send((char *)buf, "netbeans_unmodified");
#endif
}

/*
 * Send a button release event back to netbeans. Its up to netbeans
 * to decide what to do (if anything) with this event.
 */
    void
netbeans_button_release(int button)
{
    char	buf[128];
    int		bufno;

    bufno = nb_getbufno(curbuf);

    if (bufno >= 0 && curwin != NULL && curwin->w_buffer == curbuf)
    {
	int col = mouse_col - W_WINCOL(curwin) - (curwin->w_p_nu ? 9 : 1);
	long off = pos2off(curbuf, &curwin->w_cursor);

	/* sync the cursor position */
	sprintf(buf, "%d:newDotAndMark=%d %ld %ld\n", bufno, r_cmdno, off, off);
	nbdebug(("EVT: %s", buf));
	nb_send(buf, "netbeans_button_release[newDotAndMark]");

	sprintf(buf, "%d:buttonRelease=%d %d %ld %d\n", bufno, r_cmdno,
				    button, (long)curwin->w_cursor.lnum, col);
	nbdebug(("EVT: %s", buf));
	nb_send(buf, "netbeans_button_release");
    }
}


/*
 * Send a keypress event back to netbeans. This usually simulates some
 * kind of function key press. This function operates on a key code.
 */
    void
netbeans_keycommand(int key)
{
    char	keyName[60];

    netbeans_keyname(key, keyName);
    netbeans_keystring(key, keyName);
}


/*
 * Send a keypress event back to netbeans. This usually simulates some
 * kind of function key press. This function operates on a key string.
 */
    static void
netbeans_keystring(int key, char *keyName)
{
    char	buf[2*MAXPATHL];
    int		bufno = nb_getbufno(curbuf);
    long	off;
    char_u	*q;

    if (!haveConnection)
	return;


    if (bufno == -1)
    {
	nbdebug(("got keycommand for non-NetBeans buffer, opening...\n"));
	q = curbuf->b_ffname == NULL ? (char_u *)""
						 : nb_quote(curbuf->b_ffname);
	if (q == NULL)
	    return;
	vim_snprintf(buf, sizeof(buf), "0:fileOpened=%d \"%s\" %s %s\n", 0,
		q,
		"T",  /* open in NetBeans */
		"F"); /* modified */
	if (curbuf->b_ffname != NULL)
	    vim_free(q);
	nbdebug(("EVT: %s", buf));
	nb_send(buf, "netbeans_keycommand");

	if (key > 0)
	    postpone_keycommand(key);
	return;
    }

    /* sync the cursor position */
    off = pos2off(curbuf, &curwin->w_cursor);
    sprintf(buf, "%d:newDotAndMark=%d %ld %ld\n", bufno, r_cmdno, off, off);
    nbdebug(("EVT: %s", buf));
    nb_send(buf, "netbeans_keycommand");

    /* To work on Win32 you must apply patch to ExtEditor module
     * from ExtEdCaret.java.diff - make EVT_newDotAndMark handler
     * more synchronous
     */

    /* now send keyCommand event */
    vim_snprintf(buf, sizeof(buf), "%d:keyCommand=%d \"%s\"\n",
						     bufno, r_cmdno, keyName);
    nbdebug(("EVT: %s", buf));
    nb_send(buf, "netbeans_keycommand");

    /* New: do both at once and include the lnum/col. */
    vim_snprintf(buf, sizeof(buf), "%d:keyAtPos=%d \"%s\" %ld %ld/%ld\n",
	    bufno, r_cmdno, keyName,
		off, (long)curwin->w_cursor.lnum, (long)curwin->w_cursor.col);
    nbdebug(("EVT: %s", buf));
    nb_send(buf, "netbeans_keycommand");
}


/*
 * Send a save event to netbeans.
 */
    void
netbeans_save_buffer(buf_T *bufp)
{
    char_u	buf[64];
    int		bufno;
    nbbuf_T	*nbbuf;

    nbbuf = nb_bufp2nbbuf_fire(bufp, &bufno);
    if (nbbuf == NULL)
	return;

    nbbuf->modified = 0;

    sprintf((char *)buf, "%d:save=%d\n", bufno, r_cmdno);
    nbdebug(("EVT: %s", buf));
    nb_send((char *)buf, "netbeans_save_buffer");
}


/*
 * Send remove command to netbeans (this command has been turned off).
 */
    void
netbeans_deleted_all_lines(buf_T *bufp)
{
    char_u	buf[64];
    int		bufno;
    nbbuf_T	*nbbuf;

    nbbuf = nb_bufp2nbbuf_fire(bufp, &bufno);
    if (nbbuf == NULL)
	return;

    /* Don't mark as modified for initial read */
    if (nbbuf->insertDone)
	nbbuf->modified = 1;

    sprintf((char *)buf, "%d:remove=%d 0 -1\n", bufno, r_cmdno);
    nbdebug(("EVT(suppressed): %s", buf));
/*     nb_send(buf, "netbeans_deleted_all_lines"); */
}


/*
 * See if the lines are guarded. The top and bot parameters are from
 * u_savecommon(), these are the line above the change and the line below the
 * change.
 */
    int
netbeans_is_guarded(linenr_T top, linenr_T bot)
{
    signlist_T	*p;
    int		lnum;

    for (p = curbuf->b_signlist; p != NULL; p = p->next)
	if (p->id >= GUARDEDOFFSET)
	    for (lnum = top + 1; lnum < bot; lnum++)
		if (lnum == p->lnum)
		    return TRUE;

    return FALSE;
}

#if defined(FEAT_GUI_MOTIF) || defined(PROTO)
/*
 * We have multiple signs to draw at the same location. Draw the
 * multi-sign indicator instead. This is the Motif version.
 */
    void
netbeans_draw_multisign_indicator(int row)
{
    int i;
    int y;
    int x;

    x = 0;
    y = row * gui.char_height + 2;

    for (i = 0; i < gui.char_height - 3; i++)
	XDrawPoint(gui.dpy, gui.wid, gui.text_gc, x+2, y++);

    XDrawPoint(gui.dpy, gui.wid, gui.text_gc, x+0, y);
    XDrawPoint(gui.dpy, gui.wid, gui.text_gc, x+2, y);
    XDrawPoint(gui.dpy, gui.wid, gui.text_gc, x+4, y++);
    XDrawPoint(gui.dpy, gui.wid, gui.text_gc, x+1, y);
    XDrawPoint(gui.dpy, gui.wid, gui.text_gc, x+2, y);
    XDrawPoint(gui.dpy, gui.wid, gui.text_gc, x+3, y++);
    XDrawPoint(gui.dpy, gui.wid, gui.text_gc, x+2, y);
}
#endif /* FEAT_GUI_MOTIF */

#ifdef FEAT_GUI_GTK
/*
 * We have multiple signs to draw at the same location. Draw the
 * multi-sign indicator instead. This is the GTK/Gnome version.
 */
    void
netbeans_draw_multisign_indicator(int row)
{
    int i;
    int y;
    int x;
    GdkDrawable *drawable = gui.drawarea->window;

    x = 0;
    y = row * gui.char_height + 2;

    for (i = 0; i < gui.char_height - 3; i++)
	gdk_draw_point(drawable, gui.text_gc, x+2, y++);

    gdk_draw_point(drawable, gui.text_gc, x+0, y);
    gdk_draw_point(drawable, gui.text_gc, x+2, y);
    gdk_draw_point(drawable, gui.text_gc, x+4, y++);
    gdk_draw_point(drawable, gui.text_gc, x+1, y);
    gdk_draw_point(drawable, gui.text_gc, x+2, y);
    gdk_draw_point(drawable, gui.text_gc, x+3, y++);
    gdk_draw_point(drawable, gui.text_gc, x+2, y);
}
#endif /* FEAT_GUI_GTK */

/*
 * If the mouse is clicked in the gutter of a line with multiple
 * annotations, cycle through the set of signs.
 */
    void
netbeans_gutter_click(linenr_T lnum)
{
    signlist_T	*p;

    for (p = curbuf->b_signlist; p != NULL; p = p->next)
    {
	if (p->lnum == lnum && p->next && p->next->lnum == lnum)
	{
	    signlist_T *tail;

	    /* remove "p" from list, reinsert it at the tail of the sublist */
	    if (p->prev)
		p->prev->next = p->next;
	    else
		curbuf->b_signlist = p->next;
	    p->next->prev = p->prev;
	    /* now find end of sublist and insert p */
	    for (tail = p->next;
		  tail->next && tail->next->lnum == lnum
					    && tail->next->id < GUARDEDOFFSET;
		  tail = tail->next)
		;
	    /* tail now points to last entry with same lnum (except
	     * that "guarded" annotations are always last) */
	    p->next = tail->next;
	    if (tail->next)
		tail->next->prev = p;
	    p->prev = tail;
	    tail->next = p;
	    update_debug_sign(curbuf, lnum);
	    break;
	}
    }
}


/*
 * Add a sign of the reqested type at the requested location.
 *
 * Reverse engineering:
 * Apparently an annotation is defined the first time it is used in a buffer.
 * When the same annotation is used in two buffers, the second time we do not
 * need to define a new sign name but reuse the existing one.  But since the
 * ID number used in the second buffer starts counting at one again, a mapping
 * is made from the ID specifically for the buffer to the global sign name
 * (which is a number).
 *
 * globalsignmap[]	stores the signs that have been defined globally.
 * buf->signmapused[]	maps buffer-local annotation IDs to an index in
 *			globalsignmap[].
 */
/*ARGSUSED*/
    static void
addsigntype(
    nbbuf_T	*buf,
    int		typeNum,
    char_u	*typeName,
    char_u	*tooltip,
    char_u	*glyphFile,
    int		use_fg,
    int		fg,
    int		use_bg,
    int		bg)
{
    char fgbuf[32];
    char bgbuf[32];
    int i, j;

    for (i = 0; i < globalsignmapused; i++)
	if (STRCMP(typeName, globalsignmap[i]) == 0)
	    break;

    if (i == globalsignmapused) /* not found; add it to global map */
    {
	nbdebug(("DEFINEANNOTYPE(%d,%s,%s,%s,%d,%d)\n",
			    typeNum, typeName, tooltip, glyphFile, fg, bg));
	if (use_fg || use_bg)
	{
	    sprintf(fgbuf, "guifg=#%06x", fg & 0xFFFFFF);
	    sprintf(bgbuf, "guibg=#%06x", bg & 0xFFFFFF);

	    coloncmd(":highlight NB_%s %s %s", typeName, (use_fg) ? fgbuf : "",
		     (use_bg) ? bgbuf : "");
	    if (*glyphFile == NUL)
		/* no glyph, line highlighting only */
		coloncmd(":sign define %d linehl=NB_%s", i + 1, typeName);
	    else if (vim_strsize(glyphFile) <= 2)
		/* one- or two-character glyph name, use as text glyph with
		 * texthl */
		coloncmd(":sign define %d text=%s texthl=NB_%s", i + 1,
							 glyphFile, typeName);
	    else
		/* glyph, line highlighting */
		coloncmd(":sign define %d icon=%s linehl=NB_%s", i + 1,
							 glyphFile, typeName);
	}
	else
	    /* glyph, no line highlighting */
	    coloncmd(":sign define %d icon=%s", i + 1, glyphFile);

	if (STRCMP(typeName,"CurrentPC") == 0)
	    curPCtype = typeNum;

	if (globalsignmapused == globalsignmaplen)
	{
	    if (globalsignmaplen == 0) /* first allocation */
	    {
		globalsignmaplen = 20;
		globalsignmap = (char **)alloc_clear(globalsignmaplen*sizeof(char *));
	    }
	    else    /* grow it */
	    {
		int incr;
		int oldlen = globalsignmaplen;

		globalsignmaplen *= 2;
		incr = globalsignmaplen - oldlen;
		globalsignmap = (char **)vim_realloc(globalsignmap,
					   globalsignmaplen * sizeof(char *));
		memset(globalsignmap + oldlen, 0, incr * sizeof(char *));
	    }
	}

	globalsignmap[i] = (char *)typeName;
	globalsignmapused = i + 1;
    }

    /* check local map; should *not* be found! */
    for (j = 0; j < buf->signmapused; j++)
	if (buf->signmap[j] == i + 1)
	    return;

    /* add to local map */
    if (buf->signmapused == buf->signmaplen)
    {
	if (buf->signmaplen == 0) /* first allocation */
	{
	    buf->signmaplen = 5;
	    buf->signmap = (int *)alloc_clear(buf->signmaplen * sizeof(int *));
	}
	else    /* grow it */
	{
	    int incr;
	    int oldlen = buf->signmaplen;
	    buf->signmaplen *= 2;
	    incr = buf->signmaplen - oldlen;
	    buf->signmap = (int *)vim_realloc(buf->signmap,
					       buf->signmaplen*sizeof(int *));
	    memset(buf->signmap + oldlen, 0, incr * sizeof(int *));
	}
    }

    buf->signmap[buf->signmapused++] = i + 1;

}


/*
 * See if we have the requested sign type in the buffer.
 */
    static int
mapsigntype(nbbuf_T *buf, int localsigntype)
{
    if (--localsigntype >= 0 && localsigntype < buf->signmapused)
	return buf->signmap[localsigntype];

    return 0;
}


/*
 * Compute length of buffer, don't print anything.
 */
    static long
get_buf_size(buf_T *bufp)
{
    linenr_T	lnum;
    long	char_count = 0;
    int		eol_size;
    long	last_check = 100000L;

    if (bufp->b_ml.ml_flags & ML_EMPTY)
	return 0;
    else
    {
	if (get_fileformat(bufp) == EOL_DOS)
	    eol_size = 2;
	else
	    eol_size = 1;
	for (lnum = 1; lnum <= bufp->b_ml.ml_line_count; ++lnum)
	{
	    char_count += (long)STRLEN(ml_get(lnum)) + eol_size;
	    /* Check for a CTRL-C every 100000 characters */
	    if (char_count > last_check)
	    {
		ui_breakcheck();
		if (got_int)
		    return char_count;
		last_check = char_count + 100000L;
	    }
	}
	/* Correction for when last line doesn't have an EOL. */
	if (!bufp->b_p_eol && bufp->b_p_bin)
	    char_count -= eol_size;
    }

    return char_count;
}

/*
 * Convert character offset to lnum,col
 */
    static pos_T *
off2pos(buf_T *buf, long offset)
{
    linenr_T	 lnum;
    static pos_T pos;

    pos.lnum = 0;
    pos.col = 0;
#ifdef FEAT_VIRTUALEDIT
    pos.coladd = 0;
#endif

    if (!(buf->b_ml.ml_flags & ML_EMPTY))
    {
	if ((lnum = ml_find_line_or_offset(buf, (linenr_T)0, &offset)) < 0)
	    return NULL;
	pos.lnum = lnum;
	pos.col = offset;
    }

    return &pos;
}

/*
 * Convert an argument in the form "1234" to an offset and compute the
 * lnum/col from it.  Convert an argument in the form "123/12" directly to a
 * lnum/col.
 * "argp" is advanced to after the argument.
 * Return a pointer to the position, NULL if something is wrong.
 */
    static pos_T *
get_off_or_lnum(buf_T *buf, char_u **argp)
{
    static pos_T	mypos;
    long		off;

    off = strtol((char *)*argp, (char **)argp, 10);
    if (**argp == '/')
    {
	mypos.lnum = (linenr_T)off;
	++*argp;
	mypos.col = strtol((char *)*argp, (char **)argp, 10);
#ifdef FEAT_VIRTUALEDIT
	mypos.coladd = 0;
#endif
	return &mypos;
    }
    return off2pos(buf, off);
}


/*
 * Convert (lnum,col) to byte offset in the file.
 */
    static long
pos2off(buf_T *buf, pos_T *pos)
{
    long	 offset = 0;

    if (!(buf->b_ml.ml_flags & ML_EMPTY))
    {
	if ((offset = ml_find_line_or_offset(buf, pos->lnum, 0)) < 0)
	    return 0;
	offset += pos->col;
    }

    return offset;
}


/*
 * This message is printed after NetBeans opens a new file. Its
 * similar to the message readfile() uses, but since NetBeans
 * doesn't normally call readfile, we do our own.
 */
    static void
print_read_msg(buf)
    nbbuf_T	*buf;
{
    int	    lnum = buf->bufp->b_ml.ml_line_count;
    long    nchars = (long)buf->bufp->b_orig_size;
    char_u  c;

    msg_add_fname(buf->bufp, buf->bufp->b_ffname);
    c = FALSE;

    if (buf->bufp->b_p_ro)
    {
	STRCAT(IObuff, shortmess(SHM_RO) ? _("[RO]") : _("[readonly]"));
	c = TRUE;
    }
    if (!buf->bufp->b_start_eol)
    {
	STRCAT(IObuff, shortmess(SHM_LAST) ? _("[noeol]") : _("[Incomplete last line]"));
	c = TRUE;
    }
    msg_add_lines(c, (long)lnum, nchars);

    /* Now display it */
    vim_free(keep_msg);
    keep_msg = NULL;
    msg_scrolled_ign = TRUE;
    msg_trunc_attr(IObuff, FALSE, 0);
    msg_scrolled_ign = FALSE;
}


/*
 * Print a message after NetBeans writes the file. This message should be identical
 * to the standard message a non-netbeans user would see when writing a file.
 */
    static void
print_save_msg(buf, nchars)
    nbbuf_T	*buf;
    long	nchars;
{
    char_u	c;
    char_u	*p;

    if (nchars >= 0)
    {
	msg_add_fname(buf->bufp, buf->bufp->b_ffname);   /* fname in IObuff with quotes */
	c = FALSE;

	msg_add_lines(c, buf->bufp->b_ml.ml_line_count,
						(long)buf->bufp->b_orig_size);

	vim_free(keep_msg);
	keep_msg = NULL;
	msg_scrolled_ign = TRUE;
	p = msg_trunc_attr(IObuff, FALSE, 0);
	if ((msg_scrolled && !need_wait_return) || !buf->initDone)
	{
	    /* Need to repeat the message after redrawing when:
	     * - When reading from stdin (the screen will be cleared next).
	     * - When restart_edit is set (otherwise there will be a delay
	     *   before redrawing).
	     * - When the screen was scrolled but there is no wait-return
	     *   prompt. */
	    set_keep_msg(p, 0);
	}
	msg_scrolled_ign = FALSE;
	/* add_to_input_buf((char_u *)"\f", 1); */
    }
    else
    {
	char_u ebuf[BUFSIZ];

	STRCPY(ebuf, (char_u *)_("E505: "));
	STRCAT(ebuf, IObuff);
	STRCAT(ebuf, (char_u *)_("is read-only (add ! to override)"));
	STRCPY(IObuff, ebuf);
	nbdebug(("    %s\n", ebuf ));
	emsg(IObuff);
    }
}

#endif /* defined(FEAT_NETBEANS_INTG) */
