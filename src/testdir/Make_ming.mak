#
# Makefile to run all tests for Vim, on Dos-like machines
# with sh.exe or zsh.exe in the path or not.
#
# Author: Bill McCarthy
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
# test97	\{ and \$ are not escaped characters

SCRIPTS = $(SCRIPTS_ALL) $(SCRIPTS_MORE1) $(SCRIPTS_MORE4) $(SCRIPTS_WIN32)

SCRIPTS_BENCH = bench_re_freeze.out

# Must run test1 first to create small.vim.
$(SCRIPTS) $(SCRIPTS_GUI) $(SCRIPTS_WIN32) $(NEW_TESTS_RES): $(SCRIPTS_FIRST)

.SUFFIXES: .in .out .res .vim

vimall:	fixff $(SCRIPTS_FIRST) $(SCRIPTS) $(SCRIPTS_GUI) $(SCRIPTS_WIN32) newtests
	@echo ALL DONE

nongui:	fixff nolog $(SCRIPTS_FIRST) $(SCRIPTS) newtests
	@echo ALL DONE

benchmark: $(SCRIPTS_BENCH)

small: nolog
	@echo ALL DONE

gui:	fixff nolog $(SCRIPTS_FIRST) $(SCRIPTS) $(SCRIPTS_GUI) newtests
	@echo ALL DONE

win32:	fixff nolog $(SCRIPTS_FIRST) $(SCRIPTS) $(SCRIPTS_WIN32) newtests
	@echo ALL DONE

# TODO: find a way to avoid changing the distributed files.
fixff:
	-$(VIMPROG) -u dos.vim $(NO_INITS) "+argdo set ff=dos|upd" +q *.in *.ok
	-$(VIMPROG) -u dos.vim $(NO_INITS) "+argdo set ff=unix|upd" +q \
		dotest.in

clean:
	-@if exist *.out $(DEL) *.out
	-@if exist *.failed $(DEL) *.failed
	-@if exist *.res $(DEL) *.res
	-@if exist test.in $(DEL) test.in
	-@if exist test.ok $(DEL) test.ok
	-@if exist small.vim $(DEL) small.vim
	-@if exist tiny.vim $(DEL) tiny.vim
	-@if exist mbyte.vim $(DEL) mbyte.vim
	-@if exist mzscheme.vim $(DEL) mzscheme.vim
	-@if exist Xdir1 $(DELDIR) Xdir1
	-@if exist Xfind $(DELDIR) Xfind
	-@if exist XfakeHOME $(DELDIR) XfakeHOME
	-@if exist X* $(DEL) X*
	-@if exist viminfo $(DEL) viminfo
	-@if exist test.log $(DEL) test.log
	-@if exist messages $(DEL) messages
	-@if exist opt_test.vim $(DEL) opt_test.vim

test1.out: test1.in
	-@if exist wrongtermsize  $(DEL) wrongtermsize
	$(VIMPROG) -u dos.vim $(NO_INITS) -s dotest.in test1.in
	-@if exist wrongtermsize  ( \
	    echo Vim window too small- must be 80x25 or larger && exit 1 \
	    )
	-@if exist test.out $(DEL) test.out
	-@if exist viminfo  $(DEL) viminfo

.in.out:
	-@if exist $*.ok $(CP) $*.ok test.ok
	$(VIMPROG) -u dos.vim $(NO_INITS) -s dotest.in $*.in
	@diff test.out $*.ok
	-@if exist $*.out $(DEL) $*.out
	@$(MV) test.out $*.out
	-@if exist Xdir1 $(DELDIR) Xdir1
	-@if exist Xfind $(DELDIR) Xfind
	-@if exist XfakeHOME $(DELDIR) XfakeHOME
	-@if exist X* $(DEL) X*
	-@if exist test.ok $(DEL) test.ok
	-@if exist viminfo $(DEL) viminfo

nolog:
	-@if exist test.log $(DEL) test.log
	-@if exist messages $(DEL) messages

bench_re_freeze.out: bench_re_freeze.vim
	-$(DEL) benchmark.out
	$(VIMPROG) -u dos.vim $(NO_INITS) $*.in
	$(CAT) benchmark.out

# New style of tests uses Vim script with assert calls.  These are easier
# to write and a lot easier to read and debug.
# Limitation: Only works with the +eval feature.

newtests: $(NEW_TESTS_RES)

.vim.res:
	@echo $(VIMPROG) > vimcmd
	$(VIMPROG) -u NONE $(NO_INITS) -S runtest.vim $*.vim
	@$(DEL) vimcmd

test_gui.res: test_gui.vim
	@echo $(VIMPROG) > vimcmd
	$(VIMPROG) -u NONE $(NO_INITS) -S runtest.vim $<
	@$(DEL) vimcmd

test_gui_init.res: test_gui_init.vim
	@echo $(VIMPROG) > vimcmd
	$(VIMPROG) -u gui_preinit.vim -U gui_init.vim $(NO_PLUGINS) -S runtest.vim $<
	@$(DEL) vimcmd

test_options.res test_alot.res: opt_test.vim

opt_test.vim: ../option.c gen_opt_test.vim
	$(VIMPROG) -u NONE -S gen_opt_test.vim --noplugin --not-a-term ../option.c
