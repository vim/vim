vim9script

# Open Vim help on {subject} in the current window (rather than a new split)
#
# Maintainer:   The Vim Project <https://github.com/vim/vim>
# Last change:  2026 Jan 29

import autoload '../autoload/helpcurwin.vim'

command -bar -nargs=? -complete=help HelpCurwin helpcurwin.Open(<q-args>)

nnoremap <Plug>HelpCurwin; <ScriptCmd>helpcurwin.Open(expand('<cWORD>'))<CR>

# vim: ts=8 sts=2 sw=2 et
