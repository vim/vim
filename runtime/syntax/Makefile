# Portable Makefile for running syntax tests.

# To run the test manually:
# ../../src/vim -u 'testdir/runtest.vim' --cmd 'breakadd func RunTest'

# Override this if needed, the default assumes Vim was build in the src dir.
#VIMPROG = vim
VIMPROG = ../../src/vim

# "runtime" relative to "runtime/syntax/testdir"
VIMRUNTIME = ../..

# Uncomment this line to use valgrind for memory leaks and extra warnings.
# VALGRIND = valgrind --tool=memcheck --leak-check=yes --num-callers=45 --log-file=valgrind.$*

# ENVVARS = LC_ALL=C LANG=C LANGUAGE=C
# Run the syntax tests with a C locale
ENVVARS = LC_ALL=C

RUN_VIMTEST = VIMRUNTIME=$(VIMRUNTIME) $(VALGRIND) $(ENVVARS) ../$(VIMPROG) -f $(GUI_FLAG)

# Uncomment this line for debugging
# DEBUGLOG = --log testlog

# Run the tests that didn't run yet or failed previously.
# If a test succeeds a testdir/done/{name} file will be written.
# If a test fails a testdir/failed/{name}.dump file will be written.
# Progress and error messages can be found in "testdir/messages".
# Comment out the "> /dev/null" part to see the in-progress Vim behaviors.
test:
	@# the "vimcmd" file is used by the screendump utils
	@echo "../$(VIMPROG)" > testdir/vimcmd
	@echo "$(RUN_VIMTEST)" >> testdir/vimcmd
	VIMRUNTIME=$(VIMRUNTIME) $(ENVVARS) $(VIMPROG) --clean --not-a-term $(DEBUGLOG) -u testdir/runtest.vim > /dev/null 
	@rm -f testdir/Xfilter
	@# FIXME: Temporarily show the whole file to find out what goes wrong
	@#if [ -f testdir/messages ]; then tail -n 6 testdir/messages; fi
	@if [ -f testdir/messages ]; then cat testdir/messages; fi


clean testclean:
	rm -f testdir/failed/* testdir/done/* testdir/vimcmd testdir/messages testdir/Xtestscript testdir/Xfilter

# All initial phony targets; these names may clash with file extensions.
phonies = clean test testclean

# Collect all input filenames and their file extensions.
testnames != set +f; \
awk 'BEGIN { \
    for (i = 1; i < ARGC; i++) { \
        split(ARGV[i], names, /\//); \
        split(names[3], parts, /\./); \
        exts[parts[2]]; \
        print names[3]; \
    } \
    split("$(phonies)", scratch); \
    for (phony in scratch) \
        phonies[scratch[phony]]; \
    for (ext in exts) \
        print ext ((ext in phonies) ? "_" : ""); \
}' testdir/input/*.*

.PHONY: self-testing $(testnames)

$(testnames)::
	@echo $@ >> testdir/Xfilter

self-testing:: $(testnames)
	@echo self-testing > testdir/Xfilter
