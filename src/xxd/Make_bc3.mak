# Simple makefile for Borland C++ 4.0
# 3.1 can NOT be used, it has problems with the fileno() define.

# Command line variables:
# BOR		path to root of Borland C (E:\BORLANDC)
# DEBUG		set to "yes" for debugging (no)

!ifndef BOR
BOR	= e:\bc4
!endif

!if ("$(DEBUG)" == "yes")
DEBUG_FLAG = -v -DDEBUG
!else
DEBUG_FLAG =
!endif

CC	= $(BOR)\bin\bcc
INC	= -I$(BOR)\include
LIB	= -L$(BOR)\lib

# The following compile options can be changed for better machines.
#	replace -1- with -2 to produce code for a 80286 or higher
#	replace -1- with -3 to produce code for a 80386 or higher
#	add -v for source debugging
OPTIMIZE= -1- -Ox

CFLAGS	= -A -mc -DMSDOS $(DEBUG_FLAG) $(OPTIMIZE) $(INC) $(LIB)

xxd.exe: xxd.c
	$(CC) $(CFLAGS) xxd.c
