#
# Makefile to run all tests for Vim, on Dos-like machines
# with sh.exe or zsh.exe in the path or not.
#
# Author: Bill McCarthy
#
# Note that test54 has been removed until it is fixed.
#
# Requires a set of Unix tools: echo, diff, etc.

ifneq (sh.exe, $(SHELL))
DEL = rm -f
DELDIR = rm -rf
MV = mv
CP = cp
CAT = cat
DIRSLASH = /
else
DEL = del
DELDIR = rd /s /q
MV = rename
CP = copy
CAT = type
DIRSLASH = \\
endif

VIMPROG = ..$(DIRSLASH)vim

default: vimall

include Make_all.mak

# Omitted:
# test2		"\\tmp" doesn't work.
# test10	'errorformat' is different
# test12	can't unlink a swap file
# test25	uses symbolic link
# test27	can't edit file with "*" in file name
# test54	doesn't work yet
# test97	\{ and \$ are not escaped characters

SCRIPTS = $(SCRIPTS_ALL) $(SCRIPTS_MORE1) $(SCRIPTS_MORE4) $(SCRIPTS_WIN32)

SCRIPTS_BENCH = bench_re_freeze.out

# Must run test1 first to create small.vim.
$(SCRIPTS) $(SCRIPTS_GUI) $(SCRIPTS_WIN32) $(NEW_TESTS): $(SCRIPTS_FIRST)

.SUFFIXES: .in .out

vimall:	fixff $(SCRIPTS_FIRST) $(SCRIPTS) $(SCRIPTS_GUI) $(SCRIPTS_WIN32)
	echo ALL DONE

nongui:	fixff $(SCRIPTS_FIRST) $(SCRIPTS)
	echo ALL DONE

benchmark: $(SCRIPTS_BENCH)

small:
	echo ALL DONE

gui:	fixff $(SCRIPTS_FIRST) $(SCRIPTS) $(SCRIPTS_GUI)
	echo ALL DONE

win32:	fixff $(SCRIPTS_FIRST) $(SCRIPTS) $(SCRIPTS_WIN32)
	echo ALL DONE

fixff:
	-$(VIMPROG) -u dos.vim --noplugin "+argdo set ff=dos|upd" +q *.in *.ok
	-$(VIMPROG) -u dos.vim --noplugin "+argdo set ff=unix|upd" +q \
		dotest.in test60.ok test71.ok test74.ok test_listchars.ok

clean:
	-$(DEL) *.out
	-$(DEL) test.ok
	-$(DEL) small.vim
	-$(DEL) tiny.vim
	-$(DEL) mbyte.vim
	-$(DEL) mzscheme.vim
	-$(DEL) lua.vim
	-$(DELDIR) Xdir1
	-$(DELDIR) Xfind
	-$(DEL) X*
	-$(DEL) viminfo

.in.out:
	$(CP) $*.ok test.ok
	$(VIMPROG) -u dos.vim -U NONE --noplugin -s dotest.in $*.in
	diff test.out $*.ok
	-$(DEL) $*.out
	$(MV) test.out $*.out
	-$(DELDIR) Xdir1
	-$(DELDIR) Xfind
	-$(DEL) X*
	-$(DEL) test.ok
	-$(DEL) viminfo

bench_re_freeze.out: bench_re_freeze.vim
	-$(DEL) benchmark.out
	$(VIMPROG) -u dos.vim -U NONE --noplugin $*.in
	$(CAT) benchmark.out
