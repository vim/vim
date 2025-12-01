vim9script

# Open Vim help on {subject} in the current window (rather than a new split)
#
# Maintainer:   The Vim Project <https://github.com/vim/vim>
# Last change:  2025 Dec 02

# Exit when the helpcurwin plugin is loaded already
if exists('g:loaded_helpcurwin')
  finish
endif
g:loaded_helpcurwin = true

import autoload 'helpcurwin.vim'

command -bar -nargs=? -complete=help HelpCurwin helpcurwin.Open(<q-args>)

nnoremap <Plug>HelpCurwin; <ScriptCmd>helpcurwin.Open(expand('<cWORD>'))<CR>

# vim: ts=8 sts=2 sw=2 et
