" Vim filetype plugin
" Language:	svelte
" Maintainer:	Igor Lacerda <igorlafarsi@gmail.com>
" Last Change:	2024 Jun 09

if exists('b:did_ftplugin')
  finish
endif
let b:did_ftplugin = 1

setl commentstring=<!--\ %s\ -->

if exists("loaded_matchit") && !exists("b:match_words")
  let b:match_ignorecase = 1
  let b:match_words = '<!--:-->,' ..
	\	      '<:>,' ..
	\	      '<\@<=[ou]l\>[^>]*\%(>\|$\):<\@<=li\>:<\@<=/[ou]l>,' ..
	\	      '<\@<=dl\>[^>]*\%(>\|$\):<\@<=d[td]\>:<\@<=/dl>,' ..
	\	      '<\@<=\([^/!][^ \t>]*\)[^>]*\%(>\|$\):<\@<=/\1>'
  let b:html_set_match_words = 1
endif

let b:undo_ftplugin = 'setl cms<'
