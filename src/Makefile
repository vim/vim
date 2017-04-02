# Makefile for Vim on Unix and Unix-like systems	vim:ts=8:sw=8:tw=78
#
# This Makefile is loosely based on the GNU Makefile conventions found in
# standards.info.
#
# Compiling Vim, summary:
#
#	3. make
#	5. make install
#
# Compiling Vim, details:
#
# Edit this file for adjusting to your system. You should not need to edit any
# other file for machine specific things!
# The name of this file MUST be Makefile (note the uppercase 'M').
#
# 1. Edit this Makefile  {{{1
#	The defaults for Vim should work on most machines, but you may want to
#	uncomment some lines or make other changes below to tune it to your
#	system, compiler or preferences.  Uncommenting means that the '#' in
#	the first column of a line is removed.
#	- If you want a version of Vim that is small and starts up quickly,
#	  you might want to disable the GUI, X11, Perl, Python and Tcl.
#	- Uncomment the line with --disable-gui if you have Motif, GTK and/or
#	  Athena but don't want to make gvim (the GUI version of Vim with nice
#	  menus and scrollbars, but makes Vim bigger and startup slower).
#	- Uncomment --disable-darwin if on Mac OS X but you want to compile a
#	  Unix version.
#	- Uncomment the line "CONF_OPT_X = --without-x" if you have X11 but
#	  want to disable using X11 libraries.	This speeds up starting Vim,
#	  but the window title will not be set and the X11 selection can not
#	  be used.
#	- Uncomment the line "CONF_OPT_XSMP = --disable-xsmp" if you have the
#	  X11 Session Management Protocol (XSMP) library (libSM) but do not
#	  want to use it.
#	  This can speedup Vim startup but Vim loses the ability to catch the
#	  user logging out from session-managers like GNOME and work
#	  could be lost.
#	- Uncomment one or more of these lines to include an interface;
#	  each makes Vim quite a bit bigger:
#		--enable-luainterp	for Lua interpreter
#		--enable-mzschemeinterp	for MzScheme interpreter
#		--enable-perlinterp	for Perl interpreter
#		--enable-python3interp	for Python3 interpreter
#		--enable-pythoninterp	for Python interpreter
#		--enable-rubyinterp	for Ruby interpreter
#		--enable-tclinterp	for Tcl interpreter
#		--enable-cscope		for Cscope interface
#	- Uncomment one of the lines with --with-features= to enable a set of
#	  features (but not the interfaces just mentioned).
#	- Uncomment the line with --disable-acl to disable ACL support even
#	  though your system supports it.
#	- Uncomment the line with --disable-gpm to disable gpm support
#	  even though you have gpm libraries and includes.
#	- Uncomment the line with --disable-sysmouse to disable sysmouse
#	  support even though you have /dev/sysmouse and includes.
#	- Uncomment one of the lines with CFLAGS and/or CC if you have
#	  something very special or want to tune the optimizer.
#	- Search for the name of your system to see if it needs anything
#	  special.
#	- A few versions of make use '.include "file"' instead of 'include
#	  file'.  Adjust the include line below if yours does.
#
# 2. Edit feature.h  {{{1
#	Only if you do not agree with the default compile features, e.g.:
#	- you want Vim to be as vi compatible as it can be
#	- you want to use Emacs tags files
#	- you want right-to-left editing (Hebrew)
#	- you want 'langmap' support (Greek)
#	- you want to remove features to make Vim smaller
#
# 3. "make"  {{{1
#	Will first run ./configure with the options in this file. Then it will
#	start make again on this Makefile to do the compiling. You can also do
#	this in two steps with:
#		make config
#		make
#	The configuration phase creates/overwrites auto/config.h and
#	auto/config.mk.
#	The configure script is created with "make autoconf".  It can detect
#	different features of your system and act accordingly.  However, it is
#	not correct for all systems.  Check this:
#	- If you have X windows, but configure could not find it or reported
#	  another include/library directory then you wanted to use, you have
#	  to set CONF_OPT_X below.  You might also check the installation of
#	  xmkmf.
#	- If you have --enable-gui=motif and have Motif on your system, but
#	  configure reports "checking for location of gui... <not found>", you
#	  have to set GUI_INC_LOC and GUI_LIB_LOC below.
#	If you changed something, do this to run configure again:
#		make reconfig
#
#	- If you do not trust the automatic configuration code, then inspect
#	  auto/config.h and auto/config.mk, before starting the actual build
#	  phase. If possible edit this Makefile, rather than auto/config.mk --
#	  especially look at the definition of VIMLOC below. Note that the
#	  configure phase overwrites auto/config.mk and auto/config.h again.
#	- If you get error messages, find out what is wrong and try to correct
#	  it in this Makefile. You may need to do "make reconfig" when you
#	  change anything that configure uses (e.g. switching from an old C
#	  compiler to an ANSI C compiler). Only when auto/configure does
#	  something wrong you may need to change one of the other files. If
#	  you find a clean way to fix the problem, consider sending a note to
#	  the author of autoconf (bug-gnu-utils@prep.ai.mit.edu) or Vim
#	  (Bram@vim.org). Don't bother to do that when you made a hack
#	  solution for a non-standard system.
#
# 4. "make test"  {{{1
#	This is optional.  This will run Vim scripts on a number of test
#	files, and compare the produced output with the expected output.
#	If all is well, you will get the "ALL DONE" message in the end.  If a
#	test fails you get "TEST FAILURE".  See below (search for "/^test").
#
# 5. "make install"  {{{1
#	If the new Vim seems to be working OK you can install it and the
#	documentation in the appropriate location. The default is
#	"/usr/local".  Change "prefix" below to change the location.
#	"auto/pathdef.c" will be compiled again after changing this to make
#	the executable know where the help files are located.
#	Note that any existing executable is removed or overwritten.  If you
#	want to keep it you will have to make a backup copy first.
#	The runtime files are in a different directory for each version.  You
#	might want to delete an older version.
#	If you don't want to install everything, there are other targets:
#		make installvim		only installs Vim, not the tools
#		make installvimbin	only installs the Vim executable
#		make installruntime	installs most of the runtime files
#		make installrtbase	only installs the Vim help and
#							runtime files
#		make installlinks	only installs the Vim binary links
#		make installmanlinks	only installs the Vim manpage links
#		make installmacros	only installs the Vim macros
#		make installpack	only installs the packages
#		make installtutorbin	only installs the Vim tutor program
#		make installtutor	only installs the Vim tutor files
#		make installspell	only installs the spell files
#		make installtools	only installs xxd
#	If you install Vim, not to install for real but to prepare a package
#	or RPM, set DESTDIR to the root of the tree.
#
# 6. Use Vim until a new version comes out.  {{{1
#
# 7. "make uninstall_runtime"  {{{1
#	Will remove the runtime files for the current version.	This is safe
#	to use while another version is being used, only version-specific
#	files will be deleted.
#	To remove the runtime files of another version:
#		make uninstall_runtime VIMRTDIR=/vim54
#	If you want to delete all installed files, use:
#		make uninstall
#	Note that this will delete files that have the same name for any
#	version, thus you might need to do a "make install" soon after this.
#	Be careful not to remove a version of Vim that is still being used!
#	To find out which files and directories will be deleted, use:
#		make -n uninstall
# }}}
#
### This Makefile has been successfully tested on many systems. {{{
### Only the ones that require special options are mentioned here.
### Check the (*) column for remarks, listed below.
### Later code changes may cause small problems, otherwise Vim is supposed to
### compile and run without problems.

#system:	      configurations:		     version (*) tested by:
#-------------	      ------------------------	     -------  -  ----------
#AIX 3.2.5	      cc (not gcc)   -			4.5  (M) Will Fiveash
#AIX 4		      cc	     +X11 -GUI		3.27 (4) Axel Kielhorn
#AIX 4.1.4	      cc	     +X11 +GUI		4.5  (5) Nico Bakker
#AIX 4.2.1	      cc				5.2k (C) Will Fiveash
#AIX 4.3.3.12	      xic 3.6.6				5.6  (5) David R. Favor
#A/UX 3.1.1	      gcc	     +X11		4.0  (6) Jim Jagielski
#BeOS PR	      mwcc DR3				5.0n (T) Olaf Seibert
#BSDI 2.1 (x86)       shlicc2 gcc-2.6.3 -X11 X11R6	4.5  (1) Jos Backus
#BSD/OS 3.0 (x86)     gcc gcc-2.7.2.1 -X11 X11R6	4.6c (1) Jos Backus
#CX/UX 6.2	      cc	     +X11 +GUI_Mofif	5.4  (V) Kipp E. Howard
#DG/UX 5.4*	      gcc 2.5.8      GUI		5.0e (H) Jonas Schlein
#DG/UX 5.4R4.20       gcc 2.7.2      GUI		5.0s (H) Rocky Olive
#HP-UX (most)	      c89 cc				5.1  (2) Bram Moolenaar
#HP-UX_9.04	      cc	     +X11 +Motif	5.0  (2) Carton Lao
#Irix 6.3 (O2)	      cc	     ?			4.5  (L) Edouard Poor
#Irix 6.4	      cc	     ?			5.0m (S) Rick Sayre
#Irix 6.5	      cc	     ?			6.0  (S) David Harrison
#Irix 64 bit						4.5  (K) Jon Wright
#Linux 2.0	      gcc-2.7.2      Infomagic Motif	4.3  (3) Ronald Rietman
#Linux 2.0.31	      gcc	     +X11 +GUI Athena	5.0w (U) Darren Hiebert
#LynxOS 3.0.1	      2.9-gnupro-98r2 +X11 +GUI Athena  5.7.1(O) Lorenz Hahn
#LynxOS 3.1.0	      2.9-gnupro-98r2 +X11 +GUI Athena  5.7.1(O) Lorenz Hahn
#NEC UP4800 UNIX_SV 4.2MP  cc	     +X11R6 Motif,Athena4.6b (Q) Lennart Schultz
#NetBSD 1.0A	      gcc-2.4.5      -X11 -GUI		3.21 (X) Juergen Weigert
#QNX 4.2	      wcc386-10.6    -X11		4.2  (D) G.F. Desrochers
#QNX 4.23	      Watcom	     -X11		4.2  (F) John Oleynick
#SCO Unix v3.2.5      cc	     +X11 Motif		3.27 (C) M. Kuperblum
#SCO Open Server 5    gcc 2.7.2.3    +X11 +GUI Motif	5.3  (A) Glauber Ribeiro
#SINIX-N 5.43 RM400 R4000   cc	     +X11 +GUI		5.0l (I) Martin Furter
#SINIX-Z 5.42 i386    gcc 2.7.2.3    +X11 +GUI Motif	5.1  (I) Joachim Fehn
#SINIX-Y 5.43 RM600 R4000  gcc 2.7.2.3 +X11 +GUI Motif	5.1  (I) Joachim Fehn
#Reliant/SINIX 5.44   cc	     +X11 +GUI		5.5a (I) B. Pruemmer
#SNI Targon31 TOS 4.1.11 gcc-2.4.5   +X11 -GUI		4.6c (B) Paul Slootman
#Solaris 2.4 (Sparc)  cc	     +X11 +GUI		3.29 (9) Glauber
#Solaris 2.4/2.5      clcc	     +X11 -GUI openwin	3.20 (7) Robert Colon
#Solaris 2.5 (sun4m)  cc (SC4.0)     +X11R6 +GUI (CDE)	4.6b (E) Andrew Large
#Solaris 2.5	      cc	     +X11 +GUI Athena	4.2  (9) Sonia Heimann
#Solaris 2.5	      gcc 2.5.6      +X11 Motif		5.0m (R) Ant. Colombo
#Solaris 2.6	      gcc 2.8.1      ncurses		5.3  (G) Larry W. Virden
#Solaris with -lthread					5.5  (W) K. Nagano
#Solaris	      gcc				     (b) Riccardo
#SunOS 4.1.x			     +X11 -GUI		5.1b (J) Bram Moolenaar
#SunOS 4.1.3_U1 (sun4c) gcc	     +X11 +GUI Athena	5.0w (J) Darren Hiebert
#SUPER-UX 6.2 (NEC SX-4) cc	     +X11R6 Motif,Athena4.6b (P) Lennart Schultz
#Tandem/NSK                                                  (c) Matthew Woehlke
#Unisys 6035	      cc	     +X11 Motif		5.3  (8) Glauber Ribeiro
#ESIX V4.2	      cc	     +X11		6.0  (a) Reinhard Wobst
#Mac OS X 10.[23]     gcc	     Carbon		6.2  (x) Bram Moolenaar
# }}}

# (*)  Remarks: {{{
#
# (1)  Uncomment line below for shlicc2
# (2)  HPUX with compile problems or wrong digraphs, uncomment line below
# (3)  Infomagic Motif needs GUI_LIB_LOC and GUI_INC_LOC set, see below.
#      And add "-lXpm" to MOTIF_LIBS2.
# (4)  For cc the optimizer must be disabled (use CFLAGS= after running
#      configure) (symptom: ":set termcap" output looks weird).
# (5)  Compiler may need extra argument, see below.
# (6)  See below for a few lines to uncomment
# (7)  See below for lines which enable the use of clcc
# (8)  Needs some EXTRA_LIBS, search for Unisys below
# (9)  Needs an extra compiler flag to compile gui_at_sb.c, see below.
# (A)  May need EXTRA_LIBS, see below
# (B)  Can't compile GUI because there is no waitpid()...  Disable GUI below.
# (C)  Force the use of curses instead of termcap, see below.
# (D)  Uncomment lines below for QNX
# (E)  You might want to use termlib instead of termcap, see below.
# (F)  See below for instructions.
# (G)  Using ncurses version 4.2 has reported to cause a crash.  Use the
#      Sun curses library instead.
# (H)  See line for EXTRA_LIBS below.
# (I)  SINIX-N 5.42 and 5.43 need some EXTRA_LIBS.  Also for Reliant-Unix.
# (J)  If you get undefined symbols, see below for a solution.
# (K)  See lines to uncomment below for machines with 64 bit pointers.
# (L)  For Silicon Graphics O2 workstations remove "-lnsl" from auto/config.mk
# (M)  gcc version cygnus-2.0.1 does NOT work (symptom: "dl" deletes two
#      characters instead of one).
# (N)  SCO with decmouse.
# (O)  LynxOS needs EXTRA_LIBS, see below.
# (P)  For SuperUX 6.2 on NEC SX-4 see a few lines below to uncomment.
# (Q)  For UNIXSVR 4.2MP on NEC UP4800 see below for lines to uncomment.
# (R)  For Solaris 2.5 (or 2.5.1) with gcc > 2.5.6, uncomment line below.
# (S)  For Irix 6.x with MipsPro compiler, use -OPT:Olimit.  See line below.
# (T)  See ../doc/os_beos.txt.
# (U)  Must uncomment CONF_OPT_PYTHON option below to disable Python
#      detection, since the configure script runs into an error when it
#      detects Python (probably because of the bash shell).
# (V)  See lines to uncomment below.
# (X)  Need to use the .include "auto/config.mk" line below
# (Y)  See line with c89 below
# (Z)  See lines with cc or c89 below
# (a)  See line with EXTRA_LIBS below.
# (b)  When using gcc with the Solaris linker, make sure you don't use GNU
#      strip, otherwise the binary may not run: "Cannot find ELF".
# (c)  Add -lfloss to EXTRA_LIBS, see below.
# (x)  When you get warnings for precompiled header files, run
#      "sudo fixPrecomps".  Also see CONF_OPT_DARWIN below.
# }}}


#DO NOT CHANGE the next line, we need it for configure to find the compiler
#instead of using the default from the "make" program.
#Use a line further down to change the value for CC.
CC=

# Change and use these defines if configure cannot find your Motif stuff.
# Unfortunately there is no "standard" location for Motif. {{{
# These defines can contain a single directory (recommended) or a list of
# directories (for when you are working with several systems). The LAST
# directory that exists is used.
# When changed, run "make reconfig" next!
#GUI_INC_LOC = -I/usr/include/Motif2.0 -I/usr/include/Motif1.2
#GUI_LIB_LOC = -L/usr/lib/Motif2.0 -L/usr/lib/Motif1.2
### Use these two lines for Infomagic Motif (3)
#GUI_INC_LOC = -I/usr/X11R6/include
#GUI_LIB_LOC = -L/usr/X11R6/lib
# }}}

# Defaults used when auto/config.mk does not exist.
srcdir = .
VIMNAME = vim
EXNAME = ex
VIEWNAME = view

######################## auto/config.mk ######################## {{{1
# At this position auto/config.mk is included. When starting from the
# toplevel Makefile it is almost empty. After running auto/configure it
# contains settings that have been discovered for your system. Settings below
# this include override settings in auto/config.mk!

# Note: If make fails because auto/config.mk does not exist (it is not
# included in the repository), do:
#    cp config.mk.dist auto/config.mk

# (X) How to include auto/config.mk depends on the version of "make" you have,
#     if the current choice doesn't work, try the other one.

include auto/config.mk
#.include "auto/config.mk"
CClink = $(CC)

#}}}

# Include the configuration choices first, so we can override everything
# below. As shipped, this file contains a target that causes to run
# configure. Once configure was run, this file contains a list of
# make variables with predefined values instead. Thus any second invocation
# of make, will build Vim.

# CONFIGURE - configure arguments {{{1
# You can give a lot of options to configure.
# Change this to your desire and do 'make config' afterwards

# examples (can only use one!):
#CONF_ARGS = --exec-prefix=/usr
#CONF_ARGS = --with-vim-name=vim7 --with-ex-name=ex7 --with-view-name=view7
#CONF_ARGS = --with-global-runtime=/etc/vim
#CONF_ARGS = --with-local-dir=/usr/share
#CONF_ARGS = --without-local-dir

# Use this one if you distribute a modified version of Vim.
#CONF_ARGS = --with-modified-by="John Doe"

# GUI - For creating Vim with GUI (gvim) (B)
# Uncomment this line when you don't want to get the GUI version, although you
# have GTK, Motif and/or Athena.  Also use --without-x if you don't want X11
# at all.
#CONF_OPT_GUI = --disable-gui

# Uncomment one of these lines if you have that GUI but don't want to use it.
# The automatic check will use another one that can be found.
# Gnome is disabled by default, because it may cause trouble.
#
# When both GTK+ 2 and GTK+ 3 are possible then GTK+ 2 will be selected.
# To use GTK+ 3 instead use --enable-gui=gtk3 (see below).
#CONF_OPT_GUI = --disable-gtk2-check
#CONF_OPT_GUI = --enable-gnome2-check
#CONF_OPT_GUI = --disable-gtk3-check
#CONF_OPT_GUI = --disable-motif-check
#CONF_OPT_GUI = --disable-athena-check
#CONF_OPT_GUI = --disable-nextaw-check

# Uncomment one of these lines to select a specific GUI to use.
# When using "yes" or nothing, configure will use the first one found: GTK+,
# Motif or Athena.
#
# GTK versions that are known not to work 100% are rejected.
# Use "--disable-gtktest" to accept them anyway.
# For GTK 1 use Vim 7.2.
#
# GNOME means GTK with Gnome support.  If using GTK and --enable-gnome-check
# is used then GNOME will automatically be used if it is found.  If you have
# GNOME, but do not want to use it (e.g., want a GTK-only version), then use
# --enable-gui=gtk or leave out --enable-gnome-check.
#
# GNOME makes sense only for GTK+ 2.  Avoid use of --enable-gnome-check with
# GTK+ 3 build, as the functionality of GNOME is already incooperated into
# GTK+ 3.
#
# If the selected GUI isn't found, the GUI is disabled automatically
#CONF_OPT_GUI = --enable-gui=gtk2
#CONF_OPT_GUI = --enable-gui=gtk2 --disable-gtktest
#CONF_OPT_GUI = --enable-gui=gnome2
#CONF_OPT_GUI = --enable-gui=gnome2 --disable-gtktest
#CONF_OPT_GUI = --enable-gui=gtk3
#CONF_OPT_GUI = --enable-gui=gtk3 --disable-gtktest
#CONF_OPT_GUI = --enable-gui=motif
#CONF_OPT_GUI = --enable-gui=motif --with-motif-lib="-static -lXm -shared"
#CONF_OPT_GUI = --enable-gui=athena
#CONF_OPT_GUI = --enable-gui=nextaw

# Carbon GUI for Mac OS X
#CONF_OPT_GUI = --enable-gui=carbon

# Uncomment this line to run an individual test with gvim.
#GUI_TESTARG = GUI_FLAG=-g 

# DARWIN - detecting Mac OS X
# Uncomment this line when you want to compile a Unix version of Vim on
# Darwin.  None of the Mac specific options or files will be used.
#CONF_OPT_DARWIN = --disable-darwin

# Select the architecture supported.  Default is to build for the current
# platform.  Use "both" for a universal binary.  That probably doesn't work
# when including Perl, Python, etc.
#CONF_OPT_DARWIN = --with-mac-arch=i386
#CONF_OPT_DARWIN = --with-mac-arch=ppc
#CONF_OPT_DARWIN = --with-mac-arch=both

# Uncomment the next line to fail if one of the requested language interfaces
# cannot be configured.  Without this Vim will be build anyway, without
# the failing interfaces.
#CONF_OPT_FAIL = --enable-fail-if-missing

# LUA
# Uncomment one of these when you want to include the Lua interface.
# First one is for static linking, second one for dynamic loading.
# Use --with-luajit if you want to use LuaJIT instead of Lua.
# Set PATH environment variable to find lua or luajit executable.
#CONF_OPT_LUA = --enable-luainterp
#CONF_OPT_LUA = --enable-luainterp=dynamic
#CONF_OPT_LUA = --enable-luainterp --with-luajit
#CONF_OPT_LUA = --enable-luainterp=dynamic --with-luajit
# Lua installation dir (when not set uses $LUA_PREFIX or defaults to /usr)
#CONF_OPT_LUA_PREFIX = --with-lua-prefix=/usr/local

# MZSCHEME
# Uncomment this when you want to include the MzScheme interface.
# NOTE: does not work well together with valgrind.
#CONF_OPT_MZSCHEME = --enable-mzschemeinterp
# PLT/mrscheme/drscheme Home dir; the PLTHOME environment variable also works
#CONF_OPT_PLTHOME  = --with-plthome=/usr/local/plt
#CONF_OPT_PLTHOME  = --with-plthome=/usr/local/drscheme
#CONF_OPT_PLTHOME  = --with-plthome=/home/me/mz

# PERL
# Uncomment one of these when you want to include the Perl interface.
# First one is for static linking, second one for dynamic loading.
# The Perl option sometimes causes problems, because it adds extra flags
#
# to the command line.	If you see strange flags during compilation, check in
# auto/config.mk where they come from.  If it's PERL_CFLAGS, try commenting
# the next line.
# When you get an error for a missing "perl.exp" file, try creating an empty
# one: "touch perl.exp".
# This requires at least "small" features, "tiny" doesn't work.
#CONF_OPT_PERL = --enable-perlinterp
#CONF_OPT_PERL = --enable-perlinterp=dynamic

# PYTHON
# Uncomment this when you want to include the Python interface.
# Requires small features or better, fails with tiny features.
# NOTE: This may cause threading to be enabled, which has side effects (such
# as using different libraries and debugging becomes more difficult).
# NOTE: Using this together with Perl may cause a crash in initialization.
# For Python3 support make a symbolic link in /usr/local/bin:
#	ln -s python3 python3.1
# If both python2.x and python3.x are enabled then the linking will be via
# dlopen(), dlsym(), dlclose(), i.e. pythonX.Y.so must be available
# However, this may still cause problems, such as "import termios" failing.
# Build two separate versions of Vim in that case.
#CONF_OPT_PYTHON = --enable-pythoninterp
#CONF_OPT_PYTHON = --enable-pythoninterp=dynamic
#CONF_OPT_PYTHON3 = --enable-python3interp
#CONF_OPT_PYTHON3 = --enable-python3interp=dynamic

# RUBY
# Uncomment this when you want to include the Ruby interface.
# First one for static linking, second one for loading when used.
# Note: you need the development package (e.g., ruby1.9.1-dev on Ubuntu).
#CONF_OPT_RUBY = --enable-rubyinterp
#CONF_OPT_RUBY = --enable-rubyinterp=dynamic
#CONF_OPT_RUBY = --enable-rubyinterp --with-ruby-command=ruby1.9.1

# TCL
# Uncomment this when you want to include the Tcl interface.
# First one is for static linking, second one for dynamic loading.
#CONF_OPT_TCL = --enable-tclinterp
#CONF_OPT_TCL = --enable-tclinterp=dynamic
#CONF_OPT_TCL = --enable-tclinterp --with-tclsh=tclsh8.4

# CSCOPE
# Uncomment this when you want to include the Cscope interface.
#CONF_OPT_CSCOPE = --enable-cscope

# WORKSHOP - Sun Visual Workshop interface.  Only works with Motif!
#CONF_OPT_WORKSHOP = --enable-workshop

# NETBEANS - NetBeans interface. Only works with Motif, GTK, and gnome.
# Motif version must have XPM libraries (see |workshop-xpm|).
# Uncomment this when you do not want the netbeans interface.
#CONF_OPT_NETBEANS = --disable-netbeans

# CHANNEL - inter process communication. Same conditions as NetBeans.
# Uncomment this when you do not want inter process communication.
#CONF_OPT_CHANNEL = --disable-channel

# MULTIBYTE - To edit multi-byte characters.
# Uncomment this when you want to edit a multibyte language.
# It's automatically enabled with normal features, GTK or IME support.
# Note: Compile on a machine where setlocale() actually works, otherwise the
# configure tests may fail.
#CONF_OPT_MULTIBYTE = --enable-multibyte

# NLS - National Language Support
# Uncomment this when you do not want to support translated messages, even
# though configure can find support for it.
#CONF_OPT_NLS = --disable-nls

# XIM - X Input Method.  Special character input support for X11 (Chinese,
# Japanese, special symbols, etc).  Also needed for dead-key support.
# When omitted it's automatically enabled for the X-windows GUI.
# HANGUL - Input Hangul (Korean) language using internal routines.
# Uncomment one of these when you want to input a multibyte language.
#CONF_OPT_INPUT = --enable-xim
#CONF_OPT_INPUT = --disable-xim
#CONF_OPT_INPUT = --enable-hangulinput

# FONTSET - X fontset support for output of languages with many characters.
# Uncomment this when you want to output a multibyte language.
#CONF_OPT_OUTPUT = --enable-fontset

# ACL - Uncomment this when you do not want to include ACL support, even
# though your system does support it.  E.g., when it's buggy.
#CONF_OPT_ACL = --disable-acl

# gpm - For mouse support on Linux console via gpm
# Uncomment this when you do not want to include gpm support, even
# though you have gpm libraries and includes.
#CONF_OPT_GPM = --disable-gpm

# sysmouse - For mouse support on FreeBSD and DragonFly console via sysmouse
# Uncomment this when you do not want do include sysmouse support, even
# though you have /dev/sysmouse and includes.
#CONF_OPT_SYSMOUSE = --disable-sysmouse

# FEATURES - For creating Vim with more or less features
# Uncomment one of these lines when you want to include few to many features.
# The default is "huge" for most systems.
#CONF_OPT_FEAT = --with-features=tiny
#CONF_OPT_FEAT = --with-features=small
#CONF_OPT_FEAT = --with-features=normal
#CONF_OPT_FEAT = --with-features=big
#CONF_OPT_FEAT = --with-features=huge

# COMPILED BY - For including a specific e-mail address for ":version".
#CONF_OPT_COMPBY = "--with-compiledby=John Doe <JohnDoe@yahoo.com>"

# X WINDOWS DISABLE - For creating a plain Vim without any X11 related fancies
# (otherwise Vim configure will try to include xterm titlebar access)
# Also disable the GUI above, otherwise it will be included anyway.
# When both GUI and X11 have been disabled this may save about 15% of the
# code and make Vim startup quicker.
#CONF_OPT_X = --without-x

# X WINDOWS DIRECTORY - specify X directories
# If configure can't find you X stuff, or if you have multiple X11 derivatives
# installed, you may wish to specify which one to use.
# Select nothing to let configure choose.
# This here selects openwin (as found on sun).
#XROOT = /usr/openwin
#CONF_OPT_X = --x-include=$(XROOT)/include --x-libraries=$(XROOT)/lib

# X11 Session Management Protocol support
# Vim will try to use XSMP to catch the user logging out if there are unsaved
# files.  Uncomment this line to disable that (it prevents vim trying to open
# communications with the session manager).
#CONF_OPT_XSMP = --disable-xsmp

# You may wish to include xsmp but use exclude xsmp-interact if the logout
# XSMP functionality does not work well with your session-manager (at time of
# writing, this would be early GNOME-1 gnome-session: it 'freezes' other
# applications after Vim has cancelled a logout (until Vim quits).  This
# *might* be the Vim code, but is more likely a bug in early GNOME-1.
# This disables the dialog that asks you if you want to save files or not.
#CONF_OPT_XSMP = --disable-xsmp-interact

# COMPILER - Name of the compiler {{{1
# The default from configure will mostly be fine, no need to change this, just
# an example. If a compiler is defined here, configure will use it rather than
# probing for one. It is dangerous to change this after configure was run.
# Make will use your choice then -- but beware: Many things may change with
# another compiler.  It is wise to run 'make reconfig' to start all over
# again.
#CC = cc
#CC = gcc
#CC = clang

# COMPILER FLAGS - change as you please. Either before running {{{1
# configure or afterwards. For examples see below.
# When using -g with some older versions of Linux you might get a
# statically linked executable.
# When not defined, configure will try to use -O2 -g for gcc and -O for cc.
#CFLAGS = -g
#CFLAGS = -O

# Optimization limits - depends on the compiler.  Automatic check in configure
# doesn't work very well, because many compilers only give a warning for
# unrecognized arguments.
#CFLAGS = -O -OPT:Olimit=2600
#CFLAGS = -O -Olimit 2000
#CFLAGS = -O -FOlimit,2000

# Often used for GCC: mixed optimizing, lot of optimizing, debugging
#CFLAGS = -g -O2 -fno-strength-reduce -Wall -Wshadow -Wmissing-prototypes
#CFLAGS = -g -O2 -fno-strength-reduce -Wall -Wmissing-prototypes
#CFLAGS = -g -Wall -Wmissing-prototypes
#CFLAGS = -O6 -fno-strength-reduce -Wall -Wshadow -Wmissing-prototypes
#CFLAGS = -g -DDEBUG -Wall -Wshadow -Wmissing-prototypes
#CFLAGS = -g -O2 '-DSTARTUPTIME="vimstartup"' -fno-strength-reduce -Wall -Wmissing-prototypes

# Use this with GCC to check for mistakes, unused arguments, etc.
#CFLAGS = -g -Wall -Wextra -Wshadow -Wmissing-prototypes -Wunreachable-code -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=1
#CFLAGS = -g -O2 -Wall -Wextra -Wmissing-prototypes -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=1 -DU_DEBUG
#PYTHON_CFLAGS_EXTRA = -Wno-missing-field-initializers
#MZSCHEME_CFLAGS_EXTRA = -Wno-unreachable-code -Wno-unused-parameter

# EFENCE - Electric-Fence malloc debugging: catches memory accesses beyond
# allocated memory (and makes every malloc()/free() very slow).
# Electric Fence is free (search ftp sites).
# You may want to set the EF_PROTECT_BELOW environment variable to check the
# other side of allocated memory.
# On FreeBSD you might need to enlarge the number of mmaps allowed.  Do this
# as root: sysctl -w vm.max_proc_mmap=30000
#EXTRA_LIBS = /usr/local/lib/libefence.a

# Autoconf binary.
AUTOCONF = autoconf

# PURIFY - remove the # to use the "purify" program (hoi Nia++!)
#PURIFY = purify

# VALGRIND - remove the # to use valgrind for memory leaks and access errors.
#            Used for the unittest targets.
# VALGRIND = valgrind --tool=memcheck --leak-check=yes --num-callers=25 --log-file=valgrind.$@

# NBDEBUG - debugging the netbeans interface.
#EXTRA_DEFS = -DNBDEBUG

# }}}

# LINT - for running lint
#  For standard Unix lint
LINT = lint
LINT_OPTIONS = -beprxzF
#  For splint
#  It doesn't work well, crashes on include files and non-ascii characters.
#LINT = splint
#LINT_OPTIONS = +unixlib -weak -macrovarprefixexclude -showfunc -linelen 9999

# PROFILING - Uncomment the next two lines to do profiling with gcc and gprof.
# Might not work with GUI or Perl.
# For unknown reasons adding "-lc" fixes a linking problem with some versions
# of GCC.  That's probably a bug in the "-pg" implementation.
# After running Vim see the profile result with: gprof vim gmon.out | vim -
# Need to recompile everything after changing this: "make clean" "make".
#PROFILE_CFLAGS = -pg -g -DWE_ARE_PROFILING
#PROFILE_LIBS = -pg
#PROFILE_LIBS = -pg -lc


# TEST COVERAGE - Uncomment the two lines below the explanation to get code
# coverage information. (provided by Yegappan Lakshmanan)
# 1. make clean, run configure and build Vim as usual.
# 2. Generate the baseline code coverage information:
#        $ lcov -c -i -b . -d objects -o objects/coverage_base.info
# 3. Run "make test" to run the unit tests.  The code coverage information will
#    be generated in the src/objects directory.
# 4. Generate the code coverage information from the tests:
#        $ lcov -c -b . -d objects/ -o objects/coverage_test.info
# 5. Combine the baseline and test code coverage data:
#        $ lcov -a objects/coverage_base.info -a objects/coverage_test.info -o objects/coverage_total.info
# 6. Process the test coverage data and generate a report in html:
#        $ genhtml objects/coverage_total.info -o objects
# 7. Open the objects/index.html file in a web browser to view the coverage
#    information.
#
# PROFILE_CFLAGS=-g -O0 -fprofile-arcs -ftest-coverage
# LDFLAGS=--coverage


# Uncomment one of the next two lines to compile Vim with the
# address sanitizer or with the undefined sanitizer.  Works with gcc and
# clang.  May make Vim twice as slow.  Errors reported on stderr.
# More at: https://code.google.com/p/address-sanitizer/
#SANITIZER_CFLAGS = -g -O0 -fsanitize=address -fno-omit-frame-pointer
#SANITIZER_CFLAGS = -g -O0 -fsanitize=undefined -fno-omit-frame-pointer
SANITIZER_LIBS = $(SANITIZER_CFLAGS)

# MEMORY LEAK DETECTION
# Requires installing the ccmalloc library.
# Configuration is in the .ccmalloc or ~/.ccmalloc file.
# Doesn't work very well, since memory linked to from global variables
# (in libraries) is also marked as leaked memory.
#LEAK_CFLAGS = -DEXITFREE
#LEAK_LIBS = -lccmalloc

# Uncomment this line to have Vim call abort() when an internal error is
# detected.  Useful when using a tool to find errors.
#ABORT_CLFAGS = -DABORT_ON_INTERNAL_ERROR

#####################################################
###  Specific systems, check if yours is listed!  ### {{{
#####################################################

### Uncomment things here only if the values chosen by configure are wrong.
### It's better to adjust configure.ac and "make autoconf", if you can!
### Then send the required changes to configure.ac to the bugs list.

### (1) BSD/OS 2.0.1, 2.1 or 3.0 using shared libraries
###
#CC = shlicc2
#CFLAGS = -O2 -g -m486 -Wall -Wshadow -Wmissing-prototypes -fno-builtin

### (2) HP-UX with a non-ANSI cc, use the c89 ANSI compiler
###	The first probably works on all systems
###	The second should work a bit better on newer systems
###	The third should work a bit better on HPUX 11.11
###	Information provided by: Richard Allen <ra@rhi.hi.is>
#CC = c89 -D_HPUX_SOURCE
#CC = c89 -O +Onolimit +ESlit -D_HPUX_SOURCE
#CC = c89 -O +Onolimit +ESlit +e -D_HPUX_SOURCE

### (2) For HP-UX: Enable the use of a different set of digraphs.  Use this
###	when the default (ISO) digraphs look completely wrong.
###	After changing this do "touch digraph.c; make".
#EXTRA_DEFS = -DHPUX_DIGRAPHS

### (2) For HP-UX: 9.04 cpp default macro definition table of 128000 bytes
###	is too small to compile many routines.	It produces too much defining
###	and no space errors.
###	Uncomment the following to specify a larger macro definition table.
#CFLAGS = -Wp,-H256000

### (2) For HP-UX 10.20 using the HP cc, with X11R6 and Motif 1.2, with
###	libraries in /usr/lib instead of /lib (avoiding transition links).
###	Information provided by: David Green
#XROOT = /usr
#CONF_OPT_X = --x-include=$(XROOT)/include/X11R6 --x-libraries=$(XROOT)/lib/X11R6
#GUI_INC_LOC = -I/usr/include/Motif1.2
#GUI_LIB_LOC = -L/usr/lib/Motif1.2_R6

### (5) AIX 4.1.4 with cc
#CFLAGS = -O -qmaxmem=8192

###     AIX with c89 (Walter Briscoe)
#CC = c89
#CPPFLAGS = -D_ALL_SOURCE

###     AIX 4.3.3.12 with xic 3.6.6 (David R. Favor)
#       needed to avoid a problem where strings.h gets included
#CFLAGS = -qsrcmsg -O2 -qmaxmem=8192 -D__STR31__

### (W) Solaris with multi-threaded libraries (-lthread):
###	If suspending doesn't work properly, try using this line:
#EXTRA_DEFS = -D_REENTRANT

### (7) Solaris 2.4/2.5 with Centerline compiler
#CC = clcc
#X_LIBS_DIR = -L/usr/openwin/lib -R/usr/openwin/lib
#CFLAGS = -O

### (9) Solaris 2.x with cc (SunPro), using Athena.
###	Only required for compiling gui_at_sb.c.
###	Symptom: "identifier redeclared: vim_XawScrollbarSetThumb"
###	Use one of the lines (either Full ANSI or no ANSI at all)
#CFLAGS = $(CFLAGS) -Xc
#CFLAGS = $(CFLAGS) -Xs

### Solaris 2.3 with X11 and specific cc
#CC=/opt/SUNWspro/bin/cc -O -Xa -v -R/usr/openwin/lib

### Solaris with /usr/ucb/cc (it is rejected by autoconf as "cc")
#CC	    = /usr/ucb/cc
#EXTRA_LIBS = -R/usr/ucblib

### Solaris with Forte Developer and FEAT_SUN_WORKSHOP
# The Xpm library is available from http://koala.ilog.fr/ftp/pub/xpm.
#CC		= cc
#XPM_DIR		= /usr/local/xpm/xpm-3.4k-solaris
#XPM_LIB		= -L$(XPM_DIR)/lib -R$(XPM_DIR)/lib -lXpm
#XPM_IPATH	= -I$(XPM_DIR)/include
#EXTRA_LIBS	= $(XPM_LIB)
#EXTRA_IPATHS	= $(XPM_IPATH)
#EXTRA_DEFS	= -xCC -DHAVE_X11_XPM_H

### Solaris with workshop compilers: Vim is unstable when compiled with
# "-fast".  Use this instead. (Shea Martin)
#CFLAGS = -x02 -xtarget=ultra

### (R) for Solaris 2.5 (or 2.5.1) with gcc > 2.5.6 you might need this:
#LDFLAGS = -lw -ldl -lXmu
#GUI_LIB_LOC = -L/usr/local/lib

### (8) Unisys 6035 (Glauber Ribeiro)
#EXTRA_LIBS = -lnsl -lsocket -lgen

### When builtin functions cause problems with gcc (for Sun 4.1.x)
#CFLAGS = -O2 -Wall -traditional -Wno-implicit

### Apollo DOMAIN (with SYSTYPE = bsd4.3) (TESTED for version 3.0)
#EXTRA_DEFS = -DDOMAIN
#CFLAGS= -O -A systype,bsd4.3

### Coherent 4.2.10 on Intel 386 platform
#EXTRA_DEFS = -Dvoid=int
#EXTRA_LIBS = -lterm -lsocket

### SCO 3.2, with different library name for terminfo
#EXTRA_LIBS = -ltinfo

### UTS2 for Amdahl UTS 2.1.x
#EXTRA_DEFS = -DUTS2
#EXTRA_LIBS = -lsocket

### UTS4 for Amdahl UTS 4.x
#EXTRA_DEFS = -DUTS4 -Xa

### USL for Unix Systems Laboratories (SYSV 4.2)
#EXTRA_DEFS = -DUSL

### (6)  A/UX 3.1.1 with gcc (Jim Jagielski)
#CC= gcc -D_POSIX_SOURCE
#CFLAGS= -O2
#EXTRA_LIBS = -lposix -lbsd -ltermcap -lX11

### (A)  Some versions of SCO Open Server 5 (Jan Christiaan van Winkel)
###	 Also use the CONF_TERM_LIB below!
#EXTRA_LIBS = -lgen

### (D)  QNX (by G.F. Desrochers)
#CFLAGS = -g -O -mf -4

### (F)  QNX (by John Oleynick)
# 1. If you don't have an X server: Comment out CONF_OPT_GUI and uncomment
#    CONF_OPT_X = --without-x.
# 2. make config
# 3. edit auto/config.mk and remove -ldir and -ltermcap from LIBS.  It doesn't
#	have -ldir (does config find it somewhere?) and -ltermcap has at
#	least one problem so I use termlib.o instead.  The problem with
#	termcap is that it segfaults if you call it with the name of
#	a non-existent terminal type.
# 4. edit auto/config.h and add #define USE_TMPNAM
# 5. add termlib.o to OBJ
# 6. make

### (H)  for Data general DG/UX 5.4.2 and 5.4R3.10 (Jonas J. Schlein)
#EXTRA_LIBS = -lgen

### (I) SINIX-N 5.42 or 5.43 RM400 R4000 (also SINIX-Y and SINIX-Z)
#EXTRA_LIBS = -lgen -lnsl
###   For SINIX-Y this is needed for the right prototype of gettimeofday()
#EXTRA_DEFS = -D_XPG_IV

### (I) Reliant-Unix (aka SINIX) 5.44 with standard cc
#	Use both "-F O3" lines for optimization or the "-g" line for debugging
#EXTRA_LIBS = -lgen -lsocket -lnsl -lSM -lICE
#CFLAGS = -F O3 -DSINIXN
#LDFLAGS = -F O3
#CFLAGS = -g -DSINIXN

### (P)  SCO 3.2.42, with different termcap names for some useful keys DJB
#EXTRA_DEFS = -DSCOKEYS -DNETTERM_MOUSE -DDEC_MOUSE -DXTERM_MOUSE -DHAVE_GETTIMEOFDAY
#EXTRA_LIBS = -lsocket -ltermcap -lmalloc -lc_s

### (P)  SuperUX 6.2 on NEC SX-4 (Lennart Schultz)
#GUI_INC_LOC = -I/usr/include
#GUI_LIB_LOC = -L/usr/lib
#EXTRA_LIBS = -lgen

### (Q) UNIXSVR 4.2MP on NEC UP4800 (Lennart Schultz)
#GUI_INC_LOC = -I/usr/necccs/include
#GUI_LIB_LOC = -L/usr/necccs/lib/X11R6
#XROOT = /usr/necccs
#CONF_OPT_X = --x-include=$(XROOT)/include --x-libraries=$(XROOT)/lib/X11R6
#EXTRA_LIBS = -lsocket -lgen

### Irix 4.0 & 5.2 (Silicon Graphics Machines, __sgi will be defined)
# Not needed for Irix 5.3, Ives Aerts reported
#EXTRA_LIBS = -lmalloc -lc_s
# Irix 4.0, when regexp and regcmp cannot be found when linking:
#EXTRA_LIBS = -lmalloc -lc_s -lPW

### (S) Irix 6.x (MipsPro compiler): Uses different Olimit flag:
# Note:	This newer option style is used with the MipsPro compilers ONLY if
#	you are compiling an "n32" or "64" ABI binary (use either a -n32
#	flag or a -64 flag for CFLAGS).  If you explicitly use a -o32 flag,
#	then the CFLAGS option format will be the typical style (i.e.
#	-Olimit 3000).
#CFLAGS = -OPT:Olimit=3000 -O

### (S) Irix 6.5 with MipsPro C compiler.  Try this as a test to see new
#	compiler features!  Beware, the optimization is EXTREMELY thorough
#	and takes quite a long time.
# Note: See the note above.  Here, the -mips3 option automatically
#	enables either the "n32" or "64" ABI, depending on what machine you
#	are compiling on (n32 is explicitly enabled here, just to make sure).
#CFLAGS = -OPT:Olimit=3500 -O -n32 -mips3 -IPA:aggr_cprop=ON -INLINE:dfe=ON:list=ON:must=screen_char,out_char,ui_write,out_flush
#LDFLAGS= -OPT:Olimit=3500 -O -n32 -mips3 -IPA:aggr_cprop=ON -INLINE:dfe=ON:list=ON:must=screen_char,out_char,ui_write,out_flush

### (K) for SGI Irix machines with 64 bit pointers ("uname -s" says IRIX64)
###	Suggested by Jon Wright <jon@gate.sinica.edu.tw>.
###	Tested on R8000 IRIX6.1 Power Indigo2.
###	Check /etc/compiler.defaults for your compiler settings.
# either (for 64 bit pointers) uncomment the following line
#GUI_LIB_LOC = -L/usr/lib64
# then
# 1) make config
# 2) edit auto/config.mk and delete the -lelf entry in the LIBS line
# 3) make
#
# or (for 32bit pointers) uncomment the following line
#EXTRA_DEFS = -n32
#GUI_LIB_LOC = -L/usr/lib32
# then
# 1) make config
# 2) edit auto/config.mk, add -n32 to LDFLAGS
# 3) make
#
#Alternatively: use -o32 instead of -n32.
###

### (C)  On SCO Unix v3.2.5 (and probably other versions) the termcap library,
###	 which is found by configure, doesn't work correctly.  Symptom is the
###	 error message "Termcap entry too long".  Uncomment the next line.
###	 On AIX 4.2.1 (and other versions probably), libtermcap is reported
###	 not to display properly.
### after changing this, you need to do "make reconfig".
#CONF_TERM_LIB = --with-tlib=curses

### (E)  If you want to use termlib library instead of the automatically found
###	 one.  After changing this, you need to do "make reconfig".
#CONF_TERM_LIB = --with-tlib=termlib

### (a)  ESIX V4.2 (Reinhard Wobst)
#EXTRA_LIBS = -lnsl -lsocket -lgen -lXIM -lXmu -lXext

### (c)  Tandem/NSK (Matthew Woehlke)
#EXTRA_LIBS = -lfloss

### If you want to use ncurses library instead of the automatically found one
### after changing this, you need to do "make reconfig".
#CONF_TERM_LIB = --with-tlib=ncurses

### For GCC on MS-Windows, the ".exe" suffix will be added.
#EXEEXT = .exe
#LNKEXT = .exe

### (O)  For LynxOS 2.5.0, tested on PC.
#EXTRA_LIBS = -lXext -lSM -lICE -lbsd
###	 For LynxOS 3.0.1, tested on PPC
#EXTRA_LIBS= -lXext -lSM -lICE -lnetinet -lXmu -liberty -lX11
###	 For LynxOS 3.1.0, tested on PC
#EXTRA_LIBS= -lXext -lSM -lICE -lnetinet -lXmu


### (V)  For CX/UX 6.2	(on Harris/Concurrent NightHawk 4800, 5800). Remove
###	 -Qtarget if only in a 5800 environment.  (Kipp E. Howard)
#CFLAGS = -O -Qtarget=m88110compat
#EXTRA_LIBS = -lgen

# The value of QUOTESED comes from auto/config.mk.
# Uncomment the next line to use the default value.
# QUOTESED = sed -e 's/[\\"]/\\&/g' -e 's/\\"/"/' -e 's/\\";$$/";/'

##################### end of system specific lines ################### }}}

### Names of the programs and targets  {{{1
VIMTARGET	= $(VIMNAME)$(EXEEXT)
EXTARGET	= $(EXNAME)$(LNKEXT)
VIEWTARGET	= $(VIEWNAME)$(LNKEXT)
GVIMNAME	= g$(VIMNAME)
GVIMTARGET	= $(GVIMNAME)$(LNKEXT)
GVIEWNAME	= g$(VIEWNAME)
GVIEWTARGET	= $(GVIEWNAME)$(LNKEXT)
RVIMNAME	= r$(VIMNAME)
RVIMTARGET	= $(RVIMNAME)$(LNKEXT)
RVIEWNAME	= r$(VIEWNAME)
RVIEWTARGET	= $(RVIEWNAME)$(LNKEXT)
RGVIMNAME	= r$(GVIMNAME)
RGVIMTARGET	= $(RGVIMNAME)$(LNKEXT)
RGVIEWNAME	= r$(GVIEWNAME)
RGVIEWTARGET	= $(RGVIEWNAME)$(LNKEXT)
VIMDIFFNAME	= $(VIMNAME)diff
GVIMDIFFNAME	= g$(VIMDIFFNAME)
VIMDIFFTARGET	= $(VIMDIFFNAME)$(LNKEXT)
GVIMDIFFTARGET	= $(GVIMDIFFNAME)$(LNKEXT)
EVIMNAME	= e$(VIMNAME)
EVIMTARGET	= $(EVIMNAME)$(LNKEXT)
EVIEWNAME	= e$(VIEWNAME)
EVIEWTARGET	= $(EVIEWNAME)$(LNKEXT)

### Names of the tools that are also made  {{{1
TOOLS = xxd/xxd$(EXEEXT)

### Installation directories.  The defaults come from configure. {{{1
#
### prefix	the top directory for the data (default "/usr/local")
#
# Uncomment the next line to install Vim in your home directory.
#prefix = $(HOME)

### exec_prefix	is the top directory for the executable (default $(prefix))
#
# Uncomment the next line to install the Vim executable in "/usr/machine/bin"
#exec_prefix = /usr/machine

### BINDIR	dir for the executable	 (default "$(exec_prefix)/bin")
### MANDIR	dir for the manual pages (default "$(prefix)/man")
### DATADIR	dir for the other files  (default "$(prefix)/lib" or
#						  "$(prefix)/share")
# They may be different when using different architectures for the
# executable and a common directory for the other files.
#
# Uncomment the next line to install Vim in "/usr/bin"
#BINDIR   = /usr/bin
# Uncomment the next line to install Vim manuals in "/usr/share/man/man1"
#MANDIR   = /usr/share/man
# Uncomment the next line to install Vim help files in "/usr/share/vim"
#DATADIR  = /usr/share

### DESTDIR	root of the installation tree.  This is prepended to the other
#		directories.  This directory must exist.
#DESTDIR  = ~/pkg/vim

### Directory of the man pages
MAN1DIR = /man1

### Vim version (adjusted by a script)
VIMMAJOR = 8
VIMMINOR = 0

### Location of Vim files (should not need to be changed, and  {{{1
### some things might not work when they are changed!)
VIMDIR = /vim
VIMRTDIR = /vim$(VIMMAJOR)$(VIMMINOR)
HELPSUBDIR = /doc
COLSUBDIR = /colors
SYNSUBDIR = /syntax
INDSUBDIR = /indent
AUTOSUBDIR = /autoload
PLUGSUBDIR = /plugin
FTPLUGSUBDIR = /ftplugin
LANGSUBDIR = /lang
COMPSUBDIR = /compiler
KMAPSUBDIR = /keymap
MACROSUBDIR = /macros
PACKSUBDIR = /pack
TOOLSSUBDIR = /tools
TUTORSUBDIR = /tutor
SPELLSUBDIR = /spell
PRINTSUBDIR = /print
PODIR = po

### VIMLOC	common root of the Vim files (all versions)
### VIMRTLOC	common root of the runtime Vim files (this version)
### VIMRCLOC	compiled-in location for global [g]vimrc files (all versions)
### VIMRUNTIMEDIR  compiled-in location for runtime files (optional)
### HELPSUBLOC	location for help files
### COLSUBLOC	location for colorscheme files
### SYNSUBLOC	location for syntax files
### INDSUBLOC	location for indent files
### AUTOSUBLOC	location for standard autoload files
### PLUGSUBLOC	location for standard plugin files
### FTPLUGSUBLOC  location for ftplugin files
### LANGSUBLOC	location for language files
### COMPSUBLOC	location for compiler files
### KMAPSUBLOC	location for keymap files
### MACROSUBLOC	location for macro files
### PACKSUBLOC	location for packages
### TOOLSSUBLOC	location for tools files
### TUTORSUBLOC	location for tutor files
### SPELLSUBLOC	location for spell files
### PRINTSUBLOC	location for PostScript files (prolog, latin1, ..)
### SCRIPTLOC	location for script files (menu.vim, bugreport.vim, ..)
### You can override these if you want to install them somewhere else.
### Edit feature.h for compile-time settings.
VIMLOC		= $(DATADIR)$(VIMDIR)
VIMRTLOC	= $(DATADIR)$(VIMDIR)$(VIMRTDIR)
VIMRCLOC	= $(VIMLOC)
HELPSUBLOC	= $(VIMRTLOC)$(HELPSUBDIR)
COLSUBLOC	= $(VIMRTLOC)$(COLSUBDIR)
SYNSUBLOC	= $(VIMRTLOC)$(SYNSUBDIR)
INDSUBLOC	= $(VIMRTLOC)$(INDSUBDIR)
AUTOSUBLOC	= $(VIMRTLOC)$(AUTOSUBDIR)
PLUGSUBLOC	= $(VIMRTLOC)$(PLUGSUBDIR)
FTPLUGSUBLOC	= $(VIMRTLOC)$(FTPLUGSUBDIR)
LANGSUBLOC	= $(VIMRTLOC)$(LANGSUBDIR)
COMPSUBLOC	= $(VIMRTLOC)$(COMPSUBDIR)
KMAPSUBLOC	= $(VIMRTLOC)$(KMAPSUBDIR)
MACROSUBLOC	= $(VIMRTLOC)$(MACROSUBDIR)
PACKSUBLOC	= $(VIMRTLOC)$(PACKSUBDIR)
TOOLSSUBLOC	= $(VIMRTLOC)$(TOOLSSUBDIR)
TUTORSUBLOC	= $(VIMRTLOC)$(TUTORSUBDIR)
SPELLSUBLOC	= $(VIMRTLOC)$(SPELLSUBDIR)
PRINTSUBLOC	= $(VIMRTLOC)$(PRINTSUBDIR)
SCRIPTLOC	= $(VIMRTLOC)

### Only set VIMRUNTIMEDIR when VIMRTLOC is set to a different location and
### the runtime directory is not below it.
#VIMRUNTIMEDIR = $(VIMRTLOC)

### Name of the defaults/evim/mswin file target.
VIM_DEFAULTS_FILE = $(DESTDIR)$(SCRIPTLOC)/defaults.vim
EVIM_FILE	= $(DESTDIR)$(SCRIPTLOC)/evim.vim
MSWIN_FILE	= $(DESTDIR)$(SCRIPTLOC)/mswin.vim

### Name of the menu file target.
SYS_MENU_FILE	= $(DESTDIR)$(SCRIPTLOC)/menu.vim
SYS_SYNMENU_FILE = $(DESTDIR)$(SCRIPTLOC)/synmenu.vim
SYS_DELMENU_FILE = $(DESTDIR)$(SCRIPTLOC)/delmenu.vim

### Name of the bugreport file target.
SYS_BUGR_FILE	= $(DESTDIR)$(SCRIPTLOC)/bugreport.vim

### Name of the rgb.txt file target.
SYS_RGB_FILE   = $(DESTDIR)$(SCRIPTLOC)/rgb.txt

### Name of the file type detection file target.
SYS_FILETYPE_FILE = $(DESTDIR)$(SCRIPTLOC)/filetype.vim

### Name of the file type detection file target.
SYS_FTOFF_FILE	= $(DESTDIR)$(SCRIPTLOC)/ftoff.vim

### Name of the file type detection script file target.
SYS_SCRIPTS_FILE = $(DESTDIR)$(SCRIPTLOC)/scripts.vim

### Name of the ftplugin-on file target.
SYS_FTPLUGIN_FILE = $(DESTDIR)$(SCRIPTLOC)/ftplugin.vim

### Name of the ftplugin-off file target.
SYS_FTPLUGOF_FILE = $(DESTDIR)$(SCRIPTLOC)/ftplugof.vim

### Name of the indent-on file target.
SYS_INDENT_FILE = $(DESTDIR)$(SCRIPTLOC)/indent.vim

### Name of the indent-off file target.
SYS_INDOFF_FILE = $(DESTDIR)$(SCRIPTLOC)/indoff.vim

### Name of the option window script file target.
SYS_OPTWIN_FILE = $(DESTDIR)$(SCRIPTLOC)/optwin.vim

# Program to install the program in the target directory.  Could also be "mv".
INSTALL_PROG	= cp

# Program to install the data in the target directory.	Cannot be "mv"!
INSTALL_DATA	= cp
INSTALL_DATA_R	= cp -r

### Program to run on installed binary.  Use the second one to disable strip.
#STRIP = strip
#STRIP = /bin/true

### Permissions for binaries  {{{1
BINMOD = 755

### Permissions for man page
MANMOD = 644

### Permissions for help files
HELPMOD = 644

### Permissions for Perl and shell scripts
SCRIPTMOD = 755

### Permission for Vim script files (menu.vim, bugreport.vim, ..)
VIMSCRIPTMOD = 644

### Permissions for all directories that are created
DIRMOD = 755

### Permissions for all other files that are created
FILEMOD = 644

# Where to copy the man and help files from
HELPSOURCE = ../runtime/doc

# Where to copy the script files from (menu, bugreport)
SCRIPTSOURCE = ../runtime

# Where to copy the colorscheme files from
COLSOURCE = ../runtime/colors

# Where to copy the syntax files from
SYNSOURCE = ../runtime/syntax

# Where to copy the indent files from
INDSOURCE = ../runtime/indent

# Where to copy the standard plugin files from
AUTOSOURCE = ../runtime/autoload

# Where to copy the standard plugin files from
PLUGSOURCE = ../runtime/plugin

# Where to copy the ftplugin files from
FTPLUGSOURCE = ../runtime/ftplugin

# Where to copy the macro files from
MACROSOURCE = ../runtime/macros

# Where to copy the package files from
PACKSOURCE = ../runtime/pack

# Where to copy the tools files from
TOOLSSOURCE = ../runtime/tools

# Where to copy the tutor files from
TUTORSOURCE = ../runtime/tutor

# Where to copy the spell files from
SPELLSOURCE = ../runtime/spell

# Where to look for language specific files
LANGSOURCE = ../runtime/lang

# Where to look for compiler files
COMPSOURCE = ../runtime/compiler

# Where to look for keymap files
KMAPSOURCE = ../runtime/keymap

# Where to look for print resource files
PRINTSOURCE = ../runtime/print

# If you are using Linux, you might want to use this to make vim the
# default vi editor, it will create a link from vi to Vim when doing
# "make install".  An existing file will be overwritten!
# When not using it, some make programs can't handle an undefined $(LINKIT).
#LINKIT = ln -f -s $(DEST_BIN)/$(VIMTARGET) $(DESTDIR)/usr/bin/vi
LINKIT = @echo >/dev/null

###
### GRAPHICAL USER INTERFACE (GUI).  {{{1
### 'configure --enable-gui' can enable one of these for you if you did set
### a corresponding CONF_OPT_GUI above and have X11.
### Override configures choice by uncommenting all the following lines.
### As they are, the GUI is disabled.  Replace "NONE" with "ATHENA" or "MOTIF"
### for enabling the Athena or Motif GUI.
#GUI_SRC	= $(NONE_SRC)
#GUI_OBJ	= $(NONE_OBJ)
#GUI_DEFS	= $(NONE_DEFS)
#GUI_IPATH	= $(NONE_IPATH)
#GUI_LIBS_DIR	= $(NONE_LIBS_DIR)
#GUI_LIBS1	= $(NONE_LIBS1)
#GUI_LIBS2	= $(NONE_LIBS2)
#GUI_INSTALL    = $(NONE_INSTALL)
#GUI_TARGETS	= $(NONE_TARGETS)
#GUI_MAN_TARGETS= $(NONE_MAN_TARGETS)
#GUI_TESTTARGET = $(NONE_TESTTARGET)
#GUI_BUNDLE	= $(NONE_BUNDLE)

# Without a GUI install the normal way.
NONE_INSTALL = install_normal

### GTK GUI
GTK_SRC		= gui.c gui_gtk.c gui_gtk_x11.c pty.c gui_gtk_f.c \
			gui_beval.c $(GRESOURCE_SRC)
GTK_OBJ		= objects/gui.o objects/gui_gtk.o objects/gui_gtk_x11.o \
			objects/pty.o objects/gui_gtk_f.o \
			objects/gui_beval.o $(GRESOURCE_OBJ)
GTK_DEFS	= -DFEAT_GUI_GTK $(NARROW_PROTO)
GTK_IPATH	= $(GUI_INC_LOC)
GTK_LIBS_DIR	= $(GUI_LIB_LOC)
GTK_LIBS1	=
GTK_LIBS2	= $(GTK_LIBNAME)
GTK_INSTALL     = install_normal install_gui_extra
GTK_TARGETS	= installglinks
GTK_MAN_TARGETS = yes
GTK_TESTTARGET  = gui
GTK_BUNDLE	=

### Motif GUI
MOTIF_SRC	= gui.c gui_motif.c gui_x11.c pty.c gui_beval.c \
			gui_xmdlg.c gui_xmebw.c
MOTIF_OBJ	= objects/gui.o objects/gui_motif.o objects/gui_x11.o \
			objects/pty.o objects/gui_beval.o \
			objects/gui_xmdlg.o objects/gui_xmebw.o
MOTIF_DEFS	= -DFEAT_GUI_MOTIF $(NARROW_PROTO)
MOTIF_IPATH	= $(GUI_INC_LOC)
MOTIF_LIBS_DIR	= $(GUI_LIB_LOC)
MOTIF_LIBS1	=
MOTIF_LIBS2	= $(MOTIF_LIBNAME) -lXt
MOTIF_INSTALL   = install_normal install_gui_extra
MOTIF_TARGETS	= installglinks
MOTIF_MAN_TARGETS = yes
MOTIF_TESTTARGET = gui
MOTIF_BUNDLE	=

### Athena GUI
### Use Xaw3d to make the menus look a little bit nicer
#XAW_LIB = -lXaw3d
XAW_LIB = -lXaw

### When using Xaw3d, uncomment/comment the following lines to also get the
### scrollbars from Xaw3d.
#ATHENA_SRC	= gui.c gui_athena.c gui_x11.c pty.c gui_beval.c gui_at_fs.c
#ATHENA_OBJ	= objects/gui.o objects/gui_athena.o objects/gui_x11.o \
#			objects/pty.o objects/gui_beval.o objects/gui_at_fs.o
#ATHENA_DEFS	= -DFEAT_GUI_ATHENA $(NARROW_PROTO) \
#		    -Dvim_scrollbarWidgetClass=scrollbarWidgetClass \
#		    -Dvim_XawScrollbarSetThumb=XawScrollbarSetThumb
ATHENA_SRC	= gui.c gui_athena.c gui_x11.c pty.c gui_beval.c \
			gui_at_sb.c gui_at_fs.c
ATHENA_OBJ	= objects/gui.o objects/gui_athena.o objects/gui_x11.o \
			objects/pty.o objects/gui_beval.o \
			objects/gui_at_sb.o objects/gui_at_fs.o
ATHENA_DEFS	= -DFEAT_GUI_ATHENA $(NARROW_PROTO)

ATHENA_IPATH	= $(GUI_INC_LOC)
ATHENA_LIBS_DIR = $(GUI_LIB_LOC)
ATHENA_LIBS1	= $(XAW_LIB)
ATHENA_LIBS2	= -lXt
ATHENA_INSTALL  = install_normal install_gui_extra
ATHENA_TARGETS	= installglinks
ATHENA_MAN_TARGETS = yes
ATHENA_TESTTARGET = gui
ATHENA_BUNDLE	=

### neXtaw GUI
NEXTAW_LIB = -lneXtaw

NEXTAW_SRC	= gui.c gui_athena.c gui_x11.c pty.c gui_beval.c gui_at_fs.c
NEXTAW_OBJ	= objects/gui.o objects/gui_athena.o objects/gui_x11.o \
			objects/pty.o objects/gui_beval.o objects/gui_at_fs.o
NEXTAW_DEFS	= -DFEAT_GUI_ATHENA -DFEAT_GUI_NEXTAW $(NARROW_PROTO)

NEXTAW_IPATH	= $(GUI_INC_LOC)
NEXTAW_LIBS_DIR = $(GUI_LIB_LOC)
NEXTAW_LIBS1	= $(NEXTAW_LIB)
NEXTAW_LIBS2	= -lXt
NEXTAW_INSTALL  =  install_normal install_gui_extra
NEXTAW_TARGETS	=  installglinks
NEXTAW_MAN_TARGETS = yes
NEXTAW_TESTTARGET = gui
NEXTAW_BUNDLE	=

### (J)  Sun OpenWindows 3.2 (SunOS 4.1.x) or earlier that produce these ld
#	 errors:  ld: Undefined symbol
#		      _get_wmShellWidgetClass
#		      _get_applicationShellWidgetClass
# then you need to get patches 100512-02 and 100573-03 from Sun.  In the
# meantime, uncomment the following GUI_X_LIBS definition as a workaround:
#GUI_X_LIBS = -Bstatic -lXmu -Bdynamic -lXext
# If you also get cos, sin etc. as undefined symbols, try uncommenting this
# too:
#EXTRA_LIBS = /usr/openwin/lib/libXmu.sa -lm

# PHOTON GUI
PHOTONGUI_SRC	= gui.c gui_photon.c pty.c
PHOTONGUI_OBJ	= objects/gui.o objects/gui_photon.o objects/pty.o
PHOTONGUI_DEFS	= -DFEAT_GUI_PHOTON
PHOTONGUI_IPATH	=
PHOTONGUI_LIBS_DIR =
PHOTONGUI_LIBS1	= -lph -lphexlib
PHOTONGUI_LIBS2	=
PHOTONGUI_INSTALL = install_normal install_gui_extra
PHOTONGUI_TARGETS = installglinks
PHOTONGUI_MAN_TARGETS = yes
PHOTONGUI_TESTTARGET = gui
PHOTONGUI_BUNDLE =

# CARBON GUI
CARBONGUI_SRC	= gui.c gui_mac.c
CARBONGUI_OBJ	= objects/gui.o objects/gui_mac.o objects/pty.o
CARBONGUI_DEFS	= -DFEAT_GUI_MAC -fno-common -fpascal-strings \
		  -Wall -Wno-unknown-pragmas \
		  -mdynamic-no-pic -pipe
CARBONGUI_IPATH	= -I. -Iproto
CARBONGUI_LIBS_DIR =
CARBONGUI_LIBS1	= -framework Carbon
CARBONGUI_LIBS2	=
CARBONGUI_INSTALL = install_macosx
CARBONGUI_TARGETS =
CARBONGUI_MAN_TARGETS =
CARBONGUI_TESTTARGET = gui
CARBONGUI_BUNDLE = gui_bundle
APPDIR = $(VIMNAME).app
CARBONGUI_TESTARG = VIMPROG=../$(APPDIR)/Contents/MacOS/$(VIMTARGET)

# All GUI files
ALL_GUI_SRC  = gui.c gui_gtk.c gui_gtk_f.c gui_motif.c gui_xmdlg.c gui_xmebw.c gui_athena.c gui_gtk_x11.c gui_x11.c gui_at_sb.c gui_at_fs.c pty.c
ALL_GUI_PRO  = gui.pro gui_gtk.pro gui_motif.pro gui_xmdlg.pro gui_athena.pro gui_gtk_x11.pro gui_x11.pro gui_w32.pro gui_photon.pro

# }}}

### Command to create dependencies based on #include "..."
### prototype headers are ignored due to -DPROTO, system
### headers #include <...> are ignored if we use the -MM option, as
### e.g. provided by gcc-cpp.
### Include FEAT_GUI to get dependency on gui.h
### Need to change "-I /<path>" to "-isystem /<path>" for GCC 3.x.
CPP_DEPEND = $(CC) -I$(srcdir) -M$(CPP_MM) \
		`echo "$(DEPEND_CFLAGS)" $(DEPEND_CFLAGS_FILTER)`

# flags for cproto
#     This is for cproto 3 patchlevel 8 or below
#     __inline, __attribute__ and __extension__ are not recognized by cproto
#     G_IMPLEMENT_INLINES is to avoid functions defined in glib/gutils.h.
#NO_ATTR = -D__inline= -D__inline__= -DG_IMPLEMENT_INLINES \
#	  -D"__attribute__\\(x\\)=" -D"__asm__\\(x\\)=" \
#	  -D__extension__= -D__restrict="" \
#	  -D__gnuc_va_list=char -D__builtin_va_list=char

#
#     This is for cproto 3 patchlevel 9 or above (currently 4.6, 4.7g)
#     __inline and __attribute__ are now recognized by cproto
#     -D"foo()=" is not supported by all compilers so do not use it
NO_ATTR=
#
# Use this for cproto 3 patchlevel 6 or below (use "cproto -V" to check):
# PROTO_FLAGS = -f4 -d -E"$(CPP)" $(NO_ATTR)
#
# Use this for cproto 3 patchlevel 7 or above (use "cproto -V" to check):
PROTO_FLAGS = -d -E"$(CPP)" $(NO_ATTR)


################################################
##   no changes required below this line      ##
################################################

SHELL = /bin/sh

.SUFFIXES:
.SUFFIXES: .c .o .pro

PRE_DEFS = -Iproto $(DEFS) $(GUI_DEFS) $(GUI_IPATH) $(CPPFLAGS) $(EXTRA_IPATHS)
POST_DEFS = $(X_CFLAGS) $(MZSCHEME_CFLAGS) $(EXTRA_DEFS)

ALL_CFLAGS = $(PRE_DEFS) $(CFLAGS) $(PROFILE_CFLAGS) $(SANITIZER_CFLAGS) $(LEAK_CFLAGS) $(ABORT_CLFAGS) $(POST_DEFS)

# Exclude $CFLAGS for osdef.sh, for Mac 10.4 some flags don't work together
# with "-E".
OSDEF_CFLAGS = $(PRE_DEFS) $(POST_DEFS)

LINT_CFLAGS = -DLINT -I. $(PRE_DEFS) $(POST_DEFS) $(RUBY_CFLAGS) $(LUA_CFLAGS) $(PERL_CFLAGS) $(PYTHON_CFLAGS) $(PYTHON3_CFLAGS) $(TCL_CFLAGS) -Dinline= -D__extension__= -Dalloca=alloca

LINT_EXTRA = -DHANGUL_INPUT -D"__attribute__(x)="

DEPEND_CFLAGS = -DPROTO -DDEPEND -DFEAT_GUI $(LINT_CFLAGS)

# Note: MZSCHEME_LIBS must come before LIBS, because LIBS adds -lm which is
# needed by racket.
ALL_LIB_DIRS = $(GUI_LIBS_DIR) $(X_LIBS_DIR)
ALL_LIBS = \
	   $(GUI_LIBS1) \
	   $(GUI_X_LIBS) \
	   $(GUI_LIBS2) \
	   $(X_PRE_LIBS) \
	   $(X_LIBS) \
	   $(X_EXTRA_LIBS) \
	   $(MZSCHEME_LIBS) \
	   $(LIBS) \
	   $(EXTRA_LIBS) \
	   $(LUA_LIBS) \
	   $(PERL_LIBS) \
	   $(PYTHON_LIBS) \
	   $(PYTHON3_LIBS) \
	   $(TCL_LIBS) \
	   $(RUBY_LIBS) \
	   $(PROFILE_LIBS) \
	   $(SANITIZER_LIBS) \
	   $(LEAK_LIBS)

# abbreviations
DEST_BIN = $(DESTDIR)$(BINDIR)
DEST_VIM = $(DESTDIR)$(VIMLOC)
DEST_RT = $(DESTDIR)$(VIMRTLOC)
DEST_HELP = $(DESTDIR)$(HELPSUBLOC)
DEST_COL = $(DESTDIR)$(COLSUBLOC)
DEST_SYN = $(DESTDIR)$(SYNSUBLOC)
DEST_IND = $(DESTDIR)$(INDSUBLOC)
DEST_AUTO = $(DESTDIR)$(AUTOSUBLOC)
DEST_PLUG = $(DESTDIR)$(PLUGSUBLOC)
DEST_FTP = $(DESTDIR)$(FTPLUGSUBLOC)
DEST_LANG = $(DESTDIR)$(LANGSUBLOC)
DEST_COMP = $(DESTDIR)$(COMPSUBLOC)
DEST_KMAP = $(DESTDIR)$(KMAPSUBLOC)
DEST_MACRO = $(DESTDIR)$(MACROSUBLOC)
DEST_PACK = $(DESTDIR)$(PACKSUBLOC)
DEST_TOOLS = $(DESTDIR)$(TOOLSSUBLOC)
DEST_TUTOR = $(DESTDIR)$(TUTORSUBLOC)
DEST_SPELL = $(DESTDIR)$(SPELLSUBLOC)
DEST_SCRIPT = $(DESTDIR)$(SCRIPTLOC)
DEST_PRINT = $(DESTDIR)$(PRINTSUBLOC)
DEST_MAN_TOP = $(DESTDIR)$(MANDIR)

# We assume that the ".../man/xx/man1/" directory is for latin1 manual pages.
# Some systems use UTF-8, but these should find the ".../man/xx.UTF-8/man1/"
# directory first.
# FreeBSD uses ".../man/xx.ISO8859-1/man1" for latin1, use that one too.
DEST_MAN = $(DEST_MAN_TOP)$(MAN1DIR)
DEST_MAN_FR = $(DEST_MAN_TOP)/fr$(MAN1DIR)
DEST_MAN_FR_I = $(DEST_MAN_TOP)/fr.ISO8859-1$(MAN1DIR)
DEST_MAN_FR_U = $(DEST_MAN_TOP)/fr.UTF-8$(MAN1DIR)
DEST_MAN_IT = $(DEST_MAN_TOP)/it$(MAN1DIR)
DEST_MAN_IT_I = $(DEST_MAN_TOP)/it.ISO8859-1$(MAN1DIR)
DEST_MAN_IT_U = $(DEST_MAN_TOP)/it.UTF-8$(MAN1DIR)
DEST_MAN_JA_U = $(DEST_MAN_TOP)/ja$(MAN1DIR)
DEST_MAN_PL = $(DEST_MAN_TOP)/pl$(MAN1DIR)
DEST_MAN_PL_I = $(DEST_MAN_TOP)/pl.ISO8859-2$(MAN1DIR)
DEST_MAN_PL_U = $(DEST_MAN_TOP)/pl.UTF-8$(MAN1DIR)
DEST_MAN_RU = $(DEST_MAN_TOP)/ru.KOI8-R$(MAN1DIR)
DEST_MAN_RU_U = $(DEST_MAN_TOP)/ru.UTF-8$(MAN1DIR)

#	     BASIC_SRC: files that are always used
#	       GUI_SRC: extra GUI files for current configuration
#	   ALL_GUI_SRC: all GUI files for Unix
#
#		   SRC: files used for current configuration
#	      TAGS_SRC: source files used for make tags
#	     TAGS_INCL: include files used for make tags
#	       ALL_SRC: source files used for make depend and make lint

TAGS_INCL = *.h

BASIC_SRC = \
	arabic.c \
	blowfish.c \
	buffer.c \
	charset.c \
	crypt.c \
	crypt_zip.c \
	dict.c \
	diff.c \
	digraph.c \
	edit.c \
	eval.c \
	evalfunc.c \
	ex_cmds.c \
	ex_cmds2.c \
	ex_docmd.c \
	ex_eval.c \
	ex_getln.c \
	farsi.c \
	fileio.c \
	fold.c \
	getchar.c \
	hardcopy.c \
	hashtab.c \
	if_cscope.c \
	if_xcmdsrv.c \
	json.c \
	list.c \
	main.c \
	mark.c \
	memfile.c \
	memline.c \
	menu.c \
	message.c \
	misc1.c \
	misc2.c \
	move.c \
	mbyte.c \
	normal.c \
	ops.c \
	option.c \
	os_unix.c \
	auto/pathdef.c \
	popupmnu.c \
	quickfix.c \
	regexp.c \
	screen.c \
	search.c \
	sha256.c \
	spell.c \
	spellfile.c \
	syntax.c \
	tag.c \
	term.c \
	ui.c \
	undo.c \
	userfunc.c \
	version.c \
	window.c \
	$(OS_EXTRA_SRC)

SRC =	$(BASIC_SRC) \
	$(GUI_SRC) \
	$(HANGULIN_SRC) \
	$(LUA_SRC) \
	$(MZSCHEME_SRC) \
	$(PERL_SRC) \
	$(PYTHON_SRC) $(PYTHON3_SRC) \
	$(TCL_SRC) \
	$(RUBY_SRC) \
	$(WORKSHOP_SRC) \
	$(WSDEBUG_SRC)

TAGS_SRC = *.c *.cpp if_perl.xs

EXTRA_SRC = hangulin.c if_lua.c if_mzsch.c auto/if_perl.c if_perlsfio.c \
	    if_python.c if_python3.c if_tcl.c if_ruby.c \
	    gui_beval.c workshop.c wsdebug.c integration.c \
	    netbeans.c channel.c \
	    $(GRESOURCE_SRC)

# Unittest files
JSON_TEST_SRC = json_test.c
JSON_TEST_TARGET = json_test$(EXEEXT)
KWORD_TEST_SRC = kword_test.c
KWORD_TEST_TARGET = kword_test$(EXEEXT)
MEMFILE_TEST_SRC = memfile_test.c
MEMFILE_TEST_TARGET = memfile_test$(EXEEXT)
MESSAGE_TEST_SRC = message_test.c
MESSAGE_TEST_TARGET = message_test$(EXEEXT)

UNITTEST_SRC = $(JSON_TEST_SRC) $(KWORD_TEST_SRC) $(MEMFILE_TEST_SRC) $(MESSAGE_TEST_SRC)
UNITTEST_TARGETS = $(JSON_TEST_TARGET) $(KWORD_TEST_TARGET) $(MEMFILE_TEST_TARGET) $(MESSAGE_TEST_TARGET)
RUN_UNITTESTS = run_json_test run_kword_test run_memfile_test run_message_test

# All sources, also the ones that are not configured
ALL_SRC = $(BASIC_SRC) $(ALL_GUI_SRC) $(UNITTEST_SRC) $(EXTRA_SRC)

# Which files to check with lint.  Select one of these three lines.  ALL_SRC
# checks more, but may not work well for checking a GUI that wasn't configured.
# The perl sources also don't work well with lint.
LINT_SRC = $(BASIC_SRC) $(GUI_SRC) $(HANGULIN_SRC) \
	   $(PYTHON_SRC) $(PYTHON3_SRC) $(TCL_SRC) \
	   $(WORKSHOP_SRC) $(WSDEBUG_SRC) \
	   $(NETBEANS_SRC) $(CHANNEL_SRC)
#LINT_SRC = $(SRC)
#LINT_SRC = $(ALL_SRC)
#LINT_SRC = $(BASIC_SRC)

OBJ_COMMON = \
	objects/arabic.o \
	objects/buffer.o \
	objects/blowfish.o \
	objects/crypt.o \
	objects/crypt_zip.o \
	objects/dict.o \
	objects/diff.o \
	objects/digraph.o \
	objects/edit.o \
	objects/eval.o \
	objects/evalfunc.o \
	objects/ex_cmds.o \
	objects/ex_cmds2.o \
	objects/ex_docmd.o \
	objects/ex_eval.o \
	objects/ex_getln.o \
	objects/farsi.o \
	objects/fileio.o \
	objects/fold.o \
	objects/getchar.o \
	objects/hardcopy.o \
	objects/hashtab.o \
	$(HANGULIN_OBJ) \
	objects/if_cscope.o \
	objects/if_xcmdsrv.o \
	objects/list.o \
	objects/mark.o \
	objects/memline.o \
	objects/menu.o \
	objects/misc1.o \
	objects/misc2.o \
	objects/move.o \
	objects/mbyte.o \
	objects/normal.o \
	objects/ops.o \
	objects/option.o \
	objects/os_unix.o \
	objects/pathdef.o \
	objects/popupmnu.o \
	objects/quickfix.o \
	objects/regexp.o \
	objects/screen.o \
	objects/search.o \
	objects/sha256.o \
	objects/spell.o \
	objects/spellfile.o \
	objects/syntax.o \
	objects/tag.o \
	objects/term.o \
	objects/ui.o \
	objects/undo.o \
	objects/userfunc.o \
	objects/version.o \
	objects/window.o \
	$(GUI_OBJ) \
	$(LUA_OBJ) \
	$(MZSCHEME_OBJ) \
	$(PERL_OBJ) \
	$(PYTHON_OBJ) \
	$(PYTHON3_OBJ) \
	$(TCL_OBJ) \
	$(RUBY_OBJ) \
	$(OS_EXTRA_OBJ) \
	$(WORKSHOP_OBJ) \
	$(NETBEANS_OBJ) \
	$(CHANNEL_OBJ) \
	$(WSDEBUG_OBJ)

# The files included by tests are not in OBJ_COMMON.
OBJ_MAIN = \
	objects/charset.o \
	objects/json.o \
	objects/main.o \
	objects/memfile.o \
	objects/message.o

OBJ = $(OBJ_COMMON) $(OBJ_MAIN)

OBJ_JSON_TEST = \
	objects/charset.o \
	objects/memfile.o \
	objects/message.o \
	objects/json_test.o

JSON_TEST_OBJ = $(OBJ_COMMON) $(OBJ_JSON_TEST)

OBJ_KWORD_TEST = \
	objects/json.o \
	objects/memfile.o \
	objects/message.o \
	objects/kword_test.o

KWORD_TEST_OBJ = $(OBJ_COMMON) $(OBJ_KWORD_TEST)

OBJ_MEMFILE_TEST = \
	objects/charset.o \
	objects/json.o \
	objects/message.o \
	objects/memfile_test.o

MEMFILE_TEST_OBJ = $(OBJ_COMMON) $(OBJ_MEMFILE_TEST)

OBJ_MESSAGE_TEST = \
	objects/charset.o \
	objects/json.o \
	objects/memfile.o \
	objects/message_test.o

MESSAGE_TEST_OBJ = $(OBJ_COMMON) $(OBJ_MESSAGE_TEST)

ALL_OBJ = $(OBJ_COMMON) \
	  $(OBJ_MAIN) \
	  $(OBJ_JSON_TEST) \
	  $(OBJ_KWORD_TEST) \
	  $(OBJ_MEMFILE_TEST) \
	  $(OBJ_MESSAGE_TEST)


PRO_AUTO = \
	arabic.pro \
	blowfish.pro \
	buffer.pro \
	charset.pro \
	crypt.pro \
	crypt_zip.pro \
	dict.pro \
	diff.pro \
	digraph.pro \
	edit.pro \
	eval.pro \
	evalfunc.pro \
	ex_cmds.pro \
	ex_cmds2.pro \
	ex_docmd.pro \
	ex_eval.pro \
	ex_getln.pro \
	farsi.pro \
	fileio.pro \
	fold.pro \
	getchar.pro \
	hardcopy.pro \
	hashtab.pro \
	hangulin.pro \
	if_cscope.pro \
	if_lua.pro \
	if_mzsch.pro \
	if_python.pro \
	if_python3.pro \
	if_ruby.pro \
	if_xcmdsrv.pro \
	json.pro \
	list.pro \
	main.pro \
	mark.pro \
	mbyte.pro \
	memfile.pro \
	memline.pro \
	menu.pro \
	message.pro \
	misc1.pro \
	misc2.pro \
	move.pro \
	normal.pro \
	ops.pro \
	option.pro \
	os_mac_conv.pro \
	os_unix.pro \
	popupmnu.pro \
	pty.pro \
	quickfix.pro \
	regexp.pro \
	screen.pro \
	search.pro \
	sha256.pro \
	spell.pro \
	spellfile.pro \
	syntax.pro \
	tag.pro \
	term.pro \
	termlib.pro \
	ui.pro \
	undo.pro \
	userfunc.pro \
	version.pro \
	window.pro \
	gui_beval.pro \
	workshop.pro \
	netbeans.pro \
	channel.pro \
	$(ALL_GUI_PRO) \
	$(TCL_PRO)

# Resources used for the Mac are in one directory.
RSRC_DIR = os_mac_rsrc

PRO_MANUAL = os_amiga.pro os_win32.pro \
	os_mswin.pro winclip.pro os_beos.pro os_vms.pro $(PERL_PRO)

# Default target is making the executable and tools
all: $(VIMTARGET) $(TOOLS) languages $(GUI_BUNDLE)

tools: $(TOOLS)

# Run configure with all the setting from above.
#
# Note: auto/config.h doesn't depend on configure, because running configure
# doesn't always update auto/config.h.  The timestamp isn't changed if the
# file contents didn't change (to avoid recompiling everything).  Including a
# dependency on auto/config.h would cause running configure each time when
# auto/config.h isn't updated.  The dependency on auto/config.mk should make
# sure configure is run when it's needed.
#
# Remove the config.cache every time, once in a while it causes problems that
# are very hard to figure out.
#
config auto/config.mk: auto/configure config.mk.in config.h.in
	-rm -f auto/config.cache
	if test "X$(MAKECMDGOALS)" != "Xclean" \
		-a "X$(MAKECMDGOALS)" != "Xdistclean" \
		-a "X$(MAKECMDGOALS)" != "Xautoconf" \
		-a "X$(MAKECMDGOALS)" != "Xreconfig"; then \
	    GUI_INC_LOC="$(GUI_INC_LOC)" GUI_LIB_LOC="$(GUI_LIB_LOC)" \
		CC="$(CC)" CPPFLAGS="$(CPPFLAGS)" CFLAGS="$(CFLAGS)" \
		LDFLAGS="$(LDFLAGS)" $(CONF_SHELL) srcdir="$(srcdir)" \
		./configure $(CONF_OPT_GUI) $(CONF_OPT_X) $(CONF_OPT_XSMP) \
		$(CONF_OPT_DARWIN) $(CONF_OPT_FAIL) \
		$(CONF_OPT_PERL) $(CONF_OPT_PYTHON) $(CONF_OPT_PYTHON3) \
		$(CONF_OPT_TCL) $(CONF_OPT_RUBY) $(CONF_OPT_NLS) \
		$(CONF_OPT_CSCOPE) $(CONF_OPT_MULTIBYTE) $(CONF_OPT_INPUT) \
		$(CONF_OPT_OUTPUT) $(CONF_OPT_GPM) $(CONF_OPT_WORKSHOP) \
		$(CONF_OPT_FEAT) $(CONF_TERM_LIB) \
		$(CONF_OPT_COMPBY) $(CONF_OPT_ACL) $(CONF_OPT_NETBEANS) \
		$(CONF_OPT_CHANNEL) \
		$(CONF_ARGS) $(CONF_OPT_MZSCHEME) $(CONF_OPT_PLTHOME) \
		$(CONF_OPT_LUA) $(CONF_OPT_LUA_PREFIX) \
		$(CONF_OPT_SYSMOUSE); \
	fi

# Use "make reconfig" to rerun configure without cached values.
# When config.h changes, most things will be recompiled automatically.
# Invoke $(MAKE) to run config with the empty auto/config.mk.
# Invoke $(MAKE) to build all with the filled auto/config.mk.
reconfig: scratch clean
	$(MAKE) -f Makefile config
	$(MAKE) -f Makefile all

# Run autoconf to produce auto/configure.
# Note:
# - DO NOT RUN autoconf MANUALLY!  It will overwrite ./configure instead of
#   producing auto/configure.
# - autoconf is not run automatically, because a patch usually changes both
#   configure.ac and auto/configure but can't update the timestamps.  People
#   who do not have (the correct version of) autoconf would run into trouble.
#
# Two tricks are required to make autoconf put its output in the "auto" dir:
# - Temporarily move the ./configure script to ./configure.save.  Don't
#   overwrite it, it's probably the result of an aborted autoconf.
# - Use sed to change ./config.log to auto/config.log in the configure script.
# Autoconf 2.5x (2.59 at least) produces a few more files that we need to take
# care of:
# - configure.lineno: has the line numbers replaced with $LINENO.  That
#   improves patches a LOT, thus use it instead (until someone says it doesn't
#   work on some system).
# - autom4te.cache directory is created and not cleaned up.  Delete it.
# - Uses ">config.log" instead of "./config.log".
autoconf:
	if test ! -f configure.save; then mv configure configure.save; fi
	$(AUTOCONF)
	sed -e 's+>config.log+>auto/config.log+' -e 's+\./config.log+auto/config.log+' configure > auto/configure
	chmod 755 auto/configure
	mv -f configure.save configure
	-rm -rf autom4te.cache
	-rm -f auto/config.status auto/config.cache

# Run Perl to generate the Ex command lookup table.  This only needs to be run
# when a command name has been added or changed.
# NOTE: Only works when perl and vim executables are available
cmdidxs: ex_cmds.h
	if test X`perl -e "print 123"` = "X123"; then \
	   vim ex_docmd.c -c '/Beginning.*create_cmdidxs/,/End.*create_cmdidxs/! perl create_cmdidxs.pl' -c wq; \
	else \
	   echo Cannot run Perl; \
	fi

# Re-execute this Makefile to include the new auto/config.mk produced by
# configure Only used when typing "make" with a fresh auto/config.mk.
myself:
	$(MAKE) -f Makefile all


# The normal command to compile a .c file to its .o file.
# Without or with ALL_CFLAGS.
CCC_NF = $(CC) -c -I$(srcdir)
CCC = $(CCC_NF) $(ALL_CFLAGS)


# Link the target for normal use or debugging.
# A shell script is used to try linking without unnecessary libraries.
$(VIMTARGET): auto/config.mk objects $(OBJ) version.c version.h
	$(CCC) version.c -o objects/version.o
	@LINK="$(PURIFY) $(SHRPENV) $(CClink) $(ALL_LIB_DIRS) $(LDFLAGS) \
		-o $(VIMTARGET) $(OBJ) $(ALL_LIBS)" \
		MAKE="$(MAKE)" LINK_AS_NEEDED=$(LINK_AS_NEEDED) \
		sh $(srcdir)/link.sh

xxd/xxd$(EXEEXT): xxd/xxd.c
	cd xxd; CC="$(CC)" CFLAGS="$(CPPFLAGS) $(CFLAGS)" LDFLAGS="$(LDFLAGS)" \
		$(MAKE) -f Makefile

# Build the language specific files if they were unpacked.
# Generate the converted .mo files separately, it's no problem if this fails.
languages:
	@if test -n "$(MAKEMO)" -a -f $(PODIR)/Makefile; then \
		cd $(PODIR); \
		  CC="$(CC)" $(MAKE) prefix=$(DESTDIR)$(prefix); \
	fi
	-@if test -n "$(MAKEMO)" -a -f $(PODIR)/Makefile; then \
		cd $(PODIR); \
		  CC="$(CC)" $(MAKE) prefix=$(DESTDIR)$(prefix) converted; \
	fi

# Update the *.po files for changes in the sources.  Only run manually.
update-po:
	cd $(PODIR); CC="$(CC)" $(MAKE) prefix=$(DESTDIR)$(prefix) update-po

# Generate function prototypes.  This is not needed to compile vim, but if
# you want to use it, cproto is out there on the net somewhere -- Webb
#
# When generating os_amiga.pro and os_win32.pro there will be a
# few include files that can not be found, that's OK.

proto: $(PRO_AUTO) $(PRO_MANUAL)

# Filter out arguments that cproto doesn't support.
# Don't pass "-pthread", "-fwrapv" and similar arguments to cproto, it sees
# them as a list of individual flags.
# The -E"gcc -E" argument must be separate to avoid problems with shell
# quoting.
CPROTO = cproto $(PROTO_FLAGS) -DPROTO \
	 `echo '$(LINT_CFLAGS)' | sed -e 's/ -[a-z-]\+//g'`

### Would be nice if this would work for "normal" make.
### Currently it only works for (Free)BSD make.
#$(PRO_AUTO): $$(*F).c
#	$(CPROTO) -DFEAT_GUI $(*F).c > $@

# Always define FEAT_GUI.  This may generate a few warnings if it's also
# defined in auto/config.h, you can ignore that.
.c.pro:
	$(CPROTO) -DFEAT_GUI $< > proto/$@
	echo "/* vim: set ft=c : */" >> proto/$@

os_amiga.pro: os_amiga.c
	$(CPROTO) -DAMIGA -UHAVE_CONFIG_H -DBPTR=char* $< > proto/$@
	echo "/* vim: set ft=c : */" >> proto/$@

os_win32.pro: os_win32.c
	$(CPROTO) -DWIN32 -UHAVE_CONFIG_H $< > proto/$@
	echo "/* vim: set ft=c : */" >> proto/$@

os_mswin.pro: os_mswin.c
	$(CPROTO) -DWIN32 -UHAVE_CONFIG_H $< > proto/$@
	echo "/* vim: set ft=c : */" >> proto/$@

winclip.pro: winclip.c
	$(CPROTO) -DWIN32 -UHAVE_CONFIG_H $< > proto/$@
	echo "/* vim: set ft=c : */" >> proto/$@

os_beos.pro: os_beos.c
	$(CPROTO) -D__BEOS__ -UHAVE_CONFIG_H $< > proto/$@
	echo "/* vim: set ft=c : */" >> proto/$@

os_vms.pro: os_vms.c
# must use os_vms_conf.h for auto/config.h
	mv auto/config.h auto/config.h.save
	cp os_vms_conf.h auto/config.h
	$(CPROTO) -DVMS -UFEAT_GUI_ATHENA -UFEAT_GUI_NEXTAW -UFEAT_GUI_MOTIF -UFEAT_GUI_GTK $< > proto/$@
	echo "/* vim: set ft=c : */" >> proto/$@
	rm auto/config.h
	mv auto/config.h.save auto/config.h

# if_perl.pro is special: Use the generated if_perl.c for input and remove
# prototypes for local functions.
if_perl.pro: auto/if_perl.c
	$(CPROTO) -DFEAT_GUI auto/if_perl.c | sed "/_VI/d" > proto/$@

gui_gtk_gresources.pro: auto/gui_gtk_gresources.c
	$(CPROTO) -DFEAT_GUI $< > proto/$@
	echo "/* vim: set ft=c : */" >> proto/$@

notags:
	-rm -f tags

# Note: tags is made for the currently configured version, can't include both
#	Motif and Athena GUI
# You can ignore error messages for missing files.
tags TAGS: notags
	$(TAGPRG) $(TAGS_SRC) $(TAGS_INCL)

# Make a highlight file for types.  Requires Exuberant ctags and awk
types: types.vim
types.vim: $(TAGS_SRC) $(TAGS_INCL)
	ctags --c-kinds=gstu -o- $(TAGS_SRC) $(TAGS_INCL) |\
		awk 'BEGIN{printf("syntax keyword Type\t")}\
			{printf("%s ", $$1)}END{print ""}' > $@
	echo "syn keyword Constant OK FAIL TRUE FALSE MAYBE" >> $@

# TESTING
#
# Execute the test scripts and the unittests.
test check: scripttests unittests

# Execute the test scripts.  Run these after compiling Vim, before installing.
# This doesn't depend on $(VIMTARGET), because that won't work when configure
# wasn't run yet.  Restart make to build it instead.
#
# This will produce a lot of garbage on your screen, including a few error
# messages.  Don't worry about that.
# If there is a real error, there will be a difference between "testXX.out" and
# a "testXX.ok" file.
# If everything is alright, the final message will be "ALL DONE".  If not you
# get "TEST FAILURE".
#
scripttests:
	$(MAKE) -f Makefile $(VIMTARGET)
	if test -n "$(MAKEMO)" -a -f $(PODIR)/Makefile; then \
		cd $(PODIR); $(MAKE) -f Makefile check VIM=../$(VIMTARGET); \
	fi
	-if test $(VIMTARGET) != vim -a ! -r vim; then \
		ln -s $(VIMTARGET) vim; \
	fi
	cd testdir; $(MAKE) -f Makefile $(GUI_TESTTARGET) VIMPROG=../$(VIMTARGET) $(GUI_TESTARG) SCRIPTSOURCE=../$(SCRIPTSOURCE)

# Run the tests with the GUI.  Assumes vim/gvim was already built
testgui:
	cd testdir; $(MAKE) -f Makefile $(GUI_TESTTARGET) VIMPROG=../$(VIMTARGET) GUI_FLAG=-g $(GUI_TESTARG) SCRIPTSOURCE=../$(SCRIPTSOURCE)

benchmark:
	cd testdir; $(MAKE) -f Makefile benchmark VIMPROG=../$(VIMTARGET) SCRIPTSOURCE=../$(SCRIPTSOURCE)

unittesttargets:
	$(MAKE) -f Makefile $(UNITTEST_TARGETS)

# Execute the unittests one by one.
unittest unittests: $(RUN_UNITTESTS)

run_json_test: $(JSON_TEST_TARGET)
	$(VALGRIND) ./$(JSON_TEST_TARGET) || exit 1; echo $* passed;

run_kword_test: $(KWORD_TEST_TARGET)
	$(VALGRIND) ./$(KWORD_TEST_TARGET) || exit 1; echo $* passed;

run_memfile_test: $(MEMFILE_TEST_TARGET)
	$(VALGRIND) ./$(MEMFILE_TEST_TARGET) || exit 1; echo $* passed;

run_message_test: $(MESSAGE_TEST_TARGET)
	$(VALGRIND) ./$(MESSAGE_TEST_TARGET) || exit 1; echo $* passed;

# Run individual OLD style test, assuming that Vim was already compiled.
test1 \
	test_autocmd_option \
	test_autoformat_join \
	test_changelist \
	test_close_count \
	test_comparators \
	test_erasebackword \
	test_eval \
	test_fixeol \
	test_getcwd \
	test_insertcount \
	test_listchars \
	test_search_mbyte \
	test_wordcount \
	test3 test4 test5 test6 test7 test8 test9 \
	test11 test12 test14 test15 test17 test18 test19 \
	test20 test21 test22 test23 test24 test25 test26 test27 test28 test29 \
	test30 test31 test32 test33 test34 test36 test37 test38 test39 \
	test40 test41 test42 test43 test44 test45 test48 test49 \
	test50 test51 test52 test53 test54 test55 test56 test57 test58 test59 \
	test60 test64 test65 test66 test67 test68 test69 \
	test70 test72 test73 test74 test75 test77 test78 test79 \
	test80 test82 test83 test84 test85 test86 test87 test88 \
	test90 test91 test94 test95 test97 test98 test99 \
	test100 test101 test103 test104 test107 test108:
	cd testdir; rm -f $@.out; $(MAKE) -f Makefile $@.out VIMPROG=../$(VIMTARGET) $(GUI_TESTARG) SCRIPTSOURCE=../$(SCRIPTSOURCE)

# Run individual NEW style test, assuming that Vim was already compiled.
test_arglist \
	test_arabic \
	test_assert \
	test_assign \
	test_autochdir \
	test_autocmd \
	test_backspace_opt \
	test_breakindent \
	test_bufwintabinfo \
	test_cd \
	test_cdo \
	test_changedtick \
	test_channel \
	test_charsearch \
	test_charsearch_utf8 \
	test_cindent \
	test_clientserver \
	test_cmdline \
	test_command_count \
	test_crypt \
	test_cscope \
	test_cursor_func \
	test_delete \
	test_diffmode \
	test_digraph \
	test_display \
	test_edit \
	test_ex_undo \
	test_ex_z \
	test_execute_func \
	test_expand \
	test_expand_dllpath \
	test_expr \
	test_expr_utf8 \
	test_farsi \
	test_feedkeys \
	test_file_perm \
	test_fileformat \
	test_filter_cmd \
	test_filter_map \
	test_findfile \
	test_float_func \
	test_fnameescape \
	test_fnamemodify \
	test_fold \
	test_functions \
	test_ga \
	test_gf \
	test_glob2regpat \
	test_global \
	test_gn \
	test_goto \
	test_gui \
	test_gui_init \
	test_hardcopy \
	test_help \
	test_help_tagjump \
	test_hide \
	test_history \
	test_hlsearch \
	test_increment \
	test_increment_dbcs \
	test_job_fails \
	test_join \
	test_json \
	test_jumps \
	test_lambda \
	test_langmap \
	test_largefile \
	test_lispwords \
	test_listlbr \
	test_listlbr_utf8 \
	test_lua \
	test_makeencoding \
	test_man \
	test_mapping \
	test_marks \
	test_match \
	test_matchadd_conceal \
	test_matchadd_conceal_utf8 \
	test_menu \
	test_messages \
	test_mksession \
	test_mksession_utf8 \
	test_nested_function \
	test_netbeans \
	test_normal \
	test_number \
	test_options \
	test_packadd \
	test_partial \
	test_paste \
	test_perl \
	test_popup \
	test_profile \
	test_put \
	test_python2 \
	test_python3 \
	test_pyx2 \
	test_pyx3 \
	test_quickfix \
	test_quotestar \
	test_recover \
	test_regexp_latin \
	test_regexp_utf8 \
	test_reltime \
	test_retab \
	test_ruby \
	test_search \
	test_searchpos \
	test_set \
	test_signs \
	test_smartindent \
	test_sort \
	test_source_utf8 \
	test_spell \
	test_startup \
	test_startup_utf8 \
	test_stat \
	test_statusline \
	test_substitute \
	test_syn_attr \
	test_syntax \
	test_system \
	test_tabline \
	test_tabpage \
	test_tagcase \
	test_tagjump \
	test_taglist \
	test_tcl \
	test_textobjects \
	test_timers \
	test_true_false \
	test_undo \
	test_unlet \
	test_usercommands \
	test_utf8 \
	test_viminfo \
	test_vimscript \
	test_visual \
	test_window_cmd \
	test_window_id \
	test_writefile \
	test_alot_latin \
	test_alot_utf8 \
	test_alot:
	cd testdir; rm -f $@.res test.log messages; $(MAKE) -f Makefile $@.res VIMPROG=../$(VIMTARGET) $(GUI_TESTARG) SCRIPTSOURCE=../$(SCRIPTSOURCE)
	@if test -f testdir/test.log; then \
		cat testdir/test.log; \
	fi
	cat testdir/messages

newtests:
	cd testdir; rm -f $@.res test.log messages; $(MAKE) -f Makefile newtestssilent VIMPROG=../$(VIMTARGET) $(GUI_TESTARG) SCRIPTSOURCE=../$(SCRIPTSOURCE)
	@if test -f testdir/test.log; then \
		cat testdir/test.log; \
	fi
	cat testdir/messages

testclean:
	cd testdir; $(MAKE) -f Makefile clean
	if test -d $(PODIR); then \
		cd $(PODIR); $(MAKE) checkclean; \
	fi

# Unittests
# It's build just like Vim to satisfy all dependencies.
$(JSON_TEST_TARGET): auto/config.mk objects $(JSON_TEST_OBJ)
	$(CCC) version.c -o objects/version.o
	@LINK="$(PURIFY) $(SHRPENV) $(CClink) $(ALL_LIB_DIRS) $(LDFLAGS) \
		-o $(JSON_TEST_TARGET) $(JSON_TEST_OBJ) $(ALL_LIBS)" \
		MAKE="$(MAKE)" LINK_AS_NEEDED=$(LINK_AS_NEEDED) \
		sh $(srcdir)/link.sh

$(KWORD_TEST_TARGET): auto/config.mk objects $(KWORD_TEST_OBJ)
	$(CCC) version.c -o objects/version.o
	@LINK="$(PURIFY) $(SHRPENV) $(CClink) $(ALL_LIB_DIRS) $(LDFLAGS) \
		-o $(KWORD_TEST_TARGET) $(KWORD_TEST_OBJ) $(ALL_LIBS)" \
		MAKE="$(MAKE)" LINK_AS_NEEDED=$(LINK_AS_NEEDED) \
		sh $(srcdir)/link.sh

$(MEMFILE_TEST_TARGET): auto/config.mk objects $(MEMFILE_TEST_OBJ)
	$(CCC) version.c -o objects/version.o
	@LINK="$(PURIFY) $(SHRPENV) $(CClink) $(ALL_LIB_DIRS) $(LDFLAGS) \
		-o $(MEMFILE_TEST_TARGET) $(MEMFILE_TEST_OBJ) $(ALL_LIBS)" \
		MAKE="$(MAKE)" LINK_AS_NEEDED=$(LINK_AS_NEEDED) \
		sh $(srcdir)/link.sh

$(MESSAGE_TEST_TARGET): auto/config.mk objects $(MESSAGE_TEST_OBJ)
	$(CCC) version.c -o objects/version.o
	@LINK="$(PURIFY) $(SHRPENV) $(CClink) $(ALL_LIB_DIRS) $(LDFLAGS) \
		-o $(MESSAGE_TEST_TARGET) $(MESSAGE_TEST_OBJ) $(ALL_LIBS)" \
		MAKE="$(MAKE)" LINK_AS_NEEDED=$(LINK_AS_NEEDED) \
		sh $(srcdir)/link.sh

# install targets

install: $(GUI_INSTALL)

install_normal: installvim installtools $(INSTALL_LANGS) install-icons

install_gui_extra: installgtutorbin

installvim: installvimbin installtutorbin \
		installruntime installlinks installmanlinks

#
# Avoid overwriting an existing executable, somebody might be running it and
# overwriting it could cause it to crash.  Deleting it is OK, it won't be
# really deleted until all running processes for it have exited.  It is
# renamed first, in case the deleting doesn't work.
#
# If you want to keep an older version, rename it before running "make
# install".
#
installvimbin: $(VIMTARGET) $(DESTDIR)$(exec_prefix) $(DEST_BIN)
	-if test -f $(DEST_BIN)/$(VIMTARGET); then \
	  mv -f $(DEST_BIN)/$(VIMTARGET) $(DEST_BIN)/$(VIMNAME).rm; \
	  rm -f $(DEST_BIN)/$(VIMNAME).rm; \
	fi
	$(INSTALL_PROG) $(VIMTARGET) $(DEST_BIN)
	$(STRIP) $(DEST_BIN)/$(VIMTARGET)
	chmod $(BINMOD) $(DEST_BIN)/$(VIMTARGET)
# may create a link to the new executable from /usr/bin/vi
	-$(LINKIT)

# Long list of arguments for the shell script that installs the manual pages
# for one language.
INSTALLMANARGS = $(VIMLOC) $(SCRIPTLOC) $(VIMRCLOC) $(HELPSOURCE) $(MANMOD) \
		$(VIMNAME) $(VIMDIFFNAME) $(EVIMNAME)

# Install most of the runtime files
installruntime: installrtbase installmacros installpack installtutor installspell

# install the help files; first adjust the contents for the final location
installrtbase: $(HELPSOURCE)/vim.1 $(DEST_VIM) $(DEST_RT) \
		$(DEST_HELP) $(DEST_PRINT) $(DEST_COL) $(DEST_SYN) $(DEST_IND) \
		$(DEST_FTP) $(DEST_AUTO) $(DEST_AUTO)/xml $(DEST_PLUG) \
		$(DEST_TUTOR) $(DEST_SPELL) $(DEST_COMP)
	-$(SHELL) ./installman.sh install $(DEST_MAN) "" $(INSTALLMANARGS)
# Generate the help tags with ":helptags" to handle all languages.
# Move the distributed tags file aside and restore it, to avoid it being
# different from the repository.
	cd $(HELPSOURCE); if test -z "$(CROSS_COMPILING)" -a -f tags; then \
		mv -f tags tags.dist; fi
	@echo generating help tags
	-@cd $(HELPSOURCE); if test -z "$(CROSS_COMPILING)"; then \
		$(MAKE) VIMEXE=$(DEST_BIN)/$(VIMTARGET) vimtags; fi
	cd $(HELPSOURCE); \
		files=`ls *.txt tags`; \
		files="$$files `ls *.??x tags-?? 2>/dev/null || true`"; \
		$(INSTALL_DATA) $$files  $(DEST_HELP); \
		cd $(DEST_HELP); \
		chmod $(HELPMOD) $$files
	$(INSTALL_DATA)  $(HELPSOURCE)/*.pl $(DEST_HELP)
	chmod $(SCRIPTMOD) $(DEST_HELP)/*.pl
	cd $(HELPSOURCE); if test -f tags.dist; then mv -f tags.dist tags; fi
# install the menu files
	$(INSTALL_DATA) $(SCRIPTSOURCE)/menu.vim $(SYS_MENU_FILE)
	chmod $(VIMSCRIPTMOD) $(SYS_MENU_FILE)
	$(INSTALL_DATA) $(SCRIPTSOURCE)/synmenu.vim $(SYS_SYNMENU_FILE)
	chmod $(VIMSCRIPTMOD) $(SYS_SYNMENU_FILE)
	$(INSTALL_DATA) $(SCRIPTSOURCE)/delmenu.vim $(SYS_DELMENU_FILE)
	chmod $(VIMSCRIPTMOD) $(SYS_DELMENU_FILE)
# install the defaults/evim/mswin file
	$(INSTALL_DATA) $(SCRIPTSOURCE)/defaults.vim $(VIM_DEFAULTS_FILE)
	chmod $(VIMSCRIPTMOD) $(VIM_DEFAULTS_FILE)
	$(INSTALL_DATA) $(SCRIPTSOURCE)/evim.vim $(EVIM_FILE)
	chmod $(VIMSCRIPTMOD) $(EVIM_FILE)
	$(INSTALL_DATA) $(SCRIPTSOURCE)/mswin.vim $(MSWIN_FILE)
	chmod $(VIMSCRIPTMOD) $(MSWIN_FILE)
# install the rgb.txt file
	$(INSTALL_DATA) $(SCRIPTSOURCE)/rgb.txt $(SYS_RGB_FILE)
	chmod $(VIMSCRIPTMOD) $(SYS_RGB_FILE)
# install the bugreport file
	$(INSTALL_DATA) $(SCRIPTSOURCE)/bugreport.vim $(SYS_BUGR_FILE)
	chmod $(VIMSCRIPTMOD) $(SYS_BUGR_FILE)
# install the example vimrc files
	$(INSTALL_DATA) $(SCRIPTSOURCE)/vimrc_example.vim $(DEST_SCRIPT)
	chmod $(VIMSCRIPTMOD) $(DEST_SCRIPT)/vimrc_example.vim
	$(INSTALL_DATA) $(SCRIPTSOURCE)/gvimrc_example.vim $(DEST_SCRIPT)
	chmod $(VIMSCRIPTMOD) $(DEST_SCRIPT)/gvimrc_example.vim
# install the file type detection files
	$(INSTALL_DATA) $(SCRIPTSOURCE)/filetype.vim $(SYS_FILETYPE_FILE)
	chmod $(VIMSCRIPTMOD) $(SYS_FILETYPE_FILE)
	$(INSTALL_DATA) $(SCRIPTSOURCE)/ftoff.vim $(SYS_FTOFF_FILE)
	chmod $(VIMSCRIPTMOD) $(SYS_FTOFF_FILE)
	$(INSTALL_DATA) $(SCRIPTSOURCE)/scripts.vim $(SYS_SCRIPTS_FILE)
	chmod $(VIMSCRIPTMOD) $(SYS_SCRIPTS_FILE)
	$(INSTALL_DATA) $(SCRIPTSOURCE)/ftplugin.vim $(SYS_FTPLUGIN_FILE)
	chmod $(VIMSCRIPTMOD) $(SYS_FTPLUGIN_FILE)
	$(INSTALL_DATA) $(SCRIPTSOURCE)/ftplugof.vim $(SYS_FTPLUGOF_FILE)
	chmod $(VIMSCRIPTMOD) $(SYS_FTPLUGOF_FILE)
	$(INSTALL_DATA) $(SCRIPTSOURCE)/indent.vim $(SYS_INDENT_FILE)
	chmod $(VIMSCRIPTMOD) $(SYS_INDENT_FILE)
	$(INSTALL_DATA) $(SCRIPTSOURCE)/indoff.vim $(SYS_INDOFF_FILE)
	chmod $(VIMSCRIPTMOD) $(SYS_INDOFF_FILE)
	$(INSTALL_DATA) $(SCRIPTSOURCE)/optwin.vim $(SYS_OPTWIN_FILE)
	chmod $(VIMSCRIPTMOD) $(SYS_OPTWIN_FILE)
# install the print resource files
	cd $(PRINTSOURCE); $(INSTALL_DATA) *.ps $(DEST_PRINT)
	cd $(DEST_PRINT); chmod $(FILEMOD) *.ps
# install the colorscheme files
	cd $(COLSOURCE); $(INSTALL_DATA) *.vim README.txt $(DEST_COL)
	cd $(DEST_COL); chmod $(HELPMOD) *.vim README.txt
# install the syntax files
	cd $(SYNSOURCE); $(INSTALL_DATA) *.vim README.txt $(DEST_SYN)
	cd $(DEST_SYN); chmod $(HELPMOD) *.vim README.txt
# install the indent files
	cd $(INDSOURCE); $(INSTALL_DATA) *.vim README.txt $(DEST_IND)
	cd $(DEST_IND); chmod $(HELPMOD) *.vim README.txt
# install the standard autoload files
	cd $(AUTOSOURCE); $(INSTALL_DATA) *.vim README.txt $(DEST_AUTO)
	cd $(DEST_AUTO); chmod $(HELPMOD) *.vim README.txt
	cd $(AUTOSOURCE)/xml; $(INSTALL_DATA) *.vim $(DEST_AUTO)/xml
	cd $(DEST_AUTO)/xml; chmod $(HELPMOD) *.vim
# install the standard plugin files
	cd $(PLUGSOURCE); $(INSTALL_DATA) *.vim README.txt $(DEST_PLUG)
	cd $(DEST_PLUG); chmod $(HELPMOD) *.vim README.txt
# install the ftplugin files
	cd $(FTPLUGSOURCE); $(INSTALL_DATA) *.vim README.txt logtalk.dict $(DEST_FTP)
	cd $(DEST_FTP); chmod $(HELPMOD) *.vim README.txt
# install the compiler files
	cd $(COMPSOURCE); $(INSTALL_DATA) *.vim README.txt $(DEST_COMP)
	cd $(DEST_COMP); chmod $(HELPMOD) *.vim README.txt

installmacros: $(DEST_VIM) $(DEST_RT) $(DEST_MACRO)
	$(INSTALL_DATA_R) $(MACROSOURCE)/* $(DEST_MACRO)
	chmod $(DIRMOD) `find $(DEST_MACRO) -type d -print`
	chmod $(FILEMOD) `find $(DEST_MACRO) -type f -print`
	chmod $(SCRIPTMOD) $(DEST_MACRO)/less.sh
# When using CVS some CVS directories might have been copied.
# Also delete AAPDIR and *.info files.
	cvs=`find $(DEST_MACRO) \( -name CVS -o -name AAPDIR -o -name "*.info" \) -print`; \
	      if test -n "$$cvs"; then \
		 rm -rf $$cvs; \
	      fi

installpack: $(DEST_VIM) $(DEST_RT) $(DEST_PACK)
	$(INSTALL_DATA_R) $(PACKSOURCE)/* $(DEST_PACK)
	chmod $(DIRMOD) `find $(DEST_PACK) -type d -print`
	chmod $(FILEMOD) `find $(DEST_PACK) -type f -print`

# install the tutor files
installtutorbin: $(DEST_VIM)
	$(INSTALL_DATA) vimtutor $(DEST_BIN)/$(VIMNAME)tutor
	chmod $(SCRIPTMOD) $(DEST_BIN)/$(VIMNAME)tutor

installgtutorbin: $(DEST_VIM)
	$(INSTALL_DATA) gvimtutor $(DEST_BIN)/$(GVIMNAME)tutor
	chmod $(SCRIPTMOD) $(DEST_BIN)/$(GVIMNAME)tutor

installtutor: $(DEST_RT) $(DEST_TUTOR)
	-$(INSTALL_DATA) $(TUTORSOURCE)/README* $(TUTORSOURCE)/tutor* $(DEST_TUTOR)
	-rm -f $(DEST_TUTOR)/*.info
	chmod $(HELPMOD) $(DEST_TUTOR)/*

# Install the spell files, if they exist.  This assumes at least the English
# spell file is there.
installspell: $(DEST_VIM) $(DEST_RT) $(DEST_SPELL)
	if test -f $(SPELLSOURCE)/en.latin1.spl; then \
	  $(INSTALL_DATA) $(SPELLSOURCE)/*.spl $(SPELLSOURCE)/*.sug $(SPELLSOURCE)/*.vim $(DEST_SPELL); \
	  chmod $(HELPMOD) $(DEST_SPELL)/*.spl $(DEST_SPELL)/*.sug $(DEST_SPELL)/*.vim; \
	fi

# install helper program xxd
installtools: $(TOOLS) $(DESTDIR)$(exec_prefix) $(DEST_BIN) \
		$(TOOLSSOURCE) $(DEST_VIM) $(DEST_RT) $(DEST_TOOLS) \
		$(INSTALL_TOOL_LANGS)
	if test -f $(DEST_BIN)/xxd$(EXEEXT); then \
	  mv -f $(DEST_BIN)/xxd$(EXEEXT) $(DEST_BIN)/xxd.rm; \
	  rm -f $(DEST_BIN)/xxd.rm; \
	fi
	$(INSTALL_PROG) xxd/xxd$(EXEEXT) $(DEST_BIN)
	$(STRIP) $(DEST_BIN)/xxd$(EXEEXT)
	chmod $(BINMOD) $(DEST_BIN)/xxd$(EXEEXT)
	-$(SHELL) ./installman.sh xxd $(DEST_MAN) "" $(INSTALLMANARGS)

# install the runtime tools
	$(INSTALL_DATA_R) $(TOOLSSOURCE)/* $(DEST_TOOLS)
# When using CVS some CVS directories might have been copied.
	cvs=`find $(DEST_TOOLS) \( -name CVS -o -name AAPDIR \) -print`; \
	      if test -n "$$cvs"; then \
		 rm -rf $$cvs; \
	      fi
	-chmod $(FILEMOD) $(DEST_TOOLS)/*
# replace the path in some tools
	perlpath=`./which.sh perl` && sed -e "s+/usr/bin/perl+$$perlpath+" $(TOOLSSOURCE)/efm_perl.pl >$(DEST_TOOLS)/efm_perl.pl
	awkpath=`./which.sh nawk` && sed -e "s+/usr/bin/nawk+$$awkpath+" $(TOOLSSOURCE)/mve.awk >$(DEST_TOOLS)/mve.awk; if test -z "$$awkpath"; then \
		awkpath=`./which.sh gawk` && sed -e "s+/usr/bin/nawk+$$awkpath+" $(TOOLSSOURCE)/mve.awk >$(DEST_TOOLS)/mve.awk; if test -z "$$awkpath"; then \
		awkpath=`./which.sh awk` && sed -e "s+/usr/bin/nawk+$$awkpath+" $(TOOLSSOURCE)/mve.awk >$(DEST_TOOLS)/mve.awk; fi; fi
	-chmod $(SCRIPTMOD) `grep -l "^#!" $(DEST_TOOLS)/*`

# install the language specific files for tools, if they were unpacked
install-tool-languages:
	-$(SHELL) ./installman.sh xxd $(DEST_MAN_FR) "-fr" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh xxd $(DEST_MAN_FR_I) "-fr" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh xxd $(DEST_MAN_FR_U) "-fr.UTF-8" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh xxd $(DEST_MAN_IT) "-it" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh xxd $(DEST_MAN_IT_I) "-it" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh xxd $(DEST_MAN_IT_U) "-it.UTF-8" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh xxd $(DEST_MAN_JA_U) "-ja.UTF-8" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh xxd $(DEST_MAN_PL) "-pl" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh xxd $(DEST_MAN_PL_I) "-pl" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh xxd $(DEST_MAN_PL_U) "-pl.UTF-8" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh xxd $(DEST_MAN_RU) "-ru" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh xxd $(DEST_MAN_RU_U) "-ru.UTF-8" $(INSTALLMANARGS)

# install the language specific files, if they were unpacked
install-languages: languages $(DEST_LANG) $(DEST_KMAP)
	-$(SHELL) ./installman.sh install $(DEST_MAN_FR) "-fr" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh install $(DEST_MAN_FR_I) "-fr" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh install $(DEST_MAN_FR_U) "-fr.UTF-8" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh install $(DEST_MAN_IT) "-it" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh install $(DEST_MAN_IT_I) "-it" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh install $(DEST_MAN_IT_U) "-it.UTF-8" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh install $(DEST_MAN_JA_U) "-ja.UTF-8" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh install $(DEST_MAN_PL) "-pl" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh install $(DEST_MAN_PL_I) "-pl" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh install $(DEST_MAN_PL_U) "-pl.UTF-8" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh install $(DEST_MAN_RU) "-ru" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh install $(DEST_MAN_RU_U) "-ru.UTF-8" $(INSTALLMANARGS)
	-$(SHELL) ./installml.sh install "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_FR) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh install "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_FR_I) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh install "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_FR_U) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh install "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_IT) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh install "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_IT_I) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh install "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_IT_U) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh install "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_JA_U) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh install "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_PL) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh install "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_PL_I) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh install "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_PL_U) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh install "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_RU) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh install "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_RU_U) $(INSTALLMLARGS)
	if test -n "$(MAKEMO)" -a -f $(PODIR)/Makefile; then \
	   cd $(PODIR); $(MAKE) prefix=$(DESTDIR)$(prefix) LOCALEDIR=$(DEST_LANG) \
	   INSTALL_DATA=$(INSTALL_DATA) FILEMOD=$(FILEMOD) install; \
	fi
	if test -d $(LANGSOURCE); then \
	   $(INSTALL_DATA) $(LANGSOURCE)/README.txt $(LANGSOURCE)/*.vim $(DEST_LANG); \
	   chmod $(FILEMOD) $(DEST_LANG)/README.txt $(DEST_LANG)/*.vim; \
	fi
	if test -d $(KMAPSOURCE); then \
	   $(INSTALL_DATA) $(KMAPSOURCE)/README.txt $(KMAPSOURCE)/*.vim $(DEST_KMAP); \
	   chmod $(FILEMOD) $(DEST_KMAP)/README.txt $(DEST_KMAP)/*.vim; \
	fi

# Install the icons for KDE, if the directory exists and the icon doesn't.
# Always when $(DESTDIR) is not empty.
ICON48PATH = $(DESTDIR)$(DATADIR)/icons/hicolor/48x48/apps
ICON32PATH = $(DESTDIR)$(DATADIR)/icons/locolor/32x32/apps
ICON16PATH = $(DESTDIR)$(DATADIR)/icons/locolor/16x16/apps
ICONTHEMEPATH = $(DATADIR)/icons/hicolor
DESKTOPPATH = $(DESTDIR)$(DATADIR)/applications
KDEPATH = $(HOME)/.kde/share/icons
install-icons:
	if test -n "$(DESTDIR)"; then \
		$(SHELL) ./mkinstalldirs $(ICON48PATH) $(ICON32PATH) \
		$(ICON16PATH) $(DESKTOPPATH); \
	fi

	if test -d $(ICON48PATH) -a -w $(ICON48PATH) \
		-a ! -f $(ICON48PATH)/gvim.png; then \
	   $(INSTALL_DATA) $(SCRIPTSOURCE)/vim48x48.png $(ICON48PATH)/gvim.png; \
	   if test -z "$(DESTDIR)" -a -x "$(GTK_UPDATE_ICON_CACHE)" \
		   -a -w $(ICONTHEMEPATH) \
		   -a -f $(ICONTHEMEPATH)/index.theme; then \
		$(GTK_UPDATE_ICON_CACHE) -q $(ICONTHEMEPATH); \
	   fi \
	fi
	if test -d $(ICON32PATH) -a -w $(ICON32PATH) \
		-a ! -f $(ICON32PATH)/gvim.png; then \
	   $(INSTALL_DATA) $(SCRIPTSOURCE)/vim32x32.png $(ICON32PATH)/gvim.png; \
	fi
	if test -d $(ICON16PATH) -a -w $(ICON16PATH) \
		-a ! -f $(ICON16PATH)/gvim.png; then \
	   $(INSTALL_DATA) $(SCRIPTSOURCE)/vim16x16.png $(ICON16PATH)/gvim.png; \
	fi
	if test -d $(DESKTOPPATH) -a -w $(DESKTOPPATH); then \
	   $(INSTALL_DATA) $(SCRIPTSOURCE)/vim.desktop \
		$(SCRIPTSOURCE)/gvim.desktop \
		$(DESKTOPPATH); \
	   if test -z "$(DESTDIR)" -a -x "$(UPDATE_DESKTOP_DATABASE)"; then \
	      $(UPDATE_DESKTOP_DATABASE) -q $(DESKTOPPATH); \
	   fi \
	fi

$(HELPSOURCE)/vim.1 $(MACROSOURCE) $(TOOLSSOURCE):
	@echo Runtime files not found.
	@echo You need to unpack the runtime archive before running "make install".
	test -f error

$(DESTDIR)$(exec_prefix) $(DEST_BIN) \
		$(DEST_VIM) $(DEST_RT) $(DEST_HELP) \
		$(DEST_PRINT) $(DEST_COL) $(DEST_SYN) $(DEST_IND) $(DEST_FTP) \
		$(DEST_LANG) $(DEST_KMAP) $(DEST_COMP) $(DEST_MACRO) \
		$(DEST_PACK) $(DEST_TOOLS) $(DEST_TUTOR) $(DEST_SPELL) \
		$(DEST_AUTO) $(DEST_AUTO)/xml $(DEST_PLUG):
	-$(SHELL) ./mkinstalldirs $@
	-chmod $(DIRMOD) $@

# create links from various names to vim.  This is only done when the links
# (or executables with the same name) don't exist yet.
installlinks: $(GUI_TARGETS) \
			$(DEST_BIN)/$(EXTARGET) \
			$(DEST_BIN)/$(VIEWTARGET) \
			$(DEST_BIN)/$(RVIMTARGET) \
			$(DEST_BIN)/$(RVIEWTARGET) \
			$(INSTALLVIMDIFF)

installglinks: $(DEST_BIN)/$(GVIMTARGET) \
			$(DEST_BIN)/$(GVIEWTARGET) \
			$(DEST_BIN)/$(RGVIMTARGET) \
			$(DEST_BIN)/$(RGVIEWTARGET) \
			$(DEST_BIN)/$(EVIMTARGET) \
			$(DEST_BIN)/$(EVIEWTARGET) \
			$(INSTALLGVIMDIFF)

installvimdiff: $(DEST_BIN)/$(VIMDIFFTARGET)
installgvimdiff: $(DEST_BIN)/$(GVIMDIFFTARGET)

$(DEST_BIN)/$(EXTARGET):
	cd $(DEST_BIN); ln -s $(VIMTARGET) $(EXTARGET)

$(DEST_BIN)/$(VIEWTARGET):
	cd $(DEST_BIN); ln -s $(VIMTARGET) $(VIEWTARGET)

$(DEST_BIN)/$(GVIMTARGET):
	cd $(DEST_BIN); ln -s $(VIMTARGET) $(GVIMTARGET)

$(DEST_BIN)/$(GVIEWTARGET):
	cd $(DEST_BIN); ln -s $(VIMTARGET) $(GVIEWTARGET)

$(DEST_BIN)/$(RVIMTARGET):
	cd $(DEST_BIN); ln -s $(VIMTARGET) $(RVIMTARGET)

$(DEST_BIN)/$(RVIEWTARGET):
	cd $(DEST_BIN); ln -s $(VIMTARGET) $(RVIEWTARGET)

$(DEST_BIN)/$(RGVIMTARGET):
	cd $(DEST_BIN); ln -s $(VIMTARGET) $(RGVIMTARGET)

$(DEST_BIN)/$(RGVIEWTARGET):
	cd $(DEST_BIN); ln -s $(VIMTARGET) $(RGVIEWTARGET)

$(DEST_BIN)/$(VIMDIFFTARGET):
	cd $(DEST_BIN); ln -s $(VIMTARGET) $(VIMDIFFTARGET)

$(DEST_BIN)/$(GVIMDIFFTARGET):
	cd $(DEST_BIN); ln -s $(VIMTARGET) $(GVIMDIFFTARGET)

$(DEST_BIN)/$(EVIMTARGET):
	cd $(DEST_BIN); ln -s $(VIMTARGET) $(EVIMTARGET)

$(DEST_BIN)/$(EVIEWTARGET):
	cd $(DEST_BIN); ln -s $(VIMTARGET) $(EVIEWTARGET)

# Create links for the manual pages with various names to vim.	This is only
# done when the links (or manpages with the same name) don't exist yet.

INSTALLMLARGS = $(VIMNAME) $(VIMDIFFNAME) $(EVIMNAME) \
		$(EXNAME) $(VIEWNAME) $(RVIMNAME) $(RVIEWNAME) \
		$(GVIMNAME) $(GVIEWNAME) $(RGVIMNAME) $(RGVIEWNAME) \
		$(GVIMDIFFNAME) $(EVIEWNAME)

installmanlinks:
	-$(SHELL) ./installml.sh install "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN) $(INSTALLMLARGS)

uninstall: uninstall_runtime
	-rm -f $(DEST_BIN)/$(VIMTARGET)
	-rm -f $(DEST_BIN)/vimtutor
	-rm -f $(DEST_BIN)/gvimtutor
	-rm -f $(DEST_BIN)/$(EXTARGET) $(DEST_BIN)/$(VIEWTARGET)
	-rm -f $(DEST_BIN)/$(GVIMTARGET) $(DEST_BIN)/$(GVIEWTARGET)
	-rm -f $(DEST_BIN)/$(RVIMTARGET) $(DEST_BIN)/$(RVIEWTARGET)
	-rm -f $(DEST_BIN)/$(RGVIMTARGET) $(DEST_BIN)/$(RGVIEWTARGET)
	-rm -f $(DEST_BIN)/$(VIMDIFFTARGET) $(DEST_BIN)/$(GVIMDIFFTARGET)
	-rm -f $(DEST_BIN)/$(EVIMTARGET) $(DEST_BIN)/$(EVIEWTARGET)
	-rm -f $(DEST_BIN)/xxd$(EXEEXT)

# Note: the "rmdir" will fail if any files were added after "make install"
uninstall_runtime:
	-$(SHELL) ./installman.sh uninstall $(DEST_MAN) "" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh uninstall $(DEST_MAN_FR) "" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh uninstall $(DEST_MAN_FR_I) "" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh uninstall $(DEST_MAN_FR_U) "" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh uninstall $(DEST_MAN_IT) "" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh uninstall $(DEST_MAN_IT_I) "" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh uninstall $(DEST_MAN_IT_U) "" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh uninstall $(DEST_MAN_JA_U) "" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh uninstall $(DEST_MAN_PL) "" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh uninstall $(DEST_MAN_PL_I) "" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh uninstall $(DEST_MAN_PL_U) "" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh uninstall $(DEST_MAN_RU) "" $(INSTALLMANARGS)
	-$(SHELL) ./installman.sh uninstall $(DEST_MAN_RU_U) "" $(INSTALLMANARGS)
	-$(SHELL) ./installml.sh uninstall "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh uninstall "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_FR) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh uninstall "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_FR_I) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh uninstall "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_FR_U) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh uninstall "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_IT) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh uninstall "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_IT_I) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh uninstall "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_IT_U) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh uninstall "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_JA_U) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh uninstall "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_PL) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh uninstall "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_PL_I) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh uninstall "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_PL_U) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh uninstall "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_RU) $(INSTALLMLARGS)
	-$(SHELL) ./installml.sh uninstall "$(GUI_MAN_TARGETS)" \
		$(DEST_MAN_RU_U) $(INSTALLMLARGS)
	-rm -f $(DEST_MAN)/xxd.1
	-rm -f $(DEST_MAN_FR)/xxd.1 $(DEST_MAN_FR_I)/xxd.1 $(DEST_MAN_FR_U)/xxd.1
	-rm -f $(DEST_MAN_IT)/xxd.1 $(DEST_MAN_IT_I)/xxd.1 $(DEST_MAN_IT_U)/xxd.1
	-rm -f $(DEST_MAN_JA_U)/xxd.1
	-rm -f $(DEST_MAN_PL)/xxd.1 $(DEST_MAN_PL_I)/xxd.1 $(DEST_MAN_PL_U)/xxd.1
	-rm -f $(DEST_MAN_RU)/xxd.1 $(DEST_MAN_RU_U)/xxd.1
	-rm -f $(DEST_HELP)/*.txt $(DEST_HELP)/tags $(DEST_HELP)/*.pl
	-rm -f $(DEST_HELP)/*.??x $(DEST_HELP)/tags-??
	-rm -f $(SYS_RGB_FILE)
	-rm -f $(SYS_MENU_FILE) $(SYS_SYNMENU_FILE) $(SYS_DELMENU_FILE)
	-rm -f $(SYS_BUGR_FILE) $(VIM_DEFAULTS_FILE) $(EVIM_FILE) $(MSWIN_FILE)
	-rm -f $(DEST_SCRIPT)/gvimrc_example.vim $(DEST_SCRIPT)/vimrc_example.vim
	-rm -f $(SYS_FILETYPE_FILE) $(SYS_FTOFF_FILE) $(SYS_SCRIPTS_FILE)
	-rm -f $(SYS_INDOFF_FILE) $(SYS_INDENT_FILE)
	-rm -f $(SYS_FTPLUGOF_FILE) $(SYS_FTPLUGIN_FILE)
	-rm -f $(SYS_OPTWIN_FILE)
	-rm -f $(DEST_COL)/*.vim $(DEST_COL)/README.txt
	-rm -f $(DEST_SYN)/*.vim $(DEST_SYN)/README.txt
	-rm -f $(DEST_IND)/*.vim $(DEST_IND)/README.txt
	-rm -rf $(DEST_MACRO)
	-rm -rf $(DEST_PACK)
	-rm -rf $(DEST_TUTOR)
	-rm -rf $(DEST_SPELL)
	-rm -rf $(DEST_TOOLS)
	-rm -rf $(DEST_LANG)
	-rm -rf $(DEST_KMAP)
	-rm -rf $(DEST_COMP)
	-rm -f $(DEST_PRINT)/*.ps
	-rmdir $(DEST_HELP) $(DEST_PRINT) $(DEST_COL) $(DEST_SYN) $(DEST_IND)
	-rm -rf $(DEST_FTP)/*.vim $(DEST_FTP)/README.txt $(DEST_FTP)/logtalk.dict
	-rm -f $(DEST_AUTO)/*.vim $(DEST_AUTO)/README.txt $(DEST_AUTO)/xml/*.vim
	-rm -f $(DEST_PLUG)/*.vim $(DEST_PLUG)/README.txt
	-rmdir $(DEST_FTP) $(DEST_AUTO)/xml $(DEST_AUTO) $(DEST_PLUG) $(DEST_RT)
#	This will fail when other Vim versions are installed, no worries.
	-rmdir $(DEST_VIM)

# Clean up all the files that have been produced, except configure's.
# We support common typing mistakes for Juergen! :-)
clean celan: testclean
	-rm -f *.o objects/* core $(VIMTARGET).core $(VIMTARGET) vim xxd/*.o
	-rm -f $(TOOLS) auto/osdef.h auto/pathdef.c auto/if_perl.c auto/gui_gtk_gresources.c auto/gui_gtk_gresources.h
	-rm -f conftest* *~ auto/link.sed
	-rm -f testdir/opt_test.vim
	-rm -f $(UNITTEST_TARGETS)
	-rm -f runtime pixmaps
	-rm -rf $(APPDIR)
	-rm -rf mzscheme_base.c
	if test -d $(PODIR); then \
		cd $(PODIR); $(MAKE) prefix=$(DESTDIR)$(prefix) clean; \
	fi

# Make a shadow directory for compilation on another system or with different
# features.
SHADOWDIR = shadow

shadow:	runtime pixmaps
	mkdir $(SHADOWDIR)
	cd $(SHADOWDIR); ln -s ../*.[chm] ../*.in ../*.sh ../*.xs ../*.xbm ../gui_gtk_res.xml ../toolcheck ../proto ../vimtutor ../gvimtutor ../mkinstalldirs .
	mkdir $(SHADOWDIR)/auto
	cd $(SHADOWDIR)/auto; ln -s ../../auto/configure .
	mkdir $(SHADOWDIR)/po
	cd $(SHADOWDIR)/po; ln -s ../../po/*.po ../../po/*.mak ../../po/*.vim ../../po/Makefile .
	cd $(SHADOWDIR); rm -f auto/link.sed
	cp Makefile configure $(SHADOWDIR)
	rm -f $(SHADOWDIR)/auto/config.mk $(SHADOWDIR)/config.mk.dist
	cp config.mk.dist $(SHADOWDIR)/auto/config.mk
	cp config.mk.dist $(SHADOWDIR)
	mkdir $(SHADOWDIR)/xxd
	cd $(SHADOWDIR)/xxd; ln -s ../../xxd/*.[ch] ../../xxd/Make* .
	if test -d $(RSRC_DIR); then \
		cd $(SHADOWDIR); \
		ln -s ../infplist.xml .; \
		ln -s ../$(RSRC_DIR) ../os_mac.rsr.hqx ../dehqx.py .; \
	fi
	mkdir $(SHADOWDIR)/testdir
	cd $(SHADOWDIR)/testdir; ln -s ../../testdir/Makefile \
				 ../../testdir/Make_all.mak \
				 ../../testdir/README.txt \
				 ../../testdir/*.in \
				 ../../testdir/*.vim \
				 ../../testdir/*.py \
				 ../../testdir/python* \
				 ../../testdir/pyxfile \
				 ../../testdir/sautest \
				 ../../testdir/samples \
				 ../../testdir/test83-tags? \
				 ../../testdir/*.ok .

# Link needed for doing "make install" in a shadow directory.
runtime:
	-ln -s ../runtime .

# Link needed for doing "make" using GTK in a shadow directory.
pixmaps:
	-ln -s ../pixmaps .

# Update the synmenu.vim file with the latest Syntax menu.
# This is only needed when runtime/makemenu.vim was changed.
menu: ./vim ../runtime/makemenu.vim
	./vim -u ../runtime/makemenu.vim

# Start configure from scratch
scrub scratch:
	-rm -f auto/config.status auto/config.cache config.log auto/config.log
	-rm -f auto/config.h auto/link.log auto/link.sed auto/config.mk
	touch auto/config.h
	cp config.mk.dist auto/config.mk

distclean: clean scratch
	-rm -f tags

dist: distclean
	@echo
	@echo Making the distribution has to be done in the top directory

mdepend:
	-@rm -f Makefile~
	cp Makefile Makefile~
	sed -e '/\#\#\# Dependencies/q' < Makefile > tmp_make
	@for i in $(ALL_SRC) ; do \
	  echo "$$i" ; \
	  echo `echo "$$i" | sed -e 's/[^ ]*\.c$$/objects\/\1.o/'`": $$i" `\
	    $(CPP) $$i |\
	    grep '^# .*"\./.*\.h"' |\
	    sort -t'"' -u +1 -2 |\
	    sed -e 's/.*"\.\/\(.*\)".*/\1/'\
	    ` >> tmp_make ; \
	done
	mv tmp_make Makefile

depend:
	-@rm -f Makefile~
	cp Makefile Makefile~
	sed -e '/\#\#\# Dependencies/q' < Makefile > tmp_make
	-for i in $(ALL_SRC); do echo $$i; \
		$(CPP_DEPEND) $$i | \
		sed -e 's+^\([^ ]*\.o\)+objects/\1+' >> tmp_make; done
	mv tmp_make Makefile

# Run lint.  Clean up the *.ln files that are sometimes left behind.
lint:
	$(LINT) $(LINT_OPTIONS) $(LINT_CFLAGS) $(LINT_EXTRA) $(LINT_SRC)
	-rm -f *.ln

# Check dosinst.c with lint.
lintinstall:
	$(LINT) $(LINT_OPTIONS) -DWIN32 -DUNIX_LINT dosinst.c
	-rm -f dosinst.ln

###########################################################################

.c.o:
	$(CCC) $<

auto/if_perl.c: if_perl.xs
	$(PERL) -e 'unless ( $$] >= 5.005 ) { for (qw(na defgv errgv)) { print "#define PL_$$_ $$_\n" }}' > $@
	$(PERL) $(PERL_XSUBPP) -prototypes -typemap \
	    $(PERLLIB)/ExtUtils/typemap if_perl.xs >> $@

auto/osdef.h: auto/config.h osdef.sh osdef1.h.in osdef2.h.in
	CC="$(CC) $(OSDEF_CFLAGS)" srcdir=$(srcdir) sh $(srcdir)/osdef.sh

auto/pathdef.c: Makefile auto/config.mk
	-@echo creating $@
	-@echo '/* pathdef.c */' > $@
	-@echo '/* This file is automatically created by Makefile' >> $@
	-@echo ' * DO NOT EDIT!  Change Makefile only. */' >> $@
	-@echo '#include "vim.h"' >> $@
	-@echo 'char_u *default_vim_dir = (char_u *)"$(VIMRCLOC)";' | $(QUOTESED) >> $@
	-@echo 'char_u *default_vimruntime_dir = (char_u *)"$(VIMRUNTIMEDIR)";' | $(QUOTESED) >> $@
	-@echo 'char_u *all_cflags = (char_u *)"$(CC) -c -I$(srcdir) $(ALL_CFLAGS)";' | $(QUOTESED) >>  $@
	-@echo 'char_u *all_lflags = (char_u *)"$(CC) $(ALL_LIB_DIRS) $(LDFLAGS) -o $(VIMTARGET) $(ALL_LIBS) ";' | $(QUOTESED) >>  $@
	-@echo 'char_u *compiled_user = (char_u *)"' | tr -d $(NL) >> $@
	-@if test -n "$(COMPILEDBY)"; then \
		echo "$(COMPILEDBY)" | tr -d $(NL) >> $@; \
		else ((logname) 2>/dev/null || whoami) | tr -d $(NL) >> $@; fi
	-@echo '";' >> $@
	-@echo 'char_u *compiled_sys = (char_u *)"' | tr -d $(NL) >> $@
	-@if test -z "$(COMPILEDBY)"; then hostname | tr -d $(NL) >> $@; fi
	-@echo '";' >> $@
	-@sh $(srcdir)/pathdef.sh

GUI_GTK_RES_INPUTS = \
	../pixmaps/stock_vim_build_tags.png \
	../pixmaps/stock_vim_find_help.png \
	../pixmaps/stock_vim_save_all.png \
	../pixmaps/stock_vim_session_load.png \
	../pixmaps/stock_vim_session_new.png \
	../pixmaps/stock_vim_session_save.png \
	../pixmaps/stock_vim_shell.png \
	../pixmaps/stock_vim_window_maximize.png \
	../pixmaps/stock_vim_window_maximize_width.png \
	../pixmaps/stock_vim_window_minimize.png \
	../pixmaps/stock_vim_window_minimize_width.png \
	../pixmaps/stock_vim_window_split.png \
	../pixmaps/stock_vim_window_split_vertical.png

auto/gui_gtk_gresources.c: gui_gtk_res.xml $(GUI_GTK_RES_INPUTS)
	$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=../pixmaps --generate --c-name=gui_gtk --manual-register gui_gtk_res.xml
auto/gui_gtk_gresources.h: gui_gtk_res.xml $(GUI_GTK_RES_INPUTS)
	if test -z "$(GLIB_COMPILE_RESOURCES)"; then touch $@; else \
		$(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=../pixmaps --generate --c-name=gui_gtk --manual-register gui_gtk_res.xml; \
	fi

# All the object files are put in the "objects" directory.  Since not all make
# commands understand putting object files in another directory, it must be
# specified for each file separately.

objects: objects/.dirstamp

objects/.dirstamp:
	mkdir -p objects
	touch objects/.dirstamp

# All object files depend on the objects directory, so that parallel make
# works.  Can't depend on the directory itself, its timestamp changes all the
# time.
$(ALL_OBJ): objects/.dirstamp

objects/arabic.o: arabic.c
	$(CCC) -o $@ arabic.c

objects/blowfish.o: blowfish.c
	$(CCC) -o $@ blowfish.c

objects/buffer.o: buffer.c
	$(CCC) -o $@ buffer.c

objects/charset.o: charset.c
	$(CCC) -o $@ charset.c

objects/crypt.o: crypt.c
	$(CCC) -o $@ crypt.c

objects/crypt_zip.o: crypt_zip.c
	$(CCC) -o $@ crypt_zip.c

objects/dict.o: dict.c
	$(CCC) -o $@ dict.c

objects/diff.o: diff.c
	$(CCC) -o $@ diff.c

objects/digraph.o: digraph.c
	$(CCC) -o $@ digraph.c

objects/edit.o: edit.c
	$(CCC) -o $@ edit.c

objects/eval.o: eval.c
	$(CCC) -o $@ eval.c

objects/evalfunc.o: evalfunc.c
	$(CCC) -o $@ evalfunc.c

objects/ex_cmds.o: ex_cmds.c
	$(CCC) -o $@ ex_cmds.c

objects/ex_cmds2.o: ex_cmds2.c
	$(CCC) -o $@ ex_cmds2.c

objects/ex_docmd.o: ex_docmd.c
	$(CCC) -o $@ ex_docmd.c

objects/ex_eval.o: ex_eval.c
	$(CCC) -o $@ ex_eval.c

objects/ex_getln.o: ex_getln.c
	$(CCC) -o $@ ex_getln.c

objects/farsi.o: farsi.c
	$(CCC) -o $@ farsi.c

objects/fileio.o: fileio.c
	$(CCC) -o $@ fileio.c

objects/fold.o: fold.c
	$(CCC) -o $@ fold.c

objects/getchar.o: getchar.c
	$(CCC) -o $@ getchar.c

objects/hardcopy.o: hardcopy.c
	$(CCC) -o $@ hardcopy.c

objects/hashtab.o: hashtab.c
	$(CCC) -o $@ hashtab.c

objects/gui.o: gui.c
	$(CCC) -o $@ gui.c

objects/gui_at_fs.o: gui_at_fs.c
	$(CCC) -o $@ gui_at_fs.c

objects/gui_at_sb.o: gui_at_sb.c
	$(CCC) -o $@ gui_at_sb.c

objects/gui_athena.o: gui_athena.c
	$(CCC) -o $@ gui_athena.c

objects/gui_beval.o: gui_beval.c
	$(CCC) -o $@ gui_beval.c

objects/gui_gtk.o: gui_gtk.c
	$(CCC) -o $@ gui_gtk.c

objects/gui_gtk_f.o: gui_gtk_f.c
	$(CCC) -o $@ gui_gtk_f.c

objects/gui_gtk_gresources.o: auto/gui_gtk_gresources.c
	$(CCC_NF) $(PERL_CFLAGS) $(ALL_CFLAGS) -o $@ auto/gui_gtk_gresources.c

objects/gui_gtk_x11.o: gui_gtk_x11.c
	$(CCC) -o $@ gui_gtk_x11.c

objects/gui_motif.o: gui_motif.c
	$(CCC) -o $@ gui_motif.c

objects/gui_xmdlg.o: gui_xmdlg.c
	$(CCC) -o $@ gui_xmdlg.c

objects/gui_xmebw.o: gui_xmebw.c
	$(CCC) -o $@ gui_xmebw.c

objects/gui_x11.o: gui_x11.c
	$(CCC) -o $@ gui_x11.c

objects/gui_photon.o: gui_photon.c
	$(CCC) -o $@ gui_photon.c

objects/gui_mac.o: gui_mac.c
	$(CCC) -o $@ gui_mac.c

objects/hangulin.o: hangulin.c
	$(CCC) -o $@ hangulin.c

objects/if_cscope.o: if_cscope.c
	$(CCC) -o $@ if_cscope.c

objects/if_xcmdsrv.o: if_xcmdsrv.c
	$(CCC) -o $@ if_xcmdsrv.c

objects/if_lua.o: if_lua.c
	$(CCC_NF) $(LUA_CFLAGS) $(ALL_CFLAGS) -o $@ if_lua.c

objects/if_mzsch.o: if_mzsch.c $(MZSCHEME_EXTRA)
	$(CCC) -o $@ $(MZSCHEME_CFLAGS_EXTRA) if_mzsch.c

mzscheme_base.c:
	$(MZSCHEME_MZC) --c-mods mzscheme_base.c ++lib scheme/base

objects/if_perl.o: auto/if_perl.c
	$(CCC_NF) $(PERL_CFLAGS) $(ALL_CFLAGS) -o $@ auto/if_perl.c

objects/if_perlsfio.o: if_perlsfio.c
	$(CCC_NF) $(PERL_CFLAGS) $(ALL_CFLAGS) -o $@ if_perlsfio.c

objects/py_getpath.o: $(PYTHON_CONFDIR)/getpath.c
	$(CCC_NF) $(PYTHON_CFLAGS) $(ALL_CFLAGS) -o $@ \
		$(PYTHON_CONFDIR)/getpath.c \
		-I$(PYTHON_CONFDIR) -DHAVE_CONFIG_H -DNO_MAIN \
		$(PYTHON_GETPATH_CFLAGS)

objects/if_python.o: if_python.c if_py_both.h
	$(CCC_NF) $(PYTHON_CFLAGS) $(PYTHON_CFLAGS_EXTRA) $(ALL_CFLAGS) -o $@ if_python.c

objects/if_python3.o: if_python3.c if_py_both.h
	$(CCC_NF) $(PYTHON3_CFLAGS) $(PYTHON3_CFLAGS_EXTRA) $(ALL_CFLAGS) -o $@ if_python3.c

objects/if_ruby.o: if_ruby.c
	$(CCC_NF) $(RUBY_CFLAGS) $(ALL_CFLAGS) -o $@ if_ruby.c

objects/if_tcl.o: if_tcl.c
	$(CCC_NF) $(TCL_CFLAGS) $(ALL_CFLAGS) -o $@ if_tcl.c

objects/integration.o: integration.c
	$(CCC) -o $@ integration.c

objects/json.o: json.c
	$(CCC) -o $@ json.c

objects/json_test.o: json_test.c
	$(CCC) -o $@ json_test.c

objects/kword_test.o: kword_test.c
	$(CCC) -o $@ kword_test.c

objects/list.o: list.c
	$(CCC) -o $@ list.c

objects/main.o: main.c
	$(CCC) -o $@ main.c

objects/mark.o: mark.c
	$(CCC) -o $@ mark.c

objects/memfile.o: memfile.c
	$(CCC) -o $@ memfile.c

objects/memfile_test.o: memfile_test.c
	$(CCC) -o $@ memfile_test.c

objects/memline.o: memline.c
	$(CCC) -o $@ memline.c

objects/menu.o: menu.c
	$(CCC) -o $@ menu.c

objects/message.o: message.c
	$(CCC) -o $@ message.c

objects/message_test.o: message_test.c
	$(CCC) -o $@ message_test.c

objects/misc1.o: misc1.c
	$(CCC) -o $@ misc1.c

objects/misc2.o: misc2.c
	$(CCC) -o $@ misc2.c

objects/move.o: move.c
	$(CCC) -o $@ move.c

objects/mbyte.o: mbyte.c
	$(CCC) -o $@ mbyte.c

objects/normal.o: normal.c
	$(CCC) -o $@ normal.c

objects/ops.o: ops.c
	$(CCC) -o $@ ops.c

objects/option.o: option.c
	$(CCC_NF) $(LUA_CFLAGS) $(PERL_CFLAGS) $(PYTHON_CFLAGS) $(PYTHON3_CFLAGS) $(RUBY_CFLAGS) $(TCL_CFLAGS) $(ALL_CFLAGS) -o $@ option.c

objects/os_beos.o: os_beos.c
	$(CCC) -o $@ os_beos.c

objects/os_qnx.o: os_qnx.c
	$(CCC) -o $@ os_qnx.c

objects/os_macosx.o: os_macosx.m
	$(CCC) -o $@ os_macosx.m

objects/os_mac_conv.o: os_mac_conv.c
	$(CCC) -o $@ os_mac_conv.c

objects/os_unix.o: os_unix.c
	$(CCC) -o $@ os_unix.c

objects/os_mswin.o: os_mswin.c
	$(CCC) -o $@ os_mswin.c

objects/winclip.o: winclip.c
	$(CCC) -o $@ winclip.c

objects/pathdef.o: auto/pathdef.c
	$(CCC) -o $@ auto/pathdef.c

objects/pty.o: pty.c
	$(CCC) -o $@ pty.c

objects/popupmnu.o: popupmnu.c
	$(CCC) -o $@ popupmnu.c

objects/quickfix.o: quickfix.c
	$(CCC) -o $@ quickfix.c

objects/regexp.o: regexp.c regexp_nfa.c
	$(CCC) -o $@ regexp.c

objects/screen.o: screen.c
	$(CCC) -o $@ screen.c

objects/search.o: search.c
	$(CCC) -o $@ search.c

objects/sha256.o: sha256.c
	$(CCC) -o $@ sha256.c

objects/spell.o: spell.c
	$(CCC) -o $@ spell.c

objects/spellfile.o: spellfile.c
	$(CCC) -o $@ spellfile.c

objects/syntax.o: syntax.c
	$(CCC) -o $@ syntax.c

objects/tag.o: tag.c
	$(CCC) -o $@ tag.c

objects/term.o: term.c
	$(CCC) -o $@ term.c

objects/ui.o: ui.c
	$(CCC) -o $@ ui.c

objects/undo.o: undo.c
	$(CCC) -o $@ undo.c

objects/userfunc.o: userfunc.c
	$(CCC) -o $@ userfunc.c

objects/window.o: window.c
	$(CCC) -o $@ window.c

objects/workshop.o: workshop.c
	$(CCC) -o $@ workshop.c

objects/wsdebug.o: wsdebug.c
	$(CCC) -o $@ wsdebug.c

objects/netbeans.o: netbeans.c
	$(CCC) -o $@ netbeans.c

objects/channel.o: channel.c
	$(CCC) -o $@ channel.c

Makefile:
	@echo The name of the makefile MUST be "Makefile" (with capital M)!!!!

###############################################################################
### MacOS X installation
###
### This installs a runnable Vim.app in $(prefix)

REZ    = /Developer/Tools/Rez
RESDIR = $(APPDIR)/Contents/Resources
VERSION = $(VIMMAJOR).$(VIMMINOR)

### Common flags
M4FLAGSX = $(M4FLAGS) -DAPP_EXE=$(VIMNAME) -DAPP_NAME=$(VIMNAME) \
		-DAPP_VER=$(VERSION)

install_macosx: gui_bundle
# Remove the link to the runtime dir, don't want to copy all of that.
	-rm $(RESDIR)/vim/runtime
	$(INSTALL_DATA_R) $(APPDIR) $(DESTDIR)$(prefix)
# Generate the help tags file now, it won't work with "make installruntime".
	-@srcdir=`pwd`; cd $(HELPSOURCE); $(MAKE) VIMEXE=$$srcdir/$(VIMTARGET) vimtags
# Install the runtime files.  Recursive!
	-mkdir -p $(DESTDIR)$(prefix)/$(RESDIR)/vim/runtime
#	-mkdir $(DESTDIR)$(prefix)/$(APPDIR)/bin
	srcdir=`pwd`; $(MAKE) -f Makefile installruntime \
		VIMEXE=$$srcdir/$(VIMTARGET) \
		prefix=$(DESTDIR)$(prefix)/$(RESDIR)$(VIMDIR) \
		exec_prefix=$(DESTDIR)$(prefix)/$(APPDIR)/Contents \
		BINDIR=$(DESTDIR)$(prefix)/$(APPDIR)/Contents/MacOS \
		VIMLOC=$(DESTDIR)$(prefix)/$(RESDIR)$(VIMDIR) \
		VIMRTLOC=$(DESTDIR)$(prefix)/$(RESDIR)$(VIMDIR)/runtime
# Put the link back.
	ln -s `pwd`/../runtime $(RESDIR)/vim
# Copy rgb.txt, Mac doesn't always have X11
	$(INSTALL_DATA) $(SCRIPTSOURCE)/rgb.txt $(DESTDIR)$(prefix)/$(RESDIR)/vim/runtime
# TODO: Create the vimtutor and/or gvimtutor application.

gui_bundle: $(RESDIR) bundle-dir bundle-executable bundle-info bundle-resource \
	bundle-language

$(RESDIR):
	mkdir -p $@

bundle-dir: $(APPDIR)/Contents $(VIMTARGET)
# Make a link to the runtime directory, so that we can try out the executable
# without installing it.
	mkdir -p $(RESDIR)/vim
	-ln -s `pwd`/../runtime $(RESDIR)/vim

bundle-executable: $(VIMTARGET)
	mkdir -p $(APPDIR)/Contents/MacOS
	cp $(VIMTARGET) $(APPDIR)/Contents/MacOS/$(VIMTARGET)

bundle-info:  bundle-dir
	@echo "Creating PkgInfo"
	@echo -n "APPLVIM!" > $(APPDIR)/Contents/PkgInfo
	@echo "Creating Info.plist"
	m4 $(M4FLAGSX) infplist.xml > $(APPDIR)/Contents/Info.plist

bundle-resource: bundle-dir bundle-rsrc
	cp -f $(RSRC_DIR)/*.icns $(RESDIR)

### Classic resources
# Resource fork (in the form of a .rsrc file) for Classic Vim (Mac OS 9)
# This file is also required for OS X Vim.
bundle-rsrc: os_mac.rsr.hqx
	@echo "Creating resource fork"
	python dehqx.py $<
	rm -f gui_mac.rsrc
	mv gui_mac.rsrc.rsrcfork $(RESDIR)/$(VIMNAME).rsrc

# po/Make_osx.pl says something about generating a Mac message file
# for Ukrainian.  Would somebody using Mac OS X in Ukrainian
# *really* be upset that Carbon Vim was not localised in
# Ukrainian?
#
#bundle-language: bundle-dir po/Make_osx.pl
#	cd po && perl Make_osx.pl --outdir ../$(RESDIR) $(MULTILANG)
bundle-language: bundle-dir

$(APPDIR)/Contents:
	-$(SHELL) ./mkinstalldirs $(APPDIR)/Contents/MacOS
	-$(SHELL) ./mkinstalldirs $(RESDIR)/English.lproj


###############################################################################
### (automatically generated by 'make depend')
### Dependencies:
objects/arabic.o: arabic.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/blowfish.o: blowfish.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/buffer.o: buffer.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h version.h
objects/charset.o: charset.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/crypt.o: crypt.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/crypt_zip.o: crypt_zip.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/dict.o: dict.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/diff.o: diff.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/digraph.o: digraph.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/edit.o: edit.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/eval.o: eval.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h version.h
objects/evalfunc.o: evalfunc.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h version.h
objects/ex_cmds.o: ex_cmds.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h version.h
objects/ex_cmds2.o: ex_cmds2.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h version.h
objects/ex_docmd.o: ex_docmd.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/ex_eval.o: ex_eval.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/ex_getln.o: ex_getln.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/farsi.o: farsi.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/fileio.o: fileio.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/fold.o: fold.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/getchar.o: getchar.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/hardcopy.o: hardcopy.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h version.h
objects/hashtab.o: hashtab.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/if_cscope.o: if_cscope.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h if_cscope.h
objects/if_xcmdsrv.o: if_xcmdsrv.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h version.h
objects/json.o: json.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/list.o: list.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/main.o: main.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/mark.o: mark.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/memfile.o: memfile.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/memline.o: memline.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/menu.o: menu.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/message.o: message.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/misc1.o: misc1.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h version.h
objects/misc2.o: misc2.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/move.o: move.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/mbyte.o: mbyte.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/normal.o: normal.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/ops.o: ops.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h ascii.h \
 keymap.h term.h macros.h option.h structs.h regexp.h gui.h gui_beval.h \
 proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h globals.h farsi.h \
 arabic.h
objects/option.o: option.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/os_unix.o: os_unix.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h if_mzsch.h os_unixx.h
objects/pathdef.o: auto/pathdef.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/popupmnu.o: popupmnu.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/quickfix.o: quickfix.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/regexp.o: regexp.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h regexp_nfa.c
objects/screen.o: screen.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/search.o: search.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/sha256.o: sha256.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/spell.o: spell.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/spellfile.o: spellfile.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/syntax.o: syntax.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/tag.o: tag.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h ascii.h \
 keymap.h term.h macros.h option.h structs.h regexp.h gui.h gui_beval.h \
 proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h globals.h farsi.h \
 arabic.h
objects/term.o: term.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/ui.o: ui.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h ascii.h \
 keymap.h term.h macros.h option.h structs.h regexp.h gui.h gui_beval.h \
 proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h globals.h farsi.h \
 arabic.h
objects/undo.o: undo.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/userfunc.o: userfunc.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/version.o: version.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h version.h
objects/window.o: window.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/gui.o: gui.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h ascii.h \
 keymap.h term.h macros.h option.h structs.h regexp.h gui.h gui_beval.h \
 proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h globals.h farsi.h \
 arabic.h
objects/gui_gtk.o: gui_gtk.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h gui_gtk_f.h
objects/gui_gtk_f.o: gui_gtk_f.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h gui_gtk_f.h
objects/gui_motif.o: gui_motif.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h gui_xmebw.h ../pixmaps/alert.xpm \
 ../pixmaps/error.xpm ../pixmaps/generic.xpm ../pixmaps/info.xpm \
 ../pixmaps/quest.xpm gui_x11_pm.h ../pixmaps/tb_new.xpm \
 ../pixmaps/tb_open.xpm ../pixmaps/tb_close.xpm ../pixmaps/tb_save.xpm \
 ../pixmaps/tb_print.xpm ../pixmaps/tb_cut.xpm ../pixmaps/tb_copy.xpm \
 ../pixmaps/tb_paste.xpm ../pixmaps/tb_find.xpm \
 ../pixmaps/tb_find_next.xpm ../pixmaps/tb_find_prev.xpm \
 ../pixmaps/tb_find_help.xpm ../pixmaps/tb_exit.xpm \
 ../pixmaps/tb_undo.xpm ../pixmaps/tb_redo.xpm ../pixmaps/tb_help.xpm \
 ../pixmaps/tb_macro.xpm ../pixmaps/tb_make.xpm \
 ../pixmaps/tb_save_all.xpm ../pixmaps/tb_jump.xpm \
 ../pixmaps/tb_ctags.xpm ../pixmaps/tb_load_session.xpm \
 ../pixmaps/tb_save_session.xpm ../pixmaps/tb_new_session.xpm \
 ../pixmaps/tb_blank.xpm ../pixmaps/tb_maximize.xpm \
 ../pixmaps/tb_split.xpm ../pixmaps/tb_minimize.xpm \
 ../pixmaps/tb_shell.xpm ../pixmaps/tb_replace.xpm \
 ../pixmaps/tb_vsplit.xpm ../pixmaps/tb_maxwidth.xpm \
 ../pixmaps/tb_minwidth.xpm
objects/gui_xmdlg.o: gui_xmdlg.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/gui_xmebw.o: gui_xmebw.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h gui_xmebwp.h gui_xmebw.h
objects/gui_athena.o: gui_athena.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h gui_at_sb.h gui_x11_pm.h \
 ../pixmaps/tb_new.xpm ../pixmaps/tb_open.xpm ../pixmaps/tb_close.xpm \
 ../pixmaps/tb_save.xpm ../pixmaps/tb_print.xpm ../pixmaps/tb_cut.xpm \
 ../pixmaps/tb_copy.xpm ../pixmaps/tb_paste.xpm ../pixmaps/tb_find.xpm \
 ../pixmaps/tb_find_next.xpm ../pixmaps/tb_find_prev.xpm \
 ../pixmaps/tb_find_help.xpm ../pixmaps/tb_exit.xpm \
 ../pixmaps/tb_undo.xpm ../pixmaps/tb_redo.xpm ../pixmaps/tb_help.xpm \
 ../pixmaps/tb_macro.xpm ../pixmaps/tb_make.xpm \
 ../pixmaps/tb_save_all.xpm ../pixmaps/tb_jump.xpm \
 ../pixmaps/tb_ctags.xpm ../pixmaps/tb_load_session.xpm \
 ../pixmaps/tb_save_session.xpm ../pixmaps/tb_new_session.xpm \
 ../pixmaps/tb_blank.xpm ../pixmaps/tb_maximize.xpm \
 ../pixmaps/tb_split.xpm ../pixmaps/tb_minimize.xpm \
 ../pixmaps/tb_shell.xpm ../pixmaps/tb_replace.xpm \
 ../pixmaps/tb_vsplit.xpm ../pixmaps/tb_maxwidth.xpm \
 ../pixmaps/tb_minwidth.xpm
objects/gui_gtk_x11.o: gui_gtk_x11.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h auto/gui_gtk_gresources.h gui_gtk_f.h \
 ../runtime/vim32x32.xpm ../runtime/vim16x16.xpm ../runtime/vim48x48.xpm
objects/gui_x11.o: gui_x11.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h ../runtime/vim32x32.xpm \
 ../runtime/vim16x16.xpm ../runtime/vim48x48.xpm
objects/gui_at_sb.o: gui_at_sb.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h gui_at_sb.h
objects/gui_at_fs.o: gui_at_fs.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h gui_at_sb.h
objects/pty.o: pty.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h ascii.h \
 keymap.h term.h macros.h option.h structs.h regexp.h gui.h gui_beval.h \
 proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h globals.h farsi.h \
 arabic.h
objects/json_test.o: json_test.c main.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h json.c
objects/kword_test.o: kword_test.c main.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h charset.c mbyte.c
objects/memfile_test.o: memfile_test.c main.c vim.h auto/config.h feature.h \
 os_unix.h auto/osdef.h ascii.h keymap.h term.h macros.h option.h \
 structs.h regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h \
 ex_cmds.h spell.h proto.h globals.h farsi.h arabic.h memfile.c
objects/message_test.o: message_test.c main.c vim.h auto/config.h feature.h \
 os_unix.h auto/osdef.h ascii.h keymap.h term.h macros.h option.h \
 structs.h regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h \
 ex_cmds.h spell.h proto.h globals.h farsi.h arabic.h message.c
objects/hangulin.o: hangulin.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/if_lua.o: if_lua.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/if_mzsch.o: if_mzsch.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h if_mzsch.h mzscheme_base.c
objects/if_perl.o: auto/if_perl.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/if_perlsfio.o: if_perlsfio.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/if_python.o: if_python.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h if_py_both.h
objects/if_python3.o: if_python3.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h if_py_both.h
objects/if_tcl.o: if_tcl.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/if_ruby.o: if_ruby.c auto/config.h vim.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h version.h
objects/gui_beval.o: gui_beval.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h
objects/workshop.o: workshop.c auto/config.h integration.h vim.h feature.h \
 os_unix.h auto/osdef.h ascii.h keymap.h term.h macros.h option.h \
 structs.h regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h \
 ex_cmds.h spell.h proto.h globals.h farsi.h arabic.h version.h \
 workshop.h
objects/wsdebug.o: wsdebug.c
objects/integration.o: integration.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h integration.h
objects/netbeans.o: netbeans.c vim.h auto/config.h feature.h os_unix.h \
 auto/osdef.h ascii.h keymap.h term.h macros.h option.h structs.h \
 regexp.h gui.h gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h \
 proto.h globals.h farsi.h arabic.h version.h
objects/channel.o: channel.c vim.h auto/config.h feature.h os_unix.h auto/osdef.h \
 ascii.h keymap.h term.h macros.h option.h structs.h regexp.h gui.h \
 gui_beval.h proto/gui_beval.pro alloc.h ex_cmds.h spell.h proto.h \
 globals.h farsi.h arabic.h
objects/gui_gtk_gresources.o: auto/gui_gtk_gresources.c
