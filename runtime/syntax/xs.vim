" Vim syntax file
" Language:	XS (Perl extension interface language)
" Maintainer:	Michael W. Dodge <sarge@pobox.com>
" Last Change:	2001 May 09

" For version 5.x: Clear all syntax items
" For version 6.x: Quit when a syntax file was already loaded
if version < 600
  syntax clear
elseif exists("b:current_syntax")
  finish
endif

" Read the C syntax to start with
if version < 600
  source <sfile>:p:h/c.vim
else
  runtime! syntax/c.vim
endif

" XS extentions
" TODO: Figure out how to look for trailing '='.
syn keyword xsKeyword	MODULE PACKAGE PREFIX
syn keyword xsKeyword	OUTPUT: CODE: INIT: PREINIT: INPUT:
syn keyword xsKeyword	PPCODE: REQUIRE: CLEANUP: BOOT:
syn keyword xsKeyword	VERSIONCHECK: PROTOTYPES: PROTOTYPE:
syn keyword xsKeyword	ALIAS: INCLUDE: CASE:
" TODO: Figure out how to look for trailing '('.
syn keyword xsMacro	SV EXTEND PUSHs
syn keyword xsVariable	RETVAL NO_INIT
"syn match xsCast	"\<\(const\|static\|dynamic\|reinterpret\)_cast\s*<"me=e-1
"syn match xsCast	"\<\(const\|static\|dynamic\|reinterpret\)_cast\s*$"

" Define the default highlighting.
" For version 5.7 and earlier: only when not done already
" For version 5.8 and later: only when an item doesn't have highlighting yet
if version >= 508 || !exists("did_xs_syntax_inits")
  if version < 508
    let did_xs_syntax_inits = 1
    command -nargs=+ HiLink hi link <args>
  else
    command -nargs=+ HiLink hi def link <args>
  endif

  HiLink xsKeyword	Keyword
  HiLink xsMacro	Macro
  HiLink xsVariable	Identifier

  delcommand HiLink
endif

let b:current_syntax = "xs"

" vim: ts=8
