#
# Makefile for the Vim documentation on Windows
#
# 17.11.23, Restorer, <restorer@mail2k.ru>

# Common components
!INCLUDE makefile_all.mak


# НАДО: подумать, что использовать вместо awk. PowerShell?
#AWK =

#
VIMEXE = D:\Programs\Vim\vim90\vim.exe



#
GETTEXT_PATH = D:\Programs\GetText\bin



# На случай, если в системе установлен какой‐нибудь пакет типа GNUWin32,
# UNIXUtils или что‐то подобное.
# Если в системе установлена программа "touch", но она не прописана в переменной
# окружения %PATH%, то укажите здесь полный маршрут к этому файлу.
!IF EXIST ("touch.exe")
TOUCH = touch.exe $@ 
!ELSE
TOUCH = @if exist $@ ( copy /b $@+,, ) else ( type nul >$@ )
!ENDIF

# На случай, если в системе установлен какой‐нибудь пакет типа GNUWin32,
# UNIXUtils, gettext или что‐то подобное.
# Если в системе установлена программа "iconv", но она не прописана в
# переменной окружения %PATH%, то укажите здесь полный маршрут к этому файлу.
!IF EXIST ("iconv.exe")
ICONV = iconv.exe
!ELSEIF EXIST ("$(GETTEXT_PATH)\iconv.exe")
ICONV="$(GETTEXT_PATH)\iconv.exe"
!ENDIF

RM = del /q

.SUFFIXES :
.SUFFIXES : .c .o .txt .html


all : tags perlhtml $(CONVERTED)

# Use "doctags" to generate the tags file.  Only works for English!
tags : doctags $(DOCS)
	doctags $(DOCS) | sort /L C /O tags
	powershell -nologo -noprofile -Command\
	"(Get-Content -Raw tags | Get-Unique | % {$$_ -replace \"`r\", \"\"}) |\
	New-Item -Force -Path . -ItemType file -Name tags"

doctags : doctags.c
	$(CC) doctags.c


# Use Vim to generate the tags file.  Can only be used when Vim has been
# compiled and installed.  Supports multiple languages.
vimtags : $(DOCS)
	$(VIMEXE) --clean -esX -V1 -u doctags.vim



uganda.nsis.txt : uganda.*
	!powershell -nologo -noprofile -Command\
	$$ext=(Get-Item $?).Extension; (Get-Content $? ^| \
	% {$$_ -replace '\s*\*[-a-zA-Z0-9.]*\*', '' -replace 'vim:tw=78:.*', ''})\
	^| Set-Content $*$$ext
	!powershell -nologo -noprofile -Command\
	$$ext=(Get-Item $?).Extension;\
	(Get-Content -Raw $(@B)$$ext).Trim() -replace '(\r\n){3,}', '$$1$$1'\
	 ^| Set-Content $(@B)$$ext


# НАДО: подумать, что можно использовать вместо awk. PowerShell?
#html: noerrors tags $(HTMLS)
#	if exist errors.log (more errors.log)

# НАДО:
#noerrors:
#	$(RM) errors.log

# НАДО: подумать, что можно использовать вместо awk. PowerShell?
#.txt.html:


# НАДО: подумать, что можно использовать вместо awk. PowerShell?
#index.html: help.txt


# НАДО: подумать, что можно использовать вместо awk. PowerShell?
#vimindex.html: index.txt


# НАДО: подумать, что можно использовать вместо awk. PowerShell?
#tags.ref tags.html: tags

# Perl version of .txt to .html conversion.
# There can't be two rules to produce a .html from a .txt file.
# Just run over all .txt files each time one changes.  It's fast anyway.
perlhtml : tags $(DOCS)
	vim2html.pl tags $(DOCS)

# Check URLs in the help with "curl" or "powershell".
test_urls :
	$(VIMEXE) -S test_urls.vim

clean :
	$(RM) doctags.exe doctags.obj
	$(RM) *.html



arabic.txt :
	$(TOUCH)

farsi.txt :
	$(TOUCH)

hebrew.txt :
	$(TOUCH)

russian.txt :
	$(TOUCH)

gui_w32.txt :
	$(TOUCH)

if_ole.txt :
	$(TOUCH)

os_390.txt :
	$(TOUCH)

os_amiga.txt :
	$(TOUCH)

os_beos.txt :
	$(TOUCH)

os_dos.txt :
	$(TOUCH)

os_haiku.txt :
	$(TOUCH)

os_mac.txt :
	$(TOUCH)

os_mint.txt :
	$(TOUCH)

os_msdos.txt :
	$(TOUCH)

os_os2.txt :
	$(TOUCH)

os_qnx.txt :
	$(TOUCH)

os_risc.txt :
	$(TOUCH)

os_win32.txt :
	$(TOUCH)


convert-all : $(CONVERTED)
!IF [powershell -nologo -noprofile "exit $$psversiontable.psversion.major"] == 2
!ERROR Для работы требуется программа «PowerShell» версии 3.0 или выше
!ENDIF

vim-da.UTF-8.1 : vim-da.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vimdiff-da.UTF-8.1 : vimdiff-da.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vimtutor-da.UTF-8.1 : vimtutor-da.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vim-de.UTF-8.1 : vim-de.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

evim-fr.UTF-8.1 : evim-fr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vim-fr.UTF-8.1 : vim-fr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vimdiff-fr.UTF-8.1 : vimdiff-fr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vimtutor-fr.UTF-8.1 : vimtutor-fr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t utf-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

xxd-fr.UTF-8.1 : xxd-fr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

evim-it.UTF-8.1 : evim-it.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vim-it.UTF-8.1 : vim-it.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vimdiff-it.UTF-8.1 : vimdiff-it.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vimtutor-it.UTF-8.1 : vimtutor-it.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

xxd-it.UTF-8.1 : xxd-it.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-1 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28591)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

evim-pl.UTF-8.1 : evim-pl.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28592)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vim-pl.UTF-8.1 : vim-pl.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28592)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vimdiff-pl.UTF-8.1 : vimdiff-pl.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28592)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vimtutor-pl.UTF-8.1 : vimtutor-pl.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28592)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

xxd-pl.UTF-8.1 : xxd-pl.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-2 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28592)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

evim-ru.UTF-8.1 : evim-ru.1
!IF DEFINED (ICONV)
	$(ICONV) -f KOI8-R -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(20866)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vim-ru.UTF-8.1 : vim-ru.1
!IF DEFINED (ICONV)
	$(ICONV) -f KOI8-R -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(20866)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vimdiff-ru.UTF-8.1 : vimdiff-ru.1
!IF DEFINED (ICONV)
	$(ICONV) -f KOI8-R -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(20866)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vimtutor-ru.UTF-8.1 : vimtutor-ru.1
!IF DEFINED (ICONV)
	$(ICONV) -f KOI8-R -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(20866)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

xxd-ru.UTF-8.1 : xxd-ru.1
!IF DEFINED (ICONV)
	$(ICONV) -f KOI8-R -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(20866)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

evim-tr.UTF-8.1 : evim-tr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-9 -t UTF-8 $? >$@
!ELSE
!    IF [powershell -nologo -noprofile "exit $$psversiontable.psversion.major"] == 2
!    ERROR Для работы требуется программа «PowerShell» версии 3.0 или выше
!    ENDIF
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28599)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vim-tr.UTF-8.1 : vim-tr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-9 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28599)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vimdiff-tr.UTF-8.1 : vimdiff-tr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-9 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28599)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF

vimtutor-tr.UTF-8.1 : vimtutor-tr.1
!IF DEFINED (ICONV)
	$(ICONV) -f ISO-8859-9 -t UTF-8 $? >$@
!ELSE
# Преобразование в кодировку UTF-8 без МПБ и с UNIX-подобным окончанием строки
	powershell -nologo -noprofile -Command\
	[IO.File]::ReadAllText(\"$?\", [Text.Encoding]::GetEncoding(28599)) ^|\
	1>nul New-Item -Force -Path . -ItemType file -Name $@
!ENDIF



# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=0 ft=make:
