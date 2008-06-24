#
# Makefile for VIM on OS/2 using EMX	vim:ts=8:sw=8:tw=78
#
# Created by: Paul Slootman
#

### This Makefile has been successfully tested on these systems.
### Check the (*) column for remarks, listed below.
### Later code changes may cause small problems, otherwise Vim is supposed to
### compile and run without problems.
### Just to show that this is just like the Unix version!

#system:	      configurations:		     version (*) tested by:
#-------------	      ------------------------	     -------  -  ----------
#OS/2 Warp HPFS       gcc-2.7.2+emx-0.9b -GUI		4.5	 Paul Slootman
#OS/2 FAT	      gcc-2.6.3+emx	 -GUI		4.5	 Karsten Sievert

#>>>>> choose options:

### See feature.h for a list of optionals.
### Any other defines can be included here.

DEFINES = -DUSE_SYSTEM=1

#>>>>> name of the compiler and linker, name of lib directory
CC = gcc

#>>>>> end of choices

### Name of target(s)
TARGET = vim.exe

### Names of the tools that are also made
TOOLS = xxd/xxd.exe tee/tee.exe

###########################################################################

INCL = vim.h globals.h option.h keymap.h macros.h ascii.h term.h os_unix.h structs.h os_os2_cfg.h
CFLAGS = -O2 -fno-strength-reduce -DOS2 -Wall -Iproto $(DEFINES)

OBJ = \
	buffer.o \
	charset.o \
	diff.o \
	digraph.o \
	edit.o \
	eval.o \
	ex_cmds.o \
	ex_cmds2.o \
	ex_docmd.o \
	ex_eval.o \
	ex_getln.o \
	fileio.o \
	fold.o \
	getchar.o \
	hardcopy.o \
	hashtab.o \
	main.o \
	mark.o \
	memfile.o \
	memline.o \
	menu.o \
	message.o \
	misc1.o \
	misc2.o \
	move.o \
	mbyte.o \
	normal.o \
	ops.o \
	option.o \
	popupmnu.o \
	quickfix.o \
	regexp.o \
	screen.o \
	search.o \
	spell.o \
	syntax.o \
	tag.o \
	term.o \
	ui.o \
	undo.o \
	window.o \
	os_unix.o

LIBS = -ltermcap

# Default target is making the executable
all: $(TARGET) $(TOOLS)

# Link the target for normal use
LFLAGS = -Zcrtdll -s -o $(TARGET) $(LIBS)

$(TARGET): $(OBJ) version.c version.h
	$(CC) $(CFLAGS) version.c $(OBJ) $(LFLAGS)

xxd/xxd.exe: xxd/xxd.c
	cd xxd & $(MAKE) -f Make_os2.mak

tee/tee.exe: tee/tee.c
	cd tee & $(MAKE) -f Makefile

test:
	cd testdir & $(MAKE) -f Make_os2.mak

clean:
	-del *.o
	-del *.exe
	-del *.~ *~ *.bak
	cd xxd   & $(MAKE) -f Make_os2.mak clean
	cd tee   & $(MAKE) -f Makefile clean

###########################################################################

os_unix.o:	os_unix.c  $(INCL)
buffer.o:	buffer.c  $(INCL)
charset.o:	charset.c  $(INCL)
diff.o:		diff.c  $(INCL)
digraph.o:	digraph.c  $(INCL)
edit.o:		edit.c  $(INCL)
eval.o:		eval.c  $(INCL)
ex_cmds.o:	ex_cmds.c  $(INCL)
ex_cmds2.o:	ex_cmds2.c  $(INCL)
ex_docmd.o:	ex_docmd.c  $(INCL) ex_cmds.h
ex_eval.o:	ex_eval.c  $(INCL) ex_cmds.h
ex_getln.o:	ex_getln.c  $(INCL)
fileio.o:	fileio.c  $(INCL)
fold.o:		fold.c  $(INCL)
getchar.o:	getchar.c  $(INCL)
hardcopy.o:	hardcopy.c  $(INCL)
hashtab.o:	hashtab.c  $(INCL)
main.o:		main.c  $(INCL)
mark.o:		mark.c  $(INCL)
memfile.o:	memfile.c  $(INCL)
memline.o:	memline.c  $(INCL)
menu.o:		menu.c  $(INCL)
message.o:	message.c  $(INCL)
misc1.o:	misc1.c  $(INCL)
misc2.o:	misc2.c  $(INCL)
move.o:		move.c  $(INCL)
mbyte.o:	mbyte.c  $(INCL)
normal.o:	normal.c  $(INCL)
ops.o:		ops.c  $(INCL)
option.o:	option.c  $(INCL)
popupmnu.o:	popupmnu.c  $(INCL)
quickfix.o:	quickfix.c  $(INCL)
regexp.o:	regexp.c  $(INCL)
screen.o:	screen.c  $(INCL)
search.o:	search.c  $(INCL)
spell.o:	spell.c  $(INCL)
syntax.o:	syntax.c  $(INCL)
tag.o:		tag.c  $(INCL)
term.o:		term.c  $(INCL)
ui.o:		ui.c  $(INCL)
undo.o:		undo.c  $(INCL)
window.o:	window.c  $(INCL)
