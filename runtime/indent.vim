" Vim support file to switch on loading indent files for file types
"
" Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2001 Sep 12

if exists("did_indent_on")
  finish
endif
let did_indent_on = 1

augroup filetypeindent
  au FileType * if expand("<amatch>") != "" | if exists("b:did_indent") | unlet b:did_indent | endif | runtime! indent/<amatch>.vim | endif
augroup END
