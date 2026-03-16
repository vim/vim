" Vim filetype plugin file
" Language:     fish
" Maintainer:   Nicholas Boyle (github.com/nickeb96)
" Repository:   https://github.com/nickeb96/fish.vim
" Last Change:  February 1, 2023
"               2023 Aug 28 by Vim Project (undo_ftplugin)
"               2024 May 23 by Riley Bruins <ribru17@gmail.com> ('commentstring')
"               2026 Mar 16 by Phạm Bình An (add matchit support)

if exists("b:did_ftplugin")
    finish
endif

let s:saved_cpo = &cpo
set cpo-=C

let b:did_ftplugin = 1

setlocal iskeyword=@,48-57,_,192-255,-,.
setlocal comments=:#
setlocal commentstring=#\ %s
setlocal formatoptions+=crjq

let b:undo_ftplugin = "setl cms< com< fo< isk<"

" Define patterns for the matchit plugin
if exists("loaded_matchit") && !exists("b:match_words")
  let b:match_words =
      \ '\<\%(else\s\+\)\@<!if\>:\<else\%(\s\+if\)\?\>:\<end\>,' ..
      \ '\<switch\>:\<case\>:\<end\>,' ..
      \ '\<\(begin\|function\|while\|for\)\>:\<end\>'
  let b:match_ignorecase = 0

  let b:undo_ftplugin .= "|unlet! b:match_words b:match_ignorecase"
endif

" Restore 'cpo' to its original value
let &cpo = s:saved_cpo
unlet s:saved_cpo
