" Vim plugin for converting a syntax highlighted file to HTML.
" Maintainer: Ben Fritz <fritzophrenic@gmail.com>
" Last Change: 2010 Aug 02
"
" The core of the code is in $VIMRUNTIME/autoload/tohtml.vim and
" $VIMRUNTIME/syntax/2html.vim
"
" TODO:
"   * Bug: error thrown when nowrapscan is set
"   * Diff mode with xhtml gives invalid markup
"   * Diff mode does not determine encoding
"   * Line number column has one character too few on empty lines
"     without CSS.
"   * Add extra meta info (generation time, etc.)
"   * Fix strict doctype for other options?
"   * TODO comments for code cleanup scattered throughout

if exists('g:loaded_2html_plugin')
  finish
endif
let g:loaded_2html_plugin = 'vim7.3_v3'

" Define the :TOhtml command when:
" - 'compatible' is not set
" - this plugin was not already loaded
" - user commands are available.
if !&cp && !exists(":TOhtml") && has("user_commands")
  command -range=% TOhtml :call tohtml#Convert2HTML(<line1>, <line2>)
endif

" Make sure any patches will probably use consistent indent
"   vim: ts=8 sw=2 sts=2 noet
