#
# Makefile for the Vim documentation on Windows
#
# 2024-03-20, Restorer, <restorer@mail2k.ru>
#

# included common tools
!INCLUDE ..\..\src\auto\nmake\tools.mak

# Common components
!INCLUDE .\Make_all.mak


# TODO: to think about what to use instead of awk. PowerShell?
#AWK =

# Correct the following line for the where executable file Vim is installed.
# Please do not put the path in quotes.
VIMPROG = ..\..\src\vim.exe

# Correct the following line for the directory where iconv installed.
# Please do not put the path in quotes.
ICONV_PATH = D:\Programs\GetText\bin

# In case some package like GnuWin32, UnixUtils, gettext
# or something similar is installed on the system.
# If the "iconv" program is installed on the system, but it is not registered
# in the %PATH% environment variable, then specify the full path to this file.
!IF EXIST ("iconv.exe")
ICONV = iconv.exe
!ELSEIF EXIST ("$(ICONV_PATH)\iconv.exe")
ICONV = "$(ICONV_PATH)\iconv.exe"
!ENDIF

.SUFFIXES :
.SUFFIXES : .c .o .txt .html


all : tags perlhtml $(CONVERTED)

# Use "doctags" to generate the tags file.  Only works for English!
tags : doctags $(DOCS)
	doctags.exe $(DOCS) | sort /L C /O tags
	$(PS) $(PSFLAGS) \
		(Get-Content -Raw tags ^| Get-Unique ^| %%{$$_ \
		-replace \"`r\", \"\"}) \
		^| New-Item -Path . -Name tags -ItemType file -Force

doctags : doctags.c
	$(CC) doctags.c


# Use Vim to generate the tags file.  Can only be used when Vim has been
# compiled and installed.  Supports multiple languages.
vimtags : $(DOCS)
	@ "$(VIMPROG)" --clean -esX -V1 -u doctags.vim

# TODO:
#html: noerrors tags $(HTMLS)
#	if exist errors.log (more errors.log)

# TODO:
#noerrors:
#	$(RM) errors.log

# TODO:
#.txt.html:


# TODO:
#index.html: help.txt


# TODO:
#vimindex.html: index.txt


# TODO:
#tags.ref tags.html: tags

# Perl version of .txt to .html conversion.
# There can't be two rules to produce a .html from a .txt file.
# Just run over all .txt files each time one changes.  It's fast anyway.
perlhtml : tags $(DOCS)
	vim2html.pl tags $(DOCS)

# Check URLs in the help with "curl" or "powershell".
test_urls :
	"$(VIMPROG)" --clean -S test_urls.vim

clean :
	- $(RM) doctags.exe doctags.obj
	- $(RM) *.html vim-stylesheet.css


arabic.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

farsi.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

hebrew.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

russian.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

gui_w32.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

if_ole.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

os_390.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

os_amiga.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

os_beos.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

os_dos.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

os_haiku.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

os_mac.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

os_mint.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

os_msdos.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

os_os2.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

os_qnx.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

os_risc.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

os_win32.txt :
	@ <<touch.bat $@
@$(TOUCH)
<<

convert-all : $(CONVERTED)

vim-da.UTF-8.1 : vim-da.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vimdiff-da.UTF-8.1 : vimdiff-da.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vimtutor-da.UTF-8.1 : vimtutor-da.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vim-de.UTF-8.1 : vim-de.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

evim-fr.UTF-8.1 : evim-fr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vim-fr.UTF-8.1 : vim-fr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vimdiff-fr.UTF-8.1 : vimdiff-fr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vimtutor-fr.UTF-8.1 : vimtutor-fr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t utf-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

xxd-fr.UTF-8.1 : xxd-fr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

evim-it.UTF-8.1 : evim-it.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vim-it.UTF-8.1 : vim-it.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vimdiff-it.UTF-8.1 : vimdiff-it.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vimtutor-it.UTF-8.1 : vimtutor-it.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

xxd-it.UTF-8.1 : xxd-it.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28591)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

evim-pl.UTF-8.1 : evim-pl.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28592)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vim-pl.UTF-8.1 : vim-pl.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28592)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vimdiff-pl.UTF-8.1 : vimdiff-pl.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28592)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vimtutor-pl.UTF-8.1 : vimtutor-pl.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28592)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

xxd-pl.UTF-8.1 : xxd-pl.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28592)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

evim-ru.UTF-8.1 : evim-ru.1
!IF DEFINED (ICONV)
	$(ICONV) -f KOI8-R -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(20866)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vim-ru.UTF-8.1 : vim-ru.1
!IF DEFINED (ICONV)
	$(ICONV) -f KOI8-R -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(20866)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vimdiff-ru.UTF-8.1 : vimdiff-ru.1
!IF DEFINED (ICONV)
	$(ICONV) -f KOI8-R -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(20866)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vimtutor-ru.UTF-8.1 : vimtutor-ru.1
!IF DEFINED (ICONV)
	$(ICONV) -f KOI8-R -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(20866)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

xxd-ru.UTF-8.1 : xxd-ru.1
!IF DEFINED (ICONV)
	$(ICONV) -f KOI8-R -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(20866)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

evim-tr.UTF-8.1 : evim-tr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-9 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28599)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vim-tr.UTF-8.1 : vim-tr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-9 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28599)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vimdiff-tr.UTF-8.1 : vimdiff-tr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-9 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28599)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

vimtutor-tr.UTF-8.1 : vimtutor-tr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-9 -t UTF-8 $? >$@
!ELSE
# Conversion to UTF-8 encoding without BOM and with UNIX-like line ending
	$(PS) $(PSFLAGS) \
		[IO.File]::ReadAllText(\"$?\", \
		[Text.Encoding]::GetEncoding(28599)) ^| \
		1>nul New-Item -Path . -Name $@ -ItemType file -Force
!ENDIF

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=79 ft=make:
