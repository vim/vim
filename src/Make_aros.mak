# Makefile for AROS

CFLAGS = -pipe -O2 -Wall -Iproto \
         -DNO_ARP -DUSE_TMPNAM -DFEAT_GUI_AMIGA

PRG    = VIM
LIBS   =
CC     = i386-linux-aros-gcc
LD     = i386-linux-aros-gcc
RM     = rm

SRCS   = buffer.c charset.c diff.c digraph.c edit.c eval.c ex_cmds.c          \
         ex_cmds2.c ex_docmd.c ex_eval.c ex_getln.c fileio.c fold.c getchar.c \
         main.c mark.c mbyte.c memfile.c memline.c menu.c message.c misc1.c   \
         misc2.c move.c normal.c ops.c option.c os_amiga.c quickfix.c         \
         regexp.c screen.c search.c syntax.c tag.c term.c ui.c undo.c         \
         version.c window.c gui_amiga.c gui.c

OBJS   = $(SRCS:.c=.o)


$(PRG): $(OBJS)
	${LD} -o $(PRG) $(OBJS) $(LIBS)

.c.o:
	${CC} -c ${CFLAGS} $< -o $@

clean:
	$(RM) -fv $(OBJS) $(PRG)
