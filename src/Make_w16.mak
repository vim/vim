#
# Borland C++ 5.0[12] makefile for vim, 16-bit windows gui version
# By Vince Negri
# *************************************************************
# * WARNING!
# * This was originally produced by the IDE, but has since been
# * modifed to make it work properly. Adjust with care!
# * In particular, leave LinkerLocalOptsAtW16_gvim16dexe alone
# * unless you are a guru.
# *************************************************************
#
# Look for BOR below and either pass a different value or
# adjust the path as required. For example
#   make -fMake_w16.mak -DBOR=C:\PF\Borland\BC5.01 -B BccW16.cfg
#   make -fMake_w16.mak
# Note: $(BOR) is effectively ignored unless BccW16.cfg is rebuilt.
#
# Does not compile with Borland C++ 4.51 Walter Briscoe 2003-02-24
# "out of memory" from compiler if gvim16 wildly wrong. WFB 2003-03-04
#
# vim16.def must be a DOS-formatted file. (\r\n line endings.)
# It is a UNIX-formatted file (\n line endings) in vim-*-extra.tar.gz

.AUTODEPEND

#
# Borland C++ tools
#
IMPLIB  = Implib
BCC     = Bcc +BccW16.cfg
TLINK   = TLink
TLIB    = TLib
BRC     = Brc
TASM    = Tasm
#
# IDE macros
#

#
# Options
#
!ifndef BOR
BOR = D:\BC5
!endif

# !ifndef INTDIR is lethal considering CLEAN below. WFB 2003-03-13
INTDIR=w16

#  /Twe Make the target a Windows .EXE with explicit functions exportable +
#  /x   Map file off
#  /l   Include source line numbers in object map files`
#  /c   case sensitive link
#  /C   Case-sensitive exports and imports (16-bit only)
#  /k   Produce "No Stack" warning.
#  /Oa  Minimise segment alignment
#  /Oc  Minimise Chain fixes
#  /Oi  Minimise Iterated data
#  /Or  Minimise resource alignment
#  /P   -P=x  Code pack size
#  /V   Windows version for application
#  /L   Folder to search for library files
LinkerLocalOptsAtW16_gvim16dexe =/Twe/x/l/c/C/k/Or/Oc/Oa/Oi/P=65535/V3.10

CompInheritOptsAt_gvim16dexe = \
   -I$(BOR)\INCLUDE;PROTO;. \
   -DFEAT_GUI;FEAT_GUI_MSWIN;FEAT_GUI_W16;MSWIN;WIN16;MSWIN16_FASTTEXT \
   -DFEAT_TOOLBAR;WIN16_3DLOOK

#
# Dependency List
#
Dep_Gvim16 = \
   gvim16.exe

ObjFiles = \
   $(INTDIR)\buffer.obj\
   $(INTDIR)\charset.obj\
   $(INTDIR)\diff.obj\
   $(INTDIR)\digraph.obj\
   $(INTDIR)\edit.obj\
   $(INTDIR)\eval.obj\
   $(INTDIR)\ex_cmds.obj\
   $(INTDIR)\ex_cmds2.obj\
   $(INTDIR)\ex_docmd.obj\
   $(INTDIR)\ex_eval.obj\
   $(INTDIR)\ex_getln.obj\
   $(INTDIR)\fileio.obj\
   $(INTDIR)\fold.obj\
   $(INTDIR)\getchar.obj\
   $(INTDIR)\hardcopy.obj\
   $(INTDIR)\hashtab.obj\
   $(INTDIR)\gui.obj\
   $(INTDIR)\gui_w16.obj\
   $(INTDIR)\main.obj\
   $(INTDIR)\mark.obj\
   $(INTDIR)\mbyte.obj\
   $(INTDIR)\memfile.obj\
   $(INTDIR)\memline.obj\
   $(INTDIR)\menu.obj\
   $(INTDIR)\message.obj\
   $(INTDIR)\misc1.obj\
   $(INTDIR)\misc2.obj\
   $(INTDIR)\move.obj\
   $(INTDIR)\normal.obj\
   $(INTDIR)\ops.obj\
   $(INTDIR)\option.obj\
   $(INTDIR)\os_win16.obj\
   $(INTDIR)\os_msdos.obj\
   $(INTDIR)\os_mswin.obj\
   $(INTDIR)\popupmnu.obj\
   $(INTDIR)\quickfix.obj\
   $(INTDIR)\regexp.obj\
   $(INTDIR)\screen.obj\
   $(INTDIR)\search.obj\
   $(INTDIR)\spell.obj\
   $(INTDIR)\syntax.obj\
   $(INTDIR)\tag.obj\
   $(INTDIR)\term.obj\
   $(INTDIR)\ui.obj\
   $(INTDIR)\undo.obj\
   $(INTDIR)\version.obj\
   $(INTDIR)\window.obj

Dep_gvim16dexe = \
   vimtbar.lib\
   vim16.def\
   $(INTDIR)\vim16.res\
   $(ObjFiles)

# Without the following, the implicit rule in BUILTINS.MAK is picked up
# for a rule for .c.obj rather than the local implicit rule
.SUFFIXES
.SUFFIXES .c .obj
.path.c = .

# -P-	Force C++ compilation off
# -c	Compilation only
# -n    Place .OBJ files
{.}.c{$(INTDIR)}.obj:
  $(BCC) -P- -c -n$(INTDIR)\ {$< }

Gvim16 : BccW16.cfg $(Dep_Gvim16)
  echo MakeNode

gvim16.exe : $(Dep_gvim16dexe)
  $(TLINK)   $(LinkerLocalOptsAtW16_gvim16dexe) @&&|
c0wl.obj $(ObjFiles)
|,$*,,vimtbar ctl3dv2 import cwl, vim16.def,$(INTDIR)\vim16.res

# Force objects to be built if $(BOR) changes
$(ObjFiles) : Make_w16.mak BccW16.cfg

$(INTDIR)\vim16.res : vim16.rc
  $(BRC) -R @&&|
  $(CompInheritOptsAt_gvim16dexe) -fo$*.res $?
|


# Compiler configuration file
# There is no rule for $(INTDIR) as make always says it does not exist
BccW16.cfg :
	-@if not exist $(INTDIR)\$(NULL) mkdir $(INTDIR)
	Copy &&|
-3		; Generate 80386 protected-mode compatible instructions
-a		; Byte alignment
-dc		; Move string literals from data segment to code segment
-ff		; Fast floating point
-H		; Generate and use precompiled headers
-H=$(INTDIR)\gvim16.csm	; gvim16.csm is the precompiled header filename
-k-		; No standard stack frame
-ml		; Large memory model
-OW		; Suppress the inc bp/dec bp on windows far functions
-O1		; Generate smallest possible code
-O2		; Generate fastest possible code (overrides prior -O1 control)
-pr		; Fastcall calling convention passing parameters in registers
-R-		; Exclude browser information in generated .OBJ files
-v-		; Turn off source debugging
-vi		; Turn inline function expansion on
-WE		; Only __far _export functions are exported
-w		; Display warnings
-w-par		; Suppress: Parameter 'parameter' is never used
-w-pch		; Cannot create pre-compiled header: initialized data in header
-w-sig		; identifier' declared but never used
-w-ucp		; Mixing pointers to different 'char' types
-wuse		; 'identifier' declared but never used
 $(CompInheritOptsAt_gvim16dexe)
| $@

!IF "$(OS)" == "Windows_NT"
NULL=
DEL_TREE = rmdir /s /q
!ELSE
NULL=nul
DEL_TREE = deltree /y
!ENDIF

CLEAN:
	-@if exist $(INTDIR)\$(NULL) $(DEL_TREE) $(INTDIR)
	-@if exist BccW16.cfg erase BccW16.cfg
	-@if exist gvim16.exe erase gvim16.exe
