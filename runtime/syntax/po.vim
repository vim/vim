" Vim syntax file
" Language:	po (gettext)
" Maintainer:	Nam SungHyun <namsh@kldp.org>
" Last Change:	2001 Apr 26

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn match  poComment	"^#.*$"
syn match  poSources	"^#:.*$"
syn match  poStatement	"^\(domain\|msgid\|msgstr\)"
syn match  poSpecial	contained "\\\(x\x\+\|\o\{1,3}\|.\|$\)"
syn match  poFormat	"%\(\d\+\$\)\=[-+' #0*]*\(\d*\|\*\|\*\d\+\$\)\(\.\(\d*\|\*\|\*\d\+\$\)\)\=\([hlL]\|ll\)\=\([diuoxXfeEgGcCsSpn]\|\[\^\=.[^]]*\]\)" contained
syn match  poFormat	"%%" contained
syn region poString	start=+"+ skip=+\\\\\|\\"+ end=+"+
			\ contains=poSpecial,poFormat

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_po_syn_inits")
  if version < 508
    let did_po_syn_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink poComment	Comment
  HiLink poSources	PreProc
  HiLink poStatement	Statement
  HiLink poSpecial	Special
  HiLink poFormat	poSpecial
  HiLink poString	String

  delcommand HiLink
endif

let b:current_syntax = "po"

" vim:set ts=8 sts=2 sw=2 noet:
