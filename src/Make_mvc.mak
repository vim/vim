# Makefile for Vim on Win32 (Windows XP/2003/Vista/7/8/10) and Win64,
# using the Microsoft Visual C++ compilers. Known to work with VC5, VC6 (VS98),
# VC7.0 (VS2002), VC7.1 (VS2003), VC8 (VS2005), VC9 (VS2008), VC10 (VS2010),
# VC11 (VS2012), VC12 (VS2013), VC14 (VS2015) and VC15 (VS2017)
#
# To build using other Windows compilers, see INSTALLpc.txt
#
# This makefile can build the console, GUI, OLE-enable, Perl-enabled and
# Python-enabled versions of Vim for Win32 platforms.
#
# The basic command line to build Vim is:
#
#	nmake -f Make_mvc.mak
#
# This will build the console version of Vim with no additional interfaces.
# To add features, define any of the following:
#
# 	For MSVC 11, if you want to include Win32.mak, you need to specify
# 	where the file is, e.g.:
# 	   SDK_INCLUDE_DIR="C:\Program Files\Microsoft SDKs\Windows\v7.1\Include"
#
#	!!!!  After changing features do "nmake clean" first  !!!!
#
#	Feature Set: FEATURES=[TINY, SMALL, NORMAL, BIG, HUGE] (default is HUGE)
#
#	GUI interface: GUI=yes (default is no)
#
#	GUI with DirectWrite (DirectX): DIRECTX=yes
#	  (default is yes if GUI=yes, requires GUI=yes and MBYTE=yes)
#
#	Color emoji support: COLOR_EMOJI=yes
#	  (default is yes if DIRECTX=yes, requires WinSDK 8.1 or later.)
#
#	OLE interface: OLE=yes (usually with GUI=yes)
#
#	Multibyte support: MBYTE=yes (default is yes for NORMAL, BIG, HUGE)
#
#	IME support: IME=yes	(requires GUI=yes)
#	  DYNAMIC_IME=[yes or no]  (to load the imm32.dll dynamically, default
#	  is yes)
#	Global IME support: GIME=yes (requires GUI=yes)
#
#       Terminal support: TERMINAL=yes (default is yes)
#
#	Lua interface:
#	  LUA=[Path to Lua directory]
#	  DYNAMIC_LUA=yes (to load the Lua DLL dynamically)
#	  LUA_VER=[Lua version]  (default is 53)
#
#	MzScheme interface:
#	  MZSCHEME=[Path to MzScheme directory]
#	  DYNAMIC_MZSCHEME=yes (to load the MzScheme DLLs dynamically)
#	  MZSCHEME_VER=[MzScheme version] (default is 3m_a0solc (6.6))
#	  	Used for the DLL file name. E.g.:
#	  	C:\Program Files (x86)\Racket\lib\libracket3m_XXXXXX.dll
#	  MZSCHEME_DEBUG=no
#
#	Perl interface:
#	  PERL=[Path to Perl directory]
#	  DYNAMIC_PERL=yes (to load the Perl DLL dynamically)
#	  PERL_VER=[Perl version, in the form 55 (5.005), 56 (5.6.x),
#		    510 (5.10.x), etc]
#	  (default is 524)
#
#	Python interface:
#	  PYTHON=[Path to Python directory]
#	  DYNAMIC_PYTHON=yes (to load the Python DLL dynamically)
#	  PYTHON_VER=[Python version, eg 22, 23, ..., 27]  (default is 27)
#
#	Python3 interface:
#	  PYTHON3=[Path to Python3 directory]
#	  DYNAMIC_PYTHON3=yes (to load the Python3 DLL dynamically)
#	  PYTHON3_VER=[Python3 version, eg 30, 31]  (default is 36)
#
#	Ruby interface:
#	  RUBY=[Path to Ruby directory]
#	  DYNAMIC_RUBY=yes (to load the Ruby DLL dynamically)
#	  RUBY_VER=[Ruby version, eg 19, 22] (default is 22)
#	  RUBY_API_VER_LONG=[Ruby API version, eg 1.8, 1.9.1, 2.2.0]
#	  		    (default is 2.2.0)
#	    You must set RUBY_API_VER_LONG when change RUBY_VER.
#	    Note: If you use Ruby 1.9.3, set as follows:
#	      RUBY_VER=19
#	      RUBY_API_VER_LONG=1.9.1 (not 1.9.3, because the API version is 1.9.1.)
#
#	Tcl interface:
#	  TCL=[Path to Tcl directory]
#	  DYNAMIC_TCL=yes (to load the Tcl DLL dynamically)
#	  TCL_VER=[Tcl version, e.g. 80, 83]  (default is 86)
#	  TCL_VER_LONG=[Tcl version, eg 8.3] (default is 8.6)
#	    You must set TCL_VER_LONG when you set TCL_VER.
#	  TCL_DLL=[Tcl dll name, e.g. tcl86.dll]  (default is tcl86.dll)
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
#	PostScript printing: POSTSCRIPT=yes (default is no)
#
#	Netbeans Support: NETBEANS=[yes or no] (default is yes if GUI is yes)
#	Requires CHANNEL.
#
#	Netbeans Debugging Support: NBDEBUG=[yes or no] (should be no, yes
#	doesn't work)
#
#	Inter process communication: CHANNEL=[yes or no] (default is yes if GUI
#	is yes)
#
#	XPM Image Support: XPM=[path to XPM directory]
#	Default is "xpm", using the files included in the distribution.
#	Use "no" to disable this feature.
#
#	Optimization: OPTIMIZE=[SPACE, SPEED, MAXSPEED] (default is MAXSPEED)
#
#	Processor Version: CPUNR=[any, i586, i686, sse, sse2, avx, avx2] (default is
#	any)
#	  avx is available on Visual C++ 2010 and after.
#	  avx2 is available on Visual C++ 2013 Update 2 and after.
#
#	Version Support: WINVER=[0x0501, 0x0502, 0x0600, 0x0601, 0x0602,
#	0x0603, 0x0A00] (default is 0x0501)
#	Supported versions depends on your target SDK, check SDKDDKVer.h
#	See https://docs.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt
#
#	Debug version: DEBUG=yes
#	Mapfile: MAP=[no, yes or lines] (default is yes)
#	  no:    Don't write a mapfile.
#	  yes:   Write a normal mapfile.
#	  lines: Write a mapfile with line numbers (only for VC6 and later)
#
#	Static Code Analysis: ANALYZE=yes (works with VS2012 or later)
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

# Build on Windows NT/XP

TARGETOS = WINNT

!ifndef DIRECTX
DIRECTX = $(GUI)
!endif

# Select one of eight object code directories, depends on GUI, OLE, DEBUG and
# interfaces.
# If you change something else, do "make clean" first!
!if "$(GUI)" == "yes"
OBJDIR = .\ObjG
!else
OBJDIR = .\ObjC
!endif
!if "$(DIRECTX)" == "yes"
OBJDIR = $(OBJDIR)X
!endif
!if "$(OLE)" == "yes"
OBJDIR = $(OBJDIR)O
!endif
!ifdef LUA
OBJDIR = $(OBJDIR)U
!endif
!ifdef PERL
OBJDIR = $(OBJDIR)L
!endif
!ifdef PYTHON
OBJDIR = $(OBJDIR)Y
!endif
!ifdef PYTHON3
OBJDIR = $(OBJDIR)H
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

# If you include Win32.mak, it requires that CPU be set appropriately.
# To cross-compile for Win64, set CPU=AMD64 or CPU=IA64.

!ifdef PROCESSOR_ARCHITECTURE
# We're on Windows NT or using VC 6+
! ifdef CPU
ASSEMBLY_ARCHITECTURE=$(CPU)
# Using I386 for $ASSEMBLY_ARCHITECTURE doesn't work for VC7.
!  if "$(CPU)" == "I386"
CPU = i386
!  endif
! else  # !CPU
CPU = i386
!  if !defined(PLATFORM) && defined(TARGET_CPU)
PLATFORM = $(TARGET_CPU)
!  endif
!  ifdef PLATFORM
!   if ("$(PLATFORM)" == "x64") || ("$(PLATFORM)" == "X64")
CPU = AMD64
!   elseif ("$(PLATFORM)" != "x86") && ("$(PLATFORM)" != "X86")
!    error *** ERROR Unknown target platform "$(PLATFORM)". Make aborted.
!   endif
!  endif  # !PLATFORM
! endif
!else  # !PROCESSOR_ARCHITECTURE
# We're on Windows 95
CPU = i386
!endif # !PROCESSOR_ARCHITECTURE
ASSEMBLY_ARCHITECTURE=$(CPU)
OBJDIR = $(OBJDIR)$(CPU)

# Build a retail version by default

!if "$(DEBUG)" != "yes"
NODEBUG = 1
!else
!undef NODEBUG
MAKEFLAGS_GVIMEXT = DEBUG=yes
!endif


# Get all sorts of useful, standard macros from the Platform SDK,
# if SDK_INCLUDE_DIR is set or USE_WIN32MAK is set to "yes".

!ifdef SDK_INCLUDE_DIR
!include $(SDK_INCLUDE_DIR)\Win32.mak
!elseif "$(USE_WIN32MAK)"=="yes"
!include <Win32.mak>
!else
link = link
!endif


# Check VC version.
!if [echo MSVCVER=_MSC_VER> msvcver.c && $(CC) /EP msvcver.c > msvcver.~ 2> nul]
!message *** ERROR
!message Cannot run Visual C to determine its version. Make sure cl.exe is in your PATH.
!message This can usually be done by running "vcvarsall.bat", located in the bin directory where Visual Studio was installed.
!error Make aborted.
!else
!include msvcver.~
!if [del msvcver.c msvcver.~]
!endif
!endif

!if $(MSVCVER) < 1900
MSVC_MAJOR = ($(MSVCVER) / 100 - 6)
MSVCRT_VER = ($(MSVCVER) / 10 - 60)
# Visual C++ 2017 needs special handling
# it has an _MSC_VER of 1910->14.1, but is actually v15 with runtime v140
# TODO: what's the maximum value?
!elseif $(MSVCVER) >= 1910
MSVC_MAJOR = 15
MSVCRT_VER = 140
!else
MSVC_MAJOR = ($(MSVCVER) / 100 - 5)
MSVCRT_VER = ($(MSVCVER) / 10 - 50)
!endif

# Calculate MSVC_FULL for Visual C++ 8 and up.
!if $(MSVC_MAJOR) >= 8
! if [echo MSVC_FULL=_MSC_FULL_VER> msvcfullver.c && $(CC) /EP msvcfullver.c > msvcfullver.~ 2> nul]
!  message *** ERROR
!  message Cannot run Visual C to determine its version. Make sure cl.exe is in your PATH.
!  message This can usually be done by running "vcvarsall.bat", located in the bin directory where Visual Studio was installed.
!  error Make aborted.
! else
!  include msvcfullver.~
!  if [del msvcfullver.c msvcfullver.~]
!  endif
! endif
!endif


# Calculate MSVCRT_VER
!if [(set /a MSVCRT_VER="$(MSVCRT_VER)" > nul) && set MSVCRT_VER > msvcrtver.~] == 0
!include msvcrtver.~
!if [del msvcrtver.~]
!endif
!endif

# Base name of the msvcrXX.dll
!if $(MSVCRT_VER) <= 60
MSVCRT_NAME = msvcrt
!elseif $(MSVCRT_VER) <= 130
MSVCRT_NAME = msvcr$(MSVCRT_VER)
!else
MSVCRT_NAME = vcruntime$(MSVCRT_VER)
!endif

!if $(MSVC_MAJOR) == 6
CPU = ix86
!endif


# Flag to turn on Win64 compatibility warnings for VC7.x and VC8.
WP64CHECK = /Wp64

# Use multiprocess build
USE_MP = yes

#>>>>> path of the compiler and linker; name of include and lib directories
# PATH = c:\msvc20\bin;$(PATH)
# INCLUDE = c:\msvc20\include
# LIB = c:\msvc20\lib

!if "$(FEATURES)"==""
FEATURES = HUGE
!endif

!ifndef CTAGS
# this assumes ctags is Exuberant ctags
CTAGS = ctags -I INIT+ --fields=+S
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

!ifndef TERMINAL
!if "$(FEATURES)"=="HUGE"
TERMINAL = yes
!else
TERMINAL = no
!endif
!endif

!if "$(TERMINAL)" == "yes"
TERM_OBJ = \
	$(OBJDIR)/terminal.obj \
	$(OBJDIR)/term_encoding.obj \
	$(OBJDIR)/term_keyboard.obj \
	$(OBJDIR)/term_mouse.obj \
	$(OBJDIR)/term_parser.obj \
	$(OBJDIR)/term_pen.obj \
	$(OBJDIR)/term_screen.obj \
	$(OBJDIR)/term_state.obj \
	$(OBJDIR)/term_unicode.obj \
	$(OBJDIR)/term_vterm.obj
TERM_DEFS = -DFEAT_TERMINAL
TERM_DEPS = \
	libvterm/include/vterm.h \
	libvterm/include/vterm_keycodes.h \
	libvterm/src/rect.h \
	libvterm/src/utf8.h \
	libvterm/src/vterm_internal.h
!endif

!ifndef NETBEANS
NETBEANS = $(GUI)
!endif

!ifndef CHANNEL
!if "$(FEATURES)"=="HUGE"
CHANNEL = yes
!else
CHANNEL = $(GUI)
!endif
!endif

# GUI sepcific features.
!if "$(GUI)" == "yes"
# Only allow NETBEANS for a GUI build and CHANNEL.
!if "$(NETBEANS)" == "yes" && "$(CHANNEL)" == "yes"
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

# DirectWrite (DirectX)
!if "$(DIRECTX)" == "yes"
DIRECTX_DEFS	= -DFEAT_DIRECTX -DDYNAMIC_DIRECTX
!if "$(COLOR_EMOJI)" != "no"
DIRECTX_DEFS	= $(DIRECTX_DEFS) -DFEAT_DIRECTX_COLOR_EMOJI
!endif
DIRECTX_INCL	= gui_dwrite.h
DIRECTX_OBJ	= $(OUTDIR)\gui_dwrite.obj
!endif

# Only allow XPM for a GUI build.
!ifndef XPM
!ifndef USE_MSVCRT
# Both XPM and USE_MSVCRT are not set, use the included xpm files, depending
# on the architecture.
!if "$(CPU)" == "AMD64"
XPM = xpm\x64
!elseif "$(CPU)" == "i386"
XPM = xpm\x86
!else
XPM = no
!endif
!else # USE_MSVCRT
XPM = no
!endif # USE_MSVCRT
!endif # XPM
!if "$(XPM)" != "no"
# XPM - Include support for XPM signs
# See the xpm directory for more information.
XPM_OBJ   = $(OBJDIR)/xpm_w32.obj
XPM_DEFS  = -DFEAT_XPM_W32
!if $(MSVC_MAJOR) >= 14
# VC14 cannot use a library built by VC12 or eariler, because VC14 uses
# Universal CRT.
XPM_LIB   = $(XPM)\lib-vc14\libXpm.lib
!else
XPM_LIB   = $(XPM)\lib\libXpm.lib
!endif
XPM_INC	  = -I $(XPM)\include -I $(XPM)\..\include
!endif
!endif

!if "$(CHANNEL)" == "yes"
CHANNEL_PRO	= proto/channel.pro
CHANNEL_OBJ	= $(OBJDIR)/channel.obj
CHANNEL_DEFS	= -DFEAT_JOB_CHANNEL

NETBEANS_LIB	= WSock32.lib
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
# need netapi32.lib for NetUserEnum()
# gdi32.lib and comdlg32.lib for printing support
# ole32.lib and uuid.lib are needed for FEAT_SHORTCUT
CON_LIB = oldnames.lib kernel32.lib advapi32.lib shell32.lib gdi32.lib \
          comdlg32.lib ole32.lib netapi32.lib uuid.lib /machine:$(CPU)
!if "$(DELAYLOAD)" == "yes"
CON_LIB = $(CON_LIB) /DELAYLOAD:comdlg32.dll /DELAYLOAD:ole32.dll DelayImp.lib
!endif

### Set the default $(WINVER) to make it work with VC++7.0 (VS.NET)
!ifndef WINVER
WINVER = 0x0501
!endif

# If you have a fixed directory for $VIM or $VIMRUNTIME, other than the normal
# default, use these lines.
#VIMRCLOC = somewhere
#VIMRUNTIMEDIR = somewhere

CFLAGS = -c /W3 /nologo $(CVARS) -I. -Iproto -DHAVE_PATHDEF -DWIN32 \
		$(CSCOPE_DEFS) $(TERM_DEFS) $(NETBEANS_DEFS) $(CHANNEL_DEFS) \
		$(NBDEBUG_DEFS) $(XPM_DEFS) \
		$(DEFINES) -DWINVER=$(WINVER) -D_WIN32_WINNT=$(WINVER)

#>>>>> end of choices
###########################################################################

DEL_TREE = rmdir /s /q

INTDIR=$(OBJDIR)
OUTDIR=$(OBJDIR)

### Validate CPUNR
!ifndef CPUNR
# default to untargeted code
CPUNR = any
!elseif "$(CPUNR)" == "i386" || "$(CPUNR)" == "i486"
# alias i386 and i486 to i586
! message *** WARNING CPUNR=$(CPUNR) is not a valid target architecture.
! message Windows XP is the minimum target OS, with a minimum target
! message architecture of i586.
! message Retargeting to i586
CPUNR = i586
!elseif "$(CPUNR)" == "pentium4"
# alias pentium4 to sse2
! message *** WARNING CPUNR=pentium4 is deprecated in favour of sse2.
! message Retargeting to sse2.
CPUNR = sse2
!elseif "$(CPUNR)" != "any" && "$(CPUNR)" != "i586" && "$(CPUNR)" != "i686" && "$(CPUNR)" != "sse" && "$(CPUNR)" != "sse2" && "$(CPUNR)" != "avx" && "$(CPUNR)" != "avx2"
! error *** ERROR Unknown target architecture "$(CPUNR)". Make aborted.
!endif

# Convert processor ID to MVC-compatible number
!if $(MSVC_MAJOR) < 8
! if "$(CPUNR)" == "i586"
CPUARG = /G5
! elseif "$(CPUNR)" == "i686"
CPUARG = /G6
! elseif "$(CPUNR)" == "sse"
CPUARG = /G6 /arch:SSE
! elseif "$(CPUNR)" == "sse2"
CPUARG = /G7 /arch:SSE2
! elseif "$(CPUNR)" == "avx" || "$(CPUNR)" == "avx2"
!  message AVX/AVX2 Instruction Sets are not supported by Visual C++ v$(MSVC_MAJOR)
!  message Falling back to SSE2
CPUARG = /G7 /arch:SSE2
! elseif "$(CPUNR)" == "any"
CPUARG =
! endif
!else
# IA32/SSE/SSE2 are only supported on x86
! if "$(ASSEMBLY_ARCHITECTURE)" == "i386" && ("$(CPUNR)" == "i586" || "$(CPUNR)" == "i686" || "$(CPUNR)" == "any")
# VC<11 generates fp87 code by default
!  if $(MSVC_MAJOR) < 11
CPUARG =
# VC>=11 needs explicit insturctions to generate fp87 code
!  else
CPUARG = /arch:IA32
!  endif
! elseif "$(ASSEMBLY_ARCHITECTURE)" == "i386" && "$(CPUNR)" == "sse"
CPUARG = /arch:SSE
! elseif "$(ASSEMBLY_ARCHITECTURE)" == "i386" && "$(CPUNR)" == "sse2"
CPUARG = /arch:SSE2
! elseif "$(CPUNR)" == "avx"
# AVX is only supported by VC 10 and up
!  if $(MSVC_MAJOR) < 10
!   message AVX Instruction Set is not supported by Visual C++ v$(MSVC_MAJOR)
!   if "$(ASSEMBLY_ARCHITECTURE)" == "i386"
!    message Falling back to SSE2
CPUARG = /arch:SSE2
!   else
CPUARG =
!   endif
!  else
CPUARG = /arch:AVX
!  endif
! elseif "$(CPUNR)" == "avx2"
# AVX is only supported by VC 10 and up
!  if $(MSVC_MAJOR) < 10
!   message AVX2 Instruction Set is not supported by Visual C++ v$(MSVC_MAJOR)
!   if "$(ASSEMBLY_ARCHITECTURE)" == "i386"
!    message Falling back to SSE2
CPUARG = /arch:SSE2
!   else
CPUARG =
!   endif
# AVX2 is only supported by VC 12U2 and up
# 180030501 is the full version number for Visual Studio 2013/VC 12 Update 2
!  elseif $(MSVC_FULL) < 180030501
!   message AVX2 Instruction Set is not supported by Visual C++ v$(MSVC_MAJOR)-$(MSVC_FULL)
!   message Falling back to AVX
CPUARG = /arch:AVX
!  else
CPUARG = /arch:AVX2
!  endif
! endif
!endif

# Pass CPUARG to GvimExt, to avoid using version-dependent defaults
MAKEFLAGS_GVIMEXT = $(MAKEFLAGS_GVIMEXT) CPUARG="$(CPUARG)"


LIBC =
DEBUGINFO = /Zi

# Don't use /nodefaultlib on MSVC 14
!if $(MSVC_MAJOR) >= 14
NODEFAULTLIB =
!else
NODEFAULTLIB = /nodefaultlib
!endif

# Use multiprocess build on MSVC 10
!if "$(USE_MP)"=="yes"
!if $(MSVC_MAJOR) >= 10
CFLAGS = $(CFLAGS) /MP
!endif
!endif


!ifdef NODEBUG
VIM = vim
!if "$(OPTIMIZE)" == "SPACE"
OPTFLAG = /O1
!elseif "$(OPTIMIZE)" == "SPEED"
OPTFLAG = /O2
!else # MAXSPEED
OPTFLAG = /Ox
!endif

!if $(MSVC_MAJOR) >= 8
# Use link time code generation if not worried about size
!if "$(OPTIMIZE)" != "SPACE"
OPTFLAG = $(OPTFLAG) /GL
!endif
!endif

# (/Wp64 is deprecated in VC9 and generates an obnoxious warning.)
!if ($(MSVC_MAJOR) == 7) || ($(MSVC_MAJOR) == 8)
CFLAGS=$(CFLAGS) $(WP64CHECK)
!endif

# VC10 or later has stdint.h.
!if $(MSVC_MAJOR) >= 10
CFLAGS = $(CFLAGS) -DHAVE_STDINT_H
!endif

# Static code analysis generally available starting with VS2012 (VC11) or
# Windows SDK 7.1 (VC10)
!if ("$(ANALYZE)" == "yes") && ($(MSVC_MAJOR) >= 10)
CFLAGS=$(CFLAGS) /analyze
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
! if $(MSVC_MAJOR) == 4
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

INCL =	vim.h alloc.h arabic.h ascii.h ex_cmds.h farsi.h feature.h globals.h \
	keymap.h macros.h option.h os_dos.h os_win32.h proto.h regexp.h \
	spell.h structs.h term.h beval.h $(NBDEBUG_INCL)

OBJ = \
	$(OUTDIR)\arabic.obj \
	$(OUTDIR)\beval.obj \
	$(OUTDIR)\blowfish.obj \
	$(OUTDIR)\buffer.obj \
	$(OUTDIR)\charset.obj \
	$(OUTDIR)\crypt.obj \
	$(OUTDIR)\crypt_zip.obj \
	$(OUTDIR)\dict.obj \
	$(OUTDIR)\diff.obj \
	$(OUTDIR)\digraph.obj \
	$(OUTDIR)\edit.obj \
	$(OUTDIR)\eval.obj \
	$(OUTDIR)\evalfunc.obj \
	$(OUTDIR)\ex_cmds.obj \
	$(OUTDIR)\ex_cmds2.obj \
	$(OUTDIR)\ex_docmd.obj \
	$(OUTDIR)\ex_eval.obj \
	$(OUTDIR)\ex_getln.obj \
	$(OUTDIR)\farsi.obj \
	$(OUTDIR)\fileio.obj \
	$(OUTDIR)\fold.obj \
	$(OUTDIR)\getchar.obj \
	$(OUTDIR)\hardcopy.obj \
	$(OUTDIR)\hashtab.obj \
	$(OUTDIR)\json.obj \
	$(OUTDIR)\list.obj \
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
	$(OUTDIR)\winclip.obj \
	$(OUTDIR)\os_win32.obj \
	$(OUTDIR)\pathdef.obj \
	$(OUTDIR)\popupmnu.obj \
	$(OUTDIR)\quickfix.obj \
	$(OUTDIR)\regexp.obj \
	$(OUTDIR)\screen.obj \
	$(OUTDIR)\search.obj \
	$(OUTDIR)\sha256.obj \
	$(OUTDIR)\spell.obj \
	$(OUTDIR)\spellfile.obj \
	$(OUTDIR)\syntax.obj \
	$(OUTDIR)\tag.obj \
	$(OUTDIR)\term.obj \
	$(OUTDIR)\ui.obj \
	$(OUTDIR)\undo.obj \
	$(OUTDIR)\userfunc.obj \
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
	gui.h
GUI_OBJ = \
	$(OUTDIR)\gui.obj \
	$(OUTDIR)\gui_beval.obj \
	$(OUTDIR)\gui_w32.obj \
	$(OUTDIR)\os_w32exe.obj
GUI_LIB = \
	gdi32.lib version.lib $(IME_LIB) \
	winspool.lib comctl32.lib advapi32.lib shell32.lib netapi32.lib \
	/machine:$(CPU)
!else
SUBSYSTEM = console
CUI_INCL = iscygpty.h
CUI_OBJ = $(OUTDIR)\iscygpty.obj
!endif
SUBSYSTEM_TOOLS = console

!if "$(SUBSYSTEM_VER)" != ""
SUBSYSTEM = $(SUBSYSTEM),$(SUBSYSTEM_VER)
SUBSYSTEM_TOOLS = $(SUBSYSTEM_TOOLS),$(SUBSYSTEM_VER)
# Pass SUBSYSTEM_VER to GvimExt and other tools
MAKEFLAGS_GVIMEXT = $(MAKEFLAGS_GVIMEXT) SUBSYSTEM_VER=$(SUBSYSTEM_VER)
MAKEFLAGS_TOOLS = $(MAKEFLAGS_TOOLS) SUBSYSTEM_VER=$(SUBSYSTEM_VER)
!endif

!if "$(GUI)" == "yes" && "$(DIRECTX)" == "yes"
CFLAGS = $(CFLAGS) $(DIRECTX_DEFS)
GUI_INCL = $(GUI_INCL) $(DIRECTX_INCL)
GUI_OBJ = $(GUI_OBJ) $(DIRECTX_OBJ)
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
TCL_VER = 86
TCL_VER_LONG = 8.6
!endif
!message Tcl requested (version $(TCL_VER)) - root dir is "$(TCL)"
!if "$(DYNAMIC_TCL)" == "yes"
!message Tcl DLL will be loaded dynamically
!ifndef TCL_DLL
TCL_DLL = tcl$(TCL_VER).dll
!endif
CFLAGS  = $(CFLAGS) -DFEAT_TCL -DDYNAMIC_TCL -DDYNAMIC_TCL_DLL=\"$(TCL_DLL)\" \
		-DDYNAMIC_TCL_VER=\"$(TCL_VER_LONG)\"
TCL_OBJ	= $(OUTDIR)\if_tcl.obj
TCL_INC	= /I "$(TCL)\Include" /I "$(TCL)"
TCL_LIB = "$(TCL)\lib\tclstub$(TCL_VER).lib"
!else
CFLAGS  = $(CFLAGS) -DFEAT_TCL
TCL_OBJ	= $(OUTDIR)\if_tcl.obj
TCL_INC	= /I "$(TCL)\Include" /I "$(TCL)"
TCL_LIB = $(TCL)\lib\tcl$(TCL_VER)vc.lib
!endif
!endif

# Lua interface
!ifdef LUA
!ifndef LUA_VER
LUA_VER = 53
!endif
!message Lua requested (version $(LUA_VER)) - root dir is "$(LUA)"
!if "$(DYNAMIC_LUA)" == "yes"
!message Lua DLL will be loaded dynamically
!endif
CFLAGS = $(CFLAGS) -DFEAT_LUA
LUA_OBJ = $(OUTDIR)\if_lua.obj
LUA_INC = /I "$(LUA)\include" /I "$(LUA)"
!if "$(DYNAMIC_LUA)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_LUA \
		-DDYNAMIC_LUA_DLL=\"lua$(LUA_VER).dll\"
LUA_LIB = /nodefaultlib:lua$(LUA_VER).lib
!else
LUA_LIB = "$(LUA)\lib\lua$(LUA_VER).lib"
!endif
!endif

!ifdef PYTHON
!ifdef PYTHON3
DYNAMIC_PYTHON=yes
DYNAMIC_PYTHON3=yes
!endif
!endif

# PYTHON interface
!ifdef PYTHON
!ifndef PYTHON_VER
PYTHON_VER = 27
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

# PYTHON3 interface
!ifdef PYTHON3
!ifndef PYTHON3_VER
PYTHON3_VER = 36
!endif
!message Python3 requested (version $(PYTHON3_VER)) - root dir is "$(PYTHON3)"
!if "$(DYNAMIC_PYTHON3)" == "yes"
!message Python3 DLL will be loaded dynamically
!endif
CFLAGS = $(CFLAGS) -DFEAT_PYTHON3
PYTHON3_OBJ = $(OUTDIR)\if_python3.obj
PYTHON3_INC = /I "$(PYTHON3)\Include" /I "$(PYTHON3)\PC"
!if "$(DYNAMIC_PYTHON3)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_PYTHON3 \
		-DDYNAMIC_PYTHON3_DLL=\"python$(PYTHON3_VER).dll\"
PYTHON3_LIB = /nodefaultlib:python$(PYTHON3_VER).lib
!else
PYTHON3_LIB = $(PYTHON3)\libs\python$(PYTHON3_VER).lib
!endif
!endif

# MzScheme interface
!ifdef MZSCHEME
!message MzScheme requested - root dir is "$(MZSCHEME)"
!ifndef MZSCHEME_VER
MZSCHEME_VER = 3m_a0solc
!endif
!ifndef MZSCHEME_COLLECTS
MZSCHEME_COLLECTS=$(MZSCHEME)\collects
!endif
CFLAGS = $(CFLAGS) -DFEAT_MZSCHEME -I "$(MZSCHEME)\include"
!if EXIST("$(MZSCHEME)\lib\msvc\libmzsch$(MZSCHEME_VER).lib")
MZSCHEME_MAIN_LIB=mzsch
!else
MZSCHEME_MAIN_LIB=racket
!endif
!if (EXIST("$(MZSCHEME)\lib\lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).dll") \
     && !EXIST("$(MZSCHEME)\lib\libmzgc$(MZSCHEME_VER).dll")) \
    || (EXIST("$(MZSCHEME)\lib\msvc\lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).lib") \
        && !EXIST("$(MZSCHEME)\lib\msvc\libmzgc$(MZSCHEME_VER).lib"))
!message Building with Precise GC
MZSCHEME_PRECISE_GC = yes
CFLAGS = $(CFLAGS) -DMZ_PRECISE_GC
!endif
!if "$(DYNAMIC_MZSCHEME)" == "yes"
!message MzScheme DLLs will be loaded dynamically
CFLAGS = $(CFLAGS) -DDYNAMIC_MZSCHEME
!if "$(MZSCHEME_PRECISE_GC)" == "yes"
# Precise GC does not use separate dll
CFLAGS = $(CFLAGS) \
	 -DDYNAMIC_MZSCH_DLL=\"lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).dll\" \
	 -DDYNAMIC_MZGC_DLL=\"lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).dll\"
!else
CFLAGS = $(CFLAGS) \
	 -DDYNAMIC_MZSCH_DLL=\"lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).dll\" \
	 -DDYNAMIC_MZGC_DLL=\"libmzgc$(MZSCHEME_VER).dll\"
!endif
!else
!if "$(MZSCHEME_DEBUG)" == "yes"
CFLAGS = $(CFLAGS) -DMZSCHEME_FORCE_GC
!endif
!if "$(MZSCHEME_PRECISE_GC)" == "yes"
# Precise GC does not use separate dll
!if EXIST("$(MZSCHEME)\lib\lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).def")
# create .lib from .def
MZSCHEME_LIB = lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).lib
MZSCHEME_EXTRA_DEP = lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).lib
!else
MZSCHEME_LIB = "$(MZSCHEME)\lib\msvc\lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).lib"
!endif
!else
MZSCHEME_LIB = "$(MZSCHEME)\lib\msvc\libmzgc$(MZSCHEME_VER).lib" \
		"$(MZSCHEME)\lib\msvc\lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).lib"
!endif
!endif
MZSCHEME_OBJ = $(OUTDIR)\if_mzsch.obj
# increase stack size
MZSCHEME_LIB = $(MZSCHEME_LIB) /STACK:8388608
MZSCHEME_INCL = if_mzsch.h
!endif

# Perl interface
!ifdef PERL
!ifndef PERL_VER
PERL_VER = 524
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
!if exist($(PERL_INCDIR)\perl$(PERL_VER).lib)
PERL_LIB = $(PERL_INCDIR)\perl$(PERL_VER).lib
!else
# For ActivePerl 5.18 and later
PERL_LIB = $(PERL_INCDIR)\libperl$(PERL_VER).a
!endif
!endif

CFLAGS = $(CFLAGS) -DFEAT_PERL -DPERL_IMPLICIT_CONTEXT -DPERL_IMPLICIT_SYS

# Do we want to load Perl dynamically?
!if "$(DYNAMIC_PERL)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_PERL -DDYNAMIC_PERL_DLL=\"$(PERL_DLL)\"
!undef PERL_LIB
!endif

PERL_EXE = $(PERL)\Bin$(PERL_ARCH)\perl
PERL_INC = /I $(PERL_INCDIR)
!if $(MSVC_MAJOR) <= 11
# ActivePerl 5.20+ requires stdbool.h but VC2012 or earlier doesn't have it.
# Use a stub stdbool.h.
PERL_INC = $(PERL_INC) /I if_perl_msvc
!endif
PERL_OBJ = $(OUTDIR)\if_perl.obj $(OUTDIR)\if_perlsfio.obj
XSUBPP = $(PERL)\lib\ExtUtils\xsubpp
!if exist($(XSUBPP))
XSUBPP = $(PERL_EXE) $(XSUBPP)
!else
XSUBPP = xsubpp
!endif
XSUBPP_TYPEMAP = $(PERL)\lib\ExtUtils\typemap

!endif

#
# Support Ruby interface
#
!ifdef RUBY
#  Set default value
!ifndef RUBY_VER
RUBY_VER = 22
!endif
!ifndef RUBY_VER_LONG
RUBY_VER_LONG = 2.2.0
!endif
!ifndef RUBY_API_VER_LONG
RUBY_API_VER_LONG = $(RUBY_VER_LONG)
!endif
!ifndef RUBY_API_VER
RUBY_API_VER = $(RUBY_API_VER_LONG:.=)
!endif

!if $(RUBY_VER) >= 18

!ifndef RUBY_PLATFORM
!if "$(CPU)" == "i386"
RUBY_PLATFORM = i386-mswin32
!else # CPU
RUBY_PLATFORM = x64-mswin64
!endif # CPU
!if $(MSVCRT_VER) >= 70 && $(RUBY_VER) > 19
RUBY_PLATFORM = $(RUBY_PLATFORM)_$(MSVCRT_VER)
!endif # MSVCRT_VER
!endif # RUBY_PLATFORM

!ifndef RUBY_INSTALL_NAME
!ifndef RUBY_MSVCRT_NAME
# Base name of msvcrXX.dll which is used by ruby's dll.
RUBY_MSVCRT_NAME = $(MSVCRT_NAME)
!endif # RUBY_MSVCRT_NAME
!if "$(CPU)" == "i386"
RUBY_INSTALL_NAME = $(RUBY_MSVCRT_NAME)-ruby$(RUBY_API_VER)
!else # CPU
RUBY_INSTALL_NAME = x64-$(RUBY_MSVCRT_NAME)-ruby$(RUBY_API_VER)
!endif # CPU
!endif # RUBY_INSTALL_NAME

!else # $(RUBY_VER) >= 18

!ifndef RUBY_PLATFORM
RUBY_PLATFORM = i586-mswin32
!endif
!ifndef RUBY_INSTALL_NAME
RUBY_INSTALL_NAME = mswin32-ruby$(RUBY_API_VER)
!endif

!endif # $(RUBY_VER) >= 18

!message Ruby requested (version $(RUBY_VER)) - root dir is "$(RUBY)"
CFLAGS = $(CFLAGS) -DFEAT_RUBY
RUBY_OBJ = $(OUTDIR)\if_ruby.obj
!if $(RUBY_VER) >= 19
RUBY_INC = /I "$(RUBY)\lib\ruby\$(RUBY_API_VER_LONG)\$(RUBY_PLATFORM)" /I "$(RUBY)\include\ruby-$(RUBY_API_VER_LONG)" /I "$(RUBY)\include\ruby-$(RUBY_API_VER_LONG)\$(RUBY_PLATFORM)"
!else
RUBY_INC = /I "$(RUBY)\lib\ruby\$(RUBY_API_VER_LONG)\$(RUBY_PLATFORM)"
!endif
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

# CFLAGS with /Fo$(OUTDIR)/
CFLAGS_OUTDIR=$(CFLAGS) /Fo$(OUTDIR)/

# Add /opt:ref to remove unreferenced functions and data even when /DEBUG is
# added.
conflags = /nologo /subsystem:$(SUBSYSTEM) /opt:ref

PATHDEF_SRC = $(OUTDIR)\pathdef.c

!IF "$(MAP)" == "yes"
# "/map" is for debugging
conflags = $(conflags) /map
!ELSEIF "$(MAP)" == "lines"
# "/mapinfo:lines" is for debugging, only works for VC6 and later
conflags = $(conflags) /map /mapinfo:lines
!ENDIF

LINKARGS1 = $(linkdebug) $(conflags)
LINKARGS2 = $(CON_LIB) $(GUI_LIB) $(NODEFAULTLIB) $(LIBC) $(OLE_LIB) user32.lib \
		$(LUA_LIB) $(MZSCHEME_LIB) $(PERL_LIB) $(PYTHON_LIB) $(PYTHON3_LIB) $(RUBY_LIB) \
		$(TCL_LIB) $(NETBEANS_LIB) $(XPM_LIB) $(LINK_PDB)

# Report link time code generation progress if used. 
!ifdef NODEBUG
!if $(MSVC_MAJOR) >= 8
!if "$(OPTIMIZE)" != "SPACE"
LINKARGS1 = $(LINKARGS1) /LTCG:STATUS
!endif
!endif
!endif

!if $(MSVC_MAJOR) >= 11 && "$(CPU)" == "AMD64" && "$(GUI)" == "yes"
# This option is required for VC2012 or later so that 64-bit gvim can
# accept D&D from 32-bit applications.  NOTE: This disables 64-bit ASLR,
# therefore the security level becomes as same as VC2010.
LINKARGS1 = $(LINKARGS1) /HIGHENTROPYVA:NO
!endif

all:	$(VIM).exe \
	vimrun.exe \
	install.exe \
	uninstal.exe \
	xxd/xxd.exe \
	tee/tee.exe \
	GvimExt/gvimext.dll

$(VIM).exe: $(OUTDIR) $(OBJ) $(GUI_OBJ) $(CUI_OBJ) $(OLE_OBJ) $(OLE_IDL) $(MZSCHEME_OBJ) \
		$(LUA_OBJ) $(PERL_OBJ) $(PYTHON_OBJ) $(PYTHON3_OBJ) $(RUBY_OBJ) $(TCL_OBJ) \
		$(CSCOPE_OBJ) $(TERM_OBJ) $(NETBEANS_OBJ) $(CHANNEL_OBJ) $(XPM_OBJ) \
		version.c version.h
	$(CC) $(CFLAGS_OUTDIR) version.c
	$(link) $(LINKARGS1) -out:$(VIM).exe $(OBJ) $(GUI_OBJ) $(CUI_OBJ) $(OLE_OBJ) \
		$(LUA_OBJ) $(MZSCHEME_OBJ) $(PERL_OBJ) $(PYTHON_OBJ) $(PYTHON3_OBJ) $(RUBY_OBJ) \
		$(TCL_OBJ) $(CSCOPE_OBJ) $(TERM_OBJ) $(NETBEANS_OBJ) $(CHANNEL_OBJ) \
		$(XPM_OBJ) $(OUTDIR)\version.obj $(LINKARGS2)
	if exist $(VIM).exe.manifest mt.exe -nologo -manifest $(VIM).exe.manifest -updateresource:$(VIM).exe;1

$(VIM): $(VIM).exe

$(OUTDIR):
	if not exist $(OUTDIR)/nul  mkdir $(OUTDIR)

install.exe: dosinst.c
	$(CC) /nologo -DNDEBUG -DWIN32 dosinst.c kernel32.lib shell32.lib \
		user32.lib ole32.lib advapi32.lib uuid.lib \
		-link -subsystem:$(SUBSYSTEM_TOOLS)
	- if exist install.exe del install.exe
	ren dosinst.exe install.exe

uninstal.exe: uninstal.c
	$(CC) /nologo -DNDEBUG -DWIN32 uninstal.c shell32.lib advapi32.lib \
		-link -subsystem:$(SUBSYSTEM_TOOLS)

vimrun.exe: vimrun.c
	$(CC) /nologo -DNDEBUG vimrun.c -link -subsystem:$(SUBSYSTEM_TOOLS)

xxd/xxd.exe: xxd/xxd.c
	cd xxd
	$(MAKE) /NOLOGO -f Make_mvc.mak $(MAKEFLAGS_TOOLS)
	cd ..

tee/tee.exe: tee/tee.c
	cd tee
	$(MAKE) /NOLOGO -f Make_mvc.mak $(MAKEFLAGS_TOOLS)
	cd ..

GvimExt/gvimext.dll: GvimExt/gvimext.cpp GvimExt/gvimext.rc GvimExt/gvimext.h
	cd GvimExt
	$(MAKE) /NOLOGO -f Makefile $(MAKEFLAGS_GVIMEXT)
	cd ..


tags: notags
	$(CTAGS) *.c *.cpp *.h if_perl.xs

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
	cd tee
	$(MAKE) /NOLOGO -f Make_mvc.mak clean
	cd ..
	cd GvimExt
	$(MAKE) /NOLOGO -f Makefile clean
	cd ..
	- if exist testdir\*.out del testdir\*.out

test:
	cd testdir
	$(MAKE) /NOLOGO -f Make_dos.mak win32
	cd ..

testgvim:
	cd testdir
	$(MAKE) /NOLOGO -f Make_dos.mak VIMPROG=..\gvim win32
	cd ..

testclean:
	cd testdir
	$(MAKE) /NOLOGO -f Make_dos.mak clean
	cd ..

###########################################################################

# Create a default rule for transforming .c files to .obj files in $(OUTDIR)
# Batch compilation is supported by nmake 1.62 (part of VS 5.0) and later)
!IF "$(_NMAKE_VER)" == ""
.c{$(OUTDIR)/}.obj:
!ELSE
.c{$(OUTDIR)/}.obj::
!ENDIF
	$(CC) $(CFLAGS_OUTDIR) $<

# Create a default rule for transforming .cpp files to .obj files in $(OUTDIR)
# Batch compilation is supported by nmake 1.62 (part of VS 5.0) and later)
!IF "$(_NMAKE_VER)" == ""
.cpp{$(OUTDIR)/}.obj:
!ELSE
.cpp{$(OUTDIR)/}.obj::
!ENDIF
	$(CC) $(CFLAGS_OUTDIR) $<

$(OUTDIR)/arabic.obj:	$(OUTDIR) arabic.c  $(INCL)

$(OUTDIR)/beval.obj:	$(OUTDIR) beval.c  $(INCL)

$(OUTDIR)/blowfish.obj:	$(OUTDIR) blowfish.c  $(INCL)

$(OUTDIR)/buffer.obj:	$(OUTDIR) buffer.c  $(INCL)

$(OUTDIR)/charset.obj:	$(OUTDIR) charset.c  $(INCL)

$(OUTDIR)/crypt.obj:	$(OUTDIR) crypt.c  $(INCL)

$(OUTDIR)/crypt_zip.obj: $(OUTDIR) crypt_zip.c  $(INCL)

$(OUTDIR)/dict.obj:	$(OUTDIR) dict.c  $(INCL)

$(OUTDIR)/diff.obj:	$(OUTDIR) diff.c  $(INCL)

$(OUTDIR)/digraph.obj:	$(OUTDIR) digraph.c  $(INCL)

$(OUTDIR)/edit.obj:	$(OUTDIR) edit.c  $(INCL)

$(OUTDIR)/eval.obj:	$(OUTDIR) eval.c  $(INCL)

$(OUTDIR)/evalfunc.obj:	$(OUTDIR) evalfunc.c  $(INCL)

$(OUTDIR)/ex_cmds.obj:	$(OUTDIR) ex_cmds.c  $(INCL)

$(OUTDIR)/ex_cmds2.obj:	$(OUTDIR) ex_cmds2.c  $(INCL)

$(OUTDIR)/ex_docmd.obj:	$(OUTDIR) ex_docmd.c  $(INCL)

$(OUTDIR)/ex_eval.obj:	$(OUTDIR) ex_eval.c  $(INCL)

$(OUTDIR)/ex_getln.obj:	$(OUTDIR) ex_getln.c  $(INCL)

$(OUTDIR)/farsi.obj:	$(OUTDIR) farsi.c  $(INCL)

$(OUTDIR)/fileio.obj:	$(OUTDIR) fileio.c  $(INCL)

$(OUTDIR)/fold.obj:	$(OUTDIR) fold.c  $(INCL)

$(OUTDIR)/getchar.obj:	$(OUTDIR) getchar.c  $(INCL)

$(OUTDIR)/hardcopy.obj:	$(OUTDIR) hardcopy.c  $(INCL)

$(OUTDIR)/hashtab.obj:	$(OUTDIR) hashtab.c  $(INCL)

$(OUTDIR)/gui.obj:	$(OUTDIR) gui.c  $(INCL) $(GUI_INCL)

$(OUTDIR)/gui_beval.obj:	$(OUTDIR) gui_beval.c $(INCL) $(GUI_INCL)

$(OUTDIR)/gui_w32.obj:	$(OUTDIR) gui_w32.c $(INCL) $(GUI_INCL)

$(OUTDIR)/gui_dwrite.obj:	$(OUTDIR) gui_dwrite.cpp $(INCL) $(GUI_INCL)

$(OUTDIR)/if_cscope.obj: $(OUTDIR) if_cscope.c  $(INCL) if_cscope.h

$(OUTDIR)/if_lua.obj: $(OUTDIR) if_lua.c  $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(LUA_INC) if_lua.c

if_perl.c : if_perl.xs typemap
	$(XSUBPP) -prototypes -typemap $(XSUBPP_TYPEMAP) \
		-typemap typemap if_perl.xs -output if_perl.c

$(OUTDIR)/if_perl.obj: $(OUTDIR) if_perl.c  $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(PERL_INC) if_perl.c

$(OUTDIR)/if_perlsfio.obj: $(OUTDIR) if_perlsfio.c  $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(PERL_INC) if_perlsfio.c

$(OUTDIR)/if_mzsch.obj: $(OUTDIR) if_mzsch.c $(MZSCHEME_INCL) $(INCL) $(MZSCHEME_EXTRA_DEP)
	$(CC) $(CFLAGS_OUTDIR) if_mzsch.c \
		-DMZSCHEME_COLLECTS="\"$(MZSCHEME_COLLECTS:\=\\)\""

lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).lib:
	lib /DEF:"$(MZSCHEME)\lib\lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).def"

$(OUTDIR)/if_python.obj: $(OUTDIR) if_python.c if_py_both.h $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(PYTHON_INC) if_python.c

$(OUTDIR)/if_python3.obj: $(OUTDIR) if_python3.c if_py_both.h $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(PYTHON3_INC) if_python3.c

$(OUTDIR)/if_ole.obj: $(OUTDIR) if_ole.cpp  $(INCL) if_ole.h

$(OUTDIR)/if_ruby.obj: $(OUTDIR) if_ruby.c  $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(RUBY_INC) if_ruby.c

$(OUTDIR)/if_tcl.obj: $(OUTDIR) if_tcl.c  $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(TCL_INC) if_tcl.c

$(OUTDIR)/iscygpty.obj:	$(OUTDIR) iscygpty.c $(CUI_INCL)
	$(CC) $(CFLAGS_OUTDIR) iscygpty.c -D_WIN32_WINNT=0x0600 -DUSE_DYNFILEID -DENABLE_STUB_IMPL

$(OUTDIR)/json.obj:	$(OUTDIR) json.c  $(INCL)

$(OUTDIR)/list.obj:	$(OUTDIR) list.c  $(INCL)

$(OUTDIR)/main.obj:	$(OUTDIR) main.c  $(INCL) $(CUI_INCL)

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

$(OUTDIR)/channel.obj: $(OUTDIR) channel.c $(INCL)

$(OUTDIR)/normal.obj:	$(OUTDIR) normal.c  $(INCL)

$(OUTDIR)/option.obj:	$(OUTDIR) option.c  $(INCL)

$(OUTDIR)/ops.obj:	$(OUTDIR) ops.c  $(INCL)

$(OUTDIR)/os_mswin.obj:	$(OUTDIR) os_mswin.c  $(INCL)

$(OUTDIR)/terminal.obj:	$(OUTDIR) terminal.c  $(INCL) $(TERM_DEPS)

$(OUTDIR)/winclip.obj:	$(OUTDIR) winclip.c  $(INCL)

$(OUTDIR)/os_win32.obj:	$(OUTDIR) os_win32.c  $(INCL) $(MZSCHEME_INCL)

$(OUTDIR)/os_w32exe.obj:	$(OUTDIR) os_w32exe.c  $(INCL)

$(OUTDIR)/pathdef.obj:	$(OUTDIR) $(PATHDEF_SRC) $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(PATHDEF_SRC)

$(OUTDIR)/popupmnu.obj:	$(OUTDIR) popupmnu.c  $(INCL)

$(OUTDIR)/quickfix.obj:	$(OUTDIR) quickfix.c  $(INCL)

$(OUTDIR)/regexp.obj:	$(OUTDIR) regexp.c regexp_nfa.c  $(INCL)

$(OUTDIR)/screen.obj:	$(OUTDIR) screen.c  $(INCL)

$(OUTDIR)/search.obj:	$(OUTDIR) search.c  $(INCL)

$(OUTDIR)/sha256.obj:	$(OUTDIR) sha256.c  $(INCL)

$(OUTDIR)/spell.obj:	$(OUTDIR) spell.c  $(INCL)

$(OUTDIR)/spellfile.obj:	$(OUTDIR) spellfile.c  $(INCL)

$(OUTDIR)/syntax.obj:	$(OUTDIR) syntax.c  $(INCL)

$(OUTDIR)/tag.obj:	$(OUTDIR) tag.c  $(INCL)

$(OUTDIR)/term.obj:	$(OUTDIR) term.c  $(INCL)

$(OUTDIR)/ui.obj:	$(OUTDIR) ui.c  $(INCL)

$(OUTDIR)/undo.obj:	$(OUTDIR) undo.c  $(INCL)

$(OUTDIR)/userfunc.obj:	$(OUTDIR) userfunc.c  $(INCL)

$(OUTDIR)/window.obj:	$(OUTDIR) window.c  $(INCL)

$(OUTDIR)/xpm_w32.obj: $(OUTDIR) xpm_w32.c
	$(CC) $(CFLAGS_OUTDIR) $(XPM_INC) xpm_w32.c

$(OUTDIR)/vim.res:	$(OUTDIR) vim.rc gvim.exe.mnf version.h tools.bmp \
				tearoff.bmp vim.ico vim_error.ico \
				vim_alert.ico vim_info.ico vim_quest.ico
	$(RC) /nologo /l 0x409 /Fo$(OUTDIR)/vim.res $(RCFLAGS) vim.rc

iid_ole.c if_ole.h vim.tlb: if_ole.idl
	midl /nologo /error none /proxy nul /iid iid_ole.c /tlb vim.tlb \
		/header if_ole.h if_ole.idl

dimm.h dimm_i.c: dimm.idl
	midl /nologo /error none /proxy nul dimm.idl

$(OUTDIR)/dimm_i.obj: $(OUTDIR) dimm_i.c $(INCL)

$(OUTDIR)/glbl_ime.obj:	$(OUTDIR) glbl_ime.cpp  dimm.h $(INCL)


CCCTERM = $(CC) $(CFLAGS) -Ilibvterm/include -DINLINE="" \
	-DVSNPRINTF=vim_vsnprintf \
	-DIS_COMBINING_FUNCTION=utf_iscomposing_uint \
	-DWCWIDTH_FUNCTION=utf_uint2cells \
	-D_CRT_SECURE_NO_WARNINGS

$(OUTDIR)/term_encoding.obj: $(OUTDIR) libvterm/src/encoding.c $(TERM_DEPS)
	$(CCCTERM) -Fo$@ libvterm/src/encoding.c

$(OUTDIR)/term_keyboard.obj: $(OUTDIR) libvterm/src/keyboard.c $(TERM_DEPS)
	$(CCCTERM) -Fo$@ libvterm/src/keyboard.c

$(OUTDIR)/term_mouse.obj: $(OUTDIR) libvterm/src/mouse.c $(TERM_DEPS)
	$(CCCTERM) -Fo$@ libvterm/src/mouse.c

$(OUTDIR)/term_parser.obj: $(OUTDIR) libvterm/src/parser.c $(TERM_DEPS)
	$(CCCTERM) -Fo$@ libvterm/src/parser.c

$(OUTDIR)/term_pen.obj: $(OUTDIR) libvterm/src/pen.c $(TERM_DEPS)
	$(CCCTERM) -Fo$@ libvterm/src/pen.c

$(OUTDIR)/term_screen.obj: $(OUTDIR) libvterm/src/screen.c $(TERM_DEPS)
	$(CCCTERM) -Fo$@ libvterm/src/screen.c

$(OUTDIR)/term_state.obj: $(OUTDIR) libvterm/src/state.c $(TERM_DEPS)
	$(CCCTERM) -Fo$@ libvterm/src/state.c

$(OUTDIR)/term_unicode.obj: $(OUTDIR) libvterm/src/unicode.c $(TERM_DEPS)
	$(CCCTERM) -Fo$@ libvterm/src/unicode.c

$(OUTDIR)/term_vterm.obj: $(OUTDIR) libvterm/src/vterm.c $(TERM_DEPS)
	$(CCCTERM) -Fo$@ libvterm/src/vterm.c


# $CFLAGS may contain backslashes and double quotes, escape them both.
E0_CFLAGS = $(CFLAGS:\=\\)
E_CFLAGS = $(E0_CFLAGS:"=\")
# ") stop the string
# $LINKARGS2 may contain backslashes and double quotes, escape them both.
E0_LINKARGS2 = $(LINKARGS2:\=\\)
E_LINKARGS2 = $(E0_LINKARGS2:"=\")
# ") stop the string

$(PATHDEF_SRC): auto
	@echo creating $(PATHDEF_SRC)
	@echo /* pathdef.c */ > $(PATHDEF_SRC)
	@echo #include "vim.h" >> $(PATHDEF_SRC)
	@echo char_u *default_vim_dir = (char_u *)"$(VIMRCLOC:\=\\)"; >> $(PATHDEF_SRC)
	@echo char_u *default_vimruntime_dir = (char_u *)"$(VIMRUNTIMEDIR:\=\\)"; >> $(PATHDEF_SRC)
	@echo char_u *all_cflags = (char_u *)"$(CC:\=\\) $(E_CFLAGS)"; >> $(PATHDEF_SRC)
	@echo char_u *all_lflags = (char_u *)"$(link:\=\\) $(LINKARGS1:\=\\) $(E_LINKARGS2)"; >> $(PATHDEF_SRC)
	@echo char_u *compiled_user = (char_u *)"$(USERNAME)"; >> $(PATHDEF_SRC)
	@echo char_u *compiled_sys = (char_u *)"$(USERDOMAIN)"; >> $(PATHDEF_SRC)

auto:
	if not exist auto/nul mkdir auto

# End Custom Build
proto.h: \
	proto/arabic.pro \
	proto/blowfish.pro \
	proto/buffer.pro \
	proto/charset.pro \
	proto/crypt.pro \
	proto/crypt_zip.pro \
	proto/dict.pro \
	proto/diff.pro \
	proto/digraph.pro \
	proto/edit.pro \
	proto/eval.pro \
	proto/evalfunc.pro \
	proto/ex_cmds.pro \
	proto/ex_cmds2.pro \
	proto/ex_docmd.pro \
	proto/ex_eval.pro \
	proto/ex_getln.pro \
	proto/farsi.pro \
	proto/fileio.pro \
	proto/getchar.pro \
	proto/hardcopy.pro \
	proto/hashtab.pro \
	proto/json.pro \
	proto/list.pro \
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
	proto/winclip.pro \
	proto/os_win32.pro \
	proto/popupmnu.pro \
	proto/quickfix.pro \
	proto/regexp.pro \
	proto/screen.pro \
	proto/search.pro \
	proto/sha256.pro \
	proto/spell.pro \
	proto/spellfile.pro \
	proto/syntax.pro \
	proto/tag.pro \
	proto/term.pro \
	proto/ui.pro \
	proto/undo.pro \
	proto/userfunc.pro \
	proto/window.pro \
	$(NETBEANS_PRO) \
	$(CHANNEL_PRO)

.SUFFIXES: .cod .i

# Generate foo.cod (mixed source and assembly listing) from foo.c via "nmake
# foo.cod"
.c.cod:
	$(CC) $(CFLAGS) /FAcs $<

# Generate foo.i (preprocessor listing) from foo.c via "nmake foo.i"
.c.i:
	$(CC) $(CFLAGS) /P /C $<

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=0:
