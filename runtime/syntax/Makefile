# Portable Makefile for running syntax tests.

# Override this if needed, e.g. with ../../src/vim
VIMPROG = vim

# "runtime" relative to "runtime/syntax/testdir"
VIMRUNTIME = ../..

# Uncomment this line to use valgrind for memory leaks and extra warnings.
# VALGRIND = valgrind --tool=memcheck --leak-check=yes --num-callers=45 --log-file=valgrind.$*

# ENVVARS = LC_ALL=C LANG=C LANGUAGE=C

RUN_VIMTEST = VIMRUNTIME=$(VIMRUNTIME) $(VALGRIND) $(ENVVARS) ../$(VIMPROG) -f $(GUI_FLAG)

# Run the tests that didn't run yet or failed previously.
# If a test succeeds a testdir/done/{name} file will be written.
# If a test fails a testdir/failed/{name}.dump file will be written.
test:
	@# the "vimcmd" file is used by the screendump utils
	@echo "../$(VIMPROG)" > testdir/vimcmd
	@echo "$(RUN_VIMTEST)" >> testdir/vimcmd
	VIMRUNTIME=$(VIMRUNTIME) $(VIMPROG) --clean --not-a-term -u testdir/runtest.vim


clean testclean:
	rm -f testdir/failed/* testdir/done/* testdir/vimcmd
