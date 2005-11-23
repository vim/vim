" Vim syntax file
" Language:	Perl
" Maintainer:	Nick Hibma <n_hibma@van-laarhoven.org>
" Last Change:	2005 October 18
" Location:	http://www.van-laarhoven.org/vim/syntax/perl.vim
"
" Please download most recent version first before mailing
" any comments.
" See also the file perl.vim.regression.pl to check whether your
" modifications work in the most odd cases
" http://www.van-laarhoven.org/vim/syntax/perl.vim.regression.pl
"
" Original version: Sonia Heimann <niania@netsurf.org>
" Thanks to many people for their contribution.

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

" Remove any old syntax stuff that was loaded (5.x) or quit when a syntax file
" was already loaded (6.x).
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Unset perl_fold if it set but vim doesn't support it.
if version < 600 && exists("perl_fold")
  unlet perl_fold
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
    syn region perlPOD start="^=[a-z]" end="^=cut" contains=@Spell fold
  else
    syn region perlPOD start="^=[a-z]" end="^=cut" contains=@Spell
  endif
endif


" All keywords
"
if exists("perl_fold") && exists("perl_fold_blocks")
  syn match perlConditional		"\<if\>"
  syn match perlConditional		"\<elsif\>"
  syn match perlConditional		"\<unless\>"
  syn match perlConditional		"\<else\>" nextgroup=perlElseIfError skipwhite skipnl skipempty
else
  syn keyword perlConditional		if elsif unless
  syn keyword perlConditional		else nextgroup=perlElseIfError skipwhite skipnl skipempty
endif
syn keyword perlConditional		switch eq ne gt lt ge le cmp not and or xor err
if exists("perl_fold") && exists("perl_fold_blocks")
  syn match perlRepeat			"\<while\>"
  syn match perlRepeat			"\<for\>"
  syn match perlRepeat			"\<foreach\>"
  syn match perlRepeat			"\<do\>"
  syn match perlRepeat			"\<until\>"
  syn match perlRepeat			"\<continue\>"
else
  syn keyword perlRepeat		while for foreach do until continue
endif
syn keyword perlOperator		defined undef and or not bless ref
if exists("perl_fold")
  " if BEGIN/END is a keyword the perlBEGINENDFold does not work
  syn match perlControl			"\<BEGIN\|END\|CHECK\|INIT\>" contained
else
  syn keyword perlControl		BEGIN END CHECK INIT
endif

syn keyword perlStatementStorage	my local our
syn keyword perlStatementControl	goto return last next redo
syn keyword perlStatementScalar		chomp chop chr crypt index lc lcfirst length ord pack reverse rindex sprintf substr uc ucfirst
syn keyword perlStatementRegexp		pos quotemeta split study
syn keyword perlStatementNumeric	abs atan2 cos exp hex int log oct rand sin sqrt srand
syn keyword perlStatementList		splice unshift shift push pop split join reverse grep map sort unpack
syn keyword perlStatementHash		each exists keys values tie tied untie
syn keyword perlStatementIOfunc		carp confess croak dbmclose dbmopen die syscall
syn keyword perlStatementFiledesc	binmode close closedir eof fileno getc lstat print printf readdir readline readpipe rewinddir select stat tell telldir write nextgroup=perlFiledescStatementNocomma skipwhite
syn keyword perlStatementFiledesc	fcntl flock ioctl open opendir read seek seekdir sysopen sysread sysseek syswrite truncate nextgroup=perlFiledescStatementComma skipwhite
syn keyword perlStatementVector		pack vec
syn keyword perlStatementFiles		chdir chmod chown chroot glob link mkdir readlink rename rmdir symlink umask unlink utime
syn match   perlStatementFiles		"-[rwxoRWXOezsfdlpSbctugkTBMAC]\>"
syn keyword perlStatementFlow		caller die dump eval exit wantarray
syn keyword perlStatementInclude	require
syn match   perlStatementInclude	"\<\(use\|no\)\s\+\(\(attributes\|autouse\|base\|big\(int\|num\|rat\)\|blib\|bytes\|charnames\|constant\|diagnostics\|encoding\|fields\|filetest\|if\|integer\|less\|lib\|locale\|open\|ops\|overload\|re\|sigtrap\|sort\|strict\|subs\|threads\(::shared\)\=\|utf8\|vars\|vmsish\|warnings\(::register\)\=\)\>\)\="

syn keyword perlStatementProc		alarm exec fork getpgrp getppid getpriority kill pipe setpgrp setpriority sleep system times wait waitpid
syn keyword perlStatementSocket		accept bind connect getpeername getsockname getsockopt listen recv send setsockopt shutdown socket socketpair
syn keyword perlStatementIPC		msgctl msgget msgrcv msgsnd semctl semget semop shmctl shmget shmread shmwrite
syn keyword perlStatementNetwork	endhostent endnetent endprotoent endservent gethostbyaddr gethostbyname gethostent getnetbyaddr getnetbyname getnetent getprotobyname getprotobynumber getprotoent getservbyname getservbyport getservent sethostent setnetent setprotoent setservent
syn keyword perlStatementPword		getpwuid getpwnam getpwent setpwent endpwent getgrent getgrgid getlogin getgrnam setgrent endgrent
syn keyword perlStatementTime		gmtime localtime time times

syn keyword perlStatementMisc		warn formline reset scalar delete prototype lock
if !exists("perl_no_semi_keywords")
  syn keyword perlStatementScope	import
  syn keyword perlStatementNew		new
  syn keyword perlStatementCarp		carp confess croak 		
endif
syn keyword perlTodo			TODO TBD FIXME XXX contained

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
syn match  perlVarPlain		 "$^[ADEFHILMOPSTWX]\="
syn match  perlVarPlain		 "$[\\\"\[\]'&`+*.,;=%~?@$<>(-]"
syn match  perlVarPlain		 "$\(0\|[1-9]\d*\)"
" Same as above, but avoids confusion in $::foo (equivalent to $main::foo)
syn match  perlVarPlain		 "$:[^:]"
" These variables are not recognized within matches.
syn match  perlVarNotInMatches	 "$[|)]"
" This variable is not recognized within matches delimited by '/'.
syn match  perlVarSlash		 "$/"
" This variable is not recognized within matches delimited by '!'.
syn match  perlVarBang		 "$!"

" And plain identifiers
syn match  perlPackageRef	 "\(\h\w*\)\=\(::\|'\)\I"me=e-1 contained

" FIXME value between {} should be marked as string. is treated as such by Perl.
" At the moment it is marked as something greyish instead of read. Probably todo
" with transparency. Or maybe we should handle the bare word in that case. or make it into

if !exists("perl_no_scope_in_variables")
  syn match  perlVarPlain	"\\\=\([@%$]\|\$#\)\$*\(\I\i*\)\=\(\(::\|'\)\I\i*\)*\>" contains=perlPackageRef nextgroup=perlVarMember,perlVarSimpleMember,perlMethod
  syn match  perlFunctionName	"\\\=&\$*\(\I\i*\)\=\(\(::\|'\)\I\i*\)*\>" contains=perlPackageRef nextgroup=perlVarMember,perlVarSimpleMember
else
  syn match  perlVarPlain	"\\\=\([@%$]\|\$#\)\$*\(\I\i*\)\=\(\(::\|'\)\I\i*\)*\>" nextgroup=perlVarMember,perlVarSimpleMember,perlMethod
  syn match  perlFunctionName	"\\\=&\$*\(\I\i*\)\=\(\(::\|'\)\I\i*\)*\>" nextgroup=perlVarMember,perlVarSimpleMember
endif

if !exists("perl_no_extended_vars")
  syn cluster perlExpr		contains=perlStatementScalar,perlStatementRegexp,perlStatementNumeric,perlStatementList,perlStatementHash,perlStatementFiles,perlStatementTime,perlStatementMisc,perlVarPlain,perlVarNotInMatches,perlVarSlash,perlVarBang,perlVarBlock,perlShellCommand,perlFloat,perlNumber,perlStringUnexpanded,perlString,perlQQ
  syn region perlVarBlock	matchgroup=perlVarPlain start="\($#\|[@%$]\)\$*{" skip="\\}" end="}" contains=@perlExpr nextgroup=perlVarMember,perlVarSimpleMember
  syn region perlVarBlock	matchgroup=perlVarPlain start="&\$*{" skip="\\}" end="}" contains=@perlExpr
  syn match  perlVarPlain	"\\\=\(\$#\|[@%&$]\)\$*{\I\i*}" nextgroup=perlVarMember,perlVarSimpleMember
  syn region perlVarMember	matchgroup=perlVarPlain start="\(->\)\={" skip="\\}" end="}" contained contains=@perlExpr nextgroup=perlVarMember,perlVarSimpleMember
  syn match  perlVarSimpleMember	"\(->\)\={\I\i*}" nextgroup=perlVarMember,perlVarSimpleMember contains=perlVarSimpleMemberName contained
  syn match  perlVarSimpleMemberName	"\I\i*" contained
  syn region perlVarMember	matchgroup=perlVarPlain start="\(->\)\=\[" skip="\\]" end="]" contained contains=@perlExpr nextgroup=perlVarMember,perlVarSimpleMember
  syn match  perlMethod		"\(->\)\I\i*" contained
endif

" File Descriptors
syn match  perlFiledescRead	"[<]\h\w\+[>]"

syn match  perlFiledescStatementComma	"(\=\s*\u\w*\s*,"me=e-1 transparent contained contains=perlFiledescStatement
syn match  perlFiledescStatementNocomma "(\=\s*\u\w*\s*[^,[:space:]]"me=e-1 transparent contained contains=perlFiledescStatement

syn match  perlFiledescStatement	"\u\w*" contained

" Special characters in strings and matches
syn match  perlSpecialString	"\\\(\d\+\|[xX]\x\+\|c\u\|.\)" contained
syn match  perlSpecialStringU	"\\['\\]" contained
syn match  perlSpecialMatch	"{\d\+\(,\d*\)\=}" contained
syn match  perlSpecialMatch	"\[\(\]\|-\)\=[^\[\]]*\(\[\|\-\)\=\]" contained
syn match  perlSpecialMatch	"[+*()?.]" contained
syn match  perlSpecialMatch	"(?[#:=!]" contained
syn match  perlSpecialMatch	"(?<[=!]" contained
syn match  perlSpecialMatch	"(?[imsx]\+)" contained
" FIXME the line below does not work. It should mark end of line and
" begin of line as perlSpecial.
" syn match perlSpecialBEOM    "^\^\|\$$" contained

" Possible errors
"
" Highlight lines with only whitespace (only in blank delimited here documents) as errors
syn match  perlNotEmptyLine	"^\s\+$" contained
" Highlight '} else if (...) {', it should be '} else { if (...) { ' or
" '} elsif (...) {'.
"syn keyword perlElseIfError	if contained

" Variable interpolation
"
" These items are interpolated inside "" strings and similar constructs.
syn cluster perlInterpDQ	contains=perlSpecialString,perlVarPlain,perlVarNotInMatches,perlVarSlash,perlVarBang,perlVarBlock
" These items are interpolated inside '' strings and similar constructs.
syn cluster perlInterpSQ	contains=perlSpecialStringU
" These items are interpolated inside m// matches and s/// substitutions.
syn cluster perlInterpSlash	contains=perlSpecialString,perlSpecialMatch,perlVarPlain,perlVarBlock,perlSpecialBEOM
" These items are interpolated inside m## matches and s### substitutions.
syn cluster perlInterpMatch	contains=@perlInterpSlash,perlVarSlash,perlVarBang

" Shell commands
syn region  perlShellCommand	matchgroup=perlMatchStartEnd start="`" end="`" contains=@perlInterpDQ

" Constants
"
" Numbers
syn match  perlNumber	"[-+]\=\(\<\d[[:digit:]_]*L\=\>\|0[xX]\x[[:xdigit:]_]*\>\)"
syn match  perlFloat	"[-+]\=\<\d[[:digit:]_]*[eE][\-+]\=\d\+"
syn match  perlFloat	"[-+]\=\<\d[[:digit:]_]*\.[[:digit:]_]*\([eE][\-+]\=\d\+\)\="
syn match  perlFloat	"[-+]\=\<\.[[:digit:]_]\+\([eE][\-+]\=\d\+\)\="


" Simple version of searches and matches
" caters for m//, m##, m{} and m[] (and the !/ variant)
syn region perlMatch	matchgroup=perlMatchStartEnd start=+[m!]/+ end=+/[cgimosx]*+ contains=@perlInterpSlash
syn region perlMatch	matchgroup=perlMatchStartEnd start=+[m!]#+ end=+#[cgimosx]*+ contains=@perlInterpMatch
syn region perlMatch	matchgroup=perlMatchStartEnd start=+[m!]{+ end=+}[cgimosx]*+ contains=@perlInterpMatch
syn region perlMatch	matchgroup=perlMatchStartEnd start=+[m!]\[+ end=+\][cgimosx]*+ contains=@perlInterpMatch

" A special case for m!!x which allows for comments and extra whitespace in the pattern
syn region perlMatch	matchgroup=perlMatchStartEnd start=+[m!]!+ end=+![cgimosx]*+ contains=@perlInterpSlash,perlComment

" Below some hacks to recognise the // variant. This is virtually impossible to catch in all
" cases as the / is used in so many other ways, but these should be the most obvious ones.
syn region perlMatch	matchgroup=perlMatchStartEnd start=+^split /+lc=5 start=+[^$@%&]\<split /+lc=6 start=+^while /+lc=5 start=+[^$@%&]while /+lc=6 start=+^if /+lc=2 start=+[^$@%&]if /+lc=3 start=+[!=]\~\s*/+lc=2 start=+[(~]/+lc=1 start=+\.\./+lc=2 start=+\s/[^=[:space:][:digit:]$@%&]+lc=1,me=e-1,rs=e-1 start=+^/+ skip=+\\/+ end=+/[cgimosx]*+ contains=@perlInterpSlash


" Substitutions
" caters for s///, s### and s[][]
" perlMatch is the first part, perlSubstitution* is the substitution part
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<s'+  end=+'+me=e-1 contains=@perlInterpSQ nextgroup=perlSubstitutionSQ
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<s"+  end=+"+me=e-1 contains=@perlInterpMatch nextgroup=perlSubstitutionDQ
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<s/+  end=+/+me=e-1 contains=@perlInterpSlash nextgroup=perlSubstitutionSlash
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<s#+  end=+#+me=e-1 contains=@perlInterpMatch nextgroup=perlSubstitutionHash
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<s\[+ end=+\]+ contains=@perlInterpMatch nextgroup=perlSubstitutionBracket skipwhite skipempty skipnl
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<s{+ end=+}+ contains=@perlInterpMatch nextgroup=perlSubstitutionCurly skipwhite skipempty skipnl
syn region perlSubstitutionSQ		matchgroup=perlMatchStartEnd start=+'+  end=+'[ecgimosx]*+ contained contains=@perlInterpSQ
syn region perlSubstitutionDQ		matchgroup=perlMatchStartEnd start=+"+  end=+"[ecgimosx]*+ contained contains=@perlInterpDQ
syn region perlSubstitutionSlash	matchgroup=perlMatchStartEnd start=+/+  end=+/[ecgimosx]*+ contained contains=@perlInterpDQ
syn region perlSubstitutionHash		matchgroup=perlMatchStartEnd start=+#+  end=+#[ecgimosx]*+ contained contains=@perlInterpDQ
syn region perlSubstitutionBracket	matchgroup=perlMatchStartEnd start=+\[+ end=+\][ecgimosx]*+ contained contains=@perlInterpDQ
syn region perlSubstitutionCurly	matchgroup=perlMatchStartEnd start=+{+  end=+}[ecgimosx]*+ contained contains=@perlInterpDQ

" A special case for s!!!x which allows for comments and extra whitespace in the pattern
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<s!+ end=+!+me=e-1 contains=@perlInterpSlash,perlComment nextgroup=perlSubstitutionPling
syn region perlSubstitutionPling	matchgroup=perlMatchStartEnd start=+!+ end=+![ecgimosx]*+ contained contains=@perlInterpDQ

" Substitutions
" caters for tr///, tr### and tr[][]
" perlMatch is the first part, perlTranslation* is the second, translator part.
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\(tr\|y\)'+ end=+'+me=e-1 contains=@perlInterpSQ nextgroup=perlTranslationSQ
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\(tr\|y\)"+ end=+"+me=e-1 contains=@perlInterpSQ nextgroup=perlTranslationDQ
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\(tr\|y\)/+ end=+/+me=e-1 contains=@perlInterpSQ nextgroup=perlTranslationSlash
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\(tr\|y\)#+ end=+#+me=e-1 contains=@perlInterpSQ nextgroup=perlTranslationHash
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\(tr\|y\)\[+ end=+\]+ contains=@perlInterpSQ nextgroup=perlTranslationBracket skipwhite skipempty skipnl
syn region perlMatch	matchgroup=perlMatchStartEnd start=+\<\(tr\|y\){+ end=+}+ contains=@perlInterpSQ nextgroup=perlTranslationCurly skipwhite skipempty skipnl
syn region perlTranslationSQ		matchgroup=perlMatchStartEnd start=+'+ end=+'[cds]*+ contained
syn region perlTranslationDQ		matchgroup=perlMatchStartEnd start=+"+ end=+"[cds]*+ contained
syn region perlTranslationSlash		matchgroup=perlMatchStartEnd start=+/+ end=+/[cds]*+ contained
syn region perlTranslationHash		matchgroup=perlMatchStartEnd start=+#+ end=+#[cds]*+ contained
syn region perlTranslationBracket	matchgroup=perlMatchStartEnd start=+\[+ end=+\][cds]*+ contained
syn region perlTranslationCurly		matchgroup=perlMatchStartEnd start=+{+ end=+}[cds]*+ contained


" The => operator forces a bareword to the left of it to be interpreted as
" a string
syn match  perlString "\<\I\i*\s*=>"me=e-2

" Strings and q, qq, qw and qr expressions

" Parentheses in qq()
syn region perlParens	start=+(+ end=+)+ contained transparent contains=perlParens,@perlStringSQ

syn region perlStringUnexpanded	matchgroup=perlStringStartEnd start="'" end="'" contains=@Spell,@perlInterpSQ
syn region perlString		matchgroup=perlStringStartEnd start=+"+  end=+"+ contains=@Spell,@perlInterpDQ
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<q#+ end=+#+ contains=@perlInterpSQ
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<q|+ end=+|+ contains=@perlInterpSQ
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<q(+ end=+)+ contains=@perlInterpSQ,perlParens
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<q{+ end=+}+ contains=@perlInterpSQ
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<q/+ end=+/+ contains=@perlInterpSQ
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<q[qx]#+ end=+#+ contains=@perlInterpDQ
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<q[qx]|+ end=+|+ contains=@perlInterpDQ
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<q[qx](+ end=+)+ contains=@perlInterpDQ,perlParens
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<q[qx]{+ end=+}+ contains=@perlInterpDQ
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<q[qx]/+ end=+/+ contains=@perlInterpDQ
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<qw#+  end=+#+ contains=@perlInterpSQ
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<qw|+  end=+|+ contains=@perlInterpSQ
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<qw(+  end=+)+ contains=@perlInterpSQ,perlParens
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<qw{+  end=+}+ contains=@perlInterpSQ
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<qw/+  end=+/+ contains=@perlInterpSQ
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<qr#+  end=+#[imosx]*+ contains=@perlInterpMatch
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<qr|+  end=+|[imosx]*+ contains=@perlInterpMatch
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<qr(+  end=+)[imosx]*+ contains=@perlInterpMatch
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<qr{+  end=+}[imosx]*+ contains=@perlInterpMatch
syn region perlQQ		matchgroup=perlStringStartEnd start=+\<qr/+  end=+/[imosx]*+ contains=@perlInterpSlash

" Constructs such as print <<EOF [...] EOF, 'here' documents
"
if version >= 600
  " XXX Any statements after the identifier are in perlString colour (i.e.
  " 'if $a' in 'print <<EOF if $a').
  if exists("perl_fold")
    syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\z(\I\i*\)+    end=+^\z1$+ contains=@perlInterpDQ fold
    syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*"\z(.\{-}\)"+ end=+^\z1$+ contains=@perlInterpDQ fold
    syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*'\z(.\{-}\)'+ end=+^\z1$+ contains=@perlInterpSQ fold
    syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*""+         end=+^$+    contains=@perlInterpDQ,perlNotEmptyLine fold
    syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*''+         end=+^$+    contains=@perlInterpSQ,perlNotEmptyLine fold
    syn region perlAutoload	matchgroup=perlStringStartEnd start=+<<['"]\z(END_\(SUB\|OF_FUNC\|OF_AUTOLOAD\)\)['"]+ end=+^\z1$+ contains=ALL fold
  else
    syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\z(\I\i*\)+    end=+^\z1$+ contains=@perlInterpDQ
    syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*"\z(.\{-}\)"+ end=+^\z1$+ contains=@perlInterpDQ
    syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*'\z(.\{-}\)'+ end=+^\z1$+ contains=@perlInterpSQ
    syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*""+         end=+^$+    contains=@perlInterpDQ,perlNotEmptyLine
    syn region perlHereDoc	matchgroup=perlStringStartEnd start=+<<\s*''+         end=+^$+    contains=@perlInterpSQ,perlNotEmptyLine
    syn region perlAutoload	matchgroup=perlStringStartEnd start=+<<\(['"]\|\)\z(END_\(SUB\|OF_FUNC\|OF_AUTOLOAD\)\)\1+ end=+^\z1$+ contains=ALL
  endif
else
  syn match perlUntilEOFStart	"<<EOF.*"lc=5 nextgroup=perlUntilEOFDQ skipnl transparent
  syn match perlUntilEOFStart	"<<\s*\"EOF\".*" nextgroup=perlUntilEOFDQ skipnl transparent
  syn match perlUntilEOFStart	"<<\s*'EOF'.*" nextgroup=perlUntilEOFSQ skipnl transparent
  syn match perlUntilEOFStart	"<<\s*\"\".*" nextgroup=perlUntilEmptyDQ skipnl transparent
  syn match perlUntilEOFStart	"<<\s*''.*" nextgroup=perlUntilEmptySQ skipnl transparent
  syn region perlUntilEOFDQ	matchgroup=perlStringStartEnd start=++ end="^EOF$" contains=@perlInterpDQ contained
  syn region perlUntilEOFSQ	matchgroup=perlStringStartEnd start=++ end="^EOF$" contains=@perlInterpSQ contained
  syn region perlUntilEmptySQ	matchgroup=perlStringStartEnd start=++ end="^$" contains=@perlInterpDQ,perlNotEmptyLine contained
  syn region perlUntilEmptyDQ	matchgroup=perlStringStartEnd start=++ end="^$" contains=@perlInterpSQ,perlNotEmptyLine contained
  syn match perlHereIdentifier	"<<EOF"
  syn region perlAutoload	matchgroup=perlStringStartEnd start=+<<\(['"]\|\)\(END_\(SUB\|OF_FUNC\|OF_AUTOLOAD\)\)\1+ end=+^\(END_\(SUB\|OF_FUNC\|OF_AUTOLOAD\)\)$+ contains=ALL
endif


" Class declarations
"
syn match  perlPackageDecl	"^\s*\<package\s\+\S\+" contains=perlStatementPackage
syn keyword perlStatementPackage	package contained

" Functions
"       sub [name] [(prototype)] {
"
syn region perlFunction		start="\s*\<sub\>" end="[;{]"he=e-1 contains=perlStatementSub,perlFunctionPrototype,perlFunctionPRef,perlFunctionName,perlComment
syn keyword perlStatementSub	sub contained

syn match  perlFunctionPrototype	"([^)]*)" contained
if !exists("perl_no_scope_in_variables")
   syn match  perlFunctionPRef	"\h\w*::" contained
   syn match  perlFunctionName	"\h\w*[^:]" contained
else
   syn match  perlFunctionName	"\h[[:alnum:]_:]*" contained
endif


" All other # are comments, except ^#!
syn match  perlComment		"#.*" contains=@Spell,perlTodo
syn match  perlSharpBang	"^#!.*"

" Formats
syn region perlFormat		matchgroup=perlStatementIOFunc start="^\s*\<format\s\+\k\+\s*=\s*$"rs=s+6 end="^\s*\.\s*$" contains=perlFormatName,perlFormatField,perlVarPlain
syn match  perlFormatName	"format\s\+\k\+\s*="lc=7,me=e-1 contained
syn match  perlFormatField	"[@^][|<>~]\+\(\.\.\.\)\=" contained
syn match  perlFormatField	"[@^]#[#.]*" contained
syn match  perlFormatField	"@\*" contained
syn match  perlFormatField	"@[^A-Za-z_|<>~#*]"me=e-1 contained
syn match  perlFormatField	"@$" contained

" __END__ and __DATA__ clauses
if exists("perl_fold")
  syntax region perlDATA		start="^__\(DATA\|END\)__$" skip="." end="." contains=perlPOD,@perlDATA fold
else
  syntax region perlDATA		start="^__\(DATA\|END\)__$" skip="." end="." contains=perlPOD,@perlDATA
endif


"
" Folding

if exists("perl_fold")
  syn region perlPackageFold start="^package \S\+;$" end="^1;$" end="\n\+package"me=s-1 transparent fold keepend
  syn region perlSubFold     start="^\z(\s*\)\<sub\>.*[^};]$" end="^\z1}\s*$" end="^\z1}\s*\#.*$" transparent fold keepend
  syn region perlBEGINENDFold start="^\z(\s*\)\<\(BEGIN\|END\|CHECK\|INIT\)\>.*[^};]$" end="^\z1}\s*$" transparent fold keepend

  if exists("perl_fold_blocks")
    syn region perlIfFold start="^\z(\s*\)\(if\|unless\|for\|while\|until\)\s*(.*)\(\s*{\)\=\s*$" start="^\z(\s*\)foreach\s*\(\(my\|our\)\=\s*\S\+\s*\)\=(.*)\(\s*{\)\=\s*$" start="\z(\s*\)else\s*{\s*$" end="^\z1}\s*;\=$" transparent fold keepend
    syn region perlIfFold start="^\z(\s*\)do\(\s*{\)\=\s*$" end="^\z1}\s*while" end="^\z1}\s*;\=$" transparent fold keepend
  endif

  setlocal foldmethod=syntax
  syn sync fromstart
else
  " fromstart above seems to set minlines even if perl_fold is not set.
  syn sync minlines=0
endif


if version >= 508 || !exists("did_perl_syn_inits")
  if version < 508
    let did_perl_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  " The default highlighting.
  HiLink perlSharpBang		PreProc
  HiLink perlControl		PreProc
  HiLink perlInclude		Include
  HiLink perlSpecial		Special
  HiLink perlString		String
  HiLink perlCharacter		Character
  HiLink perlNumber		Number
  HiLink perlFloat		Float
  HiLink perlType		Type
  HiLink perlIdentifier		Identifier
  HiLink perlLabel		Label
  HiLink perlStatement		Statement
  HiLink perlConditional	Conditional
  HiLink perlRepeat		Repeat
  HiLink perlOperator		Operator
  HiLink perlFunction		Function
  HiLink perlFunctionPrototype	perlFunction
  HiLink perlComment		Comment
  HiLink perlTodo		Todo
  if exists("perl_string_as_statement")
    HiLink perlStringStartEnd	perlStatement
  else
    HiLink perlStringStartEnd	perlString
  endif
  HiLink perlList		perlStatement
  HiLink perlMisc		perlStatement
  HiLink perlVarPlain		perlIdentifier
  HiLink perlFiledescRead	perlIdentifier
  HiLink perlFiledescStatement	perlIdentifier
  HiLink perlVarSimpleMember	perlIdentifier
  HiLink perlVarSimpleMemberName perlString
  HiLink perlVarNotInMatches	perlIdentifier
  HiLink perlVarSlash		perlIdentifier
  HiLink perlVarBang		perlIdentifier
  HiLink perlQQ			perlString
  if version >= 600
    HiLink perlHereDoc		perlString
  else
    HiLink perlHereIdentifier	perlStringStartEnd
    HiLink perlUntilEOFDQ	perlString
    HiLink perlUntilEOFSQ	perlString
    HiLink perlUntilEmptyDQ	perlString
    HiLink perlUntilEmptySQ	perlString
    HiLink perlUntilEOF		perlString		
  endif
  HiLink perlStringUnexpanded	perlString
  HiLink perlSubstitutionSQ	perlString
  HiLink perlSubstitutionDQ	perlString
  HiLink perlSubstitutionSlash	perlString
  HiLink perlSubstitutionHash	perlString
  HiLink perlSubstitutionBracket perlString
  HiLink perlSubstitutionCurly 	perlString
  HiLink perlSubstitutionPling	perlString
  HiLink perlTranslationSlash	perlString
  HiLink perlTranslationHash	perlString
  HiLink perlTranslationBracket	perlString
  HiLink perlTranslationCurly	perlString
  HiLink perlMatch		perlString
  HiLink perlMatchStartEnd	perlStatement
  HiLink perlFormatName		perlIdentifier
  HiLink perlFormatField	perlString
  HiLink perlPackageDecl	perlType
  HiLink perlStorageClass	perlType
  HiLink perlPackageRef		perlType
  HiLink perlStatementPackage	perlStatement
  HiLink perlStatementSub	perlStatement
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
  HiLink perlStatementScope	perlStatement
  HiLink perlStatementInclude	perlInclude
  HiLink perlStatementProc	perlStatement
  HiLink perlStatementSocket	perlStatement
  HiLink perlStatementIPC	perlStatement
  HiLink perlStatementNetwork	perlStatement
  HiLink perlStatementPword	perlStatement
  HiLink perlStatementTime	perlStatement
  HiLink perlStatementMisc	perlStatement
  HiLink perlStatementNew	perlStatement
  HiLink perlFunctionName	perlIdentifier
  HiLink perlMethod		perlIdentifier
  HiLink perlFunctionPRef	perlType
  HiLink perlPOD		perlComment
  HiLink perlShellCommand	perlString
  HiLink perlSpecialAscii	perlSpecial
  HiLink perlSpecialDollar	perlSpecial
  HiLink perlSpecialString	perlSpecial
  HiLink perlSpecialStringU	perlSpecial
  HiLink perlSpecialMatch	perlSpecial
  HiLink perlSpecialBEOM	perlSpecial
  HiLink perlDATA		perlComment
  
  HiLink perlParens		Error
  
  " Possible errors
  HiLink perlNotEmptyLine	Error
  HiLink perlElseIfError	Error

  delcommand HiLink
endif

" Syncing to speed up processing
"
if !exists("perl_no_sync_on_sub")
  syn sync match perlSync	grouphere NONE "^\s*\<package\s"
  syn sync match perlSync	grouphere perlFunction "^\s*\<sub\s"
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

" vim: ts=8
