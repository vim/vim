" Vim syntax file
" Language:		shell (sh) Korn shell (ksh) bash (sh)
" Maintainer:		Dr. Charles E. Campbell, Jr.  <NdrOchipS@PcampbellAfamily.Mbiz>
" Previous Maintainer:	Lennart Schultz <Lennart.Schultz@ecmwf.int>
" Last Change:		Apr 28, 2004
" Version:		68
" URL:		http://www.erols.com/astronaut/vim/index.html#vimlinks_syntax
"
" Using the following VIM variables:
" b:is_kornshell	if defined, enhance with kornshell syntax
" b:is_bash		if defined, enhance with bash syntax
"   is_kornshell	if neither b:is_kornshell or b:is_bash is
"		defined, then if is_kornshell is set
"		b:is_kornshell is default
"   is_bash		if none of the previous three variables are
"		defined, then if is_bash is set b:is_bash is default
" g:sh_fold_enabled	if non-zero, syntax folding is enabled
"   sh_minlines		sets up syn sync minlines  (default: 200)
"   sh_maxlines		sets up syn sync maxlines  (default: twice sh_minlines)
"
" This file includes many ideas from Éric Brunet (eric.brunet@ens.fr)

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" b:is_sh is set when "#! /bin/sh" is found;
" However, it often is just a masquerade by bash (typically Linux)
" or kornshell (typically workstations with Posix "sh").
" So, when the user sets "is_bash" or "is_kornshell",
" a b:is_sh is converted into b:is_bash/b:is_kornshell,
" respectively.
if !exists("b:is_kornshell") && !exists("b:is_bash")
  if exists("is_kornshell")
    let b:is_kornshell= 1
    if exists("b:is_sh")
      unlet b:is_sh
    endif
  elseif exists("is_bash")
    let b:is_bash= 1
    if exists("b:is_sh")
      unlet b:is_sh
    endif
  else
    let b:is_sh= 1
  endif
endif

if !exists("g:sh_fold_enabled")
 let g:sh_fold_enabled= 0
endif

" sh syntax is case sensitive
syn case match

" Clusters: contains=@... clusters
"==================================
syn cluster shCaseEsacList	contains=shCaseStart,shCase,shCaseBar,shCaseIn,shComment,shDeref,shDerefSimple,shCaseCommandSub,shCaseSingleQuote,shCaseDoubleQuote,shSpecial
syn cluster shCaseList	contains=@shCommandSubList,shCaseEsac,shColon,shCommandSub,shCommandSub,shComment,shDo,shEcho,shExpr,shFor,shHereDoc,shIf,shRedir,shSetList,shSource,shStatement,shVariable,bkshFunction,shSpecial
syn cluster shColonList	contains=@shCaseList
syn cluster shCommandSubList	contains=shArithmetic,shDeref,shDerefSimple,shNumber,shOperator,shPosnParm,shSpecial,shSingleQuote,shDoubleQuote,shStatement,shVariable,shSubSh,shAlias
syn cluster shDblQuoteList	contains=shCommandSub,shDeref,shDerefSimple,shSpecial,shPosnParm
syn cluster shDerefList	contains=shDeref,shDerefSimple,shDerefVar,shDerefSpecial,shDerefWordError
syn cluster shDerefVarList	contains=shDerefOp,shDerefVarArray,shDerefOpError
syn cluster shEchoList	contains=shArithmetic,shCommandSub,shDeref,shDerefSimple,shExpr,shSingleQuote,shDoubleQuote,shSpecial
syn cluster shExprList1	contains=shCharClass,shNumber,shOperator,shSingleQuote,shDoubleQuote,shSpecial,shExpr,shDblBrace,shDeref,shDerefSimple
syn cluster shExprList2	contains=@shExprList1,@shCaseList
syn cluster shFunctionList	contains=@shCaseList,shOperator
syn cluster shHereBeginList	contains=@shCommandSubList
syn cluster shHereList	contains=shBeginHere,shHerePayload
syn cluster shHereListDQ	contains=shBeginHere,@shDblQuoteList,shHerePayload
syn cluster shIdList	contains=shCommandSub,shWrapLineOperator,shIdWhiteSpace,shDeref,shDerefSimple,shSpecial,shRedir,shSingleQuote,shDoubleQuote,shExpr
syn cluster shLoopList	contains=@shCaseList,shTestOpr,shExpr,shDblBrace,shConditional,shCaseEsac
syn cluster shSubShList	contains=@shCaseList
syn cluster shTestList	contains=shCharClass,shComment,shCommandSub,shDeref,shDerefSimple,shDoubleQuote,shExpr,shExpr,shNumber,shOperator,shSingleQuote,shSpecial,shTestOpr


" Echo:
" ====
" This one is needed INSIDE a CommandSub, so that `echo bla` be correct
syn region shEcho matchgroup=shStatement start="\<echo\>"  skip="\\$" matchgroup=shOperator end="$" matchgroup=NONE end="[<>;&|()]"me=e-1 end="\d[<>]"me=e-2 end="#"me=e-1 contains=@shEchoList
syn region shEcho matchgroup=shStatement start="\<print\>" skip="\\$" matchgroup=shOperator end="$" matchgroup=NONE end="[<>;&|()]"me=e-1 end="\d[<>]"me=e-2 end="#"me=e-1 contains=@shEchoList

" This must be after the strings, so that bla \" be correct
syn region shEmbeddedEcho contained matchgroup=shStatement start="\<print\>" skip="\\$" matchgroup=shOperator end="$" matchgroup=NONE end="[<>;&|`)]"me=e-1 end="\d[<>]"me=e-2 end="#"me=e-1 contains=shNumber,shSingleQuote,shDeref,shDerefSimple,shSpecialVar,shSpecial,shOperator,shDoubleQuote,shCharClass

" Alias:
" =====
if exists("b:is_kornshell") || exists("b:is_bash")
 syn match shStatement "\<alias\>"
 syn region shAlias matchgroup=shStatement start="\<alias\>\s\+\(\w\+\)\@=" skip="\\$" end="\>\|`"
 syn region shAlias matchgroup=shStatement start="\<alias\>\s\+\(\w\+=\)\@=" skip="\\$" end="="
endif

" Error Codes
" ===========
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

" Options interceptor
" ===================
syn match   shOption  "\s[\-+][a-zA-Z0-9]\+\>"ms=s+1
syn match   shOption  "\s--\S\+"ms=s+1

" Operators:
" =========
syn match   shOperator	"[!&;|]"
syn match   shOperator	"\[[[^:]\|\]]"
syn match   shOperator	"!\=="		skipwhite nextgroup=shPattern
syn match   shPattern	"\<\S\+\())\)\@="	contained contains=shSingleQuote,shDoubleQuote,shDeref

" Subshells:
" =========
syn region shExpr  transparent matchgroup=shExprRegion  start="{" end="}"	contains=@shExprList2
syn region shSubSh transparent matchgroup=shSubShRegion start="(" end=")"	contains=@shSubShList

" Tests
"======
"syn region  shExpr transparent matchgroup=shRange start="\[" skip=+\\\\\|\\$+ end="\]" contains=@shTestList
syn region  shExpr matchgroup=shRange start="\[" skip=+\\\\\|\\$+ end="\]" contains=@shTestList
syn region  shExpr transparent matchgroup=shStatement start="\<test\>" skip=+\\\\\|\\$+ matchgroup=NONE end="[;&|]"me=e-1 end="$" contains=@shExprList1
syn match   shTestOpr contained "<=\|>=\|!=\|==\|-.\>\|-\(nt\|ot\|ef\|eq\|ne\|lt\|le\|gt\|ge\)\>\|[!=<>]"
if exists("b:is_kornshell") || exists("b:is_bash")
 syn region  shDblBrace matchgroup=Delimiter start="\[\[" skip=+\\\\\|\\$+ end="\]\]"	contains=@shTestList
 syn region  shDblParen matchgroup=Delimiter start="((" skip=+\\\\\|\\$+ end="))"	contains=@shTestList
endif

" Character Class in Range
" ========================
syn match   shCharClass	contained	"\[:\(backspace\|escape\|return\|xdigit\|alnum\|alpha\|blank\|cntrl\|digit\|graph\|lower\|print\|punct\|space\|upper\|tab\):\]"

" Loops: do, if, while, until
" =====
syn region shDo		transparent matchgroup=shConditional start="\<do\>" matchgroup=shConditional end="\<done\>" contains=@shLoopList
syn region shIf		transparent matchgroup=shConditional start="\<if\>" matchgroup=shConditional end="\<;\_s*then\>" end="\<fi\>"   contains=@shLoopList,shDblBrace,shDblParen
syn region shFor	matchgroup=shLoop start="\<for\>" end="\<in\>" end="\<do\>"me=e-2	contains=@shLoopList,shDblParen
if exists("b:is_kornshell") || exists("b:is_bash")
 syn cluster shCaseList add=shRepeat
 syn region shRepeat   matchgroup=shLoop   start="\<while\>" end="\<in\>" end="\<do\>"me=e-2	contains=@shLoopList,shDblParen,shDblBrace
 syn region shRepeat   matchgroup=shLoop   start="\<until\>" end="\<in\>" end="\<do\>"me=e-2	contains=@shLoopList,shDblParen,shDblBrace
 syn region shCaseEsac matchgroup=shConditional start="\<select\>" matchgroup=shConditional end="\<in\>" end="\<do\>" contains=@shLoopList
else
 syn region shRepeat   matchgroup=shLoop   start="\<while\>" end="\<do\>"me=e-2		contains=@shLoopList
 syn region shRepeat   matchgroup=shLoop   start="\<until\>" end="\<do\>"me=e-2		contains=@shLoopList
endif

" Case: case...esac
" ====
syn match   shCaseBar	contained skipwhite "[^|"`'()]\{-}|"hs=e		nextgroup=shCase,shCaseStart,shCaseBar,shComment,shCaseSingleQuote,shCaseDoubleQuote
syn match   shCaseStart	contained skipwhite skipnl "("			nextgroup=shCase,shCaseBar
syn region  shCase	contained skipwhite skipnl matchgroup=shSnglCase start="[^$()]\{-})"ms=s,hs=e  end=";;" end="esac"me=s-1 contains=@shCaseList nextgroup=shCaseStart,shCase,,shComment
syn region  shCaseEsac	matchgroup=shConditional start="\<case\>" end="\<esac\>"	contains=@shCaseEsacList
syn keyword shCaseIn	contained skipwhite skipnl in			nextgroup=shCase,shCaseStart,shCaseBar,shComment,shCaseSingleQuote,shCaseDoubleQuote
syn region  shCaseSingleQuote	matchgroup=shOperator start=+'+ skip=+\\\\\|\\.+ end=+'+	contains=shStringSpecial		skipwhite skipnl nextgroup=shCaseBar	contained
syn region  shCaseDoubleQuote	matchgroup=shOperator start=+"+ skip=+\\\\\|\\.+ end=+"+	contains=@shDblQuoteList,shStringSpecial	skipwhite skipnl nextgroup=shCaseBar	contained
syn region  shCaseCommandSub	start=+`+ skip=+\\\\\|\\.+ end=+`+		contains=@shCommandSubList		skipwhite skipnl nextgroup=shCaseBar	contained

" Misc
"=====
syn match   shWrapLineOperator "\\$"
syn region  shCommandSub   start="`" skip="\\\\\|\\." end="`" contains=@shCommandSubList

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
 syn region shCommandSub matchgroup=Error start="$(" end=")" contains=@shCommandSubList
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

" String and Character constants
"===============================
syn match   shNumber	"-\=\<\d\+\>"
syn match   shSpecial	"\\\d\d\d\|\\[abcfnrtv0]"	contained
syn region  shSingleQuote	matchgroup=shOperator start=+'+ end=+'+		contains=shStringSpecial
syn region  shDoubleQuote	matchgroup=shOperator start=+"+ skip=+\\"+ end=+"+	contains=@shDblQuoteList,shStringSpecial
syn match   shStringSpecial	"[^[:print:]]"	contained
syn match   shSpecial	"\\[\\\"\'`$()#]"

" Comments
"=========
syn cluster    shCommentGroup	contains=shTodo,@Spell
syn keyword    shTodo	contained	TODO
syn match      shComment	"#.*$" contains=@shCommentGroup

" File redirection highlighted as operators
"==========================================
syn match      shRedir	"\d\=>\(&[-0-9]\)\="
syn match      shRedir	"\d\=>>-\="
syn match      shRedir	"\d\=<\(&[-0-9]\)\="
syn match      shRedir	"\d<<-\="

" Shell Input Redirection (Here Documents)
if version < 600
 syn region shHereDoc matchgroup=shRedir start="<<\s*\**END[a-zA-Z_0-9]*\**"  matchgroup=shRedir end="^END[a-zA-Z_0-9]*$" contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir start="<<-\s*\**END[a-zA-Z_0-9]*\**" matchgroup=shRedir end="^\s*END[a-zA-Z_0-9]*$" contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir start="<<\s*\**EOF\**"	matchgroup=shRedir	end="^EOF$"	contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir start="<<-\s*\**EOF\**" matchgroup=shRedir	end="^\s*EOF$"	contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir start="<<\s*\**\.\**"	matchgroup=shRedir	end="^\.$"	contains=@shDblQuoteList
 syn region shHereDoc matchgroup=shRedir start="<<-\s*\**\.\**"	matchgroup=shRedir	end="^\s*\.$"	contains=@shDblQuoteList

elseif g:sh_fold_enabled

 if v:version > 602 || (v:version == 602 && has("patch219"))
  syn region shHereDoc fold start="\(<<\s*\\\=\z(\S*\)\)\@="		matchgroup=shRedir end="^\z1$"		contains=@shHereListDQ	keepend
  syn region shHereDoc fold start="\(<<\s*\"\z(\S*\)\"\)\@="		matchgroup=shRedir end="^\z1$""		contains=@shHereList	keepend
  syn region shHereDoc fold start="\(<<\s*'\z(\S*\)'\)\@="		matchgroup=shRedir end="^\z1$""		contains=@shHereList	keepend
  syn region shHereDoc fold start="\(<<\s*\\\_$\_s*\z(\S*\)\)\@="	matchgroup=shRedir end="^\z1$""		contains=@shHereList	keepend
  syn region shHereDoc fold start="\(<<\s*\\\_$\_s*\"\z(\S*\)\"\)\@="	matchgroup=shRedir end="^\z1$""		contains=@shHereList	keepend
  syn region shHereDoc fold start="\(<<\s*\\\_$\_s*'\z(\S*\)'\)\@="	matchgroup=shRedir end="^\z1$"		contains=@shHereList	keepend
  syn region shHereDoc fold start="\(<<-\s*\z(\S*\)\)\@="		matchgroup=shRedir end="^\s*\z1$"	contains=@shHereList	keepend
  syn region shHereDoc fold start="\(<<-\s*\"\z(\S*\)\"\)\@="		matchgroup=shRedir end="^\s*\z1$""	contains=@shHereListDQ	keepend
  syn region shHereDoc fold start="\(<<-\s*'\z(\S*\)'\)\@="		matchgroup=shRedir end="^\s*\z1$""	contains=@shHereList	keepend
  syn region shHereDoc fold start="\(<<-\s*\\\_$\_s*'\z(\S*\)'\)\@="	matchgroup=shRedir end="^\s*\z1$"	contains=@shHereList	keepend
  syn region shHereDoc fold start="\(<<-\s*\\\_$\_s*\z(\S*\)\)\@="	matchgroup=shRedir end="^\s*\z1$"	contains=@shHereList	keepend
  syn region shHereDoc fold start="\(<<-\s*\\\_$\_s*\"\z(\S*\)\"\)\@="	matchgroup=shRedir end="^\s*\z1$"	contains=@shHereList	keepend
 else
  syn region shHereDoc matchgroup=shRedir fold start="<<\s*\z(\S*\)"		matchgroup=shRedir end="^\z1$"		contains=@shDblQuoteList
  syn region shHereDoc matchgroup=shRedir fold start="<<\s*\"\z(\S*\)\""		matchgroup=shRedir end="^\z1$"
  syn region shHereDoc matchgroup=shRedir fold start="<<\s*'\z(\S*\)'"		matchgroup=shRedir end="^\z1$"
  syn region shHereDoc matchgroup=shRedir fold start="<<-\s*\z(\S*\)"		matchgroup=shRedir end="^\s*\z1$"	contains=@shDblQuoteList
  syn region shHereDoc matchgroup=shRedir fold start="<<-\s*\"\z(\S*\)\""		matchgroup=shRedir end="^\s*\z1$"
  syn region shHereDoc matchgroup=shRedir fold start="<<-\s*'\z(\S*\)'"		matchgroup=shRedir end="^\s*\z1$"
  syn region shHereDoc matchgroup=shRedir fold start="<<\s*\\\_$\_s*\z(\S*\)"		matchgroup=shRedir end="^\z1$"
  syn region shHereDoc matchgroup=shRedir fold start="<<\s*\\\_$\_s*\"\z(\S*\)\""	matchgroup=shRedir end="^\z1$"
  syn region shHereDoc matchgroup=shRedir fold start="<<-\s*\\\_$\_s*'\z(\S*\)'"	matchgroup=shRedir end="^\s*\z1$"
  syn region shHereDoc matchgroup=shRedir fold start="<<-\s*\\\_$\_s*\z(\S*\)"		matchgroup=shRedir end="^\s*\z1$"
  syn region shHereDoc matchgroup=shRedir fold start="<<-\s*\\\_$\_s*\"\z(\S*\)\""	matchgroup=shRedir end="^\s*\z1$"
  syn region shHereDoc matchgroup=shRedir fold start="<<\s*\\\_$\_s*'\z(\S*\)'"		matchgroup=shRedir end="^\z1$"
 endif
else
 if v:version > 602 || (v:version == 602 && has("patch219"))
  syn region shHereDoc start="\(<<\s*\\\=\z(\S*\)\)\@="		matchgroup=shRedir end="^\z1$"		contains=@shHereList	keepend
  syn region shHereDoc start="\(<<\s*\"\z(\S*\)\"\)\@="		matchgroup=shRedir end="^\z1$""		contains=@shHereListDQ	keepend
  syn region shHereDoc start="\(<<\s*'\z(\S*\)'\)\@="		matchgroup=shRedir end="^\z1$""		contains=@shHereList	keepend
  syn region shHereDoc start="\(<<\s*\\\_$\_s*\z(\S*\)\)\@="		matchgroup=shRedir end="^\z1$""		contains=@shHereList	keepend
  syn region shHereDoc start="\(<<\s*\\\_$\_s*\"\z(\S*\)\"\)\@="	matchgroup=shRedir end="^\z1$""		contains=@shHereList	keepend
  syn region shHereDoc start="\(<<\s*\\\_$\_s*'\z(\S*\)'\)\@="		matchgroup=shRedir end="^\z1$"		contains=@shHereList	keepend
  syn region shHereDoc start="\(<<-\s*\z(\S*\)\)\@="		matchgroup=shRedir end="^\s*\z1$"	contains=@shHereList	keepend
  syn region shHereDoc start="\(<<-\s*\"\z(\S*\)\"\)\@="		matchgroup=shRedir end="^\s*\z1$""	contains=@shHereListDQ	keepend
  syn region shHereDoc start="\(<<-\s*'\z(\S*\)'\)\@="		matchgroup=shRedir end="^\s*\z1$""	contains=@shHereList	keepend
  syn region shHereDoc start="\(<<-\s*\\\_$\_s*'\z(\S*\)'\)\@="		matchgroup=shRedir end="^\s*\z1$"	contains=@shHereList	keepend
  syn region shHereDoc start="\(<<-\s*\\\_$\_s*\z(\S*\)\)\@="		matchgroup=shRedir end="^\s*\z1$"	contains=@shHereList	keepend
  syn region shHereDoc start="\(<<-\s*\\\_$\_s*\"\z(\S*\)\"\)\@="	matchgroup=shRedir end="^\s*\z1$"	contains=@shHereList	keepend
 else
  syn region shHereDoc matchgroup=shRedir start="<<\s*\\\=\z(\S*\)"	matchgroup=shRedir end="^\z1$"    contains=@shDblQuoteList
  syn region shHereDoc matchgroup=shRedir start="<<\s*\"\z(\S*\)\""	matchgroup=shRedir end="^\z1$"
  syn region shHereDoc matchgroup=shRedir start="<<-\s*\z(\S*\)"	matchgroup=shRedir end="^\s*\z1$" contains=@shDblQuoteList
  syn region shHereDoc matchgroup=shRedir start="<<-\s*'\z(\S*\)'"	matchgroup=shRedir end="^\s*\z1$"
  syn region shHereDoc matchgroup=shRedir start="<<\s*'\z(\S*\)'"	matchgroup=shRedir end="^\z1$"
  syn region shHereDoc matchgroup=shRedir start="<<-\s*\"\z(\S*\)\""	matchgroup=shRedir end="^\s*\z1$"
  syn region shHereDoc matchgroup=shRedir start="<<\s*\\\_$\_s*\z(\S*\)"	matchgroup=shRedir end="^\z1$"
  syn region shHereDoc matchgroup=shRedir start="<<-\s*\\\_$\_s*\z(\S*\)"	matchgroup=shRedir end="^\s*\z1$"
  syn region shHereDoc matchgroup=shRedir start="<<-\s*\\\_$\_s*'\z(\S*\)'"	matchgroup=shRedir end="^\s*\z1$"
  syn region shHereDoc matchgroup=shRedir start="<<\s*\\\_$\_s*'\z(\S*\)'"	matchgroup=shRedir end="^\z1$"
  syn region shHereDoc matchgroup=shRedir start="<<\s*\\\_$\_s*\"\z(\S*\)\""	matchgroup=shRedir end="^\z1$"
  syn region shHereDoc matchgroup=shRedir start="<<-\s*\\\_$\_s*\"\z(\S*\)\""	matchgroup=shRedir end="^\s*\z1$"
 endif
 if v:version > 602 || (v:version == 602 && has("patch219"))
  syn match  shHerePayload "^.*$"	contained	skipnl	nextgroup=shHerePayload	contains=@shDblQuoteList
  syn match  shBeginLine ".*$"		contained	skipnl	nextgroup=shHerePayload	contains=@shCommandSubList
  syn match  shBeginHere "<<-\=\s*\S\+"	contained		nextgroup=shBeginLine
 endif
 if exists("b:is_bash")
  syn match shRedir "<<<"
 endif
endif

" Identifiers
"============
syn match  shVariable "\<\([bwglsav]:\)\=[a-zA-Z0-9.!@_%+,]*\ze="	nextgroup=shSetIdentifier
syn match  shIdWhiteSpace  contained	"\s"
syn match  shSetIdentifier contained	"="	nextgroup=shPattern,shDeref,shDerefSimple,shDoubleQuote,shSingleQuote
if exists("b:is_bash")
  syn region shSetList matchgroup=shSet start="\<\(declare\|typeset\|local\|export\|unset\)\>[^/]"me=e-1 end="$" matchgroup=shOperator end="[;&]"me=e-1 matchgroup=NONE end="#\|="me=e-1 contains=@shIdList
  syn region shSetList matchgroup=shSet start="\<set\>[^/]"me=e-1 end="$" end="[|)]"me=e-1		 matchgroup=shOperator end="[;&]"me=e-1 matchgroup=NONE end="[#=]"me=e-1 contains=@shIdList
  syn match  shSet "\<\(declare\|typeset\|local\|export\|set\|unset\)$"
elseif exists("b:is_kornshell")
  syn region shSetList matchgroup=shSet start="\<\(typeset\|export\|unset\)\>[^/]"me=e-1 end="$"	matchgroup=shOperator end="[;&]"me=e-1 matchgroup=NONE end="[#=]"me=e-1 contains=@shIdList
  syn region shSetList matchgroup=shSet start="\<set\>[^/]"me=e-1 end="$"			matchgroup=shOperator end="[;&]"me=e-1 matchgroup=NONE end="[#=]"me=e-1 contains=@shIdList
  syn match  shSet "\<\(typeset\|set\|export\|unset\)$"
else
  syn region shSetList matchgroup=shSet start="\<\(set\|export\|unset\)\>[^/]"me=e-1 end="$" end="[)|]"me=e-1	matchgroup=shOperator end="[;&]" matchgroup=NONE end="[#=]"me=e-1 contains=@shIdList
  syn match  shStatement "\<\(set\|export\|unset\)$"
endif

" handles functions which start:  Function () {
"   Apparently Bourne shell accepts functions too,
"   even though it isn't documented by my man pages
"   for it.
syn cluster shCommandSubList  add=bkshFunction
if g:sh_fold_enabled
 syn region bkshFunctionBody	transparent fold	matchgroup=Delimiter start="^\s*\<\h\w*\>\s*()\s*{" end="}" contains=bkshFunction,bkshFunctionDelim,@shFunctionList
else
 syn region bkshFunctionBody	transparent matchgroup=Delimiter start="^\s*\<\h\w*\>\s*()\s*{" end="}" contains=bkshFunction,bkshFunctionDelim,@shFunctionList
endif
syn match bkshFunction	"^\s*\<\h\w*\>\s*()"	skipwhite skipnl contains=bkshFunctionParen
syn match bkshFunctionParen	"[()]"	contained
syn match bkshFunctionDelim	"[{}]"	contained

" Parameter Dereferencing
" =======================
syn match  shDerefSimple	"\$\w\+"
syn region shDeref	matchgroup=PreProc start="\${" end="}"	contains=@shDerefList,shDerefVarArray
syn match  shDerefWordError	"[^}$[]"	contained
syn match  shDerefSimple	"\$[-#*@!?]"
syn match  shDerefSimple	"\$\$"
if exists("b:is_bash") || exists("b:is_kornshell")
 syn region shDeref	matchgroup=PreProc start="\${##\=" end="}"	contains=@shDerefList
endif

" bash : ${!prefix*}
" bash : ${#parameter}
if exists("b:is_bash")
 syn region shDeref	matchgroup=PreProc start="\${!" end="\*\=}"	contains=@shDerefList,shDerefOp
 syn match  shDerefVar	contained	"{\@<=!\w\+"		nextgroup=@shDerefVarList
endif

syn match  shDerefSpecial	contained	"{\@<=[-*@?0]"		nextgroup=shDerefOp,shDerefOpError
syn match  shDerefSpecial	contained	"\({[#!]\)\@<=[[:alnum:]*@_]\+"	nextgroup=@shDerefVarList,shDerefOp
syn match  shDerefVar	contained	"{\@<=\w\+"		nextgroup=@shDerefVarList

" sh ksh bash : ${var[... ]...}  array reference
syn region  shDerefVarArray   contained	matchgroup=shDeref start="\[" end="]"	contains=@shCommandSubList nextgroup=shDerefOp,shDerefOpError

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
 syn match  shDerefPattern	contained	"[^{}]\+"	contains=shDeref,shDerefSimple,shDerefPattern,shDerefString,shCommandSub nextgroup=shDerefPattern
 syn region shDerefPattern	contained	start="{" end="}"	contains=shDeref,shDerefSimple,shDerefString,shCommandSub nextgroup=shDerefPattern
endif
syn region shDerefString	contained	matchgroup=shOperator start=+'+ end=+'+		contains=shStringSpecial
syn region shDerefString	contained	matchgroup=shOperator start=+"+ skip=+\\"+ end=+"+	contains=@shDblQuoteList,shStringSpecial
syn match  shDerefString	contained	"\\["']"

"	bash : ${parameter:offset}
"	bash : ${parameter:offset:length}
"	bash : ${parameter//pattern/string}
"	bash : ${parameter//pattern}
if exists("b:is_bash")
 syn region shDerefOp	contained	start=":[$[:alnum:]_]"me=e-1 end=":"me=e-1 end="}"me=e-1 contains=@shCommandSubList nextgroup=shDerefPOL
 syn match  shDerefPOL	contained	":[^}]\{1,}"	contains=@shCommandSubList
 syn match  shDerefOp	contained	"/\{1,2}"	nextgroup=shDerefPat
 syn match  shDerefPat	contained	"[^/}]\{1,}"	nextgroup=shDerefPatStringOp
 syn match  shDerefPatStringOp	contained	"/"	nextgroup=shDerefPatString
 syn match  shDerefPatString	contained	"[^}]\{1,}"
endif

" A bunch of useful sh keywords
syn keyword shStatement break cd chdir continue eval exec exit kill newgrp pwd read readonly return shift test trap ulimit umask wait
syn keyword shConditional contained elif else then
syn keyword shCondError elif else then

if exists("b:is_kornshell") || exists("b:is_bash")
 syn keyword shFunction	function
 syn keyword shStatement autoload bg false fc fg functions getopts hash history integer jobs let nohup print printf r stop suspend time times true type unalias whence

 if exists("b:is_bash")
  syn keyword shStatement bind builtin dirs disown enable help local logout popd pushd shopt source
 else
  syn keyword shStatement login newgrp
 endif
endif

" Syncs
" =====
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

" The default highlighting.
hi def link shArithRegion	shShellVariables
hi def link shCaseBar	shConditional
hi def link shCaseIn	shConditional
hi def link shCaseCommandSub	shCommandSub
hi def link shCaseDoubleQuote	shDoubleQuote
hi def link shCaseSingleQuote	shSingleQuote
hi def link shCaseStart	shConditional
hi def link shCmdSubRegion	shShellVariables
hi def link shColon	shStatement

hi def link shDeref	shShellVariables
hi def link shDerefOp	shOperator

hi def link shDerefVar	shDeref
hi def link shDerefPOL	shDerefOp
hi def link shDerefPatString	shDerefPattern
hi def link shDerefPatStringOp	shDerefOp
hi def link shDerefSimple	shDeref
hi def link shDerefSpecial	shDeref
hi def link shDerefString	shDoubleQuote
hi def link shHerePayload	shHereDoc
hi def link shBeginHere	shRedir

hi def link shDoubleQuote	shString
hi def link shEcho	shString
hi def link shEmbeddedEcho	shString
hi def link shHereDoc	shString
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
hi def link shVariable	shSetList
hi def link shWrapLineOperator	shOperator

if exists("b:is_bash")
  hi def link bashAdminStatement	shStatement
  hi def link bashSpecialVariables	shShellVariables
  hi def link bashStatement		shStatement
  hi def link bkshFunction		Function
  hi def link bkshFunctionParen		Delimiter
  hi def link bkshFunctionDelim		Delimiter
endif
if exists("b:is_kornshell")
  hi def link kshSpecialVariables	shShellVariables
  hi def link kshStatement		shStatement
  hi def link bkshFunction		Function
  hi def link bkshFunctionParen		Delimiter
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
hi def link shExprRegion		Delimiter
hi def link shFunction		Function
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

" Current Syntax
" ==============
if exists("b:is_bash")
 let b:current_syntax = "bash"
elseif exists("b:is_kornshell")
 let b:current_syntax = "ksh"
else
 let b:current_syntax = "sh"
endif

" vim: ts=16
