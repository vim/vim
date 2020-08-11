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

SCRIPTS_BENCH = test_bench_regexp.res

.SUFFIXES: .res .vim

vimall:	newtests
	@echo ALL DONE

nongui:	nolog newtests
	@echo ALL DONE

benchmark: $(SCRIPTS_BENCH)

small: nolog
	@echo ALL DONE

gui:	nolog newtests
	@echo ALL DONE

win32:	nolog newtests
	@echo ALL DONE

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

nolog:
	-@if exist test.log $(DEL) test.log
	-@if exist messages $(DEL) messages

test_bench_regexp.res: test_bench_regexp.vim
	-$(DEL) benchmark.out
	@echo $(VIMPROG) > vimcmd
	$(VIMPROG) -u NONE $(NO_INITS) -S runtest.vim $*.vim
	@$(DEL) vimcmd
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

opt_test.vim: ../optiondefs.h gen_opt_test.vim
	$(VIMPROG) -u NONE -S gen_opt_test.vim --noplugin --not-a-term ../optiondefs.h
