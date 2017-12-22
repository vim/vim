# Makefile for the Vim message translations for mingw32
#
# Eduardo F. Amatria <eferna1@platea.pntic.mec.es>
#
# Read the README_ming.txt file before using it.
#
# Use at your own risk but with care, it could even kill your canary.
#
# Previous to all you must have the environment variable LANGUAGE set to your
# language (xx) and add it to the next three lines.
#

ifndef VIMRUNTIME
ifeq (sh.exe, $(SHELL))
VIMRUNTIME = ..\..\runtime
else
VIMRUNTIME = ../../runtime
endif
endif

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

# Uncomment one of the lines below or modify it to put the path to your
# gettex binaries; I use the first
#GETTEXT_PATH = C:/gettext.win32/bin/
#GETTEXT_PATH = C:/gettext-0.10.35-w32/win32/Release/
#GETTEXT_PATH = C:/cygwin/bin/

ifeq (sh.exe, $(SHELL))
MSGFMT = set OLD_PO_FILE_INPUT=yes && $(GETTEXT_PATH)msgfmt -v
XGETTEXT = set OLD_PO_FILE_INPUT=yes && set OLD_PO_FILE_OUTPUT=yes && $(GETTEXT_PATH)xgettext
MSGMERGE = set OLD_PO_FILE_INPUT=yes && set OLD_PO_FILE_OUTPUT=yes && $(GETTEXT_PATH)msgmerge
else
MSGFMT = LANG=C OLD_PO_FILE_INPUT=yes $(GETTEXT_PATH)msgfmt -v
XGETTEXT = LANG=C OLD_PO_FILE_INPUT=yes OLD_PO_FILE_OUTPUT=yes $(GETTEXT_PATH)xgettext
MSGMERGE = LANG=C OLD_PO_FILE_INPUT=yes OLD_PO_FILE_OUTPUT=yes $(GETTEXT_PATH)msgmerge
endif

ifeq (sh.exe, $(SHELL))
MV = move
CP = copy
RM = del
MKD = mkdir
else
MV = mv -f
CP = cp -f
RM = rm -f
MKD = mkdir -p
endif

.SUFFIXES:
.SUFFIXES: .po .mo .pot
.PHONY: first_time all install clean $(LANGUAGES)

.po.mo:
	$(MSGFMT) -o $@ $<

all: $(MOFILES)

first_time:
	$(XGETTEXT) --default-domain=$(LANGUAGE) \
		--add-comments --keyword=_ --keyword=N_ $(wildcard ../*.c) ../if_perl.xs ../GvimExt/gvimext.cpp $(wildcard ../globals.h) ../if_py_both.h

$(LANGUAGES):
	$(XGETTEXT) --default-domain=$(PACKAGE) \
		--add-comments --keyword=_ --keyword=N_ $(wildcard ../*.c) ../if_perl.xs ../GvimExt/gvimext.cpp $(wildcard ../globals.h) ../if_py_both.h
	$(MV) $(PACKAGE).po $(PACKAGE).pot
	$(CP) $@.po $@.po.orig
	$(MV) $@.po $@.po.old
	$(MSGMERGE) $@.po.old $(PACKAGE).pot -o $@.po
	$(RM) $@.po.old

install:
	$(MKD) $(VIMRUNTIME)\lang\$(LANGUAGE)
	$(MKD) $(VIMRUNTIME)\lang\$(LANGUAGE)\LC_MESSAGES
	$(CP) $(LANGUAGE).mo $(VIMRUNTIME)\lang\$(LANGUAGE)\LC_MESSAGES\$(PACKAGE).mo

ifeq (sh.exe, $(SHELL))
install-all: all
	FOR %%l IN ($(LANGUAGES)) DO @IF NOT EXIST $(VIMRUNTIME)\lang\%%l $(MKD) $(VIMRUNTIME)\lang\%%l
	FOR %%l IN ($(LANGUAGES)) DO @IF NOT EXIST $(VIMRUNTIME)\lang\%%l\LC_MESSAGES $(MKD) $(VIMRUNTIME)\lang\%%l\LC_MESSAGES
	FOR %%l IN ($(LANGUAGES)) DO @$(CP) %%l.mo $(VIMRUNTIME)\lang\%%l\LC_MESSAGES\$(PACKAGE).mo
else
install-all: all
	for TARGET in $(LANGUAGES); do \
		$(MKD) $(VIMRUNTIME)/lang/$$TARGET/LC_MESSAGES ; \
		$(CP) $$TARGET.mo $(VIMRUNTIME)/lang/$$TARGET/LC_MESSAGES/$(PACKAGE).mo ; \
	done
endif

clean:
	$(RM) *.mo
	$(RM) *.pot


