" Vim syntax file
" Language:	Debian control files
" Maintainer:  Debian Vim Maintainers <pkg-vim-maintainers@lists.alioth.debian.org>
" Former Maintainers: Gerfried Fuchs <alfie@ist.org>
"                     Wichert Akkerman <wakkerma@debian.org>
" Last Change: 2008-02-23
" URL: http://git.debian.org/?p=pkg-vim/vim.git;a=blob_plain;f=runtime/ftplugin/debcontrol.vim;hb=debian

" Comments are very welcome - but please make sure that you are commenting on
" the latest version of this file.
" SPAM is _NOT_ welcome - be ready to be reported!

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
syn match debcontrolArchitecture contained "\(all\|any\|alpha\|amd64\|arm\(e[bl]\)\=\|hppa\|i386\|ia64\|m32r\|m68k\|mipsel\|mips\|powerpc\|ppc64\|s390x\=\|sh[34]\(eb\)\=\|sh\|sparc64\|sparc\|hurd-i386\|kfreebsd-\(i386\|gnu\)\|knetbsd-i386\|netbsd-\(alpha\|i386\)\)"
syn match debcontrolName contained "[a-z0-9][a-z0-9+.-]\+"
syn match debcontrolPriority contained "\(extra\|important\|optional\|required\|standard\)"
syn match debcontrolSection contained "\(\(contrib\|non-free\|non-US/main\|non-US/contrib\|non-US/non-free\|restricted\|universe\|multiverse\)/\)\=\(admin\|base\|comm\|devel\|doc\|editors\|electronics\|embedded\|games\|gnome\|graphics\|hamradio\|interpreters\|kde\|libs\|libdevel\|mail\|math\|misc\|net\|news\|oldlibs\|otherosfs\|perl\|python\|science\|shells\|sound\|text\|tex\|utils\|web\|x11\|debian-installer\)"
syn match debcontrolPackageType contained "u\?deb"
syn match debcontrolVariable contained "\${.\{-}}"
syn match debcontrolDmUpload contained "\cyes"

" A URL (using the domain name definitions from RFC 1034 and 1738), right now
" only enforce protocol and some sanity on the server/path part;
syn match debcontrolHTTPUrl contained "\vhttps?://[[:alnum:]][-[:alnum:]]*[[:alnum:]]?(\.[[:alnum:]][-[:alnum:]]*[[:alnum:]]?)*\.[[:alpha:]][-[:alnum:]]*[[:alpha:]]?(:\d+)?(/[^[:space:]]*)?$"
syn match debcontrolVcsSvn contained "\vsvn%(\+ssh)?://[[:alnum:]][-[:alnum:]]*[[:alnum:]]?(\.[[:alnum:]][-[:alnum:]]*[[:alnum:]]?)*\.[[:alpha:]][-[:alnum:]]*[[:alpha:]]?(:\d+)?(/[^[:space:]]*)?$"
syn match debcontrolVcsCvs contained "\v%(\-d *)?:pserver:[^@]+\@[[:alnum:]][-[:alnum:]]*[[:alnum:]]?(\.[[:alnum:]][-[:alnum:]]*[[:alnum:]]?)*\.[[:alpha:]][-[:alnum:]]*[[:alpha:]]?:/[^[:space:]]*%( [^[:space:]]+)?$"
syn match debcontrolVcsGit contained "\vgit://[[:alnum:]][-[:alnum:]]*[[:alnum:]]?(\.[[:alnum:]][-[:alnum:]]*[[:alnum:]]?)*\.[[:alpha:]][-[:alnum:]]*[[:alpha:]]?(:\d+)?(/[^[:space:]]*)?$"

" An email address
syn match	debcontrolEmail	"[_=[:alnum:]\.+-]\+@[[:alnum:]\./\-]\+"
syn match	debcontrolEmail	"<.\{-}>"

" #-Comments
syn match debcontrolComment "^#.*$"

syn case ignore

" List of all legal keys
syn match debcontrolKey contained "^\(Source\|Package\|Section\|Priority\|Maintainer\|Uploaders\|Build-Depends\|Build-Conflicts\|Build-Depends-Indep\|Build-Conflicts-Indep\|Standards-Version\|Pre-Depends\|Depends\|Recommends\|Suggests\|Provides\|Replaces\|Conflicts\|Essential\|Architecture\|Description\|Bugs\|Origin\|Enhances\|Homepage\|\(XS-\)\=Vcs-\(Browser\|Arch\|Bzr\|Cvs\|Darcs\|Git\|Hg\|Mtn\|Svn\)\|XC-Package-Type\|\%(XS-\)\=DM-Upload-Allowed\): *"

" Fields for which we do strict syntax checking
syn region debcontrolStrictField start="^Architecture" end="$" contains=debcontrolKey,debcontrolArchitecture,debcontrolSpace oneline
syn region debcontrolStrictField start="^\(Package\|Source\)" end="$" contains=debcontrolKey,debcontrolName oneline
syn region debcontrolStrictField start="^Priority" end="$" contains=debcontrolKey,debcontrolPriority oneline
syn region debcontrolStrictField start="^Section" end="$" contains=debcontrolKey,debcontrolSection oneline
syn region debcontrolStrictField start="^XC-Package-Type" end="$" contains=debcontrolKey,debcontrolPackageType oneline
syn region debcontrolStrictField start="^Homepage" end="$" contains=debcontrolKey,debcontrolHTTPUrl oneline keepend
syn region debcontrolStrictField start="^\%(XS-\)\?Vcs-\%(Browser\|Arch\|Bzr\|Darcs\|Hg\)" end="$" contains=debcontrolKey,debcontrolHTTPUrl oneline keepend
syn region debcontrolStrictField start="^\%(XS-\)\?Vcs-Svn" end="$" contains=debcontrolKey,debcontrolVcsSvn,debcontrolHTTPUrl oneline keepend
syn region debcontrolStrictField start="^\%(XS-\)\?Vcs-Cvs" end="$" contains=debcontrolKey,debcontrolVcsCvs oneline keepend
syn region debcontrolStrictField start="^\%(XS-\)\?Vcs-Git" end="$" contains=debcontrolKey,debcontrolVcsGit oneline keepend
syn region debcontrolStrictField start="^\%(XS-\)\?DM-Upload-Allowed" end="$" contains=debcontrolKey,debcontrolDmUpload oneline

" Catch-all for the other legal fields
syn region debcontrolField start="^\(Maintainer\|Standards-Version\|Essential\|Bugs\|Origin\|X\(S\|B\)-Python-Version\|XSBC-Original-Maintainer\|\(XS-\)\?Vcs-Mtn\):" end="$" contains=debcontrolKey,debcontrolVariable,debcontrolEmail oneline
syn region debcontrolMultiField start="^\(Build-\(Conflicts\|Depends\)\(-Indep\)\=\|\(Pre-\)\=Depends\|Recommends\|Suggests\|Provides\|Replaces\|Conflicts\|Enhances\|Uploaders\|Description\):" skip="^ " end="^$"me=s-1 end="^[^ ]"me=s-1 contains=debcontrolKey,debcontrolEmail,debcontrolVariable

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
