#
# Makefile to run all tests for Vim, on Dos-like machines.
#
# Requires a set of Unix tools: echo, diff, etc.

VIMPROG = ..\\vim

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
		test48.out test51.out test53.out test54.out \
		test55.out test56.out test57.out test58.out test59.out \
		test60.out test61.out test62.out test63.out test64.out

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
		test105.out test106.out  test107.out\
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

SCRIPTS_GUI =	test16.out

TEST_OUTFILES = $(SCRIPTS16) $(SCRIPTS) $(SCRIPTS32) $(SCRIPTS_GUI)
DOSTMP = dostmp
DOSTMP_OUTFILES = $(TEST_OUTFILES:test=dostmp\test)
DOSTMP_INFILES = $(DOSTMP_OUTFILES:.out=.in)

.SUFFIXES: .in .out

nongui:	nolog $(SCRIPTS16) $(SCRIPTS) report

small:	nolog report

gui:	nolog $(SCRIPTS16) $(SCRIPTS) $(SCRIPTS_GUI) report

win32:	nolog $(SCRIPTS16) $(SCRIPTS) $(SCRIPTS32) report

# Copy the input files to dostmp, changing the fileformat to dos.
$(DOSTMP_INFILES): $(*B).in
	if not exist $(DOSTMP)\NUL md $(DOSTMP)
	if exist $@ del $@
	$(VIMPROG) -u dos.vim --noplugin "+set ff=dos|f $@|wq" $(*B).in

# For each input file dostmp/test99.in run the tests.
# This moves test99.in to test99.in.bak temporarily.
$(TEST_OUTFILES): $(DOSTMP)\$(*B).in
	-@if exist test.out DEL test.out
	move $(*B).in $(*B).in.bak
	copy $(DOSTMP)\$(*B).in $(*B).in
	copy $(*B).ok test.ok
	$(VIMPROG) -u dos.vim -U NONE --noplugin -s dotest.in $(*B).in
	-@if exist test.out MOVE /y test.out $(DOSTMP)\$(*B).out
	-@if exist $(*B).in.bak move /y $(*B).in.bak $(*B).in
	-@del X*
	-@if exist test.ok del test.ok
	-@if exist Xdir1 rd /s /q Xdir1
	-@if exist Xfind rd /s /q Xfind
	-@if exist viminfo del viminfo
	$(VIMPROG) -u dos.vim --noplugin "+set ff=unix|f test.out|wq" \
		$(DOSTMP)\$(*B).out
	@diff test.out $*.ok & if errorlevel 1 \
		( move /y test.out $*.failed \
		 & del $(DOSTMP)\$(*B).out \
		 & echo $* FAILED >> test.log ) \
		else ( move /y test.out $*.out )

report:
	@echo ""
	@echo Test results:
	@if exist test.log ( type test.log & echo TEST FAILURE & exit /b 1 ) \
		else ( echo ALL DONE )

clean:
	-del *.out
	-del *.failed
	-if exist $(DOSTMP) rd /s /q $(DOSTMP)
	-if exist test.in del test.in
	-if exist test.ok del test.ok
	-if exist small.vim del small.vim
	-if exist tiny.vim del tiny.vim
	-if exist mbyte.vim del mbyte.vim
	-if exist mzscheme.vim del mzscheme.vim
	-if exist lua.vim del lua.vim
	-del X*
	-if exist Xdir1 rd /s /q Xdir1
	-if exist Xfind rd /s /q Xfind
	-if exist viminfo del viminfo
	-if exist test.log del test.log
	-if exist benchmark.out del benchmark.out

nolog:
	-if exist test.log del test.log

benchmark:
	bench_re_freeze.out

bench_re_freeze.out: bench_re_freeze.vim
	-if exist benchmark.out del benchmark.out
	$(VIMPROG) -u dos.vim -U NONE --noplugin $*.in
	@IF EXIST benchmark.out ( type benchmark.out )
