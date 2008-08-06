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

LANGUAGES = \
		af \
		ca \
		cs \
		de \
		en_GB \
		eo \
		es \
		fi \
		fr \
		ga \
		it \
		ja \
		ko \
		no \
		pl \
		pt_BR \
		ru \
		sk \
		sv \
		uk \
		vi \
		zh_CN \
		zh_CN.UTF-8\
		zh_TW \
		zh_TW.UTF-8 \

MOFILES = \
		af.mo \
		ca.mo \
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
		ko.mo \
		no.mo \
		pl.mo \
		pt_BR.mo \
		ru.mo \
		sk.mo \
		sv.mo \
		uk.mo \
		vi.mo \
		zh_CN.UTF-8.mo \
		zh_CN.mo \
		zh_TW.UTF-8.mo \
		zh_TW.mo \

PACKAGE = vim

# Uncomment one of the lines below or modify it to put the path to your
# gettex binaries; I use the first
#GETTEXT_PATH = C:/gettext.win32/bin/
#GETTEXT_PATH = C:/gettext-0.10.35-w32/win32/Release/
#GETTEXT_PATH = C:/cygwin/bin/

MSGFMT = $(GETTEXT_PATH)msgfmt
XGETTEXT = $(GETTEXT_PATH)xgettext
MSGMERGE = $(GETTEXT_PATH)msgmerge

MV = move
CP = copy
RM = del
MKD = mkdir

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

install:
	$(MKD) $(VIMRUNTIME)\lang\$(LANGUAGE)
	$(MKD) $(VIMRUNTIME)\lang\$(LANGUAGE)\LC_MESSAGES
	$(CP) $(LANGUAGE).mo $(VIMRUNTIME)\lang\$(LANGUAGE)\LC_MESSAGES\$(PACKAGE).mo

clean:
	$(RM) *.mo
	$(RM) *.pot


