" Vim filetype plugin
" Language:	MOM (Macros for GNU Troff)
" Maintainer:	Aman Verma
" Homepage:	https://github.com/averms/vim-nroff-ftplugin

if exists("b:did_ftplugin")
  finish
endif
let b:did_ftplugin = 1

runtime! ftplugin/nroff.vim

if executable('groff')
  let b:groff_compiler_args = '-mom'
  compiler groff
  let b:undo_ftplugin .= '| compiler make | unlet b:groff_compiler_args'
endif

