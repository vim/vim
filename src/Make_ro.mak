#
# Makefile for Vim on RISC OS - Andy Wingate
#

GCC	    = gcc -mthrowback
CFLAGS	   = -DRISCOS -DFEAT_GUI
# Optimising on ex_docmd.c seems to cause segfaults on compilation. Needs investigation.
CCEX_DOCMD = $(GCC) $(CFLAGS)
CC	   = $(GCC) $(CFLAGS) -O2
# -DUP_BC_PC_EXTERN for term.c needed as BC defined in termlib.c and term.c

TERMFLAG   = -DUP_BC_PC_EXTERN

ASMFLAGS   = -throwback -objasm -gcc

OBJS =  o.buffer o.charset o.digraph o.edit o.eval o.ex_cmds o.ex_cmds2 o.diff \
	o.ex_docmd o.ex_eval o.ex_getln o.fileio o.fold o.getchar o.main o.mark o.mbyte  \
	o.memfile o.memline o.menu o.message o.misc1 o.misc2 o.move     \
	o.normal o.ops o.option o.quickfix o.regexp o.screen o.search   \
	o.syntax o.tag o.term o.termlib o.ui o.undo o.version o.window  \
	o.os_riscos o.swis o.gui o.gui_riscos

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
# You shouldn't need to put all this information in as all but term.c have the same
# rule (and only then to save extra defines) but some versions of make are awkward.

o.buffer:	c.buffer
	$(CC) -c c.buffer -o o.buffer

o.charset:	c.charset
	$(CC) -c c.charset -o o.charset

o.digraph:	c.digraph
	$(CC) -c c.digraph -o o.digraph

o.diff:		c.diff
	$(CC) -c c.diff -o o.diff

o.edit:		c.edit
	$(CC) -c c.edit -o o.edit

o.eval:		c.eval
	$(CC) -c c.eval -o o.eval

o.ex_cmds:	c.ex_cmds
	$(CC) -c c.ex_cmds -o o.ex_cmds

o.ex_cmds2:	c.ex_cmds2
	$(CC) -c c.ex_cmds2 -o o.ex_cmds2

o.ex_docmd:	c.ex_docmd
	$(CCEX_DOCMD) -c c.ex_docmd -o o.ex_docmd

o.ex_eval:	c.ex_eval
	$(CCEX_DOCMD) -c c.ex_eval -o o.ex_eval

o.ex_getln:	c.ex_getln
	$(CC) -c c.ex_getln -o o.ex_getln

o.fileio:	c.fileio
	$(CC) -c c.fileio -o o.fileio

o.fold:		c.fold
	$(CC) -c c.fold -o o.fold

o.getchar:	c.getchar
	$(CC) -c c.getchar -o o.getchar

o.gui:		c.gui
	$(CC) -c c.gui -o o.gui

o.gui_riscos:	c.gui_riscos
	$(CC) -c c.gui_riscos -o o.gui_riscos

o.main:		c.main
	$(CC) -c c.main -o o.main

o.mark:		c.mark
	$(CC) -c c.mark -o o.mark

o.mbyte:	c.mbyte
	$(CC) -c c.mbyte -o o.mbyte

o.memfile:	c.memfile
	$(CC) -c c.memfile -o o.memfile

o.memline:	c.memline
	$(CC) -c c.memline -o o.memline

o.menu:		c.menu
	$(CC) -c c.menu -o o.menu

o.message:	c.message
	$(CC) -c c.message -o o.message

o.misc1:	c.misc1
	$(CC) -c c.misc1 -o o.misc1

o.misc2:	c.misc2
	$(CC) -c c.misc2 -o o.misc2

o.move:		c.move
	$(CC) -c c.move -o o.move

o.normal:	c.normal
	$(CC) -c c.normal -o o.normal

o.ops:		c.ops
	$(CC) -c c.ops -o o.ops

o.option:	c.option
	$(CC) -c c.option -o o.option

o.os_riscos:	c.os_riscos
	$(CC) -c c.os_riscos -o o.os_riscos

o.pty:		c.pty
	$(CC) -c c.pty -o p.pty

o.quickfix:	c.quickfix
	$(CC) -c c.quickfix -o o.quickfix

o.regexp:	c.regexp
	$(CC) -c c.regexp -o o.regexp

o.screen:	c.screen
	$(CC) -c c.screen -o o.screen

o.search:	c.search
	$(CC) -c c.search -o o.search

o.syntax:	c.syntax
	$(CC) -c c.syntax -o o.syntax

o.tag:		c.tag
	$(CC) -c c.tag -o o.tag

o.term:		c.term
	$(CC) $(TERMFLAG) -c c.term -o o.term

o.termlib:	c.termlib
	$(CC) -c c.termlib -o o.termlib

o.ui:		c.ui
	$(CC) -c c.ui -o o.ui

o.undo:		c.undo
	$(CC) -c c.undo -o o.undo

o.version:	c.version
	$(CC) -c c.version -o o.version

o.window:	c.window
	$(CC) -c c.window -o o.window
