" Vim syntax file
" Language:	Diff (context or unified)
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2003 Apr 02

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

syn match diffOnly	"^Only in .*"
syn match diffIdentical	"^Files .* and .* are identical$"
syn match diffDiffer	"^Files .* and .* differ$"
syn match diffBDiffer	"^Binary files .* and .* differ$"
syn match diffIsA	"^File .* is a .* while file .* is a .*"
syn match diffNoEOL	"^No newline at end of file .*"
syn match diffCommon	"^Common subdirectories: .*"

syn match diffRemoved	"^-.*"
syn match diffRemoved	"^<.*"
syn match diffAdded	"^+.*"
syn match diffAdded	"^>.*"
syn match diffChanged	"^! .*"

syn match diffSubname	" @@..*"ms=s+3 contained
syn match diffLine	"^@.*" contains=diffSubname
syn match diffLine	"^\<\d\+\>.*"
syn match diffLine	"^\*\*\*\*.*"

"Some versions of diff have lines like "#c#" and "#d#" (where # is a number)
syn match diffLine	"^\d\+\(,\d\+\)\=[cda]\d\+\>.*"

syn match diffFile	"^diff.*"
syn match diffFile	"^+++ .*"
syn match diffFile	"^Index: .*$"
syn match diffFile	"^==== .*$"
syn match diffOldFile	"^\*\*\* .*"
syn match diffNewFile	"^--- .*"

syn match diffComment	"^#.*"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_diff_syntax_inits")
  if version < 508
    let did_diff_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink diffOldFile	diffFile
  HiLink diffNewFile	diffFile
  HiLink diffFile	Type
  HiLink diffOnly	Constant
  HiLink diffIdentical	Constant
  HiLink diffDiffer	Constant
  HiLink diffBDiffer	Constant
  HiLink diffIsA	Constant
  HiLink diffNoEOL	Constant
  HiLink diffCommon	Constant
  HiLink diffRemoved	Special
  HiLink diffChanged	PreProc
  HiLink diffAdded	Identifier
  HiLink diffLine	Statement
  HiLink diffSubname	PreProc
  HiLink diffComment	Comment

  delcommand HiLink
endif

let b:current_syntax = "diff"

" vim: ts=8 sw=2
