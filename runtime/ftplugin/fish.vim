" Vim filetype plugin file
" Language:     fish
" Maintainer:   Nicholas Boyle (github.com/nickeb96)
" Repository:   https://github.com/nickeb96/fish.vim
" Last Change:  February 1, 2023
"               2023 Aug 28 by Vim Project (undo_ftplugin)
"               2024 May 23 by Riley Bruins <ribru17@gmail.com> ('commentstring')

if exists("b:did_ftplugin")
    finish
endif
let b:did_ftplugin = 1

setlocal iskeyword=@,48-57,_,192-255,-,.
setlocal comments=:#
setlocal commentstring=#\ %s
setlocal formatoptions+=crjq

let b:undo_ftplugin = "setl cms< com< fo< isk<"

" Define patterns for the matchit plugin
if exists("loaded_matchit") && !exists("b:match_words")
  let b:match_words =
      \ '\<\%(else\s\+\)\@<!if\>:\<else\s\+if\>:\<else\>:\<end\>,' .
      \ '\<switch\>:\<case\>:\<end\>,' .
      \ '\<\(begin\|function\|while\|for\)\>:\<end\>'
  let b:undo_ftplugin .= "|unlet! b:match_words"
endif
