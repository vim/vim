# Portable Makefile for running indent tests.

.SUFFIXES:
.PHONY: test clean testclean

VIMPROG = vim
VIMRUNTIME = ..

# Run the tests that didn't run yet or failed previously.
# If a test succeeds a testdir/*.out file will be written.
# If a test fails a testdir/*.fail file will be written.
test:
	VIMRUNTIME=$(VIMRUNTIME) $(VIMPROG) --clean --not-a-term -u testdir/runtest.vim
	for file in testdir/*.fail; do \
		if test -f "$$file"; then \
			diff -Nu $${file%.*}.in $${file}; \
	  fi; \
	done


clean testclean:
	rm -f testdir/*.fail testdir/*.out
