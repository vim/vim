# The most simplistic Makefile for Win32 using Microsoft Visual C++
# (NT and Windows 95)

xxd: xxd.exe

xxd.exe: xxd.c
     cl /nologo -DWIN32 xxd.c /link setargv.obj

clean:
     - if exist xxd.obj del xxd.obj
     - if exist xxd.exe del xxd.exe
