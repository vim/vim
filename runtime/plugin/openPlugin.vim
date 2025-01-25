vim9script

# Vim runtime support library
#
# Maintainer:   The Vim Project <https://github.com/vim/vim>
# Last Change:  2025 Jan 24

import autoload 'dist/vim9.vim'

command -complete=shellcmd -nargs=1 Launch vim9.Launch(trim(<q-args>))
command -complete=file -nargs=1 Open vim9.Open(trim(<q-args>))

const no_gx = get(g:, "nogx", get(g:, "netrw_nogx", false))
if !no_gx
  if maparg('gx', 'n') == ""
    const file = get(g:, 'netrw_gx', '<cfile>')
    nnoremap <unique> gx <scriptcmd>vim9.Open(expand(file))<CR>
  endif
  if maparg('gx', 'x') == ""
    xnoremap <unique> gx <scriptcmd>vim9.Open(getregion(getpos('v'), getpos('.'), { type: mode() })->join())<CR>
  endif
endif

# vim: ts=8 sts=2 sw=2 et
