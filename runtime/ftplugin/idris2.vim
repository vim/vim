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
" Set g:idris2#allow_tabchar = 1 to use tabs instead of spaces
if exists('g:idris2#allow_tabchar') && g:idris2#allow_tabchar != 0
  setlocal noexpandtab
else
  setlocal expandtab
endif

setlocal comments=s1:{-,mb:-,ex:-},:\|\|\|,:--
setlocal commentstring=--\ %s

" Add ? to iskeyword for Idris2's type-level operators like '?', allowing them to be treated as part of words
" The ?, incidentally, differs from _ in that _ will be bound as an implicit argument if unresolved after checking the type of test, but ? will be left as a hole to be resolved later. Otherwise, they can be used interchangeably.
" Example code `test : Vect ? Int`
setlocal iskeyword+=?

setlocal wildignore+=*.ibc

let b:undo_ftplugin = "
      \ setlocal shiftwidth< tabstop< expandtab<
      \ comments< commentstring< iskeyword< wildignore<
      \"

let b:did_ftplugin = 1
