" Vim syntax file
" Language:	Inno Setup File (iss file) and My InnoSetup extension
" Maintainer:	Dominique Stéphan (dominique@mggen.com)
" Last change:	2003 May 11

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" shut case off
syn case ignore

" Section
syn region issHeader		start="\[" end="\]"

" Label in the [Setup] Section
syn match  issLabel		"^[^=]\+="

" URL
syn match  issURL		"http[s]\=:\/\/.*$"

" syn match  issName		"[^: ]\+:"
syn match  issName		"Name:"
syn match  issName		"MinVersion:\|OnlyBelowVersion:"
syn match  issName		"Source:\|DestDir:\|DestName:\|CopyMode:"
syn match  issName		"Attribs:\|FontInstall:\|Flags:"
syn match  issName		"FileName:\|Parameters:\|WorkingDir:\|Comment:"
syn match  issName		"IconFilename:\|IconIndex:"
syn match  issName		"Section:\|Key:\|String:"
syn match  issName		"Root:\|SubKey:\|ValueType:\|ValueName:\|ValueData:"
syn match  issName		"RunOnceId:"
syn match  issName		"Type:"
syn match  issName		"Components:\|Description:\|GroupDescription\|Types:"

syn match  issComment		"^;.*$"

" folder constant
syn match  issFolder		"{[^{]*}"

" string
syn region issString	start=+"+  end=+"+ contains=issFolder

" [Dirs]
syn keyword issDirsFlags deleteafterinstall uninsalwaysuninstall uninsneveruninstall

" [Files]
syn keyword issFilesCopyMode normal onlyifdoesntexist alwaysoverwrite alwaysskipifsameorolder
syn keyword issFilesAttribs readonly hidden system
syn keyword issFilesFlags comparetimestampalso confirmoverwrite deleteafterinstall
syn keyword issFilesFlags external fontisnttruetype isreadme overwritereadonly
syn keyword issFilesFlags regserver regtypelib restartreplace
syn keyword issFilesFlags sharedfile skipifsourcedoesntexist uninsneveruninstall

" [Icons]
syn keyword issIconsFlags createonlyiffileexists runminimized uninsneveruninstall useapppaths

" [INI]
syn keyword issINIFlags createkeyifdoesntexist uninsdeleteentry uninsdeletesection uninsdeletesectionifempty

" [Registry]
syn keyword issRegRootKey   HKCR HKCU HKLM HKU HKCC
syn keyword issRegValueType none string expandsz multisz dword binary
syn keyword issRegFlags createvalueifdoesntexist deletekey deletevalue preservestringtype
syn keyword issRegFlags uninsclearvalue uninsdeletekey uninsdeletekeyifempty uninsdeletevalue

" [Run] and [UninstallRun]
syn keyword issRunFlags nowait shellexec skipifdoesntexist runminimized waituntilidle
syn keyword issRunFlags postinstall unchecked showcheckbox

" [Types]
syn keyword issTypesFlags iscustom

" [Components]
syn keyword issComponentsFlags fixed restart disablenouninstallwarning

" [UninstallDelete] and [InstallDelete]
syn keyword issInstallDeleteType files filesandordirs dirifempty


" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_iss_syntax_inits")
  if version < 508
    let did_iss_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

   " The default methods for highlighting.  Can be overridden later
   HiLink issHeader	Special
   HiLink issComment	Comment
   HiLink issLabel	Type
   HiLink issName	Type
   HiLink issFolder	Special
   HiLink issString	String
   HiLink issValue	String
   HiLink issURL	Include

   HiLink issDirsFlags		Keyword
   HiLink issFilesCopyMode	Keyword
   HiLink issFilesAttribs	Keyword
   HiLink issFilesFlags		Keyword
   HiLink issIconsFlags		Keyword
   HiLink issINIFlags		Keyword
   HiLink issRegRootKey		Keyword
   HiLink issRegValueType	Keyword
   HiLink issRegFlags		Keyword
   HiLink issRunFlags		Keyword
   HiLink issTypesFlags		Keyword
   HiLink issComponentsFlags	Keyword
   HiLink issInstallDeleteType	Keyword


  delcommand HiLink
endif

let b:current_syntax = "iss"

" vim:ts=8
