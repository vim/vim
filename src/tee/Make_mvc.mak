# A very (if not the most) simplistic Makefile for MSVC

# included common tools
!INCLUDE ..\auto\nmake\tools.mak

SUBSYSTEM = console
!IF "$(SUBSYSTEM_VER)" != ""
SUBSYSTEM = $(SUBSYSTEM),$(SUBSYSTEM_VER)
!ENDIF

CC=cl
CFLAGS=/O2 /nologo

tee.exe: tee.obj
	$(CC) $(CFLAGS) /Fo$@ $** /link /subsystem:$(SUBSYSTEM)

tee.obj: tee.c
	$(CC) $(CFLAGS) /c $**

clean:
	- $(RM) tee.obj
	- $(RM) tee.exe

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=79 ft=make:
