" Vim syntax file
" Language:		shell (sh) Korn shell (ksh) bash (sh)
" Maintainer:		Dr. Charles E. Campbell, Jr.  <NdrOchipS@PcampbellAfamily.Mbiz>
" Previous Maintainer:	Lennart Schultz <Lennart.Schultz@ecmwf.int>
" Last Change:		Apr 24, 2008
" Version:		90
" URL:		http://mysite.verizon.net/astronaut/vim/index.html#vimlinks_syntax
"
" Using the following VIM variables: {{{1
" g:is_bash		if none of the previous three variables are
"		defined, then if g:is_bash is set enhance with
"		bash syntax highlighting
" g:is_kornshell	if neither b:is_kornshell or b:is_bash is
"		defined, then if g:is_kornshell is set
"		enhance with kornshell/POSIX syntax highlighting
" g:is_posix                    this variable is the same as g:is_kornshell
" g:sh_fold_enabled	if non-zero, syntax folding is enabled
" g:sh_minlines		sets up syn sync minlines (dflt: 200)
" g:sh_maxlines		sets up syn sync maxlines (dflt: 2x sh_minlines)
"
" This file includes many ideas from Éric Brunet (eric.brunet@ens.fr)

" For version 5.x: Clear all syntax items {{{1
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" handling /bin/sh with is_kornshell/is_sh {{{1
" b:is_sh is set when "#! /bin/sh" is found;
" However, it often is just a masquerade by bash (typically Linux)
" or kornshell (typically workstations with Posix "sh").
" So, when the user sets "is_bash" or "is_kornshell",
" a b:is_sh is converted into b:is_bash/b:is_kornshell,
" respectively.
if !exists("b:is_kornshell") && !exists("b:is_bash")
  if exists("g:is_posix") && !exists("g:is_kornshell")
   let g:is_kornshell= g:is_posix
  endif
  if exists("g:is_kornshell")
    let b:is_kornshell= 1
    if exists("b:is_sh")
      unlet b:is_sh
    endif
  elseif exists("g:is_bash")
    let b:is_bash= 1
    if exists("b:is_sh")
      unlet b:is_sh
    endif
  else
    let b:is_sh= 1
  endif
endif

" set up default g:sh_fold_enabled {{{1
if !exists("g:sh_fold_enabled")
 let g:sh_fold_enabled= 0
elseif g:sh_fold_enabled != 0 && !has("folding")
 let g:sh_fold_enabled= 0
 echomsg "Ignoring g:sh_fold_enabled=".g:sh_fold_enabled."; need to re-compile vim for +fold support"
endif
if g:sh_fold_enabled && &fdm == "manual"
 set fdm=syntax
endif

" sh syntax is case sensitive {{{1
syn case match

" Clusters: contains=@... clusters {{{1
"==================================
syn cluster shCaseEsacList	contains=shCaseStart,shCase,shCaseBar,shCaseIn,shComment,shDeref,shDerefSimple,shCaseCommandSub,shCaseExSingleQuote,shCaseSingleQuote,shCaseDoubleQuote,shCtrlSeq
syn cluster shCaseList	contains=@shCommandSubList,shCaseEsac,shColon,shCommandSub,shCommandSub,shComment,shDo,shEcho,shExpr,shFor,shHereDoc,shIf,shRedir,shSetList,shSource,shStatement,shVariable,shCtrlSeq
syn cluster shColonList	contains=@shCaseList
syn cluster shCommandSubList	contains=shArithmetic,shDeref,shDerefSimple,shNumber,shOperator,shPosnParm,shExSingleQuote,shSingleQuote,shDoubleQuote,shStatement,shVariable,shSubSh,shAlias,shTest,shCtrlSeq
syn cluster shCurlyList	contains=shNumber,shComma,shDeref,shDerefSimple,shDerefSpecial
syn cluster shDblQuoteList	contains=shCommandSub,shDeref,shDerefSimple,shPosnParm,shExSingleQuote,shCtrlSeq,shSpecial
syn cluster shDerefList	contains=shDeref,shDerefSimple,shDerefVar,shDerefSpecial,shDerefWordError,shDerefPPS
syn cluster shDerefVarList	contains=shDerefOp,shDerefVarArray,shDerefOpError
syn cluster shEchoList	contains=shArithmetic,shCommandSub,shDeref,shDerefSimple,shExpr,shExSingleQuote,shSingleQuote,shDoubleQuote,shCtrlSeq
syn cluster shExprList1	contains=shCharClass,shNumber,shOperator,shExSingleQuote,shSingleQuote,shDoubleQuote,shExpr,shDblBrace,shDeref,shDerefSimple,shCtrlSeq
syn cluster shExprList2	contains=@shExprList1,@shCaseList,shTest
syn cluster shFunctionList	contains=@shCommandSubList,shCaseEsac,shColon,shCommandSub,shCommandSub,shComment,shDo,shEcho,shExpr,shFor,shHereDoc,shIf,shRedir,shSetList,shSource,shStatement,shVariable,shOperator,shFunctionStart,shCtrlSeq
if exists("b:is_kornshell") || exists("b:is_bash")
 syn cluster shFunctionList	add=shDblBrace,shDblParen
endif
syn cluster shHereBeginList	contains=@shCommandSubList
syn cluster shHereList	contains=shBeginHere,shHerePayload
syn cluster shHereListDQ	contains=shBeginHere,@shDblQuoteList,shHerePayload
syn cluster shIdList	contains=shCommandSub,shWrapLineOperator,shIdWhiteSpace,shDeref,shDerefSimple,shRedir,shExSingleQuote,shSingleQuote,shDoubleQuote,shExpr,shCtrlSeq
syn cluster shLoopList	contains=@shCaseList,shTestOpr,shExpr,shDblBrace,shConditional,shCaseEsac,shTest
syn cluster shSubShList	contains=@shCaseList,shOperator
syn cluster shTestList	contains=shCharClass,shComment,shCommandSub,shDeref,shDerefSimple,shDoubleQuote,shExpr,shExpr,shNumber,shOperator,shExSingleQuote,shSingleQuote,shTestOpr,shTest,shCtrlSeq


" Echo: {{{1
" ====
" This one is needed INSIDE a CommandSub, so that `echo bla` be correct
syn region shEcho matchgroup=shStatement start="\<echo\>"  skip="\\$" matchgroup=shOperator end="$" matchgroup=NONE end="[<>;&|()]"me=e-1 end="\d[<>]"me=e-2 end="#"me=e-1 contains=@shEchoList
syn region shEcho matchgroup=shStatement start="\<print\>" skip="\\$" matchgroup=shOperator end="$" matchgroup=NONE end="[<>;&|()]"me=e-1 end="\d[<>]"me=e-2 end="#"me=e-1 contains=@shEchoList

" This must be after the strings, so that bla \" be correct
syn region shEmbeddedEcho contained matchgroup=shStatement start="\<print\>" skip="\\$" matchgroup=shOperator end="$" matchgroup=NONE end="[<>;&|`)]"me=e-1 end="\d[<>]"me=e-2 end="#"me=e-1 contains=shNumber,shExSingleQuote,shSingleQuote,shDeref,shDerefSimple,shSpecialVar,shOperator,shDoubleQuote,shCharClass,shCtrlSeq

" Alias: {{{1
" =====
if exists("b:is_kornshell") || exists("b:is_bash")
 syn match shStatement "\<alias\>"
 syn region shAlias matchgroup=shStatement start="\<alias\>\s\+\(\w\+\)\@=" skip="\\$" end="\>\|`"
 syn region shAlias matchgroup=shStatement start="\<alias\>\s\+\(\w\+=\)\@=" skip="\\$" end="="
endif

" Error Codes: {{{1
" ============
syn match   shDoError "\<done\>"
syn match   shIfError "\<fi\>"
syn match   shInError "\<in\>"
syn match   shCaseError ";;"
syn match   shEsacError "\<esac\>"
syn match   shCurlyError "}"
syn match   shParenError ")"
if exists("b:is_kornshell")
 syn match     shDTestError "]]"
endif
syn match     shTestError "]"

" Options Interceptor: {{{1
" ====================
syn match   shOption  "\s[\-+][a-zA-Z0-9]\+\>"ms=s+1
syn match   shOption  "\s--[^ \t$`'"|]\+"ms=s+1

" File Redirection Highlighted As Operators: {{{1
"===========================================
syn match      shRedir	"\d\=>\(&[-0-9]\)\="
syn match      shRedir	"\d\=>>-\="
syn match      shRedir	"\d\=<\(&[-0-9]\)\="
syn match      shRedir	"\d<<-\="

" Operators: {{{1
" ==========
syn match   shOperator	"<<\|>>"		contained
syn match   shOperator	"[!&;|]"
syn match   shOperator	"\[[[^:]\|\]]"
syn match   shOperator	"!\=="		skipwhite nextgroup=shPattern
syn match   shPattern	"\<\S\+\())\)\@="	contained contains=shExSingleQuote,shSingleQuote,shDoubleQuote,shDeref

" Subshells: {{{1
" ==========
syn region shExpr  transparent matchgroup=shExprRegion  start="{" end="}"	contains=@shExprList2
syn region shSubSh transparent matchgroup=shSubShRegion start="(" end=")"	contains=@shSubShList

" Tests: {{{1
"=======
"syn region  shExpr transparent matchgroup=shRange start="\[" skip=+\\\\\|\\$+ end="\]" contains=@shTestList
syn region shExpr	matchgroup=shRange start="\[" skip=+\\\\\|\\$+ end="\]" contains=@shTestList
syn region shTest	transparent matchgroup=shStatement start="\<test\>" skip=+\\\\\|\\$+ matchgroup=NONE end="[;&|]"me=e-1 end="$" contains=@shExprList1
syn match  shTestOpr	contained	"<=\|>=\|!=\|==\|-.\>\|-\(nt\|ot\|ef\|eq\|ne\|lt\|le\|gt\|ge\)\>\|[!<>]"
syn match  shTestOpr	contained	'=' skipwhite nextgroup=shTestDoubleQuote,shTestSingleQuote,shTestPattern
syn match  shTestPattern	contained	'\w\+'
syn match  shTestDoubleQuote	contained	'"[^"]*"'
syn match  shTestSingleQuote	contained	'\\.'
syn match  shTestSingleQuote	contained	"'[^']*'"
if exists("b:is_kornshell") || exists("b:is_bash")
 syn region  shDblBrace matchgroup=Delimiter start="\[\[" skip=+\\\\\|\\$+ end="\]\]"	contains=@shTestList
 syn region  shDblParen matchgroup=Delimiter start="((" skip=+\\\\\|\\$+ end="))"	contains=@shTestList
endif

" Character Class In Range: {{{1
" =========================
syn match   shCharClass	contained	"\[:\(backspace\|escape\|return\|xdigit\|alnum\|alpha\|blank\|cntrl\|digit\|graph\|lower\|print\|punct\|space\|upper\|tab\):\]"

" Loops: do, if, while, until {{{1
" ======
if g:sh_fold_enabled
 syn region shDo	fold transparent matchgroup=shConditional start="\<do\>" matchgroup=shConditional end="\<done\>" contains=@shLoopList
 syn region shIf	fold transparent matchgroup=shConditional start="\<if\>" matchgroup=shConditional end="\<;\_s*then\>" end="\<fi\>"   contains=@shLoopList,shDblBrace,shDblParen,shFunctionKey
 syn region shFor	fold matchgroup=shLoop start="\<for\>" end="\<in\>" end="\<do\>"me=e-2	contains=@shLoopList,shDblParen skipwhite nextgroup=shCurlyIn
else
 syn region shDo	transparent matchgroup=shConditional start="\<do\>" matchgroup=shConditional end="\<done\>" contains=@shLoopList
 syn region shIf	transparent matchgroup=shConditional start="\<if\>" matchgroup=shConditional end="\<;\_s*then\>" end="\<fi\>"   contains=@shLoopList,shDblBrace,shDblParen,shFunctionKey
 syn region shFor	matchgroup=shLoop start="\<for\>" end="\<in\>" end="\<do\>"me=e-2	contains=@shLoopList,shDblParen skipwhite nextgroup=shCurlyIn
endif
if exists("b:is_kornshell") || exists("b:is_bash")
 syn cluster shCaseList add=shRepeat
 syn region shRepeat   matchgroup=shLoop   start="\<while\>" end="\<in\>" end="\<do\>"me=e-2	contains=@shLoopList,shDblParen,shDblBrace
 syn region shRepeat   matchgroup=shLoop   start="\<until\>" end="\<in\>" end="\<do\>"me=e-2	contains=@shLoopList,shDblParen,shDblBrace
 syn region shCaseEsac matchgroup=shConditional start="\<select\>" matchgroup=shConditional end="\<in\>" end="\<do\>" contains=@shLoopList
else
 syn region shRepeat   matchgroup=shLoop   start="\<while\>" end="\<do\>"me=e-2		contains=@shLoopList
 syn region shRepeat   matchgroup=shLoop   start="\<until\>" end="\<do\>"me=e-2		contains=@shLoopList
endif
syn region shCurlyIn   contained	matchgroup=Delimiter start="{" end="}" contains=@shCurlyList
syn match  shComma     contained	","

" Case: case...esac {{{1
" ====
syn match   shCaseBar	contained skipwhite "[^|"`'()]\{-}|"hs=e		nextgroup=shCase,shCaseStart,shCaseBar,shComment,shCaseExSingleQuote,shCaseSingleQuote,shCaseDoubleQuote
syn match   shCaseStart	contained skipwhite skipnl "("			nextgroup=shCase,shCaseBar
syn region  shCase	contained skipwhite skipnl matchgroup=shSnglCase start="\([^#$()'" \t]\|\\.\)\{-})"ms=s,hs=e  end=";;" end="esac"me=s-1 contains=@shCaseList nextgroup=shCaseStart,shCase,shComment
if g:sh_fold_enabled
 syn region  shCaseEsac	fold matchgroup=shConditional start="\<case\>" end="\<esac\>"	contains=@shCaseEsacList
else
 syn region  shCaseEsac	matchgroup=shConditional start="\<case\>" end="\<esac\>"	contains=@shCaseEsacList
endif
syn keyword shCaseIn	contained skipwhite skipnl in			nextgroup=shCase,shCaseStart,shCaseBar,shComment,shCaseExSingleQuote,shCaseSingleQuote,shCaseDoubleQuote
if exists("b:is_bash")
 syn region  shCaseExSingleQuote	matchgroup=shOperator start=+\$'+ skip=+\\\\\|\\.+ end=+'+	contains=shStringSpecial,shSpecial	skipwhite skipnl nextgroup=shCaseBar	contained
 syn region  shCaseExDoubleQuote	matchgroup=shOperator start=+\$"+ skip=+\\\\\|\\.+ end=+"+	contains=shStringSpecial,shSpecial,shCommandSub,shDeref	skipwhite skipnl nextgroup=shCaseBar	contained
else
 syn region  shCaseExSingleQuote	matchgroup=Error start=+\$'+ skip=+\\\\\|\\.+ end=+'+	contains=shStringSpecial	skipwhite skipnl nextgroup=shCaseBar	contained
endif
syn region  shCaseSingleQuote	matchgroup=shOperator start=+'+ end=+'+		contains=shStringSpecial		skipwhite skipnl nextgroup=shCaseBar	contained
syn region  shCaseDoubleQuote	matchgroup=shOperator start=+"+ skip=+\\\\\|\\.+ end=+"+	contains=@shDblQuoteList,shStringSpecial	skipwhite skipnl nextgroup=shCaseBar	contained
syn region  shCaseCommandSub	start=+`+ skip=+\\\\\|\\.+ end=+`+		contains=@shCommandSubList		skipwhite skipnl nextgroup=shCaseBar	contained

" Misc: {{{1
"======
syn match   shWrapLineOperator "\\$"
syn region  shCommandSub   start="`" skip="\\\\\|\\." end="`" contains=@shCommandSubList

" $() and $(()): {{{1
" $(..) is not supported by sh (Bourne shell).  However, apparently
" some systems (HP?) have as their /bin/sh a (link to) Korn shell
" (ie. Posix compliant shell).  /bin/ksh should work for those
" systems too, however, so the following syntax will flag $(..) as
" an Error under /bin/sh.  By consensus of vimdev'ers!
if exists("b:is_kornshell") || exists("b:is_bash")
 syn region shCommandSub matchgroup=shCmdSubRegion start="\$("  skip='\\\\\|\\.' end=")"  contains=@shCommandSubList
 syn region shArithmetic matchgroup=shArithRegion  start="\$((" skip='\\\\\|\\.' end="))" contains=@shCommandSubList
 syn match  shSkipInitWS contained	"^\s\+"
else
 syn region shCommandSub matchgroup=Error start="\$(" end=")" contains=@shCommandSubList
endif

if exists("b:is_bash")
 syn cluster shCommandSubList add=bashSpecialVariables,bashStatement
 syn cluster shCaseList add=bashAdminStatement,bashStatement
 syn keyword bashSpecialVariables contained BASH BASH_ENV BASH_VERSINFO BASH_VERSION CDPATH DIRSTACK EUID FCEDIT FIGNORE GLOBIGNORE GROUPS HISTCMD HISTCONTROL HISTFILE HISTFILESIZE HISTIGNORE HISTSIZE HOME HOSTFILE HOSTNAME HOSTTYPE IFS IGNOREEOF INPUTRC LANG LC_ALL LC_COLLATE LC_MESSAGES LINENO MACHTYPE MAIL MAILCHECK MAILPATH OLDPWD OPTARG OPTERR OPTIND OSTYPE PATH PIPESTATUS PPID PROMPT_COMMAND PS1 PS2 PS3 PS4 PWD RANDOM REPLY SECONDS SHELLOPTS SHLVL TIMEFORMAT TIMEOUT UID auto_resume histchars
 syn keyword bashStatement chmod clear complete du egrep expr fgrep find gnufind gnugrep grep install less ls mkdir mv rm rmdir rpm sed sleep sort strip tail touch
 syn keyword bashAdminStatement daemon killall killproc nice reload restart start status stop
endif

if exists("b:is_kornshell")
 syn cluster shCommandSubList add=kshSpecialVariables,kshStatement
 syn cluster shCaseList add=kshStatement
 syn keyword kshSpecialVariables contained CDPATH COLUMNS EDITOR ENV ERRNO FCEDIT FPATH HISTFILE HISTSIZE HOME IFS LINENO LINES MAIL MAILCHECK MAILPATH OLDPWD OPTARG OPTIND PATH PPID PS1 PS2 PS3 PS4 PWD RANDOM REPLY SECONDS SHELL TMOUT VISUAL
 syn keyword kshStatement cat chmod clear cp du egrep expr fgrep find grep install killall less ls mkdir mv nice printenv rm rmdir sed sort strip stty tail touch tput
endif

syn match   shSource	"^\.\s"
syn match   shSource	"\s\.\s"
syn region  shColon	start="^\s*:" end="$\|" end="#"me=e-1 contains=@shColonList

" String And Character Constants: {{{1
"================================
syn match   shNumber	"-\=\<\d\+\>#\="
syn match   shCtrlSeq	"\\\d\d\d\|\\[abcfnrtv0]"		contained
if exists("b:is_bash")
 syn match   shSpecial	"\\\o\o\o\|\\x\x\x\|\\c.\|\\[abefnrtv]"	contained
endif
if exists("b:is_bash")
 syn region  shExSingleQuote	matchgroup=shOperator start=+\$'+ skip=+\\\\\|\\.+ end=+'+	contains=shStringSpecial,shSpecial
 syn region  shExDoubleQuote	matchgroup=shOperator start=+\$"+ skip=+\\\\\|\\.+ end=+"+	contains=shStringSpecial,shSpecial,shCommandSub,shDeref
else
 syn region  shExSingleQuote	matchGroup=Error start=+\$'+ skip=+\\\\\|\\.+ end=+'+	contains=shStringSpecial
endif
syn region  shSingleQuote	matchgroup=shOperator start=+'+ end=+'+		contains=shStringSpecial,@Spell
syn region  shDoubleQuote	matchgroup=shOperator start=+"+ skip=+\\"+ end=+"+	contains=@shDblQuoteList,shStringSpecial,@Spell
syn match   shStringSpecial	"[^[:print:]]"	contained
syn match   shStringSpecial	"\%(\\\\\)*\\[\\"'`$()#]"
syn match   shSpecial	"[^\\]\zs\%(\\\\\)*\\[\\"'`$()#]"
syn match   shSpecial	"^\%(\\\\\)*\\[\\"'`$()#]"

" Comments: {{{1
"==========
syn cluster    shCommentGroup	contains=shTodo,@Spell
syn keyword    shTodo	contained	COMBAK FIXME TODO XXX
syn match      shComment	"^\s*\zs#.*$"	contains=@shCommentGroup
syn match      shComment	"#.*$"	contains=@shCommentGroup

" Here Documents: {{{1
" =========================================
if version < 600
 syn region shHereDoc matchgroup=shRedir start="<<\s*\**END[a-zA-Z_0-9]*\**"  matchgroup=shRedir end="^END[a-zA-Z_0-9]*$" contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir start="<<-\s*\**END[a-zA-Z_0-9]*\**" matchgroup=shRedir end="^\s*END[a-zA-Z_0-9]*$" contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir start="<<\s*\**EOF\**"	matchgroup=shRedir	end="^EOF$"	contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir start="<<-\s*\**EOF\**" matchgroup=shRedir	end="^\s*EOF$"	contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir start="<<\s*\**\.\**"	matchgroup=shRedir	end="^\.$"	contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir start="<<-\s*\**\.\**"	matchgroup=shRedir	end="^\s*\.$"	contains=@shDblQuoteList

elseif g:sh_fold_enabled
 syn region shHereDoc matchgroup=shRedir fold start="<<\s*\z(\S*\)"		matchgroup=shRedir end="^\z1\s*$"	contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir fold start="<<\s*\"\z(\S*\)\""		matchgroup=shRedir end="^\z1\s*$"
 syn region shHereDoc matchgroup=shRedir fold start="<<\s*'\z(\S*\)'"		matchgroup=shRedir end="^\z1\s*$"
 syn region shHereDoc matchgroup=shRedir fold start="<<-\s*\z(\S*\)"		matchgroup=shRedir end="^\s*\z1\s*$"	contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir fold start="<<-\s*\"\z(\S*\)\""		matchgroup=shRedir end="^\s*\z1\s*$"
 syn region shHereDoc matchgroup=shRedir fold start="<<-\s*'\z(\S*\)'"		matchgroup=shRedir end="^\s*\z1\s*$"
 syn region shHereDoc matchgroup=shRedir fold start="<<\s*\\\_$\_s*\z(\S*\)"		matchgroup=shRedir end="^\z1\s*$"
 syn region shHereDoc matchgroup=shRedir fold start="<<\s*\\\_$\_s*\"\z(\S*\)\""	matchgroup=shRedir end="^\z1\s*$"
 syn region shHereDoc matchgroup=shRedir fold start="<<-\s*\\\_$\_s*'\z(\S*\)'"		matchgroup=shRedir end="^\s*\z1\s*$"
 syn region shHereDoc matchgroup=shRedir fold start="<<-\s*\\\_$\_s*\z(\S*\)"		matchgroup=shRedir end="^\s*\z1\s*$"
 syn region shHereDoc matchgroup=shRedir fold start="<<-\s*\\\_$\_s*\"\z(\S*\)\""	matchgroup=shRedir end="^\s*\z1\s*$"
 syn region shHereDoc matchgroup=shRedir fold start="<<\s*\\\_$\_s*'\z(\S*\)'"		matchgroup=shRedir end="^\z1\s*$"
 syn region shHereDoc matchgroup=shRedir fold start="<<\\\z(\S*\)"		matchgroup=shRedir end="^\z1\s*$"

else
 syn region shHereDoc matchgroup=shRedir start="<<\s*\\\=\z(\S*\)"	matchgroup=shRedir end="^\z1\s*$"    contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir start="<<\s*\"\z(\S*\)\""	matchgroup=shRedir end="^\z1\s*$"
 syn region shHereDoc matchgroup=shRedir start="<<-\s*\z(\S*\)"		matchgroup=shRedir end="^\s*\z1\s*$" contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir start="<<-\s*'\z(\S*\)'"	matchgroup=shRedir end="^\s*\z1\s*$"
 syn region shHereDoc matchgroup=shRedir start="<<\s*'\z(\S*\)'"	matchgroup=shRedir end="^\z1\s*$"
 syn region shHereDoc matchgroup=shRedir start="<<-\s*\"\z(\S*\)\""	matchgroup=shRedir end="^\s*\z1\s*$"
 syn region shHereDoc matchgroup=shRedir start="<<\s*\\\_$\_s*\z(\S*\)"	matchgroup=shRedir end="^\z1\s*$"
 syn region shHereDoc matchgroup=shRedir start="<<-\s*\\\_$\_s*\z(\S*\)"	matchgroup=shRedir end="^\s*\z1\s*$"
 syn region shHereDoc matchgroup=shRedir start="<<-\s*\\\_$\_s*'\z(\S*\)'"	matchgroup=shRedir end="^\s*\z1\s*$"
 syn region shHereDoc matchgroup=shRedir start="<<\s*\\\_$\_s*'\z(\S*\)'"	matchgroup=shRedir end="^\z1\s*$"
 syn region shHereDoc matchgroup=shRedir start="<<\s*\\\_$\_s*\"\z(\S*\)\""	matchgroup=shRedir end="^\z1\s*$"
 syn region shHereDoc matchgroup=shRedir start="<<-\s*\\\_$\_s*\"\z(\S*\)\""	matchgroup=shRedir end="^\s*\z1\s*$"
 syn region shHereDoc matchgroup=shRedir start="<<\\\z(\S*\)"		matchgroup=shRedir end="^\z1\s*$"
endif

" Here Strings: {{{1
" =============
if exists("b:is_bash")
 syn match shRedir "<<<"
endif

" Identifiers: {{{1
"=============
syn match  shVariable "\<\([bwglsav]:\)\=[a-zA-Z0-9.!@_%+,]*\ze="	nextgroup=shSetIdentifier
syn match  shIdWhiteSpace  contained	"\s"
syn match  shSetIdentifier contained	"="	nextgroup=shPattern,shDeref,shDerefSimple,shDoubleQuote,shSingleQuote,shExSingleQuote
if exists("b:is_bash")
  syn region shSetList matchgroup=shSet start="\<\(declare\|typeset\|local\|export\|unset\)\>\ze[^/]" end="$"	matchgroup=shOperator end="\ze[|);&]"me=e-1 matchgroup=NONE end="#\|="me=e-1 contains=@shIdList
  syn region shSetList matchgroup=shSet start="\<set\>[^/]"me=e-1 end="$" end="\\ze[|)]"		matchgroup=shOperator end="\ze[|);&]"me=e-1 matchgroup=NONE end="[#=]"me=e-1 contains=@shIdList
  syn match  shSet "\<\(declare\|typeset\|local\|export\|set\|unset\)\>"
elseif exists("b:is_kornshell")
  syn region shSetList matchgroup=shSet start="\<\(typeset\|export\|unset\)\>\ze[^/]" end="$"		matchgroup=shOperator end="\ze[|);&]"me=e-1 matchgroup=NONE end="[#=]"me=e-1 contains=@shIdList
  syn region shSetList matchgroup=shSet start="\<set\>\ze[^/]" end="$\|\ze[})]"			matchgroup=shOperator end="\ze[|);&]"me=e-1 matchgroup=NONE end="[#=]"me=e-1 contains=@shIdList
  syn match  shSet "\<\(typeset\|set\|export\|unset\)\>"
else
  syn region shSetList matchgroup=shSet start="\<\(set\|export\|unset\)\>\ze[^/]" end="$\|\ze[|)]"	matchgroup=shOperator end="\ze[|);&]" matchgroup=NONE end="[#=]"me=e-1 contains=@shIdList
  syn match  shStatement "\<\(set\|export\|unset\)\>"
endif

" Functions: {{{1
syn keyword shFunctionKey function	skipwhite skipnl nextgroup=shFunctionTwo
" COMBAK -- look at bash09.  function foo() (line#35) is folding 38 lines.  Not being terminated properly
"syn match   shFunctionStart	"{"	contained
if g:sh_fold_enabled
 syn region shFunctionOne transparent fold	start="^\s*\h\w*\s*()\_s*\ze{"    matchgroup=shFunctionStart end="}"	contains=@shFunctionList			skipwhite skipnl nextgroup=shFunctionStart
 syn region shFunctionTwo transparent fold	start="\h\w*\s*\%(()\)\=\_s*\ze{" matchgroup=shFunctionStart end="}"	contains=shFunctionKey,@shFunctionList contained	skipwhite skipnl nextgroup=shFunctionStart
else
 syn region shFunctionOne transparent	start="^\s*\h\w*\s*()\_s*\ze{"    matchgroup=shFunctionStart end="}"	contains=@shFunctionList
 syn region shFunctionTwo transparent	start="\h\w*\s*\%(()\)\=\_s*\ze{" matchgroup=shFunctionStart end="}"	contains=shFunctionKey,@shFunctionList contained
endif

" Parameter Dereferencing: {{{1
" ========================
syn match  shDerefSimple	"\$\%(\h\w*\|\d\)"
syn region shDeref	matchgroup=PreProc start="\${" end="}"	contains=@shDerefList,shDerefVarArray
syn match  shDerefWordError	"[^}$[]"	contained
syn match  shDerefSimple	"\$[-#*@!?]"
syn match  shDerefSimple	"\$\$"
if exists("b:is_bash") || exists("b:is_kornshell")
 syn region shDeref	matchgroup=PreProc start="\${##\=" end="}"	contains=@shDerefList
endif

" bash: ${!prefix*} and ${#parameter}: {{{1
" ====================================
if exists("b:is_bash")
 syn region shDeref	matchgroup=PreProc start="\${!" end="\*\=}"	contains=@shDerefList,shDerefOp
 syn match  shDerefVar	contained	"{\@<=!\w\+"		nextgroup=@shDerefVarList
endif

syn match  shDerefSpecial	contained	"{\@<=[-*@?0]"		nextgroup=shDerefOp,shDerefOpError
syn match  shDerefSpecial	contained	"\({[#!]\)\@<=[[:alnum:]*@_]\+"	nextgroup=@shDerefVarList,shDerefOp
syn match  shDerefVar	contained	"{\@<=\w\+"		nextgroup=@shDerefVarList

" sh ksh bash : ${var[... ]...}  array reference: {{{1
syn region  shDerefVarArray   contained	matchgroup=shDeref start="\[" end="]"	contains=@shCommandSubList nextgroup=shDerefOp,shDerefOpError

" Special ${parameter OPERATOR word} handling: {{{1
" sh ksh bash : ${parameter:-word}    word is default value
" sh ksh bash : ${parameter:=word}    assign word as default value
" sh ksh bash : ${parameter:?word}    display word if parameter is null
" sh ksh bash : ${parameter:+word}    use word if parameter is not null, otherwise nothing
"    ksh bash : ${parameter#pattern}  remove small left  pattern
"    ksh bash : ${parameter##pattern} remove large left  pattern
"    ksh bash : ${parameter%pattern}  remove small right pattern
"    ksh bash : ${parameter%%pattern} remove large right pattern
syn cluster shDerefPatternList	contains=shDerefPattern,shDerefString
syn match shDerefOpError	contained	":[[:punct:]]"
syn match  shDerefOp	contained	":\=[-=?]"	nextgroup=@shDerefPatternList
syn match  shDerefOp	contained	":\=+"	nextgroup=@shDerefPatternList
if exists("b:is_bash") || exists("b:is_kornshell")
 syn match  shDerefOp	contained	"#\{1,2}"	nextgroup=@shDerefPatternList
 syn match  shDerefOp	contained	"%\{1,2}"	nextgroup=@shDerefPatternList
 syn match  shDerefPattern	contained	"[^{}]\+"	contains=shDeref,shDerefSimple,shDerefPattern,shDerefString,shCommandSub,shDerefEscape nextgroup=shDerefPattern
 syn region shDerefPattern	contained	start="{" end="}"	contains=shDeref,shDerefSimple,shDerefString,shCommandSub nextgroup=shDerefPattern
 syn match  shDerefEscape	contained	'\%(\\\\\)*\\.'
endif
syn region shDerefString	contained	matchgroup=shOperator start=+'+ end=+'+		contains=shStringSpecial
syn region shDerefString	contained	matchgroup=shOperator start=+"+ skip=+\\"+ end=+"+	contains=@shDblQuoteList,shStringSpecial
syn match  shDerefString	contained	"\\["']"

if exists("b:is_bash")
 " bash : ${parameter:offset}
 " bash : ${parameter:offset:length}
 syn region shDerefOp	contained	start=":[$[:alnum:]_]"me=e-1 end=":"me=e-1 end="}"me=e-1 contains=@shCommandSubList nextgroup=shDerefPOL
 syn match  shDerefPOL	contained	":[^}]\+"	contains=@shCommandSubList

 " bash : ${parameter//pattern/string}
 " bash : ${parameter//pattern}
 syn match  shDerefPPS	contained	'/\{1,2}'	nextgroup=shDerefPPSleft
 syn region shDerefPPSleft	contained	start='.'	skip=@\%(\\\)\/@ matchgroup=shDerefOp end='/' end='\ze}' nextgroup=shDerefPPSright contains=@shCommandSubList
 syn region shDerefPPSright	contained	start='.'	end='\ze}'	contains=@shCommandSubList
endif

" Useful sh Keywords: {{{1
" ===================
syn keyword shStatement break cd chdir continue eval exec exit kill newgrp pwd read readonly return shift test trap ulimit umask wait
syn keyword shConditional contained elif else then
syn keyword shCondError elif else then

" Useful ksh Keywords: {{{1
" ====================
if exists("b:is_kornshell") || exists("b:is_bash")
 syn keyword shStatement autoload bg false fc fg functions getopts hash history integer jobs let nohup print printf r stop suspend time times true type unalias whence

" Useful bash Keywords: {{{1
" =====================
 if exists("b:is_bash")
  syn keyword shStatement bind builtin dirs disown enable help local logout popd pushd shopt source
 else
  syn keyword shStatement login newgrp
 endif
endif

" Synchronization: {{{1
" ================
if !exists("sh_minlines")
  let sh_minlines = 200
endif
if !exists("sh_maxlines")
  let sh_maxlines = 2 * sh_minlines
endif
exec "syn sync minlines=" . sh_minlines . " maxlines=" . sh_maxlines
syn sync match shCaseEsacSync	grouphere	shCaseEsac	"\<case\>"
syn sync match shCaseEsacSync	groupthere	shCaseEsac	"\<esac\>"
syn sync match shDoSync	grouphere	shDo	"\<do\>"
syn sync match shDoSync	groupthere	shDo	"\<done\>"
syn sync match shForSync	grouphere	shFor	"\<for\>"
syn sync match shForSync	groupthere	shFor	"\<in\>"
syn sync match shIfSync	grouphere	shIf	"\<if\>"
syn sync match shIfSync	groupthere	shIf	"\<fi\>"
syn sync match shUntilSync	grouphere	shRepeat	"\<until\>"
syn sync match shWhileSync	grouphere	shRepeat	"\<while\>"

" Default Highlighting: {{{1
" =====================
hi def link shArithRegion	shShellVariables
hi def link shBeginHere	shRedir
hi def link shCaseBar	shConditional
hi def link shCaseCommandSub	shCommandSub
hi def link shCaseDoubleQuote	shDoubleQuote
hi def link shCaseIn	shConditional
hi def link shCaseSingleQuote	shSingleQuote
hi def link shCaseDoubleQuote	shSingleQuote
hi def link shCaseStart	shConditional
hi def link shCmdSubRegion	shShellVariables
hi def link shColon	shStatement
hi def link shDerefOp	shOperator
hi def link shDerefPOL	shDerefOp
hi def link shDerefPPS	shDerefOp
hi def link shDeref	shShellVariables
hi def link shDerefSimple	shDeref
hi def link shDerefSpecial	shDeref
hi def link shDerefString	shDoubleQuote
hi def link shDerefVar	shDeref
hi def link shDoubleQuote	shString
hi def link shEcho	shString
hi def link shEmbeddedEcho	shString
hi def link shExSingleQuote	shSingleQuote
hi def link shExDoubleQuote	shSingleQuote
hi def link shFunctionStart	Delimiter
hi def link shHereDoc	shString
hi def link shHerePayload	shHereDoc
hi def link shLoop	shStatement
hi def link shOption	shCommandSub
hi def link shPattern	shString
hi def link shPosnParm	shShellVariables
hi def link shRange	shOperator
hi def link shRedir	shOperator
hi def link shSingleQuote	shString
hi def link shSource	shOperator
hi def link shStringSpecial	shSpecial
hi def link shSubShRegion	shOperator
hi def link shTestOpr	shConditional
hi def link shTestPattern	shString
hi def link shTestDoubleQuote	shString
hi def link shTestSingleQuote	shString
hi def link shVariable	shSetList
hi def link shWrapLineOperator	shOperator

if exists("b:is_bash")
  hi def link bashAdminStatement	shStatement
  hi def link bashSpecialVariables	shShellVariables
  hi def link bashStatement		shStatement
  hi def link shFunctionParen		Delimiter
  hi def link shFunctionDelim		Delimiter
endif
if exists("b:is_kornshell")
  hi def link kshSpecialVariables	shShellVariables
  hi def link kshStatement		shStatement
  hi def link shFunctionParen		Delimiter
endif

hi def link shCaseError		Error
hi def link shCondError		Error
hi def link shCurlyError		Error
hi def link shDerefError		Error
hi def link shDerefOpError		Error
hi def link shDerefWordError		Error
hi def link shDoError		Error
hi def link shEsacError		Error
hi def link shIfError		Error
hi def link shInError		Error
hi def link shParenError		Error
hi def link shTestError		Error
if exists("b:is_kornshell")
  hi def link shDTestError		Error
endif

hi def link shArithmetic		Special
hi def link shCharClass		Identifier
hi def link shSnglCase		Statement
hi def link shCommandSub		Special
hi def link shComment		Comment
hi def link shConditional		Conditional
hi def link shCtrlSeq		Special
hi def link shExprRegion		Delimiter
hi def link shFunctionKey		Function
hi def link shFunctionName		Function
hi def link shNumber		Number
hi def link shOperator		Operator
hi def link shRepeat		Repeat
hi def link shSet		Statement
hi def link shSetList		Identifier
hi def link shShellVariables		PreProc
hi def link shSpecial		Special
hi def link shStatement		Statement
hi def link shString		String
hi def link shTodo		Todo
hi def link shAlias		Identifier

" Set Current Syntax: {{{1
" ===================
if exists("b:is_bash")
 let b:current_syntax = "bash"
elseif exists("b:is_kornshell")
 let b:current_syntax = "ksh"
else
 let b:current_syntax = "sh"
endif

" vim: ts=16 fdm=marker
