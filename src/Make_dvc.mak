# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=Vim - Win32 IDE for Make_mvc.mak
!MESSAGE No configuration specified.  Defaulting to Vim - Win32 IDE for\
 Make_mvc.mak.
!ENDIF

!IF "$(CFG)" != "Vim - Win32 IDE for Make_mvc.mak"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE
!MESSAGE NMAKE /f "Make_dvc.mak" CFG="Vim - Win32 IDE for Make_mvc.mak"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "Vim - Win32 IDE for Make_mvc.mak" (based on\
 "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE
NULL=nul
!ENDIF
################################################################################
# Begin Project
# PROP Target_Last_Scanned "Vim - Win32 IDE for Make_mvc.mak"
CPP=cl.exe
RSC=rc.exe
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir ""
# PROP Intermediate_Dir ""
# PROP Target_Dir ""
OUTDIR=.
INTDIR=.

ALL : "$(OUTDIR)\vimrun.exe"

CLEAN :
	-@erase ".\vimrun.exe"
	-@erase ".\vimrun.obj"

# ADD CPP /nologo /c
# ADD BASE RSC /l 0x809
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Make_dvc.bsc"
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 /machine:IX86
# ADD LINK32 /nologo /pdb:none /machine:IX86 /out:"vimrun.exe"
LINK32_FLAGS=/nologo /pdb:none /machine:IX86 /out:"$(OUTDIR)/vimrun.exe"
LINK32_OBJS= \
	"$(INTDIR)/vimrun.obj"

"$(OUTDIR)\vimrun.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

CPP_PROJ=/nologo /ML /c

.c.obj:
   $(CPP) $(CPP_PROJ) $<

.cpp.obj:
   $(CPP) $(CPP_PROJ) $<

.cxx.obj:
   $(CPP) $(CPP_PROJ) $<

.c.sbr:
   $(CPP) $(CPP_PROJ) $<

.cpp.sbr:
   $(CPP) $(CPP_PROJ) $<

.cxx.sbr:
   $(CPP) $(CPP_PROJ) $<

################################################################################
# Begin Target

# Name "Vim - Win32 IDE for Make_mvc.mak"
################################################################################
# Begin Source File

SOURCE=.\vimrun.c

"$(INTDIR)\vimrun.obj" : $(SOURCE) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
