" Vim filetype plugin file.
" Language:	Lua 4.0+
" Maintainer:	Max Ischenko <mfi@ukr.net>
" Last Change:	2001 Sep 17

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

" Set 'formatoptions' to break comment lines but not other lines, and insert
" the comment leader when hitting <CR> or using "o".
setlocal fo-=t fo+=croql

setlocal com=:--
setlocal cms="--%s"
setlocal suffixesadd=.lua


" The following lines enable the macros/matchit.vim plugin for
" extended matching with the % key.

set cpo-=C
if exists("loaded_matchit")

  let b:match_ignorecase = 0
  let b:match_words =
    \ '\<\%(do\|function\|if\)\>:' .
    \ '\<\%(return\|else\|elseif\)\>:' .
    \ '\<end\>,' .
    \ '\<repeat\>:\<until\>'

endif " exists("loaded_matchit")
