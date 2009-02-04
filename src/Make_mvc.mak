# Makefile for Vim on Win32 (Windows NT/2000/XP/2003 and Windows 95/98/Me)
# and Win64, using the Microsoft Visual C++ compilers. Known to work with
# VC5, VC6 (VS98), VC7.0 (VS2002), VC7.1 (VS2003), VC8 (VS2005),
# and VC9 (VS2008).
#
# To build using other Windows compilers, see INSTALLpc.txt
#
# This makefile can build the console, GUI, OLE-enable, Perl-enabled and
# Python-enabled versions of vim for Win32 platforms.
#
# The basic command line to build vim is:
#
#	nmake -f Make_mvc.mak
#
# This will build the console version of vim with no additional interfaces.
# To add features, define any of the following:
#
#	!!!!  After changing features do "nmake clean" first  !!!!
#
#       Feature Set: FEATURES=[TINY, SMALL, NORMAL, BIG, HUGE] (default is BIG)
#
#	GUI interface: GUI=yes (default is no)
#
#	OLE interface: OLE=yes (usually with GUI=yes)
#
#	Multibyte support: MBYTE=yes (default is no)
#
#	IME support: IME=yes	(requires GUI=yes)
#	  DYNAMIC_IME=[yes or no]  (to load the imm32.dll dynamically, default
#	  is yes)
#	Global IME support: GIME=yes (requires GUI=yes)
#
#	MzScheme interface:
#	  MZSCHEME=[Path to MzScheme directory]
#	  DYNAMIC_MZSCHEME=yes (to load the MzScheme DLLs dynamically)
#	  MZSCHEME_VER=[version, 205_000, ...]
#
#	Perl interface:
#	  PERL=[Path to Perl directory]
#	  DYNAMIC_PERL=yes (to load the Perl DLL dynamically)
#	  PERL_VER=[Perl version, in the form 55 (5.005), 56 (5.6.x), etc]
#	  (default is 56)
#
#	Python interface:
#	  PYTHON=[Path to Python directory]
#	  DYNAMIC_PYTHON=yes (to load the Python DLL dynamically)
#	  PYTHON_VER=[Python version, eg 15, 20]  (default is 22)
#
#	Ruby interface:
#	  RUBY=[Path to Ruby directory]
#	  DYNAMIC_RUBY=yes (to load the Ruby DLL dynamically)
#	  RUBY_VER=[Ruby version, eg 16, 17] (default is 18)
#	  RUBY_VER_LONG=[Ruby version, eg 1.6, 1.7] (default is 1.8)
#	    You must set RUBY_VER_LONG when change RUBY_VER.
#
#	Tcl interface:
#	  TCL=[Path to Tcl directory]
#	  DYNAMIC_TCL=yes (to load the Tcl DLL dynamically)
#	  TCL_VER=[Tcl version, e.g. 80, 83]  (default is 83)
#	  TCL_VER_LONG=[Tcl version, eg 8.3] (default is 8.3)
#	    You must set TCL_VER_LONG when you set TCL_VER.
#
#	SNiFF+ interface: SNIFF=yes
#
#	Cscope support: CSCOPE=yes
#
#	Iconv library support (always dynamically loaded):
#	  ICONV=[yes or no]  (default is yes)
#
#	Intl library support (always dynamically loaded):
#	  GETTEXT=[yes or no]  (default is yes)
#	See http://sourceforge.net/projects/gettext/
#
#       PostScript printing: POSTSCRIPT=yes (default is no)
#
#       Netbeans Support: NETBEANS=[yes or no] (default is yes if GUI is yes)
#
#       XPM Image Support: XPM=[path to XPM directory]
#
#       Optimization: OPTIMIZE=[SPACE, SPEED, MAXSPEED] (default is MAXSPEED)
#
#       Processor Version: CPUNR=[i386, i486, i586, i686, pentium4] (default is
#       i386)
#
#       Version Support: WINVER=[0x0400, 0x0500] (default is 0x0400)
#
#	Debug version: DEBUG=yes
#	Mapfile: MAP=[no, yes or lines] (default is yes)
#	  no:    Don't write a mapfile.
#	  yes:   Write a normal mapfile.
#	  lines: Write a mapfile with line numbers (only for VC6 and later)
#
#       Netbeans Debugging Support: NBDEBUG=[yes or no] (should be no, yes
#       doesn't work)
#
#       Visual C Version: MSVCVER=m.n (default derived from nmake if undefined)
#
# You can combine any of these interfaces
#
# Example: To build the non-debug, GUI version with Perl interface:
#	nmake -f Make_mvc.mak GUI=yes PERL=C:\Perl
#
# DEBUG with Make_mvc.mak and Make_dvc.mak:
#	This makefile gives a fineness of control which is not supported in
#	Visual C++ configuration files.  Therefore, debugging requires a bit of
#	extra work.
#	Make_dvc.mak is a Visual C++ project to access that support.  It may be
#	badly out of date for the Visual C++ you are using...
#	To use Make_dvc.mak:
#	1) Build Vim with Make_mvc.mak.
#	     Use a "DEBUG=yes" argument to build Vim with debug support.
#	     E.g. the following builds gvimd.exe:
#		nmake -f Make_mvc.mak debug=yes gui=yes
#	2) Use MS Devstudio and set it up to allow that file to be debugged:
#	    i) Pass Make_dvc.mak to the IDE.
#		 Use the "open workspace" menu entry to load Make_dvc.mak.
#		 Alternatively, from the command line:
#			msdev /nologo Make_dvc.mak
#		Note: Make_dvc.mak is in VC4.0 format. Later VC versions see
#		this and offer to convert it to their own format. Accept that.
#		It creates a file called Make_dvc.dsw which can then be used
#		for further operations.  E.g.
#		    msdev /nologo Make_dvc.dsw
#	    ii) Set the built executable for debugging:
#		a) Alt+F7/Debug takes you to the Debug dialog.
#		b) Fill "Executable for debug session". e.g. gvimd.exe
#		c) Fill "Program arguments". e.g. -R dosinst.c
#		d) Complete the dialog
#	3) You can now debug the executable you built with Make_mvc.mak
#
#	Note: Make_dvc.mak builds vimrun.exe, because it must build something
#	to be a valid makefile..

### See feature.h for a list of optionals.
# If you want to build some optional features without modifying the source,
# you can set DEFINES on the command line, e.g.,
#	nmake -f Make_mvc.mvc "DEFINES=-DEMACS_TAGS"

# Build on both Windows NT/XP and Windows 9x

TARGETOS = BOTH

# Select one of eight object code directories, depends on GUI, OLE, DEBUG and
# interfaces.
# If you change something else, do "make clean" first!
!if "$(GUI)" == "yes"
OBJDIR = .\ObjG
!else
OBJDIR = .\ObjC
!endif
!if "$(OLE)" == "yes"
OBJDIR = $(OBJDIR)O
!endif
!ifdef PERL
OBJDIR = $(OBJDIR)L
!endif
!ifdef PYTHON
OBJDIR = $(OBJDIR)Y
!endif
!ifdef TCL
OBJDIR = $(OBJDIR)T
!endif
!ifdef RUBY
OBJDIR = $(OBJDIR)R
!endif
!ifdef MZSCHEME
OBJDIR = $(OBJDIR)Z
!endif
!if "$(DEBUG)" == "yes"
OBJDIR = $(OBJDIR)d
!endif

# Win32.mak requires that CPU be set appropriately.
# To cross-compile for Win64, set CPU=AMD64 or CPU=IA64.

!ifdef PROCESSOR_ARCHITECTURE
# We're on Windows NT or using VC 6+
! ifdef CPU
ASSEMBLY_ARCHITECTURE=$(CPU)
# Using I386 for $ASSEMBLY_ARCHITECTURE doesn't work for VC7.
!  if ("$(ASSEMBLY_ARCHITECTURE)" == "i386") || ("$(ASSEMBLY_ARCHITECTURE)" == "I386")
ASSEMBLY_ARCHITECTURE = x86
!  endif
! else
CPU = $(PROCESSOR_ARCHITECTURE)
ASSEMBLY_ARCHITECTURE = $(PROCESSOR_ARCHITECTURE)
!  if ("$(CPU)" == "x86") || ("$(CPU)" == "X86")
CPU = i386
!  endif
! endif
!else  # !PROCESSOR_ARCHITECTURE
# We're on Windows 95
CPU = i386
!endif # !PROCESSOR_ARCHITECTURE

# Build a retail version by default

!if "$(DEBUG)" != "yes"
NODEBUG = 1
!else
!undef NODEBUG
MAKEFLAGS_GVIMEXT = DEBUG=yes
!endif


# Get all sorts of useful, standard macros from the Platform SDK.

!include <Win32.mak>

# Turn on Win64 compatibility warnings for VC7.x and VC8.
# (/Wp64 is deprecated in VC9 and generates an obnoxious warning.)
!if ("$(MSVCVER)" == "7.0") || ("$(MSVCVER)" == "7.1") || ("$(MSVCVER)" == "8.0") 
DEFINES=$(DEFINES) /Wp64
!endif

#>>>>> path of the compiler and linker; name of include and lib directories
# PATH = c:\msvc20\bin;$(PATH)
# INCLUDE = c:\msvc20\include
# LIB = c:\msvc20\lib

!ifndef CTAGS
CTAGS = ctags
!endif

!if "$(SNIFF)" == "yes"
# SNIFF - Include support for SNiFF+.
SNIFF_INCL  = if_sniff.h
SNIFF_OBJ   = $(OBJDIR)/if_sniff.obj
SNIFF_LIB   = shell32.lib
SNIFF_DEFS  = -DFEAT_SNIFF
# The SNiFF integration needs multithreaded libraries!
MULTITHREADED = yes
!endif

!ifndef CSCOPE
CSCOPE = yes
!endif

!if "$(CSCOPE)" == "yes"
# CSCOPE - Include support for Cscope
CSCOPE_INCL  = if_cscope.h
CSCOPE_OBJ   = $(OBJDIR)/if_cscope.obj
CSCOPE_DEFS  = -DFEAT_CSCOPE
!endif

!ifndef NETBEANS
NETBEANS = $(GUI)
!endif

# Only allow NETBEANS and XPM for a GUI build.
!if "$(GUI)" == "yes"
!if "$(NETBEANS)" == "yes"
# NETBEANS - Include support for Netbeans integration
NETBEANS_PRO	= proto/netbeans.pro
NETBEANS_OBJ	= $(OBJDIR)/netbeans.obj
NETBEANS_DEFS	= -DFEAT_NETBEANS_INTG

!if "$(NBDEBUG)" == "yes"
NBDEBUG_DEFS	= -DNBDEBUG
NBDEBUG_INCL	= nbdebug.h
NBDEBUG_SRC	= nbdebug.c
!endif
NETBEANS_LIB	= WSock32.lib
!endif

!ifdef XPM
# XPM - Include support for XPM signs
# you can get xpm.lib from http://iamphet.nm.ru/xpm or create it yourself
XPM_OBJ   = $(OBJDIR)/xpm_w32.obj
XPM_DEFS  = -DFEAT_XPM_W32
XPM_LIB   = $(XPM)\lib\libXpm.lib
XPM_INC	  = -I $(XPM)\include
!endif
!endif

# Set which version of the CRT to use
!if defined(USE_MSVCRT)
# CVARS = $(cvarsdll)
# !elseif defined(MULTITHREADED)
# CVARS = $(cvarsmt)
!else
# CVARS = $(cvars)
# CVARS = $(cvarsmt)
!endif

# need advapi32.lib for GetUserName()
# need shell32.lib for ExtractIcon()
# gdi32.lib and comdlg32.lib for printing support
# ole32.lib and uuid.lib are needed for FEAT_SHORTCUT
CON_LIB = oldnames.lib kernel32.lib advapi32.lib shell32.lib gdi32.lib \
          comdlg32.lib ole32.lib uuid.lib /machine:$(CPU) /nodefaultlib
!if "$(DELAYLOAD)" == "yes"
CON_LIB = $(CON_LIB) /DELAYLOAD:comdlg32.dll /DELAYLOAD:ole32.dll DelayImp.lib
!endif

### Set the default $(WINVER) to make it work with VC++7.0 (VS.NET)
#  When set to 0x0500 ":browse" stops working.
!ifndef WINVER
WINVER = 0x0400
!endif

# If you have a fixed directory for $VIM or $VIMRUNTIME, other than the normal
# default, use these lines.
#VIMRCLOC = somewhere
#VIMRUNTIMEDIR = somewhere

CFLAGS = -c /W3 /nologo $(CVARS) -I. -Iproto -DHAVE_PATHDEF -DWIN32 \
		$(SNIFF_DEFS) $(CSCOPE_DEFS) $(NETBEANS_DEFS) \
		$(NBDEBUG_DEFS) $(XPM_DEFS) \
		$(DEFINES) -DWINVER=$(WINVER) -D_WIN32_WINNT=$(WINVER) \
		/Fo$(OUTDIR)/ 

#>>>>> end of choices
###########################################################################

!ifdef OS
OS_TYPE	= winnt
DEL_TREE = rmdir /s /q
!else
OS_TYPE	= win95
DEL_TREE = deltree /y
!endif

INTDIR=$(OBJDIR)
OUTDIR=$(OBJDIR)

# Derive version of VC being used from nmake if not specified
!if "$(MSVCVER)" == ""
!if "$(_NMAKE_VER)" == ""
MSVCVER = 4.0
!endif
!if "$(_NMAKE_VER)" == "162"
MSVCVER = 5.0
!endif
!if "$(_NMAKE_VER)" == "6.00.8168.0"
MSVCVER = 6.0
CPU = ix86
!endif
!if "$(_NMAKE_VER)" == "7.00.9466"
MSVCVER = 7.0
!endif
!if "$(_NMAKE_VER)" == "7.10.3077"
MSVCVER = 7.1
!endif
!if "$(_NMAKE_VER)" == "8.00.50727.42"
MSVCVER = 8.0
!endif
!if "$(_NMAKE_VER)" == "8.00.50727.762"
MSVCVER = 8.0
!endif
!if "$(_NMAKE_VER)" == "9.00.20706.01"
MSVCVER = 9.0
!endif
!if "$(_NMAKE_VER)" == "9.00.21022.08"
MSVCVER = 9.0
!endif
!if "$(_NMAKE_VER)" == "9.00.30729.01"
MSVCVER = 9.0
!endif
!endif

# Abort bulding VIM if version of VC is unrecognised.
!ifndef MSVCVER
!message *** ERROR
!message Cannot determine Visual C version being used.  If you are using the
!message Windows SDK then you must have the environment variable MSVCVER set to
!message your version of the VC compiler.  If you are not using the Express
!message version of Visual C, you can either set MSVCVER or update this makefile
!message to handle the new value for _NMAKE_VER, "$(_NMAKE_VER)".
!error Make aborted.
!endif

# Convert processor ID to MVC-compatible number
!if ("$(MSVCVER)" != "8.0") && ("$(MSVCVER)" != "9.0")
!if "$(CPUNR)" == "i386"
CPUARG = /G3
!elseif "$(CPUNR)" == "i486"
CPUARG = /G4
!elseif "$(CPUNR)" == "i586"
CPUARG = /G5
!elseif "$(CPUNR)" == "i686"
CPUARG = /G6
!elseif "$(CPUNR)" == "pentium4"
CPUARG = /G7 /arch:SSE2
!else
CPUARG =
!endif
!else
# VC8/9 only allows specifying SSE architecture
!if "$(CPUNR)" == "pentium4"
CPUARG = /arch:SSE2
!endif
!endif

LIBC =
DEBUGINFO = /Zi

!ifdef NODEBUG
VIM = vim
!if "$(OPTIMIZE)" == "SPACE"
OPTFLAG = /O1
!elseif "$(OPTIMIZE)" == "SPEED"
OPTFLAG = /O2
!else # MAXSPEED
OPTFLAG = /Ox
!endif
!if ("$(MSVCVER)" == "8.0") || ("$(MSVCVER)" == "9.0")
# Use link time code generation if not worried about size
!if "$(OPTIMIZE)" != "SPACE"
OPTFLAG = $(OPTFLAG) /GL
!endif
!endif
CFLAGS = $(CFLAGS) $(OPTFLAG) -DNDEBUG $(CPUARG)
RCFLAGS = $(rcflags) $(rcvars) -DNDEBUG
! ifdef USE_MSVCRT
CFLAGS = $(CFLAGS) /MD
LIBC = msvcrt.lib
! else
LIBC = libcmt.lib
CFLAGS = $(CFLAGS) /Zl /MT
! endif
!else  # DEBUG
VIM = vimd
! if ("$(CPU)" == "i386") || ("$(CPU)" == "ix86")
DEBUGINFO = /ZI
! endif
CFLAGS = $(CFLAGS) -D_DEBUG -DDEBUG /Od
RCFLAGS = $(rcflags) $(rcvars) -D_DEBUG -DDEBUG
# The /fixed:no is needed for Quantify. Assume not 4.? as unsupported in VC4.0.
! if "$(MSVCVER)" == "4.0"
LIBC =
! else
LIBC = /fixed:no
! endif
! ifdef USE_MSVCRT
CFLAGS = $(CFLAGS) /MDd
LIBC = $(LIBC) msvcrtd.lib
! else
LIBC = $(LIBC) libcmtd.lib
CFLAGS = $(CFLAGS) /Zl /MTd
! endif
!endif # DEBUG

INCL =	vim.h os_win32.h ascii.h feature.h globals.h keymap.h macros.h \
	proto.h option.h structs.h term.h $(SNIFF_INCL) $(CSCOPE_INCL) \
	$(NBDEBUG_INCL)

OBJ = \
	$(OUTDIR)\buffer.obj \
	$(OUTDIR)\charset.obj \
	$(OUTDIR)\diff.obj \
	$(OUTDIR)\digraph.obj \
	$(OUTDIR)\edit.obj \
	$(OUTDIR)\eval.obj \
	$(OUTDIR)\ex_cmds.obj \
	$(OUTDIR)\ex_cmds2.obj \
	$(OUTDIR)\ex_docmd.obj \
	$(OUTDIR)\ex_eval.obj \
	$(OUTDIR)\ex_getln.obj \
	$(OUTDIR)\fileio.obj \
	$(OUTDIR)\fold.obj \
	$(OUTDIR)\getchar.obj \
	$(OUTDIR)\hardcopy.obj \
	$(OUTDIR)\hashtab.obj \
	$(OUTDIR)\main.obj \
	$(OUTDIR)\mark.obj \
	$(OUTDIR)\mbyte.obj \
	$(OUTDIR)\memfile.obj \
	$(OUTDIR)\memline.obj \
	$(OUTDIR)\menu.obj \
	$(OUTDIR)\message.obj \
	$(OUTDIR)\misc1.obj \
	$(OUTDIR)\misc2.obj \
	$(OUTDIR)\move.obj \
	$(OUTDIR)\normal.obj \
	$(OUTDIR)\ops.obj \
	$(OUTDIR)\option.obj \
	$(OUTDIR)\os_mswin.obj \
	$(OUTDIR)\os_win32.obj \
	$(OUTDIR)\pathdef.obj \
	$(OUTDIR)\popupmnu.obj \
	$(OUTDIR)\quickfix.obj \
	$(OUTDIR)\regexp.obj \
	$(OUTDIR)\screen.obj \
	$(OUTDIR)\search.obj \
	$(OUTDIR)\spell.obj \
	$(OUTDIR)\syntax.obj \
	$(OUTDIR)\tag.obj \
	$(OUTDIR)\term.obj \
	$(OUTDIR)\ui.obj \
	$(OUTDIR)\undo.obj \
	$(OUTDIR)\window.obj \
	$(OUTDIR)\vim.res

!if "$(OLE)" == "yes"
CFLAGS = $(CFLAGS) -DFEAT_OLE
RCFLAGS = $(RCFLAGS) -DFEAT_OLE
OLE_OBJ = $(OUTDIR)\if_ole.obj
OLE_IDL = if_ole.idl
OLE_LIB = oleaut32.lib
!endif

!if "$(IME)" == "yes"
CFLAGS = $(CFLAGS) -DFEAT_MBYTE_IME
!ifndef DYNAMIC_IME
DYNAMIC_IME = yes
!endif
!if "$(DYNAMIC_IME)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_IME
!else
IME_LIB = imm32.lib
!endif
!endif

!if "$(GIME)" == "yes"
CFLAGS = $(CFLAGS) -DGLOBAL_IME
OBJ = $(OBJ) $(OUTDIR)\dimm_i.obj $(OUTDIR)\glbl_ime.obj
MBYTE = yes
!endif

!if "$(MBYTE)" == "yes"
CFLAGS = $(CFLAGS) -DFEAT_MBYTE
!endif

!if "$(GUI)" == "yes"
SUBSYSTEM = windows
CFLAGS = $(CFLAGS) -DFEAT_GUI_W32
RCFLAGS = $(RCFLAGS) -DFEAT_GUI_W32
VIM = g$(VIM)
GUI_INCL = \
	gui.h \
	regexp.h \
	ascii.h \
	ex_cmds.h \
	farsi.h \
	feature.h \
	globals.h \
	gui_beval.h \
	keymap.h \
	macros.h \
	option.h \
	os_dos.h \
	os_win32.h
GUI_OBJ = \
	$(OUTDIR)\gui.obj \
	$(OUTDIR)\gui_beval.obj \
	$(OUTDIR)\gui_w32.obj \
	$(OUTDIR)\os_w32exe.obj
GUI_LIB = \
	gdi32.lib version.lib $(IME_LIB) \
	winspool.lib comctl32.lib advapi32.lib shell32.lib \
	/machine:$(CPU) /nodefaultlib
!else
SUBSYSTEM = console
!endif

# iconv.dll library (dynamically loaded)
!ifndef ICONV
ICONV = yes
!endif
!if "$(ICONV)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_ICONV
!endif

# libintl.dll library
!ifndef GETTEXT
GETTEXT = yes
!endif
!if "$(GETTEXT)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_GETTEXT
!endif

# TCL interface
!ifdef TCL
!ifndef TCL_VER
TCL_VER = 83
TCL_VER_LONG = 8.3
!endif
!message Tcl requested (version $(TCL_VER)) - root dir is "$(TCL)"
!if "$(DYNAMIC_TCL)" == "yes"
!message Tcl DLL will be loaded dynamically
TCL_DLL = tcl$(TCL_VER).dll
CFLAGS  = $(CFLAGS) -DFEAT_TCL -DDYNAMIC_TCL -DDYNAMIC_TCL_DLL=\"$(TCL_DLL)\" \
		-DDYNAMIC_TCL_VER=\"$(TCL_VER_LONG)\"
TCL_OBJ	= $(OUTDIR)\if_tcl.obj
TCL_INC	= /I "$(TCL)\Include" /I "$(TCL)"
TCL_LIB = $(TCL)\lib\tclstub$(TCL_VER).lib
!else
CFLAGS  = $(CFLAGS) -DFEAT_TCL
TCL_OBJ	= $(OUTDIR)\if_tcl.obj
TCL_INC	= /I "$(TCL)\Include" /I "$(TCL)"
TCL_LIB = $(TCL)\lib\tcl$(TCL_VER)vc.lib
!endif
!endif

# PYTHON interface
!ifdef PYTHON
!ifndef PYTHON_VER
PYTHON_VER = 22
!endif
!message Python requested (version $(PYTHON_VER)) - root dir is "$(PYTHON)"
!if "$(DYNAMIC_PYTHON)" == "yes"
!message Python DLL will be loaded dynamically
!endif
CFLAGS = $(CFLAGS) -DFEAT_PYTHON
PYTHON_OBJ = $(OUTDIR)\if_python.obj
PYTHON_INC = /I "$(PYTHON)\Include" /I "$(PYTHON)\PC"
!if "$(DYNAMIC_PYTHON)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_PYTHON \
		-DDYNAMIC_PYTHON_DLL=\"python$(PYTHON_VER).dll\"
PYTHON_LIB = /nodefaultlib:python$(PYTHON_VER).lib
!else
PYTHON_LIB = $(PYTHON)\libs\python$(PYTHON_VER).lib
!endif
!endif

# MzScheme interface
!ifdef MZSCHEME
!message MzScheme requested - root dir is "$(MZSCHEME)"
!ifndef MZSCHEME_VER
MZSCHEME_VER = 205_000
!endif
CFLAGS = $(CFLAGS) -DFEAT_MZSCHEME -I $(MZSCHEME)\include
!if "$(DYNAMIC_MZSCHEME)" == "yes"
!message MzScheme DLLs will be loaded dynamically
CFLAGS = $(CFLAGS) -DDYNAMIC_MZSCHEME \
		-DDYNAMIC_MZSCH_DLL=\"libmzsch$(MZSCHEME_VER).dll\" \
		-DDYNAMIC_MZGC_DLL=\"libmzgc$(MZSCHEME_VER).dll\"
!else
MZSCHEME_LIB = $(MZSCHEME)\lib\msvc\libmzgc$(MZSCHEME_VER).lib \
		$(MZSCHEME)\lib\msvc\libmzsch$(MZSCHEME_VER).lib
!endif
MZSCHEME_OBJ = $(OUTDIR)\if_mzsch.obj
!endif

# Perl interface
!ifdef PERL
!ifndef PERL_VER
PERL_VER = 56
!endif
!message Perl requested (version $(PERL_VER)) - root dir is "$(PERL)"
!if "$(DYNAMIC_PERL)" == "yes"
!if $(PERL_VER) >= 56
!message Perl DLL will be loaded dynamically
!else
!message Dynamic loading is not supported for Perl versions earlier than 5.6.0
!message Reverting to static loading...
!undef DYNAMIC_PERL
!endif
!endif

# Is Perl installed in architecture-specific directories?
!if exist($(PERL)\Bin\MSWin32-x86)
PERL_ARCH = \MSWin32-x86
!endif

PERL_INCDIR = $(PERL)\Lib$(PERL_ARCH)\Core

# Version-dependent stuff
!if $(PERL_VER) == 55
PERL_LIB = $(PERL_INCDIR)\perl.lib
!else
PERL_DLL = perl$(PERL_VER).dll
PERL_LIB = $(PERL_INCDIR)\perl$(PERL_VER).lib
!endif

CFLAGS = $(CFLAGS) -DFEAT_PERL

# Do we want to load Perl dynamically?
!if "$(DYNAMIC_PERL)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_PERL -DDYNAMIC_PERL_DLL=\"$(PERL_DLL)\"
!undef PERL_LIB
!endif

PERL_EXE = $(PERL)\Bin$(PERL_ARCH)\perl
PERL_INC = /I $(PERL_INCDIR)
PERL_OBJ = $(OUTDIR)\if_perl.obj $(OUTDIR)\if_perlsfio.obj
XSUBPP = $(PERL)\lib\ExtUtils\xsubpp
XSUBPP_TYPEMAP = $(PERL)\lib\ExtUtils\typemap

!endif

#
# Support Ruby interface
#
!ifdef RUBY
#  Set default value
!ifndef RUBY_VER
RUBY_VER = 18
!endif
!ifndef RUBY_VER_LONG
RUBY_VER_LONG = 1.8
!endif

!if $(RUBY_VER) >= 18
!ifndef RUBY_PLATFORM
RUBY_PLATFORM = i386-mswin32
!endif
!ifndef RUBY_INSTALL_NAME
RUBY_INSTALL_NAME = msvcrt-ruby$(RUBY_VER)
!endif
!else
!ifndef RUBY_PLATFORM
RUBY_PLATFORM = i586-mswin32
!endif
!ifndef RUBY_INSTALL_NAME
RUBY_INSTALL_NAME = mswin32-ruby$(RUBY_VER)
!endif
!endif # $(RUBY_VER) >= 18

!message Ruby requested (version $(RUBY_VER)) - root dir is "$(RUBY)"
CFLAGS = $(CFLAGS) -DFEAT_RUBY
RUBY_OBJ = $(OUTDIR)\if_ruby.obj
RUBY_INC = /I "$(RUBY)\lib\ruby\$(RUBY_VER_LONG)\$(RUBY_PLATFORM)"
RUBY_LIB = $(RUBY)\lib\$(RUBY_INSTALL_NAME).lib
# Do we want to load Ruby dynamically?
!if "$(DYNAMIC_RUBY)" == "yes"
!message Ruby DLL will be loaded dynamically
CFLAGS = $(CFLAGS) -DDYNAMIC_RUBY -DDYNAMIC_RUBY_VER=$(RUBY_VER) \
		-DDYNAMIC_RUBY_DLL=\"$(RUBY_INSTALL_NAME).dll\" 
!undef RUBY_LIB
!endif
!endif # RUBY

#
# Support PostScript printing
#
!if "$(POSTSCRIPT)" == "yes"
CFLAGS = $(CFLAGS) -DMSWINPS
!endif # POSTSCRIPT

#
# FEATURES: TINY, SMALL, NORMAL, BIG or HUGE
#
!if "$(FEATURES)"==""
FEATURES = BIG
!endif
CFLAGS = $(CFLAGS) -DFEAT_$(FEATURES)

#
# Always generate the .pdb file, so that we get debug symbols that can be used
# on a crash (doesn't add overhead to the executable).
# Generate edit-and-continue debug info when no optimization - allows to
# debug more conveniently (able to look at variables which are in registers)
#
CFLAGS = $(CFLAGS) /Fd$(OUTDIR)/ $(DEBUGINFO)
LINK_PDB = /PDB:$(VIM).pdb -debug

#
# End extra feature include
#
!message

conflags = /nologo /subsystem:$(SUBSYSTEM)

PATHDEF_SRC = $(OUTDIR)\pathdef.c

!IF "$(MAP)" == "yes"
# "/map" is for debugging
conflags = $(conflags) /map
!ELSEIF "$(MAP)" == "lines"
# "/mapinfo:lines" is for debugging, only works for VC6 and later
conflags = $(conflags) /map /mapinfo:lines
!ENDIF

LINKARGS1 = $(linkdebug) $(conflags)
LINKARGS2 = $(CON_LIB) $(GUI_LIB) $(LIBC) $(OLE_LIB)  user32.lib $(SNIFF_LIB) \
		$(MZSCHEME_LIB) $(PERL_LIB) $(PYTHON_LIB) $(RUBY_LIB) \
		$(TCL_LIB) $(NETBEANS_LIB) $(XPM_LIB) $(LINK_PDB)

# Report link time code generation progress if used. 
!ifdef NODEBUG
!if ("$(MSVCVER)" == "8.0") || ("$(MSVCVER)" == "9.0")
!if "$(OPTIMIZE)" != "SPACE"
LINKARGS1 = $(LINKARGS1) /LTCG:STATUS
!endif
!endif
!endif

all:	$(VIM).exe vimrun.exe install.exe uninstal.exe xxd/xxd.exe \
		GvimExt/gvimext.dll

$(VIM).exe: $(OUTDIR) $(OBJ) $(GUI_OBJ) $(OLE_OBJ) $(OLE_IDL) $(MZSCHEME_OBJ) \
		$(PERL_OBJ) $(PYTHON_OBJ) $(RUBY_OBJ) $(TCL_OBJ) $(SNIFF_OBJ) \
		$(CSCOPE_OBJ) $(NETBEANS_OBJ) $(XPM_OBJ) version.c version.h
	$(CC) $(CFLAGS) version.c
	$(link) $(LINKARGS1) -out:$(VIM).exe $(OBJ) $(GUI_OBJ) $(OLE_OBJ) \
		$(MZSCHEME_OBJ) $(PERL_OBJ) $(PYTHON_OBJ) $(RUBY_OBJ) \
		$(TCL_OBJ) $(SNIFF_OBJ) $(CSCOPE_OBJ) $(NETBEANS_OBJ) \
		$(XPM_OBJ) $(OUTDIR)\version.obj $(LINKARGS2)

$(VIM): $(VIM).exe

$(OUTDIR):
	if not exist $(OUTDIR)/nul  mkdir $(OUTDIR)

install.exe: dosinst.c
	$(CC) /nologo -DNDEBUG -DWIN32 dosinst.c kernel32.lib shell32.lib \
		ole32.lib advapi32.lib uuid.lib
	- if exist install.exe del install.exe
	ren dosinst.exe install.exe

uninstal.exe: uninstal.c
	$(CC) /nologo -DNDEBUG -DWIN32 uninstal.c shell32.lib advapi32.lib

vimrun.exe: vimrun.c
	$(CC) /nologo -DNDEBUG vimrun.c

xxd/xxd.exe: xxd/xxd.c
	cd xxd
	$(MAKE) /NOLOGO -f Make_mvc.mak
	cd ..

GvimExt/gvimext.dll: GvimExt/gvimext.cpp GvimExt/gvimext.rc GvimExt/gvimext.h
	cd GvimExt
	$(MAKE) /NOLOGO -f Makefile $(MAKEFLAGS_GVIMEXT)
	cd ..


tags: notags
	$(CTAGS) *.c *.cpp *.h if_perl.xs proto\*.pro

notags:
	- if exist tags del tags

clean:
	- if exist $(OUTDIR)/nul $(DEL_TREE) $(OUTDIR)
	- if exist *.obj del *.obj
	- if exist $(VIM).exe del $(VIM).exe
	- if exist $(VIM).ilk del $(VIM).ilk
	- if exist $(VIM).pdb del $(VIM).pdb
	- if exist $(VIM).map del $(VIM).map
	- if exist $(VIM).ncb del $(VIM).ncb
	- if exist vimrun.exe del vimrun.exe
	- if exist install.exe del install.exe
	- if exist uninstal.exe del uninstal.exe
	- if exist if_perl.c del if_perl.c
	- if exist dimm.h del dimm.h
	- if exist dimm_i.c del dimm_i.c
	- if exist dimm.tlb del dimm.tlb
	- if exist dosinst.exe del dosinst.exe
	cd xxd
	$(MAKE) /NOLOGO -f Make_mvc.mak clean
	cd ..
	cd GvimExt
	$(MAKE) /NOLOGO -f Makefile clean
	cd ..
	cd GvimExt
	$(MAKE) /NOLOGO -f Makefile clean
	cd ..
	- if exist testdir\*.out del testdir\*.out

test:
	cd testdir
	$(MAKE) /NOLOGO -f Make_dos.mak win32
	cd ..

testclean:
	cd testdir
	$(MAKE) /NOLOGO -f Make_dos.mak clean
	cd ..

###########################################################################

# Create a default rule for transforming .c files to .obj files in $(OUTDIR)
# Batch compilation is supported by nmake 1.62 (part of VS 5.0) and later)
!IF "$(MSVCVER)" == "4.0"
.c{$(OUTDIR)/}.obj:
!ELSE
.c{$(OUTDIR)/}.obj::
!ENDIF
	$(CC) $(CFLAGS) $<

# Create a default rule for transforming .cpp files to .obj files in $(OUTDIR)
# Batch compilation is supported by nmake 1.62 (part of VS 5.0) and later)
!IF "$(MSVCVER)" == "4.0"
.cpp{$(OUTDIR)/}.obj:
!ELSE
.cpp{$(OUTDIR)/}.obj::
!ENDIF
	$(CC) $(CFLAGS) $<

$(OUTDIR)/buffer.obj:	$(OUTDIR) buffer.c  $(INCL)

$(OUTDIR)/charset.obj:	$(OUTDIR) charset.c  $(INCL)

$(OUTDIR)/diff.obj:	$(OUTDIR) diff.c  $(INCL)

$(OUTDIR)/digraph.obj:	$(OUTDIR) digraph.c  $(INCL)

$(OUTDIR)/edit.obj:	$(OUTDIR) edit.c  $(INCL)

$(OUTDIR)/eval.obj:	$(OUTDIR) eval.c  $(INCL)

$(OUTDIR)/ex_cmds.obj:	$(OUTDIR) ex_cmds.c  $(INCL)

$(OUTDIR)/ex_cmds2.obj:	$(OUTDIR) ex_cmds2.c  $(INCL)

$(OUTDIR)/ex_docmd.obj:	$(OUTDIR) ex_docmd.c  $(INCL) ex_cmds.h

$(OUTDIR)/ex_eval.obj:	$(OUTDIR) ex_eval.c  $(INCL) ex_cmds.h

$(OUTDIR)/ex_getln.obj:	$(OUTDIR) ex_getln.c  $(INCL)

$(OUTDIR)/fileio.obj:	$(OUTDIR) fileio.c  $(INCL)

$(OUTDIR)/fold.obj:	$(OUTDIR) fold.c  $(INCL)

$(OUTDIR)/getchar.obj:	$(OUTDIR) getchar.c  $(INCL)

$(OUTDIR)/hardcopy.obj:	$(OUTDIR) hardcopy.c  $(INCL)

$(OUTDIR)/hashtab.obj:	$(OUTDIR) hashtab.c  $(INCL)

$(OUTDIR)/gui.obj:	$(OUTDIR) gui.c  $(INCL) $(GUI_INCL)

$(OUTDIR)/gui_beval.obj:	$(OUTDIR) gui_beval.c $(INCL) $(GUI_INCL)

$(OUTDIR)/gui_w32.obj:	$(OUTDIR) gui_w32.c gui_w48.c $(INCL) $(GUI_INCL)

$(OUTDIR)/if_cscope.obj: $(OUTDIR) if_cscope.c  $(INCL)

if_perl.c : if_perl.xs typemap
	$(PERL_EXE) $(XSUBPP) -prototypes -typemap $(XSUBPP_TYPEMAP) \
		-typemap typemap if_perl.xs > if_perl.c

$(OUTDIR)/if_perl.obj: $(OUTDIR) if_perl.c  $(INCL)
	$(CC) $(CFLAGS) $(PERL_INC) if_perl.c

$(OUTDIR)/if_perlsfio.obj: $(OUTDIR) if_perlsfio.c  $(INCL)
	$(CC) $(CFLAGS) $(PERL_INC) if_perlsfio.c

$(OUTDIR)/if_mzsch.obj: $(OUTDIR) if_mzsch.c  $(INCL)
	$(CC) $(CFLAGS) if_mzsch.c \
		-DMZSCHEME_COLLECTS=\"$(MZSCHEME:\=\\)\\collects\"

$(OUTDIR)/if_python.obj: $(OUTDIR) if_python.c  $(INCL)
	$(CC) $(CFLAGS) $(PYTHON_INC) if_python.c

$(OUTDIR)/if_ole.obj: $(OUTDIR) if_ole.cpp  $(INCL) if_ole.h

$(OUTDIR)/if_ruby.obj: $(OUTDIR) if_ruby.c  $(INCL)
	$(CC) $(CFLAGS) $(RUBY_INC) if_ruby.c

$(OUTDIR)/if_sniff.obj:	$(OUTDIR) if_sniff.c  $(INCL)
	$(CC) $(CFLAGS) if_sniff.c

$(OUTDIR)/if_tcl.obj: $(OUTDIR) if_tcl.c  $(INCL)
	$(CC) $(CFLAGS) $(TCL_INC) if_tcl.c

$(OUTDIR)/main.obj:	$(OUTDIR) main.c  $(INCL)

$(OUTDIR)/mark.obj:	$(OUTDIR) mark.c  $(INCL)

$(OUTDIR)/memfile.obj:	$(OUTDIR) memfile.c  $(INCL)

$(OUTDIR)/memline.obj:	$(OUTDIR) memline.c  $(INCL)

$(OUTDIR)/menu.obj:	$(OUTDIR) menu.c  $(INCL)

$(OUTDIR)/message.obj:	$(OUTDIR) message.c  $(INCL)

$(OUTDIR)/misc1.obj:	$(OUTDIR) misc1.c  $(INCL)

$(OUTDIR)/misc2.obj:	$(OUTDIR) misc2.c  $(INCL)

$(OUTDIR)/move.obj:	$(OUTDIR) move.c  $(INCL)

$(OUTDIR)/mbyte.obj: $(OUTDIR) mbyte.c  $(INCL)

$(OUTDIR)/netbeans.obj: $(OUTDIR) netbeans.c $(NBDEBUG_SRC) $(INCL)

$(OUTDIR)/normal.obj:	$(OUTDIR) normal.c  $(INCL)

$(OUTDIR)/option.obj:	$(OUTDIR) option.c  $(INCL)

$(OUTDIR)/ops.obj:	$(OUTDIR) ops.c  $(INCL)

$(OUTDIR)/os_mswin.obj:	$(OUTDIR) os_mswin.c  $(INCL)

$(OUTDIR)/os_win32.obj:	$(OUTDIR) os_win32.c  $(INCL) os_win32.h

$(OUTDIR)/os_w32exe.obj:	$(OUTDIR) os_w32exe.c  $(INCL)

$(OUTDIR)/pathdef.obj:	$(OUTDIR) $(PATHDEF_SRC) $(INCL)
	$(CC) $(CFLAGS) $(PATHDEF_SRC)

$(OUTDIR)/popupmnu.obj:	$(OUTDIR) popupmnu.c  $(INCL)

$(OUTDIR)/quickfix.obj:	$(OUTDIR) quickfix.c  $(INCL)

$(OUTDIR)/regexp.obj:	$(OUTDIR) regexp.c  $(INCL)

$(OUTDIR)/screen.obj:	$(OUTDIR) screen.c  $(INCL)

$(OUTDIR)/search.obj:	$(OUTDIR) search.c  $(INCL)

$(OUTDIR)/spell.obj:	$(OUTDIR) spell.c  $(INCL)

$(OUTDIR)/syntax.obj:	$(OUTDIR) syntax.c  $(INCL)

$(OUTDIR)/tag.obj:	$(OUTDIR) tag.c  $(INCL)

$(OUTDIR)/term.obj:	$(OUTDIR) term.c  $(INCL)

$(OUTDIR)/ui.obj:	$(OUTDIR) ui.c  $(INCL)

$(OUTDIR)/undo.obj:	$(OUTDIR) undo.c  $(INCL)

$(OUTDIR)/window.obj:	$(OUTDIR) window.c  $(INCL)

$(OUTDIR)/xpm_w32.obj: $(OUTDIR) xpm_w32.c
	$(CC) $(CFLAGS) $(XPM_INC) xpm_w32.c

$(OUTDIR)/vim.res:	$(OUTDIR) vim.rc version.h tools.bmp tearoff.bmp \
		vim.ico vim_error.ico vim_alert.ico vim_info.ico vim_quest.ico
	$(RC) /l 0x409 /Fo$(OUTDIR)/vim.res $(RCFLAGS) vim.rc

iid_ole.c if_ole.h vim.tlb: if_ole.idl
	midl /nologo /error none /proxy nul /iid iid_ole.c /tlb vim.tlb \
		/header if_ole.h if_ole.idl

dimm.h dimm_i.c: dimm.idl
	midl /nologo /error none /proxy nul dimm.idl

$(OUTDIR)/dimm_i.obj: $(OUTDIR) dimm_i.c $(INCL)

$(OUTDIR)/glbl_ime.obj:	$(OUTDIR) glbl_ime.cpp  dimm.h $(INCL)

# $CFLAGS may contain backslashes and double quotes, escape them both.
E0_CFLAGS = $(CFLAGS:\=\\)
E_CFLAGS = $(E0_CFLAGS:"=\")
# ") stop the string

$(PATHDEF_SRC): auto
	@echo creating $(PATHDEF_SRC)
	@echo /* pathdef.c */ > $(PATHDEF_SRC)
	@echo #include "vim.h" >> $(PATHDEF_SRC)
	@echo char_u *default_vim_dir = (char_u *)"$(VIMRCLOC:\=\\)"; >> $(PATHDEF_SRC)
	@echo char_u *default_vimruntime_dir = (char_u *)"$(VIMRUNTIMEDIR:\=\\)"; >> $(PATHDEF_SRC)
	@echo char_u *all_cflags = (char_u *)"$(CC:\=\\) $(E_CFLAGS)"; >> $(PATHDEF_SRC)
	@echo char_u *all_lflags = (char_u *)"$(link:\=\\) $(LINKARGS1:\=\\) $(LINKARGS2:\=\\)"; >> $(PATHDEF_SRC)
	@echo char_u *compiled_user = (char_u *)"$(USERNAME)"; >> $(PATHDEF_SRC)
	@echo char_u *compiled_sys = (char_u *)"$(USERDOMAIN)"; >> $(PATHDEF_SRC)

auto:
	if not exist auto/nul mkdir auto

# End Custom Build
proto.h: \
	proto/buffer.pro \
	proto/charset.pro \
	proto/diff.pro \
	proto/digraph.pro \
	proto/edit.pro \
	proto/eval.pro \
	proto/ex_cmds.pro \
	proto/ex_cmds2.pro \
	proto/ex_docmd.pro \
	proto/ex_eval.pro \
	proto/ex_getln.pro \
	proto/fileio.pro \
	proto/getchar.pro \
	proto/hardcopy.pro \
	proto/hashtab.pro \
	proto/main.pro \
	proto/mark.pro \
	proto/memfile.pro \
	proto/memline.pro \
	proto/menu.pro \
	proto/message.pro \
	proto/misc1.pro \
	proto/misc2.pro \
	proto/move.pro \
	proto/mbyte.pro \
	proto/normal.pro \
	proto/ops.pro \
	proto/option.pro \
	proto/os_mswin.pro \
	proto/os_win32.pro \
	proto/popupmnu.pro \
	proto/quickfix.pro \
	proto/regexp.pro \
	proto/screen.pro \
	proto/search.pro \
	proto/spell.pro \
	proto/syntax.pro \
	proto/tag.pro \
	proto/term.pro \
	proto/ui.pro \
	proto/undo.pro \
	proto/window.pro \
	$(NETBEANS_PRO)

.SUFFIXES: .cod .i

# Generate foo.cod (mixed source and assembly listing) from foo.c via "nmake
# foo.cod"
.c.cod:
	$(CC) $(CFLAGS) /FAcs $<

# Generate foo.i (preprocessor listing) from foo.c via "nmake foo.i"
.c.i:
	$(CC) $(CFLAGS) /P /C $<


# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=0:
