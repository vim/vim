#
# Makefile for VIM on Win32, using Cygnus gcc
# Last updated by Dan Sharp.  Last Change: 2010 Feb 24
#
# Also read INSTALLpc.txt!
#
# This compiles Vim as a Windows application.  If you want Vim to run as a
# Cygwin application use the Makefile (just like on Unix).
#
# GUI		no or yes: set to yes if you want the GUI version (yes)
# PERL		define to path to Perl dir to get Perl support (not defined)
#   PERL_VER	  define to version of Perl being used (56)
#   DYNAMIC_PERL  no or yes: set to yes to load the Perl DLL dynamically (yes)
# PYTHON	define to path to Python dir to get PYTHON support (not defined)
#   PYTHON_VER	    define to version of Python being used (22)
#   DYNAMIC_PYTHON  no or yes: use yes to load the Python DLL dynamically (yes)
# PYTHON3	define to path to Python3 dir to get PYTHON3 support (not defined)
#   PYTHON3_VER	    define to version of Python3 being used (22)
#   DYNAMIC_PYTHON3  no or yes: use yes to load the Python3 DLL dynamically (yes)
# TCL		define to path to TCL dir to get TCL support (not defined)
#   TCL_VER	define to version of TCL being used (83)
#   DYNAMIC_TCL no or yes: use yes to load the TCL DLL dynamically (yes)
# RUBY		define to path to Ruby dir to get Ruby support (not defined)
#   RUBY_VER	define to version of Ruby being used (16)
#   DYNAMIC_RUBY no or yes: use yes to load the Ruby DLL dynamically (yes)
# MZSCHEME	define to path to MzScheme dir to get MZSCHEME support (not defined)
#   MZSCHEME_VER      define to version of MzScheme being used (209_000)
#   DYNAMIC_MZSCHEME  no or yes: use yes to load the MzScheme DLLs dynamically (yes)
#   MZSCHEME_DLLS     path to MzScheme DLLs (libmzgc and libmzsch), for "static" build.
# LUA	define to path to Lua dir to get Lua support (not defined)
#   LUA_VER	    define to version of Lua being used (51)
#   DYNAMIC_LUA  no or yes: use yes to load the Lua DLL dynamically (yes)
# GETTEXT	no or yes: set to yes for dynamic gettext support (yes)
# ICONV		no or yes: set to yes for dynamic iconv support (yes)
# MBYTE		no or yes: set to yes to include multibyte support (yes)
# IME		no or yes: set to yes to include IME support (yes)
#   DYNAMIC_IME no or yes: set to yes to load imm32.dll dynamically (yes)
# OLE		no or yes: set to yes to make OLE gvim (no)
# DEBUG		no or yes: set to yes if you wish a DEBUGging build (no)
# CPUNR		No longer supported, use ARCH.
# ARCH		i386 through pentium4: select -march argument to compile with
#               (i386)
# USEDLL	no or yes: set to yes to use the Runtime library DLL (no)
#		For USEDLL=yes the cygwin1.dll is required to run Vim.
#		"no" does not work with latest version of Cygwin, use
#		Make_ming.mak instead.  Or set CC to gcc-3 and add
#		-L/lib/w32api to EXTRA_LIBS.
# POSTSCRIPT	no or yes: set to yes for PostScript printing (no)
# FEATURES	TINY, SMALL, NORMAL, BIG or HUGE (BIG)
# WINVER	Lowest Win32 version to support.  (0x0400)
# CSCOPE	no or yes: to include cscope interface support (yes)
# OPTIMIZE	SPACE, SPEED, or MAXSPEED: set optimization level (MAXSPEED)
# NETBEANS	no or yes: to include netbeans interface support (yes when GUI
#		is yes)
# NBDEBUG	no or yes: to include netbeans interface debugging support (no)
# XPM		define to path to XPM dir to get XPM image support (not defined)
#>>>>> choose options:
ifndef GUI
GUI=yes
endif

ifndef FEATURES
FEATURES = BIG
endif

ifndef GETTEXT
GETTEXT = yes
endif

ifndef ICONV
ICONV = yes
endif

ifndef MBYTE
MBYTE = yes
endif

ifndef IME
IME = yes
endif

ifndef ARCH
ARCH = i386
endif

ifndef WINVER
WINVER = 0x0400
endif

ifndef CSCOPE
CSCOPE = yes
endif

ifndef NETBEANS
ifeq ($(GUI),yes)
NETBEANS = yes
endif
endif

ifndef OPTIMIZE
OPTIMIZE = MAXSPEED
endif

### See feature.h for a list of optionals.
### Any other defines can be included here.

DEFINES = -DWIN32 -DHAVE_PATHDEF -DFEAT_$(FEATURES) \
	  -DWINVER=$(WINVER) -D_WIN32_WINNT=$(WINVER)
INCLUDES = -march=$(ARCH) -Iproto

#>>>>> name of the compiler and linker, name of lib directory
CROSS_COMPILE =
CC = gcc
RC = windres

##############################
# DYNAMIC_PERL=yes and no both work
##############################
ifdef PERL
DEFINES += -DFEAT_PERL
INCLUDES += -I$(PERL)/lib/CORE
EXTRA_OBJS += $(OUTDIR)/if_perl.o

ifndef DYNAMIC_PERL
DYNAMIC_PERL = yes
endif

ifndef PERL_VER
PERL_VER = 56
endif

ifeq (yes, $(DYNAMIC_PERL))
DEFINES += -DDYNAMIC_PERL -DDYNAMIC_PERL_DLL=\"perl$(PERL_VER).dll\"
else
EXTRA_LIBS += $(PERL)/lib/CORE/perl$(PERL_VER).lib
endif
endif

##############################
# DYNAMIC_PYTHON=yes works.
# DYNAMIC_PYTHON=no does not (unresolved externals on link).
##############################
ifdef PYTHON
DEFINES += -DFEAT_PYTHON
EXTRA_OBJS += $(OUTDIR)/if_python.o

ifndef DYNAMIC_PYTHON
DYNAMIC_PYTHON = yes
endif

ifndef PYTHON_VER
PYTHON_VER = 22
endif

ifeq (yes, $(DYNAMIC_PYTHON))
DEFINES += -DDYNAMIC_PYTHON -DDYNAMIC_PYTHON_DLL=\"python$(PYTHON_VER).dll\"
else
EXTRA_LIBS += $(PYTHON)/libs/python$(PYTHON_VER).lib
endif
endif

##############################
# DYNAMIC_PYTHON3=yes works.
# DYNAMIC_PYTHON3=no does not (unresolved externals on link).
##############################
ifdef PYTHON3
DEFINES += -DFEAT_PYTHON3
EXTRA_OBJS += $(OUTDIR)/if_python3.o

ifndef DYNAMIC_PYTHON3
DYNAMIC_PYTHON3 = yes
endif

ifndef PYTHON3_VER
PYTHON3_VER = 31
endif

ifeq (yes, $(DYNAMIC_PYTHON3))
DEFINES += -DDYNAMIC_PYTHON3 -DDYNAMIC_PYTHON3_DLL=\"python$(PYTHON3_VER).dll\"
else
EXTRA_LIBS += $(PYTHON3)/libs/python$(PYTHON3_VER).lib
endif
endif

##############################
# DYNAMIC_RUBY=yes works.
# DYNAMIC_RUBY=no does not (process exits).
##############################
ifdef RUBY

ifndef RUBY_VER
RUBY_VER=16
endif

ifndef RUBY_VER_LONG
RUBY_VER_LONG=1.6
endif

ifndef DYNAMIC_RUBY
DYNAMIC_RUBY = yes
endif

ifeq ($(RUBY_VER), 16)
ifndef RUBY_PLATFORM
RUBY_PLATFORM = i586-mswin32
endif
ifndef RUBY_INSTALL_NAME
RUBY_INSTALL_NAME = mswin32-ruby$(RUBY_VER)
endif
else
ifndef RUBY_PLATFORM
RUBY_PLATFORM = i386-mswin32
endif
ifndef RUBY_INSTALL_NAME
RUBY_INSTALL_NAME = msvcrt-ruby$(RUBY_VER)
endif
endif

DEFINES += -DFEAT_RUBY
INCLUDES += -I$(RUBY)/lib/ruby/$(RUBY_VER_LONG)/$(RUBY_PLATFORM)
EXTRA_OBJS += $(OUTDIR)/if_ruby.o

ifeq (yes, $(DYNAMIC_RUBY))
DEFINES += -DDYNAMIC_RUBY -DDYNAMIC_RUBY_DLL=\"$(RUBY_INSTALL_NAME).dll\"
DEFINES += -DDYNAMIC_RUBY_VER=$(RUBY_VER)
else
EXTRA_LIBS += $(RUBY)/lib/$(RUBY_INSTALL_NAME).lib
endif
endif

##############################
# DYNAMIC_MZSCHEME=yes works
# DYNAMIC_MZSCHEME=no works too
##############################
ifdef MZSCHEME
DEFINES += -DFEAT_MZSCHEME
INCLUDES += -I$(MZSCHEME)/include
EXTRA_OBJS += $(OUTDIR)/if_mzsch.o

ifndef DYNAMIC_MZSCHEME
DYNAMIC_MZSCHEME = yes
endif

ifndef MZSCHEME_VER
MZSCHEME_VER = 209_000
endif

ifndef MZSCHEME_PRECISE_GC
MZSCHEME_PRECISE_GC=no
endif

# for version 4.x we need to generate byte-code for Scheme base
ifndef MZSCHEME_GENERATE_BASE
MZSCHEME_GENERATE_BASE=no
endif

ifeq (yes, $(DYNAMIC_MZSCHEME))
DEFINES += -DDYNAMIC_MZSCHEME -DDYNAMIC_MZSCH_DLL=\"libmzsch$(MZSCHEME_VER).dll\" -DDYNAMIC_MZGC_DLL=\"libmzgc$(MZSCHEME_VER).dll\"
else
ifndef MZSCHEME_DLLS
MZSCHEME_DLLS = $(MZSCHEME)
endif
ifeq (yes,$(MZSCHEME_PRECISE_GC))
MZSCHEME_LIB=-lmzsch$(MZSCHEME_VER)
else
MZSCHEME_LIB = -lmzsch$(MZSCHEME_VER) -lmzgc$(MZSCHEME_VER)
endif
EXTRA_LIBS += -L$(MZSCHEME_DLLS) -L$(MZSCHEME_DLLS)/lib $(MZSCHEME_LIB)
endif
ifeq (yes,$(MZSCHEME_GENERATE_BASE))
DEFINES += -DINCLUDE_MZSCHEME_BASE
MZ_EXTRA_DEP += mzscheme_base.c
endif
ifeq (yes,$(MZSCHEME_PRECISE_GC))
DEFINES += -DMZ_PRECISE_GC
endif
endif

##############################
# DYNAMIC_TCL=yes and no both work.
##############################
ifdef TCL
DEFINES += -DFEAT_TCL
INCLUDES += -I$(TCL)/include
EXTRA_OBJS += $(OUTDIR)/if_tcl.o

ifndef DYNAMIC_TCL
DYNAMIC_TCL = yes
endif

ifndef TCL_VER
TCL_VER = 83
endif

ifeq (yes, $(DYNAMIC_TCL))
DEFINES += -DDYNAMIC_TCL -DDYNAMIC_TCL_DLL=\"tcl$(TCL_VER).dll\"
EXTRA_LIBS += $(TCL)/lib/tclstub$(TCL_VER).lib
else
EXTRA_LIBS += $(TCL)/lib/tcl$(TCL_VER).lib
endif
endif

##############################
# DYNAMIC_LUA=yes works.
# DYNAMIC_LUA=no does not (unresolved externals on link).
##############################
ifdef LUA
DEFINES += -DFEAT_LUA
INCLUDES += -I$(LUA)/include
EXTRA_OBJS += $(OUTDIR)/if_lua.o

ifndef DYNAMIC_LUA
DYNAMIC_LUA = yes
endif

ifndef LUA_VER
LUA_VER = 51
endif

ifeq (yes, $(DYNAMIC_LUA))
DEFINES += -DDYNAMIC_LUA -DDYNAMIC_LUA_DLL=\"lua$(LUA_VER).dll\"
else
EXTRA_LIBS += $(LUA)/lib/lua$(LUA_VER).lib
endif
endif

##############################
ifeq (yes, $(GETTEXT))
DEFINES += -DDYNAMIC_GETTEXT
endif

##############################
ifeq (yes, $(ICONV))
DEFINES += -DDYNAMIC_ICONV
endif

##############################
ifeq (yes, $(MBYTE))
DEFINES += -DFEAT_MBYTE
endif

##############################
ifeq (yes, $(IME))
DEFINES += -DFEAT_MBYTE_IME

ifndef DYNAMIC_IME
DYNAMIC_IME = yes
endif

ifeq (yes, $(DYNAMIC_IME))
DEFINES += -DDYNAMIC_IME
else
EXTRA_LIBS += -limm32
endif
endif

##############################
ifeq (yes, $(DEBUG))
DEFINES += -DDEBUG
INCLUDES += -g -fstack-check
DEBUG_SUFFIX = d
else

ifeq ($(OPTIMIZE), SIZE)
OPTFLAG = -Os
else
ifeq ($(OPTIMIZE), MAXSPEED)
OPTFLAG = -O3 -fomit-frame-pointer -freg-struct-return
else
OPTFLAG = -O2
endif
endif

# A bug in the GCC <= 3.2 optimizer can cause a crash.  The
# following option removes the problem optimization.
OPTFLAG += -fno-strength-reduce

INCLUDES += -s

endif

##############################
# USEDLL=yes will build a Cygwin32 executable that relies on cygwin1.dll.
# USEDLL=no will build a Mingw32 executable with no extra dll dependencies.
##############################
ifeq (yes, $(USEDLL))
DEFINES += -D_MAX_PATH=256 -D__CYGWIN__
else
INCLUDES += -mno-cygwin
endif

##############################
ifeq (yes, $(POSTSCRIPT))
DEFINES += -DMSWINPS
endif

##############################
ifeq (yes, $(CSCOPE))
DEFINES += -DFEAT_CSCOPE
EXTRA_OBJS += $(OUTDIR)/if_cscope.o
endif

##############################
ifeq ($(GUI),yes)

##############################
ifeq (yes, $(NETBEANS))
# Only allow NETBEANS for a GUI build.
DEFINES += -DFEAT_NETBEANS_INTG
EXTRA_OBJS += $(OUTDIR)/netbeans.o
EXTRA_LIBS += -lwsock32

ifeq (yes, $(NBDEBUG))
DEFINES += -DNBDEBUG
NBDEBUG_DEP = nbdebug.h nbdebug.c
endif

endif

##############################
ifdef XPM
# Only allow XPM for a GUI build.
DEFINES += -DFEAT_XPM_W32
INCLUDES += -I$(XPM)/include
EXTRA_OBJS += $(OUTDIR)/xpm_w32.o
EXTRA_LIBS += -L$(XPM)/lib -lXpm
endif

##############################
EXE = gvim$(DEBUG_SUFFIX).exe
OUTDIR = gobj$(DEBUG_SUFFIX)
DEFINES += -DFEAT_GUI_W32 -DFEAT_CLIPBOARD
EXTRA_OBJS += $(OUTDIR)/gui.o $(OUTDIR)/gui_w32.o $(OUTDIR)/gui_beval.o $(OUTDIR)/os_w32exe.o
EXTRA_LIBS += -mwindows -lcomctl32 -lversion
else
EXE = vim$(DEBUG_SUFFIX).exe
OUTDIR = obj$(DEBUG_SUFFIX)
LIBS += -luser32 -lgdi32 -lcomdlg32
endif

##############################
ifeq (yes, $(OLE))
DEFINES += -DFEAT_OLE
EXTRA_OBJS += $(OUTDIR)/if_ole.o
EXTRA_LIBS += -loleaut32 -lstdc++
endif

##############################
ifneq (sh.exe, $(SHELL))
DEL = rm
MKDIR = mkdir -p
DIRSLASH = /
else
DEL = del
MKDIR = mkdir
DIRSLASH = \\
endif

#>>>>> end of choices
###########################################################################

INCL = vim.h globals.h option.h keymap.h macros.h ascii.h term.h os_win32.h \
       structs.h version.h

CFLAGS = $(OPTFLAG) $(DEFINES) $(INCLUDES)

RCFLAGS = -O coff $(DEFINES)

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
	$(OUTDIR)/window.o \
	$(EXTRA_OBJS)

all: $(EXE) xxd/xxd.exe vimrun.exe install.exe uninstal.exe GvimExt/gvimext.dll

# According to the Cygwin doc 1.2 FAQ, kernel32 should not be specified for
# linking unless calling ld directly.
# See /usr/doc/cygwin-doc-1.2/html/faq_toc.html#TOC93 for more information.
$(EXE): $(OUTDIR) $(OBJ)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ) $(LIBS) -luuid -lole32 $(EXTRA_LIBS)

xxd/xxd.exe: xxd/xxd.c
	$(MAKE) -C xxd -f Make_cyg.mak CC=$(CC) USEDLL=$(USEDLL)

GvimExt/gvimext.dll: GvimExt/gvimext.cpp GvimExt/gvimext.rc GvimExt/gvimext.h
	$(MAKE) -C GvimExt -f Make_ming.mak CROSS_COMPILE=$(CROSS_COMPILE)

vimrun.exe: vimrun.c
	$(CC) $(CFLAGS) -o vimrun.exe vimrun.c  $(LIBS)

install.exe: dosinst.c
	$(CC) $(CFLAGS) -o install.exe dosinst.c  $(LIBS) -luuid -lole32

uninstal.exe: uninstal.c
	$(CC) $(CFLAGS) -o uninstal.exe uninstal.c $(LIBS)

$(OUTDIR):
	$(MKDIR) $(OUTDIR)

tags:
	command /c ctags *.c $(INCL)

clean:
	-$(DEL) $(OUTDIR)$(DIRSLASH)*.o
	-rmdir $(OUTDIR)
	-$(DEL) $(EXE) vimrun.exe install.exe uninstal.exe
ifdef PERL
	-$(DEL) if_perl.c
endif
ifdef MZSCHEME
	-$(DEL) mzscheme_base.c
endif
	-$(DEL) pathdef.c
	$(MAKE) -C xxd -f Make_cyg.mak clean
	$(MAKE) -C GvimExt -f Make_ming.mak clean

distclean: clean
	-$(DEL) obj$(DIRSLASH)*.o
	-rmdir obj
	-$(DEL) gobj$(DIRSLASH)*.o
	-rmdir gobj
	-$(DEL) objd$(DIRSLASH)*.o
	-rmdir objd
	-$(DEL) gobjd$(DIRSLASH)*.o
	-rmdir gobjd
	-$(DEL) *.exe

###########################################################################

$(OUTDIR)/%.o : %.c $(INCL)
	$(CC) -c $(CFLAGS) $< -o $@

$(OUTDIR)/ex_docmd.o:	ex_docmd.c $(INCL) ex_cmds.h
	$(CC) -c $(CFLAGS) ex_docmd.c -o $(OUTDIR)/ex_docmd.o

$(OUTDIR)/ex_eval.o:	ex_eval.c $(INCL) ex_cmds.h
	$(CC) -c $(CFLAGS) ex_eval.c -o $(OUTDIR)/ex_eval.o

$(OUTDIR)/if_cscope.o:	if_cscope.c $(INCL) if_cscope.h
	$(CC) -c $(CFLAGS) if_cscope.c -o $(OUTDIR)/if_cscope.o

$(OUTDIR)/if_ole.o:	if_ole.cpp $(INCL)
	$(CC) -c $(CFLAGS) if_ole.cpp -o $(OUTDIR)/if_ole.o

$(OUTDIR)/if_python.o : if_python.c $(INCL)
	$(CC) -c $(CFLAGS) -I$(PYTHON)/include $< -o $@

$(OUTDIR)/if_python3.o : if_python3.c $(INCL)
	$(CC) -c $(CFLAGS) -I$(PYTHON3)/include $< -o $@

if_perl.c: if_perl.xs typemap
	$(PERL)/bin/perl `cygpath -d $(PERL)/lib/ExtUtils/xsubpp` \
		-prototypes -typemap \
		`cygpath -d $(PERL)/lib/ExtUtils/typemap` if_perl.xs > $@

$(OUTDIR)/if_perl.o:	if_perl.c $(INCL)
ifeq (yes, $(USEDLL))
	$(CC) -c $(CFLAGS) -I/usr/include/mingw -D__MINGW32__ if_perl.c -o $(OUTDIR)/if_perl.o
endif

$(OUTDIR)/if_ruby.o:	if_ruby.c $(INCL)
ifeq (16, $(RUBY_VER))
	$(CC) -c $(CFLAGS) -U_WIN32 if_ruby.c -o $(OUTDIR)/if_ruby.o
endif

$(OUTDIR)/netbeans.o:	netbeans.c $(INCL) $(NBDEBUG_DEP)
	$(CC) -c $(CFLAGS) netbeans.c -o $(OUTDIR)/netbeans.o

$(OUTDIR)/if_mzsch.o:	if_mzsch.c $(INCL) if_mzsch.h $(MZ_EXTRA_DEP)
	$(CC) -c $(CFLAGS) if_mzsch.c -o $(OUTDIR)/if_mzsch.o

$(OUTDIR)/vimrc.o:	vim.rc version.h gui_w32_rc.h
	$(RC) $(RCFLAGS) vim.rc -o $(OUTDIR)/vimrc.o

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
	@echo 'char_u *all_lflags = (char_u *)"$(CC) -s -o $(EXE) $(LIBS) -luuid -lole32 $(EXTRA_LIBS)";' >> pathdef.c
	@echo 'char_u *compiled_user = (char_u *)"$(USERNAME)";' >> pathdef.c
	@echo 'char_u *compiled_sys = (char_u *)"$(USERDOMAIN)";' >> pathdef.c
else
	@echo creating pathdef.c
	@echo /* pathdef.c */ > pathdef.c
	@echo #include "vim.h" >> pathdef.c
	@echo char_u *default_vim_dir = (char_u *)"$(VIMRCLOC)"; >> pathdef.c
	@echo char_u *default_vimruntime_dir = (char_u *)"$(VIMRUNTIMEDIR)"; >> pathdef.c
	@echo char_u *all_cflags = (char_u *)"$(CC) $(CFLAGS)"; >> pathdef.c
	@echo char_u *all_lflags = (char_u *)"$(CC) -s -o $(EXE) $(LIBS) -luuid -lole32 $(EXTRA_LIBS)"; >> pathdef.c
	@echo char_u *compiled_user = (char_u *)"$(USERNAME)"; >> pathdef.c
	@echo char_u *compiled_sys = (char_u *)"$(USERDOMAIN)"; >> pathdef.c
endif

