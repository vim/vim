/* vi:set ts=8 sts=4 sw=4:
 *
 * VIM - Vi IMproved	by Bram Moolenaar
 *
 * Do ":help uganda"  in Vim to read copying and usage conditions.
 * Do ":help credits" in Vim to see a list of people who contributed.
 * See README.txt for an overview of the Vim source code.
 */

/*
 * uninstal.c:	Minimalistic uninstall program for Vim on MS-Windows
 *		Removes:
 *		- the "Edit with Vim" popup menu entry
 *		- the Vim "Open With..." popup menu entry
 *		- any Vim Batch files in the path
 *		- icons for Vim on the Desktop
 *		- the Vim entry in the Start Menu
 */

/* Include common code for dosinst.c and uninstal.c. */
#include "dosinst.h"

/*
 * Return TRUE if the user types a 'y' or 'Y', FALSE otherwise.
 */
    static int
confirm(void)
{
    char	answer[10];

    fflush(stdout);
    return (scanf(" %c", answer) == 1 && toupper(answer[0]) == 'Y');
}

#ifdef WIN3264

    static int
reg_delete_key(HKEY hRootKey, const char *key)
{
    static int did_load = FALSE;
    static HANDLE advapi_lib = NULL;
    static LONG (WINAPI *delete_key_ex)(HKEY, LPCTSTR, REGSAM, DWORD) = NULL;

    if (!did_load)
    {
	/* The RegDeleteKeyEx() function is only available on new systems.  It
	 * is required for 64-bit registry access.  For other systems fall
	 * back to RegDeleteKey(). */
	did_load = TRUE;
	advapi_lib = LoadLibrary("ADVAPI32.DLL");
	if (advapi_lib != NULL)
	    delete_key_ex = (LONG (WINAPI *)(HKEY, LPCTSTR, REGSAM, DWORD))GetProcAddress(advapi_lib, "RegDeleteKeyExA");
    }
    if (delete_key_ex != NULL) {
	return (*delete_key_ex)(hRootKey, key, KEY_WOW64_64KEY, 0);
    }
    return RegDeleteKey(hRootKey, key);
}

/*
 * Check if the popup menu entry exists and what gvim it refers to.
 * Returns non-zero when it's found.
 */
    static int
popup_gvim_path(char *buf)
{
    HKEY	key_handle;
    DWORD	value_type;
    DWORD	bufsize = BUFSIZE;
    int		r;

    /* Open the key where the path to gvim.exe is stored. */
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Vim\\Gvim", 0,
		    KEY_WOW64_64KEY | KEY_READ, &key_handle) != ERROR_SUCCESS)
	return 0;

    /* get the DisplayName out of it to show the user */
    r = RegQueryValueEx(key_handle, "path", 0,
					  &value_type, (LPBYTE)buf, &bufsize);
    RegCloseKey(key_handle);

    return (r == ERROR_SUCCESS);
}

/*
 * Check if the "Open With..." menu entry exists and what gvim it refers to.
 * Returns non-zero when it's found.
 */
    static int
openwith_gvim_path(char *buf)
{
    HKEY	key_handle;
    DWORD	value_type;
    DWORD	bufsize = BUFSIZE;
    int		r;

    /* Open the key where the path to gvim.exe is stored. */
    if (RegOpenKeyEx(HKEY_CLASSES_ROOT,
		"Applications\\gvim.exe\\shell\\edit\\command", 0,
		    KEY_WOW64_64KEY | KEY_READ, &key_handle) != ERROR_SUCCESS)
	return 0;

    /* get the DisplayName out of it to show the user */
    r = RegQueryValueEx(key_handle, "", 0, &value_type, (LPBYTE)buf, &bufsize);
    RegCloseKey(key_handle);

    return (r == ERROR_SUCCESS);
}

    static void
remove_popup(void)
{
    int		fail = 0;
    HKEY	kh;

    if (reg_delete_key(HKEY_CLASSES_ROOT, "CLSID\\{51EEE242-AD87-11d3-9C1E-0090278BBD99}\\InProcServer32") != ERROR_SUCCESS)
	++fail;
    if (reg_delete_key(HKEY_CLASSES_ROOT, "CLSID\\{51EEE242-AD87-11d3-9C1E-0090278BBD99}") != ERROR_SUCCESS)
	++fail;
    if (reg_delete_key(HKEY_CLASSES_ROOT, "*\\shellex\\ContextMenuHandlers\\gvim") != ERROR_SUCCESS)
	++fail;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", 0,
		      KEY_WOW64_64KEY | KEY_ALL_ACCESS, &kh) != ERROR_SUCCESS)
	++fail;
    else
    {
	if (RegDeleteValue(kh, "{51EEE242-AD87-11d3-9C1E-0090278BBD99}") != ERROR_SUCCESS)
	    ++fail;
	RegCloseKey(kh);
    }
    if (reg_delete_key(HKEY_LOCAL_MACHINE, "Software\\Vim\\Gvim") != ERROR_SUCCESS)
	++fail;
    if (reg_delete_key(HKEY_LOCAL_MACHINE, "Software\\Vim") != ERROR_SUCCESS)
	++fail;

    if (fail == 6)
	printf("No Vim popup registry entries could be removed\n");
    else if (fail > 0)
	printf("Some Vim popup registry entries could not be removed\n");
    else
	printf("The Vim popup registry entries have been removed\n");
}

    static void
remove_openwith(void)
{
    int		fail = 0;

    if (reg_delete_key(HKEY_CLASSES_ROOT, "Applications\\gvim.exe\\shell\\edit\\command") != ERROR_SUCCESS)
	++fail;
    if (reg_delete_key(HKEY_CLASSES_ROOT, "Applications\\gvim.exe\\shell\\edit") != ERROR_SUCCESS)
	++fail;
    if (reg_delete_key(HKEY_CLASSES_ROOT, "Applications\\gvim.exe\\shell") != ERROR_SUCCESS)
	++fail;
    if (reg_delete_key(HKEY_CLASSES_ROOT, "Applications\\gvim.exe") != ERROR_SUCCESS)
	++fail;
    if (reg_delete_key(HKEY_CLASSES_ROOT, ".htm\\OpenWithList\\gvim.exe") != ERROR_SUCCESS)
	++fail;
    if (reg_delete_key(HKEY_CLASSES_ROOT, ".vim\\OpenWithList\\gvim.exe") != ERROR_SUCCESS)
	++fail;
    if (reg_delete_key(HKEY_CLASSES_ROOT, "*\\OpenWithList\\gvim.exe") != ERROR_SUCCESS)
	++fail;

    if (fail == 7)
	printf("No Vim open-with registry entries could be removed\n");
    else if (fail > 0)
	printf("Some Vim open-with registry entries could not be removed\n");
    else
	printf("The Vim open-with registry entries have been removed\n");
}
#endif

/*
 * Check if a batch file is really for the current version.  Don't delete a
 * batch file that was written for another (possibly newer) version.
 */
    static int
batfile_thisversion(char *path)
{
    FILE	*fd;
    char	line[BUFSIZE];
    char	*p;
    int		ver_len = strlen(VIM_VERSION_NODOT);
    int		found = FALSE;

    fd = fopen(path, "r");
    if (fd != NULL)
    {
	while (fgets(line, BUFSIZE, fd) != NULL)
	{
	    for (p = line; *p != 0; ++p)
		/* don't accept "vim60an" when looking for "vim60". */
		if (strnicmp(p, VIM_VERSION_NODOT, ver_len) == 0
			&& !isdigit(p[ver_len])
			&& !isalpha(p[ver_len]))
		{
		    found = TRUE;
		    break;
		}
	    if (found)
		break;
	}
	fclose(fd);
    }
    return found;
}

    static int
remove_batfiles(int doit)
{
    char *batfile_path;
    int	 i;
    int	 found = 0;

    for (i = 1; i < TARGET_COUNT; ++i)
    {
	batfile_path = searchpath_save(targets[i].batname);
	if (batfile_path != NULL && batfile_thisversion(batfile_path))
	{
	    ++found;
	    if (doit)
	    {
		printf("removing %s\n", batfile_path);
		remove(batfile_path);
	    }
	    else
		printf(" - the batch file %s\n", batfile_path);
	    free(batfile_path);
	}
    }
    return found;
}

#ifdef WIN3264
    static void
remove_if_exists(char *path, char *filename)
{
    char buf[BUFSIZE];
    FILE *fd;

    sprintf(buf, "%s\\%s", path, filename);

    fd = fopen(buf, "r");
    if (fd != NULL)
    {
	fclose(fd);
	printf("removing %s\n", buf);
	remove(buf);
    }
}

    static void
remove_icons(void)
{
    char	path[BUFSIZE];
    int		i;

    if (get_shell_folder_path(path, "desktop"))
	for (i = 0; i < ICON_COUNT; ++i)
	    remove_if_exists(path, icon_link_names[i]);
}

    static void
remove_start_menu(void)
{
    char	path[BUFSIZE];
    int		i;
    struct stat st;

    if (get_shell_folder_path(path, VIM_STARTMENU))
    {
	for (i = 1; i < TARGET_COUNT; ++i)
	    remove_if_exists(path, targets[i].lnkname);
	remove_if_exists(path, "uninstall.lnk");
	remove_if_exists(path, "Help.lnk");
	/* Win95 uses .pif, WinNT uses .lnk */
	remove_if_exists(path, "Vim tutor.pif");
	remove_if_exists(path, "Vim tutor.lnk");
	remove_if_exists(path, "Vim online.url");
	if (stat(path, &st) == 0)
	{
	    printf("removing %s\n", path);
	    rmdir(path);
	}
    }
}
#endif

    static void
delete_uninstall_key(void)
{
#ifdef WIN3264
    reg_delete_key(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Vim " VIM_VERSION_SHORT);
#else
    FILE	*fd;
    char	buf[BUFSIZE];

    /*
     * On DJGPP we delete registry entries by creating a .inf file and
     * installing it.
     */
    fd = fopen("vim.inf", "w");
    if (fd != NULL)
    {
	fprintf(fd, "[version]\n");
	fprintf(fd, "signature=\"$CHICAGO$\"\n\n");
	fprintf(fd, "[DefaultInstall]\n");
	fprintf(fd, "DelReg=DeleteMe\n\n");
	fprintf(fd, "[DeleteMe]\n");
	fprintf(fd, "HKLM,\"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Vim " VIM_VERSION_SHORT "\"\n");
	fclose(fd);

	/* Don't know how to detect Win NT with DJGPP.  Hack: Just try the Win
	 * 95/98/ME method, since the DJGPP version can't use long filenames
	 * on Win NT anyway. */
	sprintf(buf, "rundll setupx.dll,InstallHinfSection DefaultInstall 132 %s\\vim.inf", installdir);
	run_command(buf);
#if 0
	/* Windows NT method (untested). */
	sprintf(buf, "rundll32 syssetup,SetupInfObjectInstallAction DefaultInstall 128 %s\\vim.inf", installdir);
	run_command(buf);
#endif

	remove("vim.inf");
    }
#endif
}

    int
main(int argc, char *argv[])
{
    int		found = 0;
    FILE	*fd;
#ifdef WIN3264
    int		i;
    struct stat st;
    char	icon[BUFSIZE];
    char	path[BUFSIZE];
    char	popup_path[BUFSIZE];

    /* The nsis uninstaller calls us with a "-nsis" argument. */
    if (argc == 2 && stricmp(argv[1], "-nsis") == 0)
	interactive = FALSE;
    else
#endif
	interactive = TRUE;

    /* Initialize this program. */
    do_inits(argv);

    printf("This program will remove the following items:\n");

#ifdef WIN3264
    if (popup_gvim_path(popup_path))
    {
	printf(" - the \"Edit with Vim\" entry in the popup menu\n");
	printf("   which uses \"%s\"\n", popup_path);
	if (interactive)
	    printf("\nRemove it (y/n)? ");
	if (!interactive || confirm())
	{
	    remove_popup();
	    /* Assume the "Open With" entry can be removed as well, don't
	     * bother the user with asking him again. */
	    remove_openwith();
	}
    }
    else if (openwith_gvim_path(popup_path))
    {
	printf(" - the Vim \"Open With...\" entry in the popup menu\n");
	printf("   which uses \"%s\"\n", popup_path);
	printf("\nRemove it (y/n)? ");
	if (confirm())
	    remove_openwith();
    }

    if (get_shell_folder_path(path, "desktop"))
    {
	printf("\n");
	for (i = 0; i < ICON_COUNT; ++i)
	{
	    sprintf(icon, "%s\\%s", path, icon_link_names[i]);
	    if (stat(icon, &st) == 0)
	    {
		printf(" - the \"%s\" icon on the desktop\n", icon_names[i]);
		++found;
	    }
	}
	if (found > 0)
	{
	    if (interactive)
		printf("\nRemove %s (y/n)? ", found > 1 ? "them" : "it");
	    if (!interactive || confirm())
		remove_icons();
	}
    }

    if (get_shell_folder_path(path, VIM_STARTMENU)
	    && stat(path, &st) == 0)
    {
	printf("\n - the \"%s\" entry in the Start Menu\n", VIM_STARTMENU);
	if (interactive)
	    printf("\nRemove it (y/n)? ");
	if (!interactive || confirm())
	    remove_start_menu();
    }
#endif

    printf("\n");
    found = remove_batfiles(0);
    if (found > 0)
    {
	if (interactive)
	    printf("\nRemove %s (y/n)? ", found > 1 ? "them" : "it");
	if (!interactive || confirm())
	    remove_batfiles(1);
    }

    fd = fopen("gvim.exe", "r");
    if (fd != NULL)
    {
	fclose(fd);
	printf("gvim.exe detected.  Attempting to unregister gvim with OLE\n");
	system("gvim.exe -silent -unregister");
    }

    delete_uninstall_key();

    if (interactive)
    {
	printf("\nYou may now want to delete the Vim executables and runtime files.\n");
	printf("(They are still where you unpacked them.)\n");
    }

    if (interactive)
    {
	rewind(stdin);
	printf("\nPress Enter to exit...");
	(void)getchar();
    }
    else
	sleep(3);

    return 0;
}
