![Vim Logo](https://github.com/vim/vim/blob/master/runtime/vimlogo.gif)

[![Build Status](https://travis-ci.org/vim/vim.svg?branch=master)](https://travis-ci.org/vim/vim)
[![Coverage Status](https://codecov.io/gh/vim/vim/coverage.svg?branch=master)](https://codecov.io/gh/vim/vim?branch=master)
[![Coverage Status](https://coveralls.io/repos/vim/vim/badge.svg?branch=master&service=github)](https://coveralls.io/github/vim/vim?branch=master)
[![Appveyor Build status](https://ci.appveyor.com/api/projects/status/o2qht2kjm02sgghk?svg=true)](https://ci.appveyor.com/project/chrisbra/vim)
[![Coverity Scan](https://scan.coverity.com/projects/241/badge.svg)](https://scan.coverity.com/projects/vim)
[![Debian CI](https://badges.debian.net/badges/debian/testing/vim/version.svg)](https://buildd.debian.org/vim)


## What is Vim? ##

Vim is a greatly improved version of the good old UNIX editor Vi.  Many new
features have been added: multi-level undo, syntax highlighting, command line
history, on-line help, spell checking, filename completion, block operations,
script language, etc.  There is also a Graphical User Interface (GUI)
available.  Still, Vi compatibility is maintained, those who have Vi "in the
fingers" will feel at home.  See `runtime/doc/vi_diff.txt` for differences with
Vi.

This editor is very useful for editing programs and other plain text files.
All commands are given with normal keyboard characters, so those who can type
with ten fingers can work very fast.  Additionally, function keys can be
mapped to commands by the user, and the mouse can be used.

Vim runs under MS-Windows (NT, 2000, XP, Vista, 7, 8, 10), Macintosh, VMS and
almost all flavours of UNIX.  Porting to other systems should not be very
difficult.  Older versions of Vim run on MS-DOS, MS-Windows 95/98/Me, Amiga
DOS, Atari MiNT, BeOS, RISC OS and OS/2.  These are no longer maintained.


## Distribution ##

You can often use your favorite package manager to install Vim.  On Mac and
Linux a small version of Vim is pre-installed, you still need to install Vim
if you want more features.

There are separate distributions for Unix, PC, Amiga and some other systems.
This `README.md` file comes with the runtime archive.  It includes the
documentation, syntax files and other files that are used at runtime.  To run
Vim you must get either one of the binary archives or a source archive.
Which one you need depends on the system you want to run it on and whether you
want or must compile it yourself.  Check http://www.vim.org/download.php for
an overview of currently available distributions.

Some popular places to get the latest Vim:
* Check out the git repository from [github](https://github.com/vim/vim).
* Get the source code as an [archive](https://github.com/vim/vim/releases).
* Get a Windows executable from the
[vim-win32-installer](https://github.com/vim/vim-win32-installer/releases) repository.



## Compiling ##

If you obtained a binary distribution you don't need to compile Vim.  If you
obtained a source distribution, all the stuff for compiling Vim is in the
`src` directory.  See `src/INSTALL` for instructions.


## Installation ##

See one of these files for system-specific instructions.  Either in the
READMEdir directory (in the repository) or the top directory (if you unpack an
archive):

	README_ami.txt		Amiga
	README_unix.txt		Unix
	README_dos.txt		MS-DOS and MS-Windows
	README_mac.txt		Macintosh
	README_vms.txt		VMS

There are other `README_*.txt` files, depending on the distribution you used.


## Documentation ##

The Vim tutor is a one hour training course for beginners.  Often it can be
started as `vimtutor`.  See `:help tutor` for more information.

The best is to use `:help` in Vim.  If you don't have an executable yet, read
`runtime/doc/help.txt`.  It contains pointers to the other documentation
files.  The User Manual reads like a book and is recommended to learn to use
Vim.  See `:help user-manual`.


## Copying ##

Vim is Charityware.  You can use and copy it as much as you like, but you are
encouraged to make a donation to help orphans in Uganda.  Please read the file
`runtime/doc/uganda.txt` for details (do `:help uganda` inside Vim).

Summary of the license: There are no restrictions on using or distributing an
unmodified copy of Vim.  Parts of Vim may also be distributed, but the license
text must always be included.  For modified versions a few restrictions apply.
The license is GPL compatible, you may compile Vim with GPL libraries and
distribute it.


## Sponsoring ##

Fixing bugs and adding new features takes a lot of time and effort.  To show
your appreciation for the work and motivate Bram and others to continue
working on Vim please send a donation.

Since Bram is back to a paid job the money will now be used to help children
in Uganda.  See `runtime/doc/uganda.txt`.  But at the same time donations
increase Bram's motivation to keep working on Vim!

For the most recent information about sponsoring look on the Vim web site:
	http://www.vim.org/sponsor/


## Contributing ##

If you would like to help making Vim better, see the [CONTRIBUTING.md](https://github.com/vim/vim/blob/master/CONTRIBUTING.md) file.


## Information ##

The latest news about Vim can be found on the Vim home page:
	http://www.vim.org/

If you have problems, have a look at the Vim documentation or tips:
	http://www.vim.org/docs.php
	http://vim.wikia.com/wiki/Vim_Tips_Wiki

If you still have problems or any other questions, use one of the mailing
lists to discuss them with Vim users and developers:
	http://www.vim.org/maillist.php

If nothing else works, report bugs directly:
	Bram Moolenaar <Bram@vim.org>


## Main author ##

Send any other comments, patches, flowers and suggestions to:
	Bram Moolenaar <Bram@vim.org>


This is `README.md` for version 8.1 of Vim: Vi IMproved.
