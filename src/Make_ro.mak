#
# Makefile for Vim on RISC OS - Andy Wingate
#

GCC         = gcc -mthrowback
CFLAGS     = -DRISCOS -DFEAT_GUI
CC         = $(GCC) $(CFLAGS) -O2
# -DUP_BC_PC_EXTERN for term.c needed as BC defined in termlib.c and term.c

TERMFLAG   = -DUP_BC_PC_EXTERN

ASMFLAGS   = -throwback -objasm -gcc

OBJS =  o.buffer o.charset o.diff o.digraph o.edit o.eval o.ex_cmds o.ex_cmds2  \
	o.ex_docmd o.ex_eval o.ex_getln o.fileio o.fold o.getchar \
	o.hardcopy o.hashtab o.main o.mark o.mbyte  \
	o.memfile o.memline o.menu o.message o.misc1 o.misc2 o.move     \
	o.normal o.ops o.option o.popupmnu o.quickfix o.regexp o.screen \
	o.search   \
	o.spell o.syntax o.tag o.term o.termlib o.ui o.undo o.version	\
	o.window o.os_riscos o.swis o.gui o.gui_riscos

Vim: $(OBJS)
	$(GCC) -o Vim $(OBJS)

install: Vim
	squeeze -v Vim @.!Vim.Vim

clean:	
	create o.!fake! 0
	wipe o.* ~cf
	remove Vim

o.swis: s.swis
	as $(ASMFLAGS) -o o.swis s.swis

# Rules for object files

o.%:	c.%
	$(CC) -c $< -o $@

o.buffer:	c.buffer

o.charset:	c.charset

o.digraph:	c.digraph

o.diff:		c.diff

o.edit:		c.edit

o.eval:		c.eval

o.ex_cmds:	c.ex_cmds

o.ex_cmds2:	c.ex_cmds2

o.ex_docmd:	c.ex_docmd

o.ex_eval:	c.ex_eval

o.ex_getln:	c.ex_getln

o.fileio:	c.fileio

o.fold:		c.fold

o.getchar:	c.getchar

o.hardcopy:	c.hardcopy

o.hashtab:	c.hashtab

o.gui:		c.gui

o.gui_riscos:	c.gui_riscos

o.main:		c.main

o.mark:		c.mark

o.mbyte:	c.mbyte

o.memfile:	c.memfile

o.memline:	c.memline

o.menu:		c.menu

o.message:	c.message

o.misc1:	c.misc1

o.misc2:	c.misc2

o.move:		c.move

o.normal:	c.normal

o.ops:		c.ops

o.option:	c.option

o.os_riscos:	c.os_riscos

o.pty:		c.pty

o.popupmnu:	c.popupmnu

o.quickfix:	c.quickfix

o.regexp:	c.regexp

o.screen:	c.screen

o.search:	c.search

o.spell:	c.spell

o.syntax:	c.syntax

o.tag:		c.tag

o.term:		c.term
	$(CC) $(TERMFLAG) -c c.term -o o.term

o.termlib:	c.termlib

o.ui:		c.ui

o.undo:		c.undo

o.version:	c.version

o.window:	c.window
