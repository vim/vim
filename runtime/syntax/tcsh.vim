" Vim syntax file
" Language:		C-shell (tcsh)
" Maintainor:		Gautam Iyer <gautam@math.uchicago.edu>
" Last Modified:	Sat 11 Mar 2006 11:16:47 AM CST
"
" Description: We break up each statement into a "command" and an "end" part.
" All groups are either a "command" or part of the "end" of a statement (ie
" everything after the "command"). This is because blindly highlighting tcsh
" statements as keywords caused way too many false positives. Eg:
"
" 	set history=200
"
" causes history to come up as a keyword, which we want to avoid.

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
  finish
endif

syn case match

" ----- Clusters -----
syn cluster tcshModifiers	contains=tcshModifier,tcshModifierError
syn cluster tcshQuoteList	contains=tcshDQuote,tcshSQuote,tcshBQuote
syn cluster tcshStatementEnds	contains=@tcshQuoteList,tcshComment,tcshUsrVar,TcshArgv,tcshSubst,tcshRedir,tcshMeta,tcshHereDoc,tcshSpecial,tcshArguement
syn cluster tcshStatements	contains=tcshBuiltins,tcshCommands,tcshSet,tcshSetEnv,tcshAlias,tcshIf,tcshWhile
syn cluster tcshVarList		contains=tcshUsrVar,tcshArgv,tcshSubst

" ----- Statements -----
" Tcsh commands: Any filename / modifiable variable (must be first!)
syn match tcshCommands	'\v[a-zA-Z0-9\\./_$:-]+' contains=tcshSpecial,tcshUsrVar,tcshArgv,tcshVarError nextgroup=tcshStatementEnd

" Builtin commands except (un)set(env), (un)alias, if, while, else
syn keyword tcshBuiltins nextgroup=tcshStatementEnd alloc bg bindkey break breaksw builtins bye case cd chdir complete continue default dirs echo echotc end endif endsw eval exec exit fg filetest foreach getspath getxvers glob goto hashstat history hup inlib jobs kill limit log login logout ls ls-F migrate newgrp nice nohup notify onintr popd printenv pushd rehash repeat rootnode sched setpath setspath settc setty setxvers shift source stop suspend switch telltc time umask uncomplete unhash universe unlimit ver wait warp watchlog where which

" StatementEnd is anything after a builtin / command till the lexical end of a
" statement (;, |, ||, |&, && or end of line)
syn region tcshStatementEnd	transparent contained matchgroup=tcshBuiltins start='' end='\v\\@<!(;|\|[|&]?|\&\&|$)' contains=@tcshStatementEnds

" set expressions (Contains shell variables)
syn keyword tcshShellVar contained afsuser ampm argv autocorrect autoexpand autolist autologout backslash_quote catalog cdpath color colorcat command complete continue continue_args correct cwd dextract dirsfile dirstack dspmbyte dunique echo echo_style edit ellipsis fignore filec gid group histchars histdup histfile histlit history home ignoreeof implicitcd inputmode killdup killring listflags listjobs listlinks listmax listmaxrows loginsh logout mail matchbeep nobeep noclobber noding noglob nokanji nonomatch nostat notify oid owd path printexitvalue prompt prompt2 prompt3 promptchars pushdtohome pushdsilent recexact recognize_only_executables rmstar rprompt savedirs savehist sched shell shlvl status symlinks tcsh term time tperiod tty uid user verbose version visiblebell watch who wordchars
syn keyword tcshSet	nextgroup=tcshSetEnd set unset
syn region  tcshSetEnd	contained transparent matchgroup=tcshBuiltins start='' skip="\\$" end="$\|;" contains=tcshShellVar,@tcshStatementEnds

" setenv expressions (Contains enviorenment variables)
syn keyword tcshEnvVar contained AFSUSER COLUMNS DISPLAY EDITOR GROUP HOME HOST HOSTTYPE HPATH LANG LC_CTYPE LINES LS_COLORS MACHTYPE NOREBIND OSTYPE PATH PWD REMOTEHOST SHLVL SYSTYPE TERM TERMCAP USER VENDOR VISUAL
syn keyword tcshSetEnv	nextgroup=tcshEnvEnd setenv unsetenv
syn region  tcshEnvEnd	contained transparent matchgroup=tcshBuiltins start='' skip="\\$" end="$\|;" contains=tcshEnvVar,@tcshStatementEnds

" alias and unalias (contains special aliases)
syn keyword tcshAliases contained beemcmd cwdcmd jobcmd helpcommand periodic precmd postcmd shell
syn keyword tcshAlias	nextgroup=tcshAliEnd alias unalias
syn region  tcshAliEnd	contained transparent matchgroup=tcshBuiltins start='' skip="\\$" end="$\|;" contains=tcshAliases,@tcshStatementEnds

" if statements (contains expressions / operators)
syn keyword tcshIf	nextgroup=tcshIfEnd if
syn region  tcshIfEnd	contained matchgroup=tcshBuiltins start='' skip="\\$" end="\v<then>|$" contains=tcshOperator,tcshNumber,@tcshStatementEnds

" else statements (nextgroup if)
syn keyword tcshElse	nextgroup=tcshIf skipwhite else

" while statements (contains expressions / operators)
syn keyword tcshWhile	nextgroup=tcshWhEnd while
syn region  tcshWhEnd	contained transparent matchgroup=tcshBuiltins start='' skip="\\$" end="\v$" contains=tcshOperator,tcshNumber,@tcshStatementEnds

" Expressions start with @.
syn match tcshExprStart "\v\@\s+" nextgroup=tcshExprVar
syn match tcshExprVar	contained "\v\h\w*%(\[\d+\])?" contains=tcshShellVar,tcshEnvVar nextgroup=tcshExprOp
syn match tcshExprOp	contained "++\|--"
syn match tcshExprOp	contained "\v\s*\=" nextgroup=tcshExprEnd
syn match tcshExprEnd	contained "\v.*$"hs=e+1 contains=tcshOperator,tcshNumber,@tcshVarList
syn match tcshExprEnd	contained "\v.{-};"hs=e	contains=tcshOperator,tcshNumber,@tcshVarList

" ----- Comments: -----
syn match tcshComment	"#.*" contains=tcshTodo,tcshCommentTi,tcshCommentSp,@Spell
syn match tcshSharpBang "^#! .*$"
syn match tcshCommentTi contained '\v#\s*\u\w*(\s+\u\w*)*:'hs=s+1 contains=tcshTodo
syn match tcshCommentSp contained '\v<\u{3,}>' contains=tcshTodo
syn match tcshTodo	contained '\v\c<todo>'

" ----- Strings -----
" Tcsh does not allow \" in strings unless the "backslash_quote" shell
" variable is set. Set the vim variable "tcsh_backslash_quote" to 0 if you
" want VIM to assume that no backslash quote constructs exist.

" Backquotes are treated as commands, and are not contained in anything
if(exists("tcsh_backslash_quote") && tcsh_backslash_quote == 0)
    syn region tcshSQuote	keepend contained start="\v\\@<!'" end="'" contains=@Spell
    syn region tcshDQuote	keepend contained start='\v\\@<!"' end='"' contains=@tcshVarList,tcshSpecial,@Spell
    syn region tcshBQuote	keepend start='\v\\@<!`' end='`' contains=@tcshStatements
else
    syn region tcshSQuote	contained start="\v\\@<!'" skip="\v\\\\|\\'" end="'" contains=@Spell
    syn region tcshDQuote	contained start='\v\\@<!"' end='"' contains=@tcshVarList,tcshSpecial,@Spell
    syn region tcshBQuote	keepend matchgroup=tcshBQuoteGrp start='\v\\@<!`' skip='\v\\\\|\\`' end='`' contains=@tcshStatements
endif

" ----- Variables -----
" Variable Errors. Must come first! \$ constructs will be flagged by
" tcshSpecial, so we don't consider them here.
syn match tcshVarError	'\v\$\S*'	contained

" Modifiable Variables without {}.
syn match tcshUsrVar contained "\v\$\h\w*%(\[\d+%(-\d+)?\])?" nextgroup=@tcshModifiers contains=tcshShellVar,tcshEnvVar
syn match tcshArgv   contained "\v\$%(\d+|\*)" nextgroup=@tcshModifiers

" Modifiable Variables with {}.
syn match tcshUsrVar contained "\v\$\{\h\w*%(\[\d+%(-\d+)?\])?%(:\S*)?\}" contains=@tcshModifiers,tcshShellVar,tcshEnvVar
syn match tcshArgv   contained "\v\$\{%(\d+|\*)%(:\S*)?\}" contains=@tcshModifiers

" UnModifiable Substitutions. Order is important here.
syn match tcshSubst contained	"\v\$[?#$!_<]" nextgroup=tcshModifierError
syn match tcshSubst contained	"\v\$[%#?]%(\h\w*|\d+)" nextgroup=tcshModifierError contains=tcshShellVar,tcshEnvVar
syn match tcshSubst contained	"\v\$\{[%#?]%(\h\w*|\d+)%(:\S*)?\}" contains=tcshModifierError contains=tcshShellVar,tcshEnvVar

" Variable Name Expansion Modifiers (order important)
syn match tcshModifierError	contained '\v:\S*'
syn match tcshModifier		contained '\v:[ag]?[htreuls&qx]' nextgroup=@tcshModifiers

" ----- Operators / Specials -----
" Standard redirects (except <<) [<, >, >>, >>&, >>!, >>&!]
syn match tcshRedir contained	"\v\<|\>\>?\&?!?"

" Metachars
syn match tcshMeta  contained	"\v[]{}*?[]"

" Here Documents (<<)
syn region tcshHereDoc contained matchgroup=tcshRedir start="\v\<\<\s*\z(\h\w*)" end="^\z1$" contains=@tcshVarList,tcshSpecial
syn region tcshHereDoc contained matchgroup=tcshRedir start="\v\<\<\s*'\z(\h\w*)'" start='\v\<\<\s*"\z(\h\w*)"$' start="\v\<\<\s*\\\z(\h\w*)$" end="^\z1$"

" Operators
syn match tcshOperator	contained "&&\|!\~\|!=\|<<\|<=\|==\|=\~\|>=\|>>\|\*\|\^\|\~\|||\|!\|%\|&\|+\|-\|/\|<\|>\||"
syn match tcshOperator	contained "[(){}]"

" Numbers
syn match tcshNumber	contained "\v<-?\d+>"

" Arguements
syn match tcshArguement	contained "\v\s@<=-(\w|-)*"

" Special charectors
syn match tcshSpecial	contained "\v\\@<!\\(\d{3}|.)"

" ----- Syncronising -----
if exists("tcsh_minlines")
    exec "syn sync minlines=" . tcsh_minlines
else
    syn sync minlines=15	" Except 'here' documents, nothing is long
endif

" Define highlighting of syntax groups
hi def link tcshBuiltins	statement
hi def link tcshShellVar	preproc
hi def link tcshEnvVar		tcshShellVar
hi def link tcshAliases		tcshShellVar
hi def link tcshCommands	identifier
hi def link tcshSet		tcshBuiltins
hi def link tcshSetEnv		tcshBuiltins
hi def link tcshAlias		tcshBuiltins
hi def link tcshIf		tcshBuiltins
hi def link tcshElse		tcshBuiltins
hi def link tcshWhile		tcshBuiltins
hi def link tcshExprStart	tcshBuiltins
hi def link tcshExprVar		tcshUsrVar
hi def link tcshExprOp		tcshOperator
hi def link tcshExprEnd		tcshOperator
hi def link tcshComment		comment
hi def link tcshCommentTi	preproc
hi def link tcshCommentSp	tcshCommentTi
hi def link tcshSharpBang	tcshCommentTi
hi def link tcshTodo		todo
hi def link tcshSQuote		constant
hi def link tcshDQuote		tcshSQuote
hi def link tcshBQuoteGrp	include
hi def link tcshVarError	error
hi def link tcshUsrVar		type
hi def link tcshArgv		tcshUsrVar
hi def link tcshSubst		tcshUsrVar
hi def link tcshModifier	tcshArguement
hi def link tcshModifierError	tcshVarError
hi def link tcshMeta		tcshSubst
hi def link tcshRedir		tcshOperator
hi def link tcshHereDoc		tcshSQuote
hi def link tcshOperator	operator
hi def link tcshNumber		number
hi def link tcshArguement	special
hi def link tcshSpecial		specialchar

let b:current_syntax = "tcsh"
