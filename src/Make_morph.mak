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
	arglist.c						\
	autocmd.c						\
	blowfish.c						\
	buffer.c						\
	bufwrite.c						\
	change.c						\
	charset.c						\
	cindent.c						\
	cmdexpand.c						\
	cmdhist.c						\
	crypt.c							\
	crypt_zip.c						\
	debugger.c						\
	dict.c							\
	diff.c							\
	digraph.c						\
	drawline.c						\
	drawscreen.c						\
	edit.c							\
	eval.c							\
	evalbuffer.c						\
	evalfunc.c						\
	evalvars.c						\
	evalwindow.c						\
	ex_cmds.c						\
	ex_cmds2.c						\
	ex_docmd.c						\
	ex_eval.c						\
	ex_getln.c						\
	fileio.c						\
	filepath.c						\
	findfile.c						\
	fold.c							\
	getchar.c						\
	hardcopy.c						\
	hashtab.c						\
	highlight.c						\
	indent.c						\
	insexpand.c						\
	json.c							\
	list.c							\
	main.c							\
	map.c							\
	mark.c							\
	mbyte.c							\
	memfile.c						\
	memline.c						\
	menu.c							\
	message.c						\
	misc1.c							\
	misc2.c							\
	mouse.c							\
	move.c							\
	normal.c						\
	ops.c							\
	option.c						\
	optionstr.c						\
	os_amiga.c						\
	popupmenu.c						\
	profiler.c						\
	quickfix.c						\
	regexp.c						\
	register.c						\
	scriptfile.c						\
	screen.c						\
	search.c						\
	session.c						\
	sha256.c						\
	sign.c							\
	spell.c							\
	spellfile.c						\
	spellsuggest.c						\
	syntax.c						\
	tag.c							\
	term.c							\
	testing.c						\
	textprop.c						\
	ui.c							\
	undo.c							\
	usercmd.c						\
	userfunc.c						\
	version.c						\
	viminfo.c						\
	window.c						\

OBJ =	$(SRC:.c=.o)

$(PRG): $(OBJ)
	${LD} -o $(PRG) $(OBJ) $(LIBS)

dump: $(PRG)
	$(OBJDUMP) --reloc --disassemble-all $(PRG) > $(PRG).s

clean:
	$(RM) -fv $(OBJ) $(PRG) $(PRG).s
