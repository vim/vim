" Vim syntax file
" Language:	generic configure file
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2001 Apr 25

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn keyword	confTodo	contained TODO FIXME XXX
" Avoid matching "text#text", used in /etc/disktab and /etc/gettytab
syn match	confComment	"^#.*" contains=confTodo
syn match	confComment	"\s#.*"ms=s+1 contains=confTodo
syn region	confString	start=+"+ skip=+\\\\\|\\"+ end=+"+ oneline
syn region	confString	start=+'+ skip=+\\\\\|\\'+ end=+'+ oneline

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_conf_syntax_inits")
  if version < 508
    let did_conf_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink confComment	Comment
  HiLink confTodo	Todo
  HiLink confString	String

  delcommand HiLink
endif

let b:current_syntax = "conf"

" vim: ts=8 sw=2
