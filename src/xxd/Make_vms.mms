# VMS MM[KS] makefile for XXD
# tested with MMK and MMS as well.
#
# Maintained by Zoltan Arpadffy <zoltan.arpadffy@gmail.com>
#               2025-05-24  Steven M. Schweda <sms@antinode.info>
#
######################################################################
#
# Edit the lines in the Configuration section below to select.
#
# To build: use the following command line:
#
#	mms/descrip=Make_vms.mms
#	  or if you use mmk
#	mmk/descrip=Make_vms.mms
#
# To cleanup: mms/descrip=Make_vms.mms clean 
#
######################################################################
# Configuration section.
######################################################################
# Uncomment if want a debug version. Resulting executable is DVIM.EXE
######################################################################
# DEBUG = YES

######################################################################
# End of configuration section.
#
# Please, do not change anything below without programming experience.
######################################################################

# Define old MMK architecture macros when using MMS.
#
######################################################################
# Architecture identification and product destination selection.
# Define old MMK architecture macros when using MMS.
#
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
#
# Combine command-line VAX C compiler macros.
#
.IFDEF VAXC                     # VAXC
VAXC_OR_FORCE_VAXC = 1
.ELSE                           # VAXC
.IFDEF FORCE_VAXC                   # FORCE_VAXC
VAXC_OR_FORCE_VAXC = 1
.ENDIF                              # FORCE_VAXC
.ENDIF                          # VAXC
#
# Analyze architecture-related and option macros.
# (Sense x86_64 before IA64 for old MMK and x86_64 cross tools.)
#
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
#
# Create destination directory.
	@ write sys$output "Destination: [.$(DEST)]"
	@ write sys$output ""
	@ if (f$search( "$(DEST).DIR;1") .eqs. "") then -
         create /directory [.$(DEST)]
#
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
PREFIX = /prefix=all
.ELSE                               # DECC
.IFDEF FORCE_VAXC                       # FORCE_VAXC
CC_DEF = cc /vaxc
.ELSE                                   # FORCE_VAXC
CC_DEF = cc
.ENDIF                                  # FORCE_VAXC
.ENDIF                              # DECC
.ELSE                           # __VAX__

# Not VAX, therefore DEC C (/PREFIX).

CC_DEF = cc /decc
PREFIX  = /prefix=all

# These floating-point options are the defaults on IA64 and x86_64.
# This makes Alpha consistent.
FLOAT   = /float = ieee_float /ieee_mode = denorm_results

# Large-file support.  Unavailable on VAX and very old Alpha.  To
# disable, define NOLARGE.
.IFDEF NOLARGE
.ELSE
LARGE_DEF = , "_LARGEFILE"
.ENDIF # NOLARGE [ELSE]
.ENDIF                          # __VAX__ [ELSE]

.IFDEF VAXC_OR_FORCE_VAXC       # VAXC_OR_FORCE_VAXC
.ELSE                           # VAXC_OR_FORCE_VAXC
CCVER = YES     # Unreliable with VAX C.
.ENDIF                          # VAXC_OR_FORCE_VAXC [ELSE]

CDEFS = VMS $(LARGE_DEF)
DEFS = /define = ($(CDEFS))

.IFDEF LIST                     # LIST
LIST_OPT = /list=[.$(DEST)] /show=(all, nomessages)
.ENDIF                          # LIST

.IFDEF DEBUG                    # DEBUG
TARGET  =  [.$(DEST)]dxxd.exe
CFLAGS  = /debug/noopt$(PREFIX) $(LIST_OPT) /cross_reference/include=[]
LDFLAGS = /debug
.ELSE                           # DEBUG
TARGET  =  [.$(DEST)]xxd.exe
CFLAGS  = $(OPTIMIZE) $(PREFIX) $(LIST_OPT) /include=[]

LDFLAGS =
.ENDIF                          # DEBUG [ELSE]

CC = $(CC_DEF) $(CFLAGS)

LD_DEF  = link

.SUFFIXES : .obj .c

SOURCES	= xxd.c
OBJ_BASE = xxd.obj
OBJ     = [.$(DEST)]$(OBJ_BASE)

.c.obj :
	$(CC) $(DEFS) $< /object = $@

$(TARGET) : $(OBJ)
        -@ def_dev_dir_orig = f$environment( "default")
	-@ target_name_type = -
         f$parse( "$(TARGET)", , , "NAME", "SYNTAX_ONLY")+ -
         f$parse( "$(TARGET)", , , "TYPE", "SYNTAX_ONLY")
	-@ set default [.$(DEST)]
	$(LD_DEF) $(LDFLAGS) /exe = 'target_name_type' $(OBJ_BASE)
	-@ set default 'def_dev_dir_orig'

clean :
	-@ if (f$search( "[.$(DEST)]*.*") .nes. "") then -
         delete /noconfirm [.$(DEST)]*.*;*
	-@ if (f$search( "$(DEST).DIR") .nes. "") then -
         set protection = w:d $(DEST).DIR;*
	-@ if (f$search( "$(DEST).DIR") .nes. "") then -
         delete /noconfirm $(DEST).DIR;*

help :
        mcr sys$disk:$(TARGET) -h

version :
        mcr sys$disk:$(TARGET) -v


[.$(DEST)]xxd.obj : xxd.c
