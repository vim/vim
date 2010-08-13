" Vim syntax file
" Language:     Perl 5
" Maintainer:   Andy Lester <andy@petdance.com>
" URL:          http://github.com/petdance/vim-perl/tree/master
" Last Change:  2010-08-10
" Contributors: Andy Lester <andy@petdance.com>
"               Hinrik Örn Sigurðsson <hinrik.sig@gmail.com>
"               Lukas Mai <l.mai.web.de>
"               Nick Hibma <nick@van-laarhoven.org>
"               Sonia Heimann <niania@netsurf.org>
"               and many others.
"
" Please download most recent version first before mailing
" any comments.
"
" The following parameters are available for tuning the
" perl syntax highlighting, with defaults given:
"
" unlet perl_include_pod
" unlet perl_no_scope_in_variables
" unlet perl_no_extended_vars
" unlet perl_string_as_statement
" unlet perl_no_sync_on_sub
" unlet perl_no_sync_on_global_var
" let perl_sync_dist = 100
" unlet perl_fold
" unlet perl_fold_blocks
" let perl_nofold_packages = 1
" let perl_nofold_subs = 1

if exists("b:current_syntax")
  finish
endif


" POD starts with ^=<word> and ends with ^=cut

if exists("perl_include_pod")
  " Include a while extra syntax file
  syn include @Pod syntax/pod.vim
  unlet b:current_syntax
  if exists("perl_fold")
    syn region perlPOD start="^=[a-z]" end="^=cut" contains=@Pod,@Spell,perlTodo keepend fold
    syn region perlPOD start="^=cut" end="^=cut" contains=perlTodo keepend fold
  else
    syn region perlPOD start="^=[a-z]" end="^=cut" contains=@Pod,@Spell,perlTodo keepend
    syn region perlPOD start="^=cut" end="^=cut" contains=perlTodo keepend
  endif
else
  " Use only the bare minimum of rules
  if exists("perl_fold")
    syn region perlPOD start="^=[a-z]" end="^=cut" fold
  else
    syn region perlPOD start="^=[a-z]" end="^=cut"
  endif
endif


syn cluster perlTop		contains=TOP
syn region  perlGenericBlock	matchgroup=perlGenericBlock start="{" end="}" contained transparent


" All keywords
"
syn match perlConditional		"\<\%(if\|elsif\|unless\|given\|when\|default\)\>"
syn match perlConditional		"\<else\>" nextgroup=perlElseIfError skipwhite skipnl skipempty
syn match perlRepeat			"\<\%(while\|for\%(each\)\=\|do\|until\|continue\)\>"
syn match perlOperator			"\<\%(defined\|undef\|eq\|ne\|[gl][et]\|cmp\|not\|and\|or\|xor\|not\|bless\|ref\|do\)\>"
syn match perlControl			"\<\%(BEGIN\|CHECK\|INIT\|END\|UNITCHECK\)\>"

syn match perlStatementStorage		"\<\%(my\|our\|local\|state\)\>"
syn match perlStatementControl		"\<\%(return\|last\|next\|redo\|goto\|break\)\>"
syn match perlStatementScalar		"\<\%(chom\=p\|chr\|crypt\|r\=index\|lc\%(first\)\=\|length\|ord\|pack\|sprintf\|substr\|uc\%(first\)\=\)\>"
syn match perlStatementRegexp		"\<\%(pos\|quotemeta\|split\|study\)\>"
syn match perlStatementNumeric		"\<\%(abs\|atan2\|cos\|exp\|hex\|int\|log\|oct\|rand\|sin\|sqrt\|srand\)\>"
syn match perlStatementList		"\<\%(splice\|unshift\|shift\|push\|pop\|join\|reverse\|grep\|map\|sort\|unpack\)\>"
syn match perlStatementHash		"\<\%(delete\|each\|exists\|keys\|values\)\>"
syn match perlStatementIOfunc		"\<\%(syscall\|dbmopen\|dbmclose\)\>"
syn match perlStatementFiledesc		"\<\%(binmode\|close\%(dir\)\=\|eof\|fileno\|getc\|lstat\|printf\=\|read\%(dir\|line\|pipe\)\|rewinddir\|say\|select\|stat\|tell\%(dir\)\=\|write\)\>" nextgroup=perlFiledescStatementNocomma skipwhite
syn match perlStatementFiledesc		"\<\%(fcntl\|flock\|ioctl\|open\%(dir\)\=\|read\|seek\%(dir\)\=\|sys\%(open\|read\|seek\|write\)\|truncate\)\>" nextgroup=perlFiledescStatementComma skipwhite
syn match perlStatementVector		"\<vec\>"
syn match perlStatementFiles		"\<\%(ch\%(dir\|mod\|own\|root\)\|glob\|link\|mkdir\|readlink\|rename\|rmdir\|symlink\|umask\|unlink\|utime\)\>"
syn match perlStatementFiles		"-[rwxoRWXOezsfdlpSbctugkTBMAC]\>"
syn match perlStatementFlow		"\<\%(caller\|die\|dump\|eval\|exit\|wantarray\)\>"
syn match perlStatementInclude		"\<require\>"
syn match perlStatementInclude		"\<\%(use\|no\)\s\+\%(\%(attributes\|attrs\|autouse\|parent\|base\|big\%(int\|num\|rat\)\|blib\|bytes\|charnames\|constant\|diagnostics\|encoding\%(::warnings\)\=\|feature\|fields\|filetest\|if\|integer\|less\|lib\|locale\|mro\|open\|ops\|overload\|re\|sigtrap\|sort\|strict\|subs\|threads\%(::shared\)\=\|utf8\|vars\|version\|vmsish\|warnings\%(::register\)\=\)\>\)\="
syn match perlStatementProc		"\<\%(alarm\|exec\|fork\|get\%(pgrp\|ppid\|priority\)\|kill\|pipe\|set\%(pgrp\|priority\)\|sleep\|system\|times\|wait\%(pid\)\=\)\>"
syn match perlStatementSocket		"\<\%(acept\|bind\|connect\|get\%(peername\|sock\%(name\|opt\)\)\|listen\|recv\|send\|setsockopt\|shutdown\|socket\%(pair\)\=\)\>"
syn match perlStatementIPC		"\<\%(msg\%(ctl\|get\|rcv\|snd\)\|sem\%(ctl\|get\|op\)\|shm\%(ctl\|get\|read\|write\)\)\>"
syn match perlStatementNetwork		"\<\%(\%(end\|[gs]et\)\%(host\|net\|proto\|serv\)ent\|get\%(\%(host\|net\)by\%(addr\|name\)\|protoby\%(name\|number\)\|servby\%(name\|port\)\)\)\>"
syn match perlStatementPword		"\<\%(get\%(pw\%(uid\|nam\)\|gr\%(gid\|nam\)\|login\)\)\|\%(end\|[gs]et\)\%(pw\|gr\)ent\>"
syn match perlStatementTime		"\<\%(gmtime\|localtime\|time\)\>"

syn match perlStatementMisc		"\<\%(warn\|formline\|reset\|scalar\|prototype\|lock\|tied\=\|untie\)\>"

syn keyword perlTodo			TODO TBD FIXME XXX NOTE contained

syn region perlStatementIndirObjWrap	matchgroup=perlStatementIndirObj start="\<\%(map\|grep\|sort\|print\|system\|exec\)\>\s*{" end="}" contains=@perlTop,perlGenericBlock

syn match perlLabel      "^\s*\h\w*\s*::\@!\%(\<v\d\+\s*:\)\@<!"

" Perl Identifiers.
"
" Should be cleaned up to better handle identifiers in particular situations
" (in hash keys for example)
"
" Plain identifiers: $foo, @foo, $#foo, %foo, &foo and dereferences $$foo, @$foo, etc.
" We do not process complex things such as @{${"foo"}}. Too complicated, and
" too slow. And what is after the -> is *not* considered as part of the
" variable - there again, too complicated and too slow.

" Special variables first ($^A, ...) and ($|, $', ...)
syn match  perlVarPlain		 "$^[ACDEFHILMNOPRSTVWX]\="
syn match  perlVarPlain		 "$[\\\"\[\]'&`+*.,;=%~!?@#$<>(-]"
syn match  perlVarPlain		 "%+"
syn match  perlVarPlain		 "$\%(0\|[1-9]\d*\)"
" Same as above, but avoids confusion in $::foo (equivalent to $main::foo)
syn match  perlVarPlain		 "$::\@!"
" These variables are not recognized within matches.
syn match  perlVarNotInMatches	 "$[|)]"
" This variable is not recognized within matches delimited by m//.
syn match  perlVarSlash		 "$/"

" And plain identifiers
syn match  perlPackageRef	 "[$@#%*&]\%(\%(::\|'\)\=\I\i*\%(\%(::\|'\)\I\i*\)*\)\=\%(::\|'\)\I"ms=s+1,me=e-1 contained

" To not highlight packages in variables as a scope reference - i.e. in
" $pack::var, pack:: is a scope, just set "perl_no_scope_in_variables"
" If you don't want complex things like @{${"foo"}} to be processed,
" just set the variable "perl_no_extended_vars"...

if !exists("perl_no_scope_in_variables")
  syn match  perlVarPlain       "\%([@$]\|\$#\)\$*\%(\I\i*\)\=\%(\%(::\|'\)\I\i*\)*\%(::\|\i\@<=\)" contains=perlPackageRef nextgroup=perlVarMember,perlVarSimpleMember,perlMethod
  syn match  perlVarPlain2                   "%\$*\%(\I\i*\)\=\%(\%(::\|'\)\I\i*\)*\%(::\|\i\@<=\)" contains=perlPackageRef
  syn match  perlFunctionName                "&\$*\%(\I\i*\)\=\%(\%(::\|'\)\I\i*\)*\%(::\|\i\@<=\)" contains=perlPackageRef nextgroup=perlVarMember,perlVarSimpleMember,perlMethod
else
  syn match  perlVarPlain       "\%([@$]\|\$#\)\$*\%(\I\i*\)\=\%(\%(::\|'\)\I\i*\)*\%(::\|\i\@<=\)" nextgroup=perlVarMember,perlVarSimpleMember,perlMethod
  syn match  perlVarPlain2                   "%\$*\%(\I\i*\)\=\%(\%(::\|'\)\I\i*\)*\%(::\|\i\@<=\)"
  syn match  perlFunctionName                "&\$*\%(\I\i*\)\=\%(\%(::\|'\)\I\i*\)*\%(::\|\i\@<=\)" nextgroup=perlVarMember,perlVarSimpleMember,perlMethod
endif

if !exists("perl_no_extended_vars")
  syn cluster perlExpr		contains=perlStatementIndirObjWrap,perlStatementScalar,perlStatementRegexp,perlStatementNumeric,perlStatementList,perlStatementHash,perlStatementFiles,perlStatementTime,perlStatementMisc,perlVarPlain,perlVarPlain2,perlVarNotInMatches,perlVarSlash,perlVarBlock,perlVarBlock2,perlShellCommand,perlFloat,perlNumber,perlStringUnexpanded,perlString,perlQQ,perlArrow,perlGenericBlock
  syn region perlArrow		matchgroup=perlArrow start="->\s*(" end=")" contains=@perlExpr nextgroup=perlVarMember,perlVarSimpleMember,perlMethod contained
  syn region perlArrow		matchgroup=perlArrow start="->\s*\[" end="\]" contains=@perlExpr nextgroup=perlVarMember,perlVarSimpleMember,perlMethod contained
  syn region perlArrow		matchgroup=perlArrow start="->\s*{" end="}" contains=@perlExpr nextgroup=perlVarMember,perlVarSimpleMember,perlMethod contained
  syn match  perlArrow		"->\s*{\s*\I\i*\s*}" contains=perlVarSimpleMemberName nextgroup=perlVarMember,perlVarSimpleMember,perlMethod contained
  syn region perlArrow		matchgroup=perlArrow start="->\s*\$*\I\i*\s*(" end=")" contains=@perlExpr nextgroup=perlVarMember,perlVarSimpleMember,perlMethod contained
  syn region perlVarBlock	matchgroup=perlVarPlain start="\%($#\|[$@]\)\$*{" skip="\\}" end="}" contains=@perlExpr nextgroup=perlVarMember,perlVarSimpleMember,perlMethod
  syn region perlVarBlock2	matchgroup=perlVarPlain start="[%&*]\$*{" skip="\\}" end="}" contains=@perlExpr nextgroup=perlVarMember,perlVarSimpleMember,perlMethod
  syn match  perlVarPlain2	"[%&*]\$*{\I\i*}" nextgroup=perlVarMember,perlVarSimpleMember,perlMethod
  syn match  perlVarPlain	"\%(\$#\|[@$]\)\$*{\I\i*}" nextgroup=perlVarMember,perlVarSimpleMember,perlMethod
  syn region perlVarMember	matchgroup=perlVarPlain start="\%(->\)\={" skip="\\}" end="}" contained contains=@perlExpr nextgroup=perlVarMember,perlVarSimpleMember,perlMethod
  syn match  perlVarSimpleMember	"\%(->\)\={\s*\I\i*\s*}" nextgroup=perlVarMember,perlVarSimpleMember,perlMethod contains=perlVarSimpleMemberName contained
  syn match  perlVarSimpleMemberName	"\I\i*" contained
  syn region perlVarMember	matchgroup=perlVarPlain start="\%(->\)\=\[" skip="\\]" end="]" contained contains=@perlExpr nextgroup=perlVarMember,perlVarSimpleMember,perlMethod
  syn match perlPackageConst	"__PACKAGE__" nextgroup=perlMethod
  syn match  perlMethod		"->\$*\I\i*" contained nextgroup=perlVarSimpleMember,perlVarMember,perlMethod
endif

" File Descriptors
syn match  perlFiledescRead	"<\h\w*>"

syn match  perlFiledescStatementComma	"(\=\s*\u\w*\s*,"me=e-1 transparent contained contains=perlFiledescStatement
syn match  perlFiledescStatementNocomma "(\=\s*\u\w*\s*[^, \t]"me=e-1 transparent contained contains=perlFiledescStatement

syn match  perlFiledescStatement	"\u\w*" contained

" Special characters in strings and matches
syn match  perlSpecialString	"\\\%(\o\{1,3}\|x\%({\x\+}\|\x\{1,2}\)\|c.\|[^cx]\)" contained extend
syn match  perlSpecialStringU2	"\\." extend contained transparent contains=NONE
syn match  perlSpecialStringU	"\\\\" contained
syn match  perlSpecialMatch	"\\[1-9]" contained extend
syn match  perlSpecialMatch	"\\g\%(\d\+\|{\%(-\=\d\+\|\h\w*\)}\)" contained
syn match  perlSpecialMatch	"\\k\%(<\h\w*>\|'\h\w*'\)" contained
syn match  perlSpecialMatch	"{\d\+\%(,\%(\d\+\)\=\)\=}" contained
syn match  perlSpecialMatch	"\[[]-]\=[^\[\]]*[]-]\=\]" contained
syn match  perlSpecialMatch	"[+*()?.]" contained
syn match  perlSpecialMatch	"(?[#:=!]" contained
syn match  perlSpecialMatch	"(?[impsx]*\%(-[imsx]\+\)\=)" contained
syn match  perlSpecialMatch	"(?\%([-+]\=\d\+\|R\))" contained
syn match  perlSpecialMatch	"(?\%(&\|P[>=]\)\h\w*)" contained
syn match  perlSpecialMatch	"(\*\%(\%(PRUNE\|SKIP\|THEN\)\%(:[^)]*\)\=\|\%(MARK\|\):[^)]*\|COMMIT\|F\%(AIL\)\=\|ACCEPT\))" contained

" Possible errors
"
" Highlight lines with only whitespace (only in blank delimited here documents) as errors
syn match  perlNotEmptyLine	"^\s\+$" contained
" Highlight '} else if (...) {', it should be '} else { if (...) { ' or
" '} elsif (...) {'.
syn match perlElseIfError	"\s\+if" contained
syn keyword perlElseIfError	elseif

" Variable interpolation
"
" These items are interpolated inside "" strings and similar constructs.
syn cluster perlInterpDQ	contains=perlSpecialString,perlVarPlain,perlVarNotInMatches,perlVarSlash,perlVarBlock
" These items are interpolated inside '' strings and similar constructs.
syn cluster perlInterpSQ	contains=perlSpecialStringU,perlSpecialStringU2
" These items are interpolated inside m// matches and s/// substitutions.
syn cluster perlInterpSlash	contains=perlSpecialString,perlSpecialMatch,perlVarPlain,perlVarBlock
" These items are interpolated inside m## matches and s### substitutions.
syn cluster perlInterpMatch	contains=@perlInterpSlash,perlVarSlash

" Shell commands
syn region  perlShellCommand	matchgroup=perlMatchStartEnd start="`" end="`" contains=@perlInterpDQ keepend

" Constants
"
" Numbers
syn match  perlNumber	"\<\%(0\%(x\x[[:xdigit:]_]*\|b[01][01_]*\|\o[0-7_]*\|\)\|[1-9][[:digit:]_]*\)\>"
syn match  perlFloat	"\<\d[[:digit:]_]*[eE][\-+]\=\d\+"
syn match  perlFloat	"\<\d[[:digit:]_]*\.[[:digit:]_]*\%([eE][\-+]\=\d\+\)\="
syn match  perlFloat	"\.[[:digit:]_]\+\%([eE][\-+]\=\d\+\)\="

syn match  perlString	"\<\%(v\d\+\%(\.\d\+\)*\|\d\+\%(\.\d\+\)\{2,}\)\>" contains=perlVStringV
syn match  perlVStringV	"\<v" contained


syn region perlParensSQ		start=+(+ end=+)+ extend contained transparent contains=perlParensSQ,@perlInterpSQ keepend
syn region perlBracketsSQ	start=+\[+ end=+\]+ extend contained transparent contains=perlBracketsSQ,@perlInterpSQ keepend
syn region perlBracesSQ		start=+{+ end=+}+ extend contained transparent contains=perlBracesSQ,@perlInterpSQ keepend
syn region perlAnglesSQ		start=+<+ end=+>+ extend contained transparent contains=perlAnglesSQ,@perlInterpSQ keepend

syn region perlParensDQ		start=+(+ end=+)+ extend contained transparent contains=perlParensDQ,@perlInterpDQ keepend
syn region perlBracketsDQ	start=+\[+ end=+\]+ extend contained transparent contains=perlBracketsDQ,@perlInterpDQ keepend
syn region perlBracesDQ		start=+{+ end=+}+ extend contained transparent contains=perlBracesDQ,@perlInterpDQ keepend
syn region perlAnglesDQ		start=+<+ end=+>+ extend contained transparent contains=perlAnglesDQ,@perlInterpDQ keepend


" Simple version of searches and matches
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!m\>\s*\z([^[:space:]'([{<#]\)+ end=+\z1[cgimopsx]*+ contains=@perlInterpMatch keepend
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!m#+ end=+#[cgimopsx]*+ contains=@perlInterpMatch keepend
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!m\s*'+ end=+'[cgimopsx]*+ contains=@perlInterpSQ keepend
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!m\s*/+ end=+/[cgimopsx]*+ contains=@perlInterpSlash keepend
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!m\s*(+ end=+)[cgimopsx]*+ contains=@perlInterpMatch,perlParensDQ keepend

" A special case for m{}, m<> and m[] which allows for comments and extra whitespace in the pattern
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!m\s*{+ end=+}[cgimopsx]*+ contains=@perlInterpMatch,perlComment,perlBracesDQ keepend
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!m\s*<+ end=+>[cgimopsx]*+ contains=@perlInterpMatch,perlAnglesDQ keepend
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!m\s*\[+ end=+\][cgimopsx]*+ contains=@perlInterpMatch,perlComment,perlBracketsDQ keepend

" Below some hacks to recognise the // variant. This is virtually impossible to catch in all
" cases as the / is used in so many other ways, but these should be the most obvious ones.
syn region perlMatch	matchgroup=perlMatchStartEnd start="\%([$@%&*]\@<!\%(\<split\|\<while\|\<if\|\<unless\|\.\.\|[-+*!~(\[{=]\)\s*\)\@<=/\%(/=\)\@!" start=+^/\%(/=\)\@!+ start=+\s\@<=/\%(/=\)\@![^[:space:][:digit:]$@%=]\@=\%(/\_s*\%([([{$@%&*[:digit:]"'`]\|\_s\w\|[[:upper:]_abd-fhjklnqrt-wyz]\)\)\@!+ skip=+\\/+ end=+/[cgimopsx]*+ contains=@perlInterpSlash


" Substitutions
" perlMatch is the first part, perlSubstitution* is the substitution part
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!s\>\s*\z([^[:space:]'([{<#]\)+ end=+\z1+me=e-1 contains=@perlInterpMatch nextgroup=perlSubstitutionGQQ keepend
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!s\s*'+  end=+'+me=e-1 contains=@perlInterpSQ nextgroup=perlSubstitutionSQ keepend
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!s\s*/+  end=+/+me=e-1 contains=@perlInterpSlash nextgroup=perlSubstitutionGQQ keepend
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!s#+  end=+#+me=e-1 contains=@perlInterpMatch nextgroup=perlSubstitutionGQQ keepend
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!s\s*(+ end=+)+ contains=@perlInterpMatch,perlParensDQ nextgroup=perlSubstitutionGQQ skipwhite skipempty skipnl keepend
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!s\s*<+ end=+>+ contains=@perlInterpMatch,perlAnglesDQ nextgroup=perlSubstitutionGQQ skipwhite skipempty skipnl keepend
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!s\s*\[+ end=+\]+ contains=@perlInterpMatch,perlBracketsDQ nextgroup=perlSubstitutionGQQ skipwhite skipempty skipnl keepend
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!s\s*{+ end=+}+ contains=@perlInterpMatch,perlBracesDQ nextgroup=perlSubstitutionGQQ skipwhite skipempty skipnl keepend
syn region perlSubstitutionGQQ		matchgroup=perlMatchStartEnd start=+\z([^[:space:]'([{<]\)+ end=+\z1[ecgimopsx]*+ keepend contained contains=@perlInterpDQ
syn region perlSubstitutionGQQ		matchgroup=perlMatchStartEnd start=+(+ end=+)[ecgimopsx]*+ contained contains=@perlInterpDQ,perlParensDQ keepend
syn region perlSubstitutionGQQ		matchgroup=perlMatchStartEnd start=+\[+ end=+\][ecgimopsx]*+ contained contains=@perlInterpDQ,perlBracketsDQ keepend
syn region perlSubstitutionGQQ		matchgroup=perlMatchStartEnd start=+{+ end=+}[ecgimopsx]*+ contained contains=@perlInterpDQ,perlBracesDQ keepend
syn region perlSubstitutionGQQ		matchgroup=perlMatchStartEnd start=+<+ end=+>[ecgimopsx]*+ contained contains=@perlInterpDQ,perlAnglesDQ keepend
syn region perlSubstitutionSQ		matchgroup=perlMatchStartEnd start=+'+  end=+'[ecgimopsx]*+ contained contains=@perlInterpSQ keepend 

" Translations
" perlMatch is the first part, perlTranslation* is the second, translator part.
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!\%(tr\|y\)\>\s*\z([^[:space:]([{<#]\)+ end=+\z1+me=e-1 contains=@perlInterpSQ nextgroup=perlTranslationGQ
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!\%(tr\|y\)#+ end=+#+me=e-1 contains=@perlInterpSQ nextgroup=perlTranslationGQ
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!\%(tr\|y\)\s*\[+ end=+\]+ contains=@perlInterpSQ,perlBracketsSQ nextgroup=perlTranslationGQ skipwhite skipempty skipnl
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!\%(tr\|y\)\s*(+ end=+)+ contains=@perlInterpSQ,perlParensSQ nextgroup=perlTranslationGQ skipwhite skipempty skipnl
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!\%(tr\|y\)\s*<+ end=+>+ contains=@perlInterpSQ,perlAnglesSQ nextgroup=perlTranslationGQ skipwhite skipempty skipnl
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\%(::\|'\|->\)\@<!\%(tr\|y\)\s*{+ end=+}+ contains=@perlInterpSQ,perlBracesSQ nextgroup=perlTranslationGQ skipwhite skipempty skipnl
syn region perlTranslationGQ		matchgroup=perlMatchStartEnd start=+\z([^[:space:]([{<]\)+ end=+\z1[cds]*+ contained
syn region perlTranslationGQ		matchgroup=perlMatchStartEnd start=+(+ end=+)[cds]*+ contains=perlParensSQ contained
syn region perlTranslationGQ		matchgroup=perlMatchStartEnd start=+\[+ end=+\][cds]*+ contains=perlBracketsSQ contained
syn region perlTranslationGQ		matchgroup=perlMatchStartEnd start=+{+ end=+}[cds]*+ contains=perlBracesSQ contained
syn region perlTranslationGQ		matchgroup=perlMatchStartEnd start=+<+ end=+>[cds]*+ contains=perlAnglesSQ contained


" Strings and q, qq, qw and qr expressions

syn region perlStringUnexpanded	matchgroup=perlStringStartEnd start="'" end="'" contains=@perlInterpSQ keepend
syn region perlString		matchgroup=perlStringStartEnd start=+"+  end=+"+ contains=@perlInterpDQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!q\>\s*\z([^[:space:]#([{<]\)+ end=+\z1+ contains=@perlInterpSQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!q#+ end=+#+ contains=@perlInterpSQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!q\s*(+ end=+)+ contains=@perlInterpSQ,perlParensSQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!q\s*\[+ end=+\]+ contains=@perlInterpSQ,perlBracketsSQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!q\s*{+ end=+}+ contains=@perlInterpSQ,perlBracesSQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!q\s*<+ end=+>+ contains=@perlInterpSQ,perlAnglesSQ keepend

syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!q[qx]\>\s*\z([^[:space:]#([{<]\)+ end=+\z1+ contains=@perlInterpDQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!q[qx]#+ end=+#+ contains=@perlInterpDQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!q[qx]\s*(+ end=+)+ contains=@perlInterpDQ,perlParensDQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!q[qx]\s*\[+ end=+\]+ contains=@perlInterpDQ,perlBracketsDQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!q[qx]\s*{+ end=+}+ contains=@perlInterpDQ,perlBracesDQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!q[qx]\s*<+ end=+>+ contains=@perlInterpDQ,perlAnglesDQ keepend

syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qw\s*\z([^[:space:]#([{<]\)+  end=+\z1+ contains=@perlInterpSQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qw#+  end=+#+ contains=@perlInterpSQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qw\s*(+  end=+)+ contains=@perlInterpSQ,perlParensSQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qw\s*\[+  end=+\]+ contains=@perlInterpSQ,perlBracketsSQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qw\s*{+  end=+}+ contains=@perlInterpSQ,perlBracesSQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qw\s*<+  end=+>+ contains=@perlInterpSQ,perlAnglesSQ keepend

syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qr\>\s*\z([^[:space:]#([{<'/]\)+  end=+\z1[imosx]*+ contains=@perlInterpMatch keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qr\s*/+  end=+/[imosx]*+ contains=@perlInterpSlash keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qr#+  end=+#[imosx]*+ contains=@perlInterpMatch keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qr\s*'+  end=+'[imosx]*+ contains=@perlInterpSQ keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qr\s*(+  end=+)[imosx]*+ contains=@perlInterpMatch,perlParensDQ keepend

" A special case for qr{}, qr<> and qr[] which allows for comments and extra whitespace in the pattern
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qr\s*{+  end=+}[imosx]*+ contains=@perlInterpMatch,perlBracesDQ,perlComment keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qr\s*<+  end=+>[imosx]*+ contains=@perlInterpMatch,perlAnglesDQ,perlComment keepend
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<\%(::\|'\|->\)\@<!qr\s*\[+  end=+\][imosx]*+ contains=@perlInterpMatch,perlBracketsDQ,perlComment keepend

" Constructs such as print <<EOF [...] EOF, 'here' documents
"
" XXX Any statements after the identifier are in perlString colour (i.e.
" 'if $a' in 'print <<EOF if $a'). This is almost impossible to get right it
" seems due to the 'auto-extending nature' of regions.
if exists("perl_fold")
  syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\z(\I\i*\).*+    end=+^\z1$+ contains=@perlInterpDQ fold
  syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*"\z([^\\"]*\%(\\.[^\\"]*\)*\)"+ end=+^\z1$+ contains=@perlInterpDQ fold
  syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*'\z([^\\']*\%(\\.[^\\']*\)*\)'+ end=+^\z1$+ contains=@perlInterpSQ fold
  syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*""+           end=+^$+    contains=@perlInterpDQ,perlNotEmptyLine fold
  syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*''+           end=+^$+    contains=@perlInterpSQ,perlNotEmptyLine fold
  syn region perlAutoload	matchgroup=perlStringStartEnd start=+<<\s*\(['"]\=\)\z(END_\%(SUB\|OF_FUNC\|OF_AUTOLOAD\)\)\1+ end=+^\z1$+ contains=ALL fold
else
  syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\z(\I\i*\)+    end=+^\z1$+ contains=@perlInterpDQ
  syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*"\z([^\\"]*\%(\\.[^\\"]*\)*\)"+ end=+^\z1$+ contains=@perlInterpDQ
  syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*'\z([^\\']*\%(\\.[^\\']*\)*\)'+ end=+^\z1$+ contains=@perlInterpSQ
  syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*""+           end=+^$+    contains=@perlInterpDQ,perlNotEmptyLine
  syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*''+           end=+^$+    contains=@perlInterpSQ,perlNotEmptyLine
  syn region perlAutoload	matchgroup=perlStringStartEnd start=+<<\s*\(['"]\=\)\z(END_\%(SUB\|OF_FUNC\|OF_AUTOLOAD\)\)\1+ end=+^\z1$+ contains=ALL
endif


" Class declarations
"
syn match   perlPackageDecl		"\<package\s\+\%(\h\|::\)\%(\w\|::\)*" contains=perlStatementPackage
syn keyword perlStatementPackage	package contained

" Functions
"       sub [name] [(prototype)] {
"
syn match perlSubError "[^[:space:];{#]" contained
if v:version == 701 && !has('patch221')  " XXX I hope that's the right one
    syn match perlSubAttributes ":" contained
else
    syn match perlSubAttributesCont "\h\w*\_s*\%(:\_s*\)\=" nextgroup=@perlSubAttrMaybe contained
    syn region perlSubAttributesCont matchgroup=perlSubAttributesCont start="\h\w*(" end=")\_s*\%(:\_s*\)\=" nextgroup=@perlSubAttrMaybe contained contains=@perlInterpSQ,perlParensSQ
    syn cluster perlSubAttrMaybe contains=perlSubAttributesCont,perlSubError
    syn match perlSubAttributes "" contained nextgroup=perlSubError
    syn match perlSubAttributes ":\_s*" contained nextgroup=@perlSubAttrMaybe
endif
syn match perlSubPrototypeError "(\%(\_s*\%(\%(\\\%([$@%&*]\|\[[$@%&*]\+\]\)\|[$&*]\|[@%]\%(\_s*)\)\@=\|;\%(\_s*[)$@%&*\\]\)\@=\|_\%(\_s*[);]\)\@=\)\_s*\)*\)\@>\zs\_[^)]\+" contained
syn match perlSubPrototype +(\_[^)]*)\_s*\|+ nextgroup=perlSubAttributes contained contains=perlSubPrototypeError
syn match perlSubName +\%(\h\|::\|'\w\)\%(\w\|::\|'\w\)*\_s*\|+ contained nextgroup=perlSubPrototype

syn match perlFunction +\<sub\>\_s*+ nextgroup=perlSubName

if !exists("perl_no_scope_in_variables")
   syn match  perlFunctionPRef	"\h\w*::" contained
   syn match  perlFunctionName	"\h\w*[^:]" contained
else
   syn match  perlFunctionName	"\h[[:alnum:]_:]*" contained
endif

" The => operator forces a bareword to the left of it to be interpreted as
" a string
syn match  perlString "\I\@<!-\?\I\i*\%(\s*=>\)\@="

" All other # are comments, except ^#!
syn match  perlComment		"#.*" contains=perlTodo,@Spell
syn match  perlSharpBang	"^#!.*"

" Formats
syn region perlFormat		matchgroup=perlStatementIOFunc start="^\s*\<format\s\+\k\+\s*=\s*$"rs=s+6 end="^\s*\.\s*$" contains=perlFormatName,perlFormatField,perlVarPlain,perlVarPlain2
syn match  perlFormatName	"format\s\+\k\+\s*="lc=7,me=e-1 contained
syn match  perlFormatField	"[@^][|<>~]\+\%(\.\.\.\)\=" contained
syn match  perlFormatField	"[@^]#[#.]*" contained
syn match  perlFormatField	"@\*" contained
syn match  perlFormatField	"@[^A-Za-z_|<>~#*]"me=e-1 contained
syn match  perlFormatField	"@$" contained

" __END__ and __DATA__ clauses
if exists("perl_fold")
  syntax region perlDATA		start="^__\%(DATA\|END\)__$" skip="." end="." contains=perlPOD,@perlDATA fold
else
  syntax region perlDATA		start="^__\%(DATA\|END\)__$" skip="." end="." contains=perlPOD,@perlDATA
endif

"
" Folding

if exists("perl_fold")
  " Note: this bit must come before the actual highlighting of the "package"
  " keyword, otherwise this will screw up Pod lines that match /^package/
  if !exists("perl_nofold_packages")
    syn region perlPackageFold start="^package \S\+;\s*\%(#.*\)\=$" end="^1;\=\s*\%(#.*\)\=$" end="\n\+package"me=s-1 transparent fold keepend
  endif
  if !exists("perl_nofold_subs")
    syn region perlSubFold     start="^\z(\s*\)\<sub\>.*[^};]$" end="^\z1}\s*\%(#.*\)\=$" transparent fold keepend
    syn region perlSubFold start="^\z(\s*\)\<\%(BEGIN\|END\|CHECK\|INIT\|UNITCHECK\)\>.*[^};]$" end="^\z1}\s*$" transparent fold keepend
  endif

  if exists("perl_fold_blocks")
    syn region perlBlockFold start="^\z(\s*\)\%(if\|elsif\|unless\|for\|while\|until\|given\)\s*(.*)\%(\s*{\)\=\s*\%(#.*\)\=$" start="^\z(\s*\)foreach\s*\%(\%(my\|our\)\=\s*\S\+\s*\)\=(.*)\%(\s*{\)\=\s*\%(#.*\)\=$" end="^\z1}\s*;\=\%(#.*\)\=$" transparent fold keepend
    syn region perlBlockFold start="^\z(\s*\)\%(do\|else\)\%(\s*{\)\=\s*\%(#.*\)\=$" end="^\z1}\s*while" end="^\z1}\s*;\=\%(#.*\)\=$" transparent fold keepend
  endif

  setlocal foldmethod=syntax
  syn sync fromstart
else
  " fromstart above seems to set minlines even if perl_fold is not set.
  syn sync minlines=0
endif

command -nargs=+ HiLink hi def link <args>

" The default highlighting.
HiLink perlSharpBang		PreProc
HiLink perlControl		PreProc
HiLink perlInclude		Include
HiLink perlSpecial		Special
HiLink perlString		String
HiLink perlCharacter		Character
HiLink perlNumber		Number
HiLink perlFloat		Float
HiLink perlType			Type
HiLink perlIdentifier		Identifier
HiLink perlLabel		Label
HiLink perlStatement		Statement
HiLink perlConditional		Conditional
HiLink perlRepeat		Repeat
HiLink perlOperator		Operator
HiLink perlFunction		Keyword
HiLink perlSubName		Function
HiLink perlSubPrototype		Type
HiLink perlSubAttributes	PreProc
HiLink perlSubAttributesCont	perlSubAttributes
HiLink perlComment		Comment
HiLink perlTodo			Todo
if exists("perl_string_as_statement")
  HiLink perlStringStartEnd	perlStatement
else
  HiLink perlStringStartEnd	perlString
endif
HiLink perlVStringV		perlStringStartEnd
HiLink perlList			perlStatement
HiLink perlMisc			perlStatement
HiLink perlVarPlain		perlIdentifier
HiLink perlVarPlain2		perlIdentifier
HiLink perlArrow		perlIdentifier
HiLink perlFiledescRead		perlIdentifier
HiLink perlFiledescStatement	perlIdentifier
HiLink perlVarSimpleMember	perlIdentifier
HiLink perlVarSimpleMemberName 	perlString
HiLink perlVarNotInMatches	perlIdentifier
HiLink perlVarSlash		perlIdentifier
HiLink perlQQ			perlString
HiLink perlHereDoc		perlString
HiLink perlStringUnexpanded	perlString
HiLink perlSubstitutionSQ	perlString
HiLink perlSubstitutionGQQ	perlString
HiLink perlTranslationGQ	perlString
HiLink perlMatch		perlString
HiLink perlMatchStartEnd	perlStatement
HiLink perlFormatName		perlIdentifier
HiLink perlFormatField		perlString
HiLink perlPackageDecl		perlType
HiLink perlStorageClass		perlType
HiLink perlPackageRef		perlType
HiLink perlStatementPackage	perlStatement
HiLink perlStatementStorage	perlStatement
HiLink perlStatementControl	perlStatement
HiLink perlStatementScalar	perlStatement
HiLink perlStatementRegexp	perlStatement
HiLink perlStatementNumeric	perlStatement
HiLink perlStatementList	perlStatement
HiLink perlStatementHash	perlStatement
HiLink perlStatementIOfunc	perlStatement
HiLink perlStatementFiledesc	perlStatement
HiLink perlStatementVector	perlStatement
HiLink perlStatementFiles	perlStatement
HiLink perlStatementFlow	perlStatement
HiLink perlStatementInclude	perlStatement
HiLink perlStatementProc	perlStatement
HiLink perlStatementSocket	perlStatement
HiLink perlStatementIPC		perlStatement
HiLink perlStatementNetwork	perlStatement
HiLink perlStatementPword	perlStatement
HiLink perlStatementTime	perlStatement
HiLink perlStatementMisc	perlStatement
HiLink perlStatementIndirObj	perlStatement
HiLink perlFunctionName		perlIdentifier
HiLink perlMethod		perlIdentifier
HiLink perlFunctionPRef		perlType
HiLink perlPOD			perlComment
HiLink perlShellCommand		perlString
HiLink perlSpecialAscii		perlSpecial
HiLink perlSpecialDollar	perlSpecial
HiLink perlSpecialString	perlSpecial
HiLink perlSpecialStringU	perlSpecial
HiLink perlSpecialMatch		perlSpecial
HiLink perlDATA			perlComment

" Possible errors
HiLink perlNotEmptyLine		Error
HiLink perlElseIfError		Error
HiLink perlSubPrototypeError	Error
HiLink perlSubError		Error

delcommand HiLink

" Syncing to speed up processing
"
if !exists("perl_no_sync_on_sub")
  syn sync match perlSync	grouphere NONE "^\s*\<package\s"
  syn sync match perlSync	grouphere NONE "^\s*\<sub\>"
  syn sync match perlSync	grouphere NONE "^}"
endif

if !exists("perl_no_sync_on_global_var")
  syn sync match perlSync	grouphere NONE "^$\I[[:alnum:]_:]+\s*=\s*{"
  syn sync match perlSync	grouphere NONE "^[@%]\I[[:alnum:]_:]+\s*=\s*("
endif

if exists("perl_sync_dist")
  execute "syn sync maxlines=" . perl_sync_dist
else
  syn sync maxlines=100
endif

syn sync match perlSyncPOD	grouphere perlPOD "^=pod"
syn sync match perlSyncPOD	grouphere perlPOD "^=head"
syn sync match perlSyncPOD	grouphere perlPOD "^=item"
syn sync match perlSyncPOD	grouphere NONE "^=cut"

let b:current_syntax = "perl"

" XXX Change to sts=4:sw=4
" vim:ts=8:sts=2:sw=2:expandtab:ft=vim
