" Vim syntax file
" Language:	Vgrindefs
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2001 Apr 25

" The Vgrindefs file is used to specify a language for vgrind

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Comments
syn match vgrindefsComment "^#.*"

" The fields that vgrind recognizes
syn match vgrindefsField ":ab="
syn match vgrindefsField ":ae="
syn match vgrindefsField ":pb="
syn match vgrindefsField ":bb="
syn match vgrindefsField ":be="
syn match vgrindefsField ":cb="
syn match vgrindefsField ":ce="
syn match vgrindefsField ":sb="
syn match vgrindefsField ":se="
syn match vgrindefsField ":lb="
syn match vgrindefsField ":le="
syn match vgrindefsField ":nc="
syn match vgrindefsField ":tl"
syn match vgrindefsField ":oc"
syn match vgrindefsField ":kw="

" Also find the ':' at the end of the line, so all ':' are highlighted
syn match vgrindefsField ":\\$"
syn match vgrindefsField ":$"
syn match vgrindefsField "\\$"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_vgrindefs_syntax_inits")
  if version < 508
    let did_vgrindefs_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  " The default methods for highlighting.  Can be overridden later
  HiLink vgrindefsField		Statement
  HiLink vgrindefsComment	Comment

  delcommand HiLink
endif

let b:current_syntax = "vgrindefs"

" vim: ts=8
