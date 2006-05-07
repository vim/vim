README_os2.txt for version 7.0 of Vim: Vi IMproved.

This file explains the installation of Vim on OS/2 systems.
See "README.txt" for general information about Vim.


NOTE: You will need two archives:
  vim70rt.zip	contains the runtime files (same as for the PC version)
  vim70os2.zip	contains the OS/2 executables

1. Go to the directory where you want to put the Vim files.  Examples:
	cd C:\
	cd D:\editors

2. Unpack the zip archives.  This will create a new directory "vim/vim70",
   in which all the distributed Vim files are placed.  Since the directory
   name includes the version number, it is unlikely that you overwrite
   existing files.
   Examples:
	pkunzip -d vim70os2.zip
	unzip vim70os2.zip

   After you unpacked the files, you can still move the whole directory tree
   to another location.

3. Add the directory where vim.exe is to your path.  The simplest is to add a
   line to your autoexec.bat.  Examples:
	set path=%path%;C:\vim\vim70
	set path=%path%;D:\editors\vim\vim70

That's it!


Extra remarks:

- To avoid confusion between distributed files of different versions and your
  own modified vim scripts, it is recommended to use this directory layout:
  ("C:\vim" is used here as the root, replace with the path you use)
  Your own files:
	C:\vim\_vimrc			Your personal vimrc.
	C:\vim\_viminfo			Dynamic info for 'viminfo'.
	C:\vim\...			Other files you made.
  Distributed files:
	C:\vim\vim70\vim.exe		The Vim version 7.0 executable.
	C:\vim\vim70\doc\*.txt		The version 7.0 documentation files.
	C:\vim\vim70\bugreport.vim	A Vim version 7.0 script.
	C:\vim\vim70\...		Other version 7.0 distributed files.
  In this case the $VIM environment variable would be set like this:
	set VIM=C:\vim

- You can put your Vim executable anywhere else.  If the executable is not
  with the other distributed Vim files, you should set $VIM.  The simplest is
  to add a line to your autoexec.bat.  Examples:
	set VIM=c:\vim
	set VIM=d:\editors\vim

For further information, type this inside Vim:
	:help os2
