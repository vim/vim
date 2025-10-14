#
# Makefile for setting up common tools used in Make_mvc.mak
#
# 2023-11-28, Restorer, <restorer@mail2k.ru>
#

CP = copy /Y
LS = dir
MKD = mkdir
MV = move /Y
DELTREE = rmdir /Q /S
RD = rmdir /Q /S
RM = del /F /Q
MKHLNK = mklink /H
MKDLNK = mklink /D
MKJLNK = mklink /J
MKSLNK = mklink

PS = PowerShell.exe
PSFLAGS = -NoLogo -NoProfile -Command

!IF [$(PS) $(PSFLAGS) "exit $$psversiontable.psversion.major"] == 2
!ERROR The PowerShell program version 3.0 or higher is required for work.
!ENDIF

!IF ![echo $(COMSPEC) | 1> nul find "cmd.exe"]
CMD = $(COMSPEC)
!ELSE
CMD = $(SYSTEMROOT)\System32\cmd.exe
!ENDIF
CMDFLAGS = /Q /C
CMDFLAGSEX = /V:ON /E:ON $(CMDFLAGS)

# or something similar is installed on the system.
# If the "touch" program is installed on the system, but it is not registered
# in the %PATH% environment variable, then specify the full path to this file.
!IF EXIST (touch.exe)
TOUCH = touch.exe %1
!ELSE
TOUCH = if exist %1 (copy /b %1+,, %1) else (break> %1)
#TOUCH = $(PS) $(PSFLAGS) if (Test-Path $$input -PathType Leaf) \
#	{(Get-ChildItem $$input).LastWriteTime = Get-Date} else \
#	{New-Item $$input -Type file}
!ENDIF

# vim: set noet sw=8 ts=8 sts=0 wm=0 tw=79 ft=make:
