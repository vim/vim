#
# Makefile to run all tests for Vim, on Amiga
#
# Requires "rm", "csh" and "diff"!

VIMPROG = /vim

include Make_all.mak

# These tests don't work (yet):
# test2		"\\tmp" doesn't work
# test10	'errorformat' is different
# test11	"cat" doesn't work properly
# test12	can't unlink a swap file
# test25	uses symbolic link
# test27	can't edit file with "*"
# test52	only for Win32
# test85	no Lua interface
# test86, 87	no Python interface

SCRIPTS = $(SCRIPTS_ALL) $(SCRIPTS_MORE3) $(SCRIPTS_MORE4)

.SUFFIXES: .in .out

nongui:	/tmp $(SCRIPTS)
	csh -c echo ALL DONE

clean:
	csh -c \rm -rf *.out /tmp/* Xdotest small.vim tiny.vim mbyte.vim test.ok viminfo

.in.out:
	copy $*.ok test.ok
	$(VIMPROG) -u amiga.vim -U NONE --noplugin -s dotest.in $*.in
	diff test.out $*.ok
	rename test.out $*.out
	-delete X#? ALL QUIET
	-delete test.ok

# Create a directory for temp files
/tmp:
	makedir /tmp

# Manx requires all dependencies, but we stopped updating them.
# Delete the .out file(s) to run test(s).
