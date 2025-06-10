# Portable Makefile for running indent tests.

.SUFFIXES:
.PHONY: test clean testclean

VIMPROG = vim
VIMRUNTIME = ..

# Run the tests that didn't run yet or failed previously.
# If a test succeeds a testdir/*.out file will be written.
# If a test fails a testdir/*.fail file will be written.
test:
	VIMRUNTIME=$(VIMRUNTIME) $(VIMPROG) --clean --not-a-term -u testdir/runtest.vim || \
		{ \
			retval=$$?; \
			for fail in testdir/*.fail; do \
				[ -f "$$fail" ] || continue; \
				echo "$$fail:"; \
				cat "$$fail"; \
			done; \
			exit $$retval; \
		}
	@echo "INDENT TESTS: DONE"


clean testclean:
	rm -f testdir/*.fail testdir/*.out
