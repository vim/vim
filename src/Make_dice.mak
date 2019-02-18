#
# Makefile for VIM, using DICE 3
#

#>>>>> choose options:
### See feature.h for a list of optionals.
### Any other defines can be included here.
DEFINES = -DHAVE_TGETENT -DUP_BC_PC_EXTERN -DOSPEED_EXTERN

#>>>>> if HAVE_TGETENT is defined o/termlib.o has to be used
TERMLIB = o/termlib.o
#TERMLIB =

#>>>>> end of choices
###########################################################################

CFLAGS = -c -DAMIGA -Iproto $(DEFINES)

SYMS = vim.syms
PRE = -H${SYMS}=vim.h
LIBS = -la
CC = dcc
LD = dcc

.c.o:
	${CC} ${PRE} ${CFLAGS} $< -o $@

SRC = \
	arabic.c \
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

OBJ =	o/arabic.o \
	o/autocmd.o \
	o/blowfish.o \
	o/buffer.o \
	o/charset.o \
	o/crypt.o \
	o/crypt_zip.o \
	o/dict.o \
	o/diff.o \
	o/digraph.o \
	o/edit.o \
	o/eval.o \
	o/evalfunc.o \
	o/ex_cmds.o \
	o/ex_cmds2.o \
	o/ex_docmd.o \
	o/ex_eval.o \
	o/ex_getln.o \
	o/fileio.o \
	o/findfile.o \
	o/fold.o \
	o/getchar.o \
	o/hardcopy.o \
	o/hashtab.o \
	o/indent.o \
	o/json.o \
	o/list.o \
	o/main.o \
	o/mark.o \
	o/memfile.o \
	o/memline.o \
	o/menu.o \
	o/message.o \
	o/misc1.o \
	o/misc2.o \
	o/move.o \
	o/mbyte.o \
	o/normal.o \
	o/ops.o \
	o/option.o \
	o/os_amiga.o \
	o/popupmnu.o \
	o/quickfix.o \
	o/regexp.o \
	o/screen.o \
	o/search.o \
	o/sha256.o \
	o/sign.o \
	o/spell.o \
	o/spellfile.o \
	o/syntax.o \
	o/tag.o \
	o/term.o \
	o/ui.o \
	o/undo.o \
	o/userfunc.o \
	o/window.o \
	$(TERMLIB)

Vim: $(OBJ) version.c version.h
	${CC} $(CFLAGS) version.c -o o/version.o
	${LD} -o Vim $(OBJ) o/version.o $(LIBS)

debug: $(OBJ) version.c version.h
	${CC} $(CFLAGS) version.c -o o/version.o
	${LD} -s -o Vim $(OBJ) o/version.o $(LIBS)

tags:
	csh -c ctags $(SRC) *.h

clean:
	delete o/*.o Vim $(SYMS)

$(SYMS)  : vim.h globals.h keymap.h macros.h ascii.h term.h os_amiga.h structs.h
	delete $(SYMS)

###########################################################################

o/arabic.o:	arabic.c  $(SYMS)

o/autocmd.o:	autocmd.c  $(SYMS)

o/blowfish.o:	blowfish.c  $(SYMS)

o/buffer.o:	buffer.c  $(SYMS)

o/charset.o:	charset.c  $(SYMS)

o/crypt.o:	crypt.c  $(SYMS)

o/crypt_zip.o:	crypt_zip.c  $(SYMS)

o/dict.o:	dict.c	$(SYMS)

o/diff.o:	diff.c	$(SYMS)

o/digraph.o:	digraph.c  $(SYMS)

o/edit.o:	edit.c	$(SYMS)

o/eval.o:	eval.c  $(SYMS)

o/evalfunc.o:	evalfunc.c  $(SYMS)

o/ex_cmds.o:	ex_cmds.c  $(SYMS)

o/ex_cmds2.o:	ex_cmds2.c  $(SYMS)

o/ex_docmd.o:	ex_docmd.c  $(SYMS) ex_cmds.h

o/ex_eval.o:	ex_eval.c  $(SYMS) ex_cmds.h

o/ex_getln.o:	ex_getln.c  $(SYMS)

o/fileio.o:	fileio.c  $(SYMS)

o/findfile.o:	findfile.c  $(SYMS)

o/fold.o:	fold.c  $(SYMS)

o/getchar.o: getchar.c	$(SYMS)

o/hardcopy.o: hardcopy.c	$(SYMS)

o/hashtab.o: hashtab.c	$(SYMS)

o/indent.o:	indent.c  $(SYMS)

o/json.o:	json.c  $(SYMS)

o/list.o:	list.c  $(SYMS)

o/main.o: main.c $(SYMS)

o/mark.o: mark.c	$(SYMS)

o/memfile.o:	memfile.c  $(SYMS)

o/memline.o:	memline.c  $(SYMS)

o/menu.o:	menu.c  $(SYMS)

o/message.o:	message.c  $(SYMS)

o/misc1.o:	misc1.c  $(SYMS)

o/misc2.o:	misc2.c  $(SYMS)

o/move.o:	move.c  $(SYMS)

o/mbyte.o:	mbyte.c  $(SYMS)

o/normal.o:	normal.c  $(SYMS)

o/ops.o:	ops.c  $(SYMS)

o/option.o:	option.c  $(SYMS)
# Because of a bug in DC1 2.06.40, initialisation of unions does not
# work correctly. dc1-21 is DC1 2.06.21 which does work.
#	rename dc1-21 dc1
	${CC} ${CFLAGS} option.c -o o/option.o
#	rename dc1 dc1-21

o/os_amiga.o:	os_amiga.c  $(SYMS) os_amiga.h

o/popupmnu.o:	popupmnu.c  $(SYMS)

o/quickfix.o:	quickfix.c  $(SYMS)

o/regexp.o:	regexp.c  $(SYMS) regexp.h

o/screen.o:	screen.c  $(SYMS)

o/search.o:	search.c  $(SYMS) regexp.h

o/sha256.o:	sha256.c  $(SYMS)

o/sign.o:	sign.c  $(SYMS)

o/spell.o:	spell.c  $(SYMS) spell.h

o/spellfile.o:	spellfile.c  $(SYMS) spell.h

o/syntax.o:	syntax.c  $(SYMS)

o/tag.o:	tag.c  $(SYMS)

o/term.o:	term.c  $(SYMS) term.h

o/termlib.o:	termlib.c $(SYMS)

o/ui.o: 	ui.c	$(SYMS)

o/undo.o: 	undo.c	$(SYMS)

o/userfunc.o: 	userfunc.c  $(SYMS)

o/window.o: 	window.c  $(SYMS)
