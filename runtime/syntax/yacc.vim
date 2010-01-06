" Vim syntax file
" Language:	Yacc
" Maintainer:	Charles E. Campbell, Jr. <NdrOchipS@PcampbellAfamily.Mbiz>
" Last Change:	Oct 21, 2008
" Version:	7
" URL:	http://mysite.verizon.net/astronaut/vim/index.html#vimlinks_syntax
"
" Options: {{{1
"   g:yacc_uses_cpp : if this variable exists, then C++ is loaded rather than C

" ---------------------------------------------------------------------
" this version of syntax/yacc.vim requires 6.0 or later
if version < 600
 finish
endif
if exists("b:current_syntax")
 syntax clear
endif

" ---------------------------------------------------------------------
"  Folding Support {{{1
if has("folding")
 com! -nargs=+ HiFold	<args> fold
else
 com! -nargs=+ HiFold	<args>
endif

" ---------------------------------------------------------------------
" Read the C syntax to start with {{{1
if exists("g:yacc_uses_cpp")
 syn include @yaccCode	<sfile>:p:h/cpp.vim
else
 syn include @yaccCode	<sfile>:p:h/c.vim
endif

" ---------------------------------------------------------------------
"  Yacc Clusters: {{{1
syn cluster yaccInitCluster	contains=yaccKey,yaccKeyActn,yaccBrkt,yaccType,yaccString,yaccUnionStart,yaccHeader2,yaccComment
syn cluster yaccRulesCluster	contains=yaccNonterminal,yaccString

" ---------------------------------------------------------------------
"  Yacc Sections: {{{1
HiFold syn	region	yaccInit	start='.'ms=s-1,rs=s-1	matchgroup=yaccSectionSep	end='^%%$'me=e-2,re=e-2	contains=@yaccInitCluster	nextgroup=yaccRules	skipwhite skipempty contained
HiFold syn	region	yaccInit2      start='\%^.'ms=s-1,rs=s-1	matchgroup=yaccSectionSep	end='^%%$'me=e-2,re=e-2	contains=@yaccInitCluster	nextgroup=yaccRules	skipwhite skipempty
HiFold syn	region	yaccHeader2	matchgroup=yaccSep	start="^\s*\zs%{"	end="^\s*%}"		contains=@yaccCode	nextgroup=yaccInit	skipwhite skipempty contained
HiFold syn	region	yaccHeader	matchgroup=yaccSep	start="^\s*\zs%{"	end="^\s*%}"		contains=@yaccCode	nextgroup=yaccInit	skipwhite skipempty
HiFold syn	region	yaccRules	matchgroup=yaccSectionSep	start='^%%$'		end='^%%$'me=e-2,re=e-2	contains=@yaccRulesCluster	nextgroup=yaccEndCode	skipwhite skipempty contained
HiFold syn	region	yaccEndCode	matchgroup=yaccSectionSep	start='^%%$'		end='\%$'		contains=@yaccCode	contained

" ---------------------------------------------------------------------
" Yacc Commands: {{{1
syn	match	yaccDelim	"[:|]"	contained
syn	match	yaccOper	"@\d\+"	contained

syn	match	yaccKey	"^\s*%\(token\|type\|left\|right\|start\|ident\|nonassoc\)\>"	contained
syn	match	yaccKey	"\s%\(prec\|expect\)\>"	contained
syn	match	yaccKey	"\$\(<[a-zA-Z_][a-zA-Z_0-9]*>\)\=[\$0-9]\+"	contained
syn	keyword	yaccKeyActn	yyerrok yyclearin	contained

syn	match	yaccUnionStart	"^%union"	skipwhite skipnl nextgroup=yaccUnion	contained
HiFold syn	region	yaccUnion	matchgroup=yaccCurly start="{" matchgroup=yaccCurly end="}" contains=@yaccCode	contained
syn	match	yaccBrkt	"[<>]"	contained
syn	match	yaccType	"<[a-zA-Z_][a-zA-Z0-9_]*>"	contains=yaccBrkt	contained

HiFold syn	region	yaccNonterminal	start="^\s*\a\w*\ze\_s*\(/\*\_.\{-}\*/\)\=\_s*:"	matchgroup=yaccDelim end=";"	matchgroup=yaccSectionSep end='^%%$'me=e-2,re=e-2 contains=yaccAction,yaccDelim,yaccString,yaccComment	contained
syn	region	yaccComment	start="/\*"	end="\*/"
syn	match	yaccString	"'[^']*'"	contained


" ---------------------------------------------------------------------
" I'd really like to highlight just the outer {}.  Any suggestions??? {{{1
syn	match	yaccCurlyError	"[{}]"
HiFold syn	region	yaccAction	matchgroup=yaccCurly start="{" end="}" contains=@yaccCode	contained

" ---------------------------------------------------------------------
" Yacc synchronization: {{{1
syn sync fromstart

" ---------------------------------------------------------------------
" Define the default highlighting. {{{1
if !exists("did_yacc_syn_inits")
  command -nargs=+ HiLink hi def link <args>

  " Internal yacc highlighting links {{{2
  HiLink yaccBrkt	yaccStmt
  HiLink yaccKey	yaccStmt
  HiLink yaccOper	yaccStmt
  HiLink yaccUnionStart	yaccKey

  " External yacc highlighting links {{{2
  HiLink yaccComment	Comment
  HiLink yaccCurly	Delimiter
  HiLink yaccCurlyError	Error
  HiLink yaccNonterminal	Function
  HiLink yaccDelim	Delimiter
  HiLink yaccKeyActn	Special
  HiLink yaccSectionSep	Todo
  HiLink yaccSep	Delimiter
  HiLink yaccString	String
  HiLink yaccStmt	Statement
  HiLink yaccType	Type

  " since Bram doesn't like my Delimiter :| {{{2
  HiLink Delimiter	Type

  delcommand HiLink
endif

" ---------------------------------------------------------------------
"  Cleanup: {{{1
delcommand HiFold
let b:current_syntax = "yacc"

" ---------------------------------------------------------------------
"  Modelines: {{{1
" vim: ts=15 fdm=marker
