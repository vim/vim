/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */
/*
 * dosinst.h: Common code for dosinst.c and uninstal.c
 */

/* Visual Studio 2005 has 'deprecated' many of the standard CRT functions */
#if _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifndef UNIX_LINT
# include "vimio.h"
# include <ctype.h>

# ifndef __CYGWIN__
#  include <direct.h>
# endif

# if defined(_WIN64) || defined(WIN32)
#  define WIN3264
#  include <windows.h>
#  include <shlobj.h>
# else
#  include <dir.h>
#  include <bios.h>
#  include <dos.h>
# endif
#endif

#ifdef UNIX_LINT
/* Running lint on Unix: Some things are missing. */
char *searchpath(char *name);
#endif

#if defined(DJGPP) || defined(UNIX_LINT)
# include <unistd.h>
# include <errno.h>
#endif

#include "version.h"

#if defined(DJGPP) || defined(UNIX_LINT)
# define vim_mkdir(x, y) mkdir((char *)(x), y)
#else
# if defined(WIN3264) && !defined(__BORLANDC__)
#  define vim_mkdir(x, y) _mkdir((char *)(x))
# else
#  define vim_mkdir(x, y) mkdir((char *)(x))
# endif
#endif
/* ---------------------------------------- */


#define BUFSIZE 512		/* long enough to hold a file name path */
#define NUL 0

#define FAIL 0
#define OK 1

#ifndef FALSE
# define FALSE 0
#endif
#ifndef TRUE
# define TRUE 1
#endif

#define VIM_STARTMENU "Programs\\Vim " VIM_VERSION_SHORT

int	interactive;		/* non-zero when running interactively */

/*
 * Call malloc() and exit when out of memory.
 */
    static void *
alloc(int len)
{
    char *s;

    s = malloc(len);
    if (s == NULL)
    {
	printf("ERROR: out of memory\n");
	exit(1);
    }
    return (void *)s;
}

/*
 * The toupper() in Bcc 5.5 doesn't work, use our own implementation.
 */
    static int
mytoupper(int c)
{
    if (c >= 'a' && c <= 'z')
	return c - 'a' + 'A';
    return c;
}

    static void
myexit(int n)
{
    if (!interactive)
    {
	/* Present a prompt, otherwise error messages can't be read. */
	printf("Press Enter to continue\n");
	rewind(stdin);
	(void)getchar();
    }
    exit(n);
}

#ifdef WIN3264
/* This symbol is not defined in older versions of the SDK or Visual C++ */

#ifndef VER_PLATFORM_WIN32_WINDOWS
# define VER_PLATFORM_WIN32_WINDOWS 1
#endif

static DWORD g_PlatformId;

/*
 * Set g_PlatformId to VER_PLATFORM_WIN32_NT (NT) or
 * VER_PLATFORM_WIN32_WINDOWS (Win95).
 */
    static void
PlatformId(void)
{
    static int done = FALSE;

    if (!done)
    {
	OSVERSIONINFO ovi;

	ovi.dwOSVersionInfoSize = sizeof(ovi);
	GetVersionEx(&ovi);

	g_PlatformId = ovi.dwPlatformId;
	done = TRUE;
    }
}

# ifdef __BORLANDC__
/* Borland defines its own searchpath() in dir.h */
#  include <dir.h>
# else
    static char *
searchpath(char *name)
{
    static char widename[2 * BUFSIZE];
    static char location[2 * BUFSIZE + 2];

    /* There appears to be a bug in FindExecutableA() on Windows NT.
     * Use FindExecutableW() instead... */
    PlatformId();
    if (g_PlatformId == VER_PLATFORM_WIN32_NT)
    {
	MultiByteToWideChar(CP_ACP, 0, (LPCTSTR)name, -1,
		(LPWSTR)widename, BUFSIZE);
	if (FindExecutableW((LPCWSTR)widename, (LPCWSTR)"",
		    (LPWSTR)location) > (HINSTANCE)32)
	{
	    WideCharToMultiByte(CP_ACP, 0, (LPWSTR)location, -1,
		    (LPSTR)widename, 2 * BUFSIZE, NULL, NULL);
	    return widename;
	}
    }
    else
    {
	if (FindExecutableA((LPCTSTR)name, (LPCTSTR)"",
		    (LPTSTR)location) > (HINSTANCE)32)
	    return location;
    }
    return NULL;
}
# endif
#endif

/*
 * Call searchpath() and save the result in allocated memory, or return NULL.
 */
    static char *
searchpath_save(char *name)
{
    char	*p;
    char	*s;

    p = searchpath(name);
    if (p == NULL)
	return NULL;
    s = alloc(strlen(p) + 1);
    strcpy(s, p);
    return s;
}

#ifdef WIN3264

#ifndef CSIDL_COMMON_PROGRAMS
# define CSIDL_COMMON_PROGRAMS 0x0017
#endif
#ifndef CSIDL_COMMON_DESKTOPDIRECTORY
# define CSIDL_COMMON_DESKTOPDIRECTORY 0x0019
#endif

/*
 * Get the path to a requested Windows shell folder.
 *
 * Return FAIL on error, OK on success
 */
    int
get_shell_folder_path(
	char *shell_folder_path,
	const char *shell_folder_name)
{
    /*
     * The following code was successfully built with make_mvc.mak.
     * The resulting executable worked on Windows 95, Millennium Edition, and
     * 2000 Professional.  But it was changed after testing...
     */
    LPITEMIDLIST    pidl = 0; /* Pointer to an Item ID list allocated below */
    LPMALLOC	    pMalloc;  /* Pointer to an IMalloc interface */
    int		    csidl;
    int		    alt_csidl = -1;
    static int	    desktop_csidl = -1;
    static int	    programs_csidl = -1;
    int		    *pcsidl;
    int		    r;

    if (strcmp(shell_folder_name, "desktop") == 0)
    {
	pcsidl = &desktop_csidl;
	csidl = CSIDL_COMMON_DESKTOPDIRECTORY;
	alt_csidl = CSIDL_DESKTOP;
    }
    else if (strncmp(shell_folder_name, "Programs", 8) == 0)
    {
	pcsidl = &programs_csidl;
	csidl = CSIDL_COMMON_PROGRAMS;
	alt_csidl = CSIDL_PROGRAMS;
    }
    else
    {
	printf("\nERROR (internal) unrecognised shell_folder_name: \"%s\"\n\n",
							   shell_folder_name);
	return FAIL;
    }

    /* Did this stuff before, use the same ID again. */
    if (*pcsidl >= 0)
    {
	csidl = *pcsidl;
	alt_csidl = -1;
    }

retry:
    /* Initialize pointer to IMalloc interface */
    if (NOERROR != SHGetMalloc(&pMalloc))
    {
	printf("\nERROR getting interface for shell_folder_name: \"%s\"\n\n",
							   shell_folder_name);
	return FAIL;
    }

    /* Get an ITEMIDLIST corresponding to the folder code */
    if (NOERROR != SHGetSpecialFolderLocation(0, csidl, &pidl))
    {
	if (alt_csidl < 0 || NOERROR != SHGetSpecialFolderLocation(0,
							    alt_csidl, &pidl))
	{
	    printf("\nERROR getting ITEMIDLIST for shell_folder_name: \"%s\"\n\n",
							   shell_folder_name);
	    return FAIL;
	}
	csidl = alt_csidl;
	alt_csidl = -1;
    }

    /* Translate that ITEMIDLIST to a string */
    r = SHGetPathFromIDList(pidl, shell_folder_path);

    /* Free the data associated with pidl */
    pMalloc->lpVtbl->Free(pMalloc, pidl);
    /* Release the IMalloc interface */
    pMalloc->lpVtbl->Release(pMalloc);

    if (!r)
    {
	if (alt_csidl >= 0)
	{
	    /* We probably get here for Windows 95: the "all users"
	     * desktop/start menu entry doesn't exist. */
	    csidl = alt_csidl;
	    alt_csidl = -1;
	    goto retry;
	}
	printf("\nERROR translating ITEMIDLIST for shell_folder_name: \"%s\"\n\n",
							   shell_folder_name);
	return FAIL;
    }

    /* If there is an alternative: verify we can write in this directory.
     * This should cause a retry when the "all users" directory exists but we
     * are a normal user and can't write there. */
    if (alt_csidl >= 0)
    {
	char tbuf[BUFSIZE];
	FILE *fd;

	strcpy(tbuf, shell_folder_path);
	strcat(tbuf, "\\vim write test");
	fd = fopen(tbuf, "w");
	if (fd == NULL)
	{
	    csidl = alt_csidl;
	    alt_csidl = -1;
	    goto retry;
	}
	fclose(fd);
	unlink(tbuf);
    }

    /*
     * Keep the found csidl for next time, so that we don't have to do the
     * write test every time.
     */
    if (*pcsidl < 0)
	*pcsidl = csidl;

    if (strncmp(shell_folder_name, "Programs\\", 9) == 0)
	strcat(shell_folder_path, shell_folder_name + 8);

    return OK;
}
#endif

/*
 * List of targets.  The first one (index zero) is used for the default path
 * for the batch files.
 */
#define TARGET_COUNT  9

struct
{
    char	*name;		/* Vim exe name (without .exe) */
    char	*batname;	/* batch file name */
    char	*lnkname;	/* shortcut file name */
    char	*exename;	/* exe file name */
    char	*exenamearg;	/* exe file name when using exearg */
    char	*exearg;	/* argument for vim.exe or gvim.exe */
    char	*oldbat;	/* path to existing xxx.bat or NULL */
    char	*oldexe;	/* path to existing xxx.exe or NULL */
    char	batpath[BUFSIZE];  /* path of batch file to create; not
				      created when it's empty */
} targets[TARGET_COUNT] =
{
    {"all",	"batch files"},
    {"vim",	"vim.bat",	"Vim.lnk",
					"vim.exe",    "vim.exe",  ""},
    {"gvim",	"gvim.bat",	"gVim.lnk",
					"gvim.exe",   "gvim.exe", ""},
    {"evim",	"evim.bat",	"gVim Easy.lnk",
					"evim.exe",   "gvim.exe", "-y"},
    {"view",	"view.bat",	"Vim Read-only.lnk",
					"view.exe",   "vim.exe",  "-R"},
    {"gview",	"gview.bat",	"gVim Read-only.lnk",
					"gview.exe",  "gvim.exe", "-R"},
    {"vimdiff", "vimdiff.bat",	"Vim Diff.lnk",
					"vimdiff.exe","vim.exe",  "-d"},
    {"gvimdiff","gvimdiff.bat",	"gVim Diff.lnk",
					"gvimdiff.exe","gvim.exe", "-d"},
    {"vimtutor","vimtutor.bat", "Vim tutor.lnk",
					"vimtutor.bat",  "vimtutor.bat", ""},
};

#define ICON_COUNT 3
char *(icon_names[ICON_COUNT]) =
	{"gVim " VIM_VERSION_SHORT,
	 "gVim Easy " VIM_VERSION_SHORT,
	 "gVim Read only " VIM_VERSION_SHORT};
char *(icon_link_names[ICON_COUNT]) =
	{"gVim " VIM_VERSION_SHORT ".lnk",
	 "gVim Easy " VIM_VERSION_SHORT ".lnk",
	 "gVim Read only " VIM_VERSION_SHORT ".lnk"};

/* This is only used for dosinst.c and for uninstal.c when not being able to
 * directly access registry entries. */
#if !defined(WIN3264) || defined(DOSINST)
/*
 * Run an external command and wait for it to finish.
 */
    static void
run_command(char *cmd)
{
    char	*cmd_path;
    char	cmd_buf[BUFSIZE];
    char	*p;

    /* On WinNT, 'start' is a shell built-in for cmd.exe rather than an
     * executable (start.exe) like in Win9x.  DJGPP, being a DOS program,
     * is given the COMSPEC command.com by WinNT, so we have to find
     * cmd.exe manually and use it. */
    cmd_path = searchpath_save("cmd.exe");
    if (cmd_path != NULL)
    {
	/* There is a cmd.exe, so this might be Windows NT.  If it is,
	 * we need to call cmd.exe explicitly.  If it is a later OS,
	 * calling cmd.exe won't hurt if it is present.
	 * Also, "wait" on NT expects a window title argument.
	 */
	/* Replace the slashes with backslashes. */
	while ((p = strchr(cmd_path, '/')) != NULL)
	    *p = '\\';
	sprintf(cmd_buf, "%s /c start \"vimcmd\" /w %s", cmd_path, cmd);
	free(cmd_path);
    }
    else
    {
	/* No cmd.exe, just make the call and let the system handle it. */
	sprintf(cmd_buf, "start /w %s", cmd);
    }
    system(cmd_buf);
}
#endif

/*
 * Append a backslash to "name" if there isn't one yet.
 */
    static void
add_pathsep(char *name)
{
    int		len = strlen(name);

    if (len > 0 && name[len - 1] != '\\' && name[len - 1] != '/')
	strcat(name, "\\");
}

/*
 * The normal chdir() does not change the default drive.  This one does.
 */
/*ARGSUSED*/
    int
change_drive(int drive)
{
#ifdef WIN3264
    char temp[3] = "-:";
    temp[0] = (char)(drive + 'A' - 1);
    return !SetCurrentDirectory(temp);
#else
# ifndef UNIX_LINT
    union REGS regs;

    regs.h.ah = 0x0e;
    regs.h.dl = drive - 1;
    intdos(&regs, &regs);   /* set default drive */
    regs.h.ah = 0x19;
    intdos(&regs, &regs);   /* get default drive */
    if (regs.h.al == drive - 1)
	return 0;
# endif
    return -1;
#endif
}

/*
 * Change directory to "path".
 * Return 0 for success, -1 for failure.
 */
    int
mch_chdir(char *path)
{
    if (path[0] == NUL)		/* just checking... */
	return 0;
    if (path[1] == ':')		/* has a drive name */
    {
	if (change_drive(mytoupper(path[0]) - 'A' + 1))
	    return -1;		/* invalid drive name */
	path += 2;
    }
    if (*path == NUL)		/* drive name only */
	return 0;
    return chdir(path);		/* let the normal chdir() do the rest */
}

/*
 * Expand the executable name into a full path name.
 */
#if defined(__BORLANDC__) && !defined(WIN3264)

/* Only Borland C++ has this. */
# define my_fullpath(b, n, l) _fullpath(b, n, l)

#else
    static char *
my_fullpath(char *buf, char *fname, int len)
{
# ifdef WIN3264
    /* Only GetModuleFileName() will get the long file name path.
     * GetFullPathName() may still use the short (FAT) name. */
    DWORD len_read = GetModuleFileName(NULL, buf, (size_t)len);

    return (len_read > 0 && len_read < (DWORD)len) ? buf : NULL;
# else
    char	olddir[BUFSIZE];
    char	*p, *q;
    int		c;
    char	*retval = buf;

    if (strchr(fname, ':') != NULL)	/* already expanded */
    {
	strncpy(buf, fname, len);
    }
    else
    {
	*buf = NUL;
	/*
	 * change to the directory for a moment,
	 * and then do the getwd() (and get back to where we were).
	 * This will get the correct path name with "../" things.
	 */
	p = strrchr(fname, '/');
	q = strrchr(fname, '\\');
	if (q != NULL && (p == NULL || q > p))
	    p = q;
	q = strrchr(fname, ':');
	if (q != NULL && (p == NULL || q > p))
	    p = q;
	if (p != NULL)
	{
	    if (getcwd(olddir, BUFSIZE) == NULL)
	    {
		p = NULL;		/* can't get current dir: don't chdir */
		retval = NULL;
	    }
	    else
	    {
		if (p == fname)		/* /fname		*/
		    q = p + 1;		/* -> /			*/
		else if (q + 1 == p)	/* ... c:\foo		*/
		    q = p + 1;		/* -> c:\		*/
		else			/* but c:\foo\bar	*/
		    q = p;		/* -> c:\foo		*/

		c = *q;			/* truncate at start of fname */
		*q = NUL;
		if (mch_chdir(fname))	/* change to the directory */
		    retval = NULL;
		else
		{
		    fname = q;
		    if (c == '\\')	/* if we cut the name at a */
			fname++;	/* '\', don't add it again */
		}
		*q = c;
	    }
	}
	if (getcwd(buf, len) == NULL)
	{
	    retval = NULL;
	    *buf = NUL;
	}
	/*
	 * Concatenate the file name to the path.
	 */
	if (strlen(buf) + strlen(fname) >= len - 1)
	{
	    printf("ERROR: File name too long!\n");
	    myexit(1);
	}
	add_pathsep(buf);
	strcat(buf, fname);
	if (p)
	    mch_chdir(olddir);
    }

    /* Replace forward slashes with backslashes, required for the path to a
     * command. */
    while ((p = strchr(buf, '/')) != NULL)
	*p = '\\';

    return retval;
# endif
}
#endif

/*
 * Remove the tail from a file or directory name.
 * Puts a NUL on the last '/' or '\'.
 */
    static void
remove_tail(char *path)
{
    int		i;

    for (i = strlen(path) - 1; i > 0; --i)
	if (path[i] == '/' || path[i] == '\\')
	{
	    path[i] = NUL;
	    break;
	}
}


char	installdir[BUFSIZE];	/* top of the installation dir, where the
				   install.exe is located, E.g.:
				   "c:\vim\vim60" */
int	runtimeidx;		/* index in installdir[] where "vim60" starts */
char	*sysdrive;		/* system drive or "c:\" */

/*
 * Setup for using this program.
 * Sets "installdir[]".
 */
    static void
do_inits(char **argv)
{
#ifdef DJGPP
    /*
     * Use Long File Names by default, if $LFN not set.
     */
    if (getenv("LFN") == NULL)
	putenv("LFN=y");
#endif

    /* Find out the full path of our executable. */
    if (my_fullpath(installdir, argv[0], BUFSIZE) == NULL)
    {
	printf("ERROR: Cannot get name of executable\n");
	myexit(1);
    }
    /* remove the tail, the executable name "install.exe" */
    remove_tail(installdir);

    /* change to the installdir */
    mch_chdir(installdir);

    /* Find the system drive.  Only used for searching the Vim executable, not
     * very important. */
    sysdrive = getenv("SYSTEMDRIVE");
    if (sysdrive == NULL || *sysdrive == NUL)
	sysdrive = "C:\\";
}
