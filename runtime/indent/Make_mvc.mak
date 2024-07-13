# Makefile for running indent tests on OS Windows.
# Made on the base of a indent/Makefile.
# Restorer, 13.03.2024

.SUFFIXES:

VIMPROG = vim.exe
VIMRUNTIME = ..

# Run the tests that didn't run yet or failed previously.
# If a test succeeds a testdir\*.out file will be written.
# If a test fails a testdir\*.fail file will be written.
test :
	@ set "VIMRUNTIME=$(VIMRUNTIME)"
	$(VIMPROG) --clean --not-a-term -u testdir\runtest.vim


clean testclean :
	@ if exist testdir\*.fail del /q testdir\*.fail
	@ if exist testdir\*.out del /q testdir\*.out

