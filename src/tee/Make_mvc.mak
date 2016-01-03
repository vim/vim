# A very (if not the most) simplistic Makefile for MSVC

CC=cl
CFLAGS=/O2

tee.exe: tee.obj
	$(CC) $(CFLAGS) /Fo$@ $**

tee.obj: tee.c
	$(CC) $(CFLAGS) /c $**

clean:
	- del tee.obj
	- del tee.exe
