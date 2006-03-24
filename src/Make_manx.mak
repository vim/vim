#
# Makefile for VIM on the Amiga, using Aztec/Manx C 5.0 or later
#
# Note: Not all dependencies are included. This was done to avoid having
#	to compile everything when a global variable or function is added.
#	Careful when changing a global struct or variable!
#

#>>>>> choose options:

### See feature.h for a list of optionals.
### Any other defines can be included here.
DEFINES =

#>>>>> if HAVE_TGETENT is defined obj/termlib.o has to be used
#TERMLIB = obj/termlib.o
TERMLIB =

#>>>>> choose between debugging (-bs) or optimizing (-so)
OPTIONS = -so
#OPTIONS = -bs

#>>>>> end of choices
###########################################################################

CFLAGS = $(OPTIONS) -wapruq -ps -qf -Iproto $(DEFINES) -DAMIGA

LIBS = -lc16
SYMS = vim.syms
CC = cc
LN = ln
LNFLAGS = +q
SHELL = csh
REN = $(SHELL) -c mv -f
DEL = $(SHELL) -c rm -f

SRC =	buffer.c \
	charset.c \
	diff.c \
	digraph.c \
	edit.c \
	eval.c \
	ex_cmds.c \
	ex_cmds2.c \
	ex_docmd.c \
	ex_eval.c \
	ex_getln.c \
	fileio.c \
	fold.c \
	getchar.c \
	hardcopy.c \
	hashtab.c \
	main.c \
	mark.c \
	memfile.c \
	memline.c \
	menu.c \
	message.c \
	misc1.c \
	misc2.c \
	move.c \
	mbyte.c \
	normal.c \
	ops.c \
	option.c \
	os_amiga.c \
	popupmnu.c \
	quickfix.c \
	regexp.c \
	screen.c \
	search.c \
	spell.c \
	syntax.c \
	tag.c \
	term.c \
	ui.c \
	undo.c \
	window.c \
	version.c

INCL = vim.h feature.h keymap.h macros.h ascii.h term.h structs.h os_amiga.h

OBJ =	obj/buffer.o \
	obj/charset.o \
	obj/diff.o \
	obj/digraph.o \
	obj/edit.o \
	obj/eval.o \
	obj/ex_cmds.o \
	obj/ex_cmds2.o \
	obj/ex_docmd.o \
	obj/ex_eval.o \
	obj/ex_getln.o \
	obj/fileio.o \
	obj/fold.o \
	obj/getchar.o \
	obj/hardcopy.o \
	obj/hashtab.o \
	obj/main.o \
	obj/mark.o \
	obj/memfile.o \
	obj/memline.o \
	obj/menu.o \
	obj/message.o \
	obj/misc1.o \
	obj/misc2.o \
	obj/move.o \
	obj/mbyte.o \
	obj/normal.o \
	obj/ops.o \
	obj/option.o \
	obj/os_amiga.o \
	obj/popupmnu.o \
	obj/quickfix.o \
	obj/regexp.o \
	obj/screen.o \
	obj/search.o \
	obj/spell.o \
	obj/syntax.o \
	obj/tag.o \
	obj/term.o \
	obj/ui.o \
	obj/undo.o \
	obj/window.o \
	$(TERMLIB)

PRO =	proto/buffer.pro \
	proto/charset.pro \
	proto/diff.pro \
	proto/digraph.pro \
	proto/edit.pro \
	proto/eval.pro \
	proto/ex_cmds.pro \
	proto/ex_cmds2.pro \
	proto/ex_docmd.pro \
	proto/ex_eval.pro \
	proto/ex_getln.pro \
	proto/fileio.pro \
	proto/fold.pro \
	proto/getchar.pro \
	proto/hardcopy.pro \
	proto/hashtab.pro \
	proto/main.pro \
	proto/mark.pro \
	proto/memfile.pro \
	proto/memline.pro \
	proto/menu.pro \
	proto/message.pro \
	proto/misc1.pro \
	proto/misc2.pro \
	proto/move.pro \
	proto/mbyte.pro \
	proto/normal.pro \
	proto/ops.pro \
	proto/option.pro \
	proto/os_amiga.pro \
	proto/popupmnu.pro \
	proto/quickfix.pro \
	proto/regexp.pro \
	proto/screen.pro \
	proto/search.pro \
	proto/spell.pro \
	proto/syntax.pro \
	proto/tag.pro \
	proto/term.pro \
	proto/termlib.pro \
	proto/ui.pro \
	proto/undo.pro \
	proto/window.pro

all: Vim xxd/Xxd

Vim: obj $(OBJ) version.c version.h
	$(CC) $(CFLAGS) version.c -o obj/version.o
	$(LN) $(LNFLAGS) -m -o Vim $(OBJ) obj/version.o $(LIBS)

debug: obj $(OBJ) version.c version.h
	$(CC) $(CFLAGS) version.c -o obj/version.o
	$(LN) $(LNFLAGS) -m -g -o Vim $(OBJ) obj/version.o $(LIBS)

xxd/Xxd: xxd/xxd.c
	$(SHELL) -c cd xxd; make -f Make_amiga.mak; cd ..

# Making prototypes with Manx has been removed, because it caused too many
# problems.
#proto: $(SYMS) $(PRO)

obj:
	makedir obj

tags: $(SRC) $(INCL)
	$(SHELL) -c ctags $(SRC) *.h

# can't use delete here, too many file names
clean:
	$(DEL) $(OBJ) obj/version.o \
		obj/termlib.o Vim $(SYMS) xxd/Xxd

test:
	$(SHELL) -c cd testdir; make -f Make_amiga.mak; cd ..

$(SYMS): $(INCL) $(PRO)
	$(CC) $(CFLAGS) -ho$(SYMS) vim.h

###########################################################################

# Unfortunately, Manx's make doesn't understand a .c.o rule, so each
# compilation command has to be given explicitly.

CCSYM = $(CC) $(CFLAGS) -hi$(SYMS) -o
CCNOSYM = $(CC) $(CFLAGS) -o

$(OBJ): $(SYMS)

obj/buffer.o:	buffer.c
	$(CCSYM) $@ buffer.c

obj/charset.o:	charset.c
	$(CCSYM) $@ charset.c

obj/diff.o:	diff.c
	$(CCSYM) $@ diff.c

obj/digraph.o:	digraph.c
	$(CCSYM) $@ digraph.c

obj/edit.o:	edit.c
	$(CCSYM) $@ edit.c

obj/eval.o:	eval.c
	$(CCSYM) $@ eval.c

obj/ex_cmds.o:	ex_cmds.c
	$(CCSYM) $@ ex_cmds.c

obj/ex_cmds2.o:	ex_cmds2.c
	$(CCSYM) $@ ex_cmds2.c

# Don't use $(SYMS) here, because ex_docmd.c defines DO_DECLARE_EXCMD
obj/ex_docmd.o:	ex_docmd.c ex_cmds.h
	$(CCNOSYM) $@ ex_docmd.c

obj/ex_eval.o:	ex_eval.c ex_cmds.h
	$(CCSYM) $@ ex_eval.c

obj/ex_getln.o:	ex_getln.c
	$(CCSYM) $@ ex_getln.c

obj/fileio.o:	fileio.c
	$(CCSYM) $@ fileio.c

obj/fold.o:	fold.c
	$(CCSYM) $@ fold.c

obj/getchar.o:	getchar.c
	$(CCSYM) $@ getchar.c

obj/hardcopy.o:	hardcopy.c
	$(CCSYM) $@ hardcopy.c

obj/hashtab.o:	hashtab.c
	$(CCSYM) $@ hashtab.c

# Don't use $(SYMS) here, because main.c defines EXTERN
obj/main.o:	main.c option.h globals.h
	$(CCNOSYM) $@ main.c

obj/mark.o:	mark.c
	$(CCSYM) $@ mark.c

obj/memfile.o:	memfile.c
	$(CCSYM) $@ memfile.c

obj/memline.o:	memline.c
	$(CCSYM) $@ memline.c

obj/menu.o:	menu.c
	$(CCSYM) $@ menu.c

# Don't use $(SYMS) here, because message.c defines MESSAGE_FILE
obj/message.o:	message.c
	$(CCNOSYM) $@ message.c

obj/misc1.o:	misc1.c
	$(CCSYM) $@ misc1.c

obj/misc2.o:	misc2.c
	$(CCSYM) $@ misc2.c

obj/move.o:	move.c
	$(CCSYM) $@ move.c

obj/mbyte.o: mbyte.c
	$(CCSYM) $@ mbyte.c

obj/normal.o:	normal.c
	$(CCSYM) $@ normal.c

obj/ops.o:	ops.c
	$(CCSYM) $@ ops.c

# Don't use $(SYMS) here, because option.h defines variables here
obj/option.o:	option.c
	$(CCNOSYM) $@ option.c

obj/os_amiga.o:	os_amiga.c
	$(CCSYM) $@ os_amiga.c

obj/popupmnu.o:	popupmnu.c
	$(CCSYM) $@ popupmnu.c

obj/quickfix.o:	quickfix.c
	$(CCSYM) $@ quickfix.c

obj/regexp.o:	regexp.c
	$(CCSYM) $@ regexp.c

obj/screen.o:	screen.c
	$(CCSYM) $@ screen.c

obj/search.o:	search.c
	$(CCSYM) $@ search.c

obj/spell.o:	spell.c
	$(CCSYM) $@ spell.c

obj/syntax.o:	syntax.c
	$(CCSYM) $@ syntax.c

obj/tag.o:	tag.c
	$(CCSYM) $@ tag.c

obj/term.o:	term.c term.h
	$(CCSYM) $@ term.c

obj/termlib.o:	termlib.c
	$(CCSYM) $@ termlib.c

obj/ui.o:	ui.c
	$(CCSYM) $@ ui.c

obj/undo.o:	undo.c
	$(CCSYM) $@ undo.c

obj/window.o:	window.c
	$(CCSYM) $@ window.c
