" Vim plugin for converting a syntax highlighted file to HTML.
" Maintainer: Ben Fritz <fritzophrenic@gmail.com>
" Last Change: 2010 Aug 12
"
" The core of the code is in $VIMRUNTIME/autoload/tohtml.vim and
" $VIMRUNTIME/syntax/2html.vim
"
" TODO:
"   * Restore open/closed folds and cursor position after processing each file
"     with option not to restore for speed increase
"   * Add extra meta info (generation time, etc.)
"   * Tidy up so we can use strict doctype more?
"   * Implementation detail: add threshold for writing the lines to the html
"     buffer before we're done (5000 or so lines should do it)
"   * TODO comments for code cleanup scattered throughout

if exists('g:loaded_2html_plugin')
  finish
endif
let g:loaded_2html_plugin = 'vim7.3_v6'

" Define the :TOhtml command when:
" - 'compatible' is not set
" - this plugin was not already loaded
" - user commands are available.
if !&cp && !exists(":TOhtml") && has("user_commands")
  command -range=% TOhtml :call tohtml#Convert2HTML(<line1>, <line2>)
endif

" Make sure any patches will probably use consistent indent
"   vim: ts=8 sw=2 sts=2 noet
