" Vim syntax file
" Language:	Debian control files
" Maintainer:	Gerfried Fuchs <alfie@debian.org>
" Last Change:  29 Oct 2004
" URL:          http://alfie.ist.org/software/vim/syntax/debcontrol.vim
"
" Original Version: Wichert Akkerman <wakkerma@debian.org>

" Comments are very welcome - but please make sure that you are commenting on
" the latest version of this file.
" SPAM is _NOT_ welcome - be ready to be reported!

" Standard syntax initialization
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Everything that is not explicitly matched by the rules below
syn match debcontrolElse "^.*$"

" Common seperators
syn match debControlComma ", *"
syn match debControlSpace " "

" Define some common expressions we can use later on
syn match debcontrolArchitecture contained "\(all\|any\|alpha\|amd64\|arm\|hppa\|i386\|ia64\|m68k\|mipsel\|mips\|powerpc\|s390\|sheb\|sh\|sparc64\|sparc\|hurd-i386\|kfreebsd-i386\|knetbsd-i386\|netbsd-i386\)"
syn match debcontrolName contained "[a-z][a-z0-9+-]*"
syn match debcontrolPriority contained "\(extra\|important\|optional\|required\|standard\)"
syn match debcontrolSection contained "\(\(contrib\|non-free\|non-US/main\|non-US/contrib\|non-US/non-free\)/\)\=\(admin\|base\|comm\|devel\|doc\|editors\|electronics\|embedded\|games\|gnome\|graphics\|hamradio\|interpreters\|kde\|libs\|libdevel\|mail\|math\|misc\|net\|news\|oldlibs\|otherosfs\|perl\|python\|science\|shells\|sound\|text\|tex\|utils\|web\|x11\|debian-installer\)"
syn match debcontrolVariable contained "\${.\{-}}"

" An email address
syn match	debcontrolEmail	"[_=[:alnum:]\.+-]\+@[[:alnum:]\./\-]\+"
syn match	debcontrolEmail	"<.\{-}>"

" List of all legal keys
syn match debcontrolKey contained "^\(Source\|Package\|Section\|Priority\|Maintainer\|Uploaders\|Build-Depends\|Build-Conflicts\|Build-Depends-Indep\|Build-Conflicts-Indep\|Standards-Version\|Pre-Depends\|Depends\|Recommends\|Suggests\|Provides\|Replaces\|Conflicts\|Essential\|Architecture\|Description\|Bugs\|Origin\|Enhances\): *"

" Fields for which we do strict syntax checking
syn region debcontrolStrictField start="^Architecture" end="$" contains=debcontrolKey,debcontrolArchitecture,debcontrolSpace oneline
syn region debcontrolStrictField start="^\(Package\|Source\)" end="$" contains=debcontrolKey,debcontrolName oneline
syn region debcontrolStrictField start="^Priority" end="$" contains=debcontrolKey,debcontrolPriority oneline
syn region debcontrolStrictField start="^Section" end="$" contains=debcontrolKey,debcontrolSection oneline

" Catch-all for the other legal fields
syn region debcontrolField start="^\(Maintainer\|Build-Depends\|Build-Conflicts\|Build-Depends-Indep\|Build-Conflicts-Indep\|Standards-Version\|Pre-Depends\|Depends\|Recommends\|Suggests\|Provides\|Replaces\|Conflicts\|Essential\|Bugs\|Origin\|Enhances\):" end="$" contains=debcontrolKey,debcontrolVariable,debcontrolEmail oneline
syn region debcontrolMultiField start="^\(Uploaders\|Description\):" skip="^ " end="^$"me=s-1 end="^[^ ]"me=s-1 contains=debcontrolKey,debcontrolEmail,debcontrolVariable

" Associate our matches and regions with pretty colours
if version >= 508 || !exists("did_debcontrol_syn_inits")
  if version < 508
    let did_debcontrol_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink debcontrolKey		Keyword
  HiLink debcontrolField	Normal
  HiLink debcontrolStrictField	Error
  HiLink debcontrolMultiField	Normal
  HiLink debcontrolArchitecture	Normal
  HiLink debcontrolName		Normal
  HiLink debcontrolPriority	Normal
  HiLink debcontrolSection	Normal
  HiLink debcontrolVariable	Identifier
  HiLink debcontrolEmail	Identifier
  HiLink debcontrolElse		Special

  delcommand HiLink
endif

let b:current_syntax = "debcontrol"

" vim: ts=8 sw=2
