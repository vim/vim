#
# Makefile for VIM on MSDOS, using DJGPP 2.0
#

#>>>>> choose options:

### See feature.h for a list of optionals.
### Any other defines can be included here.

DEFINES =

#>>>>> name of the compiler and linker, name of lib directory
CC = gcc

#>>>>> end of choices
###########################################################################

INCL = vim.h globals.h option.h keymap.h macros.h ascii.h term.h os_msdos.h structs.h
CFLAGS = -O2 -DMSDOS -Iproto $(DEFINES) -Wall -Dinterrupt= -Dfar= -DMAXMEM=512 -D_NAIVE_DOS_REGS

OBJ = \
	obj/buffer.o \
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
	obj/os_msdos.o \
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

all: vim.exe install.exe uninstal.exe xxd/xxd.exe

# version.c is compiled each time, so that it sets the build time.
vim.exe: obj $(OBJ) version.c version.h
	$(CC) $(CFLAGS) -s -o vim.exe version.c $(OBJ) -lpc

install.exe: dosinst.c
	$(CC) $(CFLAGS) -s -o install.exe dosinst.c -lpc

uninstal.exe: uninstal.c
	$(CC) $(CFLAGS) -s -o uninstal.exe uninstal.c -lpc

# This requires GNU make.
xxd/xxd.exe: xxd/xxd.c
	$(MAKE) --directory=xxd -f Make_djg.mak

obj:
	mkdir obj

tags:
	command /c ctags *.c $(INCL) ex_cmds.h

clean:
	-del obj\*.o
	-rmdir obj
	-del vim.exe
	-del install.exe
	-del xxd\xxd.exe
	-del testdir\*.out

# This requires GNU make.
test:
	$(MAKE) --directory=testdir -f Make_dos.mak

###########################################################################

obj/%.o: %.c obj $(INCL)
	$(CC) -c $(CFLAGS) -o $@ $<

# Extra dependency (there are actually many more...)
obj/ex_docmd.o:	ex_cmds.h
