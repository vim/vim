" Vim syntax file
" Language:             Inno Setup File (iss file) and My InnoSetup extension
" Maintainer:           Jason Mills (jmills@cs.mun.ca)
" Previous Maintainer:  Dominique Stéphan (dominique@mggen.com)
" Last Change:          2004 Jul 13

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" shut case off
syn case ignore

" Preprocessor
syn region issPreProc start="^\s*#" end="$"

" Section
syn region issHeader            start="\[" end="\]"

" Label in the [Setup] Section
syn match  issLabel             "^[^=]\+="

" URL
syn match  issURL       "http[s]\=:\/\/.*$"

" syn match  issName    "[^: ]\+:"
syn match  issName      "Name:"
syn match  issName      "MinVersion:\|OnlyBelowVersion:\|Languages:"
syn match  issName      "Source:\|DestDir:\|DestName:\|CopyMode:"
syn match  issName      "Attribs:\|Permissions:\|FontInstall:\|Flags:"
syn match  issName      "FileName:\|Parameters:\|WorkingDir:\|HotKey:\|Comment:"
syn match  issName      "IconFilename:\|IconIndex:"
syn match  issName      "Section:\|Key:\|String:"
syn match  issName      "Root:\|SubKey:\|ValueType:\|ValueName:\|ValueData:"
syn match  issName      "RunOnceId:"
syn match  issName      "Type:"
syn match  issName      "Components:\|Description:\|GroupDescription:\|Types:\|ExtraDiskSpaceRequired:"
syn match  issName      "StatusMsg:\|RunOnceId:\|Tasks:"
syn match  issName      "MessagesFile:\|LicenseFile:\|InfoBeforeFile:\|InfoAfterFile:"

syn match  issComment   "^;.*$"

" folder constant
syn match  issFolder    "{[^{]*}"

" string
syn region issString    start=+"+  end=+"+ contains=issFolder

" [Dirs]
syn keyword issDirsFlags deleteafterinstall uninsalwaysuninstall uninsneveruninstall

" [Files]
syn keyword issFilesCopyMode normal onlyifdoesntexist alwaysoverwrite alwaysskipifsameorolder dontcopy
syn keyword issFilesAttribs readonly hidden system
syn keyword issFilesPermissions full modify readexec
syn keyword issFilesFlags allowunsafefiles comparetimestampalso confirmoverwrite deleteafterinstall
syn keyword issFilesFlags dontcopy dontverifychecksum external fontisnttruetype ignoreversion 
syn keyword issFilesFlags isreadme onlyifdestfileexists onlyifdoesntexist overwritereadonly 
syn keyword issFilesFlags promptifolder recursesubdirs regserver regtypelib restartreplace
syn keyword issFilesFlags sharedfile skipifsourcedoesntexist sortfilesbyextension touch 
syn keyword issFilesFlags uninsremovereadonly uninsrestartdelete uninsneveruninstall

" [Icons]
syn keyword issIconsFlags closeonexit createonlyiffileexists dontcloseonexit 
syn keyword issIconsFlags runmaximized runminimized uninsneveruninstall useapppaths

" [INI]
syn keyword issINIFlags createkeyifdoesntexist uninsdeleteentry uninsdeletesection uninsdeletesectionifempty

" [Registry]
syn keyword issRegRootKey   HKCR HKCU HKLM HKU HKCC
syn keyword issRegValueType none string expandsz multisz dword binary
syn keyword issRegFlags createvalueifdoesntexist deletekey deletevalue dontcreatekey 
syn keyword issRegFlags preservestringtype noerror uninsclearvalue 
syn keyword issRegFlags uninsdeletekey uninsdeletekeyifempty uninsdeletevalue

" [Run] and [UninstallRun]
syn keyword issRunFlags hidewizard nowait postinstall runhidden runmaximized
syn keyword issRunFlags runminimized shellexec skipifdoesntexist skipifnotsilent 
syn keyword issRunFlags skipifsilent unchecked waituntilidle

" [Types]
syn keyword issTypesFlags iscustom

" [Components]
syn keyword issComponentsFlags dontinheritcheck exclusive fixed restart disablenouninstallwarning

" [UninstallDelete] and [InstallDelete]
syn keyword issInstallDeleteType files filesandordirs dirifempty

" [Tasks]
syn keyword issTasksFlags checkedonce dontinheritcheck exclusive restart unchecked 


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
   HiLink issHeader     Special
   HiLink issComment    Comment
   HiLink issLabel      Type
   HiLink issName       Type
   HiLink issFolder     Special
   HiLink issString     String
   HiLink issValue      String
   HiLink issURL        Include
   HiLink issPreProc    PreProc 

   HiLink issDirsFlags          Keyword
   HiLink issFilesCopyMode      Keyword
   HiLink issFilesAttribs       Keyword
   HiLink issFilesFlags         Keyword
   HiLink issIconsFlags         Keyword
   HiLink issINIFlags           Keyword
   HiLink issRegRootKey         Keyword
   HiLink issRegValueType       Keyword
   HiLink issRegFlags           Keyword
   HiLink issRunFlags           Keyword
   HiLink issTypesFlags         Keyword
   HiLink issComponentsFlags    Keyword
   HiLink issInstallDeleteType  Keyword
   HiLink issTasksFlags         Keyword

  delcommand HiLink
endif

let b:current_syntax = "iss"

" vim:ts=8
