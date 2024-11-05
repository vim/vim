" Vim ftplugin file
" Language:	Idris2
" Last Change:	2024 Nov 05
" Maintainer:	Serhii Khoma <srghma@gmail.com>
" License:             Vim (see :h license)
" Repository:          https://github.com/ShinKage/idris2-nvim
"
" Based on ftplugin/idris2.vim from https://github.com/edwinb/idris2-vim

if exists("b:did_ftplugin")
  finish
endif

setlocal shiftwidth=2
setlocal tabstop=2
if exists('g:idris2#allow_tabchar') && g:idris2#allow_tabchar != 0
  setlocal noexpandtab
else
  setlocal expandtab
endif

setlocal comments=s1:{-,mb:-,ex:-},:\|\|\|,:--
setlocal commentstring=--\ %s
setlocal iskeyword+=?
setlocal wildignore+=*.ibc

let b:did_ftplugin = 1
