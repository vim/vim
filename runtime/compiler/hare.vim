vim9script

# Vim compiler file.
# Compiler:    Hare
# Maintainer:  Amelia Clarke <selene@perilune.dev>
# Last Change: 2026 Jan 24
# Upstream:    https://git.sr.ht/~sircmpwn/hare.vim

if exists('g:current_compiler')
  finish
endif
g:current_compiler = 'hare'

if filereadable('Makefile') || filereadable('makefile')
  CompilerSet makeprg=make
else
  const makeprg = 'hare build ' .. get(g:, 'hare_makeprg_params', '-q')
  execute 'CompilerSet makeprg=' .. escape(makeprg, ' "\|')
endif

CompilerSet errorformat=
  \%E%o:%l:%v:\ error:\ %m,
  \%E%o:%l:%v:\ syntax\ error:\ %m,
  \%E%o:%l:%v:\ %\\%%(unexpected\ name\ %\\)%\\@=%m,
  \%C,%C\ %.%#,%C%l\ %.%#,
  \%trror:\ %o:\ %\\%%(%\\h%\\w%\\+%\\%%(::%\\h%\\w%\\+%\\)%#:\ %\\)%\\@=%m,
  \%trror:\ %m,
  \%+EAbort:\ %m%>,
  \%C%.%#,
  \%-G%.%#

augroup HareQuickFix
  autocmd!
  autocmd QuickFixCmdPost make hare#QuickFixPaths()
  autocmd QuickFixCmdPost lmake hare#QuickFixPaths()
augroup END

# vim: et sts=2 sw=2 ts=8 tw=80
