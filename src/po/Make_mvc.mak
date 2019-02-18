# Makefile for the Vim message translations for MSVC
# (based on make_ming.mak)
#
# Mike Williams <mrw@eandem.co.uk>
#
# Please read README_mvc.txt before using this file.
#

!ifndef VIMRUNTIME
VIMRUNTIME = ..\..\runtime
!endif

# get LANGUAGES, MOFILES and MOCONVERTED
!include Make_all.mak

PACKAGE = vim

# Correct the following line for the directory where gettext et al is installed
GETTEXT_PATH = H:\gettext.0.14.4\bin

MSGFMT = $(GETTEXT_PATH)\msgfmt -v
XGETTEXT = $(GETTEXT_PATH)\xgettext
MSGMERGE = $(GETTEXT_PATH)\msgmerge

MV = move
CP = copy
RM = del
MKD = mkdir
LS = dir

LSFLAGS = /b /on /l /s

INSTALLDIR = $(VIMRUNTIME)\lang\$(LANGUAGE)\LC_MESSAGES

.SUFFIXES:
.SUFFIXES: .po .mo .pot

.po.mo:
	set OLD_PO_FILE_INPUT=yes
	$(MSGFMT) -o $@ $<

all: $(MOFILES) $(MOCONVERTED)

files:
	$(LS) $(LSFLAGS) ..\*.c ..\if_perl.xs ..\GvimExt\gvimext.cpp ..\globals.h ..\if_py_both.h ..\vim.h > .\files

first_time: files
	set OLD_PO_FILE_INPUT=yes
	set OLD_PO_FILE_OUTPUT=yes
	$(XGETTEXT) --default-domain=$(LANGUAGE) --add-comments --keyword=_ --keyword=N_ --keyword=NGETTEXT:1,2 --files-from=.\files

$(LANGUAGES): files
	set OLD_PO_FILE_INPUT=yes
	set OLD_PO_FILE_OUTPUT=yes
	$(XGETTEXT) --default-domain=$(PACKAGE) --add-comments --keyword=_ --keyword=N_ --keyword=NGETTEXT:1,2 --files-from=.\files
	$(MV) $(PACKAGE).po $(PACKAGE).pot
	$(CP) $@.po $@.po.orig
	$(MV) $@.po $@.po.old
	$(MSGMERGE) $@.po.old $(PACKAGE).pot -o $@.po
	$(RM) $@.po.old

install:
	if not exist $(INSTALLDIR) $(MKD) $(INSTALLDIR)
	$(CP) $(LANGUAGE).mo $(INSTALLDIR)\$(PACKAGE).mo

install-all: all
	FOR %%l IN ($(LANGUAGES)) DO @IF NOT EXIST $(VIMRUNTIME)\lang\%%l\LC_MESSAGES $(MKD) $(VIMRUNTIME)\lang\%%l\LC_MESSAGES
	FOR %%l IN ($(LANGUAGES)) DO @$(CP) %%l.mo $(VIMRUNTIME)\lang\%%l\LC_MESSAGES\$(PACKAGE).mo

clean:
	$(RM) *.mo
	$(RM) *.pot
