" Vim filetype plugin file
" Language:	SQL (Common for Oracle, Microsoft SQL Server, Sybase)
" Version:	0.02
" Maintainer:	David Fishburn <fishburn@ianywhere.com>
" Last Change:	Tue May 27 2003 09:33:31

" This file should only contain values that are common to all SQL languages
" Oracle, Microsoft SQL Server, Sybase ASA/ASE, MySQL, and so on
" If additional features are required create:
" vimfiles/after/ftplugin/sql.vim
" to override and add any of your own settings

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Don't load another plugin for this buffer
let b:did_ftplugin = 1

" Define patterns for the matchit macro
if !exists("b:match_words")
    " SQL is generally case insensitive
    let b:match_ignorecase = 1
    let b:match_words =
		\ '\<begin\>:\<end\>\(;\)\?$,'.
		\ '\<if\>:\<elsif\>:\<elseif\>:\<else\>:'.
		\ '\%(\<end\s\+\)\@<!' . '\<if\>:\<end\s\+if\>,'.
		\ '\<loop\>:\<break\>:\<continue\>:'.
		\ '\%(\<end\s\+\)\@<!' . '\<loop\>:\<end\s\+loop\>,'.
		\ '\<for\>:\<break\>:\<continue\>:'.
		\ '\%(\<end\s\+\)\@<!' . '\<for\>:\<end\s\+for\>,'.
		\ '\<case\>:\<when\>:\<default\>:'.
		\ '\%(\<end\s\+\)\@<!' . '\<case\>:\<end\s\+case\>'
endif
