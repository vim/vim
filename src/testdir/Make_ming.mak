#
# Makefile to run all tests for Vim, on Dos-like machines
# with sh.exe or zsh.exe in the path or not.
#
# Author: Bill McCarthy
#
# Note that test54 has been removed until it is fixed.
#
# Requires a set of Unix tools: echo, diff, etc.

ifneq (sh.exe, $(SHELL))
DEL = rm -f
MV = mv
CP = cp
CAT = cat
DIRSLASH = /
else
DEL = del
MV = rename
CP = copy
CAT = type
DIRSLASH = \\
endif

VIMPROG = ..$(DIRSLASH)vim

# Omitted:
# test2		"\\tmp" doesn't work.
# test10	'errorformat' is different
# test12	can't unlink a swap file
# test25	uses symbolic link
# test27	can't edit file with "*" in file name
# test97	\{ and \$ are not escaped characters.

SCRIPTS16 =	test1.out test19.out test20.out test22.out \
		test23.out test24.out test28.out test29.out \
		test35.out test36.out test43.out \
		test44.out test45.out test46.out test47.out \
		test48.out test51.out test53.out            \
		test55.out test56.out test57.out test58.out test59.out \
		test60.out test61.out test62.out test63.out test64.out

# Had to remove test54 which doesn't work yet.
#		                                 test54.out

SCRIPTS =	test3.out test4.out test5.out test6.out test7.out \
		test8.out test9.out test11.out test13.out test14.out \
		test15.out test17.out test18.out test21.out test26.out \
		test30.out test31.out test32.out test33.out test34.out \
		test37.out test38.out test39.out test40.out test41.out \
		test42.out test52.out test65.out test66.out test67.out \
		test68.out test69.out test71.out test72.out test73.out \
		test74.out test75.out test76.out test77.out test78.out \
		test79.out test80.out test81.out test82.out test83.out \
		test84.out test85.out test86.out test87.out test88.out \
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

SCRIPTS32 =	test50.out test70.out

SCRIPTS_GUI = test16.out

SCRIPTS_BENCH = bench_re_freeze.out

.SUFFIXES: .in .out

vimall:	fixff $(SCRIPTS16) $(SCRIPTS) $(SCRIPTS_GUI) $(SCRIPTS32)
	echo ALL DONE

nongui:	fixff $(SCRIPTS16) $(SCRIPTS)
	echo ALL DONE

benchmark: $(SCRIPTS_BENCH)

small:
	echo ALL DONE

gui:	fixff $(SCRIPTS16) $(SCRIPTS) $(SCRIPTS_GUI)
	echo ALL DONE

win32:	fixff $(SCRIPTS16) $(SCRIPTS) $(SCRIPTS32)
	echo ALL DONE

fixff:
	-$(VIMPROG) -u dos.vim --noplugin "+argdo set ff=dos|upd" +q *.in *.ok
	-$(VIMPROG) -u dos.vim --noplugin "+argdo set ff=unix|upd" +q \
		dotest.in test60.ok test71.ok test74.ok

clean:
	-$(DEL) *.out
	-$(DEL) test.ok
	-$(DEL) small.vim
	-$(DEL) tiny.vim
	-$(DEL) mbyte.vim
	-$(DEL) mzscheme.vim
	-$(DEL) lua.vim
	-$(DEL) X*
	-$(DEL) viminfo

.in.out:
	$(CP) $*.ok test.ok
	$(VIMPROG) -u dos.vim -U NONE --noplugin -s dotest.in $*.in
	diff test.out $*.ok
	-$(DEL) $*.out
	$(MV) test.out $*.out
	-$(DEL) X*
	-$(DEL) test.ok
	-$(DEL) viminfo

bench_re_freeze.out: bench_re_freeze.vim
	-$(DEL) benchmark.out
	$(VIMPROG) -u dos.vim -U NONE --noplugin $*.in
	$(CAT) benchmark.out
