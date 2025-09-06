vim9script

# Vim compiler file.
# Compiler:    Hare
# Maintainer:  Amelia Clarke <selene@perilune.dev>
# Last Change: 2025 Sep 06
# Upstream:    https://git.sr.ht/~sircmpwn/hare.vim

if exists('g:current_compiler')
  finish
endif

if filereadable('Makefile') || filereadable('makefile')
  CompilerSet makeprg=make
else
  const makeprg = 'hare build '
    .. get(b:, 'hare_makeprg_params', get(g:, 'hare_makeprg_params', '-q'))
  execute 'CompilerSet makeprg=' .. escape(makeprg, ' "\|')
endif

CompilerSet errorformat=
  \%o:%l:%v:\ syntax\ error:\ %m,
  \%o:%l:%v:\ error:\ %m,
  \Error:\ %m,
  \%-G%.%#

augroup HareQuickFix
  autocmd!
  autocmd QuickFixCmdPost make hare#QuickFixPaths()
  autocmd QuickFixCmdPost lmake hare#QuickFixPaths()
augroup END

g:current_compiler = 'hare'

# vim: et sts=2 sw=2 ts=8 tw=80
