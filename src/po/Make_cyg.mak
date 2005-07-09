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

LANGUAGES =	af ca cs de en_GB es fr ga it ja ko no pl ru sk sv uk vi zh_TW \
		cs.cp1250 ja.sjis pl.cp1250 ru.cp1251 sk.cp1250 uk.cp1251 \
		zh_TW.UTF-8 zh_CN zh_CN.cp936 zh_CN.UTF-8
MOFILES =	af.mo ca.mo cs.mo de.mo en_GB.mo es.mo fr.mo ga.mo it.mo ja.mo \
		ko.mo no.mo pl.mo ru.mo sk.mo sv.mo uk.mo vi.mo \
		cs.cp1250.mo ja.sjis.mo pl.cp1250.mo ru.cp1251.mo sk.cp1250.mo uk.cp1251.mo \
		zh_TW.mo zh_TW.UTF-8.mo zh_CN.mo zh_CN.cp936.mo zh_CN.UTF-8.mo

PACKAGE = vim

# Uncomment one of the lines below or modify it to put the path to your
# gettex binaries; I use the first
ifndef GETTEXT_PATH
#GETTEXT_PATH = C:/gettext.win32/bin/
#GETTEXT_PATH = C:/gettext-0.10.35-w32/win32/Release/
GETTEXT_PATH = /bin/
endif

MSGFMT = $(GETTEXT_PATH)msgfmt
XGETTEXT = $(GETTEXT_PATH)xgettext
MSGMERGE = $(GETTEXT_PATH)msgmerge

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


