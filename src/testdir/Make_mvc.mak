#
# Makefile to run all tests for Vim, on Dos-like machines.
#
# Requires a set of Unix tools: echo, diff, etc.
#

# included common tools
!INCLUDE ..\auto\nmake\tools.mak

# Testing may be done with a debug build 
!IF EXIST(..\\vimd.exe) && !EXIST(..\\vim.exe)
VIMPROG = ..\\vimd.exe
!ELSE
VIMPROG = ..\\vim.exe
!ENDIF

DIFF = diff.exe

default: nongui

!INCLUDE .\Make_all.mak

# Explicit dependencies.
test_options_all.res: opt_test.vim

TEST_OUTFILES = $(SCRIPTS_TINY_OUT)
DOSTMP = dostmp
DOSTMP_OUTFILES = $(TEST_OUTFILES:test=dostmp\test)
DOSTMP_INFILES = $(DOSTMP_OUTFILES:.out=.in)

.SUFFIXES: .in .out .res .vim

# Add --gui-dialog-file to avoid getting stuck in a dialog.
COMMON_ARGS = $(NO_INITS) --gui-dialog-file guidialog

nongui:	nolog tinytests newtests report

gui:	nolog tinytests newtests report

tiny:	nolog tinytests report

benchmark: $(SCRIPTS_BENCH)

report:
	@ rem without the +eval feature test_result.log is a copy of test.log
	@ if exist test.log ( $(CP) test.log test_result.log > nul ) \
		else ( echo No failures reported > test_result.log )
	$(VIMPROG) -u NONE $(COMMON_ARGS) -S util\summarize.vim messages
	- if exist starttime $(RM) starttime
	@ echo:
	@ echo Test results:
	@ $(CMD) /C type test_result.log
	@ if exist test.log ( echo TEST FAILURE & exit /b 1 ) \
		else ( echo ALL DONE )


# Execute an individual new style test, e.g.:
# 	nmake -f Make_mvc.mak test_largefile
$(NEW_TESTS):
	- if exist $@.res $(RM) $@.res
	- if exist test.log $(RM) test.log
	- if exist messages $(RM) messages
	- if exist starttime $(RM) starttime
	@ $(MAKE) -lf Make_mvc.mak VIMPROG=$(VIMPROG) $@.res
	@ type messages
	@ if exist test.log exit 1


# Delete files that may interfere with running tests.  This includes some files
# that may result from working on the tests, not only from running them.
clean:
	- if exist *.out $(RM) *.out
	- if exist *.failed $(RM) *.failed
	- if exist *.res $(RM) *.res
	- if exist $(DOSTMP) $(RD) $(DOSTMP)
	- if exist test.in $(RM) test.in
	- if exist test.ok $(RM) test.ok
	- if exist Xdir1 $(RD) Xdir1
	- if exist Xfind $(RD) Xfind
	- if exist XfakeHOME $(RD) XfakeHOME
	- if exist X* $(RM) X*
	- for /d %i in (X*) do @$(RD) %i
	- if exist viminfo $(RM) viminfo
	- if exist test.log $(RM) test.log
	- if exist test_result.log $(RM) test_result.log
	- if exist messages $(RM) messages
	- if exist starttime $(RM) starttime
	- if exist benchmark.out $(RM) benchmark.out
	- if exist opt_test.vim $(RM) opt_test.vim
	- if exist guidialog $(RM) guidialog
	- if exist guidialogfile $(RM) guidialogfile

nolog:
	- if exist test.log $(RM) test.log
	- if exist test_result.log $(RM) test_result.log
	- if exist messages $(RM) messages
	- if exist starttime $(RM) starttime


# Tiny tests.  Works even without the +eval feature.
tinytests: $(SCRIPTS_TINY_OUT)

# Copy the input files to dostmp, changing the fileformat to dos.
$(DOSTMP_INFILES): $(*B).in
	if not exist $(DOSTMP)\NUL $(MKD) $(DOSTMP)
	if exist $@ $(RM) $@
	$(VIMPROG) -u util\dos.vim $(COMMON_ARGS) "+set ff=dos|f $@|wq" $(*B).in

# For each input file dostmp/test99.in run the tests.
# This moves test99.in to test99.in.bak temporarily.
$(TEST_OUTFILES): $(DOSTMP)\$(*B).in
	-@ if exist test.out $(RM) test.out
	-@ if exist $(DOSTMP)\$(*B).out $(RM) $(DOSTMP)\$(*B).out
	$(MV) $(*B).in $(*B).in.bak > nul
	$(CP) $(DOSTMP)\$(*B).in $(*B).in > nul
	$(CP) $(*B).ok test.ok > nul
	$(VIMPROG) -u util\dos.vim $(COMMON_ARGS) -s dotest.in $(*B).in
	-@ if exist test.out $(MV) test.out $(DOSTMP)\$(*B).out > nul
	-@ if exist $(*B).in.bak $(MV) $(*B).in.bak $(*B).in > nul
	-@ if exist test.ok $(RM) test.ok
	-@ if exist Xdir1 $(RD) Xdir1
	-@ if exist Xfind $(RD) Xfind
	-@ if exist XfakeHOME $(RD) XfakeHOME
	-@ $(RM) X*
	-@ if exist viminfo $(RM) viminfo
	$(VIMPROG) -u util\dos.vim $(COMMON_ARGS) "+set ff=unix|f test.out|wq" \
		$(DOSTMP)\$(*B).out
	@ $(DIFF) test.out $*.ok & if errorlevel 1 \
		( $(MV) test.out $*.failed > nul \
		 & $(RM) $(DOSTMP)\$(*B).out \
		 & echo $* FAILED >> test.log ) \
		else ( $(MV) test.out $*.out > nul )


# New style of tests uses Vim script with assert calls.  These are easier
# to write and a lot easier to read and debug.
# Limitation: Only works with the +eval feature.

newtests: newtestssilent
	@ if exist messages type messages

newtestssilent: $(NEW_TESTS_RES)

.vim.res:
	@ echo $(VIMPROG) > vimcmd
	$(VIMPROG) -u NONE $(COMMON_ARGS) -S runtest.vim $*.vim
	@ $(RM) vimcmd

test_gui.res: test_gui.vim
	@ echo $(VIMPROG) > vimcmd
	$(VIMPROG) -u NONE $(COMMON_ARGS) -S runtest.vim $*.vim
	@ $(RM) vimcmd

test_gui_init.res: test_gui_init.vim
	@ echo $(VIMPROG) > vimcmd
	$(VIMPROG) -u util\gui_preinit.vim -U util\gui_init.vim $(NO_PLUGINS) \
		-S runtest.vim $*.vim
	@ $(RM) vimcmd

opt_test.vim: util/gen_opt_test.vim ../optiondefs.h \
		../../runtime/doc/options.txt
	$(VIMPROG) -e -s -u NONE $(COMMON_ARGS) --nofork -S $**
	@ if exist test.log ( type test.log & exit /b 1 )

test_bench_regexp.res: test_bench_regexp.vim
	- if exist benchmark.out $(RM) benchmark.out
	@ echo $(VIMPROG) > vimcmd
	$(VIMPROG) -u NONE $(COMMON_ARGS) -S runtest.vim $*.vim
	@ $(RM) vimcmd
	@ if exist benchmark.out ( type benchmark.out )

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=79 ft=make:
