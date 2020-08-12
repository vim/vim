#
# Makefile to run all tests for Vim, on Dos-like machines
# with sh.exe or zsh.exe in the path or not.
#
# Author: Bill McCarthy
#
# Requires a set of Unix tools: echo, diff, etc.

# Don't use unix-like shell.
SHELL = cmd.exe

DEL = del
DELDIR = rd /s /q
MV = move /y
CP = copy /y
CAT = type

VIMPROG = ..\\vim

default: nongui

include Make_all.mak

TEST_OUTFILES = $(SCRIPTS_TINY)
DOSTMP = dostmp
# Keep $(DOSTMP)/*.in
.PRECIOUS: $(patsubst %.out, $(DOSTMP)/%.in, $(TEST_OUTFILES))

.SUFFIXES: .in .out .res .vim

tiny:	nolog $(SCRIPTS_TINY) report

nongui:	nolog $(SCRIPTS_TINY) newtests report

gui:	nolog $(SCRIPTS_TINY) newtests report

benchmark: $(SCRIPTS_BENCH)

report:
	@rem without the +eval feature test_result.log is a copy of test.log
	@if exist test.log ( copy /y test.log test_result.log > nul ) \
		else ( echo No failures reported > test_result.log )
	$(VIMPROG) -u NONE $(NO_INITS) -S summarize.vim messages
	@echo.
	@echo Test results:
	@cmd /c type test_result.log
	@if exist test.log ( echo TEST FAILURE & exit /b 1 ) \
		else ( echo ALL DONE )

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

# Copy the input files to dostmp, changing the fileformat to dos.
$(DOSTMP)/%.in : %.in
	if not exist $(DOSTMP)\nul mkdir $(DOSTMP)
	if not exist $@ $(DEL) $@
	$(VIMPROG) -u dos.vim $(NO_INITS) "+set ff=dos|f $@|wq" $<

%.out : $(DOSTMP)/%.in
	-@if exist test.out $(DEL) test.out
	-@if exist $(DOSTMP)\$@ $(DEL) $(DOSTMP)\$@
	$(MV) $(notdir $<) $(notdir $<).bak > NUL
	$(CP) $(DOSTMP)\$(notdir $<) $(notdir $<) > NUL
	$(CP) $(basename $@).ok test.ok > NUL
	$(VIMPROG) -u dos.vim $(NO_INITS) -s dotest.in $(notdir $<)
	-@if exist test.out $(MV) test.out $(DOSTMP)\$@ > NUL
	-@if exist $(notdir $<).bak $(MV) $(notdir $<).bak $(notdir $<) > NUL
	-@if exist test.ok $(DEL) test.ok
	-@if exist Xdir1 $(DELDIR) /s /q Xdir1
	-@if exist Xfind $(DELDIR) Xfind
	-@if exist XfakeHOME $(DELDIR) XfakeHOME
	-@del X*
	-@if exist viminfo del viminfo
	$(VIMPROG) -u dos.vim $(NO_INITS) "+set ff=unix|f test.out|wq" \
		$(DOSTMP)\$@
	@diff test.out $(basename $@).ok & if errorlevel 1 \
		( $(MV) test.out $(basename $@).failed > NUL \
		 & del $(DOSTMP)\$@ \
		 & echo $(basename $@) FAILED >> test.log ) \
		else ( $(MV) test.out $(basename $@).out > NUL )

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
