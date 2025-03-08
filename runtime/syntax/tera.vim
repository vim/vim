" Vim syntax file
" Language:	Tera
" Maintainer:	Muntasir Mahmud <muntasir.joypurhat@gmail.com>
" Last Change:	2025 Mar 08

if exists("b:current_syntax")
  finish
endif

" Detect the underlying language based on filename pattern
" For files like file.html.tera, we want to load html syntax
let s:filename = expand("%:t")
let s:dotpos = strridx(s:filename, '.', strridx(s:filename, '.tera') - 1)
let s:underlying_filetype = ""

if s:dotpos != -1
  let s:underlying_ext = s:filename[s:dotpos+1:strridx(s:filename, '.tera')-1]
  if s:underlying_ext != "" && s:underlying_ext != "tera"
    let s:underlying_filetype = s:underlying_ext
  endif
endif

" Load the underlying language syntax if detected
if s:underlying_filetype != ""
  execute "runtime! syntax/" . s:underlying_filetype . ".vim"
  unlet! b:current_syntax
else
  " Default to HTML if no specific language detected
  runtime! syntax/html.vim
  unlet! b:current_syntax
endif

" Define syntax items
syn region teraCommentBlock start="{#" end="#}" contains=@Spell
syn region teraRaw start="{%" end="%}" contains=teraKeyword,teraIdentifier,teraString,teraNumber,teraOperator,teraFunction,teraBoolean,teraFilter
syn region teraExpression start="{{" end="}}" contains=teraIdentifier,teraString,teraNumber,teraOperator,teraFunction,teraBoolean,teraFilter

" Keywords for control structures
syn keyword teraKeyword contained if else elif endif for endfor in macro endmacro block endblock extends include import set endset break continue filter endfilter raw endraw with endwith

" Operators
syn match teraOperator contained "==\|!=\|>=\|<=\|>\|<\|+\|-\|*\|/\|%\|and\|or\|not\|is\|as"

" Functions and Identifiers
syn match teraFunction contained "\<\w\+\ze("
syn match teraIdentifier contained "\<\w\+\>"

" Filters
syn match teraFilter contained "|\_s*\w\+"

" String literals
syn region teraString contained start=+"+ skip=+\\"+ end=+"+ contains=@Spell
syn region teraString contained start=+'+ skip=+\\'+ end=+'+ contains=@Spell

" Number literals
syn match teraNumber contained "\<\d\+\>"
syn match teraNumber contained "\<\d\+\.\d\+\>"

" Boolean values
syn keyword teraBoolean contained true false

" Highlighting links
hi def link teraCommentBlock Comment
hi def link teraKeyword Statement
hi def link teraOperator Operator
hi def link teraFunction Function
hi def link teraIdentifier Identifier
hi def link teraString String
hi def link teraNumber Number
hi def link teraBoolean Boolean
hi def link teraFilter PreProc

" Special highlighting for raw blocks and expressions
hi def link teraRaw PreProc
hi def link teraExpression PreProc

let b:current_syntax = "tera"
