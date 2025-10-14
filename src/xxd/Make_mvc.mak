# The most simplistic Makefile for Win32 using Microsoft Visual C++
# (NT and Windows 95)

# included common tools
!INCLUDE ..\auto\nmake\tools.mak

SUBSYSTEM = console
!IF "$(SUBSYSTEM_VER)" != ""
SUBSYSTEM = $(SUBSYSTEM),$(SUBSYSTEM_VER)
!ENDIF

xxd: xxd.exe

xxd.exe: xxd.c
	cl /nologo /source-charset:utf-8 -DWIN32 xxd.c -link \
		-subsystem:$(SUBSYSTEM)

# This was for an older compiler
#    cl /nologo -DWIN32 xxd.c /link setargv.obj

clean:
	- if exist xxd.obj $(RM) xxd.obj
	- if exist xxd.exe $(RM) xxd.exe

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=79 ft=make:
