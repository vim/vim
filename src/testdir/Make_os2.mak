#
# Makefile to run all tests for Vim, on OS/2
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
# test85	no Lua interface
# test86, 87	no Python interface
# test97	\{ and \$ are not escaped characters.

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
		test61.out test62.out test63.out test64.out test65.out \
		test66.out test67.out test68.out test69.out test70.out \
		test71.out test72.out test73.out test74.out test75.out \
		test76.out test77.out test78.out test79.out test80.out \
		test81.out test82.out test83.out test84.out test88.out \
		test89.out test90.out test91.out test92.out test93.out \
		test94.out test95.out test96.out test98.out test99.out \
		test100.out test101.out test102.out test103.out test104.out \
		test105.out test106.out test107.out \
		test_argument_0count.out \
		test_argument_count.out \
		test_autoformat_join.out \
		test_breakindent.out \
		test_changelist.out \
		test_close_count.out \
		test_command_count.out \
		test_erasebackword.out \
		test_eval.out \
		test_insertcount.out \
		test_listchars.out \
		test_listlbr.out \
		test_listlbr_utf8.out \
		test_mapping.out \
		test_marks.out \
		test_nested_function.out \
		test_options.out \
		test_qf_title.out \
		test_signs.out \
		test_textobjects.out \
		test_utf8.out

SCRIPTS_BENCH = bench_re_freeze.out

.SUFFIXES: .in .out

all:	/tmp $(SCRIPTS)
	@echo ALL DONE

$(SCRIPTS): $(VIMPROG)

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

