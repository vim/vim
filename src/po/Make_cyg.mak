# Makefile for the Vim message translations for Cygwin
# by Tony Mechelynck <antoine.mechelynck@skynet.be>
# after Make_ming.mak by
# Eduardo F. Amatria <eferna1@platea.pntic.mec.es>
#
# Read the README_ming.txt file before using it.
#
# Use at your own risk but with care, it could even kill your canary.
#

ifndef VIMRUNTIME
VIMRUNTIME = ../../runtime
endif

LANGUAGES =	af \
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
		ja.sjis \
		ko \
		ko.UTF-8 \
		no \
		pl \
		pl.cp1250 \
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
		zh_CN.UTF-8 \
		zh_CN.cp936 \
		zh_TW \
		zh_TW.UTF-8 \

MOFILES =	af.mo \
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
		ja.mo \
		ja.sjis.mo \
		ko.mo \
		ko.UTF-8.mo \
		no.mo \
		pl.cp1250.mo \
		pl.mo \
		pt_BR.mo \
		ru.cp1251.mo \
		ru.mo \
		sk.cp1250.mo \
		sk.mo \
		sv.mo \
		uk.cp1251.mo \
		uk.mo \
		vi.mo \
		zh_CN.UTF-8.mo \
		zh_CN.cp936.mo \
		zh_CN.mo \
		zh_TW.UTF-8.mo \
		zh_TW.mo \

PACKAGE = vim

# Uncomment one of the lines below or modify it to put the path to your
# gettext binaries
ifndef GETTEXT_PATH
#GETTEXT_PATH = C:/gettext.win32/bin/
#GETTEXT_PATH = C:/gettext-0.10.35-w32/win32/Release/
GETTEXT_PATH = /bin/
endif

# The OLD_PO_FILE_INPUT and OLD_PO_FILE_OUTPUT are for the new GNU gettext
# tools 0.10.37, which use a slightly different .po file format that is not
# compatible with Solaris (and old gettext implementations) unless these are
# set.  gettext 0.10.36 will not work!
MSGFMT = OLD_PO_FILE_INPUT=yes $(GETTEXT_PATH)msgfmt -v
XGETTEXT = OLD_PO_FILE_INPUT=yes OLD_PO_FILE_OUTPUT=yes $(GETTEXT_PATH)xgettext
MSGMERGE = OLD_PO_FILE_INPUT=yes OLD_PO_FILE_OUTPUT=yes $(GETTEXT_PATH)msgmerge

# MV = move
# CP = copy
# RM = del
# MKD = mkdir
MV = mv -f
CP = cp -f
RM = rm -f
MKD = mkdir -p

.SUFFIXES:
.SUFFIXES: .po .mo .pot
.PHONY: first_time all install clean $(LANGUAGES)

.po.mo:
	$(MSGFMT) -o $@ $<

all: $(MOFILES)

first_time:
	$(XGETTEXT) --default-domain=$(LANGUAGE) \
		--add-comments --keyword=_ --keyword=N_ $(wildcard ../*.c) ../if_perl.xs $(wildcard ../globals.h)

$(LANGUAGES):
	$(XGETTEXT) --default-domain=$(PACKAGE) \
		--add-comments --keyword=_ --keyword=N_ $(wildcard ../*.c) ../if_perl.xs $(wildcard ../globals.h)
	$(MV) $(PACKAGE).po $(PACKAGE).pot
	$(CP) $@.po $@.po.orig
	$(MV) $@.po $@.po.old
	$(MSGMERGE) $@.po.old $(PACKAGE).pot -o $@.po
	$(RM) $@.po.old

install: $(MOFILES)
	for TARGET in $(LANGUAGES); do \
		$(MKD) $(VIMRUNTIME)/lang/$$TARGET/LC_MESSAGES ; \
		$(CP) $$TARGET.mo $(VIMRUNTIME)/lang/$$TARGET/LC_MESSAGES/$(PACKAGE).mo ; \
	done

clean:
	$(RM) *.mo
	$(RM) *.pot


