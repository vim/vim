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
VIM = ..\vim

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

PO_INPUTLIST = \
	..\*.c \
	..\if_perl.xs \
	..\GvimExt\gvimext.cpp \
	..\errors.h \
	..\globals.h \
	..\if_py_both.h \
	..\vim.h \
	gvim.desktop.in \
	vim.desktop.in

PO_VIM_INPUTLIST = \
	..\..\runtime\optwin.vim

PO_VIM_JSLIST = \
	optwin.js

files: $(PO_INPUTLIST) $(PO_VIM_INPUTLIST)
	$(LS) $(LSFLAGS) $(PO_INPUTLIST) > .\files
	echo $(PO_VIM_JSLIST)>> .\files

first_time: files
	$(VIM) -u NONE --not-a-term -S tojavascript.vim $(LANGUAGE).pot $(PO_VIM_INPUTLIST)
	set OLD_PO_FILE_INPUT=yes
	set OLD_PO_FILE_OUTPUT=yes
	$(XGETTEXT) --default-domain=$(LANGUAGE) --add-comments --keyword=_ --keyword=N_ --keyword=NGETTEXT:1,2 --files-from=.\files
	$(VIM) -u NONE --not-a-term -S fixfilenames.vim $(LANGUAGE).pot $(PO_VIM_INPUTLIST)
	$(RM) *.js

$(PACKAGE).pot: files
	$(VIM) -u NONE --not-a-term -S tojavascript.vim $(PACKAGE).pot $(PO_VIM_INPUTLIST)
	set OLD_PO_FILE_INPUT=yes
	set OLD_PO_FILE_OUTPUT=yes
	$(XGETTEXT) --default-domain=$(PACKAGE) --add-comments --keyword=_ --keyword=N_ --keyword=NGETTEXT:1,2 --files-from=.\files
	$(MV) $(PACKAGE).po $(PACKAGE).pot
	$(VIM) -u NONE --not-a-term -S fixfilenames.vim $(PACKAGE).pot $(PO_VIM_INPUTLIST)
	$(RM) *.js

# Don't add a dependency here, we only want to update the .po files manually
$(LANGUAGES):
	@$(MAKE) -nologo -f Make_mvc.mak $(PACKAGE).pot GETTEXT_PATH=$(GETTEXT_PATH)
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
	$(RM) files
