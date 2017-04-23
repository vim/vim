#
# Makefile to run all tests for Vim, on Dos-like machines.
#
# Requires a set of Unix tools: echo, diff, etc.

VIMPROG = ..\\vim

default: nongui

!include Make_all.mak

# Omitted:
# test2		"\\tmp" doesn't work.
# test10	'errorformat' is different
# test12	can't unlink a swap file
# test25	uses symbolic link
# test49	fails in various ways
# test97	\{ and \$ are not escaped characters.

SCRIPTS = $(SCRIPTS_ALL) $(SCRIPTS_MORE1) $(SCRIPTS_MORE3) $(SCRIPTS_MORE4)

TEST_OUTFILES = $(SCRIPTS_FIRST) $(SCRIPTS) $(SCRIPTS_WIN32) $(SCRIPTS_GUI)
DOSTMP = dostmp
DOSTMP_OUTFILES = $(TEST_OUTFILES:test=dostmp\test)
DOSTMP_INFILES = $(DOSTMP_OUTFILES:.out=.in)

.SUFFIXES: .in .out .res .vim

nongui:	nolog $(SCRIPTS_FIRST) $(SCRIPTS) newtests report

small:	nolog report

gui:	nolog $(SCRIPTS_FIRST) $(SCRIPTS) $(SCRIPTS_GUI) newtests report

win32:	nolog $(SCRIPTS_FIRST) $(SCRIPTS) $(SCRIPTS_WIN32) newtests report

# Copy the input files to dostmp, changing the fileformat to dos.
$(DOSTMP_INFILES): $(*B).in
	if not exist $(DOSTMP)\NUL md $(DOSTMP)
	if exist $@ del $@
	$(VIMPROG) -u dos.vim $(NO_INITS) "+set ff=dos|f $@|wq" $(*B).in

# For each input file dostmp/test99.in run the tests.
# This moves test99.in to test99.in.bak temporarily.
$(TEST_OUTFILES): $(DOSTMP)\$(*B).in
	-@if exist test.out DEL test.out
	-@if exist $(DOSTMP)\$(*B).out DEL $(DOSTMP)\$(*B).out
	move $(*B).in $(*B).in.bak > nul
	copy $(DOSTMP)\$(*B).in $(*B).in > nul
	copy $(*B).ok test.ok > nul
	$(VIMPROG) -u dos.vim $(NO_INITS) -s dotest.in $(*B).in
	-@if exist test.out MOVE /y test.out $(DOSTMP)\$(*B).out > nul
	-@if exist $(*B).in.bak move /y $(*B).in.bak $(*B).in > nul
	-@if exist test.ok del test.ok
	-@if exist Xdir1 rd /s /q Xdir1
	-@if exist Xfind rd /s /q Xfind
	-@del X*
	-@if exist viminfo del viminfo
	$(VIMPROG) -u dos.vim $(NO_INITS) "+set ff=unix|f test.out|wq" \
		$(DOSTMP)\$(*B).out
	@diff test.out $*.ok & if errorlevel 1 \
		( move /y test.out $*.failed > nul \
		 & del $(DOSTMP)\$(*B).out \
		 & echo $* FAILED >> test.log ) \
		else ( move /y test.out $*.out > nul )

# Must run test1 first to create small.vim.
# This rule must come after the one that copies the input files to dostmp to
# allow for running an individual test.
$(SCRIPTS) $(SCRIPTS_GUI) $(SCRIPTS_WIN32) $(NEW_TESTS): $(SCRIPTS_FIRST)

report:
	@echo ""
	@echo Test results:
	@if exist test.log ( type test.log & echo TEST FAILURE & exit /b 1 ) \
		else ( echo ALL DONE )

clean:
	-del *.out
	-del *.failed
	-del *.res
	-if exist $(DOSTMP) rd /s /q $(DOSTMP)
	-if exist test.in del test.in
	-if exist test.ok del test.ok
	-if exist small.vim del small.vim
	-if exist tiny.vim del tiny.vim
	-if exist mbyte.vim del mbyte.vim
	-if exist mzscheme.vim del mzscheme.vim
	-if exist lua.vim del lua.vim
	-if exist Xdir1 rd /s /q Xdir1
	-if exist Xfind rd /s /q Xfind
	-del X*
	-for /d %i in (X*) do @rmdir /s/q %i
	-if exist viminfo del viminfo
	-if exist test.log del test.log
	-if exist messages del messages
	-if exist benchmark.out del benchmark.out
	-if exist opt_test.vim del opt_test.vim

nolog:
	-if exist test.log del test.log
	-if exist messages del messages

benchmark:
	bench_re_freeze.out

bench_re_freeze.out: bench_re_freeze.vim
	-if exist benchmark.out del benchmark.out
	$(VIMPROG) -u dos.vim $(NO_INITS) $*.in
	@IF EXIST benchmark.out ( type benchmark.out )

# New style of tests uses Vim script with assert calls.  These are easier
# to write and a lot easier to read and debug.
# Limitation: Only works with the +eval feature.

newtests: $(NEW_TESTS)

.vim.res:
	@echo "$(VIMPROG)" > vimcmd
	$(VIMPROG) -u NONE $(NO_INITS) -S runtest.vim $*.vim
	@del vimcmd

test_gui.res: test_gui.vim
	@echo "$(VIMPROG)" > vimcmd
	$(VIMPROG) -u NONE $(NO_INITS) -S runtest.vim $*.vim
	@del vimcmd

test_gui_init.res: test_gui_init.vim
	@echo "$(VIMPROG)" > vimcmd
	$(VIMPROG) -u gui_preinit.vim -U gui_init.vim $(NO_PLUGINS) -S runtest.vim $*.vim
	@del vimcmd

opt_test.vim: ../option.c gen_opt_test.vim
	$(VIMPROG) -u NONE -S gen_opt_test.vim --noplugin --not-a-term ../option.c
