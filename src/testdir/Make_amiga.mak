#
# Makefile to run all tests for Vim, on Amiga
#
# Requires "rm", "csh" and "diff"!

VIMPROG = /vim

default: nongui

include Make_all.mak

.SUFFIXES: .res .vim

nongui:	/tmp
	csh -c echo ALL DONE

clean:
	csh -c \rm -rf *.out Xdir1 Xfind XfakeHOME Xdotest small.vim tiny.vim mbyte.vim test.ok viminfo

# Create a directory for temp files
/tmp:
	makedir /tmp

# Manx requires all dependencies, but we stopped updating them.
# Delete the .out file(s) to run test(s).
