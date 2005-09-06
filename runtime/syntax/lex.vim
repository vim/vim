" Vim syntax file
" Language:	Lex
" Maintainer:	Dr. Charles E. Campbell, Jr. <NdrOchipS@PcampbellAfamily.Mbiz>
" Last Change:	Sep 06, 2005
" Version:	7
" URL:	http://mysite.verizon.net/astronaut/vim/index.html#vimlinks_syntax
"
" Option:
"   lex_uses_cpp : if this variable exists, then C++ is loaded rather than C

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Read the C syntax to start with
if version >= 600
  if exists("lex_uses_cpp")
    runtime! syntax/cpp.vim
  else
    runtime! syntax/c.vim
  endif
  unlet b:current_syntax
else
  if exists("lex_uses_cpp")
    so <sfile>:p:h/cpp.vim
  else
    so <sfile>:p:h/c.vim
  endif
endif

" --- ========= ---
" --- Lex stuff ---
" --- ========= ---

"I'd prefer to use lex.* , but it doesn't handle forward definitions yet
syn cluster lexListGroup		contains=lexAbbrvBlock,lexAbbrv,lexAbbrv,lexAbbrvRegExp,lexInclude,lexPatBlock,lexPat,lexBrace,lexPatString,lexPatTag,lexPatTag,lexPatComment,lexPatCodeLine,lexMorePat,lexPatSep,lexSlashQuote,lexPatCode,cInParen,cUserLabel,cOctalZero,cCppSkip,cErrInBracket,cErrInParen,cOctalError,cCppOut2,cCommentStartError,cParenError
syn cluster lexListPatCodeGroup	contains=lexAbbrvBlock,lexAbbrv,lexAbbrv,lexAbbrvRegExp,lexInclude,lexPatBlock,lexPat,lexBrace,lexPatTag,lexPatTag,lexPatComment,lexPatCodeLine,lexMorePat,lexPatSep,lexSlashQuote,cInParen,cUserLabel,cOctalZero,cCppSkip,cErrInBracket,cErrInParen,cOctalError,cCppOut2,cCommentStartError,cParenError

" Abbreviations Section
syn region lexAbbrvBlock	start="^\(\h\+\s\|%{\)" end="^\ze%%$"	skipnl	nextgroup=lexPatBlock contains=lexAbbrv,lexInclude,lexAbbrvComment,lexStartState
syn match  lexAbbrv		"^\I\i*\s"me=e-1			skipwhite	contained nextgroup=lexAbbrvRegExp
syn match  lexAbbrv		"^%[sx]"					contained
syn match  lexAbbrvRegExp	"\s\S.*$"lc=1				contained nextgroup=lexAbbrv,lexInclude
syn region lexInclude	matchgroup=lexSep	start="^%{" end="%}"	contained	contains=ALLBUT,@lexListGroup
syn region lexAbbrvComment	start="^\s\+/\*"	end="\*/"			contains=@Spell
syn region lexStartState	matchgroup=lexAbbrv	start="^%\a\+"	end="$"			contained

"%% : Patterns {Actions}
syn region lexPatBlock	matchgroup=Todo	start="^%%$" matchgroup=Todo end="^%%$"	skipnl skipwhite contains=lexPat,lexPatTag,lexPatComment
syn region lexPat		start=+\S+ skip="\\\\\|\\."	end="\s"me=e-1	contained nextgroup=lexMorePat,lexPatSep contains=lexPatString,lexSlashQuote,lexBrace
syn region lexBrace	start="\[" skip=+\\\\\|\\+		end="]"		contained
syn region lexPatString	matchgroup=String start=+"+	skip=+\\\\\|\\"+	matchgroup=String end=+"+	contained
syn match  lexPatTag	"^<\I\i*\(,\I\i*\)*>*"			contained nextgroup=lexPat,lexPatTag,lexMorePat,lexPatSep
syn match  lexPatTag	+^<\I\i*\(,\I\i*\)*>*\(\\\\\)*\\"+		contained nextgroup=lexPat,lexPatTag,lexMorePat,lexPatSep
syn region lexPatComment	start="^\s*/\*" end="\*/"		skipnl	contained contains=cTodo skipwhite nextgroup=lexPatComment,lexPat,@Spell
syn match  lexPatCodeLine	".*$"					contained contains=ALLBUT,@lexListGroup
syn match  lexMorePat	"\s*|\s*$"			skipnl	contained nextgroup=lexPat,lexPatTag,lexPatComment
syn match  lexPatSep	"\s\+"					contained nextgroup=lexMorePat,lexPatCode,lexPatCodeLine
syn match  lexSlashQuote	+\(\\\\\)*\\"+				contained
syn region lexPatCode matchgroup=Delimiter start="{" matchgroup=Delimiter end="}"	skipnl contained contains=ALLBUT,@lexListPatCodeGroup

syn keyword lexCFunctions	BEGIN	input	unput	woutput	yyleng	yylook	yytext
syn keyword lexCFunctions	ECHO	output	winput	wunput	yyless	yymore	yywrap

" <c.vim> includes several ALLBUTs; these have to be treated so as to exclude lex* groups
syn cluster cParenGroup	add=lex.*
syn cluster cDefineGroup	add=lex.*
syn cluster cPreProcGroup	add=lex.*
syn cluster cMultiGroup	add=lex.*

" Synchronization
syn sync clear
syn sync minlines=300
syn sync match lexSyncPat	grouphere  lexPatBlock	"^%[a-zA-Z]"
syn sync match lexSyncPat	groupthere lexPatBlock	"^<$"
syn sync match lexSyncPat	groupthere lexPatBlock	"^%%$"

" The default highlighting.
hi def link lexSlashQuote	lexPat
hi def link lexBrace	lexPat
hi def link lexAbbrvComment	lexPatComment

hi def link lexAbbrvRegExp	Macro
hi def link lexAbbrv	SpecialChar
hi def link lexCFunctions	Function
hi def link lexMorePat	SpecialChar
hi def link lexPatComment	Comment
hi def link lexPat		Function
hi def link lexPatString	Function
hi def link lexPatTag	Special
hi def link lexSep		Delimiter
hi def link lexStartState	Statement

let b:current_syntax = "lex"

" vim:ts=10
