" Vim syntax file
" Language:    Debian control files
" Maintainer:  Debian Vim Maintainers <pkg-vim-maintainers@lists.alioth.debian.org>
" Former Maintainers: Gerfried Fuchs <alfie@ist.org>
"                     Wichert Akkerman <wakkerma@debian.org>
" Last Change: 2012 Jun 06
" URL: http://anonscm.debian.org/hg/pkg-vim/vim/raw-file/unstable/runtime/syntax/debcontrol.vim

" Standard syntax initialization
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Should match case except for the keys of each field
syn case match

" Everything that is not explicitly matched by the rules below
syn match debcontrolElse "^.*$"

" Common seperators
syn match debControlComma ", *"
syn match debControlSpace " "

" Define some common expressions we can use later on
syn match debcontrolArchitecture contained "\%(all\|linux-any\|\%(any-\)\=\%(alpha\|amd64\|arm\%(e[bl]\|hf\)\=\|avr32\|hppa\|i386\|ia64\|lpia\|m32r\|m68k\|mips\%(el\)\=\|powerpc\|ppc64\|s390x\=\|sh[34]\(eb\)\=\|sh\|sparc\%(64\)\=\)\|hurd-\%(i386\|any\)\|kfreebsd-\%(i386\|amd64\|any\)\|knetbsd-\%(i386\|any\)\|kopensolaris-\%(i386\|any\)\|netbsd-\%(alpha\|i386\|any\)\|any\)"
syn match debcontrolMultiArch contained "\%(no\|foreign\|allowed\|same\)"
syn match debcontrolName contained "[a-z0-9][a-z0-9+.-]\+"
syn match debcontrolPriority contained "\(extra\|important\|optional\|required\|standard\)"
syn match debcontrolSection contained "\v((contrib|non-free|non-US/main|non-US/contrib|non-US/non-free|restricted|universe|multiverse)/)?(admin|cli-mono|comm|database|debian-installer|debug|devel|doc|editors|education|electronics|embedded|fonts|games|gnome|gnustep|gnu-r|graphics|hamradio|haskell|httpd|interpreters|introspection|java|kde|kernel|libs|libdevel|lisp|localization|mail|math|metapackages|misc|net|news|ocaml|oldlibs|otherosfs|perl|php|python|ruby|science|shells|sound|text|tex|utils|vcs|video|web|x11|xfce|zope)"
syn match debcontrolPackageType contained "u\?deb"
syn match debcontrolVariable contained "\${.\{-}}"
syn match debcontrolDmUpload contained "\cyes"

" A URL (using the domain name definitions from RFC 1034 and 1738), right now
" only enforce protocol and some sanity on the server/path part;
syn match debcontrolHTTPUrl contained "\vhttps?://[[:alnum:]][-[:alnum:]]*[[:alnum:]]?(\.[[:alnum:]][-[:alnum:]]*[[:alnum:]]?)*\.[[:alpha:]][-[:alnum:]]*[[:alpha:]]?(:\d+)?(/[^[:space:]]*)?$"
syn match debcontrolVcsSvn contained "\vsvn%(\+ssh)?://[[:alnum:]][-[:alnum:]]*[[:alnum:]]?(\.[[:alnum:]][-[:alnum:]]*[[:alnum:]]?)*\.[[:alpha:]][-[:alnum:]]*[[:alpha:]]?(:\d+)?(/[^[:space:]]*)?$"
syn match debcontrolVcsCvs contained "\v%(\-d *)?:pserver:[^@]+\@[[:alnum:]][-[:alnum:]]*[[:alnum:]]?(\.[[:alnum:]][-[:alnum:]]*[[:alnum:]]?)*\.[[:alpha:]][-[:alnum:]]*[[:alpha:]]?:/[^[:space:]]*%( [^[:space:]]+)?$"
syn match debcontrolVcsGit contained "\v%(git|http)://[[:alnum:]][-[:alnum:]]*[[:alnum:]]?(\.[[:alnum:]][-[:alnum:]]*[[:alnum:]]?)*\.[[:alpha:]][-[:alnum:]]*[[:alpha:]]?(:\d+)?(/[^[:space:]]*)?$"

" An email address
syn match	debcontrolEmail	"[_=[:alnum:]\.+-]\+@[[:alnum:]\./\-]\+"
syn match	debcontrolEmail	"<.\{-}>"

" #-Comments
syn match debcontrolComment "^#.*$" contains=@Spell

syn case ignore

" List of all legal keys
syn match debcontrolKey contained "^\%(Source\|Package\|Section\|Priority\|\%(XSBC-Original-\)\=Maintainer\|Uploaders\|Build-\%(Conflicts\|Depends\)\%(-Indep\)\=\|Standards-Version\|\%(Pre-\)\=Depends\|Recommends\|Suggests\|Provides\|Replaces\|Conflicts\|Enhances\|Breaks\|Essential\|Architecture\|Multi-Arch\|Description\|Bugs\|Origin\|X[SB]-Python-Version\|Homepage\|\(XS-\)\=Vcs-\(Browser\|Arch\|Bzr\|Cvs\|Darcs\|Git\|Hg\|Mtn\|Svn\)\|\%(XC-\)\=Package-Type\|\%(XS-\)\=DM-Upload-Allowed\): *"

" Fields for which we do strict syntax checking
syn region debcontrolStrictField start="^Architecture" end="$" contains=debcontrolKey,debcontrolArchitecture,debcontrolSpace oneline
syn region debcontrolStrictField start="^Multi-Arch" end="$" contains=debcontrolKey,debcontrolMultiArch oneline
syn region debcontrolStrictField start="^\(Package\|Source\)" end="$" contains=debcontrolKey,debcontrolName oneline
syn region debcontrolStrictField start="^Priority" end="$" contains=debcontrolKey,debcontrolPriority oneline
syn region debcontrolStrictField start="^Section" end="$" contains=debcontrolKey,debcontrolSection oneline
syn region debcontrolStrictField start="^\%(XC-\)\=Package-Type" end="$" contains=debcontrolKey,debcontrolPackageType oneline
syn region debcontrolStrictField start="^Homepage" end="$" contains=debcontrolKey,debcontrolHTTPUrl oneline keepend
syn region debcontrolStrictField start="^\%(XS-\)\=Vcs-\%(Browser\|Arch\|Bzr\|Darcs\|Hg\)" end="$" contains=debcontrolKey,debcontrolHTTPUrl oneline keepend
syn region debcontrolStrictField start="^\%(XS-\)\=Vcs-Svn" end="$" contains=debcontrolKey,debcontrolVcsSvn,debcontrolHTTPUrl oneline keepend
syn region debcontrolStrictField start="^\%(XS-\)\=Vcs-Cvs" end="$" contains=debcontrolKey,debcontrolVcsCvs oneline keepend
syn region debcontrolStrictField start="^\%(XS-\)\=Vcs-Git" end="$" contains=debcontrolKey,debcontrolVcsGit oneline keepend
syn region debcontrolStrictField start="^\%(XS-\)\=DM-Upload-Allowed" end="$" contains=debcontrolKey,debcontrolDmUpload oneline

" Catch-all for the other legal fields
syn region debcontrolField start="^\%(\%(XSBC-Original-\)\=Maintainer\|Standards-Version\|Essential\|Bugs\|Origin\|X[SB]-Python-Version\|\%(XS-\)\=Vcs-Mtn\):" end="$" contains=debcontrolKey,debcontrolVariable,debcontrolEmail oneline
syn region debcontrolMultiField start="^\%(Build-\%(Conflicts\|Depends\)\%(-Indep\)\=\|\%(Pre-\)\=Depends\|Recommends\|Suggests\|Provides\|Replaces\|Conflicts\|Enhances\|Breaks\|Uploaders\|Description\):" skip="^ " end="^$"me=s-1 end="^[^ #]"me=s-1 contains=debcontrolKey,debcontrolEmail,debcontrolVariable,debcontrolComment
syn region debcontrolMultiFieldSpell start="^\%(Description\):" skip="^ " end="^$"me=s-1 end="^[^ #]"me=s-1 contains=debcontrolKey,debcontrolEmail,debcontrolVariable,debcontrolComment,@Spell

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
  HiLink debcontrolMultiArch	Normal
  HiLink debcontrolName		Normal
  HiLink debcontrolPriority	Normal
  HiLink debcontrolSection	Normal
  HiLink debcontrolPackageType	Normal
  HiLink debcontrolVariable	Identifier
  HiLink debcontrolEmail	Identifier
  HiLink debcontrolVcsSvn	Identifier
  HiLink debcontrolVcsCvs	Identifier
  HiLink debcontrolVcsGit	Identifier
  HiLink debcontrolHTTPUrl	Identifier
  HiLink debcontrolDmUpload	Identifier
  HiLink debcontrolComment	Comment
  HiLink debcontrolElse		Special

  delcommand HiLink
endif

let b:current_syntax = "debcontrol"

" vim: ts=8 sw=2
