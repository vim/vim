" Vim syntax file
" Language:	Vim .viminfo file
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2001 Apr 25

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" The lines that are NOT recognized
syn match viminfoError "^[^\t].*"

" The one-character one-liners that are recognized
syn match viminfoStatement "^[/&$@:?=%!<]"

" The two-character one-liners that are recognized
syn match viminfoStatement "^[-'>"]."
syn match viminfoStatement +^"".+
syn match viminfoStatement "^\~[/&]"
syn match viminfoStatement "^\~[hH]"
syn match viminfoStatement "^\~[mM][sS][lL][eE]\d\+\~\=[/&]"

syn match viminfoOption "^\*.*=" contains=viminfoOptionName
syn match viminfoOptionName "\*\a*"ms=s+1 contained

" Comments
syn match viminfoComment "^#.*"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_viminfo_syntax_inits")
  if version < 508
    let did_viminfo_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink viminfoComment		Comment
  HiLink viminfoError		Error
  HiLink viminfoStatement	Statement

  delcommand HiLink
endif

let b:current_syntax = "viminfo"

" vim: ts=8 sw=2
