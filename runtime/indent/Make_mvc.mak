#
# Makefile for running indent tests on OS Windows.
# Made on the base of a indent/Makefile.
# 2024-03-13, Restorer
#

# included common tools
!INCLUDE ..\..\src\auto\nmake\tools.mak

LSFLAGS = /A:-D /B /O:N /L /S

.SUFFIXES:

VIMPROG = ..\..\src\vim.exe
VIMRUNTIME = ..

# Run the tests that didn't run yet or failed previously.
# If a test succeeds a testdir\*.out file will be written.
# If a test fails a testdir\*.fail file will be written.
test :
	@ set "VIMRUNTIME=$(VIMRUNTIME)"
	@ $(VIMPROG) --clean --not-a-term -u testdir\runtest.vim && \
		(echo:&echo:    INDENT TESTS: DONE &echo:) || \
		<<echofail.bat
set "retval=%ERRORLEVEL%"
@echo off
echo:&echo:    INDENT TESTS: FAILED
for /F %%G in ('2^> nul $(LS) $(LSFLAGS) testdir\*.fail') do (
call set "fail=%%fail%% %%G")
if defined fail (
for %%G in (%fail%) do @(echo:&echo:    %%~nxG:&echo: && type %%G)
)
exit /B %retval%
<<

clean testclean :
	@ if exist testdir\*.fail $(RM) testdir\*.fail
	@ if exist testdir\*.out $(RM) testdir\*.out

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=79 ft=make:
