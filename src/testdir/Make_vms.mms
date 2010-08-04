#
# Makefile to run all tests for Vim on VMS
#
# Authors:	Zoltan Arpadffy, <arpadffy@polarhome.com>
#		Sandor Kopanyi,  <sandor.kopanyi@mailbox.hu>
#
# Last change:  2010 Aug 04
#
# This has been tested on VMS 6.2 to 8.3 on DEC Alpha, VAX and IA64.
# Edit the lines in the Configuration section below to select.
#
# Execute with:
#		mms/descrip=Make_vms.mms
# Cleanup with:
#		mms/descrip=Make_vms.mms clean
#
# Make files are MMK compatible.
#
# NOTE: You can run this script just in X/Window environment. It will
# create a new terminals, therefore you have to set up your DISPLAY
# logical. More info in VMS documentation or with: help set disp.
#
#######################################################################
# Configuration section.
#######################################################################

# Uncomment if you want tests in GUI mode.  Terminal mode is default.
# WANT_GUI  = YES

# Comment out if you want to run Unix specific tests as well, but please
# be aware, that on OpenVMS will fail, because of cat, rm, etc commands
# and directory handling.
# WANT_UNIX = YES

# Comment out if you want to run Win32 specific tests as well, but please
# be aware, that on OpenVMS will fail, because of cat, rm, etc commands
# and directory handling.
# WANT_WIN = YES

# Comment out if you want to run spell checker tests. 
# They fail because VMS does not support file names.
# WANT_SPELL = YES

# Comment out if you want to run mzschema  tests.
# It fails because VMS does not support this feature yet.
# WANT_MZSCH = YES

# Comment out if you have gzip on your system
# HAVE_GZIP = YES

# Comment out if you have GNU compatible diff on your system
# HAVE_GDIFF = YES

#######################################################################
# End of configuration section.
#
# Please, do not change anything below without programming experience.
#######################################################################

VIMPROG = <->vim.exe

.SUFFIXES : .out .in

SCRIPT = test1.out  test2.out  test3.out  test4.out  test5.out  \
	 test6.out  test7.out  test8.out  test9.out  test10a.out\
	 test13.out test14.out test15.out test17.out \
	 test18.out test19.out test20.out test21.out test22.out \
	 test23.out test24.out test26.out \
	 test28.out test29.out test30.out test31.out test32.out \
	 test33.out test34.out test35.out test36.out test37.out \
	 test38.out test39.out test40.out test41.out test42.out \
	 test43.out test44.out test45.out test46.out \
	 test48.out test51.out test53.out test54.out test55.out \
	 test56.out test57.out test60.out \
	 test61.out test62.out test63.out test64.out test65.out \
	 test66.out test67.out test68.out test69.out \
	 test71.out test72.out

# Known problems:
# Test 30: a problem around mac format - unknown reason
#
# Test 32: VMS is not case sensitive and all filenames are lowercase within Vim
# (this should be changed in order to preserve the original filename) - should
# be fixed. VMS allows just one dot in the filename
#
# Test 58 and 59: Failed/Hangs - VMS does not support spell files (file names
# with too many dots).
#
# Test 72: unknown reason

.IFDEF WANT_GUI
SCRIPT_GUI = test16.out
GUI_OPTION = -g
.ENDIF

.IFDEF WANT_UNIX
SCRIPT_UNIX = test10.out test12.out test25.out test27.out test49.out test73.out
.ENDIF

.IFDEF WANT_WIN
SCRIPT_WIN = test50.out test52.out
.ENDIF

.IFDEF WANT_SPELL
SCRIPT_SPELL = test58.out test59.out 
.ENDIF

.IFDEF WANT_MZSCH
SCRIPT_MZSCH = test70.out 
.ENDIF

.IFDEF HAVE_GZIP
SCRIPT_GZIP = test11.out
.ENDIF

.IFDEF HAVE_GDIFF
SCRIPT_GDIFF = test47.out
.ENDIF

.in.out :
	-@ !clean up before doing the test
	-@ if "''F$SEARCH("test.out.*")'" .NES. "" then delete/noconfirm/nolog test.out.*
	-@ if "''F$SEARCH("$*.out.*")'"   .NES. "" then delete/noconfirm/nolog $*.out.*
	-@ write sys$output " "
	-@ write sys$output "-----------------------------------------------"
	-@ write sys$output "                "$*" "
	-@ write sys$output "-----------------------------------------------"
	-@ !run the test
	-@ create/term/wait/nodetach mcr $(VIMPROG) $(GUI_OPTION) -u vms.vim --noplugin -s dotest.in $*.in
	-@ !analyse the result
	-@ directory /size/date test.out
	-@ if "''F$SEARCH("test.out.*")'" .NES. "" then rename/nolog test.out $*.out 
	-@ if "''F$SEARCH("$*.out.*")'"   .NES. "" then differences /par $*.out $*.ok;
	-@ !clean up after the test
	-@ if "''F$SEARCH("Xdotest.*")'"  .NES. "" then delete/noconfirm/nolog Xdotest.*.*

all : clean nolog $(START_WITH) $(SCRIPT) $(SCRIPT_GUI) $(SCRIPT_UNIX) $(SCRIPT_WIN) $(SCRIPT_SPELL) $(SCRIPT_GZIP) \
    $(SCRIPT_GDIFF) $(SCRIPT_MZSCH) nolog 
	-@ write sys$output " "
	-@ write sys$output "-----------------------------------------------"
	-@ write sys$output "                All done"
	-@ write sys$output "-----------------------------------------------"
	-@ deassign sys$output
	-@ delete/noconfirm/nolog x*.*.*
	-@ type test.log

nolog :
	-@ define sys$output test.log
	-@ write sys$output "-----------------------------------------------"
	-@ write sys$output "           Standard VIM test cases"
	-@ write sys$output "-----------------------------------------------"
	-@ write sys$output " OpenVMS version: ''F$GETSYI("VERSION")'"
	-@ write sys$output " Vim version:"
	-@ mcr $(VIMPROG) --version
	-@ write sys$output " Test date:"
	-@ show time
	-@ write sys$output "-----------------------------------------------"
	-@ write sys$output "                Test results:"
	-@ write sys$output "-----------------------------------------------"
	-@ write sys$output "MAKE_VMS.MMS options:"
	-@ write sys$output "   WANT_GUI  = ""$(WANT_GUI)"" "
	-@ write sys$output "   WANT_UNIX = ""$(WANT_UNIX)"" "
	-@ write sys$output "   WANT_WIN  = ""$(WANT_WIN)"" "
	-@ write sys$output "   WANT_SPELL= ""$(WANT_SPELL)"" "
	-@ write sys$output "   WANT_MZSCH= ""$(WANT_MZSCH)"" "
	-@ write sys$output "   HAVE_GZIP = ""$(HAVE_GZIP)"" "
	-@ write sys$output "   HAVE_GDIFF= ""$(HAVE_GDIFF)"" "
	-@ write sys$output "Default vimrc file is VMS.VIM:"
	-@ write sys$output "-----------------------------------------------"
	-@ type VMS.VIM

clean :
	-@ if "''F$SEARCH("*.out")'"        .NES. "" then delete/noconfirm/nolog *.out.*
	-@ if "''F$SEARCH("test.log")'"     .NES. "" then delete/noconfirm/nolog test.log.*
	-@ if "''F$SEARCH("test.ok")'"      .NES. "" then delete/noconfirm/nolog test.ok.*
	-@ if "''F$SEARCH("Xdotest.*")'"    .NES. "" then delete/noconfirm/nolog Xdotest.*.*
	-@ if "''F$SEARCH("*.*_sw*")'"      .NES. "" then delete/noconfirm/nolog *.*_sw*.*
	-@ if "''F$SEARCH("*.failed")'"     .NES. "" then delete/noconfirm/nolog *.failed.*
	-@ if "''F$SEARCH("*.rej")'"        .NES. "" then delete/noconfirm/nolog *.rej.*
	-@ if "''F$SEARCH("tiny.vim")'"     .NES. "" then delete/noconfirm/nolog tiny.vim.*
	-@ if "''F$SEARCH("small.vim")'"    .NES. "" then delete/noconfirm/nolog small.vim.*
	-@ if "''F$SEARCH("mbyte.vim")'"    .NES. "" then delete/noconfirm/nolog mbyte.vim.*
	-@ if "''F$SEARCH("mzscheme.vim")'" .NES. "" then delete/noconfirm/nolog mzscheme.vim.*
	-@ if "''F$SEARCH("viminfo.*")'"    .NES. "" then delete/noconfirm/nolog viminfo.*.*

