#
# Makefile to run all tests for Vim on VMS
#
# Authors:	Zoltan Arpadffy, <arpadffy@polarhome.com>
#		Sandor Kopanyi,  <sandor.kopanyi@mailbox.hu>
#
# Last change:  2016 Nov 04
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

# Comment out if you want to run mzschema tests.
# It fails because VMS does not support this feature yet.
# WANT_MZSCH = YES

# Comment out if you have ODS-5 file system
# HAVE_ODS5 = YES

# Comment out if you have gzip on your system
# HAVE_GZIP = YES

# Comment out if you have GNU compatible diff on your system
# HAVE_GDIFF = YES

# Comment out if you have GNU compatible cksum on your system
# HAVE_CKSUM = YES

# Comment out if you have ICONV support
# HAVE_ICONV = YES

# Comment out if you have LUA support
# HAVE_LUA = YES

# Comment out if you have PYTHON support
# HAVE_PYTHON = YES

#######################################################################
# End of configuration section.
#
# Please, do not change anything below without programming experience.
#######################################################################

VIMPROG = <->vim.exe

.SUFFIXES : .out .in

SCRIPT = test1.out  test3.out  test4.out  test5.out  \
       test7.out  test8.out  test9.out  \
       test14.out test15.out \
       test19.out test20.out test22.out \
       test23.out test24.out test26.out \
       test28.out test29.out test30.out test31.out test32.out \
       test33.out test34.out test36.out test37.out \
       test38.out test39.out test40.out test41.out test42.out \
       test43.out test44.out test45.out \
       test48.out test49.out test51.out test53.out test54.out \
       test55.out test56.out test57.out test60.out \
       test64.out test65.out \
       test66.out test67.out test68.out test69.out \
       test72.out test75.out \
       test77a.out test78.out test79.out test80.out \
       test82.out test84.out test88.out \
       test90.out test91.out test94.out \
       test95.out test98.out test99.out \
       test103.out test104.out \
       test107.out test108.out\
       test_autocmd_option.out \
       test_autoformat_join.out \
       test_breakindent.out \
       test_changelist.out \
       test_close_count.out \
       test_comparators.out \
       test_erasebackword.out \
       test_eval.out \
       test_fixeol.out \
       test_getcwd.out \
       test_insertcount.out \
       test_listchars.out \
       test_listlbr.out \
       test_listlbr_utf8.out \
       test_search_mbyte.out \
       test_utf8.out \
       test_wordcount.out

# Known problems:
# test17: ?
#
# test30: bug, most probably - a problem around mac format
#
# test32: VMS is not case sensitive and all filenames are lowercase within Vim
# (this should be changed in order to preserve the original filename) - should
# be fixed. VMS allows just one dot in the filename
#
# test58, test59: Failed/Hangs - VMS does not support spell files (file names
# with too many dots).
#
# test72: bug - Vim hangs at :rename (while rename works well otherwise)
# test78: bug - Vim dies at :recover Xtest 
# test83: ?
# test85: no Lua interface
# test89: bug - findfile() does not work on VMS (just in the current directory) 
# test97, test102: Just ODS-5 supports space and special chars in the filename.
# On ODS-2 tests fail. 

.IFDEF WANT_GUI
SCRIPT_GUI = test16.out
GUI_OPTION = -g
.ENDIF

.IFDEF WANT_UNIX
SCRIPT_UNIX = test10.out test12.out test17.out test25.out test27.out test49.out test73.out
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

.IFDEF HAVE_ODS5                                                                                                                                   
SCRIPT_ODS5 = test97.out test102.out                                                                                                   
.ENDIF  

.IFDEF HAVE_GZIP
SCRIPT_GZIP = test11.out
.ENDIF

.IFDEF HAVE_GDIFF
SCRIPT_GDIFF = test47.out
.ENDIF

.IFDEF HAVE_CKSUM
SCRIPT_CKSUM = test77.out
.ENDIF

.IFDEF HAVE_ICONV
SCRIPT_ICONV = test83.out
.ENDIF

.IFDEF HAVE_LUA
SCRIPT_LUA = test85.out
.ENDIF

.IFDEF HAVE_PYTHON
SCRIPT_PYTHON = test86.out test87.out
.ENDIF

.in.out :
	-@ !clean up before doing the test
	-@ if "''F$SEARCH("test.out.*")'" .NES. "" then delete/noconfirm/nolog test.out.*
	-@ if "''F$SEARCH("$*.out.*")'"   .NES. "" then delete/noconfirm/nolog $*.out.*
	-@ ! define TMP if not set - some tests use it
	-@ if "''F$TRNLNM("TMP")'" .EQS. "" then define/nolog TMP []
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
	-@ if "''F$SEARCH("Xtest.*")'"    .NES. "" then delete/noconfirm/nolog Xtest.*.*

all : clean nolog $(START_WITH) $(SCRIPT) $(SCRIPT_GUI) $(SCRIPT_UNIX) $(SCRIPT_WIN) $(SCRIPT_SPELL) $(SCRIPT_ODS5) $(SCRIPT_GZIP) \
    $(SCRIPT_GDIFF) $(SCRIPT_MZSCH) $(SCRIPT_CKSUM) $(SCRIPT_ICONV) $(SCRIPT_LUA) $(SCRIPT_PYTHON) nolog 
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
	-@ write sys$output "   WANT_GUI   = ""$(WANT_GUI)"" "
	-@ write sys$output "   WANT_UNIX  = ""$(WANT_UNIX)"" "
	-@ write sys$output "   WANT_WIN   = ""$(WANT_WIN)"" "
	-@ write sys$output "   WANT_SPELL = ""$(WANT_SPELL)"" "
	-@ write sys$output "   WANT_MZSCH = ""$(WANT_MZSCH)"" "
	-@ write sys$output "   HAVE_ODS5  = ""$(HAVE_ODS5)"" "
	-@ write sys$output "   HAVE_GZIP  = ""$(HAVE_GZIP)"" "
	-@ write sys$output "   HAVE_GDIFF = ""$(HAVE_GDIFF)"" "
	-@ write sys$output "   HAVE_CKSUM = ""$(HAVE_CKSUM)"" "	  
	-@ write sys$output "   HAVE_ICONV = ""$(HAVE_ICONV)"" "
	-@ write sys$output "   HAVE_LUA   = ""$(HAVE_LUA)"" "
	-@ write sys$output "   HAVE_PYTHON= ""$(HAVE_PYTHON)"" "
	-@ write sys$output "Default vimrc file is VMS.VIM:"
	-@ write sys$output "-----------------------------------------------"
	-@ type VMS.VIM

clean :
	-@ if "''F$SEARCH("*.out")'"        .NES. "" then delete/noconfirm/nolog *.out.*
	-@ if "''F$SEARCH("test.log")'"     .NES. "" then delete/noconfirm/nolog test.log.*
	-@ if "''F$SEARCH("test.ok")'"      .NES. "" then delete/noconfirm/nolog test.ok.*
	-@ if "''F$SEARCH("Xdotest.*")'"    .NES. "" then delete/noconfirm/nolog Xdotest.*.*
	-@ if "''F$SEARCH("Xtest*.*")'"     .NES. "" then delete/noconfirm/nolog Xtest*.*.*
	-@ if "''F$SEARCH("XX*.*")'"        .NES. "" then delete/noconfirm/nolog XX*.*.*
	-@ if "''F$SEARCH("_un_*.*")'"      .NES. "" then delete/noconfirm/nolog _un_*.*.*
	-@ if "''F$SEARCH("*.*_sw*")'"      .NES. "" then delete/noconfirm/nolog *.*_sw*.*
	-@ if "''F$SEARCH("*.failed")'"     .NES. "" then delete/noconfirm/nolog *.failed.*
	-@ if "''F$SEARCH("*.rej")'"        .NES. "" then delete/noconfirm/nolog *.rej.*
	-@ if "''F$SEARCH("tiny.vim")'"     .NES. "" then delete/noconfirm/nolog tiny.vim.*
	-@ if "''F$SEARCH("small.vim")'"    .NES. "" then delete/noconfirm/nolog small.vim.*
	-@ if "''F$SEARCH("mbyte.vim")'"    .NES. "" then delete/noconfirm/nolog mbyte.vim.*
	-@ if "''F$SEARCH("mzscheme.vim")'" .NES. "" then delete/noconfirm/nolog mzscheme.vim.*
	-@ if "''F$SEARCH("lua.vim")'"      .NES. "" then delete/noconfirm/nolog lua.vim.*
	-@ if "''F$SEARCH("viminfo.*")'"    .NES. "" then delete/noconfirm/nolog viminfo.*.*

