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
  let b:match_words = '^\s\+\.if\>:^\s\+\.elseif:^\s\+\.else\>:^\s\+\.endif\>,^\s\+\.group:^\s\+\.gmember:^\s\+\.endgroup,^\s\+\.loop:^\s\+\.break:^\s\+\.endloop,^\s\+\.macro:^\s\+\.mexit:^\s\+\.endm,^\s\+\.asmfunc:^\s\+\.endasmfunc,^\s\+\.c\?struct:^\s\+\.endstruct,^\s\+\.c\?union:^\s\+\.endunion,^\s\+\.c\?proc:^\s\+\.return:^\s\+\.endproc'
  let b:match_ignorecase = 1
  let b:undo_ftplugin ..= " | unlet! b:match_ignorecase b:match_words"
endif
