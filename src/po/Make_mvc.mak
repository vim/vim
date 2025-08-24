# Makefile for the Vim message translations for MSVC
# (based on make_ming.mak)
#
# Mike Williams, <mrw@eandem.co.uk>
# 2024-01-06, Restorer, <restorer@mail2k.ru>
#
# Please read README_mvc.txt before using this file.
#

# included common tools
!INCLUDE ..\auto\nmake\tools.mak

!IFNDEF LANGUAGE
! IF ![$(PS) $(PSFLAGS) Set-Content -Path .\_lng.tmp \
	-Value "LANGUAGE=$$((Get-UICulture).TwoLetterISOLanguageName)"]
!  INCLUDE .\_lng.tmp
!  IF [$(RM) .\_lng.tmp]
!  ENDIF
!  MESSAGE
!  MESSAGE The %LANGUAGE% environment variable is not set.
!  MESSAGE This variable will be temporarily set to "$(LANGUAGE)" while "nmake.exe" is running.
!  MESSAGE See README_mvc.txt for more information on the %LANGUAGE% environment variable.
!  MESSAGE
! ENDIF
!ELSE
! MESSAGE LANGUAGE is already set "$(LANGUAGE)"
!ENDIF

# Get LANGUAGES, MOFILES, MOCONVERTED and others.
!INCLUDE .\Make_all.mak

!IFNDEF VIMRUNTIME
VIMRUNTIME = ..\..\runtime
!ENDIF

# Correct the following line for the where executable file vim is
# installed.  Please do not put the path in quotes.
!IFNDEF VIMPROG
VIMPROG = ..\vim.exe
!ENDIF

# Correct the following line for the directory where gettext et al is
# installed.  Please do not put the path in quotes.
!IFNDEF GETTEXT_PATH
GETTEXT_PATH = D:\Programs\GetText\bin
!ENDIF

INSTALLDIR = $(VIMRUNTIME)\lang\$(LANGUAGE)\LC_MESSAGES
PACKAGE = vim

# Starting from version 0.22, msgfmt forcibly converts text to UTF-8 regardless
# of the value of the "charset" field.
!IF ![$(GETTEXT_PATH)\msgfmt.exe --help | 1> nul find "--no-convert"]
MSGFMT = "$(GETTEXT_PATH)\msgfmt.exe" -v --no-convert
!ELSE
MSGFMT = "$(GETTEXT_PATH)\msgfmt.exe" -v
!ENDIF
XGETTEXT = "$(GETTEXT_PATH)\xgettext.exe"
MSGMERGE = "$(GETTEXT_PATH)\msgmerge.exe"

# In case some package like GnuWin32, UnixUtils, gettext
# or something similar is installed on the system.
# If the "iconv" program is installed on the system, but it is not registered
# in the %PATH% environment variable, then specify the full path to this file.
!IF EXIST ("iconv.exe")
ICONV = iconv.exe
!ELSEIF EXIST ("$(GETTEXT_PATH)\iconv.exe")
ICONV = "$(GETTEXT_PATH)\iconv.exe"
!ENDIF

LSFLAGS = /B /ON /L /S

!IF ![$(PS) $(PSFLAGS) Set-Content -Path .\_year.tmp \
	-Value Year=$$((Get-Date).Year)]
! INCLUDE .\_year.tmp
! IF [$(RM) .\_year.tmp]
! ENDIF
!ENDIF

.SUFFIXES:
.SUFFIXES: .po .mo .pot .ck

all: $(MOFILES) $(MOCONVERTED)

originals : $(MOFILES)

converted: $(MOCONVERTED)

.po.ck:
	"$(VIMPROG)" -u NONE --noplugins -e -s --cmd "set enc=utf-8" \
		-S check.vim \
		-c "if error == 0 | q | else | num 2 | cq | endif" $<
	@ <<touch.bat $@
$(TOUCH)
<<

check: $(CHECKFILES)

checkclean:
	$(RM) *.ck

# Norwegian/Bokmal: "nb" is an alias for "no".
nb.po: no.po
	$(CP) no.po nb.po

# Convert ja.po to create ja.sjis.po.
ja.sjis.po: ja.po
	@ $(MAKE) -nologo -f Make_mvc.mak sjiscorr
	- $(RM) $@
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t CP932 $? | .\sjiscorr.exe > $@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(932))
	type $@ | .\sjiscorr.exe > $@.tmp
	@ $(MV) $@.tmp $@
!ENDIF
	$(PS) $(PSFLAGS) $$out = [System.IO.File]::ReadAllText(\"$@\", \
		[System.Text.Encoding]::GetEncoding(932)) \
		-replace \"`r`n\", \"`n\"; \
		[System.IO.File]::WriteAllText(\"$@\", $$out, \
		[System.Text.Encoding]::GetEncoding(932))

sjiscorr: sjiscorr.c
	$(CC) sjiscorr.c

# Convert ja.po to create ja.euc-jp.po.
ja.euc-jp.po: ja.po
	- $(RM) $@
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t EUC-JP $? > $@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(20932))
!ENDIF
	$(PS) $(PSFLAGS) $$out = [System.IO.File]::ReadAllText(\"$@\", \
		[System.Text.Encoding]::GetEncoding(20932)) -replace \
		'charset=utf-8', 'charset=EUC-JP' -replace \
		'# Original translations', \
		'# Generated from $?, DO NOT EDIT'; \
		[System.IO.File]::WriteAllText(\"$@\", $$out, \
		[System.Text.Encoding]::GetEncoding(20932))

# Convert cs.po to create cs.cp1250.po.
cs.cp1250.po: cs.po
	- $(RM) $@
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t CP1250 $? > $@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(28592)), \
		[System.Text.Encoding]::GetEncoding(1250))
!ENDIF
	$(PS) $(PSFLAGS) $$out = [System.IO.File]::ReadAllText(\"$@\", \
		[System.Text.Encoding]::GetEncoding(1250)) -replace \
		'charset=iso-8859-2', 'charset=CP1250' -replace \
		'# Original translations', \
		'# Generated from $?, DO NOT EDIT'; \
		[System.IO.File]::WriteAllText(\"$@\", $$out, \
		[System.Text.Encoding]::GetEncoding(1250))

# Convert pl.po to create pl.cp1250.po.
pl.cp1250.po: pl.po
	- $(RM) $@
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t CP1250 $? > $@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(28592)), \
		[System.Text.Encoding]::GetEncoding(1250))
!ENDIF
	$(PS) $(PSFLAGS) $$out = [System.IO.File]::ReadAllText(\"$@\", \
		[System.Text.Encoding]::GetEncoding(1250)) -replace \
		'charset=iso-8859-2', 'charset=CP1250' -replace \
		'# Original translations', \
		'# Generated from $?, DO NOT EDIT'; \
		[System.IO.File]::WriteAllText(\"$@\", $$out, \
		[System.Text.Encoding]::GetEncoding(1250))

# Convert pl.po to create pl.UTF-8.po.
pl.UTF-8.po: pl.po
	- $(RM) $@
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t UTF-8 $? > $@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(28592)))
!ENDIF
	$(PS) $(PSFLAGS) (Get-Content -Raw -Encoding UTF8 $@ \
		^| % {$$_-replace 'charset=iso-8859-2', 'charset=UTF-8' \
		-replace '# Original translations', \
		'# Generated from $?, DO NOT EDIT'}) \
		^| 1>nul New-Item -Path . -Name $@ -ItemType file -Force

# Convert sk.po to create sk.cp1250.po.
sk.cp1250.po: sk.po
	- $(RM) $@
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t CP1250 $? > $@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(28592)), \
		[System.Text.Encoding]::GetEncoding(1250))
!ENDIF
	$(PS) $(PSFLAGS) $$out = [System.IO.File]::ReadAllText(\"$@\", \
		[System.Text.Encoding]::GetEncoding(1250)) -replace \
		'charset=iso-8859-2', 'charset=CP1250' -replace \
		'# Original translations', \
		'# Generated from $?, DO NOT EDIT'; \
		[System.IO.File]::WriteAllText(\"$@\", $$out, \
		[System.Text.Encoding]::GetEncoding(1250))

# Convert zh_CN.UTF-8.po to create zh_CN.po.
zh_CN.po: zh_CN.UTF-8.po
	- $(RM) $@
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t GB2312 $? > $@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(936))

!ENDIF
	$(PS) $(PSFLAGS) $$out = [System.IO.File]::ReadAllText(\"$@\", \
		[System.Text.Encoding]::GetEncoding(936)) -replace \
		'charset=UTF-8', 'charset=GB2312' -replace \
		'# Original translations', \
		'# Generated from $?, DO NOT EDIT'; \
		[System.IO.File]::WriteAllText(\"$@\", $$out, \
		[System.Text.Encoding]::GetEncoding(936))

# Convert zh_CN.UTF-8.po to create zh_CN.cp936.po.
# Set 'charset' to gbk to avoid that msfmt generates a warning.
# This used to convert from zh_CN.po, but that results in a conversion error.
zh_CN.cp936.po: zh_CN.UTF-8.po
	- $(RM) $@
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t CP936 $? > $@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(20936))

!ENDIF
	$(PS) $(PSFLAGS) $$out = [System.IO.File]::ReadAllText(\"$@\", \
		[System.Text.Encoding]::GetEncoding(20936)) -replace \
		'charset=UTF-8', 'charset=GBK' -replace \
		'# Original translations', \
		'# Generated from $?, DO NOT EDIT'; \
		[System.IO.File]::WriteAllText(\"$@\", $$out, \
		[System.Text.Encoding]::GetEncoding(20936))

# Convert zh_TW.UTF-8.po to create zh_TW.po.
zh_TW.po: zh_TW.UTF-8.po
	- $(RM) $@
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t BIG5 $? > $@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(950))
!ENDIF
	$(PS) $(PSFLAGS) $$out = [System.IO.File]::ReadAllText(\"$@\", \
		[System.Text.Encoding]::GetEncoding(950)) -replace \
		'charset=UTF-8', 'charset=BIG5' -replace \
		'# Original translations', \
		'# Generated from $?, DO NOT EDIT'; \
		[System.IO.File]::WriteAllText(\"$@\", $$out, \
		[System.Text.Encoding]::GetEncoding(950))

# Convert zh_TW.UTF-8.po to create zh_TW.po with backslash characters.
# Requires doubling backslashes in the second byte.  Don't depend on big5corr,
# it should only be compiled when zh_TW.po is outdated.

#
#  06.11.23, added by Restorer:
#  For more details, see:
#  https://github.com/vim/vim/pull/3261
#  https://github.com/vim/vim/pull/3476
#  https://github.com/vim/vim/pull/12153
#  (read all comments)
#
#  I checked the workability on the list of backslash characters
#  specified in zh_TW.UTF-8.po. It works.
#  But it is better to have someone native speaker check it.
#

#zh_TW.po: zh_TW.UTF-8.po
#	@$(MAKE) -nologo -f Make_mvc.mak big5corr
#	- $(RM) $@
#!IF DEFINED (ICONV)
#	$(ICONV) -f UTF-8 -t BIG5 $? | .\big5corr.exe > $@
#!ELSE
#	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
#		[System.IO.File]::ReadAllText(\"$?\", \
#		[System.Text.Encoding]::GetEncoding(65001)), \
#		[System.Text.Encoding]::GetEncoding(950))
#	type $@ | .\big5corr.exe > tmp.$@
#	@$(MV) tmp.$@ $@
#!ENDIF
#	$(PS) $(PSFLAGS) $$out = [System.IO.File]::ReadAllText(\"$@\", \
#		[System.Text.Encoding]::GetEncoding(950)) \
#		-replace \"`r`n\", \"`n\"; \
#		[System.IO.File]::WriteAllText(\"$@\", $$out, \
#		[System.Text.Encoding]::GetEncoding(950))

# See above in the zh_TW.po conversion section for backslashes.
#big5corr: big5corr.c
#	$(CC) big5corr.c

# Convert ko.UTF-8.po to create ko.po.
ko.po: ko.UTF-8.po
	- $(RM) $@
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t EUC-KR $? > $@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(51949))

!ENDIF
	$(PS) $(PSFLAGS) $$out = [System.IO.File]::ReadAllText(\"$@\", \
		[System.Text.Encoding]::GetEncoding(51949)) -replace \
		'charset=UTF-8', 'charset=EUC-KR' -replace \
		'# Original translations', \
		'# Generated from $?, DO NOT EDIT'; \
		[System.IO.File]::WriteAllText(\"$@\", $$out, \
		[System.Text.Encoding]::GetEncoding(51949))

# Convert ru.po to create ru.cp1251.po.
ru.cp1251.po: ru.po
	- $(RM) $@
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t CP1251 $? > $@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(1251))
!ENDIF
	$(PS) $(PSFLAGS) $$out = [System.IO.File]::ReadAllText(\"$@\", \
		[System.Text.Encoding]::GetEncoding(1251)) -replace \
		'charset=UTF-8', 'charset=CP1251' -replace \
		'# Original translations', \
		'# Generated from $?, DO NOT EDIT'; \
		[System.IO.File]::WriteAllText(\"$@\", $$out, \
		[System.Text.Encoding]::GetEncoding(1251))

# Convert uk.po to create uk.cp1251.po.
uk.cp1251.po: uk.po
	- $(RM) $@
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t CP1251 $? > $@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(1251))
!ENDIF
	$(PS) $(PSFLAGS) $$out = [System.IO.File]::ReadAllText(\"$@\", \
		[System.Text.Encoding]::GetEncoding(1251)) -replace \
		'charset=UTF-8', 'charset=CP1251' -replace \
		'# Original translations', \
		'# Generated from $?, DO NOT EDIT'; \
		[System.IO.File]::WriteAllText(\"$@\", $$out, \
		[System.Text.Encoding]::GetEncoding(1251))

.po.mo:
	set OLD_PO_FILE_INPUT=yes
	$(MSGFMT) -o $@ $<

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

files: $(PO_INPUTLIST)
	$(LS) $(LSFLAGS) $(PO_INPUTLIST) > .\files

first_time: files
	"$(VIMPROG)" -u NONE --not-a-term -S tojavascript.vim $(LANGUAGE).po \
		$(PO_VIM_INPUTLIST)
	@ $(CP) /B .\files+.\vim_to_js .\allfiles
	set OLD_PO_FILE_INPUT=yes
	set OLD_PO_FILE_OUTPUT=yes
	$(XGETTEXT) --default-domain=$(LANGUAGE) --add-comments \
		$(XGETTEXT_KEYWORDS) --files-from=.\allfiles \
		--copyright-holder="$(Year), The Vim Project" \
		--package-name=Vim --msgid-bugs-address="vim-dev@vim.org"
	"$(VIMPROG)" -u NONE --not-a-term -S fixfilenames.vim $(LANGUAGE).po \
		$(PO_VIM_INPUTLIST)
	$(RM) *.js .\vim_to_js
	@ $(MAKE) -lf Make_mvc.mak clean

$(PACKAGE).pot: files
	"$(VIMPROG)" -u NONE --not-a-term -S tojavascript.vim $(PACKAGE).pot \
		$(PO_VIM_INPUTLIST)
	@ $(CP) /B .\files+.\vim_to_js .\allfiles
	set OLD_PO_FILE_INPUT=yes
	set OLD_PO_FILE_OUTPUT=yes
	$(XGETTEXT) --default-domain=$(PACKAGE) --output=$(PACKAGE).pot \
		--add-comments $(XGETTEXT_KEYWORDS) --files-from=.\allfiles \
		--no-location --copyright-holder="$(Year), The Vim Project" \
		--package-name=Vim --msgid-bugs-address="vim-dev@vim.org"
	"$(VIMPROG)" -u NONE --not-a-term -S fixfilenames.vim $(PACKAGE).pot \
		$(PO_VIM_INPUTLIST)
	$(RM) *.js .\vim_to_js
	@ $(MAKE) -lf Make_mvc.mak clean

# Only original translations with default encoding should be updated.
# The files that are converted to a different encoding clearly state "DO NOT EDIT".
update-po: $(MOFILES:.mo=)

# Don't add a dependency here, we only want to update the .po files manually.
$(LANGUAGES):
	@ $(MAKE) -lf Make_mvc.mak "GETTEXT_PATH=$(GETTEXT_PATH)" $(PACKAGE).pot
	$(CP) $@.po $@.po.orig
	$(MV) $@.po $@.po.old
	$(MSGMERGE) $@.po.old $(PACKAGE).pot -o $@.po
	$(RM) $@.po.old

install: $(LANGUAGE).mo
	if not exist "$(INSTALLDIR)" $(MKD) "$(INSTALLDIR)"
	$(CP) $(LANGUAGE).mo "$(INSTALLDIR)\$(PACKAGE).mo"

install-all: all
	for %%l in ($(LANGUAGES)) do \
		@if not exist "$(VIMRUNTIME)\lang\%%l\LC_MESSAGES" \
		$(MKD) "$(VIMRUNTIME)\lang\%%l\LC_MESSAGES"
	for %%l in ($(LANGUAGES)) do @$(CP) %%l.mo \
		"$(VIMRUNTIME)\lang\%%l\LC_MESSAGES\$(PACKAGE).mo"

cleanup-po: $(LANGUAGE).po
	@ "$(VIMPROG)" -u NONE -e -s -S cleanup.vim -c wq $(LANGUAGE).po

cleanup-po-all: $(POFILES)
	!@ "$(VIMPROG)" -u NONE -e -s -S cleanup.vim -c wq $**

#######
# For translations of plug-ins
#######

# Preparing the POT file of the plug-in package
POT_PLUGPACKAGE_PATH = $(MAKEDIR)
$(PLUGPACKAGE).pot : $(PO_PLUG_INPUTLIST)
	"$(VIMPROG)" -u NONE --not-a-term -S tojavascript.vim \
		$(PLUGPACKAGE).pot $**
	$(XGETTEXT) --from-code=UTF-8 --default-domain=$(PLUGPACKAGE) \
		--package-name=$(PLUGPACKAGE) \
		--output-dir="$(POT_PLUGPACKAGE_PATH)" \
		--output=$(PLUGPACKAGE).pot --files-from=.\vim_to_js
	"$(VIMPROG)" -u NONE --not-a-term -S fixfilenames.vim \
		"$(POT_PLUGPACKAGE_PATH)\$(PLUGPACKAGE).pot" $**
	$(RM) *.js .\vim_to_js

# Converting the PO file of the plug-in package to the binary format of the MO file
MO_PLUGPACKAGE_PATH = $(MAKEDIR)
$(PLUGPACKAGE).mo : $(PO_PLUGPACKAGE)
	$(MSGFMT) -o $(MO_PLUGPACKAGE_PATH)\$@ $?


clean: checkclean
	- $(RM) *.mo
	- $(RM) *.orig
	- $(RM) files allfiles
	- $(RM) sjiscorr.obj sjiscorr.exe
#	- $(RM) *.pot
#	- $(RM) big5corr.obj big5corr.exe

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=79 ft=make:
