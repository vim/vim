" Vim filetype plugin file
" Language:	Objective C
" Maintainer: The Vim Project <https://github.com/vim/vim>
" Former Maintainer:	Bram Moolenaar <Bram@vim.org>
" Last Change:	2023 Aug 10

" Only do this when not done yet for this buffer
if exists("b:did_ftplugin")
  finish
endif

" Behaves just like C
runtime! ftplugin/c.vim ftplugin/c_*.vim ftplugin/c/*.vim
