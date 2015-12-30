#
# Makefile to run all tests for Vim, on OS/2
#
# OUTDATED, probably doesn't work.
#
# Requires a set of Unix tools: echo, diff, etc.
#

VIMPROG = ../vim.exe

default: all

include Make_all.mak

# Omitted:
# test2		"\\tmp" doesn't work.
# test10	'errorformat' is different
# test11	requires sed
# test12	can't unlink a swap file
# test25	uses symbolic link
# test27	can't edit file with "*" in file name
# test52	only for Win32
# test85	no Lua interface
# test86, 87	no Python interface
# test97	\{ and \$ are not escaped characters.

SCRIPTS = $(SCRIPTS_ALL) $(SCRIPTS_MORE3) $(SCRIPTS_MORE4)

SCRIPTS_BENCH = bench_re_freeze.out

.SUFFIXES: .in .out

all:	/tmp $(SCRIPTS_FIRST) $(SCRIPTS)
	@echo ALL DONE

$(SCRIPTS_FIRST) $(SCRIPTS): $(VIMPROG)

# Must run test1 first to create small.vim.
$(SCRIPTS): $(SCRIPTS_FIRST)

benchmark: $(SCRIPTS_BENCH)

clean:
	-rm -rf *.out Xdotest test.ok tiny.vim small.vim mbyte.vim viminfo

# Make sure all .in and .out files are in DOS fileformat.
.in.out:
	$(VIMPROG) -u NONE -s todos.vim $*.in
	$(VIMPROG) -u NONE -s todos.vim $*.ok
	copy $*.ok test.ok
	$(VIMPROG) -u os2.vim --noplugin -s dotest.in $*.in
	$(VIMPROG) -u NONE -s todos.vim test.out
	diff test.out $*.ok
	rename test.out $*.out
	-rm -rf X* viminfo
	-del test.ok

# Create a directory for temp files
/tmp:
	-mkdir /tmp

bench_re_freeze.out: bench_re_freeze.vim
	-del $*.failed test.ok benchmark.out
	copy $*.ok test.ok
	$(VIMPROG) -u os2.vim --noplugin -s dotest.in $*.in
	type benchmark.out

