#
# Makefile to run all tests for Vim
#

VIMPROG = ../vim

# Uncomment this line for using valgrind.
# The output goes into a file "valgrind.$PID" (sorry, no test number).
# VALGRIND = valgrind --tool=memcheck --leak-check=yes --num-callers=15 --logfile=valgrind

SCRIPTS = test1.out test2.out test3.out test4.out test5.out test6.out \
		test7.out test8.out test9.out test10.out test11.out \
		test12.out  test13.out test14.out test15.out test17.out \
		test18.out test19.out test20.out test21.out test22.out \
		test23.out test24.out test25.out test26.out test27.out \
		test28.out test29.out test30.out test31.out test32.out \
		test33.out test34.out test35.out test36.out test37.out \
		test38.out test39.out test40.out test41.out test42.out \
		test43.out test44.out test45.out test46.out test47.out \
		test48.out test49.out test51.out test52.out test53.out \
		test54.out test55.out test56.out test57.out test58.out \
		test59.out test60.out test61.out test62.out test63.out \
		test64.out test65.out

SCRIPTS_GUI = test16.out

.SUFFIXES: .in .out

nongui:	nolog $(SCRIPTS) report

gui:	nolog $(SCRIPTS) $(SCRIPTS_GUI) report

report:
	@echo
	@echo 'Test results:'
	@/bin/sh -c "if test -f test.log; \
		then cat test.log; echo TEST FAILURE; exit 1; \
		else echo ALL DONE; \
		fi"

$(SCRIPTS) $(SCRIPTS_GUI): $(VIMPROG)

clean:
	-rm -rf *.out *.failed *.rej *.orig test.log tiny.vim small.vim mbyte.vim test.ok X* valgrind.pid* viminfo

test1.out: test1.in
	-rm -f $*.failed tiny.vim small.vim mbyte.vim test.ok X* viminfo
	$(VALGRIND) $(VIMPROG) -u unix.vim -U NONE --noplugin -s dotest.in $*.in
	@/bin/sh -c "if diff test.out $*.ok; \
		then mv -f test.out $*.out; \
		else echo; \
		echo test1 FAILED - Something basic is wrong; \
		echo; exit 1; fi"
	-rm -rf X* viminfo

.in.out:
	-rm -rf $*.failed test.ok test.out X* viminfo
	cp $*.ok test.ok
	# Sleep a moment to avoid that the xterm title is messed up
	@-sleep .2
	-$(VALGRIND) $(VIMPROG) -u unix.vim -U NONE --noplugin -s dotest.in $*.in
	@/bin/sh -c "if test -f test.out; then\
		  if diff test.out $*.ok; \
		  then mv -f test.out $*.out; \
		  else echo $* FAILED >>test.log; mv -f test.out $*.failed; \
		  fi \
		else echo $* NO OUTPUT >>test.log; \
		fi"
	-rm -rf X* test.ok viminfo

test49.out: test49.vim

test60.out: test60.vim

nolog:
	-rm -f test.log
