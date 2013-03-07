# Makefile for VIM on Win32
#
# Info at http://www.mingw.org
# Alternative x86 and 64-builds: http://mingw-w64.sourceforge.net
# Also requires GNU make, which you can download from the same sites.
# Get missing libraries from http://gnuwin32.sf.net.
#
# Tested on Win32 NT 4 and Win95.
#
# To make everything, just 'make -f Make_ming.mak'.
# To make just e.g. gvim.exe, 'make -f Make_ming.mak gvim.exe'.
# After a run, you can 'make -f Make_ming.mak clean' to clean up.
#
# NOTE: Sometimes 'GNU Make' will stop after building vimrun.exe -- I think
# it's just run out of memory or something.  Run again, and it will continue
# with 'xxd'.
#
# "make upx" makes *compressed* versions of the 32 bit GUI and console EXEs,
# using the excellent UPX compressor:
#     http://upx.sourceforge.net/
# "make mpress" uses the MPRESS compressor for 32- and 64-bit EXEs:
#     http://www.matcode.com/mpress.htm
#
# Maintained by Ron Aaron <ronaharon@yahoo.com> et al.
# Updated 2012 Sep 5.

#>>>>> choose options:
# set to yes for a debug build
DEBUG=no
# set to SIZE for size, SPEED for speed, MAXSPEED for maximum optimization
OPTIMIZE=MAXSPEED
# set to yes to make gvim, no for vim
GUI=yes
# FEATURES=[TINY | SMALL  | NORMAL | BIG | HUGE]
# Set to TINY to make minimal version (few features).
FEATURES=BIG
# Set to one of i386, i486, i586, i686 as the minimum target processor.
# For amd64/x64 architecture set ARCH=x86-64 .
ARCH=i386
# Set to yes to cross-compile from unix; no=native Windows.
CROSS=no
# Set to path to iconv.h and libiconv.a to enable using 'iconv.dll'.
#ICONV="."
ICONV=yes
GETTEXT=yes
# Set to yes to include multibyte support.
MBYTE=yes
# Set to yes to include IME support.
IME=yes
DYNAMIC_IME=yes
# Set to yes to enable writing a postscript file with :hardcopy.
POSTSCRIPT=no
# Set to yes to enable OLE support.
OLE=no
# Set the default $(WINVER) to make it work with pre-Win2k.
ifndef WINVER
WINVER = 0x0500
endif
# Set to yes to enable Cscope support.
CSCOPE=yes
# Set to yes to enable Netbeans support.
NETBEANS=$(GUI)


# Link against the shared version of libstdc++ by default.  Set
# STATIC_STDCPLUS to "yes" to link against static version instead.
ifndef STATIC_STDCPLUS
STATIC_STDCPLUS=no
endif

# If the user doesn't want gettext, undefine it.
ifeq (no, $(GETTEXT))
GETTEXT=
endif
# Added by E.F. Amatria <eferna1@platea.ptic.mec.es> 2001 Feb 23
# Uncomment the first line and one of the following three if you want Native Language
# Support.  You'll need gnu_gettext.win32, a MINGW32 Windows PORT of gettext by
# Franco Bez <franco.bez@gmx.de>.  It may be found at
# http://home.a-city.de/franco.bez/gettext/gettext_win32_en.html
# Tested with mingw32 with GCC-2.95.2 on Win98
# Updated 2001 Jun 9
#GETTEXT=c:/gettext.win32.msvcrt
#STATIC_GETTEXT=USE_STATIC_GETTEXT
#DYNAMIC_GETTEXT=USE_GETTEXT_DLL
#DYNAMIC_GETTEXT=USE_SAFE_GETTEXT_DLL
SAFE_GETTEXT_DLL_OBJ = $(GETTEXT)/src/safe_gettext_dll/safe_gettext_dll.o
# Alternatively, if you uncomment the two following lines, you get a "safe" version
# without linking the safe_gettext_dll.o object file.
#DYNAMIC_GETTEXT=DYNAMIC_GETTEXT
#GETTEXT_DYNAMIC=gnu_gettext.dll
INTLPATH=$(GETTEXT)/lib/mingw32
INTLLIB=gnu_gettext

# If you are using gettext-0.10.35 from http://sourceforge.net/projects/gettext
# or gettext-0.10.37 from http://sourceforge.net/projects/mingwrep/
# uncomment the following, but I can't build a static version with them, ?-(|
#GETTEXT=c:/gettext-0.10.37-20010430
#STATIC_GETTEXT=USE_STATIC_GETTEXT
#DYNAMIC_GETTEXT=DYNAMIC_GETTEXT
#INTLPATH=$(GETTEXT)/lib
#INTLLIB=intl

# uncomment 'PERL' if you want a perl-enabled version
#PERL=C:/perl
ifdef PERL
ifndef PERL_VER
PERL_VER=56
endif
ifndef DYNAMIC_PERL
DYNAMIC_PERL=yes
endif
# on Linux, for cross-compile, it's here:
#PERLLIB=/home/ron/ActivePerl/lib
# on NT, it's here:
PERLLIB=$(PERL)/lib
PERLLIBS=$(PERLLIB)/Core
XSUBPPTRY=$(PERLLIB)/ExtUtils/xsubpp
XSUBPP_EXISTS=$(shell perl -e "print 1 unless -e '$(XSUBPPTRY)'")
ifeq "$(XSUBPP_EXISTS)" ""
XSUBPP=perl $(XSUBPPTRY)
else
XSUBPP=xsubpp
endif
endif

# uncomment 'LUA' if you want a Lua-enabled version
#LUA=/usr/local
ifdef LUA
ifndef DYNAMIC_LUA
DYNAMIC_LUA=yes
endif

ifndef LUA_VER
LUA_VER=51
endif

ifeq (no,$(DYNAMIC_LUA))
LUA_LIB = -L$(LUA)/lib -llua
endif

endif

# uncomment 'MZSCHEME' if you want a MzScheme-enabled version
#MZSCHEME=d:/plt
ifdef MZSCHEME
ifndef DYNAMIC_MZSCHEME
DYNAMIC_MZSCHEME=yes
endif

ifndef MZSCHEME_VER
MZSCHEME_VER=205_000
endif

ifndef MZSCHEME_PRECISE_GC
MZSCHEME_PRECISE_GC=no
endif

# for version 4.x we need to generate byte-code for Scheme base
ifndef MZSCHEME_GENERATE_BASE
MZSCHEME_GENERATE_BASE=no
endif

ifndef MZSCHEME_USE_RACKET
MZSCHEME_MAIN_LIB=mzsch
else
MZSCHEME_MAIN_LIB=racket
endif

ifeq (no,$(DYNAMIC_MZSCHEME))
ifeq (yes,$(MZSCHEME_PRECISE_GC))
MZSCHEME_LIB=-l$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER)
else
MZSCHEME_LIB = -l$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER) -lmzgc$(MZSCHEME_VER)
endif
# the modern MinGW can dynamically link to dlls directly.
# point MZSCHEME_DLLS to where you put libmzschXXXXXXX.dll and libgcXXXXXXX.dll
ifndef MZSCHEME_DLLS
MZSCHEME_DLLS=$(MZSCHEME)
endif
MZSCHEME_LIBDIR=-L$(MZSCHEME_DLLS) -L$(MZSCHEME_DLLS)\lib
endif

endif

# Python support -- works with the ActiveState python 2.0 release (and others
# too, probably)
#
# uncomment 'PYTHON' to make python-enabled version
# Put the path to the python distro here.  If cross compiling from Linux, you
# will also need to convert the header files to unix instead of dos format:
#   for fil in *.h ; do vim -e -c 'set ff=unix|w|q' $fil
# and also, you will need to make a mingw32 'libpython20.a' to link with:
#   cd $PYTHON/libs
#   pexports python20.dll > python20.def
#   dlltool -d python20.def -l libpython20.a
# on my Linux box, I put the Python stuff here:
#PYTHON=/home/ron/ActivePython-2.0.0-202/src/Core
# on my NT box, it's here:
#PYTHON=c:/python20

ifdef PYTHON
ifndef DYNAMIC_PYTHON
DYNAMIC_PYTHON=yes
endif

ifndef PYTHON_VER
PYTHON_VER=22
endif

ifeq (no,$(DYNAMIC_PYTHON))
PYTHONLIB=-L$(PYTHON)/libs -lpython$(PYTHON_VER)
endif
# my include files are in 'win32inc' on Linux, and 'include' in the standard
# NT distro (ActiveState)
ifeq ($(CROSS),no)
PYTHONINC=-I $(PYTHON)/include
else
PYTHONINC=-I $(PYTHON)/win32inc
endif
endif

#PYTHON3: See comment for Python 2 above

ifdef PYTHON3
ifndef DYNAMIC_PYTHON3
DYNAMIC_PYTHON3=yes
endif

ifndef PYTHON3_VER
PYTHON3_VER=31
endif

ifeq (no,$(DYNAMIC_PYTHON3))
PYTHON3LIB=-L$(PYTHON3)/libs -lPYTHON$(PYTHON3_VER)
endif

ifeq ($(CROSS),no)
PYTHON3INC=-I $(PYTHON3)/include
else
PYTHON3INC=-I $(PYTHON3)/win32inc
endif
endif

#	TCL interface:
#	  TCL=[Path to TCL directory]
#	  DYNAMIC_TCL=yes (to load the TCL DLL dynamically)
#	  TCL_VER=[TCL version, eg 83, 84] (default is 83)
#TCL=c:/tcl
ifdef TCL
ifndef DYNAMIC_TCL
DYNAMIC_TCL=yes
endif
ifndef TCL_VER
TCL_VER = 83
endif
TCLINC += -I$(TCL)/include
endif


#	Ruby interface:
#	  RUBY=[Path to Ruby directory]
#	  DYNAMIC_RUBY=yes (to load the Ruby DLL dynamically)
#	  RUBY_VER=[Ruby version, eg 16, 17] (default is 16)
#	  RUBY_VER_LONG=[Ruby version, eg 1.6, 1.7] (default is 1.6)
#	    You must set RUBY_VER_LONG when changing RUBY_VER.
#	    You must set RUBY_API_VER version to RUBY_VER_LONG.
#	    Don't set ruby API version to RUBY_VER like 191.
#RUBY=c:/ruby
ifdef RUBY
ifndef DYNAMIC_RUBY
DYNAMIC_RUBY=yes
endif
#  Set default value
ifndef RUBY_VER
RUBY_VER = 16
endif
ifndef RUBY_VER_LONG
RUBY_VER_LONG = 1.6
endif
ifndef RUBY_API_VER
RUBY_API_VER = $(subst .,,$(RUBY_VER_LONG))
endif

ifndef RUBY_PLATFORM
ifeq ($(RUBY_VER), 16)
RUBY_PLATFORM = i586-mswin32
else
ifneq ($(wildcard $(RUBY)/lib/ruby/$(RUBY_VER_LONG)/i386-mingw32),)
RUBY_PLATFORM = i386-mingw32
else
ifneq ($(wildcard $(RUBY)/lib/ruby/$(RUBY_VER_LONG)/x64-mingw32),)
RUBY_PLATFORM = x64-mingw32
else
RUBY_PLATFORM = i386-mswin32
endif
endif
endif
endif

ifndef RUBY_INSTALL_NAME
ifeq ($(RUBY_VER), 16)
RUBY_INSTALL_NAME = mswin32-ruby$(RUBY_API_VER)
else
ifeq ($(ARCH),x86-64)
RUBY_INSTALL_NAME = x64-msvcrt-ruby$(RUBY_API_VER)
else
RUBY_INSTALL_NAME = msvcrt-ruby$(RUBY_API_VER)
endif
endif
endif

ifeq (19, $(word 1,$(sort 19 $(RUBY_VER))))
RUBY_19_OR_LATER = 1
endif

RUBYINC = -I $(RUBY)/lib/ruby/$(RUBY_VER_LONG)/$(RUBY_PLATFORM)
ifdef RUBY_19_OR_LATER
RUBYINC += -I $(RUBY)/include/ruby-$(RUBY_VER_LONG) -I $(RUBY)/include/ruby-$(RUBY_VER_LONG)/$(RUBY_PLATFORM)
endif
ifeq (no, $(DYNAMIC_RUBY))
RUBYLIB = -L$(RUBY)/lib -l$(RUBY_INSTALL_NAME)
endif

endif # RUBY

# See feature.h for a list of options.
# Any other defines can be included here.
DEF_GUI=-DFEAT_GUI_W32 -DFEAT_CLIPBOARD
DEFINES=-DWIN32 -DWINVER=$(WINVER) -D_WIN32_WINNT=$(WINVER) \
	-DHAVE_PATHDEF -DFEAT_$(FEATURES)
ifeq ($(CROSS),yes)
# cross-compiler prefix:
CROSS_COMPILE = i586-pc-mingw32msvc-
DEL = rm
MKDIR = mkdir -p
DIRSLASH = /
else
# normal (Windows) compilation:
CROSS_COMPILE =
ifneq (sh.exe, $(SHELL))
DEL = rm
MKDIR = mkdir -p
DIRSLASH = /
else
DEL = del
MKDIR = mkdir
DIRSLASH = \\
endif
endif
CC := $(CROSS_COMPILE)gcc
WINDRES := $(CROSS_COMPILE)windres
WINDRES_CC = $(CC)

#>>>>> end of choices
###########################################################################

CFLAGS = -Iproto $(DEFINES) -pipe -w -march=$(ARCH) -Wall
WINDRES_FLAGS = --preprocessor="$(WINDRES_CC) -E -xc" -DRC_INVOKED

ifdef GETTEXT
DEFINES += -DHAVE_GETTEXT -DHAVE_LOCALE_H
GETTEXTINCLUDE = $(GETTEXT)/include
GETTEXTLIB = $(INTLPATH)
ifeq (yes, $(GETTEXT))
DEFINES += -DDYNAMIC_GETTEXT
else
ifdef DYNAMIC_GETTEXT
DEFINES += -D$(DYNAMIC_GETTEXT)
ifdef GETTEXT_DYNAMIC
DEFINES += -DGETTEXT_DYNAMIC -DGETTEXT_DLL=\"$(GETTEXT_DYNAMIC)\"
endif
endif
endif
endif

ifdef PERL
CFLAGS += -I$(PERLLIBS) -DFEAT_PERL -L$(PERLLIBS)
ifeq (yes, $(DYNAMIC_PERL))
CFLAGS += -DDYNAMIC_PERL -DDYNAMIC_PERL_DLL=\"perl$(PERL_VER).dll\"
endif
endif

ifdef LUA
CFLAGS += -I$(LUA)/include -DFEAT_LUA
ifeq (yes, $(DYNAMIC_LUA))
CFLAGS += -DDYNAMIC_LUA -DDYNAMIC_LUA_DLL=\"lua$(LUA_VER).dll\"
endif
endif

ifdef MZSCHEME
CFLAGS += -I$(MZSCHEME)/include -DFEAT_MZSCHEME -DMZSCHEME_COLLECTS=\"$(MZSCHEME)/collects\"
ifeq (yes, $(DYNAMIC_MZSCHEME))
CFLAGS += -DDYNAMIC_MZSCHEME -DDYNAMIC_MZSCH_DLL=\"lib$(MZSCHEME_MAIN_LIB)$(MZSCHEME_VER).dll\" -DDYNAMIC_MZGC_DLL=\"libmzgc$(MZSCHEME_VER).dll\"
endif
ifeq (yes, "$(MZSCHEME_DEBUG)")
CFLAGS += -DMZSCHEME_FORCE_GC
endif
endif

ifdef RUBY
CFLAGS += -DFEAT_RUBY $(RUBYINC)
ifeq (yes, $(DYNAMIC_RUBY))
CFLAGS += -DDYNAMIC_RUBY -DDYNAMIC_RUBY_DLL=\"$(RUBY_INSTALL_NAME).dll\"
CFLAGS += -DDYNAMIC_RUBY_VER=$(RUBY_VER)
endif
endif

ifdef PYTHON
CFLAGS += -DFEAT_PYTHON 
ifeq (yes, $(DYNAMIC_PYTHON))
CFLAGS += -DDYNAMIC_PYTHON
endif
endif

ifdef PYTHON3 
CFLAGS += -DFEAT_PYTHON3 
ifeq (yes, $(DYNAMIC_PYTHON3))
CFLAGS += -DDYNAMIC_PYTHON3 
endif
endif

ifdef TCL
CFLAGS += -DFEAT_TCL $(TCLINC)
ifeq (yes, $(DYNAMIC_TCL))
CFLAGS += -DDYNAMIC_TCL -DDYNAMIC_TCL_DLL=\"tcl$(TCL_VER).dll\"
endif
endif

ifeq ($(POSTSCRIPT),yes)
DEFINES += -DMSWINPS
endif

ifeq (yes, $(OLE))
DEFINES += -DFEAT_OLE
endif

ifeq ($(CSCOPE),yes)
DEFINES += -DFEAT_CSCOPE
endif

ifeq ($(NETBEANS),yes)
# Only allow NETBEANS for a GUI build.
ifeq (yes, $(GUI))
DEFINES += -DFEAT_NETBEANS_INTG

ifeq ($(NBDEBUG), yes)
DEFINES += -DNBDEBUG
NBDEBUG_INCL = nbdebug.h
NBDEBUG_SRC = nbdebug.c
endif
endif
endif

# Only allow XPM for a GUI build.
ifeq (yes, $(GUI))

ifndef XPM
ifeq ($(ARCH),i386)
XPM = xpm/x86
endif
ifeq ($(ARCH),i486)
XPM = xpm/x86
endif
ifeq ($(ARCH),i586)
XPM = xpm/x86
endif
ifeq ($(ARCH),i686)
XPM = xpm/x86
endif
ifeq ($(ARCH),x86-64)
XPM = xpm/x64
endif
endif
ifdef XPM
ifneq ($(XPM),no)
CFLAGS += -DFEAT_XPM_W32 -I $(XPM)/include -I $(XPM)/../include
endif
endif

endif

ifeq ($(DEBUG),yes)
CFLAGS += -g -fstack-check
DEBUG_SUFFIX=d
else
ifeq ($(OPTIMIZE), SIZE)
CFLAGS += -Os
else
ifeq ($(OPTIMIZE), MAXSPEED)
CFLAGS += -O3
CFLAGS += -fomit-frame-pointer -freg-struct-return
else  # SPEED
CFLAGS += -O2
endif
endif
CFLAGS += -s
endif

LIB = -lkernel32 -luser32 -lgdi32 -ladvapi32 -lcomdlg32 -lcomctl32 -lversion
GUIOBJ =  $(OUTDIR)/gui.o $(OUTDIR)/gui_w32.o $(OUTDIR)/gui_beval.o $(OUTDIR)/os_w32exe.o
OBJ = \
	$(OUTDIR)/blowfish.o \
	$(OUTDIR)/buffer.o \
	$(OUTDIR)/charset.o \
	$(OUTDIR)/diff.o \
	$(OUTDIR)/digraph.o \
	$(OUTDIR)/edit.o \
	$(OUTDIR)/eval.o \
	$(OUTDIR)/ex_cmds.o \
	$(OUTDIR)/ex_cmds2.o \
	$(OUTDIR)/ex_docmd.o \
	$(OUTDIR)/ex_eval.o \
	$(OUTDIR)/ex_getln.o \
	$(OUTDIR)/fileio.o \
	$(OUTDIR)/fold.o \
	$(OUTDIR)/getchar.o \
	$(OUTDIR)/hardcopy.o \
	$(OUTDIR)/hashtab.o \
	$(OUTDIR)/main.o \
	$(OUTDIR)/mark.o \
	$(OUTDIR)/memfile.o \
	$(OUTDIR)/memline.o \
	$(OUTDIR)/menu.o \
	$(OUTDIR)/message.o \
	$(OUTDIR)/misc1.o \
	$(OUTDIR)/misc2.o \
	$(OUTDIR)/move.o \
	$(OUTDIR)/mbyte.o \
	$(OUTDIR)/normal.o \
	$(OUTDIR)/ops.o \
	$(OUTDIR)/option.o \
	$(OUTDIR)/os_win32.o \
	$(OUTDIR)/os_mswin.o \
	$(OUTDIR)/winclip.o \
	$(OUTDIR)/pathdef.o \
	$(OUTDIR)/popupmnu.o \
	$(OUTDIR)/quickfix.o \
	$(OUTDIR)/regexp.o \
	$(OUTDIR)/screen.o \
	$(OUTDIR)/search.o \
	$(OUTDIR)/sha256.o \
	$(OUTDIR)/spell.o \
	$(OUTDIR)/syntax.o \
	$(OUTDIR)/tag.o \
	$(OUTDIR)/term.o \
	$(OUTDIR)/ui.o \
	$(OUTDIR)/undo.o \
	$(OUTDIR)/version.o \
	$(OUTDIR)/vimrc.o \
	$(OUTDIR)/window.o

ifdef PERL
OBJ += $(OUTDIR)/if_perl.o
endif
ifdef LUA
OBJ += $(OUTDIR)/if_lua.o
endif
ifdef MZSCHEME
OBJ += $(OUTDIR)/if_mzsch.o
MZSCHEME_INCL = if_mzsch.h
ifeq (yes,$(MZSCHEME_GENERATE_BASE))
CFLAGS += -DINCLUDE_MZSCHEME_BASE
MZ_EXTRA_DEP += mzscheme_base.c
endif
ifeq (yes,$(MZSCHEME_PRECISE_GC))
CFLAGS += -DMZ_PRECISE_GC
endif
endif
ifdef PYTHON
OBJ += $(OUTDIR)/if_python.o
endif
ifdef PYTHON3
OBJ += $(OUTDIR)/if_python3.o
endif
ifdef RUBY
OBJ += $(OUTDIR)/if_ruby.o
endif
ifdef TCL
OBJ += $(OUTDIR)/if_tcl.o
endif
ifeq ($(CSCOPE),yes)
OBJ += $(OUTDIR)/if_cscope.o
endif
ifeq ($(NETBEANS),yes)
# Only allow NETBEANS for a GUI build.
ifeq (yes, $(GUI))
OBJ += $(OUTDIR)/netbeans.o
LIB += -lwsock32
endif
endif
ifdef XPM
# Only allow XPM for a GUI build.
ifeq (yes, $(GUI))
OBJ += $(OUTDIR)/xpm_w32.o
# You'll need libXpm.a from http://gnuwin32.sf.net
LIB += -L $(XPM)/lib -lXpm
endif
endif


ifdef MZSCHEME
MZSCHEME_SUFFIX = Z
endif

ifeq ($(GUI),yes)
TARGET := gvim$(DEBUG_SUFFIX).exe
DEFINES += $(DEF_GUI)
OBJ += $(GUIOBJ)
LFLAGS += -mwindows
OUTDIR = gobj$(DEBUG_SUFFIX)$(MZSCHEME_SUFFIX)$(ARCH)
else
TARGET := vim$(DEBUG_SUFFIX).exe
OUTDIR = obj$(DEBUG_SUFFIX)$(MZSCHEME_SUFFIX)$(ARCH)
endif

ifdef GETTEXT
ifneq (yes, $(GETTEXT))
CFLAGS += -I$(GETTEXTINCLUDE)
ifndef STATIC_GETTEXT
LIB += -L$(GETTEXTLIB) -l$(INTLLIB)
ifeq (USE_SAFE_GETTEXT_DLL, $(DYNAMIC_GETTEXT))
OBJ+=$(SAFE_GETTEXT_DLL_OBJ)
endif
else
LIB += -L$(GETTEXTLIB) -lintl
endif
endif
endif

ifdef PERL
ifeq (no, $(DYNAMIC_PERL))
LIB += -lperl$(PERL_VER)
endif
endif

ifdef TCL
LIB += -L$(TCL)/lib
ifeq (yes, $(DYNAMIC_TCL))
LIB += -ltclstub$(TCL_VER)
else
LIB += -ltcl$(TCL_VER)
endif
endif

ifeq (yes, $(OLE))
LIB += -loleaut32
OBJ += $(OUTDIR)/if_ole.o
ifeq (yes, $(STATIC_STDCPLUS))
LIB += -Wl,-Bstatic -lstdc++ -Wl,-Bdynamic
else
LIB += -lstdc++
endif
endif

ifeq (yes, $(MBYTE))
DEFINES += -DFEAT_MBYTE
endif

ifeq (yes, $(IME))
DEFINES += -DFEAT_MBYTE_IME
ifeq (yes, $(DYNAMIC_IME))
DEFINES += -DDYNAMIC_IME
else
LIB += -limm32
endif
endif

ifdef ICONV
ifneq (yes, $(ICONV))
LIB += -L$(ICONV)
CFLAGS += -I$(ICONV)
endif
DEFINES+=-DDYNAMIC_ICONV
endif

all: $(TARGET) vimrun.exe xxd/xxd.exe install.exe uninstal.exe GvimExt/gvimext.dll

vimrun.exe: vimrun.c
	$(CC) $(CFLAGS) -o vimrun.exe vimrun.c $(LIB)

install.exe: dosinst.c
	$(CC) $(CFLAGS) -o install.exe dosinst.c $(LIB) -lole32 -luuid

uninstal.exe: uninstal.c
	$(CC) $(CFLAGS) -o uninstal.exe uninstal.c $(LIB)

$(TARGET): $(OUTDIR) $(OBJ)
	$(CC) $(CFLAGS) $(LFLAGS) -o $@ $(OBJ) $(LIB) -lole32 -luuid $(LUA_LIB) $(MZSCHEME_LIBDIR) $(MZSCHEME_LIB) $(PYTHONLIB) $(PYTHON3LIB) $(RUBYLIB)

upx: exes
	upx gvim.exe
	upx vim.exe

mpress: exes
	mpress gvim.exe
	mpress vim.exe

xxd/xxd.exe: xxd/xxd.c
	$(MAKE) -C xxd -f Make_ming.mak CC=$(CC)

GvimExt/gvimext.dll: GvimExt/gvimext.cpp GvimExt/gvimext.rc GvimExt/gvimext.h
	$(MAKE) -C GvimExt -f Make_ming.mak CROSS=$(CROSS) CROSS_COMPILE=$(CROSS_COMPILE)

clean:
	-$(DEL) $(OUTDIR)$(DIRSLASH)*.o
	-$(DEL) $(OUTDIR)$(DIRSLASH)*.res
	-rmdir $(OUTDIR)
	-$(DEL) *.exe
	-$(DEL) pathdef.c
ifdef PERL
	-$(DEL) if_perl.c
endif
ifdef MZSCHEME
	-$(DEL) mzscheme_base.c
endif
	$(MAKE) -C GvimExt -f Make_ming.mak clean
	$(MAKE) -C xxd -f Make_ming.mak clean

###########################################################################
INCL = vim.h feature.h os_win32.h os_dos.h ascii.h keymap.h term.h macros.h \
	structs.h regexp.h option.h ex_cmds.h proto.h globals.h farsi.h \
	gui.h

$(OUTDIR)/if_python.o : if_python.c $(INCL)
	$(CC) -c $(CFLAGS) $(PYTHONINC) -DDYNAMIC_PYTHON_DLL=\"python$(PYTHON_VER).dll\" $< -o $@

$(OUTDIR)/if_python3.o : if_python3.c $(INCL)
	$(CC) -c $(CFLAGS) $(PYTHON3INC) -DDYNAMIC_PYTHON3_DLL=\"PYTHON$(PYTHON3_VER).dll\" $< -o $@

$(OUTDIR)/%.o : %.c $(INCL)
	$(CC) -c $(CFLAGS) $< -o $@

$(OUTDIR)/vimrc.o: vim.rc version.h gui_w32_rc.h
	$(WINDRES) $(WINDRES_FLAGS) $(DEFINES) \
	    --input-format=rc --output-format=coff -i vim.rc -o $@

$(OUTDIR):
	$(MKDIR) $(OUTDIR)

$(OUTDIR)/ex_docmd.o:	ex_docmd.c $(INCL) ex_cmds.h
	$(CC) -c $(CFLAGS) ex_docmd.c -o $(OUTDIR)/ex_docmd.o

$(OUTDIR)/ex_eval.o:	ex_eval.c $(INCL) ex_cmds.h
	$(CC) -c $(CFLAGS) ex_eval.c -o $(OUTDIR)/ex_eval.o

$(OUTDIR)/gui_w32.o:	gui_w32.c gui_w48.c $(INCL)
	$(CC) -c $(CFLAGS) gui_w32.c -o $(OUTDIR)/gui_w32.o

$(OUTDIR)/if_cscope.o:	if_cscope.c $(INCL) if_cscope.h
	$(CC) -c $(CFLAGS) if_cscope.c -o $(OUTDIR)/if_cscope.o

# Remove -D__IID_DEFINED__ for newer versions of the w32api
$(OUTDIR)/if_ole.o: if_ole.cpp $(INCL)
	$(CC) $(CFLAGS) -c -o $(OUTDIR)/if_ole.o if_ole.cpp

$(OUTDIR)/if_ruby.o: if_ruby.c $(INCL)
ifeq (16, $(RUBY))
	$(CC) $(CFLAGS) -U_WIN32 -c -o $(OUTDIR)/if_ruby.o if_ruby.c
endif

if_perl.c: if_perl.xs typemap
	$(XSUBPP) -prototypes -typemap \
	     $(PERLLIB)/ExtUtils/typemap if_perl.xs > $@

$(OUTDIR)/netbeans.o:	netbeans.c $(INCL) $(NBDEBUG_INCL) $(NBDEBUG_SRC)
	$(CC) -c $(CFLAGS) netbeans.c -o $(OUTDIR)/netbeans.o

$(OUTDIR)/if_mzsch.o:	if_mzsch.c $(INCL) if_mzsch.h $(MZ_EXTRA_DEP)
	$(CC) -c $(CFLAGS) if_mzsch.c -o $(OUTDIR)/if_mzsch.o

mzscheme_base.c:
	$(MZSCHEME)/mzc --c-mods mzscheme_base.c ++lib scheme/base

pathdef.c: $(INCL)
ifneq (sh.exe, $(SHELL))
	@echo creating pathdef.c
	@echo '/* pathdef.c */' > pathdef.c
	@echo '#include "vim.h"' >> pathdef.c
	@echo 'char_u *default_vim_dir = (char_u *)"$(VIMRCLOC)";' >> pathdef.c
	@echo 'char_u *default_vimruntime_dir = (char_u *)"$(VIMRUNTIMEDIR)";' >> pathdef.c
	@echo 'char_u *all_cflags = (char_u *)"$(CC) $(CFLAGS)";' >> pathdef.c
	@echo 'char_u *all_lflags = (char_u *)"$(CC) $(CFLAGS) $(LFLAGS) -o $(TARGET) $(LIB) -lole32 -luuid $(LUA_LIB) $(MZSCHEME_LIBDIR) $(MZSCHEME_LIB) $(PYTHONLIB) $(PYTHON3LIB) $(RUBYLIB)";' >> pathdef.c
	@echo 'char_u *compiled_user = (char_u *)"$(USERNAME)";' >> pathdef.c
	@echo 'char_u *compiled_sys = (char_u *)"$(USERDOMAIN)";' >> pathdef.c
else
	@echo creating pathdef.c
	@echo /* pathdef.c */ > pathdef.c
	@echo #include "vim.h" >> pathdef.c
	@echo char_u *default_vim_dir = (char_u *)"$(VIMRCLOC)"; >> pathdef.c
	@echo char_u *default_vimruntime_dir = (char_u *)"$(VIMRUNTIMEDIR)"; >> pathdef.c
	@echo char_u *all_cflags = (char_u *)"$(CC) $(CFLAGS)"; >> pathdef.c
	@echo char_u *all_lflags = (char_u *)"$(CC) $(CFLAGS) $(LFLAGS) -o $(TARGET) $(LIB) -lole32 -luuid $(LUA_LIB) $(MZSCHEME_LIBDIR) $(MZSCHEME_LIB) $(PYTHONLIB) $(PYTHON3LIB) $(RUBYLIB)"; >> pathdef.c
	@echo char_u *compiled_user = (char_u *)"$(USERNAME)"; >> pathdef.c
	@echo char_u *compiled_sys = (char_u *)"$(USERDOMAIN)"; >> pathdef.c
endif
