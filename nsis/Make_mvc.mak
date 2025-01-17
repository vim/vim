#
# Makefile for MS Windows for create self-installing exe of Vim.
# 05/04/2024, Restorer restorer@mail2k.ru
#


#!INCLUDE .\Make_all.mak

.SUFFIXES :

!IFDEF PROGRAMW6432
ProgFiles=%%PROGRAMFILES(x86)%%
!ELSE
ProgFiles=$(PROGRAMFILES)
!ENDIF

!IFDEF VIMSRC
MKNSISFLAGS = /D"VIMSRC=$(VIMSRC)"
!ENDIF

!IFDEF VIMRT
MKNSISFLAGS = $(MKNSISFLAGS) /D"VIMRT=$(VIMRT)"
!ENDIF

!IFDEF VIMTOOLS
MKNSISFLAGS = $(MKNSISFLAGS) /D"VIMTOOLS=$(VIMTOOLS)"
!ENDIF

!IFDEF GETTEXT
MKNSISFLAGS = $(MKNSISFLAGS) /D"GETTEXT=$(GETTEXT)"
!ENDIF

!IFDEF HAVE_UPX
MKNSISFLAGS = $(MKNSISFLAGS) /DHAVE_UPX=$(HAVE_UPX)
!ENDIF

!IFDEF HAVE_NLS
MKNSISFLAGS = $(MKNSISFLAGS) /DHAVE_NLS=$(HAVE_NLS)
!ENDIF

!IFDEF HAVE_MULTI_LANG
MKNSISFLAGS = $(MKNSISFLAGS) /DHAVE_MULTI_LANG=$(HAVE_MULTI_LANG)
!ENDIF

!IFDEF WIN64
MKNSISFLAGS = $(MKNSISFLAGS) /DWIN64=$(WIN64)
!ENDIF

!IFDEF INCLUDE_LIBGCC
MKNSISFLAGS = $(MKNSISFLAGS) /DINCLUDE_LIBGCC=$(INCLUDE_LIBGCC)
!ENDIF

!IFDEF X
XX = /X"$(X:;=" /X")"
!ENDIF

# If necessary, correct the full path of the NSIS compiler in the next line.
# Please do not enclose the path in quotation marks.
MKNSIS = $(ProgFiles)\NSIS

PS = powershell.exe
RM = del /f /q
RD = rmdir /s /q

MKNSISFLAGS = /INPUTCHARSET UTF8 $(MKNSISFLAGS)
PSFLAGS = -NoLogo -NoProfile -Command

# Read MAJOR and MINOR from version.h.
!IF ![for /F "tokens=2,3" %G in ( \
	'findstr /rc:"VIM_VERSION_MINOR[	^]*[0-9^]" \
	/rc:"VIM_VERSION_MAJOR[	^]*[0-9^]" ..\src\version.h') do \
	@if "VIM_VERSION_MAJOR"=="%G" (echo MAJOR=%H>>_ver.tmp) \
	else echo MINOR=%H>>_ver.tmp]
! INCLUDE .\_ver.tmp
! IF [$(RM) .\_ver.tmp]
! ENDIF
!ENDIF

# Read PATCHLEVEL from version.c
!IF ![for /F %G in ( \
	'findstr /nblc:"static int included_patches[^]" ..\src\version.c \
	^| (set /p "_t=" ^& set /a _t+=2 ^)') do \
	@cmd /q /c "for /F "skip=%G delims=, " %H in (..\src\version.c) do \
			(echo PATCH=%H>_patchlvl.tmp & exit /b)"]
! INCLUDE .\_patchlvl.tmp
! IF [$(RM) .\_patchlvl.tmp]
! ENDIF
!ENDIF
!IF $(PATCH) < 10
PATCH = 000$(PATCH)
!ELSEIF $(PATCH) < 100
PATCH = 00$(PATCH)
!ELSEIF $(PATCH) < 1000
PATCH = 0$(PATCH)
!ENDIF


all : makeinst

makeinst : prepare
	^"$(MKNSIS)\makensis.exe" $(MKNSISFLAGS) gvim.nsi $(XX)

prepare : unzipicons gvim_version.nsh license rename

unzipicons : icons.zip
	@ if exist %|fF\nul $(RD) %|fF
	@ $(PS) $(PSFLAGS) \
		Add-Type -AssemblyName 'System.IO.Compression.FileSystem'; \
		[System.IO.Compression.ZipFile]::ExtractToDirectory(\"$**\", \".\")

gvim_version.nsh : Make_mvc.mak
	@ 1> $@ echo:^# Generated from Makefile: define the version numbers
	@ 1>> $@ echo:^!ifndef __GVIM_VER__NSH__
	@ 1>> $@ echo:^!define __GVIM_VER__NSH__
	@ 1>> $@ echo:^!define VER_MAJOR $(MAJOR)
	@ 1>> $@ echo:^!define VER_MINOR $(MINOR)
	@ 1>> $@ echo:^!define PATCHLEVEL $(PATCH)
	@ 1>> $@ echo:^!endif

license : ..\lang\LICENSE.*.txt ..\LICENSE
	!@ $(PS) $(PSFLAGS) \
		Get-Content -Path '$**' -Encoding UTF8 ^| \
		Set-Content -Path '..\lang\$(**B).nsis.txt' -Enc Unicode -Force

rename :
	@ ..\tools\rename.bat "$(SRC)" "$(DST)" 1> nul

clean :
	@ if exist .\gvim_version.nsh $(RM) .\gvim_version.nsh
	@ if exist ..\lang\LICENSE*.nsis.txt $(RM) ..\lang\LICENSE*.nsis.txt
	@ if exist .\icons\nul $(RD) .\icons
	@ if exist .\gvim??.exe $(RM) .\gvim??.exe

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=0 ft=make:
