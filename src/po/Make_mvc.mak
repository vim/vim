# Makefile for the Vim message translations for MSVC
# (based on make_ming.mak)
#
# Mike Williams <mrw@eandem.co.uk>
#
# Please read README_mvc.txt before using this file.
#

LANGUAGES =	af ca cs de en_GB es fr it ja ko no pl ru sk sv uk zh_TW \
		zh_TW.UTF-8 zh_CN zh_CN.UTF-8
MOFILES =	af.mo ca.mo cs.mo de.mo en_GB.mo es.mo fr.mo it.mo ja.mo \
		ko.mo no.mo pl.mo ru.mo sk.mo sv.mo uk.mo \
		zh_TW.mo zh_TW.UTF-8.mo zh_CN.mo zh_CN.UTF-8.mo

PACKAGE = vim

# Correct the following line for the installation directory of gettext
GETTEXT_PATH = H:\gettext.win32.msvcrt\bin

MSGFMT = $(GETTEXT_PATH)\msgfmt
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
	$(MSGFMT) -o $@ $<

all: $(MOFILES)

files:
	$(LS) $(LSFLAGS) ..\*.c ..\if_perl.xs ..\globals.h > .\files

first_time: files
	$(XGETTEXT) --default-domain=$(LANGUAGE) --add-comments --keyword=_ --keyword=N_ --files-from=.\files

$(LANGUAGES): files
	$(XGETTEXT) --default-domain=$(PACKAGE) --add-comments --keyword=_ --keyword=N_ --files-from=.\files
	$(MV) $(PACKAGE).po $(PACKAGE).pot
	$(CP) $@.po $@.po.orig
	$(MV) $@.po $@.po.old
	$(MSGMERGE) $@.po.old $(PACKAGE).pot -o $@.po
	$(RM) $@.po.old

install:
	if not exist $(INSTALLDIR) $(MKD) $(INSTALLDIR)
	$(CP) $(LANGUAGE).mo $(INSTALLDIR)\$(PACKAGE).mo

clean:
	$(RM) *.mo
	$(RM) *.pot
