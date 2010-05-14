" Vim syntax file
" Language:     XS (Perl extension interface language)
" Maintainer:   Andy Lester <andy@petdance.com>
" URL:          http://github.com/petdance/vim-perl
" Last Change:  2009-08-14

" Quit when a syntax file was already loaded
if exists("b:current_syntax")
    finish
endif

" Read the C syntax to start with
runtime! syntax/c.vim

" XS extentions
" TODO: Figure out how to look for trailing '='.
syn keyword xsKeyword   MODULE PACKAGE PREFIX
syn keyword xsKeyword   OUTPUT: CODE: INIT: PREINIT: INPUT:
syn keyword xsKeyword   PPCODE: REQUIRE: CLEANUP: BOOT:
syn keyword xsKeyword   VERSIONCHECK: PROTOTYPES: PROTOTYPE:
syn keyword xsKeyword   ALIAS: INCLUDE: CASE:
" TODO: Figure out how to look for trailing '('.
syn keyword xsMacro     SV EXTEND PUSHs
syn keyword xsVariable  RETVAL NO_INIT
"syn match xsCast       "\<\(const\|static\|dynamic\|reinterpret\)_cast\s*<"me=e-1
"syn match xsCast       "\<\(const\|static\|dynamic\|reinterpret\)_cast\s*$"

" Define the default highlighting, but only when an item doesn't have highlighting yet
command -nargs=+ HiLink hi def link <args>

HiLink xsKeyword    Keyword
HiLink xsMacro      Macro
HiLink xsVariable   Identifier

delcommand HiLink

let b:current_syntax = "xs"

" vim: ts=8
