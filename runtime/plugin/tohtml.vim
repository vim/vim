" Vim plugin for converting a syntax highlighted file to HTML.
" Maintainer: Bram Moolenaar <Bram@vim.org>
" Last Change: 2010 Jul 11
"
" The core of the code is in $VIMRUNTIME/autoload/tohtml.vim

" Define the :TOhtml command when:
" - 'compatible' is not set
" - this plugin was not already loaded
" - user commands are available.
if !&cp && !exists(":TOhtml") && has("user_commands")
  command -range=% TOhtml :call tohtml#Convert2HTML(<line1>, <line2>)
endif
