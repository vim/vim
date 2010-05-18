# Makefile for Borland C++ 3.1 or 4.0 to compile a 16 bit version of Vim.
#
# There are compilation options at the end of this file.
#
# Command line variables:
# BOR		path to root of Borland C (E:\BORLANDC)
# DEBUG		set to "yes" for debugging (no)
# SPAWNO	path to the spawno library directory, empty if you do not have
#		it; use 8.3 filenames! (C:\CC\SPAWN)

.AUTODEPEND

!ifndef BOR
BOR = E:\BORLANDC
!endif

!if ("$(DEBUG)" == "yes")
DEBUG_FLAG = -v
!else
DEBUG_FLAG =
!endif

CC = $(BOR)\bin\bcc.exe +VIM.CFG
TLINK = $(BOR)\bin\tlink.exe

!ifndef SPAWNO
SPAWNO = C:\CC\SPAWN
!endif

!if ("$(SPAWNO)" == "")
LIBPATH = $(BOR)\LIB
INCLUDEPATH = $(BOR)\INCLUDE
SPAWND =
SPAWNL =
!else
LIBPATH = $(BOR)\LIB;$(SPAWNO)
INCLUDEPATH = $(BOR)\INCLUDE;$(SPAWNO)
SPAWND = ;SPAWNO
SPAWNL = spawnl.lib
!endif


#		*Implicit Rules*
#
# use -v for debugging
#
.c.obj:
	$(CC) -c $(DEBUG_FLAG) {$< }

#		*List Macros*


EXE_dependencies = \
	blowfish.obj \
	buffer.obj \
	charset.obj \
	diff.obj \
	digraph.obj \
	edit.obj \
	eval.obj \
	ex_cmds.obj \
	ex_cmds2.obj \
	ex_docmd.obj \
	ex_eval.obj \
	ex_getln.obj \
	fileio.obj \
	fold.obj \
	getchar.obj \
	hardcopy.obj \
	hashtab.obj \
	main.obj \
	mark.obj \
	memfile.obj \
	memline.obj \
	menu.obj \
	message.obj \
	misc1.obj \
	misc2.obj \
	move.obj \
	os_msdos.obj \
	normal.obj \
	ops.obj \
	option.obj \
	popupmnu.obj \
	quickfix.obj \
	regexp.obj \
	screen.obj \
	search.obj \
	sha256.obj \
	spell.obj \
	syntax.obj \
	tag.obj \
	term.obj \
	ui.obj \
	undo.obj \
	window.obj

all: vim.exe install.exe uninstal.exe xxd/xxd.exe

#		*Explicit Rules*

vim.exe: vim.cfg $(EXE_dependencies) version.c
	$(CC) $(DEBUG_FLAG) -c version.c
	$(TLINK) /x/c/L$(LIBPATH) $(DEBUG_FLAG) @&&|
c0l.obj $(EXE_dependencies) version.obj
vim
		# no map file
$(SPAWNL) cl.lib
|

install.exe: dosinst.c
	$(CC) -einstall $(DEBUG_FLAG) dosinst.c

uninstal.exe: uninstal.c
	$(CC) $(DEBUG_FLAG) uninstal.c

# This may fail for older make versions, building xxd will fail anyway then.
xxd/xxd.exe: xxd/xxd.c
	cd xxd
	$(MAKE) -f Make_bc3.mak BOR=$(BOR) DEBUG=$(DEBUG)
	cd ..

# cleaning up: Delete all generated files
clean:
	-del *.obj
	-del vim.exe
	-del vim.sym
	-del install.exe
	-del uninstal.exe
	-del xxd\*.obj
	-del xxd\xxd.exe
	-del vim.cfg
	-del testdir\*.out

# Individual File Dependencies (incomplete)
ex_docmd.obj: ex_docmd.c ex_cmds.h

ex_eval.obj: ex_eval.c ex_cmds.h

main.obj: main.c globals.h option.h

term.obj: term.c term.h

version.obj: version.c version.h


# Compiler Configuration File
#
# The following compile options can be changed for better machines.
#	replace -1- with -2 to produce code for a 80286 or higher
#	replace -1- with -3 to produce code for a 80386 or higher
#	add -v for source debugging
vim.cfg: Make_bc3.mak
	copy &&|
-ml
-1-
-f-
-C
-N
-O
-Z
-k-
-d
-h
-vi-
-H=VIM.SYM
-w-par
-weas
-wpre
-Iproto
-I$(INCLUDEPATH)
-L$(LIBPATH)
-DMSDOS;FEAT_TINY$(SPAWND)
| vim.cfg

test:
	cd testdir
	$(MAKE) -f Make_dos.mak small
	cd ..
