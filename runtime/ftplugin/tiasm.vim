" Vim filetype plugin file
" Language:	TI linear assembly language
" Maintainer:	Wu, Zhenyu <wuzhenyu@ustc.edu>
" Last Change:	2024 Oct 22

if exists("b:did_ftplugin") | finish | endif
let b:did_ftplugin = 1

setlocal comments=:;
setlocal commentstring=;\ %s

let b:undo_ftplugin = "setl commentstring< comments<"

if !exists("b:match_words")
  let b:match_words = '^\s*\.if\>:^\s*\.else\>:^\s*\.endif\>'
  let b:match_ignorecase = 1
  let b:undo_ftplugin ..= " | unlet! b:match_ignorecase b:match_words"
endif
