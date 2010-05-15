/* vi:set ts=8 sts=4 sw=4:
 *
 * if_sniff.c Interface between Vim and SNiFF+
 *
 * See README.txt for an overview of the Vim source code.
 */

#include "vim.h"

#ifdef WIN32
# include <stdio.h>
# include "vimio.h"
# include <process.h>
# include <string.h>
# include <assert.h>
#else
# ifdef FEAT_GUI_X11
#  include "gui_x11.pro"
# endif
# include "os_unixx.h"
#endif

static int sniffemacs_pid;

int fd_from_sniff;
int sniff_connected = 0;
int sniff_request_waiting = 0;
int want_sniff_request = 0;

#define MAX_REQUEST_LEN 512

#define NEED_SYMBOL	2
#define EMPTY_SYMBOL	4
#define NEED_FILE	8
#define SILENT		16
#define DISCONNECT	32
#define CONNECT		64

#define RQ_NONE		0
#define RQ_SIMPLE	1
#define RQ_CONTEXT	NEED_FILE + NEED_SYMBOL
#define RQ_SCONTEXT	NEED_FILE + NEED_SYMBOL + EMPTY_SYMBOL
#define RQ_NOSYMBOL	NEED_FILE
#define RQ_SILENT	RQ_NOSYMBOL + SILENT
#define RQ_CONNECT	RQ_NONE + CONNECT
#define RQ_DISCONNECT	RQ_SIMPLE + DISCONNECT

struct sn_cmd
{
    char *cmd_name;
    char cmd_code;
    char *cmd_msg;
    int  cmd_type;
};

struct sn_cmd_list
{
    struct sn_cmd* sniff_cmd;
    struct sn_cmd_list* next_cmd;
};

static struct sn_cmd sniff_cmds[] =
{
    { "toggle",		'e', N_("Toggle implementation/definition"),RQ_SCONTEXT },
    { "superclass",	's', N_("Show base class of"),		RQ_CONTEXT },
    { "overridden",	'm', N_("Show overridden member function"),RQ_SCONTEXT },
    { "retrieve-file",	'r', N_("Retrieve from file"),		RQ_CONTEXT },
    { "retrieve-project",'p', N_("Retrieve from project"),	RQ_CONTEXT },
    { "retrieve-all-projects",
			'P', N_("Retrieve from all projects"),	RQ_CONTEXT },
    { "retrieve-next",	'R', N_("Retrieve"),	RQ_CONTEXT },
    { "goto-symbol",	'g', N_("Show source of"),		RQ_CONTEXT },
    { "find-symbol",	'f', N_("Find symbol"),			RQ_CONTEXT },
    { "browse-class",	'w', N_("Browse class"),		RQ_CONTEXT },
    { "hierarchy",	't', N_("Show class in hierarchy"),	RQ_CONTEXT },
    { "restr-hier",	'T', N_("Show class in restricted hierarchy"),RQ_CONTEXT },
    { "xref-to",	'x', N_("Xref refers to"),		RQ_CONTEXT },
    { "xref-by",	'X', N_("Xref referred by"),		RQ_CONTEXT },
    { "xref-has",	'c', N_("Xref has a"),			RQ_CONTEXT },
    { "xref-used-by",	'C', N_("Xref used by"),		RQ_CONTEXT },
    { "show-docu",	'd', N_("Show docu of"),		RQ_CONTEXT },
    { "gen-docu",	'D', N_("Generate docu for"),		RQ_CONTEXT },
    { "connect",	'y', NULL,				RQ_CONNECT },
    { "disconnect",	'q', NULL,				RQ_DISCONNECT },
    { "font-info",	'z', NULL,				RQ_SILENT },
    { "update",		'u', NULL,				RQ_SILENT },
    { NULL,		'\0', NULL, 0}
};


static char *SniffEmacs[2] = {"sniffemacs", (char *)NULL};  /* Yes, Emacs! */
static int fd_to_sniff;
static int sniff_will_disconnect = 0;
static char msg_sniff_disconnect[] = N_("Cannot connect to SNiFF+. Check environment (sniffemacs must be found in $PATH).\n");
static char sniff_rq_sep[] = " ";
static struct sn_cmd_list *sniff_cmd_ext = NULL;

/* Initializing vim commands
 * executed each time vim connects to Sniff
 */
static char *init_cmds[]= {
    "augroup sniff",
    "autocmd BufWritePost * sniff update",
    "autocmd BufReadPost * sniff font-info",
    "autocmd VimLeave * sniff disconnect",
    "augroup END",

    "let g:sniff_connected = 1",

    "if ! exists('g:sniff_mappings_sourced')|"
	"if ! exists('g:sniff_mappings')|"
	    "if exists('$SNIFF_DIR4')|"
		"let g:sniff_mappings='$SNIFF_DIR4/config/integrations/vim/sniff.vim'|"
	    "else|"
		"let g:sniff_mappings='$SNIFF_DIR/config/sniff.vim'|"
	    "endif|"
	"endif|"
	"let g:sniff_mappings=expand(g:sniff_mappings)|"
	"if filereadable(g:sniff_mappings)|"
	    "execute 'source' g:sniff_mappings|"
	    "let g:sniff_mappings_sourced=1|"
	"endif|"
    "endif",

    NULL
};

/*-------- Function Prototypes ----------------------------------*/

static int ConnectToSniffEmacs __ARGS((void));
static void sniff_connect __ARGS((void));
static void HandleSniffRequest __ARGS((char* buffer));
static int get_request __ARGS((int fd, char *buf, int maxlen));
static void WriteToSniff __ARGS((char *str));
static void SendRequest __ARGS((struct sn_cmd *command, char* symbol));
static void vi_msg __ARGS((char *));
static void vi_error_msg __ARGS((char *));
static char *vi_symbol_under_cursor __ARGS((void));
static void vi_open_file __ARGS((char *));
static char *vi_buffer_name __ARGS((void));
static buf_T *vi_find_buffer __ARGS((char *));
static void vi_exec_cmd __ARGS((char *));
static void vi_set_cursor_pos __ARGS((long char_nr));
static long vi_cursor_pos __ARGS((void));

/* debug trace */
#if 0
static FILE* _tracefile = NULL;
#define SNIFF_TRACE_OPEN(file) if (!_tracefile) _tracefile = fopen(file, "w")
#define SNIFF_TRACE(msg) fprintf(_tracefile, msg); fflush(_tracefile);
#define SNIFF_TRACE1(msg, arg) fprintf(_tracefile, msg,arg); fflush(_tracefile);
#define SNIFF_TRACE_CLOSE fclose(_tracefile); _tracefile=NULL;
#else
#define SNIFF_TRACE_OPEN(file)
#define SNIFF_TRACE(msg)
#define SNIFF_TRACE1(msg, arg)
#define SNIFF_TRACE_CLOSE
#endif

/*-------- Windows Only Declarations -----------------------------*/
#ifdef WIN32

static int  sniff_request_processed=1;
static HANDLE sniffemacs_handle=NULL;
static HANDLE readthread_handle=NULL;
static HANDLE handle_to_sniff=NULL;
static HANDLE handle_from_sniff=NULL;

struct sniffBufNode
{
    struct sniffBufNode *next;
    int    bufLen;
    char   buf[MAX_REQUEST_LEN];
};
static struct sniffBufNode *sniffBufStart=NULL;
static struct sniffBufNode *sniffBufEnd=NULL;
static HANDLE hBufferMutex=NULL;

# ifdef FEAT_GUI_W32
    extern HWND s_hwnd;       /* gvim's Window handle */
# endif
/*
 * some helper functions for Windows port only
 */

    static HANDLE
ExecuteDetachedProgram(char *szBinary, char *szCmdLine,
    HANDLE hStdInput, HANDLE hStdOutput)
{
    BOOL bResult;
    DWORD nError;
    PROCESS_INFORMATION aProcessInformation;
    PROCESS_INFORMATION *pProcessInformation= &aProcessInformation;
    STARTUPINFO aStartupInfo;
    STARTUPINFO *pStartupInfo= &aStartupInfo;
    DWORD dwCreationFlags= 0;
    char szPath[512];
    HINSTANCE hResult;

    hResult = FindExecutable(szBinary, ".", szPath);
    if ((int)hResult <= 32)
    {
	/* can't find the exe file */
	return NULL;
    }

    ZeroMemory(pStartupInfo, sizeof(*pStartupInfo));
    pStartupInfo->dwFlags= STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;
    pStartupInfo->hStdInput = hStdInput;
    pStartupInfo->hStdOutput = hStdOutput;
    pStartupInfo->wShowWindow= SW_HIDE;
    pStartupInfo->cb = sizeof(STARTUPINFO);

    bResult= CreateProcess(
	szPath,
	szCmdLine,
	NULL,    /* security attr for process */
	NULL,    /* security attr for primary thread */
	TRUE,    /* DO inherit stdin and stdout */
	dwCreationFlags, /* creation flags */
	NULL,    /* environment */
	".",    /* current directory */
	pStartupInfo,   /* startup info: NULL crashes  */
	pProcessInformation /* process information: NULL crashes */
    );
    nError= GetLastError();
    if (bResult)
    {
	CloseHandle(pProcessInformation->hThread);
	CloseHandle(hStdInput);
	CloseHandle(hStdOutput);
	return(pProcessInformation->hProcess);
    }
    else
	return(NULL);
}

/*
 * write to the internal Thread / Thread communications buffer.
 * Return TRUE if successful, FALSE else.
 */
    static BOOL
writeToBuffer(char *msg, int len)
{
    DWORD dwWaitResult;     /* Request ownership of mutex. */
    struct sniffBufNode *bn;
    int bnSize;

    SNIFF_TRACE1("writeToBuffer %d\n", len);
    bnSize = sizeof(struct sniffBufNode) - MAX_REQUEST_LEN + len + 1;
    if (bnSize < 128) bnSize = 128; /* minimum length to avoid fragmentation */
    bn = (struct sniffBufNode *)malloc(bnSize);
    if (!bn)
	return FALSE;

    memcpy(bn->buf, msg, len);
    bn->buf[len]='\0';    /* terminate CString for added safety */
    bn->next = NULL;
    bn->bufLen = len;
    /* now, acquire a Mutex for adding the string to our linked list */
    dwWaitResult = WaitForSingleObject(
	hBufferMutex,   /* handle of mutex */
	1000L);   /* one-second time-out interval */
    if (dwWaitResult == WAIT_OBJECT_0)
    {
	/* The thread got mutex ownership. */
	if (sniffBufEnd)
	{
	    sniffBufEnd->next = bn;
	    sniffBufEnd = bn;
	}
	else
	    sniffBufStart = sniffBufEnd = bn;
	/* Release ownership of the mutex object. */
	if (! ReleaseMutex(hBufferMutex))
	{
	    /* Deal with error. */
	}
	return TRUE;
    }

    /* Cannot get mutex ownership due to time-out or mutex object abandoned. */
    free(bn);
    return FALSE;
}

/*
 * read from the internal Thread / Thread communications buffer.
 * Return TRUE if successful, FALSE else.
 */
    static int
ReadFromBuffer(char *buf, int maxlen)
{
    DWORD dwWaitResult;     /* Request ownership of mutex. */
    int   theLen;
    struct sniffBufNode *bn;

    dwWaitResult = WaitForSingleObject(
	hBufferMutex,   /* handle of mutex */
	1000L);		/* one-second time-out interval */
    if (dwWaitResult == WAIT_OBJECT_0)
    {
	if (!sniffBufStart)
	{
	    /* all pending Requests Processed */
	    theLen = 0;
	}
	else
	{
	    bn = sniffBufStart;
	    theLen = bn->bufLen;
	    SNIFF_TRACE1("ReadFromBuffer %d\n", theLen);
	    if (theLen >= maxlen)
	    {
		/* notify the user of buffer overflow? */
		theLen = maxlen-1;
	    }
	    memcpy(buf, bn->buf, theLen);
	    buf[theLen] = '\0';
	    if (! (sniffBufStart = bn->next))
	    {
		sniffBufEnd = NULL;
		sniff_request_processed = 1;
	    }
	    free(bn);
	}
	if (! ReleaseMutex(hBufferMutex))
	{
	    /* Deal with error. */
	}
	return theLen;
    }

    /* Cannot get mutex ownership due to time-out or mutex object abandoned. */
    return -1;
}

/* on Win32, a separate Thread reads the input pipe. get_request is not needed here. */
    static void __cdecl
SniffEmacsReadThread(void *dummy)
{
    static char	ReadThreadBuffer[MAX_REQUEST_LEN];
    int		ReadThreadLen=0;
    int		result=0;
    int		msgLen=0;
    char	*msgStart, *msgCur;

    SNIFF_TRACE("begin thread\n");
    /* Read from the pipe to SniffEmacs */
    while (sniff_connected)
    {
	if (!ReadFile(handle_from_sniff,
		ReadThreadBuffer + ReadThreadLen,    /* acknowledge rest in buffer */
		MAX_REQUEST_LEN - ReadThreadLen,
		&result,
		NULL))
	{
	    DWORD err = GetLastError();
	    result = -1;
	}

	if (result < 0)
	{
	    /* probably sniffemacs died... log the Error? */
	    sniff_disconnect(1);
	}
	else if (result > 0)
	{
	    ReadThreadLen += result-1;   /* total length of valid chars */
	    for(msgCur=msgStart=ReadThreadBuffer; ReadThreadLen > 0; msgCur++, ReadThreadLen--)
	    {
		if (*msgCur == '\0' || *msgCur == '\r' || *msgCur == '\n')
		{
		    msgLen = msgCur-msgStart; /* don't add the CR/LF chars */
		    if (msgLen > 0)
			writeToBuffer(msgStart, msgLen);
		    msgStart = msgCur + 1; /* over-read single CR/LF chars */
		}
	    }

	/* move incomplete message to beginning of buffer */
	ReadThreadLen = msgCur - msgStart;
	if (ReadThreadLen > 0)
	    mch_memmove(ReadThreadBuffer, msgStart, ReadThreadLen);

	if (sniff_request_processed)
	{
	    /* notify others that new data has arrived */
	    sniff_request_processed = 0;
	    sniff_request_waiting = 1;
#ifdef FEAT_GUI_W32
	    PostMessage(s_hwnd, WM_USER, (WPARAM)0, (LPARAM)0);
#endif
	    }
	}
    }
    SNIFF_TRACE("end thread\n");
}
#endif /* WIN32 */
/*-------- End of Windows Only Declarations ------------------------*/


/* ProcessSniffRequests
 * Function that should be called from outside
 * to process the waiting sniff requests
 */
    void
ProcessSniffRequests()
{
    static char buf[MAX_REQUEST_LEN];
    int len;

    while (sniff_connected)
    {
#ifdef WIN32
	len = ReadFromBuffer(buf, sizeof(buf));
#else
	len = get_request(fd_from_sniff, buf, sizeof(buf));
#endif
	if (len < 0)
	{
	    vi_error_msg(_("E274: Sniff: Error during read. Disconnected"));
	    sniff_disconnect(1);
	    break;
	}
	else if (len > 0)
	    HandleSniffRequest( buf );
	else
	    break;
    }

    if (sniff_will_disconnect)	/* Now the last msg has been processed */
	sniff_disconnect(1);
}

    static struct sn_cmd *
find_sniff_cmd(cmd)
    char *cmd;
{
    struct sn_cmd *sniff_cmd = NULL;
    int i;
    for(i=0; sniff_cmds[i].cmd_name; i++)
    {
	if (!strcmp(cmd, sniff_cmds[i].cmd_name))
	{
	    sniff_cmd = &sniff_cmds[i];
	    break;
	}
    }
    if (!sniff_cmd)
    {
	struct sn_cmd_list *list = sniff_cmd_ext;
	while(list)
	{
	    if (!strcmp(cmd, list->sniff_cmd->cmd_name))
	    {
		sniff_cmd = list->sniff_cmd;
		break;
	    }
	    list = list->next_cmd;
	}
    }
    return sniff_cmd;
}

    static int
add_sniff_cmd(cmd, def, msg)
    char *cmd;
    char *def;
    char *msg;
{
    int rc = 0;
    if (def != NULL && def[0] != NUL && find_sniff_cmd(cmd) == NULL)
    {
	struct sn_cmd_list *list = sniff_cmd_ext;
	struct sn_cmd *sniff_cmd = (struct sn_cmd*)malloc(sizeof(struct sn_cmd));
	struct sn_cmd_list *cmd_node = (struct sn_cmd_list*)malloc(sizeof(struct sn_cmd_list));
	int rq_type = 0;

	/* unescape message text */
	char *p = msg;
	char *end = p+strlen(msg);
	while(*p)
	{
	    if (*p == '\\')
		mch_memmove(p,p+1,end-p);
	    p++;
	}
	SNIFF_TRACE1("request name = %s\n",cmd);
	SNIFF_TRACE1("request def = %s\n",def);
	SNIFF_TRACE1("request msg = %s\n",msg);

	while(list && list->next_cmd)
	    list = list->next_cmd;
	if (!list)
	    sniff_cmd_ext = cmd_node;
	else
	    list->next_cmd = cmd_node;

	sniff_cmd->cmd_name = cmd;
	sniff_cmd->cmd_code = def[0];
	sniff_cmd->cmd_msg = msg;
	switch(def[1])
	{
	    case 'f':
		rq_type = RQ_NOSYMBOL;
		break;
	    case 's':
		rq_type = RQ_CONTEXT;
		break;
	    case 'S':
		rq_type = RQ_SCONTEXT;
		break;
	    default:
		rq_type = RQ_SIMPLE;
		break;
	}
	sniff_cmd->cmd_type = rq_type;
	cmd_node->sniff_cmd = sniff_cmd;
	cmd_node->next_cmd = NULL;
	rc = 1;
    }
    return rc;
}

/* ex_sniff
 * Handle ":sniff" command
 */
    void
ex_sniff(eap)
    exarg_T	*eap;
{
    char_u	*arg = eap->arg;
    char_u *symbol = NULL;
    char_u *cmd = NULL;

    SNIFF_TRACE_OPEN("if_sniff.log");
    if (ends_excmd(*arg))	/* no request: print available commands */
    {
	int i;
	msg_start();
	msg_outtrans_attr((char_u *)"-- SNiFF+ commands --", hl_attr(HLF_T));
	for(i=0; sniff_cmds[i].cmd_name; i++)
	{
	    msg_putchar('\n');
	    msg_outtrans((char_u *)":sniff ");
	    msg_outtrans((char_u *)sniff_cmds[i].cmd_name);
	}
	msg_putchar('\n');
	msg_outtrans((char_u *)_("SNiFF+ is currently "));
	if (!sniff_connected)
	    msg_outtrans((char_u *)_("not "));
	msg_outtrans((char_u *)_("connected"));
	msg_end();
    }
    else	/* extract command name and symbol if present */
    {
	symbol = skiptowhite(arg);
	cmd  = vim_strnsave(arg, (int)(symbol-arg));
	symbol = skipwhite(symbol);
	if (ends_excmd(*symbol))
	    symbol = NULL;
	if (!strcmp((char *)cmd, "addcmd"))
	{
	    char_u *def = skiptowhite(symbol);
	    char_u *name = vim_strnsave(symbol, (int)(def-symbol));
	    char_u *msg;
	    def = skipwhite(def);
	    msg = skiptowhite(def);
	    def = vim_strnsave(def, (int)(msg-def));
	    msg = skipwhite(msg);
	    if (ends_excmd(*msg))
		msg = vim_strsave(name);
	    else
		msg = vim_strnsave(msg, (int)(skiptowhite_esc(msg)-msg));
	    if (!add_sniff_cmd((char*)name, (char*)def, (char*)msg))
	    {
		vim_free(msg);
		vim_free(def);
		vim_free(name);
	    }
	}
	else
	{
	    struct sn_cmd* sniff_cmd = find_sniff_cmd((char*)cmd);
	    if (sniff_cmd)
		SendRequest(sniff_cmd, (char *)symbol);
	    else
		EMSG2(_("E275: Unknown SNiFF+ request: %s"), cmd);
	}
	vim_free(cmd);
    }
}


    static void
sniff_connect()
{
    if (sniff_connected)
	return;
    if (ConnectToSniffEmacs())
	vi_error_msg(_("E276: Error connecting to SNiFF+"));
    else
    {
	int i;

	for (i = 0; init_cmds[i]; i++)
	    vi_exec_cmd(init_cmds[i]);
    }
}

    void
sniff_disconnect(immediately)
    int immediately;
{
    if (!sniff_connected)
	return;
    if (immediately)
    {
	vi_exec_cmd("augroup sniff");
	vi_exec_cmd("au!");
	vi_exec_cmd("augroup END");
	vi_exec_cmd("unlet g:sniff_connected");
	sniff_connected = 0;
	want_sniff_request = 0;
	sniff_will_disconnect = 0;
#ifdef FEAT_GUI
	if (gui.in_use)
	    gui_mch_wait_for_chars(0L);
#endif
#ifdef WIN32
	while(sniffBufStart != NULL)
	{
	    struct sniffBufNode *node = sniffBufStart;
	    sniffBufStart = sniffBufStart->next;
	    free(node);
	}
	sniffBufStart = sniffBufEnd = NULL;
	sniff_request_processed = 1;
	CloseHandle(handle_to_sniff);
	CloseHandle(handle_from_sniff);
	WaitForSingleObject(sniffemacs_handle, 1000L);
	CloseHandle(sniffemacs_handle);
	sniffemacs_handle = NULL;
	WaitForSingleObject(readthread_handle, 1000L);
	readthread_handle = NULL;
	CloseHandle(hBufferMutex);
	hBufferMutex = NULL;
	SNIFF_TRACE_CLOSE;
#else
	close(fd_to_sniff);
	close(fd_from_sniff);
	wait(NULL);
#endif
    }
    else
    {
#ifdef WIN32
	_sleep(2);
	if (!sniff_request_processed)
	    ProcessSniffRequests();
#else
	sleep(2);		    /* Incoming msg could disturb edit */
#endif
	sniff_will_disconnect = 1;  /* We expect disconnect msg in 2 secs */
    }
}


/* ConnectToSniffEmacs
 * Connect to Sniff: returns 1 on error
 */
    static int
ConnectToSniffEmacs()
{
#ifdef WIN32		/* Windows Version of the Code */
    HANDLE ToSniffEmacs[2], FromSniffEmacs[2];
    SECURITY_ATTRIBUTES sa;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    if (! CreatePipe(&ToSniffEmacs[0], &ToSniffEmacs[1], &sa, 0))
	return 1;
    if (! CreatePipe(&FromSniffEmacs[0], &FromSniffEmacs[1], &sa, 0))
	return 1;

    sniffemacs_handle = ExecuteDetachedProgram(SniffEmacs[0], SniffEmacs[0],
	ToSniffEmacs[0], FromSniffEmacs[1]);

    if (sniffemacs_handle)
    {
	handle_to_sniff  = ToSniffEmacs[1];
	handle_from_sniff = FromSniffEmacs[0];
	sniff_connected = 1;
	hBufferMutex = CreateMutex(
	    NULL,			/* no security attributes */
	    FALSE,			/* initially not owned */
	    "SniffReadBufferMutex");    /* name of mutex */
	if (hBufferMutex == NULL)
	{
	    /* Check for error. */
	}
	readthread_handle = (HANDLE)_beginthread(SniffEmacsReadThread, 0, NULL);
	return 0;
    }
    else
    {
	/* error in spawn() */
	return 1;
    }

#else		/* UNIX Version of the Code */
    int ToSniffEmacs[2], FromSniffEmacs[2];

    if (pipe(ToSniffEmacs) != 0)
	return 1;
    if (pipe(FromSniffEmacs) != 0)
	return 1;

    /* fork */
    if ((sniffemacs_pid=fork()) == 0)
    {
	/* child */

	/* prepare communication pipes */
	close(ToSniffEmacs[1]);
	close(FromSniffEmacs[0]);

	dup2(ToSniffEmacs[0],fileno(stdin));   /* write to ToSniffEmacs[1] */
	dup2(FromSniffEmacs[1],fileno(stdout));/* read from FromSniffEmacs[0] */

	close(ToSniffEmacs[0]);
	close(FromSniffEmacs[1]);

	/* start sniffemacs */
	execvp (SniffEmacs[0], SniffEmacs);
	{
/*	    FILE *out = fdopen(FromSniffEmacs[1], "w"); */
	    sleep(1);
	    fputs(_(msg_sniff_disconnect), stdout);
	    fflush(stdout);
	    sleep(3);
#ifdef FEAT_GUI
	    if (gui.in_use)
		gui_exit(1);
#endif
	    exit(1);
	}
	return 1;
    }
    else if (sniffemacs_pid > 0)
    {
	/* parent process */
	close(ToSniffEmacs[0]);
	fd_to_sniff  = ToSniffEmacs[1];
	close(FromSniffEmacs[1]);
	fd_from_sniff = FromSniffEmacs[0];
	sniff_connected = 1;
	return 0;
    }
    else   /* error in fork() */
	return 1;
#endif		/* UNIX Version of the Code */
}


/* HandleSniffRequest
 * Handle one request from SNiFF+
 */
    static void
HandleSniffRequest(buffer)
    char *buffer;
{
    char VICommand[MAX_REQUEST_LEN];
    char command;
    char *arguments;
    char *token;
    char *argv[3];
    int argc = 0;
    buf_T  *buf;

    const char *SetTab     = "set tabstop=%d";
    const char *SelectBuf  = "buf %s";
    const char *DeleteBuf  = "bd %s";
    const char *UnloadBuf  = "bun %s";
    const char *GotoLine   = "%d";

    command   = buffer[0];
    arguments = &buffer[1];
    token = strtok(arguments, sniff_rq_sep);
    while(argc <3)
    {
	if (token)
	{
	    argv[argc] = (char*)vim_strsave((char_u *)token);
	    token = strtok(0, sniff_rq_sep);
	}
	else
	    argv[argc] = strdup("");
	argc++;
    }

    switch (command)
    {
	case 'o' :  /* visit file at char pos */
	case 'O' :  /* visit file at line number */
	{
	    char *file = argv[0];
	    int position = atoi(argv[1]);

	    buf = vi_find_buffer(file);
	    setpcmark();      /* insert current pos in jump list [mark.c]*/
	    if (!buf)
		vi_open_file(file);
	    else if (buf!=curbuf)
	    {
		vim_snprintf(VICommand, sizeof(VICommand),
						     (char *)SelectBuf, file);
		vi_exec_cmd(VICommand);
	    }
	    if (command == 'o')
		vi_set_cursor_pos((long)position);
	    else
	    {
		vim_snprintf(VICommand, sizeof(VICommand),
					     (char *)GotoLine, (int)position);
		vi_exec_cmd(VICommand);
	    }
	    checkpcmark();	/* [mark.c] */
#if defined(FEAT_GUI_X11) || defined(FEAT_GUI_W32)
	    if (gui.in_use && !gui.in_focus)  /* Raise Vim Window */
	    {
# ifdef FEAT_GUI_W32
		SetForegroundWindow(s_hwnd);
# else
		extern Widget vimShell;

		XSetInputFocus(gui.dpy, XtWindow(vimShell), RevertToNone,
			CurrentTime);
		XRaiseWindow(gui.dpy, XtWindow(vimShell));
# endif
	    }
#endif
	    break;
	}
	case 'p' :  /* path of file has changed */
	    /* when changing from shared to private WS (checkout) */
	{
	    char *file = argv[0];
	    char *new_path = argv[1];

	    buf = vi_find_buffer(file);
	    if (buf && !buf->b_changed) /* delete buffer only if not modified */
	    {
		vim_snprintf(VICommand, sizeof(VICommand),
						     (char *)DeleteBuf, file);
		vi_exec_cmd(VICommand);
	    }
	    vi_open_file(new_path);
	    break;
	}
	case 'w' :  /* writability has changed */
	    /* Sniff sends request twice,
	     * but only the last one is the right one */
	{
	    char *file = argv[0];
	    int writable = atoi(argv[1]);

	    buf = vi_find_buffer(file);
	    if (buf)
	    {
		buf->b_p_ro = !writable;
		if (buf != curbuf)
		{
		    buf->b_flags |= BF_CHECK_RO + BF_NEVERLOADED;
		    if (writable && !buf->b_changed)
		    {
			vim_snprintf(VICommand, sizeof(VICommand),
						     (char *)UnloadBuf, file);
			vi_exec_cmd(VICommand);
		    }
		}
		else if (writable && !buf->b_changed)
		{
		    vi_exec_cmd("e");
		}
	    }
	    break;
	}
	case 'h' :  /* highlight info */
	    break;  /* not implemented */

	case 't' :  /* Set tab width */
	{
	    int tab_width = atoi(argv[1]);

	    if (tab_width > 0 && tab_width <= 16)
	    {
		vim_snprintf(VICommand, sizeof(VICommand),
						   (char *)SetTab, tab_width);
		vi_exec_cmd(VICommand);
	    }
	    break;
	}
	case '|':
	{
	    /* change the request separator */
	    sniff_rq_sep[0] = arguments[0];
	    /* echo the request */
	    WriteToSniff(buffer);
	    break;
	}
	case 'A' :  /* Warning/Info msg */
	    vi_msg(arguments);
	    if (!strncmp(arguments, "Disconnected", 12))
		sniff_disconnect(1);	/* unexpected disconnection */
	    break;
	case 'a' :  /* Error msg */
	    vi_error_msg(arguments);
	    if (!strncmp(arguments, "Cannot connect", 14))
		sniff_disconnect(1);
	    break;

	default :
	    break;
    }
    while(argc)
	vim_free(argv[--argc]);
}


#ifndef WIN32
/* get_request
 * read string from fd up to next newline (excluding the nl),
 * returns  length of string
 *	    0 if no data available or no complete line
 *	   <0 on error
 */
    static int
get_request(fd, buf, maxlen)
    int		fd;
    char	*buf;
    int		maxlen;
{
    static char	inbuf[1024];
    static int	pos = 0, bytes = 0;
    int		len;
#ifdef HAVE_SELECT
    struct timeval tval;
    fd_set	rfds;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);
    tval.tv_sec  = 0;
    tval.tv_usec = 0;
#else
    struct pollfd fds;

    fds.fd = fd;
    fds.events = POLLIN;
#endif

    for (len = 0; len < maxlen; len++)
    {
	if (pos >= bytes)	    /* end of buffer reached? */
	{
#ifdef HAVE_SELECT
	    if (select(fd + 1, &rfds, NULL, NULL, &tval) > 0)
#else
	    if (poll(&fds, 1, 0) > 0)
#endif
	    {
		pos = 0;
		bytes = read(fd, inbuf, sizeof(inbuf));
		if (bytes <= 0)
		    return bytes;
	    }
	    else
	    {
		pos = pos-len;
		buf[0] = '\0';
		return 0;
	    }
	}
	if ((buf[len] = inbuf[pos++]) =='\n')
	    break;
    }
    buf[len] = '\0';
    return len;
}
#endif     /* WIN32 */


    static void
SendRequest(command, symbol)
    struct sn_cmd *command;
    char *symbol;
{
    int		cmd_type = command->cmd_type;
    static char cmdstr[MAX_REQUEST_LEN];
    static char msgtxt[MAX_REQUEST_LEN];
    char	*buffer_name = NULL;

    if (cmd_type == RQ_CONNECT)
    {
	sniff_connect();
	return;
    }
    if (!sniff_connected && !(cmd_type & SILENT))
    {
	vi_error_msg(_("E278: SNiFF+ not connected"));
	return;
    }

    if (cmd_type & NEED_FILE)
    {
	if (!curbuf->b_sniff)
	{
	    if (!(cmd_type & SILENT))
		vi_error_msg(_("E279: Not a SNiFF+ buffer"));
	    return;
	}
	buffer_name = vi_buffer_name();
	if (buffer_name == NULL)
	    return;
	if (cmd_type & NEED_SYMBOL)
	{
	    if (cmd_type & EMPTY_SYMBOL)
		symbol = " ";
	    else if (!symbol && !(symbol = vi_symbol_under_cursor()))
		return;	    /* error msg already displayed */
	}

	if (symbol)
	    vim_snprintf(cmdstr, sizeof(cmdstr), "%c%s%s%ld%s%s\n",
		command->cmd_code,
		buffer_name,
		sniff_rq_sep,
		vi_cursor_pos(),
		sniff_rq_sep,
		symbol
	    );
	else
	    vim_snprintf(cmdstr, sizeof(cmdstr), "%c%s\n",
		    command->cmd_code, buffer_name);
    }
    else    /* simple request */
    {
	cmdstr[0] = command->cmd_code;
	cmdstr[1] = '\n';
	cmdstr[2] = '\0';
    }
    if (command->cmd_msg && !(cmd_type & SILENT))
    {
	if ((cmd_type & NEED_SYMBOL) && !(cmd_type & EMPTY_SYMBOL))
	{
	    vim_snprintf(msgtxt, sizeof(msgtxt), "%s: %s",
						 _(command->cmd_msg), symbol);
	    vi_msg(msgtxt);
	}
	else
	    vi_msg(_(command->cmd_msg));
    }
    WriteToSniff(cmdstr);
    if (cmd_type & DISCONNECT)
	sniff_disconnect(0);
}



    static void
WriteToSniff(str)
    char *str;
{
    int bytes;
#ifdef WIN32
    if (! WriteFile(handle_to_sniff, str, strlen(str), &bytes, NULL))
    {
	DWORD err=GetLastError();
	bytes = -1;
    }
#else
    bytes = write(fd_to_sniff, str, strlen(str));
#endif
    if (bytes<0)
    {
	vi_msg(_("Sniff: Error during write. Disconnected"));
	sniff_disconnect(1);
    }
}

/*-------- vim helping functions --------------------------------*/

    static void
vi_msg(str)
    char *str;
{
    if (str != NULL && *str != NUL)
	MSG((char_u *)str);
}

    static void
vi_error_msg(str)
    char *str;
{
    if (str != NULL && *str != NUL)
	EMSG((char_u *)str);
}

    static void
vi_open_file(fname)
    char *fname;
{
    ++no_wait_return;
    do_ecmd(0, (char_u *)fname, NULL, NULL, ECMD_ONE, ECMD_HIDE+ECMD_OLDBUF,
	    curwin);
    curbuf->b_sniff = TRUE;
    --no_wait_return;					/* [ex_docmd.c] */
}

    static buf_T *
vi_find_buffer(fname)
    char *fname;
{			    /* derived from buflist_findname() [buffer.c] */
    buf_T	*buf;

    for (buf = firstbuf; buf != NULL; buf = buf->b_next)
	if (buf->b_sfname != NULL && fnamecmp(fname, buf->b_sfname) == 0)
	    return (buf);
    return NULL;
}


    static char *
vi_symbol_under_cursor()
{
    int		len;
    char	*symbolp;
    char	*p;
    static char sniff_symbol[256];

    len = find_ident_under_cursor((char_u **)&symbolp, FIND_IDENT);
    /* [normal.c] */
    if (len <= 0)
	return NULL;
    for (p=sniff_symbol; len; len--)
	*p++ = *symbolp++;
    *p = '\0';
    return sniff_symbol;
}


    static char *
vi_buffer_name()
{
    return (char *)curbuf->b_sfname;
}

    static void
vi_exec_cmd(vicmd)
    char *vicmd;
{
    do_cmdline_cmd((char_u *)vicmd);  /* [ex_docmd.c] */
}

/*
 * Set cursor on character position
 * derived from cursor_pos_info() [buffer.c]
 */
    static void
vi_set_cursor_pos(char_pos)
    long char_pos;
{
    linenr_T	lnum;
    long	char_count = 1;  /* first position = 1 */
    int		line_size;
    int		eol_size;

    if (char_pos == 0)
    {
	char_pos = 1;
    }
    if (get_fileformat(curbuf) == EOL_DOS)
	eol_size = 2;
    else
	eol_size = 1;
    for (lnum = 1; lnum <= curbuf->b_ml.ml_line_count; ++lnum)
    {
	line_size = STRLEN(ml_get(lnum)) + eol_size;
	if (char_count+line_size > char_pos) break;
	char_count += line_size;
    }
    curwin->w_cursor.lnum = lnum;
    curwin->w_cursor.col  = char_pos - char_count;
}

    static long
vi_cursor_pos()
{
    linenr_T	lnum;
    long	char_count=1;  /* sniff starts with pos 1 */
    int		line_size;
    int		eol_size;

    if (curbuf->b_p_tx)
	eol_size = 2;
    else
	eol_size = 1;
    for (lnum = 1; lnum < curwin->w_cursor.lnum; ++lnum)
    {
	line_size = STRLEN(ml_get(lnum)) + eol_size;
	char_count += line_size;
    }
    return char_count + curwin->w_cursor.col;
}
