" Vim syntax file
" Language:         ConTeXt typesetting engine
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-07-04

if exists("b:current_syntax")
  finish
endif

let s:cpo_save = &cpo
set cpo&vim

syn keyword contextTodo       TODO FIXME XXX NOTE

syn region  contextComment    display oneline start='%' end='$'
                              \ contains=contextTodo
syn region  contextComment    display oneline start='^\s*%[CDM]' end='$'
                              \ contains=ALL

syn match   contextStatement  display '\\[a-zA-Z@]\+' contains=@NoSpell

syn match   contextBlockDelim display '\\\%(start\|stop\)\a\+'
                              \ contains=@NoSpell

syn match   contextDelimiter  '[][{}]'

syn match   contextEscaped    display '\\\_[\{}|&%$ ]'
syn region  contextEscaped    display matchgroup=contextPreProc
                              \ start='\\type\z(\A\)' end='\z1'
syn region  contextEscaped    display matchgroup=contextPreProc
                              \ start='\\type\={' end='}'
syn region  contextEscaped    display matchgroup=contextPreProc
                              \ start='\\type\=<<' end='>>'
syn region  contextEscaped    matchgroup=contextPreProc
                              \ start='\\start\z(\a*\%(typing\|typen\)\)'
                              \ end='\\stop\z1'
syn region  contextEscaped    display matchgroup=contextPreProc
                              \ start='\\\h\+Type{' end='}'
syn region  contextEscaped    display matchgroup=contextPreProc
                              \ start='\\Typed\h\+{' end='}'

"syn region  contextMath       matchgroup=contextMath start='\$' end='\$'
"                              \ contains=contextStatement

syn match   contextBuiltin    '\\\%(newif\|def\|gdef\|global\|let\|glet\|bgroup\)\>'
                              \ contains=@NoSpell
syn match   contextBuiltin    '\\\%(begingroup\|egroup\|endgroup\|long\|catcode\)\>'
                              \ contains=@NoSpell
syn match   contextBuiltin    '\\\%(unprotect\|unexpanded\|if\|else\|fi\|ifx\)\>'
                              \ contains=@NoSpell
syn match   contextBuiltin    '\\\%(futurelet\|protect\)\>' contains=@NoSpell
syn match   contextBuiltin    '\\\%([lr]q\)\>' contains=@NoSpell

syn match   contextPreProc    '^\s*\\\%(start\|stop\)\=\%(component\|environment\|project\|product\).*$'
                              \ contains=@NoSpell
syn match   contextPreProc    '^\s*\\input\s\+.*$' contains=@NoSpell

syn match   contextSectioning '\\chapter\>' contains=@NoSpell
syn match   contextSectioning '\\\%(sub\)*section\>' contains=@NoSpell

syn match   contextSpecial    '\\crlf\>\|\\par\>\|-\{2,3}\||[<>/]\=|'
                              \ contains=@NoSpell
syn match   contextSpecial    '\\[`'"]'
syn match   contextSpecial    +\\char\%(\d\{1,3}\|'\o\{1,3}\|"\x\{1,2}\)\>+
                              \ contains=@NoSpell
syn match   contextSpecial    '\^\^.'
syn match   contextSpecial    '`\%(\\.\|\^\^.\|.\)'

syn match   contextStyle      '\\\%(em\|tt\|rm\|ss\|hw\|cg\)\>'
                              \ contains=@NoSpell
syn match   contextFont       '\\\%(CAP\|Cap\|cap\|Caps\|kap\|nocap\)\>'
                              \ contains=@NoSpell
syn match   contextFont       '\\\%(Word\|WORD\|Words\|WORDS\)\>'
                              \ contains=@NoSpell
syn match   contextFont       '\\\%(vi\{1,3}\|ix\|xi\{0,2}\)\>'
                              \ contains=@NoSpell
syn match   contextFont       '\\\%(tf[abcdx]\|bfx\|[is]lx\)\>'
                              \ contains=@NoSpell
syn match   contextFont       '\\\%(b[fsi]\|s[cl]\|it\|os\|mf\)\>'
                              \ contains=@NoSpell

syn match   contextDimension  '[+-]\=\s*\%(\d\+\%([.,]\d*\)\=\|[.,]\d\+\)\s*\%(true\)\=\s*\%(p[tc]\|in\|bp\|c[mc]\|mm\|dd\|sp\|e[mx]\)\>'
                              \ contains=@NoSpell

hi def link contextTodo       Todo
hi def link contextComment    Comment
hi def link contextEscaped    Special
hi def link contextStatement  Identifier
hi def link contextMath       String
hi def link contextBlockDelim Keyword
hi def link contextBuiltin    Keyword
hi def link contextDelimiter  Delimiter
hi def link contextPreProc    PreProc
hi def link contextSectioning PreProc
hi def link contextSpecial    Special
hi def link contextStyle      contextType
hi def link contextFont       contextType
hi def link contextType       Type
hi def link contextDimension  Number

let b:current_syntax = "context"

let &cpo = s:cpo_save
unlet s:cpo_save
