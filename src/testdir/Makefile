#
# Makefile to run all tests for Vim
#

# Use console or GUI.
VIMPROG = ../vim
# VIMPROG = ../gvim

SCRIPTSOURCE = ../../runtime

# Uncomment this line to use valgrind for memory leaks and extra warnings.
#   The output goes into a file "valgrind.testN"
#   Vim should be compiled with EXITFREE to avoid false warnings.
#   This will make testing about 10 times as slow.
# VALGRIND = valgrind --tool=memcheck --leak-check=yes --num-callers=25 --log-file=valgrind.$*

default: nongui

# The list of tests is common to all systems.
# This defines NEW_TESTS, SCRIPTS_ALL, SCRIPTS_MORE* and SCRIPTS_GUI.
include Make_all.mak


SCRIPTS = $(SCRIPTS_ALL) \
	  $(SCRIPTS_MORE1) \
	  $(SCRIPTS_MORE2) \
	  $(SCRIPTS_MORE3) \
	  $(SCRIPTS_MORE4)

SCRIPTS_BENCH = bench_re_freeze.out

.SUFFIXES: .in .out .res .vim

nongui:	nolog $(SCRIPTS_FIRST) $(SCRIPTS) newtests report

gui:	nolog $(SCRIPTS_FIRST) $(SCRIPTS) $(SCRIPTS_GUI) newtests report

benchmark: $(SCRIPTS_BENCH)

report:
	@echo
	@echo 'Test results:'
	@/bin/sh -c "if test -f test.log; \
		then cat test.log; echo TEST FAILURE; exit 1; \
		else echo ALL DONE; \
		fi"

$(SCRIPTS_FIRST) $(SCRIPTS) $(SCRIPTS_GUI) $(NEW_TESTS): $(VIMPROG)

# Must run test1 first to create small.vim.
$(SCRIPTS) $(SCRIPTS_GUI) $(NEW_TESTS): $(SCRIPTS_FIRST)

RM_ON_RUN = test.out X* viminfo
RM_ON_START = tiny.vim small.vim mbyte.vim mzscheme.vim lua.vim test.ok benchmark.out
RUN_VIM = VIMRUNTIME=$(SCRIPTSOURCE); export VIMRUNTIME; $(VALGRIND) $(VIMPROG) -f $(GUI_FLAG) -u unix.vim $(NO_PLUGIN) -s dotest.in

clean:
	-rm -rf *.out *.failed *.res *.rej *.orig test.log messages $(RM_ON_RUN) $(RM_ON_START) valgrind.*

test1.out: test1.in
	-rm -rf $*.failed $(RM_ON_RUN) $(RM_ON_START) wrongtermsize
	$(RUN_VIM) $*.in
	@/bin/sh -c "if test -f wrongtermsize; \
		then echo; \
		echo test1 FAILED - terminal size must be 80x24 or larger; \
		echo; exit 1; \
		elif diff test.out $*.ok; \
		then mv -f test.out $*.out; \
		else echo; \
		echo test1 FAILED - Something basic is wrong; \
		echo; exit 1; fi"
	-rm -rf X* viminfo

.in.out:
	-rm -rf $*.failed test.ok $(RM_ON_RUN)
	cp $*.ok test.ok
	# Sleep a moment to avoid that the xterm title is messed up.
	# 200 msec is sufficient, but only modern sleep supports a fraction of
	# a second, fall back to a second if it fails.
	@-/bin/sh -c "sleep .2 > /dev/null 2>&1 || sleep 1"
	-$(RUN_VIM) $*.in

	# For flaky tests retry one time.  No tests at the moment.
	#@/bin/sh -c "if test -f test.out -a $* = test61; then \
	#	  if diff test.out $*.ok; \
	#	  then echo flaky test ok first time; \
	#	  else rm -rf $*.failed $(RM_ON_RUN); \
	#		$(RUN_VIM) $*.in; \
	#	  fi \
	#	fi"

	# Check if the test.out file matches test.ok.
	@/bin/sh -c "if test -f test.out; then \
		  if diff test.out $*.ok; \
		  then mv -f test.out $*.out; \
		  else echo $* FAILED >>test.log; mv -f test.out $*.failed; \
		  fi \
		else echo $* NO OUTPUT >>test.log; \
		fi"
	@/bin/sh -c "if test -f valgrind; then\
		  mv -f valgrind valgrind.$*; \
		fi"
	-rm -rf X* test.ok viminfo

bench_re_freeze.out: bench_re_freeze.vim
	-rm -rf benchmark.out $(RM_ON_RUN)
	# Sleep a moment to avoid that the xterm title is messed up.
	# 200 msec is sufficient, but only modern sleep supports a fraction of
	# a second, fall back to a second if it fails.
	@-/bin/sh -c "sleep .2 > /dev/null 2>&1 || sleep 1"
	-$(RUN_VIM) $*.in
	@/bin/sh -c "if test -f benchmark.out; then cat benchmark.out; fi"

nolog:
	-rm -f test.log messages


# New style of tests uses Vim script with assert calls.  These are easier
# to write and a lot easier to read and debug.
# Limitation: Only works with the +eval feature.
RUN_VIMTEST = VIMRUNTIME=$(SCRIPTSOURCE); export VIMRUNTIME; $(VALGRIND) $(VIMPROG) -f $(GUI_FLAG) -u unix.vim $(NO_PLUGIN)

newtests: newtestssilent
	@/bin/sh -c "if test -f messages && grep -q 'SKIPPED\|FAILED' messages; then cat messages && if test -f test.log; then cat test.log; fi ; fi"

newtestssilent: $(NEW_TESTS)


.vim.res:
	@echo "$(RUN_VIMTEST)" > vimcmd
	$(RUN_VIMTEST) -U NONE -S runtest.vim $*.vim
	@rm vimcmd
