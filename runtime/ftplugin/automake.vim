" Vim filetype plugin file
" Language:         Automake
" Maintainer:       Nikolai Weibull <now@bitwi.se>
" Latest Revision:  2006-04-19

if exists("b:did_ftplugin")
  finish
endif

runtime! ftplugin/make.vim ftplugin/make_*.vim ftplugin/make/*.vim
