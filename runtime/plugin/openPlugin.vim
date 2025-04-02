vim9script

# Vim runtime support library
#
# Maintainer:   The Vim Project <https://github.com/vim/vim>
# Last Change:  2025 Apr 02

if exists("g:loaded_openPlugin") || &cp
  finish
endif
g:loaded_openPlugin = 1

import autoload 'dist/vim9.vim'

command -complete=shellcmd -nargs=1 Launch vim9.Launch(trim(<q-args>))

# technically, -nargs=1 is correct, but this throws E480: No match 
# when the argument contains a wildchar on Windows
command -complete=file -nargs=* Open vim9.Open(trim(<q-args>))

const no_gx = get(g:, "nogx", get(g:, "netrw_nogx", false))
if !no_gx
  def GetWordUnderCursor(): string
    const url = matchstr(expand("<cWORD>"), '\%(\%(http\|ftp\|irc\)s\?\|file\)://\S\{-}\ze[^A-Za-z0-9/]*$')
    if !empty(url)
      return url
    endif

    const user_var = get(g:, 'gx_word', get(g:, 'netrw_gx', '<cfile>'))
    return expand(user_var)
  enddef

  if maparg('gx', 'n') == ""
    nnoremap <unique> gx <scriptcmd>vim9.Open(GetWordUnderCursor())<CR>
  endif
  if maparg('gx', 'x') == ""
    xnoremap <unique> gx <scriptcmd>vim9.Open(getregion(getpos('v'), getpos('.'), { type: mode() })->join())<CR>
  endif
endif

# vim: ts=8 sts=2 sw=2 et
