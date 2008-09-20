# Makefile to run tests for Vim, on Dos-like machines
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
DIRSLASH = /
else
DEL = del
MV = rename
CP = copy
DIRSLASH = \\
endif

VIMPROG = ..$(DIRSLASH)vim

# Omitted:
# test2		"\\tmp" doesn't work.
# test10	'errorformat' is different
# test12	can't unlink a swap file
# test25	uses symbolic link
# test27	can't edit file with "*" in file name
# test31	16 bit version runs out of memory...

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
		test42.out test52.out test65.out

SCRIPTS32 =	test50.out

SCRIPTS_GUI = test16.out

.SUFFIXES: .in .out

vimall:	fixff $(SCRIPTS16) $(SCRIPTS) $(SCRIPTS_GUI) $(SCRIPTS32)
	echo ALL DONE

nongui:	fixff $(SCRIPTS16) $(SCRIPTS)
	echo ALL DONE

small:
	echo ALL DONE

gui:	fixff $(SCRIPTS16) $(SCRIPTS) $(SCRIPTS_GUI)
	echo ALL DONE

win32:	fixff $(SCRIPTS16) $(SCRIPTS) $(SCRIPTS32)
	echo ALL DONE

fixff:
	-$(VIMPROG) -u dos.vim --noplugin "+argdo set ff=dos|upd" +q *.in *.ok

clean:
	-$(DEL) *.out
	-$(DEL) test.ok
	-$(DEL) small.vim
	-$(DEL) tiny.vim
	-$(DEL) mbyte.vim
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
