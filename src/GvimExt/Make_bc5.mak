### USEDLL  no for statically linked version of run-time, yes for DLL runtime
### BOR	    path to root of Borland C install (c:\bc5)

### (requires cc3250.dll be available in %PATH%)
!if ("$(USEDLL)"=="")
USEDLL = no
!endif

### BOR: root of the BC installation
!if ("$(BOR)"=="")
BOR = c:\bc5
!endif

CC	= $(BOR)\bin\Bcc32
BRC	= $(BOR)\bin\brc32
LINK	= $(BOR)\BIN\ILink32
INCLUDE = $(BOR)\include;.
LIB	= $(BOR)\lib

!if ("$(USEDLL)"=="yes")
RT_DEF = -D_RTLDLL
RT_LIB = cw32i.lib
!else
RT_DEF =
RT_LIB = cw32.lib
!endif


all : gvimext.dll

gvimext.obj : gvimext.cpp gvimext.h
	$(CC) -tWD -I$(INCLUDE) -c -DFEAT_GETTEXT $(RT_DEF) -w- gvimext.cpp

gvimext.res : gvimext.rc
	$(BRC) -r gvimext.rc

gvimext.dll : gvimext.obj gvimext.res
	$(LINK) -L$(LIB) -aa gvimext.obj, gvimext.dll, , c0d32.obj $(RT_LIB) import32.lib, gvimext.def, gvimext.res

clean :
	-@del gvimext.obj
	-@del gvimext.res
	-@del gvimext.dll
