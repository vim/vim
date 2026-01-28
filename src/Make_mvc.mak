# Makefile for Vim on Win32 (Windows 7/8/10/11) and Win64, using the Microsoft
# Visual C++ compilers. Known to work with VC14 (VS2015), VC14.1 (VS2017),
# VC14.2 (VS2019) and VC14.3 (VS2022).
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
#	!!!!  After changing any features do "nmake clean" first  !!!!
#
#	Feature Set: FEATURES=[TINY, NORMAL, HUGE] (default is HUGE)
#
#   	Name to add to the version: MODIFIED_BY=[name of modifier]
#
#	GUI interface: GUI=yes (default is no)
#
#	GUI with DirectWrite (DirectX): DIRECTX=yes
#	  (default is yes if GUI=yes, requires GUI=yes)
#
#	Color emoji support: COLOR_EMOJI=yes
#	  (default is yes if DIRECTX=yes, requires WinSDK 8.1 or later.)
#
#	OLE interface: OLE=yes (usually with GUI=yes)
#
#	IME support: IME=yes	(default is yes)
#	  DYNAMIC_IME=[yes or no]  (to load the imm32.dll dynamically, default
#	  is yes)
#
#	Terminal support: TERMINAL=yes (default is yes if FEATURES is HUGE)
#	  Will also enable CHANNEL
#
#	Sound support: SOUND=yes (default is yes)
#
#	Sodium support: SODIUM=[Path to Sodium directory]
#	  DYNAMIC_SODIUM=yes (to load the Sodium DLL dynamically)
#	  You need to install the msvc package from
#	  https://download.libsodium.org/libsodium/releases/
#	  and package the libsodium.dll with Vim
#
#
#	DLL support (EXPERIMENTAL): VIMDLL=yes (default is no)
#	  Creates vim{32,64}.dll, and stub gvim.exe and vim.exe.
#	  The shared codes between the GUI and the console are built into
#	  the DLL.  This reduces the total file size and memory usage.
#	  Also supports `vim -g` and the `:gui` command.
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
#	   Used for the DLL file name. E.g.:
#	   C:\Program Files (x86)\Racket\lib\libracket3m_XXXXXX.dll
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
#	  PYTHON3_VER=[Python3 version, eg 30, 31]  (default is 38)
#
#	Ruby interface:
#	  RUBY=[Path to Ruby directory]
#	  DYNAMIC_RUBY=yes (to load the Ruby DLL dynamically)
#	  RUBY_VER=[Ruby version, eg 19, 22] (default is 22)
#	  RUBY_API_VER_LONG=[Ruby API version, eg 1.9.1, 2.2.0]
#	        (default is 2.2.0)
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
#	  Requires CHANNEL.
#
#	Netbeans Debugging Support: NBDEBUG=[yes or no] (should be no, yes
#	doesn't work)
#
#	Inter process communication: CHANNEL=[yes or no] (default is yes if GUI
#	is yes or TERMINAL is yes)
#
#	XPM Image Support: XPM=[path to XPM directory]
#	Default is "xpm", using the files included in the distribution.
#	Use "no" to disable this feature.
#
#	Optimization: OPTIMIZE=[SPACE, SPEED, MAXSPEED] (default is MAXSPEED)
#
#	Processor Version:
#	 For x86: CPUNR=[any, i686, sse, sse2, avx, avx2, avx512]
#	 For x64: CPUNR=[sse2, avx, avx2, avx512]
#	                (default is sse2 (both x86 and x64))
#	  avx is available on Visual C++ 2010 and after.
#	  avx2 is available on Visual C++ 2013 Update 2 and after.
#	  avx512 is available on Visual C++ 2017 and after.
#	 For ARM64:
#	  See: https://learn.microsoft.com/en-us/cpp/build/reference/arch-arm64
#
#	Version Support: WINVER=[0x0601, 0x0602, 0x0603, 0x0A00] (default is
#	0x0601)
#	Supported versions depends on your target SDK, check SDKDDKVer.h
#	See https://learn.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt
#
#	Debug version: DEBUG=yes
#	Mapfile: MAP=[no, yes or lines] (default is yes)
#	  no:    Don't write a mapfile.
#	  yes:   Write a normal mapfile.
#	  lines: Write a mapfile with line numbers (only for VC6 and later)
#
#	Static Code Analysis: ANALYZE=yes (works with VS2012 or later)
#
#	Address Sanitizer: ASAN=yes (works with VS2019 or later)
#
# You can combine any of these interfaces
#
# Example: To build the non-debug, GUI version with Perl interface:
#	nmake -f Make_mvc.mak GUI=yes PERL=C:\Perl

### See feature.h for a list of optionals.
# If you want to build some optional features without modifying the source,
# you can set DEFINES on the command line, e.g.,
#	nmake -f Make_mvc.mvc "DEFINES=-DEMACS_TAGS"

# included common tools
!INCLUDE .\auto\nmake\tools.mak

# Read MAJOR and MINOR from version.h.
!IFNDEF MAJOR
! IF ![for /F "tokens=3" %G in \
	('findstr /RC:"VIM_VERSION_MAJOR[	^]*[0-9^]" .\version.h') \
	do @(echo:MAJOR=%G> .\_major.tmp)]
!  INCLUDE .\_major.tmp
!  IF [$(RM) .\_major.tmp]
!  ENDIF
! ELSE
MAJOR = 9
! ENDIF
!ENDIF

!IFNDEF MINOR
! IF ![for /F "tokens=3" %G in \
	('findstr /RC:"VIM_VERSION_MINOR[	^]*[0-9^]" .\version.h') \
	do @(echo:MINOR=%G> .\_minor.tmp)]
!  INCLUDE .\_minor.tmp
!  IF [$(RM) .\_minor.tmp]
!  ENDIF
! ELSE
MINOR = 1
! ENDIF
!ENDIF

# Read PATCHLEVEL from version.c
!IFNDEF PATCHLEVEL
! IF ![for /F %G in \
	('findstr /NBLC:"static int included_patches" .\version.c \
	^| (set /p "_t=" ^& set /a _t+=2 ^)') do \
	@$(CMD) $(CMDFLAGS) "for /F "skip=%G delims=, " %H in \
	(.\version.c) do (echo:PATCHLEVEL=%H> .\_patchlvl.tmp & exit /b)"]
!  INCLUDE .\_patchlvl.tmp
!  IF [$(RM) .\_patchlvl.tmp]
!  ENDIF
! ELSE
PATCHLEVEL = 0
! ENDIF
!ENDIF

!MESSAGE Vim version: $(MAJOR).$(MINOR).$(PATCHLEVEL)

!IF "$(VIMDLL)" == "yes"
GUI = yes
!ENDIF

!IFNDEF DIRECTX
DIRECTX = $(GUI)
!ENDIF

# Select a code directory, depends on GUI, OLE, DEBUG, interfaces and etc.
# If you change something else, do "make clean" first!
!IF "$(VIMDLL)" == "yes"
OBJDIR = .\ObjD
!ELSEIF "$(GUI)" == "yes"
OBJDIR = .\ObjG
!ELSE
OBJDIR = .\ObjC
!ENDIF
!IF "$(DIRECTX)" == "yes" && "$(GUI)" == "yes"
OBJDIR = $(OBJDIR)X
!ENDIF
!IF "$(OLE)" == "yes"
OBJDIR = $(OBJDIR)O
!ENDIF
!IFDEF LUA
OBJDIR = $(OBJDIR)U
!ENDIF
!IFDEF PERL
OBJDIR = $(OBJDIR)L
!ENDIF
!IFDEF PYTHON
OBJDIR = $(OBJDIR)Y
!ENDIF
!IFDEF PYTHON3
OBJDIR = $(OBJDIR)H
!ENDIF
!IFDEF TCL
OBJDIR = $(OBJDIR)T
!ENDIF
!IFDEF RUBY
OBJDIR = $(OBJDIR)R
!ENDIF
!IFDEF MZSCHEME
OBJDIR = $(OBJDIR)Z
!ENDIF
!IFDEF USE_MSVCRT
OBJDIR = $(OBJDIR)V
!ENDIF
!IF "$(DEBUG)" == "yes"
OBJDIR = $(OBJDIR)d
!ENDIF

!IFDEF CPU
! IF "$(CPU)" == "I386"
CPU = i386
! ENDIF
!ELSE  # !CPU
CPU = i386
! IFNDEF PLATFORM
!  IFDEF TARGET_CPU
PLATFORM = $(TARGET_CPU)
!  ELSEIF defined(VSCMD_ARG_TGT_ARCH)
PLATFORM = $(VSCMD_ARG_TGT_ARCH)
!  ENDIF
! ENDIF
! IFDEF PLATFORM
!  IF ("$(PLATFORM)" == "x64") || ("$(PLATFORM)" == "X64")
CPU = AMD64
!  ELSEIF ("$(PLATFORM)" == "arm64") || ("$(PLATFORM)" == "ARM64")
CPU = ARM64
!  ELSEIF ("$(PLATFORM)" != "x86") && ("$(PLATFORM)" != "X86")
!   ERROR *** ERROR Unknown target platform "$(PLATFORM)". Make aborted.
!  ENDIF
! ENDIF  # !PLATFORM
!ENDIF
OBJDIR = $(OBJDIR)$(CPU)

# Build a retail version by default

!IF "$(DEBUG)" != "yes"
NODEBUG = 1
!ELSE
! UNDEF NODEBUG
MAKEFLAGS_GVIMEXT = DEBUG=yes
!ENDIF

LINK = link

# Check VC version.
!IF [echo MSVCVER=_MSC_VER> msvcver.c && \
	echo MSVC_FULL=_MSC_FULL_VER>> msvcver.c && \
	$(CC) /EP msvcver.c > msvcver.~ 2> nul]
! MESSAGE *** ERROR
! MESSAGE Cannot run Visual C to determine its version. Make sure cl.exe is in your PATH.
! MESSAGE This can usually be done by running "vcvarsall.bat", located in the bin directory where Visual Studio was installed.
! ERROR Make aborted.
!ELSE
! INCLUDE msvcver.~
! IF [$(RM) msvcver.c msvcver.~]
! ENDIF
!ENDIF

!IF $(MSVCVER) < 1900
! MESSAGE *** ERROR
! MESSAGE Unsupported MSVC version.
! MESSAGE Please use Visual C++ 2015 or later.
! ERROR Make aborted.
!ENDIF

MSVC_MAJOR = ($(MSVCVER) / 100 - 5)
MSVCRT_VER = ($(MSVCVER) / 100 * 10 - 50)

# Calculate MSVCRT_VER
!IF [(set /a MSVCRT_VER="$(MSVCRT_VER)" > nul) && set MSVCRT_VER > msvcrtver.~] == 0
! INCLUDE msvcrtver.~
! IF [$(RM) msvcrtver.~]
! ENDIF
!ENDIF

# Show the versions (for debugging).
#!MESSAGE _MSC_VER=$(MSVCVER)
#!MESSAGE _MSC_FULL_VER=$(MSVC_FULL)
#!MESSAGE MSVCRT_VER=$(MSVCRT_VER)

# Base name of the msvcrXX.dll (vcruntimeXXX.dll)
MSVCRT_NAME = vcruntime$(MSVCRT_VER)

### Set the default $(WINVER) to make it work with Windows 7
!IFNDEF WINVER
! IF "$(CPU)" == "ARM64"
WINVER = 0x0A00
! ELSE
WINVER = 0x0601
! ENDIF
!ENDIF

# Use multiprocess build
USE_MP = yes

!IF "$(FEATURES)" == ""
FEATURES = HUGE
!ENDIF

!IFNDEF CTAGS
# this assumes ctags is Exuberant ctags
CTAGS = ctags -I INIT+,INIT2+,INIT3+,INIT4+,INIT5+ --fields=+S
!ENDIF

!IFNDEF CSCOPE
CSCOPE = yes
!ENDIF

!IF "$(CSCOPE)" == "yes"
# CSCOPE - Include support for Cscope
CSCOPE_DEFS = -DFEAT_CSCOPE
!ENDIF

!IFNDEF TERMINAL
! IF "$(FEATURES)" == "HUGE"
TERMINAL = yes
! ELSE
TERMINAL = no
! ENDIF
!ENDIF

!IF "$(TERMINAL)" == "yes"
TERM_OBJ = \
	$(OBJDIR)/terminal.obj \
	$(OBJDIR)/libvterm/encoding.obj \
	$(OBJDIR)/libvterm/keyboard.obj \
	$(OBJDIR)/libvterm/mouse.obj \
	$(OBJDIR)/libvterm/parser.obj \
	$(OBJDIR)/libvterm/pen.obj \
	$(OBJDIR)/libvterm/screen.obj \
	$(OBJDIR)/libvterm/state.obj \
	$(OBJDIR)/libvterm/unicode.obj \
	$(OBJDIR)/libvterm/vterm.obj
TERM_DEFS = -DFEAT_TERMINAL
TERM_DEPS = \
	libvterm/include/vterm.h \
	libvterm/include/vterm_keycodes.h \
	libvterm/src/rect.h \
	libvterm/src/utf8.h \
	libvterm/src/vterm_internal.h
!ENDIF

!IFNDEF SOUND
! IF "$(FEATURES)" == "HUGE"
SOUND = yes
! ELSE
SOUND = no
! ENDIF
!ENDIF

!IFNDEF SODIUM
SODIUM = no
!ENDIF
!IFNDEF DYNAMIC_SODIUM
DYNAMIC_SODIUM = yes
!ENDIF

!IF "$(SODIUM)" != "no"
! IF "$(CPU)" == "AMD64"
SOD_LIB = $(SODIUM)\x64\Release\v143\dynamic
! ELSEIF "$(CPU)" == "i386"
SOD_LIB = $(SODIUM)\Win32\Release\v143\dynamic
! ELSE
SODIUM = no
! ENDIF
!ENDIF

!IF "$(SODIUM)" != "no"
SOD_INC = /I "$(SODIUM)\include"
! IF "$(DYNAMIC_SODIUM)" == "yes"
SODIUM_DLL = libsodium.dll
SOD_DEFS = -DHAVE_SODIUM -DDYNAMIC_SODIUM -DDYNAMIC_SODIUM_DLL=\"$(SODIUM_DLL)\"
SOD_LIB =
! ELSE
SOD_DEFS = -DHAVE_SODIUM
SOD_LIB = $(SOD_LIB)\libsodium.lib
! ENDIF
!ENDIF

!IFNDEF NETBEANS
NETBEANS = $(GUI)
!ENDIF

!IFNDEF CHANNEL
! IF "$(FEATURES)" == "HUGE" || "$(TERMINAL)" == "yes"
CHANNEL = yes
! ELSE
CHANNEL = $(GUI)
! ENDIF
!ENDIF

# GUI specific features.
!IF "$(GUI)" == "yes"
# Only allow NETBEANS for a GUI build and CHANNEL.
! IF "$(NETBEANS)" == "yes" && "$(CHANNEL)" == "yes"
# NETBEANS - Include support for Netbeans integration
NETBEANS_PRO = proto/netbeans.pro
NETBEANS_OBJ = $(OBJDIR)/netbeans.obj
NETBEANS_DEFS = -DFEAT_NETBEANS_INTG

!  IF "$(NBDEBUG)" == "yes"
NBDEBUG_DEFS = -DNBDEBUG
NBDEBUG_INCL = nbdebug.h
NBDEBUG_SRC = nbdebug.c
!  ENDIF
! ENDIF

# DirectWrite (DirectX)
! IF "$(DIRECTX)" == "yes"
DIRECTX_DEFS = -DFEAT_DIRECTX -DDYNAMIC_DIRECTX
!  IF "$(COLOR_EMOJI)" != "no"
DIRECTX_DEFS = $(DIRECTX_DEFS) -DFEAT_DIRECTX_COLOR_EMOJI
!  ENDIF
DIRECTX_INCL = gui_dwrite.h
DIRECTX_OBJ = $(OUTDIR)\gui_dwrite.obj
! ENDIF

# Only allow XPM for a GUI build.
! IFNDEF XPM
!  IFNDEF USE_MSVCRT
# Both XPM and USE_MSVCRT are not set, use the included xpm files, depending
# on the architecture.
!   IF "$(CPU)" == "AMD64"
XPM = xpm\x64
!   ELSEIF "$(CPU)" == "ARM64"
XPM = xpm\arm64
!   ELSEIF "$(CPU)" == "i386"
XPM = xpm\x86
!   ELSE
XPM = no
!   ENDIF
!  ELSE # USE_MSVCRT
XPM = no
!  ENDIF # USE_MSVCRT
! ENDIF # XPM
! IF "$(XPM)" != "no"
# XPM - Include support for XPM signs
# See the xpm directory for more information.
XPM_OBJ = $(OBJDIR)/xpm_w32.obj
XPM_DEFS = -DFEAT_XPM_W32
XPM_LIB = $(XPM)\lib-vc14\libXpm.lib
XPM_INC = -I $(XPM)\include -I $(XPM)\..\include
! ENDIF
!ENDIF # GUI

!IF "$(SOUND)" == "yes"
SOUND_PRO = proto/sound.pro
SOUND_OBJ = $(OBJDIR)/sound.obj
SOUND_DEFS = -DFEAT_SOUND
SOUND_LIB = winmm.lib
!ENDIF

!IF "$(CHANNEL)" == "yes"
CHANNEL_PRO = proto/job.pro proto/channel.pro
CHANNEL_OBJ = $(OBJDIR)/job.obj $(OBJDIR)/channel.obj
CHANNEL_DEFS = -DFEAT_JOB_CHANNEL -DFEAT_IPV6 -DHAVE_INET_NTOP

NETBEANS_LIB = Ws2_32.lib
!ENDIF

# need advapi32.lib for GetUserName()
# need shell32.lib for ExtractIcon()
# need netapi32.lib for NetUserEnum()
# gdi32.lib and comdlg32.lib for printing support
# ole32.lib and uuid.lib are needed for FEAT_SHORTCUT
CON_LIB = oldnames.lib kernel32.lib advapi32.lib shell32.lib gdi32.lib \
	comdlg32.lib ole32.lib netapi32.lib uuid.lib user32.lib \
	/machine:$(CPU)
!IF "$(DELAYLOAD)" == "yes"
CON_LIB = $(CON_LIB) /DELAYLOAD:comdlg32.dll /DELAYLOAD:ole32.dll DelayImp.lib
!ENDIF

# If you have a fixed directory for $VIM or $VIMRUNTIME, other than the normal
# default, use these lines.
#VIMRCLOC = somewhere
#VIMRUNTIMEDIR = somewhere

CFLAGS = -c /W3 /GF /nologo -I. -Iproto -DHAVE_PATHDEF -DWIN32 -DHAVE_STDINT_H \
	$(CSCOPE_DEFS) $(TERM_DEFS) $(SOUND_DEFS) $(NETBEANS_DEFS) \
	$(NBDEBUG_DEFS) $(XPM_DEFS) $(SOD_DEFS) $(SOD_INC) $(CHANNEL_DEFS) \
	$(DEFINES) $(CI_CFLAGS) -DWINVER=$(WINVER) -D_WIN32_WINNT=$(WINVER) \
	/source-charset:utf-8

RCFLAGS = -DVIM_VERSION_PATCHLEVEL=$(PATCHLEVEL)

#>>>>> end of choices
###########################################################################

INTDIR = $(OBJDIR)
OUTDIR = $(OBJDIR)

### Validate CPUNR
!IF "$(CPU)" == "i386" || "$(CPU)" == "AMD64"
! IFNDEF CPUNR
# default to SSE2
CPUNR = sse2
! ELSEIF "$(CPU)" == "i386" \
	&& ("$(CPUNR)" == "i386" || "$(CPUNR)" == "i486" || "$(CPUNR)" == "i586")
# alias i386, i486 and i586 to i686
!  MESSAGE *** WARNING CPUNR=$(CPUNR) is not a valid target architecture.
!  MESSAGE Windows 7 is the minimum target OS, with a minimum target
!  MESSAGE architecture of i686.
!  MESSAGE Retargeting to i686
CPUNR = i686
! ELSEIF "$(CPUNR)" == "pentium4"
# alias pentium4 to sse2
!  MESSAGE *** WARNING CPUNR=pentium4 is deprecated in favour of sse2.
!  MESSAGE Retargeting to sse2.
CPUNR = sse2
! ELSEIF ("$(CPU)" != "i386" \
		|| ("$(CPUNR)" != "any" && "$(CPUNR)" != "i686" \
			&& "$(CPUNR)" != "sse" )) \
	&& "$(CPUNR)" != "sse2" && "$(CPUNR)" != "avx" \
	&& "$(CPUNR)" != "avx2" && "$(CPUNR)" != "avx512"
!  ERROR *** ERROR Unknown target architecture "$(CPUNR)". Make aborted.
! ENDIF
!ELSEIF "$(CPU)" == "ARM64"
# TODO: Validate CPUNR depending on the VS version.
CPUNR = armv8.0
!ENDIF

# Convert processor ID to MVC-compatible number
!IF "$(CPU)" == "i386" || "$(CPU)" == "AMD64"
# IA32/SSE/SSE2 are only supported on x86
! IF "$(CPU)" == "i386" \
	&& ("$(CPUNR)" == "i686" || "$(CPUNR)" == "any")
CPUARG = /arch:IA32
! ELSEIF "$(CPU)" == "i386" && "$(CPUNR)" == "sse"
CPUARG = /arch:SSE
! ELSEIF "$(CPU)" == "i386" && "$(CPUNR)" == "sse2"
CPUARG = /arch:SSE2
! ELSEIF "$(CPUNR)" == "avx"
CPUARG = /arch:AVX
! ELSEIF "$(CPUNR)" == "avx2"
CPUARG = /arch:AVX2
! ELSEIF "$(CPUNR)" == "avx512"
CPUARG = /arch:AVX512
! ENDIF
!ELSEIF "$(CPU)" == "ARM64" && defined(CPUNR)
CPUARG = /arch:$(CPUNR)
!ENDIF

# Pass CPUARG to GvimExt, to avoid using version-dependent defaults
MAKEFLAGS_GVIMEXT = $(MAKEFLAGS_GVIMEXT) CPUARG="$(CPUARG)"

!IF "$(VIMDLL)" == "yes"
VIMDLLBASE = vim
! IF "$(CPU)" == "i386"
VIMDLLBASE = $(VIMDLLBASE)32
! ELSE
VIMDLLBASE = $(VIMDLLBASE)64
! ENDIF
! IF "$(DEBUG)" == "yes"
VIMDLLBASE = $(VIMDLLBASE)d
! ENDIF
!ENDIF

LIBC =
DEBUGINFO = /Zi

# Use multiprocess build.
!IF "$(USE_MP)" == "yes"
CFLAGS = $(CFLAGS) /MP
!ENDIF

# Use static code analysis
!IF "$(ANALYZE)" == "yes"
CFLAGS = $(CFLAGS) /analyze
!ENDIF

# Address Sanitizer (ASAN) generally available starting with VS2019 version
# 16.9
!IF ("$(ASAN)" == "yes") && ($(MSVC_FULL) >= 192829913)
CFLAGS = $(CFLAGS) /fsanitize=address
!ENDIF

!IFDEF NODEBUG

VIM = vim
! IF "$(OPTIMIZE)" == "SPACE"
OPTFLAG = /O1
! ELSEIF "$(OPTIMIZE)" == "SPEED"
OPTFLAG = /O2
! ELSE # MAXSPEED
OPTFLAG = /Ox
! ENDIF

# Use link time code generation if not worried about size
! IF "$(OPTIMIZE)" != "SPACE"
OPTFLAG = $(OPTFLAG) /GL
! ENDIF

CFLAGS = $(CFLAGS) $(OPTFLAG) -DNDEBUG $(CPUARG)
RCFLAGS = $(RCFLAGS) -DNDEBUG
! IFDEF USE_MSVCRT
CFLAGS = $(CFLAGS) /MD
LIBC = msvcrt.lib
! ELSE
CFLAGS = $(CFLAGS) /Zl /MT
LIBC = libcmt.lib
! ENDIF

!ELSE  # DEBUG

VIM = vimd
! IF ("$(CPU)" == "i386") || ("$(CPU)" == "ix86")
DEBUGINFO = /ZI
! ENDIF
CFLAGS = $(CFLAGS) -D_DEBUG -DDEBUG /Od
RCFLAGS = $(RCFLAGS) -D_DEBUG -DDEBUG
# The /fixed:no is needed for Quantify.
LIBC = /fixed:no
! IFDEF USE_MSVCRT
CFLAGS = $(CFLAGS) /MDd
LIBC = $(LIBC) msvcrtd.lib
! ELSE
CFLAGS = $(CFLAGS) /Zl /MTd
LIBC = $(LIBC) libcmtd.lib
! ENDIF

!ENDIF # DEBUG

# Visual Studio 2005 has 'deprecated' many of the standard CRT functions
CFLAGS_DEPR = -D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE
CFLAGS = $(CFLAGS) $(CFLAGS_DEPR)

!INCLUDE .\Make_all.mak
!INCLUDE .\testdir\Make_all.mak

INCL = vim.h alloc.h ascii.h ex_cmds.h feature.h errors.h globals.h \
	keymap.h macros.h option.h os_dos.h os_win32.h proto.h regexp.h \
	spell.h structs.h termdefs.h beval.h $(NBDEBUG_INCL)

OBJ = \
	$(OUTDIR)\alloc.obj \
	$(OUTDIR)\arabic.obj \
	$(OUTDIR)\arglist.obj \
	$(OUTDIR)\autocmd.obj \
	$(OUTDIR)\beval.obj \
	$(OUTDIR)\blob.obj \
	$(OUTDIR)\blowfish.obj \
	$(OUTDIR)\buffer.obj \
	$(OUTDIR)\bufwrite.obj \
	$(OUTDIR)\change.obj \
	$(OUTDIR)\charset.obj \
	$(OUTDIR)\cindent.obj \
	$(OUTDIR)\clientserver.obj \
	$(OUTDIR)\clipboard.obj \
	$(OUTDIR)\cmdexpand.obj \
	$(OUTDIR)\cmdhist.obj \
	$(OUTDIR)\crypt.obj \
	$(OUTDIR)\crypt_zip.obj \
	$(OUTDIR)\debugger.obj \
	$(OUTDIR)\dict.obj \
	$(OUTDIR)\diff.obj \
	$(OUTDIR)\digraph.obj \
	$(OUTDIR)\drawline.obj \
	$(OUTDIR)\drawscreen.obj \
	$(OUTDIR)\edit.obj \
	$(OUTDIR)\eval.obj \
	$(OUTDIR)\evalbuffer.obj \
	$(OUTDIR)\evalfunc.obj \
	$(OUTDIR)\evalvars.obj \
	$(OUTDIR)\evalwindow.obj \
	$(OUTDIR)\ex_cmds.obj \
	$(OUTDIR)\ex_cmds2.obj \
	$(OUTDIR)\ex_docmd.obj \
	$(OUTDIR)\ex_eval.obj \
	$(OUTDIR)\ex_getln.obj \
	$(OUTDIR)\fileio.obj \
	$(OUTDIR)\filepath.obj \
	$(OUTDIR)\findfile.obj \
	$(OUTDIR)\float.obj \
	$(OUTDIR)\fold.obj \
	$(OUTDIR)\fuzzy.obj \
	$(OUTDIR)\getchar.obj \
	$(OUTDIR)\gc.obj \
	$(OUTDIR)\gui_xim.obj \
	$(OUTDIR)\hardcopy.obj \
	$(OUTDIR)\hashtab.obj \
	$(OUTDIR)\help.obj \
	$(OUTDIR)\highlight.obj \
	$(OUTDIR)\if_cscope.obj \
	$(OUTDIR)\indent.obj \
	$(OUTDIR)\insexpand.obj \
	$(OUTDIR)\json.obj \
	$(OUTDIR)\linematch.obj \
	$(OUTDIR)\list.obj \
	$(OUTDIR)\locale.obj \
	$(OUTDIR)\logfile.obj \
	$(OUTDIR)\main.obj \
	$(OUTDIR)\map.obj \
	$(OUTDIR)\mark.obj \
	$(OUTDIR)\match.obj \
	$(OUTDIR)\mbyte.obj \
	$(OUTDIR)\memfile.obj \
	$(OUTDIR)\memline.obj \
	$(OUTDIR)\menu.obj \
	$(OUTDIR)\message.obj \
	$(OUTDIR)\misc1.obj \
	$(OUTDIR)\misc2.obj \
	$(OUTDIR)\mouse.obj \
	$(OUTDIR)\move.obj \
	$(OUTDIR)\normal.obj \
	$(OUTDIR)\ops.obj \
	$(OUTDIR)\option.obj \
	$(OUTDIR)\optionstr.obj \
	$(OUTDIR)\os_mswin.obj \
	$(OUTDIR)\os_win32.obj \
	$(OUTDIR)\pathdef.obj \
	$(OUTDIR)\popupmenu.obj \
	$(OUTDIR)\popupwin.obj \
	$(OUTDIR)\profiler.obj \
	$(OUTDIR)\quickfix.obj \
	$(OUTDIR)\regexp.obj \
	$(OUTDIR)\register.obj \
	$(OUTDIR)\scriptfile.obj \
	$(OUTDIR)\screen.obj \
	$(OUTDIR)\search.obj \
	$(OUTDIR)\session.obj \
	$(OUTDIR)\sha256.obj \
	$(OUTDIR)\sign.obj \
	$(OUTDIR)\spell.obj \
	$(OUTDIR)\spellfile.obj \
	$(OUTDIR)\spellsuggest.obj \
	$(OUTDIR)\strings.obj \
	$(OUTDIR)\syntax.obj \
	$(OUTDIR)\tabpanel.obj \
	$(OUTDIR)\tag.obj \
	$(OUTDIR)\term.obj \
	$(OUTDIR)\testing.obj \
	$(OUTDIR)\textformat.obj \
	$(OUTDIR)\textobject.obj \
	$(OUTDIR)\textprop.obj \
	$(OUTDIR)\time.obj \
	$(OUTDIR)\tuple.obj \
	$(OUTDIR)\typval.obj \
	$(OUTDIR)\ui.obj \
	$(OUTDIR)\undo.obj \
	$(OUTDIR)\usercmd.obj \
	$(OUTDIR)\userfunc.obj \
	$(OUTDIR)\vim9class.obj \
	$(OUTDIR)\vim9cmds.obj \
	$(OUTDIR)\vim9compile.obj \
	$(OUTDIR)\vim9execute.obj \
	$(OUTDIR)\vim9expr.obj \
	$(OUTDIR)\vim9generics.obj \
	$(OUTDIR)\vim9instr.obj \
	$(OUTDIR)\vim9script.obj \
	$(OUTDIR)\vim9type.obj \
	$(OUTDIR)\viminfo.obj \
	$(OUTDIR)\winclip.obj \
	$(OUTDIR)\window.obj \

!IF "$(VIMDLL)" == "yes"
OBJ = $(OBJ) $(OUTDIR)\os_w32dll.obj $(OUTDIR)\vimd.res
EXEOBJC = $(OUTDIR)\os_w32exec.obj $(OUTDIR)\vimc.res
EXEOBJG = $(OUTDIR)\os_w32exeg.obj $(OUTDIR)\vimg.res
CFLAGS = $(CFLAGS) -DVIMDLL
! IFDEF MZSCHEME
EXECFLAGS =
EXELIBC = $(LIBC)
! ELSE
EXECFLAGS = -DUSE_OWNSTARTUP /GS-
EXELIBC =
! ENDIF
!ELSE
OBJ = $(OBJ) $(OUTDIR)\os_w32exe.obj $(OUTDIR)\vim.res
!ENDIF

!IF "$(OLE)" == "yes"
CFLAGS = $(CFLAGS) -DFEAT_OLE
RCFLAGS = $(RCFLAGS) -DFEAT_OLE
OLE_OBJ = $(OUTDIR)\if_ole.obj
OLE_IDL = if_ole.idl
OLE_LIB = oleaut32.lib
!ENDIF

!IFNDEF IME
IME = yes
!ENDIF
!IF "$(IME)" == "yes"
CFLAGS = $(CFLAGS) -DFEAT_MBYTE_IME
! IFNDEF DYNAMIC_IME
DYNAMIC_IME = yes
! ENDIF
! IF "$(DYNAMIC_IME)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_IME
! ELSE
IME_LIB = imm32.lib
! ENDIF
!ENDIF

!IF "$(GUI)" == "yes"
SUBSYSTEM = windows
CFLAGS = $(CFLAGS) -DFEAT_GUI_MSWIN
RCFLAGS = $(RCFLAGS) -DFEAT_GUI_MSWIN
! IF "$(VIMDLL)" == "yes"
SUBSYSTEM_CON = console
GVIM = g$(VIM)
CUI_INCL = iscygpty.h
CUI_OBJ = $(OUTDIR)\iscygpty.obj
RCFLAGS = $(RCFLAGS) -DVIMDLL
! ELSE
VIM = g$(VIM)
! ENDIF
GUI_INCL = \
	gui.h
GUI_OBJ = \
	$(OUTDIR)\gui.obj \
	$(OUTDIR)\gui_beval.obj \
	$(OUTDIR)\gui_w32.obj
GUI_LIB = \
	version.lib $(IME_LIB) winspool.lib comctl32.lib
!ELSE
SUBSYSTEM = console
CUI_INCL = iscygpty.h
CUI_OBJ = $(OUTDIR)\iscygpty.obj
!ENDIF
SUBSYSTEM_TOOLS = console

XDIFF_OBJ = $(OBJDIR)/xdiffi.obj \
	$(OBJDIR)/xemit.obj \
	$(OBJDIR)/xprepare.obj \
	$(OBJDIR)/xutils.obj \
	$(OBJDIR)/xhistogram.obj \
	$(OBJDIR)/xpatience.obj

XDIFF_DEPS = \
	xdiff/xdiff.h \
	xdiff/xdiffi.h \
	xdiff/xemit.h \
	xdiff/xinclude.h \
	xdiff/xmacros.h \
	xdiff/xprepare.h \
	xdiff/xtypes.h \
	xdiff/xutils.h


!IF "$(SUBSYSTEM_VER)" != ""
SUBSYSTEM = $(SUBSYSTEM),$(SUBSYSTEM_VER)
SUBSYSTEM_TOOLS = $(SUBSYSTEM_TOOLS),$(SUBSYSTEM_VER)
! IF "$(VIMDLL)" == "yes"
SUBSYSTEM_CON = $(SUBSYSTEM_CON),$(SUBSYSTEM_VER)
! ENDIF
# Pass SUBSYSTEM_VER to GvimExt and other tools
MAKEFLAGS_GVIMEXT = $(MAKEFLAGS_GVIMEXT) SUBSYSTEM_VER=$(SUBSYSTEM_VER)
MAKEFLAGS_TOOLS = $(MAKEFLAGS_TOOLS) SUBSYSTEM_VER=$(SUBSYSTEM_VER)
!ENDIF

!IF "$(GUI)" == "yes" && "$(DIRECTX)" == "yes"
CFLAGS = $(CFLAGS) $(DIRECTX_DEFS)
GUI_INCL = $(GUI_INCL) $(DIRECTX_INCL)
GUI_OBJ = $(GUI_OBJ) $(DIRECTX_OBJ)
!ENDIF

# iconv.dll library (dynamically loaded)
!IFNDEF ICONV
ICONV = yes
!ENDIF
!IF "$(ICONV)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_ICONV
!ENDIF

# libintl.dll library
!IFNDEF GETTEXT
GETTEXT = yes
!ENDIF
!IF "$(GETTEXT)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_GETTEXT
!ENDIF

# TCL interface
!IFDEF TCL
! IFNDEF TCL_VER
TCL_VER = 86
TCL_VER_LONG = 8.6
! ENDIF
! MESSAGE Tcl requested (version $(TCL_VER)) - root dir is "$(TCL)"
! IF "$(DYNAMIC_TCL)" == "yes"
!  MESSAGE Tcl DLL will be loaded dynamically
!  IFNDEF TCL_DLL
TCL_DLL = tcl$(TCL_VER).dll
!  ENDIF
CFLAGS = $(CFLAGS) -DFEAT_TCL -DDYNAMIC_TCL -DDYNAMIC_TCL_DLL=\"$(TCL_DLL)\" \
		-DDYNAMIC_TCL_VER=\"$(TCL_VER_LONG)\"
TCL_OBJ = $(OUTDIR)\if_tcl.obj
TCL_INC = /I "$(TCL)\Include" /I "$(TCL)"
TCL_LIB = "$(TCL)\lib\tclstub$(TCL_VER).lib"
! ELSE
CFLAGS = $(CFLAGS) -DFEAT_TCL
TCL_OBJ = $(OUTDIR)\if_tcl.obj
TCL_INC = /I "$(TCL)\Include" /I "$(TCL)"
TCL_LIB = "$(TCL)\lib\tcl$(TCL_VER)vc.lib"
! ENDIF
!ENDIF

# Lua interface
!IFDEF LUA
! IFNDEF LUA_VER
LUA_VER = 53
! ENDIF
! MESSAGE Lua requested (version $(LUA_VER)) - root dir is "$(LUA)"
! IF "$(DYNAMIC_LUA)" == "yes"
!  MESSAGE Lua DLL will be loaded dynamically
!  ENDIF
CFLAGS = $(CFLAGS) -DFEAT_LUA
LUA_OBJ = $(OUTDIR)\if_lua.obj
LUA_INC = /I "$(LUA)\include" /I "$(LUA)"
! IF "$(DYNAMIC_LUA)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_LUA \
		-DDYNAMIC_LUA_DLL=\"lua$(LUA_VER).dll\"
LUA_LIB = /nodefaultlib:lua$(LUA_VER).lib
! ELSE
LUA_LIB = "$(LUA)\lib\lua$(LUA_VER).lib"
! ENDIF
!ENDIF

!IF defined(PYTHON) && defined(PYTHON3)
DYNAMIC_PYTHON = yes
DYNAMIC_PYTHON3 = yes
!ENDIF

# PYTHON interface
!IFDEF PYTHON
! IFNDEF PYTHON_VER
PYTHON_VER = 27
! ENDIF
! MESSAGE Python requested (version $(PYTHON_VER)) - root dir is "$(PYTHON)"
! IF "$(DYNAMIC_PYTHON)" == "yes"
!  MESSAGE Python DLL will be loaded dynamically
! ENDIF
CFLAGS = $(CFLAGS) -DFEAT_PYTHON
PYTHON_OBJ = $(OUTDIR)\if_python.obj
PYTHON_INC = /I "$(PYTHON)\Include" /I "$(PYTHON)\PC"
! IF "$(DYNAMIC_PYTHON)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_PYTHON \
		-DDYNAMIC_PYTHON_DLL=\"python$(PYTHON_VER).dll\"
PYTHON_LIB = /nodefaultlib:python$(PYTHON_VER).lib
! ELSE
PYTHON_LIB = "$(PYTHON)\libs\python$(PYTHON_VER).lib"
! ENDIF
!ENDIF

# PYTHON3 interface
!IFDEF PYTHON3
! IFNDEF DYNAMIC_PYTHON3_STABLE_ABI
!  IF "$(DYNAMIC_PYTHON3)" == "yes"
DYNAMIC_PYTHON3_STABLE_ABI = yes
!  ENDIF
! ENDIF
! IFNDEF PYTHON3_VER
PYTHON3_VER = 38
! ENDIF
! IF "$(DYNAMIC_PYTHON3_STABLE_ABI)" == "yes"
PYTHON3_NAME = python3
! ELSE
PYTHON3_NAME = python$(PYTHON3_VER)
! ENDIF
! IFNDEF DYNAMIC_PYTHON3_DLL
DYNAMIC_PYTHON3_DLL = $(PYTHON3_NAME).dll
! ENDIF
! MESSAGE Python3 requested (version $(PYTHON3_VER)) - root dir is "$(PYTHON3)"
! IF "$(DYNAMIC_PYTHON3)" == "yes"
!  MESSAGE Python3 DLL will be loaded dynamically
! ENDIF
CFLAGS = $(CFLAGS) -DFEAT_PYTHON3
PYTHON3_OBJ = $(OUTDIR)\if_python3.obj
PYTHON3_INC = /I "$(PYTHON3)\Include" /I "$(PYTHON3)\PC"
! IF "$(DYNAMIC_PYTHON3)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_PYTHON3 \
		-DDYNAMIC_PYTHON3_DLL=\"$(DYNAMIC_PYTHON3_DLL)\"
!  IF "$(DYNAMIC_PYTHON3_STABLE_ABI)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_PYTHON3_STABLE_ABI
PYTHON3_INC = $(PYTHON3_INC) -DPy_LIMITED_API=0x3080000
!  ENDIF
PYTHON3_LIB = /nodefaultlib:$(PYTHON3_NAME).lib
! ELSE
CFLAGS = $(CFLAGS) -DPYTHON3_DLL=\"$(DYNAMIC_PYTHON3_DLL)\"
PYTHON3_LIB = "$(PYTHON3)\libs\$(PYTHON3_NAME).lib"
! ENDIF
!ENDIF

# MzScheme interface
!IFDEF MZSCHEME
! MESSAGE MzScheme requested - root dir is "$(MZSCHEME)"
! IFNDEF MZSCHEME_VER
MZSCHEME_VER = 3m_a0solc
! ENDIF
! IFNDEF MZSCHEME_COLLECTS
MZSCHEME_COLLECTS = $(MZSCHEME)\collects
! ENDIF
CFLAGS = $(CFLAGS) -DFEAT_MZSCHEME -I "$(MZSCHEME)\include"
! IF EXIST("$(MZSCHEME)\lib\msvc\libmzsch$(MZSCHEME_VER).lib")
MZSCHEME_MAIN_LIB = mzsch
! ELSE
MZSCHEME_MAIN_LIB = racket
! ENDIF
! IF (EXIST("$(MZSCHEME)\lib\lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).dll") \
	&& !EXIST("$(MZSCHEME)\lib\libmzgc$(MZSCHEME_VER).dll")) \
	|| (EXIST("$(MZSCHEME)\lib\msvc\lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).lib") \
	&& !EXIST("$(MZSCHEME)\lib\msvc\libmzgc$(MZSCHEME_VER).lib"))
!  MESSAGE Building with Precise GC
MZSCHEME_PRECISE_GC = yes
CFLAGS = $(CFLAGS) -DMZ_PRECISE_GC
! ENDIF
! IF "$(DYNAMIC_MZSCHEME)" == "yes"
!  MESSAGE MzScheme DLLs will be loaded dynamically
CFLAGS = $(CFLAGS) -DDYNAMIC_MZSCHEME
!  IF "$(MZSCHEME_PRECISE_GC)" == "yes"
# Precise GC does not use separate dll
CFLAGS = $(CFLAGS) \
	-DDYNAMIC_MZSCH_DLL=\"lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).dll\" \
	-DDYNAMIC_MZGC_DLL=\"lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).dll\"
!  ELSE
CFLAGS = $(CFLAGS) \
	-DDYNAMIC_MZSCH_DLL=\"lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).dll\" \
	-DDYNAMIC_MZGC_DLL=\"libmzgc$(MZSCHEME_VER).dll\"
!  ENDIF
! ELSE
!  IF "$(MZSCHEME_DEBUG)" == "yes"
CFLAGS = $(CFLAGS) -DMZSCHEME_FORCE_GC
!  ENDIF
!  IF "$(MZSCHEME_PRECISE_GC)" == "yes"
# Precise GC does not use separate dll
!   IF EXIST("$(MZSCHEME)\lib\lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).def")
# create .lib from .def
MZSCHEME_LIB = lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).lib
MZSCHEME_EXTRA_DEP = lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).lib
!   ELSE
MZSCHEME_LIB = "$(MZSCHEME)\lib\msvc\lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).lib"
!   ENDIF
!  ELSE
MZSCHEME_LIB = "$(MZSCHEME)\lib\msvc\libmzgc$(MZSCHEME_VER).lib" \
	"$(MZSCHEME)\lib\msvc\lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).lib"
!  ENDIF
! ENDIF
MZSCHEME_OBJ = $(OUTDIR)\if_mzsch.obj
# increase stack size
MZSCHEME_LIB = $(MZSCHEME_LIB) /STACK:8388608
MZSCHEME_INCL = if_mzsch.h
!ENDIF

# Perl interface
!IFDEF PERL
! IFNDEF PERL_VER
PERL_VER = 524
! ENDIF
! MESSAGE Perl requested (version $(PERL_VER)) - root dir is "$(PERL)"
! IF "$(DYNAMIC_PERL)" == "yes"
!  MESSAGE Perl DLL will be loaded dynamically
! ENDIF

# Is Perl installed in architecture-specific directories?
! IF exist($(PERL)\Bin\MSWin32-x86)
PERL_ARCH = \MSWin32-x86
! ENDIF

PERL_INCDIR = $(PERL)\Lib$(PERL_ARCH)\Core

# Version-dependent stuff
PERL_DLL = perl$(PERL_VER).dll
! IF exist($(PERL_INCDIR)\perl$(PERL_VER).lib)
PERL_LIB = $(PERL_INCDIR)\perl$(PERL_VER).lib
! ELSE
# For ActivePerl 5.18 and later
PERL_LIB = $(PERL_INCDIR)\libperl$(PERL_VER).a
! ENDIF

CFLAGS = $(CFLAGS) -DFEAT_PERL -DPERL_IMPLICIT_CONTEXT -DPERL_IMPLICIT_SYS

# Do we want to load Perl dynamically?
! IF "$(DYNAMIC_PERL)" == "yes"
CFLAGS = $(CFLAGS) -DDYNAMIC_PERL -DDYNAMIC_PERL_DLL=\"$(PERL_DLL)\"
!  UNDEF PERL_LIB
! ENDIF

PERL_EXE = $(PERL)\Bin$(PERL_ARCH)\perl
PERL_INC = /I $(PERL_INCDIR)
PERL_OBJ = $(OUTDIR)\if_perl.obj
XSUBPP = $(PERL)\lib\ExtUtils\xsubpp
! IF exist($(XSUBPP))
XSUBPP = $(PERL_EXE) $(XSUBPP)
! ELSE
XSUBPP = xsubpp
! ENDIF
XSUBPP_TYPEMAP = $(PERL)\lib\ExtUtils\typemap

!ENDIF

#
# Support Ruby interface
#
!IFDEF RUBY
#  Set default value
! IFNDEF RUBY_VER
RUBY_VER = 22
! ENDIF
! IFNDEF RUBY_VER_LONG
RUBY_VER_LONG = 2.2.0
! ENDIF
! IFNDEF RUBY_API_VER_LONG
RUBY_API_VER_LONG = $(RUBY_VER_LONG)
! ENDIF
! IFNDEF RUBY_API_VER
RUBY_API_VER = $(RUBY_API_VER_LONG:.=)
! ENDIF

! IFNDEF RUBY_PLATFORM
!  IF "$(CPU)" == "i386"
RUBY_PLATFORM = i386-mswin32
!  ELSE # CPU
RUBY_PLATFORM = x64-mswin64
!  ENDIF # CPU
RUBY_PLATFORM = $(RUBY_PLATFORM)_$(MSVCRT_VER)
! ENDIF # RUBY_PLATFORM

! IFNDEF RUBY_INSTALL_NAME
!  IFNDEF RUBY_MSVCRT_NAME
# Base name of msvcrXX.dll which is used by ruby's dll.
RUBY_MSVCRT_NAME = $(MSVCRT_NAME)
!  ENDIF # RUBY_MSVCRT_NAME
!  IF "$(CPU)" == "i386"
RUBY_INSTALL_NAME = $(RUBY_MSVCRT_NAME)-ruby$(RUBY_API_VER)
!  ELSE # CPU
!   IF EXIST($(RUBY)/lib/ruby/$(RUBY_API_VER_LONG)/x64-mingw-ucrt)
RUBY_INSTALL_NAME = x64-ucrt-ruby$(RUBY_API_VER)
!   ELSE
RUBY_INSTALL_NAME = x64-$(RUBY_MSVCRT_NAME)-ruby$(RUBY_API_VER)
!   ENDIF
!  ENDIF # CPU
! ENDIF # RUBY_INSTALL_NAME

! MESSAGE Ruby requested (version $(RUBY_VER)) - root dir is "$(RUBY)"
CFLAGS = $(CFLAGS) -DFEAT_RUBY
RUBY_OBJ = $(OUTDIR)\if_ruby.obj
RUBY_INC = /I "$(RUBY)\include\ruby-$(RUBY_API_VER_LONG)" \
	/I "$(RUBY)\include\ruby-$(RUBY_API_VER_LONG)\$(RUBY_PLATFORM)"
RUBY_LIB = "$(RUBY)\lib\$(RUBY_INSTALL_NAME).lib"
# Do we want to load Ruby dynamically?
! IF "$(DYNAMIC_RUBY)" == "yes"
!  MESSAGE Ruby DLL will be loaded dynamically
CFLAGS = $(CFLAGS) -DDYNAMIC_RUBY \
	-DDYNAMIC_RUBY_DLL=\"$(RUBY_INSTALL_NAME).dll\"
!  UNDEF RUBY_LIB
! ENDIF
CFLAGS = $(CFLAGS) -DRUBY_VERSION=$(RUBY_VER)
!ENDIF # RUBY

#
# Support PostScript printing
#
!IF "$(POSTSCRIPT)" == "yes"
CFLAGS = $(CFLAGS) -DMSWINPS
!ENDIF # POSTSCRIPT

#
# FEATURES: TINY, NORMAL, or HUGE
#
CFLAGS = $(CFLAGS) -DFEAT_$(FEATURES)

#
# MODIFIED_BY - Name of who modified a release version
#
!IF "$(MODIFIED_BY)" != ""
CFLAGS = $(CFLAGS) -DMODIFIED_BY=\"$(MODIFIED_BY)\"
!ENDIF

#
# Always generate the .pdb file, so that we get debug symbols that can be used
# on a crash (doesn't add overhead to the executable).
# Generate edit-and-continue debug info when no optimization - allows to
# debug more conveniently (able to look at variables which are in registers)
#
CFLAGS = $(CFLAGS) /Fd$(OUTDIR)/ $(DEBUGINFO)
!IF "$(VIMDLL)" == "yes"
LINK_PDB = /PDB:$(VIMDLLBASE).pdb -debug
!ELSE
LINK_PDB = /PDB:$(VIM).pdb -debug
!ENDIF

#
# End extra feature include
#
!MESSAGE

# CFLAGS with /Fo$(OUTDIR)/
CFLAGS_OUTDIR = $(CFLAGS) /Fo$(OUTDIR)/

PATHDEF_SRC = $(OUTDIR)\pathdef.c

LINKARGS1 = /nologo
LINKARGS2 = $(CON_LIB) $(GUI_LIB) $(LIBC) $(OLE_LIB) \
	$(LUA_LIB) $(MZSCHEME_LIB) $(PERL_LIB) $(PYTHON_LIB) \
	$(PYTHON3_LIB) $(RUBY_LIB) $(TCL_LIB) $(SOUND_LIB) \
	$(NETBEANS_LIB) $(XPM_LIB) $(SOD_LIB) $(LINK_PDB)

!IFDEF NODEBUG
# Add /opt:ref to remove unreferenced functions and data even when /DEBUG is
# added.
LINKARGS1 = $(LINKARGS1) /opt:ref
!ELSE
LINKARGS1 = $(LINKARGS1) /opt:noref /opt:noicf
!ENDIF

!IF "$(MAP)" == "yes"
# "/map" is for debugging
LINKARGS1 = $(LINKARGS1) /map
!ELSEIF "$(MAP)" == "lines"
# "/mapinfo:lines" is for debugging, only works for VC6 and later
LINKARGS1 = $(LINKARGS1) /map /mapinfo:lines
!ENDIF

# Enable link time code generation if needed.
!IFDEF NODEBUG
! IF "$(OPTIMIZE)" != "SPACE"
!  IF "$(CI)" == "true" || "$(CI)" == "True"
# Enable link time code generation, but do not show the progress.
LINKARGS1 = $(LINKARGS1) /LTCG
!  ELSE
# Report link time code generation progress.
LINKARGS1 = $(LINKARGS1) /LTCG:STATUS
!  ENDIF
! ENDIF
!ENDIF

!IF "$(CPU)" == "AMD64" && "$(GUI)" == "yes"
# This option is required for VC2012 or later so that 64-bit gvim can
# accept D&D from 32-bit applications.  NOTE: This disables 64-bit ASLR,
# therefore the security level becomes as same as VC2010.
LINKARGS1 = $(LINKARGS1) /HIGHENTROPYVA:NO
!ENDIF

!IF "$(VIMDLL)" == "yes"
MAIN_TARGET = $(GVIM).exe $(VIM).exe $(VIMDLLBASE).dll
!ELSE
MAIN_TARGET = $(VIM).exe
!ENDIF

# Target to run individual tests.
VIMTESTTARGET = $(VIM).exe

all: $(MAIN_TARGET) \
	vimrun.exe \
	install.exe \
	uninstall.exe \
	xxd/xxd.exe \
	tee/tee.exe \
	GvimExt/gvimext.dll

# To get around the command line limit: Make use of nmake's response files to
# capture the arguments for $(LINK) in a file  using the @<<ARGS<< syntax.

!IF "$(VIMDLL)" == "yes"

$(VIMDLLBASE).dll: $(OUTDIR) $(OBJ) $(XDIFF_OBJ) $(GUI_OBJ) $(CUI_OBJ) \
		$(OLE_OBJ) $(OLE_IDL) $(MZSCHEME_OBJ) $(LUA_OBJ) $(PERL_OBJ) \
		$(PYTHON_OBJ) $(PYTHON3_OBJ) $(RUBY_OBJ) $(TCL_OBJ) \
		$(TERM_OBJ) $(SOUND_OBJ) $(NETBEANS_OBJ) $(CHANNEL_OBJ) \
		$(XPM_OBJ) version.c version.h
	$(CC) $(CFLAGS_OUTDIR) version.c
	$(LINK) @<<
$(LINKARGS1) /dll -out:$(VIMDLLBASE).dll $(OBJ) $(XDIFF_OBJ)
$(GUI_OBJ) $(CUI_OBJ) $(OLE_OBJ) $(LUA_OBJ) $(MZSCHEME_OBJ) $(PERL_OBJ)
$(PYTHON_OBJ) $(PYTHON3_OBJ) $(RUBY_OBJ) $(TCL_OBJ) $(TERM_OBJ) $(SOUND_OBJ)
$(NETBEANS_OBJ) $(CHANNEL_OBJ) $(XPM_OBJ) $(OUTDIR)\version.obj $(LINKARGS2)
<<

$(GVIM).exe: $(OUTDIR) $(EXEOBJG) $(VIMDLLBASE).dll
	$(LINK) $(LINKARGS1) /subsystem:$(SUBSYSTEM) -out:$(GVIM).exe \
		$(EXEOBJG) $(VIMDLLBASE).lib $(EXELIBC)

$(VIM).exe: $(OUTDIR) $(EXEOBJC) $(VIMDLLBASE).dll
	$(LINK) $(LINKARGS1) /subsystem:$(SUBSYSTEM_CON) -out:$(VIM).exe \
		$(EXEOBJC) $(VIMDLLBASE).lib $(EXELIBC)

!ELSE

$(VIM).exe: $(OUTDIR) $(OBJ) $(XDIFF_OBJ) $(GUI_OBJ) $(CUI_OBJ) \
		$(OLE_OBJ) $(OLE_IDL) $(MZSCHEME_OBJ) $(LUA_OBJ) $(PERL_OBJ) \
		$(PYTHON_OBJ) $(PYTHON3_OBJ) $(RUBY_OBJ) $(TCL_OBJ) \
		$(TERM_OBJ) $(SOUND_OBJ) $(NETBEANS_OBJ) $(CHANNEL_OBJ) \
		$(XPM_OBJ) version.c version.h
	$(CC) $(CFLAGS_OUTDIR) version.c
	$(LINK) @<<
$(LINKARGS1) /subsystem:$(SUBSYSTEM) -out:$(VIM).exe $(OBJ) $(XDIFF_OBJ)
$(GUI_OBJ) $(CUI_OBJ) $(OLE_OBJ) $(LUA_OBJ) $(MZSCHEME_OBJ) $(PERL_OBJ)
$(PYTHON_OBJ) $(PYTHON3_OBJ) $(RUBY_OBJ) $(TCL_OBJ) $(TERM_OBJ) $(SOUND_OBJ)
$(NETBEANS_OBJ) $(CHANNEL_OBJ) $(XPM_OBJ) $(OUTDIR)\version.obj $(LINKARGS2)
<<

!ENDIF

$(VIM): $(VIM).exe

$(OUTDIR):
	@ if not exist $(OUTDIR)/nul  $(MKD) $(OUTDIR:/=\)

$(OUTDIR)/libvterm: $(OUTDIR)
	@ if not exist $(OUTDIR)/libvterm/nul  $(MKD) $(OUTDIR:/=\)\libvterm

CFLAGS_INST = /nologo /O2 -DNDEBUG -DWIN32 -DWINVER=$(WINVER) \
	-D_WIN32_WINNT=$(WINVER) $(CFLAGS_DEPR)

CFLAGS_INST = $(CFLAGS_INST) -DVIM_VERSION_PATCHLEVEL=$(PATCHLEVEL)

install.exe: dosinst.c dosinst.h version.h
	$(CC) $(CFLAGS_INST) /Fe$@ dosinst.c kernel32.lib shell32.lib \
		user32.lib ole32.lib advapi32.lib uuid.lib \
		-link -subsystem:$(SUBSYSTEM_TOOLS)

uninstall.exe: uninstall.c dosinst.h version.h
	$(CC) $(CFLAGS_INST) uninstall.c shell32.lib advapi32.lib \
		-link -subsystem:$(SUBSYSTEM_TOOLS)

vimrun.exe: vimrun.c
	$(CC) /nologo -DNDEBUG vimrun.c -link -subsystem:$(SUBSYSTEM_TOOLS)

xxd/xxd.exe: xxd/xxd.c
	cd xxd
	$(MAKE) -lf Make_mvc.mak $(MAKEFLAGS_TOOLS)
	cd ..

tee/tee.exe: tee/tee.c
	cd tee
	$(MAKE) -lf Make_mvc.mak $(MAKEFLAGS_TOOLS)
	cd ..

GvimExt/gvimext.dll: GvimExt/gvimext.cpp GvimExt/gvimext.rc GvimExt/gvimext.h
	cd GvimExt
	$(MAKE) -lf Make_mvc.mak $(MAKEFLAGS_GVIMEXT)
	cd ..


tags: notags
	$(CTAGS) $(TAGS_FILES)

notags:
	- if exist tags $(RM) tags

clean: testclean
	- if exist $(OUTDIR)/nul $(DELTREE) $(OUTDIR)
	- if exist *.obj $(RM) *.obj
	- if exist $(VIM).exe $(RM) $(VIM).exe
	- if exist $(VIM).exp $(RM) $(VIM).exp
	- if exist $(VIM).lib $(RM) $(VIM).lib
	- if exist $(VIM).ilk $(RM) $(VIM).ilk
	- if exist $(VIM).pdb $(RM) $(VIM).pdb
	- if exist $(VIM).map $(RM) $(VIM).map
	- if exist $(VIM).ncb $(RM) $(VIM).ncb
!IF "$(VIMDLL)" == "yes"
	- if exist $(GVIM).exe $(RM) $(GVIM).exe
	- if exist $(GVIM).exp $(RM) $(GVIM).exp
	- if exist $(GVIM).lib $(RM) $(GVIM).lib
	- if exist $(GVIM).map $(RM) $(GVIM).map
	- if exist $(VIMDLLBASE).dll $(RM) $(VIMDLLBASE).dll
	- if exist $(VIMDLLBASE).ilk $(RM) $(VIMDLLBASE).ilk
	- if exist $(VIMDLLBASE).lib $(RM) $(VIMDLLBASE).lib
	- if exist $(VIMDLLBASE).exp $(RM) $(VIMDLLBASE).exp
	- if exist $(VIMDLLBASE).pdb $(RM) $(VIMDLLBASE).pdb
	- if exist $(VIMDLLBASE).map $(RM) $(VIMDLLBASE).map
!ENDIF
	- if exist vimrun.exe $(RM) vimrun.exe
	- if exist install.exe $(RM) install.exe
	- if exist uninstall.exe $(RM) uninstall.exe
	- if exist if_perl.c $(RM) if_perl.c
	- if exist auto\if_perl.c $(RM) auto\if_perl.c
	- if exist dosinst.exe $(RM) dosinst.exe
	cd xxd
	$(MAKE) -lf Make_mvc.mak clean
	cd ..
	cd tee
	$(MAKE) -lf Make_mvc.mak clean
	cd ..
	cd GvimExt
	$(MAKE) -lf Make_mvc.mak clean
	cd ..

# Run Vim script to generate the Ex command lookup table.
# This only needs to be run when a command name has been added or changed.
# If this fails because you don't have Vim yet, first build and install Vim
# without changes.
cmdidxs: ex_cmds.h
	vim.exe --clean -N -X --not-a-term -u create_cmdidxs.vim -c quit

# Run Vim script to generate the normal/visual mode command lookup table.
# This only needs to be run when a new normal/visual mode command has been
# added.  If this fails because you don't have Vim yet:
#   - change nv_cmds[] in nv_cmds.h to add the new normal/visual mode command.
#   - run "make nvcmdidxs" to generate nv_cmdidxs.h
nvcmdidxs: nv_cmds.h
	$(CC) /nologo -I. -Iproto -DNDEBUG create_nvcmdidxs.c \
		-link -subsystem:$(SUBSYSTEM_TOOLS)
	vim.exe --clean -N -X --not-a-term -u create_nvcmdidxs.vim -c quit
	- $(RM) create_nvcmdidxs.exe

test:
	cd testdir
	$(MAKE) -lf Make_mvc.mak
	cd ..

testgvim testgui:
	cd testdir
	$(MAKE) -lf Make_mvc.mak "VIMPROG=..\gvim.exe"
	cd ..

testtiny:
	cd testdir
	$(MAKE) -lf Make_mvc.mak tiny
	cd ..

testgvimtiny:
	cd testdir
	$(MAKE) -lf Make_mvc.mak "VIMPROG=..\gvim.exe" tiny
	cd ..

testclean:
	cd testdir
	$(MAKE) -lf Make_mvc.mak clean
	cd ..

# Run individual OLD style test.
# These do not depend on the executable, compile it when needed.
$(SCRIPTS_TINY):
	cd testdir
	- if exist $@.out $(RM) $@.out
	$(MAKE) -lf Make_mvc.mak VIMPROG=..\$(VIMTESTTARGET) nolog
	$(MAKE) -lf Make_mvc.mak VIMPROG=..\$(VIMTESTTARGET) $@.out
	@ if exist test.log ( type test.log & exit /b 1 )
	cd ..

# Run individual NEW style test.
# These do not depend on the executable, compile it when needed.
$(NEW_TESTS):
	cd testdir
	- if exist $@.res $(RM) $@.res
	$(MAKE) -lf Make_mvc.mak VIMPROG=..\$(VIMTESTTARGET) nolog
	$(MAKE) -lf Make_mvc.mak VIMPROG=..\$(VIMTESTTARGET) $@.res
	$(MAKE) -lf Make_mvc.mak VIMPROG=..\$(VIMTESTTARGET) report
	cd ..

# Run Vim9 tests.
# These do not depend on the executable, compile it when needed.
test_vim9:
	cd testdir
	- $(RM) test_vim9_*.res
	$(MAKE) -lf Make_mvc.mak VIMPROG=..\$(VIMTESTTARGET) nolog
	$(MAKE) -lf Make_mvc.mak VIMPROG=..\$(VIMTESTTARGET) $(TEST_VIM9_RES)
	$(MAKE) -lf Make_mvc.mak VIMPROG=..\$(VIMTESTTARGET) report
	cd ..

###########################################################################

# Create a default rule for transforming .c files to .obj files in $(OUTDIR)
.c{$(OUTDIR)}.obj::
	$(CC) $(CFLAGS_OUTDIR) $<

# Create a default rule for xdiff.
{xdiff}.c{$(OUTDIR)}.obj::
	$(CC) $(CFLAGS_OUTDIR) $<

# Create a default rule for transforming .cpp files to .obj files in $(OUTDIR)
.cpp{$(OUTDIR)}.obj::
	$(CC) $(CFLAGS_OUTDIR) $<

$(OUTDIR)/alloc.obj: $(OUTDIR) alloc.c $(INCL)

$(OUTDIR)/arabic.obj: $(OUTDIR) arabic.c $(INCL)

$(OUTDIR)/arglist.obj: $(OUTDIR) arglist.c $(INCL)

$(OUTDIR)/autocmd.obj: $(OUTDIR) autocmd.c $(INCL)

$(OUTDIR)/beval.obj: $(OUTDIR) beval.c $(INCL)

$(OUTDIR)/blob.obj: $(OUTDIR) blob.c $(INCL)

$(OUTDIR)/blowfish.obj: $(OUTDIR) blowfish.c $(INCL)

$(OUTDIR)/buffer.obj: $(OUTDIR) buffer.c $(INCL) version.h

$(OUTDIR)/bufwrite.obj: $(OUTDIR) bufwrite.c $(INCL)

$(OUTDIR)/change.obj: $(OUTDIR) change.c $(INCL)

$(OUTDIR)/charset.obj: $(OUTDIR) charset.c $(INCL)

$(OUTDIR)/cindent.obj: $(OUTDIR) cindent.c $(INCL)

$(OUTDIR)/clientserver.obj: $(OUTDIR) clientserver.c $(INCL)

$(OUTDIR)/clipboard.obj: $(OUTDIR) clipboard.c $(INCL)

$(OUTDIR)/cmdexpand.obj: $(OUTDIR) cmdexpand.c $(INCL)

$(OUTDIR)/cmdhist.obj: $(OUTDIR) cmdhist.c $(INCL)

$(OUTDIR)/crypt.obj: $(OUTDIR) crypt.c $(INCL)

$(OUTDIR)/crypt_zip.obj: $(OUTDIR) crypt_zip.c $(INCL)

$(OUTDIR)/debugger.obj: $(OUTDIR) debugger.c $(INCL)

$(OUTDIR)/dict.obj: $(OUTDIR) dict.c $(INCL)

$(OUTDIR)/diff.obj: $(OUTDIR) diff.c $(INCL)

$(OUTDIR)/xdiffi.obj: $(OUTDIR) xdiff/xdiffi.c $(XDIFF_DEPS)

$(OUTDIR)/xemit.obj: $(OUTDIR) xdiff/xemit.c $(XDIFF_DEPS)

$(OUTDIR)/xprepare.obj: $(OUTDIR) xdiff/xprepare.c $(XDIFF_DEPS)

$(OUTDIR)/xutils.obj: $(OUTDIR) xdiff/xutils.c $(XDIFF_DEPS)

$(OUTDIR)/xhistogram.obj: $(OUTDIR) xdiff/xhistogram.c $(XDIFF_DEPS)

$(OUTDIR)/xpatience.obj: $(OUTDIR) xdiff/xpatience.c $(XDIFF_DEPS)

$(OUTDIR)/digraph.obj: $(OUTDIR) digraph.c $(INCL)

$(OUTDIR)/drawline.obj: $(OUTDIR) drawline.c $(INCL)

$(OUTDIR)/drawscreen.obj: $(OUTDIR) drawscreen.c $(INCL)

$(OUTDIR)/edit.obj: $(OUTDIR) edit.c $(INCL)

$(OUTDIR)/eval.obj: $(OUTDIR) eval.c $(INCL)

$(OUTDIR)/evalbuffer.obj: $(OUTDIR) evalbuffer.c $(INCL)

$(OUTDIR)/evalfunc.obj: $(OUTDIR) evalfunc.c $(INCL) version.h

$(OUTDIR)/evalvars.obj: $(OUTDIR) evalvars.c $(INCL) version.h

$(OUTDIR)/evalwindow.obj: $(OUTDIR) evalwindow.c $(INCL)

$(OUTDIR)/ex_cmds.obj: $(OUTDIR) ex_cmds.c $(INCL) version.h

$(OUTDIR)/ex_cmds2.obj: $(OUTDIR) ex_cmds2.c $(INCL) version.h

$(OUTDIR)/ex_docmd.obj: $(OUTDIR) ex_docmd.c $(INCL) ex_cmdidxs.h

$(OUTDIR)/ex_eval.obj: $(OUTDIR) ex_eval.c $(INCL)

$(OUTDIR)/ex_getln.obj: $(OUTDIR) ex_getln.c $(INCL)

$(OUTDIR)/fileio.obj: $(OUTDIR) fileio.c $(INCL)

$(OUTDIR)/filepath.obj: $(OUTDIR) filepath.c $(INCL)

$(OUTDIR)/findfile.obj: $(OUTDIR) findfile.c $(INCL)

$(OUTDIR)/float.obj: $(OUTDIR) float.c $(INCL)

$(OUTDIR)/fold.obj: $(OUTDIR) fold.c $(INCL)

$(OUTDIR)/fuzzy.obj: $(OUTDIR) fuzzy.c $(INCL)

$(OUTDIR)/getchar.obj: $(OUTDIR) getchar.c $(INCL)

$(OUTDIR)/gc.obj: $(OUTDIR) gc.c $(INCL)

$(OUTDIR)/gui_xim.obj: $(OUTDIR) gui_xim.c $(INCL)

$(OUTDIR)/hardcopy.obj: $(OUTDIR) hardcopy.c $(INCL) version.h

$(OUTDIR)/hashtab.obj: $(OUTDIR) hashtab.c $(INCL)

$(OUTDIR)/help.obj: $(OUTDIR) help.c $(INCL)

$(OUTDIR)/highlight.obj: $(OUTDIR) highlight.c $(INCL)

$(OUTDIR)/indent.obj: $(OUTDIR) indent.c $(INCL)

$(OUTDIR)/insexpand.obj: $(OUTDIR) insexpand.c $(INCL)

$(OUTDIR)/gui.obj: $(OUTDIR) gui.c $(INCL) $(GUI_INCL)

$(OUTDIR)/gui_beval.obj: $(OUTDIR) gui_beval.c $(INCL) $(GUI_INCL)

$(OUTDIR)/gui_w32.obj: $(OUTDIR) gui_w32.c $(INCL) $(GUI_INCL) version.h

$(OUTDIR)/gui_dwrite.obj: $(OUTDIR) gui_dwrite.cpp gui_dwrite.h

$(OUTDIR)/if_cscope.obj: $(OUTDIR) if_cscope.c $(INCL)

$(OUTDIR)/if_lua.obj: $(OUTDIR) if_lua.c $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(LUA_INC) if_lua.c

auto/if_perl.c: if_perl.xs typemap
	$(XSUBPP) -prototypes -typemap $(XSUBPP_TYPEMAP) \
		-typemap typemap if_perl.xs -output $@

$(OUTDIR)/if_perl.obj: $(OUTDIR) auto/if_perl.c $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(PERL_INC) auto/if_perl.c

$(OUTDIR)/if_mzsch.obj: $(OUTDIR) if_mzsch.c $(MZSCHEME_INCL) $(INCL) \
			$(MZSCHEME_EXTRA_DEP)
	$(CC) $(CFLAGS_OUTDIR) if_mzsch.c \
		-DMZSCHEME_COLLECTS="\"$(MZSCHEME_COLLECTS:\=\\)\""

lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).lib:
	lib /DEF:"$(MZSCHEME)\lib\lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).def"

$(OUTDIR)/if_python.obj: $(OUTDIR) if_python.c if_py_both.h $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(PYTHON_INC) if_python.c

$(OUTDIR)/if_python3.obj: $(OUTDIR) if_python3.c if_py_both.h $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(PYTHON3_INC) if_python3.c

$(OUTDIR)/if_ole.obj: $(OUTDIR) if_ole.cpp $(INCL) if_ole.h

$(OUTDIR)/if_ruby.obj: $(OUTDIR) if_ruby.c $(INCL) version.h
	$(CC) $(CFLAGS_OUTDIR) $(RUBY_INC) if_ruby.c

$(OUTDIR)/if_tcl.obj: $(OUTDIR) if_tcl.c $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(TCL_INC) if_tcl.c

$(OUTDIR)/iscygpty.obj: $(OUTDIR) iscygpty.c $(CUI_INCL)

$(OUTDIR)/job.obj: $(OUTDIR) job.c $(INCL)

$(OUTDIR)/json.obj: $(OUTDIR) json.c $(INCL)

$(OUTDIR)/linematch.obj: $(OUTDIR) linematch.c $(INCL)

$(OUTDIR)/list.obj: $(OUTDIR) list.c $(INCL)

$(OUTDIR)/locale.obj: $(OUTDIR) locale.c $(INCL)

$(OUTDIR)/logfile.obj: $(OUTDIR) logfile.c $(INCL)

$(OUTDIR)/main.obj: $(OUTDIR) main.c $(INCL) $(CUI_INCL)

$(OUTDIR)/map.obj: $(OUTDIR) map.c $(INCL)

$(OUTDIR)/mark.obj: $(OUTDIR) mark.c $(INCL)

$(OUTDIR)/match.obj: $(OUTDIR) match.c $(INCL)

$(OUTDIR)/memfile.obj: $(OUTDIR) memfile.c $(INCL)

$(OUTDIR)/memline.obj: $(OUTDIR) memline.c $(INCL)

$(OUTDIR)/menu.obj: $(OUTDIR) menu.c $(INCL)

$(OUTDIR)/message.obj: $(OUTDIR) message.c $(INCL)

$(OUTDIR)/misc1.obj: $(OUTDIR) misc1.c $(INCL) version.h

$(OUTDIR)/misc2.obj: $(OUTDIR) misc2.c $(INCL)

$(OUTDIR)/mouse.obj: $(OUTDIR) mouse.c $(INCL)

$(OUTDIR)/move.obj: $(OUTDIR) move.c $(INCL)

$(OUTDIR)/mbyte.obj: $(OUTDIR) mbyte.c $(INCL)

$(OUTDIR)/netbeans.obj: $(OUTDIR) netbeans.c $(NBDEBUG_SRC) $(INCL) version.h

$(OUTDIR)/channel.obj: $(OUTDIR) channel.c $(INCL)

$(OUTDIR)/normal.obj: $(OUTDIR) normal.c $(INCL) nv_cmdidxs.h nv_cmds.h

$(OUTDIR)/option.obj: $(OUTDIR) option.c $(INCL) optiondefs.h

$(OUTDIR)/optionstr.obj: $(OUTDIR) optionstr.c $(INCL)

$(OUTDIR)/ops.obj: $(OUTDIR) ops.c $(INCL)

$(OUTDIR)/os_mswin.obj: $(OUTDIR) os_mswin.c $(INCL)

$(OUTDIR)/terminal.obj: $(OUTDIR) terminal.c $(INCL) $(TERM_DEPS)

$(OUTDIR)/winclip.obj: $(OUTDIR) winclip.c $(INCL)

$(OUTDIR)/os_win32.obj: $(OUTDIR) os_win32.c $(INCL) $(MZSCHEME_INCL)

$(OUTDIR)/os_w32dll.obj: $(OUTDIR) os_w32dll.c

$(OUTDIR)/os_w32exe.obj: $(OUTDIR) os_w32exe.c $(INCL)

$(OUTDIR)/os_w32exec.obj: $(OUTDIR) os_w32exe.c $(INCL)
	$(CC) $(CFLAGS:-DFEAT_GUI_MSWIN=) $(EXECFLAGS) /Fo$@ os_w32exe.c

$(OUTDIR)/os_w32exeg.obj: $(OUTDIR) os_w32exe.c $(INCL)
	$(CC) $(CFLAGS) $(EXECFLAGS) /Fo$@ os_w32exe.c

$(OUTDIR)/pathdef.obj: $(OUTDIR) $(PATHDEF_SRC) $(INCL)
	$(CC) $(CFLAGS_OUTDIR) $(PATHDEF_SRC)

$(OUTDIR)/popupmenu.obj: $(OUTDIR) popupmenu.c $(INCL)

$(OUTDIR)/popupwin.obj: $(OUTDIR) popupwin.c $(INCL)

$(OUTDIR)/profiler.obj: $(OUTDIR) profiler.c $(INCL)

$(OUTDIR)/quickfix.obj: $(OUTDIR) quickfix.c $(INCL)

$(OUTDIR)/regexp.obj: $(OUTDIR) regexp.c regexp_bt.c regexp_nfa.c $(INCL)

$(OUTDIR)/register.obj: $(OUTDIR) register.c $(INCL)

$(OUTDIR)/scriptfile.obj: $(OUTDIR) scriptfile.c $(INCL)

$(OUTDIR)/screen.obj: $(OUTDIR) screen.c $(INCL)

$(OUTDIR)/search.obj: $(OUTDIR) search.c $(INCL)

$(OUTDIR)/session.obj: $(OUTDIR) session.c $(INCL)

$(OUTDIR)/sha256.obj: $(OUTDIR) sha256.c $(INCL)

$(OUTDIR)/sign.obj: $(OUTDIR) sign.c $(INCL)

$(OUTDIR)/spell.obj: $(OUTDIR) spell.c $(INCL)

$(OUTDIR)/spellfile.obj: $(OUTDIR) spellfile.c $(INCL)

$(OUTDIR)/spellsuggest.obj: $(OUTDIR) spellsuggest.c $(INCL)

$(OUTDIR)/strings.obj: $(OUTDIR) strings.c $(INCL)

$(OUTDIR)/syntax.obj: $(OUTDIR) syntax.c $(INCL)

$(OUTDIR)/tabpanel.obj: $(OUTDIR) tabpanel.c $(INCL)

$(OUTDIR)/tag.obj: $(OUTDIR) tag.c $(INCL)

$(OUTDIR)/term.obj: $(OUTDIR) term.c $(INCL)

$(OUTDIR)/term.obj: $(OUTDIR) testing.c $(INCL)

$(OUTDIR)/textformat.obj: $(OUTDIR) textformat.c $(INCL)

$(OUTDIR)/textobject.obj: $(OUTDIR) textobject.c $(INCL)

$(OUTDIR)/textprop.obj: $(OUTDIR) textprop.c $(INCL)

$(OUTDIR)/time.obj: $(OUTDIR) time.c $(INCL)

$(OUTDIR)/tuple.obj: $(OUTDIR) tuple.c $(INCL)

$(OUTDIR)/typval.obj: $(OUTDIR) typval.c $(INCL)

$(OUTDIR)/ui.obj: $(OUTDIR) ui.c $(INCL)

$(OUTDIR)/undo.obj: $(OUTDIR) undo.c $(INCL)

$(OUTDIR)/usercmd.obj: $(OUTDIR) usercmd.c $(INCL)

$(OUTDIR)/userfunc.obj: $(OUTDIR) userfunc.c $(INCL)

$(OUTDIR)/version.obj: $(OUTDIR) version.c $(INCL) version.h

$(OUTDIR)/vim9class.obj: $(OUTDIR) vim9class.c $(INCL) vim9.h

$(OUTDIR)/vim9cmds.obj: $(OUTDIR) vim9cmds.c $(INCL) vim9.h

$(OUTDIR)/vim9compile.obj: $(OUTDIR) vim9compile.c $(INCL) vim9.h

$(OUTDIR)/vim9execute.obj: $(OUTDIR) vim9execute.c $(INCL) vim9.h

$(OUTDIR)/vim9expr.obj: $(OUTDIR) vim9expr.c $(INCL) vim9.h

$(OUTDIR)/vim9generics.obj: $(OUTDIR) vim9generics.c $(INCL) vim9.h

$(OUTDIR)/vim9instr.obj: $(OUTDIR) vim9instr.c $(INCL) vim9.h

$(OUTDIR)/vim9script.obj: $(OUTDIR) vim9script.c $(INCL) vim9.h

$(OUTDIR)/vim9type.obj: $(OUTDIR) vim9type.c $(INCL) vim9.h

$(OUTDIR)/viminfo.obj: $(OUTDIR) viminfo.c $(INCL) version.h

$(OUTDIR)/window.obj: $(OUTDIR) window.c $(INCL)

$(OUTDIR)/xpm_w32.obj: $(OUTDIR) xpm_w32.c
	$(CC) $(CFLAGS_OUTDIR) $(XPM_INC) xpm_w32.c

!IF "$(VIMDLL)" == "yes"
$(OUTDIR)/vimc.res: $(OUTDIR) vim.rc vim.manifest version.h gui_w32_rc.h \
				vim.ico
	$(RC) /nologo /l 0x409 /Fo$@ $(RCFLAGS:-DFEAT_GUI_MSWIN=) vim.rc

$(OUTDIR)/vimg.res: $(OUTDIR) vim.rc vim.manifest version.h gui_w32_rc.h \
				vim.ico
	$(RC) /nologo /l 0x409 /Fo$@ $(RCFLAGS) vim.rc

$(OUTDIR)/vimd.res: $(OUTDIR) vim.rc version.h gui_w32_rc.h \
			tools.bmp tearoff.bmp vim.ico vim_error.ico \
			vim_alert.ico vim_info.ico vim_quest.ico
	$(RC) /nologo /l 0x409 /Fo$@ $(RCFLAGS) \
		-DRCDLL -DVIMDLLBASE=\"$(VIMDLLBASE)\" vim.rc
!ELSE
$(OUTDIR)/vim.res: $(OUTDIR) vim.rc vim.manifest version.h gui_w32_rc.h \
			tools.bmp tearoff.bmp vim.ico vim_error.ico \
			vim_alert.ico vim_info.ico vim_quest.ico
	$(RC) /nologo /l 0x409 /Fo$@ $(RCFLAGS) vim.rc
!ENDIF

iid_ole.c if_ole.h vim.tlb: if_ole.idl
	midl /nologo /error none /proxy nul /iid iid_ole.c /tlb vim.tlb \
		/header if_ole.h if_ole.idl


CCCTERM = $(CC) $(CFLAGS) -Ilibvterm/include -DINLINE="" \
	-DVSNPRINTF=vim_vsnprintf \
	-DSNPRINTF=vim_snprintf \
	-DIS_COMBINING_FUNCTION=utf_iscomposing_uint \
	-DWCWIDTH_FUNCTION=utf_uint2cells \
	-DGET_SPECIAL_PTY_TYPE_FUNCTION=get_special_pty_type \
	-D_CRT_SECURE_NO_WARNINGS

# Create a default rule for vterm.
{libvterm/src}.c{$(OUTDIR)/libvterm}.obj::
	$(CCCTERM) /Fo$(OUTDIR)/libvterm/ $<

$(OUTDIR)/libvterm/encoding.obj: $(OUTDIR)/libvterm libvterm/src/encoding.c \
				$(TERM_DEPS)

$(OUTDIR)/libvterm/keyboard.obj: $(OUTDIR)/libvterm libvterm/src/keyboard.c \
				$(TERM_DEPS)

$(OUTDIR)/libvterm/mouse.obj: $(OUTDIR)/libvterm libvterm/src/mouse.c \
				$(TERM_DEPS)

$(OUTDIR)/libvterm/parser.obj: $(OUTDIR)/libvterm libvterm/src/parser.c \
				$(TERM_DEPS)

$(OUTDIR)/libvterm/pen.obj: $(OUTDIR)/libvterm libvterm/src/pen.c $(TERM_DEPS)

$(OUTDIR)/libvterm/screen.obj: $(OUTDIR)/libvterm libvterm/src/screen.c \
				$(TERM_DEPS)

$(OUTDIR)/libvterm/state.obj: $(OUTDIR)/libvterm libvterm/src/state.c \
				$(TERM_DEPS)

$(OUTDIR)/libvterm/unicode.obj: $(OUTDIR)/libvterm libvterm/src/unicode.c \
				$(TERM_DEPS)

$(OUTDIR)/libvterm/vterm.obj: $(OUTDIR)/libvterm libvterm/src/vterm.c \
				$(TERM_DEPS)


# $CFLAGS may contain backslashes, quotes and chevrons, escape them all.
E0_CFLAGS = $(CFLAGS:\=\\)
E00_CFLAGS = $(E0_CFLAGS:"=\")
# ") stop the string
E000_CFLAGS = $(E00_CFLAGS:<=^^<)
E_CFLAGS = $(E000_CFLAGS:>=^^>)
# $LINKARGS2 may contain backslashes, quotes and chevrons, escape them all.
E0_LINKARGS2 = $(LINKARGS2:\=\\)
E00_LINKARGS2 = $(E0_LINKARGS2:"=\")
# ") stop the string
E000_LINKARGS2 = $(E00_LINKARGS2:<=^^<)
E_LINKARGS2 = $(E000_LINKARGS2:>=^^>)

$(PATHDEF_SRC): Make_mvc.mak
	@ echo creating $(PATHDEF_SRC)
	@ echo /* pathdef.c */ > $(PATHDEF_SRC)
	@ echo #include "vim.h" >> $(PATHDEF_SRC)
	@ echo char_u *default_vim_dir = (char_u *)"$(VIMRCLOC:\=\\)"; \
		>> $(PATHDEF_SRC)
	@ echo char_u *default_vimruntime_dir = \
		(char_u *)"$(VIMRUNTIMEDIR:\=\\)"; >> $(PATHDEF_SRC)
	@ echo char_u *all_cflags = (char_u *)"$(CC:\=\\) $(E_CFLAGS)"; \
		>> $(PATHDEF_SRC)
	@ echo char_u *all_lflags = \
		(char_u *)"$(LINK:\=\\) $(LINKARGS1:\=\\) $(E_LINKARGS2)"; \
		>> $(PATHDEF_SRC)
	@ echo char_u *compiled_user = (char_u *)"$(USERNAME)"; \
		>> $(PATHDEF_SRC)
	@ echo char_u *compiled_sys = (char_u *)"$(USERDOMAIN)"; \
		>> $(PATHDEF_SRC)

# End Custom Build
proto.h: \
	proto/alloc.pro \
	proto/arabic.pro \
	proto/arglist.pro \
	proto/autocmd.pro \
	proto/blob.pro \
	proto/blowfish.pro \
	proto/buffer.pro \
	proto/bufwrite.pro \
	proto/change.pro \
	proto/charset.pro \
	proto/cindent.pro \
	proto/clientserver.pro \
	proto/clipboard.pro \
	proto/cmdexpand.pro \
	proto/cmdhist.pro \
	proto/crypt.pro \
	proto/crypt_zip.pro \
	proto/debugger.pro \
	proto/dict.pro \
	proto/diff.pro \
	proto/digraph.pro \
	proto/drawline.pro \
	proto/drawscreen.pro \
	proto/edit.pro \
	proto/eval.pro \
	proto/evalbuffer.pro \
	proto/evalfunc.pro \
	proto/evalvars.pro \
	proto/evalwindow.pro \
	proto/ex_cmds.pro \
	proto/ex_cmds2.pro \
	proto/ex_docmd.pro \
	proto/ex_eval.pro \
	proto/ex_getln.pro \
	proto/fileio.pro \
	proto/filepath.pro \
	proto/findfile.pro \
	proto/float.pro \
	proto/fuzzy.pro \
	proto/getchar.pro \
	proto/gc.pro \
	proto/gui_xim.pro \
	proto/hardcopy.pro \
	proto/hashtab.pro \
	proto/help.pro \
	proto/highlight.pro \
	proto/indent.pro \
	proto/insexpand.pro \
	proto/json.pro \
	proto/linematch.pro \
	proto/list.pro \
	proto/locale.pro \
	proto/logfile.pro \
	proto/main.pro \
	proto/map.pro \
	proto/mark.pro \
	proto/match.pro \
	proto/memfile.pro \
	proto/memline.pro \
	proto/menu.pro \
	proto/message.pro \
	proto/misc1.pro \
	proto/misc2.pro \
	proto/mouse.pro \
	proto/move.pro \
	proto/mbyte.pro \
	proto/normal.pro \
	proto/ops.pro \
	proto/option.pro \
	proto/optionstr.pro \
	proto/os_mswin.pro \
	proto/winclip.pro \
	proto/os_win32.pro \
	proto/popupmenu.pro \
	proto/popupwin.pro \
	proto/profiler.pro \
	proto/quickfix.pro \
	proto/regexp.pro \
	proto/register.pro \
	proto/scriptfile.pro \
	proto/screen.pro \
	proto/search.pro \
	proto/session.pro \
	proto/sha256.pro \
	proto/sign.pro \
	proto/spell.pro \
	proto/spellfile.pro \
	proto/spellsuggest.pro \
	proto/strings.pro \
	proto/syntax.pro \
	proto/tabpanel.pro \
	proto/tag.pro \
	proto/term.pro \
	proto/testing.pro \
	proto/textformat.pro \
	proto/textobject.pro \
	proto/textprop.pro \
	proto/time.pro \
	proto/tuple.pro \
	proto/typval.pro \
	proto/ui.pro \
	proto/undo.pro \
	proto/usercmd.pro \
	proto/userfunc.pro \
	proto/vim9class.pro \
	proto/vim9cmds.pro \
	proto/vim9compile.pro \
	proto/vim9execute.pro \
	proto/vim9expr.pro \
	proto/vim9generics.pro \
	proto/vim9instr.pro \
	proto/vim9script.pro \
	proto/vim9type.pro \
	proto/viminfo.pro \
	proto/window.pro \
	$(SOUND_PRO) \
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

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=79 ft=make:
