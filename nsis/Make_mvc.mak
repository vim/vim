#
# Makefile for MS Windows for create self-installing exe of Vim.
# 2024‐04-05, Restorer, restorer@mail2k.ru
#


# included common tools
!INCLUDE ..\src\auto\nmake\tools.mak

# Read MAJOR and MINOR from version.h.
!IFNDEF MAJOR
! IF ![for /F "tokens=3" %G in \
	('findstr /RC:"VIM_VERSION_MAJOR[	^]*[0-9^]" ..\src\version.h') \
	do @(echo:MAJOR=%G>> .\_major.tmp)]
!  INCLUDE .\_major.tmp
!  IF [$(RM) .\_major.tmp]
!  ENDIF
! ELSE
MAJOR = 9
! ENDIF
!ENDIF

!IFNDEF MINOR
! IF ![for /F "tokens=3" %G in \
	('findstr /RC:"VIM_VERSION_MINOR[	^]*[0-9^]" ..\src\version.h') \
	do @(echo:MINOR=%G>> .\_minor.tmp)]
!  INCLUDE .\_minor.tmp
!  IF [$(RM) .\_minor.tmp]
!  ENDIF
! ELSE
MINOR = 1
! ENDIF
!ENDIF

# Read PATCHLEVEL from version.c
!IFNDEF PATCHLEVEL
! IF ![for /F %G in \
	('findstr /NBLC:"static int included_patches" ..\src\version.c \
	^| (set /p "_t=" ^& set /a _t+=2 ^)') do \
	@$(CMD) $(CMDFLAGS) "for /F "skip=%G delims=, " %H in \
	(..\src\version.c) do (echo:PATCHLEVEL=%H> .\_patchlvl.tmp & exit /b)"]
!  INCLUDE .\_patchlvl.tmp
!  IF [$(RM) .\_patchlvl.tmp]
!  ENDIF
! ELSE
PATCHLEVEL = 0
! ENDIF
!ENDIF

!IF $(PATCHLEVEL) < 10
PATCHLEVEL = 000$(PATCHLEVEL)
!ELSEIF $(PATCHLEVEL) < 100
PATCHLEVEL = 00$(PATCHLEVEL)
!ELSEIF $(PATCHLEVEL) < 1000
PATCHLEVEL = 0$(PATCHLEVEL)
!ENDIF

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

!IFDEF ARM64
MKNSISFLAGS = $(MKNSISFLAGS) /DARM64=$(ARM64)
!ENDIF

!IFDEF INCLUDE_LIBGCC
MKNSISFLAGS = $(MKNSISFLAGS) /DINCLUDE_LIBGCC=$(INCLUDE_LIBGCC)
!ENDIF

!IFDEF X
XX = /X"$(X:;=" /X")"
!ENDIF

# If necessary, correct the full path of the NSIS compiler in the next line.
# Please do not enclose the path in quotation marks.
MKNSIS = $(ProgFiles)\NSIS\makensis.exe

MKNSISFLAGS = /INPUTCHARSET UTF8 $(MKNSISFLAGS)


all : makeinst

makeinst : prepare
	^"$(MKNSIS)" $(MKNSISFLAGS) gvim.nsi $(XX)

prepare : unzipicons gvim_version.nsh license rename

unzipicons : icons.zip
	@ if exist %|fF\nul $(RD) %|fF
	@ $(PS) $(PSFLAGS) \
		Add-Type -AssemblyName 'System.IO.Compression.FileSystem'; \
		[System.IO.Compression.ZipFile]::ExtractToDirectory(\"$**\", \
		\".\")

gvim_version.nsh : Make_mvc.mak
	@ 1> $@ echo:^# Generated from Makefile: define the version numbers
	@ 1>> $@ echo:^!ifndef __GVIM_VER__NSH__
	@ 1>> $@ echo:^!define __GVIM_VER__NSH__
	@ 1>> $@ echo:^!define VER_MAJOR $(MAJOR)
	@ 1>> $@ echo:^!define VER_MINOR $(MINOR)
	@ 1>> $@ echo:^!define PATCHLEVEL $(PATCHLEVEL)
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

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=79 ft=make:
