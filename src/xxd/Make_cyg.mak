# The most simplistic Makefile, for Cygnus gcc on MS-DOS

ifndef USEDLL
USEDLL = no
endif

ifeq (yes, $(USEDLL))
DEFINES =
LIBS    = -lc
else
DEFINES = -mno-cygwin
LIBS    =
endif

CFLAGS = -O2 -Wall -DWIN32 $(DEFINES)

ifneq (sh.exe, $(SHELL))
DEL = rm
else
DEL = del
endif

xxd.exe: xxd.c
	gcc $(CFLAGS) -s -o xxd.exe xxd.c $(LIBS)

clean:
	-$(DEL) xxd.exe
