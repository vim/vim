# Makefile for GvimExt, using MSVC
# Options:
#   DEBUG=yes		Build debug version (for VC7 and maybe later)
#   CPUARG=		/arch:IA32/AVX/etc, call from main makefile to set
#   			automatically from CPUNR
#

# included common tools
!INCLUDE ..\auto\nmake\tools.mak

TARGETOS = WINNT

!IFNDEF APPVER
APPVER = 6.01
!ENDIF
# Set the default $(WINVER) to make it work with Windows 7.
!IFNDEF WINVER
WINVER = 0x0601
!ENDIF

!IF "$(DEBUG)" != "yes"
NODEBUG = 1
!ENDIF

!IFNDEF CPU
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
! ENDIF
!ENDIF

!IFDEF SDK_INCLUDE_DIR
! INCLUDE $(SDK_INCLUDE_DIR)\Win32.mak
!ELSEIF "$(USE_WIN32MAK)"=="yes"
! INCLUDE <Win32.mak>
!ELSE
cc = cl
link = link
rc = rc
cflags = -nologo -c
lflags = -incremental:no -nologo
rcflags = /r
olelibsdll = ole32.lib uuid.lib oleaut32.lib user32.lib gdi32.lib advapi32.lib
!ENDIF

# include CPUARG
cflags = $(cflags) $(CPUARG)

# set WINVER and _WIN32_WINNT
cflags = $(cflags) -DWINVER=$(WINVER) -D_WIN32_WINNT=$(WINVER)

!IF "$(CL)" == "/D_USING_V110_SDK71_"
rcflags = $(rcflags) /D_USING_V110_SDK71_
!ENDIF

SUBSYSTEM = console
!IF "$(SUBSYSTEM_VER)" != ""
SUBSYSTEM = $(SUBSYSTEM),$(SUBSYSTEM_VER)
!ENDIF

!IF "$(CPU)" == "AMD64" || "$(CPU)" == "ARM64"
OFFSET = 0x11C000000
!ELSE
OFFSET = 0x1C000000
!ENDIF

all: gvimext.dll

gvimext.dll: gvimext.obj gvimext.res
	$(link) $(lflags) -dll -def:gvimext.def -base:$(OFFSET) \
		-out:$*.dll $** $(olelibsdll) shell32.lib comctl32.lib \
		-subsystem:$(SUBSYSTEM)

gvimext.obj: gvimext.h

.cpp.obj:
	$(cc) $(cflags) -DFEAT_GETTEXT $(cvarsmt) $*.cpp

gvimext.res: gvimext.rc
	$(rc) /nologo $(rcflags) $(rcvars) gvimext.rc

clean:
	- if exist gvimext.dll $(RM) gvimext.dll
	- if exist gvimext.lib $(RM) gvimext.lib
	- if exist gvimext.exp $(RM) gvimext.exp
	- if exist gvimext.obj $(RM) gvimext.obj
	- if exist gvimext.res $(RM) gvimext.res

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=79 ft=make:
