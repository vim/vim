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

LANGUAGES = \
		af \
		ca \
		cs \
		cs.cp1250 \
		de \
		en_GB \
		eo \
		es \
		fi \
		fr \
		ga \
		it \
		ja \
		ja.euc-jp \
		ja.sjis \
		ko \
		ko.UTF-8 \
		nb \
		nl \
		no \
		pl \
		pl.cp1250 \
		pl.UTF-8 \
		pt_BR \
		ru \
		ru.cp1251 \
		sk \
		sk.cp1250 \
		sv \
		uk \
		uk.cp1251 \
		vi \
		zh_CN \
		zh_CN.cp936 \
		zh_CN.UTF-8 \
		zh_TW \
		zh_TW.UTF-8 \

MOFILES = \
		af.mo \
		ca.mo \
		cs.cp1250.mo \
		cs.mo \
		de.mo \
		en_GB.mo \
		eo.mo \
		es.mo \
		fi.mo \
		fr.mo \
		ga.mo \
		it.mo \
		ja.euc-jp.mo \
		ja.mo \
		ja.sjis.mo \
		ko.mo \
		ko.UTF-8.mo \
		nb.mo \
		nl.mo \
		no.mo \
		pl.cp1250.mo \
		pl.mo \
		pl.UTF-8.mo \
		pt_BR.mo \
		ru.cp1251.mo \
		ru.mo \
		sk.cp1250.mo \
		sk.mo \
		sv.mo \
		uk.cp1251.mo \
		uk.mo \
		vi.mo \
		zh_CN.mo \
		zh_CN.cp936.mo \
		zh_CN.UTF-8.mo \
		zh_TW.mo \
		zh_TW.UTF-8.mo \

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

all: $(MOFILES)

files:
	$(LS) $(LSFLAGS) ..\*.c ..\if_perl.xs ..\GvimExt\gvimext.cpp ..\globals.h ..\if_py_both.h > .\files

first_time: files
	set OLD_PO_FILE_INPUT=yes
	set OLD_PO_FILE_OUTPUT=yes
	$(XGETTEXT) --default-domain=$(LANGUAGE) --add-comments --keyword=_ --keyword=N_ --files-from=.\files

$(LANGUAGES): files
	set OLD_PO_FILE_INPUT=yes
	set OLD_PO_FILE_OUTPUT=yes
	$(XGETTEXT) --default-domain=$(PACKAGE) --add-comments --keyword=_ --keyword=N_ --files-from=.\files
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
