# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **
#
# When       Who       What
# 1999-08-01 Anon      Original VisVim.dsp
# 2001-08-08 W.Briscoe Back-ported to a condensed VC4 Makefile
#		       Reduced inter-dependency of Release and Debug builds.
#

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

!IF "$(CFG)" == ""
CFG=VisVim - Win32 Release
!MESSAGE No configuration specified.  Defaulting to VisVim - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "VisVim - Win32 Release" && "$(CFG)" != "VisVim - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "VisVim.mak" CFG="VisVim - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "VisVim - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "VisVim - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
DEL_TREE = rmdir /s /q
!ELSE 
NULL=nul
DEL_TREE = deltree /y
!ENDIF 
# Begin Project
# PROP Target_Last_Scanned "VisVim - Win32 Release"
# PROP Use_MFC 2
CPP=cl.exe
RSC=rc.exe
LINK32=link.exe

!IF  "$(CFG)" == "VisVim - Win32 Release"

# PROP Use_Debug_Libraries 0
# PROP Output_Dir ".\Release"
# PROP Intermediate_Dir ".\Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release
CPP_OBJS=.\Release/

# ADD CPP /MD /O2 /D "NDEBUG" /I.\Release
CPP_PROJ= /MD /O2 /D "NDEBUG" /I.\Release
# ADD RSC /d "NDEBUG
RSC_PROJ= /d "NDEBUG"
# ADD LINK32 /pdb:none
LINK32_FLAGS=/pdb:none

!ELSEIF  "$(CFG)" == "VisVim - Win32 Debug"

# PROP Use_Debug_Libraries 1
# PROP Output_Dir ".\Debug"
# PROP Intermediate_Dir ".\Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug
CPP_OBJS=.\Debug/

# ADD CPP /MDd /Gm /Zi /Od /D "_DEBUG" /I.\Debug
CPP_PROJ= /MDd /Gm /Zi /Od /D "_DEBUG" /I.\Debug /Fd"$(INTDIR)/"
MTL_PROJ= /D "_DEBUG"
# ADD RSC /d "_DEBUG
RSC_PROJ= /d "_DEBUG"
# ADD LINK32 /debug /pdbtype:sept /pdb:".\Debug/VisVim.pdb"
LINK32_FLAGS=/debug /pdbtype:sept /pdb:"$(OUTDIR)/VisVim.pdb"

!ENDIF 

# ADD CPP            /nologo /W3 /GX /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /c
CPP_PROJ=$(CPP_PROJ) /nologo /W3 /GX /D "WIN32" /D "_WINDOWS" /D "_WINDLL" /D "_AFXDLL" /D "_USRDLL" /c /Fo"$(INTDIR)/"
# ADD RSC            /l 0x409 /d "_AFXDLL"
RSC_PROJ=$(RSC_PROJ) /l 0x409 /d "_AFXDLL" /fo"$(INTDIR)/VisVim.res"
# ADD LINK32                 /nologo /subsystem:windows /dll /machine:I386 /incremental:no
LINK32_FLAGS=$(LINK32_FLAGS) /nologo /subsystem:windows /dll /machine:I386\
 /incremental:no /def:".\VisVim.def"\
 /out:"$(OUTDIR)/VisVim.dll" /implib:"$(OUTDIR)/VisVim.lib"

ALL : "$(OUTDIR)\VisVim.dll"

CLEAN : 
	-@if exist "$(INTDIR)/$(NULL)" $(DEL_TREE) "$(INTDIR)"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

LINK32_OBJS= \
	"$(INTDIR)/VisVim.res" \
	"$(INTDIR)/VisVim.obj" \
	"$(INTDIR)/StdAfx.obj" \
	"$(INTDIR)/Reg.obj" \
	"$(INTDIR)/DSAddIn.obj" \
	"$(INTDIR)/OleAut.obj" \
	"$(INTDIR)/Commands.obj"

"$(OUTDIR)\VisVim.dll" : "$(OUTDIR)" ".\VisVim.def" $(LINK32_OBJS)
    $(LINK32) $(LINK32_FLAGS) $(LINK32_OBJS)

{.}.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

{.}.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

# Begin Target

# Name "VisVim - Win32 Release"
# Name "VisVim - Win32 Debug"

# Begin Source File

SOURCE=.\VisVim.cpp

"$(INTDIR)\VisVim.obj" : $(SOURCE) "$(INTDIR)"

# End Source File
# Begin Source File

SOURCE=.\VisVim.def
# End Source File
# Begin Source File

SOURCE=.\VisVim.odl

!IF  "$(CFG)" == "VisVim - Win32 Release"

# PROP Ignore_Default_Tool 1
# Begin Custom Build

"$(INTDIR)\VisVim.tlb" : $(SOURCE) "$(INTDIR)"
	midl /nologo /mktyplib203 /win32 /tlb VisVim.tlb /h VSVTypes.h .\VisVim.odl /out .\Release /D "NDEBUG"

# End Custom Build

!ELSEIF  "$(CFG)" == "VisVim - Win32 Debug"

# PROP Ignore_Default_Tool 1
# Begin Custom Build

"$(INTDIR)\VisVim.tlb" : $(SOURCE) "$(INTDIR)"
	midl /nologo /mktyplib203 /win32 /tlb VisVim.tlb /h VSVTypes.h .\VisVim.odl /out .\Debug /D "_DEBUG"

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\StdAfx.cpp

"$(INTDIR)\StdAfx.obj" : $(SOURCE) "$(INTDIR)"

# End Source File
# Begin Source File

SOURCE=.\VisVim.rc

"$(INTDIR)\VisVim.res" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\VisVim.tlb"
 $(RSC) $(RSC_PROJ) /i "$(INTDIR)" $(SOURCE)

# End Source File
# Begin Source File

SOURCE=.\Reg.cpp

"$(INTDIR)\Reg.obj" : $(SOURCE) "$(INTDIR)"

# End Source File
# Begin Source File

SOURCE=.\DSAddIn.cpp

"$(INTDIR)\DSAddIn.obj" : $(SOURCE) "$(INTDIR)"

# End Source File
# Begin Source File

SOURCE=.\OleAut.cpp

"$(INTDIR)\OleAut.obj" : $(SOURCE) "$(INTDIR)"

# End Source File
# Begin Source File

SOURCE=.\Commands.cpp

"$(INTDIR)\Commands.obj" : $(SOURCE) "$(INTDIR)"

# End Source File
# End Target
# End Project
