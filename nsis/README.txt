This builds a one-click install for Vim for MS Windows using the Nullsoft
Installation System (NSIS), available at http://nsis.sourceforge.net/

To build the installable .exe file:

1.  Unpack three archives:
	PC sources
	PC runtime
	PC language files
    You can generate these from the Unix sources and runtime plus the extra
    archive (see the Makefile in the top directory).

2.  Go to the src directory and build:
	gvim.exe (the OLE version),
	vimrun.exe,
	install.exe,
	uninstall.exe,
	tee/tee.exe,
	xxd/xxd.exe

    Then execute tools/rename.bat to rename the executables.

3.  Go to the GvimExt directory and build gvimext.dll (or get it from a binary
    archive).  Both 64- and 32-bit versions are needed and should be placed
    as follows:
	64-bit: src/GvimExt/gvimext64.dll
	32-bit: src/GvimExt/gvimext.dll

4.  Get a "diff.exe" program.  If you skip this the built-in diff will always
    be used (which is fine for most users).
    You can find one in previous Vim versions or in this archive:
	https://www.mediafire.com/file/9edk4g3xvfgzby0/diff4Vim.zip/file
    When will you have "diff.exe" put it in the "../.." directory (above the
    "vim91" directory, it's the same for all Vim versions).  However, you can
    specify another directory by passing /DVIMTOOLS=<dir> option to the
    "makensis.exe" program via the command line.

5.  For the terminal window to work in Vim, the library winpty is required.
    You can get it at the following url:
	https://github.com/rprichard/winpty/releases/download/0.4.3/winpty-0.4.3-msvc2015.zip
    For the 32-bit version, rename "winpty.dll" from ia32/bin to "winpty32.dll",
    and for the 64-bit version — "winpty.dll" from x64/bin to "winpty64.dll".
    Put the renamed file and "winpty-agent.exe" in "../.." (above the "vim91"
    directory).  However, you can specify another directory by passing
    /DVIMTOOLS=<dir> option to the "makensis.exe" program via the command line.

6.  To use stronger encryption, add the Sodium library.  You can get it here:
	https://github.com/jedisct1/libsodium/releases/download/1.0.19-RELEASE/libsodium-1.0.19-msvc.zip
    Unpack the archive.  Put the "libsodium.dll" from
    path/to/libsodium/Win32/Release/v143/dynamic for the 32‐bit version or
    path/to/libsodium/X64/Release/v143/dynamic for the 64‐bit version in the
    "../.." directory (above the "vim91" directory, where "diff.exe" and
    "winpty{32|64}.dll").

7.  On MS Windows do "nmake.exe -f Make_mvc.mak uganda.nsis.txt" in runtime/doc.
    On Unix-like system do "make runtime/doc/uganda.nsis.txt" in top directory
    or "make uganda.nsis.txt" in runtime/doc.  The created files
    "uganda.nsis.???" will be automatically converted to DOS file format.

8.  Get gettext and iconv DLLs from the following site:
	https://github.com/mlocati/gettext-iconv-windows/releases
    Both 64- and 32-bit versions are needed.
    Download the files gettextX.X.X.X-iconvX.XX-shared-{32,64}.zip, extract
    DLLs and place them as follows:

	<GETTEXT directory>
	    |
	    + gettext32/
	    |	libintl-8.dll
	    |	libiconv-2.dll
	    |	libgcc_s_sjlj-1.dll
	    |
	    + gettext64/
		libintl-8.dll
		libiconv-2.dll

    The default <GETTEXT directory> is "..", however, you can specify another
    directory by passing /DGETTEXT=<dir> option to "makensis.exe" program via
    the command line.


Install NSIS if you didn't do that already.
Download Unicode version the ShellExecAsUser plug-in for NSIS from:
	https://nsis.sourceforge.io/ShellExecAsUser_plug-in
and put ShellExecAsUser.dll to path\to\NSIS\Plugins\x86-unicode


Unpack the images:
	cd nsis
	unzip icons.zip or 7z x icons.zip (on Unix-like or MS Windows)
	WinRar.exe x icons.zip (on MS Windows)

Then build gvim.exe:
	cd nsis
	makensis.exe [options] gvim.nsi

Options (not mandatory):
    /DVIMSRC=<dir>	— directory where location of gvim_ole.exe, vimw32.exe,
			    GvimExt/*, etc.
    /DVIMRT=<dir>	— directory where location of runtime files
    /DVIMTOOLS=<dir>    — directory where location of extra tools: diff.exe,
			    winpty{32|64}.dll, winpty-agent.exe, libsodium.dll
    /DGETTEXT=<dir>     — directory where location of gettext libraries
    /DHAVE_UPX=1	— additional compression of the installer.  UPX program
			    must be installed.
    /DHAVE_NLS=0	— do not add native language support
    /DHAVE_MULTI_LANG=0 — to create an English-only the installer
    /DWIN64=1		— to create a 64-bit the installer
