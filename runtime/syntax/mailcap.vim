" Vim syntax file
" Language:	Mailcap configuration file
" Maintainer:	Doug Kearns <djkea2@gus.gscit.monash.edu.au>
" Last Change:	2004 Nov 27
" URL:		http://gus.gscit.monash.edu.au/~djkea2/vim/syntax/mailcap.vim

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn match  mailcapComment	"^#.*"

syn region mailcapString	start=+"+ end=+"+ contains=mailcapSpecial oneline

syn match  mailcapDelimiter	"\\\@<!;"

syn match  mailcapSpecial	"\\\@<!%[nstF]"
syn match  mailcapSpecial	"\\\@<!%{[^}]*}"

syn case ignore
syn match  mailcapFlag		"\(=\s*\)\@<!\<\(needsterminal\|copiousoutput\|x-\w\+\)\>"
syn match  mailcapFieldname	"\<\(compose\|composetyped\|print\|edit\|test\|x11-bitmap\|nametemplate\|textualnewlines\|description\|x-\w+\)\>\ze\s*="
syn match  mailcapTypeField	"^\(text\|image\|audio\|video\|application\|message\|multipart\|model\|x-[[:graph:]]\+\)\(/\(\*\|[[:graph:]]\+\)\)\=\ze\s*;"
syn case match

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_mailcap_syntax_inits")
  if version < 508
    let did_mailcap_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink mailcapComment		Comment
  HiLink mailcapDelimiter	Delimiter
  HiLink mailcapFlag		Statement
  HiLink mailcapFieldname	Statement
  HiLink mailcapSpecial		Identifier
  HiLink mailcapTypeField	Type
  HiLink mailcapString		String

  delcommand HiLink
endif

let b:current_syntax = "mailcap"

" vim: tabstop=8
