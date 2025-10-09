#
# Makefile for Vim on OpenVMS
#
# Maintainer:   Zoltan Arpadffy <zoltan.arpadffy@gmail.com>
# Last change:  2025-07-04  Steven M. Schweda <sms@antinode.info>
#
# This script has been tested on VMS 6.2 to 9.2 on VAX, ALPHA, IA64 and X86_64
# with MMS and MMK
#
# The following could be built:
#	vim.exe:	standard (terminal, GUI/Motif, GUI/GTK)
#	dvim.exe:	debug
#
######################################################################
#
# Edit the lines in the Configuration section below for fine tuning.
#
# To build:    mms/descrip=Make_vms.mms /ignore=warning
# To clean up: mms/descrip=Make_vms.mms clean
# To display --help report: mms/descrip=Make_vms.mms help
# To display --version report: mms/descrip=Make_vms.mms version
#
# Hints and detailed description could be found in INSTALLVMS.TXT file.
#
######################################################################
# Configuration section.
######################################################################

# Build model selection
# TINY   - No optional features enabled
# NORMAL - A default selection of features enabled
# HUGE   - All possible features enabled.
# Please select one of these alternatives above.
MODEL = HUGE

# GUI or terminal mode executable.
# Comment out if you want just the character terminal mode only.
# GUI with Motif
# GUI = YES

# GUI with GTK
# If you have GTK installed you might want to enable this option.
# NOTE: you will need to properly define GTK_DIR below
# NOTE: since Vim 7.3 GTK 2+ is used that is not ported to VMS,
#       therefore this option should not be used
# GTK = YES

# GUI/Motif with XPM
# If you have XPM installed you might want to build Motif version with toolbar
# XPM = YES

# Comment out if you want the compiler version with :ver command.
# NOTE: This part can make some complications if you're using some
# predefined symbols/flags for your compiler. If does, just leave behind
# the comment variable CCVER.
.IFDEF VAXC_OR_FORCE_VAXC       # VAXC_OR_FORCE_VAXC
.ELSE                           # VAXC_OR_FORCE_VAXC
CCVER = YES                     # Unreliable with VAX C.
.ENDIF                          # VAXC_OR_FORCE_VAXC [ELSE]

# Uncomment if want a debug version. Resulting executable is DVIM.EXE
# Development purpose only! Normally, it should not be defined. !!!
# DEBUG = YES

# Languages support for Perl, Python, TCL etc.
# If you don't need it really, leave them behind the comment.
# You will need related libraries, include files etc.
# VIM_TCL    = YES
# VIM_PERL   = YES
# VIM_PYTHON = YES
# VIM_PYTHON3= YES
# VIM_RUBY   = YES
# VIM_LUA    = YES

# X Input Method.  For entering special languages like chinese and
# Japanese.
# If you don't need it really, leave it behind the comment.
# VIM_XIM = YES

# Allow any white space to separate the fields in a tags file
# When not defined, only a TAB is allowed.
# VIM_TAG_ANYWHITE = YES

# Allow FEATURE_MZSCHEME
# VIM_MZSCHEME = YES

# Use ICONV
# VIM_ICONV = YES

# If you modified the source code and plan to distribute the build
# please, let the users know that.
# MODIFIED_BY = "name surname <your@email.com>"

######################################################################
# Directory, library and include files configuration section.
# Normally you need not to change anything below. !
# These may need to be defined if things are not in standard locations
#
# You can find some explanation in INSTALLVMS.TXT
######################################################################

# Architecture identification and product destination selection.

# Define old MMK architecture macros when using MMS.

.IFDEF MMS$ARCH_NAME            # MMS$ARCH_NAME
ALPHA_X_ALPHA = 1
IA64_X_IA64 = 1
VAX_X_VAX = 1
X86_64_X_X86_64 = 1
.IFDEF ARCH                         # ARCH
ARCH_NAME = $(ARCH)
.ELSE                               # ARCH
ARCH_NAME = $(MMS$ARCH_NAME)
.ENDIF                              # ARCH
.IFDEF $(ARCH_NAME)_X_ALPHA         # $(ARCH_NAME)_X_ALPHA
__ALPHA__ = 1
.ENDIF                              # $(ARCH_NAME)_X_ALPHA
.IFDEF $(ARCH_NAME)_X_IA64          # $(ARCH_NAME)_X_IA64
__IA64__ = 1
.ENDIF                              # $(ARCH_NAME)_X_IA64
.IFDEF $(ARCH_NAME)_X_VAX           # $(ARCH_NAME)_X_VAX
__VAX__ = 1
.ENDIF                              # $(ARCH_NAME)_X_VAX
.IFDEF $(ARCH_NAME)_X_X86_64        # $(ARCH_NAME)_X_X86_64
__X86_64__ = 1
.ENDIF                              # $(ARCH_NAME)_X_X86_64
.ELSE                           # MMS$ARCH_NAME
.IFDEF __MMK__                      # __MMK__
.IFDEF ARCH                             # ARCH
.IFDEF __$(ARCH)__                          # __$(ARCH)__
.ELSE                                       # __$(ARCH)__
__$(ARCH)__ = 1
.ENDIF                                      # __$(ARCH)__
.ENDIF                                  # ARCH
.ENDIF                              # __MMK__
.ENDIF                          # MMS$ARCH_NAME

# Combine command-line VAX C compiler macros.

.IFDEF VAXC                     # VAXC
VAXC_OR_FORCE_VAXC = 1
.ELSE                           # VAXC
.IFDEF FORCE_VAXC                   # FORCE_VAXC
VAXC_OR_FORCE_VAXC = 1
.ENDIF                              # FORCE_VAXC
.ENDIF                          # VAXC

# Analyze architecture-related and option macros.
# (Sense x86_64 before IA64 for old MMK and x86_64 cross tools.)

.IFDEF __X86_64__               # __X86_64__
DECC = 1
DESTM = X86_64
.ELSE                           # __X86_64__
.IFDEF __IA64__                     # __IA64__
DECC = 1
DESTM = IA64
.ELSE                               # __IA64__
.IFDEF __ALPHA__                        # __ALPHA__
DECC = 1
DESTM = ALPHA
.ELSE                                   # __ALPHA__
.IFDEF __VAX__                              # __VAX__
.IFDEF VAXC_OR_FORCE_VAXC                       # VAXC_OR_FORCE_VAXC
DESTM = VAXV
.ELSE                                           # VAXC_OR_FORCE_VAXC
DECC = 1
DESTM = VAX
.ENDIF                                          # VAXC_OR_FORCE_VAXC
.ELSE                                       # __VAX__
DESTM = UNK
UNK_DEST = 1
.ENDIF                                      # __VAX__
.ENDIF                                  # __ALPHA__
.ENDIF                              # __IA64__
.ENDIF                          # __X86_64__

.IFDEF PROD                     # PROD
DEST = $(PROD)
.ELSE                           # PROD
DEST = $(DESTM)
.ENDIF                          # PROD

.FIRST
.IFDEF __MMK__                  # __MMK__
        @ write sys$output ""
.ENDIF                          # __MMK__

# Create destination directory.
	@ write sys$output "Destination: [.$(DEST)]"
	@ write sys$output ""
	@ if (f$search( "$(DEST).DIR;1") .eqs. "") then -
         create /directory [.$(DEST)]

# Compiler setup

# Optimization.  The .c.obj rule will override this for specific modules
# where the VAX C compilers hang.   See VAX_NOOPTIM_LIST, below.
OPTIMIZE= /optim

.IFDEF __VAX__                  # __VAX__

# List of modules for which "Compaq C V6.4-005 on OpenVMS VAX V7.3"
# hangs.  Add more as needed (plus-separated).
VAX_NOOPTIM_LIST = blowfish+regexp+sha256

# Compiler command.
# Default: CC /DECC.  On non-VAX, or VAX with only DEC C installed,
# /DECC is harmless.  If both DEC C and VAX C are installed, and VAX C
# was selected as the default, then /DECC must be specified explicitly. 
# If both are installed, and DEC C is the default, but VAX C is desired,
# then define FORCE_VAXC to get VAX C (CC /VAXC).  If only VAX C is
# installed, then define VAXC to get (plain) CC.

.IFDEF DECC                         # DECC
CC_DEF = cc /decc
PREFIX = /prefix=all/name=(upper,short) /repository=[.$(DEST)]
.ELSE                               # DECC
.IFDEF FORCE_VAXC                       # FORCE_VAXC
CC_DEF = cc /vaxc
.ELSE                                   # FORCE_VAXC
CC_DEF = cc
.ENDIF                                  # FORCE_VAXC
.ENDIF                              # DECC
.ELSE                           # __VAX__

# Not VAX, therefore DEC C (with /NAMES, /PREFIX, and /REPOSITORY).

CC_DEF = cc /decc
PREFIX  = /prefix=all/name=(upper,short) /repository=[.$(DEST)]

# These floating-point options are the defaults on IA64 and x86_64.
# This makes Alpha consistent.
FLOAT   = /float = ieee_float /ieee_mode = denorm_results

# Large-file support.  Unavailable on VAX and very old Alpha.  To
# disable, define NOLARGE.
.IFDEF NOLARGE
.ELSE
LARGE_DEF = , "_LARGEFILE"
.ENDIF # NOLARGE [ELSE]

# .IFDEF MMSX86_64
# ARCH_DEF=        # ,__CRTL_VER_OVERRIDE=80400000
# .ENDIF

.ENDIF                          # __VAX__

.IFDEF LIST
LIST_OPT = /list=[.$(DEST)] /show=(all, nomessages)
MAP_OPT = /map /cross_reference /full
.ENDIF # LIST


LD_DEF  = link
C_INC   = [.proto]

.IFDEF DEBUG
DEBUG_DEF = ,"DEBUG"
TARGET    = [.$(DEST)]dvim.exe
CFLAGS    = /debug/noopt$(PREFIX)$(FLOAT)$(LIST_OPT)
LDFLAGS   = /debug $(MAP_OPT)
.ELSE
TARGET    = [.$(DEST)]vim.exe
CFLAGS    = $(OPTIMIZE)$(PREFIX)$(FLOAT)$(LIST_OPT)
LDFLAGS   = $(MAP_OPT)
.ENDIF

# Predefined VIM directories
# Please, use $VIM and $VIMRUNTIME logicals instead
VIMLOC  = ""
VIMRUN  = ""

CONFIG_H = os_vms_conf.h

# GTK or XPM but not both
.IFDEF GTK
.IFDEF GUI
.ELSE
GUI = YES
.ENDIF
.IFDEF XPM
XPM = ""
.ENDIF
.ENDIF

.IFDEF XPM
.IFDEF GUI
.ELSE
GUI = YES
.ENDIF
.IFDEF GTK
GTK = ""
.ENDIF
.ENDIF

.IFDEF GUI
# X/Motif/GTK executable  (also works in terminal mode )

.IFDEF GTK
# NOTE: you need to set up your GTK_DIR (GTK root directory), because it is
# unique on every system - logicals are not accepted
# please note: directory should end with . in order to /trans=conc work
# This value for GTK_DIR is an example.
GTK_DIR  = DKA0:[WORK.GTK1210.]
DEFS     = ,"HAVE_CONFIG_H","FEAT_GUI_GTK"
LIBS     = ,OS_VMS_GTK.OPT/OPT
GUI_FLAG = /float=ieee/ieee=denorm/WARNINGS=(DISABLE=MACROREDEF)
GUI_SRC  = gui.c gui_gtk.c gui_gtk_f.c gui_gtk_x11.c gui_beval.c pty.c
GUI_OBJ  = \
[.$(DEST)]gui.obj \
[.$(DEST)]gui_gtk.obj \
[.$(DEST)]gui_gtk_f.obj \
[.$(DEST)]gui_gtk_x11.obj \
[.$(DEST)]gui_beval.obj \
[.$(DEST)]pty.obj

GUI_INC  = ,"/gtk_root/gtk","/gtk_root/glib"
# GUI_INC_VER is used just for :ver information
# this string should escape from C and DCL in the same time
GUI_INC_VER= ,\""/gtk_root/gtk\"",\""/gtk_root/glib\""
.ELSE
MOTIF	 = YES
.IFDEF XPM
DEFS     = ,"HAVE_CONFIG_H","FEAT_GUI_MOTIF","HAVE_XPM"
XPM_INC  = ,[.xpm.include]
XPM_LIB  = ,OS_VMS_XPM.OPT/OPT
.ELSE
DEFS     = ,"HAVE_CONFIG_H","FEAT_GUI_MOTIF"
XPM_INC  =
.ENDIF
LIBS     = ,OS_VMS_MOTIF.OPT/OPT
GUI_FLAG = /WARNINGS=(DISABLE=MACROREDEF)
GUI_SRC  = gui.c gui_motif.c gui_x11.c gui_beval.c gui_xmdlg.c gui_xmebw.c
GUI_OBJ  = \
[.$(DEST)]gui.obj \
[.$(DEST)]gui_motif.obj \
[.$(DEST)]gui_x11.obj \
[.$(DEST)]gui_beval.obj \
[.$(DEST)]gui_xmdlg.obj \
[.$(DEST)]gui_xmebw.obj

GUI_INC  =
.ENDIF

# You need to define these variables if you do not have DECW files
# at standard location
GUI_INC_DIR = ,decw$include:
# GUI_LIB_DIR = ,sys$library:

.ELSE
# Character terminal only executable
DEFS	 = ,"HAVE_CONFIG_H"
LIBS	 =
.ENDIF

.IFDEF VIM_PERL
# Perl related setup.
PERL	 = perl
PERL_DEF = ,"FEAT_PERL"
PERL_SRC = if_perl.xs
PERL_OBJ = \
[.$(DEST)]if_perl.obj

PERL_LIB = ,OS_VMS_PERL.OPT/OPT
PERL_INC = ,dka0:[perlbuild.perl.lib.vms_axp.5_6_1.core]
.ENDIF

.IFDEF VIM_PYTHON
# Python related setup.
PYTHON_DEF = ,"FEAT_PYTHON"
PYTHON_SRC = if_python.c
PYTHON_OBJ = [.$(DEST)]if_python.obj
PYTHON_LIB = ,OS_VMS_PYTHON.OPT/OPT
PYTHON_INC = ,PYTHON_INCLUDE
.ENDIF

.IFDEF VIM_PYTHON3
# Python related setup.
PYTHON3_DEF = ,"FEAT_PYTHON3"
PYTHON3_SRC = if_python3.c
PYTHON3_OBJ = [.$(DEST)]if_python3.obj
PYTHON3_LIB = ,OS_VMS_PYTHON3.OPT/OPT
PYTHON3_INC = ,PYTHON3_INCLUDE
.ENDIF


.IFDEF VIM_TCL
# TCL related setup.
TCL_DEF = ,"FEAT_TCL"
TCL_SRC = if_tcl.c
TCL_OBJ = [.$(DEST)]if_tcl.obj
TCL_LIB = ,OS_VMS_TCL.OPT/OPT
TCL_INC = ,dka0:[tcl80.generic]
.ENDIF

.IFDEF VIM_RUBY
# RUBY related setup.
RUBY_DEF = ,"FEAT_RUBY"
RUBY_SRC = if_ruby.c
RUBY_OBJ = [.$(DEST)]if_ruby.obj
RUBY_LIB = ,OS_VMS_RUBY.OPT/OPT
RUBY_INC =
.ENDIF

.IFDEF VIM_LUA
# LUA related setup.
LUA_DEF = ,"FEAT_LUA"
LUA_SRC = if_lua.c
LUA_OBJ = [.$(DEST)]if_lua.obj
LUA_LIB = ,OS_VMS_LUA.OPT/OPT
LUA_INC = ,LUA$ROOT:[INCLUDE]
.ENDIF

.IFDEF VIM_XIM
# XIM related setup.
.IFDEF GUI
XIM_DEF = ,"FEAT_XIM"
.ENDIF
.ENDIF

.IFDEF VIM_MZSCHEME
# MZSCHEME related setup
MZSCHEME_DEF = ,"FEAT_MZSCHEME"
MZSCHEME_SRC = if_mzsch.c
MZSCHEME_OBJ = [.$(DEST)]if_mzsch.obj
.ENDIF

.IFDEF VIM_ICONV
# ICONV related setup
ICONV_DEF = ,"USE_ICONV"
.ENDIF

# XDIFF related setup.
XDIFF_SRC = xdiffi.c,xemit.c,xprepare.c,xutils.c,xhistogram.c,xpatience.c
XDIFF_OBJ = \
[.$(DEST)]xdiffi.obj,\
[.$(DEST)]xemit.obj,\
[.$(DEST)]xprepare.obj,\
[.$(DEST)]xutils.obj,\
[.$(DEST)]xhistogram.obj,\
[.$(DEST)]xpatience.obj

XDIFF_INC = ,[.xdiff]

.IFDEF MODIFIED_BY
DEF_MODIFIED = YES
.ELSE
DEF_MODIFIED = NO
.ENDIF

######################################################################
# End of configuration section.
# Please, do not change anything below without programming experience.
######################################################################

MODEL_DEF = "FEAT_$(MODEL)"

# These go into pathdef.c
VIMUSER = "''F$EDIT(F$GETJPI(" ","USERNAME"),"TRIM")'"
VIMHOST = "''F$TRNLNM("SYS$NODE")'''F$TRNLNM("UCX$INET_HOST")'.''F$TRNLNM("UCX$INET_DOMAIN")'"

.SUFFIXES : .obj .c     # Case problems with old MMS?  Ignore: %MMS-I-ALRINSUFFLST

ALL_CFLAGS = /def=($(MODEL_DEF)$(DEFS)$(DEBUG_DEF)$(PERL_DEF) -
 $(PYTHON_DEF)$(PYTHON3_DEF) $(TCL_DEF)$(RUBY_DEF)$(LUA_DEF) -
 $(XIM_DEF)$(TAG_DEF)$(MZSCHEME_DEF) $(ICONV_DEF)$(ARCH_DEF) -
 $(LARGE_DEF)) -
 $(CFLAGS)$(GUI_FLAG) -
 /include=([.$(DEST)],$(C_INC)$(GUI_INC_DIR)$(GUI_INC)$(PERL_INC) -
 $(PYTHON_INC)$(PYTHON3_INC)$(TCL_INC)$(XDIFF_INC)$(XPM_INC))

# CFLAGS displayed in :ver information
# It is specially formatted for correct display of unix like includes
# as $(GUI_INC) - replaced with $(GUI_INC_VER)
# Otherwise should not be any other difference.
ALL_CFLAGS_VER1 = /def=($(MODEL_DEF)$(DEFS)$(DEBUG_DEF)$(PERL_DEF) -
 $(PYTHON_DEF)$(PYTHON3_DEF) -
 $(TCL_DEF)$(RUBY_DEF)$(LUA_DEF)$(XIM_DEF)$(TAG_DEF)$(MZSCHEME_DEF)
ALL_CFLAGS_VER2 = $(ICONV_DEF)$(ARCH_DEF)$(LARGE_DEF)) -
 $(CFLAGS)$(GUI_FLAG) -
 /include=($(C_INC)$(GUI_INC_DIR)$(GUI_INC_VER)$(PERL_INC) -
 $(PYTHON_INC)$(PYTHON3_INC) $(TCL_INC)$(XDIFF_INC)$(XPM_INC))

ALL_LIBS = $(LIBS) $(GUI_LIB_DIR) $(GUI_LIB) $(XPM_LIB)\
	   $(PERL_LIB) $(PYTHON_LIB) $(PYTHON3_LIB) $(TCL_LIB) $(RUBY_LIB) $(LUA_LIB)

SRC = \
 alloc.c \
 arabic.c \
 arglist.c \
 autocmd.c \
 beval.c \
 blob.c \
 blowfish.c \
 buffer.c \
 bufwrite.c \
 change.c \
 channel.c \
 charset.c \
 cindent.c \
 clientserver.c \
 clipboard.c \
 cmdexpand.c \
 cmdhist.c \
 crypt.c \
 crypt_zip.c \
 debugger.c \
 dict.c \
 diff.c \
 digraph.c \
 drawline.c \
 drawscreen.c \
 edit.c \
 eval.c \
 evalbuffer.c \
 evalfunc.c \
 evalvars.c \
 evalwindow.c \
 ex_cmds.c \
 ex_cmds2.c \
 ex_docmd.c \
 ex_eval.c \
 ex_getln.c \
 fileio.c \
 filepath.c, \
 findfile.c \
 float.c \
 fold.c \
 fuzzy.c \
 getchar.c \
 gc.c \
 gui_xim.c \
 hardcopy.c \
 hashtab.c \
 help.c \
 highlight.c \
 if_cscope.c \
 if_xcmdsrv.c \
 indent.c \
 insexpand.c \
 job.c \
 json.c \
 linematch.c \
 list.c \
 locale.c \
 logfile.c \
 main.c \
 map.c \
 mark.c \
 match.c \
 mbyte.c \
 memfile.c \
 memline.c \
 menu.c \
 message.c \
 misc1.c \
 misc2.c \
 mouse.c \
 move.c \
 normal.c \
 ops.c \
 option.c \
 optionstr.c \
 os_unix.c \
 os_vms.c \
 [.$(DEST)]pathdef.c \
 popupmenu.c \
 popupwin.c \
 profiler.c \
 pty.c \
 quickfix.c \
 regexp.c \
 register.c \
 screen.c \
 scriptfile.c \
 search.c \
 session.c \
 sha256.c \
 sign.c \
 sound.c \
 spell.c \
 spellfile.c \
 spellsuggest.c \
 strings.c \
 syntax.c \
 tabpanel.c \
 tag.c \
 term.c \
 terminal.c \
 termlib.c \
 testing.c \
 textformat.c \
 textobject.c \
 textprop.c \
 time.c \
 tuple.c \
 typval.c \
 ui.c \
 undo.c \
 usercmd.c \
 userfunc.c \
 version.c \
 vim9class.c \
 vim9cmds.c \
 vim9compile.c \
 vim9execute.c \
 vim9expr.c \
 vim9instr.c \
 vim9generics.c \
 vim9script.c \
 vim9type.c \
 viminfo.c \
 window.c \
 $(GUI_SRC) \
 $(XDIFF_SRC) \
 $(LUA_SRC) \
 $(MZSCHEME_SRC) \
 $(PERL_SRC) \
 $(PYTHON_SRC) \
 $(PYTHON3_SRC) \
 $(TCL_SRC) \
 $(RUBY_SRC)

OBJ = \
 [.$(DEST)]alloc.obj \
 [.$(DEST)]arabic.obj \
 [.$(DEST)]arglist.obj \
 [.$(DEST)]autocmd.obj \
 [.$(DEST)]beval.obj \
 [.$(DEST)]blob.obj \
 [.$(DEST)]blowfish.obj \
 [.$(DEST)]buffer.obj \
 [.$(DEST)]bufwrite.obj \
 [.$(DEST)]change.obj \
 [.$(DEST)]channel.obj \
 [.$(DEST)]charset.obj \
 [.$(DEST)]cindent.obj \
 [.$(DEST)]clientserver.obj \
 [.$(DEST)]clipboard.obj \
 [.$(DEST)]cmdexpand.obj \
 [.$(DEST)]cmdhist.obj \
 [.$(DEST)]crypt.obj \
 [.$(DEST)]crypt_zip.obj \
 [.$(DEST)]debugger.obj \
 [.$(DEST)]dict.obj \
 [.$(DEST)]diff.obj \
 [.$(DEST)]digraph.obj \
 [.$(DEST)]drawline.obj \
 [.$(DEST)]drawscreen.obj \
 [.$(DEST)]edit.obj \
 [.$(DEST)]eval.obj \
 [.$(DEST)]evalbuffer.obj \
 [.$(DEST)]evalfunc.obj \
 [.$(DEST)]evalvars.obj \
 [.$(DEST)]evalwindow.obj \
 [.$(DEST)]ex_cmds.obj \
 [.$(DEST)]ex_cmds2.obj \
 [.$(DEST)]ex_docmd.obj \
 [.$(DEST)]ex_eval.obj \
 [.$(DEST)]ex_getln.obj \
 [.$(DEST)]fileio.obj \
 [.$(DEST)]filepath.obj \
 [.$(DEST)]findfile.obj \
 [.$(DEST)]float.obj \
 [.$(DEST)]fold.obj \
 [.$(DEST)]fuzzy.obj \
 [.$(DEST)]getchar.obj \
 [.$(DEST)]gc.obj \
 [.$(DEST)]gui_xim.obj \
 [.$(DEST)]hardcopy.obj \
 [.$(DEST)]hashtab.obj \
 [.$(DEST)]help.obj \
 [.$(DEST)]highlight.obj \
 [.$(DEST)]if_cscope.obj \
 [.$(DEST)]if_mzsch.obj \
 [.$(DEST)]if_xcmdsrv.obj \
 [.$(DEST)]indent.obj \
 [.$(DEST)]insexpand.obj \
 [.$(DEST)]job.obj \
 [.$(DEST)]json.obj \
 [.$(DEST)]linematch.obj \
 [.$(DEST)]list.obj \
 [.$(DEST)]locale.obj \
 [.$(DEST)]logfile.obj \
 [.$(DEST)]main.obj \
 [.$(DEST)]map.obj \
 [.$(DEST)]mark.obj \
 [.$(DEST)]match.obj \
 [.$(DEST)]mbyte.obj \
 [.$(DEST)]memfile.obj \
 [.$(DEST)]memline.obj \
 [.$(DEST)]menu.obj \
 [.$(DEST)]message.obj \
 [.$(DEST)]misc1.obj \
 [.$(DEST)]misc2.obj \
 [.$(DEST)]mouse.obj \
 [.$(DEST)]move.obj \
 [.$(DEST)]normal.obj \
 [.$(DEST)]ops.obj \
 [.$(DEST)]option.obj \
 [.$(DEST)]optionstr.obj \
 [.$(DEST)]os_unix.obj \
 [.$(DEST)]os_vms.obj \
 [.$(DEST)]pathdef.obj \
 [.$(DEST)]popupmenu.obj \
 [.$(DEST)]popupwin.obj \
 [.$(DEST)]profiler.obj \
 [.$(DEST)]pty.obj \
 [.$(DEST)]quickfix.obj \
 [.$(DEST)]regexp.obj \
 [.$(DEST)]register.obj \
 [.$(DEST)]screen.obj \
 [.$(DEST)]scriptfile.obj \
 [.$(DEST)]search.obj \
 [.$(DEST)]session.obj \
 [.$(DEST)]sha256.obj \
 [.$(DEST)]sign.obj \
 [.$(DEST)]sound.obj \
 [.$(DEST)]spell.obj \
 [.$(DEST)]spellfile.obj \
 [.$(DEST)]spellsuggest.obj \
 [.$(DEST)]strings.obj \
 [.$(DEST)]syntax.obj \
 [.$(DEST)]tabpanel.obj \
 [.$(DEST)]tag.obj \
 [.$(DEST)]term.obj \
 [.$(DEST)]terminal.obj \
 [.$(DEST)]termlib.obj \
 [.$(DEST)]testing.obj \
 [.$(DEST)]textformat.obj \
 [.$(DEST)]textobject.obj \
 [.$(DEST)]textprop.obj \
 [.$(DEST)]time.obj \
 [.$(DEST)]tuple.obj \
 [.$(DEST)]typval.obj \
 [.$(DEST)]ui.obj \
 [.$(DEST)]undo.obj \
 [.$(DEST)]usercmd.obj \
 [.$(DEST)]userfunc.obj \
 [.$(DEST)]version.obj \
 [.$(DEST)]vim9class.obj \
 [.$(DEST)]vim9cmds.obj \
 [.$(DEST)]vim9compile.obj \
 [.$(DEST)]vim9execute.obj \
 [.$(DEST)]vim9expr.obj \
 [.$(DEST)]vim9instr.obj \
 [.$(DEST)]vim9generics.obj \
 [.$(DEST)]vim9script.obj \
 [.$(DEST)]vim9type.obj \
 [.$(DEST)]viminfo.obj \
 [.$(DEST)]window.obj \
 $(GUI_OBJ) \
 $(XDIFF_OBJ) \
 $(LUA_OBJ) \
 $(MZSCHEME_OBJ) \
 $(PERL_OBJ) \
 $(PYTHON_OBJ) \
 $(PYTHON3_OBJ) \
 $(TCL_OBJ) \
 $(RUBY_OBJ)

# Default target is making the executable
all : [.$(DEST)]config.h mmk_compat motif_env gtk_env perl_env \
      python_env tcl_env ruby_env lua_env $(TARGET)
	! $@

[.$(DEST)]config.h : $(CONFIG_H)
	copy/nolog $(CONFIG_H) [.$(DEST)]config.h
	-@ open/append ac [.$(DEST)]config.h
        -@ hash[0,8]=35
	-@ quotes[0,8]=34
        -@ if ""$(DEF_MODIFIED)"" .EQS. "YES" then write ac ''hash',"define MODIFIED_BY ",''quotes',$(MODIFIED_BY),''quotes'
	-@ close ac

mmk_compat :
	-@ open/write pd [.$(DEST)]pathdef.c
	-@ write pd "/* Empty file to satisfy MMK depend.  */"
	-@ write pd "/* It will be overwritten later... */"
	-@ close pd
clean :
	-@ if (f$search( "[.$(DEST)]*.*") .nes. "") then -
         delete /noconfirm [.$(DEST)]*.*;*
	-@ if (f$search( "$(DEST).DIR") .nes. "") then -
         set protection = w:d $(DEST).DIR;*
	-@ if (f$search( "$(DEST).DIR") .nes. "") then -
         delete /noconfirm $(DEST).DIR;*

help :
	mcr sys$disk:$(TARGET) --help

version :
	mcr sys$disk:$(TARGET) --version

# Link the target
$(TARGET) : $(OBJ)
#     make an OPT file - as the obj file list is too long for one command line
	-@ DIRECTORY [.$(DEST)]*.OBJ. /BRIEF/COLUMNS=1 /NOHEADING -
            /NOTRAILING /SELECT=FILE=(NONODE,NODEVICE,NODIRECTORY,NOVERSION) -
            /OUTPUT=[.$(DEST)]ALL_OBJS_LIST.OPT
	-@ def_dev_dir_orig = f$environment( "default")
	-@ target_name_type = -
         f$parse( "$(TARGET)", , , "NAME", "SYNTAX_ONLY")+ -
         f$parse( "$(TARGET)", , , "TYPE", "SYNTAX_ONLY")
        -@ set default [.$(DEST)]
	$(LD_DEF) $(LDFLAGS) /exe = 'target_name_type' -
         ALL_OBJS_LIST.OPT/OPT $(ALL_LIBS)
	-@ set default 'def_dev_dir_orig'

.c.obj :
# Override /optimize for selected modules on VAX.
.IFDEF __VAX__                  # __VAX__
	@ mod = f$parse( "$@", , , "NAME", "SYNTAX_ONLY")
	@ mod = "+"+ f$edit( mod, "LOWERCASE")+ "+"
	@ optim_qual = ""
	@ if (f$locate( mod, "+$(VAX_NOOPTIM_LIST)+") .lt. -
         f$length( "+$(VAX_NOOPTIM_LIST)+")) then optim_qual = "/nooptim"
	@ if (f$locate( mod, "+$(VAX_NOOPTIM_LIST)+") .lt. -
         f$length( "+$(VAX_NOOPTIM_LIST)+")) then -
	@ write sys$output -
          "                *** NOTE: USING SPECIAL /NOOPTIMIZE RULE. ***"
	$(CC_DEF) $(ALL_CFLAGS) 'optim_qual' $< /object = $@
.ELSE                           # __VAX__
	$(CC_DEF) $(ALL_CFLAGS) $< /object = $@
.ENDIF                          # __VAX__ [ELSE]

[.$(DEST)]pathdef.c : check_ccver $(CONFIG_H)
	-@ write sys$output "creating PATHDEF.C file."
	-@ open/write pd $@
	-@ write pd "/* pathdef.c -- DO NOT EDIT! */"
	-@ write pd "/* This file is automatically created by MAKE_VMS.MMS"
	-@ write pd " * Change the file MAKE_VMS.MMS Only. */"
	-@ write pd "typedef unsigned char   char_u;"
	-@ write pd "char_u *default_vim_dir = (char_u *)"$(VIMLOC)";"
	-@ write pd "char_u *default_vimruntime_dir = (char_u *)"$(VIMRUN)";"
	-@ write pd "char_u *all_cflags = (char_u *)""$(CC_DEF)"""
	-@ write pd " ""$(ALL_CFLAGS_VER1)"""
	-@ write pd " ""$(ALL_CFLAGS_VER2)"";"
	-@ write pd "char_u *all_lflags = (char_u *)""$(LD_DEF)$(LDFLAGS)"""
	-@ write pd " ""/exe=$(TARGET) ALL_OBJS_LIST.OPT/OPT $(ALL_LIBS)"";"
	-@ write pd "char_u *compiler_version = (char_u *) ""''CC_VER'"";"
	-@ write pd "char_u *compiled_user = (char_u *) "$(VIMUSER)";"
	-@ write pd "char_u *compiled_sys  = (char_u *) "$(VIMHOST)";"
	-@ write pd "char_u *compiled_arch = (char_u *) ""$(MMSARCH_NAME)"";"
	-@ write pd "char_u *compiled_vers = (char_u *) """ + -
            f$getsyi( "version")+ """;"
	-@ close pd

[.$(DEST)]if_perl.c : if_perl.xs
	-@ $(PERL) PERL_ROOT:[LIB.ExtUtils]xsubpp -prototypes -typemap - PERL_ROOT:[LIB.ExtUtils]typemap if_perl.xs >> $@

make_vms.mms :
	-@ write sys$output "The name of the makefile MUST be <MAKE_VMS.MMS> !!!"
# WHY???  (SMS.)

.IFDEF CCVER
# This part can make some complications if you're using some predefined
# symbols/flags for your compiler. If does, just comment out CCVER variable
check_ccver :
	-@ define /user_mode sys$error nl:
	-@ define /user_mode sys$output [.$(DEST)]cc_ver.tmp
	-@ $(CC_DEF)/version
	-@ open/read file [.$(DEST)]cc_ver.tmp
	-@ read file CC_VER
	-@ close file
	-@ delete/noconfirm/nolog [.$(DEST)]cc_ver.tmp.*
.ELSE
check_ccver :
	-@ !
.ENDIF

.IFDEF MOTIF
motif_env :
.IFDEF XPM
	-@ write sys$output "using DECW/Motif/XPM environment."
        -@ write sys$output "creating OS_VMS_XPM.OPT file."
	-@ open/write opt_file [.$(DEST)]OS_VMS_XPM.OPT
.IFDEF __ALPHA__
ARCH_XPM = axp
.ELSE
ARCH_XPM = $(ARCH)
.ENDIF
	-@ write opt_file "[.xpm.vms.$(ARCH_XPM)]libxpm.olb/lib"
	-@ close opt_file
.ELSE
	-@ write sys$output "using DECW/Motif environment."
.ENDIF
	-@ write sys$output "creating OS_VMS_MOTIF.OPT file."
	-@ open/write opt_file [.$(DEST)]OS_VMS_MOTIF.OPT
	-@ write opt_file "sys$share:decw$xmlibshr12.exe/share"
	-@ write opt_file "sys$share:decw$xtlibshrr5.exe/share"
	-@ write opt_file "sys$share:decw$xlibshr.exe/share"
	-@ close opt_file
.ELSE
motif_env :
	-@ !
.ENDIF


.IFDEF GTK
gtk_env :
	-@ write sys$output "using GTK environment:"
	-@ define/nolog gtk_root /trans=conc $(GTK_DIR)
	-@ show logical gtk_root
	-@ write sys$output "    include path: "$(GUI_INC)""
	-@ write sys$output "creating OS_VMS_GTK.OPT file."
	-@ open/write opt_file [.$(DEST)]OS_VMS_GTK.OPT
	-@ write opt_file "gtk_root:[glib]libglib.exe /share,-"
	-@ write opt_file "gtk_root:[glib.gmodule]libgmodule.exe /share,-"
	-@ write opt_file "gtk_root:[gtk.gdk]libgdk.exe /share,-"
	-@ write opt_file "gtk_root:[gtk.gtk]libgtk.exe /share,-"
	-@ write opt_file "sys$share:decw$xmlibshr12.exe/share,-"
	-@ write opt_file "sys$share:decw$xtlibshrr5.exe/share,-"
	-@ write opt_file "sys$share:decw$xlibshr.exe/share"
	-@ close opt_file
.ELSE
gtk_env :
	-@ !
.ENDIF

.IFDEF VIM_PERL
perl_env :
	-@ write sys$output "using PERL environment:"
	-@ show logical PERLSHR
	-@ write sys$output "    include path: ""$(PERL_INC)"""
	-@ show symbol perl
	-@ open/write pd [.$(DEST)]if_perl.c
	-@ write pd "/* Empty file to satisfy MMK depend.  */"
	-@ write pd "/* It will be overwritten later... */"
	-@ close pd
	-@ write sys$output "creating OS_VMS_PERL.OPT file."
	-@ open/write opt_file [.$(DEST)]OS_VMS_PERL.OPT
	-@ write opt_file "PERLSHR /share"
	-@ close opt_file
.ELSE
perl_env :
	-@ !
.ENDIF

.IFDEF VIM_PYTHON
python_env :
	-@ write sys$output "using PYTHON environment:"
	-@ show logical PYTHON_INCLUDE
	-@ show logical PYTHON_OLB
	-@ write sys$output "creating OS_VMS_PYTHON.OPT file."
	-@ open/write opt_file [.$(DEST)]OS_VMS_PYTHON.OPT
	-@ write opt_file "PYTHON_OLB:PYTHON.OLB /share"
	-@ close opt_file
.ELSE
python_env :
	-@ !
.ENDIF

.IFDEF VIM_PYTHON3
python3_env :
	-@ write sys$output "using PYTHON3 environment:"
	-@ show logical PYTHON3_INCLUDE
	-@ show logical PYTHON3_OLB
	-@ write sys$output "creating OS_VMS_PYTHON3.OPT file."
	-@ open/write opt_file [.$(DEST)]OS_VMS_PYTHON3.OPT
	-@ write opt_file "PYTHON3_OLB:PYTHON3.OLB /share"
	-@ close opt_file
.ELSE
python3_env :
	-@ !
.ENDIF

.IFDEF VIM_TCL
tcl_env :
	-@ write sys$output "using TCL environment:"
	-@ show logical TCLSHR
	-@ write sys$output "    include path: ""$(TCL_INC)"""
	-@ write sys$output "creating OS_VMS_TCL.OPT file."
	-@ open/write opt_file [.$(DEST)]OS_VMS_TCL.OPT
	-@ write opt_file "TCLSHR /share"
	-@ close opt_file
.ELSE
tcl_env :
	-@ !
.ENDIF

.IFDEF VIM_RUBY
ruby_env :
	-@ write sys$output "using RUBY environment:"
	-@ write sys$output "    include path: ""$(RUBY_INC)"""
	-@ write sys$output "creating OS_VMS_RUBY.OPT file."
	-@ open/write opt_file [.$(DEST)]OS_VMS_RUBY.OPT
	-@ write opt_file "RUBYSHR /share"
	-@ close opt_file
.ELSE
ruby_env :
	-@ !
.ENDIF

.IFDEF VIM_LUA
lua_env :
	-@ write sys$output "using LUA environment:"
	-@ write sys$output "    include path: ""$(LUA_INC)"""
	-@ write sys$output "creating OS_VMS_LUA.OPT file."
	-@ open/write opt_file [.$(DEST)]OS_VMS_LUA.OPT
	-@ write opt_file "LUA$ROOT:[LIB]LUA$SHR.EXE /share"
	-@ close opt_file
.ELSE
lua_env :
	-@ !
.ENDIF

[.$(DEST)]alloc.obj : alloc.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]arabic.obj : arabic.c vim.h
[.$(DEST)]arglist.obj : arglist.c vim.h [.$(DEST)]config.h feature.h os_unix.h
[.$(DEST)]autocmd.obj : autocmd.c vim.h [.$(DEST)]config.h feature.h os_unix.h
[.$(DEST)]blowfish.obj : blowfish.c vim.h [.$(DEST)]config.h feature.h os_unix.h
[.$(DEST)]blob.obj : blob.c vim.h [.$(DEST)]config.h feature.h os_unix.h
[.$(DEST)]buffer.obj : buffer.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]bufwrite.obj : bufwrite.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]change.obj : change.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]charset.obj : charset.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]channel.obj : channel.c vim.h [.$(DEST)]config.h feature.h
[.$(DEST)]cindent.obj : cindent.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]clientserver.obj : clientserver.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]clipboard.obj : clipboard.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]cmdexpand.obj : cmdexpand.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]cmdhist.obj : cmdhist.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]crypt.obj : crypt.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h regexp.h gui.h \
 beval.h alloc.h ex_cmds.h spell.h proto.h \
 errors.h globals.h
[.$(DEST)]crypt_zip.obj : crypt_zip.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h \
 regexp.h gui.h beval.h alloc.h ex_cmds.h spell.h \
 proto.h errors.h globals.h
[.$(DEST)]debugger.obj : debugger.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]dict.obj : dict.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h regexp.h gui.h \
 beval.h alloc.h ex_cmds.h spell.h proto.h \
 errors.h globals.h
[.$(DEST)]diff.obj : diff.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]digraph.obj : digraph.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]drawline.obj : drawline.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]drawscreen.obj : drawscreen.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]edit.obj : edit.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]eval.obj : eval.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]evalbuffer.obj : evalbuffer.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h \
 regexp.h gui.h beval.h alloc.h ex_cmds.h spell.h \
 proto.h errors.h globals.h
[.$(DEST)]evalfunc.obj : evalfunc.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h \
 regexp.h gui.h beval.h alloc.h ex_cmds.h spell.h \
 proto.h errors.h globals.h version.h
[.$(DEST)]evalvars.obj : evalvars.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h \
 regexp.h gui.h beval.h alloc.h ex_cmds.h spell.h \
 proto.h errors.h globals.h version.h
[.$(DEST)]evalwindow.obj : evalwindow.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h \
 regexp.h gui.h beval.h alloc.h ex_cmds.h spell.h \
 proto.h errors.h globals.h
[.$(DEST)]ex_cmds.obj : ex_cmds.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]ex_cmds2.obj : ex_cmds2.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]ex_docmd.obj : ex_docmd.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h ex_cmdidxs.h
[.$(DEST)]ex_eval.obj : ex_eval.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]ex_getln.obj : ex_getln.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]fileio.obj : fileio.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]filepath.obj : filepath.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]findfile.obj : findfile.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]float.obj : float.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]fold.obj : fold.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]fuzzy.obj : fuzzy.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]getchar.obj : getchar.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]gc.obj : gc.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]gui_xim.obj : gui_xim.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]hardcopy.obj : hardcopy.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]hashtab.obj : hashtab.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]help.obj : help.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]highlight.obj : highlight.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]if_cscope.obj : if_cscope.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]if_xcmdsrv.obj : if_xcmdsrv.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]if_mzsch.obj : if_mzsch.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h \
 regexp.h gui.h beval.h ex_cmds.h proto.h \
 errors.h globals.h if_mzsch.h
[.$(DEST)]indent.obj : indent.c vim.h [.$(DEST)]config.h feature.h os_unix.h
[.$(DEST)]insexpand.obj : insexpand.c vim.h [.$(DEST)]config.h feature.h os_unix.h
[.$(DEST)]job.obj : job.c vim.h [.$(DEST)]config.h feature.h os_unix.h
[.$(DEST)]json.obj : json.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]linematch.obj : linematch.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]list.obj : list.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h regexp.h gui.h \
 beval.h alloc.h ex_cmds.h spell.h proto.h \
 errors.h globals.h
[.$(DEST)]locale.obj : locale.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h regexp.h gui.h \
 beval.h alloc.h ex_cmds.h spell.h proto.h \
 errors.h globals.h
[.$(DEST)]logfile.obj : logfile.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h regexp.h gui.h \
 beval.h alloc.h ex_cmds.h spell.h proto.h \
 errors.h globals.h
[.$(DEST)]main.obj : main.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h \
 arabic.c
[.$(DEST)]map.obj : map.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]mark.obj : mark.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]match.obj : match.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]memfile.obj : memfile.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]memline.obj : memline.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]menu.obj : menu.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]message.obj : message.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]misc1.obj : misc1.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h \
 version.h
[.$(DEST)]misc2.obj : misc2.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]mouse.obj : mouse.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]move.obj : move.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]mbyte.obj : mbyte.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]normal.obj : normal.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h nv_cmdidxs.h nv_cmds.h
[.$(DEST)]ops.obj : ops.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]option.obj : option.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h optiondefs.h
[.$(DEST)]optionstr.obj : optionstr.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]os_unix.obj : os_unix.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h os_unixx.h
[.$(DEST)]os_vms.obj : os_vms.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h os_unixx.h
[.$(DEST)]pathdef.obj : [.$(DEST)]pathdef.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]popupmenu.obj : popupmenu.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]popupwin.obj : popupwin.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]pty.obj : pty.c vim.h [.$(DEST)]config.h feature.h os_unix.h
[.$(DEST)]profiler.obj : profiler.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]quickfix.obj : quickfix.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]regexp.obj : regexp.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]register.obj : register.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]scriptfile.obj : scriptfile.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]screen.obj : screen.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]search.obj : search.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]session.obj : session.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]sha256.obj : sha256.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h regexp.h gui.h \
 beval.h alloc.h ex_cmds.h spell.h proto.h \
 errors.h globals.h
[.$(DEST)]sign.obj : sign.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h regexp.h gui.h \
 beval.h alloc.h ex_cmds.h spell.h proto.h \
 errors.h globals.h
[.$(DEST)]sound.obj : sound.c vim.h [.$(DEST)]config.h feature.h
[.$(DEST)]spell.obj : spell.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]spellfile.obj : spellfile.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h \
 regexp.h gui.h beval.h alloc.h ex_cmds.h spell.h \
 proto.h errors.h globals.h
[.$(DEST)]spellsuggest.obj : spellsuggest.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h \
 regexp.h gui.h beval.h alloc.h ex_cmds.h spell.h \
 proto.h errors.h globals.h
[.$(DEST)]strings.obj : strings.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h \
 regexp.h gui.h beval.h alloc.h ex_cmds.h spell.h \
 proto.h errors.h globals.h
[.$(DEST)]syntax.obj : syntax.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]tabpanel.obj : tabpanel.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]tag.obj : tag.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]term.obj : term.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]terminal.obj : terminal.c vim.h [.$(DEST)]config.h feature.h os_unix.h
[.$(DEST)]termlib.obj : termlib.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]testing.obj : testing.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]textformat.obj : textformat.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]textobject.obj : textobject.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]textprop.obj : textprop.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]time.obj : time.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]tuple.obj : tuple.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]typval.obj : typval.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]ui.obj : ui.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]undo.obj : undo.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]usercmd.obj : usercmd.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h \
 regexp.h gui.h beval.h alloc.h ex_cmds.h spell.h \
 proto.h errors.h globals.h
[.$(DEST)]userfunc.obj : userfunc.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h option.h structs.h \
 regexp.h gui.h beval.h alloc.h ex_cmds.h spell.h \
 proto.h errors.h globals.h
[.$(DEST)]version.obj : version.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]viminfo.obj : viminfo.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]vim9class.obj : vim9class.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]vim9cmds.obj : vim9cmds.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]vim9compile.obj : vim9compile.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]vim9execute.obj : vim9execute.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]vim9expr.obj : vim9expr.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]vim9instr.obj : vim9instr.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]vim9generics.obj : vim9generics.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]vim9script.obj : vim9script.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]vim9type.obj : vim9type.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]window.obj : window.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]gui.obj : gui.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]gui_gtk.obj : gui_gtk.c gui_gtk_f.h vim.h [.$(DEST)]config.h feature.h \
 os_unix.h   ascii.h keymap.h termdefs.h macros.h structs.h \
 regexp.h gui.h beval.h option.h ex_cmds.h \
 proto.h errors.h globals.h [-.pixmaps]stock_icons.h
[.$(DEST)]gui_gtk_f.obj : gui_gtk_f.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h gui_gtk_f.h
[.$(DEST)]gui_motif.obj : gui_motif.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h [-.pixmaps]alert.xpm [-.pixmaps]error.xpm \
 [-.pixmaps]generic.xpm [-.pixmaps]info.xpm [-.pixmaps]quest.xpm
[.$(DEST)]gui_athena.obj : gui_athena.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h gui_at_sb.h
[.$(DEST)]gui_gtk_x11.obj : gui_gtk_x11.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h gui_gtk_f.h [-.runtime]vim32x32.xpm \
 [-.runtime]vim16x16.xpm [-.runtime]vim48x48.xpm version.h
[.$(DEST)]gui_x11.obj : gui_x11.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h [-.runtime]vim32x32.xpm \
 [-.runtime]vim16x16.xpm [-.runtime]vim48x48.xpm [-.pixmaps]tb_new.xpm \
 [-.pixmaps]tb_open.xpm [-.pixmaps]tb_close.xpm [-.pixmaps]tb_save.xpm \
 [-.pixmaps]tb_print.xpm [-.pixmaps]tb_cut.xpm [-.pixmaps]tb_copy.xpm \
 [-.pixmaps]tb_paste.xpm [-.pixmaps]tb_find.xpm \
 [-.pixmaps]tb_find_next.xpm [-.pixmaps]tb_find_prev.xpm \
 [-.pixmaps]tb_find_help.xpm [-.pixmaps]tb_exit.xpm \
 [-.pixmaps]tb_undo.xpm [-.pixmaps]tb_redo.xpm [-.pixmaps]tb_help.xpm \
 [-.pixmaps]tb_macro.xpm [-.pixmaps]tb_make.xpm \
 [-.pixmaps]tb_save_all.xpm [-.pixmaps]tb_jump.xpm \
 [-.pixmaps]tb_ctags.xpm [-.pixmaps]tb_load_session.xpm \
 [-.pixmaps]tb_save_session.xpm [-.pixmaps]tb_new_session.xpm \
 [-.pixmaps]tb_blank.xpm [-.pixmaps]tb_maximize.xpm \
 [-.pixmaps]tb_split.xpm [-.pixmaps]tb_minimize.xpm \
 [-.pixmaps]tb_shell.xpm [-.pixmaps]tb_replace.xpm \
 [-.pixmaps]tb_vsplit.xpm [-.pixmaps]tb_maxwidth.xpm \
 [-.pixmaps]tb_minwidth.xpm
[.$(DEST)]gui_at_sb.obj : gui_at_sb.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h gui_at_sb.h
[.$(DEST)]gui_at_fs.obj : gui_at_fs.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h gui_at_sb.h
[.$(DEST)]pty.obj : pty.c vim.h [.$(DEST)]config.h feature.h os_unix.h   \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h gui.h beval.h \
 option.h ex_cmds.h proto.h errors.h globals.h
[.$(DEST)]if_perl.obj : [.auto]if_perl.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]if_python.obj : if_python.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]if_tcl.obj : if_tcl.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]if_ruby.obj : if_ruby.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]if_lua.obj : if_lua.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 errors.h globals.h version.h
[.$(DEST)]beval.obj : beval.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]gui_beval.obj : gui_beval.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h
[.$(DEST)]netbeans.obj : netbeans.c vim.h [.$(DEST)]config.h feature.h os_unix.h \
 ascii.h keymap.h termdefs.h macros.h structs.h regexp.h \
 gui.h beval.h option.h ex_cmds.h proto.h \
 errors.h globals.h version.h
[.$(DEST)]gui_xmdlg.obj : gui_xmdlg.c [.$(DEST)]config.h vim.h feature.h os_unix.h
[.$(DEST)]gui_xmebw.obj : gui_xmebw.c [.$(DEST)]config.h vim.h feature.h os_unix.h
[.$(DEST)]xdiffi.obj : [.xdiff]xdiffi.c [.xdiff]xinclude.h [.$(DEST)]config.h vim.h feature.h os_unix.h
[.$(DEST)]xemit.obj : [.xdiff]xemit.c [.xdiff]xinclude.h [.$(DEST)]config.h vim.h feature.h os_unix.h
[.$(DEST)]xprepare.obj : [.xdiff]xprepare.c [.xdiff]xinclude.h [.$(DEST)]config.h vim.h feature.h os_unix.h
[.$(DEST)]xutils.obj : [.xdiff]xutils.c [.xdiff]xinclude.h [.$(DEST)]config.h vim.h feature.h os_unix.h
[.$(DEST)]xhistogram.obj : [.xdiff]xhistogram.c [.xdiff]xinclude.h [.$(DEST)]config.h vim.h feature.h os_unix.h
[.$(DEST)]xpatience.obj : [.xdiff]xpatience.c [.xdiff]xinclude.h [.$(DEST)]config.h vim.h feature.h os_unix.h
