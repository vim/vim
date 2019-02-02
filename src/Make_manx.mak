#
# Makefile for VIM on the Amiga, using Aztec/Manx C 5.0 or later
#
# NOTE: THIS IS OLD AND PROBABLY NO LONGER WORKS.
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

SRC =	arabic.c \
	autocmd.c \
	blowfish.c \
	buffer.c \
	charset.c \
	crypt.c \
	crypt_zip.c \
	dict.c \
	diff.c \
	digraph.c \
	edit.c \
	eval.c \
	evalfunc.c \
	ex_cmds.c \
	ex_cmds2.c \
	ex_docmd.c \
	ex_eval.c \
	ex_getln.c \
	farsi.c \
	fileio.c \
	findfile.c \
	fold.c \
	getchar.c \
	hardcopy.c \
	hashtab.c \
	indent.c \
	json.c \
	list.c \
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
	sha256.c \
	sign.c \
	spell.c \
	spellfile.c \
	syntax.c \
	tag.c \
	term.c \
	ui.c \
	undo.c \
	userfunc.c \
	window.c \
	version.c

INCL = vim.h feature.h keymap.h macros.h ascii.h term.h structs.h os_amiga.h

OBJ =	obj/arabic.o \
	obj/autocmd.o \
	obj/blowfish.o \
	obj/buffer.o \
	obj/charset.o \
	obj/crypt.o \
	obj/crypt_zip.o \
	obj/dict.o \
	obj/diff.o \
	obj/digraph.o \
	obj/edit.o \
	obj/eval.o \
	obj/evalfunc.o \
	obj/ex_cmds.o \
	obj/ex_cmds2.o \
	obj/ex_docmd.o \
	obj/ex_eval.o \
	obj/ex_getln.o \
	obj/farsi.o \
	obj/fileio.o \
	obj/findfile.o \
	obj/fold.o \
	obj/getchar.o \
	obj/hardcopy.o \
	obj/hashtab.o \
	obj/indent.o \
	obj/json.o \
	obj/list.o \
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
	obj/sha256.o \
	obj/sign.o \
	obj/spell.o \
	obj/spellfile.o \
	obj/syntax.o \
	obj/tag.o \
	obj/term.o \
	obj/ui.o \
	obj/undo.o \
	obj/userfunc.o \
	obj/window.o \
	$(TERMLIB)

PRO =	proto/arabic.pro \
	proto/autocmd.pro \
	proto/blowfish.pro \
	proto/buffer.pro \
	proto/charset.pro \
	proto/crypt.pro \
	proto/crypt_zip.pro \
	proto/dict.pro \
	proto/diff.pro \
	proto/digraph.pro \
	proto/edit.pro \
	proto/eval.pro \
	proto/evalfunc.pro \
	proto/ex_cmds.pro \
	proto/ex_cmds2.pro \
	proto/ex_docmd.pro \
	proto/ex_eval.pro \
	proto/ex_getln.pro \
	proto/farsi.pro \
	proto/fileio.pro \
	proto/findfile.pro \
	proto/fold.pro \
	proto/getchar.pro \
	proto/hardcopy.pro \
	proto/hashtab.pro \
	proto/indent.pro \
	proto/json.pro \
	proto/list.pro \
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
	proto/sha256.pro \
	proto/sign.pro \
	proto/spell.pro \
	proto/spellfile.pro \
	proto/syntax.pro \
	proto/tag.pro \
	proto/term.pro \
	proto/termlib.pro \
	proto/ui.pro \
	proto/undo.pro \
	proto/userfunc.pro \
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

obj/arabic.o:	arabic.c
	$(CCSYM) $@ arabic.c

obj/autocmd.o:	autocmd.c
	$(CCSYM) $@ autocmd.c

obj/blowfish.o:	blowfish.c
	$(CCSYM) $@ blowfish.c

obj/buffer.o:	buffer.c
	$(CCSYM) $@ buffer.c

obj/charset.o:	charset.c
	$(CCSYM) $@ charset.c

obj/crypt.o:	crypt.c
	$(CCSYM) $@ crypt.c

obj/crypt_zip.o: crypt_zip.c
	$(CCSYM) $@ crypt_zip.c

obj/dict.o:	dict.c
	$(CCSYM) $@ dict.c

obj/diff.o:	diff.c
	$(CCSYM) $@ diff.c

obj/digraph.o:	digraph.c
	$(CCSYM) $@ digraph.c

obj/edit.o:	edit.c
	$(CCSYM) $@ edit.c

obj/eval.o:	eval.c
	$(CCSYM) $@ eval.c

obj/evalfunc.o:	evalfunc.c
	$(CCSYM) $@ evalfunc.c

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

obj/farsi.o:	farsi.c
	$(CCSYM) $@ farsi.c

obj/fileio.o:	fileio.c
	$(CCSYM) $@ fileio.c

obj/findfile.o:	findfile.c
	$(CCSYM) $@ findfile.c

obj/fold.o:	fold.c
	$(CCSYM) $@ fold.c

obj/getchar.o:	getchar.c
	$(CCSYM) $@ getchar.c

obj/hardcopy.o:	hardcopy.c
	$(CCSYM) $@ hardcopy.c

obj/hashtab.o:	hashtab.c
	$(CCSYM) $@ hashtab.c

obj/indent.o:	indent.c
	$(CCSYM) $@ indent.c

obj/json.o:	json.c
	$(CCSYM) $@ json.c

obj/list.o:	list.c
	$(CCSYM) $@ list.c

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

obj/sha256.o:	sha256.c
	$(CCSYM) $@ sha256.c

obj/sign.o:	sign.c
	$(CCSYM) $@ sign.c

obj/spell.o:	spell.c
	$(CCSYM) $@ spell.c

obj/spellfile.o: spellfile.c
	$(CCSYM) $@ spellfile.c

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

obj/userfunc.o:	userfunc.c
	$(CCSYM) $@ userfunc.c

obj/window.o:	window.c
	$(CCSYM) $@ window.c
