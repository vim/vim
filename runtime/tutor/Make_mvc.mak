#
# Makefile for converting the Vim tutorial on Windows.
#
# 21.11.24, Restorer, restorer@mail2k.ru
#
# Use the UTF-8 version as the original and create the others with conversion.
# For some translation files of chapter one, conversion from traditional
# encodings to UTF-8 encoding is performed.


!IF [powershell -nologo -noprofile "exit $$psversiontable.psversion.major"] == 2
!ERROR The program "PowerShell" version 3.0 or higher is required to work
!ENDIF

# Common components
!INCLUDE Make_all.mak

# Correct the following line for the directory where iconv is installed.
# Please do not put the path in quotes.
ICONV_PATH = D:\Programs\GetText\bin

# In case some package like GnuWin32, UnixUtils, gettext
# or something similar is installed on the system.
# If the "iconv" program is installed on the system, but it is not registered
# in the %PATH% environment variable, then specify the full path to this file.
!IF EXIST ("iconv.exe")
ICONV = "iconv.exe"
!ELSEIF EXIST ("$(ICONV_PATH)\iconv.exe")
ICONV = "$(ICONV_PATH)\iconv.exe"
!ENDIF

RM = del /q
CP = copy /y
HDLNK = mklink /h
PS = PowerShell.exe

PSFLAGS = -NoLogo -NoProfile -Command

.SUFFIXES :

all : $(CONVERTED)

tutor1.utf-8 : tutor1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

tutor2 : tutor2.utf-8
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-1 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28591))
!ENDIF

tutor1.bar tutor2.bar :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-1 $@.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$@.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28591))
!ENDIF

tutor1.ca.utf-8 : tutor1.ca
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

tutor2.ca : tutor2.ca.utf-8
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-1 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28591))
!ENDIF

tutor1.cs tutor2.cs :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-2 $@.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$@.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28592))
!ENDIF

tutor1.cs.cp1250 tutor2.cs.cp1250 :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t CP1250 $(@R).utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$(@R).utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(1250))
!ENDIF

tutor1.da tutor2.da :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-4 $@.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$@.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28594))
!ENDIF

tutor1.de.utf-8 : tutor1.de
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

tutor2.de : tutor2.de.utf-8
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-1 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28591))
!ENDIF

tutor1.el tutor2.el :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-7 $@.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$@.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28597))
!ENDIF

tutor1.el.cp737 tutor2.el.cp737 :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t CP737 $(@R).utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$(@R).utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(737))
!ENDIF

tutor1.eo tutor2.eo :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-3 $@.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$@.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28593))
!ENDIF

tutor1.es tutor2.es :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-1 $@.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$@.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28591))
!ENDIF

tutor1.fr.utf-8 : tutor1.fr
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

tutor2.fr : tutor2.fr.utf-8
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-1 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28591))
!ENDIF

tutor1.hr tutor2.hr :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-2 $@.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$@.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28592))
!ENDIF

tutor1.hr.cp1250 tutor2.hr.cp1250 :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t CP1250 $(@R).utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$(@R).utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(1250))
!ENDIF

tutor1.hu tutor2.hu :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-2 $@.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$@.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28592))
!ENDIF

tutor1.hu.cp1250 tutor2.hu.cp1250 :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t CP1250 $(@R).utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$(@R).utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(1250))
!ENDIF

tutor1.it.utf-8 : tutor1.it
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

tutor2.it : tutor2.it.utf-8
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-1 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28591))
!ENDIF

tutor1.ja.sjis tutor2.ja.sjis :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t CP932 $(@R).utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$(@R).utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(932))
!ENDIF

tutor1.ja.euc tutor2.ja.euc :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t EUC-JP $(@R).utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$(@R).utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(51932))
!ENDIF

tutor1.ko tutor2.ko :
	$(HDLNK) $@ $@.utf-8

tutor1.ko.euc tutor2.ko.euc :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t EUC-KR $(@R).utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$(@R).utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(51949))
!ENDIF

tutor1.nl tutor2.nl :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-1 $@.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$@.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28591))
!ENDIF

tutor1.no.utf-8 : tutor1.no
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

tutor2.no : tutor2.no.utf-8
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-1 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28591))
!ENDIF

# nb is an alias for no
tutor1.nb tutor2.nb : $$(@R).no
	$(HDLNK) $@ $?

tutor1.nb.utf-8 tutor2.nb.utf-8 : $$(@R)
	$(HDLNK) $@ %|dpfF.no.utf-8

tutor1.pl tutor2.pl :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-2 $@.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$@.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28592))
!ENDIF

tutor1.pl.cp1250 tutor2.pl.cp1250 :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t CP1250 $(@R).utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$(@R).utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(1252))
!ENDIF

tutor1.pt tutor2.pt :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-15 $@.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$@.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28605))
!ENDIF

tutor1.ru tutor2.ru :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t KOI8-R $@.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$@.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(20866))
!ENDIF

tutor1.ru.cp1251 tutor2.ru.cp1251 :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t CP1251 $(@R).utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$(@R).utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(1251))
!ENDIF

tutor1.sk tutor2.sk :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-2 $@.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$@.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28592))
!ENDIF

tutor1.sk.cp1250 tutor2.sk.cp1250 :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t CP1250 $(@R).utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$(@R).utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(1252))
!ENDIF

tutor1.sr.cp1250 tutor2.sr.cp1250 :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t CP1250 $(@R).utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$(@R).utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(1252))
!ENDIF

tutor1.sv.utf-8 : tutor1.sv
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-15 -t UTF-8 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(28605)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

tutor2.sv : tutor2.sv.utf-8
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-15 $? >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28605))
!ENDIF

tutor1.tr.iso9 tutor2.tr.iso9 :
!IF DEFINED (ICONV)
	$(ICONV) -f UTF-8 -t ISO-8859-9 $*.utf-8 >$@
!ELSE
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$*.utf-8\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(28599))
!ENDIF

tutor1.zh.utf-8 : tutor1.zh.big5
	$(PS) $(PSFLAGS) [System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(950)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force

tutor2.zh.big5 : tutor2.zh.utf-8
	$(PS) $(PSFLAGS) [System.IO.File]::WriteAllText(\"$@\", \
		[System.IO.File]::ReadAllText(\"$?\", \
		[System.Text.Encoding]::GetEncoding(65001)), \
		[System.Text.Encoding]::GetEncoding(950))

clean :
	@for %%G in ($(CONVERTED)) do (if exist .\%%G $(RM) .\%%G)

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=0 ft=make:
