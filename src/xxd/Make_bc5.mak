# The most simplistic Makefile for Win32 (NT and Windows 95).
# Used for Borland C++.

!if ("$(BOR)"=="")
BOR = c:\bc5
!endif
!if ("$(BCC)"=="")
BCC = bcc32
!endif

xxd: xxd.exe

xxd.exe: xxd.c
	$(BCC) -I$(BOR)\include -L$(BOR)\lib -DWIN32 xxd.c $(BOR)\lib\wildargs.obj

clean:
	- del xxd.obj
	- del xxd.exe
