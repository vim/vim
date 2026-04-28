#
# Makefile to run all tests for Vim
#
# Tests run in per-test isolated working directories under workdir/ so that
# they can execute in parallel without racing on shared temp files.  The
# layout for each test is a symlink tree that preserves relative-path depth:
#
#   workdir/                     symlinks to vim root (runtime/, ...)
#   workdir/$test/               symlinks to src/ contents
#   workdir/$test/testdir/       symlinks to testdir/ contents, test cwd
#
# Tests cd into workdir/$test/testdir/ so '../' resolves to src/ and
# '../../' resolves to the vim root, matching the real directory layout.
# The nolog target creates the level-1 (vim root) symlinks once; the
# setup_workdir.sh script creates per-test levels 2 and 3.

# Use console or GUI.
VIMPROG = ../vim
XXDPROG = ../xxd/xxd
# VIMPROG = ../gvim

SCRIPTSOURCE = ../../runtime

# Comment out this line to see the verbose output of tests.
#
# Catches SwapExists to avoid hanging at the ATTENTION prompt.
REDIR_TEST_TO_NULL = --cmd 'au SwapExists * let v:swapchoice = "e"' | LC_ALL=C awk '/Executing Test_/{match($$0, "([0-9][0-9]:[0-9][0-9] *)?Executing Test_[^\\)]*\\)"); print substr($$0, RSTART, RLENGTH) "\r"; fflush()}'

# Uncomment this line to use valgrind for memory leaks and extra warnings.
#   The output goes into a file "valgrind.testN"
#   Vim should be compiled with EXITFREE to avoid false warnings.
#   This will make testing about 10 times as slow.
# VALGRIND = valgrind --tool=memcheck --leak-check=yes --num-callers=45 --log-file=valgrind.$*

# To execute one test, e.g. in gdb, use something like this:
# run -f -u util/unix.vim --gui-dialog-file guidialog -U NONE --noplugin --not-a-term -S runtest.vim test_name.vim

default: nongui

# The list of tests is common to all systems.
# This defines SCRIPTS_TINY_OUT, NEW_TESTS and NEW_TESTS_RES.
include Make_all.mak

# Explicit dependencies.
test_options_all.res: opt_test.vim

.SUFFIXES: .in .out .res .vim

# Ordering: nolog must run before any tests, report must run after all tests.
# The individual tests within tinytests/newtests can run in parallel.
#
# We use recursive make to enforce sequencing (nolog → tests → report)
# without GNU order-only prerequisites.  Each @$(MAKE) line completes
# before the next starts; the middle one inherits -j for parallelism.
nongui:
	@$(MAKE) nolog
	@$(MAKE) tinytests newtests
	@$(MAKE) report

gui:
	@$(MAKE) nolog
	@$(MAKE) tinytests newtests
	@$(MAKE) report

tiny:
	@$(MAKE) nolog
	@$(MAKE) tinytests
	@$(MAKE) report_tiny

benchmark: $(SCRIPTS_BENCH)

# Assumes nolog and tinytests have already run (called via recursive
# make from the tiny target above).  Do not invoke directly.
report_tiny:
	@# Collect per-test failure logs.
	@/bin/sh -c 'cat workdir/*/test.log > test.log 2>/dev/null || true'
	@/bin/sh -c "if test -s test.log; \
		then cp test.log test_result.log; \
		else echo No failures reported > test_result.log; \
		fi"
	@rm -f starttime
	@echo
	@echo 'Test results:'
	@cat test_result.log
	@/bin/sh -c "if test -s test.log; \
		then echo TEST FAILURE; exit 1; \
		else echo ALL DONE; \
		fi"

# Assumes nolog, tinytests, and newtests have already run (called via
# recursive make from nongui/gui above).  Do not invoke directly.
report:
	@# Collect per-test output from workdirs.
	@/bin/sh -c 'cat workdir/*/testdir/messages > messages 2>/dev/null || true'
	@/bin/sh -c 'cat workdir/*/testdir/test.log > test.log 2>/dev/null || true'
	@# Collect per-test failed screendump directories so they are
	@# available in the usual testdir/failed/ location for debugging.
	@/bin/sh -c 'for d in workdir/*/testdir/failed; do \
		test -d "$$d" || continue; \
		mkdir -p failed; \
		cp -r "$$d"/* failed/ 2>/dev/null; \
	done || true'
	@# without the +eval feature test_result.log is a copy of test.log
	@/bin/sh -c "if test -s test.log; \
		then cp test.log test_result.log; \
		else echo No failures reported > test_result.log; \
		fi"
	$(VIMPROG) -u NONE $(NO_INITS) -S util/summarize.vim messages $(REDIR_TEST_TO_NULL)
	@rm -f starttime
	@echo
	@echo 'Test results:'
	@cat test_result.log
	@/bin/sh -c "if test -s test.log; \
		then echo TEST FAILURE; exit 1; \
		else echo ALL DONE; \
		fi"

$(SCRIPTS_TINY_OUT) $(NEW_TESTS_RES): $(VIMPROG)

# For $ASAN_OPTIONS and $UBSAN_OPTIONS append the testname to it.
# This assumes $ASAN_OPTIONS contains log_path as last part of the environment variable
# For Github CI, those variables are set in .github/workflows/ci.yml

# Execute an individual new style test, e.g.:
# 	make test_largefile
$(NEW_TESTS):
	rm -f $@.res $(CLEANUP_FILES)
	-rm -rf workdir
	@MAKEFLAGS=--no-print-directory $(MAKE) -f Makefile $@.res VIMPROG=$(VIMPROG) XXDPROG=$(XXDPROG) SCRIPTSOURCE=$(SCRIPTSOURCE)
	@/bin/sh -c 'cat workdir/*/messages 2>/dev/null || true'
	@/bin/sh -c 'if cat workdir/*/test.log 2>/dev/null; then exit 1; fi'

codestyle:
	rm -f test_codestyle.res $(CLEANUP_FILES)
	-rm -rf workdir
	@MAKEFLAGS=--no-print-directory $(MAKE) -f Makefile test_codestyle.res VIMPROG=$(VIMPROG) XXDPROG=$(XXDPROG) SCRIPTSOURCE=$(SCRIPTSOURCE)
	@/bin/sh -c 'cat workdir/*/messages 2>/dev/null || true'
	@/bin/sh -c 'if cat workdir/*/test.log 2>/dev/null; then exit 1; fi'

# Run only tests specific for Vim9 script
test_vim9:
	rm -f test_vim9_*.res $(CLEANUP_FILES)
	-rm -rf workdir
	@MAKEFLAGS=--no-print-directory $(MAKE) -f Makefile $(TEST_VIM9_RES) VIMPROG=$(VIMPROG) XXDPROG=$(XXDPROG) SCRIPTSOURCE=$(SCRIPTSOURCE)
	@/bin/sh -c 'cat workdir/*/messages > messages 2>/dev/null || true'
	@/bin/sh -c 'cat workdir/*/test.log > test.log 2>/dev/null || true'
	@rm -f starttime
	@MAKEFLAGS=--no-print-directory $(MAKE) -f Makefile report VIMPROG=$(VIMPROG) XXDPROG=$(XXDPROG) SCRIPTSOURCE=$(SCRIPTSOURCE)
	@if test -f test.log; then \
		exit 1; \
	fi

RUN_VIMPROG = VIMRUNTIME=$(SCRIPTSOURCE) $(VALGRIND) $(VIMPROG) -f $(GUI_FLAG) -u util/unix.vim $(NO_INITS) -s dotest.in

# Delete files that may interfere with running tests.  This includes some files
# that may result from working on the tests, not only from running them.
clean:
	-rm -rf *.out *.failed *.res *.rej *.orig XfakeHOME Xdir1 Xfind failed
	-rm -f opt_test.vim test_result.log $(CLEANUP_FILES)
	-rm -f gen_opt_test.log
	-rm -rf test.out test.ok benchmark.out X* viminfo
	-rm -f valgrind.*
	-rm -f asan.* asan_test_*
	-rm -f guidialog guidialogfile
	-rm -rf workdir

# Delete the files produced by benchmarking, so they can run again.
benchmarkclean:
	rm -f $(SCRIPTS_BENCH)

nolog:
	-rm -f test_result.log $(CLEANUP_FILES)
	-rm -rf workdir
	@# Level 1: vim root (runtime/, etc.) for ../../ references.
	@# Created here once, not per-test in setup_workdir.sh, because
	@# ln -s follows existing symlinks to directories: if workdir/runtime
	@# already exists as a symlink, "ln -s .../runtime workdir/runtime"
	@# creates runtime/runtime inside the real runtime/ dir.  Neither
	@# ln -sf nor rm+ln nor test -L guards fix this under make -j
	@# (ln -sf and bare ln -s both follow; rm+ln races).  Running the
	@# loop once here, serially before any tests start, avoids the issue.
	@TESTDIR=`pwd`; mkdir -p workdir; \
	for f in "$$TESTDIR"/../../*; do \
		b=$$(basename "$$f"); \
		case "$$b" in src) continue;; esac; \
		ln -s "$$f" workdir/"$$b" 2>/dev/null || true; \
	done
	@# Record the start time once so parallel tests don't race on it.
	@date +%s > starttime


# Tiny tests.  Works even without the +eval feature.
tinytests: $(SCRIPTS_TINY_OUT)

# Each old-style test runs in its own workdir to avoid races on shared
# filenames (test.ok, test.out, X*, viminfo).
.in.out:
	-rm -rf $*.failed workdir/$*
	@sh setup_workdir.sh $*
	cp $*.ok workdir/$*/testdir/test.ok
	@# Sleep a moment to avoid that the xterm title is messed up.
	@# 200 msec is sufficient, but only modern sleep supports a fraction of
	@# a second, fall back to a second if it fails.
	@-/bin/sh -c "sleep .2 > /dev/null 2>&1 || sleep 1"
	@TESTDIR=`pwd`; \
	if test -n "$${ASAN_OPTIONS}"; then \
		cd workdir/$*/testdir && ASAN_OPTIONS="$${ASAN_OPTIONS}_$*" UBSAN_OPTIONS="$${UBSAN_OPTIONS}_$*" VIMRUNTIME=$$TESTDIR/$(SCRIPTSOURCE) $(VALGRIND) $$TESTDIR/$(VIMPROG) -f $(GUI_FLAG) -u $$TESTDIR/util/unix.vim $(NO_INITS) -s $$TESTDIR/dotest.in $$TESTDIR/$*.in $(REDIR_TEST_TO_NULL) ; \
	else \
		cd workdir/$*/testdir && VIMRUNTIME=$$TESTDIR/$(SCRIPTSOURCE) $(VALGRIND) $$TESTDIR/$(VIMPROG) -f $(GUI_FLAG) -u $$TESTDIR/util/unix.vim $(NO_INITS) -s $$TESTDIR/dotest.in $$TESTDIR/$*.in $(REDIR_TEST_TO_NULL) ; \
	fi

	@# Check if the test.out file matches test.ok.
	@/bin/sh -c "if test -f workdir/$*/testdir/test.out; then \
		  if diff workdir/$*/testdir/test.out $*.ok; \
		  then mv -f workdir/$*/testdir/test.out $*.out; \
		  else echo $* FAILED >>workdir/$*/test.log; mv -f workdir/$*/testdir/test.out $*.failed; \
		  fi \
		else echo $* NO OUTPUT >>workdir/$*/test.log; \
		fi"
	@/bin/sh -c "if test -f workdir/$*/testdir/valgrind; then\
		  mv -f workdir/$*/testdir/valgrind valgrind.$*; \
		fi"
	-rm -rf workdir/$*/testdir/X* workdir/$*/testdir/test.ok workdir/$*/testdir/viminfo


# New style of tests uses Vim script with assert calls.  These are easier
# to write and a lot easier to read and debug.
# Limitation: Only works with the +eval feature.
# Add --gui-dialog-file to avoid getting stuck in a dialog.
RUN_VIMTEST = VIMRUNTIME=$(SCRIPTSOURCE) $(VALGRIND) $(VIMPROG) -f $(GUI_FLAG) -u util/unix.vim --gui-dialog-file guidialog

newtests: $(NEW_TESTS_RES)

# Each new-style test runs with TEST_WORKDIR set so that runtest.vim
# creates temp files in an isolated directory.
.vim.res:
	@sh setup_workdir.sh $*
	@TESTDIR=`pwd`; \
	echo "$$TESTDIR/$(VIMPROG)" > workdir/$*/testdir/vimcmd; \
	echo "VIMRUNTIME=$$TESTDIR/$(SCRIPTSOURCE) $(VALGRIND) $$TESTDIR/$(VIMPROG) -f $(GUI_FLAG) -u $$TESTDIR/util/unix.vim --gui-dialog-file guidialog" >> workdir/$*/testdir/vimcmd
	@TESTDIR=`pwd`; \
	if test -n "$${ASAN_OPTIONS}"; then \
		ASAN_OPTIONS="$${ASAN_OPTIONS}_$*" UBSAN_OPTIONS="$${UBSAN_OPTIONS}_$*" TEST_WORKDIR=$$TESTDIR/workdir/$*/testdir $(RUN_VIMTEST) $(NO_INITS) -S runtest.vim $*.vim $(REDIR_TEST_TO_NULL) ; \
	else \
		TEST_WORKDIR=$$TESTDIR/workdir/$*/testdir $(RUN_VIMTEST) $(NO_INITS) -S runtest.vim $*.vim $(REDIR_TEST_TO_NULL) ; \
	fi
	@rm -f workdir/$*/testdir/vimcmd

test_gui.res: test_gui.vim
	@sh setup_workdir.sh $*
	@TESTDIR=`pwd`; \
	echo "$$TESTDIR/$(VIMPROG)" > workdir/$*/testdir/vimcmd; \
	echo "$(RUN_GVIMTEST)" >> workdir/$*/testdir/vimcmd
	@TESTDIR=`pwd`; \
	if test -n "$${ASAN_OPTIONS}"; then \
		ASAN_OPTIONS="$${ASAN_OPTIONS}_$*" UBSAN_OPTIONS="$${UBSAN_OPTIONS}_$*" TEST_WORKDIR=$$TESTDIR/workdir/$*/testdir $(RUN_VIMTEST) -u NONE $(NO_INITS) -S runtest.vim $< ; \
	else \
		TEST_WORKDIR=$$TESTDIR/workdir/$*/testdir $(RUN_VIMTEST) -u NONE $(NO_INITS) -S runtest.vim $< ; \
	fi

	@rm -f workdir/$*/testdir/vimcmd

test_gui_init.res: test_gui_init.vim
	@sh setup_workdir.sh $*
	@TESTDIR=`pwd`; \
	echo "$$TESTDIR/$(VIMPROG)" > workdir/$*/testdir/vimcmd; \
	echo "$(RUN_GVIMTEST_WITH_GVIMRC)" >> workdir/$*/testdir/vimcmd
	@TESTDIR=`pwd`; \
	if test -n "$${ASAN_OPTIONS}"; then \
		ASAN_OPTIONS="$${ASAN_OPTIONS}_$*" UBSAN_OPTIONS="$${UBSAN_OPTIONS}_$*" TEST_WORKDIR=$$TESTDIR/workdir/$*/testdir $(RUN_VIMTEST) -u util/gui_preinit.vim -U util/gui_init.vim $(NO_PLUGINS) -S runtest.vim $< ; \
	else \
		TEST_WORKDIR=$$TESTDIR/workdir/$*/testdir $(RUN_VIMTEST) -u util/gui_preinit.vim -U util/gui_init.vim $(NO_PLUGINS) -S runtest.vim $< ; \
	fi
	@rm -f workdir/$*/testdir/vimcmd

GEN_OPT_DEPS = util/gen_opt_test.vim ../optiondefs.h ../../runtime/doc/options.txt

opt_test.vim: $(GEN_OPT_DEPS)
	if test -n "$${ASAN_OPTIONS}"; then \
		ASAN_OPTIONS="$${ASAN_OPTIONS}_$*" UBSAN_OPTIONS="$${UBSAN_OPTIONS}_$*" $(VIMPROG) -e -s -u NONE $(NO_INITS) --nofork --gui-dialog-file guidialog -S $(GEN_OPT_DEPS) ; \
	else \
		$(VIMPROG) -e -s -u NONE $(NO_INITS) --nofork --gui-dialog-file guidialog -S $(GEN_OPT_DEPS) ; \
	fi
	@if test -f gen_opt_test.log; then \
		cat gen_opt_test.log; \
		exit 1; \
	fi

test_xxd.res:
	@sh setup_workdir.sh $*
	@TESTDIR=`pwd`; \
	if test -n "$${ASAN_OPTIONS}"; then \
		XXD=$(XXDPROG); export XXD; ASAN_OPTIONS="$${ASAN_OPTIONS}_$*" UBSAN_OPTIONS="$${UBSAN_OPTIONS}_$*" TEST_WORKDIR=$$TESTDIR/workdir/$*/testdir $(RUN_VIMTEST) $(NO_INITS) -S runtest.vim test_xxd.vim ; \
	else \
		XXD=$(XXDPROG); export XXD; TEST_WORKDIR=$$TESTDIR/workdir/$*/testdir $(RUN_VIMTEST) $(NO_INITS) -S runtest.vim test_xxd.vim ; \
	fi

test_bench_regexp.res: test_bench_regexp.vim
	@sh setup_workdir.sh $*
	@# Sleep a moment to avoid that the xterm title is messed up.
	@# 200 msec is sufficient, but only modern sleep supports a fraction of
	@# a second, fall back to a second if it fails.
	@-/bin/sh -c "sleep .2 > /dev/null 2>&1 || sleep 1"
	@TESTDIR=`pwd`; \
	if test -n "$${ASAN_OPTIONS}"; then \
		ASAN_OPTIONS="$${ASAN_OPTIONS}_$*" UBSAN_OPTIONS="$${UBSAN_OPTIONS}_$*" TEST_WORKDIR=$$TESTDIR/workdir/$*/testdir $(RUN_VIMTEST) $(NO_INITS) -S runtest.vim $*.vim $(REDIR_TEST_TO_NULL) ; \
	else \
		TEST_WORKDIR=$$TESTDIR/workdir/$*/testdir $(RUN_VIMTEST) $(NO_INITS) -S runtest.vim $*.vim $(REDIR_TEST_TO_NULL) ; \
	fi
	@/bin/sh -c "if test -f workdir/$*/testdir/benchmark.out; then cat workdir/$*/testdir/benchmark.out; fi"
