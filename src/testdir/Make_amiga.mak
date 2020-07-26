#
# Makefile to run all tests for Vim, on Amiga
#
# Requires "rm", "csh" and "diff"!

VIMPROG = /vim

default: nongui

include Make_all.mak

SCRIPTS = $(SCRIPTS_ALL) $(SCRIPTS_MORE4)

# Must run test1 first to create small.vim.
$(SCRIPTS) $(SCRIPTS_GUI) $(NEW_TESTS_RES): $(SCRIPTS_FIRST)

.SUFFIXES: .in .out

nongui:	/tmp $(SCRIPTS_FIRST) $(SCRIPTS)
	csh -c echo ALL DONE

clean:
	csh -c \rm -rf *.out Xdir1 Xfind XfakeHOME Xdotest small.vim tiny.vim mbyte.vim test.ok viminfo

.in.out:
	copy $*.ok test.ok
	$(VIMPROG) -u amiga.vim -U NONE --noplugin --not-a-term -s dotest.in $*.in
	diff test.out $*.ok
	rename test.out $*.out
	-delete X#? ALL QUIET
	-delete test.ok

# Create a directory for temp files
/tmp:
	makedir /tmp

# Manx requires all dependencies, but we stopped updating them.
# Delete the .out file(s) to run test(s).
