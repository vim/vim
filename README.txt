README.txt for version 7.2 of Vim: Vi IMproved.


WHAT IS VIM

Vim is an almost compatible version of the UNIX editor Vi.  Many new features
have been added: multi-level undo, syntax highlighting, command line history,
on-line help, spell checking, filename completion, block operations, etc.
There is also a Graphical User Interface (GUI) available.  See
"runtime/doc/vi_diff.txt" for differences with Vi.

This editor is very useful for editing programs and other plain ASCII files.
All commands are given with normal keyboard characters, so those who can type
with ten fingers can work very fast.  Additionally, function keys can be
defined by the user, and the mouse can be used.

Vim currently runs under Amiga DOS, MS-DOS, MS-Windows 95/98/Me/NT/2000/XP,
Atari MiNT, Macintosh, BeOS, VMS, RISC OS, OS/2 and almost all flavours of
UNIX.  Porting to other systems should not be very difficult.


DISTRIBUTION

There are separate distributions for Unix, PC, Amiga and some other systems.
This README.txt file comes with the runtime archive.  It includes the
documentation, syntax files and other files that are used at runtime.  To run
Vim you must get either one of the binary archives or a source archive.
Which one you need depends on the system you want to run it on and whether you
want or must compile it yourself.  Check "http://www.vim.org/download.php" for
an overview of currently available distributions.


DOCUMENTATION

The best is to use ":help" in Vim.  If you don't have an executable yet, read
"runtime/doc/help.txt".  It contains pointers to the other documentation
files.  The User Manual reads like a book and is recommended to learn to use
Vim.  See ":help user-manual".

The vim tutor is a one hour training course for beginners.  Mostly it can be
started as "vimtutor".  See ":help tutor" for more information.


COPYING

Vim is Charityware.  You can use and copy it as much as you like, but you are
encouraged to make a donation to orphans in Uganda.  Please read the file
"runtime/doc/uganda.txt" for details (do ":help uganda" inside Vim).

Summary of the license: There are no restrictions on using or distributing an
unmodified copy of Vim.  Parts of Vim may also be distributed, but the license
text must always be included.  For modified versions a few restrictions apply.
The license is GPL compatible, you may compile Vim with GPL libraries and
distribute it.


SPONSORING

Fixing bugs and adding new features takes a lot of time and effort.  To show
your appreciation for the work and motivate Bram and others to continue
working on Vim please send a donation.

Since Bram is back to a paid job the money will now be used to help children
in Uganda.  See runtime/doc/uganda.txt.  But at the same time donations
increase Bram's motivation to keep working on Vim!

For the most recent information about sponsoring look on the Vim web site:

	http://www.vim.org/sponsor/


COMPILING

If you obtained a binary distribution you don't need to compile Vim.  If you
obtained a source distribution, all the stuff for compiling Vim is in the
"src" directory.  See src/INSTALL for instructions.


INSTALLATION

See one of these files for system-specific instructions:
README_ami.txt		Amiga
README_unix.txt		Unix
README_dos.txt		MS-DOS and MS-Windows
README_os2.txt		OS/2
README_mac.txt		Macintosh
README_vms.txt		VMS


INFORMATION

The latest news about Vim can be found on the Vim home page:
	http://www.vim.org/

If you have problems, have a look at the Vim FAQ:
	http://vimdoc.sf.net/vimfaq.html

Send bug reports to:
	Bram Moolenaar <Bram@vim.org>

There are five mailing lists for Vim:
<vim@vim.org>
	For discussions about using existing versions of Vim: Useful mappings,
	questions, answers, where to get a specific version, etc.
<vim-dev@vim.org>
	For discussions about changing Vim: New features, porting, beta-test
	versions, etc.
<vim-announce@vim.org>
	Announcements about new versions of Vim; also beta-test versions and
	ports to different systems.
<vim-multibyte@vim.org>
	For discussions about using and improving the multi-byte aspects of
	Vim: XIM, Hangul, fontset, etc.
<vim-mac@vim.org>
	For discussions about using and improving Vim on the Macintosh.

For more info and URLs of the archives see "http://www.vim.org/maillist.php".

NOTE:
- You can only send messages to these lists if you have subscribed!
- You need to send the messages from the same location as where you subscribed
  from (to avoid spam mail).
- Maximum message size is 40000 characters.

If you want to join a maillist, send a message to
	<vim-help@vim.org>
Make sure that your "From:" address is correct.  Then the list server will
send you a help message.


MAIN AUTHOR

Send any other comments, patches, pizza and suggestions to:

	Bram Moolenaar		E-mail:	Bram@vim.org
	Finsterruetihof 1
	8134 Adliswil
	Switzerland
