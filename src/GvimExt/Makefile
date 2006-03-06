# Makefile for GvimExt, using MSVC
# Options:
#   DEBUG=yes		Build debug version (for VC7 and maybe later)
#

TARGETOS=BOTH
APPVER=4.0

!if "$(DEBUG)" != "yes"
NODEBUG = 1
!endif

!include <win32.mak>

all: gvimext.dll

gvimext.dll:    gvimext.obj	\
		gvimext.res
# $(implib) /NOLOGO -machine:$(CPU) -def:gvimext.def $** -out:gvimext.lib
# $(link) $(dlllflags) -base:0x1C000000 -out:$*.dll $** $(olelibsdll) shell32.lib gvimext.lib comctl32.lib gvimext.exp
  $(link) $(lflags) -dll -def:gvimext.def -base:0x1C000000 -out:$*.dll $** $(olelibsdll) shell32.lib comctl32.lib

gvimext.obj: gvimext.h

.cpp.obj:
    $(cc) $(cflags) -DFEAT_GETTEXT $(cvarsdll) $*.cpp

gvimext.res: gvimext.rc
    $(rc) $(rcflags) $(rcvars)  gvimext.rc

clean:
    - if exist gvimext.dll del gvimext.dll
    - if exist gvimext.lib del gvimext.lib
    - if exist gvimext.exp del gvimext.exp
    - if exist gvimext.obj del gvimext.obj
    - if exist gvimext.res del gvimext.res
