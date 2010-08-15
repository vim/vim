# This Makefile has two purposes:
# 1. Starting the compilation of Vim for Unix.
# 2. Creating the various distribution files.


#########################################################################
# 1. Starting the compilation of Vim for Unix.
#
# Using this Makefile without an argument will compile Vim for Unix.
# "make install" is also possible.
#
# NOTE: If this doesn't work properly, first change directory to "src" and use
# the Makefile there:
#	cd src
#	make [arguments]
# Noticed on AIX systems when using this Makefile: Trying to run "cproto" or
# something else after Vim has been compiled.  Don't know why...
# Noticed on OS/390 Unix: Restarts configure.
#
# The first (default) target is "first".  This will result in running
# "make first", so that the target from "src/auto/config.mk" is picked
# up properly when config didn't run yet.  Doing "make all" before configure
# has run can result in compiling with $(CC) empty.

first:
	@echo "Starting make in the src directory."
	@echo "If there are problems, cd to the src directory and run make there"
	cd src && $(MAKE) $@

# Some make programs use the last target for the $@ default; put the other
# targets separately to always let $@ expand to "first" by default.
all install uninstall tools config configure reconfig proto depend lint tags types test testclean clean distclean:
	@echo "Starting make in the src directory."
	@echo "If there are problems, cd to the src directory and run make there"
	cd src && $(MAKE) $@


#########################################################################
# 2. Creating the various distribution files.
#
# TARGET	PRODUCES		CONTAINS
# unixall	vim-#.#.tar.bz2		All runtime files and sources, for Unix
#
# html		vim##html.zip		HTML docs
#
# dossrc	vim##src.zip		sources for MS-DOS
# dosrt		vim##rt.zip		runtime for MS-DOS
# dosbin	vim##d16.zip		binary for MS-DOS 16 bits
#		vim##d32.zip		binary for MS-DOS 32 bits
#		vim##w32.zip		binary for Win32
#		gvim##.zip		binary for GUI Win32
#		gvim##ole.zip		OLE exe for Win32 GUI
#		gvim##_s.zip		exe for Win32s GUI
#
# OBSOLETE
# amisrc	vim##src.tgz		sources for Amiga
# amirt		vim##rt.tgz		runtime for Amiga
# amibin	vim##bin.tgz		binary for Amiga
#
# os2bin	vim##os2.zip		binary for OS/2
#					(use RT from dosrt)
#
# farsi		farsi##.zip		Farsi fonts
#
#    All output files are created in the "dist" directory.  Existing files are
#    overwritten!
#    To do all this you need the Unix archive and compiled binaries.
#    Before creating an archive first delete all backup files, *.orig, etc.

MAJOR = 7
MINOR = 3

# Uncomment this line if the Win32s version is to be included.
DOSBIN_S =  dosbin_s

# Uncomment this line if the 16 bit DOS version is to be included.
# DOSBIN_D16 = dosbin_d16

# CHECKLIST for creating a new version:
#
# - Update Vim version number.  For a test version in: src/version.h, Contents,
#   MAJOR/MINOR above, VIMMAJOR and VIMMINOR in src/Makefile, README*.txt,
#   runtime/doc/*.txt and nsis/gvim.nsi. Other things in README_os2.txt.  For a
#   minor/major version: src/GvimExt/GvimExt.reg, src/vim.def, src/vim16.def,
#   src/gvim.exe.mnf.
# - Adjust the date and other info in src/version.h.
# - Correct included_patches[] in src/version.c.
# - Compile Vim with GTK, Perl, Python, Python3, TCL, Ruby, MZscheme, Lua (if
#   you can make it all work), Cscope and "huge" features.  Exclude workshop
#   and SNiFF.
# - With these features: "make proto" (requires cproto and Motif installed;
#   ignore warnings for missing include files, fix problems for syntax errors).
# - With these features: "make depend" (works best with gcc).
# - If you have a lint program: "make lint" and check the output (ignore GTK
#   warnings).
# - Enable the efence library in "src/Makefile" and run "make test".  Disable
#   Python and Ruby to avoid trouble with threads (efence is not threadsafe).
# - Check for missing entries in runtime/makemenu.vim (with checkmenu script).
# - Check for missing options in runtime/optwin.vim et al. (with check.vim).
# - Do "make menu" to update the runtime/synmenu.vim file.
# - Add remarks for changes to runtime/doc/version7.txt.
# - Check that runtime/doc/help.txt doesn't contain entries in "LOCAL
#   ADDITIONS".
# - In runtime/doc run "make" and "make html" to check for errors.
# - Check if src/Makefile and src/feature.h don't contain any personal
#   preferences or the GTK, Perl, etc. mentioned above.
# - Check file protections to be "644" for text and "755" for executables (run
#   the "check" script).
# - Check compiling on Amiga, MS-DOS and MS-Windows.
# - Delete all *~, *.sw?, *.orig, *.rej files
# - "make unixall", "make html"
# - Make diff files against the previous release: "makediff7 7.1 7.2"
#
# Amiga: (OBSOLETE, Amiga files are no longer distributed)
# - "make amisrc", move the archive to the Amiga and compile:
#   "make -f Make_manx.mak" (will use "big" features by default).
# - Run the tests: "make -f Make_manx.mak test"
# - Place the executables Vim and Xxd in this directory (set the executable
#   flag).
# - "make amirt", "make amibin".
#
# PC:
# - Run make on Unix to update the ".mo" files.
# - "make dossrc" and "make dosrt".  Unpack the archives on a PC.
# 16 bit DOS version: (OBSOLETE, 16 bit version doesn't build)
# - Set environment for compiling with Borland C++ 3.1.
# - "bmake -f Make_bc3.mak BOR=E:\borlandc" (compiling xxd might fail, in that
#   case set environment for compiling with Borland C++ 4.0 and do
#   "make -f make_bc3.mak BOR=E:\BC4 xxd/xxd.exe").
#   NOTE: this currently fails because Vim is too big.
# - "make test" and check the output.
# - Rename the executables to "vimd16.exe", "xxdd16.exe", "installd16.exe" and
#   "uninstald16.exe".
# 32 bit DOS version:
# - Set environment for compiling with DJGPP; "gmake -f Make_djg.mak".
# - "rm testdir/*.out", "gmake -f Make_djg.mak test" and check the output for
#   "ALL DONE".
# - Rename the executables to "vimd32.exe", "xxdd32.exe", "installd32.exe" and
#   "uninstald32.exe".
# Win32 console version:
# - Set environment for Visual C++ 2008, e.g.:
#   "E:\Microsoft Visual Studio 9.0\VC\bin\vcvars32.bat".  Or, when using the
#   Visual C++ Toolkit 2003: "msvcsetup.bat" (adjust the paths when necessary).
#   For Windows 98/ME the 2003 version is required, but then it won't work on
#   Windows 7 and 64 bit.
# - "nmake -f Make_mvc.mak"
# - "rm testdir/*.out", "nmake -f Make_mvc.mak test" and check the output.
# - Rename the executables to "vimw32.exe", "xxdw32.exe".
# - Rename vim.pdb to vimw32.pdb.
# - When building the Win32s version later, delete vimrun.exe, install.exe and
#   uninstal.exe.  Otherwise rename executables to installw32.exe and
#   uninstalw32.exe.
# Win32 GUI version:
# - "nmake -f Make_mvc.mak GUI=yes.
# - move "gvim.exe" to here (otherwise the OLE version will overwrite it).
# - Move gvim.pdb to here.
# - Delete vimrun.exe, install.exe and uninstal.exe.
# - Copy "GvimExt/gvimext.dll" to here.
# Win32 GUI version with OLE, PERL, TCL, PYTHON and dynamic IME:
# - Run src/bigvim.bat ("nmake -f Make_mvc.mak GUI=yes OLE=yes IME=yes ...)
# - Rename "gvim.exe" to "gvim_ole.exe".
# - Rename gvim.pdb to "gvim_ole.pdb".
# - Delete install.exe and uninstal.exe.
# - If building the Win32s version delete vimrun.exe.
# Win32s GUI version:
# - Set environment for Visual C++ 4.1 (requires a new console window):
#   "vcvars32.bat" (use the path for VC 4.1 e:\msdev\bin)
# - "nmake -f Make_mvc.mak GUI=yes INTL=no clean" (use the path for VC 4.1)
# - "nmake -f Make_mvc.mak GUI=yes INTL=no" (use the path for VC 4.1)
# - Rename "gvim.exe" to "gvim_w32s.exe".
# - Rename "install.exe" to "installw32.exe"
# - Rename "uninstal.exe" to "uninstalw32.exe"
# - The produced uninstalw32.exe and vimrun.exe are used.
# Create the archives:
# - Copy all the "*.exe" files to where this Makefile is.
# - Copy all the "*.pdb" files to where this Makefile is.
# - "make dosbin".
# NSIS self installing exe:
# - To get NSIS see http://nsis.sourceforge.net
# - Make sure gvim_ole.exe, vimd32.exe, vimw32.exe, installw32.exe,
#   uninstalw32.exe and xxdw32.exe have been build as mentioned above.
# - copy these files (get them from a binary archive or build them):
#	gvimext.dll in src/GvimExt
#	gvimext64.dll in src/GvimExt
#	VisVim.dll in src/VisVim
#   Note: VisVim needs to be build with MSVC 5, newer versions don't work.
#   gvimext64.dll can be obtained from http://code.google.com/p/vim-win3264/
#	It is part of vim72.zip as vim72/gvimext.dll.
# - make sure there is a diff.exe two levels up
# - go to ../nsis and do "makensis gvim.nsi" (takes a few minutes).
# - Copy gvim##.exe to the dist directory.
#
# OS/2: (OBSOLETE, OS/2 version is no longer distributed)
# - Unpack the Unix archive.
# - "make -f Make_os2.mak".
# - Rename the executables to vimos2.exe, xxdos2.exe and teeos2.exe and copy
#   them to here.
# - "make os2bin".

VIMVER	= vim-$(MAJOR).$(MINOR)
VERSION = $(MAJOR)$(MINOR)
VDOT	= $(MAJOR).$(MINOR)
VIMRTDIR = vim$(VERSION)

# Vim used for conversion from "unix" to "dos"
VIM	= vim

# How to include Filelist depends on the version of "make" you have.
# If the current choice doesn't work, try the other one.

include Filelist
#.include "Filelist"


# All output is put in the "dist" directory.
dist:
	mkdir dist

# Clean up some files to avoid they are included.
prepare:
	if test -f runtime/doc/uganda.nsis.txt; then \
		rm runtime/doc/uganda.nsis.txt; fi

# For the zip files we need to create a file with the comment line
dist/comment:
	mkdir dist/comment

COMMENT_RT = comment/$(VERSION)-rt
COMMENT_D16 = comment/$(VERSION)-bin-d16
COMMENT_D32 = comment/$(VERSION)-bin-d32
COMMENT_W32 = comment/$(VERSION)-bin-w32
COMMENT_GVIM = comment/$(VERSION)-bin-gvim
COMMENT_OLE = comment/$(VERSION)-bin-ole
COMMENT_W32S = comment/$(VERSION)-bin-w32s
COMMENT_SRC = comment/$(VERSION)-src
COMMENT_OS2 = comment/$(VERSION)-bin-os2
COMMENT_HTML = comment/$(VERSION)-html
COMMENT_FARSI = comment/$(VERSION)-farsi

dist/$(COMMENT_RT): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) runtime files for MS-DOS and MS-Windows" > dist/$(COMMENT_RT)

dist/$(COMMENT_D16): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) binaries for MS-DOS 16 bit real mode" > dist/$(COMMENT_D16)

dist/$(COMMENT_D32): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) binaries for MS-DOS 32 bit protected mode" > dist/$(COMMENT_D32)

dist/$(COMMENT_W32): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) binaries for MS-Windows NT/95" > dist/$(COMMENT_W32)

dist/$(COMMENT_GVIM): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) GUI binaries for MS-Windows NT/95" > dist/$(COMMENT_GVIM)

dist/$(COMMENT_OLE): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) MS-Windows GUI binaries with OLE support" > dist/$(COMMENT_OLE)

dist/$(COMMENT_W32S): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) GUI binaries for MS-Windows 3.1/3.11" > dist/$(COMMENT_W32S)

dist/$(COMMENT_SRC): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) sources for MS-DOS and MS-Windows" > dist/$(COMMENT_SRC)

dist/$(COMMENT_OS2): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) binaries + runtime files for OS/2" > dist/$(COMMENT_OS2)

dist/$(COMMENT_HTML): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) documentation in HTML" > dist/$(COMMENT_HTML)

dist/$(COMMENT_FARSI): dist/comment
	echo "Vim - Vi IMproved - v$(VDOT) Farsi language files" > dist/$(COMMENT_FARSI)

unixall: dist prepare
	-rm -f dist/$(VIMVER).tar.bz2
	-rm -rf dist/$(VIMRTDIR)
	mkdir dist/$(VIMRTDIR)
	tar cf - \
		$(RT_ALL) \
		$(RT_ALL_BIN) \
		$(RT_UNIX) \
		$(RT_UNIX_DOS_BIN) \
		$(RT_SCRIPTS) \
		$(LANG_GEN) \
		$(LANG_GEN_BIN) \
		$(SRC_ALL) \
		$(SRC_UNIX) \
		$(SRC_DOS_UNIX) \
		$(EXTRA) \
		$(LANG_SRC) \
		| (cd dist/$(VIMRTDIR); tar xf -)
# Need to use a "distclean" config.mk file
	cp -f src/config.mk.dist dist/$(VIMRTDIR)/src/auto/config.mk
# Create an empty config.h file, make dependencies require it
	touch dist/$(VIMRTDIR)/src/auto/config.h
# Make sure configure is newer than config.mk to force it to be generated
	touch dist/$(VIMRTDIR)/src/configure
# Make sure ja.sjis.po is newer than ja.po to avoid it being regenerated.
# Same for cs.cp1250.po, pl.cp1250.po and sk.cp1250.po.
	touch dist/$(VIMRTDIR)/src/po/ja.sjis.po
	touch dist/$(VIMRTDIR)/src/po/cs.cp1250.po
	touch dist/$(VIMRTDIR)/src/po/pl.cp1250.po
	touch dist/$(VIMRTDIR)/src/po/sk.cp1250.po
	touch dist/$(VIMRTDIR)/src/po/zh_CN.cp936.po
	touch dist/$(VIMRTDIR)/src/po/ru.cp1251.po
	touch dist/$(VIMRTDIR)/src/po/uk.cp1251.po
# Create the archive.
	cd dist && tar cf $(VIMVER).tar $(VIMRTDIR)
	bzip2 dist/$(VIMVER).tar

# Amiga runtime - OBSOLETE
amirt: dist prepare
	-rm -f dist/vim$(VERSION)rt.tar.gz
	-rm -rf dist/Vim
	mkdir dist/Vim
	mkdir dist/Vim/$(VIMRTDIR)
	tar cf - \
		$(ROOT_AMI) \
		$(RT_ALL) \
		$(RT_ALL_BIN) \
		$(RT_SCRIPTS) \
		$(RT_AMI) \
		$(RT_NO_UNIX) \
		$(RT_AMI_DOS) \
		| (cd dist/Vim/$(VIMRTDIR); tar xf -)
	mv dist/Vim/$(VIMRTDIR)/vimdir.info dist/Vim.info
	mv dist/Vim/$(VIMRTDIR)/runtime.info dist/Vim/$(VIMRTDIR).info
	mv dist/Vim/$(VIMRTDIR)/runtime/* dist/Vim/$(VIMRTDIR)
	rmdir dist/Vim/$(VIMRTDIR)/runtime
	cd dist && tar cf vim$(VERSION)rt.tar Vim Vim.info
	gzip -9 dist/vim$(VERSION)rt.tar
	mv dist/vim$(VERSION)rt.tar.gz dist/vim$(VERSION)rt.tgz

# Amiga binaries - OBSOLETE
amibin: dist prepare
	-rm -f dist/vim$(VERSION)bin.tar.gz
	-rm -rf dist/Vim
	mkdir dist/Vim
	mkdir dist/Vim/$(VIMRTDIR)
	tar cf - \
		$(ROOT_AMI) \
		$(BIN_AMI) \
		Vim \
		Xxd \
		| (cd dist/Vim/$(VIMRTDIR); tar xf -)
	mv dist/Vim/$(VIMRTDIR)/vimdir.info dist/Vim.info
	mv dist/Vim/$(VIMRTDIR)/runtime.info dist/Vim/$(VIMRTDIR).info
	cd dist && tar cf vim$(VERSION)bin.tar Vim Vim.info
	gzip -9 dist/vim$(VERSION)bin.tar
	mv dist/vim$(VERSION)bin.tar.gz dist/vim$(VERSION)bin.tgz

# Amiga sources - OBSOLETE
amisrc: dist prepare
	-rm -f dist/vim$(VERSION)src.tar.gz
	-rm -rf dist/Vim
	mkdir dist/Vim
	mkdir dist/Vim/$(VIMRTDIR)
	tar cf - \
		$(ROOT_AMI) \
		$(SRC_ALL) \
		$(SRC_AMI) \
		$(SRC_AMI_DOS) \
		| (cd dist/Vim/$(VIMRTDIR); tar xf -)
	mv dist/Vim/$(VIMRTDIR)/vimdir.info dist/Vim.info
	mv dist/Vim/$(VIMRTDIR)/runtime.info dist/Vim/$(VIMRTDIR).info
	cd dist && tar cf vim$(VERSION)src.tar Vim Vim.info
	gzip -9 dist/vim$(VERSION)src.tar
	mv dist/vim$(VERSION)src.tar.gz dist/vim$(VERSION)src.tgz

no_title.vim: Makefile
	echo "set notitle noicon nocp nomodeline viminfo=" >no_title.vim

# MS-DOS sources
dossrc: dist no_title.vim dist/$(COMMENT_SRC) runtime/doc/uganda.nsis.txt
	-rm -rf dist/vim$(VERSION)src.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(SRC_ALL) \
		$(SRC_DOS) \
		$(SRC_AMI_DOS) \
		$(SRC_DOS_UNIX) \
		runtime/doc/uganda.nsis.txt \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	mv dist/vim/$(VIMRTDIR)/runtime/* dist/vim/$(VIMRTDIR)
	rmdir dist/vim/$(VIMRTDIR)/runtime
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -X -u no_title.vim -c ":set tx|wq" {} \;
	tar cf - \
		$(SRC_DOS_BIN) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	cd dist && zip -9 -rD -z vim$(VERSION)src.zip vim <$(COMMENT_SRC)

runtime/doc/uganda.nsis.txt: runtime/doc/uganda.txt
	cd runtime/doc && $(MAKE) uganda.nsis.txt

dosrt: dist dist/$(COMMENT_RT) dosrt_unix2dos
	-rm -rf dist/vim$(VERSION)rt.zip
	cd dist && zip -9 -rD -z vim$(VERSION)rt.zip vim <$(COMMENT_RT)

# Split in two parts to avoid an "argument list too long" error.
dosrt_unix2dos: dist prepare no_title.vim
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	mkdir dist/vim/$(VIMRTDIR)/lang
	cd src && MAKEMO=yes $(MAKE) languages
	tar cf - \
		$(RT_ALL) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	tar cf - \
		$(RT_SCRIPTS) \
		$(RT_DOS) \
		$(RT_NO_UNIX) \
		$(RT_AMI_DOS) \
		$(LANG_GEN) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -X -u no_title.vim -c ":set tx|wq" {} \;
	tar cf - \
		$(RT_UNIX_DOS_BIN) \
		$(RT_ALL_BIN) \
		$(RT_DOS_BIN) \
		$(LANG_GEN_BIN) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	mv dist/vim/$(VIMRTDIR)/runtime/* dist/vim/$(VIMRTDIR)
	rmdir dist/vim/$(VIMRTDIR)/runtime
# Add the message translations.  Trick: skip ja.mo and use ja.sjis.mo instead.
# Same for cs.mo / cs.cp1250.mo, pl.mo / pl.cp1250.mo, sk.mo / sk.cp1250.mo,
# zh_CN.mo / zh_CN.cp936.mo, uk.mo / uk.cp1251.mo and ru.mo / ru.cp1251.mo.
	for i in $(LANG_DOS); do \
	      if test "$$i" != "src/po/ja.mo" -a "$$i" != "src/po/pl.mo" -a "$$i" != "src/po/cs.mo" -a "$$i" != "src/po/sk.mo" -a "$$i" != "src/po/zh_CN.mo" -a "$$i" != "src/po/ru.mo" -a "$$i" != "src/po/uk.mo"; then \
		n=`echo $$i | sed -e "s+src/po/\([-a-zA-Z0-9_]*\(.UTF-8\)*\)\(.sjis\)*\(.cp1250\)*\(.cp1251\)*\(.cp936\)*.mo+\1+"`; \
		mkdir dist/vim/$(VIMRTDIR)/lang/$$n; \
		mkdir dist/vim/$(VIMRTDIR)/lang/$$n/LC_MESSAGES; \
		cp $$i dist/vim/$(VIMRTDIR)/lang/$$n/LC_MESSAGES/vim.mo; \
	      fi \
	    done
	cp libintl.dll dist/vim/$(VIMRTDIR)/


# Convert runtime files from Unix fileformat to dos fileformat.
# Used before uploading.  Don't delete the AAPDIR/sign files!
runtime_unix2dos: dosrt_unix2dos
	-rm -rf `find runtime/dos -type f -print | sed -e /AAPDIR/d`
	cd dist/vim/$(VIMRTDIR); tar cf - * \
		| (cd ../../../runtime/dos; tar xf -)

dosbin: prepare dosbin_gvim dosbin_w32 dosbin_d32 dosbin_ole $(DOSBIN_S) $(DOSBIN_D16)

# make Win32 gvim
dosbin_gvim: dist no_title.vim dist/$(COMMENT_GVIM)
	-rm -rf dist/gvim$(VERSION).zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_DOS) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -X -u no_title.vim -c ":set tx|wq" {} \;
	cp gvim.exe dist/vim/$(VIMRTDIR)/gvim.exe
	cp xxdw32.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp vimrun.exe dist/vim/$(VIMRTDIR)/vimrun.exe
	cp installw32.exe dist/vim/$(VIMRTDIR)/install.exe
	cp uninstalw32.exe dist/vim/$(VIMRTDIR)/uninstal.exe
	cp gvimext.dll dist/vim/$(VIMRTDIR)/gvimext.dll
	cd dist && zip -9 -rD -z gvim$(VERSION).zip vim <$(COMMENT_GVIM)
	cp gvim.pdb dist/gvim$(VERSION).pdb

# make Win32 console
dosbin_w32: dist no_title.vim dist/$(COMMENT_W32)
	-rm -rf dist/vim$(VERSION)w32.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_DOS) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -X -u no_title.vim -c ":set tx|wq" {} \;
	cp vimw32.exe dist/vim/$(VIMRTDIR)/vim.exe
	cp xxdw32.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp installw32.exe dist/vim/$(VIMRTDIR)/install.exe
	cp uninstalw32.exe dist/vim/$(VIMRTDIR)/uninstal.exe
	cd dist && zip -9 -rD -z vim$(VERSION)w32.zip vim <$(COMMENT_W32)
	cp vimw32.pdb dist/vim$(VERSION)w32.pdb

# make 32bit DOS
dosbin_d32: dist no_title.vim dist/$(COMMENT_D32)
	-rm -rf dist/vim$(VERSION)d32.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_DOS) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -X -u no_title.vim -c ":set tx|wq" {} \;
	cp vimd32.exe dist/vim/$(VIMRTDIR)/vim.exe
	cp xxdd32.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp installd32.exe dist/vim/$(VIMRTDIR)/install.exe
	cp uninstald32.exe dist/vim/$(VIMRTDIR)/uninstal.exe
	cp csdpmi4b.zip dist/vim/$(VIMRTDIR)
	cd dist && zip -9 -rD -z vim$(VERSION)d32.zip vim <$(COMMENT_D32)

# make 16bit DOS
dosbin_d16: dist no_title.vim dist/$(COMMENT_D16)
	-rm -rf dist/vim$(VERSION)d16.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_DOS) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -X -u no_title.vim -c ":set tx|wq" {} \;
	cp vimd16.exe dist/vim/$(VIMRTDIR)/vim.exe
	cp xxdd16.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp installd16.exe dist/vim/$(VIMRTDIR)/install.exe
	cp uninstald16.exe dist/vim/$(VIMRTDIR)/uninstal.exe
	cd dist && zip -9 -rD -z vim$(VERSION)d16.zip vim <$(COMMENT_D16)

# make Win32 gvim with OLE
dosbin_ole: dist no_title.vim dist/$(COMMENT_OLE)
	-rm -rf dist/gvim$(VERSION)ole.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_DOS) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -X -u no_title.vim -c ":set tx|wq" {} \;
	cp gvim_ole.exe dist/vim/$(VIMRTDIR)/gvim.exe
	cp xxdw32.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp vimrun.exe dist/vim/$(VIMRTDIR)/vimrun.exe
	cp installw32.exe dist/vim/$(VIMRTDIR)/install.exe
	cp uninstalw32.exe dist/vim/$(VIMRTDIR)/uninstal.exe
	cp gvimext.dll dist/vim/$(VIMRTDIR)/gvimext.dll
	cp README_ole.txt dist/vim/$(VIMRTDIR)
	cp src/VisVim/VisVim.dll dist/vim/$(VIMRTDIR)/VisVim.dll
	cp src/VisVim/README_VisVim.txt dist/vim/$(VIMRTDIR)
	cd dist && zip -9 -rD -z gvim$(VERSION)ole.zip vim <$(COMMENT_OLE)
	cp gvim_ole.pdb dist/gvim$(VERSION)ole.pdb

# make Win32s gvim
dosbin_s: dist no_title.vim dist/$(COMMENT_W32S)
	-rm -rf dist/gvim$(VERSION)_s.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_DOS) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -X -u no_title.vim -c ":set tx|wq" {} \;
	cp gvim_w32s.exe dist/vim/$(VIMRTDIR)/gvim.exe
	cp xxdd32.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp README_w32s.txt dist/vim/$(VIMRTDIR)
	cp installw32.exe dist/vim/$(VIMRTDIR)/install.exe
	cp uninstalw32.exe dist/vim/$(VIMRTDIR)/uninstal.exe
	cd dist && zip -9 -rD -z gvim$(VERSION)_s.zip vim <$(COMMENT_W32S)

os2bin: dist no_title.vim dist/$(COMMENT_OS2)
	-rm -rf dist/vim$(VERSION)os2.zip
	-rm -rf dist/vim
	mkdir dist/vim
	mkdir dist/vim/$(VIMRTDIR)
	tar cf - \
		$(BIN_OS2) \
		| (cd dist/vim/$(VIMRTDIR); tar xf -)
	find dist/vim/$(VIMRTDIR) -type f -exec $(VIM) -e -X -u no_title.vim -c ":set tx|wq" {} \;
	cp vimos2.exe dist/vim/$(VIMRTDIR)/vim.exe
	cp xxdos2.exe dist/vim/$(VIMRTDIR)/xxd.exe
	cp teeos2.exe dist/vim/$(VIMRTDIR)/tee.exe
	cp emx.dll emxlibcs.dll dist/vim/$(VIMRTDIR)
	cd dist && zip -9 -rD -z vim$(VERSION)os2.zip vim <$(COMMENT_OS2)

html: dist dist/$(COMMENT_HTML)
	-rm -rf dist/vim$(VERSION)html.zip
	cd runtime/doc && zip -9 -z ../../dist/vim$(VERSION)html.zip *.html <../../dist/$(COMMENT_HTML)

farsi: dist dist/$(COMMENT_FARSI)
	-rm -f dist/farsi$(VERSION).zip
	zip -9 -rD -z dist/farsi$(VERSION).zip farsi < dist/$(COMMENT_FARSI)
