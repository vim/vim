" Vim syntax file
" Language:	Model
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2001 Apr 25

" very basic things only (based on the vgrindefs file).
" If you use this language, please improve it, and send me the patches!

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" A bunch of keywords
syn keyword modelKeyword abs and array boolean by case cdnl char copied dispose
syn keyword modelKeyword div do dynamic else elsif end entry external FALSE false
syn keyword modelKeyword fi file for formal fortran global if iff ift in integer include
syn keyword modelKeyword inline is lbnd max min mod new NIL nil noresult not notin od of
syn keyword modelKeyword or procedure public read readln readonly record recursive rem rep
syn keyword modelKeyword repeat res result return set space string subscript such then TRUE
syn keyword modelKeyword true type ubnd union until varies while width

" Special keywords
syn keyword modelBlock beginproc endproc

" Comments
syn region modelComment start="\$" end="\$" end="$"

" Strings
syn region modelString start=+"+ end=+"+

" Character constant (is this right?)
syn match modelString "'."

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_model_syntax_inits")
  if version < 508
    let did_model_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink modelKeyword	Statement
  HiLink modelBlock	PreProc
  HiLink modelComment	Comment
  HiLink modelString	String

  delcommand HiLink
endif

let b:current_syntax = "model"

" vim: ts=8 sw=2
