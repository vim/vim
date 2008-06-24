#
# Makefile to run al tests for Vim, on OS/2
#
# Requires a set of Unix tools: echo, diff, etc.

VIMPROG = ../vim.exe

# Omitted:
# test2		"\\tmp" doesn't work.
# test10	'errorformat' is different
# test11	requires sed
# test12	can't unlink a swap file
# test25	uses symbolic link
# test27	can't edit file with "*" in file name
# test52	only for Win32

SCRIPTS = test1.out test3.out test4.out test5.out test6.out \
		test7.out test8.out test9.out \
		test13.out test14.out test15.out test17.out \
		test18.out test19.out test20.out test21.out test22.out \
		test23.out test24.out test26.out \
		test28.out test29.out test30.out test31.out test32.out \
		test33.out test34.out test35.out test36.out test37.out \
		test38.out test39.out test40.out test41.out test42.out \
		test43.out test44.out test45.out test46.out test47.out \
		test48.out test51.out test53.out test54.out test55.out \
		test56.out test57.out test58.out test59.out test60.out \
		test61.out test62.out test63.out test64.out test65.out

.SUFFIXES: .in .out

all:	/tmp $(SCRIPTS)
	@echo ALL DONE

$(SCRIPTS): $(VIMPROG)

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
