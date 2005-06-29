" Vim filetype plugin file
" Language:         Automake
" Maintainer:       Nikolai Weibull <nikolai+work.vim@bitwi.se>
" Latest Revision:  2005-06-22

if exists("b:did_ftplugin")
  finish
endif

runtime! ftplugin/make.vim ftplugin/make_*.vim ftplugin/make/*.vim
