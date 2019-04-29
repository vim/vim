# Microsoft Developer Studio Generated NMAKE File, Format Version 4.00
# ** DO NOT EDIT **
#
# Make_ivc.mak Makefile to build vim in both IDE and nmake.
# This file can be imported as a workspace into Visual Studio.  It must be in
# DOS fileformat then!
#
# It is worth making the file read-only as the VC4 IDE will try to overwrite
# it with a HUGELY expanded clone of itself.
#
# The following points are worth noting:
# 1) Comments here are ignored by VC[456].0 IDEs
# 2) # ADD LINK32 /pdb:.\Dbg/vimd.pdb is written so rather than
#    # ADD LINK32 /pdb:".\Dbg/vimd.pdb" to avoid VC4 -> VC5 conversion failure
# 3) It is good to delete .pdb file before linking to cope with switch among
#    VC[456] as IDE clean action does not remove that file and link clashes
#    with it. The following works in VC5 but not in VC4 which does not support
#    pre-link actions. The nmake action does such deletions.
# Begin Special Build Tool
PreLink_Cmds=@if exist .\oleDbg\gvimd.pdb del .\oleDbg\gvimd.pdb
# End Special Build Tool
# 4) I was unable to make !IFDEF OLE, etc. work in the VC4 IDE.
#    I was aiming for 4 configurations with sub-configurations selected by
#    environment variables.
# 5) Optimisation is not supported by disabled versions of VC. This results in
#    messages for Release builds like:
#      Command line warning D4025 : overriding '/O2' with '/Od'
# 6) nmake 1.62 and later support batch compilation. I was unable to use this
#    in a manner acceptable to earlier IDEs.
#
# History
#
# When       Who       What
# 2001-07-06 W.Briscoe Original derived from Make_[go]vc.mak with less noise
# 2001-07-08 W.Briscoe Further noise reduction; consistent .map and .pdb logic
#		       Added install.exe rule, etc.; Removed unused libraries.
# 2001-08-09 W.Briscoe Restored VC4.0-required trailing space in !MESSAGE afore
#		       Enhanced if_ole.idl rule to use /out argument.
#		       Default rules now relative to . to reduce IDE/nmake difs

# TARGTYPE "Win32 (x86) Console Application" 0x0103

!IF "$(CFG)" == ""
CFG=Vim - Win32 Release gvim OLE
!MESSAGE No configuration specified.  Defaulting to Vim - Win32 Release gvim OLE.
!ENDIF

!IF "$(CFG)" != "Vim - Win32 Release gvim OLE"\
 && "$(CFG)" != "Vim - Win32 Debug gvim OLE"\
 && "$(CFG)" != "Vim - Win32 Release gvim"\
 && "$(CFG)" != "Vim - Win32 Debug gvim"\
 && "$(CFG)" != "Vim - Win32 Release vim"\
 && "$(CFG)" != "Vim - Win32 Debug vim"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE
!MESSAGE NMAKE /f "Make_ivc.mak" CFG="Vim - Win32 Debug vim"
!MESSAGE
!MESSAGE Possible choices for configuration are:
!MESSAGE
!MESSAGE "Vim - Win32 Release gvim OLE" (based on "Win32 (x86) Console Application")
!MESSAGE "Vim - Win32 Debug gvim OLE"   (based on "Win32 (x86) Console Application")
!MESSAGE "Vim - Win32 Release gvim"     (based on "Win32 (x86) Console Application")
!MESSAGE "Vim - Win32 Debug gvim"       (based on "Win32 (x86) Console Application")
!MESSAGE "Vim - Win32 Release vim"      (based on "Win32 (x86) Console Application")
!MESSAGE "Vim - Win32 Debug vim"        (based on "Win32 (x86) Console Application")
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
# PROP Target_Last_Scanned "Vim - Win32 Debug vim"
# PROP Use_MFC 0

RSC=rc.exe
CPP=cl.exe
LINK32=link.exe

CPP_PROJ= /nologo /MT /W3 /GX /I ".\proto" /D "WIN32" /c
# ADD CPP /nologo /MT /W3 /GX /I ".\proto" /D "WIN32" /c

LINK32_FLAGS= oldnames.lib kernel32.lib user32.lib gdi32.lib version.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib netapi32.lib uuid.lib /nologo /machine:I386 /nodefaultlib
# ADD LINK32  oldnames.lib kernel32.lib user32.lib gdi32.lib version.lib comdlg32.lib comctl32.lib advapi32.lib shell32.lib ole32.lib uuid.lib /nologo /machine:I386 /nodefaultlib
# SUBTRACT LINK32 /incremental:yes

RSC_PROJ= /l 0x409 /d "FEAT_GUI_MSWIN"
# ADD RSC /l 0x409 /d "FEAT_GUI_MSWIN"

!IF  "$(CFG)" == "Vim - Win32 Release gvim OLE"

# PROP Use_Debug_Libraries 0
# PROP Output_Dir .\oleRel
# PROP Intermediate_Dir .\oleRel

INTDIR=.\oleRel
VIM=gvim
EXTRAS="$(INTDIR)/if_ole.obj" "$(INTDIR)/vim.res" "$(INTDIR)/gui.obj" "$(INTDIR)/gui_w32.obj" "$(INTDIR)/gui_beval.obj" "$(INTDIR)/os_w32exe.obj"

CPP_PROJ=$(CPP_PROJ) /Zi /O2 /D "NDEBUG" /D "FEAT_GUI_MSWIN" /D "DYNAMIC_GETTEXT" /D "FEAT_OLE" /Fd.\oleRel/ /Fo.\oleRel/
# ADD CPP            /Zi /O2 /D "NDEBUG" /D "FEAT_GUI_MSWIN" /D "DYNAMIC_GETTEXT" /D "FEAT_OLE" /Fd.\oleRel/ /Fo.\oleRel/

RSC_PROJ=$(RSC_PROJ) /I ".\oleRel" /d "NDEBUG" /d "FEAT_OLE" /fo.\oleRel\vim.res
# ADD RSC            /I ".\oleRel" /d "NDEBUG" /d "FEAT_OLE" /fo.\oleRel\vim.res

LINK32_FLAGS=$(LINK32_FLAGS) /pdb:.\oleRel/gvim.pdb -debug:full -debugtype:cv,fixup /map:.\oleDbg\gvim.map libc.lib oleaut32.lib /subsystem:windows /out:.\gvim.exe
# ADD LINK32                 /pdb:.\oleRel/gvim.pdb -debug:full -debugtype:cv,fixup /map:.\oleDbg\gvim.map libc.lib oleaut32.lib /subsystem:windows /out:.\gvim.exe

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug gvim OLE"

# PROP Use_Debug_Libraries 1
# PROP Output_Dir .\oleDbg
# PROP Intermediate_Dir .\oleDbg

INTDIR=.\oleDbg
VIM=gvimd
EXTRAS="$(INTDIR)/if_ole.obj" "$(INTDIR)/vim.res" "$(INTDIR)/gui.obj" "$(INTDIR)/gui_w32.obj" "$(INTDIR)/gui_beval.obj" "$(INTDIR)/os_w32exe.obj"

CPP_PROJ=$(CPP_PROJ) /Zi /Od /D "_DEBUG" /D "FEAT_GUI_MSWIN" /D "DYNAMIC_GETTEXT" /D "FEAT_OLE" /Fd.\oleDbg/ /Fo.\oleDbg/
# ADD CPP            /Zi /Od /D "_DEBUG" /D "FEAT_GUI_MSWIN" /D "DYNAMIC_GETTEXT" /D "FEAT_OLE" /Fd.\oleDbg/ /Fo.\oleDbg/

RSC_PROJ=$(RSC_PROJ) /I .\oleDbg /d "_DEBUG" /d "FEAT_OLE" /fo.\oleDbg\vim.res
# ADD RSC            /I .\oleDbg /d "_DEBUG" /d "FEAT_OLE" /fo.\oleDbg\vim.res

LINK32_FLAGS=$(LINK32_FLAGS) libcd.lib oleaut32.lib /subsystem:windows /debug /profile /pdb:.\oleDbg/gvimd.pdb -debug:full -debugtype:cv,fixup /map:.\oleDbg\gvimd.map /out:.\gvimd.exe
# ADD LINK32                 libcd.lib oleaut32.lib /subsystem:windows /debug /profile /pdb:.\oleDbg/gvimd.pdb -debug:full -debugtype:cv,fixup /map:.\oleDbg\gvimd.map /out:.\gvimd.exe


!ELSEIF  "$(CFG)" == "Vim - Win32 Release gvim"

# PROP Use_Debug_Libraries 0
# PROP Output_Dir .\gRel
# PROP Intermediate_Dir .\gRel

INTDIR=.\gRel
VIM=gvim
EXTRAS="$(INTDIR)/vim.res" "$(INTDIR)/gui.obj" "$(INTDIR)/gui_w32.obj" "$(INTDIR)/gui_beval.obj" "$(INTDIR)/os_w32exe.obj"

CPP_PROJ=$(CPP_PROJ) /Zi /O2 /D "NDEBUG" /D "FEAT_GUI_MSWIN" /Fd.\gRel/ /Fo.\gRel/
# ADD CPP            /Zi /O2 /D "NDEBUG" /D "FEAT_GUI_MSWIN" /Fd.\gRel/ /Fo.\gRel/

RSC_PROJ=$(RSC_PROJ) /d "NDEBUG" /fo.\gRel\vim.res
# ADD RSC            /d "NDEBUG" /fo.\gRel\vim.res

LINK32_FLAGS=$(LINK32_FLAGS) /pdb:.\gRel/gvim.pdb -debug:full -debugtype:cv,fixup /map:.\oleDbg\gvim.map libc.lib /subsystem:windows /out:.\gvim.exe
# ADD LINK32                 /pdb:.\gRel/gvim.pdb -debug:full -debugtype:cv,fixup /map:.\oleDbg\gvim.map libc.lib /subsystem:windows /out:.\gvim.exe

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug gvim"

# PROP Use_Debug_Libraries 1
# PROP Output_Dir .\gDbg
# PROP Intermediate_Dir .\gDbg

INTDIR=.\gDbg
VIM=gvimd
EXTRAS="$(INTDIR)/vim.res" "$(INTDIR)/gui.obj" "$(INTDIR)/gui_w32.obj" "$(INTDIR)/gui_beval.obj" "$(INTDIR)/os_w32exe.obj"

CPP_PROJ=$(CPP_PROJ) /Zi /Od /D "_DEBUG" /D "FEAT_GUI_MSWIN" /Fd.\gDbg/ /Fo.\gDbg/
# ADD CPP            /Zi /Od /D "_DEBUG" /D "FEAT_GUI_MSWIN" /Fd.\gDbg/ /Fo.\gDbg/

RSC_PROJ=$(RSC_PROJ) /d "_DEBUG" /fo.\gDbg\vim.res
# ADD RSC            /d "_DEBUG" /fo.\gDbg\vim.res

LINK32_FLAGS=$(LINK32_FLAGS) libcd.lib /subsystem:windows /debug /profile /pdb:.\gDbg/gvimd.pdb -debug:full -debugtype:cv,fixup /map:.\gDbg\gvimd.map /out:.\gvimd.exe
# ADD LINK32                 libcd.lib /subsystem:windows /debug /profile /pdb:.\gDbg/gvimd.pdb -debug:full -debugtype:cv,fixup /map:.\gDbg\gvimd.map /out:.\gvimd.exe

!ELSEIF  "$(CFG)" == "Vim - Win32 Release vim"

# PROP Use_Debug_Libraries 0
# PROP Output_Dir .\Rel
# PROP Intermediate_Dir .\Rel

INTDIR=.\Rel
VIM=vim
EXTRAS=

CPP_PROJ=$(CPP_PROJ) /Zi /O2 /D "NDEBUG" /Fd.\Rel/ /Fo.\Rel/
# ADD CPP            /Zi /O2 /D "NDEBUG" /Fd.\Rel/ /Fo.\Rel/

LINK32_FLAGS=$(LINK32_FLAGS) /pdb:.\Rel/vim.pdb -debug:full -debugtype:cv,fixup /map:.\oleDbg\vim.map libc.lib /subsystem:console /out:.\vim.exe
# ADD LINK32                 /pdb:.\Rel/vim.pdb -debug:full -debugtype:cv,fixup /map:.\oleDbg\vim.map libc.lib /subsystem:console /out:.\vim.exe

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug vim"

# PROP Use_Debug_Libraries 1
# PROP Output_Dir .\Dbg
# PROP Intermediate_Dir .\Dbg

INTDIR=.\Dbg
VIM=vimd
EXTRAS=

CPP_PROJ=$(CPP_PROJ) /Zi /Od /D "_DEBUG" /Fd.\Dbg/ /Fo.\Dbg/
# ADD CPP            /Zi /Od /D "_DEBUG" /Fd.\Dbg/ /Fo.\Dbg/

LINK32_FLAGS=$(LINK32_FLAGS) libcd.lib /subsystem:console /debug /profile /pdb:.\Dbg/vimd.pdb -debug:full -debugtype:cv,fixup /map:.\Dbg/vimd.map /out:.\vimd.exe
# ADD LINK32                 libcd.lib /subsystem:console /debug /profile /pdb:.\Dbg/vimd.pdb -debug:full -debugtype:cv,fixup /map:.\Dbg/vimd.map /out:.\vimd.exe

!ENDIF

ALL : .\$(VIM).exe vimrun.exe install.exe uninstal.exe xxd/xxd.exe GvimExt/gvimext.dll

LINK32_OBJS= \
	$(EXTRAS) \
	"$(INTDIR)/arabic.obj" \
	"$(INTDIR)/autocmd.obj" \
	"$(INTDIR)/blowfish.obj" \
	"$(INTDIR)/buffer.obj" \
	"$(INTDIR)/charset.obj" \
	"$(INTDIR)/crypt.obj" \
	"$(INTDIR)/crypt_zip.obj" \
	"$(INTDIR)/debugger.obj" \
	"$(INTDIR)/dict.obj" \
	"$(INTDIR)/diff.obj" \
	"$(INTDIR)/digraph.obj" \
	"$(INTDIR)/edit.obj" \
	"$(INTDIR)/eval.obj" \
	"$(INTDIR)/evalfunc.obj" \
	"$(INTDIR)/ex_cmds.obj" \
	"$(INTDIR)/ex_cmds2.obj" \
	"$(INTDIR)/ex_docmd.obj" \
	"$(INTDIR)/ex_eval.obj" \
	"$(INTDIR)/ex_getln.obj" \
	"$(INTDIR)/fileio.obj" \
	"$(INTDIR)/findfile.obj" \
	"$(INTDIR)/fold.obj" \
	"$(INTDIR)/getchar.obj" \
	"$(INTDIR)/hardcopy.obj" \
	"$(INTDIR)/hashtab.obj" \
	"$(INTDIR)/indent.obj" \
	"$(INTDIR)/insexpand.obj" \
	"$(INTDIR)/json.obj" \
	"$(INTDIR)/list.obj" \
	"$(INTDIR)/main.obj" \
	"$(INTDIR)/mark.obj" \
	"$(INTDIR)/mbyte.obj" \
	"$(INTDIR)/memfile.obj" \
	"$(INTDIR)/memline.obj" \
	"$(INTDIR)/menu.obj" \
	"$(INTDIR)/message.obj" \
	"$(INTDIR)/misc1.obj" \
	"$(INTDIR)/misc2.obj" \
	"$(INTDIR)/move.obj" \
	"$(INTDIR)/normal.obj" \
	"$(INTDIR)/ops.obj" \
	"$(INTDIR)/option.obj" \
	"$(INTDIR)/os_mswin.obj" \
	"$(INTDIR)/winclip.obj" \
	"$(INTDIR)/os_win32.obj" \
	"$(INTDIR)/popupmnu.obj" \
	"$(INTDIR)/quickfix.obj" \
	"$(INTDIR)/regexp.obj" \
	"$(INTDIR)/screen.obj" \
	"$(INTDIR)/search.obj" \
	"$(INTDIR)/sha256.obj" \
	"$(INTDIR)/sign.obj" \
	"$(INTDIR)/spell.obj" \
	"$(INTDIR)/spellfile.obj" \
	"$(INTDIR)/syntax.obj" \
	"$(INTDIR)/tag.obj" \
	"$(INTDIR)/term.obj" \
	"$(INTDIR)/ui.obj" \
	"$(INTDIR)/undo.obj" \
	"$(INTDIR)/usercmd.obj" \
	"$(INTDIR)/userfunc.obj" \
	"$(INTDIR)/version.obj" \
	"$(INTDIR)/window.obj"

".\$(VIM).exe" : "$(INTDIR)" $(EXTRAS) $(LINK32_OBJS)
	@if exist $(INTDIR)\$(VIM).pdb del $(INTDIR)\$(VIM).pdb
	$(LINK32) $(LINK32_FLAGS) $(LINK32_OBJS)

"$(INTDIR)" :
	if not exist "$(INTDIR)/$(NULL)" mkdir "$(INTDIR)"

CLEAN :
	-@if exist "$(INTDIR)/$(NULL)" $(DEL_TREE) "$(INTDIR)"
	-@if exist $(VIM).exe erase $(VIM).exe
	-@if exist $(VIM).ilk erase $(VIM).ilk
	-@if exist $(VIM).map erase $(VIM).map
	-@if exist $(VIM).pdb erase $(VIM).pdb
	-@if exist DLLDATA.C erase DLLDATA.C
	-@if exist Make_ivc.bak attrib -r Make_ivc.bak
	-@if exist Make_ivc.bak erase Make_ivc.bak
	-@if exist Make_ivc.dsp erase Make_ivc.dsp
	-@if exist Make_ivc.dsw erase Make_ivc.dsw
	-@if exist Make_ivc.mdp erase Make_ivc.mdp
	-@if exist Make_ivc.ncb erase Make_ivc.ncb
	-@if exist Make_ivc.opt erase Make_ivc.opt
	-@if exist Make_ivc.plg erase Make_ivc.plg
	-@if exist dosinst.obj erase dosinst.obj
	-@if exist install.exe erase install.exe
	-@if exist uninstal.exe erase uninstal.exe
	-@if exist uninstal.obj erase uninstal.obj
	-@if exist vimrun.exe erase vimrun.exe
	-@if exist vimrun.obj erase vimrun.obj


install.exe: dosinst.c
	$(CPP) /Fe$@ /nologo /W3 -DNDEBUG -DWIN32 dosinst.c kernel32.lib shell32.lib user32.lib ole32.lib advapi32.lib uuid.lib

uninstal.exe: uninstal.c
	$(CPP) /nologo /W3 -DNDEBUG -DWIN32 uninstal.c shell32.lib advapi32.lib

vimrun.exe: vimrun.c
	$(CPP) /nologo /W3 -DNDEBUG vimrun.c

xxd/xxd.exe: xxd/xxd.c
	cd xxd
	$(MAKE) /NOLOGO -f Make_mvc.mak
	cd ..

GvimExt/gvimext.dll: GvimExt/gvimext.cpp GvimExt/gvimext.rc GvimExt/gvimext.h
	cd GvimExt
	$(MAKE) /NOLOGO -f Makefile
	cd ..

{.}.c{$(INTDIR)/}.obj:
	$(CPP) $(CPP_PROJ) $<

{.}.cpp{$(INTDIR)/}.obj:
	$(CPP) $(CPP_PROJ) /I $(INTDIR) $<

{.}.rc{$(INTDIR)/}.res:
	$(RSC) $(RSC_PROJ) $<

# Begin Target

# Name "Vim - Win32 Release gvim OLE"
# Name "Vim - Win32 Debug gvim OLE"
# Name "Vim - Win32 Release gvim"
# Name "Vim - Win32 Debug gvim"
# Name "Vim - Win32 Release vim"
# Name "Vim - Win32 Debug vim"

# Begin Source File

SOURCE=.\arabic.c
# End Source File
# Begin Source File
#
SOURCE=.\autocmd.c
# End Source File
# Begin Source File

SOURCE=.\blowfish.c
# End Source File
# Begin Source File

SOURCE=.\buffer.c
# End Source File
# Begin Source File

SOURCE=.\charset.c
# End Source File
# Begin Source File

SOURCE=.\crypt.c
# End Source File
# Begin Source File

SOURCE=.\crypt_zip.c
# End Source File
# Begin Source File

SOURCE=.\debugger.c
# End Source File
# Begin Source File

SOURCE=.\dict.c
# End Source File
# Begin Source File

SOURCE=.\diff.c
# End Source File
# Begin Source File

SOURCE=.\digraph.c
# End Source File
# Begin Source File

SOURCE=.\edit.c
# End Source File
# Begin Source File

SOURCE=.\eval.c
# End Source File
# Begin Source File

SOURCE=.\evalfunc.c
# End Source File
# Begin Source File

SOURCE=.\ex_cmds.c
# End Source File
# Begin Source File

SOURCE=.\ex_cmds2.c
# End Source File
# Begin Source File

SOURCE=.\ex_docmd.c
# End Source File
# Begin Source File

SOURCE=.\ex_eval.c
# End Source File
# Begin Source File

SOURCE=.\ex_getln.c
# End Source File
# Begin Source File

SOURCE=.\fileio.c
# End Source File
# Begin Source File
#
SOURCE=.\findfile.c
# End Source File
# Begin Source File

SOURCE=.\fold.c
# End Source File
# Begin Source File

SOURCE=.\getchar.c
# End Source File
# Begin Source File

SOURCE=.\hardcopy.c
# End Source File
# Begin Source File

SOURCE=.\hashtab.c
# End Source File
# Begin Source File
#
SOURCE=.\indent.c
# End Source File
# Begin Source File
#
SOURCE=.\insexpand.c
# End Source File
# Begin Source File

SOURCE=.\gui.c

!IF      "$(CFG)" == "Vim - Win32 Release vim"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug vim"

# PROP Exclude_From_Build 1

!ENDIF

# End Source File
# Begin Source File

SOURCE=.\gui_w32.c

!IF      "$(CFG)" == "Vim - Win32 Release vim"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug vim"

# PROP Exclude_From_Build 1

!ENDIF

# End Source File
# Begin Source File

SOURCE=.\gui_beval.c

!IF      "$(CFG)" == "Vim - Win32 Release vim"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug vim"

# PROP Exclude_From_Build 1

!ENDIF

# End Source File
# Begin Source File

SOURCE=.\os_w32exe.c

!IF      "$(CFG)" == "Vim - Win32 Release vim"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug vim"

# PROP Exclude_From_Build 1

!ENDIF

# End Source File
# Begin Source File

SOURCE=.\if_ole.cpp

!IF  "$(CFG)" == "Vim - Win32 Release gvim OLE"

# PROP Ignore_Default_Tool 1
# Begin Custom Build

"$(INTDIR)\if_ole.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\if_ole.h"
 cl.exe /nologo /MT /W3 /GX /I ".\proto" /D "WIN32" /c /Zi /O2 /D "NDEBUG" /D "FEAT_GUI_MSWIN" /D "FEAT_OLE" /Fd.\oleRel/ /Fo.\oleRel/ /I ".\oleRel" .\if_ole.cpp
 @rem This is the default rule with /I "$(IntDir)" added

# End Custom Build

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug gvim OLE"

# PROP Ignore_Default_Tool 1
# Begin Custom Build

"$(INTDIR)\if_ole.obj" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\if_ole.h"
 cl.exe /nologo /MT /W3 /GX /I ".\proto" /D "WIN32" /c /Zi /Od /D "_DEBUG" /D "FEAT_GUI_MSWIN" /D "FEAT_OLE" /Fd.\oleDbg/ /Fo.\oleDbg/ /I ".\oleDbg" .\if_ole.cpp
 @rem This is the default rule with /I "$(IntDir)" added

# End Custom Build

!ELSEIF  "$(CFG)" == "Vim - Win32 Release gvim"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug gvim"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Vim - Win32 Release vim"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug vim"

# PROP Exclude_From_Build 1

!ENDIF

# End Source File
# Begin Source File

SOURCE=.\if_ole.idl

!IF  "$(CFG)" == "Vim - Win32 Release gvim OLE"

# PROP Ignore_Default_Tool 1
# Begin Custom Build

"$(INTDIR)\if_ole.h" : $(SOURCE) "$(INTDIR)"
	if exist .\if_ole.h del .\if_ole.h
	midl /out .\oleRel /iid iid_ole.c /tlb vim.tlb /proxy nul /header if_ole.h .\if_ole.idl

# End Custom Build

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug gvim OLE"

# PROP Ignore_Default_Tool 1
# Begin Custom Build

"$(INTDIR)\if_ole.h" : $(SOURCE) "$(INTDIR)"
	if exist .\if_ole.h del .\if_ole.h
	midl /out .\oleDbg /iid iid_ole.c /tlb vim.tlb /proxy nul /header if_ole.h .\if_ole.idl

# End Custom Build

!ELSEIF  "$(CFG)" == "Vim - Win32 Release gvim"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug gvim"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Vim - Win32 Release vim"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug vim"

# PROP Exclude_From_Build 1

!ENDIF

# End Source File
# Begin Source File

SOURCE=.\json.c
# End Source File
# Begin Source File

SOURCE=.\list.c
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=.\mark.c
# End Source File
# Begin Source File

SOURCE=.\mbyte.c
# End Source File
# Begin Source File

SOURCE=.\memfile.c
# End Source File
# Begin Source File

SOURCE=.\memline.c
# End Source File
# Begin Source File

SOURCE=.\menu.c
# End Source File
# Begin Source File

SOURCE=.\message.c
# End Source File
# Begin Source File

SOURCE=.\misc1.c
# End Source File
# Begin Source File

SOURCE=.\misc2.c
# End Source File
# Begin Source File

SOURCE=.\move.c
# End Source File
# Begin Source File

SOURCE=.\normal.c
# End Source File
# Begin Source File

SOURCE=.\ops.c
# End Source File
# Begin Source File

SOURCE=.\option.c
# End Source File
# Begin Source File

SOURCE=.\os_mswin.c
# End Source File
# Begin Source File

SOURCE=.\winclip.c
# End Source File
# Begin Source File

SOURCE=.\os_win32.c
# End Source File
# Begin Source File

SOURCE=.\popupmnu.c
# End Source File
# Begin Source File

SOURCE=.\quickfix.c
# End Source File
# Begin Source File

SOURCE=.\regexp.c
# End Source File
# Begin Source File

SOURCE=.\screen.c
# End Source File
# Begin Source File

SOURCE=.\search.c
# End Source File
# Begin Source File

SOURCE=.\sha256.c
# End Source File
# Begin Source File

SOURCE=.\sign.c
# End Source File
# Begin Source File

SOURCE=.\spell.c
# End Source File
# Begin Source File

SOURCE=.\spellfile.c
# End Source File
# Begin Source File

SOURCE=.\syntax.c
# End Source File
# Begin Source File

SOURCE=.\tag.c
# End Source File
# Begin Source File

SOURCE=.\term.c
# End Source File
# Begin Source File

SOURCE=.\ui.c
# End Source File
# Begin Source File

SOURCE=.\undo.c
# End Source File
# Begin Source File

SOURCE=.\usercmd.c
# End Source File
# Begin Source File

SOURCE=.\userfunc.c
# End Source File
# Begin Source File

SOURCE=.\version.c
# End Source File
# Begin Source File

SOURCE=.\vim.rc

!IF  "$(CFG)" == "Vim - Win32 Release gvim OLE"

"$(INTDIR)\vim.res" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\if_ole.h"

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug gvim OLE"

"$(INTDIR)\vim.res" : $(SOURCE) "$(INTDIR)" "$(INTDIR)\if_ole.h"

!ELSEIF  "$(CFG)" == "Vim - Win32 Release gvim"

"$(INTDIR)\vim.res" : $(SOURCE) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug gvim"

"$(INTDIR)\vim.res" : $(SOURCE) "$(INTDIR)"

!ELSEIF  "$(CFG)" == "Vim - Win32 Release vim"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "Vim - Win32 Debug vim"

# PROP Exclude_From_Build 1

!ENDIF

# End Source File
# Begin Source File

SOURCE=.\window.c
# End Source File
# End Target
# End Project
