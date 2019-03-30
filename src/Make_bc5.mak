#
# Makefile for Vim.
# Compiler: Borland C++ 5.0 and later 32-bit compiler
#  Targets: Win32 (Windows NT and Windows 95) (with/without GUI)
#
# NOTE: THIS IS OLD AND PROBABLY NO LONGER WORKS.
#
# Contributed by Ben Singer.
# Updated 4/1997 by Ron Aaron
#	2016: removed support for 16 bit DOS
#	6/1997 - added support for 16 bit DOS
#	Note: this has been tested, and works, for BC5.  Your mileage may vary.
#	Has been reported NOT to work with BC 4.52.  Maybe it can be fixed?
#	10/1997 - ron - fixed bugs w/ BC 5.02
#	8/1998 - ron - updated with new targets, fixed some stuff
#	3/2000 - Bram: Made it work with BC 5.5 free command line compiler,
#			cleaned up variables.
#	6/2001 - Dan - Added support for compiling Python and TCL
#	7/2001 - Dan - Added support for compiling Ruby
#
# It builds on Windows 95 and NT-Intel, producing the same binary in either
# case.  To build using Microsoft Visual C++, use Make_mvc.mak.
#
# This should work with the free Borland command line compiler, version 5.5.
# You need at least sp1 (service pack 1).  With sp2 it compiles faster.
# Use a command like this:
# <path>\bin\make /f Make_bc5.mak BOR=<path>
#

# let the make utility do the hard work:
.AUTODEPEND
.CACHEAUTODEPEND

# VARIABLES:
# name		value (default)
#
# BOR		path to root of Borland C install (c:\bc5)
# LINK		name of the linker ($(BOR)\bin\ilink32)
# GUI		no or yes: set to yes if you want the GUI version (yes)
# LUA     define to path to Lua dir to get Lua support (not defined)
#   LUA_VER	  define to version of Lua being used (51)
#   DYNAMIC_LUA  no or yes: set to yes to load the Lua DLL dynamically (no)
# PERL		define to path to Perl dir to get Perl support (not defined)
#   PERL_VER	  define to version of Perl being used (56)
#   DYNAMIC_PERL  no or yes: set to yes to load the Perl DLL dynamically (no)
# PYTHON	define to path to Python dir to get PYTHON support (not defined)
#   PYTHON_VER	    define to version of Python being used (22)
#   DYNAMIC_PYTHON  no or yes: use yes to load the Python DLL dynamically (no)
# PYTHON3	define to path to Python3 dir to get PYTHON3 support (not defined)
#   PYTHON3_VER	    define to version of Python3 being used (31)
#   DYNAMIC_PYTHON3  no or yes: use yes to load the Python3 DLL dynamically (no)
# TCL		define to path to TCL dir to get TCL support (not defined)
#   TCL_VER	define to version of TCL being used (83)
#   DYNAMIC_TCL no or yes: use yes to load the TCL DLL dynamically (no)
# RUBY		define to path to Ruby dir to get Ruby support (not defined)
#		NOTE: You may have to remove the defines for uid_t and gid_t
#		from the Ruby config.h header file.
#   RUBY_VER	define to version of Ruby being used (16)
#		NOTE: compilation on WinNT/2K/XP requires
#		at least version 1.6.5 of Ruby.  Earlier versions
#		of Ruby will cause a compile error on these systems.
#   RUBY_VER_LONG  same, but in format with dot. (1.6)
#   DYNAMIC_RUBY no or yes: use yes to load the Ruby DLL dynamically (no)
# IME		no or yes: set to yes for multi-byte IME support (yes)
#   DYNAMIC_IME no or yes: set to yes to load imm32.dll dynamically (yes)
# GETTEXT	no or yes: set to yes for multi-language support (yes)
# ICONV		no or yes: set to yes for dynamic iconv support (yes)
# OLE		no or yes: set to yes to make OLE gvim (no)
# DEBUG		no or yes: set to yes if you wish a DEBUGging build (no)
# CODEGUARD	no or yes: set to yes if you want to use CODEGUARD (no)
# CPUNR		1 through 6: select -CPU argument to compile with (3)
#		3 for 386, 4 for 486, 5 for pentium, 6 for pentium pro.
# USEDLL	no or yes: set to yes to use the Runtime library DLL (no)
#		For USEDLL=yes the cc3250.dll is required to run Vim.
# ALIGN		1, 2 or 4: Alignment to use (4 for Win32)
# FASTCALL	no or yes: set to yes to use register-based function protocol (yes)
# OPTIMIZE	SPACE, SPEED, or MAXSPEED: type of optimization (MAXSPEED)
# POSTSCRIPT	no or yes: set to yes for PostScript printing
# FEATURES	TINY, SMALL, NORMAL, BIG or HUGE (BIG for WIN32)
# WINVER	0x0400 or 0x0500: minimum Win32 version to support (0x0400)
# CSCOPE	no or yes: include support for Cscope interface (yes)
# NETBEANS	no or yes: include support for Netbeans interface; also
#		requires CHANNEL (yes if GUI
#		is yes)
# NBDEBUG	no or yes: include support for debugging Netbeans interface (no)
# CHANNEL	no or yes: include support for inter process communication (yes
#		if GUI is yes)
# XPM		define to path to XPM dir to get support for loading XPM images.

### BOR: root of the BC installation
!if ("$(BOR)"=="")
BOR = c:\bc5
!endif

### LINK: Name of the linker: ilink32 (this is below)

### GUI: yes for GUI version, no for console version
!if ("$(GUI)"=="")
GUI = yes
!endif

### IME: yes for multibyte support, no to disable it.
!if ("$(IME)"=="")
IME = yes
!endif
!if ("$(DYNAMIC_IME)"=="")
DYNAMIC_IME = yes
!endif

### GETTEXT: yes for multilanguage support, no to disable it.
!if ("$(GETTEXT)"=="")
GETTEXT = yes
!endif

### ICONV: yes to enable dynamic-iconv support, no to disable it
!if ("$(ICONV)"=="")
ICONV = yes
!endif

### CSCOPE: yes to enable Cscope support, no to disable it
!if ("$(CSCOPE)"=="")
CSCOPE = yes
!endif

### NETBEANS: yes to enable NetBeans interface support, no to disable it
!if ("$(NETBEANS)"=="") && ("$(GUI)"=="yes")
NETBEANS = yes
!endif

### CHANNEL: yes to enable inter process communication, no to disable it
!if ("$(CHANNEL)"=="") && ("$(GUI)"=="yes")
CHANNEL = yes
!endif

### LUA: uncomment this line if you want lua support in vim
# LUA=c:\lua

### PERL: uncomment this line if you want perl support in vim
# PERL=c:\perl

### PYTHON: uncomment this line if you want python support in vim
# PYTHON=c:\python22

### PYTHON3: uncomment this line if you want python3 support in vim
# PYTHON3=c:\python31

### RUBY: uncomment this line if you want ruby support in vim
# RUBY=c:\ruby

### TCL: uncomment this line if you want tcl support in vim
# TCL=c:\tcl

### OLE: no for normal gvim, yes for OLE-capable gvim (only works with GUI)
#OLE = yes

### DEBUG: Uncomment to make an executable for debugging
# DEBUG = yes
!if ("$(DEBUG)"=="yes")
DEBUG_FLAG = -v
!endif

### CODEGUARD: Uncomment to use the CODEGUARD stuff (BC 5.0 or later):
# CODEGUARD = yes
!if ("$(CODEGUARD)"=="yes")
CODEGUARD_FLAG = -vG
!endif

### CPUNR: set your target processor (3 to 6)
!if ("$(CPUNR)" == "i386") || ("$(CPUNR)" == "3")
CPUNR = 3
!elif ("$(CPUNR)" == "i486") || ("$(CPUNR)" == "4")
CPUNR = 4
!elif ("$(CPUNR)" == "i586") || ("$(CPUNR)" == "5")
CPUNR = 5
!elif ("$(CPUNR)" == "i686") || ("$(CPUNR)" == "6")
CPUNR = 6
!else
CPUNR = 3
!endif

### Comment out to use precompiled headers (faster, but uses lots of disk!)
HEADERS = -H -H=vim.csm -Hc

### USEDLL: no for statically linked version of run-time, yes for DLL runtime
!if ("$(USEDLL)"=="")
USEDLL = no
!endif

### ALIGN: alignment you desire: (1,2 or 4: s/b 4 for Win32)
!if ("$(ALIGN)"=="")
ALIGN = 4
!endif

### FASTCALL: yes to use FASTCALL calling convention (RECOMMENDED!), no otherwise
#   Incompatible when calling external functions (like MSVC-compiled DLLs), so
#   don't use FASTCALL when linking with external libs.
!if ("$(FASTCALL)"=="") && \
	("$(LUA)"=="") && \
	("$(PYTHON)"=="") && \
	("$(PYTHON3)"=="") && \
	("$(PERL)"=="") && \
	("$(TCL)"=="") && \
	("$(RUBY)"=="") && \
	("$(ICONV)"!="yes") && \
	("$(IME)"!="yes") && \
	("$(XPM)"=="")
FASTCALL = yes
!endif

### OPTIMIZE: SPEED to optimize for speed, SPACE otherwise (SPEED RECOMMENDED)
!if ("$(OPTIMIZE)"=="")
OPTIMIZE = MAXSPEED
!endif

### FEATURES: TINY, SMALL, NORMAL, BIG or HUGE (BIG for WIN32)
!if ("$(FEATURES)"=="")
FEATURES = BIG
!endif

### POSTSCRIPT: uncomment this line if you want PostScript printing
#POSTSCRIPT = yes

###
# If you have a fixed directory for $VIM or $VIMRUNTIME, other than the normal
# default, use these lines.
#VIMRCLOC = somewhere
#VIMRUNTIMEDIR = somewhere

### Set the default $(WINVER) to make it work with Bcc 5.5.
!ifndef WINVER
WINVER = 0x0400
!endif

#
# Sanity checks for the above options:
#

OSTYPE = WIN32

#
# Optimizations: change as desired (RECOMMENDATION: Don't change!):
#
!if ("$(DEBUG)"=="yes")
OPT = -Od -N
!else
!if ("$(OPTIMIZE)"=="SPACE")
OPT = -O1 -f- -d
!elif ("$(OPTIMIZE)"=="MAXSPEED")
OPT = -O2 -f- -d -Ocavi -O
!else
OPT = -O2 -f- -d -Oc -O
!endif
!if ("$(FASTCALL)"=="yes")
OPT = $(OPT) -pr
!endif
!if ("$(CODEGUARD)"!="yes")
OPT = $(OPT) -vi-
!endif
!endif
# shouldn't have to change:
LIB = $(BOR)\lib
INCLUDE = $(BOR)\include;.;proto
DEFINES = -DFEAT_$(FEATURES) -DWIN32 -DHAVE_PATHDEF \
	  -DWINVER=$(WINVER) -D_WIN32_WINNT=$(WINVER)

!ifdef LUA
INTERP_DEFINES = $(INTERP_DEFINES) -DFEAT_LUA
INCLUDE = $(LUA)\include;$(INCLUDE)
!  ifndef LUA_VER
LUA_VER = 51
!  endif
!  if ("$(DYNAMIC_LUA)" == "yes")
INTERP_DEFINES = $(INTERP_DEFINES) -DDYNAMIC_LUA -DDYNAMIC_LUA_DLL=\"lua$(LUA_VER).dll\"
LUA_LIB_FLAG = /nodefaultlib:
!  endif
!endif

!ifdef PERL
INTERP_DEFINES = $(INTERP_DEFINES) -DFEAT_PERL
INCLUDE = $(PERL)\lib\core;$(INCLUDE)
!  ifndef PERL_VER
PERL_VER = 56
!  endif
!  if ("$(DYNAMIC_PERL)" == "yes")
!    if ($(PERL_VER) > 55)
INTERP_DEFINES = $(INTERP_DEFINES) -DDYNAMIC_PERL -DDYNAMIC_PERL_DLL=\"perl$(PERL_VER).dll\"
PERL_LIB_FLAG = /nodefaultlib:
!    else
!      message "Cannot dynamically load Perl versions less than 5.6.  Loading statically..."
!    endif
!  endif
!endif

!ifdef PYTHON
!ifdef PYTHON3
DYNAMIC_PYTHON=yes
DYNAMIC_PYTHON3=yes
!endif
!endif

!ifdef PYTHON
INTERP_DEFINES = $(INTERP_DEFINES) -DFEAT_PYTHON
!ifndef PYTHON_VER
PYTHON_VER = 22
!endif
!if "$(DYNAMIC_PYTHON)" == "yes"
INTERP_DEFINES = $(INTERP_DEFINES) -DDYNAMIC_PYTHON -DDYNAMIC_PYTHON_DLL=\"python$(PYTHON_VER).dll\"
PYTHON_LIB_FLAG = /nodefaultlib:
!endif
!endif

!ifdef PYTHON3
INTERP_DEFINES = $(INTERP_DEFINES) -DFEAT_PYTHON3
!ifndef PYTHON3_VER
PYTHON3_VER = 31
!endif
!if "$(DYNAMIC_PYTHON3)" == "yes"
INTERP_DEFINES = $(INTERP_DEFINES) -DDYNAMIC_PYTHON3 -DDYNAMIC_PYTHON3_DLL=\"python$(PYTHON3_VER).dll\"
PYTHON3_LIB_FLAG = /nodefaultlib:
!endif
!endif


!ifdef RUBY
!ifndef RUBY_VER
RUBY_VER = 16
!endif
!ifndef RUBY_VER_LONG
RUBY_VER_LONG = 1.6
!endif

!if "$(RUBY_VER)" == "16"
!ifndef RUBY_PLATFORM
RUBY_PLATFORM = i586-mswin32
!endif
!ifndef RUBY_INSTALL_NAME
RUBY_INSTALL_NAME = mswin32-ruby$(RUBY_VER)
!endif
!else
!ifndef RUBY_PLATFORM
RUBY_PLATFORM = i386-mswin32
!endif
!ifndef RUBY_INSTALL_NAME
RUBY_INSTALL_NAME = msvcrt-ruby$(RUBY_VER)
!endif
!endif

INTERP_DEFINES = $(INTERP_DEFINES) -DFEAT_RUBY
INCLUDE = $(RUBY)\lib\ruby\$(RUBY_VER_LONG)\$(RUBY_PLATFORM);$(INCLUDE)

!if "$(DYNAMIC_RUBY)" == "yes"
INTERP_DEFINES = $(INTERP_DEFINES) -DDYNAMIC_RUBY -DDYNAMIC_RUBY_DLL=\"$(RUBY_INSTALL_NAME).dll\"
INTERP_DEFINES = $(INTERP_DEFINES) -DDYNAMIC_RUBY_VER=$(RUBY_VER)
RUBY_LIB_FLAG = /nodefaultlib:
!endif
!endif

!ifdef TCL
INTERP_DEFINES = $(INTERP_DEFINES) -DFEAT_TCL
INCLUDE = $(TCL)\include;$(INCLUDE)
!ifndef TCL_VER
TCL_VER = 83
!endif
TCL_LIB = $(TCL)\lib\tcl$(TCL_VER).lib
TCL_LIB_FLAG =
!if "$(DYNAMIC_TCL)" == "yes"
INTERP_DEFINES = $(INTERP_DEFINES) -DDYNAMIC_TCL -DDYNAMIC_TCL_DLL=\"tcl$(TCL_VER).dll\"
TCL_LIB = tclstub$(TCL_VER)-bor.lib
TCL_LIB_FLAG =
!endif
!endif
#
# DO NOT change below:
#
CPUARG = -$(CPUNR)
ALIGNARG = -a$(ALIGN)
#
!if ("$(DEBUG)"=="yes")
DEFINES=$(DEFINES) -DDEBUG -D_DEBUG
!endif
#
!if ("$(OLE)"=="yes")
DEFINES = $(DEFINES) -DFEAT_OLE
!endif
#
!if ("$(IME)"=="yes")
MBDEFINES = $(MBDEFINES) -DFEAT_MBYTE_IME
!if ("$(DYNAMIC_IME)" == "yes")
MBDEFINES = $(MBDEFINES) -DDYNAMIC_IME
!endif
!endif
!if ("$(ICONV)"=="yes")
MBDEFINES = $(MBDEFINES) -DDYNAMIC_ICONV
!endif
!if ("$(GETTEXT)"=="yes")
MBDEFINES = $(MBDEFINES) -DDYNAMIC_GETTEXT
!endif

!if ("$(CSCOPE)"=="yes")
DEFINES = $(DEFINES) -DFEAT_CSCOPE
!endif

!if ("$(GUI)"=="yes")
DEFINES = $(DEFINES) -DFEAT_GUI_MSWIN -DFEAT_CLIPBOARD
!if ("$(DEBUG)"=="yes")
TARGET = gvimd.exe
!else
TARGET = gvim.exe
!endif
EXETYPE=-W
STARTUPOBJ = c0w32.obj
LINK2 = -aa
RESFILE = vim.res
!else
!undef NETBEANS
!undef CHANNEL
!undef XPM
!if ("$(DEBUG)"=="yes")
TARGET = vimd.exe
!else
# for now, anyway: VIMDLL is only for the GUI version
TARGET = vim.exe
!endif
EXETYPE=-WC
STARTUPOBJ = c0x32.obj
LINK2 = -ap -OS -o -P
RESFILE = vim.res
!endif

!if ("$(NETBEANS)"=="yes")
!if ("$(CHANNEL)"!="yes")
# cannot use Netbeans without CHANNEL
NETBEANS = no
!else
DEFINES = $(DEFINES) -DFEAT_NETBEANS_INTG
!if ("$(NBDEBUG)"=="yes")
DEFINES = $(DEFINES) -DNBDEBUG
NBDEBUG_DEP = nbdebug.h nbdebug.c
!endif
!endif
!endif

!if ("$(CHANNEL)"=="yes")
DEFINES = $(DEFINES) -DFEAT_JOB_CHANNEL
!endif

!ifdef XPM
!if ("$(GUI)"=="yes")
DEFINES = $(DEFINES) -DFEAT_XPM_W32
INCLUDE = $(XPM)\include;$(INCLUDE)
!endif
!endif

!if ("$(USEDLL)"=="yes")
DEFINES = $(DEFINES) -D_RTLDLL
!endif

!if ("$(DEBUG)"=="yes")
OBJDIR	= $(OSTYPE)\objdbg
!else
!if ("$(GUI)"=="yes")
!if ("$(OLE)"=="yes")
OBJDIR	= $(OSTYPE)\oleobj
!else
OBJDIR	= $(OSTYPE)\gobj
!endif
!else
OBJDIR	= $(OSTYPE)\obj
!endif
!endif

!if ("$(POSTSCRIPT)"=="yes")
DEFINES = $(DEFINES) -DMSWINPS
!endif

##### BASE COMPILER/TOOLS RULES #####
MAKE = $(BOR)\bin\make
CFLAGS = -w-aus -w-par -w-pch -w-ngu -w-csu -I$(INCLUDE)
BRC = $(BOR)\BIN\brc32
!if ("$(LINK)"=="")
LINK	= $(BOR)\BIN\ILink32
!endif
CC   = $(BOR)\BIN\Bcc32
LFLAGS	= -OS -Tpe -c -m -L$(LIB) $(DEBUG_FLAG) $(LINK2)
LFLAGSDLL  = -Tpd -c -m -L$(LIB) $(DEBUG_FLAG) $(LINK2)
CFLAGS = $(CFLAGS) -d -RT- -k- -Oi $(HEADERS) -f-

CC1 = -c
CC2 = -o
CCARG = +$(OBJDIR)\bcc.cfg

# implicit rules:

# Without the following, the implicit rule in BUILTINS.MAK is picked up
# for a rule for .c.obj rather than the local implicit rule
.SUFFIXES
.SUFFIXES .c .obj
.path.c = .

{.}.c{$(OBJDIR)}.obj:
	$(CC) $(CCARG) $(CC1) -n$(OBJDIR)\ {$< }

.cpp.obj:
	$(CC) $(CCARG) $(CC1) $(CC2)$@ $*.cpp

vimmain = \
	$(OBJDIR)\os_w32exe.obj
vimwinmain = \
	$(OBJDIR)\os_w32exe.obj

vimobj =  \
	$(OBJDIR)\arabic.obj \
	$(OBJDIR)\autocmd.obj \
	$(OBJDIR)\blowfish.obj \
	$(OBJDIR)\buffer.obj \
	$(OBJDIR)\charset.obj \
	$(OBJDIR)\crypt.obj \
	$(OBJDIR)\crypt_zip.obj \
	$(OBJDIR)\dict.obj \
	$(OBJDIR)\diff.obj \
	$(OBJDIR)\digraph.obj \
	$(OBJDIR)\edit.obj \
	$(OBJDIR)\eval.obj \
	$(OBJDIR)\evalfunc.obj \
	$(OBJDIR)\ex_cmds.obj \
	$(OBJDIR)\ex_cmds2.obj \
	$(OBJDIR)\ex_docmd.obj \
	$(OBJDIR)\ex_eval.obj \
	$(OBJDIR)\ex_getln.obj \
	$(OBJDIR)\fileio.obj \
	$(OBJDIR)\findfile.obj \
	$(OBJDIR)\fold.obj \
	$(OBJDIR)\getchar.obj \
	$(OBJDIR)\hardcopy.obj \
	$(OBJDIR)\hashtab.obj \
	$(OBJDIR)\indent.obj \
	$(OBJDIR)\insexpand.obj \
	$(OBJDIR)\json.obj \
	$(OBJDIR)\list.obj \
	$(OBJDIR)\main.obj \
	$(OBJDIR)\mark.obj \
	$(OBJDIR)\memfile.obj \
	$(OBJDIR)\memline.obj \
	$(OBJDIR)\menu.obj \
	$(OBJDIR)\message.obj \
	$(OBJDIR)\misc1.obj \
	$(OBJDIR)\misc2.obj \
	$(OBJDIR)\move.obj \
	$(OBJDIR)\mbyte.obj \
	$(OBJDIR)\normal.obj \
	$(OBJDIR)\ops.obj \
	$(OBJDIR)\option.obj \
	$(OBJDIR)\popupmnu.obj \
	$(OBJDIR)\quickfix.obj \
	$(OBJDIR)\regexp.obj \
	$(OBJDIR)\screen.obj \
	$(OBJDIR)\search.obj \
	$(OBJDIR)\sha256.obj \
	$(OBJDIR)\sign.obj \
	$(OBJDIR)\spell.obj \
	$(OBJDIR)\spellfile.obj \
	$(OBJDIR)\syntax.obj \
	$(OBJDIR)\tag.obj \
	$(OBJDIR)\term.obj \
	$(OBJDIR)\ui.obj \
	$(OBJDIR)\undo.obj \
	$(OBJDIR)\userfunc.obj \
	$(OBJDIR)\version.obj \
	$(OBJDIR)\window.obj \
	$(OBJDIR)\pathdef.obj

!if ("$(OLE)"=="yes")
vimobj = $(vimobj) \
	$(OBJDIR)\if_ole.obj
!endif

!ifdef LUA
vimobj = $(vimobj) \
    $(OBJDIR)\if_lua.obj
!endif

!ifdef PERL
vimobj = $(vimobj) \
    $(OBJDIR)\if_perl.obj
!endif

!ifdef PYTHON
vimobj = $(vimobj) \
    $(OBJDIR)\if_python.obj
!endif

!ifdef PYTHON3
vimobj = $(vimobj) \
    $(OBJDIR)\if_python3.obj
!endif

!ifdef RUBY
vimobj = $(vimobj) \
    $(OBJDIR)\if_ruby.obj
!endif

!ifdef TCL
vimobj = $(vimobj) \
    $(OBJDIR)\if_tcl.obj
!endif

!if ("$(CSCOPE)"=="yes")
vimobj = $(vimobj) \
    $(OBJDIR)\if_cscope.obj
!endif

!if ("$(NETBEANS)"=="yes")
vimobj = $(vimobj) \
    $(OBJDIR)\netbeans.obj
!endif

!if ("$(CHANNEL)"=="yes")
vimobj = $(vimobj) \
    $(OBJDIR)\channel.obj
!endif

!ifdef XPM
vimobj = $(vimobj) \
    $(OBJDIR)\xpm_w32.obj
!endif

!if ("$(GUI)"=="yes")
vimobj = $(vimobj) \
	$(vimwinmain) \
	$(OBJDIR)\gui.obj \
	$(OBJDIR)\gui_beval.obj \
	$(OBJDIR)\gui_w32.obj
!endif

vimobj = $(vimobj) \
	$(OBJDIR)\os_win32.obj $(OBJDIR)\os_mswin.obj $(OBJDIR)\winclip.obj
# Blab what we are going to do:
MSG = Compiling $(OSTYPE) $(TARGET) $(OLETARGET), with:
!if ("$(GUI)"=="yes")
MSG = $(MSG) GUI
!endif
!if ("$(OLE)"=="yes")
MSG = $(MSG) OLE
!endif
!if ("$(USEDLL)"=="yes")
MSG = $(MSG) USEDLL
!endif
!if ("$(FASTCALL)"=="yes")
MSG = $(MSG) FASTCALL
!endif
!if ("$(IME)"=="yes")
MSG = $(MSG) IME
! if "$(DYNAMIC_IME)" == "yes"
MSG = $(MSG)(dynamic)
! endif
!endif
!if ("$(GETTEXT)"=="yes")
MSG = $(MSG) GETTEXT
!endif
!if ("$(ICONV)"=="yes")
MSG = $(MSG) ICONV
!endif
!if ("$(DEBUG)"=="yes")
MSG = $(MSG) DEBUG
!endif
!if ("$(CODEGUARD)"=="yes")
MSG = $(MSG) CODEGUARD
!endif
!if ("$(CSCOPE)"=="yes")
MSG = $(MSG) CSCOPE
!endif
!if ("$(NETBEANS)"=="yes")
MSG = $(MSG) NETBEANS
!endif
!if ("$(CHANNEL)"=="yes")
MSG = $(MSG) CHANNEL
!endif
!ifdef XPM
MSG = $(MSG) XPM
!endif
!ifdef LUA
MSG = $(MSG) LUA
! if "$(DYNAMIC_LUA)" == "yes"
MSG = $(MSG)(dynamic)
! endif
!endif
!ifdef PERL
MSG = $(MSG) PERL
! if "$(DYNAMIC_PERL)" == "yes"
MSG = $(MSG)(dynamic)
! endif
!endif
!ifdef PYTHON
MSG = $(MSG) PYTHON
! if "$(DYNAMIC_PYTHON)" == "yes"
MSG = $(MSG)(dynamic)
! endif
!endif
!ifdef PYTHON3
MSG = $(MSG) PYTHON3
! if "$(DYNAMIC_PYTHON3)" == "yes"
MSG = $(MSG)(dynamic)
! endif
!endif
!ifdef RUBY
MSG = $(MSG) RUBY
! if "$(DYNAMIC_RUBY)" == "yes"
MSG = $(MSG)(dynamic)
! endif
!endif
!ifdef TCL
MSG = $(MSG) TCL
! if "$(DYNAMIC_TCL)" == "yes"
MSG = $(MSG)(dynamic)
! endif
!endif
MSG = $(MSG) cpu=$(CPUARG)
MSG = $(MSG) Align=$(ALIGNARG)

!message $(MSG)

TARGETS = $(TARGETS) $(TARGET)

# Targets:
all: vim vimrun.exe install.exe xxd uninstal.exe GvimExt/gvimext.dll

vim: $(OSTYPE) $(OBJDIR) $(OBJDIR)\bcc.cfg $(TARGETS)
	@if exist $(OBJDIR)\version.obj del $(OBJDIR)\version.obj
	@if exist auto\pathdef.c del auto\pathdef.c

$(OSTYPE):
	-@md $(OSTYPE)

$(OBJDIR):
	-@md $(OBJDIR)

xxd:
	@cd xxd
	$(MAKE) /f Make_bc5.mak BOR="$(BOR)" BCC="$(CC)"
	@cd ..

GvimExt/gvimext.dll: GvimExt/gvimext.cpp GvimExt/gvimext.rc GvimExt/gvimext.h
	cd GvimExt
	$(MAKE) /f Make_bc5.mak USEDLL=$(USEDLL) BOR=$(BOR)
	cd ..

install.exe: dosinst.c $(OBJDIR)\bcc.cfg
	$(CC) $(CCARG) -WC -DWIN32 -einstall dosinst.c

uninstal.exe: uninstal.c $(OBJDIR)\bcc.cfg
	$(CC) $(CCARG) -WC -DWIN32 -O2 -euninstal uninstal.c

clean:
!if "$(OS)" == "Windows_NT"
	# For Windows NT/2000, doesn't work on Windows 95/98...
	# $(COMSPEC) needed to ensure rmdir.exe is not run
	-@$(COMSPEC) /C rmdir /Q /S $(OBJDIR)
!else
	# For Windows 95/98, doesn't work on Windows NT/2000...
	-@deltree /y $(OBJDIR)
!endif
	-@del *.res
	-@del vim32*.dll
	-@del vim32*.lib
	-@del *vim*.exe
	-@del *install*.exe
	-@del *.csm
	-@del *.map
	-@del *.ilc
	-@del *.ild
	-@del *.ilf
	-@del *.ils
	-@del *.tds
!ifdef LUA
	-@del lua.lib
!endif
!ifdef PERL
	-@del perl.lib
	-@del if_perl.c
	-@del auto\if_perl.c
!endif
!ifdef PYTHON
	-@del python.lib
!endif
!ifdef PYTHON3
	-@del python3.lib
!endif
!ifdef RUBY
	-@del ruby.lib
!endif
!ifdef TCL
	-@del tcl.lib
!endif
!ifdef XPM
	-@del xpm.lib
!endif
	cd xxd
	$(MAKE) /f Make_bc5.mak BOR="$(BOR)" clean
	cd ..
	cd GvimExt
	$(MAKE) /f Make_bc5.mak BOR="$(BOR)" clean
	cd ..


$(TARGET): $(OBJDIR) $(vimobj) $(OBJDIR)\$(RESFILE)
  $(LINK) @&&|
	$(LFLAGS) +
	$(STARTUPOBJ) +
	$(vimobj)
	$<,$*
!if ("$(CODEGUARD)"=="yes")
	cg32.lib+
!endif
# $(OSTYPE)==WIN32 causes os_mswin.c compilation. FEAT_SHORTCUT in it needs OLE
	ole2w32.lib +
	import32.lib+
!ifdef LUA
	$(LUA_LIB_FLAG)lua.lib+
!endif
!ifdef PERL
	$(PERL_LIB_FLAG)perl.lib+
!endif
!ifdef PYTHON
	$(PYTHON_LIB_FLAG)python.lib+
!endif
!ifdef PYTHON3
	$(PYTHON3_LIB_FLAG)python3.lib+
!endif
!ifdef RUBY
	$(RUBY_LIB_FLAG)ruby.lib+
!endif
!ifdef TCL
	$(TCL_LIB_FLAG)tcl.lib+
!endif
!ifdef XPM
	xpm.lib+
!endif
!if ("$(USEDLL)"=="yes")
	cw32i.lib
!else
	cw32.lib
!endif

	$(OBJDIR)\$(RESFILE)
|

test:
	cd testdir
	$(MAKE) /NOLOGO -f Make_dos.mak win32
	cd ..

$(OBJDIR)\ex_docmd.obj:  ex_docmd.c ex_cmds.h

$(OBJDIR)\ex_eval.obj:  ex_eval.c ex_cmds.h

$(OBJDIR)\if_ole.obj: if_ole.cpp

$(OBJDIR)\if_lua.obj: if_lua.c lua.lib
	$(CC) $(CCARG) $(CC1) $(CC2)$@ -pc if_lua.c

$(OBJDIR)\if_perl.obj: auto/if_perl.c perl.lib
	$(CC) $(CCARG) $(CC1) $(CC2)$@ -pc auto/if_perl.c

auto/if_perl.c: if_perl.xs typemap
	$(PERL)\bin\perl.exe $(PERL)\lib\ExtUtils\xsubpp -prototypes -typemap \
	    $(PERL)\lib\ExtUtils\typemap if_perl.xs -output $@

$(OBJDIR)\if_python.obj: if_python.c if_py_both.h python.lib
	$(CC) -I$(PYTHON)\include $(CCARG) $(CC1) $(CC2)$@ -pc if_python.c

$(OBJDIR)\if_python3.obj: if_python3.c if_py_both.h python3.lib
	$(CC) -I$(PYTHON3)\include $(CCARG) $(CC1) $(CC2)$@ -pc if_python3.c

$(OBJDIR)\if_ruby.obj: if_ruby.c ruby.lib
	$(CC) $(CCARG) $(CC1) $(CC2)$@ -pc if_ruby.c

$(OBJDIR)\if_tcl.obj: if_tcl.c tcl.lib
	$(CC) $(CCARG) $(CC1) $(CC2)$@ -pc if_tcl.c

$(OBJDIR)\xpm_w32.obj: xpm_w32.c xpm.lib
	$(CC) $(CCARG) $(CC1) $(CC2)$@ -pc xpm_w32.c

$(OBJDIR)\netbeans.obj: netbeans.c $(NBDEBUG_DEP)
	$(CC) $(CCARG) $(CC1) $(CC2)$@ netbeans.c

$(OBJDIR)\channel.obj: channel.c
	$(CC) $(CCARG) $(CC1) $(CC2)$@ channel.c

$(OBJDIR)\vim.res: vim.rc version.h tools.bmp tearoff.bmp \
		vim.ico vim_error.ico vim_alert.ico vim_info.ico vim_quest.ico
	$(BRC) -fo$(OBJDIR)\vim.res -i $(BOR)\include -w32 -r vim.rc @&&|
	$(DEFINES)
|

$(OBJDIR)\pathdef.obj:	auto\pathdef.c
	$(CC) $(CCARG) $(CC1) $(CC2)$@ auto\pathdef.c


# Need to escape both quotes and backslashes in $INTERP_DEFINES
INTERP_DEFINES_ESC_BKS=$(INTERP_DEFINES:\=\\)
INTERP_DEFINES_ESC=$(INTERP_DEFINES_ESC_BKS:"=\")

# Note:  the silly /*"*/ below are there to trick make into accepting
# the # character as something other than a comment without messing up
# the preprocessor directive.
auto\pathdef.c::
	-@md auto
	@echo creating auto/pathdef.c
	@copy /y &&|
/* pathdef.c */
/*"*/#include "vim.h"/*"*/

char_u *default_vim_dir = (char_u *)"$(VIMRCLOC:\=\\)";
char_u *default_vimruntime_dir = (char_u *)"$(VIMRUNTIMEDIR:\=\\)";
char_u *all_cflags = (char_u *)"$(CC:\=\\) $(CFLAGS:\=\\) $(DEFINES) $(MBDEFINES) $(INTERP_DEFINES_ESC) $(OPT) $(EXETYPE) $(CPUARG) $(ALIGNARG) $(DEBUG_FLAG) $(CODEGUARD_FLAG)";
char_u *all_lflags = (char_u *)"$(LINK:\=\\) $(LFLAGS:\=\\)";
char_u *compiled_user = (char_u *)"$(USERNAME)";
char_u *compiled_sys = (char_u *)"$(USERDOMAIN)";
| auto\pathdef.c

lua.lib: $(LUA)\lib\lua$(LUA_VER).lib
	coff2omf $(LUA)\lib\lua$(LUA_VER).lib $@

perl.lib: $(PERL)\lib\CORE\perl$(PERL_VER).lib
	coff2omf $(PERL)\lib\CORE\perl$(PERL_VER).lib $@

python.lib: $(PYTHON)\libs\python$(PYTHON_VER).lib
	coff2omf $(PYTHON)\libs\python$(PYTHON_VER).lib $@

python3.lib: $(PYTHON3)\libs\python$(PYTHON3_VER).lib
	coff2omf $(PYTHON3)\libs\python$(PYTHON3_VER).lib $@

ruby.lib: $(RUBY)\lib\$(RUBY_INSTALL_NAME).lib
	coff2omf $(RUBY)\lib\$(RUBY_INSTALL_NAME).lib $@

# For some reason, the coff2omf method doesn't work on libXpm.lib, so
# we have to manually generate an import library straight from the DLL.
xpm.lib: $(XPM)\lib\libXpm.lib
	implib -a $@ $(XPM)\bin\libXpm.dll

tcl.lib: $(TCL_LIB)
!if ("$(DYNAMIC_TCL)" == "yes")
	copy $(TCL_LIB) $@
!else
	coff2omf $(TCL_LIB) $@
!endif

!if ("$(DYNAMIC_TCL)" == "yes")
tclstub$(TCL_VER)-bor.lib:
	-@IF NOT EXIST $@ ECHO You must download tclstub$(TCL_VER)-bor.lib separately and\
	place it in the src directory in order to compile a dynamic TCL-enabled\
	(g)vim with the Borland compiler.  You can get the tclstub$(TCL_VER)-bor.lib file\
	at http://mywebpage.netscape.com/sharppeople/vim/tclstub$(TCL_VER)-bor.lib
!endif

# vimrun.exe:
vimrun.exe: vimrun.c
!if ("$(USEDLL)"=="yes")
	$(CC) -WC -O1 -I$(INCLUDE) -L$(LIB) -D_RTLDLL vimrun.c cw32mti.lib
!else
	$(CC) -WC -O1 -I$(INCLUDE) -L$(LIB) vimrun.c
!endif

# The dependency on $(OBJDIR) is to have bcc.cfg generated each time.
$(OBJDIR)\bcc.cfg: Make_bc5.mak $(OBJDIR)
  copy /y &&|
	$(CFLAGS)
	-L$(LIB)
	$(DEFINES)
	$(MBDEFINES)
	$(INTERP_DEFINES)
	$(EXETYPE)
	$(DEBUG_FLAG)
	$(OPT)
	$(CODEGUARD_FLAG)
	$(CPUARG)
	$(ALIGNARG)
| $@

# vi:set sts=4 sw=4:

