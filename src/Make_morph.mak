#
# Makefile for VIM, using MorphOS SDK (gcc 2.95.3)
#

# Uncomment the following two lines and comment the two after in
# case you want to play with GVIM MorphOS. But it's still known
# to not work at all. So meanwhile it's better to stick with VIM.

# GVIM =	-DFEAT_GUI_AMIGA
# GVIMSRC =	gui_amiga.c gui.c

GVIM =
GVIMSRC =

CFLAGS =	-c						\
		-pipe						\
		-O2						\
		-Wall						\
								\
		-DNO_ARP					\
		-DUSE_TMPNAM					\
		${GVIM}						\
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

SRC =	buffer.c						\
	charset.c						\
	diff.c							\
	digraph.c						\
	edit.c							\
	eval.c							\
	ex_cmds.c						\
	ex_cmds2.c						\
	ex_docmd.c						\
	ex_eval.c						\
	ex_getln.c						\
	fileio.c						\
	fold.c							\
	getchar.c						\
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
	quickfix.c						\
	regexp.c						\
	screen.c						\
	search.c						\
	syntax.c						\
	tag.c							\
	term.c							\
	ui.c							\
	undo.c							\
	version.c						\
	window.c						\
	${GVIMSRC}

OBJ =	$(SRC:.c=.o)

$(PRG): $(OBJ)
	${LD} -o $(PRG) $(OBJ) $(LIBS)

dump: $(PRG)
	$(OBJDUMP) --reloc --disassemble-all $(PRG) > $(PRG).s

clean:
	$(RM) -fv $(OBJ) $(PRG) $(PRG).s
