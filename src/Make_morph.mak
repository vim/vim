#
# Makefile for VIM, using MorphOS SDK (gcc 2.95.3)
#

CFLAGS =	-c						\
		-pipe						\
		-O2						\
		-Wall						\
								\
		-DNO_ARP					\
		-DUSE_TMPNAM					\
								\
		-I proto					\
								\
		-noixemul

PRG =		Vim
LIBS =		-noixemul -s
CC =		gcc
LD =		gcc
OBJDUMP =	objdump
RM =		rm

.c.o:
	${CC} ${CFLAGS} $< -o $@

SRC =	arabic.c						\
	blowfish.c						\
	buffer.c						\
	charset.c						\
	crypt.c							\
	crypt_zip.c						\
	dict.c							\
	diff.c							\
	digraph.c						\
	edit.c							\
	eval.c							\
	evalfunc.c						\
	ex_cmds.c						\
	ex_cmds2.c						\
	ex_docmd.c						\
	ex_eval.c						\
	ex_getln.c						\
	farsi.c							\
	fileio.c						\
	fold.c							\
	getchar.c						\
	hardcopy.c						\
	hashtab.c						\
	json.c							\
	list.c							\
	main.c							\
	mark.c							\
	mbyte.c							\
	memfile.c						\
	memline.c						\
	menu.c							\
	message.c						\
	misc1.c							\
	misc2.c							\
	move.c							\
	normal.c						\
	ops.c							\
	option.c						\
	os_amiga.c						\
	popupmnu.c						\
	quickfix.c						\
	regexp.c						\
	screen.c						\
	search.c						\
	sha256.c						\
	sign.c							\
	spell.c							\
	spellfile.c						\
	syntax.c						\
	tag.c							\
	term.c							\
	ui.c							\
	undo.c							\
	userfunc.c						\
	version.c						\
	window.c						\

OBJ =	$(SRC:.c=.o)

$(PRG): $(OBJ)
	${LD} -o $(PRG) $(OBJ) $(LIBS)

dump: $(PRG)
	$(OBJDUMP) --reloc --disassemble-all $(PRG) > $(PRG).s

clean:
	$(RM) -fv $(OBJ) $(PRG) $(PRG).s
